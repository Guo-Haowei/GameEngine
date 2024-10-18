#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

static void pointShadowPassFunc(const Subpass* p_subpass, int p_pass_id) {
    OPTICK_EVENT();

    auto& manager = GraphicsManager::singleton();
    auto render_data = manager.getRenderData();

    auto& pass_ptr = render_data->point_shadow_passes[p_pass_id];
    if (!pass_ptr) {
        return;
    }

    // prepare render data
    auto [width, height] = p_subpass->depth_attachment->getSize();

    // @TODO: instead of render the same object 6 times
    // set up different object list for different pass
    const RenderData::Pass& pass = *pass_ptr.get();

    const auto& light_matrices = pass.light_component.getMatrices();
    for (int i = 0; i < 6; ++i) {
        g_per_pass_cache.cache.u_proj_view_matrix = light_matrices[i];
        g_per_pass_cache.cache.u_point_light_position = pass.light_component.getPosition();
        g_per_pass_cache.cache.u_point_light_far = pass.light_component.getMaxDistance();
        g_per_pass_cache.update();

        manager.setRenderTarget(p_subpass, i);
        manager.clear(p_subpass, CLEAR_DEPTH_BIT);

        Viewport viewport{ width, height };
        manager.setViewport(viewport);

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                manager.bindUniformSlot<BoneConstantBuffer>(render_data->m_bone_uniform.get(), draw.bone_idx);
            }

            manager.setPipelineState(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

            manager.bindUniformSlot<PerBatchConstantBuffer>(render_data->m_batch_uniform.get(), draw.batch_idx);

            manager.setMesh(draw.mesh_data);
            manager.drawElements(draw.mesh_data->index_count);
        }
    }
}

static void shadowPassFunc(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto render_data = GraphicsManager::singleton().getRenderData();
    if (!render_data->has_sun_light) {
        return;
    }

    auto& manager = GraphicsManager::singleton();
    manager.setRenderTarget(p_subpass);
    auto [width, height] = p_subpass->depth_attachment->getSize();

    manager.clear(p_subpass, CLEAR_DEPTH_BIT);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](const std::string& name, int slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = manager.findRenderTarget(name);
        if (!resource) {
            return;
        }

        manager.bindTexture(p_dimension, resource->texture->get_handle(), slot);
    };

    bind_slot(RT_RES_SHADOW_MAP, u_shadow_map_slot);

    const int actual_width = width / MAX_CASCADE_COUNT;

    for (int cascade_idx = 0; cascade_idx < MAX_CASCADE_COUNT; ++cascade_idx) {
        Viewport viewport{ actual_width, height };
        viewport.top_left_x = cascade_idx * actual_width;
        manager.setViewport(viewport);

        RenderData::Pass& pass = render_data->shadow_passes[cascade_idx];
        pass.fillPerpass(g_per_pass_cache.cache);
        g_per_pass_cache.update();

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                manager.bindUniformSlot<BoneConstantBuffer>(render_data->m_bone_uniform.get(), draw.bone_idx);
            }

            // @TODO: sort the objects so there's no need to switch pipeline
            manager.setPipelineState(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

            manager.bindUniformSlot<PerBatchConstantBuffer>(render_data->m_batch_uniform.get(), draw.batch_idx);

            manager.setMesh(draw.mesh_data);
            manager.drawElements(draw.mesh_data->index_count);
        }
    }

    // manager.unbindTexture(Dimension::TEXTURE_2D, u_shadow_map_slot);
}

void RenderPassCreator::addShadowPass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    const int shadow_res = DVAR_GET_INT(r_shadow_res);
    DEV_ASSERT(math::isPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
    DEV_ASSERT(math::isPowerOfTwo(point_shadow_res));

    auto shadow_map = manager.createRenderTarget(RenderTargetDesc{ RT_RES_SHADOW_MAP,
                                                                   PixelFormat::D32_FLOAT,
                                                                   AttachmentType::SHADOW_2D,
                                                                   MAX_CASCADE_COUNT * shadow_res, shadow_res },
                                                 shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = SHADOW_PASS;
    auto pass = m_graph.createPass(desc);

    // @TODO: refactor
    SubPassFunc funcs[] = {
        [](const Subpass* p_subpass) {
            pointShadowPassFunc(p_subpass, 0);
        },
        [](const Subpass* p_subpass) {
            pointShadowPassFunc(p_subpass, 1);
        },
        [](const Subpass* p_subpass) {
            pointShadowPassFunc(p_subpass, 2);
        },
        [](const Subpass* p_subpass) {
            pointShadowPassFunc(p_subpass, 3);
        },
    };

    static_assert(array_length(funcs) == MAX_LIGHT_CAST_SHADOW_COUNT);

    if (manager.getBackend() == Backend::OPENGL) {
        for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
            auto point_shadow_map = manager.createRenderTarget(RenderTargetDesc{ RT_RES_POINT_SHADOW_MAP + std::to_string(i),
                                                                                 PixelFormat::D32_FLOAT,
                                                                                 AttachmentType::SHADOW_CUBE_MAP,
                                                                                 point_shadow_res, point_shadow_res },
                                                               shadow_cube_map_sampler());

            auto subpass = manager.createSubpass(SubpassDesc{
                .depth_attachment = point_shadow_map,
                .func = funcs[i],
            });
            pass->addSubpass(subpass);
        }
    }

    auto subpass = manager.createSubpass(SubpassDesc{
        .depth_attachment = shadow_map,
        .func = shadowPassFunc,
    });
    pass->addSubpass(subpass);
}

}  // namespace my::rg
