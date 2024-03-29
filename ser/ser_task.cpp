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
pthread_mutex_t mutexx;
int last_fd;
long long last_len = 0;

int readn(int fd, char *buf, int size)
{
    char *p = buf; //辅助指针记录位置
    int count = size;
    while (count > 0)
    {
        int len = recv(fd, p, count, 0);
        if (len == -1)
        {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            perror("recve");
            return -1;
        }
        else if (len == 0)
        {                        //发送端端开连接
            return size - count; //返回总共读到的字节数
        }
        else
        {
            p += len;     //有效内存处继续接收数据
            count -= len; //还有多少没有接收
        }
    }
    return size;
}
//
//单纯地接受数据
int recvMsg(int cfd, char **msg)
{
    //第二个参数是一个传出参数
    // char head[4];
    int len = 0;
    readn(cfd, (char *)&len, 4); //先读取前面4个字节数
    int count = ntohl(len);      //网络字节序转换成主机字节序
    printf("要接收的数据块的大小是:%d\n", count);
    char *data = (char *)malloc(count + 1);
    int length = readn(cfd, data, count);
    if (length != count)
    {
        printf("接收数据失败\n");
        printf("%d->%d    %d\n", cfd, length, count);
        close(cfd);
        free(data);
        return -1;
    }

    data[count] = '\0';
    printf("%s\n", data);
    *msg = data;
    return length;
}

//发送指定长度size的字符串
int writen(int cfd, const char *msg, int size)
{
    const char *buf = msg;
    int current = size;
    while (current > 0)
    {
        int len = send(cfd, buf, size, 0);
        //往对应的写缓冲区里面写内容，但是缓冲区内容有限
        //返回实际发送的字节数
        if (len == -1)
        {
            close(cfd);
            return -1;
        }
        else if (len == 0)
        {
            continue; //重新发送
        }
        else
        {
            buf += len;     //指针偏移
            current -= len; //剩余的字节数量
        }
    }
    return size;
}

//发送数据
int sendMsg(int cfd, std::string Msg, int len)
{
    if (cfd < 0 || Msg.size() < 0 || len <= 0)
    {
        return -1;
    }
    //将string类型的数据首先转化成指针类型
    const char *msg = Msg.c_str();
    char *data = (char *)malloc(len + 4);
    int biglen = htonl(len);
    memcpy(data, &biglen, 4);
    // printf("%d", data[0]);
    //内存拷贝，连续的4个字节数拷贝到内存中去
    //这四个字节数是包头
    // printf("cheng gong\n");
    memcpy(data + 4, msg, len);
    //将需要发送的内容进行拷贝
    int ret = writen(cfd, data, len + 4);
    if (ret == -1)
    {
        // perror("send\n");
        close(cfd);
    }
    return ret;
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
        t = "mygroup";
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

        pthread_mutex_lock(&mutexx);
        s = "mymessage" + *it;
        db->Get(leveldb::ReadOptions(), s, &v);
        k = json::parse(v);
        auto y = k.get<jjjson::mymessage>();
        char buf[4096];
        sprintf(buf, "the group :%s 已经被群主%s解散", user.group.c_str(), user.name.c_str());
        y.mes.push_back(buf);
        k = y;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, k.dump());
        pthread_mutex_unlock(&mutexx);
    }
    s="group"+user.group;
    db->Delete(leveldb::WriteOptions(), s); //删群
}

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

        s = "mygroup";
        s += user.name;
        jjjson::myGroup g;
        g.mygroup.clear();
        g.status.clear();
        m = g;
        db->Put(leveldb::WriteOptions(), s, m.dump());
        db->Get(leveldb::ReadOptions(), s, &value);
        f[0] = '1';

        s = "mymessage";
        s += user.name;
        jjjson::mymessage mes;
        mes.mes.clear();
        j = mes;
        db->Put(leveldb::WriteOptions(), s, j.dump());
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
            user.time = 0;
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

    sendMsg(user.fd, value, value.size());
    
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
    sendMsg(user.fd, t, t.size());
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
    s = "mygroup";
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
    string x(f);
    sendMsg(user.fd, x, x.size());
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
    string x(f);
    sendMsg(user.fd, x, x.size());
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
    
    pthread_mutex_lock(&mutexx);
    string s = "mymessage" + user.name;
    db->Get(leveldb::ReadOptions(), s, &value);
    k = json::parse(value);
    auto t = k.get<jjjson::mymessage>();
    t.mes.push_back("exit");
    k = t;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, k.dump());
    pthread_mutex_unlock(&mutexx);

    // cout << "thjis" << tmp.status << endl;

    if (status.ok())
    {
        f[0] = '1';
    }
    else
        f[0] = '0';
    string x(f);
    sendMsg(user.fd, x, x.size());
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

    pthread_mutex_lock(&mutexx);
    s = "mymessage" + user.friendname;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::mymessage>();
    char buf[4096];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%s want to become your friend", user.name.c_str());
    string t(buf);
    tmp.mes.push_back(t);
    j = tmp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
     pthread_mutex_unlock(&mutexx);
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

        pthread_mutex_lock(&mutexx);
        s = "mymessage" + user.friendname;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto t = j.get<jjjson::mymessage>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s accept to become your friend", user.name.c_str());
        t.mes.push_back(buf);
        j = t;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        pthread_mutex_unlock(&mutexx);

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
        char b[4096];
        memset(b, 0, sizeof(b));
        sprintf(b, "%s reject to become your friend", user.name.c_str());
        t.box.push_back(b);
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
        string x(f);
        sendMsg(user.fd, x, x.size());
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
            string x(f);
            sendMsg(user.fd, x, x.size());
            return;
        }
    }

    for (auto iter = myfriend.request.begin(); iter != myfriend.request.end(); iter++) //发过请求
    {
        if (*iter == user.name)
        {
            f[0] = '2';
            string x(f);
            sendMsg(user.fd, x, x.size());
            return;
        }
    }
    if (f[0] != '0' && f[0] != '2' && f[0] != '3')
    {
        f[0] = '1';
        string x(f);
        sendMsg(user.fd, x, x.size());

        // Add_friend(user);
        return;
    }
}
void add_friend(jjjson::usr user)
{
    string value;
    char f[1];
    // cout<<"lala"<<user.friendname<<endl;
    auto status = db->Get(leveldb::ReadOptions(), user.friendname, &value);
    if (!status.ok()) //没有此人
    {
        f[0] = '0';
        string x(f);
        sendMsg(user.fd, x, x.size());
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
            string x(f);
            sendMsg(user.fd, x, x.size());
            return;
        }
    }

    for (auto iter = myfriend.request.begin(); iter != myfriend.request.end(); iter++) //发过请求
    {
        if (*iter == user.name)
        {
            f[0] = '2';
            string x(f);
            sendMsg(user.fd, x, x.size());
            return;
        }
    }
    if (f[0] != '0' && f[0] != '2' && f[0] != '3')
    {
        f[0] = '1';
        string x(f);
        sendMsg(user.fd, x, x.size());

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

     pthread_mutex_lock(&mutexx);
    s = "mymessage" + user.friendname;
    status = db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp3 = j.get<jjjson::mymessage>();
    char buf[4096];
    sprintf(buf, "%s delete you!", user.name.c_str());
    tmp3.mes.push_back(buf);
    j = tmp3;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
    pthread_mutex_unlock(&mutexx);
    string x = "1";
    sendMsg(user.fd, x, x.size());
}

void Chat_sb(jjjson::usr user)
{
    string value;
    string v;
    json j;
    db->Get(leveldb::ReadOptions(), user.name, &v);
    j = json::parse(v);
    
    auto m = j.get<jjjson::usr>();
    m.choice = "chat_sb";
    m.friendname = user.friendname;
    j = m;
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
        tmp.friendname.clear();
        j = tmp;
        db->Delete(leveldb::WriteOptions(), user.name);
        db->Put(leveldb::WriteOptions(), user.name, j.dump());

        sendMsg(user.fd, buf, buf.size());
        return;
    }
    string s = user.friendname;
    s += user.name;
    // printf("1\n");
    db->Get(leveldb::ReadOptions(), s, &value); //先放到对方的未读消息并存到对方的消息记录
                                                // cout << "valie:" << value << endl;
    j = json::parse(value);
    
    
    // printf("*****\n");
    auto mp = j.get<jjjson::Fri_chat>();
    // cout << "Jj" << j << endl;
    string h;
    h = user.name + " :" + user.mes_fri;
    if (mp.history.size() > 20) // 超过50条消息就把前面的删了
    {
        mp.history.erase(mp.history.begin());
        mp.history.push_back(h);
        mp.time.erase(mp.time.begin());
        mp.time.push_back(user.time);
    }
    else
    {
        mp.history.push_back(h);
        mp.time.push_back(user.time);
    }
    mp.unread.push_back(user.mes_fri);
    mp.unread_t.push_back(user.time);
    j = mp;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());

    s = user.name;
    s += user.friendname;
    cout<<"table"<<s<<endl;
    db->Get(leveldb::ReadOptions(), s, &value); //放到自己的消息记录
    j = json::parse(value);
    auto t = j.get<jjjson::Fri_chat>();
    if (t.history.size() > 20) // 超过50条消息就把前面的删了
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
    db->Get(leveldb::ReadOptions(), user.friendname, &value);
    j = json::parse(value);
    auto i = j.get<jjjson::usr>();
    if (i.friendname != user.name || i.choice != "chat_sb")
    {
        pthread_mutex_lock(&mutexx);
        char buf[200];
        sprintf(buf, "%s say to you : %s", user.name.c_str(), user.mes_fri.c_str());
        string t(buf);
        string dfg = "mymessage" + user.friendname;
        string op;
        db->Get(leveldb::ReadOptions(), dfg, &op);
        j = json::parse(op);
        cout<<"6"<<endl;
        auto hh = j.get<jjjson::mymessage>();

        hh.mes.push_back(t);
        j = hh;
        db->Delete(leveldb::WriteOptions(), dfg);
        db->Put(leveldb::WriteOptions(), dfg, j.dump());
        pthread_mutex_unlock(&mutexx);
    }
    else
    {
        string h = user.friendname;
        h += user.name;
        db->Get(leveldb::ReadOptions(), h, &value);
        j = json::parse(value);
        cout<<"7"<<endl;
        auto f = j.get<jjjson::Fri_chat>();
        sendMsg(i.fd, value, value.size());
        f.unread.clear();
        f.unread_t.clear();
        j = f;
        db->Delete(leveldb::WriteOptions(), h);
        db->Put(leveldb::WriteOptions(), h, j.dump());
    }
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
            
            // send(user.fd,buf,20,0);
            break;
        }

        // printf("000\n");
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
            sendMsg(user.fd, t, t.size());
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
    sendMsg(user.fd, t, t.size());
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
        s = "mygroup";
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

    string x(f);
    sendMsg(user.fd, x, x.size());
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
    {   pthread_mutex_lock(&mutexx);
        s = "mymessage" + *it;
        db->Get(leveldb::ReadOptions(), s, &value); //放到消息列表
        j = json::parse(value);
        auto q = j.get<jjjson::mymessage>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s want to  enter %s", user.name.c_str(), user.group.c_str());
        q.mes.push_back(buf);
        j = q;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        pthread_mutex_unlock(&mutexx);
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

        s = "mygroup"; //建我的群表
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
        
         pthread_mutex_lock(&mutexx);
        s = "mymessage" + user.friendname;
        db->Get(leveldb::ReadOptions(), s, &value); //放到消息列表
        j = json::parse(value);
        auto q = j.get<jjjson::mymessage>();
        char buf[4096];
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "%s accept  you to enter %s", user.name.c_str(), user.group.c_str());
        q.mes.push_back(buf);
        j = q;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        pthread_mutex_unlock(&mutexx);

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
        s = "mygroup";
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

         pthread_mutex_lock(&mutexx);
        s = "mymessage" + user.friendname;
        char buf[4096];
        memset(buf, 0, 4096);
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto p = j.get<jjjson::mymessage>();
        sprintf(buf, "%s set you become manager!\n", user.name.c_str());
        p.mes.push_back(buf);
        j = p;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        pthread_mutex_unlock(&mutexx);
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
        s = "mygroup";
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

         pthread_mutex_lock(&mutexx);
        s = "mymessage" + user.friendname;
        char buf[4096];
        memset(buf, 0, 4096);
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto p = j.get<jjjson::mymessage>();
        sprintf(buf, "%s canel your manager!\n", user.name.c_str());
        p.mes.push_back(buf);
        j = p;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        pthread_mutex_unlock(&mutexx);
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

    s = "mygroup";
    s += user.friendname;
    db->Get(leveldb::ReadOptions(), s, &value);
    cout << "mygroup" << value << endl;
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

    pthread_mutex_lock(&mutexx);
    s = "mymessage" + user.friendname;
    char buf[4096];
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto t = j.get<jjjson::mymessage>();
    sprintf(buf, "%s already kick you from group : %s", user.name.c_str(), user.group.c_str());
    t.mes.push_back(buf);
    j = t;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
    pthread_mutex_unlock(&mutexx);
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
    cout << "f:" << f << endl;
    string x(f);
    cout << "x:" << x << endl;
    sendMsg(user.fd, x, x.size());
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
        s = "mygroup";
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

void Offline_mes_gro(jjjson::usr user)
{
    string value;
    json j;
    string s = "group_chat";
    s += user.group;
    s += user.name;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Gro_chat>();
    if (!tmp.unread_mes.empty())
    {
        sendMsg(user.fd, value, value.size());
        tmp.unread_mes.clear();
        tmp.unread_t.clear();
        j = tmp;
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

        sendMsg(user.fd, buf, buf.size());
        return;
    }
    else if ((user.mes_fri == ""))
    {
        cout << "start" << endl;
        Offline_mes_gro(user);
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
            sendMsg(y.fd, r, r.size());
            cout << "myfd" << y.fd << endl;
            j = json::parse(r);
            auto u = j.get<jjjson::Gro_chat>();
            u.unread_mes.clear();
            u.unread_t.clear();
            j = u;
            db->Delete(leveldb::WriteOptions(), ss);
            db->Put(leveldb::WriteOptions(), ss, j.dump());
        }
        else
        {
            pthread_mutex_lock(&mutexx);
            s = "mymessage" + *it;
            db->Get(leveldb::ReadOptions(), s, &value);
            j = json::parse(value);
            auto c = j.get<jjjson::mymessage>();
            char buf[1000];
            sprintf(buf, "%s send a message from group:%s : %s", user.name.c_str(), user.group.c_str(), user.mes_fri.c_str());
            c.mes.push_back(buf);
            j = c;
            db->Delete(leveldb::WriteOptions(), s);
            db->Put(leveldb::WriteOptions(), s, j.dump());
            pthread_mutex_unlock(&mutexx);
        }
    }
}

void Offline_mes_fri(jjjson::usr user)
{
    string value;
    json j;
    string s = user.name;
    s += user.friendname;
    db->Get(leveldb::ReadOptions(), s, &value);
    sendMsg(user.fd, value, value.size());
    j = json::parse(value);
    auto tmp = j.get<jjjson::Fri_chat>();
    tmp.unread.clear();
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
    sendMsg(user.fd, value, value.size());
}

void *R_file(void *arg)
{
    pthread_detach(pthread_self());
    last_choice = "recv_file_fri";
    long long tmplen = 0;
    long long ret = 0;
    long long ret2 = 0;
    cout << last_path << endl;
    int fd = open(last_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    char buff[4096];
    memset(buff, 0, 4096);

    while (1)
    {

        ret2 = recv(last_fd, buff, 4096, 0);
        // cout<<errno<<endl;

        // if(tmplen+ret2>last_len){
        //     ret2=last_len-tmplen;
        // }
        // cout<<ret2<<endl;
        //  cout << buf << endl;
        //  if (strcmp(buf, "over") == 0)
        //  {
        //      cout << "over" << endl;
        //      last_choice = "";
        //      break;
        //  }

        // if(ret<4096)
        // buf[ret]='\0';

        ret = write(fd, buff, ret2);
        if (ret > 0)
            tmplen += ret;
        cout << tmplen << endl;
        cout << last_len << endl;
        if (tmplen >= last_len)
        {
            last_choice = "";
            break;
        }
    }

    close(fd);
    return NULL;
}

void Recv_file_fri(jjjson::usr user)
{
    pthread_t tid;
    last_fd = user.fd;
    last_len = user.id;
    string path = "/home/czx/chatroom/" + user.filename;
    last_path = path;
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
    
    char buf[200];
    pthread_mutex_lock(&mutexx);
    s = "mymessage" + user.friendname;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto ta = j.get<jjjson::mymessage>();
    sprintf(buf, "%s send a file: %s to you", user.name.c_str(), user.filename.c_str());
    string t(buf);
    ta.mes.push_back(t);
    j = ta;
    db->Delete(leveldb::WriteOptions(), s);
    db->Put(leveldb::WriteOptions(), s, j.dump());
    pthread_mutex_unlock(&mutexx);
    pthread_create(&tid, NULL, R_file, NULL);
}

void Send_file_fri(jjjson::usr user)
{
    long long ret = 0, sum = 0;
    long long retw = 0;
    string path = "/home/czx/chatroom/" + user.filename;
    int fd;
    if ((fd = open(path.c_str(), O_RDONLY)) < 0)
    {
        close(fd);
        cout << "open error" << endl;
        return;
    }
    char x[4096];
    memset(x, 0, 4096);
    struct stat st;
    lstat(path.c_str(), &st);
    while (1)
    {
        ret = read(fd, x, 4096);

        // x[ret] = '\0';

        // cout << x << endl;
        // cout << ret << endl;
        // cout << strlen(x) << endl;

        retw = send(user.fd, x, ret,0);
        if (retw > 0)
            sum += retw;
        memset(x, 0, 4096);
        if (ret > retw)
        {
            lseek(fd, sum, SEEK_SET);
        }
        if (sum >= st.st_size)
        {

            break;
        }
    }

   
    string value;
    json j;
    string s = user.name;
    s += user.friendname;
   
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
{
    last_choice = "recv_file_gro";
    last_fd = user.fd;
    last_len = user.id;
    json j;
    string value;
    string s;
    char buf[4096];
    s = "group";
    s += user.group;
    db->Get(leveldb::ReadOptions(), s, &value);
    j = json::parse(value);
    auto tmp = j.get<jjjson::Group>();
    for (auto it = tmp.member.begin(); it != tmp.member.end(); it++)
    {
        if (*it == user.name)
            continue;
        
        pthread_mutex_lock(&mutexx);
        s = "mymessage" + *it;
        db->Get(leveldb::ReadOptions(), s, &value);
        j = json::parse(value);
        auto t = j.get<jjjson::mymessage>();
        sprintf(buf, "in group :%s :%s send a file: %s to you ", user.group.c_str(), user.name.c_str(), user.filename.c_str());
        t.mes.push_back(buf);
        j = t;
        db->Delete(leveldb::WriteOptions(), s);
        db->Put(leveldb::WriteOptions(), s, j.dump());
        pthread_mutex_unlock(&mutexx);

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

    string path = "/home/czx/chatroom/" + user.filename;
    last_path = path;
    pthread_create(&tid, NULL, R_file, NULL);
}

void Send_file_gro(jjjson ::usr user)
{
    string path = "/home/czx/chatroom/" + user.filename;
    int fd;
    if ((fd = open(path.c_str(), O_RDONLY)) < 0)
    {
        close(fd);
        cout << "open error" << endl;
        return;
    }
    long long ret = 0, sum = 0, retw = 0;
    char x[4096];
    memset(x, 0, 4096);
    struct stat st;
    lstat(path.c_str(), &st);
    while (1)
    {
        ret = read(fd, x, 4096);

        // x[ret] = '\0';

        // cout << x << endl;
        // cout << ret << endl;
        // cout << strlen(x) << endl;

        retw = send(user.fd, x, ret,0);
        if (retw > 0)
            sum += retw;
        memset(x, 0, 4096);
        if (ret > retw)
        {
            lseek(fd, sum, SEEK_SET);
        }
        if (sum >= st.st_size)
        {

            break;
        }
    }

    
    string value;
    json j;
    string s = "group_chat";
    s += user.group;
    s += user.name;
    
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

void *Inform(void *arg)
{
    pthread_detach(pthread_self());
    jjjson::usr user = *(jjjson::usr *)arg;
    string s = "mymessage" + user.name;
    cout << user.fd << endl;
    cout << s << endl;

    while (1)
    {
        
        string value;
        value.clear();
        pthread_mutex_lock(&mutexx);
        auto status = db->Get(leveldb::ReadOptions(), s, &value);
        // cout<<"ha"<<value<<endl;
        json j = json::parse(value);
        auto tmp = j.get<jjjson::mymessage>();

        if (tmp.mes.size() != 0)
        {
            // cout<<tmp.mes.size()<<endl;
            auto it = tmp.mes.begin();
            sendMsg(user.fd, (*it), (*it).size());
            tmp.mes.erase(tmp.mes.begin());
            // send(user.fd, value.c_str(), value.size(), 0);
            // tmp.mes.clear();
            j = tmp;
            db->Delete(leveldb::WriteOptions(), s);
            db->Put(leveldb::WriteOptions(), s, j.dump());
            // break;
            if(*it=="exit")
            { pthread_mutex_unlock(&mutexx);
              break;
            }
        }
        pthread_mutex_unlock(&mutexx);
    }
    return NULL;
}

void *task(jjjson::usr arg)
{
    //pthread_detach(pthread_self());
    // cout << last_choice << endl;
    char buf[4096];
    memset(buf, 0, 4096);
    // int len =0;

    // while((len=recv(tmpfd, buf, 4096, 0))<=0);
    // cout<<len<<endl;
    // cout<<errno<<endl;
    // cout<<buf<<endl;
    if (errno == 11)
    {
        cout << "on" << endl;
        errno = 0;
        return NULL;
    }

    // buf[len] = '\0';
    //  cout << "tes;t" << buf << endl;
    // if (len == 0)
    //{   cout<<"oyeah!"<<endl;
    //   epoll_ctl(epollfd, EPOLL_CTL_DEL, tmpfd, NULL);
    //  close(tmpfd);
    //}
    // else
    {
        // string s(buf);
        // cout << s << endl;
        // json t = json::parse(s);
        // cout<<"1"<<endl;
        jjjson::usr user = arg;
        cout<<"user_fri"<<user.mes_fri<<endl;
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
        else if (tmp.choice.compare("add_friend") == 0)
        {
            add_friend(user);
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
            sendMsg(user.fd, value, value.size());
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
            sendMsg(user.fd, value, value.size());
           
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
                string x(f);
                sendMsg(user.fd, x, x.size());
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
            string x(f);
            sendMsg(user.fd, x, x.size());
        }
        // else if (tmp.choice.compare("inform") == 0)
        //{  pthread_t ttid;
        // pthread_create(&ttid,NULL,Inform,(void *)&user);
        // Inform(user);
        //}

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

            string zz = "quit";
            sendMsg(user.fd, zz, zz.size());
        }

        // else if (tmp.choice.compare("recv_mes") == 0)
        // {
        //     string value;
        //     db->Get(leveldb::ReadOptions(), user.name, &value);
        //     json j = json::parse(value);
        //     auto t = j.get<jjjson::usr>();
        //     t.choice = "chat_sb";
        //     t.friendname = user.friendname;
        //     t.mes_fri = "";
        //     j = t;
        //     string z = j.dump();
        //     db->Delete(leveldb::WriteOptions(), user.name);
        //     db->Put(leveldb::WriteOptions(), user.name, z);
        //     pthread_create(&tid, NULL, Recv_mes, (void *)(&tmp));
        //     // Recv_mes((void*)&user);
        // }
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
            string x(f);
            sendMsg(user.fd, x, x.size());
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
            sendMsg(user.fd, value, value.size());
        }
        else if (tmp.choice.compare("agree_group") == 0 || tmp.choice.compare("reject_group") == 0)
        {
            Deal_group_req(user);
        }
        else if (tmp.choice.compare("look_group") == 0)
        {
            string value;
            value.clear();
            string s = "mygroup";
            s += user.name;
            db->Get(leveldb::ReadOptions(), s, &value);
            sendMsg(user.fd, value, value.size());
        }
        else if (tmp.choice.compare("check_member") == 0)
        {
            string value;
            value.clear();
            string s = "group";
            s += user.group;
            db->Get(leveldb::ReadOptions(), s, &value);
            sendMsg(user.fd, value, value.size());
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
        else if (tmp.choice.compare("offline_mes_fri") == 0)
        {
            Offline_mes_fri(user);
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
            memset(buf, 0, 1024);
            s = "mymessage" + user.friendname;
            db->Get(leveldb::ReadOptions(), s, &value);
            j = json::parse(value);
            auto tmp = j.get<jjjson::mymessage>();
            sprintf(buf, "%s send a file: %s to you", user.name.c_str(), user.filename.c_str());
            string t(buf);
            tmp.mes.push_back(t);
            j = tmp;
            db->Delete(leveldb::WriteOptions(), s);
            db->Put(leveldb::WriteOptions(), s, j.dump());

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
            sendMsg(user.fd, value, value.size());
        }
        else if (tmp.choice.compare("recv_file_gro") == 0)
        {
            Recv_file_gro(user);
        }
        else if (tmp.choice.compare("check_file_gro") == 0)
        {
            string s = "group_chat";
            string value;
            json j;
            s += user.group;
            s += user.name;
            db->Get(leveldb::ReadOptions(), s, &value);
            sendMsg(user.fd, value, value.size());
        }
        else if (tmp.choice.compare("send_file_gro") == 0)
        {
            Send_file_gro(user);
        }
        else if (tmp.choice.compare("file_size") == 0)
        {
            json j;
            string s;
            string path = "/home/czx/chatroom/" + user.filename;
            struct stat st;
            lstat(path.c_str(), &st);
            jjjson ::usr k;
            k.id = st.st_size;
            j = k;
            s = j.dump();
            sendMsg(user.fd, s, s.size());
        }
    }
    return NULL;
}