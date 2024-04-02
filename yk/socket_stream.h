#ifndef __YK__SOCKET__STREAM_H__
#define __YK__SOCKET__STREAM_H__
#include "stream.h"
#include "socket.h"

namespace yk{

class SocketStream : public Stream{
public:    
    typedef std::shared_ptr<SocketStream> ptr;
    SocketStream(Socket::ptr sock,bool owner = true);//owner 全权管理，例如close操作
    ~SocketStream();
    int read(void* buffer,size_t length) override;
    int read(ByteArray::ptr ba,size_t length) override;
    int write(const void* buffer,size_t length) override;
    int write(ByteArray::ptr ba,size_t length) override;
    void close()override;
    Socket::ptr getSocket() const {return m_socket;}
    bool isConnected() const ;
protected:
    
    Socket::ptr m_socket;
    bool m_owner;
};



}




#endif
