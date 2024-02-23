local geomath = require('geomath')
local scene_helper = require('scene-helper')

local Vector3 = geomath.Vector3

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

local scene = Scene.get()

-- create point light
scene_helper.create_point_light(scene, 'PointLight', Vector3:create(0, 4, 0))

-- create ground
local ground_size = Vector3:create(6, 0.03, 6);
Scene.create_entity(scene, scene_helper.build_cube_desc('Ground', Vector3.ZERO, ground_size, 0.0))

-- create objects
for t = 1, 21 do
    local x = (t - 1) % 7
    local y = math.floor((t - 1) / 7)

    local translate = Vector3:create(x - 3, 5 - y, 0)
    local s = 0.25
    local size = Vector3:create(s, s, s)

    if t % 2 == 0 then
        Scene.create_entity(scene, scene_helper.build_cube_desc('Cube' .. t, translate, size, 1.0))
    else
        Scene.create_entity(scene, scene_helper.build_sphere_desc('Sphere' .. t, translate, s, 1.0))
    end
end

return true
