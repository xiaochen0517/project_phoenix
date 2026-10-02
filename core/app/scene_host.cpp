#include "app/scene_host.h"

#include "imgui.h"
#include "log/app_log.h"
#include "raylib.h"
#include "rlImGui.h"

#include <cstddef>
#include <string>

namespace app {
SceneHost::SceneHost() = default;

bool SceneHost::add(const std::string& name, std::unique_ptr<Game> scene) {
    if (!scene) {
        app_log::error("SceneHost::add: null scene for '" + name + "'");
        return false;
    }
    if (find(name) != nullptr) {
        app_log::error("SceneHost::add: duplicate scene name '" + name + "'");
        return false;
    }
    scenes_.push_back(Entry{name, std::move(scene), false});
    return true;
}

SceneHost::Entry* SceneHost::find(const std::string& name) {
    for (Entry& entry : scenes_) {
        if (entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}

const SceneHost::Entry* SceneHost::find(const std::string& name) const {
    for (const Entry& entry : scenes_) {
        if (entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}

bool SceneHost::switch_to(const std::string& name) {
    Entry* entry = find(name);
    if (entry == nullptr) {
        app_log::error("SceneHost::switch_to: unknown scene '" + name + "'");
        return false;
    }
    // 懒初始化: 首次切换到目标场景时才 init。
    if (!entry->initialized) {
        if (!entry->scene->init()) {
            app_log::error("SceneHost::switch_to: scene init failed '" + name + "'");
            return false;
        }
        entry->initialized = true;
    }
    active_ = name;
    return true;
}

const std::string& SceneHost::active_name() const {
    return active_;
}

bool SceneHost::init() {
    if (scenes_.empty()) {
        app_log::error("SceneHost::init: no scenes registered");
        return false;
    }
    // 默认场景 = 第一个注册的场景 (main.cpp 组装顺序保证 demo 在前)。
    return switch_to(scenes_.front().name);
}

void SceneHost::update(float fixedDt) {
    const Entry* entry = find(active_);
    if (entry != nullptr) {
        entry->scene->update(fixedDt);
    }
}

void SceneHost::render(float alpha) {
    Entry* entry = find(active_);
    if (entry == nullptr) {
        return;
    }

    BeginDrawing();
    entry->scene->render(alpha);
    rlImGuiBegin();
    renderSceneSelector();
    entry->scene->renderUi();
    rlImGuiEnd();
    EndDrawing();
}

void SceneHost::renderUi() {
    // App 只调用 render()（render 内部已绘制选择器 + 活动场景 UI），本方法不单独使用。
}

void SceneHost::shutdown() {
    for (Entry& entry : scenes_) {
        if (entry.initialized) {
            entry.scene->shutdown();
        }
    }
}

void SceneHost::renderSceneSelector() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("场景")) {
            std::string previousCategory;
            for (const Entry& entry : scenes_) {
                const std::size_t slash = entry.name.find('/');
                const std::string category = slash == std::string::npos ? std::string() : entry.name.substr(0, slash);
                if (category != previousCategory) {
                    if (!previousCategory.empty()) {
                        ImGui::Separator();
                    }
                    if (!category.empty()) {
                        ImGui::TextDisabled("%s", category.c_str());
                    }
                    previousCategory = category;
                }
                if (ImGui::MenuItem(entry.name.c_str(), nullptr, entry.name == active_)) {
                    switch_to(entry.name);
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
} // namespace app
