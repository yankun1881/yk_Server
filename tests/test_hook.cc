#include"yk/hook.h"
#include"yk/iomanager.h"
#include"yk/log.h"
#include <arpa/inet.h>
#include <string.h>

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

void test_sleep(){
    yk::IOManager iom(1);
    iom.schedule([](){
        sleep(2);
        YK_LOG_INFO( g_logger) <<"sleep 2";
    });
    iom.schedule([](){
        sleep(3);
        YK_LOG_INFO( g_logger) <<"sleep 3";
    });
    YK_LOG_INFO(g_logger) << "test_sleep";  
}

void test_sock(){
    int sock = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET,"153.3.238.102",&addr.sin_addr.s_addr);
    YK_LOG_INFO(g_logger) << "connect begin";
    
    int rt = connect(sock, (const sockaddr*)& addr, sizeof(addr));
    YK_LOG_INFO(g_logger) << "connect rt = " << rt <<" error= " << errno;
    if(rt){
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock,data,sizeof(data),0);
    if(rt <= 0){
        return;
    }
    
    std::string buff;
    buff.resize(10240);
    
    rt = recv(sock,&buff[0],buff.size(),0);
    YK_LOG_INFO(g_logger) << "recv  rt = " << rt << " errno = " << errno;
    if(rt <= 0){
        return ;
    }

    buff.resize(rt);
    YK_LOG_INFO(g_logger) << buff;
    


}



int main(int argc, char** argv){
    yk::IOManager iom;
    iom.schedule(test_sock);
    return 0;
}
