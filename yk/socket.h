#ifndef __YK__SOCKET_H__
#define __YK__SOCKET_H__

#include <memory>
#include "address.h"

namespace yk{

class Socket : public std::enable_shared_from_this<Socket>{
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> wptr;
    
    enum Type {
        /// TCP类型
        TCP = SOCK_STREAM,
        /// UDP类型
        UDP = SOCK_DGRAM
    };

    enum Family {
        /// IPv4 socket
        IPv4 = AF_INET,
        /// IPv6 socket
        IPv6 = AF_INET6,
        /// Unix socket
        UNIX = AF_UNIX,
    };
    static Socket::ptr CreateTCP(yk::Address::ptr address);
    static Socket::ptr CreateUDP(yk::Address::ptr address);
    static Socket::ptr CreateTCPSocket();
    static Socket::ptr CreateUDPSocket();
  
    static Socket::ptr CreateTCPSocket6();

   
    static Socket::ptr CreateUDPSocket6();

    static Socket::ptr CreateUnixTCPSocket();

    static Socket::ptr CreateUnixUDPSocket();


    // 构造函数，传入协议族、类型和协议
    Socket(int family, int type, int protocol = 0);
    ~Socket();

    // 获取发送超时时间
    int64_t getSendTimeout();
    // 设置发送超时时间
    void setSendTimeout(int64_t v);

    // 获取接收超时时间
    int64_t getRecvTimeout();
    // 设置接收超时时间
    void setRecvTimeout(int64_t v);

    // 初始化套接字
    bool init(int sock);

    // 获取套接字选项
    bool getOption(int level, int option, void* result, size_t* len);
    template<class T>
    bool getOption(int level, int option, T& result){
        size_t length = sizeof(T);
        return getOption(level, option, &result, length);
    }

    // 设置套接字选项
    bool setOption(int level, int option, void* result, socklen_t len);
    
    template<class T>
    bool setOption(int level, int option, T& result){
        size_t length = sizeof(T);
        return setOption(level, option, &result, length);
    }

    // 绑定地址
    bool bind(const Address::ptr addr);
    // 连接远程地址
    bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
    // 监听
    bool listen(int backlog = SOMAXCONN);
    // 接受连接
    Socket::ptr accept();
    // 关闭套接字
    bool close();

    // 发送数据
    int send(const void* buffer, size_t length, int flags = 0);
    int send(const iovec* buffers, size_t length, int flags = 0);
    int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0);

    // 接收数据
    int recv(void* buffer, size_t length, int flags = 0);
    int recv(iovec* buffers, size_t length, int flags = 0);
    int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
    int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0);

    // 获取远程地址
    Address::ptr getRemoteAddress();
    // 获取本地地址
    Address::ptr getLocalAddress();

    // 获取协议族
    int getFamily() const { return m_family; }
    // 获取类型
    int getType() const { return m_type; }
    // 获取协议
    int getProtocol() const { return m_protocol; }
    
    // 判断是否已连接
    bool isConnected() const { return m_isConnected; }
    // 判断套接字是否有效
    bool isValid() const;
    
    // 获取错误信息
    int getError();
    // 获取套接字描述符
    int getSocket() const{return m_sock;}
    
    // 打印套接字信息
    std::ostream& dump(std::ostream& os) const;
    
    // 取消读操作
    bool cancelRead();
    // 取消写操作
    bool cancelWrite();
    // 取消接受
    bool cancelAccept();
    // 取消所有操作
    bool cancelAll();

private:
    // 初始化套接字
    void initSock();
    // 创建新的套接字
    void newSock();

private:
    int m_sock;                 // 套接字描述符
    int m_family;               // 协议族
    int m_type;                 // 类型
    int m_protocol;             // 协议
    bool m_isConnected;         // 是否已连接

    Address::ptr m_localAddress;    // 本地地址
    Address::ptr m_remoteAddress;   // 远程地址

};
std::ostream& operator<< (std::ostream& os, const Socket& sock);
}

#endif
