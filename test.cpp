#include"leveldb/db.h"
#include<cassert>
#include<iostream>
#include<string>
using namespace std;
int main()
{
   leveldb::DB* pwd;
   leveldb::Options options;
   std::string value;
   options.create_if_missing=true;
   leveldb::Status status =leveldb::DB::Open(options,"pwd",&pwd);
   if(status.ok())
   {
       printf("build successfully");
   }
   string key;
   cout<<"what's your name?"<<endl;
   cin>>key;
   string password;
   cout<<"password"<<endl;
   cin>>password;
   leveldb::Status s =pwd->Put(leveldb::WriteOptions(),key,password);
   leveldb::DB* F;
   leveldb::Options opt;
   opt.create_if_missing=false;
   string a="123";
   string b="456";
  //F->Put(leveldb::WriteOptions(),a,b);
   s=pwd->Get(leveldb::ReadOptions(),key, &value);
   cout<<value<<endl;
   //s=F->Get(leveldb::ReadOptions(),"123", &value);
   //cout<<value<<endl;
   


   delete pwd;
   delete F;



}