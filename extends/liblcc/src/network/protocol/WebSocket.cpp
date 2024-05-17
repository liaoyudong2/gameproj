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
        _frameReader.fin = WebSocketFin::Normal;
        _frameReader.mode = WebSocketFinMode::Normal;
        _frameReader.step = WebSocketStep::FinOpCode;
        memset(&_frameReader._finHeader, 0, sizeof(_frameReader._finHeader));
        memset(_frameReader._frameAssist, 0, sizeof(_frameReader._frameAssist));
        memset(_frameReader._frameHeader, 0, sizeof(_frameReader._frameHeader));
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

    /*-------------------------------------------------------------------
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-------+-+-------------+-------------------------------+
    |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    | |1|2|3|       |K|             |                               |
    +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    |     Extended payload length continued, if payload len == 127  |
    + - - - - - - - - - - - - - - - +-------------------------------+
    |                               |Masking-key, if MASK set to 1  |
    +-------------------------------+-------------------------------+
    | Masking-key (continued)       |          Payload Data         |
    +-------------------------------- - - - - - - - - - - - - - - - +
    :                     Payload Data continued ...                :
    + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    |                     Payload Data continued ...                |
    +---------------------------------------------------------------+
    1. FIN：表示这个数据是不是接收完毕，为1表示收到的数据是完整的，占1bit
    2. RSV1～3：用于扩展，通常都为0，各占1bit
    3. OPCODE：表示报文的类型，占4bit
    (1). 0x00：标识一个中间数据包
    (2). 0x01：标识一个text数据包
    (3). 0x02：标识一个二进制数据包
    (4). 0x03～07：保留
    (5). 0x08：标识一个断开连接数据包
    (6). 0x09：标识一个ping数据包
    (7). 0x0A：标识一个pong数据包
    (8). 0x0B～F：保留
    4. MASK：用于表示数据是否经常掩码处理，为1时，Masking-key即存在，占1bit
    5. Payload len：表示数据长度，即Payload Data的长度，当Payload len为0～125时，表示的值就是Payload Data的真实长度；
    当Payload len为126时，报文其后的2个字节形成的16bits无符号整型数的值是Payload Data的真实长度（网络字节序，需转换）；
    当Payload len为127时，报文其后的8个字节形成的64bits无符号整型数的值是Payload Data的真实长度（网络字节序，需转换）；
    6. Masking-key：掩码，当Mask为1时存在，占4字节32bit
    7. Payload Data：表示数据
    -----------------------------------------------------------------------*/
    bool WebSocketProtocol::Read(const char *buf, unsigned int size) {
        unsigned long pos = 0;
        unsigned long left = 0;
        unsigned long nread = 0;
        auto stream = reinterpret_cast<unsigned char *>(const_cast<char *>(buf));
        WebSocketFrameHeader *header = &_frameReader._frameHeader[static_cast<int>(_frameReader.mode)];
        WebSocketFrameAssist *assist = &_frameReader._frameAssist[static_cast<int>(_frameReader.mode)];
        while (pos < size) {
            const unsigned char p = buf[pos];
            switch (_frameReader.step) {
                case WebSocketStep::FinOpCode: {
                    _frameReader.fin = CheckFinStatus(p);
                    if (_frameReader.fin == WebSocketFin::Error) {
                        return false;
                    }
                    if (_frameReader.fin == WebSocketFin::Normal) {
                        _frameReader.mode = WebSocketFinMode::Normal;
                        _frameReader._frameBuffers[static_cast<int>(_frameReader.mode)].clear();
                    } else {
                        _frameReader.mode = WebSocketFinMode::Fin;
                    }
                    header = &_frameReader._frameHeader[static_cast<int>(_frameReader.mode)];
                    assist = &_frameReader._frameAssist[static_cast<int>(_frameReader.mode)];
                    memset(header, 0, sizeof(WebSocketFrameHeader));
                    memset(assist, 0, sizeof(WebSocketFrameAssist));
                    if ((p & 0x40) || (p & 0x20) || (p & 0x10)) {
                        // rsv not support
                        return false;
                    }
                    header->fin = ((p & 0x80) == 0x80);
                    header->opcode = static_cast<WebSocketOpcode>(p & 0xf);
                    if ((header->opcode > WebSocketOpcode::Binary && header->opcode < WebSocketOpcode::Close) || header
                        ->opcode > WebSocketOpcode::Pong) {
                        // 0x03~0x07 0xb~0xf not support
                        return false;
                    }
                    if (_frameReader.fin == WebSocketFin::Begin) {
                        _frameReader._frameBuffers[static_cast<int>(_frameReader.mode)].clear();
                        memset(&_frameReader._finHeader, 0, sizeof(WebSocketFrameHeader));
                        _frameReader._finHeader.fin = header->fin;
                        _frameReader._finHeader.opcode = header->opcode;
                    }
                    _frameReader.step = WebSocketStep::MaskPayloadLen;
                    break;
                }
                case WebSocketStep::MaskPayloadLen: {
                    assist->payload = 0;
                    header->mask = ((p & 0x80) == 0x80);
                    header->payloadLen = p & 0x7f;
                    // <126  payload | =126 payload16 | =127 payload64
                    if (header->payloadLen == 0x7e) {
                        assist->payload = 16;
                        header->payloadLen = 0;
                        _frameReader.step = WebSocketStep::PayloadLength;
                    } else if (header->payloadLen == 0x7f) {
                        assist->payload = 64;
                        header->payloadLen = 0;
                        _frameReader.step = WebSocketStep::PayloadLength;
                    } else {
                        if (header->mask) {
                            _frameReader.step = WebSocketStep::PayloadMask;
                        } else {
                            _frameReader.step = WebSocketStep::PayloadData;
                        }
                    }
                    break;
                }
                case WebSocketStep::PayloadLength: {
                    assist->payload -= 8;
                    header->payloadLen += (p << assist->payload);
                    if (assist->payload == 0) {
                        if (_frameReader.fin != WebSocketFin::Normal) {
                            _frameReader._finHeader.payloadLen += header->payloadLen;
                        }
                        _frameReader.step = WebSocketStep::PayloadData;
                        if (header->mask) {
                            _frameReader.step = WebSocketStep::PayloadMask;
                        } else {
                            if (PayloadZeroDataCallback()) {
                                // payload len is zero
                                _frameReader.step = WebSocketStep::FinOpCode;
                            }
                        }
                    }
                    break;
                }
                case WebSocketStep::PayloadMask: {
                    assist->maskIndex = 0;
                    header->maskData[assist->mask++] = p;
                    if (_frameReader.fin == WebSocketFin::Begin) {
                        _frameReader._finHeader.mask = true;
                        _frameReader._finHeader.maskData[assist->mask - 1] = p;
                    }
                    if (assist->mask == 4) {
                        _frameReader.step = WebSocketStep::PayloadData;
                        if (PayloadZeroDataCallback()) {
                            // payload len is zero
                            _frameReader.step = WebSocketStep::FinOpCode;
                        }
                    }
                    break;
                }
                case WebSocketStep::PayloadData: {
                    left = size - pos;
                    // payload mask decode
                    if (header->mask) {
                        unsigned long i = pos;
                        const unsigned long loop = std::min(left, header->payloadLen);
                        for (unsigned long n = 0; n < loop; ++n, ++i) {
                            stream[i] = stream[i] ^ header->maskData[assist->maskIndex++ % 4];
                        }
                    }
                    nread = header->payloadLen - assist->payloadRead;
                    // payload data check length
                    if (left >= nread) {
                        const bool complete = _frameReader.mode == WebSocketFinMode::Normal || _frameReader.fin == WebSocketFin::End;
                        PayLoadDataCallback(reinterpret_cast<const char *>(stream + pos), nread, complete);
                        pos += nread;
                        assist->payloadRead = header->payloadLen;
                        _frameReader.step = WebSocketStep::FinOpCode;
                    } else {
                        pos += left;
                        PayLoadDataCallback(reinterpret_cast<const char *>(stream + pos), left, false);
                        pos += nread;
                        assist->payloadRead += left;
                    }
                    --pos;
                    break;
                }
            }
            ++pos;
        }
        return true;
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
        buf[0] = (static_cast<unsigned short>(code) >> 8);
        buf[1] = static_cast<int>(code) & 0xff;
        Write(reinterpret_cast<const char *>(buf), 2, WebSocketFin::Normal, WebSocketOpcode::Close);
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

    const char *WebSocketProtocol::GetErrorDesc(WebSocketCode code) {
        switch (code) {
            WEBSOCKET_ERRNO_MAP(WEBSOCKET_STRERROR_GEN)
        }
        return "unknown code";
    }
#undef WEBSOCKET_STRERROR_GEN

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

    bool WebSocketProtocol::PayloadZeroDataCallback() {
        if (_frameReader._frameHeader[static_cast<int>(_frameReader.mode)].payloadLen == 0) {
            PayLoadDataCallback(nullptr, 0, true);
            return true;
        }
        return false;
    }

    void WebSocketProtocol::PayLoadDataCallback(const char *buf, unsigned long size, bool complete) {
        std::string &buffer = _frameReader._frameBuffers[static_cast<int>(_frameReader.mode)];
        WebSocketFrameHeader &header = _frameReader._frameHeader[static_cast<int>(_frameReader.mode)];
        if (!complete) {
            if (buffer.capacity() == 0) {
                buffer.resize(0x10000);
                buffer.clear();
            }
            buffer.append(buf, size);
        } else {
            if (!buffer.empty()) {
                buffer.append(buf, size);
                if (_frameReader.mode == WebSocketFinMode::Fin) {
                    _implement->IWebSocketReceive(_frameReader._finHeader, buffer.c_str(), buffer.size());
                } else {
                    _implement->IWebSocketReceive(header, buffer.c_str(), buffer.size());
                }
            } else {
                _implement->IWebSocketReceive(header, buf, size);
            }
        }
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

    WebSocketFin WebSocketProtocol::CheckFinStatus(unsigned char p) {
        const bool fin = (p & 0x80) == 0x80;
        const unsigned char opcode = (p & 0xf);
        if (fin == 0 && opcode != 0) {
            return WebSocketFin::Begin;
        } else if (fin == 0 && opcode == 0) {
            return WebSocketFin::Continue;
        } else if (fin == 1 && opcode == 0) {
            return WebSocketFin::End;
        } else if (fin == 1 && opcode != 0) {
            return WebSocketFin::Normal;
        } else {
            return WebSocketFin::Error;
        }
    }
}
