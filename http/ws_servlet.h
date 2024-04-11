#ifndef __HTTP_WS_SERVLET_H__
#define __HTTP_WS_SERVLET_H__

#include "ws_session.h" // 引入 WebSocket 会话相关的头文件
#include "yk/thread.h" // 引入线程相关的头文件
#include "http_servlet.h" // 引入 HTTP Servlet 相关的头文件

namespace yk {
namespace http {

// WebSocket Servlet 基类，继承自 Servlet
class WSServlet : public Servlet {
public:
    typedef std::shared_ptr<WSServlet> ptr; // WebSocket Servlet 智能指针类型

    // 构造函数，接受名称作为参数
    WSServlet(const std::string& name)
        :Servlet(name) {
    }

    // 虚析构函数
    virtual ~WSServlet() {}

    // 处理 HTTP 请求的方法，重载自父类，WebSocket 无需实现该方法
    virtual int32_t handle(yk::http::HttpRequest::ptr request
                   , yk::http::HttpResponse::ptr response
                   , yk::http::HttpSession::ptr session) override {
        return 0;
    }

    // WebSocket 连接建立时调用的方法，纯虚函数
    virtual int32_t onConnect(yk::http::HttpRequest::ptr header
                              ,yk::http::WSSession::ptr session) = 0;

    // WebSocket 连接关闭时调用的方法，纯虚函数
    virtual int32_t onClose(yk::http::HttpRequest::ptr header
                             ,yk::http::WSSession::ptr session) = 0;

    // 处理 WebSocket 消息的方法，纯虚函数
    virtual int32_t handle(yk::http::HttpRequest::ptr header
                           ,yk::http::WSFrameMessage::ptr msg
                           ,yk::http::WSSession::ptr session) = 0;

    // 获取 WebSocket Servlet 的名称
    const std::string& getName() const { return m_name;}
protected:
    std::string m_name; // WebSocket Servlet 的名称
};

// 使用函数作为 WebSocket Servlet 的处理方法
class FunctionWSServlet : public WSServlet {
public:
    typedef std::shared_ptr<FunctionWSServlet> ptr; // FunctionWSServlet 智能指针类型
    typedef std::function<int32_t (yk::http::HttpRequest::ptr header
                              ,yk::http::WSSession::ptr session)> on_connect_cb; // 连接回调函数类型
    typedef std::function<int32_t (yk::http::HttpRequest::ptr header
                             ,yk::http::WSSession::ptr session)> on_close_cb; // 关闭回调函数类型
    typedef std::function<int32_t (yk::http::HttpRequest::ptr header
                           ,yk::http::WSFrameMessage::ptr msg
                           ,yk::http::WSSession::ptr session)> callback; // 处理消息回调函数类型

    // 构造函数，接受消息处理回调函数和连接、关闭回调函数作为参数
    FunctionWSServlet(callback cb
                      ,on_connect_cb connect_cb = nullptr
                      ,on_close_cb close_cb = nullptr);

    // WebSocket 连接建立时调用的方法
    virtual int32_t onConnect(yk::http::HttpRequest::ptr header
                              ,yk::http::WSSession::ptr session) override;

    // WebSocket 连接关闭时调用的方法
    virtual int32_t onClose(yk::http::HttpRequest::ptr header
                             ,yk::http::WSSession::ptr session) override;

    // 处理 WebSocket 消息的方法
    virtual int32_t handle(yk::http::HttpRequest::ptr header
                           ,yk::http::WSFrameMessage::ptr msg
                           ,yk::http::WSSession::ptr session) override;
protected:
    callback m_callback; // 消息处理回调函数
    on_connect_cb m_onConnect; // 连接回调函数
    on_close_cb m_onClose; // 关闭回调函数
};

// WebSocket Servlet 分发器类
class WSServletDispatch : public ServletDispatch {
public:
    typedef std::shared_ptr<WSServletDispatch> ptr; // WSServletDispatch 智能指针类型
    typedef RWMutex RWMutexType; // 读写锁类型

    // 构造函数
    WSServletDispatch();

    // 添加 WebSocket Servlet 的方法
    void addServlet(const std::string& uri
                    ,FunctionWSServlet::callback cb
                    ,FunctionWSServlet::on_connect_cb connect_cb = nullptr
                    ,FunctionWSServlet::on_close_cb close_cb = nullptr);

    // 添加通配符 WebSocket Servlet 的方法
    void addGlobServlet(const std::string& uri
                    ,FunctionWSServlet::callback cb
                    ,FunctionWSServlet::on_connect_cb connect_cb = nullptr
                    ,FunctionWSServlet::on_close_cb close_cb = nullptr);

    // 获取与 URI 匹配的 WebSocket Servlet 的方法
    WSServlet::ptr getWSServlet(const std::string& uri);
};

} // namespace http
} // namespace yk

#endif
