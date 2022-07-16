#include <iostream>
#include <nlohmann/json.hpp> //引入json.hpp，该文件已经放在系统默认路径：/usr/local/include/nlohmann/json.hpp
using namespace std;
// for convenience
using json = nlohmann::json;
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
    jjjson::stu s;
    s.name = "czx";
    s.age = 1;
    s.sex = 0;
    json j = s; //将struct 变成 json
    cout << j << endl; //输出json对象值
    auto m = j.get<jjjson::stu>();  //json变回struct 
    cout << m.name<<endl;
    cout << m.age<<endl;
    cout << m.sex<<endl;

    cout<<"____________________________________"<<endl;

    jjjson::pwd p;
    p.name="czx";
    p.pwd="123";
    json x=p;
    cout << x << endl; //输出json对象值

    auto qq=x.dump();//将json转成字符串
    cout<<qq<<endl;
    cout<<"_______________"<<endl;

    auto mm=json::parse(qq); //将字符串转回json
    cout<<mm<<endl;
    
    //auto
    //auto n = qq.parse().get<jjjson::pwd>();
    //cout << n.name<<endl;
    //cout << n.pwd<<endl;
    return 0;
}
