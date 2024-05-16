//
// Created by liao on 2024/5/14.
//
#include "network/TcpServer.h"

namespace Lcc {
    TcpServer::TcpServer(ServerImplement *impl): _error(0),
                                                 _status(Status::None),
                                                 _handle(nullptr),
                                                 _isession(0),
                                                 _implement(impl),
                                                 _hostAddress() {
    }

    TcpServer::~TcpServer() = default;

    void TcpServer::Listen(const char *host) {
        if (host && _status == Status::None && HostParse(host, _hostAddress)) {
            _status = Status::Address;
            return AddressParse();
        }
    }

    void TcpServer::Shutdown() {
        if (_handle && !uv_is_closing(reinterpret_cast<const uv_handle_t *>(_handle))) {
            uv_close(reinterpret_cast<uv_handle_t *>(_handle), TcpServer::UvServerShutdownCallback);
            ShutdownAllSessions();
        }
    }

    void TcpServer::Enable(ProtocolPluginCreator *creator) {
        if (creator && creator->ICreatorInit()) {
            _creatorVec.emplace_back(creator);
        }
    }

    void TcpServer::ShutdownAllSessions() const {
        for (const auto it: _sessionMap) {
            it.second->stream->Shutdown();
        }
    }

    void TcpServer::ShutdownSession(unsigned int session) {
        auto sessionStream = GetSessionStream(session);
        if (sessionStream) {
            sessionStream->Shutdown();
        }
    }

    const Utils::HostAddress &TcpServer::GetListenAddress() const {
        return _hostAddress;
    }

    void TcpServer::SessionWrite(unsigned int session, const char *buf, unsigned int size) {
        const auto sessionStream = GetSessionStream(session);
        if (sessionStream) {
            sessionStream->Write(buf, size);
        }
    }

    void TcpServer::AddressParse() {
        if (_status == Status::Address) {
            _handle = static_cast<uv_tcp_t *>(::malloc(sizeof(uv_tcp_t)));
            _implement->IServerInit(_handle);
            uv_handle_set_data(reinterpret_cast<uv_handle_t *>(_handle), this);
            auto req = static_cast<uv_getaddrinfo_t *>(::malloc(sizeof(uv_getaddrinfo_t)));
            uv_handle_set_data(reinterpret_cast<uv_handle_t *>(req), this);
            uv_getaddrinfo(_handle->loop, req, TcpServer::UvAddressParseCallback, _hostAddress.host, nullptr, nullptr);
        }
    }

    void TcpServer::AddressListening() {
        if (_status == Status::Init) {
            int err;
            if (_hostAddress.v6) {
                sockaddr_in6 addr6{};
                uv_ip6_addr(_hostAddress.ip, _hostAddress.port, &addr6);
                err = uv_tcp_bind(_handle, reinterpret_cast<const sockaddr *>(&addr6), 0);
            } else {
                sockaddr_in addr4{};
                uv_ip4_addr(_hostAddress.ip, _hostAddress.port, &addr4);
                err = uv_tcp_bind(_handle, reinterpret_cast<const sockaddr *>(&addr4), 0);
            }
            if (err == 0) {
                err = uv_listen(reinterpret_cast<uv_stream_t *>(_handle), 128, TcpServer::UvNewSessionCallback);
                if (err == 0) {
                    _status = Status::Listened;
                    _implement->IServerListenReport(true, 0, nullptr);
                    return;
                }
            }
            AddressListenFail(err);
        }
    }

    void TcpServer::AddressListenFail(int status) {
        _error = status;
        _errdesc = uv_strerror(status);
        _status = Status::ListenFail;
        _implement->IServerListenReport(false, status, _errdesc.c_str());
        Shutdown();
    }

    TcpStream *TcpServer::GetSessionStream(unsigned int session) {
        const auto sessionObject = GetSessionObject(session);
        if (sessionObject && sessionObject->valid) {
            return sessionObject->stream;
        }
        return nullptr;
    }

    TcpServer::SessionObject *TcpServer::GetSessionObject(unsigned int session) {
        auto it = _sessionMap.find(session);
        if (it != _sessionMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool TcpServer::IStreamInit(StreamHandle &handle) {
        unsigned int session = 0;
        while (session == 0) {
            while (++_isession) {
                if (_sessionMap.find(_isession) == _sessionMap.end()) {
                    session = _isession;
                    break;
                }
            }
        }
        handle.tcpSession = session;
        uv_tcp_init(_handle->loop, &handle.tcpHandle);
        uv_accept(reinterpret_cast<uv_stream_t *>(_handle), reinterpret_cast<uv_stream_t *>(&handle.tcpHandle));
        return true;
    }

    void TcpServer::IStreamOpen(unsigned int session) {
        const auto sessionObject = GetSessionObject(session);
        if (sessionObject) {
            sessionObject->valid = true;
            _implement->IServerSessionOpen(session);
        }
    }

    void TcpServer::IStreamReceive(unsigned int session, const char *buf, unsigned int size) {
        _implement->IServerSessionReceive(session, buf, size);
    }

    void TcpServer::IStreamBeforeClose(unsigned int session, int err, const char *errMsg) {
        const auto sessionObject = GetSessionObject(session);
        if (sessionObject && sessionObject->valid) {
            _implement->IServerSessionBeforeClose(session, err, errMsg);
        }
    }

    void TcpServer::IStreamAfterClose(unsigned int session) {
        const auto it = _sessionMap.find(session);
        if (it != _sessionMap.end()) {
            const auto sessionObject = it->second;
            delete sessionObject->stream;
            delete sessionObject;
            _sessionMap.erase(it);
            _implement->IServerSessionAfterClose(session);
        }
    }

    void TcpServer::UvAddressParseCallback(uv_getaddrinfo_t *info, int status, addrinfo *res) {
        auto self = static_cast<TcpServer *>(uv_handle_get_data(reinterpret_cast<uv_handle_t *>(info)));
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
            self->AddressListenFail(status);
        }
        uv_freeaddrinfo(res);
        ::free(info);
        if (self->_status == Status::Init) {
            self->AddressListening();
        }
    }

    void TcpServer::UvNewSessionCallback(uv_stream_t *server, int status) {
        if (status == 0) {
            auto self = static_cast<TcpServer *>(uv_handle_get_data(reinterpret_cast<const uv_handle_t *>(server)));
            auto sessionObject = new SessionObject;
            sessionObject->valid = false;
            sessionObject->stream = new TcpStream(reinterpret_cast<StreamImplement *>(self));
            if (sessionObject->stream->Init()) {
                for (auto creator: self->_creatorVec) {
                    sessionObject->stream->EnableProtocolPlugin(
                        creator->ICreatorAlloc(reinterpret_cast<ProtocolImplement *>(sessionObject->stream)));
                }
                self->_sessionMap[sessionObject->stream->GetSession()] = sessionObject;
                if (!sessionObject->stream->Startup()) {
                    sessionObject->stream->Shutdown();
                }
            } else {
                sessionObject->stream->Shutdown();
                delete sessionObject;
            }
        }
    }

    void TcpServer::UvServerShutdownCallback(uv_handle_t *handle) {
        auto self = static_cast<TcpServer *>(uv_handle_get_data(handle));
        for (auto creator: self->_creatorVec) {
            creator->ICreatorRelease();
        }
        ::free(self->_handle);
        self->_handle = nullptr;
        self->_creatorVec.clear();
        self->_implement->IServerShutdown();
    }
}
