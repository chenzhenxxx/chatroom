#include"ser_task.cpp"
int main()
{
    struct sockaddr_in serveraddr;
    int listenfd;
    int len;
    char buf[MAXLEN];
    pthread_t tid;
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
     leveldb::Options options;
     options.create_if_missing = true;
    
     leveldb::Status status = leveldb::DB::Open(options, "chatroom", &db);
     assert(status.ok());



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
                {   char buif[4096];
                    int tmpfd = events[i].data.fd;
                    int len = recv(tmpfd, buf,4096, 0);
                    string s(buf);
                    auto tmp=json::parse(s);
                    jjjson::usr  u =tmp.get<jjjson::usr>();
                    jjjson::usr *user =&u;


                    if (len == 0)
                    {  
                        epoll_ctl(epollfd, EPOLL_CTL_DEL, tmpfd, NULL);
                        close(tmpfd);
                        break;
                    }
                    else
                    {   
                        pthread_create(&tid,NULL,task,(void *)user);
                    }
                }
            }
        }
    }
    delete db;
    close(listenfd);
}
