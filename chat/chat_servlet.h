#ifndef __CHAT_CHAT_SERVLET_H__
#define __CHAT_CHAT_SERVLET_H__

#include "../http/ws_servlet.h"

namespace chat {

class ChatWSServlet : public yk::http::WSServlet {
public:
    typedef std::shared_ptr<ChatWSServlet> ptr;
    ChatWSServlet();
    virtual int32_t onConnect(yk::http::HttpRequest::ptr header
                              ,yk::http::WSSession::ptr session) override;
    virtual int32_t onClose(yk::http::HttpRequest::ptr header
                             ,yk::http::WSSession::ptr session) override;
    virtual int32_t handle(yk::http::HttpRequest::ptr header
                           ,yk::http::WSFrameMessage::ptr msg
                           ,yk::http::WSSession::ptr session) override;
};

}

#endif
