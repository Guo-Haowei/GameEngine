#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

static void pointShadowPassFunc(const DrawPass* p_subpass, int p_pass_id) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto& ctx = gm.getContext();

    auto& pass_ptr = gm.point_shadow_passes[p_pass_id];
    if (!pass_ptr) {
        return;
    }

    // prepare render data
    auto [width, height] = p_subpass->depth_attachment->getSize();

    // @TODO: instead of render the same object 6 times
    // set up different object list for different pass
    const PassContext& pass = *pass_ptr.get();

    const auto& light_matrices = pass.light_component.getMatrices();
    for (int i = 0; i < 6; ++i) {
        g_point_shadow_cache.cache.g_point_light_matrix = light_matrices[i];
        g_point_shadow_cache.cache.g_point_light_position = pass.light_component.getPosition();
        g_point_shadow_cache.cache.g_point_light_far = pass.light_component.getMaxDistance();
        g_point_shadow_cache.update();

        gm.setRenderTarget(p_subpass, i);
        gm.clear(p_subpass, CLEAR_DEPTH_BIT);

        Viewport viewport{ width, height };
        gm.setViewport(viewport);

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.bindUniformSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
            }

            gm.setPipelineState(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

            gm.bindUniformSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

            gm.setMesh(draw.mesh_data);
            gm.drawElements(draw.mesh_data->index_count);
        }
    }
}

static void shadowPassFunc(const DrawPass* p_subpass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto& ctx = gm.getContext();

    gm.setRenderTarget(p_subpass);
    auto [width, height] = p_subpass->depth_attachment->getSize();

    gm.clear(p_subpass, CLEAR_DEPTH_BIT);

    Viewport viewport{ width, height };
    gm.setViewport(viewport);

    PassContext& pass = gm.shadow_passes[0];
    gm.bindUniformSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.bindUniformSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        // @TODO: sort the objects so there's no need to switch pipeline
        gm.setPipelineState(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

        gm.bindUniformSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.setMesh(draw.mesh_data);
        gm.drawElements(draw.mesh_data->index_count);
    }

    gm.unsetRenderTarget();
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
                                                                   1 * shadow_res, shadow_res },
                                                 shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = SHADOW_PASS;
    auto pass = m_graph.createPass(desc);
    {
        auto draw_pass = manager.createDrawPass(DrawPassDesc{
            .depth_attachment = shadow_map,
            .exec_func = shadowPassFunc,
        });
        pass->addDrawPass(draw_pass);
    }

    // @TODO: refactor
    DrawPassExecuteFunc funcs[] = {
        [](const DrawPass* p_subpass) {
            pointShadowPassFunc(p_subpass, 0);
        },
        [](const DrawPass* p_subpass) {
            pointShadowPassFunc(p_subpass, 1);
        },
        [](const DrawPass* p_subpass) {
            pointShadowPassFunc(p_subpass, 2);
        },
        [](const DrawPass* p_subpass) {
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

            auto subpass = manager.createDrawPass(DrawPassDesc{
                .depth_attachment = point_shadow_map,
                .exec_func = funcs[i],
            });
            pass->addDrawPass(subpass);
        }
    }
}

}  // namespace my::rg
