-- file: battery.lua
Battery = {}
Battery.__index = Battery
setmetatable(Battery, GameObject)

function Battery.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Battery)
    return self
end

function Battery:OnUpdate(timestep)
    local transform = scene.GetTransform(self.id)
    transform:Rotate(Vector3(timestep, timestep, 0))
end

function Battery:OnCollision(other)
    local transform = scene.GetTransform(self.id)
    transform:Translate(Vector3(0, -1000, 0))
end
