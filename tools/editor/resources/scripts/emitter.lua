-- file: emitter.lua
Emitter = {}
Emitter.__index = Emitter
setmetatable(Emitter, GameObject)

function Emitter.new(id)
    local self = GameObject.new(id)
    setmetatable(self, Emitter)
    self.lifetime = 0;
    return self
end

function Emitter:OnUpdate(timestep)
    self.lifetime = self.lifetime - timestep
    self.lifetime = math.max(self.lifetime, -1)

    local emitter = g_scene:GetMeshEmitter(self.id)
    if self.lifetime < 0 then
        emitter:Stop()
    end
end