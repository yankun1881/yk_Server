#include <memory>         
#include "stream.h"       
#include "bytearray.h"   

namespace yk {

class Message {
public:
    typedef std::shared_ptr<Message> ptr;  
    enum MessageType {                    
        REQUEST = 1,                      // 请求消息
        RESPONSE = 2,                     // 响应消息
        NOTIFY = 3                        // 通知消息
    };
    virtual ~Message() {}                 

    virtual ByteArray::ptr toByteArray();  // 将消息转换为字节数组的虚函数声明
    virtual bool serializeToByteArray(ByteArray::ptr bytearray) = 0;  // 序列化消息到字节数组的纯虚函数声明
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) = 0;     // 从字节数组解析消息的纯虚函数声明

    virtual std::string toString() const = 0;         // 将消息转换为字符串
    virtual const std::string& getName() const = 0;   // 获取消息名称
    virtual int32_t getType() const = 0;              // 获取消息类型
};

class MessageDecoder {
public:
    typedef std::shared_ptr<MessageDecoder> ptr; 

    virtual ~MessageDecoder() {}
    virtual Message::ptr parseFrom(Stream::ptr stream) = 0;  // 从流中解析消息
    virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) = 0;  // 将消息序列化到流
};

class Request : public Message {
public:
    typedef std::shared_ptr<Request> ptr;        

    Request();                                   // 默认构造函数

    uint32_t getSn() const { return m_sn;}        // 获取序列号
    uint32_t getCmd() const { return m_cmd;}      // 获取命令号

    void setSn(uint32_t v) { m_sn = v;}           // 设置序列号
    void setCmd(uint32_t v) { m_cmd = v;}         // 设置命令号

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;  
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;    
protected:
    uint32_t m_sn;                               // 序列号成员变量
    uint32_t m_cmd;                              // 命令号成员变量
};

class Response : public Message {
public:
    typedef std::shared_ptr<Response> ptr;      
    Response();                                  

    uint32_t getSn() const { return m_sn;}        // 获取序列号
    uint32_t getCmd() const { return m_cmd;}      // 获取命令号
    uint32_t getResult() const { return m_result;}  // 获取结果值
    const std::string& getResultStr() const { return m_resultStr;}  // 获取结果字符串

    void setSn(uint32_t v) { m_sn = v;}           // 设置序列号
    void setCmd(uint32_t v) { m_cmd = v;}         // 设置命令号
    void setResult(uint32_t v) { m_result = v;}   // 设置结果值
    void setResultStr(const std::string& v) { m_resultStr = v;}  // 设置结果字符串

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;  
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;    
protected:
    uint32_t m_sn;                               // 序列号
    uint32_t m_cmd;                              // 命令号
    uint32_t m_result;                           // 结果值
    std::string m_resultStr;                     // 结果字符串
};

class Notify : public Message {
public:
    typedef std::shared_ptr<Notify> ptr;        
    Notify();                                    // 默认构造函数

    uint32_t getNotify() const { return m_notify;}  // 获取通知值
    void setNotify(uint32_t v) { m_notify = v;}    // 设置通知值

    virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;  // 重写的将Notify对象序列化为字节数组的函数声明
    virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;    // 重写的从字节数组解析出Notify对象的函数声明
protected:
    uint32_t m_notify;                           // 通知值成员变量
};

} // namespace yk
