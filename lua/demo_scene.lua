api.log.info("demo_scene: init")

local function on_player_spawn(entity_id)
    api.log.info("demo_scene: player spawned, entity_id=" .. tostring(entity_id))
    api.camera.switch("overview")
    api.log.info("demo_scene: switch -> active=" .. api.camera.get_active())
end

api.event.on("player_spawn", on_player_spawn)
