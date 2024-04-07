#ifndef __YK_APPLICATION_H__
#define __YK_APPLICATION_H__

#include "../http/http_server.h"

namespace yk {

class Application {
public:
    Application();

    static Application* GetInstance() { return s_instance;}
    bool init(int argc, char** argv);
    bool run();

    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs);
    void listAllServer(std::map<std::string, std::vector<TcpServer::ptr> >& servers);

private:
    int main(int argc, char** argv);
    int run_fiber();
private:
    int m_argc = 0;
    char** m_argv = nullptr;

    std::vector<http::HttpServer::ptr>  m_httpservers;
    IOManager::ptr m_mainIOManager;
    static Application* s_instance;

};

}

#endif
