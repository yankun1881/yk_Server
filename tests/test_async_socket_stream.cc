#include "streams/async_socket_stream.h"
#include "yk/socket.h"
#include <iostream>
using namespace yk;
static void test() {
    // 创建一个Socket对象
    Socket::ptr socket = std::make_shared<Socket>(SOCK_STREAM,AF_INET);
    if (!socket->connect("127.0.0.1", 80)) {
        std::cerr << "Failed to connect to server." << std::endl;
        return;
    }

    // 创建一个AsyncSocketStream对象
    AsyncSocketStream::ptr stream = std::make_shared<AsyncSocketStream>(socket);

    // 设置连接和断开连接的回调函数
    stream->setConnectCb([](AsyncSocketStream::ptr s) -> bool {
        std::cout << "Connected to server." << std::endl;
        return true;
    });

    stream->setDisconnectCb([](AsyncSocketStream::ptr s) {
        std::cout << "Disconnected from server." << std::endl;
    });

    // 启动异步读写操作
    if (!stream->start()) {
        std::cerr << "Failed to start async operations." << std::endl;
        return;
    }

    // 模拟发送数据
    std::string testData = "Hello, server!";
    stream->setData(testData);
    stream->enqueue(std::make_shared<AsyncSocketStream::Ctx>());

    // 模拟接收数据
    stream->setAutoConnect(true);
    stream->addCtx(std::make_shared<AsyncSocketStream::Ctx>());

    // 等待一定时间，观察异步操作的结果
    std::this_thread::sleep_for(std::chrono::seconds(10));
    stream->close();
}

int main() {
    test();
    return 0;
}