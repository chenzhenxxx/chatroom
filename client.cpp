#include<netinet/in.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<fcntl.h>
#include<iostream>
using namespace std;
#define PORT 10000
#define MAXLEN 4096
int main(int argc,char **argv)
{
    struct sockaddr_in serveraddr;
    int cfd;
    int len;
    char buf[MAXLEN];
    
     cfd=socket(AF_INET,SOCK_STREAM,0); 
     bzero(&serveraddr,sizeof(serveraddr));
     serveraddr.sin_family=AF_INET;
     inet_pton(AF_INET,"127.0.0.1",&serveraddr.sin_addr);
     serveraddr.sin_port=htons(PORT);

     connect(cfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)); //将cfd的主动socket连接到由serveraddr指定的监听socket；
     while(1)
     { printf("qsr !\n");
       char buf[4096];
       scanf("%s",buf);
       send(cfd,buf,strlen(buf),0);
       
     }
     close(cfd);

}