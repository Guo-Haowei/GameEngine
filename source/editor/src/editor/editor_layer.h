#pragma once
#include "core/base/noncopyable.h"
#include "core/framework/application.h"
#include "panels/menu_bar.h"
#include "panels/panel.h"
#include "scene/scene.h"

namespace my {

enum {
    DISPLAY_FXAA_IMAGE = 0,
    DISPLAY_GBUFFER_DEPTH,
    DISPLAY_GBUFFER_BASE_COLOR,
    DISPLAY_GBUFFER_NORMAL,
    DISPLAY_SSAO,
    DISPLAY_SHADOW_MAP,
};

class EditorLayer : public Layer, public NonCopyable {
public:
    enum State {
        STATE_PICKING,
        STATE_TRANSLATE,
        STATE_ROTATE,
        STATE_SCALE,
    };

    EditorLayer();

    void attach() override {}
    void update(float dt) override;
    void render() override;

    void select_entity(ecs::Entity selected);
    ecs::Entity get_selected_entity() const { return m_selected; }
    State get_state() const { return m_state; }
    void set_state(State state) { m_state = state; }

    uint64_t get_displayed_image() const { return m_displayed_image; }
    void set_displayed_image(uint64_t p_image) { m_displayed_image = p_image; }

private:
    void dock_space(Scene& scene);
    void add_panel(std::shared_ptr<Panel> panel);

    std::shared_ptr<MenuBar> m_menu_bar;
    std::vector<std::shared_ptr<Panel>> m_panels;
    ecs::Entity m_selected;
    State m_state{ STATE_PICKING };

    uint64_t m_displayed_image = 0;
};

}  // namespace my