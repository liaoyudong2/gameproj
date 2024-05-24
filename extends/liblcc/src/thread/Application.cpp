//
// Created by liao on 2024/5/24.
//
#include "uv.h"
#include "thread/Application.h"

namespace Lcc {
    Application::Application(): _running(false) {
    }

    Application::~Application() = default;

    void Application::Run() {
        if (!_running) {
            if (IInit()) {
                _running = true;
                while (_running) {
                    IRun();
                }
            }
            IShutdown();
        }
    }

    void Application::Shutdown() {
        _running = false;
    }

    void Application::Sleep(unsigned int ms) {
        uv_sleep(ms);
    }
}
