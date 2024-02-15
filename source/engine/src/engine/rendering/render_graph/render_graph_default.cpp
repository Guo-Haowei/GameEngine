#include "render_graph_default.h"

// @TODO: refactor
#include "core/base/rid_owner.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_data.h"
#include "rendering/rendering_dvars.h"
#include "rendering/shader_program_manager.h"
#include "servers/display_server.h"

extern void FillMaterialCB(const MaterialData* mat, MaterialConstantBuffer& cb);

extern my::RIDAllocator<MeshData> g_meshes;
extern my::RIDAllocator<MaterialData> g_materials;

namespace my {

void create_render_graph_default(RenderGraph& graph) {
    auto [w, h] = DisplayServer::singleton().get_frame_size();

    const int res = DVAR_GET_INT(r_shadow_res);
    DEV_ASSERT(math::is_power_of_two(res));

    // @TODO: split resource
    {  // shadow pass
        RenderPassDesc desc;
        desc.name = SHADOW_PASS;
        desc.depth_attachment = RenderTargetDesc{ SHADOW_PASS_OUTPUT, FORMAT_D32_FLOAT };
        desc.func = shadow_pass_func;
        desc.width = res;
        desc.height = res;

        graph.add_pass(desc);
    }
    {  // gbuffer pass
        RenderPassDesc desc;
        desc.name = GBUFFER_PASS;
        desc.dependencies = {};
        desc.color_attachments = {
            RenderTargetDesc{ GBUFFER_PASS_OUTPUT_POSITION, FORMAT_R16G16B16A16_FLOAT },
            RenderTargetDesc{ GBUFFER_PASS_OUTPUT_NORMAL, FORMAT_R16G16B16A16_FLOAT },
            RenderTargetDesc{ GBUFFER_PASS_OUTPUT_ALBEDO, FORMAT_R8G8B8A8_UINT },
        };
        desc.depth_attachment = RenderTargetDesc{ GBUFFER_PASS_OUTPUT_DEPTH, FORMAT_D32_FLOAT };
        desc.func = gbuffer_pass_func;
        desc.width = w;
        desc.height = h;

        graph.add_pass(desc);
    }
    {  // ssao pass
        RenderPassDesc desc;
        desc.name = SSAO_PASS;
        desc.dependencies = { GBUFFER_PASS };
        desc.color_attachments = { RenderTargetDesc{ SSAO_PASS_OUTPUT, FORMAT_R32_FLOAT } };
        desc.func = ssao_pass_func;
        desc.width = w;
        desc.height = h;

        graph.add_pass(desc);
    }
    {  // lighting pass
        RenderPassDesc desc;
        desc.name = LIGHTING_PASS;
        desc.dependencies = { GBUFFER_PASS, SHADOW_PASS, SSAO_PASS };
        desc.color_attachments = { RenderTargetDesc{ LIGHTING_PASS_OUTPUT, FORMAT_R8G8B8A8_UINT } };
        desc.func = lighting_pass_func;
        desc.width = w;
        desc.height = h;

        graph.add_pass(desc);
    }
    {  // viewer pass(final pass)
        RenderPassDesc desc;
        desc.name = FINAL_PASS;
        desc.dependencies = { LIGHTING_PASS };
        desc.color_attachments = { RenderTargetDesc{ "viewer_map", FORMAT_R8G8B8A8_UINT } };
        desc.width = w;
        desc.height = h;
        desc.func = final_pass_func;

        graph.add_pass(desc);
    }

    // @TODO: allow recompile
    graph.compile();
}

}  // namespace my
