local geomath = require('geomath')
local scene_helper = require('scene-helper')

local Vector3 = geomath.Vector3

-- Renderer.set_env_map('@res://env/sky.hdr')

local scene = Scene.get()

scene_helper.create_point_light(scene, 'light1', Vector3:new(0, 3, 0))

-- create walls

local s = 4;
local scale = Vector3:new(s, s, s)
function create_wall(p_name, p_translate, p_rotate, p_color)
    local desc = scene_helper.build_plane_desc(p_name, Vector3.ZERO, scale)
    p_color = p_color or Vector3.ONE
    desc.transform = {
        translate = p_translate,
        rotate = p_rotate,
    }

    local metallic = 0.01
    local roughness = 0.99
    local material = scene_helper.build_material_desc(p_color, metallic, roughness)
    desc.material = material
    return scene:create_entity(desc)
end

function create_sphere(p_position, p_scalar)
    local desc = scene_helper.build_sphere_desc('Sphere', p_position, p_scalar)
    local material = scene_helper.build_material_desc(Vector3.ONE, 0.7, 0.3)
    desc.material = material
    return scene:create_entity(desc)
end

function create_cube(p_position, p_scale)
    local desc = scene_helper.build_cube_desc('Cube', p_position, p_scale)
    -- local material = scene_helper.build_material_desc(Vector3.ONE, 0.9, 0.1)
    -- desc.material = material
    return scene:create_entity(desc)
end

create_sphere(Vector3:new(-2, -1, 0), 1.0)
create_cube(Vector3:new(2, -2, 0), Vector3:new(1, 2, 1))
create_cube(Vector3:new(-2, -3, 0), Vector3:new(1, 1, 1))

create_wall('back-wall', Vector3:new(0, 0, -s), Vector3.ZERO, Vector3.UNIT_Z)
create_wall('front-wall', Vector3:new(0, 0, -s), Vector3:new(0, 180, 0))
create_wall('left-wall', Vector3:new(0, 0, -s), Vector3:new(0, 90, 0), Vector3.UNIT_X) -- left
create_wall('right-wall', Vector3:new(0, 0, -s), Vector3:new(0, -90, 0), Vector3.UNIT_Y) -- right
create_wall('up-wall', Vector3:new(0, 0, -s), Vector3:new(90, 0, 0))
create_wall('down-wall', Vector3:new(0, 0, -s), Vector3:new(-90, 0, 0))