#include "rock_server.h"
#include "yk/log.h"
#include "yk/module.h"

namespace yk {

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

RockServer::RockServer(const std::string& type
                       ,yk::IOManager* worker
                       ,yk::IOManager* io_worker
                       ,yk::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker) {
    setType(type);
}

void RockServer::handleClient(Socket::ptr client) {
    YK_LOG_DEBUG(g_logger) << "handleClient " << *client;
    yk::RockSession::ptr session(new yk::RockSession(client));
    session->setWorker(getWorker());
    ModuleMgr::GetInstance()->foreach(Module::ROCK,
            [session](Module::ptr m) {
        m->onConnect(session);
    });
    session->setDisconnectCb(
        [](AsyncSocketStream::ptr stream) {
             ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [stream](Module::ptr m) {
                m->onDisconnect(stream);
            });
        }
    );
    session->setRequestHandler(
        [](yk::RockRequest::ptr req
           ,yk::RockResponse::ptr rsp
           ,yk::RockStream::ptr conn)->bool {
            //YK_LOG_INFO(g_logger) << "handleReq " << req->toString()
            //                         << " body=" << req->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, req, rsp, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleRequest(req, rsp, conn);
            });
            return rt;
        }
    ); 
    session->setNotifyHandler(
        [](yk::RockNotify::ptr nty
           ,yk::RockStream::ptr conn)->bool {
            YK_LOG_INFO(g_logger) << "handleNty " << nty->toString()
                                     << " body=" << nty->getBody();
            bool rt = false;
            ModuleMgr::GetInstance()->foreach(Module::ROCK,
                    [&rt, nty, conn](Module::ptr m) {
                if(rt) {
                    return;
                }
                rt = m->handleNotify(nty, conn);
            });
            return rt;
        }
    );
    session->start();
}

}
