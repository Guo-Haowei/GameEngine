-- file: hair.lua
Hair = {}
Hair.__index = Hair
setmetatable(Hair, GameObject)

function Hair.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Hair)
    local transform = g_scene:GetTransform(self.id)
    local scale = transform:GetScale();
    self.scale_y = scale.y
    return self
end

function Hair:OnUpdate(timestep)
    local transform = g_scene:GetTransform(self.id)
    local new_scale = self.scale_y + timestep
    if new_scale > 0.95 then
        new_scale = 0.5
    end
    self.scale_y = new_scale

    local scale = transform:GetScale()
    transform:SetScale(Vector3(scale.x, new_scale, scale.z))
end