//
// Created by liao on 2024/5/14.
//

#ifndef LCC_TCPSERVER_H
#define LCC_TCPSERVER_H

#include <unordered_map>
#include "utils/Address.h"
#include "network/Interface.h"
#include "network/TcpStream.h"

namespace Lcc {
    class TcpServer : public StreamImplement {
        enum class Status {
            None,
            Address,
            Init,
            Listened,
            ListenFail,
        };

    public:
        TcpServer(ServerImplement *impl);

        ~TcpServer() override;

        void Listen(const char *host);

    protected:
        /**
         * 解析地址
         */
        void AddressParse();

        /**
         * 解析地址后的监听
         */
        void AddressListening();

        /**
         * 监听地址失败
         * @param status 失败错误码
         */
        void AddressListenFail(int status);

    protected:
        bool IStreamInit(StreamHandle &handle) override;

        void IStreamOpen(unsigned int session) override;

        void IStreamReceive(unsigned int session, const char *buf, unsigned int size) override;

        void IStreamBeforeClose(unsigned int session, int err, const char *errMsg) override;

        void IStreamAfterClose(unsigned int session) override;

    protected:
        static void UvAddressParseCallback(uv_getaddrinfo_t *info, int status, addrinfo *res);

        static void UvNewSessionCallback(uv_stream_t *server, int status);

    private:
        int _error;
        Status _status;
        uv_tcp_t *_handle;
        unsigned int _isession;
        ServerImplement *_implement;

    private:
        std::string _errdesc;
        Utils::HostAddress _hostAddress;
        std::vector<ProtocolPluginCreator *> _creatorVec;
        std::unordered_map<unsigned int, TcpStream *> _sessionMap;
        std::unordered_map<unsigned int, TcpStream *> _invalidMap;
    };
}

#endif //LCC_TCPSERVER_H
