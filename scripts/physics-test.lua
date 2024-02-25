local geomath = require('geomath')
local scene_helper = require('scene-helper')
local Vector3 = geomath.Vector3

Renderer.set_env_map('@res://env/sky.hdr')

local scene = Scene.get()

-- create point light
scene_helper.create_point_light(scene, 'PointLight', Vector3:new(0, 4, 0))

-- create ground
local transform = scene:create_entity({
    name = 'transform',
    transform = {},
})

local ground_size = Vector3:new(6, 0.03, 6)
local ground = scene:create_entity_detached(scene_helper.build_cube_desc('Ground', Vector3.ZERO, ground_size, 0.0))
scene:attach(ground, transform)

-- create objects
for t = 1, 21 do
    local x = (t - 1) % 7
    local y = math.floor((t - 1) / 7)

    local translate = Vector3:new(x - 3, 5 - y, 0)
    local s = 0.25
    local size = Vector3:new(s, s, s)

    local desc;

    if t % 2 == 0 then
        desc = scene_helper.build_cube_desc('Cube' .. t, translate, size, 1.0)
    else
        desc = scene_helper.build_sphere_desc('Sphere' .. t, translate, s, 1.0)
    end

    local material = scene_helper.build_material_desc(Vector3.ONE, 0.99, 0.01)
    desc.material = material
    local object = scene:create_entity_detached(desc)
    scene:attach(object, transform)
end
