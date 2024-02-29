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
return scene_helper
