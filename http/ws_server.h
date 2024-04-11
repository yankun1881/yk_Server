#ifndef __YK_HTTP_WS_SERVER_H__
#define __YK_HTTP_WS_SERVER_H__

#include "yk/tcp_server.h"
#include "ws_session.h" 
#include "ws_servlet.h" 

namespace yk {
namespace http {

class WSServer : public TcpServer {
public:
    typedef std::shared_ptr<WSServer> ptr; // WebSocket 服务器智能指针类型

    // 构造函数，接受工作线程、IO 线程和接受线程作为参数
    WSServer(yk::IOManager* worker = yk::IOManager::GetThis()
             , yk::IOManager* io_worker = yk::IOManager::GetThis()
             , yk::IOManager* accept_worker = yk::IOManager::GetThis());

    // 获取 WebSocket Servlet 调度器的方法
    WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch;}

    // 设置 WebSocket Servlet 调度器的方法
    void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v;}
protected:
    // 处理客户端连接的方法，重载自父类
    virtual void handleClient(Socket::ptr client) override;
protected:
    WSServletDispatch::ptr m_dispatch; // WebSocket Servlet 调度器
};

} // namespace http
} // namespace yk

#endif
