//
// Created by liao on 2024/5/13.
//

#ifndef LCC_MBEDTLSPLUGIN_H
#define LCC_MBEDTLSPLUGIN_H

#include <string>
#include <buffer/Bio.h>
#include <network/ProtocolPlugin.h>
#include <network/protocol/MbedTLS.h>

namespace Lcc {
    class MbedTLSPlugin : public ProtocolPlugin {
    public:
        explicit MbedTLSPlugin(ProtocolImplement *impl);

        ~MbedTLSPlugin() override;

        Protocol::MbedTLS &GetMbedTLS();

    protected:
        int IProtocolLastError() override;

        const char *IProtocolLastErrDesc() override;

        bool IProtocolPluginOpen() override;

        void IProtocolPluginRead(const char *buf, unsigned int size) override;

        void IProtocolPluginWrite(const char *buf, unsigned int size) override;

        void IProtocolPluginClose() override;

        void IProtocolPluginRelease() override;

    protected:
        bool Handshake();

        void WantFlush();

    protected:
        void ImplementOpen();

        void ImplementClose();

        void ImplementReceive(const char *buf, unsigned int size);

    protected:
        static int MbedTLSRecvCallback(void *ctx, unsigned char *buf, size_t size);

        static int MbedTLSSendCallback(void *ctx, const unsigned char *buf, size_t size);

    private:
        int _error;
        std::string _buffer;
        std::string _errorstr;
        BufferBio _bufferIn;
        BufferBio _bufferOut;
        Protocol::MbedTLS _mbedtls;
    };

    class MbedTLSPluginCreator : public ProtocolPluginCreator {
    public:
        MbedTLSPluginCreator();

        ~MbedTLSPluginCreator() override;

        bool InitializeClientMode(const char *host, const char *caroot);

        bool InitializeServerMode(const char *cert, const char *key, const char *password);

    protected:
        void ICreatorRelease() override;

        ProtocolPlugin *ICreatorAlloc(ProtocolImplement *impl) override;

    private:
        std::string _host;
        std::string _caroot;
        Protocol::MbedTLS *_mbedtls;
    };
}

#endif //LCC_MBEDTLSPLUGIN_H
