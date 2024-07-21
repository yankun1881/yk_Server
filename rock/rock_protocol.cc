#include "rock_protocol.h"
#include "yk/log.h"
#include "yk/config.h"
#include "yk/endian.h"

namespace yk {

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

static yk::ConfigVar<uint32_t>::ptr g_rock_protocol_max_length
    = yk::Config::Lookup("rock.protocol.max_length",
                            (uint32_t)(1024 * 1024 * 64), "rock protocol max length");

static yk::ConfigVar<uint32_t>::ptr g_rock_protocol_gzip_min_length
    = yk::Config::Lookup("rock.protocol.gzip_min_length",
                            (uint32_t)(1024 * 4), "rock protocol gizp min length");

bool RockBody::serializeToByteArray(ByteArray::ptr bytearray) {
    bytearray->writeStringVint(m_body);
    return true;
}

bool RockBody::parseFromByteArray(ByteArray::ptr bytearray) {
    m_body = bytearray->readStringVint();
    return true;
}

std::shared_ptr<RockResponse> RockRequest::createResponse() {
    RockResponse::ptr rt(new RockResponse);
    rt->setSn(m_sn);
    rt->setCmd(m_cmd);
    return rt;
}

std::string RockRequest::toString() const {
    std::stringstream ss;
    ss << "[RockRequest sn=" << m_sn
       << " cmd=" << m_cmd
       << " body.length=" << m_body.size()
       << "]";
    return ss.str();
}

const std::string& RockRequest::getName() const {
    static const std::string& s_name = "RockRequest";
    return s_name;
}

int32_t RockRequest::getType() const {
    return Message::REQUEST;
}

bool RockRequest::serializeToByteArray(ByteArray::ptr bytearray) {
    try {
        bool v = true;
        v &= Request::serializeToByteArray(bytearray);
        v &= RockBody::serializeToByteArray(bytearray);
        return v;
    } catch (...) {
        YK_LOG_ERROR(g_logger) << "RockRequest serializeToByteArray error";
    }
    return false;
}

bool RockRequest::parseFromByteArray(ByteArray::ptr bytearray) {
    try {
        bool v = true;
        v &= Request::parseFromByteArray(bytearray);
        v &= RockBody::parseFromByteArray(bytearray);
        return v;
    } catch (...) {
        YK_LOG_ERROR(g_logger) << "RockRequest parseFromByteArray error";
    }
    return false;
}

std::string RockResponse::toString() const {
    std::stringstream ss;
    ss << "[RockResponse sn=" << m_sn
       << " cmd=" << m_cmd
       << " result=" << m_result
       << " result_msg=" << m_resultStr
       << " body.length=" << m_body.size()
       << "]";
    return ss.str();
}

const std::string& RockResponse::getName() const {
    static const std::string& s_name = "RockResponse";
    return s_name;
}

int32_t RockResponse::getType() const {
    return Message::RESPONSE;
}

bool RockResponse::serializeToByteArray(ByteArray::ptr bytearray) {
    try {
        bool v = true;
        v &= Response::serializeToByteArray(bytearray);
        v &= RockBody::serializeToByteArray(bytearray);
        return v;
    } catch (...) {
        YK_LOG_ERROR(g_logger) << "RockResponse serializeToByteArray error";
    }
    return false;
}

bool RockResponse::parseFromByteArray(ByteArray::ptr bytearray) {
    try {
        bool v = true;
        v &= Response::parseFromByteArray(bytearray);
        v &= RockBody::parseFromByteArray(bytearray);
        return v;
    } catch (...) {
        YK_LOG_ERROR(g_logger) << "RockResponse parseFromByteArray error";
    }
    return false;
}

std::string RockNotify::toString() const {
    std::stringstream ss;
    ss << "[RockNotify notify=" << m_notify
       << " body.length=" << m_body.size()
       << "]";
    return ss.str();
}

const std::string& RockNotify::getName() const {
    static const std::string& s_name = "RockNotify";
    return s_name;
}

int32_t RockNotify::getType() const {
    return Message::NOTIFY;
}

bool RockNotify::serializeToByteArray(ByteArray::ptr bytearray) {
    try {
        bool v = true;
        v &= Notify::serializeToByteArray(bytearray);
        v &= RockBody::serializeToByteArray(bytearray);
        return v;
    } catch (...) {
        YK_LOG_ERROR(g_logger) << "RockNotify serializeToByteArray error";
    }
    return false;
}

bool RockNotify::parseFromByteArray(ByteArray::ptr bytearray) {
    try {
        bool v = true;
        v &= Notify::parseFromByteArray(bytearray);
        v &= RockBody::parseFromByteArray(bytearray);
        return v;
    } catch (...) {
        YK_LOG_ERROR(g_logger) << "RockNotify parseFromByteArray error";
    }
    return false;
}

static const uint8_t s_rock_magic[2] = {0xab, 0xcd};

RockMsgHeader::RockMsgHeader()
    :magic{0xab, 0xcd}
    ,version(1)
    ,flag(0)
    ,length(0) {
}

Message::ptr RockMessageDecoder::parseFrom(Stream::ptr stream) {
    try {
        RockMsgHeader header;
        if(stream->readFixSize(&header, sizeof(header)) <= 0) {
            YK_LOG_ERROR(g_logger) << "RockMessageDecoder decode head error";
            return nullptr;
        }

        if(memcmp(header.magic, s_rock_magic, sizeof(s_rock_magic))) {
            YK_LOG_ERROR(g_logger) << "RockMessageDecoder head.magic error";
            return nullptr;
        }

        if(header.version != 0x1) {
            YK_LOG_ERROR(g_logger) << "RockMessageDecoder head.version != 0x1";
            return nullptr;
        }

        header.length = yk::byteswapOnLittleEndian(header.length);
        if((uint32_t)header.length >= g_rock_protocol_max_length->getValue()) {
            YK_LOG_ERROR(g_logger) << "RockMessageDecoder head.length("
                                      << header.length << ") >="
                                      << g_rock_protocol_max_length->getValue();
            return nullptr;
        }
        yk::ByteArray::ptr ba(new yk::ByteArray);
        if(stream->readFixSize(ba, header.length) <= 0) {
            YK_LOG_ERROR(g_logger) << "RockMessageDecoder read body fail length=" << header.length;
            return nullptr;
        }

        ba->setPosition(0);
        if(header.flag & 0x1) { //gizp
        }
        uint8_t type = ba->readFuint8();
        Message::ptr msg;
        switch(type) {
            case Message::REQUEST:
                msg.reset(new RockRequest);
                break;
            case Message::RESPONSE:
                msg.reset(new RockResponse);
                break;
            case Message::NOTIFY:
                msg.reset(new RockNotify);
                break;
            default:
                YK_LOG_ERROR(g_logger) << "RockMessageDecoder invalid type=" << (int)type;
                return nullptr;
        }

        if(!msg->parseFromByteArray(ba)) {
            YK_LOG_ERROR(g_logger) << "RockMessageDecoder parseFromByteArray fail type=" << (int)type;
            return nullptr;
        }
        return msg;
    } catch (std::exception& e) {
        YK_LOG_ERROR(g_logger) << "RockMessageDecoder except:" << e.what();
    } catch (...) {
        YK_LOG_ERROR(g_logger) << "RockMessageDecoder except";
    }
    return nullptr;
}

int32_t RockMessageDecoder::serializeTo(Stream::ptr stream, Message::ptr msg) {
    RockMsgHeader header;
    auto ba = msg->toByteArray();
    ba->setPosition(0);
    header.length = ba->getSize();
    header.length = yk::byteswapOnLittleEndian(header.length);
    if(stream->writeFixSize(&header, sizeof(header)) <= 0) {
        YK_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo write header fail";
        return -3;
    }
    if(stream->writeFixSize(ba, ba->getReadSize()) <= 0) {
        YK_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo write body fail";
        return -4;
    }
    return sizeof(header) + ba->getSize();
}

}
