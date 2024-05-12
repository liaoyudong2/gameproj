//
// Created by liao on 2024/5/3.
//

#ifndef LCC_PROTOCOL_PLUGIN_H
#define LCC_PROTOCOL_PLUGIN_H

#include "network/Interface.h"

namespace Lcc {
    class ProtocolPlugin {
    public:
        ProtocolPlugin(ProtocolLevel level, ProtocolImplement *impl) : _level(level), _impl(impl) {
        }

        virtual ~ProtocolPlugin() = default;

        ProtocolImplement *GetImpl() { return _impl; }
        ProtocolLevel GetLevel() const { return _level; }

    public:
        /**
         * 协议插件启动
         * @return 启动协议插件结果
         */
        virtual bool IProtocolPluginOpen() = 0;

        /**
         * 协议插件读取数据触发
         * @param buf 数据流
         * @param size 数据大小
         */
        virtual void IProtocolPluginRead(const char *buf, unsigned int size) = 0;

        /**
         * 协议插件写数据触发
         * @param buf 数据流
         * @param size 数据大小
         */
        virtual void IProtocolPluginWrite(const char *buf, unsigned int size) = 0;

        /**
         * 协议插件关闭触发
         */
        virtual void IProtocolPluginClose() = 0;

        /**
         * 协议插件释放
         */
        virtual void IProtocolPluginRelease() = 0;

    protected:
        ProtocolLevel _level;
        ProtocolImplement *_impl;
    };

    class ProtocolPluginCreator {
    public:
        virtual ~ProtocolPluginCreator() = default;

        /**
         * 协议插件创造器释放
         */
        virtual void ICreatorRelease() = 0;

        /**
         * 
         * @param impl 协议插件创造器分配插件类
         * @return 插件类
         */
        virtual ProtocolPlugin *ICreatorAlloc(ProtocolImplement *impl) = 0;
    };
}

#endif //LCC_PROTOCOL_PLUGIN_H
