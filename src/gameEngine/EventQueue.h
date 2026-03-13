#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include "../lowLevelEntities/GameEvent.h"
#include <queue>
#include <vector>

class EventQueue {
private:
    std::queue<GameEvent> events;
public:
    void push(GameEvent event) {
        events.push(std::move(event));
    }

    bool empty() const { return events.empty(); }

    GameEvent pop() {
        GameEvent e = std::move(events.front());
        events.pop();
        return e;
    }

    std::vector<GameEvent> drain() {
        std::vector<GameEvent> result;
        while (!events.empty()) {
            result.push_back(std::move(events.front()));
            events.pop();
        }
        return result;
    }
};

#endif
