#include"yk/tcp_server.h"
#include"yk/iomanager.h"
#include"yk/socket.h"
#include"yk/log.h"
#include"yk/bytearray.h"

#include <unistd.h>

static yk::Logger::ptr g_logger = YK_LOG_ROOT();


class EchoServer : public yk::TcpServer{
public:
    EchoServer(int type);
    void handleClient(yk::Socket::ptr client) override;
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    : m_type(type){
}

void EchoServer::handleClient(yk::Socket::ptr client) {
    YK_LOG_INFO(g_logger) << "handleclient " << *client;
    yk::ByteArray::ptr ba(new yk::ByteArray);
    while (true) {
        ba->clear();
        std::vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);
        int rt = client->recv(&iovs[0], iovs.size());
        if (rt == 0) {
            YK_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        } else if (rt < 0) {
            YK_LOG_INFO(g_logger) << "client error rt=" << rt << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if (m_type == 1) { // text
            YK_LOG_INFO(g_logger) << ba->toString();
        } else {
            YK_LOG_INFO(g_logger) << ba->toHexString();
        }
        
    }
}

int type = 1;

void run() {
    YK_LOG_INFO(g_logger) << "server type= " << type ;
    EchoServer::ptr es(new EchoServer(type));
    auto addr = yk::IPAddress::LookupAny("0.0.0.0:8021");
    while (!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

// telnet 127.0.0.1 8021 发送指令
int main(int argc ,char** argv){
    if(argc < 2){
        YK_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] <<" -b]";
        return 0;
    }

    if(!strcmp(argv[1],"-b")){
        type = 2;
    }
    yk::IOManager iom(2);
    iom.schedule(run);
    return 0;
}