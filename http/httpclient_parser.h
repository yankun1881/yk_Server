/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * @file httpclient_parser.h
 * @brief HTTP 客户端解析器头文件
 */

#ifndef httpclient_parser_h
#define httpclient_parser_h

#include "http11_common.h"

typedef struct httpclient_parser {
  int cs;                // 当前解析状态
  size_t body_start;      // 响应体起始位置
  int content_len;        // 内容长度
  int status;             // 状态码
  int chunked;            // 是否分块传输编码
  int chunks_done;        // 分块传输是否完成
  int close;              // 连接是否关闭
  size_t nread;           // 已读取字节数
  size_t mark;            // 标记位置
  size_t field_start;     // 字段起始位置
  size_t field_len;       // 字段长度

  void *data;             // 数据

  field_cb http_field;    // HTTP 字段回调函数
  element_cb reason_phrase;  // 原因短语回调函数
  element_cb status_code;    // 状态码回调函数
  element_cb chunk_size;     // 分块大小回调函数
  element_cb http_version;   // HTTP 版本回调函数
  element_cb header_done;    // 头部解析完成回调函数
  element_cb last_chunk;     // 最后一个分块回调函数
} httpclient_parser;

int httpclient_parser_init(httpclient_parser *parser);   // 初始化解析器
int httpclient_parser_finish(httpclient_parser *parser); // 完成解析
int httpclient_parser_execute(httpclient_parser *parser, const char *data, size_t len, size_t off); // 执行解析
int httpclient_parser_has_error(httpclient_parser *parser);   // 检查是否发生错误
int httpclient_parser_is_finished(httpclient_parser *parser);  // 检查解析是否完成

#define httpclient_parser_nread(parser) (parser)->nread  // 获取已读取字节数

#endif

