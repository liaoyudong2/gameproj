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

        void Run(unsigned int sleepms);

        void Shutdown();

    protected:
        virtual bool IInit() = 0;

        virtual void IRun() = 0;

        virtual void IShutdown() = 0;

    private:
        std::atomic<bool> _running;
    };
}

#endif //LCC_APPLICATION_H
