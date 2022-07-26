#include<iostream>
using namespace std;
int main()
{

    time_t t;
    t=time(NULL);
    cout<<ctime(&t);

}