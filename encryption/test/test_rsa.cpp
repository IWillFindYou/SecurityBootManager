#include<stdio.h> 
#include<stdlib.h> 
#include<string.h> 

char * rsa_coding(int, int, char *);

#define BUF_SIZE 1024

int main(){
	int n, e, d, i, size;
	char * en;
	char input[BUF_SIZE], temp[BUF_SIZE]; 
	char * msg;

	n = 143;
	e = 7;
	d = 103;	

	printf("\nENTER MESSAGE\n"); 
	fflush(stdin);	
	scanf("%s", input); 

	
	en = rsa_coding(n, e, input); // 암호화
	printf("\nTHE ENCRYPTED MESSAGE IS\n");

	for(i=0; en[i] != 0; i++)
		temp[i] = en[i];
	temp[i] = 0;
	size = i;

	for(i=0; en[i] !=0; i++) 
		printf("%c",en[i]); 

	
	msg = rsa_coding(n, d, en); // 복호화
	printf("\nTHE DECRYPTED MESSAGE IS\n"); 
	for(i=0; msg[i] !=0; i++) 
		printf("%c", msg[i]); 
	printf("\n");

	return 0;
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
