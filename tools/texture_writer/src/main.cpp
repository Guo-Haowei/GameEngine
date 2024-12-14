#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "engine/core/os/threads.h"
#include "engine/core/os/timer.h"
#include "engine/core/systems/job_system.h"
#include "engine/core/math/geomath.h"
#include "engine/core/framework/engine.h"
#include "pbr.hlsl.h"

int main(int, const char**) {
    using namespace my;

    constexpr int width = 512;
    constexpr int height = 512;
    constexpr int job_count = width * height;
    constexpr int channels = 3;

    float* image_data = new float[width * height * channels];
    
    engine::InitializeCore();

    Timer timer;

    jobsystem::Context ctx;
    ctx.Dispatch(job_count, 256, [&](jobsystem::JobArgs args) {
        const int x = args.jobIndex % width;
        const int y = args.jobIndex / width;
        const int index = (y * width + x) * channels;
        const float u = (x + 0.5f) / (float)(width);
        const float v = 1.0f - (y + 0.5f) / (float)(height);
        Vector2f color = IntegrateBRDF(u, v);
        image_data[index + 0] = color.r;
        image_data[index + 1] = color.g;
        image_data[index + 2] = 0.0f;
    });
    ctx.Wait();

    constexpr const char* out_file = "my_output_image.hdr";
    stbi_write_hdr(out_file, width, height, channels, image_data);

    LOG_OK("Result written to '{}' in {}", out_file, timer.GetDurationString());

    thread::RequestShutdown();
    engine::FinalizeCore();

    delete[] image_data;
    return 0;
}
