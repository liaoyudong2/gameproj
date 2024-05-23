//
// Created by liao on 2024/5/23.
//
#include "thread/Thread.h"

namespace Lcc {
    Thread::Thread(): _sleepms(0), _status(Status::Shutdown), _shutdown(false) {
    }

    Thread::~Thread() {
    }

    bool Thread::Startup(unsigned int sleepms) {
        if (_status == Status::Shutdown) {
            _sleepms = sleepms;
            uv_barrier_init(&_waitBarrier, 2);
            _error = uv_thread_create(&_thread, Thread::UvThreadRunMain, this);
            uv_barrier_wait(&_waitBarrier);
            uv_barrier_destroy(&_waitBarrier);
        }
        return _status == Status::Running;
    }

    void Thread::Shutdown() {
        if (_status == Status::Running) {
            _shutdown = true;
            auto current = uv_thread_self();
            if (uv_thread_equal(&_thread, &current) == 0) {
                uv_thread_join(&_thread);
            }
        }
    }

    bool Thread::QueueMessage(void *message) {
        if (_status == Status::Running && !_shutdown) {
            _messages.enqueue(message);
            return true;
        }
        return false;
    }

    void Thread::LoopQueueCommand() {
        void *message = nullptr;
        while (true) {
            if (!_messages.try_dequeue(message)) {
                break;
            }
            IMessage(message);
        }
    }

    void Thread::UvThreadRunMain(void *arg) {
        auto self = static_cast<Thread *>(arg);
        if (self->IInit()) {
            self->_status = Status::Running;
            uv_barrier_wait(&self->_waitBarrier);
            if (self->_sleepms > 0) {
                while (!self->_shutdown) {
                    self->LoopQueueCommand();
                    uv_sleep(self->_sleepms);
                }
            }
            self->LoopQueueCommand();
            self->IShutdown();
        }
        self->_shutdown = false;
        self->_status = Status::Shutdown;
    }
}
