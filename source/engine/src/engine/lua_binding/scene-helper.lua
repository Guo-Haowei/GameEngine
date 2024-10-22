local geomath = require('geomath')
local Vector3 = geomath.Vector3

local scene_helper = {}

--[[
local transform = {
    translate = Vector3.ZERO,
    rotate = Vector3.ZERO,
    scale = Vector3.ONE,
}
local rigid_body = {
    shape = "SHAPE_CUBE",
    size = Vector3.HALF,
    mass = 1.0,
}
local material = {
    base_color = Vector3.ONE,
    metallic = 0.0,
    roughness = 1.0,
}
--]]

function scene_helper.create_omni_light(p_scene, p_name, p_rotation)
    return p_scene:create_entity({
        type = 'OMNI_LIGHT',
        name = p_name,
        transform = { rotate = p_rotation },
    })
end

function scene_helper.create_point_light(p_scene, p_name, p_position)
    return p_scene:create_entity({
        type = 'POINT_LIGHT',
        name = p_name,
        transform = { translate = p_position },
    })
end

function scene_helper.build_material_desc(p_base_color, p_metallic, p_roughness)
    p_base_color = p_base_color or Vector3.ONE
    p_metallic = p_metallic or 0
    p_roughness = p_roughness or 0.9
    return {
        base_color = p_base_color,
        metallic = geomath.clamp(p_metallic, 0.0, 1.0),
        roughness = geomath.clamp(p_roughness, 0.05, 1.0),
    }
end

function scene_helper.build_sphere_desc(p_name, p_position, p_radius, p_mass)
    p_position = p_position or Vector3.ZERO
    p_radius = p_radius or 0.5
    local desc = {
        name = p_name,
        sphere = { radius = p_radius },
        transform = { translate = p_position },
    }
    if p_mass ~= nil then
        desc.rigid_body = {
            shape = 'SHAPE_SPHERE',
            radius = p_radius,
            mass = p_mass,
        }
    end
    return desc
end

function scene_helper.build_cube_desc(p_name, p_position, p_size, p_mass)
    p_position = p_position or Vector3.ZERO
    p_size = p_size or Vector3.HALF
    local desc = {
        name = p_name,
        cube = { size = p_size },
        transform = { translate = p_position },
    }
    if p_mass ~= nil then
        desc.rigid_body = {
            shape = 'SHAPE_CUBE',
            size = p_size,
            mass = p_mass,
        }
    end
    return desc
end

function scene_helper.build_plane_desc(p_name, p_position, p_size, p_mass)
    p_position = p_position or Vector3.ZERO
    p_size = p_size or Vector3.HALF
    local desc = {
        name = p_name,
        plane = { size = p_size },
        transform = { translate = p_position },
    }
    if p_mass ~= nil then
        desc.rigid_body = {
            shape = 'SHAPE_PLANE',
            size = p_size,
            mass = p_mass,
        }
    end
    return desc
end

function scene_helper.build_cornell(p_scale, p_config)
    local scale = Vector3:new(p_scale, p_scale, p_scale)
    function create_wall(p_name, p_translate, p_rotation, p_color)
        local desc = scene_helper.build_plane_desc(p_name, Vector3.ZERO, scale)
        p_color = p_color or Vector3.ONE
        desc.transform = {
            translate = p_translate,
            rotate = p_rotation,
        }

        local metallic = 0.01
        local roughness = 0.99
        local material = scene_helper.build_material_desc(p_color, metallic, roughness)
        desc.material = material
        local scene = Scene.get()
        return scene:create_entity(desc)
    end

    local config = p_config and p_config or {}

    if not config.no_back then
        create_wall('back-wall', Vector3:new(0, 0, -p_scale), Vector3.ZERO, Vector3.UNIT_Z)
    end
    if not config.no_front then
        create_wall('front-wall', Vector3:new(0, 0, -p_scale), Vector3:new(0, 180, 0))
    end
    if not config.no_left then
        create_wall('left-wall', Vector3:new(0, 0, -p_scale), Vector3:new(0, 90, 0), Vector3.UNIT_X)
    end
    if not config.no_right then
        create_wall('right-wall', Vector3:new(0, 0, -p_scale), Vector3:new(0, -90, 0), Vector3.UNIT_Y)
    end
    if not config.no_top then
        create_wall('top-wall', Vector3:new(0, 0, -p_scale), Vector3:new(90, 0, 0))
    end
    if not config.no_bottom then
        create_wall('bottom-wall', Vector3:new(0, 0, -p_scale), Vector3:new(-90, 0, 0))
    end

    return
end

return scene_helper
