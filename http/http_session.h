#ifndef __YK_HTTP_SESSION_H__
#define __YK_HTTP_SESSION_H__

#include "yk/socket_stream.h"
#include "http.h"
#include "http_parser.h"
#include<memory>

namespace yk{
namespace http{


class HttpSession:public SocketStream{
public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr sock, bool owner = true); 
    HttpRequest::ptr recvRequest();

    int sendResponse(HttpResponse::ptr rsp);    

};




}




}


#endif