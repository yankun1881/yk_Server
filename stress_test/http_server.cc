#include "http/http_server.h"
#include "yk/log.h"
yk::Logger::ptr g_logger = YK_LOG_ROOT();

void run(){
    yk::Address::ptr addr = yk::Address::LookupAnyIPAddress("0.0.0.0:8022");
    if(!addr){
        YK_LOG_ERROR(g_logger) << "get address error";
        return ; 
    }
    yk::http::HttpServer::ptr http_server(new yk::http::HttpServer(true));
    while(!http_server->bind(addr)){
        YK_LOG_DEBUG(g_logger) << "bind " << *addr << " fail";
        sleep(1);
    }
    http_server->start(); 
}


int main(int argc , char** argv ){
    yk::IOManager iom(3);
    iom.schedule(run);
}

