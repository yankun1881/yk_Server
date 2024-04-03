#include "http/http.h"
#include "http/http_parser.h"
#include "yk/log.h"

static yk::Logger::ptr g_logger = YK_LOG_NAME("system");

 char test_request_data[] = "POST / HTTP/1.1\r\n"
                                "Host: www.baidu.com\r\n"
                                "Content-Length: 10\r\n\r\n"
                                "1234567890";

void test_request(){
    yk::http::HttpRequest::ptr req(new yk::http::HttpRequest);
    req->setHeader("host","www.baidu.com");
    req->setBody("hello yk");

    req->dump(std::cout) << std::endl;
}

void test_response(){
    yk::http::HttpResponse::ptr rsp(new yk::http::HttpResponse);
    rsp->setHeader("X-X","yk");
    rsp->setBody("hello yk");
    rsp->setStatus((yk::http::HttpStatus)400);
    rsp->setClose(false);
    rsp->dump(std::cout) << std::endl;
}

void test_request_parser(){
    yk::http::HttpRequestParser parser;
    std::string tmp = test_request_data;
    size_t s = parser.execute(test_request_data,tmp.size());
    YK_LOG_INFO(g_logger) << "execute rt = " << s
        << " has_error = " << parser.hasError()
        <<" is_finished = " << parser.isFinished();
    YK_LOG_INFO(g_logger) << parser.getData()->toString();
}

const char test_response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";

void test_response_parser() {
    yk::http::HttpResponseParser parser;
    std::string tmp = test_response_data;
    size_t s = parser.execute(&tmp[0], tmp.size(), true);
    YK_LOG_ERROR(g_logger) << "execute rt=" << s
        << " has_error=" << parser.hasError()
        << " is_finished=" << parser.isFinished()
        << " total=" << tmp.size()
        << " content_length=" << parser.getContentLength()
        << " tmp[s]=" << tmp[s];

    tmp.resize(tmp.size() - s);

    YK_LOG_INFO(g_logger) << parser.getData()->toString();
    YK_LOG_INFO(g_logger) << tmp;
}



int main(int argc, char** argv){
    //test_response();
    //test_request_parser();
    test_response_parser();
    return 0;
}