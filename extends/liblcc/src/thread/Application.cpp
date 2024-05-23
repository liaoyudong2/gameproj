//
// Created by liao on 2024/5/24.
//
#include "uv.h"
#include "thread/Application.h"

namespace Lcc {

    Application::Application(): _running(false) {
    }

    Application::~Application() = default;

    void Application::Run(unsigned int sleepms) {
        if (!_running && sleepms > 0) {
            if (IInit()) {
                _running = true;
                while (_running) {
                    IRun();
                    uv_sleep(sleepms);
                }
            }
            IShutdown();
        }
    }

    void Application::Shutdown() {
        _running = false;
    }
}
