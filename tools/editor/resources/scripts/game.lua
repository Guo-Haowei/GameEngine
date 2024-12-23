Game = {}
Game.__index = Game

function Game.new()
    local self = setmetatable({}, Game)

    local battery_pool = {}
    local rock_pool = {}

    local scripts = g_scene:GetAllLuaScripts()
    for i = 1, #scripts do
        local script = g_scene:GetScript(scripts[i])
        local ref = script:GetRef()
        local class = script:GetClass()
        if class == 'Battery' then
            local object = debug.getregistry()[ref]
            battery_pool[#battery_pool + 1] = object
        end
        if class == 'Rock' then
            local object = debug.getregistry()[ref]
            rock_pool[#rock_pool + 1] = object
        end
    end

    self.last_spawn = 0.0
    self.time = 0.0
    self.batteries = { dead = battery_pool, alive = {} }
    self.rocks = { dead = rock_pool, alive = {} }
    return self
end

function Game:OnUpdate(timestep)
    self.time = self.time + timestep

    local function spawn(type, offset)
        local ENTITY_LIFE_TIME = 12
        local WORLD_SPEED = 0.3
        local OCEAN_RADIUS = 240.0
        local MIN_HEIGHT = 15
        local MAX_HEIGHT = 45
 
        local item = table.remove(type.dead)
        if not item then
            return false
        end

        item.life_time = 12
        local transform = g_scene:GetTransform(item.id)
        local angle = self.time * WORLD_SPEED
        local d = OCEAN_RADIUS + math.random(MIN_HEIGHT, MAX_HEIGHT)
        angle = math.deg(angle) - offset
        local translation = Vector3(d * math.sin(angle), d * math.cos(angle), 0)
        transform:SetTranslation(translation)

        type.alive[#type.alive + 1] = item
        print('item ' .. item.id ..  ' spawned at ' .. angle)
        return true
    end

    -- spawn new items
    if self.time - self.last_spawn > 1 then
        if spawn(self.batteries, 60) then
           self.last_spawn = self.time
        end
        if spawn(self.rocks, 85) then
            self.last_spawn = self.time
        end
    end

    -- recycle dead items
    local function recycle(type)
        local tmp = {}
        for i = #type.alive, 1, -1 do
            local item = type.alive[i]
            item.life_time = item.life_time - timestep
            if item.life_time > 0 then
                tmp[#tmp + 1] = item
            else
                item.life_time = -1
                type.dead[#type.dead + 1] = item
                -- move dead item away
                local transform = g_scene:GetTransform(item.id)
                transform:SetTranslation(Vector3(0, 900, 0))
                print('item ' .. item.id .. ' recycled')
            end
        end
        type.alive = tmp
    end

    recycle(self.batteries)
    recycle(self.rocks)
end

