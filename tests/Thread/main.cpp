//
// Created by liao on 2024/5/23.
//
#include <iostream>
#include <thread/Thread.h>

class Thread : public Lcc::Thread {
public:
    ~Thread() override = default;

protected:
    inline bool IInit() override {
        std::cout << "Thread::IInit" << std::endl;
        return true;
    }

    inline void IMessage(void *message) override {
        std::cout << "Thread::IMessage: " << message << std::endl;
    }

    inline void IShutdown() override {
        std::cout << "Thread::IShutdown" << std::endl;
    }
};

std::atomic<bool> g_threadrun;

void Exit(int sig) {
    g_threadrun = false;
}

char message[] = "1234567890";

int main(int argc, char *argv[]) {
    signal(SIGINT, Exit);
    signal(SIGTERM, Exit);

    Thread thread;
    g_threadrun = thread.Startup(10);
    while (g_threadrun) {}
    thread.QueueMessage(message);
    thread.Shutdown();
    return 0;
}
