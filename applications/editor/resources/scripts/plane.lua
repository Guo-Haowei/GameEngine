local function on_update()
    local delta = input.mouse_move()
    local translation = Vector3f.new(0.1 * delta.x, -0.1 * delta.y, 0.0)
    scene_helper.entity_translate(translation)
end

local function on_collide()
end

on_update()
