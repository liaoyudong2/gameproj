//
// Created by liao on 2024/5/3.
//

#ifndef LCC_TCPCLIENT_H
#define LCC_TCPCLIENT_H

#include "utils/Address.h"
#include "network/TcpStream.h"

namespace Lcc {
    class TcpClient : public StreamImplement {
        enum class Status {
            None,
            Address,
            Init,
            Connecting,
            Connected,
            ConnectFail,
        };

    public:
        explicit TcpClient(ClientImplement *impl);

        ~TcpClient() override;

        /**
         * 连接远端地址
         * @param host 远端地址
         */
        void Connect(const char *host);

        /**
         * 关闭连接
         */
        void Shutdown();

        /**
         * 启用协议插件支持
         * @param creator 协议插件创造器
         */
        void Enable(ProtocolPluginCreator *creator);

        /**
         * 向流写数据
         * @param buf 数据流
         * @param size 数据长度
         */
        void Write(const char *buf, unsigned int size);

        /**
         * 获取会话id
         * @return 会话id
         */
        unsigned int GetSession() const;

    protected:
        /**
         * 解析地址
         */
        void AddressParse();

        /**
         * 解析地址后的连接
         */
        void AddressConnecting();

        /**
         * 连接地址失败
         * @param status 失败错误码
         */
        void AddressConnectFail(int status);

    protected:
        bool IStreamInit(StreamHandle &handle) override;

        void IStreamOpen(unsigned int session) override;

        void IStreamReceive(unsigned int session, const char *buf, unsigned int size) override;

        void IStreamBeforeClose(unsigned int session, int err, const char *errMsg) override;

        void IStreamAfterClose(unsigned int session) override;

    protected:
        static void UvAddressParseCallback(uv_getaddrinfo_t *info, int status, addrinfo *res);

        static void UvConnectStatusCallback(uv_connect_t *req, int status);

    private:
        Status _status;
        uv_tcp_t *_handle;
        TcpStream *_tcpStream;
        ClientImplement *_implement;

    private:
        Utils::HostAddress _hostAddress;
        std::vector<ProtocolPluginCreator *> _creatorVec;
    };
}

#endif //LCC_TCPCLIENT_H
