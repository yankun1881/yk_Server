#ifndef __YK_MODULE_H__
#define __YK_MODULE_H__

#include "yk/stream.h" 
#include "yk/singleton.h" 
#include "yk/thread.h" 
#include "rock/rock_protocol.h"
#include "rock/rock_stream.h"
#include <map> 
#include <unordered_map> 
namespace yk {

class Module {
public:
    enum Type {
        MODULE = 0, // 普通模块
        ROCK = 1,   // Rock 模块
    };
    typedef std::shared_ptr<Module> ptr;
        Module(const std::string& name
            ,const std::string& version
            ,const std::string& filename
            ,uint32_t type = MODULE);
    virtual ~Module() {}

    // 在解析命令行参数之前执行的函数
    virtual void onBeforeArgsParse(int argc, char** argv);
    // 在解析命令行参数之后执行的函数
    virtual void onAfterArgsParse(int argc, char** argv);

    // 加载模块的函数
    virtual bool onLoad();
    // 卸载模块的函数
    virtual bool onUnload();

    // 处理连接的函数
    virtual bool onConnect(yk::Stream::ptr stream);
    // 处理断开连接的函数
    virtual bool onDisconnect(yk::Stream::ptr stream);
    
    // 服务器准备就绪时执行的函数
    virtual bool onServerReady();
    // 服务器启动时执行的函数
    virtual bool onServerUp();

    // 获取模块状态字符串的函数
    virtual std::string statusString();

    // 获取模块名称
    const std::string& getName() const { return m_name;}
    // 获取模块版本
    const std::string& getVersion() const { return m_version;}
    // 获取模块文件名
    const std::string& getFilename() const { return m_filename;}
    // 获取模块ID
    const std::string& getId() const { return m_id;}

    // 设置模块文件名
    void setFilename(const std::string& v) { m_filename = v;}

    // 获取模块类型
    uint32_t getType() const { return m_type;}
    
    virtual bool handleRequest(yk::Message::ptr req
                               ,yk::Message::ptr rsp
                               ,yk::Stream::ptr stream);
    virtual bool handleNotify(yk::Message::ptr notify
                              ,yk::Stream::ptr stream);
    // 注册服务的函数
    void registerService(const std::string& server_type,
            const std::string& domain, const std::string& service);
protected:
    std::string m_name; // 模块名称
    std::string m_version; // 模块版本
    std::string m_filename; // 模块文件名
    std::string m_id; // 模块ID
    uint32_t m_type; // 模块类型
};

class RockModule : public Module {
public:
    typedef std::shared_ptr<RockModule> ptr;
    RockModule(const std::string& name
               ,const std::string& version
               ,const std::string& filename);

    virtual bool handleRockRequest(yk::RockRequest::ptr request
                        ,yk::RockResponse::ptr response
                        ,yk::RockStream::ptr stream) = 0;
    virtual bool handleRockNotify(yk::RockNotify::ptr notify
                        ,yk::RockStream::ptr stream) = 0;

    virtual bool handleRequest(yk::Message::ptr req
                               ,yk::Message::ptr rsp
                               ,yk::Stream::ptr stream);
    virtual bool handleNotify(yk::Message::ptr notify
                              ,yk::Stream::ptr stream);

};

class ModuleManager {
public:
    typedef RWMutex RWMutexType;

    ModuleManager();

    void add(Module::ptr m);
    void del(const std::string& name);
    void delAll();
    void init();

    Module::ptr get(const std::string& name);

    // 处理连接的函数
    void onConnect(Stream::ptr stream);
    // 处理断开连接的函数
    void onDisconnect(Stream::ptr stream);

    // 获取所有模块的函数
    void listAll(std::vector<Module::ptr>& ms);
    // 根据类型获取模块的函数
    void listByType(uint32_t type, std::vector<Module::ptr>& ms);
    // 遍历指定类型的模块的函数
    void foreach(uint32_t type, std::function<void(Module::ptr)> cb);
private:
    // 初始化模块的私有函数
    void initModule(const std::string& path);
private:
    RWMutexType m_mutex;                                                // 读写锁
    std::unordered_map<std::string, Module::ptr> m_modules;             // 模块映射
    std::unordered_map<uint32_t
        ,std::unordered_map<std::string, Module::ptr> > m_type2Modules; // 类型到模块的映射
};

// 定义模块管理器单例
typedef yk::Singleton<ModuleManager> ModuleMgr;

}

#endif
