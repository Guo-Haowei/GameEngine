#pragma once
#include "engine/runtime/application.h"
#include "engine/runtime/engine.h"
#include "engine/core/string/string_builder.h"

namespace my {

extern Application* CreateApplication();

int Main(int p_argc, const char** p_argv) {
    int result = 0;
    {
        engine::InitializeCore();
        Application* app = CreateApplication();
        DEV_ASSERT(app);

        if (auto res = app->Initialize(p_argc, p_argv); !res) {
            StringStreamBuilder builder;
            builder << res.error();
            LOG_ERROR("{}", builder.ToString());
        } else {
            Application::Run(app);
        }

        app->Finalize();
        delete app;
        engine::FinalizeCore();
    }
    return result;
}

}  // namespace my
