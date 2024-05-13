//
// Created by liao on 2024/5/3.
//
#include "network/TcpClient.h"
#include "network/plugin/MbedTLSPlugin.h"

namespace Lcc {
    TcpClient::TcpClient(ClientImplement *impl) : _status(Status::None),
                                                  _handle(nullptr),
                                                  _tcpStream(nullptr),
                                                  _implement(impl),
                                                  _hostAddress() {
    }

    TcpClient::~TcpClient() = default;

    void TcpClient::Connect(const char *host) {
        if (host && _status == Status::None && HostParse(host, _hostAddress)) {
            _status = Status::Address;
            return AddressParse();
        }
    }

    void TcpClient::Shutdown() {
        if (_tcpStream) {
            _tcpStream->Shutdown();
        }
    }

    void TcpClient::Enable(ProtocolPluginCreator *creator) {
        if (creator) {
            _creatorVec.emplace_back(creator);
        }
    }

    void TcpClient::Write(const char *buf, unsigned int size) {
        if (_status == Status::Connected && _tcpStream) {
            _tcpStream->Write(buf, size);
        }
    }

    unsigned int TcpClient::GetSession() const {
        if (_tcpStream) {
            return _tcpStream->GetSession();
        }
        return 0;
    }

    void TcpClient::AddressParse() {
        if (_status == Status::Address) {
            _tcpStream = new TcpStream(reinterpret_cast<StreamImplement *>(this));
            if (!_tcpStream->Init()) {
                return AddressConnectFail(_tcpStream->LastErrCode());
            }
            auto req = static_cast<uv_getaddrinfo_t *>(::malloc(sizeof(uv_getaddrinfo_t)));
            uv_handle_set_data(reinterpret_cast<uv_handle_t *>(req), this);
            uv_getaddrinfo(_handle->loop, req, TcpClient::UvAddressParseCallback, _hostAddress.host, nullptr,
                           nullptr);
        }
    }

    void TcpClient::AddressConnecting() {
        if (_status == Status::Init) {
            int err;
            auto req = reinterpret_cast<uv_connect_t *>(::malloc(sizeof(uv_connect_t)));
            uv_handle_set_data(reinterpret_cast<uv_handle_t *>(req), this);
            if (_hostAddress.v6) {
                sockaddr_in6 addr6{};
                uv_ip6_addr(_hostAddress.ip, _hostAddress.port, &addr6);
                err = uv_tcp_connect(req, _handle, reinterpret_cast<const sockaddr *>(&addr6),
                                     TcpClient::UvConnectStatusCallback);
            } else {
                sockaddr_in addr4{};
                uv_ip4_addr(_hostAddress.ip, _hostAddress.port, &addr4);
                err = uv_tcp_connect(req, _handle, reinterpret_cast<const sockaddr *>(&addr4),
                                     TcpClient::UvConnectStatusCallback);
            }
            if (err == 0) {
                _status = Status::Connecting;
            } else {
                AddressConnectFail(err);
            }
        }
    }

    void TcpClient::AddressConnectFail(int status) {
        _status = Status::ConnectFail;
        _implement->IClientReport(_tcpStream->GetSession(), false, status != 0 ? uv_strerror(status) : nullptr);
        _tcpStream->Shutdown();
    }

    bool TcpClient::IStreamInit(StreamHandle &handle) {
        if (_implement->IClientInit(handle)) {
            _handle = &handle.tcpHandle;
            return true;
        }
        return false;
    }

    void TcpClient::IStreamOpen(unsigned int session) {
        _implement->IClientReport(session, true, nullptr);
    }

    void TcpClient::IStreamReceive(unsigned int session, const char *buf, unsigned int size) {
        _implement->IClientReceive(session, buf, size);
    }

    void TcpClient::IStreamBeforeClose(unsigned int session, int err, const char *errMsg) {
        if (_status == Status::Connected) {
            _implement->IClientBeforeDisconnect(session, err, errMsg);
        }
    }

    void TcpClient::IStreamAfterClose(unsigned int session) {
        delete _tcpStream;
        _tcpStream = nullptr;
        _handle = nullptr;
        for (auto creator: _creatorVec) {
            creator->ICreatorRelease();
        }
        _creatorVec.clear();
        if (_status == Status::Connected) {
            _implement->IClientAfterDisconnect(session);
        }
        _status = Status::None;
    }

    void TcpClient::UvAddressParseCallback(uv_getaddrinfo_t *info, int status, addrinfo *res) {
        auto self = static_cast<TcpClient *>(uv_handle_get_data(reinterpret_cast<uv_handle_t *>(info)));
        if (status == 0) {
            if (res->ai_protocol == IPPROTO_TCP || res->ai_protocol == IPPROTO_IP || res->ai_protocol == IPPROTO_UDP) {
                uv_ip4_name(reinterpret_cast<sockaddr_in *>(res->ai_addr), self->_hostAddress.ip,
                            sizeof(self->_hostAddress.ip));
            } else if (res->ai_protocol == IPPROTO_IPV6) {
                self->_hostAddress.v6 = true;
                uv_ip6_name(reinterpret_cast<sockaddr_in6 *>(res->ai_addr), self->_hostAddress.ip,
                            sizeof(self->_hostAddress.ip));
            }
            self->_status = Status::Init;
        } else {
            self->AddressConnectFail(status);
        }
        uv_freeaddrinfo(res);
        ::free(info);
        if (self->_status == Status::Init) {
            self->AddressConnecting();
        }
    }

    void TcpClient::UvConnectStatusCallback(uv_connect_t *req, int status) {
        auto self = static_cast<TcpClient *>(uv_handle_get_data(reinterpret_cast<uv_handle_t *>(req)));
        ::free(req);
        if (status != 0) {
            self->AddressConnectFail(status);
        } else {
            self->_status = Status::Connected;
            // TODO WebSocket
            if (self->_hostAddress.ssl) {
                auto creator = new MbedTLSPluginCreator;
                creator->InitializeClientMode(self->_hostAddress.host, nullptr);
                self->_creatorVec.emplace_back(creator);
            }
            for (auto creator: self->_creatorVec) {
                self->_tcpStream->EnableProtocolPlugin(
                    creator->ICreatorAlloc(reinterpret_cast<ProtocolImplement *>(self->_tcpStream)));
            }
            if (!self->_tcpStream->Startup()) {
                self->AddressConnectFail(self->_tcpStream->LastErrCode());
            }
        }
    }
}
