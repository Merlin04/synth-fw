#pragma once

#include <forward_list>

//#define EVENT_DEBUG

template<typename TMessage, typename TListener>
class EventBase {
protected:
    std::forward_list<TListener> listeners;
public:
    void add_listener(TListener listener) {
#ifdef EVENT_DEBUG
        Serial.println("EventBase: adding listener");
#endif
        listeners.push_front(listener);
    }

    void remove_listener(TListener listener) {
#ifdef EVENT_DEBUG
        Serial.println("EventBase: removing listener");
#endif
        listeners.remove(listener);
    }

    virtual void emit(TMessage& message) = 0;
};

template<typename TMessage>
using VoidEventBase = EventBase<TMessage, void (*)(TMessage&)>;

template<typename TMessage>
class BroadcastEvent : public VoidEventBase<TMessage> {
public:
    void emit(TMessage& message) {
        if(VoidEventBase<TMessage>::listeners.empty()) return;
#ifdef EVENT_DEBUG
        Serial.println("BroadcastEvent: emitting");
#endif
        for(auto& listener : VoidEventBase<TMessage>::listeners) {
            listener(message);
        }
    }
};

// "dispatcher event" - keep a stack of handlers for some event, and when emitted,
// just call the handler on the top of the stack
template<typename TMessage>
class DispatcherEvent : public VoidEventBase<TMessage> {
public:
    void emit(TMessage& message) {
        if(!VoidEventBase<TMessage>::listeners.empty()) {
#ifdef EVENT_DEBUG
            Serial.println("DispatcherEvent: there is a listener, calling it");
#endif
            auto front = VoidEventBase<TMessage>::listeners.front();
            front(message);
        } else {
#ifdef EVENT_DEBUG
            Serial.println("DispatcherEvent: no listener");
#endif
        }
    }
};

// "bubbling event" - keep a stack of handlers for some event, and when emitted,
// traverse the listener stack from top to bottom calling the listeners and stop
// when a listener returns true
template<typename TMessage>
using BubblingEventBase = EventBase<TMessage, bool (*)(TMessage&)>;

template<typename TMessage>
class BubblingEvent : public BubblingEventBase<TMessage> {
public:
    void emit(TMessage& message) {
        if(BubblingEventBase<TMessage>::listeners.empty()) return;
        for(auto& listener : BubblingEventBase<TMessage>::listeners) {
#ifdef EVENT_DEBUG
            Serial.println("BubblingEvent: calling listener");
#endif
            if(listener(message)) {
#ifdef EVENT_DEBUG
                Serial.println("BubblingEvent: listener returned true, stopping");
#endif
                break;
            }
        }
    }
};