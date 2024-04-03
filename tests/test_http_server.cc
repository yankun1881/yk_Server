#include "http/http.h"
#include "http/http_server.h"
#include "yk/log.h"

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");
void run(){
    yk::http::HttpServer::ptr server(new yk::http::HttpServer);
    yk::Address::ptr addr = yk::Address::LookupAnyIPAddress("10.0.16.9:8031");
    while(!server->bind(addr)){
        sleep(2);

    }
    auto sd =server->getServletDispatch();

    sd->addServlet("/yk/xx",[](yk::http::HttpRequest::ptr req
                        ,yk::http::HttpResponse::ptr rsp
                        ,yk::http::HttpSession::ptr session){
                            rsp->setBody(req->toString());
                            return 0;
                        });
    sd->addGlobServlet("/yk/*",[](yk::http::HttpRequest::ptr req
                        ,yk::http::HttpResponse::ptr rsp
                        ,yk::http::HttpSession::ptr session){
                            rsp->setBody("Glob:\r\n"+req->toString());
                            return 0;
                        });
    


    server->start();

}

int main(int argc, char** argv){
    yk::IOManager iom(2);
    iom.schedule(run);
    return 0;
}