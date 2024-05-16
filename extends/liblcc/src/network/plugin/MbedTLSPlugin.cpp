//
// Created by liao on 2024/5/13.
//
#include "network/plugin/MbedTLSPlugin.h"

namespace Lcc {
    MbedTLSPlugin::MbedTLSPlugin(ProtocolImplement *impl) : ProtocolPlugin(ProtocolLevel::StreamWithSSL, impl),
                                                            _error(0) {
        _buffer.resize(0x4000); // 16384
        _buffer.clear();
        _errorstr.resize(512);
        _errorstr.clear();
    }

    MbedTLSPlugin::~MbedTLSPlugin() {
        _mbedtls.Release();
    }

    Protocol::MbedTLS &MbedTLSPlugin::GetMbedTLS() {
        return _mbedtls;
    }

    int MbedTLSPlugin::IProtocolLastError() {
        return _error;
    }

    const char *MbedTLSPlugin::IProtocolLastErrDesc() {
        if (_error) {
            mbedtls_strerror(_error, const_cast<char *>(_errorstr.data()), _errorstr.capacity());
        }
        return _errorstr.c_str();
    }

    bool MbedTLSPlugin::IProtocolPluginOpen() {
        mbedtls_ssl_set_bio(_mbedtls.GetSSLContext(), this, MbedTLSPlugin::MbedTLSSendCallback,
                            MbedTLSPlugin::MbedTLSRecvCallback, nullptr);
        if (_mbedtls.ClientMode()) {
            if (!Handshake()) {
                ImplementClose();
                return false;
            }
        }
        return true;
    }

    bool MbedTLSPlugin::IProtocolPluginRead(const char *buf, unsigned int size) {
        _bufferIn.Write(buf, size);
        mbedtls_ssl_context *ctx = _mbedtls.GetSSLContext();
        if (ctx->private_state != MBEDTLS_SSL_HANDSHAKE_OVER) {
            if (!Handshake()) {
                ImplementClose();
                return false;
            }
            if (ctx->private_state == MBEDTLS_SSL_HANDSHAKE_OVER) {
                if (!_mbedtls.Verify()) {
                    ImplementClose();
                    return false;
                }
                ImplementOpen();
            }
        }
        if (ctx->private_state == MBEDTLS_SSL_HANDSHAKE_OVER) {
            while (_bufferIn.UsedSize() > 0) {
                int r = mbedtls_ssl_read(ctx, reinterpret_cast<unsigned char *>(const_cast<char *>(_buffer.data())),
                                         _buffer.capacity());
                if (r <= 0 && r != MBEDTLS_ERR_SSL_WANT_READ && r != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    _error = r;
                    ImplementClose();
                    return false;
                }
                ImplementReceive(_buffer.data(), r);
            }
        }
        return true;
    }

    void MbedTLSPlugin::IProtocolPluginWrite(const char *buf, unsigned int size) {
        unsigned int offset = 0;
        mbedtls_ssl_context *ctx = _mbedtls.GetSSLContext();
        do {
            int r = mbedtls_ssl_write(ctx, reinterpret_cast<const unsigned char *>(buf + offset), size - offset);
            if (r > 0 || r == MBEDTLS_ERR_SSL_WANT_READ || r == MBEDTLS_ERR_SSL_WANT_WRITE) {
                WantFlush();
            } else {
                _error = r;
                return ImplementClose();
            }
            offset += r;
        } while (offset < size);
    }

    void MbedTLSPlugin::IProtocolPluginClose() {
        bool handshaked = _mbedtls.GetSSLContext()->private_state == MBEDTLS_SSL_HANDSHAKE_OVER;
        _mbedtls.Release();
        if (handshaked) {
            ImplementClose();
        }
    }

    void MbedTLSPlugin::IProtocolPluginRelease() {
        delete this;
    }

    bool MbedTLSPlugin::Handshake() {
        mbedtls_ssl_context *ctx = _mbedtls.GetSSLContext();
        while (ctx->private_state != MBEDTLS_SSL_HANDSHAKE_OVER) {
            _error = mbedtls_ssl_handshake_step(ctx);
            if (_error != 0 && _error != MBEDTLS_ERR_SSL_WANT_READ && _error != MBEDTLS_ERR_SSL_WANT_WRITE) {
                return false;
            }
            if (_error == MBEDTLS_ERR_SSL_WANT_READ) {
                break;
            }
        }
        WantFlush();
        return true;
    }

    void MbedTLSPlugin::WantFlush() {
        unsigned int l = _bufferOut.UsedSize();
        while (l > 0) {
            const unsigned int r = _bufferOut.Read(const_cast<char *>(_buffer.data()), _buffer.capacity());
            GetImpl()->IProtocolWrite(ProtocolLevel::StreamWithSSL, _buffer.data(), r);
            l -= r;
        }
    }

    void MbedTLSPlugin::ImplementOpen() {
        GetImpl()->IProtocolOpen(ProtocolLevel::StreamWithSSL);
    }

    void MbedTLSPlugin::ImplementClose() {
        GetImpl()->IProtocolClose(ProtocolLevel::StreamWithSSL);
    }

    void MbedTLSPlugin::ImplementReceive(const char *buf, unsigned int size) {
        GetImpl()->IProtocolRead(ProtocolLevel::StreamWithSSL, buf, size);
    }


    int MbedTLSPlugin::MbedTLSRecvCallback(void *ctx, unsigned char *buf, size_t size) {
        auto plugin = static_cast<MbedTLSPlugin *>(ctx);
        int r = plugin->_bufferIn.Read(reinterpret_cast<char *>(buf), size);
        if (r > 0) {
            return r;
        }
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    int MbedTLSPlugin::MbedTLSSendCallback(void *ctx, const unsigned char *buf, size_t size) {
        auto plugin = static_cast<MbedTLSPlugin *>(ctx);
        int r = plugin->_bufferOut.Write(reinterpret_cast<const char *>(buf), size);
        if (r > 0) {
            return r;
        }
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }

    MbedTLSPluginCreator::MbedTLSPluginCreator(): _init(false), _mbedtls(nullptr) {
    }

    MbedTLSPluginCreator::~MbedTLSPluginCreator() {
        if (_mbedtls) {
            _mbedtls->Release();
            delete _mbedtls;
        }
    };

    bool MbedTLSPluginCreator::InitializeClientMode(const char *host, const char *caroot) {
        if (!host) {
            return false;
        }
        _host.assign(host);
        if (caroot) {
            _caroot.assign(caroot);
        }
        _init = true;
        return true;
    }

    bool MbedTLSPluginCreator::InitializeServerMode(const char *cert, const char *key, const char *password) {
        if (!_mbedtls) {
            _mbedtls = new Protocol::MbedTLS;
            if (_mbedtls->InitializeForServer(cert, key, password)) {
                _init = true;
                return true;
            }
            _mbedtls->Release();
            delete _mbedtls;
            _mbedtls = nullptr;
        }
        return false;
    }

    bool MbedTLSPluginCreator::ICreatorInit() {
        return _init;
    }

    void MbedTLSPluginCreator::ICreatorRelease() {
        delete this;
    }

    ProtocolPlugin *MbedTLSPluginCreator::ICreatorAlloc(ProtocolImplement *impl) {
        auto plugin = new MbedTLSPlugin(impl);
        if (_mbedtls) {
            if (plugin->GetMbedTLS().InitializeForSession(*_mbedtls)) {
                return plugin;
            }
            _mbedtls->Release();
        } else {
            if (plugin->GetMbedTLS().InitializeForClient(_host.c_str(), _caroot.empty() ? nullptr : _caroot.c_str())) {
                return plugin;
            }
        }
        delete plugin;
        return nullptr;
    }
}
