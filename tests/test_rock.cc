#include "yk/yk.h"
#include "rock/rock_stream.h"

static yk::Logger::ptr g_logger = YK_LOG_ROOT();

yk::RockConnection::ptr conn(new yk::RockConnection);
void run() {
    conn->setAutoConnect(true);
    yk::Address::ptr addr = yk::Address::LookupAny("127.0.0.1:8061");
    if(!conn->connect(addr)) {
        YK_LOG_INFO(g_logger) << "connect " << *addr << " false";
    }
    conn->start();

    yk::IOManager::GetThis()->addTimer(1000, [](){
        yk::RockRequest::ptr req(new yk::RockRequest);
        static uint32_t s_sn = 0;
        req->setSn(++s_sn);
        req->setCmd(100);
        req->setBody("hello world sn=" + std::to_string(s_sn));

        auto rsp = conn->request(req, 300);
        if(rsp->response) {
            YK_LOG_INFO(g_logger) << rsp->response->toString();
        } else {
            YK_LOG_INFO(g_logger) << "error result=" << rsp->result;
        }
    }, true);
}

int main(int argc, char** argv) {
    yk::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
