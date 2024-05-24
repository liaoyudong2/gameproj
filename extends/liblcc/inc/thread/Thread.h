//
// Created by liao on 2024/5/23.
//

#ifndef LCC_THREAD_H
#define LCC_THREAD_H

#include "uv.h"
#include "concurrentqueue.h"

namespace Lcc {
    class Thread {
        enum class Status {
            Running,
            Shutdown,
        };

    public:
        Thread();

        virtual ~Thread();

        /**
         * 启动线程
         * @return 是否启动线程成功
         */
        bool Startup();

        /**
         * 关闭线程
         */
        void Shutdown();

        /**
         * 获取是否正在运行
         * @return 是否运行中
         */
        bool Running() const;

        /**
         * 尝试压入消息队列
         * @param message 消息
         * @return 是否压入队列成功
         */
        bool QueueMessage(void *message);

    protected:
        /**
         * 事件驱动初始化
         */
        void EventLoopInit();

        /**
         * 事件循环派发
         */
        void EventLoopQueueCommand();

    protected:
        /**
         * 线程初始化
         * @return 是否初始化OK
         */
        virtual bool IInit() = 0;

        /**
         * 线程执行
         * @param message 需要处理的消息
         */
        virtual void IMessage(void *message) = 0;

        /**
         * 线程关闭
         */
        virtual void IShutdown() = 0;

        /**
         * 返回uv loop句柄
         * @return 事件loop句柄
         */
        uv_loop_t *GetEventLoop();

    protected:
        static void UvThreadRunMain(void *arg);

        static void UvThreadShutdown(uv_handle_t *handle);

        static void UvThreadEventTrigger(uv_async_t *async);

        static void UvThreadShutdownTrigger(uv_async_t *async);

    private:
        int _error;
        Status _status;
        uv_thread_t _thread;
        uv_loop_t _threadLoop;
        uv_barrier_t _waitSignal;
        uv_async_t _eventTrigger;
        uv_async_t _shutdownTrigger;
        moodycamel::ConcurrentQueue<void *> _messages;
    };
}

#endif //LCC_THREAD_H
