#ifndef __YK_ZKCLIENT_H__
#define __YK_ZKCLIENT_H__

#include "config.h"
#include <string>
#include <zookeeper/zookeeper.h>

namespace yk {
/**
 * @brief ZooKeeper客户端类
 * 
 */
class ZkClient {
public:
friend class LexicalCast<std::string, ZkClient>;
friend class LexicalCast<ZkClient, std::string>;
    typedef std::shared_ptr<ZkClient> ptr;
    ZkClient();
    ~ZkClient();

    /**
     * @brief 启动客户端
     * 
     */
    void start();

    /**
     * @brief 创建Znode节点
     * 
     * @param path 路径
     * @param data 节点数据
     * @param flags 节点标志，可以是如下参数
     * 0: 表示永久性节点，默认情况
     * ZOO_EPHEMERAL: 临时性节点
     * ZOO_SEQUENCE: 路径名后添加唯一的、单调递增的序号
     */
    int create(const std::string& path, const std::string& data, int flags = 0);

    /**
     * @brief 获取节点数据
     * 
     * @param path 节点路径
     * @return std::string 
     */
    std::string getData(const std::string& path);
    
    //获取子节点数据
    void getChildren(std::vector<std::string>& children,const std::string& path);
    
    void close();

    int32_t exists(const std::string& path,bool b);

    const std::string getHosts(){return ip + ":" + std::to_string(port);}

    bool getEnable()const { return enable; }
private:
    zhandle_t* zkHandler_ = nullptr; // zookeeper句柄
    std::string ip = "127.0.0.1";
    uint64_t port = 2181;
    uint64_t timeout = 3000;
    bool enable = false;
};



template<>
class LexicalCast<std::string, ZkClient>{

public:
    ZkClient operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        ZkClient p;
        if(!node["ip"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取Wdress失败";
            return p;
        }
        if(!node["port"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取Rdress失败";
            return p;
        }
        if(!node["timeout"]){
            YK_LOG_ERROR(YK_LOG_ROOT()) << " 获取password失败";
            return p;
        }
        p.ip = node["ip"].as<std::string>();
        p.port = node["port"].as<uint64_t>();
        p.timeout = node["timeout"].as<uint64_t>();
        p.enable = true;
        return p;        
    }
};

template<>
class LexicalCast<ZkClient, std::string>{
public:
    std::string operator() (const ZkClient & p){
        YAML::Node node;
        node["ip"] = p.ip;
        node["port"] = p.port;
        node["timeout"] = p.timeout;
        std::stringstream ss;
        ss << node;
        return ss.str();    
    }
};
typedef yk::Singleton<ZkClient> ZkClientMgr;

class MyServer{
public:
friend class LexicalCast<std::string, MyServer>;
friend class LexicalCast<MyServer, std::string>;

    MyServer(){};
    ~MyServer(){};
    std::string getIp(){return ip;}
    int32_t getPort(){return port;}
    std::string getName(){return name;}
private:
    std::string ip;
    int32_t port;
    std::string name;
};

template<>
class LexicalCast<std::string, MyServer>{

public:
    MyServer operator() (const std::string& v){
        YAML::Node node = YAML::Load(v);
        MyServer p;
        if(!node["ip"]){
            return p;
        }
        if(!node["port"]){
            return p;
        }
        if(!node["name"]){
            return p;
        }
        p.ip = node["ip"].as<std::string>();
        p.port = node["port"].as<int32_t>();
        p.name = node["name"].as<std::string>();
        return p;        
    }
};

template<>
class LexicalCast<MyServer, std::string>{
public:
    std::string operator() (const MyServer & p){
        YAML::Node node;
        node["ip"] = p.ip;
        node["port"] = p.port;
        node["name"] = p.name;
        std::stringstream ss;
        ss << node;
        return ss.str();    
    }
};
typedef yk::Singleton<MyServer> MyServerMgr;







}
#endif // __YK_ZKCLIENT_H__