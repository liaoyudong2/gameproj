//
// Created by liao on 2024/5/3.
//

#ifndef LCC_ADDRESS_H
#define LCC_ADDRESS_H

#include <cstring>
#include <regex>
#include "utils/Strings.h"

namespace Lcc {
    namespace Utils {
        enum class HostProtocol {
            Tcp,
            Http,
            Websocket,
        };

        struct HostAddress {
            bool v6;
            bool ssl;
            int port;
            char ip[128];
            char host[128];
            HostProtocol protocol;
        };

        inline bool HostParse(std::string &&host, HostAddress &addr) {
            bool nonePort = false;
            bool chkValid = false;
            std::smatch sm;
            // 匹配ip端口类
            std::regex regex_ip("(http|https|ws|wss|tcp)://([0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}):([0-9]{2,5})");
            if (std::regex_match(host, sm, regex_ip)) {
                // (协议) (xxx.xxx.xxx.xxx) (端口)
                chkValid = true;
            } else {
                // 匹配域名类
                std::regex regex_host("(http|https|ws|wss|tcp)://([0-9a-zA-z\\-_.]+):([0-9]{2,5})");
                if (std::regex_match(host, sm, regex_host)) {
                    // (协议) (域名) (端口)
                    chkValid = true;
                } else {
                    if (host.find("http") != std::string::npos) {
                        std::regex regex_host2("(http|https|ws|wss|tcp)://([0-9a-zA-z\\-_.]+)");
                        if (std::regex_match(host, sm, regex_host2)) {
                            // (https) (域名) (80|443)
                            chkValid = true;
                            nonePort = true;
                        }
                    }
                }
            }
            if (chkValid) {
                addr.v6 = false;
                addr.port = 0;
                memset(&addr.ip, 0, sizeof(addr.ip));
                memset(&addr.host, 0, sizeof(addr.host));
                // 匹配
                std::string protocol = sm[1].str();
                StringToLower(protocol);
                if (protocol.find("ws") != std::string::npos) {
                    addr.protocol = HostProtocol::Websocket;
                    if (protocol.find("wss") != std::string::npos) {
                        addr.ssl = true;
                    }
                } else if (protocol.find("http") != std::string::npos) {
                    addr.protocol = HostProtocol::Http;
                    if (protocol.find("https") != std::string::npos) {
                        addr.ssl = true;
                    }
                } else {
                    addr.protocol = HostProtocol::Tcp;
                }
                StringCopy(addr.host, sizeof(addr.host), sm[2].str().c_str(), sm[2].str().size());
                if (!nonePort) {
                    chkValid = StringToNumber(sm[3].str(), &addr.port);
                } else {
                    addr.port = 80;
                    if (addr.ssl) {
                        addr.port = 443;
                    }
                }
            }
            return chkValid;
        }
    }
}

#endif //LCC_ADDRESS_H
