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
      grub_net_tcp_close(sock, GRUB_NET_TCP_ABORT);
      return err;
    }
    grub_memcpy(ptr, packBuff, packSize);
  }
  err = grub_net_send_tcp_packet(sock, nb, 1);

  return err;
}

static grub_err_t
bootcontrol_tcp_receive (grub_net_tcp_socket_t sock __attribute__ ((unused)),
    struct grub_net_buff *nb, void *f __attribute__ ((unused)))
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
    grub_net_tcp_close(sock, GRUB_NET_TCP_DISCARD);
    grub_load_normal_mode();
    break;
  case SHUTDOWN_DEVICE:
    // 종료 처리
    grub_net_tcp_close(sock, GRUB_NET_TCP_DISCARD);
    grub_command_execute("exit", 0, 0);
    break;
  }

  return GRUB_ERR_NONE;
}

static void
bootcontrol_tcp_err (grub_net_tcp_socket_t sock __attribute__ ((unused)),
    void* f __attribute__ ((unused)))
{
}

static grub_net_tcp_socket_t
bootcontrol_server_connect(const char* ipaddr, const int port)
{
  char serve[100];
  grub_strcpy(serve, ipaddr);

  grub_net_tcp_socket_t sock = grub_net_tcp_open(serve, port, 
      bootcontrol_tcp_receive, bootcontrol_tcp_err, bootcontrol_tcp_err, 0);

  if (!sock) {
    grub_net_tcp_close(sock, GRUB_NET_TCP_ABORT);
    return NULL;
  }

  return sock;
}

static grub_err_t
bootcontrol_server_close(grub_net_tcp_socket_t sock)
{
  if (!sock)
    return grub_errno;

  grub_net_tcp_close(sock, GRUB_NET_TCP_DISCARD);
  return GRUB_ERR_NONE;
}

static grub_err_t
bootcontrol_send_init(grub_net_tcp_socket_t sock, char* macaddr)
{
  int protocol = 0;
  int deviceType = PC;
  char sbuf[100];
  
  if (!sock)
    return grub_errno;

  // 장치 등록 요청 패킷 전송
  // Protocol : SET_DEVICE
  //------------------------------------------------------------------------------------------
  protocol = SET_DEVICE;
  grub_memcpy(sbuf, &deviceType, 4);
  grub_memcpy(sbuf + 4, macaddr, grub_strlen(macaddr) + 1);
  grub_send_protocol_packet(sock, protocol, sbuf, 4 + grub_strlen(macaddr) + 1);
  //------------------------------------------------------------------------------------------

  // delay 300ms
  grub_millisleep(300);

  // 부팅 여부를 묻기 위한 패킷 전송
  // Protocol : BOOTING_REQUEST
  //------------------------------------------------------------------------------------------
  protocol = BOOTING_REQUEST;
  grub_send_protocol_packet(sock, protocol, macaddr, grub_strlen(macaddr) + 1);
  //------------------------------------------------------------------------------------------

  return GRUB_ERR_NONE;
}

static grub_err_t
bootcontrol_recv_init(grub_net_tcp_socket_t sock)
{
  struct grub_net_buff *nnb = grub_netbuff_alloc(GRUB_NET_TCP_RESERVE_SIZE + 512);
  grub_netbuff_reserve(nnb, GRUB_NET_TCP_RESERVE_SIZE);
  grub_netbuff_put(nnb, 512);

  grub_net_recv_tcp_packet(nnb, sock->inf, &(sock->out_nla));
  grub_netbuff_free(nnb);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_bootcontrol (grub_extcmd_context_t ctxt __attribute__ ((unused)),
    int argc __attribute__ ((unused)),
    char **args __attribute__ ((unused)))
{
  grub_cls ();
  grub_load_normal_mode();
  return 0;
 
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

  grub_net_tcp_socket_t sock = bootcontrol_server_connect("220.89.104.165", 10880);
  if (!sock)
    return grub_errno;

  // 서버 연결후 서버로 로그 패킷 전송
  bootcontrol_send_init(sock, buf);
  // 서버로 부터 메시지 수신 루프 실행
  bootcontrol_recv_init(sock);

  return bootcontrol_server_close(sock);
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT(bootcontrol)
{
  cmd = grub_register_extcmd ("bootcontrol", grub_cmd_bootcontrol, 0, 0, N_("Boot Control Module"), 0);
}

GRUB_MOD_FINI(bootcontrol)
{
  grub_unregister_extcmd (cmd);
}
