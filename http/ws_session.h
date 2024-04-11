#ifndef __YK_HTTP_WS_SESSION_H__
#define __YK_HTTP_WS_SESSION_H__

#include "yk/config.h"
#include "http_session.h"
#include <stdint.h>

namespace yk {
namespace http {

#pragma pack(1)
/**
 * @brief WebSocket帧头结构体
 */
struct WSFrameHead {
    enum OPCODE {
        /// 数据分片帧
        CONTINUE = 0,
        /// 文本帧
        TEXT_FRAME = 1,
        /// 二进制帧
        BIN_FRAME = 2,
        /// 断开连接
        CLOSE = 8,
        /// PING
        PING = 0x9,
        /// PONG
        PONG = 0xA
    };
    uint32_t opcode: 4; ///< 操作码
    bool rsv3: 1; ///< 保留位3
    bool rsv2: 1; ///< 保留位2
    bool rsv1: 1; ///< 保留位1
    bool fin: 1; ///< 结束标志
    uint32_t payload: 7; ///< 负载长度
    bool mask: 1; ///< 掩码标志
    
    std::string toString() const;
};
#pragma pack()

/**
 * @brief WebSocket帧消息类
 */
class WSFrameMessage {
public:
    typedef std::shared_ptr<WSFrameMessage> ptr;
    /**
     * @brief 构造函数
     * @param[in] opcode 操作码
     * @param[in] data 消息数据
     */
    WSFrameMessage(int opcode = 0, const std::string& data = "");

    /**
     * @brief 获取操作码
     * @return 操作码
     */
    int getOpcode() const { return m_opcode;}

    /**
     * @brief 设置操作码
     * @param[in] v 操作码
     */
    void setOpcode(int v) { m_opcode = v;}

    /**
     * @brief 获取消息数据
     * @return 消息数据的引用
     */
    const std::string& getData() const { return m_data;}

    /**
     * @brief 获取消息数据的引用
     * @return 消息数据的引用
     */
    std::string& getData() { return m_data;}

    /**
     * @brief 设置消息数据
     * @param[in] v 消息数据
     */
    void setData(const std::string& v) { m_data = v;}
private:
    int m_opcode; ///< 操作码
    std::string m_data; ///< 消息数据
};

/**
 * @brief WebSocket会话类
 */
class WSSession : public HttpSession {
public:
    typedef std::shared_ptr<WSSession> ptr;
    /**
     * @brief 构造函数
     * @param[in] sock 套接字指针
     * @param[in] owner 是否拥有套接字
     */
    WSSession(Socket::ptr sock, bool owner = true);

    /**
     * @brief 处理WebSocket握手请求
     * @return 握手成功返回HttpRequest对象，失败返回nullptr
     */
    HttpRequest::ptr handleShake();

    /**
     * @brief 接收WebSocket消息
     * @return 接收到的WebSocket消息对象
     */
    WSFrameMessage::ptr recvMessage();

    /**
     * @brief 发送WebSocket消息
     * @param[in] msg 消息对象
     * @param[in] fin 结束标志
     * @return 发送的字节数，失败返回-1
     */
    int32_t sendMessage(WSFrameMessage::ptr msg, bool fin = true);

    /**
     * @brief 发送文本消息
     * @param[in] msg 消息内容
     * @param[in] opcode 操作码，默认为文本帧
     * @param[in] fin 结束标志，默认为true
     * @return 发送的字节数，失败返回-1
     */
    int32_t sendMessage(const std::string& msg, int32_t opcode = WSFrameHead::TEXT_FRAME, bool fin = true);

    /**
     * @brief 发送Ping消息
     * @return 发送的字节数，失败返回-1
     */
    int32_t ping();

    /**
     * @brief 发送Pong消息
     * @return 发送的字节数，失败返回-1
     */
    int32_t pong();
private:
    bool handleServerShake();
    bool handleClientShake();
};

/**
 * @brief WebSocket消息接收函数
 * @param[in] stream 流对象指针
 * @param[in] client 是否为客户端
 * @return 接收到的WebSocket消息对象
 */
extern WSFrameMessage::ptr WSRecvMessage(Stream* stream, bool client);

/**
 * @brief WebSocket消息发送函数
 * @param[in] stream 流对象指针
 * @param[in] msg 消息对象
 * @param[in] client 是否为客户端
 * @param[in] fin 结束标志
 * @return 发送的字节数，失败返回-1
 */
extern int32_t WSSendMessage(Stream* stream, WSFrameMessage::ptr msg, bool client, bool fin);

/**
 * @brief 发送Ping消息
 * @param[in] stream 流对象指针
 * @return 发送的字节数，失败返回-1
 */
extern int32_t WSPing(Stream* stream);

/**
 * @brief 发送Pong消息
 * @param[in] stream 流对象指针
 * @return 发送的字节数，失败返回-1
 */
extern int32_t WSPong(Stream* stream);

}
}

#endif
