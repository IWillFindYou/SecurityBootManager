#ifndef SETDEVICEPACKET_H_
#define SETDEVICEPACKET_H_

class SetDevicePacket : public Packet
{
private:
    string mMacAddr;
    int    mDeviceType;
public:
    string getMacAddr() { return this->mMacAddr; }
    int    getDeviceType() { return this->mDeviceType; }

    int    parser(char* buff, int size);
    char*  encode(int* size);
    int    execute();
};

#endif

