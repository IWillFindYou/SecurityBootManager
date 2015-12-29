#ifndef BOOTINGREQUESTPACKET_H_
#define BOOTINGREQUESTPACKET_H_

class BootingRequestPacket : public Packet
{
private:
    string mGroupMacAddr;
public:
    int   parser(char* buff, int size);
    char* encode(int* size);
    int   execute();
};

#endif

