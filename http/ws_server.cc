#include "ws_server.h"
#include "yk/log.h"

namespace yk {
namespace http {

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

WSServer::WSServer(yk::IOManager* worker, yk::IOManager* io_worker, yk::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) {
    m_dispatch.reset(new WSServletDispatch);
    setType("websocket_server");
}

void WSServer::handleClient(Socket::ptr client) {
    YK_LOG_DEBUG(g_logger) << "handleClient " << *client;
    WSSession::ptr session(new WSSession(client));
    do {
        // 进行 WebSocket 握手
        HttpRequest::ptr header = session->handleShake();
        // 如果握手失败，则记录错误日志并退出循环
        if(!header) {
            YK_LOG_DEBUG(g_logger) << "handleShake error";
            break;
        }
        WSServlet::ptr servlet = m_dispatch->getWSServlet(header->getPath());
        if(!servlet) {
            YK_LOG_DEBUG(g_logger) << "no match WSServlet";
            break;
        }
        int rt = servlet->onConnect(header, session);
        if(rt) {
            YK_LOG_DEBUG(g_logger) << "onConnect return " << rt;
            break;
        }
        while(true) {
            auto msg = session->recvMessage();
            if(!msg) {
                break;
            }
            rt = servlet->handle(header, msg, session);
            if(rt) {
                YK_LOG_DEBUG(g_logger) << "handle return " << rt;
                break;
            }
        }
        servlet->onClose(header, session);
    } while(0);
    session->close();
}

}
}
