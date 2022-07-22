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
string judge;
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
        // cout << j << endl;
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
            tmp.status = 1;
            j = tmp;
            // cout << j << endl;
            db->Delete(leveldb::WriteOptions(), user.name);
            db->Put(leveldb::WriteOptions(), user.name, j.dump());
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
    string value;
    string s = j.dump();
    // cout << j << endl;
    db->Get(leveldb::ReadOptions(), user.name, &value);
    json k = json::parse(value);
    auto tmp = k.get<jjjson::usr>();
    db->Delete(leveldb::WriteOptions(), user.name);
    tmp.name = user.name;
    tmp.answer = user.answer;
    tmp.question = user.question;
    tmp.pwd = user.pwd;
    k = tmp;
    auto status = db->Put(leveldb::WriteOptions(), user.name, k.dump());
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
    string value;
    char f[1];
    db->Get(leveldb::ReadOptions(), user.name, &value);
    json k = json::parse(value);
    auto tmp = k.get<jjjson::usr>();
    db->Delete(leveldb::WriteOptions(), user.name);
    tmp.status = 0;
    k = tmp;
    auto status = db->Put(leveldb::WriteOptions(), user.name, k.dump());

    db->Get(leveldb::ReadOptions(), user.name, &value);
    k = json::parse(value);
    tmp = k.get<jjjson::usr>();
    // cout << "thjis" << tmp.status << endl;

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

    db->Get(leveldb::ReadOptions(), user.friendname, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::usr>();
    char buf[4096];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s want to become your friend", user.name.c_str());
    string t(buf);
    tmp.box.push_back(t);
    j = tmp;
    db->Delete(leveldb::WriteOptions(), user.friendname);
    db->Put(leveldb::WriteOptions(), user.friendname, j.dump());
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
        for (auto iter = fri.request.begin(); iter != fri.request.end(); iter++) //删申请
        {
            if (*iter == user.friendname)
            {
                fri.request.erase(iter);
                break;
            }
        }

        //加到好友列表
        fri.myfri.push_back(user.friendname);
        fri.ship[user.friendname]=0;
        j = fri;
        string tmp = j.dump();
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, tmp);

        s = "friend";
        s += user.friendname;
        db->Get(leveldb::ReadOptions(), s, &tmp); //对方的添加
        j = json::parse(tmp);
        auto fri1 = j.get<jjjson::Friend>();
        fri1.myfri.push_back(user.name);
        fri1.ship[user.name]=0;
        j = fri1;
        tmp = j.dump();
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, tmp);

        db->Get(leveldb::ReadOptions(), user.friendname, &value);
        j = json::parse(value);
        auto t = j.get<jjjson::usr>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s accept to become your friend", user.name.c_str());
        t.box.push_back(buf);
        j = t;
        db->Delete(leveldb::WriteOptions(), user.friendname);
        db->Put(leveldb::WriteOptions(), user.friendname, j.dump());

        //创建两个朋友消息表
        s = user.name;
        s += user.friendname;
        jjjson::Fri_chat chat;
        json k = chat;
        db->Put(leveldb::WriteOptions(), s, k.dump());
        s = user.friendname;
        s += user.name;
        k = chat;
        db->Put(leveldb::WriteOptions(), s, k.dump());
    }
    else if (user.choice == "reject_friend")
    {
        string value;
        string s = "friend";
        s += user.name;
        auto status = db->Get(leveldb::ReadOptions(), s, &value);
        auto j = json::parse(value);
        auto fri = j.get<jjjson::Friend>();
        for (auto iter = fri.request.begin(); iter != fri.request.end(); iter++) //删申请
        {
            if (*iter == user.friendname)
            {
                fri.request.erase(iter);
                break;
            }
        }
        j = fri;
        string tmp = j.dump();
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, tmp);

        db->Get(leveldb::ReadOptions(), user.friendname, &value);
        j = json::parse(value);
        auto t = j.get<jjjson::usr>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s reject to become your friend", user.name.c_str());
        t.box.push_back(buf);
        j = t;
        db->Delete(leveldb::WriteOptions(), user.friendname);
        db->Put(leveldb::WriteOptions(), user.friendname, j.dump());
    }
}

void Check_friend(jjjson::usr user)
{
    string value;
    char f[1];
    //cout<<"lala"<<user.friendname<<endl;
    auto status = db->Get(leveldb::ReadOptions(), user.friendname, &value);
    if (!status.ok()) //没有此人
    {
        f[0] = '0';
        send(user.fd, f, 1, 0);
        return;
    }
    string s = "friend";
    s += user.friendname;
    status = db->Get(leveldb::ReadOptions(), s, &value);
    json j = json::parse(value);
    auto myfriend = j.get<jjjson::Friend>();

    for (auto iter = myfriend.myfri.begin(); iter != myfriend.myfri.end(); iter++) //已是朋友
    {
        if (*iter == user.name)
        {
            f[0] = '3';
            send(user.fd, f, 1, 0);
            return;
        }
    }

    for (auto iter = myfriend.request.begin(); iter != myfriend.request.end(); iter++) //发过请求
    {
        if (*iter == user.name)
        {
            f[0] = '2';
            send(user.fd, f, 1, 0);
            return;
        }
    }
    if (f[0] != '0' && f[0] != '2' && f[0] != '3')
    {
        f[0] = '1';
        send(user.fd, f, 1, 0);

        Add_friend(user);
        return;
    }
}

void Delete_fri(jjjson::usr user)
{
    char f[1];
    string s = "friend";
    s += user.name;
    string value;
    auto status = db->Get(leveldb::ReadOptions(), s, &value);
    json j = json::parse(value);
    auto tmp1 = j.get<jjjson::Friend>();
    for (auto it = tmp1.myfri.begin(); it != tmp1.myfri.end(); it++) //删自己的好友列表
    {
        if (*it == user.friendname)
        {
            tmp1.myfri.erase(it);
            break;
        }
    }
    j = tmp1;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    s = "friend";
    s += user.friendname;
    status = db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp2 = j.get<jjjson::Friend>();
    for (auto it = tmp2.myfri.begin(); it != tmp2.myfri.end(); it++) //删自己的好友列表
    {
        if (*it == user.name)
        {
            tmp2.myfri.erase(it);
            break;
        }
    }
    j = tmp2;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    status = db->Get(leveldb::ReadOptions(), user.friendname, &value);
    j = json::parse(value);
    auto tmp3 = j.get<jjjson::usr>();
    char buf[4096];
    sprintf(buf, "%s delete you!", user.name.c_str());
    tmp3.box.push_back(buf);
    j = tmp3;
    db->Delete(leveldb::WriteOptions(), user.friendname);
    db->Put(leveldb::WriteOptions(), user.friendname, j.dump());
    f[0] = '1';
    send(user.fd, f, 1, 0);
}

void Chat_sb(jjjson::usr user)
{
    string value;
    judge.clear();
    if (user.mes_fri == "quit")
        return;
    string s = user.friendname;
    s += user.name;
    // printf("1\n");
    db->Get(leveldb::ReadOptions(), s, &value); //先放到对方的未读消息并存到对方的消息记录
    // cout << "valie:" << value << endl;
    json j = json::parse(value);
    // printf("*****\n");
    auto tmp = j.get<jjjson::Fri_chat>();
    // cout << "Jj" << j << endl;
    string h;
    h = user.name + " :" + user.mes_fri;
    if(tmp.history.size()>50) // 超过50条消息就把前面的删了
    { tmp.history.erase(tmp.history.begin());
      tmp.history.push_back(h);
      tmp.time.erase(tmp.time.begin());
      tmp.time.push_back(user.time);
    }
    else
    {
       tmp.history.push_back(h);
       tmp.time.push_back(user.time);
    }
    // cout << "))))))))))" << endl;
    // cout << user.mes_fri << endl;
    tmp.unread.push_back(user.mes_fri);
    tmp.unread_t.push_back(user.time);
    j = tmp;
    // cout<<j<<endl;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
    // printf("2\n");
    // printf("***\n");

    s = user.name;
    s += user.friendname;
    db->Get(leveldb::ReadOptions(), s, &value); //放到自己的消息记录
    j = json::parse(value);
    auto t = j.get<jjjson::Fri_chat>();
     if(t.history.size()>50) // 超过50条消息就把前面的删了
    { t.history.erase(t.history.begin());
      t.history.push_back(h);
      t.time.erase(t.time.begin());
      t.time.push_back(user.time);
    }
    else
    {
       t.history.push_back(h);
       t.time.push_back(user.time);
    }
    j = t;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
}

void *Recv_mes(void *arg)
{
    pthread_detach(pthread_self());
    string value;
    jjjson::usr user = *(jjjson::usr *)arg;
    string s = user.name;
    s += user.friendname;
    while (1)
    {
        //printf("000\n");
        db->Get(leveldb::ReadOptions(), s, &value);
        json j = json::parse(value);
        string t = j.dump();
        auto q = j.get<jjjson::Fri_chat>();
        auto p = q;
        string b;
        db->Get(leveldb::ReadOptions(),user.friendname,&b);
        json g=json::parse(b);
        auto x=g.get<jjjson::usr>();
        db->Get(leveldb::ReadOptions(),user.name,&b);
         g=json::parse(b);
        auto y=g.get<jjjson::usr>();
        //cout<<"ss"<<x.choice<<endl;
        //cout<<x.friendname<<endl;
        if ((q.unread.size() != 0)&&x.name==y.friendname&&y.choice=="chat_sb")
        {   
            send(user.fd, t.c_str(), t.size(), 0);
            q.unread.clear();
            q.unread_t.clear();
            j = q;
            db->Delete(leveldb::WriteOptions(), s);
            db->Put(leveldb::WriteOptions(), s, j.dump());
        }
        // pthread_mutex_lock(&mutexx);
        // for(auto it=p.unread.begin();it!=p.unread.end();it++)
        //{
        // if(*it=="quit")
        // printf("over %d\n",user.fd);
        // char buf[20]="quit";
        // break;
        //}
        if (judge == "quit")
        {
            printf("over %d\n", user.fd);
            // char buf[20]="quit";
            // cout<<user.fd<<endl;
            // sleep(1);
            // send(user.fd,buf,20,0);
            judge.clear();
            break;
        }
        // pthread_mutex_unlock(&mutexx);
    }
    return NULL;
}

void Check_history(jjjson::usr user)
{   string s=user.name;
    s+=user.friendname;
    string value;
    db->Get(leveldb::ReadOptions(),s,&value);
    json j=json::parse(value);
    string t=j.dump();
    //cout<<"hello"<<t<<endl;
    send(user.fd,t.c_str(),t.size(),0);
}

void Shield_fri(jjjson::usr user)
{
   
         string value;           //自己的
        string s="friend";
        s+=user.name;
        db->Get(leveldb::ReadOptions(),s,&value);
        
        //cout<<"myvla"<<value<<endl;

        json j=json::parse(value);
        auto tmp=j.get<jjjson::Friend>();
         if(user.choice=="start_shield")
         {
        tmp.ship[user.friendname]=1;
         }
         else
         {
             tmp.ship[user.friendname]=0;
         }
        j=tmp;
        db->Delete(leveldb::WriteOptions(),s);
        db->Put(leveldb::WriteOptions(),s,j.dump());


     
         s="friend";                   //好友的
        s+=user.friendname;
        db->Get(leveldb::ReadOptions(),s,&value);
        
        //cout<<"otttvla"<<value<<endl;

         j=json::parse(value);
        tmp=j.get<jjjson::Friend>();
         if(user.choice=="start_shield")
         {
           tmp.ship[user.name]=1;
         }
         else
         {
             tmp.ship[user.name]=0;
         }
        j=tmp;
        db->Delete(leveldb::WriteOptions(),s);
        db->Put(leveldb::WriteOptions(),s,j.dump());

        db->Get(leveldb::ReadOptions(),s,&value);
        //cout<<"ans"<<value<<endl;
        

        
    
}

void *task(void *arg)
{   judge.clear();
    pthread_detach(pthread_self());
    char buf[4096];
    memset(buf, 0, 4096);
    int len = recv(tmpfd, buf, 4096, 0);
    // cout<<"test"<<buf<<endl;
    if (len == 0)
    {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, tmpfd, NULL);
        close(tmpfd);
    }
    else
    {
        string s(buf);
        json t = json::parse(s);
        jjjson::usr user = t.get<jjjson::usr>();
        user.fd = tmpfd;
        jjjson::usr tmp = user;
        // cout << user.mes_fri << endl;
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
            Check_friend(user);
        }
        else if (tmp.choice.compare("friend_req") == 0)
        {
            string value;
            string s = "friend";
            s += user.name;
            auto status = db->Get(leveldb::ReadOptions(), s, &value);
            // cout << "look:" << value.c_str() << endl;
            send(user.fd, value.c_str(), value.size(), 0);
        }
        else if (tmp.choice.compare("agree_friend") == 0 || tmp.choice.compare("reject_friend") == 0)
        {
            Deal_friendreq(user);
        }
        else if (tmp.choice.compare("look_friend") == 0)
        {
            string value;
            string s = "friend";
            s += user.name;
            auto status = db->Get(leveldb::ReadOptions(), s, &value);
            send(user.fd, value.c_str(), value.size(), 0);
            json j = json::parse(value);
            auto tmp = j.get<jjjson::Friend>();
            for (auto it = tmp.myfri.begin(); it != tmp.myfri.end(); it++)
            {
                char f[1];
                string val;
                db->Get(leveldb::ReadOptions(), *it, &val);
                j = json::parse(val);
                auto t = j.get<jjjson::usr>();
                if (t.status == 0)
                {
                    f[0] = '0';
                }
                else
                {
                    f[0] = '1';
                }
                send(user.fd, f, 1, 0);
            }
        }
        else if (tmp.choice.compare("check") == 0)
        {
            string value;
            auto status = db->Get(leveldb::ReadOptions(), user.name, &value);
            json j = json::parse(value);
            auto us = j.get<jjjson::usr>();
            char f[1];
            if (us.box.empty())
            {
                f[0] = '0';
            }
            else
            {
                f[0] = '1';
            }
            send(user.fd, f, 1, 0);
        }
        else if (tmp.choice.compare("inform") == 0)
        {
            string value;
            auto status = db->Get(leveldb::ReadOptions(), user.name, &value);
            send(user.fd, value.c_str(), value.size(), 0);
            json j = json::parse(value);
            auto tmp = j.get<jjjson::usr>();
            tmp.box.clear();
            j = tmp;
            db->Delete(leveldb::WriteOptions(), user.name);
            db->Put(leveldb::WriteOptions(), user.name, j.dump());
        }

        else if (tmp.choice.compare("delete_friend") == 0)
        {   
            Delete_fri(user);
        }
        else if (tmp.choice.compare("chat_sb") == 0)
        {   
            Chat_sb(user);
        }
        else if (tmp.choice.compare("quit_chatfri") == 0)
        {
            // pthread_join(tid,NULL);
             string value;
            db->Get(leveldb::ReadOptions(),user.name,&value);
            json j=json::parse(value);
            auto t=j.get<jjjson::usr>();
            t.choice=" ";
            t.friendname=" ";
            j=t;
            string z=j.dump();
            db->Delete(leveldb::WriteOptions(),user.name);
            db->Put(leveldb::WriteOptions(),user.name,z);
            judge = "quit";
            char buf[20] = "quit";
            send(user.fd, buf, 20, 0);
        }

        else if (tmp.choice.compare("recv_mes") == 0)
        {   string value;
            db->Get(leveldb::ReadOptions(),user.name,&value);
            json j=json::parse(value);
            auto t=j.get<jjjson::usr>();
            t.choice="chat_sb";
            t.friendname=user.friendname;
            j=t;
            string z=j.dump();
            db->Delete(leveldb::WriteOptions(),user.name);
            db->Put(leveldb::WriteOptions(),user.name,z);
            pthread_create(&tid, NULL, Recv_mes, (void *)(&tmp));
            // Recv_mes((void*)&user);
        }
        else if (tmp.choice.compare("check_history")==0)
        {   //printf("enter");
            Check_history(user);
        }
        else if (tmp.choice.compare("start_shield")==0||tmp.choice.compare("canel_shield")==0)
        {  
            Shield_fri(user);
        }
        else if (tmp.choice.compare("check_shield")==0)
        {   //cout<<"hepp"<<endl;
            string s="friend";
            s+=user.name;
            string value;
            db->Get(leveldb::ReadOptions(),s,&value);
            //cout<<"jjj"<<value<<endl;
            json j=json::parse(value);
            auto tmp=j.get<jjjson::Friend>();
            char f[1];
            if(tmp.ship[user.friendname]==0)
            {
                f[0]='1';
            }
            else
            {
                f[0]='0';
            }
            send(user.fd,f,1,0);
        }


        
    }
    return NULL;
}
