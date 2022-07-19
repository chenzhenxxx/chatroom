#include "json_use.h"
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <random>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>
#define PORT 10000
#define MAXLEN 4096
using namespace std;

void Sign(jjjson::usr user)
{
    string value;
    json j = user;
    char f[1];
    auto status = db->Get(leveldb::ReadOptions(), user.name, &value);

    // cout<<"sec="<<user.fd<<endl;
    if (status.ok())
    {
        cout << "已经注册！" << endl;
        f[0] = '0';
    }
    else
    {
        //cout << j << endl;
        db->Put(leveldb::WriteOptions(), user.name, j.dump()); //初始化个人表
        jjjson::Friend fri;
        json m = fri;
        fri.from = "";
        fri.to = "";
        string s = "friend";
        s += user.name;
        db->Put(leveldb::WriteOptions(), s, m.dump()); //初始化朋友表
        f[0] = '1';
    }

    write(user.fd, f, 1);
}
void Login(jjjson::usr user)
{
    string value;
    char f[1];
    auto status = db->Get(leveldb::ReadOptions(), user.name, &value);
    if (status.ok())
    {
        auto j = json::parse(value);
        auto tmp = j.get<jjjson::usr>();
        if (user.pwd == tmp.pwd) //成功
        {
            f[0] = '1';
            user.status = 1;
            json tmp = user;
            cout << tmp << endl;
            db->Delete(leveldb::WriteOptions(), user.name);
            db->Put(leveldb::WriteOptions(), user.name, tmp.dump());
        }
        else
            f[0] = '2'; //密码错误
    }
    else
    {
        f[0] = '0'; //用户不存在
    }
    write(user.fd, f, 1);
}

void Settings(jjjson::usr user)
{  
    json j = user;
    char f[1];
    string s = j.dump();
    cout << j << endl;
    db->Delete(leveldb::WriteOptions(), user.name);
    auto status = db->Put(leveldb::WriteOptions(), user.name, s);
    if (status.ok())
    {
        f[0] = '1';
    }
    else
        f[0] = '0';
    send(user.fd, f, 1, 0);
}

void Offline(jjjson::usr user)
{
    char f[1];
    json j = user;
    string s = j.dump();
    db->Delete(leveldb::WriteOptions(), user.name);
    auto status = db->Put(leveldb::WriteOptions(), user.name, s);
    if (status.ok())
    {
        f[0] = '1';
    }
    else
        f[0] = '0';
    send(user.fd, f, 1, 0);
}

void Add_friend(jjjson::usr user)
{
    string value;
    string s = "friend";
    s += user.friendname;
    auto status = db->Get(leveldb::ReadOptions(), s, &value);
    auto j = json::parse(value);
    auto fri = j.get<jjjson::Friend>();
    fri.request.push_back(user.name);
    j = fri;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
}

void Deal_friendreq(jjjson::usr user)
{
    if (user.choice == "agree_friend")
    {
        string value;
        string s = "friend";
        s += user.name;
        auto status = db->Get(leveldb::ReadOptions(), s, &value);
        auto j = json::parse(value);
        auto fri = j.get<jjjson::Friend>();
        for (auto iter = fri.request.begin(); iter != fri.request.end(); iter++)  //删申请
        {
            if (*iter == user.friendname)
            {
                fri.request.erase(iter);
                break;
            }
        }
         
        db->Get(leveldb::ReadOptions(), user.friendname, &value);  //加到好友列表
        j = json::parse(value);
        auto he = j.get<jjjson::usr>();
        fri.myfri.push_back(user.friendname);
        j=fri;
        string tmp=j.dump();
        db->Delete(leveldb::WriteOptions(),s);
        db->Put(leveldb::WriteOptions(), s,tmp);

       
        s="friend";
        s+=user.friendname;
        db->Get(leveldb::ReadOptions(), s, &tmp);  //对方的添加
        j=json::parse(tmp);
        auto fri1=j.get<jjjson::Friend>();
        fri1.myfri.push_back(user.name);
        j=fri1;
        tmp=j.dump();
        db->Delete(leveldb::WriteOptions(),s);
        db->Put(leveldb::WriteOptions(), s,tmp);

     }
     else if(user.choice=="reject_friend")
     {
         string value;
        string s = "friend";
        s += user.name;
        auto status = db->Get(leveldb::ReadOptions(), s, &value);
        auto j = json::parse(value);
        auto fri = j.get<jjjson::Friend>();
        for (auto iter = fri.request.begin(); iter != fri.request.end(); iter++)  //删申请
        {
            if (*iter == user.friendname)
            {
                fri.request.erase(iter);
                break;
            }
        }
        j=fri;
        string tmp=j.dump();
        db->Delete(leveldb::WriteOptions(),s);
        db->Put(leveldb::WriteOptions(), s,tmp);

     }
}

void *task(void *arg)
{
    pthread_detach(pthread_self());
    jjjson::usr user = *(jjjson::usr *)arg;
    jjjson::usr tmp = user;
    if (tmp.choice.compare("sign") == 0)
    {
        Sign(user);
    }
    else if (tmp.choice.compare("login") == 0)
    {
        Login(user);
    }
    else if (tmp.choice.compare("settings") == 0)
    {
        Settings(user);
    }
    else if (tmp.choice.compare("offline") == 0)
    {
        Offline(user);
    }
    else if (tmp.choice.compare("check_friend") == 0)
    {
        string value;
        char f[1];
        auto status = db->Get(leveldb::ReadOptions(), user.friendname, &value);
        if (!status.ok())
        {
            f[0] = '0';
        }
        else
        {
            f[0] = '1';
        }
        send(user.fd, f, 1, 0);
        if (f[0] == '1')
        {
            Add_friend(user);
        }
    }
    else if (tmp.choice.compare("friend_req") == 0)
    {
        string value;
        string s = "friend";
        s += user.name;
        auto status = db->Get(leveldb::ReadOptions(), s, &value);
            cout<<"look:"<<value.c_str()<<endl;
            send(user.fd, value.c_str(), value.size(), 0);
        
    }
    else if (tmp.choice.compare("agree_friend") == 0 || tmp.choice.compare("reject_friend") == 0)
    {
        Deal_friendreq(user);
    }
    else if(tmp.choice.compare("look_friend")==0)
    {
        string value;
        string s = "friend";
        s += user.name;
        auto status = db->Get(leveldb::ReadOptions(), s, &value);
        send(user.fd, value.c_str(), value.size(), 0);

    }
}
