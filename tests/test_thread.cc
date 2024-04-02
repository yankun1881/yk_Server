#include"yk/yk.h"
static yk::Logger::ptr g_logger = YK_LOG_NAME("root");
#include<time.h>


int count = 100000;

//yk::RWMutex s_mutex;
yk::RWMutex s_mutex;
//yk::Mutex s_mutex;
void fun1(){
    YK_LOG_INFO(g_logger) << "name: " << yk::Thread::GetName()
                            << " this.name: " << yk::Thread::GetThis()->getName()
                            <<"id: "<< yk::GetThreadId()
                            <<"  this.id : " << yk::Thread::GetThis()->getId(); 

    for(int i = 0; i < 10000; ++i)
    {
        yk::RWMutex::ReadLock lock(s_mutex);
        ++count;
    }

}

void fun2(){
    while(--count > 0){
        YK_LOG_INFO(g_logger) << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"<< yk::Thread::GetName();
        
    }
}

void fun3(){
    while(--count > 0){
        YK_LOG_INFO(g_logger) << "****************************************"<< yk::Thread::GetName();
        
    }
}


void fun4(){
    clock_t start;
    start = clock();
    YK_LOG_INFO(g_logger) << "thread test begin " ;
    YAML::Node root = YAML::LoadFile("/home/ubuntu/myServer/bin/conf/test_thread.yml");
    yk::Config::LoadFromYaml(root);
    std::vector<yk::Thread::ptr> thrs;
    for(int i = 0 ; i < 5; ++i ){
        yk::Thread::ptr thr(new yk::Thread(&fun2, "name_" + std::to_string(i)));
        yk::Thread::ptr thr1(new yk::Thread(&fun3, "name_" + std::to_string(i*10)));
        thrs.push_back(thr); 
        thrs.push_back(thr1); 
    }
    

    for(size_t i = 0 ; i < thrs.size(); ++i ){
        thrs[i]->join(); 
    }

    YK_LOG_INFO(g_logger) << "thread test end" << " time: " << clock()-start;

    YK_LOG_INFO(g_logger) << count;
}


int main(int argc, char** argv){
    fun4();


    return 0;
}