local geomath = require('geomath')
local scene_helper = require('scene-helper')

local Vector3 = geomath.Vector3

-- Renderer.set_env_map('@res://env/sky.hdr')

local scene = Scene.get()

scene_helper.create_point_light(scene, 'light1', Vector3:new(0, 2, 2))

-- create walls

local s = 4;
local scale = Vector3:new(s, s, s)
local desc = scene_helper.build_plane_desc('plane', Vector3.ZERO, scale)
desc.transform = {
    translate = Vector3:new(0, 0, -s)
}

local material = scene_helper.build_material_desc(Vector3.ONE, 0.1, 0.99)
desc.material = material
scene:create_entity(desc)