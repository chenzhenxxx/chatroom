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
int main()
{
    struct sockaddr_in seraddr;
    int cfd;
    cfd=socket(AF_INET,SOCK_STREAM,0);
    if(cfd==-1)
    {
        perror("socket");
        exit(-1);
    }
    bzero(&seraddr,sizeof(seraddr));
    seraddr.sin_family=AF_INET;
    inet_pton(AF_INET,"127.0.0.1",&seraddr.sin_addr);
    seraddr.sin_port=htons(5555);
    if(connect(cfd,(struct sockaddr*)&seraddr,sizeof(seraddr)==-1))
    {
            perror("connect");
            exit(-1);
    }


}