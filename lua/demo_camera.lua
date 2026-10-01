-- P0-08 运行验证演示: 相机由 Lua 驱动 (配置 / 切换 / 改参数)。
-- 在 ModelViewerGame::init() 中经 do_file 执行一次, 用于验证 api.camera.* 链路与画面效果。

api.log.info("demo_camera: start")

api.camera.switch("overview")
api.log.info("demo_camera: switch -> active=" .. api.camera.get_active())

api.camera.set_param("fov", 35)
api.log.info("demo_camera: set fov=35")

api.camera.set_param("distance", 18)
api.log.info("demo_camera: set distance=18")

api.log.info("demo_camera: done, active=" .. api.camera.get_active())
