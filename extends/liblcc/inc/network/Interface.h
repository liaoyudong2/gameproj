//
// Created by liao on 2024/5/2.
//

#ifndef LCC_INTERFACE_H
#define LCC_INTERFACE_H

#include "libuv/uv.h"

namespace Lcc {
    // 协议数据层级
    enum class ProtocolLevel {
        // 基础流数据
        Stream,
        // 带ssl加密的流数据
        StreamWithSSL,
        // WebSocket
        WebSocket,
        // 应用层
        Application,
    };

    /**
     * 协议数据流的基础处理接口
     */
    class ProtocolImplement {
    public:
        virtual ~ProtocolImplement() = default;

        /**
         * 当协议数据流启动时触发
         * @param streamLevel 协议数据流来源层级
         */
        virtual void IProtocolOpen(ProtocolLevel streamLevel) = 0;

        /**
         * 当协议数据流需要进行对外写时触发
         * @param streamLevel 协议数据流来源层级
         * @param buf 协议数据流内容
         * @param size 协议数据流长度
         */
        virtual void IProtocolWrite(ProtocolLevel streamLevel, const char *buf, unsigned int size) = 0;

        /**
         * 当协议数据流需要进行读取操作时触发
         * @param streamLevel 协议数据流来源层级
         * @param buf 协议数据流内容
         * @param size 协议数据流长度
         */
        virtual void IProtocolRead(ProtocolLevel streamLevel, const char *buf, unsigned int size) = 0;

        /**
         * 当协议数据流进行关闭时触发
         * @param streamLevel 协议数据流来源层级
         */
        virtual void IProtocolClose(ProtocolLevel streamLevel) = 0;
    };

    class StreamHandle {
    public:
        uv_tcp_t tcpHandle;
        unsigned int tcpSession;

    public:
        inline StreamHandle() : tcpHandle(), tcpSession(0) {
        }

        inline bool IsActive() const {
            return !uv_is_closing(reinterpret_cast<const uv_handle_t *>(&tcpHandle));
        }
    };

    class StreamImplement {
    public:
        virtual ~StreamImplement() = default;

        /**
         * 对流处理对象进行初始化
         * @param handle 流处理句柄
         * @return 初始化结果
         */
        virtual bool IStreamInit(StreamHandle &handle) = 0;

        /**
         * 连接开启时触发
         * @param session 流处理会话
         */
        virtual void IStreamOpen(unsigned int session) = 0;

        /**
         * 连接收到数据时触发
         * @param session 流处理会话
         * @param buf 流数据
         * @param size 流数据大小
         */
        virtual void IStreamReceive(unsigned int session, const char *buf, unsigned int size) = 0;

        /**
         * 连接关闭前触发
         * @param session 流处理会话
         * @param err 错误码
         * @param errMsg 错误信息
         */
        virtual void IStreamBeforeClose(unsigned int session, int err, const char *errMsg) = 0;

        /**
         * 连接已经关闭时触发
         * @param session 流处理会话
         */
        virtual void IStreamAfterClose(unsigned int session) = 0;
    };

    class ClientImplement {
    public:
        virtual ~ClientImplement() = default;

        /**
         *  初始化流处理句柄
         * @param handle 流处理句柄
         * @return 初始化是否成功
         */
        virtual bool IClientInit(StreamHandle &handle) = 0;

        /**
         * 连接成功/失败时触发
         * @param connected 是否连接成功
         * @param err 失败原因
         */
        virtual void IClientReport(bool connected, const char *err) = 0;

        /**
         * 收到数据时触发
         * @param buf 接收到的流数据
         * @param size 数据长度
         */
        virtual void IClientReceive(const char *buf, unsigned int size) = 0;

        /**
         * 将要断开连接时触发
         * @param err 错误码
         * @param errMsg 错误信息
         */
        virtual void IClientBeforeDisconnect(int err, const char *errMsg) = 0;

        /**
         * 连接完全断开连接时触发
         */
        virtual void IClientAfterDisconnect() = 0;
    };

    class ServerImplement {
    public:
        virtual ~ServerImplement() = default;

        /**
         * 初始化流处理句柄
         * @param handle uv_tcp_t句柄
         * @return 初始化是否成功
         */
        virtual bool IServerInit(uv_tcp_t *handle) = 0;

        /**
         * 服务监听状态触发
         * @param listened 是否监听成功
         * @param err 错误码
         * @param errMsg 错误描述
         */
        virtual void IServerListenReport(bool listened, int err, const char *errMsg) = 0;

        /**
         * 服务完全关闭时触发
         */
        virtual void IServerShutdown() = 0;

        /**
         * 会话连接成功
         * @param session 会话id
         */
        virtual void IServerSessionOpen(unsigned int session) = 0;

        /**
         * 会话收到数据
         * @param session 会话id
         * @param buf 接收到的流数据
         * @param size 数据长度
         */
        virtual void IServerSessionReceive(unsigned int session, const char *buf, unsigned int size) = 0;

        /**
         * 会话将要断开连接时触发
         * @param session 会话id
         * @param err 错误码
         * @param errMsg 错误信息
         */
        virtual void IServerSessionBeforeClose(unsigned int session, int err, const char *errMsg) = 0;

        /**
         * 会话已经断开连接时触发
         * @param session 会话id
         */
        virtual void IServerSessionAfterClose(unsigned int session) = 0;
    };
}

#endif //LCC_INTERFACE_H
