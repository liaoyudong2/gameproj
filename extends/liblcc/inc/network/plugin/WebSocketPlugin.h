//
// Created by liao on 2024/5/16.
//

#ifndef LCC_WEBSOCKETPLUGIN_H
#define LCC_WEBSOCKETPLUGIN_H

#include <string>
#include "network/ProtocolPlugin.h"
#include "network/protocol/WebSocket.h"

namespace Lcc {
    class WebSocketPlugin : public ProtocolPlugin, public WebSocketImplement {
    public:
        explicit WebSocketPlugin(ProtocolImplement *impl);

        WebSocketPlugin(WebSocketOpcode opcode, ProtocolImplement *impl);

        ~WebSocketPlugin() override;

        /**
         * 初始化服务端模式
         */
        void InitializeServerMode();

        /**
         * 初始化客户端模式
         * @param host 远端地址
         */
        void InitializeClientMode(const char *host);

    protected:
        /**
         * 关闭
         * @param code 原因
         */
        void ImplementClose(WebSocketCode code);

    protected:
        int IProtocolLastError() override;

        const char *IProtocolLastErrDesc() override;

        bool IProtocolPluginOpen() override;

        bool IProtocolPluginRead(const char *buf, unsigned int size) override;

        void IProtocolPluginWrite(const char *buf, unsigned int size) override;

        void IProtocolPluginClose() override;

        void IProtocolPluginRelease() override;

    protected:
        void IWebSocketInit(WebSocketMode &mode) override;

        void IWebSocketReceive(WebSocketFrameHeader &header, const char *buf, unsigned int size) override;

        void IWebSocketWrite(const char *buf, unsigned int size) override;

    private:
        int _error;
        bool _handshaked;
        std::string _errorstr;
        std::string _hostname;
        std::string _serverKey;
        WebSocketOpcode _opcode;
        WebSocketProtocol _protocol;
    };
}

#endif //LCC_WEBSOCKETPLUGIN_H
