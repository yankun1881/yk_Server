#include "application.h"

#include <unistd.h>
#include <signal.h>

#include "yk/tcp_server.h"
#include "yk/daemon.h"
#include "yk/config.h"
#include "yk/env.h"
#include "yk/log.h"
#include "yk/module.h"
#include "yk/worker.h"
#include "http/ws_server.h"
#include "mysql/sql.h"

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
static yk::ConfigVar<yk::SQL>::ptr g_sql_value_config =
    yk::Config::Lookup("sql",yk::SQL(),"mysql"); 

/*struct HttpServerConf
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
};*/

static yk::ConfigVar<std::vector<TcpServerConf> >::ptr g_servers_conf
    = yk::Config::Lookup("servers", std::vector<TcpServerConf>(), "http server config");




/*template<>
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
};*/


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

    ModuleMgr::GetInstance()->init();

    std::string pidfile = g_server_work_path->getValue()
                                + "/" + g_server_pid_file->getValue();
    if(yk::FSUtil::IsRunningPidfile(pidfile)) {
        YK_LOG_ERROR(g_logger) << "server is running:" << pidfile;
        return false;
    }
    if(!yk::FSUtil::Mkdir(g_server_work_path->getValue())) {
        YK_LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
            << " errno=" << errno << " errstr=" << strerror(errno);
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
    YK_LOG_INFO(g_logger) << "main";
    
    {
        std::string pidfile = g_server_work_path->getValue()
                                    + "/" + g_server_pid_file->getValue();
        std::ofstream ofs(pidfile);
        if(!ofs) {
            YK_LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
            return false;
        }
        ofs << getpid();
    }

    m_mainIOManager.reset(new yk::IOManager(1, true, "main"));
    m_mainIOManager->schedule(std::bind(&Application::run_fiber, this));
    m_mainIOManager->addTimer(2000, [](){
    }, true);
    m_mainIOManager->stop();
    return 0;
}


int Application::run_fiber(){
    std::vector<Module::ptr> modules;
    ModuleMgr::GetInstance()->listAll(modules);
    bool has_error = false;
    for(auto& i : modules) {
        if(!i->onLoad()) {
            YK_LOG_ERROR(g_logger) << "module name="
                << i->getName() << " version=" << i->getVersion()
                << " filename=" << i->getFilename();
            has_error = true;
        }
    }
    if(has_error) {
        _exit(0);
    }

    yk::WorkerMgr::GetInstance()->init();
    auto sql = SQLMgr::GetInstance();
    *sql = g_sql_value_config->getValue();
    if(!sql->start()){
        YK_LOG_ERROR(g_logger) << "Database connection failure";
    }else{
        YK_LOG_INFO(g_logger) << "Database connection ready";    
    }
    auto http_confs = g_servers_conf->getValue();
    std::vector<TcpServer::ptr> svrs;
    for(auto& i : http_confs) {
        YK_LOG_DEBUG(g_logger) << std::endl << LexicalCast<TcpServerConf, std::string>()(i);

        std::vector<Address::ptr> address;
        for(auto& a : i.address) {
            size_t pos = a.find(":");
            if(pos == std::string::npos) {
                //YK_LOG_ERROR(g_logger) << "invalid address: " << a;
                address.push_back(UnixAddress::ptr(new UnixAddress(a)));
                continue;
            }
            int32_t port = atoi(a.substr(pos + 1).c_str());
            //127.0.0.1
            auto addr = yk::IPAddress::Create(a.substr(0, pos).c_str(), port);
            if(addr) {
                address.push_back(addr);
                continue;
            }
            std::vector<std::pair<Address::ptr, uint32_t> > result;
            if(yk::Address::GetInterfaceAddresses(result,
                                        a.substr(0, pos))) {
                for(auto& x : result) {
                    auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                    if(ipaddr) {
                        ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                    }
                    address.push_back(ipaddr);
                }
                continue;
            }

            auto aaddr = yk::Address::LookupAny(a);
            if(aaddr) {
                address.push_back(aaddr);
                continue;
            }
            YK_LOG_ERROR(g_logger) << "invalid address: " << a;
            _exit(0);
        }
        IOManager* accept_worker = yk::IOManager::GetThis();
        IOManager* io_worker = yk::IOManager::GetThis();
        IOManager* process_worker = yk::IOManager::GetThis();
        if(!i.accept_worker.empty()) {
            accept_worker = yk::WorkerMgr::GetInstance()->getAsIOManager(i.accept_worker).get();
            if(!accept_worker) {
                YK_LOG_ERROR(g_logger) << "accept_worker: " << i.accept_worker
                    << " not exists";
                _exit(0);
            }
        }
        if(!i.io_worker.empty()) {
            io_worker = yk::WorkerMgr::GetInstance()->getAsIOManager(i.io_worker).get();
            if(!io_worker) {
                YK_LOG_ERROR(g_logger) << "io_worker: " << i.io_worker
                    << " not exists";
                _exit(0);
            }
        }
        if(!i.process_worker.empty()) {
            process_worker = yk::WorkerMgr::GetInstance()->getAsIOManager(i.process_worker).get();
            if(!process_worker) {
                YK_LOG_ERROR(g_logger) << "process_worker: " << i.process_worker
                    << " not exists";
                _exit(0);
            }
        }

        TcpServer::ptr server;
        if(i.type == "http") {
            server.reset(new yk::http::HttpServer(i.keepalive,
                            process_worker, io_worker, accept_worker));
        }else if(i.type == "ws") {
            server.reset(new yk::http::WSServer(
                            process_worker, io_worker, accept_worker));
        }else {
            YK_LOG_ERROR(g_logger) << "invalid server type=" << i.type
                << LexicalCast<TcpServerConf, std::string>()(i);
            _exit(0);
        }
        if(!i.name.empty()) {
            server->setName(i.name);
        }
        std::vector<Address::ptr> fails;
        if(!server->bind(address, fails)) {
            for(auto& x : fails) {
                YK_LOG_ERROR(g_logger) << "bind address fail:"
                    << *x;
            }
            _exit(0);
        }
        //server->start();
        m_servers[i.type].push_back(server);
        svrs.push_back(server);
    }

    if(!g_service_discovery_zk->getValue().empty()) {

        std::vector<TcpServer::ptr> svrs;
        if(getServer("http", svrs)){
            std::string ip_and_port;
            for(auto& i : svrs) {
                auto socks = i->getSocks();
                for(auto& s : socks) {
                    auto addr = std::dynamic_pointer_cast<IPv4Address>(s->getLocalAddress());
                    if(!addr) {
                        continue;
                    }
                    auto str = addr->toString();
                    ip_and_port = addr->toString();
                }
                if(!ip_and_port.empty()) {
                    break;
                }
            }
        }
    }

    for(auto& i : modules) {
        i->onServerReady();
    }

    for(auto& i : svrs) {
        i->start();
    }
    for(auto& i : modules) {
        i->onServerUp();
    }
    return 0;
}

bool Application::getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs) {
    auto it = m_servers.find(type);
    if(it == m_servers.end()) {
        return false;
    }
    svrs = it->second;
    return true;
}

void Application::listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers) {
    servers = m_servers;
}

}
