#include "tcp_server.h"
#include "config.h"



namespace yk
{
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

static yk::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout =
    yk::Config::Lookup("tcp_server.read_timeout",(uint64_t)(60*1000*2),
        "tcp server read timeout");


TcpServer::TcpServer(yk::IOManager* worker,
                    yk::IOManager* io_worker,
                    yk::IOManager* accept_worker)
    :m_worker(worker)
    ,m_ioWorker(io_worker)
    ,m_acceptWorker(accept_worker)
    ,m_readTimeout(g_tcp_server_read_timeout->getValue())
    ,m_name("yk/1.0.0")
    ,m_isStop(true) {
}

bool TcpServer::bind(yk::Address::ptr addr){
    std::vector<Address::ptr> addrs;
    addrs.push_back(addr);
    std::vector<Address::ptr> fails;
    return  bind(addrs,fails);
}
bool TcpServer::bind(const std::vector<Address::ptr>& addrs
                    ,std::vector<Address::ptr>& fails){
    bool rt = true;
    for(auto & addr : addrs){
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)){
            YK_LOG_ERROR(g_logger) << "bind fail errno = "
             << errno << " errstr = " << strerror(errno)
             <<" addr= [" << addr->toString() << "]";
            rt = false;
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()){
             YK_LOG_ERROR(g_logger) << "listen fail errno = "
             << errno << " errstr = " << strerror(errno)
             <<" addr= [ " << addr->toString() << "]";
            rt = false;
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
        
    }

    if(!rt){
        m_socks.clear();
        return false;
    }

    for(auto& i : m_socks){
        YK_LOG_INFO(g_logger) << "server bind success: " << *i;
    }

    return rt;
}
void TcpServer::startAccept(Socket::ptr sock){
    while(!m_isStop){
        Socket::ptr client = sock->accept();
        if(client){
            //bind 表达式，类的非静态对象，第二个参数需传递this指针，若为多线程，则需传递智能指针
            m_worker->schedule(std::bind(&TcpServer::handleClient,shared_from_this(),client));
        }else{
            YK_LOG_ERROR(g_logger) << "accept errno = "<< errno
                        << " errstr = " << strerror(errno);
        }

    }
}

void TcpServer::handleClient(Socket::ptr client){

}

bool TcpServer::start(){
    if(!m_isStop){
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks){
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                    shared_from_this(),sock));
    }
    return true;

}
bool TcpServer::stop(){
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this,self](){
        for(auto & sock:m_socks){
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
    return true;
}




} 



