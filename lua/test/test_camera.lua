-- 相机绑定测试脚本: 定义若干无参函数, 供 C++ call_function 调用验证 api.camera.* 调用链。

function switch_to_overview()
    return api.camera.switch("overview")
end

function switch_by_id()
    return api.camera.switch(1)
end

function current_name()
    return api.camera.get_active()
end

function current_id()
    return api.camera.get_active_id()
end

function change_fov()
    return api.camera.set_param("fov", 60)
end

function change_distance()
    return api.camera.set_param("distance", 20)
end

function camera_count()
    return #api.camera.list()
end

function shake_placeholder()
    return api.camera.shake(1, 2, 3)
end

function set_type_ortho()
    return api.camera.set_type("isometric_ortho")
end

function set_type_invalid()
    return api.camera.set_type("perspective")
end

function get_type_str()
    return api.camera.get_type()
end

function set_follow_player()
    return api.camera.set_follow(7, 0.25)
end

function set_follow_bad_smoothing()
    return api.camera.set_follow(7, 0)
end

function clear_follow_player()
    return api.camera.clear_follow()
end
