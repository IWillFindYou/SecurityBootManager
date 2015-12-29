#ifndef PINGDEVICEPACKET_H_
#define PINGDEVICEPACKET_H_

class PingDevicePacket : public Packet
{
public:
    int   parser(char* buff, int size);
    char* encode(int* size);
    int   execute();
};

#endif

