#include "http/ws_server.h"
#include "yk/log.h"

static yk::Logger::ptr g_logger = YK_LOG_ROOT();

void run() {
    yk::http::WSServer::ptr server(new yk::http::WSServer);
    yk::Address::ptr addr = yk::Address::LookupAnyIPAddress("0.0.0.0:8020");
    if(!addr) {
        YK_LOG_ERROR(g_logger) << "get address error";
        return;
    }
    auto fun = [](yk::http::HttpRequest::ptr header
                  ,yk::http::WSFrameMessage::ptr msg
                  ,yk::http::WSSession::ptr session) {
        session->sendMessage(msg);
        return 0;
    };

    server->getWSServletDispatch()->addServlet("/yk", fun);
    while(!server->bind(addr)) {
        YK_LOG_ERROR(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    server->start();
}

int main(int argc, char** argv) {
    yk::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
