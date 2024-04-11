#include "ws_session.h"
#include "yk/log.h"
#include "yk/endian.h"
#include "yk/util.h"
#include <string.h>

namespace yk {
namespace http {

// WebSocket会话的日志记录器
static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

// WebSocket消息的最大大小配置项，默认为32MB
yk::ConfigVar<uint32_t>::ptr g_websocket_message_max_size
    = yk::Config::Lookup("websocket.message.max_size"
            ,(uint32_t) 1024 * 1024 * 32, "websocket message max size");


std::string WSFrameHead::toString() const {
    std::stringstream ss;
    ss << "[WSFrameHead fin=" << fin
       << " rsv1=" << rsv1
       << " rsv2=" << rsv2
       << " rsv3=" << rsv3
       << " opcode=" << opcode
       << " mask=" << mask
       << " payload=" << payload
       << "]";
    return ss.str();
}


WSSession::WSSession(Socket::ptr sock, bool owner)
    :HttpSession(sock, owner) {
}

HttpRequest::ptr WSSession::handleShake() {
    HttpRequest::ptr req;
    do {
        // 接收HTTP请求
        req = recvRequest();
        if(!req) {
            YK_LOG_INFO(g_logger) << "invalid http request";
            break;
        }

        // 检查HTTP请求头是否符合WebSocket握手规范
        if(strcasecmp(req->getHeader("Upgrade").c_str(), "websocket")) {
            YK_LOG_INFO(g_logger) << "http header Upgrade != websocket";
            break;
        }
        if(strcasecmp(req->getHeader("Connection").c_str(), "Upgrade")) {
            YK_LOG_INFO(g_logger) << "http header Connection != Upgrade";
            break;
        }
        if(req->getHeaderAs<int>("Sec-webSocket-Version") != 13) {
            YK_LOG_INFO(g_logger) << "http header Sec-webSocket-Version != 13";
            break;
        }
        std::string key = req->getHeader("Sec-WebSocket-Key");
        if(key.empty()) {
            YK_LOG_INFO(g_logger) << "http header Sec-WebSocket-Key = null";
            break;
        }

        // 生成Sec-WebSocket-Accept响应头
        std::string v = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        v = yk::base64encode(yk::sha1sum(v));
        req->setWebsocket(true);

        // 创建WebSocket握手响应
        auto rsp = req->createResponse();
        rsp->setStatus(HttpStatus::SWITCHING_PROTOCOLS);
        rsp->setWebsocket(true);
        rsp->setReason("Web Socket Protocol Handshake");
        rsp->setHeader("Upgrade", "websocket");
        rsp->setHeader("Connection", "Upgrade");
        rsp->setHeader("Sec-WebSocket-Accept", v);

        // 发送WebSocket握手响应
        sendResponse(rsp);

        // 记录日志并返回请求对象
        YK_LOG_DEBUG(g_logger) << *req;
        YK_LOG_DEBUG(g_logger) << *rsp;
        return req;
    } while(false);

    // 处理失败，记录日志并返回空指针
    if(req) {
        YK_LOG_INFO(g_logger) << *req;
    }
    return nullptr;
}

WSFrameMessage::WSFrameMessage(int opcode, const std::string& data)
    :m_opcode(opcode)
    ,m_data(data) {
}


WSFrameMessage::ptr WSSession::recvMessage() {
    return WSRecvMessage(this, false);
}

int32_t WSSession::sendMessage(WSFrameMessage::ptr msg, bool fin) {
    return WSSendMessage(this, msg, false, fin);
}

int32_t WSSession::sendMessage(const std::string& msg, int32_t opcode, bool fin) {
    return WSSendMessage(this, std::make_shared<WSFrameMessage>(opcode, msg), false, fin);
}

int32_t WSSession::ping() {
    return WSPing(this);
}

WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client) {
    int opcode = 0;
    std::string data;
    int cur_len = 0;
    do {
        WSFrameHead ws_head;
        if(stream->readFixSize(&ws_head, sizeof(ws_head)) <= 0) {
            break;
        }
        YK_LOG_DEBUG(g_logger) << "WSFrameHead " << ws_head.toString();

        // 处理Ping消息
        if(ws_head.opcode == WSFrameHead::PING) {
            YK_LOG_INFO(g_logger) << "PING";
            if(WSPong(stream) <= 0) {
                break;
            }
        } else if(ws_head.opcode == WSFrameHead::PONG) {
        } else if(ws_head.opcode == WSFrameHead::CONTINUE
                || ws_head.opcode == WSFrameHead::TEXT_FRAME
                || ws_head.opcode == WSFrameHead::BIN_FRAME) {
            // 处理文本或二进制消息
            if(!client && !ws_head.mask) {
                YK_LOG_INFO(g_logger) << "WSFrameHead mask != 1";
                break;
            }
            uint64_t length = 0;
            if(ws_head.payload == 126) {
                uint16_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0) {
                    break;
                }
                length = yk::byteswapOnLittleEndian(len);
            } else if(ws_head.payload == 127) {
                uint64_t len = 0;
                if(stream->readFixSize(&len, sizeof(len)) <= 0) {
                    break;
                }
                length = yk::byteswapOnLittleEndian(len);
            } else {
                length = ws_head.payload;
            }

            // 检查消息大小是否超过配置的最大限制
            if((cur_len + length) >= g_websocket_message_max_size->getValue()) {
                YK_LOG_WARN(g_logger) << "WSFrameMessage length > "
                    << g_websocket_message_max_size->getValue()
                    << " (" << (cur_len + length) << ")";
                break;
            }

            // 掩码解析
            char mask[4] = {0};
            if(ws_head.mask) {
                if(stream->readFixSize(mask, sizeof(mask)) <= 0) {
                    break;
                }
            }

            // 读取消息数据并解码
            data.resize(cur_len + length);
            if(stream->readFixSize(&data[cur_len], length) <= 0) {
                break;
            }
            if(ws_head.mask) {
                for(int i = 0; i < (int)length; ++i) {
                    data[cur_len + i] ^= mask[i % 4];
                }
            }
            cur_len += length;

            // 记录消息类型
            if(!opcode && ws_head.opcode != WSFrameHead::CONTINUE) {
                opcode = ws_head.opcode;
            }

            // 如果消息帧为结束帧，返回消息对象
            if(ws_head.fin) {
                YK_LOG_DEBUG(g_logger) << data;
                return WSFrameMessage::ptr(new WSFrameMessage(opcode, std::move(data)));
            }
        } else {
            YK_LOG_DEBUG(g_logger) << "invalid opcode=" << ws_head.opcode;
        }
    } while(true);
    stream->close();
    return nullptr;
}

int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin) {
    do {
        // 创建WebSocket消息头
        WSFrameHead ws_head;
        memset(&ws_head, 0, sizeof(ws_head));
        ws_head.fin = fin;
        ws_head.opcode = msg->getOpcode();
        ws_head.mask = client;
        uint64_t size = msg->getData().size();
        if(size < 126) {
            ws_head.payload = size;
        } else if(size < 65536) {
            ws_head.payload = 126;
        } else {
            ws_head.payload = 127;
        }
        
        // 发送消息头
        if(stream->writeFixSize(&ws_head, sizeof(ws_head)) <= 0) {
            break;
        }

        // 发送长度字段
        if(ws_head.payload == 126) {
            uint16_t len = size;
            len = yk::byteswapOnLittleEndian(len);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        } else if(ws_head.payload == 127) {
            uint64_t len = yk::byteswapOnLittleEndian(size);
            if(stream->writeFixSize(&len, sizeof(len)) <= 0) {
                break;
            }
        }

        // 发送掩码
        if(client) {
            char mask[4];
            uint32_t rand_value = rand();
            memcpy(mask, &rand_value, sizeof(mask));
            std::string& data = msg->getData();
            for(size_t i = 0; i < data.size(); ++i) {
                data[i] ^= mask[i % 4];
            }

            if(stream->writeFixSize(mask, sizeof(mask)) <= 0) {
                break;
            }
        }

        // 发送消息数据
        if(stream->writeFixSize(msg->getData().c_str(), size) <= 0) {
            break;
        }
        return size + sizeof(ws_head);
    } while(0);
    stream->close();
    return -1;
}

int32_t WSSession::pong() {
    return WSPong(this);
}

int32_t WSPing(Stream* stream) {
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PING;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0) {
        stream->close();
    }
    return v;
}

int32_t WSPong(Stream* stream) {
    WSFrameHead ws_head;
    memset(&ws_head, 0, sizeof(ws_head));
    ws_head.fin = 1;
    ws_head.opcode = WSFrameHead::PONG;
    int32_t v = stream->writeFixSize(&ws_head, sizeof(ws_head));
    if(v <= 0) {
        stream->close();
    }
    return v;
}

}
}
