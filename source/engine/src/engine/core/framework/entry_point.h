#pragma once
#include "core/framework/application.h"

extern my::Application* CreateApplication();

namespace my {

int Main(int p_argc, const char** p_argv) {
    my::Application* editor = CreateApplication();
    return editor->Run(p_argc, p_argv);
}

}  // namespace my
