#单线程
Server Software:        yk/1.0.0
Server Hostname:        106.54.20.64
Server Port:            8020

Document Path:          /
Document Length:        133 bytes

Concurrency Level:      200
Time taken for tests:   222.905 seconds
Complete requests:      100000
Failed requests:        0
Non-2xx responses:      100000
Total transferred:      24200000 bytes
HTML transferred:       13300000 bytes
Requests per second:    448.62 [#/sec] (mean)
Time per request:       445.809 [ms] (mean)
Time per request:       2.229 [ms] (mean, across all concurrent requests)
Transfer rate:          106.02 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        3  302 939.6      3   31586
Processing:     3  118 737.7      3   54786
Waiting:        1  109 726.5      3   54786
Total:          6  420 1247.2      7   55812

Percentage of the requests served within a certain time (ms)
  50%      7
  66%      9
  75%    218
  80%   1008
  90%   1036
  95%   2049
  98%   3267
  99%   5055
 100%  55812 (longest request)

 #2线程
 Server Software:        yk/1.0.0
Server Hostname:        106.54.20.64
Server Port:            8021

Document Path:          /
Document Length:        133 bytes

Concurrency Level:      200
Time taken for tests:   214.129 seconds
Complete requests:      100000
Failed requests:        0
Non-2xx responses:      100000
Total transferred:      24200000 bytes
HTML transferred:       13300000 bytes
Requests per second:    467.01 [#/sec] (mean)
Time per request:       428.259 [ms] (mean)
Time per request:       2.141 [ms] (mean, across all concurrent requests)
Transfer rate:          110.37 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        3  301 941.5      3   64807
Processing:     3  117 659.5      3   49952
Waiting:        0  106 646.7      3   49952
Total:          6  418 1189.5      7   64829

Percentage of the requests served within a certain time (ms)
  50%      7
  66%     20
  75%    220
  80%   1011
  90%   1038
  95%   2040
  98%   3257
  99%   4647
 100%  64829 (longest request)
 
 
 
 #三线程
 Server Software:        yk/1.0.0
Server Hostname:        106.54.20.64
Server Port:            8022

Document Path:          /
Document Length:        133 bytes

Concurrency Level:      200
Time taken for tests:   217.881 seconds
Complete requests:      100000
Failed requests:        0
Non-2xx responses:      100000
Total transferred:      24200000 bytes
HTML transferred:       13300000 bytes
Requests per second:    458.97 [#/sec] (mean)
Time per request:       435.762 [ms] (mean)
Time per request:       2.179 [ms] (mean, across all concurrent requests)
Transfer rate:          108.47 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        3  293 903.8      3   32636
Processing:     3  125 773.6      3   49152
Waiting:        3  114 756.5      3   49152
Total:          6  418 1239.0      7   50184



#nginx 
Percentage of the requests served within a certain time (ms)
  50%      7
  66%      9
  75%    218
  80%   1008
  90%   1035
  95%   2045
  98%   3268
  99%   5563
 100%  50184 (longest request)

 Server Software:        nginx/1.18.0
Server Hostname:        106.54.20.64
Server Port:            80

Document Path:          /
Document Length:        612 bytes

Concurrency Level:      200
Time taken for tests:   373.982 seconds
Complete requests:      100000
Failed requests:        0
Total transferred:      85400000 bytes
HTML transferred:       61200000 bytes
Requests per second:    267.39 [#/sec] (mean)
Time per request:       747.965 [ms] (mean)
Time per request:       3.740 [ms] (mean, across all concurrent requests)
Transfer rate:          223.00 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        3  577 1805.5      3   65053
Processing:     3  157 767.7      3   68100
Waiting:        3  157 767.7      3   68100
Total:          6  734 2000.7      7   69120

Percentage of the requests served within a certain time (ms)
  50%      7
  66%    217
  75%   1017
  80%   1024
  90%   1885
  95%   3051
  98%   7177
  99%   7899
 100%  69120 (longest request)



 //经过测算带宽利用率都达到瓶颈 百分之四十以上，所以该框架已经是这个带宽极限了