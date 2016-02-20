#include "Global.h"

char * PacketSecurity::incoding(char * buff){
    
    int n = 143, e = 7; // 추후 수정
    
    return rsa(n, e, buff);
}

char * PacketSecurity::decoding(char * buff){

    int n = 143, e = 103; // 추후 수정

    return rsa(n, e, buff);
}

char * PacketSecurity::rsa(int key1, int key2, char * buff){

    int i, j, k, temp[BUF_SIZE];
    char *result=(char*)malloc(sizeof(char) * BUF_SIZE);

    for(i=0; buff[i] != 0; i++) {
        temp[i] = buff[i]; 
        if(temp[i] < 0) temp[i] = 256 + temp[i];
    }   

    i=0; 
    while(buff[i] != 0)  
    {   
        k=1; 
        for(j=0;j<key2;j++) 
        {
            k=k*temp[i]; 
            k=k%key1; 
        }

        result[i]= (char)k; 
        i++; 
    }   
    result[i] = '\0'; 

    return result;
} 
