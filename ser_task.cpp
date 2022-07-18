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
#define PORT 10000
#define MAXLEN 4096
using namespace std;

void Sign(jjjson::usr user)
{   string value;
    json j=user;
    char buf[20];
    auto status=db->Get(leveldb::ReadOptions(),user.name,&value);
    if(status.ok())
    {   cout<<"已经注册！"<<endl;
        strcpy(buf,"fail");
    }
    else
    { db->Put(leveldb::WriteOptions(),user.name,j.dump());
       strcpy(buf,"ok");
    }
    buf[strlen(buf)]='\0';
    send(user.fd,buf,20,0);
}
void *task(void *arg)
{
    pthread_detach(pthread_self());
    jjjson::usr user=*(jjjson::usr *)arg;
    jjjson::usr tmp=user;
    if(tmp.choice.compare("sign")==0)
    {
        Sign(user);
    }
}
