local called = false
api.event.on("evt", function(payload)
    called = true
end)

function was_called()
    return called
end

local count = 0
api.event.on("evt_multi", function()
    count = count + 1
end)
api.event.on("evt_multi", function()
    count = count + 1
end)

function handler_count()
    return count
end

local off_called = false
api.event.on("evt_off", function()
    off_called = true
end)
api.event.off("evt_off")

function off_was_called()
    return off_called
end
