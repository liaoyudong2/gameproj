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

        ProtocolLevel GetLevel() const { return _level; }
        ProtocolImplement *GetImpl() const { return _impl; }

    public:
        /**
         * 返回错误码
         * @return 错误码
         */
        virtual int IProtocolLastError() = 0;

        /**
         * 返回错误码描述
         * @return 错误码信息
         */
        virtual const char *IProtocolLastErrDesc() = 0;

        /**
         * 协议插件启动
         * @return 启动协议插件结果
         */
        virtual bool IProtocolPluginOpen() = 0;

        /**
         * 协议插件读取数据触发
         * @param buf 数据流
         * @param size 数据大小
         * @return 是否读取成功
         */
        virtual bool IProtocolPluginRead(const char *buf, unsigned int size) = 0;

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
         * 获取插件构造器是否初始化完成
         * @return 是否初始化完成
         */
        virtual bool ICreatorInit() = 0;

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
