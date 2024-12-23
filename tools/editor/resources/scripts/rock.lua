-- file: rock.lua
Rock = {}
Rock.__index = Rock
setmetatable(Rock, GameObject)

function Rock.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Rock)
    self.lifetime = -1
    return self
end

function Rock:OnUpdate(timestep)
    local transform = g_scene:GetTransform(self.id)
    transform:Rotate(Vector3(timestep, timestep, 0))
end

function Rock:OnCollision(other)
    local transform = g_scene:GetTransform(self.id)
    transform:SetTranslation(Vector3(0, 1000, 0))
end
