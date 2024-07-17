#ifndef __YK_CONN_LOOP_H__
#define __YK_CONN_LOOP_H__

#include<memory>
#include <list>
#include "thread"
#include "sql.h"
#include "yk/timer.h"
namespace yk{

class ConnPool {
    
friend class LexicalCast<std::string, ConnPool>;
friend class LexicalCast<ConnPool, std::string>;
public:
    typedef std::shared_ptr<ConnPool> ptr;
    typedef yk::Mutex MutexType;
    int init();        //读取日志之后必须初始化
    ConnPool() = default;
    Conn::ptr getConn(); // 从连接池中取出一个连接
    ~ConnPool(); // 析构函数
    int addConn(); // 增加连接
    void produceConn(); // 生产数据库连接
    void recycleConn(); // 销毁数据库连接
    bool getStatus(){return m_status;}
private:
    size_t m_minSize = 1;  //最小连接数
    uint64_t m_timeout = 1000*60*10; //单位ms
	std::string dress;
	std::string user;
	std::string password;
	std::string database;
    std::list<Conn*> m_conns;    //连接池
    sql::mysql::MySQL_Driver* driver = nullptr;
    bool m_status = false;;
    MutexType m_mutex;
};
template<>
class LexicalCast<std::string, ConnPool>{

public:
    ConnPool operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        ConnPool p;
        if(!node["dress"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取dress失败";
            return p;
        }
        if(!node["user"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取user失败";
            return p;
        }
        if(!node["password"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取password失败";
            return p;
        }
        if(!node["database"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取database失败";
            return p;
        }
        if(!node["minSize"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取database失败";
            return p;
        }
        if(!node["timeout"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取database失败";
            return p;
        }
        p.dress = node["dress"].as<std::string>();
        p.user = node["user"].as<std::string>();
        p.password = node["password"].as<std::string>();
        p.database = node["database"].as<std::string>();
        p.m_minSize = node["minSize"].as<size_t>(); 
        p.m_timeout = node["timeout"].as<uint64_t>(); 
        return p;        
    }
};

template<>
class LexicalCast<ConnPool, std::string>{
public:
    std::string operator() (const ConnPool & p){
        YAML::Node node;
        node["dress"] = p.dress;
        node["user"] = p.user;
        node["password"] = p.password;
        node["database"] = p.database;
        node["minSize"] = p.m_minSize;
        node["timeout"] = p.m_timeout;
        std::stringstream ss;
        ss << node;
        return ss.str();    
    }
};
typedef yk::Singleton<ConnPool> ConnPoolMgr;

}
#endif