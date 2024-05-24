//
// Created by liao on 2024/5/24.
//

#ifndef LCC_APPLICATION_H
#define LCC_APPLICATION_H

#include <atomic>

namespace Lcc {
    class Application {
    public:
        Application();

        virtual ~Application();

        void Run();

        void Shutdown();

        void Sleep(unsigned int ms);

    protected:
        virtual bool IInit() = 0;

        virtual void IRun() = 0;

        virtual void IShutdown() = 0;

    private:
        std::atomic<bool> _running;
    };
}

#endif //LCC_APPLICATION_H
