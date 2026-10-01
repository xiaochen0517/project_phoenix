#include "app/app.h"
#include "app/model_viewer_game.h"
#include "log/app_log.h"

#include <memory>

int main() {
    app_log::init();

    app::App app(std::make_unique<app::ModelViewerGame>());
    return app.run();
}
