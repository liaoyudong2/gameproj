//
// Created by liao on 2024/5/16.
//

#ifndef LCC_WEBSOCKET_H
#define LCC_WEBSOCKET_H

#include <map>
#include <string>

namespace Lcc {
    enum class WebSocketOpcode {
        Text = 0x1,
        Binary = 0x2,
        Close = 0x8,
        Ping = 0x9,
        Pong = 0xa
    };

    enum class WebSocketFin {
        Begin,
        Continue,
        End,
        Normal,
        Error,
    };

    enum class WebSocketFinMode {
        Normal,
        Fin,
    };

    enum class WebSocketStep {
        // fin opcode byte
        FinOpCode,
        // mask & payload len
        MaskPayloadLen,
        // payload
        PayloadLength,
        // mask
        PayloadMask,
        // payload data
        PayloadData,
    };

    struct WebSocketMode {
        bool mark;
        WebSocketOpcode opcode;
    };

    struct WebSocketFrameHeader {
        bool fin;
        bool mask;
        WebSocketOpcode opcode;
        unsigned long payloadLen;
        unsigned char maskData[4];
    };

    struct WebSocketFrameAssist {
        unsigned char payload;
        unsigned char mask;
        unsigned long maskIndex;
        unsigned long payloadRead;
    };

    struct WebSocketFrameReader {
        WebSocketFinMode mode;
        WebSocketFin fin;
        WebSocketStep step;
        std::string _frameBuffers[2];
        WebSocketFrameHeader _finHeader;
        WebSocketFrameHeader _frameHeader[2];
        WebSocketFrameAssist _frameAssist[2];
    };

    enum class WebSocketCode {
        Normal = 1000, // 正常关闭,意思是已建立连接
        GoingAway = 1001, // 表示某个端点正在"离开",例如服务器关闭或浏览器已离开页面
        ProtocolError = 1002, // 表示端点正在终止连接到期到协议错误
        Unsupported = 1003, // 示端点正在终止连接因为它收到了一种它不能接受的数据(例如:只理解文本数据的端点可以发送这个,如果它接收二进制消息)
        Reserve = 1004, // 预定保留
        NoStatus = 1005, // 保留,表明连接需要状态码来表明
        AbNormal = 1006, // 保留值,连接异常关闭
        UnsupportedData = 1007, // 表示端点正在终止连接,因为它接收到的消息中的数据不是与消息的类型一致(例如:非 UTF-8)
        PolicyViolation = 1008, // 表示端点正在终止连接,因为它收到了违反其政策的消息,不知道什么状态码时可以直接用
        TooLarge = 1009, // 表示端点正在终止连接,因为它收到了一个太大的消息过程
        MissingExtension = 1010, // 表示端点(客户端)正在终止连接,因为它期望服务器协商一个或更多扩展,但服务器没有在响应中返回它们WebSocket握手的消息
        InternalError = 1011, // 客户端由于遇到没有预料的情况阻止其完成请求, 因此服务端断开连接
        ServiceRestart = 1012, // 服务器由于重启而断开连接
        TryAgainLater = 1013, // 服务器由于临时原因断开连接, 如服务器过载因此断开一部分客户端连接
        Reserve2 = 1014, // 由 WebSocket 标准保留以便未来使用
        TLSHandshake = 1015, // TLS Handshake 保留。 表示连接由于无法完成 TLS 握手而关闭 (例如无法验证服务器证书)
        /**
            1016–1999 - 由 WebSocket 标准保留以便未来使用。
            2000–2999 - 由 WebSocket 拓展保留使用。
            3000–3999 - 可以由库或框架使用。 不应由应用使用。 可以在 IANA 注册, 先到先得。
            4000–4999 - 可以由应用使用。
        */
    };

    class WebSocketImplement {
    public:
        virtual ~WebSocketImplement() = default;

        /**
         * 初始化
         * @param mode 模式
         */
        virtual void IWebSocketInit(WebSocketMode &mode) = 0;

        /**
         * 收到帧数据时触发
         * @param header 帧数据头部信息
         * @param buf 帧数据
         * @param size 数据长度
         */
        virtual void IWebSocketReceive(WebSocketFrameHeader &header, const char *buf, unsigned int size) = 0;

        /**
         * 需要发送帧数据时触发
         * @param buf 数据
         * @param size 数据长度
         */
        virtual void IWebSocketWrite(const char *buf, unsigned int size);
    };

    class WebSocketProtocol {
    public:
        explicit WebSocketProtocol(WebSocketImplement *impl);

        ~WebSocketProtocol();

        /**
         * 初始化
         */
        void Initialize();

        /**
         * 握手请求数据和校验密钥
         * @param host 远端地址
         * @param serverKey 输出的请求对应server密钥
         */
        void HandshakeRequest(const char *host, std::string &serverKey);

        /**
         * server端对数据进行响应
         * @param buf 输入数据
         * @param size 数据长度
         * @return 是否输入数据有异常
         */
        bool HandshakeResponse(const char *buf, unsigned int size);

        /**
         * 校验server返回的密钥
         * @param buf 输入数据流
         * @param size 数据长度
         * @param serverKey 待对比校验的server密钥
         * @return 是否密钥一致
         */
        bool CheckServerSecKey(const char *buf, unsigned int size, std::string &serverKey);

        /**
         * pong响应
         * @param buf 输入数据流
         * @param size 数据长度
         */
        void Pong(const char *buf, unsigned int size);

        /**
        * 读取帧数据
        * @param buf 输入数据流
        * @param size 数据长度
        * @return 是否读取正常
        */
        bool Read(const char *buf, unsigned int size);

        /**
         * 写数据
         * @param buf 输入数据流
         * @param size 数据长度
         */
        void Write(const char *buf, unsigned int size);

        /**
         * 推送关闭
         * @param code 关闭原因
         */
        void Close(WebSocketCode code);

        /**
         * 获取错误码描述
         * @param code 错误码
         * @return 返回错误描述
         */
        const char *GetErrorDesc(WebSocketCode code);

    protected:
        /**
        * 写数据
         * @param buf 输入数据流
         * @param size 数据长度
         * @param fin 发送切片模式
         * @param opcode 操作码
         */
        void Write(const char *buf, unsigned int size, WebSocketFin fin, WebSocketOpcode opcode);

        /**
         * 空数据的触发
         */
        void PayloadZeroDataCallback();

        /**
         * 触发数据回调
         * @param buf 数据
         * @param size 数据长度
         * @param complete 是否完全获取
         */
        void PayLoadDataCallback(const char *buf, unsigned long size, bool complete);

    protected:
        /**
         * 生成WebSocket密钥
         * @param key 输出的密钥
         */
        static void CreateSecWebSocketKey(std::string &key);

        /**
         * 解析头部数据
         * @param ss 解析流
         * @param keyMap 映射表
         */
        static void ParserHeaderMap(std::istringstream &ss, std::map<std::string, std::string> &keyMap);

        /**
         * 检查字节对应的fin状态
         * @param p 字节
         * @return 返回fin状态
         */
        static WebSocketFin CheckFinStatus(unsigned char p);

    private:
        WebSocketMode _mode;
        WebSocketImplement *_implement;
        WebSocketFrameReader _frameReader;
    };
}

#endif //LCC_WEBSOCKET_H
