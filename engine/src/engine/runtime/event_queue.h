#pragma once

namespace my {

class EventListener;
class Scene;

class IEvent {
public:
    IEvent() {}

    virtual ~IEvent() = default;
};

class SceneChangeEvent : public IEvent {
public:
    SceneChangeEvent(Scene* scene) : m_scene(scene) {}

    const Scene* GetScene() const { return m_scene; }

protected:
    Scene* m_scene;
};

class ResizeEvent : public IEvent {
public:
    ResizeEvent(int width, int height) : m_width(width), m_height(height) {}

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

protected:
    int m_width;
    int m_height;
};

class EventListener {
public:
    virtual ~EventListener() {}
    virtual void EventReceived(std::shared_ptr<IEvent> p_event) = 0;
};

class EventQueue {
public:
    void EnqueueEvent(std::shared_ptr<IEvent> p_event);

    void DispatchEvent(std::shared_ptr<IEvent> p_event);

    void FlushEvents();

    void RegisterListener(EventListener* p_listener);

    void UnregisterListener(EventListener* p_listener);

private:
    std::queue<std::shared_ptr<IEvent>> m_events;
    std::vector<EventListener*> m_listeners;
};

}  // namespace my
