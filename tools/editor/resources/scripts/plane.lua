-- file: plane.lua
Plane = {}
Plane.__index = Plane
setmetatable(Plane, GameObject)

function Plane.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Plane)
    self.speed = Vector2(0, 0)
    self.displacement = Vector2(0, 0)
    return self
end

function Plane:OnUpdate(timestep)
end

function Plane:OnCollision(other)
end