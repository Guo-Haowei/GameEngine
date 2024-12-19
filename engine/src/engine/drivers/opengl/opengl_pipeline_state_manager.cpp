#include "opengl_pipeline_state_manager.h"

#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/string/string_utils.h"
#include "opengl_helpers.h"
#include "shader_resource_defines.hlsl.h"

namespace my {

namespace fs = std::filesystem;

struct TextureSlot {
    const char *name;
    int slot;
};

static constexpr TextureSlot s_textureSots[] = {
#define SRV(TYPE, NAME, SLOT, BINDING) \
    TextureSlot{ "t_" #NAME, SLOT },
    SRV_DEFINES
#undef SRV
};

OpenGlPipelineState::~OpenGlPipelineState() {
    if (programId) {
        glDeleteProgram(programId);
    }
}

static auto ProcessShader(const fs::path &p_path, int p_depth) -> Result<std::string> {
    constexpr int max_depth = 100;
    if (p_depth >= max_depth) {
        return HBN_ERROR(ErrorCode::ERR_COMPILATION_FAILED, "circular includes in file '{}'!", p_path.string());
    }

    BufferAsset *source_handle = nullptr;
    {
        auto res = AssetManager::GetSingleton().LoadFileSync(FilePath{ p_path.string() });
        if (!res) {
            return HBN_ERROR(res.error());
        }
        source_handle = dynamic_cast<BufferAsset *>(res->get());
    }

    if (source_handle->buffer.empty()) {
        return HBN_ERROR(ErrorCode::ERR_COMPILATION_FAILED, "file '{}' is empty", p_path.string());
    }

    std::string source(source_handle->buffer.begin(), source_handle->buffer.end());

    std::string final_string;
    std::stringstream ss(source);
    for (std::string line; std::getline(ss, line);) {
        constexpr const char pattern[] = "#include";
        if (line.find(pattern) == 0) {
            const char *lineStr = line.c_str();
            const char *quote1 = strchr(lineStr, '"');
            const char *quote2 = strrchr(lineStr, '"');
            DEV_ASSERT(quote1 && quote2 && (quote1 != quote2));
            std::string file_to_include(quote1 + 1, quote2);

            fs::path new_path = p_path;
            new_path.remove_filename();
            new_path = new_path / file_to_include;

            auto res = ProcessShader(new_path, p_depth + 1);
            if (!res) {
                return HBN_ERROR(res.error());
            }

            final_string.append(*res);
        } else {
            final_string.append(line);
        }

        final_string.push_back('\n');
    }

    return final_string;
}

static auto CreateShader(std::string_view p_file, GLenum p_type) -> Result<GLuint> {
    std::string file{ p_file };
    file.append(".glsl");
    fs::path fullpath = fs::path{ ROOT_FOLDER } / "engine" / "shader" / "glsl_generated" / file;

    bool is_generated = true;
    if (!fs::exists(fullpath)) {
        is_generated = false;
        fullpath = fs::path{ ROOT_FOLDER } / "engine" / "shader" / "glsl" / file;
    }

    auto result = ProcessShader(fullpath, 0);
    if (!result) {
        // LOG_FATAL("Failed to create shader program '{}', reason: {}", p_file, res.error());
        return HBN_ERROR(result.error());
    }

    // @TODO: fix this
    std::string fullsource;
    if (!is_generated) {
        fullsource =
            "#version 460 core\n"
            "#extension GL_NV_gpu_shader5 : require\n"
            "#extension GL_NV_shader_atomic_float : enable\n"
            "#extension GL_NV_shader_atomic_fp16_vector : enable\n"
            "#extension GL_ARB_bindless_texture : require\n"
            "#define GLSL_LANG 1\n"
            "";

        // for (const auto &define : p_defines) {
        //     fullsource.append(std::format("#define {} {}\n", define.name, define.value));
        // }
    }

    fullsource.append(*result);
    const char *sources[] = { fullsource.c_str() };

    GLuint shader_id = glCreateShader(p_type);
    glShaderSource(shader_id, 1, sources, nullptr);
    glCompileShader(shader_id);

    GLint status = GL_FALSE, length = 0;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::vector<char> buffer(length + 1);
        glGetShaderInfoLog(shader_id, length, nullptr, buffer.data());
        LOG_ERROR("[glsl] failed to compile shader_id '{}'\ndetails:\n{}", p_file, buffer.data());
        glDeleteShader(shader_id);
        return HBN_ERROR(ErrorCode::ERR_COMPILATION_FAILED, "[glsl] failed to compile shader_id '{}'", p_file);
    }

    if (status == GL_FALSE) {
        glDeleteShader(shader_id);
        return HBN_ERROR(ErrorCode::ERR_COMPILATION_FAILED, "failed to compile shader '{}'", p_file);
    }

    return shader_id;
}

auto OpenGlPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDesc &p_desc) -> Result<std::shared_ptr<PipelineState>> {
    return CreatePipelineImpl(p_desc);
}

auto OpenGlPipelineStateManager::CreateComputePipeline(const PipelineStateDesc &p_desc) -> Result<std::shared_ptr<PipelineState>> {
    return CreatePipelineImpl(p_desc);
}

auto OpenGlPipelineStateManager::CreatePipelineImpl(const PipelineStateDesc &p_desc) -> Result<std::shared_ptr<PipelineState>> {
    GLuint program_id = glCreateProgram();
    std::vector<GLuint> shaders;
    auto create_shader_helper = [&](std::string_view path, GLenum type) {
        auto res = CreateShader(path, type);
        if (res) {
            glAttachShader(program_id, *res);
            shaders.push_back(*res);
        }
        return res;
    };

    ON_SCOPE_EXIT([&]() {
        for (GLuint id : shaders) {
            glDeleteShader(id);
        }
    });

    switch (p_desc.type) {
        case PipelineStateType::GRAPHICS: {
            Result<GLuint> result(0);
            do {
                if (!p_desc.vs.empty()) {
                    result = create_shader_helper(p_desc.vs, GL_VERTEX_SHADER);
                    if (!result) { break; }
                }
                if (!p_desc.gs.empty()) {
                    result = create_shader_helper(p_desc.gs, GL_GEOMETRY_SHADER);
                    if (!result) { break; }
                }
                if (!p_desc.ps.empty()) {
                    result = create_shader_helper(p_desc.ps, GL_FRAGMENT_SHADER);
                    if (!result) { break; }
                }
            } while (0);
            if (!result) {
                return HBN_ERROR(result.error());
            }
        } break;
        case PipelineStateType::COMPUTE: {
            DEV_ASSERT(!p_desc.cs.empty());
            auto result = create_shader_helper(p_desc.cs, GL_COMPUTE_SHADER);
            if (!result) {
                return HBN_ERROR(result.error());
            }
        } break;
        default:
            CRASH_NOW();
            break;
    }

    DEV_ASSERT(!shaders.empty());

    glLinkProgram(program_id);
    GLint status = GL_FALSE;
    GLint length = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, &status);
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &length);
    if (length > 0) {
        std::vector<char> buffer(length + 1);
        glGetProgramInfoLog(program_id, length, nullptr, buffer.data());
        if (status == GL_TRUE) {
            LOG_WARN("[glsl] warning\ndetails:\n{}", buffer.data());
        } else {
            LOG_ERROR("[glsl] failed to link program\ndetails:\n{}", buffer.data());
            return HBN_ERROR(ErrorCode::ERR_CANT_CREATE);
        }
    }

    if (status == GL_FALSE) {
        glDeleteProgram(program_id);
        program_id = 0;
    }

    auto program = std::make_shared<OpenGlPipelineState>(p_desc);
    program->programId = program_id;

    // set constants
    glUseProgram(program_id);
    for (int i = 0; i < array_length(s_textureSots); ++i) {
        const int location = glGetUniformLocation(program_id, s_textureSots[i].name);
        if (location != -1) {
            glUniform1i(location, s_textureSots[i].slot);
        }
    }

    // Setup texture locations
    auto set_location = [&](const char *p_name, int p_slot) {
        const int location = glGetUniformLocation(program_id, p_name);
#if 0
        if (location < 0) {
            LOG_WARN("{} not found, location {}", p_name, location);
        } else {
            LOG_OK("{} found, location {}", p_name, location);
        }
#endif
        glUniform1i(location, p_slot);
    };
    set_location("SPIRV_Cross_Combinedt_BloomInputTextureSPIRV_Cross_DummySampler", GetBloomInputTextureSlot());
    set_location("SPIRV_Cross_Combinedt_BloomInputTextures_linearClampSampler", GetBloomInputTextureSlot());
    set_location("SPIRV_Cross_Combinedt_TextureLightings_linearClampSampler", GetTextureLightingSlot());
    set_location("SPIRV_Cross_Combinedt_TextureHighlightSelectSPIRV_Cross_DummySampler", GetTextureHighlightSelectSlot());
    set_location("SPIRV_Cross_Combinedt_TextureHighlightSelects_linearClampSampler", GetTextureHighlightSelectSlot());
    set_location("SPIRV_Cross_Combinedt_SkyboxHdrs_linearClampSampler", GetSkyboxHdrSlot());
    set_location("SPIRV_Cross_Combinedt_Skyboxs_cubemapClampSampler", GetSkyboxSlot());
    set_location("SPIRV_Cross_Combinedt_BaseColorMaps_linearClampSampler", GetBaseColorMapSlot());
    glUseProgram(0);

    return program;
}

}  // namespace my
