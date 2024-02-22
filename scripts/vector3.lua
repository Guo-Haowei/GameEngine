local Vector3 = {}
Vector3.__index = Vector3

function Vector3:create(p_x, p_y, p_z)
    local this = { p_x, p_y, p_z }
    setmetatable(this, Vector3)

    return this
end

Vector3.ZERO = Vector3:create(0, 0, 0)
Vector3.HALF = Vector3:create(0.5, 0.5, 0.5)
Vector3.ONE = Vector3:create(1, 1, 1)

return Vector3