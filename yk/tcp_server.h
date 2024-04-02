#ifndef YK_TCP_SERVER
#define YK_TCP_SERVER


#include <memory>
#include <functional>
#include "iomanager.h"
#include "socket.h"
#include "noncopyable.h"

namespace yk
{


class TcpServer: public std::enable_shared_from_this<TcpServer> , Noncopyable{
public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(IOManager* woker = yk::IOManager::GetThis(),
                IOManager* accept_woker = yk::IOManager::GetThis());

    virtual ~TcpServer(){}
    virtual bool bind(yk::Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs,
            std::vector<Address::ptr>& fails);
    virtual bool start();
    virtual bool stop();

    uint64_t getReadTimeout() const {return m_readTimeout;}
    void setReadTimeout(uint64_t v) { m_readTimeout = v;}
    std::string getName()const {return m_name;}
    void setName(const std::string& v){m_name = v;}

    bool isStop(){return m_isStop;}
protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);
private:
    std::vector<Socket::ptr> m_socks;   //监听多地址
    IOManager* m_worker;
    IOManager* m_acceptWorker;  
    uint64_t m_readTimeout;             //设置超时时间
    std::string m_name;
    bool m_isStop;                      //是否停止

};

    
} 



#endif