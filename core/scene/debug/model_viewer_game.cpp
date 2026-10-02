#include "scene/debug/model_viewer_game.h"

#include "camera/camera_raylib.h"
#include "camera/free_fly.h"
#include "imgui.h"
#include "log/app_log.h"
#include "rlImGui.h"
#include "script/api_registry.h"
#include "util/asset_path.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {
constexpr const char* kDefaultModelPath = "assets/models/body/Bob_Idle.glb";
constexpr const char* kDefaultTexturePath = "assets/texture/body/MaleBody01.png";

constexpr Color kBackgroundColor{32, 36, 40, 255};

void resetCamera(Camera3D& camera, const Vector3& position, const Vector3& target, const Vector3& up, float fovy,
                 int projection) {
    camera.position = position;
    camera.target = target;
    camera.up = up;
    camera.fovy = fovy;
    camera.projection = projection;
}

// 根据模型包围盒调整相机位置, 使模型完整入镜
void fitCameraToModel(Camera3D& camera, const model_loader::ModelScene& scene) {
    float center[3];
    float size[3];
    if (!scene.bounds(center, size)) {
        return;
    }

    const float maxSize = std::max({size[0], size[1], size[2]});
    if (maxSize <= 0.0f) {
        return;
    }

    const float distance = (maxSize / std::tan(camera.fovy * 0.5f * DEG2RAD)) * 1.4f;
    resetCamera(camera, {center[0] + distance * 0.6f, center[1] + distance * 0.5f, center[2] + distance},
                {center[0], center[1], center[2]}, {0.0f, 1.0f, 0.0f}, camera.fovy, camera.projection);
}

// 将模型平移, 使其本地包围盒的底面中心落在世界原点 (角色站在网格平面上, 水平居中)
void centerModelOnGrid(model_loader::Transform& transform, const model_loader::ModelScene& scene) {
    float center[3];
    float size[3];
    if (!scene.bounds(center, size)) {
        return;
    }
    transform.position[0] = -center[0];
    transform.position[1] = -(center[1] - size[1] * 0.5f);
    transform.position[2] = -center[2];
}
} // namespace

namespace scene::debug {
bool ModelViewerGame::init() {
    // 相机默认值
    resetCamera(camera_, {8.0f, 6.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f, CAMERA_PERSPECTIVE);

    std::snprintf(modelPath_, sizeof(modelPath_), "%s", kDefaultModelPath);
    std::snprintf(texturePath_, sizeof(texturePath_), "%s", kDefaultTexturePath);

    // 启动时加载默认模型, 验证 raylib 模型加载链路 (必须在 InitWindow 之后, 纹理上传需要 GL 上下文)
    const std::string resolved = asset_path::resolve(kDefaultModelPath);
    std::string error;
    if (!scene_.load(resolved, &error)) {
        app_log::error("Startup model load failed: " + error);
    } else {
        centerModelOnGrid(transform_, scene_);
        std::string texError;
        if (!scene_.applyTexture(model_loader::MaterialMapType::Albedo, asset_path::resolve(kDefaultTexturePath),
                                 &texError)) {
            app_log::warn("Startup texture apply failed: " + texError);
        }
    }

    // P0-08: 相机系统骨架最小接线 —— 引入 camera::Manager + Lua 引擎, 注册 api.camera.*,
    // 加载演示脚本由 Lua 配置/切换相机, 并把活动相机同步到渲染相机 (camera_)。
    camera::Params params;
    params.distance = 15.0f;
    cameraManager_.create("default", camera::Type::Isometric, params);
    params.distance = 25.0f;
    cameraManager_.create("overview", camera::Type::Isometric, params);
    params.distance = 8.0f;
    cameraManager_.create("near", camera::Type::Isometric, params);
    cameraManager_.set_active("default");

    api::register_all(lua_);
    api::register_camera(lua_.raw_state(), cameraManager_);
    lua_.do_file(asset_path::resolve("lua/demo_camera.lua"));

    camera_ = camera::to_camera3d(*cameraManager_.active_params());

    return true;
}

void ModelViewerGame::update(float fixedDt) {
    // P0 阶段无固定步长业务逻辑; 30Hz 推进计数由 App::run 完成。
    (void)fixedDt;
}

void ModelViewerGame::render(float alpha) {
    // 模型动画按帧推进 (保留平滑); alpha 暂未用于插值, 留待后续业务。
    (void)alpha;
    scene_.update(GetFrameTime());

    // 环绕相机控制 (鼠标移动旋转 / 滚轮缩放), 悬停在 ImGui 面板上时不生效
    if (orbitalControl_ && !ImGui::GetIO().WantCaptureMouse) {
        UpdateCamera(&camera_, CAMERA_ORBITAL);
    }

    // 手动飞行相机 (WASD 移动 / 右键旋转)
    if (manualControl_) {
        camera::update_free_fly(camera_, cameraMoveSpeed_, cameraRotateSpeed_, manualCapturing_);
    } else if (manualCapturing_) {
        // 关闭手动操作时若仍在捕获, 恢复光标 (防御性)
        manualCapturing_ = false;
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        EnableCursor();
    }

    ClearBackground(kBackgroundColor);
    BeginMode3D(camera_);
    DrawGrid(20, 1.0f);
    scene_.draw(transform_, drawWires_);
    if (showBounds_) {
        scene_.drawBounds(transform_);
    }
    EndMode3D();
}

void ModelViewerGame::shutdown() {
    scene_.unload();
}

void ModelViewerGame::renderUi() {
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Phoenix")) {
        ImGui::Text("FPS: %d", GetFPS());
        ImGui::Separator();

        // ---- 模型 ----
        if (ImGui::CollapsingHeader("模型", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::InputText("路径", modelPath_, sizeof(modelPath_));
            if (ImGui::Button("加载模型")) {
                const std::string resolved = asset_path::resolve(modelPath_);
                std::string error;
                if (scene_.load(resolved, &error)) {
                    app_log::info("Model loaded: " + resolved);
                    centerModelOnGrid(transform_, scene_);
                    std::string texError;
                    if (!scene_.applyTexture(model_loader::MaterialMapType::Albedo, asset_path::resolve(texturePath_),
                                             &texError)) {
                        app_log::warn(texError);
                    }
                } else {
                    app_log::error(error);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("默认路径")) {
                std::snprintf(modelPath_, sizeof(modelPath_), "%s", kDefaultModelPath);
            }

            ImGui::InputText("贴图路径", texturePath_, sizeof(texturePath_));
            if (ImGui::Button("应用贴图")) {
                std::string texError;
                if (!scene_.applyTexture(model_loader::MaterialMapType::Albedo, asset_path::resolve(texturePath_),
                                         &texError)) {
                    app_log::error(texError);
                }
            }

            if (scene_.isLoaded()) {
                const model_loader::ModelStats stats = scene_.stats();
                ImGui::Text("网格: %d  顶点: %d  三角形: %d", stats.meshCount, stats.vertexCount, stats.triangleCount);
                ImGui::Text("材质: %d  动画: %d", stats.materialCount, stats.animationCount);
                ImGui::Separator();

                ImGui::DragFloat3("位置", transform_.position, 0.05f);
                ImGui::DragFloat3("旋转轴", transform_.rotationAxis, 0.01f);
                ImGui::SliderFloat("旋转角度", &transform_.rotationAngle, -180.0f, 180.0f);
                ImGui::DragFloat3("缩放", transform_.scale, 0.01f);
                ImGui::Checkbox("线框显示", &drawWires_);
                ImGui::Checkbox("包围盒", &showBounds_);
                ImGui::Separator();

                if (scene_.animationCount() > 0) {
                    const std::vector<std::string>& names = scene_.animationNames();
                    std::vector<const char*> items;
                    items.reserve(names.size());
                    for (const std::string& name : names) {
                        items.push_back(name.c_str());
                    }

                    int active = scene_.activeAnimation();
                    if (active < 0) {
                        active = 0;
                    }
                    if (ImGui::Combo("动画", &active, items.data(), static_cast<int>(items.size()))) {
                        scene_.setActiveAnimation(active);
                    }

                    bool playing = scene_.playing();
                    if (ImGui::Checkbox("播放", &playing)) {
                        scene_.setPlaying(playing);
                    }
                    bool loop = scene_.loop();
                    if (ImGui::Checkbox("循环", &loop)) {
                        scene_.setLoop(loop);
                    }
                    float speed = scene_.speed();
                    if (ImGui::SliderFloat("速度", &speed, 0.0f, 3.0f)) {
                        scene_.setSpeed(speed);
                    }
                    ImGui::Text("当前帧: %.1f", scene_.currentFrame());
                } else {
                    ImGui::TextDisabled("该模型不含动画数据");
                }
            } else {
                ImGui::TextDisabled("未加载模型");
            }
        }

        // ---- 相机 ----
        if (ImGui::CollapsingHeader("相机", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Checkbox("鼠标环绕控制 (移动旋转 / 滚轮缩放)", &orbitalControl_) && orbitalControl_) {
                manualControl_ = false;
            }
            if (ImGui::Checkbox("手动操作 (WASD/空格/Ctrl 移动, 右键旋转)", &manualControl_) && manualControl_) {
                orbitalControl_ = false;
            }

            ImGui::BeginDisabled(!manualControl_);
            ImGui::SliderFloat("移动速度", &cameraMoveSpeed_, 0.1f, 50.0f);
            ImGui::SliderFloat("旋转速度", &cameraRotateSpeed_, 0.0005f, 0.01f, "%.4f");
            ImGui::EndDisabled();

            // 活动相机参数编辑: 与 Lua api.camera.set_param 使用同一键集与数据源
            // (distance / target_* / up_* / fov / projection)。isometric 相机的位置由
            // distance + target 推导, 故面板不直接暴露 position (由 set_param 联动重算)。
            const camera::Params* params = cameraManager_.active_params();
            ImGui::BeginDisabled(orbitalControl_ || manualControl_ || params == nullptr);
            if (params != nullptr) {
                bool changed = false;

                float distance = params->distance;
                if (ImGui::DragFloat("距离", &distance, 0.1f)) {
                    changed |= cameraManager_.set_param_active("distance", distance);
                }

                float target[3] = {params->target[0], params->target[1], params->target[2]};
                if (ImGui::DragFloat3("目标点", target, 0.1f)) {
                    changed |= cameraManager_.set_param_active("target_x", target[0]);
                    changed |= cameraManager_.set_param_active("target_y", target[1]);
                    changed |= cameraManager_.set_param_active("target_z", target[2]);
                }

                float up[3] = {params->up[0], params->up[1], params->up[2]};
                if (ImGui::DragFloat3("Up", up, 0.01f)) {
                    changed |= cameraManager_.set_param_active("up_x", up[0]);
                    changed |= cameraManager_.set_param_active("up_y", up[1]);
                    changed |= cameraManager_.set_param_active("up_z", up[2]);
                }

                float fovy = params->fovy;
                if (ImGui::SliderFloat("FOV", &fovy, 20.0f, 120.0f)) {
                    changed |= cameraManager_.set_param_active("fov", fovy);
                }

                int projection = params->projection;
                if (ImGui::Combo("投影", &projection, "透视\0正交\0")) {
                    changed |= cameraManager_.set_param_active("projection", projection);
                }

                if (changed) {
                    camera_ = camera::to_camera3d(*cameraManager_.active_params());
                }
            } else {
                ImGui::TextDisabled("无活动相机");
            }
            ImGui::EndDisabled();
            ImGui::Separator();

            if (ImGui::Button("默认视角")) {
                resetCamera(camera_, {8.0f, 6.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f,
                            CAMERA_PERSPECTIVE);
            }
            ImGui::SameLine();
            if (ImGui::Button("前视")) {
                resetCamera(camera_, {0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f,
                            CAMERA_PERSPECTIVE);
            }
            ImGui::SameLine();
            if (ImGui::Button("后视")) {
                resetCamera(camera_, {0.0f, 0.0f, -10.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f,
                            CAMERA_PERSPECTIVE);
            }
            ImGui::SameLine();
            if (ImGui::Button("左视")) {
                resetCamera(camera_, {-10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f,
                            CAMERA_PERSPECTIVE);
            }
            ImGui::SameLine();
            if (ImGui::Button("右视")) {
                resetCamera(camera_, {10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f,
                            CAMERA_PERSPECTIVE);
            }
            if (ImGui::Button("俯视")) {
                resetCamera(camera_, {0.0f, 10.0f, 0.001f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 45.0f,
                            CAMERA_PERSPECTIVE);
            }
            ImGui::SameLine();
            if (ImGui::Button("适配模型")) {
                fitCameraToModel(camera_, scene_);
            }
        }
    }
    ImGui::End();
}
} // namespace scene::debug
