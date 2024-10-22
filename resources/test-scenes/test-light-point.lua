local geomath = require('geomath')
local scene_helper = require('scene-helper')

local Vector3 = geomath.Vector3

local scene = Scene.get()

scene_helper.create_point_light(scene, 'light1', Vector3:new(0, 3, 0))

-- create walls

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

scene_helper.build_cornell(4, {
    no_top = true
})
