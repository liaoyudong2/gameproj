//
// Created by liao on 2024/5/2.
//

#ifndef LCC_TCP_STREAM_H
#define LCC_TCP_STREAM_H

#include <string>
#include <vector>
#include "network/ProtocolPlugin.h"

namespace Lcc {
    class TcpStream : public ProtocolImplement {
    public:
        /**
         * 初始化传递接口处理对象
         * @param impl 接口实现类对象
         */
        explicit TcpStream(StreamImplement *impl);

        ~TcpStream() override;

        bool Init();

        /**
         * 启动流数据开始处理
         * @return 是否启动成功
         */
        bool Startup();

        /**
         * 关闭流处理
         */
        void Shutdown();

        /**
         * 获取是否处于激活状态
         * @return 是否激活状态
         */
        bool IsActive() const;

        /**
         * 获取会话
         * @return 会话
         */
        unsigned int GetSession() const;

        /**
         * 获取异常错误码
         * @return 异常错误码
         */
        int LastErrCode() const;

        /**
         * 获取异常描述
         * @return 异常描述
         */
        const char *LastErrorDesc() const;

        /**
         * 获取流远端地址
         * @param addr 流远端地址
         */
        void GetPeerAddress(std::string &addr) const;

        /**
         * 启用协议插件
         * @param plugin 协议插件对象
         * @return 是否成功
         */
        void EnableProtocolPlugin(ProtocolPlugin *plugin);

        /**
         * 向流写数据
         * @param buf 数据流
         * @param size 数据长度
         */
        void Write(const char *buf, unsigned int size);

    protected:
        /**
         * 根据传入的协议等级，按排序方式获取下一个需要操作的协议插件
         * @param level 协议等级
         * @param desc 是否降序获取协议处理插件
         * @return 协议处理插件
         */
        ProtocolPlugin *GetLevelPlugin(ProtocolLevel level, bool desc = false);

        /**
         * 流关闭
         */
        void StreamShutdown();

    protected:
        void IProtocolOpen(ProtocolLevel streamLevel) override;

        void IProtocolWrite(ProtocolLevel streamLevel, const char *buf, unsigned int size) override;

        void IProtocolRead(ProtocolLevel streamLevel, const char *buf, unsigned int size) override;

        void IProtocolClose(ProtocolLevel streamLevel) override;

    protected:
        static void UvMemoryAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);

        static void UvReadCallback(uv_stream_t *stream, ssize_t readLen, const uv_buf_t *buf);

        static void UvWriteCallback(uv_write_t *req, int status);

        static void UvShutdownCallback(uv_shutdown_t *req, int status);

        static void UvCloseCallback(uv_handle_t *handle);

    protected:
        bool _init;
        int _error;
        StreamImplement *_implement;

    private:
        std::string _errdesc;
        std::string _allocBuffer;
        StreamHandle _streamHandle{};
        std::vector<ProtocolPlugin *> _protocolPluginVec;
    };
}

#endif //LCC_TCP_STREAM_H
