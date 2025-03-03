#include "cc_muduo/Timestamp.h"
#include "cc_muduo/EventLoop.h"
#include "cc_muduo/InetAddress.h"
#include "cc_muduo/Logger.h"
#include "cc_muduo/TcpClient.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <memory>

class EchoClient : public std::enable_shared_from_this<EchoClient>
{
public:
    EchoClient(EventLoop *loop, const InetAddress &serverAddr)
        : client_(loop, serverAddr, "EchoClient"),
          messageCount_(0)
    {
        client_.setConnectionCallback(
            [this](const TcpConnectionPtr &conn)
            { onConnection(conn); });
        client_.setMessageCallback(
            [this](const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
            {
                onMessage(conn, buf, time);
            });
    }

    void connect()
    {
        client_.connect();
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Client connected to %s", conn->peerAddress().toIpPort().c_str());

            // 连接成功后，发送一条消息
            conn->send("Hello from client!\n");
        }
        else
        {
            LOG_INFO("Client disconnected from %s", conn->peerAddress().toIpPort().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg(buf->retrieveAllAsString());
        std::cout << "Received message [" << msg << "] at " << time.toString() << std::endl;

        ++messageCount_;

        // 每次收到服务器响应后，发送下一条消息
        if (messageCount_ < 5) // 只发送5条消息
        {
            // 2秒后发送下一条消息
            conn->getLoop()->runAfter(std::chrono::seconds(2), [conn, weak_this = std::weak_ptr<EchoClient>(shared_from_this())]()
                                      {
            if (auto shared_this = weak_this.lock())
            {
                std::string nextMsg = "Message #" + std::to_string(shared_this->messageCount_) + " from client\n";
                LOG_INFO("Sending: %s", nextMsg.c_str());
                conn->send(nextMsg);
            }
            else
            {
                LOG_ERROR("EchoClient object has been destroyed, skipping sending message.");
            } });
        }
        else
        {
            LOG_INFO("Echo client finished.");
            conn->shutdown(); // 完成后主动关闭连接
        }
    }

    TcpClient client_;
    int messageCount_;
};

int main(int argc, char *argv[])
{
    LOG_INFO("pid = %d", getpid());

    // 设置服务器地址，默认为本地回环地址的8888端口
    std::string serverIp = "192.168.159.128";
    uint16_t serverPort = 8000;

    if (argc > 1)
        serverIp = argv[1];
    if (argc > 2)
        serverPort = static_cast<uint16_t>(std::stoi(argv[2]));

    InetAddress serverAddr(serverIp, serverPort);

    EventLoop loop;

    auto client = std::make_shared<EchoClient>(&loop, serverAddr);
    client->connect();

    loop.loop(); // 启动事件循环

    return 0;
}
