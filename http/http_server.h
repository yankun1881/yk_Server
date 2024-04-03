#ifndef __YK_HTTP_SERVER_H__
#define __YK_HTTP_SERVER_H__

#include "yk/tcp_server.h"
#include "http/http_session.h"
#include "http/http_servlet.h"

namespace yk{

namespace http
{
    
class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false
            ,IOManager* worker = IOManager::GetThis()
            ,IOManager* accept_worker = IOManager::GetThis() );

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}

protected:
    virtual void handleClient(Socket::ptr client) override;
    
private:
    bool m_isKeepalive; //是否为长连接
    ServletDispatch::ptr m_dispatch;
};

}



}


#endif