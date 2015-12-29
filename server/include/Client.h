#ifndef CLIENT_H_
#define CLIENT_H_

class Client : public Device
{
public:
    int send(Packet& p);
};

#endif

