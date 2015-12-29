#ifndef PHONE_H_
#define PHONE_H_

class Phone : public Device
{
public:
    int send(Packet& p);
};

#endif

