#include "event_queue.h"

namespace my {

void EventQueue::EnqueueEvent(std::shared_ptr<IEvent> p_event) {
    m_events.push(p_event);
}

void EventQueue::DispatchEvent(std::shared_ptr<IEvent> p_event) {
    for (auto& listener : m_listeners) {
        listener->EventReceived(p_event);
    }
}

void EventQueue::FlushEvents() {
    while (!m_events.empty()) {
        auto event = m_events.front();
        DispatchEvent(event);
        m_events.pop();
    }
}

void EventQueue::RegisterListener(EventListener* p_listener) {
    m_listeners.push_back(p_listener);
}

void EventQueue::UnregisterListener(EventListener* p_listener) {
    auto it = std::find(m_listeners.begin(), m_listeners.end(), p_listener);
    if (it != m_listeners.end()) {
        m_listeners.erase(it);
    }
}

}  // namespace my
