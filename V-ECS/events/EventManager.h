#pragma once

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <iostream>

namespace vecs {

	struct EventData {};

	// The Event Manager handles receiving events and passing
	// them onto any subscribers that exist for that event
	class EventManager {
	public:
		// Pass an instance of an object and a member function on that object to call
		// The member function must return void and take a single parameter, a subclass of EventData
		// that represents the event being subscribed to. The subclass may contain extra information
		// specific to this event that can be used inside the event listener
		template <class E, class T,
			// Enforces that E is a subclass of EventData
			typename = std::enable_if_t<std::is_base_of_v<EventData, E>>>
		static void addListener(T* const object, void(T::* const mf)(E*)) {
			// The following would require the passed function to accept explicity EventData* as its parameter,
			// but I wanted it to be able to be a subclass. Since I use the subclass' typeinfo as the key
			// in my listeners map, I know the event will always be of the right subclass, but can't figure
			// out how to tell the compiler that with a compiler assertion or otherwise
			//listeners[event].emplace_back(std::bind(mf, object, std::placeholders::_1));
			// So this is probably a little less efficient,
			// but I create a lambda for each listener that does the pointer conversion
			auto cb = std::bind(mf, object, std::placeholders::_1);
			listeners[typeid(E)].emplace_back([cb](EventData* event) { cb(static_cast<E*>(event)); });
		}

		// Send an event, represented by an instance of a struct deriving from EventData,
		// to any listeners for that event
		template <class T,
			// Enforces that T is a subclass of EventData
			typename = std::enable_if_t<std::is_base_of_v<EventData, T>>>
		static void fire(T event) {
			for (auto callback : listeners[typeid(T)]) {
				callback(&event);
			}
		}

	private:
		static std::unordered_map<std::type_index, std::vector<std::function<void(EventData*)>>> listeners;
	};
}
