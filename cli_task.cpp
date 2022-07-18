#include "json_use.h"
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
  json j;
  j = user;
  string ifo = j.dump();
  char buf[1];
  send(cfd, ifo.c_str(), ifo.size(), 0);
  read(cfd, buf, 1);
  cout << buf << endl;
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

int  menu(jjjson::usr user)
{
  printf("     ***********         welcome %s       **********  \n", user.name.c_str());
  printf("    ***********         1.个人信息设置       **********  \n");
  printf("   ***********          2.好友               **********  \n");
  printf("  ***********           3.群                  ***********  \n");
  printf(" ***********          4.退出                    **********  \n");
  int select;
  scanf("%d", &select);
  switch(select)
  {
    case 1:settings(user);break;
    case 2:break;
    case 3:break;
    case 4: return -1;break;
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
  cout << buf << endl;
  if (strcmp(buf, "1") == 0)
  {
    cout << "login sucuess!" << endl;
    menu(user);
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
  }

  return 1;
}