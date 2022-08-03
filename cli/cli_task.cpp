#include "json_use.h"
int cfd;

void *Inform(void *arg)
{
  json j;
  char buf[4096];
  struct sockaddr_in ser;
  int ccfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&ser, sizeof(ser));
  ser.sin_family = AF_INET;
  inet_pton(AF_INET, "192.168.30.111", &ser.sin_addr);
  ser.sin_port = htons(PORT);
  // // pthread_create(&tid,NULL,Recv,void *base)
  connect(ccfd, (struct sockaddr *)&ser, sizeof(ser));

  jjjson::usr u = *(jjjson::usr *)arg;
  u.fd = ccfd;
  u.choice = "inform";
  j = u;
  string s = j.dump();
  while (1)
  {
    send(u.fd, s.c_str(), s.size(), 0);
    int ret=recv(u.fd, buf, 4096, 0);
    cout<<ret<<endl;
    string t(buf);
    j = json::parse(t);
    auto tmp = j.get<jjjson::usr>();
    for (auto it = tmp.box.begin(); it != tmp.box.end(); it++)
    {
      if (*it == "exit")
      { cout<<"you already quit"<<endl;
        close(ccfd);
        return NULL;
      }
      cout << "*****" << *it << endl;
    }
  }
}
void Check(jjjson::usr user)
{
  char f[1];
  user.choice = "check";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (strcmp(f, "1") == 0)
  {
    cout << "有通知请注意查收！" << endl;
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
  user.pwd = getpass("请输入密码!\n");
  printf("请设置密保问题！\n");
  cin >> user.question;
  printf("请设置密保答案！\n");
  cin >> user.answer;
  user.status = 0;
  user.id = 0;
  user.time = 0;
  user.buf.clear();
  user.friendid = 0;
  json j;
  j = user;
  string ifo = j.dump();
  char buf[1];
  memset(buf, 0, 1);
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

void settings(jjjson::usr user)
{
  while (1)
  {
    // Inform(user);
    printf("     ***********         welcome %s       **********  \n", user.name.c_str());
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
      cin >> user.name;
      break;
    case 2:
      cout << "请输入新密码" << endl;
      cin >> user.pwd;
      break;
    case 3:
      cout << "请输入新密保" << endl;
      cin >> user.question;
      break;
    case 4:
      cout << "请输入新答案" << endl;
      cin >> user.answer;
      break;
    }
    if (select == 5)
    {
      user.choice = "settings";
      json j;
      j = user;
      string ifo = j.dump();
      char buf[1];
      memset(buf, 0, 1);
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

void Find_pwd()
{
  json j;
  string s;
  char f[1];
  cout << "请输入要找回密码的账号" << endl;
  jjjson::usr user;
  cin >> user.friendname; //其实是偷懒为了方便，因为服务器只写了个看好友的
  user.name = user.friendname;
  user.choice = "check_friend";
  char buf[4096];
  j = user;
  s = j.dump();
  memset(buf, 0, 4096);
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] == '0')
  {
    cout << "账号不存在" << endl;
    return;
  }
  user.choice = "find_pwd";
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, buf, 4096, 0);

  j = json::parse(buf);
  auto tmp = j.get<jjjson::usr>();
  cout << "Question:" << tmp.question << endl;
  cout << "请输入密保答案" << endl;
  cin >> user.answer;
  user.choice = "true_pwd";
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);

  memset(buf, 0, 4096);
  recv(cfd, buf, 4096, 0);
  j = json::parse(buf);
  tmp = j.get<jjjson::usr>();
  if (tmp.pwd != "")
  {
    cout << "请记住密码 ：" << tmp.pwd << endl;
  }
  else
  {
    cout << "密保答案错误！" << endl;
  }
}

void Add_friend(jjjson::usr user)
{
  cout << "请输入你想添加的好友账号！" << endl;
  cin >> user.friendname;
  if (user.name == user.friendname)
  {
    cout << "请勿添加自己！" << endl;
    return;
  }
  user.choice = "check_friend";
  json j = user;
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
void deal_req(jjjson::usr user)
{
  while (1)
  {

    char buf[4096];
    memset(buf, 0, sizeof(buf));
    user.choice = "friend_req";
    json j = user;
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
    printf("     ***********         welcome %s       **********  \n", user.name.c_str());
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
        user.choice = "agree_friend";
      else if (x == 2)
        user.choice = "reject_friend";
      if (x != 3)
      {
        user.friendname = s;
        json j = user;
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
void Delete_friend(jjjson::usr user)
{
  cout << "请输入想删除的用户（按0退出）" << endl;
  cin >> user.friendname;
  user.choice = "check_friend";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  char f[1];
  recv(cfd, f, 1, 0);
  if (strcmp(f, "0") == 0)
  {
    cout << "此用户不存在！" << endl;
  }
  else if (strcmp(f, "3") != 0)
  {
    cout << "不是盆友！" << endl;
  }
  user.choice = "delete_friend";
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  memset(f, 0, sizeof(f));
  recv(cfd, f, 1, 0);
  if (strcmp(f, "1") == 0)
  {
    cout << "成功删除" << endl;
  }
  else
  {
    cout << "删除失败！" << endl;
  }
}

void *recv_chat(jjjson::usr arg)
{ // pthread_detach(pthread_self());
  jjjson::usr tmp = arg;
  // tmp.choice = "recv_mes";
  json j = tmp;
  char buf[4096];
  // string s = j.dump();
  //  send(cfd, s.c_str(), s.size(), 0);
  while (1)
  {

    memset(buf, 0, 4096);

    int ret = recv(cfd, buf, 4096, 0);

    if ((strcmp(buf, "quit")) == 0)
    {
      // cout << "gameover" << endl;
      break;
    }
    else
    {
      string t(buf);

      json j = json::parse(t);
      auto q = j.get<jjjson::Fri_chat>();
      for (auto it = q.unread.begin(); it != q.unread.end(); it++)
      {
        printf("%50c", ' ');
        cout << LIGHT_RED << tmp.friendname << " :" << *it << endl;
        printf("%40c", ' ');
        cout << ctime(&q.unread_t[0]) << NONE << endl;
        q.unread_t.erase(q.unread_t.begin());
      }
    }
  }
  return NULL;
}

void Check_history(jjjson::usr user)
{
  char buf[10000];
  memset(buf, 0, 10000);
  user.choice = "check_history";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, buf, 10000, 0);
  buf[strlen(buf)] = '\0';
  // cout<<"dfs"<<buf<<endl;
  string t(buf);
  json k = json::parse(t);
  auto w = k.get<jjjson::Fri_chat>();
  for (auto it = w.history.begin(); it != w.history.end(); it++)
  {
    cout << *it << endl;
    cout << ctime(&w.time[0]) << endl;
    w.time.erase(w.time.begin());
  }
}

void send_file_fri(jjjson::usr user)
{
  char path[1000], name[100];
  memset(path, 0, 1000);
  memset(name, 0, 100);
  int cnt = 0;
  cout << "请输入文件地址" << endl;
  cin >> path;
  int sign;
  for (int i = strlen(path) - 1; i >= 0; i--)
  {
    if (path[i] == '/')
    {
      sign = i;
      break;
    }
  }
  for (int j = sign + 1; j < strlen(path); j++)
  {
    name[cnt++] = path[j];
  }

  user.filename = name;
  int fd;
  if ((fd = open(path, O_RDONLY)) < 0)
  {
    cout << "open error" << endl;
    return;
  }
  json j;
  string s;
  long long ret = 0;
  user.choice = "recv_file_fri";
  struct stat st;
  stat(path, &st);
  user.id = st.st_size;
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  long long retw = 0, sum = 0;
  char x[4096];
  memset(x, 0, 4096);
  sleep(1);
  while (1)
  { // user.buf.clear();
    ret = read(fd, x, 4095);
    // cout << "1" << endl;
    x[ret] = '\0';
    // user.buf=x;
    // cout << x << endl;
    cout << ret << endl;
    cout << strlen(x) << endl;
    // j=user;
    // s=j.dump();
    sleep(0.01);
    retw = send(cfd, x, ret, 0);
    if (retw > 0)
      sum += retw;
    memset(x, 0, 4096);
    if (ret > retw)
    {
      lseek(fd, sum, SEEK_SET);
    }
    // sleep(1);
    // cout << sum << endl;
    // cout << st.st_size << endl;
    if (sum >= st.st_size)
    {
      // sleep(1);
      // char buf[5] = "over";
      // send(cfd, buf, sizeof(buf), 0);
      break;
    }
    // user.buf.clear();
  }
  // sleep(1);
  // char buf[5]="over";
  // send(cfd,buf,4,0);
  // sleep(1);
  sleep(1);
  close(fd);
}

void recv_file_fri(jjjson::usr user)
{
  while (1)
  {
    int fd;
    json j;
    string s;
    int flag = 0;
    user.choice = "check_file";
    char buf[4096];
    j = user;
    s = j.dump();
    memset(buf, 0, 4096);
    send(cfd, s.c_str(), s.size(), 0);
    recv(cfd, buf, 4096, 0);
    string t(buf);
    j = json::parse(t);
    auto x = j.get<jjjson::Fri_chat>();
    for (auto it = x.file.begin(); it != x.file.end(); it++)
    {
      cout << "*******" << *it << endl;
    }
    cout << "请选择操作对象(输入0退出)" << endl;
    string q;
    cin >> q;
    if (q == "0")
      break;
    for (auto it = x.file.begin(); it != x.file.end(); it++)
    {
      if (q == *it)
      {
        flag = 1;
      }
    }
    if (flag == 0)
    {
      cout << "请选择正确的对象" << endl;
      continue;
    }
    cout << "请选择保存路径" << endl;
    string path;
    cin >> path;
    path += "/";
    path += q;
    cout << path << endl;
    if ((fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0)
    {
      cout << "create file error" << endl;
      continue;
    }

    memset(buf, 0, 4096);
    long long size = 0;
    long long tmplen = 0;
    user.filename = q;
    user.choice = "file_size";
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    recv(cfd, buf, 4096, 0);
    string d(buf);
    j = json::parse(d);
    auto tt = j.get<jjjson::usr>();
    size = tt.id;
    sleep(1);

    user.choice = "send_file_fri";
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);

    long long ret = 0, ret2 = 0;
    memset(buf, 0, 4096);
    while (1)
    {

      ret2 = recv(cfd, buf, 4095, 0);
      ret = write(fd, buf, ret2);
      if (ret > 0)
        tmplen += ret;
      if (tmplen >= size)
      {
        break;
      }

      memset(buf, 0, 4096);
    }
    close(fd);
  }
}

void Chat_sb(jjjson::usr user)
{
  char f[4096];
  memset(f, 0, 4096);
  cout << "请输入聊天对象" << endl;
  cin >> user.friendname;
  user.choice = "check_friend";
  json j = user;
  string r = j.dump();
  send(cfd, r.c_str(), r.size(), 0);
  recv(cfd, f, 4096, 0);
  sleep(1);
  // cout<<"f=="<<f<<endl;
  if (f[0] == '0')
  {
    cout << "没有此人！" << endl;
    return;
  }
  else if (f[0] != '3')
  {
    cout << "不是好友" << endl;
    return;
  }
  if (f[0] == '3')
  {
    char f[1];
    user.choice = "check_shield";
    json c = user;
    string h = c.dump();
    send(cfd, h.c_str(), h.size(), 0);
    recv(cfd, f, 1, 0);
    if (f[0] == '0')
    {
      cout << user.friendname << "已经被你屏蔽，无法聊天！" << endl;
      return;
    }
    printf("请和%s愉快的聊天吧！\n", user.friendname.c_str());
  }
  cout << "1.开始聊天" << endl;
  cout << "2.查看聊天记录" << endl;
  cout << "3.发文件" << endl;
  cout << "4.收文件" << endl;
  cout << "5.输入其他键退出" << endl;
  int select;
  cin >> select;
  if (select == 1)
  {
    json c;
    string h;
    pthread_t tid;
    thread recvv(recv_chat, user);
    user.choice = "offline_mes_fri";
    json k = user;
    string l = k.dump();
    send(cfd, l.c_str(), l.size(), 0);
    user.choice = "chat_sb"; //唤醒
    user.mes_fri = "";
    c = user;
    h = c.dump();
    send(cfd, h.c_str(), h.size(), 0);
    cout << "ok";
    while (1)
    {
      string s;
      user.choice = "chat_sb";
      cin >> s;
      time_t t;
      t = time(NULL);
      user.mes_fri = s;
      user.time = t;
      if (s == "quit")
      {
        // user.choice = "quit_chatfri";
        json j = user;
        string l = j.dump();
        send(cfd, l.c_str(), l.size(), 0);
        break;
      }
      cout << LIGHT_BLUE << user.name << " :" << s << endl;
      cout << ctime(&t) << NONE << endl;
      json j = user;
      string l = j.dump();
      send(cfd, l.c_str(), l.size(), 0);
    }
    recvv.join();
  }
  else if (select == 2)
  {
    Check_history(user);
  }
  else if (select == 3)
  {
    send_file_fri(user);
  }
  else if (select == 4)
  {
    recv_file_fri(user);
  }
}

void Shield_fri(jjjson::usr user)
{

  char f[1];
  cout << "请输入对象" << endl;
  cin >> user.friendname;
  user.choice = "check_friend";
  json j = user;
  string r = j.dump();
  send(cfd, r.c_str(), r.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] == '0')
  {
    cout << "没有此人！" << endl;
    return;
  }
  else if (f[0] != '3')
  {
    cout << "不是好友" << endl;
    return;
  }

  string s;
  cout << "1.屏蔽     2.取消屏蔽       3.退出" << endl;
  cin >> s;
  if (s == "3")
    return;
  else
  {

    if (s == "1")
    {
      user.choice = "start_shield";
    }
    else if (s == "2")
    {
      user.choice = "canel_shield";
    }
    json j = user;
    string s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
  }
}

void send_file(jjjson::usr user)
{
  string path;
  cout << "请输入文件地址" << endl;
  cin >> path;
  cout << "请输保存文件名" << endl;
  cin >> user.filename;
  int fd;
  if ((fd = open(path.c_str(), O_RDONLY)) < 0)
  {
    cout << "open error" << endl;
    return;
  }
  json j;
  string s;
  int ret = 0;
  user.choice = "recv_file";
  struct stat st;
  stat(path.c_str(), &st);
  user.id = st.st_size;

  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);

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
    sleep(0.1);
    send(cfd, x, ret, 0);
    memset(x, 0, 4096);
    // sleep(1);
    if (ret != 4095)
    {
      sleep(1);
      char buf[5] = "over";
      send(cfd, buf, sizeof(buf), 0);
      break;
    }
    // user.buf.clear();
  }
  // sleep(1);
  // char buf[5]="over";
  // send(cfd,buf,4,0);
  // sleep(1);
  close(fd);
}

void Friend(jjjson::usr user)
{
  while (1)
  { // system("clear");
    // Inform(user);
    cout << "************friend" << endl;
    user.choice = "look_friend";

    json j = user;
    string s = j.dump();

    send(cfd, s.c_str(), s.size(), 0);
    char tmpfri[4096];
    s.clear();
    memset(tmpfri, 0, sizeof(tmpfri));
    recv(cfd, tmpfri, 4096, 0);
    // tmpfri[strlen(tmpfri)] = '\0';
    // cout << "thiuss" << tmpfri << endl;
    s = tmpfri;
    // printf("111\n");
    // cout << "this" << s << endl;
    auto m = json::parse(s);

    auto fri = m.get<jjjson::Friend>();
    for (auto iter = fri.myfri.begin(); iter != fri.myfri.end(); iter++)
    {
      char f[1];
      memset(f, 0, 1);
      recv(cfd, f, 1, 0);
      cout << "************" << *iter << "*********";
      if (f[0] == '0')
      {
        cout << "offline" << endl;
      }
      else
      {
        cout << "online" << endl;
      }
    }
    printf("     ***********         welcome %s       **********  \n", user.name.c_str());
    printf("    ***********         1.添加好友          **********  \n");
    printf("   ***********          2.删除好友           **********  \n");
    printf("  ***********           3.私聊               ***********  \n");
    printf(" ***********            4.屏蔽好友(开启/取消)             **********  \n");
    printf("***********             5.处理好友申请           **********  \n");
    // printf("***********             6.发送文件          **********  \n");
    // printf("***********             7.接受文件           **********  \n");
    printf("***********             6.退出                    **********  \n");
    int select;
    cin >> select;
    switch (select)
    {
    case 1:
      Add_friend(user);
      break;
    case 2:
      Delete_friend(user);
      break;
    case 3:
      Chat_sb(user);
      break;
    case 4:
      Shield_fri(user);
      break;
    case 5:
      deal_req(user);
      break;
      // case 6 :
      // send_file(user);
      // break;
    }
    if (select == 6)
    {
      system("clear");
      return;
    }
  }
}

void Build_group(jjjson::usr user)
{
  char f[1];
  cout << "请输入想要创建的群" << endl;
  cin >> user.group;
  user.choice = "build_group";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] == '1')
  {
    cout << "创建成功" << endl;
  }
  else
  {
    cout << "该群已经创建" << endl;
  }
}

void Join_group(jjjson::usr user)
{
  char f[1];
  cout << "请输入想加入的群聊" << endl;
  cin >> user.group;
  user.choice = "look_g";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] == '1')
  {
    cout << "该群不存在" << endl;
  }
  else if (f[0] == '0')
  {
    user.choice = "join_group";
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    cout << "成功发送请求" << endl;
  }
  else if (f[0] == '3')
  {
    cout << "已经发送过请求，请勿重复发送！" << endl;
  }
  else if (f[0] == '4')
  {
    cout << "已经是群成员" << endl;
  }
}

void deal_group_req(jjjson::usr user)
{
  int flag = 0;
  while (1)
  {
    flag = 0;
    char buf[4096];
    memset(buf, 0, sizeof(buf));
    user.choice = "group_req";
    json j = user;
    string s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    recv(cfd, buf, 4096, 0);
    buf[strlen(buf)] = '\0';
    string tmp(buf);
    j = json::parse(tmp);

    auto g = j.get<jjjson::Group>();
    if (g.owner == user.name)
    {
      flag = 1;
    }
    for (auto it = g.manager.begin(); it != g.manager.end(); it++)
    {
      if (*it == user.name)
      {
        flag = 1;
      }
    }
    if (flag == 0)
    {
      cout << "你不是管理员或群主！" << endl;
      return;
    }

    for (auto it = g.join_req.begin(); it != g.join_req.end(); it++)
    {
      cout << "from____________" << *it << endl;
    }
    printf("     ***********         welcome %s       **********  \n", user.name.c_str());
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
        user.choice = "agree_group";
      else if (x == 2)
        user.choice = "reject_group";
      if (x != 3)
      {
        user.friendname = s;
        json j = user;
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

void check_member(jjjson::usr user)
{
  json j;
  string s;
  char buf[4096];
  memset(buf, 0, 4096);
  user.choice = "check_member";
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, buf, 4096, 0);
  string t(buf);
  j = json::parse(t);
  auto tmp = j.get<jjjson::Group>();
  cout << "***********" << user.group << "************" << endl;
  cout << "*******成员*******"
       << "***身份****" << endl;
  cout << "*******" << tmp.owner << "**********owner****" << endl;
  for (auto it = tmp.manager.begin(); it != tmp.manager.end(); it++)
  {
    if (*it != tmp.owner)
      cout << "*******" << *it << "**********manager****" << endl;
  }
  for (auto it = tmp.member.begin(); it != tmp.member.end(); it++)
  {
    int flag = 1;
    if (tmp.owner == *it)
    {
      flag = 0;
      continue;
    }
    for (auto i = tmp.manager.begin(); i != tmp.manager.end(); i++)
    {
      if (*i == *it)
      {
        flag = 0;
        break;
      }
    }
    if (flag == 0)
      continue;
    cout << "*******" << *it << "**********member****" << endl;
  }
}

void set_manager(jjjson::usr user)
{

  char f[1];
  json j;
  string s;
  user.choice = "look_g";
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] != '6')
  {
    cout << "不是群主，无权操作" << endl;
    return;
  }
  while (1)
  {
    cout << "******1.添加管理员        2.取消管理员        3.退出******" << endl;
    string select;
    cin >> select;
    if (select == "1")
    {
      string tmpname = user.name;
      cout << "请输入想要添加的管理员" << endl;
      cin >> user.name;
      user.friendname = user.name;
      user.choice = "look_g";
      j = user;
      s = j.dump();
      send(cfd, s.c_str(), s.size(), 0);
      recv(cfd, f, 1, 0);
      user.name = tmpname;
      if (f[0] != '4' && f[0] != '5' && f[0] != '6')
      {
        cout << "不是群成员" << endl;
        return;
      }
      if (f[0] == '5' || f[0] == '6')
      {
        cout << "已是管理员" << endl;
        return;
      }
      user.choice = "set_manager";
    }
    else if (select == "2")
    {
      string tmpname = user.name;
      cout << "请输入想要撤销的管理员" << endl;
      cin >> user.name;
      user.friendname = user.name;
      user.choice = "look_g";
      j = user;
      s = j.dump();
      send(cfd, s.c_str(), s.size(), 0);
      recv(cfd, f, 1, 0);
      user.name = tmpname;
      if (f[0] != '4' && f[0] != '5' && f[0] != '6')
      {
        cout << "不是群成员" << endl;
        return;
      }
      if ((f[0] != '5') && (f[0] != '6'))
      {
        cout << "不是管理员" << endl;
        return;
      }
      else if (f[0] == '6')
      {
        cout << "此人是群主无权更改" << endl;
        return;
      }
      user.choice = "canel_manager";
    }
    else
    {
      break;
    }
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
  }
}

void kick_sb(jjjson::usr user)
{
  char f[1];
  json j;
  string s;
  user.choice = "look_g";
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] != '6' && f[0] != '5')
  {
    cout << "不是管理员，无权操作" << endl;
    return;
  }
  while (1)
  {
    check_member(user);

    cout << "*****1.踢人       2.退出*****" << endl;
    string select;
    cin >> select;
    if (select == "1")
    {
      string tmpname = user.name;
      cout << "请输入想要踢的人" << endl;
      cin >> user.name;
      user.friendname = user.name;
      user.choice = "look_g";
      j = user;
      s = j.dump();
      send(cfd, s.c_str(), s.size(), 0);
      recv(cfd, f, 1, 0);
      user.name = tmpname;
      if (f[0] != '4' && f[0] != '5' && f[0] != '6')
      {
        cout << "不是群成员" << endl;
        return;
      }
      if (f[0] == '5' || f[0] == '6')
      {
        cout << "是管理员，无法踢人" << endl;
        return;
      }
      user.choice = "kick_sb";
      j = user;
      s = j.dump();
      send(cfd, s.c_str(), s.size(), 0);
    }
    else
    {
      break;
    }
  }
}

void withdraw_group(jjjson::usr user)
{
  cout << "请确认你要退出群：" << user.group << endl;
  cout << "*****1.确认       *****2.取消****" << endl;
  string select;
  cin >> select;
  if (select == "1")
  {
    user.choice = "withdraw_group";
    json j;
    string s;
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
  }
  else
  {
    return;
  }
}

void *recv_chat_group(jjjson::usr arg)
{ // pthread_detach(pthread_self());
  // jjjson::usr tmp = arg;
  // tmp.choice = "recv_mes_gro";
  // json j = tmp;
  // string s = j.dump();
  // send(cfd, s.c_str(), s.size(), 0);
  while (1)
  {

    char buf[4096];
    memset(buf, 0, 4096);

    int ret = recv(cfd, buf, 4096, 0);
    if ((strcmp(buf, "quit")) == 0)
    {
      cout << "gameover" << endl;
      break;
    }
    else
    {
      string t(buf);
      // cout<<"khj--"<<t<<endl;
      json j = json::parse(t);
      auto q = j.get<jjjson::Gro_chat>();
      for (auto it = q.unread_mes.begin(); it != q.unread_mes.end(); it++)
      {
        printf("%50c", ' ');
        cout << LIGHT_RED << *it << endl;
        printf("%40c", ' ');
        cout << ctime(&q.unread_t[0]) << NONE << endl;
        q.unread_t.erase(q.unread_t.begin());
      }
    }
  }
  return NULL;
}

void Check__group_history(jjjson::usr user)
{
  json j;
  string s;
  char buf[10000];
  string value;
  user.choice = "check_group_history";
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, buf, 10000, 0);
  string t(buf);
  cout << t << endl;
  j = json::parse(t);
  auto tmp = j.get<jjjson::Group>();
  for (auto it = tmp.history.begin(); it != tmp.history.end(); it++)
  {
    cout << *it << endl;
    cout << ctime(&tmp.time[0]) << endl;
    tmp.time.erase(tmp.time.begin());
  }
}

void chat_group(jjjson::usr user)
{

  cout << "1.开始聊天" << endl;
  cout << "2.查看聊天记录" << endl;
  // cout<<"3.传文件"<<endl;
  // cout<<"4.收文件"<<endl;
  cout << "3.输入其他键退出" << endl;
  string select;
  cin >> select;
  if (select == "1")
  {
    pthread_t tid;
    thread recvv(recv_chat_group, user);
    //  user.choice = "offline_mes_gro"; //获取离线消息
    //  json k = user;
    //  string l = k.dump();
    string l;
    send(cfd, l.c_str(), l.size(), 0);
    user.choice = "chat_group"; //先唤醒聊天状态
    user.mes_fri = "";
    json j = user;
    l = j.dump();
    send(cfd, l.c_str(), l.size(), 0);
    while (1)
    {
      string m;
      string s;
      s = "from:" + user.name + ":";
      user.choice = "chat_group";
      cin >> m;
      s += m;
      time_t t;
      t = time(NULL);
      user.mes_fri = s;
      user.time = t;
      if (m == "quit")
      {
        user.mes_fri = m;
        // user.choice = "quit_chatgro";
        json j = user;
        string l = j.dump();
        send(cfd, l.c_str(), l.size(), 0);
        break;
      }
      cout << LIGHT_BLUE << user.name << " :" << s << endl;
      cout << ctime(&t) << NONE << endl;
      json j = user;
      string l = j.dump();
      send(cfd, l.c_str(), l.size(), 0);
    }
    recvv.join();
  }
  else if (select == "2")
  {
    Check__group_history(user);
  }
}

void send_file_gro(jjjson::usr user)
{
  char path[1000], name[100];
  cout << "请输入文件地址" << endl;
  cin >> path;
  int sign = 0, cnt = 0;
  for (int i = strlen(path) - 1; i >= 0; i--)
  {
    if (path[i] == '/')
    {
      sign = i;
      break;
    }
  }
  for (int j = sign + 1; j < strlen(path); j++)
  {
    name[cnt++] = path[j];
  }
  user.filename = name;
  int fd;
  if ((fd = open(path, O_RDONLY)) < 0)
  {
    cout << "open error" << endl;
    return;
  }
  json j;
  string s;
  int ret = 0;
  user.choice = "recv_file_gro";
  struct stat st;
  stat(path, &st);
  user.id = st.st_size;
  j = user;
  s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);

  long long retw = 0, sum = 0;
  char x[4096];
  memset(x, 0, 4096);
  sleep(1);
  while (1)
  {
    ret = read(fd, x, 4096);
    sleep(0.1);
    retw = send(cfd, x, ret, 0);
    if (retw > 0)
      sum += retw;

    memset(x, 0, 4096);
    if (ret > retw)
    {
      lseek(fd, sum, SEEK_SET);
    }
    cout << sum << endl;
    cout << st.st_size << endl;
    if (sum >= st.st_size)
    {
      break;
    }
  }
  sleep(1);
  close(fd);
}

void recv_file_gro(jjjson::usr user)
{
  while (1)
  {
    int fd;
    json j;
    string s;
    int flag = 0;
    user.choice = "check_file_gro";
    char buf[4096];
    j = user;
    s = j.dump();
    memset(buf, 0, 4096);
    send(cfd, s.c_str(), s.size(), 0);
    recv(cfd, buf, 4096, 0);
    string t(buf);
    cout << t << endl;
    j = json::parse(t);
    auto x = j.get<jjjson::Gro_chat>();
    for (auto it = x.filename.begin(); it != x.filename.end(); it++)
    {
      cout << "*******" << *it << endl;
    }
    cout << "请选择操作对象(输入0退出)" << endl;
    string q;
    cin >> q;
    if (q == "0")
      break;
    for (auto it = x.filename.begin(); it != x.filename.end(); it++)
    {
      if (q == *it)
      {
        flag = 1;
      }
    }
    if (flag == 0)
    {
      cout << "请选择正确的对象" << endl;
      continue;
    }
    cout << "请选择保存路径" << endl;
    string path;
    cin >> path;
    path += "/";
    path += q;
    cout << path << endl;
    if ((fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666)) < 0)
    {
      cout << "create file error" << endl;
      continue;
    }
    memset(buf, 0, 4096);
    long long size = 0;
    long long tmplen = 0;
    user.filename = q;
    user.choice = "file_size";
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    recv(cfd, buf, 4096, 0);
    string d(buf);
    j = json::parse(d);
    auto tt = j.get<jjjson::usr>();
    size = tt.id;
    sleep(1);

    user.filename = q;
    user.choice = "send_file_gro";
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);

    long long ret = 0, ret2 = 0;
    memset(buf, 0, 4096);
    while (1)
    {

      ret2 = recv(cfd, buf, 4095, 0);
      ret = write(fd, buf, ret2);
      if (ret > 0)
        tmplen += ret;
      if (tmplen >= size)
      {
        break;
      }

      memset(buf, 0, 4096);
    }
    close(fd);
  }
}

void Enter_group(jjjson::usr user)
{
  char f[1];
  cout << "请输入进入的群聊（0退出）" << endl;
  cin >> user.group;
  if (user.group == "0")
    return;
  user.choice = "look_g";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  sleep(1);
  if (f[0] == '1')
  {
    cout << "该群不存在" << endl;
    return;
  }
  else if (f[0] != '4' && f[0] != '6' && f[0] != '5')
  {
    cout << "你不是该群成员" << endl;
    return;
  }
  while (1)
  {
    // Inform(user);
    printf("     ***********         welcome %s       **********  \n", user.name.c_str());
    printf("    ***********         1.查看群成员列表          **********  \n");
    printf("   ***********          2.设置/取消管理员       **********  \n");
    printf("  ***********           3.开始群聊             ***********  \n");
    printf(" ***********            4.退出群聊          **********  \n");
    printf(" ***********            5.处理群请求             **********  \n");
    printf(" ***********            6.踢人                    **********  \n");
    printf(" ***********            7.传输文件                    **********  \n");
    printf(" ***********            8.接受文件                    **********  \n");
    printf(" ***********            9. 退出                   **********  \n");
    int select;
    cin >> select;
    switch (select)
    {
    case 1:
      check_member(user);
      break;
    case 2:
      set_manager(user);
      break;
    case 3:
      chat_group(user);
      break;
    case 4:
      withdraw_group(user);
      break;
    case 5:
      deal_group_req(user);
      break;
    case 6:
      kick_sb(user);
      break;
    case 7:
      send_file_gro(user);
      break;
    case 8:
      recv_file_gro(user);
    }
    if (select == 9)
    {
      break;
    }
  }
  system("clear");
}

void disband_group(jjjson::usr user)
{
  char f[1];
  cout << "请输入解散的群聊" << endl;
  cin >> user.group;
  user.choice = "look_g";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] == '1')
  {
    cout << "该群不存在" << endl;
    return;
  }
  else if (f[0] != '6')
  {
    cout << "你不是该群群主" << endl;
    return;
  }
  cout << "*****1.确认     2.取消*****" << endl;
  string a;
  cin >> a;
  if (a != "1")
    return;
  else
  {
    user.choice = "disband_group";
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
  }
}

void Group(jjjson::usr user)
{
  while (1)
  {
    // Inform(user);
    char buf[4096];
    memset(buf, 0, 4096);
    string s;
    json j;
    user.choice = "look_group";
    j = user;
    s = j.dump();
    send(cfd, s.c_str(), s.size(), 0);
    recv(cfd, buf, 4096, 0);
    sleep(1);
    buf[strlen(buf)] = '\0';
    string t(buf);
    j = json::parse(t);
    auto tmp = j.get<jjjson::myGroup>();
    cout << "*********群名       ********身份********" << endl;
    for (auto it = tmp.mygroup.begin(); it != tmp.mygroup.end(); it++)
    {
      cout << "        " << *it << "              ";
      if (tmp.status[*it] == 1)
      {
        cout << "owner" << endl;
      }
      else if (tmp.status[*it] == 2)
      {
        cout << "manager" << endl;
      }
      else if (tmp.status[*it] == 3)
      {
        cout << "member" << endl;
      }
    }

    printf("     ***********         welcome %s       **********  \n", user.name.c_str());
    printf("    ***********         1.建立群聊          **********  \n");
    printf("   ***********          2.申请加入群聊           **********  \n");
    printf("  ***********           3.进入群聊            ***********  \n");
    printf("***********             4. 解散群          **********  \n");
    printf("***********             5. 退出                   **********  \n");
    int select;
    cin >> select;
    switch (select)
    {
    case 1:
      Build_group(user);
      break;
    case 2:
      Join_group(user);
      break;
    case 3:
      Enter_group(user);
      break;
    case 4:
      disband_group(user);
      break;
    }
    if (select == 5)
    {
      break;
    }
  }
  system("clear");
}

int Logout(jjjson::usr user)
{
  char f[1];
  user.choice = "logout";
  json j = user;
  string s = j.dump();
  send(cfd, s.c_str(), s.size(), 0);
  recv(cfd, f, 1, 0);
  if (f[0] == '1')
    return 1;
  else
    return 0;
}

int menu(jjjson::usr user)
{
  while (1)
  {
    // Check(user);
    // Inform(user);
    printf("     ***********         welcome %s       **********  \n", user.name.c_str());
    printf("    ***********         1.个人信息设置       **********  \n");
    printf("   ***********          2.好友               **********  \n");
    printf("  ***********           3.群                  ***********  \n");
    printf("  ***********           4.查看通知               ***********  \n");
    printf(" ***********            5.注销账号                **********  \n");
    printf(" ***********            6.退出                      **********  \n");
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
      Group(user);
      break;
    case 4:
      // Inform(user);
      break;
    case 5:
      if (Logout(user))
      {
        cout << "注销成功" << endl;
        return 0;
      }
      else
        cout << "注销失败" << endl;
      break;

    case 6:
      user.choice = "offline";
      json j;
      j = user;
      string ifo = j.dump();
      char buf[1];
      send(cfd, ifo.c_str(), ifo.size(), 0);
      read(cfd, buf, 1);
      if (strcmp(buf, "1") == 0)
      {
        cout << "退出成功！\n";
      }
      system("clear");
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

  user.pwd = getpass("请输入密码!\n");
  json j;
  j = user;
  string ifo = j.dump();
  char buf[1];
  user.answer = "";
  user.box.clear();
  user.fd = 0;
  user.friendid = 0;
  user.friendname = "";
  user.id = 0;
  user.mes_fri = "";
  user.question = "";
  user.time = 0;
  user.status = 0;
  send(cfd, ifo.c_str(), ifo.size(), 0);
  read(cfd, buf, 1);
  if (strcmp(buf, "1") == 0)
  {
    cout << "login sucuess!" << endl;
    pthread_t t;
    pthread_create(&t, NULL, Inform, (void *)&user);
    menu(user);
  }
  else if (strcmp(buf, "2") == 0)
  {
    cout << "password error " << endl;
  }
  // else if(strcmp(buf, "6") == 0)
  //{
  // cout<<"此账号已经登录！"<<endl;
  //}
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
    cout << "/*************************************************/" << endl;
    cout << "/*  ____   _                  ____ _           _    */" << endl;
    cout << "/* / ___| | |_  __ _  _ __    / ___| |__   __ _| |_  */" << endl;
    cout << "/* \\___ \\ | __ / _`  | '__|  | |   | '_ \\ / _` | __| */" << endl;
    cout << "/*  ___)  | | | (_|  | |     | |___| | | | (_| | |_  */" << endl;
    cout << "/* |____/  \\__ \\__,_ |_|      \\____|_| |_|\\__,_|\\__| */" << endl;
    cout << "/*                                               */" << endl;
    cout << "/*************************************************/" << endl;
    printf("    ***********        1.login          **********  \n");
    printf("   ***********         2.sign up          **********  \n");
    printf("   ***********         3.find_pwd          **********  \n");
    printf("  ***********          4.quit               ***********  \n");
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
      Find_pwd();
    case 4:
      return -1;
      break;
    }
  }
  system("clear");

  return 1;
}