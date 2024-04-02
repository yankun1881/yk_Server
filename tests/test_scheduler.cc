#include"yk/yk.h"
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
static int s_count = 5;
void test_fiber(){
    YK_LOG_INFO(g_logger) << "test in fiber " << s_count;
    sleep(1);
    if(--s_count >= 0){
        yk::Scheduler::GetThis()->schedule(&test_fiber);
    }
}

int main(int argc, char** argv){
    YK_LOG_INFO(g_logger) << "main";
    yk::Scheduler sc(3,false,"test");
    sc.start();
    sleep(2);
    YK_LOG_INFO(g_logger) << "schedule";    
    sc.schedule(&test_fiber);
    sc.stop();
    YK_LOG_INFO(g_logger) << "over";
    return 0;
}


