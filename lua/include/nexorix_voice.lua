-- ============================================================
--  Nexorix Voice API
--  Interface Lua de alto nível para o sistema de voz.
--
--  Uso:
--    require("include/nexorix_voice")
--    NX.Voice.CreateStream(...)
--    NX.Voice.SetPlayerMuted(playerid, true)
--    NX.Hook("OnPlayerVoiceStart", function(playerid, channels) ... end)
-- ============================================================

NX.Voice = NX.Voice or {}

-- ============================================================
-- Constants
-- ============================================================

NX.Voice.FILTER = {
    CHORUS      = 0,
    COMPRESSOR  = 1,
    DISTORTION  = 2,
    ECHO        = 3,
    FLANGER     = 4,
    GARGLE      = 5,
    I3DL2REVERB = 6,
    PARAMEQ     = 7,
    REVERB      = 8,
}

NX.Voice.TARGET = {
    VEHICLE = 1,
    PLAYER  = 2,
    OBJECT  = 3,
}

NX.Voice.NONE = 0xFFFF

-- ============================================================
-- Player Functions
-- ============================================================

function NX.Voice.GetPlayerVersion(playerid)
    return nx_voice_get_version(playerid)
end

function NX.Voice.HasMicro(playerid)
    return nx_voice_has_micro(playerid)
end

function NX.Voice.IsPlayerConnected(playerid)
    return nx_voice_get_version(playerid) > 0
end

function NX.Voice.SetPlayerMuted(playerid, muted)
    if muted then
        nx_voice_disable_listener(playerid)
    else
        nx_voice_enable_listener(playerid)
    end
end

function NX.Voice.IsPlayerMuted(playerid)
    return not nx_voice_check_listener(playerid)
end

function NX.Voice.EnableMicrophone(playerid, channels)
    nx_voice_enable_speaker(playerid, channels)
end

function NX.Voice.DisableMicrophone(playerid, channels)
    nx_voice_disable_speaker(playerid, channels)
end

function NX.Voice.SetPushToTalkKey(playerid, key, channels)
    nx_voice_set_key(playerid, key, channels)
end

function NX.Voice.Play(playerid, channels)
    nx_voice_play(playerid, channels)
end

function NX.Voice.Stop(playerid, channels)
    nx_voice_stop(playerid, channels)
end

-- ============================================================
-- Stream Object
-- ============================================================

local StreamMT = {}
StreamMT.__index = StreamMT

function StreamMT:GetID()
    return self._id
end

function StreamMT:GetName()
    return self._name
end

function StreamMT:GetType()
    return self._type
end

function StreamMT:AddPlayer(playerid)
    nx_voice_attach_listener(self._id, playerid)
end

function StreamMT:RemovePlayer(playerid)
    nx_voice_detach_listener(self._id, playerid or NX.Voice.NONE)
end

function StreamMT:HasPlayer(playerid)
    return nx_voice_has_listener(self._id, playerid or NX.Voice.NONE)
end

function StreamMT:SetVolume(volume)
    nx_voice_set_volume(self._id, volume)
end

function StreamMT:SetPanning(panning)
    nx_voice_set_panning(self._id, panning)
end

function StreamMT:SetDistance(distance)
    nx_voice_set_distance(self._id, distance)
end

function StreamMT:SetPosition(x, y, z)
    nx_voice_set_position(self._id, x, y, z)
end

function StreamMT:AttachToVehicle(vehicleid)
    local target = (NX.Voice.TARGET.VEHICLE << 14) | (vehicleid & 0x3FFF)
    nx_voice_set_target(self._id, target)
end

function StreamMT:AttachToPlayer(playerid)
    local target = (NX.Voice.TARGET.PLAYER << 14) | (playerid & 0x3FFF)
    nx_voice_set_target(self._id, target)
end

function StreamMT:AttachToObject(objectid)
    local target = (NX.Voice.TARGET.OBJECT << 14) | (objectid & 0x3FFF)
    nx_voice_set_target(self._id, target)
end

function StreamMT:DetachTarget()
    nx_voice_set_target(self._id, 0)
end

function StreamMT:SetEffect(effect)
    local eid = effect and effect:GetID() or NX.Voice.NONE
    nx_voice_set_effect(self._id, eid)
end

function StreamMT:SetIcon(icon)
    nx_voice_set_icon(self._id, icon)
end

function StreamMT:SetTransiter(enabled)
    if enabled then
        nx_voice_enable_transiter(self._id)
    else
        nx_voice_disable_transiter(self._id)
    end
end

function StreamMT:Destroy()
    if self._id >= 0 then
        nx_voice_delete_stream(self._id)
        self._id = -1
    end
end

-- ============================================================
-- Stream Factory
-- ============================================================

function NX.Voice.CreateStream(opts)
    opts = opts or {}
    local distance = opts.distance or 0.0
    local id = nx_voice_create_stream(distance)
    if id < 0 then return nil end
    local stream = setmetatable({
        _id = id,
        _name = opts.name or ("stream_" .. id),
        _type = (distance > 0) and "proximity" or "global",
    }, StreamMT)
    return stream
end

function NX.Voice.CreateProximityStream(distance, opts)
    opts = opts or {}
    opts.distance = distance
    return NX.Voice.CreateStream(opts)
end

function NX.Voice.CreateGlobalStream(opts)
    opts = opts or {}
    opts.distance = 0.0
    return NX.Voice.CreateStream(opts)
end

-- ============================================================
-- Channel Object
-- ============================================================

local ChannelMT = {}
ChannelMT.__index = ChannelMT

function NX.Voice.CreateChannel(opts)
    opts = opts or {}
    local stream
    if opts.type == "proximity" then
        stream = NX.Voice.CreateProximityStream(opts.distance or 50.0, opts)
    else
        stream = NX.Voice.CreateGlobalStream(opts)
    end
    if not stream then return nil end
    local channel = setmetatable({
        _stream = stream,
        _name = opts.name or stream:GetName(),
        _type = opts.type or "global",
        _players = {},
        _channel_index = opts.channel_index or 0,
    }, ChannelMT)
    return channel
end

function ChannelMT:GetName()
    return self._name
end

function ChannelMT:GetStream()
    return self._stream
end

function ChannelMT:GetType()
    return self._type
end

function ChannelMT:AddPlayer(playerid)
    self._stream:AddPlayer(playerid)
    nx_voice_attach_stream(playerid, self._stream:GetID(), {self._channel_index})
    self._players[playerid] = true
end

function ChannelMT:RemovePlayer(playerid)
    self._stream:RemovePlayer(playerid)
    nx_voice_detach_stream(playerid, self._stream:GetID(), {self._channel_index})
    self._players[playerid] = nil
end

function ChannelMT:HasPlayer(playerid)
    return self._players[playerid] == true
end

function ChannelMT:GetPlayers()
    local list = {}
    for pid, _ in pairs(self._players) do
        list[#list + 1] = pid
    end
    return list
end

function ChannelMT:GetPlayerCount()
    local count = 0
    for _ in pairs(self._players) do count = count + 1 end
    return count
end

function ChannelMT:SetVolume(volume)
    self._stream:SetVolume(volume)
end

function ChannelMT:SetEffect(effect)
    self._stream:SetEffect(effect)
end

function ChannelMT:SetIcon(icon)
    self._stream:SetIcon(icon)
end

function ChannelMT:Destroy()
    for pid, _ in pairs(self._players) do
        self:RemovePlayer(pid)
    end
    self._stream:Destroy()
end

-- ============================================================
-- Effect Object
-- ============================================================

local EffectMT = {}
EffectMT.__index = EffectMT

function NX.Voice.CreateEffect()
    local id = nx_voice_create_effect()
    if id < 0 then return nil end
    return setmetatable({ _id = id }, EffectMT)
end

function EffectMT:GetID()
    return self._id
end

function EffectMT:AddFilter(filter_type, priority, params)
    return nx_voice_append_filter(self._id, filter_type, priority, params or {})
end

function EffectMT:RemoveFilter(filter_type, priority)
    nx_voice_remove_filter(self._id, filter_type, priority)
end

function EffectMT:Destroy()
    if self._id >= 0 then
        nx_voice_delete_effect(self._id)
        self._id = -1
    end
end

-- ============================================================
-- Preset Effects
-- ============================================================

function NX.Voice.CreateRadioEffect()
    local effect = NX.Voice.CreateEffect()
    if not effect then return nil end
    effect:AddFilter(NX.Voice.FILTER.COMPRESSOR, 0, {
        gain = 5.0, attack = 0.5, release = 200.0,
        threshold = -20.0, ratio = 4.0, predelay = 4.0
    })
    effect:AddFilter(NX.Voice.FILTER.PARAMEQ, 1, {
        center = 3000.0, bandwidth = 12.0, gain = 3.0
    })
    return effect
end

function NX.Voice.CreatePhoneEffect()
    local effect = NX.Voice.CreateEffect()
    if not effect then return nil end
    effect:AddFilter(NX.Voice.FILTER.PARAMEQ, 0, {
        center = 2000.0, bandwidth = 6.0, gain = -6.0
    })
    effect:AddFilter(NX.Voice.FILTER.DISTORTION, 1, {
        gain = -2.0, edge = 10.0,
        posteqcenterfrequency = 2000.0,
        posteqbandwidth = 1000.0,
        prelowpasscutoff = 4000.0
    })
    return effect
end

function NX.Voice.CreateMegaphoneEffect()
    local effect = NX.Voice.CreateEffect()
    if not effect then return nil end
    effect:AddFilter(NX.Voice.FILTER.COMPRESSOR, 0, {
        gain = 10.0, attack = 0.5, release = 100.0,
        threshold = -15.0, ratio = 6.0, predelay = 2.0
    })
    effect:AddFilter(NX.Voice.FILTER.PARAMEQ, 1, {
        center = 4000.0, bandwidth = 8.0, gain = 6.0
    })
    return effect
end

function NX.Voice.CreateReverbEffect()
    local effect = NX.Voice.CreateEffect()
    if not effect then return nil end
    effect:AddFilter(NX.Voice.FILTER.I3DL2REVERB, 0, {
        room = -1000, roomhf = -100,
        roomrollofffactor = 0.0,
        decaytime = 2.0, decayhfratio = 0.5,
        reflections = -2000, reflectionsdelay = 0.02,
        reverb = 200, reverbdelay = 0.04,
        diffusion = 100.0, density = 100.0,
        hfreference = 5000.0
    })
    return effect
end

nx_print("[NX.Voice] Voice API v1.0 loaded.")
