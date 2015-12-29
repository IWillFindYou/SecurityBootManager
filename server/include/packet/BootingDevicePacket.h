#ifndef BOOTINGDEVICEPACKET_H_
#define BOOTINGDEVICEPACKET_H_

class BootingDevicePacket : public Packet
{
private:
    string mMacAddr;
public:
    int   parser(char* buff, int size);
    char* encode(int* size);
    int   execute();
};

#endif

