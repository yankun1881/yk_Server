#include<iostream>
#include<string>
#include "../yk/log.cc"
#include "../yk/util.cc"

int main(int argc,char** argv){

    /* 自己定义log

    yk::Logger::ptr logger(new yk::Logger);
    logger->addAppender(yk::LogAppender::ptr(new yk::StdoutLogAppender()));
    yk::LogEvent::ptr event(new yk::LogEvent(__FILE__,__LINE__,0,yk::GeteThreadId(),yk::GeteFiberId(),time(0)));

    event->getSS() << std::string("hello,this is yk's server"); 
    
    logger->log(yk::LogLevel::DEBUG,event);*/
    
    /* 宏定义log

    yk::Logger::ptr logger(new yk::Logger);
    logger->addAppender(yk::LogAppender::ptr(new yk::StdoutLogAppender()));
    YK_LOG_DEBUG(logger) << std::string("hello,this is yk's server") ;
    */
    
    yk::Logger::ptr logger(new yk::Logger);
    logger->addAppender(yk::LogAppender::ptr(new yk::StdoutLogAppender()));
    yk::LogAppender::ptr file_appender(new yk::FileLogAppender("./log.txt"));
    //yk::LogFormatter::ptr fmt(new yk::LogFormatter("%d%T[%p]%T%m%n"));
    //file_appender->setFormatter(fmt);
    logger->addAppender(file_appender);
    YK_LOG_DEBUG(logger) << "t";
    YK_LOG_FMT_DEBUG(logger,"test macro fmt error ","aa");

    //auto l = yk::LoggerMgr::GetInstance()->getLogger("xx");
   // YK_LOG_INFO(l) << "XXX";

}
