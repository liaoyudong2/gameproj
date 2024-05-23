//
// Created by liao on 2024/5/23.
//

#ifndef LCC_THREAD_H
#define LCC_THREAD_H

#include <atomic>
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
         * @param sleepms 休眠毫秒 0不循环 >0休眠时间
         * @return 是否启动线程成功
         */
        bool Startup(unsigned int sleepms);

        /**
         * 关闭线程
         */
        void Shutdown();

        /**
         * 尝试压入消息队列
         * @param message 消息
         * @return 是否压入队列成功
         */
        bool QueueMessage(void *message);

    protected:
        /**
         * 事件循环派发
         */
        void LoopQueueCommand();

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

    protected:
        static void UvThreadRunMain(void *arg);

    private:
        int _error;
        uv_thread_t _thread;
        unsigned int _sleepms;
        uv_barrier_t _waitBarrier;
        std::atomic<Status> _status;
        std::atomic<bool> _shutdown;
        moodycamel::ConcurrentQueue<void *> _messages;
    };
}

#endif //LCC_THREAD_H
