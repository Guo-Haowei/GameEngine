#pragma once
#include "core/base/ring_buffer.h"
#include "core/framework/application.h"
#include "editor/editor_command.h"
#include "editor/editor_window.h"
#include "editor/menu_bar.h"
#include "scene/scene.h"

namespace my {

// @TODO: refactor
enum {
    DISPLAY_FXAA_IMAGE = 0,
    DISPLAY_GBUFFER_DEPTH,
    DISPLAY_GBUFFER_BASE_COLOR,
    DISPLAY_GBUFFER_NORMAL,
    DISPLAY_SSAO,
    DISPLAY_SHADOW_MAP,
};

class EditorLayer : public Layer {
public:
    enum State {
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

    void add_plane(ecs::Entity parent);
    void add_cube(ecs::Entity parent);
    void add_sphere(ecs::Entity parent);
    void add_point_light(ecs::Entity parent);
    void add_omin_light(ecs::Entity parent);

private:
    void add_object(EditorCommandName name, ecs::Entity parent);

    void dock_space(Scene& scene);
    void add_panel(std::shared_ptr<EditorWindow> panel);

    void buffer_command(std::shared_ptr<EditorCommand> command);
    void flush_commands(Scene& scene);
    void undo_command(Scene& scene);

    std::shared_ptr<MenuBar> m_menu_bar;
    std::vector<std::shared_ptr<EditorWindow>> m_panels;
    ecs::Entity m_selected;
    State m_state{ STATE_TRANSLATE };

    uint64_t m_displayed_image = 0;
    std::list<std::shared_ptr<EditorCommand>> m_command_buffer;
    RingBuffer<std::shared_ptr<EditorCommand>, 32> m_command_history;
};

}  // namespace my