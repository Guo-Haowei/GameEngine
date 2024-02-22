local Vector3 = require('vector3')


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
--]]

-- TODO: move to common place
function create_point_light(p_scene, p_name, p_position)
    Scene.create_entity(p_scene, {
        type = 'POINT_LIGHT',
        name = p_name,
        transform = { translate = p_position },
    })
end

function build_sphere_desc(p_name, p_position, p_radius, p_mass)
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

function build_cube_desc(p_name, p_position, p_size, p_mass)
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

local scene = Scene.get()

-- create point light
create_point_light(scene, 'PointLight', Vector3:create(0, 4, 0))

-- create ground
local ground_size = Vector3:create(6, 0.03, 6);
Scene.create_entity(scene, build_cube_desc('Ground', Vector3.ZERO, ground_size, 0.0))

-- create objects
for t = 1, 21 do
    local x = (t - 1) % 7
    local y = math.floor((t - 1) / 7)

    local translate = Vector3:create(x - 3, 5 - y, 0)
    local s = 0.25
    local size = Vector3:create(s, s, s)

    if t % 2 == 0 then
        Scene.create_entity(scene, build_cube_desc('Cube' .. t, translate, size, 1.0))
    else
        Scene.create_entity(scene, build_sphere_desc('Sphere' .. t, translate, s, 1.0))
    end
end

return true
