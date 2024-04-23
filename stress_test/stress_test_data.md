## 三线程 
Server Software: yk/1.0.0

Server Hostname: 106.54.20.64

Server Port: 8022

  

Document Path: /

Document Length: 133 bytes

  

Concurrency Level: 200

Time taken for tests: 217.881 seconds

Complete requests: 100000

Failed requests: 0

Non-2xx responses: 100000

Total transferred: 24200000 bytes

HTML transferred: 13300000 bytes

Requests per second: 458.97 [#/sec] (mean)

Time per request: 435.762 [ms] (mean)

Time per request: 2.179 [ms] (mean, across all concurrent requests)

Transfer rate: 108.47 [Kbytes/sec] received

  

Connection Times (ms)

min mean[+/-sd] median max

Connect: 3 293 903.8 3 32636

Processing: 3 125 773.6 3 49152

Waiting: 3 114 756.5 3 49152

Total: 6 418 1239.0 7 50184

## nginx

Percentage of the requests served within a certain time (ms)

50% 7

66% 9

75% 218

80% 1008

90% 1035

95% 2045

98% 3268

99% 5563

100% 50184 (longest request)

  

Server Software: nginx/1.18.0

Server Hostname: 106.54.20.64

Server Port: 80

  

Document Path: /

Document Length: 612 bytes

  

Concurrency Level: 200

Time taken for tests: 373.982 seconds

Complete requests: 100000

Failed requests: 0

Total transferred: 85400000 bytes

HTML transferred: 61200000 bytes

Requests per second: 267.39 [#/sec] (mean)

Time per request: 747.965 [ms] (mean)

Time per request: 3.740 [ms] (mean, across all concurrent requests)

Transfer rate: 223.00 [Kbytes/sec] received

  

Connection Times (ms)

min mean[+/-sd] median max

Connect: 3 577 1805.5 3 65053

Processing: 3 157 767.7 3 68100

Waiting: 3 157 767.7 3 68100

Total: 6 734 2000.7 7 69120

  

Percentage of the requests served within a certain time (ms)

50% 7

66% 217

75% 1017

80% 1024

90% 1885

95% 3051

98% 7177

99% 7899

100% 69120 (longest request)



## 我的云服务器4M带宽，经过测算带宽利用率都达到瓶颈 百分之四十以上，发送+响应达到极限

  

## 本机ip进行压测 三线程

ubuntu@VM-16-9-ubuntu:~$ ab -n 100000 -c 200 http://127.0.0.1:8022/

This is ApacheBench, Version 2.3 <$Revision: 1879490 $>

Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/

Licensed to The Apache Software Foundation, http://www.apache.org/

  

Benchmarking 127.0.0.1 (be patient)

Completed 10000 requests

Completed 20000 requests

Completed 30000 requests

Completed 40000 requests

Completed 50000 requests

Completed 60000 requests

Completed 70000 requests

Completed 80000 requests

Completed 90000 requests

Completed 100000 requests

Finished 100000 requests

  
  

Server Software: yk/1.0.0

Server Hostname: 127.0.0.1

Server Port: 8022

  

Document Path: /

Document Length: 338 bytes

  

Concurrency Level: 200

Time taken for tests: 9.986 seconds

Complete requests: 100000

Failed requests: 0

Non-2xx responses: 100000

Total transferred: 46200000 bytes

HTML transferred: 33800000 bytes

Requests per second: 10014.13 [#/sec] (mean)

Time per request: 19.972 [ms] (mean)

Time per request: 0.100 [ms] (mean, across all concurrent requests)

Transfer rate: 4518.10 [Kbytes/sec] received

  

Connection Times (ms)

min mean[+/-sd] median max

Connect: 0 6 5.4 6 34

Processing: 0 13 9.9 13 215

Waiting: 0 11 4.3 10 39

Total: 0 20 11.0 19 222
Percentage of the requests served within a certain time (ms)

50% 19

66% 22

75% 23

80% 25

90% 28

95% 30

98% 35

99% 40

100% 222 (longest request)

## nginx

ubuntu@VM-16-9-ubuntu:~$ ab -n 100000 -c 200 http://127.0.0.1:8092/

This is ApacheBench, Version 2.3 <$Revision: 1879490 $>

Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/

Licensed to The Apache Software Foundation, http://www.apache.org/

  

Benchmarking 127.0.0.1 (be patient)

Completed 10000 requests

Completed 20000 requests

Completed 30000 requests

Completed 40000 requests

Completed 50000 requests

Completed 60000 requests

Completed 70000 requests

Completed 80000 requests

Completed 90000 requests

Completed 100000 requests

Finished 100000 requests

  
  

Server Software: nginx/1.18.0

Server Hostname: 127.0.0.1

Server Port: 8092

  

Document Path: /

Document Length: 162 bytes

  

Concurrency Level: 200

Time taken for tests: 6.188 seconds

Complete requests: 100000

Failed requests: 0

Non-2xx responses: 100000

Total transferred: 32100000 bytes

HTML transferred: 16200000 bytes

Requests per second: 16159.42 [#/sec] (mean)

Time per request: 12.377 [ms] (mean)

Time per request: 0.062 [ms] (mean, across all concurrent requests)

Transfer rate: 5065.60 [Kbytes/sec] received

  

Connection Times (ms)

min mean[+/-sd] median max

Connect: 0 5 1.8 5 16

Processing: 1 7 2.6 7 30

Waiting: 0 6 2.4 5 29

Total: 3 12 3.4 11 32

  

Percentage of the requests served within a certain time (ms)

50% 11

66% 13

75% 14

80% 14

90% 17

95% 19

98% 22

99% 24

100% 32 (longest request)

  

### 结论

qbs ykServer : nginx 10000:16000

传输速率(kb/s) ykServer : nginx 4518.10 : 5274.40

效率 ykServer:nginx 0.8566: 1

大约比nginx慢百分之15