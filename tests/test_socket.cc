#include"yk/socket.h"
#include"yk/yk.h"

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
void test_socket(){
    yk::IPAddress::ptr addr = yk::Address::LookupAnyIPAddress("www.baidu.com");
    if(addr){
        YK_LOG_INFO(g_logger) << "ip = :" << addr->toString();
    }else{
        YK_LOG_ERROR(g_logger) << "get address fail";
    }
    
    yk::Socket::ptr sock = yk::Socket::CreateTCP(addr);
    addr->setPort(80);
    YK_LOG_INFO(g_logger) << "ip = :" << addr->toString();
    if(!sock->connect(addr)){
        YK_LOG_ERROR(g_logger) << "connet "<< addr->toString()<<" fail";
    }else {
        YK_LOG_ERROR(g_logger) << "connet" << addr->toString() << " connected";
    }
    
    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(data,sizeof(data));
    if(rt <= 0){
        return;
    }
    
    std::string buff;
    buff.resize(4096);
    
    rt = sock->recv(&buff[0],buff.size());
    YK_LOG_INFO(g_logger) << "recv  rt = " << rt << " errno = " << errno;
    if(rt <= 0){
        return ;
    }

    buff.resize(rt);
    YK_LOG_INFO(g_logger) << buff;

    
}
int main(int argc, char** argv){
    yk::IOManager iom;
    iom.schedule(&test_socket); 
    return 0;
}