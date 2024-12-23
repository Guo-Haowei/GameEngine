-- file: earth.lua
Earth = {}
Earth.__index = Earth
setmetatable(Earth, GameObject)

function Earth.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Earth)
    return self
end

function Earth:OnUpdate(timestep)
    local WORLD_SPEED = 0.3
    local transform = scene.GetTransform(self.id)
    local rad = timestep * WORLD_SPEED
    transform:Rotate(Vector3(0, 0, rad))
end