#pragma once
#include "core/base/ring_buffer.h"
#include "core/framework/application.h"
#include "editor/editor_command.h"
#include "editor/editor_window.h"
#include "editor/menu_bar.h"
#include "scene/scene.h"

namespace my {

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

    void select_entity(ecs::Entity p_selected);
    ecs::Entity get_selected_entity() const { return m_selected; }
    State get_state() const { return m_state; }
    void set_state(State p_state) { m_state = p_state; }

    uint64_t get_displayed_image() const { return m_displayed_image; }
    void set_displayed_image(uint64_t p_image) { m_displayed_image = p_image; }

    void add_component(ComponentType p_type, ecs::Entity p_target);
    void add_entity(EntityType p_type, ecs::Entity p_parent);
    void remove_entity(ecs::Entity p_target);

private:
    void dock_space(Scene& p_scene);
    void add_panel(std::shared_ptr<EditorItem> p_panel);

    void buffer_command(std::shared_ptr<EditorCommand> p_command);
    void flush_commands(Scene& p_scene);
    void undo_command(Scene& p_scene);

    std::shared_ptr<MenuBar> m_menu_bar;
    std::vector<std::shared_ptr<EditorItem>> m_panels;
    ecs::Entity m_selected;
    State m_state{ STATE_TRANSLATE };

    uint64_t m_displayed_image = 0;
    std::list<std::shared_ptr<EditorCommand>> m_command_buffer;
    RingBuffer<std::shared_ptr<EditorCommand>, 32> m_command_history;
};

}  // namespace my