-- file: propeller.lua
Propeller = {}
Propeller.__index = Propeller
setmetatable(Propeller, GameObject)

function Propeller.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Propeller)
    return self
end

function Propeller:OnUpdate(timestep)
    local transform = g_scene:GetTransform(self.id)
    local rad = timestep * 10
    transform:Rotate(Vector3(rad, 0, 0))
end