local geomath = require('geomath')
local Vector3 = geomath.Vector3

local scene_helper = {}

function scene_helper.create_point_light(p_scene, p_name, p_position)
    Scene.create_entity(p_scene, {
        type = 'POINT_LIGHT',
        name = p_name,
        transform = { translate = p_position },
    })
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
    return desc;
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
    return desc;
end

return scene_helper
