#include "json_use.h"

void Check(jjjson::usr *user)
{
  char f[1];
  (*user).choice="check";
  json j=*user;
  string s=j.dump();
  send(cfd,s.c_str(),s.size(),0);
  recv(cfd,f,1,0);
  if(strcmp(f,"1")==0)
  {
     cout<<"有通知请注意查收！"<<endl;
  }

}

void sign_up()
{
  jjjson::usr user;

  // user.id=(int)time(NULL);
  // printf("注册ID为%d\n请注意保存",user.id);
  user.choice = "sign";
  printf("请输入用户名！\n");
  cin >> user.name;
  printf("请输入密码!\n");
  cin >> user.pwd;
  printf("请设置密保问题！\n");
  cin >> user.question;
  printf("请设置密保答案！\n");
  cin >> user.answer;
  user.status = 0;
  user.id = 0;
  user.friendid = 0;
  json j;
  j = user;
  string ifo = j.dump();
  char buf[1];
  send(cfd, ifo.c_str(), ifo.size(), 0);
  read(cfd, buf, 1);
  if (strcmp(buf, "1") == 0)
  {
    cout << "sign up sucuess!" << endl;
  }
  else
  {
    cout << "sorry! " << endl;
  }
  return;
}

void settings(jjjson::usr *user)
{
  while (1)
  {
    printf("     ***********         welcome %s       **********  \n", (*user).name.c_str());
    printf("    ***********         1.昵称              **********  \n");
    printf("   ***********          2.密码               **********  \n");
    printf("  ***********           3.密保问题             ***********  \n");
    printf(" ***********            4.答案                  **********  \n");
    printf("***********             5.退出                   **********  \n");

    int select;
    cin >> select;
    switch (select)
    {
    case 1:
      cout << "请输入新昵称" << endl;
      cin >> (*user).name;
      break;
    case 2:
      cout << "请输入新密码" << endl;
      cin >> (*user).pwd;
      break;
    case 3:
      cout << "请输入新密保" << endl;
      cin >> (*user).question;
      break;
    case 4:
      cout << "请输入新答案" << endl;
      cin >> (*user).answer;
      break;
    }
    if (select == 5)
    {
      (*user).choice = "settings";
      json j;
      j = *user;
      string ifo = j.dump();
      char buf[1];
      send(cfd, ifo.c_str(), ifo.size(), 0);
      read(cfd, buf, 1);
      if (strcmp(buf, "1") == 0)
      {
        cout << "修噶成功！" << endl;
      }
      else
      {
        cout << "修噶失败！" << endl;
      }
      break;
    }
  }
}

void Add_friend(jjjson::usr *user)
{
  cout << "请输入你想添加的好友账号！" << endl;
  cin >> (*user).friendname;
  if ((*user).name == (*user).friendname)
  {
    cout << "请勿添加自己！" << endl;
    return;
  }
  (*user).choice = "check_friend";
  json j = *user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  char f[1];
  recv(cfd, f, 1, 0);
  if (strcmp(f, "0") == 0)
  {
    cout << "此用户不存在！" << endl;
  }
  else if (strcmp(f, "2") == 0)
  {
    cout << "已经过发送请求！请勿重复发送！" << endl;
  }
  else if (f[0] == '3')
  {
    cout << "已是朋友！" << endl;
  }
  else
  {
    cout << "已成功发送！" << endl;
  }
}
void deal_req(jjjson::usr *user)
{
  while (1)
  {

    char buf[4096];
    memset(buf,0,sizeof(buf));
    (*user).choice = "friend_req";
    json j = *user;
    string s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    recv(cfd, buf, 4096, 0);
    buf[strlen(buf)] = '\0';
    string tmp(buf);
    j = json::parse(tmp);

    auto fri = j.get<jjjson::Friend>();
    for (auto it = fri.request.begin(); it != fri.request.end(); it++)
    {
      cout << "from____________" << *it << endl;
    }
    printf("     ***********         welcome %s       **********  \n", (*user).name.c_str());
    printf("    ***********         1.操作         **********  \n");
    printf("    ***********         2.退出              *************\n");
    int select;
    cin >> select;
    switch (select)
    {
    case 1:
      cout << "请选择操作对象" << endl;
      string s;
      cin >> s;
      printf("    ***********  1.同意     2.拒绝      3.取消         **********  \n");
      int x;
      cin >> x;
      if (x == 1)
        (*user).choice = "agree_friend";
      else if (x == 2)
        (*user).choice = "reject_friend";
      if (x != 3)
      {
        (*user).friendname = s;
        json j = *user;
        string s = j.dump();
        send(cfd, s.c_str(), s.size(), 0);
      }
      break;
    }
    if (select == 2)
    {
      break;
    }
  }
}
void Delete_friend(jjjson::usr *user)
{   cout<<"请输入想删除的用户（按0退出）"<<endl;
    cin>>(*user).friendname;
   (*user).choice="check_friend";
    json j = *user;
    string s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    char f[1];
    recv(cfd, f, 1, 0);
    if (strcmp(f, "0") == 0)
    {
    cout << "此用户不存在！" << endl;
    }
    else if(strcmp(f,"3")!=0)
    {
      cout<<"不是盆友！"<<endl;
    }
    (*user).choice="delete_friend";
     j = *user;
     s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    memset(f,0,sizeof(f));
    recv(cfd,f,1,0);
    if(strcmp(f,"1")==0)
    {
      cout<<"成功删除"<<endl;
    }
    else
    {
      cout<<"删除失败！"<<endl;
    }
    


   


}
void Friend(jjjson::usr *user)
{
  while (1)
  {
    cout << "************friend" << endl;
    (*user).choice = "look_friend";

    json j = *user;
    string s = j.dump();

    send(cfd, s.c_str(), s.size(), 0);
    char tmpfri[4096];
    memset(tmpfri, 0, sizeof(tmpfri));
    recv(cfd, tmpfri, 4096, 0);
    tmpfri[strlen(tmpfri)] = '\0';
    s = tmpfri;
    auto m = json::parse(s);
    auto fri = m.get<jjjson::Friend>();
    for (auto iter = fri.myfri.begin(); iter != fri.myfri.end(); iter++) //删申请
    {
      cout << "************" << *iter << endl;
    }
    printf("     ***********         welcome %s       **********  \n", (*user).name.c_str());
    printf("    ***********         1.添加好友          **********  \n");
    printf("   ***********          2.删除好友           **********  \n");
    printf("  ***********           3.私聊               ***********  \n");
    printf(" ***********            4.屏蔽好友             **********  \n");
    printf("***********             5.处理好友申请           **********  \n");
    printf("***********            6.退出                   **********  \n");
    int select;
    cin >> select;
    switch (select)
    {
    case 1:
      Add_friend(user);
      break;
    case 2: Delete_friend(user);
      break;
    case 3:
      break;
    case 4:
      break;
    case 5:
      deal_req(user);
      break;
    }
    if (select == 6)
    {
      return;
    }
  }
}

void Inform(jjjson::usr *user)
{ cout<<"*********I N F O R M"<<endl; 
  char buf[4096];
   memset(buf,0,4096);
   (*user).choice="inform";
   json j=*user;
  string s=j.dump();
  send(cfd,s.c_str(),s.size(),0);
  recv(cfd,buf,4096,0);
  string tmp(buf);
  j=json::parse(tmp);
  auto uu=j.get<jjjson::usr>();
  for(auto it=uu.box.begin();it!=uu.box.end();it++)
  {
    cout<<*it<<endl;
  }
}

int menu(jjjson::usr *user)
{
  while (1)
  {  Check(user);
    printf("     ***********         welcome %s       **********  \n", (*user).name.c_str());
    printf("    ***********         1.个人信息设置       **********  \n");
    printf("   ***********          2.好友               **********  \n");
    printf("  ***********           3.群                  ***********  \n");
    printf("  ***********           4.查看通知               ***********  \n");
    printf(" ***********            5.退出                  **********  \n");
    int select;
    scanf("%d", &select);
    switch (select)
    {
    case 1:
      settings(user);
      break;

    case 2:
      Friend(user);
      break;

    case 3:
      break;
    case 4:
      Inform(user);
      break;
    case 5:
      (*user).status = 0;
      (*user).choice = "offline";
      json j;
      j = *user;
      string ifo = j.dump();
      char buf[1];
      send(cfd, ifo.c_str(), ifo.size(), 0);
      read(cfd, buf, 1);
      if (strcmp(buf, "1") == 0)
      {
        cout << "退出成功！\n";
      }
      return -1;
      break;
    }
  }
  return 0;
}
void login()
{
  jjjson::usr user;

  // user.id=(int)time(NULL);
  // printf("注册ID为%d\n请注意保存",user.id);
  user.choice = "login";
  printf("请输入用户名！\n");
  cin >> user.name;
  printf("请输入密码!\n");
  cin >> user.pwd;
  json j;
  j = user;
  string ifo = j.dump();
  char buf[1];
  send(cfd, ifo.c_str(), ifo.size(), 0);
  read(cfd, buf, 1);
  if (strcmp(buf, "1") == 0)
  {
    cout << "login sucuess!" << endl;
    menu(&user);
  }
  else if (strcmp(buf, "2") == 0)
  {
    cout << "password error " << endl;
  }
  else
  {
    cout << "don't exit the account" << endl;
  }
}
int login_menu()
{
  while (1)
  {
    int select;
    printf("     ***********     star chatroom    **********  \n");
    printf("    ***********        1.login          **********  \n");
    printf("   ***********         2.sign up          **********  \n");
    printf("  ***********          3.quit               ***********  \n");
    fflush(stdin);
    scanf("%d", &select);
    switch (select)
    {
    case 1:
      login();
      break;
    case 2:
      sign_up();
      break;
    case 3:
      return -1;
      break;
    }
  }

  return 1;
}