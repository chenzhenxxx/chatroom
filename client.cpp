#include"json_use.h"
#include<iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <random>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include<sys/epoll.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>
using namespace std;
#define PORT 10000
#define MAXLEN 4096
int cfd;
void sign_up()
{
    jjjson::usr user;

    user.id=(int)time(NULL);
    printf("注册ID为%d\n请注意保存",user.id);
    printf("请输入用户名！\n");
    scanf("%s",user.name);
    printf("请输入密码!\n");
    scanf("%s",user.pwd);
    printf("请设置密保问题！\n");
    scanf("%s",user.question);
    printf("请设置密保答案！\n");
    scanf("%s",user.answer);
    json j;
    j=user;
    string ifo=j.dump();
    char buf[4096];
    send(cfd,ifo.c_str(),ifo.size());
    recv(cfd,buf,4096);
    if(strcmp(buf,ok)==0)
    {
      cout<<"sign up sucuess!"<<endl;
    }



}

void login_menu()
{   int select;
   printf("     ***********     star chatroom    **********  \n");
   printf("    ***********        1.login          **********  \n");
   printf("   ***********         2.sign up          **********  \n");
   printf("  ***********          3.quit               ***********  \n");
   scanf("%d",&select);
   switch(select)
   {
       //case 1: login();
       case 2:sign_up();
       //case 3: quit();
   }



}
int main(int argc,char **argv)
{
    struct sockaddr_in serveraddr;
    int len;
    char buf[MAXLEN];
    
     cfd=socket(AF_INET,SOCK_STREAM,0); 
     bzero(&serveraddr,sizeof(serveraddr));
     serveraddr.sin_family=AF_INET;
     inet_pton(AF_INET,"127.0.0.1",&serveraddr.sin_addr);
     serveraddr.sin_port=htons(PORT);

     connect(cfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)); //将cfd的主动socket连接到由serveraddr指定的监听socket；
     //while(1)
     { //printf("qsr !\n");
       //char buf[4096];
       //scanf("%s",buf);
        //jjjson::pwd p;
          //p.name="czx";
          ////p.pwd="123";
           //json x=p;
           //cout << x << endl;
           //string m=x.dump();
       //send(cfd,m.c_str(),m.size(),0);
       
     //}
     login_menu();
     close(cfd);
     }

}