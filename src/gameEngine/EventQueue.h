#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include "../lowLevelEntities/GameEvent.h"
#include <deque>
#include <vector>

class EventQueue {
private:
    std::deque<GameEvent> events;
public:
    void push(GameEvent event) {
        events.push_back(std::move(event));
    }

    bool empty() const { return events.empty(); }
    size_t size() const { return events.size(); }

    GameEvent pop() {
        GameEvent e = std::move(events.front());
        events.pop_front();
        return e;
    }

    // Get copies of events added since index 'from' (for network broadcast)
    std::vector<GameEvent> getEventsFrom(size_t from) const {
        std::vector<GameEvent> result;
        for (size_t i = from; i < events.size(); i++) {
            result.push_back(events[i]);
        }
        return result;
    }

    std::vector<GameEvent> drain() {
        std::vector<GameEvent> result;
        while (!events.empty()) {
            result.push_back(std::move(events.front()));
            events.pop_front();
        }
        return result;
    }
};

#endif
