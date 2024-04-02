#ifndef http11_parser_h
#define http11_parser_h

#include "http11_common.h"

// HTTP解析器结构体
typedef struct http_parser { 
  int cs;               // 当前状态
  size_t body_start;    // 消息体开始位置
  int content_len;      // 消息体长度
  size_t nread;         // 读取的字节数
  size_t mark;          // 标记位置
  size_t field_start;   // 字段开始位置
  size_t field_len;     // 字段长度
  size_t query_start;   // 查询字符串开始位置
  int xml_sent;         // 是否发送了XML
  int json_sent;        // 是否发送了JSON

  void *data;           // 用户数据

  int uri_relaxed;      // URI是否宽松模式
  field_cb http_field;  // HTTP字段回调函数
  element_cb request_method;  // 请求方法回调函数
  element_cb request_uri;      // 请求URI回调函数
  element_cb fragment;         // 片段回调函数
  element_cb request_path;     // 请求路径回调函数
  element_cb query_string;     // 查询字符串回调函数
  element_cb http_version;     // HTTP版本号回调函数
  element_cb header_done;      // 头部解析完成回调函数
  
} http_parser;

// 初始化HTTP解析器
int http_parser_init(http_parser *parser);

// 结束HTTP解析
int http_parser_finish(http_parser *parser);

// 执行HTTP解析
size_t http_parser_execute(http_parser *parser, const char *data, size_t len, size_t off);

// 判断解析是否有错误
int http_parser_has_error(http_parser *parser);

// 判断解析是否结束
int http_parser_is_finished(http_parser *parser);

// 获取已读取的字节数
#define http_parser_nread(parser) (parser)->nread 

#endif
