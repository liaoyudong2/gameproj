//
// Created by liao on 2024/5/2.
//
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "network/TcpStream.h"

namespace Lcc {
    TcpStream::TcpStream(StreamImplement *impl) : _init(false),
                                                  _error(0),
                                                  _implement(impl) {
        _allocBuffer.resize(0x10000);
        _allocBuffer.clear();
    }

    TcpStream::~TcpStream() = default;

    bool TcpStream::Init() {
        if (!_init) {
            _init = _implement->IStreamInit(_streamHandle);
            if (_init) {
                uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&_streamHandle.tcpHandle), this);
            }
        }
        return _init;
    }

    bool TcpStream::Startup() {
        if (!Init()) {
            return false;
        }
        _error = uv_read_start(reinterpret_cast<uv_stream_t *>(&_streamHandle.tcpHandle),
                               TcpStream::UvMemoryAlloc, TcpStream::UvReadCallback);
        if (_error == 0) {
            IProtocolOpen(ProtocolLevel::Stream);
            return true;
        }
        _errdesc = uv_strerror(_error);
        return false;
    }

    void TcpStream::Shutdown() {
        if (_streamHandle.IsActive()) {
            auto req = static_cast<uv_shutdown_t *>(::malloc(sizeof(uv_shutdown_t)));
            uv_handle_set_data(reinterpret_cast<uv_handle_t *>(req), this);
            if (uv_shutdown(req, reinterpret_cast<uv_stream_t *>(&_streamHandle.tcpHandle),
                            TcpStream::UvShutdownCallback)) {
                ::free(req);
            }
        }
    }

    bool TcpStream::IsActive() const {
        return _streamHandle.IsActive();
    }

    unsigned int TcpStream::GetSession() const {
        return _streamHandle.tcpSession;
    }

    int TcpStream::LastErrCode() const {
        return _error;
    }

    const char *TcpStream::LastErrorDesc() const {
        if (_error == 0) {
            return nullptr;
        }
        return uv_strerror(_error);
    }

    void TcpStream::GetPeerAddress(std::string &addr) const {
        if (IsActive()) {
            sockaddr_storage storage{};
            int len = sizeof(storage);
            if (uv_tcp_getpeername(reinterpret_cast<const uv_tcp_t *>(&_streamHandle.tcpHandle),
                                   reinterpret_cast<sockaddr *>(&storage), &len) == 0) {
                char host[128] = {0};
                if (storage.ss_family == AF_INET) {
                    uv_ip4_name(reinterpret_cast<const sockaddr_in *>(&storage), host, 128);
                } else if (storage.ss_family == AF_INET6) {
                    uv_ip6_name(reinterpret_cast<const sockaddr_in6 *>(&storage), host, 128);
                }
                addr.assign(host, sizeof(host));
            }
        }
    }

    void TcpStream::EnableProtocolPlugin(ProtocolPlugin *plugin) {
        if (plugin) {
            _protocolPluginVec.emplace_back(plugin);
            std::sort(_protocolPluginVec.begin(), _protocolPluginVec.end(), [](ProtocolPlugin *a, ProtocolPlugin *b) {
                return a->GetLevel() < b->GetLevel();
            });
        }
    }

    void TcpStream::Write(const char *buf, unsigned int size) {
        if (IsActive() && uv_is_writable(reinterpret_cast<const uv_stream_t *>(&_streamHandle.tcpHandle))) {
            IProtocolWrite(ProtocolLevel::Application, buf, size);
        }
    }

    ProtocolPlugin *TcpStream::GetLevelPlugin(ProtocolLevel level, bool desc) {
        if (desc) {
            for (auto it = _protocolPluginVec.rbegin(); it != _protocolPluginVec.rend(); ++it) {
                const auto plugin = *it;
                if (plugin->GetLevel() < level) {
                    return plugin;
                }
            }
        } else {
            for (auto it: _protocolPluginVec) {
                if (it->GetLevel() > level) {
                    return it;
                }
            }
        }
        return nullptr;
    }

    void TcpStream::IProtocolOpen(ProtocolLevel streamLevel) {
        auto plugin = GetLevelPlugin(streamLevel);
        if (plugin) {
            if (!plugin->IProtocolPluginOpen()) {
                _error = plugin->IProtocolLastError();
                _errdesc = plugin->IProtocolLastErrDesc();
            }
        } else {
            _implement->IStreamOpen(_streamHandle.tcpSession);
        }
    }

    void TcpStream::IProtocolWrite(ProtocolLevel streamLevel, const char *buf, unsigned int size) {
        auto plugin = GetLevelPlugin(streamLevel, true);
        if (plugin) {
            plugin->IProtocolPluginWrite(buf, size);
        } else {
            auto req = static_cast<uv_write_t *>(::malloc(sizeof(uv_write_t) + sizeof(uv_buf_t)));
            auto *ubuf = reinterpret_cast<uv_buf_t *>(req + 1);
            if (size > 0) {
                ubuf->base = static_cast<char *>(::malloc(size));
                memcpy(ubuf->base, buf, size);
                ubuf->len = size;
            } else {
                ubuf->base = nullptr;
                ubuf->len = 0;
            }
            if (uv_write(req, reinterpret_cast<uv_stream_t *>(&_streamHandle.tcpHandle), ubuf, 1,
                         TcpStream::UvWriteCallback)) {
                ::free(req);
            }
        }
    }

    void TcpStream::IProtocolRead(ProtocolLevel streamLevel, const char *buf, unsigned int size) {
        auto plugin = GetLevelPlugin(streamLevel);
        if (plugin) {
            if (!plugin->IProtocolPluginRead(buf, size)) {
                _error = plugin->IProtocolLastError();
                _errdesc = plugin->IProtocolLastErrDesc();
            }
        } else {
            _implement->IStreamReceive(_streamHandle.tcpSession, buf, size);
        }
    }

    void TcpStream::IProtocolClose(ProtocolLevel streamLevel) {
        auto plugin = GetLevelPlugin(streamLevel);
        if (plugin) {
            plugin->IProtocolPluginClose();
        } else {
            Shutdown();
        }
    }

    void TcpStream::UvMemoryAlloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
        auto self = static_cast<TcpStream *>(uv_handle_get_data(handle));
        buf->base = const_cast<char *>(self->_allocBuffer.data());
        buf->len = self->_allocBuffer.capacity();
    }

    void TcpStream::UvReadCallback(uv_stream_t *stream, ssize_t readLen, const uv_buf_t *buf) {
        auto self = static_cast<TcpStream *>(uv_handle_get_data(reinterpret_cast<const uv_handle_t *>(stream)));
        if (readLen > 0) {
            self->IProtocolRead(ProtocolLevel::Stream, buf->base, readLen);
        } else if (readLen == 0) {
            // 可能为 0，这并不表示错误或 EOF。这相当于EAGAIN或EWOULDBLOCK
        } else {
            self->_error = static_cast<int>(readLen);
            self->_errdesc = uv_strerror(self->_error);
            self->IProtocolClose(ProtocolLevel::Application);
        }
    }

    void TcpStream::UvWriteCallback(uv_write_t *req, int status) {
        auto *ubuf = reinterpret_cast<uv_buf_t *>(req + 1);
        ::free(ubuf->base);
        ::free(req);
    }

    void TcpStream::UvShutdownCallback(uv_shutdown_t *req, int status) {
        auto self = static_cast<TcpStream *>(uv_handle_get_data(reinterpret_cast<const uv_handle_t *>(req)));
        if (self->_streamHandle.IsActive()) {
            uv_close(reinterpret_cast<uv_handle_t *>(&self->_streamHandle.tcpHandle), TcpStream::UvCloseCallback);
            self->_implement->IStreamBeforeClose(self->_streamHandle.tcpSession, self->_error,
                                                 self->_errdesc.empty() ? nullptr : self->_errdesc.c_str());
        }
        ::free(req);
    }

    void TcpStream::UvCloseCallback(uv_handle_t *handle) {
        auto self = static_cast<TcpStream *>(uv_handle_get_data(handle));
        for (auto plugin: self->_protocolPluginVec) {
            plugin->IProtocolPluginRelease();
        }
        self->_protocolPluginVec.clear();
        self->_implement->IStreamAfterClose(self->_streamHandle.tcpSession);
    }
}
