GameObject = {}
GameObject.__index = GameObject

function GameObject.new(id)
	local self = setmetatable({}, GameObject)
	self.id = id
	return self
end

function GameObject:OnUpdate(timestep)
	print("hello from GameObject")
end

-- propeller.lua
Propeller = {}
Propeller.__index = Propeller
setmetatable(Propeller, GameObject)

function Propeller.new(id)
	local self = GameObject.new(id)
	setmetatable(self, Propeller)
	return self
end

function Propeller:OnUpdate(timestep)
	print("hello from Propeller")
end
