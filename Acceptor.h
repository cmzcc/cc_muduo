#pragma once
#include"noncopyable.h"
#include "InetAddress.h" // 包含 InetAddress 类定义

#include"Socket.h"
#include<memory>
#include"Channel.h"
#include<functional>
class EventLoop;
class InetAddress;


class Acceptor:noncopyable
{
public:
    using NewConnectionCallback=std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);
    ~Acceptor();
    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_=cb;
    }
    bool listening()const {return listenning_;}
    void listen();
private:
    void handleRead(); 
    EventLoop *loop_;//Acceptor用的是用户定义的那个baseloop,也称作mainloop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};