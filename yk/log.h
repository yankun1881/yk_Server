#ifndef _YK_LOG_H__
#define _YK_LOG_H__
#include"singleton.h"
#include"util.h"
#include"thread.h"

#include<string>
#include<stdint.h>
#include<memory>
#include<iostream>

#include<sstream>
#include<fstream>
#include<map>
#include<list>
#include<vector>
 
#define YK_LOG_LEVEL(logger,level) \
    if(logger->getLevel() <= level) \
        yk::LogEventWrap(yk::LogEvent::ptr(new yk::LogEvent(logger,level, \
        __FILE__, __LINE__, 0, yk::GetThreadId(),\
         yk::GetFiberId(),yk::Thread::GetName(),time(0)))).getSS()

#define YK_LOG_DEBUG(logger) YK_LOG_LEVEL(logger,yk::LogLevel::DEBUG)
#define YK_LOG_INFO(logger)  YK_LOG_LEVEL(logger,yk::LogLevel::INFO)
#define YK_LOG_WARN(logger)  YK_LOG_LEVEL(logger,yk::LogLevel::WARN)
#define YK_LOG_ERROR(logger) YK_LOG_LEVEL(logger,yk::LogLevel::ERROR)
#define YK_LOG_FATAL(logger) YK_LOG_LEVEL(logger,yk::LogLevel::FATAL)


#define YK_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        yk::LogEventWrap(yk::LogEvent::ptr(new yk::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, yk::GetThreadId(),\
                yk::GetFiberId(),yk::Thread::GetName(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define YK_LOG_FMT_DEBUG(logger, fmt, ...) YK_LOG_FMT_LEVEL(logger, yk::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define YK_LOG_FMT_INFO(logger, fmt, ...)  YK_LOG_FMT_LEVEL(logger, yk::LogLevel::INFO, fmt, __VA_ARGS__)
#define YK_LOG_FMT_WARN(logger, fmt, ...)  YK_LOG_FMT_LEVEL(logger, yk::LogLevel::WARN, fmt, __VA_ARGS__)
#define YK_LOG_FMT_ERROR(logger, fmt, ...) YK_LOG_FMT_LEVEL(logger, yk::LogLevel::ERROR, fmt, __VA_ARGS__)
#define YK_LOG_FMT_ERROR(logger, fmt, ...) YK_LOG_FMT_LEVEL(logger, yk::LogLevel::ERROR, fmt, __VA_ARGS__)

#define YK_LOG_ROOT()  yk::LoggerMgr::GetInstance()->getRoot()
#define YK_LOG_NAME(name)  yk::LoggerMgr::GetInstance()->getLogger(name)



namespace yk{

                                            //打印10000条日志   100000        单位c_clock 
//typedef Mutex MutexType; //互斥量定义           50000            350000   
//typedef Spinlock MutexType; //自旋锁         200000~ 250000     35000000      可能是双核服务器比较慢
//typedef CASLock MutexType;   //乐观锁        120000~200000    35000000        
//typedef NullMutex MutexType; // 测试线程安全  
class Logger;   

//日志级别
class LogLevel{
public:
    enum Level{    
        UNKNOW = 0, 
        DEBUG = 1,
        INFO = 2,
        WARN = 3,
        ERROR = 4,
        FATAL = 5
    };
    static const char* ToString(LogLevel::Level level);
    static LogLevel::Level FromString(const std::string& str);
};


//日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level,const char* file,int32_t line, uint32_t elapse,
        u_int32_t thread_id, uint32_t fiber_id,std::string threadName,uint64_t time);
    ~LogEvent();
    
    const char*  getFile() const {return m_file;}
    int32_t  getLine() const {return m_line;}
    uint32_t  getElapse() const {return m_elapse;}
    uint32_t  getThreadId() const {return m_threadId;}
    uint32_t  getFiberId() const {return m_fiberId;}
    uint64_t   getTime() const {return m_time;}
    const std::string& getThreadName() const {return m_threadName;}
    std::shared_ptr<Logger> getLogger() const { return m_logger;}
            
    LogLevel::Level getLevel() const { return m_level;}
    std::string   getContent() const {return m_ss.str();}
    std::stringstream& getSS() { return m_ss;}
    void format(const char* fmt, ...);
    void format(const char* fmt,va_list al);
private:
    const char* m_file = nullptr;   //文件名
    int32_t m_line = 0;             //行号
    uint32_t m_elapse = 0;          //程序运行时间（ms）
    uint32_t m_threadId = 0;        //线程ID
    uint32_t m_fiberId = 0;         //协程ID
    std::string m_threadName;       //线程名 
    uint64_t m_time = 0;            //时间戳
    

    std::stringstream m_ss;         //日志内容流
    std::shared_ptr<Logger> m_logger;
     LogLevel::Level m_level;
};


class LogEventWrap{
public:
    LogEventWrap(LogEvent::ptr e);
    ~LogEventWrap();
    LogEvent::ptr getEvent() const { return m_event;}
    std::stringstream& getSS();
private:
    LogEvent::ptr m_event;
};


    
//日志格式器
class LogFormatter{
public:
    typedef std::shared_ptr<LogFormatter> ptr;

    std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event);
        void init();

    bool isError() const { return m_error;} // 解析是否出错
    LogFormatter(const std::string & pattern);
    const std::string getPattern() const{return m_pattern;}

    class FormatItem{
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem(){}
        virtual void format(std::ostream& os,std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

    };
    
    

private:
    /// 日志格式模板
    std::string m_pattern;
    /// 日志格式解析后格式
    std::vector<FormatItem::ptr> m_items;
    /// 是否有错误
    bool m_error = false; 

};





//日志输出地
class LogAppender{
public:
    typedef Mutex MutexType;
    typedef std::shared_ptr<LogAppender> ptr;

    virtual ~LogAppender(){}

    virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
    void setFormatter(LogFormatter::ptr val);
    LogFormatter::ptr getFormatter();
    virtual std::string toYamlString() = 0;

protected:
    LogLevel::Level m_level;
    LogFormatter::ptr m_formatter = LogFormatter::ptr(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T[%N]%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    MutexType m_mutex;


private:


};




//日志器
class Logger:public std::enable_shared_from_this<Logger>{
friend class LoggerManager;
public:
    typedef Mutex MutexType;
    typedef std::shared_ptr<Logger> ptr;
    
    Logger(const std::string & name ="root");
    
    void log(LogLevel::Level level,  LogEvent::ptr event);
    
    void debug(LogEvent::ptr event);
    void info(LogEvent::ptr event);
    void warn(LogEvent::ptr event);
    void erro(LogEvent::ptr event);
    void fatal(LogEvent::ptr event);
    
    void addAppender(LogAppender::ptr addAppender);
    void delAppender(LogAppender::ptr addAppender);
    
    void clearAppenders();

    LogLevel::Level getLevel() {return m_level;};
    void setLevel(LogLevel::Level val){m_level = val;}

    const std::string& getName() const {return m_name;};

    void setFormatter(LogFormatter::ptr val);
    void setFormatter(const std::string&  val);
    LogFormatter::ptr getFormatter();   

    std::string toYamlString();
private:
    std::string m_name;                         //日志名称
    LogLevel::Level  m_level;                  //日志级别
    std::list<LogAppender::ptr> m_appenders;    //Append集合
    LogFormatter::ptr m_formatter;
    Logger::ptr m_root; 
    MutexType m_mutex;
};

//控制台输出
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override;
    std::string toYamlString() override;

};

//输出到文件
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

    //重新打开文件，文件打开成功返回true
    bool reopen();
    std::string toYamlString() override;

    
private:
    std::string m_filename;
    std::ofstream m_filestream;
    uint64_t m_lastTime = 0;
};

//管理日志
class LoggerManager{
public:
    typedef Mutex MutexType;
    LoggerManager();
    Logger::ptr getLogger(const std::string& name);

    void init();
    Logger::ptr getRoot() const {return m_root;}

    std::string toYamlString();
private:

    std::map<std::string, Logger::ptr> m_loggers;
    //主日志器
    Logger::ptr m_root;
    MutexType m_mutex; 
};

//日志器管理单例模式
typedef yk::Singleton<LoggerManager> LoggerMgr;



};



#endif



