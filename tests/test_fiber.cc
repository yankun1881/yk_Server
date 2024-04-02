#include"yk/yk.h"
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

void run_in_fiber(){
    YK_LOG_INFO(g_logger) << "run_in_fiber begin";
    yk::Fiber::YieldToHold();
    YK_LOG_INFO(g_logger) << "run_in_fiber end";
    yk::Fiber::YieldToHold();
}

void test_fiber(){
    YK_LOG_INFO(g_logger) << "main begin -1";
    {
        sleep(1);
        yk::Fiber::GetThis();
        YK_LOG_INFO(g_logger) <<"main begin";
        yk::Fiber::ptr fiber(new yk::Fiber(run_in_fiber));
        fiber->swapIn();
        YK_LOG_INFO(g_logger) <<"main after swpIn";   
        fiber->swapIn();    
        sleep(1);
        YK_LOG_INFO(g_logger) <<"main after end"; 
        fiber->swapIn();       
    }
    YK_LOG_INFO(g_logger) << "main end 2";
}

void test_thread_fiber(){
    yk::Thread::SetName("main");
    std::vector<yk::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i){
        thrs.push_back(yk::Thread::ptr(
            new yk::Thread(&test_fiber,"name_"+std::to_string(i))
        ));
    }
    for(auto i: thrs){
        i->join();
    }
}

int main(int argc, char** argv){
    
    return 0;
}


