#include "opengl_pipeline_state_manager.h"

#include "core/framework/asset_manager.h"

namespace my {

static std::string process_shader(const std::string &source, int depth) {
    constexpr int max_depth = 100;
    if (depth >= max_depth) {
        LOG_FATAL("maximum include depth {} exceeded", max_depth);
    }

    std::string result;
    std::stringstream ss(source);
    for (std::string line; std::getline(ss, line);) {
        constexpr const char pattern[] = "#include";
        if (line.find(pattern) == 0) {
            const char *lineStr = line.c_str();
            const char *quote1 = strchr(lineStr, '"');
            const char *quote2 = strrchr(lineStr, '"');
            DEV_ASSERT(quote1 && quote2 && (quote1 != quote2));
            std::string file_to_include(quote1 + 1, quote2);

            file_to_include = "@res://glsl/" + file_to_include;
            // @TODO: nested include
            auto buffer = AssetManager::singleton().load_file_sync(file_to_include);
            DEV_ASSERT(buffer);
            std::string extra(buffer->buffer.begin(), buffer->buffer.end());
            if (extra.empty()) {
                LOG_ERROR("[filesystem] failed to read shader '{}'", file_to_include);
            }
            result.append(process_shader(extra, depth + 1));
        } else {
            result.append(line);
        }

        result.push_back('\n');
    }

    return result;
}

static GLuint create_shader(std::string_view file, GLenum type) {
    auto source_binary = AssetManager::singleton().load_file_sync(std::string(file));
    DEV_ASSERT(source_binary);

    std::string source(source_binary->buffer.begin(), source_binary->buffer.end());
    if (source.empty()) {
        LOG_ERROR("[filesystem] failed to read shader '{}'", file);
        return 0;
    }

    std::string fullsource =
        "#version 460 core\n"
        "#extension GL_NV_gpu_shader5 : require\n"
        "#extension GL_NV_shader_atomic_float : enable\n"
        "#extension GL_NV_shader_atomic_fp16_vector : enable\n"
        "#extension GL_ARB_bindless_texture : require\n"
        "";

    fullsource.append(process_shader(source, 0));
    const char *sources[] = { fullsource.c_str() };

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, sources, nullptr);
    glCompileShader(shader);

    GLint status = GL_FALSE, length = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::vector<char> buffer(length + 1);
        glGetShaderInfoLog(shader, length, nullptr, buffer.data());
        LOG_FATAL("[glsl] failed to compile shader '{}'\ndetails:\n{}", file, buffer.data());
    }

    if (status == GL_FALSE) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

std::shared_ptr<PipelineState> OpenGLPipelineStateManager::create(const PipelineCreateInfo &info) {
    GLuint programID = glCreateProgram();
    std::vector<GLuint> shaders;
    auto create_shader_helper = [&](std::string_view path, GLenum type) {
        if (!path.empty()) {
            GLuint shader = create_shader(path, type);
            glAttachShader(programID, shader);
            shaders.push_back(shader);
        }
    };

    ON_SCOPE_EXIT([&]() {
        for (GLuint id : shaders) {
            glDeleteShader(id);
        }
    });

    if (!info.cs.empty()) {
        DEV_ASSERT(info.vs.empty());
        DEV_ASSERT(info.ps.empty());
        DEV_ASSERT(info.gs.empty());
        create_shader_helper(info.cs, GL_COMPUTE_SHADER);
    } else if (!info.vs.empty()) {
        DEV_ASSERT(info.cs.empty());
        create_shader_helper(info.vs, GL_VERTEX_SHADER);
        create_shader_helper(info.ps, GL_FRAGMENT_SHADER);
        create_shader_helper(info.gs, GL_GEOMETRY_SHADER);
    }

    DEV_ASSERT(!shaders.empty());

    glLinkProgram(programID);
    GLint status = GL_FALSE, length = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &status);
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::vector<char> buffer(length + 1);
        glGetProgramInfoLog(programID, length, nullptr, buffer.data());
        LOG_FATAL("[glsl] failed to link program\ndetails:\n{}", buffer.data());
    }

    if (status == GL_FALSE) {
        glDeleteProgram(programID);
        programID = 0;
    }

    auto program = std::make_shared<OpenGLPipelineState>();
    program->program_id = programID;
    return program;
}

}  // namespace my
