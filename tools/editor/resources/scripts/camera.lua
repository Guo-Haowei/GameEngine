-- file: camera.lua
Camera = {}
Camera.__index = Camera
setmetatable(Camera, GameObject)

function Camera.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Camera)
    return self
end

function Camera:OnUpdate(timestep)
    local camera = g_scene:GetPerspectiveCamera(self.id)
    local mouse_move = input.GetMouseMove()
    if (math.abs(mouse_move.x) > 2) then
        local angle = camera:GetFovy()
        angle = angle + mouse_move.x * timestep * 2
        angle = math.clamp(angle, 35, 80)
        camera:SetFovy(angle)
    end
end