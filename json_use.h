#include <iostream>
#include<iostream>
#include <memory>
#include <mutex>
#include <ostream>
#include <random>
#include <stdexcept>
#include <stdlib.h>
#include<pthread.h>
#include <string>
#include<fcntl.h>
#include<sys/types.h>
#include <sys/socket.h>
#include<sys/epoll.h>
#include <sys/stat.h>
#include <thread>
#include <vector>
#include <stdlib.h>
#include <unistd.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include <nlohmann/json.hpp> //引入json.hpp，该文件已经放在系统默认路径：/usr/local/include/nlohmann/json.hpp
#include<leveldb/db.h>
leveldb::DB *db;
using namespace std;
// for convenience
using json = nlohmann::json;
#define PORT 10000
#define MAXLEN 4096
int cfd;

namespace jjjson
{
    class usr
    {
    public:
        int id;
        int fd;
        string friendname;
        int friendid;
        string name;
        string pwd;
        int status;
        string question;
        string answer;
        string choice;
        vector<string> box;
    };
    
    class Friend
    {   public:
        vector<string> myfri;
        vector<string> request;
        string from;
        string to;
        vector<string> buf;
        vector<int> time;

    };


    void to_json(json &j, const usr &p)
    {
        j = json{{"friendname",p.friendname},{"friendid",p.friendid},{"id",p.id}, {"fd", p.fd},{"name", p.name},{"pwd",p.pwd},{"status",p.status},{"question",p.question},{"answer",p.answer},{"choice",p.choice},{"box",p.box}};
    }
    void from_json(const json &j, usr &p)
    {
        j.at("name").get_to(p.name);
        j.at("id").get_to(p.id);
        j.at("pwd").get_to(p.pwd);
        j.at("fd").get_to(p.fd);
        j.at("status").get_to(p.status);
        j.at("question").get_to(p.question);
        j.at("answer").get_to(p.answer);
        j.at("choice").get_to(p.choice);
        j.at("box").get_to(p.box);
        j.at("friendname").get_to(p.friendname);
        j.at("friendid").get_to(p.friendid);
    }
    void to_json(json &j,const Friend &p)
    {
        j=json{{"myfri",p.myfri},{"request",p.request},{"from",p.from},{"to",p.to},{"buf",p.buf},{"time",p.time}};
    }
    void from_json(const json &j,Friend &p)
    {
        j.at("myfri").get_to(p.myfri);
        j.at("request").get_to(p.request);
        j.at("from").get_to(p.from);
        j.at("to").get_to(p.to);
        j.at("buf").get_to(p.buf);
        j.at("time").get_to(p.time);

    }
   
}