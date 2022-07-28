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
string last_choice;
string last_path;
int last_fd;
long long last_len;
void Disband_group(jjjson::usr user);
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

        s = "group";
        s += user.name;
        jjjson::myGroup g;
        g.mygroup.clear();
        g.status.clear();
        m = g;
        db->Put(leveldb::WriteOptions(), s, m.dump());
        db->Get(leveldb::ReadOptions(), s, &value);
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
        // if(tmp.status==1)
        //{
        //   f[0]='6';
        //  write(user.fd, f, 1);
        //  return;

        //}
        if (user.pwd == tmp.pwd) //成功
        {
            f[0] = '1';
            user.buf.clear();
            tmp.status = 1;
            tmp.fd = tmpfd;
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

void Find_pwd(jjjson::usr user)
{
    char buf[4096];
    string value;
    db->Get(leveldb::ReadOptions(), user.name, &value);
    json i = json::parse(value);
    auto x = i.get<jjjson::usr>();

    send(user.fd, value.c_str(), value.size(), 0);
    // sleep(1);
    // recv(user.fd,buf,4096,0);
    // string t(buf);
    // cout<<"thisd"<<buf<<endl;
    // json j=json::parse(t);
    // auto y=j.get<jjjson::usr>();
    // if(x.answer==y.answer)
    //{
    //    user.pwd=x.pwd;
    // }
    // else
    // {
    // user.pwd="";
    //}
    // j=user;
    // t=j.dump();
    // send(user.fd,t.c_str(),t.size(),0);
}
void True_pwd(jjjson::usr user)
{
    json j;
    string t;
    char buf[4096];
    string value;
    db->Get(leveldb::ReadOptions(), user.name, &value);
    json i = json::parse(value);
    auto x = i.get<jjjson::usr>();
    if (user.answer == x.answer)
    {
        user.pwd = x.pwd;
    }
    else
    {
        user.pwd.clear();
    }
    j = user;
    t = j.dump();
    send(user.fd, t.c_str(), t.size(), 0);
}

void Logout(jjjson::usr user)
{
    string value;
    string s = "friend";
    s += user.name;
    db->Get(leveldb::ReadOptions(), s, &value);
    json j = json::parse(value);
    auto tmp = j.get<jjjson::Friend>();
    // cout<<"lala"<<j<<endl;
    for (auto it = tmp.myfri.begin(); it != tmp.myfri.end(); it++)
    {
        string v;
        string t = "friend";
        t += *it;
        db->Get(leveldb::ReadOptions(), t, &v);
        json x = json::parse(v);
        auto q = x.get<jjjson::Friend>();
        for (auto i = q.myfri.begin(); i != q.myfri.end(); it++)
        {
            if (*i == user.name)
            {
                q.myfri.erase(i); //把我从好友的聊天列表删除
                db->Delete(leveldb::WriteOptions(), t);
                json c = q;
                db->Put(leveldb::WriteOptions(), t, c.dump());
                break;
            }
        }
        string w = user.name;
        w += *it;
        db->Delete(leveldb::WriteOptions(), w); //把我和它的聊天库删了
        w = *it;
        w += user.name;
        db->Delete(leveldb::WriteOptions(), w); //把它和我的聊天库删了
    }
    s = "group";
    s += user.name;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto b = j.get<jjjson::myGroup>();
    for (auto it = b.mygroup.begin(); it != b.mygroup.end(); it++)
    {
        string vl;
        json q;
        string y = "group";
        y += *it;
        db->Get(leveldb::ReadOptions(), y, &vl);
        q = json::parse(vl);
        auto k = q.get<jjjson::Group>();
        if (k.owner == user.name) //注销的人是群主
        {
            user.group = *it;
            Disband_group(user);
            continue;
        }
        for (auto j = k.manager.begin(); j != k.manager.end(); j++)
        {
            if (*j == user.name)
            {
                k.manager.erase(j);
                break;
            }
        }
        for (auto m = k.member.begin(); m != k.member.end(); m++)
        {
            if (*m == user.name)
            {
                k.member.erase(m);
                break;
            }
        }
        q = k;
        db->Delete(leveldb::WriteOptions(), y); //改群信息表
        db->Put(leveldb::WriteOptions(), y, q.dump());

        s = "group_chat"; //删我和群的聊天表
        s += *it;
        s += user.name;
        db->Delete(leveldb::WriteOptions(), s);
    }
    s = "group";
    s += user.name;
    db->Delete(leveldb::WriteOptions(), s);
    char f[1];
    auto status = db->Delete(leveldb::WriteOptions(), user.name); //把自己删了
    string h = "friend";
    h += user.name;
    db->Delete(leveldb::WriteOptions(), h); //把自己的朋友表删除

    if (status.ok())
    {
        f[0] = '1';
    }
    else
    {
        f[0] = '0';
    }

    send(user.fd, f, 1, 0);
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
        fri.ship[user.friendname] = 0;
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
        fri1.ship[user.name] = 0;
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
    // cout<<"lala"<<user.friendname<<endl;
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
    if (tmp.history.size() > 50) // 超过50条消息就把前面的删了
    {
        tmp.history.erase(tmp.history.begin());
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
    if (t.history.size() > 50) // 超过50条消息就把前面的删了
    {
        t.history.erase(t.history.begin());
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
        string qq;
        db->Get(leveldb::ReadOptions(), user.name, &qq);
        json k = json::parse(qq);
        auto dd = k.get<jjjson::usr>();

        if (dd.mes_fri == "quit")
        {
            printf("over %d\n", user.fd);
            // char buf[20]="quit";
            // cout<<user.fd<<endl;
            // sleep(1);
            // send(user.fd,buf,20,0);
            break;
        }

        printf("000\n");
        db->Get(leveldb::ReadOptions(), s, &value);
        json j = json::parse(value);
        string t = j.dump();
        auto q = j.get<jjjson::Fri_chat>();
        auto p = q;
        string b;
        db->Get(leveldb::ReadOptions(), user.friendname, &b);
        json g = json::parse(b);
        auto x = g.get<jjjson::usr>();
        db->Get(leveldb::ReadOptions(), user.name, &b);
        g = json::parse(b);
        auto y = g.get<jjjson::usr>();
        // cout<<"ss"<<x.choice<<endl;
        // cout<<x.friendname<<endl;
        if ((q.unread.size() != 0) && x.name == y.friendname && y.choice == "chat_sb")
        {
            send(user.fd, t.c_str(), t.size(), 0);
            q.unread.clear();
            q.unread_t.clear();
            j = q;
            db->Delete(leveldb::WriteOptions(), s);
            db->Put(leveldb::WriteOptions(), s, j.dump());
        }

        // for(auto it=p.unread.begin();it!=p.unread.end();it++)
        //{
        // if(*it=="quit")
        // printf("over %d\n",user.fd);
        // char buf[20]="quit";
        // break;
        //}

        // pthread_mutex_unlock(&mutexx);
    }
    return NULL;
}

void Check_history(jjjson::usr user)
{
    string s = user.name;
    s += user.friendname;
    string value;
    db->Get(leveldb::ReadOptions(), s, &value);
    json j = json::parse(value);
    string t = j.dump();
    // cout<<"hello"<<t<<endl;
    send(user.fd, t.c_str(), t.size(), 0);
}

void Shield_fri(jjjson::usr user)
{

    string value; //自己的
    string s = "friend";
    s += user.name;
    db->Get(leveldb::ReadOptions(), s, &value);

    // cout<<"myvla"<<value<<endl;

    json j = json::parse(value);
    auto tmp = j.get<jjjson::Friend>();
    if (user.choice == "start_shield")
    {
        tmp.ship[user.friendname] = 1;
    }
    else
    {
        tmp.ship[user.friendname] = 0;
    }
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    s = "friend"; //好友的
    s += user.friendname;
    db->Get(leveldb::ReadOptions(), s, &value);

    // cout<<"otttvla"<<value<<endl;

    j = json::parse(value);
    tmp = j.get<jjjson::Friend>();
    if (user.choice == "start_shield")
    {
        tmp.ship[user.name] = 1;
    }
    else
    {
        tmp.ship[user.name] = 0;
    }
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    // db->Get(leveldb::ReadOptions(),s,&value);
    // cout<<"ans"<<value<<endl;
}

void Build_create(jjjson::usr user)
{
    char f[1];
    string value;
    string s = "group";
    s += user.group;
    cout << s << endl;
    auto status = db->Get(leveldb::ReadOptions(), s, &value);
    if (!status.ok())
    {
        f[0] = '1';
        jjjson::Group g;
        g.owner = user.name;
        g.manager.push_back(user.name);
        g.member.push_back(user.name);
        g.join_req.clear();
        json k = g;
        db->Put(leveldb::WriteOptions(), s, k.dump()); //建群信息表

        s.clear();
        s = "group";
        s += user.name;
        db->Get(leveldb::ReadOptions(), s, &value);
        cout << "firstgroup" << endl;
        k = json::parse(value);
        auto my_group = k.get<jjjson::myGroup>();
        my_group.mygroup.push_back(user.group);
        my_group.status[user.group] = 1; //群主
        k = my_group;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, k.dump()); //个人的群表

        json j;
        s = "group_chat"; //建群聊表
        s += user.group;
        s += user.name;
        jjjson::Gro_chat gc;
        j = gc;
        db->Put(leveldb::WriteOptions(), s, j.dump());
    }
    else
    {
        f[0] = '0';
    }

    send(user.fd, f, 1, 0);
}

void Join_group(jjjson::usr user)
{
    string value;
    json j;
    string s = "group";
    s += user.group;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Group>();
    tmp.join_req.push_back(user.name);
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    for (auto it = tmp.manager.begin(); it != tmp.manager.end(); it++)
    {
        db->Get(leveldb::ReadOptions(), *it, &value); //放到消息列表
        j = json::parse(value);
        auto q = j.get<jjjson::usr>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s want to  enter %s", user.name.c_str(), user.group.c_str());
        q.box.push_back(buf);
        j = q;
        db->Delete(leveldb::WriteOptions(), *it);
        db->Put(leveldb::WriteOptions(), *it, j.dump());
    }
}

void Deal_group_req(jjjson::usr user)
{

    if (user.choice == "agree_group")
    {
        string value;
        json j;
        string s = "group";
        s += user.group;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto tmp = j.get<jjjson::Group>();
        tmp.member.push_back(user.friendname);
        for (auto it = tmp.join_req.begin(); it != tmp.join_req.end(); it++)
        {
            if (*it == user.friendname)
            {
                tmp.join_req.erase(it);
                break;
            }
        }
        j = tmp;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());

        s = "group"; //建我的群表
        s += user.friendname;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        cout << "this " << j << endl;
        auto t = j.get<jjjson::myGroup>();
        t.mygroup.push_back(user.group);
        t.status[user.group] = 3;
        j = t;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());

        db->Get(leveldb::ReadOptions(), user.friendname, &value); //放到消息列表
        j = json::parse(value);
        auto q = j.get<jjjson::usr>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s accept  you to enter %s", user.name.c_str(), user.group.c_str());
        q.box.push_back(buf);
        j = q;
        db->Delete(leveldb::WriteOptions(), user.friendname);
        db->Put(leveldb::WriteOptions(), user.friendname, j.dump());

        s = "group_chat"; //建群聊表
        s += user.group;
        s += user.friendname;
        jjjson::Gro_chat gc;
        j = gc;
        db->Put(leveldb::WriteOptions(), s, j.dump());
    }

    else if (user.choice == "reject_group")
    {
        string value;
        json j;
        string s = "group";
        s += user.group;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto tmp = j.get<jjjson::Group>();
        for (auto it = tmp.join_req.begin(); it != tmp.join_req.end(); it++)
        {
            if (*it == user.name)
            {
                tmp.join_req.erase(it);
                break;
            }
        }
        j = tmp;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());

        db->Get(leveldb::ReadOptions(), user.friendname, &value); //放到消息列表
        j = json::parse(value);
        auto q = j.get<jjjson::usr>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s reject  you to enter %s", user.name.c_str(), user.group.c_str());
        q.box.push_back(buf);
        j = q;
        db->Delete(leveldb::WriteOptions(), user.friendname);
        db->Put(leveldb::WriteOptions(), user.friendname, j.dump());
    }
}

void Set_manager(jjjson::usr user)
{
    cout << "1" << endl;
    if (user.choice == "set_manager")
    {
        string s;
        string value;
        json j;
        s = "group";
        s += user.group;
        cout << "hi" << s << endl;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto tmp = j.get<jjjson::Group>();
        tmp.manager.push_back(user.friendname);
        j = tmp;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        s.clear();
        s = "group";
        s += user.friendname;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        // cout<<"佛"<<s<<endl;
        // cout<<j<<endl;
        auto t = j.get<jjjson::myGroup>();
        t.status[user.group] = 2;
        j = t;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());

        char buf[4096];
        memset(buf, 0, 4096);
        db->Get(leveldb::ReadOptions(), user.friendname, &value);
        j = json::parse(value);
        auto p = j.get<jjjson::usr>();
        sprintf(buf, "%s set you become manager!\n", user.name.c_str());
        p.box.push_back(buf);
        j = p;
        db->Delete(leveldb::WriteOptions(), user.friendname);
        db->Put(leveldb::WriteOptions(), user.friendname, j.dump());
    }
    else if (user.choice == "canel_manager")
    {
        string s;
        string value;
        json j;
        s = "group";
        s += user.group;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto tmp = j.get<jjjson::Group>();
        for (auto it = tmp.manager.begin(); it != tmp.manager.end(); it++)
        {
            if (*it == user.friendname)
            {
                tmp.manager.erase(it);
                break;
            }
        }

        j = tmp;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        value.clear();
        s.clear();
        s = "group";
        s += user.friendname;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        // cout<<"佛挡杀佛法是"<<s<<endl;
        // cout<<value<<endl;
        auto t = j.get<jjjson::myGroup>();
        t.status[user.group] = 3;
        j = t;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());

        char buf[4096];
        memset(buf, 0, 4096);
        db->Get(leveldb::ReadOptions(), user.friendname, &value);
        j = json::parse(value);
        auto p = j.get<jjjson::usr>();
        sprintf(buf, "%s canel your manager!\n", user.name.c_str());
        p.box.push_back(buf);
        j = p;
        db->Delete(leveldb::WriteOptions(), user.friendname);
        db->Put(leveldb::WriteOptions(), user.friendname, j.dump());
    }
}

void Kick_sb(jjjson::usr user)
{
    string value;
    string s;
    json j;
    s = "group";
    s += user.group;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Group>();
    for (auto it = tmp.member.begin(); it != tmp.member.end(); it++)
    {
        if (*it == user.friendname)
        {
            tmp.member.erase(it);
            break;
        }
    }
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    s = "group";
    s += user.friendname;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tq = j.get<jjjson::myGroup>();
    for (auto it = tq.mygroup.begin(); it != tq.mygroup.end(); it++)
    {
        if (*it == user.group)
        {
            tq.mygroup.erase(it);
            break;
        }
    }
    j = tq;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    char buf[4096];
    db->Get(leveldb::ReadOptions(), user.friendname, &value);
    j = json::parse(value);
    auto t = j.get<jjjson::usr>();
    sprintf(buf, "%s already kick you from group : %s", user.name.c_str(), user.group.c_str());
    t.box.push_back(buf);
    j = t;
    db->Delete(leveldb::WriteOptions(), user.friendname);
    db->Put(leveldb::WriteOptions(), user.friendname, j.dump());
}

void Disband_group(jjjson::usr user)
{
    string s = "group";
    s += user.group;
    string value;
    json j;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Group>();
    for (auto it = tmp.member.begin(); it != tmp.member.end(); it++)
    {
        string t, v;
        json k;
        string r;
        t = "group";
        t += *it;
        db->Get(leveldb::ReadOptions(), t, &v);
        k = json::parse(v);
        auto tt = k.get<jjjson::myGroup>();
        for (auto i = tt.mygroup.begin(); i != tt.mygroup.end(); i++)
        {
            if (*i == user.group)
            {
                tt.mygroup.erase(i);
                // tt.status[user.group]=0;
                break;
            }
        }
        db->Delete(leveldb::WriteOptions(), t);
        k = tt;
        db->Put(leveldb::WriteOptions(), t, k.dump());
        db->Get(leveldb::ReadOptions(), *it, &v);
        k = json::parse(v);
        auto y = k.get<jjjson::usr>();
        char buf[4096];
        sprintf(buf, "the group :%s 已经被群主%s解散", user.group.c_str(), user.name.c_str());
        y.box.push_back(buf);
        k = y;
        db->Delete(leveldb::WriteOptions(), *it);
        db->Put(leveldb::WriteOptions(), *it, k.dump());
    }

    db->Delete(leveldb::WriteOptions(), s); //删群
}

void Look_g(jjjson::usr user)
{
    char f[1];
    string value;
    string s = "group";
    s += user.group;
    cout << s << endl;
    auto status = db->Get(leveldb::ReadOptions(), s, &value);
    if (!status.ok()) //群不存在
    {
        f[0] = '1';
    }
    else
    {
        f[0] = '0';
        cout << value << endl;
        json j = json::parse(value);
        auto tmp = j.get<jjjson::Group>();
        for (auto it = tmp.join_req.begin(); it != tmp.join_req.end(); it++) //发送过请求
        {
            if (*it == user.name)
            {
                f[0] = '3';
                break;
            }
        }
        for (auto it = tmp.member.begin(); it != tmp.member.end(); it++) //已经是群成员
        {
            if (*it == user.name)
            {
                f[0] = '4';
                break;
            }
        }
        for (auto it = tmp.manager.begin(); it != tmp.manager.end(); it++) //已经是群管理
        {
            if (*it == user.name)
            {
                f[0] = '5';
                break;
            }
        }
        if (tmp.owner == user.name)
        {
            f[0] = '6';
        }
    }

    send(user.fd, f, 1, 0);
}

void Withdraw_group(jjjson::usr user)
{
    string value;
    json j;
    string s = "group";
    s += user.group;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Group>();
    if (user.name == tmp.owner) //群主要退群 (等于把群解散)
    {
        Disband_group(user);
    }

    else
    {
        for (auto it = tmp.manager.begin(); it != tmp.manager.end(); it++)
        {
            if (*it == user.name)
            {
                tmp.manager.erase(it);
                break;
            }
        }

        for (auto it = tmp.member.begin(); it != tmp.member.end(); it++)
        {
            if (*it == user.name)
            {
                tmp.member.erase(it);
                break;
            }
        }
        j = tmp;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        s = "group";
        s += user.name;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto t = j.get<jjjson::myGroup>();
        for (auto i = t.mygroup.begin(); i != t.mygroup.end(); i++)
        {
            if (*i == user.group)
            {
                t.mygroup.erase(i);
                break;
            }
        }
        j = t;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
    }
}

void Chat_group(jjjson::usr user)
{
    string v;
    json j;
    db->Get(leveldb::ReadOptions(), user.name, &v);
    j = json::parse(v);
    auto tmp = j.get<jjjson::usr>();
    tmp.choice = "chat_group";
    tmp.group = user.group;
    j = tmp;
    db->Delete(leveldb::WriteOptions(), user.name);
    db->Put(leveldb::WriteOptions(), user.name, j.dump());
    if (user.mes_fri == "")
        return;
    if (user.mes_fri == "quit")
    {
        string buf = "quit";
        db->Get(leveldb::ReadOptions(), user.name, &v);
        j = json::parse(v);
        auto tmp = j.get<jjjson::usr>();
        tmp.choice = "";
        tmp.group.clear();
        j = tmp;
        db->Delete(leveldb::WriteOptions(), user.name);
        db->Put(leveldb::WriteOptions(), user.name, j.dump());

        send(user.fd, buf.c_str(), buf.size(), 0);
        return;
    }

    string s = "group";
    string value;
    s += user.group;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto t = j.get<jjjson::Group>();
    if (t.history.size() > 30) // 超过50条消息就把前面的删了
    {
        t.history.erase(t.history.begin());
        t.history.push_back(user.mes_fri);
        t.time.erase(t.time.begin());
        t.time.push_back(user.time);
    }
    else
    {
        t.history.push_back(user.mes_fri);
        t.time.push_back(user.time);
    }
    j = t;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    for (auto it = t.member.begin(); it != t.member.end(); it++)
    {
        if (*it == user.name)
            continue;
        string r;
        string ss = "group_chat";
        ss += user.group;
        ss += *it;
        db->Get(leveldb::ReadOptions(), ss, &r);
        j = json::parse(r);
        auto u = j.get<jjjson::Gro_chat>();
        u.unread_mes.push_back(user.mes_fri);
        u.unread_t.push_back(user.time);
        j = u;
        db->Delete(leveldb::WriteOptions(), ss);
        db->Put(leveldb::WriteOptions(), ss, j.dump());

        string m;
        db->Get(leveldb::ReadOptions(), *it, &m);
        json k = json::parse(m);
        auto y = k.get<jjjson::usr>();
        if (y.choice == "chat_group" && y.group == user.group)
        {
            string r;
            string ss = "group_chat";
            ss += user.group;
            ss += *it;
            db->Get(leveldb::ReadOptions(), ss, &r);
            send(y.fd, r.c_str(), r.size(), 0);
            cout << "myfd" << y.fd << endl;
            j = json::parse(r);
            auto u = j.get<jjjson::Gro_chat>();
            u.unread_mes.clear();
            u.unread_t.clear();
            j = u;
            db->Delete(leveldb::WriteOptions(), ss);
            db->Put(leveldb::WriteOptions(), ss, j.dump());
        }
    }
}

void Offline_mes_gro(jjjson::usr user)
{
    string value;
    json j;
    string s = "group_chat";
    s += user.group;
    s += user.name;
    db->Get(leveldb::ReadOptions(), s, &value);
    send(user.fd, value.c_str(), value.size(), 0);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Gro_chat>();
    tmp.unread_mes.clear();
    tmp.unread_t.clear();
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
}

void Check_group_history(jjjson::usr user)
{
    string value;
    string s = "group";
    json j;
    s += user.group;
    db->Get(leveldb::ReadOptions(), s, &value);
    send(user.fd, value.c_str(), value.size(), 0);
}

void *R_file(void *arg)
{
    pthread_detach(pthread_self());
    char buf[4096];
    long long tmplen = 0;
    int ret;
    memset(buf, 0, 4096);
    cout << last_path << endl;
    int fd = open(last_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    while (1)
    {

        ret = recv(last_fd, buf, 4095, 0);
        // cout << buf << endl;
        if (strcmp(buf, "over") == 0)
        {
            cout << "over" << endl;
            last_choice = "";
            break;
        }

        // if(ret<4096)
        // buf[ret]='\0';
        write(fd, buf, ret);

        // memset(buf,0,4096);
    }

    close(fd);
    return NULL;
}

void Recv_file_fri(jjjson::usr user)
{
    pthread_t tid;

    last_choice = "recv_file_fri";
    last_fd = user.fd;
    last_len = user.id;

    string value;
    json j;
    string s = user.friendname;
    s += user.name;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Fri_chat>();
    tmp.file.push_back(user.filename);
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    db->Get(leveldb::ReadOptions(), user.friendname, &value);
    j = json::parse(value);
    auto t = j.get<jjjson::usr>();
    char buf[4096];
    sprintf(buf, "%s send a file : %s to you", user.name.c_str(), user.filename.c_str());
    t.box.push_back(buf);
    j = t;
    db->Delete(leveldb::WriteOptions(), user.friendname);
    db->Put(leveldb::WriteOptions(), user.friendname, j.dump());

    string path = "/home/chenzhenxxx/chatroom/" + user.filename;
    last_path = path;
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    // cout<<user.buf<<endl;
    // cout<<path<<endl;
    // write(fd,user.buf.c_str(),user.buf.size());
    pthread_create(&tid, NULL, R_file, NULL);
    close(fd);
}

void Send_file_fri(jjjson::usr user)
{
    string path = "/home/chenzhenxxx/chatroom/" + user.filename;
    int fd;
    if ((fd = open(path.c_str(), O_RDONLY)) < 0)
    {
        cout << "open error" << endl;
        return;
    }
    int ret = 0;
    char x[4096];
    memset(x, 0, 4096);
    while ((ret = read(fd, x, 4095)) > 0)
    { // user.buf.clear();
        cout << "1" << endl;
        x[ret] = '\0';
        // user.buf=x;
        cout << x << endl;
        cout << ret << endl;
        cout << strlen(x) << endl;
        // j=user;
        // s=j.dump();
        sleep(0.01);
        send(user.fd, x, ret, 0);
        memset(x, 0, 4096);
        // sleep(1);
        if (ret != 4095)
        {
            cout << "q!" << endl;
            sleep(1);
            char buf[5] = "over";
            send(user.fd, buf, sizeof(buf), 0);
            break;
        }
        // user.buf.clear();
    }
    // sleep(1);
    // char buf[5]="over";
    // send(cfd,buf,4,0);
    // sleep(1);
    string value;
    json j;
    string s = user.name;
    s += user.friendname;
    // cout<<"my"<<user.name<<endl;
    // cout<<"friend"<<user.friendname<<endl;
    // cout<<"this"<<s<<endl;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Fri_chat>();
    for (auto it = tmp.file.begin(); it != tmp.file.end(); it++)
    {
        if (*it == user.filename)
        {
            tmp.file.erase(it);
            break;
        }
    }
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
    cout << "good" << endl;

    close(fd);
}

void Recv_file_gro(jjjson::usr user)
{   last_choice = "recv_file_gro";
    last_fd = user.fd;
    last_len = user.id;
    json j;
    string value;
    string s;
    char buf[1024];
    s = "group";
    s += user.group;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Group>();
    for (auto it = tmp.member.begin(); it != tmp.member.end(); it++)
    {
        if (*it == user.name)
            continue;
        db->Get(leveldb::ReadOptions(), *it, &value);
        j = json::parse(value);
        auto t = j.get<jjjson::usr>();
        sprintf(buf, "in group :%s :%s send a file: %s to you ", user.group.c_str(), user.name.c_str(), user.filename.c_str());
        t.box.push_back(buf);
        j = t;
        db->Delete(leveldb::WriteOptions(), *it);
        db->Put(leveldb::WriteOptions(), *it, j.dump());

        s = "group_chat";
        s += user.group;
        s += *it;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto y = j.get<jjjson::Gro_chat>();
        y.filename.push_back(user.filename);
        j = y;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
    }

    string path = "/home/chenzhenxxx/chatroom/" + user.filename;
    last_path = path;
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    // cout<<user.buf<<endl;
    // cout<<path<<endl;
    // write(fd,user.buf.c_str(),user.buf.size());
    pthread_create(&tid, NULL, R_file, NULL);
    close(fd);
}

void Send_file_gro(jjjson :: usr user)
{
    string path = "/home/chenzhenxxx/chatroom/" + user.filename;
    int fd;
    if ((fd = open(path.c_str(), O_RDONLY)) < 0)
    {
        cout << "open error" << endl;
        return;
    }
    int ret = 0;
    char x[4096];
    memset(x, 0, 4096);
    while ((ret = read(fd, x, 4095)) > 0)
    { // user.buf.clear();
        cout << "1" << endl;
        x[ret] = '\0';
        // user.buf=x;
        cout << x << endl;
        cout << ret << endl;
        cout << strlen(x) << endl;
        // j=user;
        // s=j.dump();
        sleep(0.01);
        send(user.fd, x, ret, 0);
        memset(x, 0, 4096);
        // sleep(1);
        if (ret != 4095)
        {
            cout << "q!" << endl;
            sleep(1);
            char buf[5] = "over";
            send(user.fd, buf, sizeof(buf), 0);
            break;
        }
        // user.buf.clear();
    }
    // sleep(1);
    // char buf[5]="over";
    // send(cfd,buf,4,0);
    // sleep(1);
    string value;
    json j;
    string s = "group_chat";
    s +=user.group;
    s+=user.name;
    // cout<<"my"<<user.name<<endl;
    // cout<<"friend"<<user.friendname<<endl;
    // cout<<"this"<<s<<endl;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Gro_chat>();
    for (auto it = tmp.filename.begin(); it != tmp.filename.end(); it++)
    {
        if (*it == user.filename)
        {
            tmp.filename.erase(it);
            break;
        }
    }
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
    cout << "good" << endl;

    close(fd);
}


void *task(void *arg)
{
    pthread_detach(pthread_self());
    cout << last_choice << endl;
    if (last_choice == "recv_file_fri"||last_choice=="recv_file_gro")
    {
        return NULL;
    }
    char buf[10000];
    memset(buf, 0, 10000);
    int len = recv(tmpfd, buf, 9999, 0);
    buf[len] = '\0';
    cout << "tes;t" << buf << endl;
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
        cout << user.mes_fri << endl;
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
            string value;
            db->Get(leveldb::ReadOptions(), user.name, &value);
            json j = json::parse(value);
            auto t = j.get<jjjson::usr>();
            t.choice = " ";
            t.friendname = " ";
            t.mes_fri = "quit";
            j = t;
            string z = j.dump();
            db->Delete(leveldb::WriteOptions(), user.name);
            db->Put(leveldb::WriteOptions(), user.name, z);
            char buf[20] = "quit";
            send(user.fd, buf, 20, 0);
        }

        else if (tmp.choice.compare("recv_mes") == 0)
        {
            string value;
            db->Get(leveldb::ReadOptions(), user.name, &value);
            json j = json::parse(value);
            auto t = j.get<jjjson::usr>();
            t.choice = "chat_sb";
            t.friendname = user.friendname;
            t.mes_fri = "";
            j = t;
            string z = j.dump();
            db->Delete(leveldb::WriteOptions(), user.name);
            db->Put(leveldb::WriteOptions(), user.name, z);
            pthread_create(&tid, NULL, Recv_mes, (void *)(&tmp));
            // Recv_mes((void*)&user);
        }
        else if (tmp.choice.compare("check_history") == 0)
        { // printf("enter");
            Check_history(user);
        }
        else if (tmp.choice.compare("start_shield") == 0 || tmp.choice.compare("canel_shield") == 0)
        {
            Shield_fri(user);
        }
        else if (tmp.choice.compare("check_shield") == 0)
        { // cout<<"hepp"<<endl;
            string s = "friend";
            s += user.name;
            string value;
            db->Get(leveldb::ReadOptions(), s, &value);
            // cout<<"jjj"<<value<<endl;
            json j = json::parse(value);
            auto tmp = j.get<jjjson::Friend>();
            char f[1];
            if (tmp.ship[user.friendname] == 0)
            {
                f[0] = '1';
            }
            else
            {
                f[0] = '0';
            }
            send(user.fd, f, 1, 0);
        }
        else if (tmp.choice.compare("logout") == 0)
        {
            Logout(user);
        }
        else if (tmp.choice.compare("find_pwd") == 0)
        {
            Find_pwd(user);
        }
        else if (tmp.choice.compare("true_pwd") == 0)
        {
            True_pwd(user);
        }
        else if (tmp.choice.compare("build_group") == 0)
        {
            Build_create(user);
        }
        else if (tmp.choice.compare("join_group") == 0)
        {
            Join_group(user);
        }
        else if (tmp.choice.compare("group_req") == 0)
        {
            string value;
            string s = "group";
            s += user.group;
            db->Get(leveldb::ReadOptions(), s, &value);
            send(user.fd, value.c_str(), value.size(), 0);
        }
        else if (tmp.choice.compare("agree_group") == 0 || tmp.choice.compare("reject_group") == 0)
        {
            Deal_group_req(user);
        }
        else if (tmp.choice.compare("look_group") == 0)
        {
            string value;
            value.clear();
            string s = "group";
            s += user.name;
            db->Get(leveldb::ReadOptions(), s, &value);
            send(user.fd, value.c_str(), value.size(), 0);
        }
        else if (tmp.choice.compare("check_member") == 0)
        {
            string value;
            value.clear();
            string s = "group";
            s += user.group;
            db->Get(leveldb::ReadOptions(), s, &value);
            send(user.fd, value.c_str(), value.size(), 0);
        }
        else if (tmp.choice.compare("set_manager") == 0 || tmp.choice.compare("canel_manager") == 0)
        {
            Set_manager(user);
        }
        else if (tmp.choice.compare("kick_sb") == 0)
        {
            Kick_sb(user);
        }
        else if (tmp.choice.compare("look_g") == 0)
        {
            Look_g(user);
        }
        else if (tmp.choice.compare("disband_group") == 0)
        {
            Disband_group(user);
        }
        else if (tmp.choice.compare("withdraw_group") == 0)
        {
            Withdraw_group(user);
        }
        else if (tmp.choice.compare("chat_group") == 0)
        {
            Chat_group(user);
        }
        else if (tmp.choice.compare("offline_mes_gro") == 0)
        {
            Offline_mes_gro(user);
        }
        else if (tmp.choice.compare("check_group_history") == 0)
        {
            Check_group_history(user);
        }
        else if (tmp.choice.compare("recv_file_fri") == 0)
        {
            json j;
            string value;
            string s;
            char buf[1024];
            db->Get(leveldb::ReadOptions(), user.friendname, &value);
            j = json::parse(value);
            auto tmp = j.get<jjjson::usr>();
            sprintf(buf, "%s send a file: %s to you ", user.name.c_str(), user.filename.c_str());
            tmp.box.push_back(buf);
            j = tmp;
            db->Delete(leveldb::WriteOptions(), user.friendname);
            db->Put(leveldb::WriteOptions(), user.friendname, j.dump());

            Recv_file_fri(user);
        }
        else if (tmp.choice.compare("send_file_fri") == 0)
        {
            Send_file_fri(user);
        }
        else if (tmp.choice.compare("check_file") == 0)
        {
            string s;
            string value;
            json j;
            s = user.name;
            s += user.friendname;
            db->Get(leveldb::ReadOptions(), s, &value);
            send(user.fd, value.c_str(), value.size(), 0);
        }
        else if (tmp.choice.compare("recv_file_gro") == 0)
        {
            Recv_file_gro(user);
        }
     else if (tmp.choice.compare("check_file_gro") == 0)
        {
            string s="group_chat";
            string value;
            json j;
            s+=user.group;
            s += user.name;
            db->Get(leveldb::ReadOptions(), s, &value);
            send(user.fd, value.c_str(), value.size(), 0);
        }
        else if (tmp.choice.compare("send_file_gro") == 0)
        {
            Send_file_gro(user);
        }
        
    }
    return NULL;
}