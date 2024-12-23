const char* g_lua_always_load = R"(
GameObject = {}
GameObject.__index = GameObject

function GameObject.new(id)
    local self = setmetatable({}, GameObject)
    self.id = id
    return self
end

function GameObject:OnUpdate(timestep)
end

function GameObject:OnCollision(other)
end

function math.clamp(value, min, max)
    return math.max(min, math.min(max, value))
end
)";