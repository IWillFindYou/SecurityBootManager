#include "Global.h"
#include "Protocol.h"
#include "Packet.h"
#include "Device.h"
#include "packet/PingDevicePacket.h"

int PingDevicePacket::parser(char* buff, int size)
{
    return 0;
}

char* PingDevicePacket::encode(int* size)
{
    int ping = 0xFFFF;

    int protocol = PING_DEVICE;
    // 전체 패킷 길이
    int bufflen = 8 + sizeof(ping);
    char *buff = new char[100];
    memcpy(buff + 0, (char *)&protocol, 4);
    memcpy(buff + 4, (char *)&bufflen, 4);
    memcpy(buff + 8, (char *)&ping, bufflen - 8);

    *size = bufflen;

    return buff;
}

int PingDevicePacket::execute()
{
    return 0;
}

