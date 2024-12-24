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
    local battery_particle = {}
    local rock_particle = {}

    local scripts = g_scene:GetAllLuaScripts()
    for i = 1, #scripts do
        local id = scripts[i]
        local script = g_scene:GetScript(id)
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
        if class == 'Particle' then
            local object = debug.getregistry()[ref]
            local name = g_scene:GetName(id)
            if string.starts_with(name, 'battery') then
                battery_particle[#battery_particle + 1] = object
            elseif string.starts_with(name, 'rock') then
                rock_particle[#rock_particle + 1] = object
            else
                error('unknown particle ' .. name)
            end
        end
    end

    self.time = 0
    self.batteries = { name = 'battery', dead = battery_pool, alive = {}, last_spawn = 0, interval = 1 }
    self.rocks = { name = 'rock', dead = rock_pool, alive = {}, last_spawn = 0, interval = 1 }
    self.rock_particle = { dead = rock_particle, alive = {} }
    self.battery_particle = { dead = battery_particle, alive = {} }
    self.angle = 0
    
    Game.instance = self
    return self
end

function Game:RequestParticle(count, position, name)
    local dead = self[name].dead
    local alive = self[name].alive
    for i = 1, count do
        local item = table.remove(dead)
        if not item then
            engine.log('failed to allocate [' .. name .. '], consider increase the capacity')
            break
        end

        local vx = math.random() * 3 - 1.5
        local vy = math.random() - 0.5
        local vz = math.random() * 3 - 1.5
        item.velocity = Vector3(vx, vy, vz)
        item.life_time = g.PARTICLE_LIFE_TIME
        item.sacle = 0.5
        item.rotation = Vector3(math.random(), math.random(), math.random())

        local transform = g_scene:GetTransform(item.id)

        transform:SetTranslation(Vector3(
            position.x,
            position.y + g.OCEAN_RADIUS,
            position.z))
        alive[#alive + 1] = item
    end
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

            item.life_time = g.ENTITY_LIFE_TIME
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
    recycle(self.battery_particle)
    recycle(self.rock_particle)
end

function Game.RequestRockParticle(position)
    Game.instance:RequestParticle(8, position, 'rock_particle')
end

function Game.RequestBatteryParticle(position)
    Game.instance:RequestParticle(6, position, 'battery_particle')
end