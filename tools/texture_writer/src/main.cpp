#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/systems/job_system/job_system.h"
#include "engine/core/math/geomath.h"
#include "engine/core/framework/engine.h"
#include "engine/core/math/color.h"
#include "engine/core/math/vector_math.h"
#include "pbr.hlsl.h"

using namespace my;

static void WriteImageWrapper(const char* p_file, std::function<void(const char*)> p_func) {
    Timer timer;
    p_func(p_file);
    LOG_OK("Result written to '{}' in {}", p_file, timer.GetDurationString());
}

void WriteBrdfImage(const char* p_file) {
    constexpr int width = 512;
    constexpr int height = 512;
    constexpr int job_count = width * height;
    constexpr int channels = 3;

    float* image_data = new float[width * height * channels];
    jobsystem::Context ctx;
    ctx.Dispatch(job_count, 256, [&](jobsystem::JobArgs args) {
        const int index = args.jobIndex;
        const int x = index % width;
        const int y = index / width;
        const float u = (x + 0.5f) / (float)(width);
        const float v = 1.0f - (y + 0.5f) / (float)(height);
        Vector2f color = IntegrateBRDF(u, v);
        image_data[channels * index + 0] = color.r;
        image_data[channels * index + 1] = color.g;
        image_data[channels * index + 2] = 0.0f;
    });
    ctx.Wait();

    stbi_write_hdr(p_file, width, height, channels, image_data);

    delete[] image_data;
}

void WriteAviatorSkyImage(const char* p_file) {
    constexpr int width = 2048;
    constexpr int height = 1024;
    constexpr int job_count = width * height;
    constexpr int channels = 4;

    auto bottom = Color::Hex(0XE4E0BA);
    auto top = Color::Hex(0xF7D9AA);

    float* image_data = new float[width * height * channels];
    jobsystem::Context ctx;
    ctx.Dispatch(job_count, 256, [&](jobsystem::JobArgs args) {
        const int index = args.jobIndex;
        const int y = index / width;
        float v = 1.0f - (y + 0.5f) / (float)(height);
        auto color = math::Lerp(top, bottom, v);
        image_data[channels * index + 0] = color.r;
        image_data[channels * index + 1] = color.g;
        image_data[channels * index + 2] = color.b;
        image_data[channels * index + 3] = 1.0f;
    });
    ctx.Wait();

    stbi_write_hdr(p_file, width, height, channels, image_data);

    delete[] image_data;
}

int main(int, const char**) {
    
    engine::InitializeCore();

    WriteImageWrapper("aviator_sky.hdr", WriteAviatorSkyImage);

    thread::RequestShutdown();
    engine::FinalizeCore();

    return 0;
}
