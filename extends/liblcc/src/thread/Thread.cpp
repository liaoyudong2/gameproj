//
// Created by liao on 2024/5/23.
//
#include "thread/Thread.h"

namespace Lcc {
    Thread::Thread(): _status(Status::Shutdown) {
    }

    Thread::~Thread() {
    }

    bool Thread::Startup() {
        if (_status == Status::Shutdown) {
            uv_barrier_init(&_waitSignal, 2);
            _error = uv_thread_create(&_thread, Thread::UvThreadRunMain, this);
            uv_barrier_wait(&_waitSignal);
            uv_barrier_destroy(&_waitSignal);
        }
        return _status == Status::Running;
    }

    void Thread::Shutdown() {
        if (Running()) {
            _status = Status::Shutdown;
            uv_async_send(&_shutdownTrigger);
            uv_thread_join(&_thread);
        }
    }

    bool Thread::Running() const {
        return _status == Status::Running;
    }

    bool Thread::QueueMessage(void *message) {
        if (Running()) {
            _messages.enqueue(message);
            uv_async_send(&_eventTrigger);
            return true;
        }
        return false;
    }

    void Thread::EventLoopInit() {
        uv_loop_init(&_threadLoop);
        uv_async_init(&_threadLoop, &_eventTrigger, Thread::UvThreadEventTrigger);
        uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&_eventTrigger), this);
        uv_async_init(&_threadLoop, &_shutdownTrigger, Thread::UvThreadShutdownTrigger);
        uv_handle_set_data(reinterpret_cast<uv_handle_t *>(&_shutdownTrigger), this);
        if (IInit()) {
            _status = Status::Running;
            uv_barrier_wait(&_waitSignal);
        }
        uv_run(&_threadLoop, UV_RUN_DEFAULT);
        uv_loop_close(&_threadLoop);
    }

    void Thread::EventLoopQueueCommand() {
        void *message = nullptr;
        while (_messages.try_dequeue(message)) {
            IMessage(message);
        }
    }

    uv_loop_t *Thread::GetEventLoop() {
        return &_threadLoop;
    }

    void Thread::UvThreadRunMain(void *arg) {
        static_cast<Thread *>(arg)->EventLoopInit();
    }

    void Thread::UvThreadShutdown(uv_handle_t *handle) {
        static_cast<Thread *>(uv_handle_get_data(handle))->IShutdown();
    }

    void Thread::UvThreadEventTrigger(uv_async_t *async) {
        auto self = static_cast<Thread *>(uv_handle_get_data(reinterpret_cast<const uv_handle_t *>(async)));
        self->EventLoopQueueCommand();
    }

    void Thread::UvThreadShutdownTrigger(uv_async_t *async) {
        auto self = static_cast<Thread *>(uv_handle_get_data(reinterpret_cast<const uv_handle_t *>(async)));
        uv_close(reinterpret_cast<uv_handle_t *>(&self->_eventTrigger), nullptr);
        uv_close(reinterpret_cast<uv_handle_t *>(&self->_shutdownTrigger), Thread::UvThreadShutdown);
    }
}
