//
// Created by liao on 2024/5/12.
//
#include <iostream>
#include <liblcc/inc/network/TcpServer.h>
#include <network/plugin/WebSocketPlugin.h>

uv_loop_t *g_loop = nullptr;

class WebSocketServer final : public Lcc::TcpServer, public Lcc::ServerImplement {
public:
    explicit WebSocketServer() : Lcc::TcpServer(this) {
    }

    bool IServerInit(uv_tcp_t *handle) override {
        uv_tcp_init(g_loop, handle);
        return true;
    }

    void IServerListenReport(bool listened, int err, const char *errMsg) override {
        Lcc::Utils::HostAddress address = GetListenAddress();
        if (listened) {
            std::cout << "IServerReport: 监听地址[" << address.ip << ":" << address.port << "] 成功" << std::endl;
        } else {
            std::cout << "IServerReport: 监听地址[" << address.ip << ":" << address.port << "] 失败 [" << err << ":" << errMsg
                    << "]" << std::endl;
        }
    }

    void IServerShutdown() override {
        std::cout << "IServerShutdown: 监听完全关闭" << std::endl;
    }

    void IServerSessionOpen(unsigned int session) override {
        std::cout << "IServerSessionOpen:[" << session << "] 连接成功" << std::endl;
    }

    void IServerSessionReceive(unsigned int session, const char *buf, unsigned int size) override {
        std::cout << "IServerSessionReceive: [" << session << "] 接收消息, 长度[" << size << "]" << std::endl;
        const char *request =
                "POST / HTTP/1.1\r\nconnection: keep-alive\r\nHost: www.baidu.com\r\nuser-agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.132 Safari/537.36\r\ncontent-type: text/html\r\nContent-Length: 0\r\n\r\n";
        SessionWrite(session, request, strlen(request));
        // Shutdown();
    }

    void IServerSessionBeforeClose(unsigned int session, int err, const char *errMsg) override {
        if (errMsg) {
            std::cout << "IServerSessionBeforeClose:[" << session << "] 异常断开连接 [" << errMsg << "]" << std::endl;
        } else {
            std::cout << "IServerSessionBeforeClose:[" << session << "] 断开连接" << std::endl;
        }
    }

    void IServerSessionAfterClose(unsigned int session) override {
        std::cout << "IServerSessionAfterClose:[" << session << "] 连接完全断开" << std::endl;
    }
};

int main(int argc, char *argv[]) {
    WebSocketServer server;

    g_loop = static_cast<uv_loop_t *>(::malloc(sizeof(uv_loop_t)));
    uv_loop_init(g_loop);

    auto websocket = new Lcc::WebSocketPluginCreator;
    websocket->InitializeServerMode(Lcc::WebSocketOpcode::Text);
    server.Enable(websocket);
    server.Listen("tcp://127.0.0.1:8080");

    uv_run(g_loop, UV_RUN_DEFAULT);
    uv_loop_close(g_loop);
    ::free(g_loop);
    g_loop = nullptr;
    return 0;
}
