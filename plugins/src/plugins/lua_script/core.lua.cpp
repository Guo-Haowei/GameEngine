#define LUA(str) (const char*)"\n" #str

// clang-format off
const char* g_lua_always_load = LUA(
    GameObject = {};
    GameObject.__index = GameObject;
    function GameObject.new(id)
        local self = setmetatable({}, GameObject);
        self.id = id;
        return self;
    end
    function GameObject : OnUpdate(timestep)
    end
    function GameObject : OnCollision(other)
    end
    function math.clamp(value, min, max)
        return math.max(min, math.min(max, value));
    end
);
// clang-format on

#undef LUA