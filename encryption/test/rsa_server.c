#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
void error_handling(char *);
char * rsa_coding(int, int, char *);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	char rsa_recv_msg[BUF_SIZE];
	char *recv_msg, *send_msg, *rsa_send_msg ;
	int str_len, i;

	struct sockaddr_in serv_adr;
	struct sockaddr_in clnt_adr;
	socklen_t clnt_adr_sz;

	serv_sock=socket(PF_INET, SOCK_STREAM, 0);   
	if(serv_sock==-1)
		error_handling("socket() error");

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family=AF_INET;
	serv_adr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_adr.sin_port=htons(15243);

	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		error_handling("bind() error");

	if(listen(serv_sock, 5)==-1)
		error_handling("listen() error");

	clnt_adr_sz=sizeof(clnt_adr);

	while(1)
	{
		clnt_sock=accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
		if(clnt_sock==-1)
			error_handling("accept() error");
		else
			printf("Connected client %d \n", i+1);

		// READ
		str_len = read(clnt_sock, rsa_recv_msg, BUF_SIZE-1);
		rsa_recv_msg[str_len] = '\0';

		recv_msg = rsa_coding(143, 103, rsa_recv_msg);
		printf("Read Message : %s\n", rsa_recv_msg);
		printf("Decoding Read message : %s\n", recv_msg);
		
		// SEND
		send_msg = "SERVER#RECEIVE#SUCESS\0";
		rsa_send_msg = rsa_coding(143, 7, send_msg);
		printf("Send Message : %s\n",  send_msg);
		printf("Send Incoding Message : %s\n", rsa_send_msg);
		write(clnt_sock, rsa_send_msg, strlen(rsa_send_msg));

		close(clnt_sock);
	}


	close(serv_sock);
	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
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
