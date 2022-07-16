#include<netinet/in.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<fcntl.h>
#include<iostream>
#include <nlohmann/json.hpp>
using namespace std;
#define PORT 10000
#define MAXLEN 4096
using json= nlohmann::json;


namespace jjjson
{
    class stu
    {
    public:
        string name;
        int age;
        int sex;
    };
    class pwd
    {  public:
        string name;
        string pwd;
    };
    void to_json(json &j, const stu &p)
    {
        j = json{{"name", p.name}, {"sex", p.sex}, {"age", p.age}};
    }

    void from_json(const json &j, stu &p)
    {
        j.at("name").get_to(p.name);
        j.at("sex").get_to(p.sex);
        j.at("age").get_to(p.age);
    };
    void to_json(json &j, const pwd &p)
    {
        j = json{{"name", p.name}, {"pwd", p.pwd}};
    }

    void from_json(const json &j,pwd &p)
    {
        j.at("name").get_to(p.name);
        j.at("pwd").get_to(p.pwd);
    };
}






int main(int argc,char **argv)
{
    struct sockaddr_in serveraddr;
    int cfd;
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
        jjjson::pwd p;
          p.name="czx";
          p.pwd="123";
           json x=p;
           cout << x << endl;
           string m=x.dump();
       send(cfd,m.c_str(),m.size(),0);
       
     }
     close(cfd);

}