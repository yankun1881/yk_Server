#ifndef __YK_APPLICATION_H__
#define __YK_APPLICATION_H__

#include "../http/http_server.h"
#include "service_discovery.h"

namespace yk {

class Application {
public:
    Application();

    static Application* GetInstance() { return s_instance;}
    bool init(int argc, char** argv);
    bool run();

    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs);
    void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);
    ZKServiceDiscovery::ptr getServiceDiscovery() const { return m_serviceDiscovery;}

private:
    int main(int argc, char** argv);
    int run_fiber();

    //启动线程
    int run_thread();
private:
    int m_argc = 0;
    char** m_argv = nullptr;

    std::map<std::string, std::vector<TcpServer::ptr> > m_servers;
    IOManager::ptr m_mainIOManager;
    ZKServiceDiscovery::ptr m_serviceDiscovery;

    static Application* s_instance;


};

}

#endif
