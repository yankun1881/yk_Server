#include "rock_stream.h"
#include "yk/log.h"
#include "yk/config.h"
#include "yk/worker.h"

namespace yk {

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
static yk::ConfigVar<std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> > >::ptr g_rock_services =
    yk::Config::Lookup("rock_services", std::unordered_map<std::string
    ,std::unordered_map<std::string, std::string> >(), "rock_services");


std::string RockResult::toString() const {
    std::stringstream ss;
    ss << "[RockResult result=" << result
       << " used=" << used
       << " response=" << (response ? response->toString() : "null")
       << " request=" << (request ? request->toString() : "null")
       << "]";
    return ss.str();
}

RockStream::RockStream(Socket::ptr sock)
    :AsyncSocketStream(sock, true)
    ,m_decoder(new RockMessageDecoder) {
    if(sock){
        YK_LOG_DEBUG(g_logger) << "RockStream::RockStream "
        << this << " "
        << sock->toString();
    }

}

RockStream::~RockStream() {
    if(m_socket){
        YK_LOG_DEBUG(g_logger) << "RockStream::RockStream "
        << this << " "
        << m_socket->toString();
    }
}

int32_t RockStream::sendMessage(Message::ptr msg) {
    if(isConnected()) {
        RockSendCtx::ptr ctx(new RockSendCtx);
        ctx->msg = msg;
        enqueue(ctx);
        return 1;
    } else {
        return -1;
    }
}

RockResult::ptr RockStream::request(RockRequest::ptr req, uint32_t timeout_ms) {
    if(isConnected()) {
        RockCtx::ptr ctx(new RockCtx);
        ctx->request = req;
        ctx->sn = req->getSn();
        ctx->timeout = timeout_ms;
        ctx->scheduler = yk::Scheduler::GetThis();
        ctx->fiber = yk::Fiber::GetThis();
        addCtx(ctx);
        uint64_t ts = yk::GetCurrentMS();
        ctx->timer = yk::IOManager::GetThis()->addTimer(timeout_ms,
                std::bind(&RockStream::onTimeOut, shared_from_this(), ctx));
        enqueue(ctx);
        yk::Fiber::YieldToHold();
        return std::make_shared<RockResult>(ctx->result, yk::GetCurrentMS() - ts, ctx->response, req);
    } else {
        return std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, 0, nullptr, req);
    }
}

bool RockStream::RockSendCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<RockStream>(stream)
                ->m_decoder->serializeTo(stream, msg) > 0;
}

bool RockStream::RockCtx::doSend(AsyncSocketStream::ptr stream) {
    return std::dynamic_pointer_cast<RockStream>(stream)
                ->m_decoder->serializeTo(stream, request) > 0;
}

AsyncSocketStream::Ctx::ptr RockStream::doRecv() {
    //YK_LOG_INFO(g_logger) << "doRecv " << this;
    auto msg = m_decoder->parseFrom(shared_from_this());
    if(!msg) {
        innerClose();
        return nullptr;
    }

    int type = msg->getType();
    if(type == Message::RESPONSE) {
        auto rsp = std::dynamic_pointer_cast<RockResponse>(msg);
        if(!rsp) {
            YK_LOG_WARN(g_logger) << "RockStream doRecv response not RockResponse: "
                << msg->toString();
            return nullptr;
        }
        RockCtx::ptr ctx = getAndDelCtxAs<RockCtx>(rsp->getSn());
        if(!ctx) {
            YK_LOG_WARN(g_logger) << "RockStream request timeout reponse="
                << rsp->toString();
            return nullptr;
        }
        ctx->result = rsp->getResult();
        ctx->response = rsp;
        return ctx;
    } else if(type == Message::REQUEST) {
        auto req = std::dynamic_pointer_cast<RockRequest>(msg);
        if(!req) {
            YK_LOG_WARN(g_logger) << "RockStream doRecv request not RockRequest: "
                << msg->toString();
            return nullptr;
        }
        if(m_requestHandler) {
            m_worker->schedule(std::bind(&RockStream::handleRequest,
                        std::dynamic_pointer_cast<RockStream>(shared_from_this()),
                        req));
        } else {
            YK_LOG_WARN(g_logger) << "unhandle request " << req->toString();
        }
    } else if(type == Message::NOTIFY) {
        auto nty = std::dynamic_pointer_cast<RockNotify>(msg);
        if(!nty) {
            YK_LOG_WARN(g_logger) << "RockStream doRecv notify not RockNotify: "
                << msg->toString();
            return nullptr;
        }

        if(m_notifyHandler) {
            m_worker->schedule(std::bind(&RockStream::handleNotify,
                        std::dynamic_pointer_cast<RockStream>(shared_from_this()),
                        nty));
        } else {
            YK_LOG_WARN(g_logger) << "unhandle notify " << nty->toString();
        }
    } else {
        YK_LOG_WARN(g_logger) << "RockStream recv unknow type=" << type
            << " msg: " << msg->toString();
    }
    return nullptr;
}

void RockStream::handleRequest(yk::RockRequest::ptr req) {
    yk::RockResponse::ptr rsp = req->createResponse();
    if(!m_requestHandler(req, rsp
        ,std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
        sendMessage(rsp);
        //innerClose();
        close();
    } else {
        sendMessage(rsp);
    }
}

void RockStream::handleNotify(yk::RockNotify::ptr nty) {
    if(!m_notifyHandler(nty
        ,std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
        //innerClose();
        close();
    }
}

RockSession::RockSession(Socket::ptr sock)
    :RockStream(sock) {
    m_autoConnect = false;
}

RockConnection::RockConnection()
    :RockStream(nullptr) {
    m_autoConnect = true;
}

bool RockConnection::connect(yk::Address::ptr addr) {
    m_socket = yk::Socket::CreateTCP(addr);
    return m_socket->connect(addr);
}


static SocketStream::ptr create_rock_stream(const std::string & ip, int port) {
    yk::IPAddress::ptr addr = yk::Address::LookupAnyIPAddress(ip);
    if(!addr) {
//        YK_LOG_ERROR(g_logger) << "invalid service info: " << info->toString();
        return nullptr;
    }
    addr->setPort(port);

    RockConnection::ptr conn(new RockConnection);

    yk::WorkerMgr::GetInstance()->schedule("service_io", [conn, addr](){
        conn->connect(addr);
    });
    return conn;
}



}
