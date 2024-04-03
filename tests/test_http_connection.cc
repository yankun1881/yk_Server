#include <iostream>
#include "yk/log.h"
#include "http/http_connection.h"
#include "yk/iomanager.h"
#include "yk/address.h"


static yk::Logger::ptr g_logger = YK_LOG_ROOT();

void test_pool(){
    yk::http::HttpConnectionPool::ptr pool(new yk::http::HttpConnectionPool(
        "www.baidu.com","",80,10,1000*30,5 ));
    yk::IOManager::GetThis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 300);
            YK_LOG_INFO(g_logger) << r->toString();
    }, true);
}


void run() {
    yk::Address::ptr addr = yk::Address::LookupAnyIPAddress("www.baidu.com:80"); // 逗号应该是点号
    if (!addr) {
        YK_LOG_INFO(g_logger) << "get addr error"; 
        return;
    }

    yk::Socket::ptr sock = yk::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if (!rt) {
        YK_LOG_INFO(g_logger) << "connect failed";
        return;
    }

    yk::http::HttpConnection::ptr conn(new yk::http::HttpConnection(sock)); // Httpconnection 应该是 HttpConnection
    yk::http::HttpRequest::ptr req(new yk::http::HttpRequest);
    //req->setPath("/blog/");
    req->setHeader("host","www.baidu.com");
    YK_LOG_INFO(g_logger) << "req:" << std::endl << *req; 
    conn->sendRequest(req);
    auto rsp = conn->recvResponse();
    if (!rsp) {
        YK_LOG_INFO(g_logger) << "recv response error"; 
        return;
    }
    YK_LOG_INFO(g_logger) << "rsp:" << std::endl << *rsp;

    std::ofstream ofs("rsp.dat");
    ofs << *rsp;


    YK_LOG_INFO(g_logger) << " =========================";

    auto r = yk::http::HttpConnection::DoGet("http://www.baidu.com/",300);
    YK_LOG_INFO(g_logger) << "result = " << r->result
        <<" error = " << r->error
        <<" rsp = " << (r->response ? r->response->toString() : "");
    test_pool();
}


int main(int argc,char** argv){
    yk::IOManager iom(2);
    iom.schedule(run);
}



