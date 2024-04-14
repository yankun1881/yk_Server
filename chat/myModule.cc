#include "myModule.h"
#include "chat_servlet.h"
#include "resource_servlet.h"
#include "../yk/log.h"
#include "../yk/env.h"
#include "../yk/tcp_server.h"
#include "../yk/application.h"
#include "../http/http_server.h"
#include "../http/ws_server.h"
namespace chat
{
static yk::Logger::ptr g_logger = YK_LOG_ROOT();
 
MyModule::MyModule()
    :Module("chat_room", "1.0", ""){
}
bool MyModule::onLoad(){
    YK_LOG_INFO(g_logger) << "onLoad";
    return true;
}
bool MyModule::onUnload(){
    YK_LOG_INFO(g_logger) << "onUnload";
    return true;
}
bool MyModule::onServerReady(){
    YK_LOG_INFO(g_logger) << "onServerReady";
    std::vector<yk::TcpServer::ptr> svrs;
    if(!yk::Application::GetInstance()->getServer("http", svrs)) {
        YK_LOG_INFO(g_logger) << "no httpserver alive";
        return false;
    }

    for(auto& i : svrs) {
        yk::http::HttpServer::ptr http_server =
            std::dynamic_pointer_cast<yk::http::HttpServer>(i);
        if(!i) {
            continue;
        }
        auto slt_dispatch = http_server->getServletDispatch();

        yk::http::ResourceServlet::ptr slt(new yk::http::ResourceServlet(
                    yk::EnvMgr::GetInstance()->getCwd()
        ));
        slt_dispatch->addServlet("/html/index.html", slt);
        YK_LOG_INFO(g_logger) << "addServlet";
    }

    svrs.clear();
    if(!yk::Application::GetInstance()->getServer("ws", svrs)) {
        YK_LOG_INFO(g_logger) << "no ws alive";
        return false;
    }

    for(auto& i : svrs) {
        yk::http::WSServer::ptr ws_server =
            std::dynamic_pointer_cast<yk::http::WSServer>(i);

        yk::http::ServletDispatch::ptr slt_dispatch = ws_server->getWSServletDispatch();
        ChatWSServlet::ptr slt(new ChatWSServlet);
        slt_dispatch->addServlet("/yk/chat", slt);
    }
    return true;
}
bool MyModule::onServerUp(){
    YK_LOG_INFO(g_logger) << "onServerUp";
    return true;
}

extern "C" {

yk::Module* CreateModule() {
    yk::Module* module = new chat::MyModule;
    YK_LOG_INFO(chat::g_logger) << "CreateModule " << module;
    return module;
}

void DestoryModule(yk::Module* module) {
    YK_LOG_INFO(chat::g_logger) << "CreateModule " << module;
    delete module;
}


} 

}