#ifndef __YK_ROCK_SERVER_H__
#define __YK_ROCK_SERVER_H__

#include "rock/rock_stream.h"
#include "yk/tcp_server.h"

namespace yk {

class RockServer : public TcpServer {
public:
    typedef std::shared_ptr<RockServer> ptr;
    RockServer(const std::string& type = "rock"
               ,yk::IOManager* worker = yk::IOManager::GetThis()
               ,yk::IOManager* io_worker = yk::IOManager::GetThis()
               ,yk::IOManager* accept_worker = yk::IOManager::GetThis());

protected:
    virtual void handleClient(Socket::ptr client) override;
};

}

#endif
