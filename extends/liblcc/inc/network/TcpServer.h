//
// Created by liao on 2024/5/14.
//

#ifndef LCC_TCPSERVER_H
#define LCC_TCPSERVER_H

#include <vector>
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

        struct SessionObject {
            bool valid;
            TcpStream *stream;
        };

    public:
        explicit TcpServer(ServerImplement *impl);

        ~TcpServer() override;

        /**
         * 启动监听
         * @param host 监听地址
         */
        void Listen(const char *host);

        /**
         * 关闭监听
         */
        void Shutdown();

        /**
         * 启用协议插件支持
         * @param creator 协议插件创造器
         */
        void Enable(ProtocolPluginCreator *creator);

        /**
         * 关闭所有连接
         */
        void ShutdownAllSessions() const;

        /**
         * 关闭指定会话
         * @param session 会话id
         */
        void ShutdownSession(unsigned int session);

        /**
         * 获取监听地址信息
         * @return 地址信息
         */
        const Utils::HostAddress &GetListenAddress() const;

        /**
         * 向会话写数据
         * @param session 会话id
         * @param buf 数据
         * @param size 数据长度
         */
        void SessionWrite(unsigned int session, const char *buf, unsigned int size);

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

        /**
         * 查询会话对象
         * @param session 会话id
         * @return 查询到的会话对象
         */
        TcpStream *GetSessionStream(unsigned int session);

        /**
         * 获取会话object
         * @param session 会话id
         * @return 会话object
         */
        SessionObject *GetSessionObject(unsigned int session);

    protected:
        bool IStreamInit(StreamHandle &handle) override;

        void IStreamOpen(unsigned int session) override;

        void IStreamReceive(unsigned int session, const char *buf, unsigned int size) override;

        void IStreamBeforeClose(unsigned int session, int err, const char *errMsg) override;

        void IStreamAfterClose(unsigned int session) override;

    protected:
        static void UvAddressParseCallback(uv_getaddrinfo_t *info, int status, addrinfo *res);

        static void UvNewSessionCallback(uv_stream_t *server, int status);

        static void UvServerShutdownCallback(uv_handle_t *handle);

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
        std::unordered_map<unsigned int, SessionObject *> _sessionMap;
    };
}

#endif //LCC_TCPSERVER_H
