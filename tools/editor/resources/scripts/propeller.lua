local function on_update()
    local transform = scene.GetTransformComponent()
    local rad = timestep * 9
    transform:rotate(Vector3(rad, 0, 0))
end

local function on_collide()
end

on_update()
