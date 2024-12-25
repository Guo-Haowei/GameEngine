-- file: plane.lua
local g = require('@res://scripts/constants.lua')
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
    local function normalize(value, low, high, clamp_low, clamp_high)
        local bounded = math.clamp(value, low, high)
        return (bounded - low) / (high - low) * (clamp_high - clamp_low)
    end

    local cursor = input.GetCursor()
    local display_size = display:GetWindowSize()
    cursor = cursor / display_size
    cursor = Vector2(2, 2) * cursor
    cursor = cursor - Vector2(1, 1)
    cursor.y = -cursor.y

    local transform = g_scene:GetTransform(self.id)
    local translate = transform:GetTranslation()
    self.displacement = self.displacement + self.speed

    local target_x = normalize(cursor.x, -1, 1, -g.AMP_WIDTH, -0.7 * g.AMP_WIDTH) + self.displacement.x
    local target_y = normalize(cursor.y, -0.75, 0.75, translate.y -g.AMP_WIDTH, translate.y + g.AMP_WIDTH) + self.displacement.y

    local speed = 3 * timestep
    local delta = Vector2(target_x - translate.x, target_y - translate.y) * Vector2(speed, speed)
    translate.x = translate.x + delta.x
    translate.y = translate.y + delta.y
    translate.y = math.clamp(translate.y, g.MIN_HEIGHT, g.MAX_HEIGHT)

    transform:SetTranslation(translate)

    self.speed.x = 0.8 * self.speed.x
    self.speed.y = 0.8 * self.speed.y
    self.displacement.x = 0.9 * self.displacement.x 
    self.displacement.y = 0.9 * self.displacement.y

    local rotate_z = 0.3 * delta.y
    rotate_z = math.clamp(rotate_z, -60, 60)
    local quaternion = Quaternion(Vector3(0, 0, rotate_z))
    transform:SetRotation(quaternion)
end

function Plane:OnCollision(other)
    local rigid = g_scene:GetRigidBody(other)
    local type = rigid.collision_type
    -- TODO: use enum instead of numbers
    if type == 2 then
        local plane_transform = g_scene:GetTransform(self.id)
        local rock_transform = g_scene:GetTransform(other)
        local plane_position = plane_transform:GetWorldTranslation()
        local rock_position = rock_transform:GetWorldTranslation()
        local dist = plane_position - rock_position
        dist:normalize()
        self.speed = Vector2(30 * dist.x, 30 * dist.y)
    elseif type == 4 then
        -- engine.log('Battery!')
    else
        error('Unknown type ' .. type)
    end
end