#include "ecs/ecs.h"

namespace ecs {
void SystemManager::add(System system) {
    systems_.push_back(std::move(system));
}

void SystemManager::update(Registry& registry, float dt) {
    for (const System& system : systems_) {
        system(registry, dt);
    }
}
} // namespace ecs
