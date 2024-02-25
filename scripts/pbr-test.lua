local geomath = require('geomath')
local scene_helper = require('scene-helper')
local Vector3 = geomath.Vector3

local scene = Scene.get()

-- create point light
-- local light_pos = {
--     Vector3:new(-5, -5, 5),
--     Vector3:new(5, -5, 5),
--     Vector3:new(5, 5, 5),
--     Vector3:new(-5, 5, 5),
-- }

-- for idx = 1, 4 do
--     scene_helper.create_point_light(scene, 'PointLight', light_pos[idx])
-- end

local num_row = 7
local num_col = 7
local spacing = 2.5

for row = 0, num_row - 1 do
    for col = 0, num_col - 1 do
        local x = (col - 0.5 * num_col) * spacing
        local y = (row - 0.5 * num_row) * spacing
        local translate = Vector3:new(x, y, 0)
        local id = row * num_col + col
        local desc = scene_helper.build_sphere_desc('Sphere' .. id, translate, 1.0)
        local material = scene_helper.build_material_desc(Vector3.UNIX_X, row / num_row, col / num_col)
        desc.material = material
        Scene.create_entity(scene, desc)
    end
end