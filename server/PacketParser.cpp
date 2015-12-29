#include "Global.h"
#include "Protocol.h"

#include "Packet.h"
#include "PacketParser.h"
#include "packet/SetDevicePacket.h"
#include "packet/ShutdownDevicePacket.h"
#include "packet/BootingDevicePacket.h"
#include "packet/BootingRequestPacket.h"

Packet* PacketParser::__getPacketObject(Protocol protocol)
{
    switch (protocol) {
    case SET_DEVICE:
        return new SetDevicePacket();
    case SHUTDOWN_DEVICE:
        return new ShutdownDevicePacket();
    case BOOTING_DEVICE:
        return new BootingDevicePacket();
    case BOOTING_REQUEST:
        return new BootingRequestPacket();
    }
    return NULL;
}

Packet* PacketParser::decode(char* buff, int buffSize)
{
    Protocol protocol = this->decodeProtocol(buff);
    int size = *((int *) buff + 4);

    if (protocol > START_PROTOCOL && protocol < END_PROTOCOL) {
        Packet* packet = this->__getPacketObject(protocol);
        cout << "Protocol -> Object : " << packet << endl;
        if (packet != NULL && packet->parser(buff + 8, size - 8) != -1)
            return packet;
    }

    return NULL;
}

Protocol PacketParser::decodeProtocol(char* buff)
{
    int protocol = *((int *) buff);
    cout << "Receive Protocol : " << protocol << endl;

    if (protocol > START_PROTOCOL && protocol < END_PROTOCOL)
        return (Protocol) protocol;

    return END_PROTOCOL;
}

