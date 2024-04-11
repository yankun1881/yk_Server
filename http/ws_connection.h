#ifndef __YK_HTTP_WS_CONNECTION_H__
#define __YK_HTTP_WS_CONNECTION_H__

#include "http_connection.h" 
#include "ws_session.h" 

namespace yk {
namespace http {

class WSConnection : public HttpConnection {
public:
    typedef std::shared_ptr<WSConnection> ptr;

    // 构造函数，接受套接字指针和所有权标志作为参数
    WSConnection(Socket::ptr sock, bool owner = true);

    // 创建 WebSocket 连接的静态方法，接受 URL、超时时间和头部信息作为参数
    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(const std::string& url
                                    ,uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers = {});

    // 创建 WebSocket 连接的静态方法，接受 URI、超时时间和头部信息作为参数
    static std::pair<HttpResult::ptr, WSConnection::ptr> Create(Uri::ptr uri
                                    ,uint64_t timeout_ms
                                    , const std::map<std::string, std::string>& headers = {});

    // 接收消息的方法，返回 WebSocket 帧消息智能指针
    WSFrameMessage::ptr recvMessage();

    // 发送消息的方法，接受 WebSocket 帧消息智能指针和结束标志作为参数
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);

    // 发送消息的方法，接受消息内容、操作码和结束标志作为参数
    int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);

    // 发送 ping 消息的方法，返回发送的字节数
    int32_t ping();

    // 发送 pong 消息的方法，返回发送的字节数
    int32_t pong();
};

} // namespace http
} // namespace yk

#endif
