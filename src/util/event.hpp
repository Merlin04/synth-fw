#pragma once

#include <forward_list>

template<typename TMessage, typename TListener>
class EventBase {
protected:
    std::forward_list<TListener> listeners;
public:
    void add_listener(TListener listener) {
        listeners.push_front(listener);
    }

    void remove_listener(TListener listener) {
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
        VoidEventBase<TMessage>::listeners.front()(message);
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
        for(auto& listener : BubblingEventBase<TMessage>::listeners) {
            if(listener(message)) {
                break;
            }
        }
    }
};