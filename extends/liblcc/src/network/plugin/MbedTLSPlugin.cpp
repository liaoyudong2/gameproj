//
// Created by liao on 2024/5/13.
//
#include "network/plugin/MbedTLSPlugin.h"

namespace Lcc {
    MbedTLSPlugin::MbedTLSPlugin(ProtocolImplement *impl) : ProtocolPlugin(ProtocolLevel::StreamWithSSL, impl),
                                                            _error(0) {
        _buffer.resize(0x4000); // 16384
        _buffer.clear();
    }

    MbedTLSPlugin::~MbedTLSPlugin() {
        _mbedtls.Release();
    }

    Protocol::MbedTLS &MbedTLSPlugin::GetMbedTLS() {
        return _mbedtls;
    }

    bool MbedTLSPlugin::IProtocolPluginOpen() {
        mbedtls_ssl_set_bio(_mbedtls.GetSSLContext(), this, MbedTLSPlugin::MbedTLSSendCallback, MbedTLSPlugin::MbedTLSRecvCallback, nullptr);
        if (_mbedtls.ClientMode()) {
            return Handshake();
        }
        return true;
    }

    void MbedTLSPlugin::IProtocolPluginRead(const char *buf, unsigned int size) {
        _bufferIn.Write(buf, size);
        mbedtls_ssl_context *ctx = _mbedtls.GetSSLContext();
        if (ctx->private_state != MBEDTLS_SSL_HANDSHAKE_OVER) {
            if (!Handshake()) {
                return ImplementClose();
            }
            if (ctx->private_state == MBEDTLS_SSL_HANDSHAKE_OVER) {
                if (!_mbedtls.Verify()) {
                    return ImplementClose();
                }
                ImplementOpen();
            }
        }
        if (ctx->private_state == MBEDTLS_SSL_HANDSHAKE_OVER) {
            while (_bufferIn.UsedSize() > 0) {
                int r = mbedtls_ssl_read(ctx, reinterpret_cast<unsigned char *>(const_cast<char *>(_buffer.data())), _buffer.capacity());
                if (r <= 0 && r != MBEDTLS_ERR_SSL_WANT_READ && r != MBEDTLS_ERR_SSL_WANT_WRITE) {
                    _error = r;
                    return ImplementClose();
                }
                ImplementReceive(_buffer.data(), r);
            }
        }
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
        if (l > 0) {
            do {
                unsigned int r = _bufferOut.Read(const_cast<char *>(_buffer.data()), _buffer.capacity());
                GetImpl()->IProtocolWrite(ProtocolLevel::StreamWithSSL, _buffer.data(), r);
                l -= r;
            } while (l > 0);
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
        auto plugin = reinterpret_cast<MbedTLSPlugin *>(ctx);
        int r = plugin->_bufferIn.Read(reinterpret_cast<char *>(buf), size);
        if (r > 0) {
            return r;
        }
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    int MbedTLSPlugin::MbedTLSSendCallback(void *ctx, const unsigned char *buf, size_t size) {
        auto plugin = reinterpret_cast<MbedTLSPlugin *>(ctx);
        int r = plugin->_bufferOut.Write(reinterpret_cast<const char *>(buf), size);
        if (r > 0) {
            return r;
        }
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }
}
