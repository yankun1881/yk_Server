# C++高性能服务器框架

此项目主要是我跟随 [sylar](https://github.com/sylar-yin/sylar)的[从零开始开发服务器框架](https://www.bilibili.com/video/BV184411s7qF)视频完成。该项目完整的实现了一个高性能服务器框架的所需的基本模块。主要特点：

 - 协程模块使用ucontext实现了协程库。协程调度采用非对称调度方式。
 - IO协程调度模块，封装了epoll（Linux），并支持定时器功能（使用epoll实现定时器，精度毫秒级）
 - 采用Ragel（有限状态机，性能媲美汇编），实现了HTTP/1.1的简单协议实现和uri的解析

## 1.日志模块

支持流式日志风格写日志和格式化风格写日志，支持日志格式自定义，日志级别，多日志分离等等功能 
流式日志使用：YK_LOG_INFO(g_logger)  << "this is a log" ; 
格式化日志使用：YK_LOG_FMT_INFO(g_logger, "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n", "this is a log"); 

| 格式化字符 |含义  |
|--|--|
|%m  |  消息内容|
| %p | 日志等级 |
|%c  |  日志名称|
|  %t|线程ID  |
|%n|换行符|
| %d | 日志时间 |
| %f| 文件名|
|%l|行号|
| %T| 制表符 |
| %N| 线程名称|

默认格式%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n


## 2.配置模块

采用约定由于配置的思想。定义即可使用。不需要单独去解析。支持变更通知功能。使用YAML文件做为配置内容。
解析过程为递归执行，支持级别复合格式的数据类型，支持STL容器(vector,list,set,map等等)。

yk::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");
g_log_defines->addListener([](const std::set<LogDefine>& old_value,const std::set<LogDefine>& new_value){....}//配置更改
static yk::ConfigVar<int>::ptr g_tcp_connect_timeout =
	yk::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout"); //更改超时时间


## 3.线程模块

线程模块，封装了pthread里面的一些常用功能，线程，互斥锁，读写锁，自旋锁，乐观锁等对象。

## 4.协程模块

协程是基于ucontext_t来实现的，配置socket hook，可以把复杂的异步调用，封装成同步操作。

## 5.协程调度模块

协程调度器，管理协程的调度，内部实现为一个线程池，支持协程在多线程中切换，协程调度采用非对称调度方式。

## 6.IO协程调度模块

继承与协程调度器，封装了epoll（Linux），并支持定时器功能（使用epoll实现定时器，精度毫秒级）,支持Socket读写时间的添加，删除，取消功能。支持一次性定时器，循环定时器，条件定时器等功能。
yk::IOManager iom(2); //两个线程

## 7.Hook模块
hook系统底层和socket相关的API，socket io相关的API，以及sleep系列的API。hook的开启控制是线程粒度的。可以自由选择。
## 8.Socket模块
封装了Socket类，提供所有socket API功能，统一封装了地址类，将IPv4，IPv6，Unix地址统一起来。并且提供域名，IP解析功能。



