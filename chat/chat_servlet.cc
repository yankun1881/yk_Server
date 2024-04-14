#include "chat_servlet.h"
#include "yk/log.h"
#include "protocol.h"

namespace chat {

static yk::Logger::ptr g_logger = YK_LOG_ROOT();

yk::RWMutex m_mutex;
std::map<std::string, yk::http::WSSession::ptr> m_sessions;

bool session_exists(const std::string& id) {
    YK_LOG_INFO(g_logger) << "session_exists id=" << id;
    yk::RWMutex::ReadLock lock(m_mutex);
    auto it = m_sessions.find(id);
    return it != m_sessions.end();
}

void session_add(const std::string& id, yk::http::WSSession::ptr session) {
    YK_LOG_INFO(g_logger) << "session_add id=" << id;
    yk::RWMutex::WriteLock lock(m_mutex);
    m_sessions[id] = session;
}

void session_del(const std::string& id) {
    YK_LOG_INFO(g_logger) << "session_add del=" << id;
    yk::RWMutex::WriteLock lock(m_mutex);
    m_sessions.erase(id);
}

int32_t SendMessage(yk::http::WSSession::ptr session
                    , ChatMessage::ptr msg) {
    YK_LOG_INFO(g_logger) << msg->toString() << " - " << session;
    return session->sendMessage(msg->toString()) > 0 ? 0: 1;
}

void session_notify(ChatMessage::ptr msg, yk::http::WSSession::ptr session = nullptr) {
    yk::RWMutex::ReadLock lock(m_mutex);
    auto sessions = m_sessions;
    lock.unlock();

    for(auto& i : sessions) {
        if(i.second == session) {
            continue;
        }
        SendMessage(i.second, msg);
    }
}

ChatWSServlet::ChatWSServlet()
    :yk::http::WSServlet("chat_servlet") {
}

int32_t ChatWSServlet::onConnect(yk::http::HttpRequest::ptr header
                              ,yk::http::WSSession::ptr session) {
    YK_LOG_INFO(g_logger) << "onConnect ";
    return 0;
}

int32_t ChatWSServlet::onClose(yk::http::HttpRequest::ptr header
                             ,yk::http::WSSession::ptr session) {
    auto id = header->getHeader("$id");
    YK_LOG_INFO(g_logger) << "onClose "<< " id=" << id;
    if(!id.empty()) {
        session_del(id);
        ChatMessage::ptr nty(new ChatMessage);
        nty->set("type", "user_leave");
        nty->set("time", yk::TimeToStr());
        nty->set("name", id);
        session_notify(nty);
    }
    return 0;
}

int32_t ChatWSServlet::handle(yk::http::HttpRequest::ptr header
                           ,yk::http::WSFrameMessage::ptr msgx
                           ,yk::http::WSSession::ptr session) {
    YK_LOG_INFO(g_logger) << "handle " << session
            << " opcode=" << msgx->getOpcode()
            << " data=" << msgx->getData();

    auto msg = ChatMessage::Create(msgx->getData());
    auto id = header->getHeader("$id");
    if(!msg) {
        if(!id.empty()) {
            yk::RWMutex::WriteLock lock(m_mutex);
            m_sessions.erase(id);
        }
        return 1;
    }

    ChatMessage::ptr rsp(new ChatMessage);
    auto type = msg->get("type");
    if(type == "login_request") {
        rsp->set("type", "login_response");
        auto name = msg->get("name");
        if(name.empty()) {
            rsp->set("result", "400");
            rsp->set("msg", "name is null");
            return SendMessage(session, rsp);
        }
        if(!id.empty()) {
            rsp->set("result", "401");
            rsp->set("msg", "logined");
            return SendMessage(session, rsp);
        }
        if(session_exists(id)) {
            rsp->set("result", "402");
            rsp->set("msg", "name exists");
            return SendMessage(session, rsp);
        }
        id = name;
        header->setHeader("$id", id);
        rsp->set("result", "200");
        rsp->set("msg", "ok");
        session_add(id, session);

        ChatMessage::ptr nty(new ChatMessage);
        nty->set("type", "user_enter");
        nty->set("time", yk::TimeToStr());
        nty->set("name", name);
        session_notify(nty, session);
        return SendMessage(session, rsp);
    } else if(type == "send_request") {
        rsp->set("type", "send_response");
        auto m = msg->get("msg");
        if(m.empty()) {
            rsp->set("result", "500");
            rsp->set("msg", "msg is null");
            return SendMessage(session, rsp);
        }
        if(id.empty()) {
            rsp->set("result", "501");
            rsp->set("msg", "not login");
            return SendMessage(session, rsp);
        }

        rsp->set("result", "200");
        rsp->set("msg", "ok");

        ChatMessage::ptr nty(new ChatMessage);
        nty->set("type", "msg");
        nty->set("time", yk::TimeToStr());
        nty->set("name", id);
        nty->set("msg", m);
        session_notify(nty, nullptr);
        return SendMessage(session, rsp);
    }
    return 0;
}

}
