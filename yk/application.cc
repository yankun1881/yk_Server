#include "application.h"

#include <unistd.h>
#include <signal.h>

#include "yk/tcp_server.h"
#include "yk/daemon.h"
#include "yk/config.h"
#include "yk/env.h"
#include "yk/log.h"

namespace yk {

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

static yk::ConfigVar<std::string>::ptr g_server_work_path =
    yk::Config::Lookup("server.work_path"
            ,std::string("/home/ubuntu/work/yk")
            , "server work path");

static yk::ConfigVar<std::string>::ptr g_server_pid_file =
    yk::Config::Lookup("server.pid_file"
            ,std::string("yk.pid")
            , "server pid file");

static yk::ConfigVar<std::string>::ptr g_service_discovery_zk =
    yk::Config::Lookup("service_discovery.zk"
            ,std::string("")
            , "service discovery zookeeper");
struct HttpServerConf
{
    std::vector<std::string> address;   
    int keepalive = 0;
    int timeout = 1000*2*60;
    std::string name;
    bool isValid()const{
        return !address.empty();
    }
    bool operator == (const HttpServerConf & oth) const{
        return address == oth.address
                && keepalive == oth.keepalive
                && timeout == oth.timeout
                && name == oth.name;
    }
};

static yk::ConfigVar<std::vector<HttpServerConf> >::ptr g_http_servers_conf
    = yk::Config::Lookup("http_servers", std::vector<HttpServerConf>(), "http server config");




template<>
class LexicalCast<std::string,HttpServerConf>{
public:
    HttpServerConf operator()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        HttpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        if(node["address"].IsDefined()){
            for(size_t i = 0; i < node["address"].size();++i){
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

template<>
class LexicalCast<HttpServerConf,std::string>{
public:
    std::string operator()(const HttpServerConf& conf){
        YAML::Node node ;
        node["name"] = conf.name;
        node["timeout"] = conf.timeout;
        node["keepalive"] = conf.keepalive;
        for(auto & i : conf.address){
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
}

bool Application::init(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;

    yk::EnvMgr::GetInstance()->addHelp("s", "start with the terminal");
    yk::EnvMgr::GetInstance()->addHelp("d", "run as daemon");
    yk::EnvMgr::GetInstance()->addHelp("c", "conf path default: ./conf");
    yk::EnvMgr::GetInstance()->addHelp("p", "print help");

    if(!yk::EnvMgr::GetInstance()->init(argc, argv)) {
        yk::EnvMgr::GetInstance()->printHelp();
        return false;
    }
    if(yk::EnvMgr::GetInstance()->has("p")) {
        yk::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    int run_type = 0;
    if(yk::EnvMgr::GetInstance()->has("s")) {
        run_type = 1;
    }
    if(yk::EnvMgr::GetInstance()->has("d")) {
        run_type = 2;
    }

    if(run_type == 0) {
        yk::EnvMgr::GetInstance()->printHelp();
        return false;
    }

    std::string conf_path = yk::EnvMgr::GetInstance()->getAbsolutePath(
        yk::EnvMgr::GetInstance()->get("c","conf")
    );
    YK_LOG_INFO(g_logger)<< "log conf path: "<< conf_path;
    yk::Config::LoadFromConfDir(conf_path);

    if(!yk::FSUtil::Mkdir(g_server_work_path->getValue())){
        YK_LOG_INFO(g_logger)<< "creat work path [" << g_server_work_path->getValue()
            << " errno = " << errno << " errstr = " << strerror(errno);
        return false;
    }


    return true;
}

bool Application::run() {
    bool is_daemon = yk::EnvMgr::GetInstance()->has("d");
    return start_daemon(m_argc, m_argv,
            std::bind(&Application::main, this, std::placeholders::_1,
                std::placeholders::_2), is_daemon);
}

int Application::main(int argc, char** argv) {
    std::string pidfile = g_server_work_path->getValue()
                                + "/" + g_server_pid_file->getValue();
    if(FSUtil::IsRunningPidfile(pidfile)){
        YK_LOG_ERROR(g_logger) << "server is running : " << pidfile;
        return false;
    }
    std::ofstream ofs(pidfile);
    if(!ofs){
        YK_LOG_INFO(g_logger) << " open pidfile " << pidfile << " failed ";
        return false;
    }
    ofs << getpid();
    
    yk::IOManager iom(1);
    iom.schedule(std::bind(&Application::run_fiber,this));
    iom.stop(); 
    return 0;
}


int Application::run_fiber(){
    auto http_confs = g_http_servers_conf->getValue();

    for(auto & i : http_confs){
        YK_LOG_INFO(g_logger) << LexicalCast<HttpServerConf,std::string>()(i);
        std::vector<Address::ptr> address;
        for(auto& a : i.address){
            size_t pos = a.find(":");
            if(pos == std::string::npos){
                YK_LOG_ERROR(g_logger) << " invalid address : " << a;
                continue;
            }
            auto addr = Address::LookupAnyIPAddress(a);
            if(addr){
                address.push_back(addr);
                continue;
            }
            std::vector<std::pair<Address::ptr,uint32_t>>result;
            if(!yk::Address::GetInterfaceAddresses(result,a.substr(0,pos))){
                YK_LOG_ERROR(g_logger) << "invalid address : " << a;
                continue;
            }
            for(auto& x : result){
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                if(ipaddr){
                    ipaddr->setPort(atoi(a.substr(pos+1).c_str()));
                }
                address.push_back(ipaddr);
            }
        }
        yk::http::HttpServer::ptr server(new yk::http::HttpServer(i.keepalive));
        std::vector<Address::ptr> fails;
        if(server->bind(address,fails)){
            for(auto & x : fails){
                YK_LOG_ERROR(g_logger) << "bind address fail = "
                        << *x;
            }
        }
        server->start();
        m_httpservers.push_back(server);
    }
    return 0;
}


}
