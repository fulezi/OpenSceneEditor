#ifndef SOLEIL__EVENTMANAGER_H_
#define SOLEIL__EVENTMANAGER_H_

#include <functional>
#include <map>
#include <queue>
#include <string>
#include <vector>

namespace ose {

  class EventManager
  {
  public:
    typedef std::string           EventName;
    typedef std::function<void()> Callback;

    static EventManager Instance;

  public:
    EventManager();

    void enroll(EventName name, Callback callback);

    void emit(EventName name);

    void processEvents(void);

    void delay(Callback callback);

  private:
    std::map<EventName, std::vector<Callback>> registeredEvents;
    std::queue<EventName> queuedEvents;
    std::queue<Callback>  delayedEvents;
  };

} // ose

#endif /* SOLEIL__EVENTMANAGER_H_ */
