#include "render_graph_vxgi.h"

#include "core/base/rid_owner.h"
#include "core/math/frustum.h"
#include "rendering/rendering_dvars.h"
#include "servers/display_server.h"

// @TODO: refactor
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_data.h"
#include "rendering/shader_program_manager.h"

extern GpuTexture g_albedoVoxel;
extern GpuTexture g_normalVoxel;
extern MeshData g_box;

extern void FillMaterialCB(const MaterialData* mat, MaterialConstantBuffer& cb);

extern my::RIDAllocator<MeshData> g_meshes;
extern my::RIDAllocator<MaterialData> g_materials;

namespace my::rg {

// @TODO: refactor render passes
void shadow_pass_func(int layer) {
    const my::Scene& scene = SceneManager::get_scene();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glClear(GL_DEPTH_BUFFER_BIT);

    const int res = DVAR_GET_INT(r_shadow_res);

    glViewport(0, 0, res, res);

    auto render_data = GraphicsManager::singleton().get_render_data();
    RenderData::Pass& pass = render_data->shadow_passes[layer];

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *scene.get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= SC_BONE_MAX);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        const auto& program = ShaderProgramManager::get(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);
        program.bind();

        g_perBatchCache.cache.c_projection_view_model_matrix = pass.projection_view_matrix * draw.world_matrix;
        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.Update();

        glBindVertexArray(draw.mesh_data->vao);
        glDrawElements(GL_TRIANGLES, draw.mesh_data->count, GL_UNSIGNED_INT, 0);
    }

    glCullFace(GL_BACK);
    glUseProgram(0);
}

void voxelization_pass_func(int) {
    g_albedoVoxel.clear();
    g_normalVoxel.clear();

    const int voxel_size = DVAR_GET_INT(r_voxel_size);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glViewport(0, 0, voxel_size, voxel_size);

    // @TODO: fix
    constexpr int IMAGE_VOXEL_ALBEDO_SLOT = 0;
    constexpr int IMAGE_VOXEL_NORMAL_SLOT = 1;
    g_albedoVoxel.bindImageTexture(IMAGE_VOXEL_ALBEDO_SLOT);
    g_normalVoxel.bindImageTexture(IMAGE_VOXEL_NORMAL_SLOT);
    ShaderProgramManager::get(PROGRAM_VOXELIZATION).bind();

    auto render_data = GraphicsManager::singleton().get_render_data();
    RenderData::Pass& pass = render_data->voxel_pass;

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *render_data->scene->get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= SC_BONE_MAX);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        g_perBatchCache.cache.c_projection_view_model_matrix = pass.projection_view_matrix * draw.world_matrix;
        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.Update();

        glBindVertexArray(draw.mesh_data->vao);

        for (const auto& subset : draw.subsets) {
            FillMaterialCB(subset.material_data, g_materialCache.cache);
            g_materialCache.Update();

            glDrawElements(GL_TRIANGLES, subset.index_count, GL_UNSIGNED_INT, (void*)(subset.index_offset * sizeof(uint32_t)));
        }
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // post process
    ShaderProgramManager::get(PROGRAM_VOXELIZATION_POST).bind();

    constexpr GLuint workGroupX = 512;
    constexpr GLuint workGroupY = 512;
    const GLuint workGroupZ = (voxel_size * voxel_size * voxel_size) / (workGroupX * workGroupY);

    glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    g_albedoVoxel.bind();
    g_albedoVoxel.genMipMap();
    g_normalVoxel.bind();
    g_normalVoxel.genMipMap();
}

void gbuffer_pass_func(int) {
    auto [frameW, frameH] = DisplayServer::singleton().get_frame_size();
    glViewport(0, 0, frameW, frameH);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto render_data = GraphicsManager::singleton().get_render_data();
    RenderData::Pass& pass = render_data->main_pass;

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.armature_id.is_valid();

        if (has_bone) {
            auto& armature = *render_data->scene->get_component<ArmatureComponent>(draw.armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= SC_BONE_MAX);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.Update();
        }

        const auto& program = ShaderProgramManager::get(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);
        program.bind();

        g_perBatchCache.cache.c_projection_view_model_matrix = pass.projection_view_matrix * draw.world_matrix;
        g_perBatchCache.cache.c_model_matrix = draw.world_matrix;
        g_perBatchCache.Update();

        glBindVertexArray(draw.mesh_data->vao);

        for (const auto& subset : draw.subsets) {
            FillMaterialCB(subset.material_data, g_materialCache.cache);
            g_materialCache.Update();

            glDrawElements(GL_TRIANGLES, subset.index_count, GL_UNSIGNED_INT, (void*)(subset.index_offset * sizeof(uint32_t)));
        }
    }

    glUseProgram(0);
}

void ssao_pass_func(int) {
    auto [frameW, frameH] = DisplayServer::singleton().get_frame_size();
    glViewport(0, 0, frameW, frameH);

    const auto& shader = ShaderProgramManager::get(PROGRAM_SSAO);

    glClear(GL_COLOR_BUFFER_BIT);

    shader.bind();

    R_DrawQuad();

    shader.unbind();
}

void lighting_pass_func(int) {
    auto [frameW, frameH] = DisplayServer::singleton().get_frame_size();
    glViewport(0, 0, frameW, frameH);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto& program = ShaderProgramManager::get(PROGRAM_LIGHTING_VXGI);
    program.bind();
    R_DrawQuad();
    program.unbind();
}

void fxaa_pass_func(int) {
    auto [frameW, frameH] = DisplayServer::singleton().get_frame_size();
    glViewport(0, 0, frameW, frameH);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto& program = ShaderProgramManager::get(PROGRAM_FXAA);
    program.bind();
    R_DrawQuad();
    program.unbind();
}

void final_pass_func(int) {
    auto [frameW, frameH] = DisplayServer::singleton().get_frame_size();
    glClearColor(.1f, .1f, .1f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto& program = ShaderProgramManager::get(PROGRAM_FINAL_IMAGE);

    program.bind();
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, frameW, frameH);

    R_DrawQuad();

    program.unbind();
}

void debug_vxgi_pass_func(int) {
    auto [width, height] = my::DisplayServer::singleton().get_frame_size();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // @TODO: stop using window width and height
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);

    const auto& program = my::ShaderProgramManager::get(my::PROGRAM_DEBUG_VOXEL);
    program.bind();

    glBindVertexArray(g_box.vao);

    const int size = DVAR_GET_INT(r_voxel_size);
    glDrawElementsInstanced(GL_TRIANGLES, g_box.count, GL_UNSIGNED_INT, 0, size * size * size);

    program.unbind();
}

void create_render_graph_vxgi(RenderGraph& graph) {
    auto [w, h] = DisplayServer::singleton().get_frame_size();
    const int shadow_res = DVAR_GET_INT(r_shadow_res);
    int num_cascades = 4;
    DEV_ASSERT(math::is_power_of_two(shadow_res));

    auto gbuffer_attachment0 = graph.create_resource(ResourceDesc{ RT_RES_GBUFFER_POSITION,
                                                                   FORMAT_R16G16B16A16_FLOAT,
                                                                   RT_COLOR_ATTACHMENT,
                                                                   w, h });
    auto gbuffer_attachment1 = graph.create_resource(ResourceDesc{ RT_RES_GBUFFER_NORMAL,
                                                                   FORMAT_R16G16B16A16_FLOAT,
                                                                   RT_COLOR_ATTACHMENT,
                                                                   w, h });
    auto gbuffer_attachment2 = graph.create_resource(ResourceDesc{ RT_RES_GBUFFER_BASE_COLOR,
                                                                   FORMAT_R8G8B8A8_UINT,
                                                                   RT_COLOR_ATTACHMENT,
                                                                   w, h });
    auto gbuffer_depth = graph.create_resource(ResourceDesc{ RT_RES_GBUFFER_DEPTH,
                                                             FORMAT_D32_FLOAT,
                                                             RT_DEPTH_ATTACHMENT,
                                                             w, h });
    auto ssao_attachment = graph.create_resource(ResourceDesc{ RT_RES_SSAO,
                                                               FORMAT_R32_FLOAT,
                                                               RT_COLOR_ATTACHMENT,
                                                               w, h });
    auto lighting_attachment = graph.create_resource(ResourceDesc{ RT_RES_LIGHTING,
                                                                   FORMAT_R8G8B8A8_UINT,
                                                                   RT_COLOR_ATTACHMENT,
                                                                   w, h });
    auto fxaa_attachment = graph.create_resource(ResourceDesc{ RT_RES_FXAA,
                                                               FORMAT_R8G8B8A8_UINT,
                                                               RT_COLOR_ATTACHMENT,
                                                               w, h });
    // auto view_attachment = graph.create_resource(ResourceDesc{ RT_RES_FINAL_IMAGE,
    //                                                            FORMAT_R8G8B8A8_UINT,
    //                                                            RT_COLOR_ATTACHMENT,
    //                                                            w, h });

    std::vector<std::string> shadow_passes;
    {  // shadow pass
        for (int i = 0; i < num_cascades; ++i) {
            auto shadow_map = graph.create_resource(ResourceDesc{ RT_RES_SHADOW_MAP + std::to_string(i),
                                                                  FORMAT_D32_FLOAT,
                                                                  RT_SHADOW_MAP,
                                                                  shadow_res, shadow_res });
            RenderPassDesc desc;
            desc.name = SHADOW_PASS + std::to_string(i);
            shadow_passes.push_back(desc.name);
            desc.depth_attachment = shadow_map;
            desc.func = shadow_pass_func;
            desc.layer = i;
            graph.add_pass(desc);
        }
    }
    {  // gbuffer pass
        RenderPassDesc desc;
        desc.name = GBUFFER_PASS;
        desc.dependencies = {};
        desc.color_attachments = { gbuffer_attachment0, gbuffer_attachment1, gbuffer_attachment2 };
        desc.depth_attachment = gbuffer_depth;
        desc.func = gbuffer_pass_func;
        graph.add_pass(desc);
    }
    {  // voxel pass
        RenderPassDesc desc;
        desc.type = RENDER_PASS_COMPUTE;
        desc.name = VOXELIZATION_PASS;
        desc.dependencies = shadow_passes;
        desc.func = voxelization_pass_func;
        graph.add_pass(desc);
    }
    {  // ssao pass
        RenderPassDesc desc;
        desc.name = SSAO_PASS;
        desc.dependencies = { GBUFFER_PASS };
        desc.color_attachments = { ssao_attachment };
        desc.func = ssao_pass_func;
        graph.add_pass(desc);
    }
    {  // lighting pass
        RenderPassDesc desc;
        desc.name = LIGHTING_PASS;
        desc.dependencies = shadow_passes;
        desc.dependencies.push_back(GBUFFER_PASS);
        desc.dependencies.push_back(SSAO_PASS);
        desc.dependencies.push_back(VOXELIZATION_PASS);
        desc.color_attachments = { lighting_attachment };
        desc.func = lighting_pass_func;
        graph.add_pass(desc);
    }
    {  // fxaa pass
        RenderPassDesc desc;
        desc.name = FXAA_PASS;
        desc.dependencies = { LIGHTING_PASS };
        desc.color_attachments = { fxaa_attachment };
        desc.func = fxaa_pass_func;
        graph.add_pass(desc);
    }
    //{  // viewer pass(final pass)
    //    RenderPassDesc desc;
    //    desc.name = FINAL_PASS;
    //    desc.dependencies = { FXAA_PASS };
    //    desc.color_attachments = { view_attachment };
    //    desc.func = final_pass_func;

    //    graph.add_pass(desc);
    //}

    // @TODO: allow recompile
    graph.compile();
}

void create_render_graph_vxgi_debug(RenderGraph& graph) {
    unused(graph);
#if 0
    // @TODO: split resource
    auto [w, h] = DisplayServer::singleton().get_frame_size();

    const int res = DVAR_GET_INT(r_shadow_res);
    DEV_ASSERT(math::is_power_of_two(res));

    {  // shadow pass
        RenderPassDesc desc;
        desc.name = SHADOW_PASS;
        desc.depth_attachment = ResourceDesc{ SHADOW_PASS_OUTPUT, FORMAT_D32_FLOAT, RT_SHADDW_MAP_ARRAY };
        desc.func = shadow_pass_func;
        desc.width = res;
        desc.height = res;

        graph.add_pass(desc);
    }
    {  // voxel pass
        RenderPassDesc desc;
        desc.type = RENDER_PASS_COMPUTE;
        desc.name = VOXELIZATION_PASS;
        desc.dependencies = { SHADOW_PASS };
        desc.func = voxelization_pass_func;

        graph.add_pass(desc);
    }
    {  // vxgi debug pass
        RenderPassDesc desc;
        desc.name = VXGI_DEBUG_PASS;
        desc.dependencies = { SHADOW_PASS, VOXELIZATION_PASS };
        desc.color_attachments = { ResourceDesc{ LIGHTING_PASS_OUTPUT, FORMAT_R8G8B8A8_UINT, RT_COLOR_ATTACHMENT } };
        desc.depth_attachment = { ResourceDesc{ "depth", FORMAT_D32_FLOAT, RT_DEPTH_ATTACHMENT } };
        desc.func = debug_vxgi_pass_func;
        desc.width = w;
        desc.height = h;

        graph.add_pass(desc);
    }
    {  // fxaa pass
        RenderPassDesc desc;
        desc.name = FXAA_PASS;
        desc.dependencies = { VXGI_DEBUG_PASS };
        desc.color_attachments = { ResourceDesc{ FXAA_PASS_OUTPUT, FORMAT_R8G8B8A8_UINT, RT_COLOR_ATTACHMENT } };
        desc.func = fxaa_pass_func;
        desc.width = w;
        desc.height = h;

        graph.add_pass(desc);
    }

    // @TODO: allow recompile
    graph.compile();
#endif
}

}  // namespace my::rg
