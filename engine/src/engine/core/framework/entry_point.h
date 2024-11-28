#pragma once
#include "engine/core/framework/application.h"
#include "engine/core/framework/engine.h"

extern my::Application* CreateApplication();

namespace my {

int Main(int p_argc, const char** p_argv) {
    int result = 0;
    {
        engine::InitializeCore();
        my::Application* app = CreateApplication();
        DEV_ASSERT(app);
        result = app->Run(p_argc, p_argv);
        delete app;
        engine::FinalizeCore();
    }
    return result;
}

}  // namespace my
