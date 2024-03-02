#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/rendering_dvars.h"

// @TODO: remove API sepcific code
#include "drivers/opengl/opengl_prerequisites.h"

namespace my::rg {

static void point_shadow_pass_func(const Subpass* p_subpass, int p_pass_id) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto render_data = gm.get_render_data();

    auto& pass_ptr = render_data->point_shadow_passes[p_pass_id];
    if (!pass_ptr) {
        return;
    }

    // prepare render data
    auto [width, height] = p_subpass->depth_attachment->get_size();

    // @TODO: fix this
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    // @TODO: instead of render the same object 6 times
    // set up different object list for different pass
    const RenderData::Pass& pass = *pass_ptr.get();

    const auto& light_matrices = pass.light_component.get_matrices();
    for (int i = 0; i < 6; ++i) {
        g_per_pass_cache.cache.u_proj_view_matrix = light_matrices[i];
        g_per_pass_cache.cache.u_point_light_position = pass.light_component.get_position();
        g_per_pass_cache.cache.u_point_light_far = pass.light_component.get_max_distance();
        g_per_pass_cache.update();

        gm.set_render_target(p_subpass, i);
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.uniform_bind_slot<BoneConstantBuffer>(render_data->m_bone_uniform.get(), draw.bone_idx);
            }

            gm.set_pipeline_state(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

            gm.uniform_bind_slot<PerBatchConstantBuffer>(render_data->m_batch_uniform.get(), draw.batch_idx);

            gm.set_mesh(draw.mesh_data);
            gm.draw_elements(draw.mesh_data->index_count);
        }
    }
}

static void shadow_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    gm.set_render_target(p_subpass);
    auto [width, height] = p_subpass->depth_attachment->get_size();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glClear(GL_DEPTH_BUFFER_BIT);

    int actual_width = width / MAX_CASCADE_COUNT;
    auto render_data = GraphicsManager::singleton().get_render_data();

    for (int cascade_idx = 0; cascade_idx < MAX_CASCADE_COUNT; ++cascade_idx) {
        glViewport(cascade_idx * actual_width, 0, actual_width, height);

        RenderData::Pass& pass = render_data->shadow_passes[cascade_idx];
        pass.fill_perpass(g_per_pass_cache.cache);
        g_per_pass_cache.update();

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.uniform_bind_slot<BoneConstantBuffer>(render_data->m_bone_uniform.get(), draw.bone_idx);
            }

            gm.set_pipeline_state(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

            gm.uniform_bind_slot<PerBatchConstantBuffer>(render_data->m_batch_uniform.get(), draw.batch_idx);

            gm.set_mesh(draw.mesh_data);
            gm.draw_elements(draw.mesh_data->index_count);
        }

        glCullFace(GL_BACK);
        glUseProgram(0);
    }
}

void create_shadow_pass(RenderGraph& p_graph) {
    GraphicsManager& manager = GraphicsManager::singleton();

    const int shadow_res = DVAR_GET_INT(r_shadow_res);
    DEV_ASSERT(math::is_power_of_two(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
    DEV_ASSERT(math::is_power_of_two(point_shadow_res));

    auto shadow_map = manager.create_render_target(RenderTargetDesc{ RT_RES_SHADOW_MAP,
                                                                     PixelFormat::D32_FLOAT,
                                                                     AttachmentType::SHADOW_2D,
                                                                     MAX_CASCADE_COUNT * shadow_res, shadow_res },
                                                   shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = SHADOW_PASS;
    auto pass = p_graph.create_pass(desc);

    // @TODO: refactor
    SubPassFunc funcs[] = {
        [](const Subpass* p_subpass) {
            point_shadow_pass_func(p_subpass, 0);
        },
        [](const Subpass* p_subpass) {
            point_shadow_pass_func(p_subpass, 1);
        },
        [](const Subpass* p_subpass) {
            point_shadow_pass_func(p_subpass, 2);
        },
        [](const Subpass* p_subpass) {
            point_shadow_pass_func(p_subpass, 3);
        },
    };

    static_assert(array_length(funcs) == MAX_LIGHT_CAST_SHADOW_COUNT);

    for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
        auto point_shadow_map = manager.create_render_target(RenderTargetDesc{ RT_RES_POINT_SHADOW_MAP + std::to_string(i),
                                                                               PixelFormat::D32_FLOAT,
                                                                               AttachmentType::SHADOW_CUBE_MAP,
                                                                               point_shadow_res, point_shadow_res },
                                                             shadow_cube_map_sampler());

        auto subpass = manager.create_subpass(SubpassDesc{
            .depth_attachment = point_shadow_map,
            .func = funcs[i],
        });
        pass->add_sub_pass(subpass);
    }

    auto subpass = manager.create_subpass(SubpassDesc{
        .depth_attachment = shadow_map,
        .func = shadow_pass_func,
    });
    pass->add_sub_pass(subpass);
}

}  // namespace my::rg
