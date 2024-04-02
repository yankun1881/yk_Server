#include "yk/address.h"
#include "yk/log.h"

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");


void test(){ 
    std::vector<yk::Address::ptr> addrs;
    bool v = yk::Address::Lookup(addrs,"www.baidu.com:http");
    if(!v){
        YK_LOG_ERROR(g_logger) << "lookup fail";
        return ;
    }
    for(auto & x : addrs){
        YK_LOG_ERROR(g_logger) << x->toString() << std::endl;
    }
}

void test_iface(){
    std::multimap<std::string,std::pair<yk::Address::ptr,uint32_t> >results;
    bool v = yk::Address::GetInterfaceAddresses(results);
    if(!v){
        YK_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return ;
    }
    for(auto& i : results){
        YK_LOG_ERROR(g_logger) << i.first << "--" <<i.second.first->toString() <<" -" << i.second.second;
    }
}

void test_ipv4(){
    auto addr = yk::IPAddress::Create("www.baidu.com");
    if(addr){
        YK_LOG_INFO(g_logger) << addr->toString();
    }

}

int main(int argc, char** argv){
    //test();
    //test_iface();
    test_ipv4();
    return 0;
}


