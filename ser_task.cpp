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
    char f[1];
    auto status=db->Get(leveldb::ReadOptions(),user.name,&value);

    //cout<<"sec="<<user.fd<<endl;
    if(status.ok())
    {   cout<<"已经注册！"<<endl;
        f[0]='0';
    }
    else
    {  
        db->Put(leveldb::WriteOptions(),user.name,j.dump());
        
       f[0]='1';
       cout<<f<<endl;
    }
   
    write(user.fd,f,1);
}
void Login(jjjson::usr user)
{
    string value;
    char f[1];
    auto status=db->Get(leveldb::ReadOptions(),user.name,&value);
    if(status.ok())
    {  auto  j=json::parse(value);
       auto tmp=j.get<jjjson::usr>();
       if(user.pwd==tmp.pwd) //成功
         {
             f[0]='1';
             user.status=1;
             json tmp=user;
             db->Put(leveldb::WriteOptions(),user.name,tmp.dump());
         }
         else
         f[0]='2'; //密码错误
    }
    else
    {
        f[0]='0';  //用户不存在
    }
    write(user.fd,f,1);

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
    else if(tmp.choice.compare("login")==0)
    {
        Login(user);
    }
}
