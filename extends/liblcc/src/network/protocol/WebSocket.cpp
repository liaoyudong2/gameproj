//
// Created by liao on 2024/5/16.
//
#include <ctime>
#include <random>
#include <sstream>
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"
#include "network/protocol/WebSocket.h"

#include <utils/Address.h>

namespace Lcc {
    WebSocketProtocol::WebSocketProtocol(WebSocketImplement *impl) : _mode({
                                                                         .mark = false, .opcode = WebSocketOpcode::Text
                                                                     }),
                                                                     _implement(impl) {
    }

    WebSocketProtocol::~WebSocketProtocol() = default;

    void WebSocketProtocol::Initialize() {
        _implement->IWebSocketInit(_mode);
    }

    void WebSocketProtocol::HandshakeRequest(const char *host, std::string &serverKey) {
        if (host) {
            std::string key;
            CreateSecWebSocketKey(key);

            std::string request =
                    "GET / HTTP/1.1\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 0\r\nHost: ${HostName}\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Key: ${SecWsKey}\r\nSec-WebSocket-Version: 13\r\n\r\n";
            request.replace(request.find("${SecWsKey}"), 11, key);
            request.replace(request.find("${HostName}"), 11, host);
            key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

            unsigned char sha1sum[20];
            mbedtls_sha1(reinterpret_cast<const unsigned char *>(key.data()), key.size(), sha1sum);

            size_t l;
            serverKey.resize(64);
            mbedtls_base64_encode(reinterpret_cast<unsigned char *>(const_cast<char *>(serverKey.data())),
                                  serverKey.capacity(), &l, reinterpret_cast<const unsigned char *>(sha1sum), 20);
            serverKey.resize(l);

            _implement->IWebSocketWrite(request.c_str(), request.size());
        }
    }

    bool WebSocketProtocol::HandshakeResponse(const char *buf, unsigned int size) {
        if (buf && size > 0) {
            std::string line;
            std::string in(buf, size);
            std::istringstream ss(in);
            std::getline(ss, line);
            if (line.find("GET", 0) != 0) {
                return false;
            }
            std::map<std::string, std::string> keyMap;
            ParserHeaderMap(ss, keyMap);

            const auto hostIt = keyMap.find("host");
            if (hostIt == keyMap.end()) {
                return false;
            }
            const auto keyIt = keyMap.find("sec-websocket-key");
            if (keyIt == keyMap.end()) {
                return false;
            }
            std::string serverKey = keyIt->second + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

            unsigned char sha1sum[20];
            mbedtls_sha1(reinterpret_cast<const unsigned char *>(serverKey.data()), serverKey.size(), sha1sum);

            size_t l;
            std::string b64;
            b64.resize(64);
            mbedtls_base64_encode(reinterpret_cast<unsigned char *>(const_cast<char *>(b64.data())), b64.capacity(), &l,
                                  reinterpret_cast<const unsigned char *>(sha1sum), 20);
            b64.resize(l);

            std::string response =
                    "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ${SecWsKey}\r\nWebSocket-Location: ${HostName}\r\nWebSocket-Protocol: WebManagerSocket\r\n\r\n";
            response.replace(response.find("${SecWsKey}"), 11, b64);
            response.replace(response.find("${HostName}"), 11, hostIt->second);

            _implement->IWebSocketWrite(response.c_str(), response.size());
            return true;
        }
        return false;
    }

    bool WebSocketProtocol::CheckServerSecKey(const char *buf, unsigned int size, std::string &serverKey) {
        if (buf && size > 0) {
            std::string line;
            std::string in(buf, size);
            std::istringstream ss(in);

            std::getline(ss, line);
            if (line.find("HTTP/1.1 101 Switching Protocols", 0) != 0) {
                return false;
            }
            std::map<std::string, std::string> keyMap;
            ParserHeaderMap(ss, keyMap);
            const auto keyIt = keyMap.find("sec-websocket-accept");
            if (keyIt == keyMap.end()) {
                return false;
            }
            if (keyIt->second != serverKey) {
                return false;
            }
            return true;
        }
        return false;
    }

    void WebSocketProtocol::Pong(const char *buf, unsigned int size) {
        Write(buf, size, WebSocketFin::Normal, WebSocketOpcode::Pong);
    }

    void WebSocketProtocol::Write(const char *buf, unsigned int size) {
        static constexpr unsigned long finSize = 0x4000;
        const int loop = static_cast<int>(size / finSize + 1);
        if (loop > 1) {
            auto fin = WebSocketFin::Begin;
            unsigned long len = finSize;
            for (int n = 0; n < loop; ++n) {
                if (n > 0) {
                    fin = WebSocketFin::Continue;
                    if (n + 1 == loop) {
                        fin = WebSocketFin::End;
                        len = size - finSize * n;
                    }
                }
                Write(buf + n * finSize, len, fin, _mode.opcode);
            }
            return;
        }
        Write(buf, size, WebSocketFin::Normal, _mode.opcode);
    }

    void WebSocketProtocol::Close(WebSocketCode code) {
        unsigned char buf[2] = {0};
        buf[0] = static_cast<unsigned short>(code) >> 8;
        buf[1] = static_cast<int>(code) & 0xff;
        _implement->IWebSocketWrite(reinterpret_cast<const char *>(buf), 2);
    }

    void WebSocketProtocol::Write(const char *buf, unsigned int size, WebSocketFin fin, WebSocketOpcode opcode) {
        unsigned long len = size;
        if (size > 0xffff) {
            len += 10;
        } else if (size > 0x7d) {
            len += 4;
        } else {
            len += 2;
        }
        if (_mode.mark) {
            len += 4;
        }
        unsigned long pos = 0;
        unsigned char mark = 0;
        unsigned char payload = 0;
        unsigned char maskData[4] = {0};
        unsigned long payloadLen = 0;
        auto step = WebSocketStep::FinOpCode;
        auto stream = static_cast<unsigned char *>(::malloc(len));
        std::default_random_engine eg(static_cast<unsigned int>(time(nullptr)));
        std::uniform_int_distribution<int> dis(1, 0xff);
        while (pos < len) {
            switch (step) {
                case WebSocketStep::FinOpCode: {
                    switch (fin) {
                        case WebSocketFin::Begin:
                            stream[pos] = static_cast<int>(opcode) & 0xf;
                            break;
                        case WebSocketFin::Continue:
                            stream[pos] = 0;
                            break;
                        case WebSocketFin::End:
                            stream[pos] = 0x80;
                            break;
                        default:
                            stream[pos] = 0x80 + (static_cast<int>(opcode) & 0xf);
                            break;
                    }
                    step = WebSocketStep::MaskPayloadLen;
                    break;
                }
                case WebSocketStep::MaskPayloadLen: {
                    stream[pos] = 0x0;
                    if (_mode.mark) {
                        stream[pos] += 0x80;
                    }
                    if (size > 0xffff) {
                        stream[pos] += 0x7f;
                        payload = 64;
                        step = WebSocketStep::PayloadLength;
                    } else if (size > 0x7d) {
                        stream[pos] += 0x7e;
                        payload = 16;
                        step = WebSocketStep::PayloadLength;
                    } else {
                        stream[pos] += (size & 0x7f);
                        step = WebSocketStep::PayloadData;
                        if (_mode.mark) {
                            step = WebSocketStep::PayloadMask;
                        }
                    }
                    break;
                }
                case WebSocketStep::PayloadLength: {
                    payload -= 8;
                    stream[pos] = static_cast<unsigned char>(size >> payload);
                    if (payload == 0) {
                        step = WebSocketStep::PayloadData;
                        if (_mode.mark) {
                            step = WebSocketStep::PayloadMask;
                        }
                    }
                    break;
                }
                case WebSocketStep::PayloadMask: {
                    stream[pos] = static_cast<unsigned char>(dis(eg));
                    maskData[mark++] = stream[pos];
                    if (mark == 4) {
                        step = WebSocketStep::PayloadData;
                    }
                    break;
                }
                case WebSocketStep::PayloadData: {
                    if (_mode.mark) {
                        stream[pos] = buf[payloadLen] ^ maskData[payloadLen % 4];
                    } else {
                        stream[pos] = buf[payloadLen];
                    }
                    ++payloadLen;
                    break;
                }
            }
            ++pos;
        }
        _implement->IWebSocketWrite(reinterpret_cast<const char *>(stream), len);
        ::free(stream);
    }

    void WebSocketProtocol::CreateSecWebSocketKey(std::string &key) {
        key.clear();
        std::default_random_engine eg(static_cast<unsigned int>(time(nullptr)));
        std::uniform_int_distribution<int> dis(1, 0xff);
        // make 16 0x100 values
        unsigned char rs[16];
        for (unsigned char &r: rs) {
            r = dis(eg);
        }
        size_t l;
        key.resize(64);
        mbedtls_base64_encode(reinterpret_cast<unsigned char *>(const_cast<char *>(key.data())), key.capacity(), &l, rs,
                              16);
        key.resize(l);
    }

    void WebSocketProtocol::ParserHeaderMap(std::istringstream &ss, std::map<std::string, std::string> &keyMap) {
        std::string line;
        while (std::getline(ss, line)) {
            if (line == "\r" || line[line.size() - 1] != '\r') {
                continue;
            }
            const std::string::size_type pos = line.find(": ");
            if (pos == std::string::npos) {
                continue;
            }
            auto key = std::string(line, 0, pos);
            Utils::StringToLower(key);
            keyMap[key] = std::string(line.c_str() + pos + 2, line.size() - 1 - pos - 2);
        }
    }

#define WEBSOCKET_ERRNO_MAP(XX)                                                                                                                               \
    XX(WebSocketCode::Normal, "正常关闭,意思是已建立连接")                                                                                                 \
    XX(WebSocketCode::GoingAway, "表示某个端点正在离开,例如服务器关闭或浏览器已离开页面")                                                                  \
    XX(WebSocketCode::ProtocolError, "表示端点正在终止连接到期到协议错误")                                                                                 \
    XX(WebSocketCode::Unsupported, "示端点正在终止连接因为它收到了一种它不能接受的数据(例如:只理解文本数据的端点可以发送这个,如果它接收二进制消息)")       \
    XX(WebSocketCode::Reserve, "预定保留")                                                                                                                 \
    XX(WebSocketCode::NoStatus, "保留,表明连接需要状态码来表明")                                                                                           \
    XX(WebSocketCode::AbNormal, "保留值,连接异常关闭")                                                                                                     \
    XX(WebSocketCode::UnsupportedData, "表示端点正在终止连接,因为它接收到的消息中的数据不是与消息的类型一致(例如:非UTF-8)")                                \
    XX(WebSocketCode::PolicyViolation, "表示端点正在终止连接,因为它收到了违反其政策的消息,不知道什么状态码时可以直接用")                                   \
    XX(WebSocketCode::TooLarge, "示端点正在终止连接,因为它收到了一个太大的消息过程")                                                                       \
    XX(WebSocketCode::MissingExtension, "表示端点(客户端)正在终止连接,因为它期望服务器协商一个或更多扩展,但服务器没有在响应中返回它们WebSocket握手的消息") \
    XX(WebSocketCode::InternalError, "客户端由于遇到没有预料的情况阻止其完成请求,因此服务端断开连接")                                                      \
    XX(WebSocketCode::ServiceRestart, "服务器由于重启而断开连接")                                                                                          \
    XX(WebSocketCode::TryAgainLater, "服务器由于临时原因断开连接,如服务器过载因此断开一部分客户端连接")                                                    \
    XX(WebSocketCode::Reserve2, "由WebSocket标准保留以便未来使用")                                                                                         \
    XX(WebSocketCode::TLSHandshake, "TLS Handshake 保留。 表示连接由于无法完成 TLS 握手而关闭 (例如无法验证服务器证书)")

#define WEBSOCKET_STRERROR_GEN(name, msg) case name: return msg;

    inline const char *WebSocketProtocol::GetErrorDesc(WebSocketCode code) {
        switch (code) {
            WEBSOCKET_ERRNO_MAP(WEBSOCKET_STRERROR_GEN)
        }
        return "unknown code";
    }
#undef WEBSOCKET_STRERROR_GEN
}
