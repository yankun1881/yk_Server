
  

# C++高性能服务器框架

  

  

此项目主要是我跟随 [sylar](https://github.com/sylar-yin/sylar)完成。该项目实现了一个高性能分布式服务器框架。并且使用该服务器框架开发了[在线聊天室](http://www.ykchatroom.online) 主要特点：

  

  

- 协程模块使用ucontext实现了协程库。协程调度采用非对称调度方式。

  

- IO协程调度模块，封装了epoll（Linux），并支持定时器功能（使用epoll实现定时器，精度毫秒级）

  

- 采用[http-parser](https://github.com/nodejs/http-parser)，实现了HTTP/1.1的简单协议实现和uri的解析

  

- 新增websocket协议，并采用websocket协议实现在线聊天室

  

- 使用异步通信协议进行分布式服务器之间通信，采用Protobuf编码压缩数据，使用zookeeper管理服务。（我将在[聊天室项目](https://github.com/yankun1881/yk_chat_room)实现rpc调用及消息队列）

  
  

## 项目环境

  

- Ubuntu 22.04.4 LTS

  

## 项目依赖

  

#### CMake

sudo apt install cmake

  

#### yamlcpp

sudo apt install libyaml-cpp-dev

  

#### jsoncpp

sudo apt install libjsoncpp-dev

  

#### OpenSSL

sudo apt install libssl-dev

  

#### MySQLCppCONN

sudo apt install libmysqlcppconn-dev

  

#### boost库

sudo apt install libboost-all-dev

  

#### zookeeper_mt
sudo apt install libzookeeper_mt-dev
  
  

## 项目介绍

  

### 1.日志模块

  

  

支持流式日志风格写日志和格式化风格写日志，支持日志格式自定义，日志级别，多日志分离等等功能<br />

  

默认支持每日创建新日志文件，配置路径为相对路径。<br />

  

流式日志使用：YK_LOG_INFO(g_logger) << "this is a log" ;<br />

  

格式化日志使用：YK_LOG_FMT_INFO(g_logger, "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n", "this is a log");<br />


### 2.配置模块

采用约定由于配置的思想。定义即可使用。不需要单独去解析。支持变更通知功能。使用YAML文件做为配置内容。

解析过程为递归执行，支持级别复合格式的数据类型，支持STL容器(vector,list,set,map等等)。


## 3.线程模块

线程模块，封装了pthread里面的一些常用功能，线程，互斥锁，读写锁，自旋锁，乐观锁等对象。
## 4.协程模块

协程是基于ucontext_t来实现的，配置socket hook，可以把复杂的异步调用，封装成同步操作。

## 5.协程调度模块

协程调度器，管理协程的调度，内部实现为一个线程池，支持协程在多线程中切换，协程调度采用非对称调度方式。

## 6.IO协程调度模块

继承与协程调度器，封装了epoll（Linux），并支持定时器功能（使用epoll实现定时器，精度毫秒级）,支持Socket读写时间的添加，删除，取消功能。支持一次性定时器，循环定时器，条件定时器等功能。

## 7.Hook模块

hook系统底层和socket相关的API，socket io相关的API，以及sleep系列的API。hook的开启控制是线程粒度的。可以自由选择。

## 8.Socket模块

  

封装了Socket类，提供所有socket API功能，统一封装了地址类，将IPv4，IPv6，Unix地址统一起来。并且提供域名，IP解析功能。

 

## 9.TcpServer模块

  

基于Socket类，封装了一个通用的TcpServer的服务器类，提供简单的API，使用便捷，可以快速绑定一个或多个地址，启动服务，监听端口，accept连接，处理socket连接等功能。具体业务功能更的服务器实现，只需要继承该类就可以快速实现

  

  

## 10.HTTP模块

  

采用Ragel（有限状态机，性能媲美汇编），实现了HTTP/1.1的简单协议实现和uri的解析。基于SocketStream实现了HttpConnection(HTTP的客户端)和HttpSession(HTTP服务器端的链接）。基于TcpServer实现了HttpServer。提供了完整的HTTP的客户端API请求功能，HTTP基础API服务器功能

## 11.Servlet模块

仿照java的servlet，实现了一套Servlet接口，实现了ServletDispatch，FunctionServlet。NotFoundServlet。支持uri的精准匹配，模糊匹配等功能。和HTTP模块，一起提供HTTP服务器功能
  

## 13.服务器启动模块

  

服务器启动的读取配置和启动，实现了读取日志配置，读取服务器类型配置，读取线程分配配置，读取mysql配置。可以选择通过守护进程启动。

  

## 新增部分

  

### websocket协议

websocket继承http ，复用http的连接

### mysql 连接

可以通过读取配置进行mysql的连接，新增数据库连接池，可配置超时时间以及最少连接数。

### redis连接
可以配置哨兵模式，读写分离。

### rock模块
自定义通信协议，采用Protobuf编码压缩数据

### 服务管理
采用zookeeper管理服务，并实现服务的发现与注册，负载均衡。
