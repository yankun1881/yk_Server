#include "ws_connection.h"
#include "yk/util.h"

namespace yk {
namespace http {

WSConnection::WSConnection(Socket::ptr sock, bool owner) 
    :HttpConnection(sock, owner) {
}

std::pair<HttpResult::ptr, WSConnection::ptr> WSConnection::Create(const std::string& url
                                       ,uint64_t timeout_ms
                                       ,const std::map<std::string, std::string>& headers) {
    Uri::ptr uri = Uri::Create(url);
    if(!uri) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL,
                    nullptr, "invalid url:" + url), nullptr);
    }
    return Create(uri, timeout_ms, headers);
}

std::pair<HttpResult::ptr, WSConnection::ptr> WSConnection::Create(Uri::ptr uri
                                    ,uint64_t timeout_ms
                                    ,const std::map<std::string, std::string>& headers) {
    // 获取地址对象
    Address::ptr addr = uri->createAddress();
    if(!addr) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST
                , nullptr, "invalid host: " + uri->getHost()), nullptr);
    }
    // 创建TCP套接字
    Socket::ptr sock = Socket::CreateTCP(addr);
    if(!sock) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR
                , nullptr, "create socket fail: " + addr->toString()
                        + " errno=" + std::to_string(errno)
                        + " errstr=" + std::string(strerror(errno))), nullptr);
    }
    // 连接服务器
    if(!sock->connect(addr)) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString()), nullptr);
    }
    // 设置接收超时时间
    sock->setRecvTimeout(timeout_ms);
    // 创建WebSocket连接对象
    WSConnection::ptr conn = std::make_shared<WSConnection>(sock);

    // 创建HTTP请求对象
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(HttpMethod::GET);
    bool has_host = false;
    bool has_conn = false;
    for(auto& i : headers) {
        if(strcasecmp(i.first.c_str(), "connection") == 0) {
            has_conn = true;
        } else if(!has_host && strcasecmp(i.first.c_str(), "host") == 0) {
            has_host = !i.second.empty();
        }
        // 设置HTTP头部
        req->setHeader(i.first, i.second);
    }
    // 设置WebSocket相关头部
    req->setWebsocket(true);
    if(!has_conn) {
        req->setHeader("connection", "Upgrade");
    }
    req->setHeader("Upgrade", "websocket");
    req->setHeader("Sec-webSocket-Version", "13");
    req->setHeader("Sec-webSocket-Key", yk::base64encode(random_string(16)));
    if(!has_host) {
        req->setHeader("Host", uri->getHost());
    }

   // 发送HTTP请求
   int rt = conn->sendRequest(req);
    if(rt == 0) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                , nullptr, "send request closed by peer: " + addr->toString()), nullptr);
    }
    if(rt < 0) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                    , nullptr, "send request socket error errno=" + std::to_string(errno)
                    + " errstr=" + std::string(strerror(errno))), nullptr);
    }
    // 接收HTTP响应
    auto rsp = conn->recvResponse();
    if(!rsp) {
        return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                    , nullptr, "recv response timeout: " + addr->toString()
                    + " timeout_ms:" + std::to_string(timeout_ms)), nullptr);
    }

    // 判断是否为WebSocket协议
    if(rsp->getStatus() != HttpStatus::SWITCHING_PROTOCOLS) {
        return std::make_pair(std::make_shared<HttpResult>(50
                    , rsp, "not websocket server " + addr->toString()), nullptr);
    }
    return std::make_pair(std::make_shared<HttpResult>((int)HttpResult::Error::OK
                , rsp, "ok"), conn);
}

WSFrameMessage::ptr WSConnection::recvMessage() {
    return WSRecvMessage(this, true);
}

int32_t WSConnection::sendMessage(WSFrameMessage::ptr msg, bool fin) {
    return WSSendMessage(this, msg, true, fin);
}

int32_t WSConnection::sendMessage(const std::string& msg, int32_t opcode, bool fin) {
    return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), true, fin);
}

int32_t WSConnection::ping() {
    return WSPing(this);
}

int32_t WSConnection::pong() {
    return WSPong(this);
}

}
}
