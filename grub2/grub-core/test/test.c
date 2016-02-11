#include <grub/DeviceType.h>
#include <grub/Protocol.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/extcmd.h>
#include <grub/i18n.h>
#include <grub/priority_queue.h>

#include <grub/net.h>
#include <grub/net/arp.h>
#include <grub/net/tcp.h>
#include <grub/net/ip.h>
#include <grub/net/ethernet.h>
#include <grub/net/netbuff.h>
#include <grub/term.h>
#include <grub/time.h>
#include <grub/file.h>
#include <grub/i18n.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/dl.h>
#include <grub/command.h>
#include <grub/env.h>
#include <grub/loader.h>
#include <grub/bufio.h>
#include <grub/kernel.h>

GRUB_MOD_LICENSE ("GPLv3+");

struct grub_net_tcp_socket
{
  struct grub_net_tcp_socket *next;
  struct grub_net_tcp_socket **prev;

  int established;
  int i_closed;
  int they_closed;
  int in_port;
  int out_port;
  int errors;
  int they_reseted;
  int i_reseted;
  int i_stall;
  grub_uint32_t my_start_seq;
  grub_uint32_t my_cur_seq;
  grub_uint32_t their_start_seq;
  grub_uint32_t their_cur_seq;
  grub_uint16_t my_window;
  struct unacked *unack_first;
  struct unacked *unack_last;
  grub_err_t (*recv_hook) (grub_net_tcp_socket_t sock, struct grub_net_buff *nb, void *recv);
  void (*error_hook) (grub_net_tcp_socket_t sock, void *recv);
  void (*fin_hook) (grub_net_tcp_socket_t sock, void *recv);
  void *hook_data;
  grub_net_network_level_address_t out_nla;
  grub_net_link_level_address_t ll_target_addr;
  struct grub_net_network_level_interface *inf;
  grub_net_packets_t packs;
  grub_priority_queue_t pq;
};

/* Load the normal mode module and execute the normal mode if possible.  */
static void
grub_load_normal_mode (void)
{
  /* Load the module.  */
  grub_dl_load ("normal");

  /* Print errors if any.  */
  grub_print_error ();
  grub_errno = 0;

  grub_command_execute ("normal", 0, 0);
}

static grub_err_t
grub_send_protocol_packet (grub_net_tcp_socket_t sock, int protocol, char* buf, int bufSize)
{
  static grub_err_t err;
  if (!sock) {
      return err;
  }

  char packBuff[512];
  int packSize = 8 + bufSize;
  grub_memcpy(packBuff + 0, &protocol, 4);
  grub_memcpy(packBuff + 4, &packSize, 4);
  grub_memcpy(packBuff + 8, buf, bufSize);

  struct grub_net_buff *nb = grub_netbuff_alloc(GRUB_NET_TCP_RESERVE_SIZE + packSize);
  if (nb) {
    grub_netbuff_reserve(nb, GRUB_NET_TCP_RESERVE_SIZE);
    grub_uint8_t *ptr = nb->tail;
    err = grub_netbuff_put(nb, packSize);
    if (err)
    {
      grub_netbuff_free(nb);
      //grub_net_tcp_close(sock, GRUB_NET_TCP_ABORT);
      return err;
    }
    grub_memcpy(ptr, packBuff, packSize);
  }
  err = grub_net_send_tcp_packet(sock, nb, 1);
  grub_printf ("grub_send_protocol_packet\n");

  return err;
}

/* Wait GRUB Network Boot */
static void
grub_network_boot_wait (void)
{
  grub_cls ();

  grub_printf ("Welcome to GRUB Network!\n");
}

static grub_err_t
hello_tcp_receive (grub_net_tcp_socket_t sock __attribute__ ((unused)), struct grub_net_buff *nb, void *f __attribute__ ((unused)))
{
  grub_err_t err;

  if (!sock)
  {
      grub_netbuff_free(nb);
      return err;
  }

  char* ptr = (char *) nb->data;
  int protocol = *((int *) ptr);

  switch (protocol) {
  case BOOTING_DEVICE:
    // 부팅 처리
    grub_net_tcp_close(sock, GRUB_NET_TCP_ABORT);
    grub_load_normal_mode();
    break;
  case SHUTDOWN_DEVICE:
    // 종료 처리
    grub_net_tcp_close(sock, GRUB_NET_TCP_ABORT);
    grub_command_execute("exit", 0, 0);
    break;
  }

  return GRUB_ERR_NONE;
}

static void
hello_tcp_err (grub_net_tcp_socket_t sock __attribute__ ((unused)), void* f __attribute__ ((unused)))
{
}

static grub_err_t
grub_cmd_hello (grub_extcmd_context_t ctxt __attribute__ ((unused)),
    int argc __attribute__ ((unused)),
    char **args __attribute__ ((unused)))
{
  grub_network_boot_wait();
 
  // 네트워크 인터페이스 정보를 얻음
  //------------------------------------------------------------------------------------------
  struct grub_net_card *card;
  char buf[GRUB_NET_MAX_STR_HWADDR_LEN];
  FOR_NET_CARDS(card)
  {
    grub_net_hwaddr_to_str (&card->default_address, buf);
    break;
  }
  //------------------------------------------------------------------------------------------

  char serve[100];
  grub_strcpy(serve, "220.89.104.165");
  int port = 10880;

  //grub_printf ("MAC Addr : %s\n", buf);
  grub_net_tcp_socket_t sock = grub_net_tcp_open(serve, port, hello_tcp_receive, hello_tcp_err, hello_tcp_err, 0);
  if (!sock) {
    grub_net_tcp_close(sock, GRUB_NET_TCP_ABORT);
    return grub_errno;
  }

  // 장치 등록 요청 패킷 전송
  // Protocol : SET_DEVICE
  //------------------------------------------------------------------------------------------
  int protocol = SET_DEVICE;
  int deviceType = PC;
  char sbuf[100];
  grub_memcpy(sbuf, &deviceType, 4);
  grub_memcpy(sbuf + 4, buf, grub_strlen(buf) + 1);
  grub_send_protocol_packet(sock, protocol, sbuf, 4 + grub_strlen(buf) + 1);
  //------------------------------------------------------------------------------------------

  grub_sleep(1);

  // 부팅 여부를 묻기 위한 패킷 전송
  // Protocol : BOOTING_REQUEST
  //------------------------------------------------------------------------------------------
  protocol = BOOTING_REQUEST;
  grub_send_protocol_packet(sock, protocol, buf, grub_strlen(buf) + 1);
  //------------------------------------------------------------------------------------------

  struct grub_net_buff *nnb = grub_netbuff_alloc(GRUB_NET_TCP_RESERVE_SIZE + 512);
  grub_netbuff_reserve(nnb, GRUB_NET_TCP_RESERVE_SIZE);
  grub_netbuff_put(nnb, 512);

  grub_net_recv_tcp_packet(nnb, sock->inf, &(sock->out_nla));

  grub_netbuff_free(nnb);
  grub_net_tcp_close(sock, GRUB_NET_TCP_ABORT);
  grub_load_normal_mode();

  return 0;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(hello)
{
  cmd = grub_register_extcmd ("test", grub_cmd_hello, 0, 0, N_("Extension Network Test Command!"), 0);
}

GRUB_MOD_FINI(hello)
{
  grub_unregister_extcmd (cmd);
}
