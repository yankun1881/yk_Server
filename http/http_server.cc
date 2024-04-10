
#include "http_server.h"
#include "http_servlet.h"
#include "yk/log.h"

namespace yk{

namespace http
{
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive
               ,yk::IOManager* worker
               ,yk::IOManager* io_worker
               ,yk::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker)
    ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);
    setType("http");
}

void HttpServer::handleClient(Socket::ptr client){
    YK_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do{
        auto req = session->recvRequest();
        if(!req){
            YK_LOG_WARN(g_logger) << "recv http requst fail, errno = "
                                    << errno << " errstr = " << strerror(errno)
                                    <<" client: " << *client;
            break;
        }

        HttpResponse::ptr rsp( new HttpResponse(req->getVersion(),req->isClose()||!m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    }while(m_isKeepalive);
    session->close();
}




}


}