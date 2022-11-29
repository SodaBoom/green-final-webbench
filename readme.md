# web benchmark
0. 构建服务镜像
```sh
docker build -t green-webbench .
# docker run -dit -p 8080:8080 -v /home/wq/workplace/green-final-webbench:/root/green-webbench --name green-webbench green-webbench sh
docker run -dit -v /home/wq/workplace/green-final-webbench:/root/green-webbench --name green-webbench --network=host green-webbench sh
docker exec -it green-webbench sh
```

1. 编译（产物在output目录中)
```sh
make all
```

2. apache ab进行压测
并发1000个请求，一共发起100W个请求
```sh
ab -n 1000000 -c 1000 -k -r 127.0.0.1:8080/collect_energy/1/2
```

3. 启动服务器
```sh
docker exec -it green-webbench sh
cd /root/green-webbench
# 每个连接有一个线程处理
./output/conn_per_thread_server.out
# N个epoll
./output/epoll_server.out
# httplib库, epoll+线程池
./output/httplib_server.out.out
# cinatra库, aio
./output/cinatra_server.out.out

# java-springmvc (参数可能有误，本地跑出来的效果并不好)
java -Xmx4G -Xms4G -Xmn3G -XX:MetaspaceSize=256M -XX:MaxMetaspaceSize=256M -XX:+UseG1GC -XX:+AlwaysPreTouch -XX:+UseFastAccessorMethods -jar ./output/springmvc_server.jar

# java-flux (参数可能有误，本地跑出来的效果并不好)
java -Xmx3G -Xms3G -Xmn3G -XX:MetaspaceSize=256M -XX:MaxMetaspaceSize=256M -XX:+UseG1GC -XX:+AlwaysPreTouch -XX:+UseFastAccessorMethods -jar ./output/flux_server.jar
```
4. apache ab压测工具
100万个请求，1000个链接。
```sh
ab -n 1000000 -c 1000 -k -r 127.0.0.1:8080/collect_energy/1/2
# ab -m POST -n 1000000 -c 1000 -k -r 127.0.0.1:8080/collect_energy/1/2
```

5. 实验结果

| framework | qps (requests per second) |
| :-----| ----: |
| httplib_server | 21707 |
| cinatra_server | 97013 |
| conn_per_thread_server | 74274 |
| epoll_server | 106323 |

6. 参考
[Web Framework Benchmarks](https://www.techempower.com/benchmarks/#section=data-r21&test=plaintext)
