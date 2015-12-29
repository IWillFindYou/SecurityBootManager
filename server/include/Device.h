#ifndef DEVICE_H_
#define DEVICE_H_

// 장치 종류에 대한 enum 상수
enum DeviceType { UNKNOWN, PHONE, PC };

class Packet;
class Device
{
protected:
    int        mSock;
    char*      mIpAddr;
    int        mPort;
    char*      mMacAddr;

    int        mType;
public:
    Device() : mSock(0), mIpAddr(NULL), mPort(0), mMacAddr(NULL) {}
    virtual ~Device() {}

    void setSock(int sock)         { this->mSock = sock; }
    void setIpAddr(char* ipAddr)
    {
        if (this->mIpAddr == NULL)
            this->mIpAddr = new char[20];

        sprintf(this->mIpAddr, "%s", ipAddr);
    }
    void setPort(const int port)   { this->mPort = port; }
    void setMacAddr(char* macAddr)
    {
        if (this->mMacAddr == NULL)
            this->mMacAddr = new char[100];

        sprintf(this->mMacAddr, "%s", macAddr);
    }

    void  setDeviceType(int  type)  { this->mType = type; }

    int   getSock()       { return this->mSock; }
    char* getIpAddr()     { return this->mIpAddr; }
    int   getPort()       { return this->mPort; }
    char* getMacAddr()    { return this->mMacAddr; }
    int   getDeviceType() { return this->mType; }

    int   send(Packet* p);
};

#endif

