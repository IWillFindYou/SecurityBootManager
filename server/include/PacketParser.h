#ifndef PACKETPARSER_H_
#define PACKETPARSER_H_

class PacketParser
{
public:
    Packet*  decode(char* buff, int buffSize);
    Protocol decodeProtocol(char* buff);

private:
    Packet*  __getPacketObject(Protocol protocol);
};

#endif

