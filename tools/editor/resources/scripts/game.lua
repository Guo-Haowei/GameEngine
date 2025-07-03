-- file: game.lua
local g = require('@res://scripts/constants.lua')

Game = {}
Game.__index = Game

-- @TODO: refactor
string.starts_with = function(str, prefix)
    return string.sub(str, 1, string.len(prefix)) == prefix
end

function Game.new()
    local self = setmetatable({}, Game)

    local battery_pool = {}
    local rock_pool = {}
    local battery_emitter = {}
    local rock_emitter = {}

    local scripts = g_scene:GetAllLuaScripts()
    for i = 1, #scripts do
        local id = scripts[i]
        local script = g_scene:GetScript(id)
        local ref = script:GetRef()
        local class = script:GetClass()
        if class == 'Battery' then
            local object = debug.getregistry()[ref]
            battery_pool[#battery_pool + 1] = object
        elseif class == 'Rock' then
            local object = debug.getregistry()[ref]
            rock_pool[#rock_pool + 1] = object
        elseif class == 'Emitter' then
            local object = debug.getregistry()[ref]
            local name = g_scene:GetName(id)
            if string.starts_with(name, 'emitter::rock') then
                rock_emitter[#rock_emitter + 1] = object
            elseif string.starts_with(name, 'emitter::battery') then
                battery_emitter[#battery_emitter + 1] = object
            else
                error('unknown emitter ' .. name)
            end
        end
    end

    self.time = 0
    self.batteries = { name = 'battery', dead = battery_pool, alive = {}, last_spawn = 0, interval = 1 }
    self.rocks = { name = 'rock', dead = rock_pool, alive = {}, last_spawn = 0, interval = 1 }
    self.angle = 0

    self.battery_emitter = battery_emitter
    self.rock_emitter = rock_emitter

    Game.instance = self
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
            count = 3 + math.random(0, 2)
        end

        offset = offset + math.random(-3, 3)
        local d = g.OCEAN_RADIUS + math.random(g.MIN_HEIGHT, g.MAX_HEIGHT)
        for i = 1, count do
            local item = table.remove(type.dead)
            if not item then
                engine.log('failed to allocate [' .. type.name .. '], consider increase the capacity')
                return
            end

            item.lifetime = g.ENTITY_LIFETIME
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
            item.lifetime = item.lifetime - timestep
            if item.lifetime > 0 then
                tmp[#tmp + 1] = item
            else
                item.lifetime = -1
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

function Game:RequestParticle(emitters, position)
    for i = 1, #emitters do
        local object = emitters[i]
        local emitter = g_scene:GetMeshEmitter(object.id)
        if not emitter:IsRunning() then
            object.lifetime = g.EMITTER_LIFETIME
            emitter:Reset()
            emitter:Start()
            local transform = g_scene:GetTransform(object.id)
            position.y = position.y - 2
            transform:SetTranslation(position)
            return
        end
    end
end

function Game.RequestRockParticle(position)
    Game.instance:RequestParticle(Game.instance.rock_emitter, position)
end

function Game.RequestBatteryParticle(position)
    Game.instance:RequestParticle(Game.instance.battery_emitter, position)
end
