
#include "EventManager.hpp"

namespace ose {

  EventManager::EventManager() {}

  void EventManager::enroll(EventName name, Callback callback)
  {
    registeredEvents[name].push_back(callback);
  }

  void EventManager::emit(EventName name) { queuedEvents.push(name); }

  void EventManager::delay(Callback callback)
  {
    delayedEvents.push(callback);
  }

  
  void EventManager::processEvents(void)
  {
    while (queuedEvents.size() > 0) {
      EventName e = queuedEvents.front();
      if (registeredEvents.count(e) > 0) {
        auto list = registeredEvents.at(e);
        for (const auto& cb : list) {
          cb();
        }
      }
      queuedEvents.pop();
    }

    while (delayedEvents.size() > 0)
      {
	auto call = delayedEvents.front();
	call();
	delayedEvents.pop();
      }
  }

  EventManager EventManager::Instance;
  
} // ose
