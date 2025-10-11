#include <afterhours/ah.h>
#include <fmt/format.h>

#include <afterhours/src/developer.h>
#include <afterhours/src/logging.h>

#include "../game.h"
#include "../game_state_manager.h"
#include "../input_mapping.h"
#include "../preload.h" // FontID
#include "../query.h"
#include "../settings.h"
#include "../translation_manager.h"
#include "navigation.h"

using namespace afterhours;

struct MapConfig;

using namespace afterhours::ui;
using namespace afterhours::ui::imm;
using Screen = GameStateManager::Screen;

auto height_at_720p(float value) { return screen_pct(value / 720.f); }
auto width_at_720p(float value) { return screen_pct(value / 1280.f); }

struct SetupGameStylingDefaults
    : System<afterhours::ui::UIContext<InputAction>> {

  virtual void once(float) override {
    auto &styling_defaults = afterhours::ui::imm::UIStylingDefaults::get();

    styling_defaults
        .set_theme_color(afterhours::ui::Theme::Usage::Primary,
                         afterhours::Color{139, 69, 19, 255}) // Saddle brown
        .set_theme_color(afterhours::ui::Theme::Usage::Secondary,
                         afterhours::Color{34, 139, 34, 255}) // Forest green
        .set_theme_color(afterhours::ui::Theme::Usage::Accent,
                         afterhours::Color{255, 140, 0, 255}) // Dark orange
        .set_theme_color(afterhours::ui::Theme::Usage::Background,
                         afterhours::Color{64, 64, 64, 255}) // Dark gray
        .set_theme_color(afterhours::ui::Theme::Usage::Font,
                         afterhours::Color{245, 245, 220, 255}) // Beige
        .set_theme_color(afterhours::ui::Theme::Usage::DarkFont,
                         afterhours::Color{255, 255, 255,
                                           255}); // White for dark backgrounds

    // Set the default font for all components based on current language
    styling_defaults.set_default_font(
        get_font_name(translation_manager::get_font_for_language()), 16.f);

    // Component-specific styling
    styling_defaults.set_component_config(
        ComponentType::Button,
        ComponentConfig{}
            .with_padding(Padding{.top = screen_pct(5.f / 720.f),
                                  .left = pixels(0.f),
                                  .bottom = screen_pct(5.f / 720.f),
                                  .right = pixels(0.f)})
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));

    styling_defaults.set_component_config(
        ComponentType::Slider,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Secondary));

    styling_defaults.set_component_config(
        ComponentType::Checkbox,
        ComponentConfig{}
            .with_size(ComponentSize{screen_pct(200.f / 1280.f),
                                     screen_pct(50.f / 720.f)})
            .with_color_usage(Theme::Usage::Primary));
  }
};

struct ScheduleMainMenuUI : System<afterhours::ui::UIContext<InputAction>> {
  input::PossibleInputCollector inpc;

  virtual bool should_run(float) override {
    inpc = input::get_input_collector();
    return GameStateManager::get().is_menu_active();
  }

  virtual void for_each_with(Entity &entity, UIContext<InputAction> &context,
                             float) override {
    GameStateManager::get().update_screen();

    switch (GameStateManager::get().active_screen) {
    case Screen::Main:
      GameStateManager::get().active_screen = main_screen(entity, context);
      break;
    case Screen::Settings:
      GameStateManager::get().active_screen = settings_screen(entity, context);
      break;
    case Screen::None:
    default:
      GameStateManager::get().active_screen =
          GameStateManager::get().active_screen;
      break;
    }
  }

  Screen main_screen(Entity &entity, UIContext<InputAction> &context);
  Screen settings_screen(Entity &entity, UIContext<InputAction> &context);

  void exit_game() { running = false; }
};

// Reusable UI component functions
namespace ui_helpers {

// Reusable styled button component
ElementResult create_styled_button(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   std::function<void()> on_click,
                                   int index = 0) {

  if (imm::button(context, mk(parent, index),
                  ComponentConfig{}
                      .with_debug_name(label)
                      .with_padding(Padding{.top = pixels(5.f),
                                            .left = pixels(0.f),
                                            .bottom = pixels(5.f),
                                            .right = pixels(0.f)})
                      .with_label(label))) {
    on_click();
    return {true, parent};
  }

  return {false, parent};
}

// Reusable volume slider component
ElementResult create_volume_slider(UIContext<InputAction> &context,
                                   Entity &parent, const std::string &label,
                                   float &volume,
                                   std::function<void(float)> on_change,
                                   int index = 0) {

  if (auto result = slider(context, mk(parent, index), volume,
                           ComponentConfig{}.with_label(label).with_padding(
                               Padding{.top = pixels(5.f),
                                       .left = pixels(0.f),
                                       .bottom = pixels(5.f),
                                       .right = pixels(0.f)}),
                           SliderHandleValueLabelPosition::OnHandle)) {
    volume = result.as<float>();
    on_change(volume);
    return {true, parent};
  }

  return {false, parent};
}

// Reusable screen container component
ElementResult create_screen_container(UIContext<InputAction> &context,
                                      Entity &parent,
                                      const std::string &debug_name) {

  return imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
          .with_absolute_position()
          .with_debug_name(debug_name));
}

// Reusable control group component
ElementResult create_control_group(UIContext<InputAction> &context,
                                   Entity &parent,
                                   const std::string &debug_name) {

  return imm::div(
      context, mk(parent),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(0.2f), screen_pct(1.f)})
          .with_padding(
              Padding{.top = screen_pct(0.02f), .left = screen_pct(0.02f)})
          .with_flex_direction(FlexDirection::Column)
          .with_debug_name(debug_name));
}

// Reusable top-left container component
ElementResult create_top_left_container(UIContext<InputAction> &context,
                                        Entity &parent,
                                        const std::string &debug_name,
                                        int index = 0) {

  return imm::div(
      context, mk(parent, index),
      ComponentConfig{}
          .with_size(ComponentSize{screen_pct(0.2f), screen_pct(1.f)})
          .with_padding(
              Padding{.top = screen_pct(0.02f), .left = screen_pct(0.02f)})
          .with_flex_direction(FlexDirection::Column)
          .with_debug_name(debug_name));
}

} // namespace ui_helpers

Screen ScheduleMainMenuUI::main_screen(Entity &entity,
                                       UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "main_screen");

  // Add a background
  auto bg =
      imm::div(context, mk(elem.ent()),
               ComponentConfig{}
                   .with_size(ComponentSize{screen_pct(1.f), screen_pct(1.f)})
                   .with_color_usage(Theme::Usage::Background)
                   .with_debug_name("main_background")
                   .with_rounded_corners(RoundedCorners().all_sharp()));

  auto top_left = ui_helpers::create_top_left_container(context, bg.ent(),
                                                        "main_top_left", 0);

  // Play button
  ui_helpers::create_styled_button(
      context, top_left.ent(), "Play",
      []() { GameStateManager::get().start_game(); }, 0);

  // Settings button
  ui_helpers::create_styled_button(
      context, top_left.ent(), "Settings",
      []() { navigation::to(GameStateManager::Screen::Settings); }, 1);

  // Exit button
  ui_helpers::create_styled_button(
      context, top_left.ent(), "Quit", [this]() { exit_game(); }, 2);

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

Screen ScheduleMainMenuUI::settings_screen(Entity &entity,
                                           UIContext<InputAction> &context) {
  auto elem =
      ui_helpers::create_screen_container(context, entity, "settings_screen");
  auto top_left = ui_helpers::create_top_left_container(context, elem.ent(),
                                                        "settings_top_left", 0);
  {
    ui_helpers::create_styled_button(
        context, top_left.ent(), "Back",
        []() {
          Settings::get().update_resolution(
              EntityHelper::get_singleton_cmp<
                  window_manager::ProvidesCurrentResolution>()
                  ->current_resolution);
          navigation::back();
        },
        0);
  }

  // Master volume slider
  {
    float master_volume = Settings::get().get_master_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(), "Master Volume", master_volume,
        [](float volume) { Settings::get().update_master_volume(volume); }, 1);
  }

  // Music volume slider
  {
    float music_volume = Settings::get().get_music_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(), "Music Volume", music_volume,
        [](float volume) { Settings::get().update_music_volume(volume); }, 2);
  }

  // SFX volume slider
  {
    float sfx_volume = Settings::get().get_sfx_volume();
    ui_helpers::create_volume_slider(
        context, top_left.ent(), "SFX Volume", sfx_volume,
        [](float volume) { Settings::get().update_sfx_volume(volume); }, 3);
  }

  // Fullscreen checkbox
  if (imm::checkbox(context, mk(top_left.ent(), 4),
                    Settings::get().get_fullscreen_enabled(),
                    ComponentConfig{}
                        .with_label("Fullscreen")
                        .with_padding(Padding{.top = pixels(5.f),
                                              .left = pixels(0.f),
                                              .bottom = pixels(5.f),
                                              .right = pixels(0.f)}))) {
    Settings::get().toggle_fullscreen();
  }

  // Post Processing checkbox
  if (imm::checkbox(context, mk(top_left.ent(), 5),
                    Settings::get().get_post_processing_enabled(),
                    ComponentConfig{}
                        .with_label("Post Processing")
                        .with_padding(Padding{.top = pixels(5.f),
                                              .left = pixels(0.f),
                                              .bottom = pixels(5.f),
                                              .right = pixels(0.f)}))) {
    Settings::get().toggle_post_processing();
  }

  return GameStateManager::get().next_screen.value_or(
      GameStateManager::get().active_screen);
}

void register_ui_systems(afterhours::SystemManager &systems) {
  ui::register_before_ui_updates<InputAction>(systems);
  {
    systems.register_update_system(
        std::make_unique<SetupGameStylingDefaults>());
    systems.register_update_system(std::make_unique<NavigationSystem>());
    systems.register_update_system(std::make_unique<ScheduleMainMenuUI>());
  }
  ui::register_after_ui_updates<InputAction>(systems);
}

void enforce_ui_singletons(afterhours::SystemManager &systems) {
  systems.register_update_system(
      std::make_unique<
          afterhours::developer::EnforceSingleton<MenuNavigationStack>>());
}

void add_ui_singleton_components(afterhours::Entity &entity) {
  entity.addComponent<MenuNavigationStack>();
  afterhours::EntityHelper::registerSingleton<MenuNavigationStack>(entity);
}
