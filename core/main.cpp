#include "app/app.h"
#include "app/scene_host.h"
#include "log/app_log.h"
#include "scene/debug/model_viewer_game.h"
#include "scene/main_game/demo_scene.h"

#include <memory>

int main() {
    app_log::init();

    // 组装场景切换载体: 默认场景 = 第一个注册的场景 (main_game/demo)。
    auto host = std::make_unique<app::SceneHost>();
    host->add("main_game/demo", std::make_unique<scene::main_game::DemoScene>());
    host->add("debug/model_viewer", std::make_unique<scene::debug::ModelViewerGame>());

    app::App app(std::move(host));
    return app.run();
}
