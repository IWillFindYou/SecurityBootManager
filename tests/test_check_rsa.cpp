#include<stdio.h> 
#include<stdlib.h> 	
#include<string.h> 

char * rsa_coding(int, int, char *);

#define BUF_SIZE 1024

bool assert_func(){
	int n, e, d, i;
	char * en;
	char temp[BUF_SIZE]; 
	char * msg;

	n = 143;
	e = 7;
	d = 103;	

	fflush(stdin);	
	en = rsa_coding(n, e, "SecurityBootManager"); // 암호화
	
	for(i=0; en[i] != 0; i++)
		temp[i] = en[i];
	temp[i] = 0;
	
	msg = rsa_coding(n, d, en); // 복호화

	return !!(!strcmp(msg, "SecurityBootManager"));
}
char * rsa_coding(int key1, int key2, char * buff){

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
