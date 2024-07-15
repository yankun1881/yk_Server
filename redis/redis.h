#ifndef __YK_REDIS_H__
#define __YK_REDIS_H__

#include <string>
#include <vector>
#include <memory>


#include <sw/redis++/redis++.h>
#include <sw/redis++/sentinel.h>
#include <sw/redis++/connection.h>
#include <sw/redis++/connection_pool.h>



#include"yk/config.h"
#include"yk/singleton.h"
namespace yk{

class single{
    typedef std::shared_ptr<single> ptr; 
    
public:
    single(std::string ip ,int port,int db,int size,
      std::string password);
    single();
    int init();     //尝试连接
    std::shared_ptr<sw::redis::Redis> getConn(); 
private:
    sw::redis::ConnectionOptions connection_options;
    sw::redis::ConnectionPoolOptions pool_options;
};

//redis++没有做读写分离，每次使用需标明使用的是哪个节点
class sentinel{
public:
    sentinel(std::vector<std::pair<std::string,int> >& potrs,int db,
      std::string password);
    sentinel();


    std::shared_ptr<sw::redis::Redis> getMaster();
    std::shared_ptr<sw::redis::Redis> getSlave();
private:
    std::vector<std::pair<std::string,int> > ports;
    int db = 0;
    std::string password;
    int pool_size = 1;
private:
    sw::redis::SentinelOptions sentinel_opts;
    std::shared_ptr<sw::redis::Sentinel> sent;
    std::shared_ptr<sw::redis::Redis> master;
    std::shared_ptr<sw::redis::Redis> slave;
};

class RedisConn{
public:
friend class LexicalCast<std::string, RedisConn>;
friend class LexicalCast<RedisConn, std::string>;
    RedisConn();
    RedisConn(std::string Wdress,std::string Rdress,std::string password);
    
    std::shared_ptr<sw::redis::Redis> getMaster();
    std::shared_ptr<sw::redis::Redis> getSlave();
private:
    std::string Wdress = "127.0.0.1:6379";
    std::string Rdress = "127.0.0.1:6379";
    std::string password = "defalut";
};



template<>
class LexicalCast<std::string, RedisConn>{

public:
    RedisConn operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        RedisConn p;
        if(!node["Wdress"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取Wdress失败";
            return p;
        }
        if(!node["Rdress"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取Rdress失败";
            return p;
        }
        if(!node["password"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取password失败";
            return p;
        }
        p.Wdress = node["Wdress"].as<std::string>();
        p.Rdress = node["Rdress"].as<std::string>();
        p.password = node["password"].as<std::string>();
        return p;        
    }
};

template<>
class LexicalCast<RedisConn, std::string>{
public:
    std::string operator() (const RedisConn & p){
        YAML::Node node;
        node["Wdress"] = p.Wdress;
        node["Rdress"] = p.Rdress;
        node["password"] = p.password;
        std::stringstream ss;
        ss << node;
        return ss.str();    
    }
};
typedef yk::Singleton<RedisConn> RedisConnMgr;

}

#endif