#include"yk/daemon.h"
#include"yk/iomanager.h"
#include"yk/log.h"

static yk::Logger::ptr g_logger = YK_LOG_ROOT();

int server_main(int argc, char** argv){
    yk::IOManager iom(1);
    iom.addTimer(1000,[](){
        YK_LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
            if(++count > 10) {
                exit(1);
            }
    },true);
    return 0;
}

int main(int argc, char** argv){
    
    return yk::start_daemon(argc, argv, server_main, argc != 1);
}