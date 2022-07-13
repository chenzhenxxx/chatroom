#include <algorithm>
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <bits/types/FILE.h>
#include <chrono>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <exception>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <ostream>
#include <random>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include<sys/epoll.h>
#include<iostream>
using namespace std;

int main()
{
  struct sockaddr_in svaddr;
  struct sockaddr_in claddr;
  socklen_t addrlen;
  int sfd=socket(AF_INET,SOCK_STREAM,0);
  int cfd;
  bzero(&svaddr,sizeof(svaddr));
  svaddr.sin_family=AF_INET;
  svaddr.sin_port=htons(5555);
  svaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  bind(sfd,(struct sockaddr *)&svaddr,sizeof(svaddr));
  listen(sfd,100);
  int len=0;
  int epollfd=epoll_create(10);
  if(epollfd==-1)
   {
       cout<<"epoll create error ."<<endl;
       close(sfd);
       return -1;
   }
   epoll_event ev;
   ev.events=EPOLLIN;
   if(epoll_ctl(epollfd,EPOLL_CTL_ADD,sfd,&ev)==-1)
   {

   }
   while(1)
   {
       epoll_event epoll_events[4096];
       int n=epoll_wait(epollfd,epoll_events,4096,1000);
       if(n<0)
       {
           if(errno==EINTR)
           {
               continue;
           }
           break;
       }
       else if(n==0)
       {
           continue;
       }

        for(int i=0;i<n;i++)
        {
            if(epoll_events[i].events==EPOLLIN){
                if(epoll_events->data.fd==sfd)
                {
                    struct sockaddr_in clientaddr;
                    socklen_t clientaddrlen=sizeof(clientaddr);
                    int clientfd=accept(sfd,(struct sockaddr *)&clientaddr,&clientaddrlen);
                    if(clientfd==-1)
                     {
                         break;
                     }
                    epoll_event ev;
                    ev.data.fd=clientfd;
                    ev.events=EPOLLIN;
                    if(epoll_ctl(epollfd,EPOLL_CTL_ADD,clientfd,&ev)==-1)
                    {
                        std::cout << "epoll_ctl error." << std::endl;
                        close(sfd);
                        return -1;
                    }
                }

            }

         else 
         {


             
         }
        }

   }
}