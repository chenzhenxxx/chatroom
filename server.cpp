#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <vector>
#include <errno.h>
#include <sys/epoll.h>
#include<fcntl.h>
#include<nlohmann/json.hpp>
#define PORT 10000
#define MAXLEN 4096
using namespace std;
using json =nlohmann::json;
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
int main()
{
    struct sockaddr_in serveraddr;
    int listenfd;
    int len;
    char buf[MAXLEN];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(PORT);
    bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, 20);
    int epollfd = epoll_create(10);
    epoll_event ev;
    ev.events = EPOLLIN|EPOLLET;
    int flag=fcntl(listenfd,F_GETFL);
    flag|=O_NONBLOCK;
    fcntl(listenfd,F_SETFL,flag);
    ev.data.fd = listenfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    while (1)
    {
        epoll_event events[4096];
        int n = epoll_wait(epollfd, events, 4096, -1);
        printf("%d\n",n);
        if (n < 0)
        {
            // 被信号中断
            if (errno == EINTR)
                continue;
            // 出错,退出
            break;
        }
        else if (n == 0)
        {
            // 超时,继续
            continue;
        }
        for (int i = 0; i < n; i++)
        {    
            if (events[i].events & EPOLLIN)
            {
                if (events[i].data.fd == listenfd)
                {
                    struct sockaddr_in cliaddr;
                    socklen_t cliaddrlen = sizeof(cliaddr);
                    int cfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddrlen);
                    epoll_event evv;
                    int flag=fcntl(cfd,F_GETFL);
                    flag|=O_NONBLOCK;
                    fcntl(cfd,F_SETFL,flag);
                    evv.data.fd = cfd;
                    evv.events = EPOLLIN|EPOLLET;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, cfd, &evv);
                }

                else
                {   
                    char buf[4096];
                    memset(buf,0,sizeof(buf));
                    int tmpfd = events[i].data.fd;
                    int len = recv(tmpfd, buf, 4096, 0);
                    if (len == 0)
                    {  
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, tmpfd, NULL);
                        close(tmpfd);
                        break;
                    }
                    else
                    {   buf[strlen(buf)]='\0';
                        json j=json::parse(buf);
                        auto m = j.get<jjjson::pwd>();
                        cout<<m.name<<endl;
                        cout<<m.pwd<<endl;
                        cout<<j<<endl;
                    }
                }
            }
        }
    }
    close(listenfd);
}
