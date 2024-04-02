#include"yk/tcp_server.h"
#include"yk/iomanager.h"
#include"yk/log.h"
#include <unistd.h>

static yk::Logger::ptr g_logger = YK_LOG_ROOT();
void run(){
    auto addr =  yk::IPAddress::LookupAny("0.0.0.0:8033");
    auto addr2 = yk::UnixAddress::ptr(new yk::UnixAddress("/tmp/unix_addr"));

    std::vector<yk::Address::ptr> addrs;
    addrs.push_back(addr);
    addrs.push_back(addr2);
    yk::TcpServer::ptr tcp_server(new yk::TcpServer);
    std::vector<yk::Address::ptr> fails;
    while(!tcp_server->bind(addrs,fails)){
        sleep(2);
    }
    tcp_server->start();
}

int main(int argc ,char** argv){
    yk::IOManager iom(2);
    iom.schedule(run);
    return 0;
}