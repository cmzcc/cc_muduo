# 默认目标
all: testserver testClient

# 编译服务器
testserver:
	g++ -g -o testserver testserver.cpp -L/lib -lcc_muduo -lpthread

# 编译客户端
testClient:
	g++ -g -o testClient testClient.cpp -L/lib -lpthread -lcc_muduo


# 清理目标
clean:
	rm -f testserver
	rm -f testClient
