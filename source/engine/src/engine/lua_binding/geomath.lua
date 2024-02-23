local geomath = {}

local Vector3 = {}
Vector3.__index = Vector3

function Vector3:new(p_x, p_y, p_z)
    local this = { p_x, p_y, p_z }
    setmetatable(this, Vector3)

    return this
end

Vector3.ZERO = Vector3:new(0, 0, 0)
Vector3.HALF = Vector3:new(0.5, 0.5, 0.5)
Vector3.ONE = Vector3:new(1, 1, 1)
Vector3.UNIX_X = Vector3:new(1, 0, 0)
Vector3.UNIX_Y = Vector3:new(0, 1, 0)
Vector3.UNIX_Z = Vector3:new(0, 0, 1)

function geomath.clamp(a, min, max)
    a = math.max(a, min)
    a = math.min(a, max)
    return a
end

geomath.Vector3 = Vector3

return geomath