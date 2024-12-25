-- file: battery.lua
Battery = {}
Battery.__index = Battery
setmetatable(Battery, GameObject)

function Battery.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Battery)
    self.lifetime = -1
    return self
end

function Battery:OnUpdate(timestep)
    local transform = g_scene:GetTransform(self.id)
    transform:Rotate(Vector3(timestep, timestep, 0))
end

function Battery:OnCollision(other)
    local transform = g_scene:GetTransform(self.id)
    local position = transform:GetWorldTranslation()
    transform:SetTranslation(Vector3(0, 1000, 0))
    Game.RequestBatteryParticle(position)
end