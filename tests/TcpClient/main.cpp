//
// Created by liao on 2024/5/12.
//
#include <iostream>
#include <liblcc/inc/network/TcpClient.h>

uv_loop_t *g_loop = nullptr;

class TcpClient final : public Lcc::TcpClient, public Lcc::ClientImplement {
public:
    explicit TcpClient() : Lcc::TcpClient(this) {
    }

    inline bool IClientInit(Lcc::StreamHandle &handle) override {
        handle.tcpSession = 1;
        uv_tcp_init(g_loop, &handle.tcpHandle);
        return true;
    }

    inline void IClientReport(unsigned int session, bool connected, const char *err) override {
        if (connected) {
            std::cout << "IClientReport:[" << session << "] 连接成功" << std::endl;
            const char *request = "POST / HTTP/1.1\r\nconnection: keep-alive\r\nHost: www.baidu.com\r\nuser-agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 Safari/537.36\r\ncontent-type: text/html\r\nContent-Length: 0\r\n\r\n";
            Write(request, strlen(request));
        } else {
            std::cout << "IClientReport:[" << session << "] 连接失败 [" << err << "]" << std::endl;
        }
    }

    inline void IClientReceive(unsigned int session, const char *buf, unsigned int size) override {
        std::cout << "IClientReceive: [" << session << "] 接收消息, 长度[" << size << "]" << std::endl;
        Shutdown();
    }

    inline void IClientBeforeDisconnect(unsigned int session, int err, const char *errMsg) override {
        if (errMsg) {
            std::cout << "IClientBeforeDisconnect:[" << session << "] 异常断开连接 [" << errMsg << "]" << std::endl;
        } else {
            std::cout << "IClientBeforeDisconnect:[" << session << "] 断开连接" << std::endl;
        }
    }

    inline void IClientAfterDisconnect(unsigned int session) override {
        std::cout << "IClientAfterDisconnect:[" << session << "] 连接完全断开" << std::endl;
    }
};

int main(int argc, char *argv[]) {
    TcpClient client;

    g_loop = static_cast<uv_loop_t *>(::malloc(sizeof(uv_loop_t)));
    uv_loop_init(g_loop);

    // client.Connect("tcp://192.168.1.74:8081");
    client.Connect("https://www.baidu.com");

    uv_run(g_loop, UV_RUN_DEFAULT);
    uv_loop_close(g_loop);
    ::free(g_loop);
    g_loop = nullptr;
    return 0;
}
