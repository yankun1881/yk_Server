#include"log.h"
#include<map>
#include<iostream>
#include<functional>
#include <stdarg.h>
#include<string.h>
#include<time.h>
#include"config.h"
#include"env.h"
namespace yk{

class Logger;
class LogLevel;
class LogFormatter;



const char* LogLevel::ToString(LogLevel::Level level) {
    switch(level) {
#define XX(name) \
    case LogLevel::name: \
        return #name; \
        break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str){
    #define XX(name) \
        if(str == #name){\
            return LogLevel::name; \
        }  \
    
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
    return LogLevel::UNKNOW;
    #undef XX
}




LogEvent::LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level,const char* file,int32_t line, uint32_t elapse,
        u_int32_t thread_id, uint32_t fiber_id,std::string threadName, uint64_t time)
        :m_file(file)
        ,m_line(line)
        ,m_elapse(elapse)
        ,m_threadId(thread_id)
        ,m_fiberId(fiber_id)
        ,m_threadName(threadName)
        ,m_time(time)
        ,m_logger(logger)
        ,m_level(level) {


        }
LogEvent::~LogEvent(){

}

void LogEvent::format(const char* fmt, ...) {
    // 使用可变参数列表初始化
    va_list al;
    va_start(al, fmt);
    // 调用另一个重载的 format 函数
    format(fmt, al);
    va_end(al);
}

void LogEvent::format(const char* fmt, va_list al) {
    // 初始化一个空指针
    char* buf = nullptr;
    // 使用 vasprintf 函数格式化日志消息，并获取长度
    int len = vasprintf(&buf, fmt, al);
    // 如果格式化成功
    if (len != -1) {
        // 将格式化后的字符串追加到日志消息流中
        m_ss << std::string(buf, len);
        // 释放 buf 指向的内存
        free(buf);
    } 
}




LogEventWrap::LogEventWrap(LogEvent::ptr e)
:m_event(e){

}
LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event->getLevel(),m_event);
}
std::stringstream& LogEventWrap::getSS(){
    return m_event->getSS(); 
}
 


Logger::Logger(const std::string & name): 
    m_name(name),
    m_level(LogLevel::DEBUG) 
        {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T[%N]%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

        if(name == "root"){
            m_appenders.push_back(LogAppender::ptr(new StdoutLogAppender));
    }
}

void Logger::addAppender(LogAppender::ptr addAppender){
    MutexType::Lock lock(m_mutex);
    if(!addAppender->getFormatter()){
        addAppender->setFormatter(m_formatter);
    }

    m_appenders.push_back(addAppender);
}

void Logger::delAppender(LogAppender::ptr addAppender){
    MutexType::Lock lock(m_mutex);
    
    for(auto it = m_appenders.begin();
            it != m_appenders.end();++it){
                if(*it == addAppender ){
                    m_appenders.erase(it);
                    break;
                }
            }
}

void Logger::clearAppenders(){
    MutexType::Lock lock(m_mutex);

    m_appenders.clear();
}

void Logger::setFormatter(LogFormatter::ptr val){
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    for(auto & i : m_appenders){
        i->setFormatter(m_formatter);
    }
}
    
void Logger::setFormatter(const std::string&  val){
        yk::LogFormatter::ptr new_val(new yk::LogFormatter(val));
    if(new_val->isError()){
        std::cout <<" Logger setFormatter name = " << m_name
                    << " value = "<< val << " invalid formatter" << std::endl;
    }
    setFormatter(new_val); 
}

void LogAppender::setFormatter(LogFormatter::ptr val){
    MutexType::Lock lock(m_mutex);
    m_formatter = val;

}
LogFormatter::ptr LogAppender::getFormatter(){
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}




LogFormatter::ptr Logger::getFormatter(){
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

std::string Logger::toYamlString() {
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW) {
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }

    for(auto& i : m_appenders) {
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }   
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void Logger::log(LogLevel::Level level,  LogEvent::ptr event){
    if(level >= m_level){
        auto self = shared_from_this();
        MutexType::Lock lock(m_mutex);
        if(!m_appenders.empty()){
            for(auto& i : m_appenders)
            {
                i->log(self, level, event);
            }
        }else if(m_root){
            m_root->log(level,event); 
        }
    }    
}

void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}

void Logger::erro(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL, event);
}



FileLogAppender::FileLogAppender(const std::string& filename)
:m_filename(filename){
    if(!reopen()){
        std::cout << filename << "open errno";
    }
}

void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)  {
    if(level >= m_level){
        uint64_t now = time(0);
        if(now != m_lastTime){
            reopen();
            m_lastTime = now;
        }
        MutexType::Lock lock(m_mutex);
        if(!(m_filestream << m_formatter->format(logger,level,event))){
            std::cout <<"file cout error" << std::endl;
        }
    } 
}

bool FileLogAppender::reopen() {
    if (m_filestream.is_open()) {
        m_filestream.close();
    }

    m_filestream.open(m_filename, std::ios::out | std::ios::app);
    if (!m_filestream.is_open()) {
        std::cerr << "Failed to open file: " << m_filename << std::endl;
        return false;
    }

    return true;
}


void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)  {
    if(level >= m_level){
        std::string str = m_formatter->format(logger,level,event);
        std::cout << str <<std::endl;
    }  
}

std::string StdoutLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    std::stringstream ss;
    ss << node;
    return ss.str();
}

std::string FileLogAppender::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::ToString(m_level);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}
    

std::string LogFormatter::format(std::shared_ptr<Logger> logger,LogLevel::Level level,LogEvent::ptr event){
    std::stringstream ss;
    for(auto &i : m_items){
        i->format(ss,logger,level,event);
    }
    return ss.str();
}


class MessageFormatItem : public LogFormatter:: FormatItem{
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event)override{
        os << event->getContent();
    }   
};
class LevelFormatItem : public LogFormatter:: FormatItem{
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << LogLevel::ToString(level);
    }
};
class ElapseFormatItem : public LogFormatter:: FormatItem{
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem{
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter:: FormatItem{
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getThreadId();
    }
};

class ThreadNameFormatItem : public LogFormatter:: FormatItem{
public:
    ThreadNameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getThreadName();
    }
};

class FiberIdFormatItem : public LogFormatter:: FormatItem{
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter:: FormatItem{
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
    : m_format(format){
        if(m_format.empty()){
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time,&tm);
        char buf[64];

        strftime(buf,sizeof(buf),m_format.c_str(),&tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter:: FormatItem{
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter:: FormatItem{
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter:: FormatItem{
public:
    StringFormatItem(const std::string & str)
        : m_string(str) {};
    
    void format(std::ostream& os,std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)override{
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
private:
    std::string m_string;
};


LogFormatter::LogFormatter(const std::string & pattern)
    :m_pattern(pattern){
        init();
    }



//%xxx
    void LogFormatter::init(){
        std::vector<std::tuple<std::string,std::string,int>> vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i)
        {
        if(m_pattern[i] != '%'){
            nstr.append(1,m_pattern[i]);
            continue;
        }

        if((i+1) < m_pattern.size()){
            if(m_pattern[i+1] == '%'){
                nstr.append(1,'%');
                continue;
            }
        }

        
            size_t  n = i+1;
        int fmt_status = 0;
        int fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()) {
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; //解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }

        }
        if(!nstr.empty()){
            vec.push_back(std::make_tuple(nstr,std::string(),0));
    }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr( new C(fmt) );}}

        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:tab
        XX(F, FiberIdFormatItem),            //F:协程号
        XX(N, ThreadNameFormatItem),             //N:线程名
#undef XX
    };

    //%m -- 消息体
        //%p -- 等级
        //%r -- 启动后的时间
        //%c -- 日志名称
        //%t -- 线程id
        //%n -- 回车换行
        //%d -- 时间
        //%f -- 文件名
        //%l -- 行号
        for(auto &i : vec){
        if(std::get<2>(i) == 0){
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }else{
            auto  it =s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()){
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %"+ std::get<0>(i) + ">>")));
                }
                else{
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
//std::cout << "("<<std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) <<")"<<std::endl;
    }
    }

    LoggerManager::LoggerManager(){
        
        m_root.reset(new Logger);
        m_loggers[m_root->m_name] = m_root;

        init();
    }
    Logger::ptr LoggerManager::getLogger(const std::string& name){
        MutexType::Lock lock(m_mutex);
        
        auto it = m_loggers.find(name);
        if(it != m_loggers.end()){
            return it->second;
        }
        Logger::ptr logger(new Logger(name));
        m_loggers[name] = logger;
        logger->m_root = m_root;
        return logger;
    }
    


    struct LogAppenderDefine{
        int type = 0; // 1 file 2 stdout 
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator == (const LogAppenderDefine& oth) const {
            return type == oth.type
                && level == oth.level
                && formatter == oth.formatter
                && file == oth.file;
        }


    };

    struct LogDefine{
        std::string name ;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::vector<yk::LogAppenderDefine> appenders;


        bool operator == (const LogDefine& oth) const {
            return name == oth.name
                && level == oth.level
                && formatter == oth.formatter
                && appenders == oth.appenders;

        }

        bool operator < (const LogDefine& oth) const {
            return name < oth.name;

        }
    };




template<>
class LexicalCast<std::string, LogDefine>{
public:
    LogDefine operator()(const std::string& v) {
        YAML::Node n = YAML::Load(v);
        LogDefine ld;
        if(!n["name"].IsDefined()) {
            std::cout << "log config error: name is null, " << n
                      << std::endl;
            throw std::logic_error("log config name is null");
        }
        ld.name = n["name"].as<std::string>();
        ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
        if(n["formatter"].IsDefined()) {
            ld.formatter = n["formatter"].as<std::string>();
        }
        if(n["appenders"].IsDefined()) {
            //std::cout << "==" << ld.name << " = " << n["appenders"].size() << std::endl;
            for(size_t x = 0; x < n["appenders"].size(); ++x) {
                auto a = n["appenders"][x];
                if(!a["type"].IsDefined()) {
                    std::cout << "log config error: appender type is null, " << a
                              << std::endl;
                    continue;
                }
                std::string type = a["type"].as<std::string>();
                LogAppenderDefine lad;
                if(type == "FileLogAppender") {
                    lad.type = 1;
                    if(!a["file"].IsDefined()) {
                        std::cout << "log config error: fileappender file is null, " << a
                              << std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else if(type == "StdoutLogAppender") {
                    lad.type = 2;
                    if(a["formatter"].IsDefined()) {
                        lad.formatter = a["formatter"].as<std::string>();
                    }
                } else {
                    std::cout << "log config error: appender type is invalid, " << a
                              << std::endl;
                    continue;
                }

                ld.appenders.push_back(lad);
            }
        }
        return ld;
    }
};

template<>
class LexicalCast<LogDefine, std::string>{
public:
    std::string operator()(const LogDefine& i) {
        YAML::Node n;
        n["name"] = i.name;
        if(i.level != LogLevel::UNKNOW) {
            n["level"] = LogLevel::ToString(i.level);
        }
        if(!i.formatter.empty()) {
            n["formatter"] = i.formatter;
        }

        for(auto& a : i.appenders) {
            YAML::Node na;
            if(a.type == 1) {
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            } else if(a.type == 2) {
                na["type"] = "StdoutLogAppender";
            }
            if(a.level != LogLevel::UNKNOW) {
                na["level"] = LogLevel::ToString(a.level);
            }

            if(!a.formatter.empty()) {
                na["formatter"] = a.formatter;
            }

            n["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << n;
        return ss.str();
    }
};

yk::ConfigVar<std::set<LogDefine>>::ptr g_log_defines = 
    yk::Config::Lookup("logs",std::set<LogDefine>(),"logs config");


//从配置中读取 
struct LogIniter{
    LogIniter(){
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,const std::set<LogDefine>& new_value){
            YK_LOG_INFO(YK_LOG_ROOT()) << "on_logger_conf_changed";
            for(auto& i : new_value) {
                auto it = old_value.find(i);
                yk::Logger::ptr logger;
                if(it == old_value.end()) {
                    //新增logger
                    logger = YK_LOG_NAME(i.name);
                } else {
                    if(!(i == *it)) {
                        //修改的logger
                        logger = YK_LOG_NAME(i.name);
                    } else {
                        continue;
                    }
                }
                logger->setLevel(i.level);
                if(!i.formatter.empty()) {
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppenders();
                for(auto& a : i.appenders) {
                    yk::LogAppender::ptr ap;
                    if(a.type == 1) {
                        ap.reset(new FileLogAppender(a.file));
                    } else if(a.type == 2) {
                        if(!yk::EnvMgr::GetInstance()->has("d")) {
                            ap.reset(new StdoutLogAppender);
                        } else {
                            continue;
                        }
                    }
                    if(!a.formatter.empty()) {
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()) {
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    else{
                        if(!i.formatter.empty()) {
                                ap->setFormatter(LogFormatter::ptr(new LogFormatter(i.formatter)));
                        }
                    }
                    logger->addAppender(ap);
                }
            }
 
            for(auto& i : old_value) {
                auto it = new_value.find(i);
                if(it == new_value.end()) {
                    //删除logger
                    auto logger = YK_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)0);
                    logger->clearAppenders();
                }
            }
        });
    }
};

static LogIniter __log_init;
std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_loggers) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}
    void LoggerManager::init() {
    }
}