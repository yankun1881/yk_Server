#include "http/ws_connection.h"
#include "yk/iomanager.h"
#include "yk/util.h"

void run() {
    auto rt = yk::http::WSConnection::Create("http://127.0.0.1:8020/yk", 1000);
    if(!rt.second) {
        std::cout << rt.first->toString() << std::endl;
        return;
    }

    auto conn = rt.second;
    while(true) {
        //for(int i = 0; i < 1100; ++i) {
        for(int i = 0; i < 1; ++i) {
            conn->sendMessage(yk::random_string(60), yk::http::WSFrameHead::TEXT_FRAME, false);
        }
        conn->sendMessage(yk::random_string(65), yk::http::WSFrameHead::TEXT_FRAME, true);
        auto msg = conn->recvMessage();
        if(!msg) {
            break;
        }
        std::cout << "opcode=" << msg->getOpcode()
                  << " data=" << msg->getData() << std::endl;

        sleep(10);
    }
}

int main(int argc, char** argv) {
    srand(time(0));
    yk::IOManager iom(1);
    iom.schedule(run);
    return 0;
}
