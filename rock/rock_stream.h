#ifndef __YK_ROCK_ROCK_STREAM_H__
#define __YK_ROCK_ROCK_STREAM_H__

#include "streams/async_socket_stream.h"
#include "rock_protocol.h"
#include <boost/any.hpp>

namespace yk {

struct RockResult {
   typedef std::shared_ptr<RockResult> ptr; 
   RockResult(int32_t _result, int32_t _used, RockResponse::ptr rsp, RockRequest::ptr req)
    :result(_result)
    ,used(_used)
    ,response(rsp)
    ,request(req) {
   }
   int32_t result;  // 结果码
   int32_t used;    // 使用时间
   RockResponse::ptr response;  // Rock 响应
   RockRequest::ptr request;    // Rock 请求

   std::string toString() const;  // 将结果转换为字符串的方法
};

class RockStream : public yk::AsyncSocketStream {
public:
    typedef std::shared_ptr<RockStream> ptr;
    typedef std::function<bool(yk::RockRequest::ptr
                               ,yk::RockResponse::ptr
                               ,yk::RockStream::ptr)> request_handler;  // 请求处理器的类型
    typedef std::function<bool(yk::RockNotify::ptr
                               ,yk::RockStream::ptr)> notify_handler;     // 通知处理器的类型

    RockStream(Socket::ptr sock);  // 构造函数
    ~RockStream();                 // 析构函数

    int32_t sendMessage(Message::ptr msg);  // 发送消息的方法
    RockResult::ptr request(RockRequest::ptr req, uint32_t timeout_ms);  // 发起请求的方法

    request_handler getRequestHandler() const { return m_requestHandler;}  // 获取请求处理器
    notify_handler getNotifyHandler() const { return m_notifyHandler;}     // 获取通知处理器

    void setRequestHandler(request_handler v) { m_requestHandler = v;}  // 设置请求处理器
    void setNotifyHandler(notify_handler v) { m_notifyHandler = v;}     // 设置通知处理器

    template<class T>
    void setData(const T& v) {  // 设置任意类型的数据
        m_data = v;
    }

    template<class T>
    T getData() {  // 获取任意类型的数据
        try {
            return boost::any_cast<T>(m_data);
        } catch(...) {
        }
        return T();
    }
protected:
    struct RockSendCtx : public SendCtx {  // Rock 发送上下文
        typedef std::shared_ptr<RockSendCtx> ptr;
        Message::ptr msg;

        virtual bool doSend(AsyncSocketStream::ptr stream) override;  // 实际发送消息的方法
    };

    struct RockCtx : public Ctx {  // Rock 上下文
        typedef std::shared_ptr<RockCtx> ptr;
        RockRequest::ptr request;
        RockResponse::ptr response;

        virtual bool doSend(AsyncSocketStream::ptr stream) override;  // 实际发送请求的方法
    };

    virtual Ctx::ptr doRecv() override;  // 接收消息的方法

    void handleRequest(yk::RockRequest::ptr req);  // 处理请求的方法
    void handleNotify(yk::RockNotify::ptr nty);    // 处理通知的方法
private:
    RockMessageDecoder::ptr m_decoder;       // Rock 消息解码器
    request_handler m_requestHandler;        // 请求处理器
    notify_handler m_notifyHandler;          // 通知处理器
    boost::any m_data;                       // 任意类型的数据
};


class RockSession : public RockStream {
public:
    typedef std::shared_ptr<RockSession> ptr;
    RockSession(Socket::ptr sock);  // 构造函数
};


class RockConnection : public RockStream {
public:
    typedef std::shared_ptr<RockConnection> ptr;
    RockConnection();                // 构造函数
    bool connect(yk::Address::ptr addr);  // 连接到指定地址的方法
};



}

#endif
