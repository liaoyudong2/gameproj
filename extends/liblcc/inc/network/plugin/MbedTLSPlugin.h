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
        BufferBio _bufferIn;
        BufferBio _bufferOut;
        Protocol::MbedTLS _mbedtls;

    };
}

#endif //LCC_MBEDTLSPLUGIN_H
