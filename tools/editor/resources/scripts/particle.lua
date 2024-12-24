-- file: particle.lua
local g = require('@res://scripts/constants.lua')

Particle = {}
Particle.__index = Particle
setmetatable(Particle, GameObject)

function Particle.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Particle)
    self.life_time = -1
    self.velocity = Vector3(0, 0, 0)
    self.rotation = Vector3(0, 0, 0)
    self.scale = 0.5
    return self
end

function Particle:OnUpdate(timestep)
    if self.life_time < 0 then
        return
    end

    local transform = g_scene:GetTransform(self.id)

    transform:Translate(self.velocity)

    local s = self.life_time / g.PARTICLE_LIFE_TIME
    s = math.max(self.sacle, 0.1)
    transform:SetScale(Vector3(s, s, s))
    transform:Rotate(Vector3(
        timestep * self.rotation.x,
        timestep * self.rotation.y,
        timestep * self.rotation.z
    ))

    self.velocity.x = 0.9 * self.velocity.x
    self.velocity.y = self.velocity.y - timestep
    self.velocity.z = 0.9 * self.velocity.z
end