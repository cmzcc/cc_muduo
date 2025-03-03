# cc_muduo: A multithreaded C++ network library

[![Build Status](https://travis-ci.org/guangqianpeng/tinyev.svg?branch=master)](https://travis-ci.org/guangqianpeng/tinyev)

## 简介

cc_muduo是仿照muduo[1]实现的一个基于Reactor模式的多线程C++网络库，代码量约为3500行。

- 多线程依赖于C++11提供的std::thread库，而不是重新封装POSIX thread API。

- 定时器依赖于C++11提供的std::chrono库，而不是自己实现Timstamp类，也不用直接调用`gettimeofday()`。这样写的好处之一是我们不必再为定时器API的时间单位操心[2]：

  ```c++
    TimerId runAt(TimePoint time, Timer::TimerCallback cb);
    TimerId runAfter(Duration delay, Timer::TimerCallback cb);
    TimerId runEvery(Duration interval, Timer::TimerCallback cb);
  ```

- 默认为accept socket开启SO_RESUEPORT选项，这样每个线程都有自己的Acceptor，就不必在线程间传递connection socket。开启该选项后内核会将TCP连接分摊给各个线程，因此不必担心负载均衡的问题。

- 仅具有简单的日志输出功能，用于调试。

- 仅使用epoll，不使用poll和select。

## 示例

一个简单的echo服务器如下：

```C++
#include<cc_muduo/TcpServer.h>
#include<cc_muduo/Logger.h>
#include<cc_muduo/InetAddress.h>
#include<string>
#include<functional>

class EchoServer
{
public:
    EchoServer(EventLoop *loop,
            const InetAddress &addr,
            const std::string &name)
        :server_(loop,addr,name)
        ,loop_(loop)
    {
        //注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection,this,std::placeholders::_1)
        );
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage,this,
                std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)
        );
        //设置合适的loop线程数量
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }
private:
    //连接建立或断开的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
          if (conn->connected())
          {
            LOG_INFO("Conn UP : %s",conn->peerAddress().toIpPort().c_str());
          }
          else
          {
              LOG_INFO("Conn DOWN : %s", conn->peerAddress().toIpPort().c_str());
          }
          
             
    }
    //可读写事件回调
    void onMessage(const TcpConnectionPtr &conn,
                    Buffer *buf,
                    Timestamp time)
    {
        std::string msg=buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();//关闭写端 EPOLLUP-> closeCallback_
    }
    EventLoop *loop_;
    TcpServer server_;
};

```

这个实现非常简单，读者只需关注`onMessage`回调函数，它将收到消息发回客户端。


然后，我们给服务器加上多线程功能。实现起来非常简单，只需加一行代码即可：

```c++
EchoServer：：void start()
{
  // set thread num here
  server_.setNumThread(2);
  server_.start();
}
```

最后，main函数如下：

```c++
int main()
{
    EventLoop loop;
    std::string serverIp = "192.168.159.128";
    uint16_t serverPort = 8000;



    InetAddress addr(serverIp, serverPort);
    EchoServer server(&loop,addr,"EchoServer-01");//Acceptoer non-blocking listenfd  create bind
    server.start();//listen loopthread  listenfd->AcceptChannel ->mainLoop->
    loop.loop();//启动mainLoop 的底层Poller

    return 0;
}
```

## 安装

```shell
$ git clone git@github.com:cmzcc/cc_muduo.git
$ cd cc_muduo
$ sudo ./autobuild.sh
```

会自动生成在 `/usr/lib/libcc_muduo.so`

## 更多

更多c++相关请来我的[博客](https://cmzcc.github.io)。

## 参考

[[1]](https://github.com/chenshuo/muduo) Muduo is a multithreaded C++ network library based on the reactor pattern.

[[2]](https://www.bilibili.com/video/BV1UE4m1R72y/?p=53&spm_id_from=333.1007.top_right_bar_window_history.content.click) 在此基础上增加计时器和客户端功能