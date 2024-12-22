GameObject = {}
GameObject.__index = GameObject

function GameObject.new(id)
	local self = setmetatable({}, GameObject)
	self.id = id
	return self
end

function GameObject:OnUpdate(timestep)
	-- print("hello from GameObject")
end
