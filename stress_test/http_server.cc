#include "http/http_server.h"
#include "yk/log.h"
#include "yk/env.h"
yk::Logger::ptr g_logger = YK_LOG_ROOT();

void run(){
    yk::Address::ptr addr = yk::Address::LookupAnyIPAddress("127.0.0.1:8022");
    if(!addr){
        YK_LOG_ERROR(g_logger) << "get address error";
        return ; 
    }
    yk::http::HttpServer::ptr http_server(new yk::http::HttpServer(false));
    while(!http_server->bind(addr)){
        YK_LOG_DEBUG(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    http_server->start(); 
}


int main(int argc , char** argv ){
    yk::EnvMgr::GetInstance()->init(argc, argv);
    std::string conf_path = yk::EnvMgr::GetInstance()->getAbsolutePath(
        yk::EnvMgr::GetInstance()->get("c","conf")
    );
    std::cout << "log conf path: "<< conf_path;
    yk::Config::LoadFromConfDir(conf_path);
    yk::IOManager iom(2);
    iom.schedule(run);
}

