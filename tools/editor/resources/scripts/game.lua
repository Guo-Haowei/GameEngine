-- file: game.lua
local g = require('@res://scripts/constants.lua')

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

    self.time = 0
    self.batteries = { name = 'battery', dead = battery_pool, alive = {}, last_spawn = 0, interval = 1 }
    self.rocks = { name = 'rock', dead = rock_pool, alive = {}, last_spawn = 0, interval = 1 }
    self.angle = 0
    return self
end

function Game:OnUpdate(timestep)
    self.time = self.time + timestep
    self.angle = self.angle + timestep * g.WORLD_SPEED

    local function spawn(type, offset)
        if self.time - type.last_spawn < type.interval then
            return
        end

        local count = 1
        if type.name == 'battery' then
            count = 2 + math.floor(math.random(0, 3))
        end

        offset = offset + math.random(-3, 3)
        local d = g.OCEAN_RADIUS + math.random(g.MIN_HEIGHT, g.MAX_HEIGHT)
        for i = 1, count do
            local item = table.remove(type.dead)
            if not item then
                engine.Log('failed to allocate [' .. type.name .. ']')
                return
            end

            item.life_time = 12
            local transform = g_scene:GetTransform(item.id)
            local angle = self.angle + math.rad(offset)
            d = d + math.random(-2, 2)
            offset = offset + 2
            local translation = Vector3(d * math.sin(angle), d * math.cos(angle), 0)
            transform:SetTranslation(translation)
            type.alive[#type.alive + 1] = item
        end

        type.last_spawn = self.time
    end

    -- spawn new items
    spawn(self.batteries, 40)
    spawn(self.rocks, 50)

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
                transform:SetTranslation(Vector3(0, 1000, 0))
            end
        end
        type.alive = tmp
    end

    recycle(self.batteries)
    recycle(self.rocks)
end