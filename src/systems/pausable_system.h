#pragma once

#include "../game_state_manager.h"
#include <afterhours/ah.h>

template <typename... Components>
struct PausableSystem : afterhours::System<Components...> {
  virtual bool should_run(float) override {
    return !GameStateManager::get().is_paused();
  }
};
