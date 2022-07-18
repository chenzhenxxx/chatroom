#include"json_use.h"
void sign_up()
{
    jjjson::usr user;

    //user.id=(int)time(NULL);
    //printf("注册ID为%d\n请注意保存",user.id);
    user.fd=cfd;
    user.choice="sign";
    printf("请输入用户名！\n");
    cin>>user.name;
    printf("请输入密码!\n");
    cin>>user.pwd;
    printf("请设置密保问题！\n");
    cin>>user.question;
    printf("请设置密保答案！\n");
    cin>>user.answer;
    json j;
    j=user;
    string ifo=j.dump();
    char buf[20];
    send(cfd,ifo.c_str(),ifo.size(),0);
    recv(cfd,buf,20,0);
    cout<<user.fd<<endl;
    cout<<"1"<<buf<<endl;
    if(strcmp(buf,"ok")==0)
    {
      cout<<"sign up sucuess!"<<endl;
    }
    else
    {
      cout<<"sorry! "<<endl;
    }



}

void login_menu()
{   int select;
   printf("     ***********     star chatroom    **********  \n");
   printf("    ***********        1.login          **********  \n");
   printf("   ***********         2.sign up          **********  \n");
   printf("  ***********          3.quit               ***********  \n");
   scanf("%d",&select);
   switch(select)
   {
       //case 1: login();
       case 2:sign_up();
       //case 3: quit();
   }



}