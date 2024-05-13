//
// Created by liao on 2024/5/13.
//
#include "network/protocol/MbedTLS.h"

namespace Lcc {
    namespace Protocol {
        MbedTLS::MbedTLS() : _error(0), _mode(Mode::None), _caroot(false) {
            _errorstr.resize(512);
        }

        MbedTLS::~MbedTLS() {
            Release();
        }

        void MbedTLS::Release() {
            if (Enabled()) {
                switch (_mode) {
                    case Mode::ClientMode:
                    case Mode::ServerMode: {
                        mbedtls_pk_free(&_pkCtx);
                        mbedtls_ssl_free(&_sslCtx);
                        mbedtls_x509_crt_free(&_x509Crt);
                        mbedtls_ssl_config_free(&_sslCfg);
                        mbedtls_entropy_free(&_entropyCtx);
                        mbedtls_ctr_drbg_free(&_ctrDrbgCtx);
                        break;
                    }
                    case Mode::SessionMode: {
                        mbedtls_ssl_free(&_sslCtx);
                        break;
                    }
                    default: break;
                }
                _mode = Mode::None;
            }
        }

        int MbedTLS::GetError() const {
            return _error;
        }

        const char *MbedTLS::GetErrorDesc() const {
            if (_error != 0) {
                mbedtls_strerror(_error, const_cast<char *>(_errorstr.data()), _errorstr.size());
            }
            return _errorstr.c_str();
        }

        mbedtls_ssl_context * MbedTLS::GetSSLContext() {
            return &_sslCtx;
        }

        bool MbedTLS::Verify() const {
            if (Enabled()) {
                if (_mode == Mode::ClientMode && _caroot) {
                    return mbedtls_ssl_get_verify_result(&_sslCtx) == 0;
                }
            }
            return false;
        }

        bool MbedTLS::Enabled() const {
            return _mode != Mode::None;
        }

        bool MbedTLS::ClientMode() const {
            return _mode == Mode::ClientMode;
        }

        bool MbedTLS::InitializeForSession(const MbedTLS &ssl) {
            if (!Enabled()) {
                _errorstr.clear();
                mbedtls_ssl_init(&_sslCtx);
                do {
                    _error = mbedtls_ssl_setup(&_sslCtx, &ssl._sslCfg);
                    if (_error != 0) {
                        break;
                    }
                    _mode = Mode::SessionMode;
                    return true;
                } while (false);
                Release();
            }
            return false;
        }

        bool MbedTLS::InitializeForClient(const char *host, const char *caroot) {
            if (!Enabled() && host) {
                do {
                    _errorstr.clear();
                    mbedtls_pk_init(&_pkCtx);
                    mbedtls_ssl_init(&_sslCtx);
                    mbedtls_x509_crt_init(&_x509Crt);
                    mbedtls_ssl_config_init(&_sslCfg);
                    mbedtls_entropy_init(&_entropyCtx);
                    mbedtls_ctr_drbg_init(&_ctrDrbgCtx);
                    _error = mbedtls_ctr_drbg_seed(&_ctrDrbgCtx, mbedtls_entropy_func, &_entropyCtx, nullptr, 0);
                    if (_error != 0) {
                        break;
                    }
                    _error = mbedtls_ssl_config_defaults(&_sslCfg, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                                         MBEDTLS_SSL_PRESET_DEFAULT);
                    if (_error != 0) {
                        break;
                    }
                    if (caroot) {
                        _error = mbedtls_x509_crt_parse_file(&_x509Crt, caroot);
                        if (_error != 0) {
                            break;
                        }
                        mbedtls_ssl_conf_authmode(&_sslCfg, MBEDTLS_SSL_VERIFY_REQUIRED);
                    } else {
                        mbedtls_ssl_conf_authmode(&_sslCfg, MBEDTLS_SSL_VERIFY_OPTIONAL);
                    }
                    mbedtls_ssl_conf_rng(&_sslCfg, mbedtls_ctr_drbg_random, &_ctrDrbgCtx);;
                    mbedtls_ssl_conf_ca_chain(&_sslCfg, &_x509Crt, nullptr);
                    _error = mbedtls_ssl_setup(&_sslCtx, &_sslCfg);
                    if (_error != 0) {
                        break;
                    }
                    _error = mbedtls_ssl_set_hostname(&_sslCtx, host);
                    if (_error != 0) {
                        break;
                    }
                    _mode = Mode::ClientMode;
                    _caroot = caroot != nullptr;
                    return true;
                } while (false);
                Release();
            }
            return false;
        }

        bool MbedTLS::InitializeForServer(const char *cert, const char *key, const char *password) {
            if (!Enabled() && cert && key && password) {
                do {
                    _errorstr.clear();
                    mbedtls_pk_init(&_pkCtx);
                    mbedtls_ssl_init(&_sslCtx);
                    mbedtls_x509_crt_init(&_x509Crt);
                    mbedtls_ssl_config_init(&_sslCfg);
                    mbedtls_entropy_init(&_entropyCtx);
                    mbedtls_ctr_drbg_init(&_ctrDrbgCtx);
                    _error = mbedtls_ctr_drbg_seed(&_ctrDrbgCtx, mbedtls_entropy_func, &_entropyCtx, nullptr, 0);
                    if (_error != 0) {
                        break;
                    }
                    _error = mbedtls_x509_crt_parse_file(&_x509Crt, cert);
                    if (_error != 0) {
                        break;
                    }
                    _error = mbedtls_pk_parse_keyfile(&_pkCtx, key, password, mbedtls_ctr_drbg_random, &_ctrDrbgCtx);
                    if (_error != 0) {
                        break;
                    }
                    _error = mbedtls_ssl_config_defaults(&_sslCfg, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM,
                                                         MBEDTLS_SSL_PRESET_DEFAULT);
                    if (_error != 0) {
                        break;
                    }
                    mbedtls_ssl_conf_rng(&_sslCfg, mbedtls_ctr_drbg_random, &_ctrDrbgCtx);
                    mbedtls_ssl_conf_ca_chain(&_sslCfg, _x509Crt.next, nullptr);
                    _error = mbedtls_ssl_conf_own_cert(&_sslCfg, &_x509Crt, &_pkCtx);
                    if (_error != 0) {
                        break;
                    }
                    _error = mbedtls_ssl_setup(&_sslCtx, &_sslCfg);
                    if (_error != 0) {
                        break;
                    }
                    _mode = Mode::ServerMode;
                    return true;
                } while (false);
                Release();
            }
            return false;
        }
    }
}
