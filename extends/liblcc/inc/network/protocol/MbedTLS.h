//
// Created by liao on 2024/5/13.
//

#ifndef LCC_MBEDTLS_H
#define LCC_MBEDTLS_H

#include <string>
#include "mbedtls/ssl.h"
#include "mbedtls/x509.h"
#include "mbedtls/error.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

namespace Lcc {
    namespace Protocol {
        class MbedTLS {
            enum class Mode {
                None,
                ClientMode,
                ServerMode,
                SessionMode,
            };

        public:
            MbedTLS();
            virtual ~MbedTLS();

            /**
             * 释放证书
             */
            void Release();

            /**
             * 获取异常错误码
             * @return 错误码
             */
            int GetError() const;

            /**
             * 获取错误描述
             * @return 返回描述
             */
            const char *GetErrorDesc() const;

            /**
             * 获取证书连接是否有效性
             * @return 返回校验结果
             */
            bool Verify() const;

            /**
             * 获取是否以及初始化证书
             * @return 是否已经初始化
             */
            bool Enabled() const;

            /**
             * 给server的session对象初始化证书信息
             * @param ssl 证书对象
             * @return 是否初始化成功
             */
            bool InitializeForSession(const MbedTLS &ssl);

            /**
             * 初始化客户端证书信息
             * @param host 远端地址
             * @param caroot 根证书路径(可选)
             * @return 是否初始化成功
             */
            bool InitializeForClient(const char *host, const char *caroot);

            /**
             * 初始化服务端证书信息
             * @param cert 证书路径
             * @param key 证书密钥路径
             * @param password 证书密码
             * @return 是否初始化成功
             */
            bool InitializeForServer(const char *cert, const char *key, const char *password);

        private:
            int _error;
            Mode _mode;
            bool _caroot;
            std::string _errorstr;

        private:
            mbedtls_x509_crt _x509Crt;
            mbedtls_pk_context _pkCtx;
            mbedtls_ssl_config _sslCfg;
            mbedtls_ssl_context _sslCtx;
            mbedtls_entropy_context _entropyCtx;
            mbedtls_ctr_drbg_context _ctrDrbgCtx;
        };
    }
}

#endif //LCC_MBEDTLS_H
