#include "render_graphs.h"

// @TODO: refactor
#include "core/base/rid_owner.h"
#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "rendering/GpuTexture.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_data.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

void base_color_pass(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    gm.set_render_target(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto depth_buffer = p_subpass->depth_attachment;
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    glViewport(0, 0, width, height);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto render_data = GraphicsManager::singleton().get_render_data();
    RenderData::Pass& pass = render_data->main_pass;

    pass.fill_perpass(g_per_pass_cache.cache);
    g_per_pass_cache.update();

    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.tmp_armature_id.is_valid();

        if (has_bone) {
            auto& armature = *render_data->scene->get_component<ArmatureComponent>(draw.tmp_armature_id);
            DEV_ASSERT(armature.bone_transforms.size() <= MAX_BONE_COUNT);

            memcpy(g_boneCache.cache.c_bones, armature.bone_transforms.data(), sizeof(mat4) * armature.bone_transforms.size());
            g_boneCache.update();
        }

        gm.set_pipeline_state(has_bone ? PROGRAM_BASE_COLOR_ANIMATED : PROGRAM_BASE_COLOR_STATIC);

        g_per_batch_uniform.cache = render_data->m_batch_buffers[draw.batch_buffer_id];
        g_per_batch_uniform.update();

        gm.set_mesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            GraphicsManager::singleton().fill_material_constant_buffer(subset.material, g_materialCache.cache);
            g_materialCache.update();

            gm.draw_elements(subset.index_count, subset.index_offset);
        }
    }
}

void create_render_graph_base_color(RenderGraph& graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    GraphicsManager& manager = GraphicsManager::singleton();

    auto color_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_BASE_COLOR,
                                                                           PixelFormat::R8G8B8A8_UINT,
                                                                           AttachmentType::COLOR_2D,
                                                                           w, h },
                                                         nearest_sampler());
    auto depth_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_BASE_COLOR_DEPTH,
                                                                           PixelFormat::D32_FLOAT,
                                                                           AttachmentType::DEPTH_2D,
                                                                           w, h },
                                                         nearest_sampler());

    {  // base color
        RenderPassDesc desc;
        desc.name = BASE_COLOR_PASS;
        auto pass = graph.create_pass(desc);
        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { color_attachment },
            .depth_attachment = depth_attachment,
            .func = base_color_pass,
        });
        pass->add_sub_pass(subpass);
    }

    graph.compile();
}

}  // namespace my::rg
