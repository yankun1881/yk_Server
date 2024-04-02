#include "../yk/yk.h"
#include "../yk/iomanager.h"
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h> 
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
int sock = 0;
void test_fiber(){
    YK_LOG_INFO(g_logger) << "test_fiber";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "153.3.238.110", &addr.sin_addr.s_addr);
    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))){
    }else if(errno ==   EINPROGRESS){
        YK_LOG_INFO(g_logger) << "errno = EINPROGRESS";
        yk::IOManager::GetThis()->addEvent(sock,yk::IOManager::READ,[](){
            YK_LOG_INFO(g_logger) << "read callback";
        });
        yk::IOManager::GetThis()->addEvent(sock,yk::IOManager::WRITE,[](){
            YK_LOG_INFO(g_logger) << "write callback";
            
            yk::IOManager::GetThis()->cancelEvent(sock,yk::IOManager::READ);
            close(sock);
        });
        
    }else{
        YK_LOG_INFO(g_logger) << "error";
    }


}


void test1(){
    yk::IOManager iom(2,false);
    iom.schedule(&test_fiber);
}   

yk::Timer::ptr timer;
void test_timer(){
    yk::IOManager iom(2);
    timer =  iom.addTimer(1000,[](){
        static int i = 0;
        YK_LOG_INFO(g_logger) << " hello timer i = " << i;
        if(++i == 3){
            timer->reset(2000,true);
            //timer->cancel();
        }
    },true);
}

int main(){
    //test1();
    test_timer();
    return 0;
}