#ifndef PACKETSECURITY_H
#define PACKETSECURITY_H

class PacketSecurity
{
    public:
        char * incode(char *);
        char * decode(char *);

    private:
        char * rsa(int, int, char *);
};

#endif