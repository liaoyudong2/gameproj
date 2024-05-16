//
// Created by liao on 2024/5/16.
//
#include "network/plugin/WebSocketPlugin.h"

namespace Lcc {
    WebSocketPlugin::WebSocketPlugin(ProtocolImplement *impl): WebSocketPlugin(WebSocketOpcode::Text, impl) {
    }

    WebSocketPlugin::WebSocketPlugin(WebSocketOpcode opcode, ProtocolImplement *impl) : ProtocolPlugin(
            ProtocolLevel::WebSocket, impl),
        _error(0),
        _handshaked(false),
        _opcode(opcode),
        _protocol(this) {
        _errorstr.resize(256);
        _errorstr.clear();
    }

    WebSocketPlugin::~WebSocketPlugin() = default;

    void WebSocketPlugin::InitializeServerMode() {
        _protocol.Initialize();
    }

    void WebSocketPlugin::InitializeClientMode(const char *host) {
        if (host) {
            _hostname = host;
            _protocol.Initialize();
        }
    }

    void WebSocketPlugin::ImplementClose(WebSocketCode code) {
        _protocol.Close(code);
        _error = static_cast<int>(code);
        _errorstr = _protocol.GetErrorDesc(code);
        _impl->IProtocolClose(ProtocolLevel::WebSocket);
    }

    int WebSocketPlugin::IProtocolLastError() {
        return _error;
    }

    const char *WebSocketPlugin::IProtocolLastErrDesc() {
        return _errorstr.c_str();
    }

    bool WebSocketPlugin::IProtocolPluginOpen() {
        if (!_hostname.empty() && !_handshaked) {
            _protocol.HandshakeRequest(_hostname.c_str(), _serverKey);
        }
        return true;
    }

    bool WebSocketPlugin::IProtocolPluginRead(const char *buf, unsigned int size) {
        if (!_handshaked) {
            if (!_hostname.empty()) {
                if (!_protocol.CheckServerSecKey(buf, size, _serverKey)) {
                    _impl->IProtocolClose(ProtocolLevel::WebSocket);
                    return false;
                }
            } else {
                if (!_protocol.HandshakeResponse(buf, size)) {
                    _impl->IProtocolClose(ProtocolLevel::WebSocket);
                    return false;
                }
            }
            _handshaked = true;
            _impl->IProtocolOpen(ProtocolLevel::WebSocket);
        } else {
            if (!_protocol.Read(buf, size)) {
                ImplementClose(WebSocketCode::AbNormal);
                return false;
            }
        }
        return true;
    }

    void WebSocketPlugin::IProtocolPluginWrite(const char *buf, unsigned int size) {
        _protocol.Write(buf, size);
    }

    void WebSocketPlugin::IProtocolPluginClose() {
        ImplementClose(WebSocketCode::Normal);
    }

    void WebSocketPlugin::IProtocolPluginRelease() {
        delete this;
    }

    void WebSocketPlugin::IWebSocketInit(WebSocketMode &mode) {
        mode.mark = !_hostname.empty();
        mode.opcode = _opcode;
    }

    void WebSocketPlugin::IWebSocketReceive(WebSocketFrameHeader &header, const char *buf, unsigned int size) {
        switch (header.opcode) {
            case WebSocketOpcode::Close:
                ImplementClose(WebSocketCode::Normal);
                break;
            case WebSocketOpcode::Pong:
                _protocol.Pong(buf, size);
                break;
            default: {
                if (header.opcode != _opcode) {
                    ImplementClose(WebSocketCode::Unsupported);
                    break;
                }
                _impl->IProtocolRead(ProtocolLevel::WebSocket, buf, size);
                break;
            }
        }
    }

    void WebSocketPlugin::IWebSocketWrite(const char *buf, unsigned int size) {
        _impl->IProtocolWrite(ProtocolLevel::WebSocket, buf, size);
    }

    WebSocketPluginCreator::WebSocketPluginCreator(): _init(false), _opcode(WebSocketOpcode::Binary) {
    }

    void WebSocketPluginCreator::InitializeServerMode(WebSocketOpcode opcode) {
        _init = true;
        _opcode = opcode;
    }

    void WebSocketPluginCreator::InitializeClientMode(WebSocketOpcode opcode, const char *hostname) {
        if (hostname) {
            _hostname = hostname;
        }
        _init = true;
        _opcode = opcode;
    }

    bool WebSocketPluginCreator::ICreatorInit() {
        return _init;
    }

    void WebSocketPluginCreator::ICreatorRelease() {
        delete this;
    }

    ProtocolPlugin * WebSocketPluginCreator::ICreatorAlloc(ProtocolImplement *impl) {
        auto plugin = new WebSocketPlugin(_opcode, impl);
        if (_hostname.empty()) {
            plugin->InitializeServerMode();
        } else {
            plugin->InitializeClientMode(_hostname.c_str());
        }
        return plugin;
    }
}
