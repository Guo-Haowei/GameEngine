-- file: earth.lua
local g = require('@res://scripts/constants.lua')

Earth = {}
Earth.__index = Earth
setmetatable(Earth, GameObject)

function Earth.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Earth)
    return self
end

function Earth:OnUpdate(timestep)
    local transform = g_scene:GetTransform(self.id)
    local rad = timestep * g.WORLD_SPEED
    transform:Rotate(Vector3(0, 0, rad))
end