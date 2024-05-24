//
// Created by liao on 2024/5/23.
//
#include <iostream>
#include <thread/Thread.h>
#include <thread/Application.h>

char message[] = "1234567890";

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

class Application : public Lcc::Application {
public:
    ~Application() override = default;

protected:
    inline bool IInit() override {
        std::cout << "Application::IInit" << std::endl;
        return _thread.Startup();
    }

    inline void IRun() override {
        std::cout << "Application::IRun" << std::endl;
        _thread.QueueMessage(message);
        Sleep(1000);
    }

    inline void IShutdown() override {
        _thread.Shutdown();
        std::cout << "Application::IShutdown" << std::endl;
    }

private:
    Thread _thread;
};

Application app;

void Exit(int sig) {
    app.Shutdown();
}

int main(int argc, char *argv[]) {
    signal(SIGINT, Exit);
    signal(SIGTERM, Exit);

    app.Run();
    return 0;
}
