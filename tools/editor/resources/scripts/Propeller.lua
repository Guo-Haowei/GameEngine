-- propeller.lua
Propeller = {}
Propeller.__index = Propeller
setmetatable(Propeller, GameObject)

function Propeller.new(id)
	local self = GameObject.new(id)
	setmetatable(self, Propeller)
	print("id is " .. id)
	return self
end

function Propeller:OnUpdate(timestep)
    local transform = scene.GetTransformComponent(self.id)
    local rad = timestep * 4
    transform:rotate(Vector3(rad, 0, 0))
end
