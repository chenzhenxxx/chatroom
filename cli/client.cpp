
#include"cli_task.cpp"


using namespace std;


int main(int argc,char **argv)
{   
    struct sockaddr_in serveraddr;
    int len;
    char buf[MAXLEN];
    pthread_t tid;
     cfd=socket(AF_INET,SOCK_STREAM,0); 
     bzero(&serveraddr,sizeof(serveraddr));
     serveraddr.sin_family=AF_INET;
     inet_pton(AF_INET,"192.168.30.111",&serveraddr.sin_addr);
     serveraddr.sin_port=htons(PORT);
     //pthread_create(&tid,NULL,Recv,void *base)
     connect(cfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)); //将cfd的主动socket连接到由serveraddr指定的监听socket；
     cout<<"cfd"<<cfd<<endl;
     
        login_menu();     
      close(cfd);
      return 0;
     

}