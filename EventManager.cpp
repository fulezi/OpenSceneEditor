/*
 * Copyright (C) 2017  Florian GOLESTIN
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "EventManager.h"

#include <cassert>

namespace Soleil {

  void EventManager::enroll(const EventType& eventType, EventListener listener)
  {
    EventListenerList& listeners = registeredListeners[eventType];

    listeners.push_back(listener);
  }

  void EventManager::emit(EventPtr event)
  {
    queues[activeQueue].push_front(event);
  }

  void EventManager::processEvents(void)
  {
    // Swap the acrive queue and clear it
    EventQueue& queuedEvents = queues[activeQueue];
    activeQueue              = (activeQueue + 1) % queues.size();
    queues[activeQueue].clear();

    while (queuedEvents.size() > 0) {
      EventPtr e = queuedEvents.front();
      queuedEvents.pop_front();

      auto findIt = registeredListeners.find(e->getType());
      if (findIt != registeredListeners.end()) {
        EventListenerList& listeners = findIt->second;
        for (EventListener listener : listeners) {
          listener(*e);
        }
      }
    }
  }

  static std::unique_ptr<EventManager> mainEventManager;

  void EventManager::Init(void)
  {
    mainEventManager = std::make_unique<EventManager>();
  }

  void EventManager::Enroll(const EventType& eventType, EventListener listener)
  {
    assert(mainEventManager && "Event Manager not initialized");

    mainEventManager->enroll(eventType, listener);
  }

  void EventManager::Emit(EventPtr event)
  {
    assert(mainEventManager && "Event Manager not initialized");

    mainEventManager->emit(event);
  }
  
  void EventManager::ProcessEvents(void)
  {
    assert(mainEventManager && "Event Manager not initialized");

    mainEventManager->processEvents();
  }

} // Soleil
