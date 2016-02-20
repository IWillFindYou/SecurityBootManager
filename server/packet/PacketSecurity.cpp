#ifndef PACKETSECURITY_H
#define PACKETSECURITY_H

class PacketSecurity
{
    public:
        char * incoding(char *buff);
        char * decoding(char *buff);

    private:
        char * rsa(int key1, int key2, char *buff);
};

#endif