#pragma once

#include <concepts>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <type_traits>

#include <flex/core/typeTraits.hpp>

#include "utils/utils.hpp"


namespace photon {
	template <typename Value>
	concept event_value = std::same_as<Value, std::remove_cvref_t<Value>>
		&& std::move_constructible<Value>
		&& std::is_move_assignable_v<Value>;

	template <typename Key>
	concept event_key = std::is_scoped_enum_v<Key>;

	template <event_key Key>
	struct EventBase {
		Key key;
		std::size_t queueId;
		std::size_t uuid;
	};

	template <event_key auto key, event_value Value>
	struct Event : EventBase<decltype(key)> {
		constexpr Event(std::size_t queueId, std::size_t uuid, flex::forward_of<Value> auto&& value) noexcept :
			photon::EventBase<decltype(key)> {
				.key = key,
				.queueId = queueId,
				.uuid = uuid
			},
			value {std::forward<decltype(value)> (value)}
		{}
		Value value;
	};

	namespace internals::events {
		template <event_key key, typename T>
		struct is_event_of_key : std::false_type {};

		template <event_key Key, Key key, event_value Value>
		struct is_event_of_key<Key, Event<key, Value>> : std::true_type {};

		template <typename T, typename Key>
		concept event_of_key = event_key<Key> && is_event_of_key<Key, std::remove_cvref_t<T>>::value;

		template <event_key Key, event_of_key<Key> T>
		struct get_event_key;
		template <event_key Key, Key key, event_value Value>
		struct get_event_key<Key, Event<key, Value>> {
			static constexpr auto value = key;
		};

		template <event_key Key, event_of_key<Key> T>
		struct get_event_value;
		template <event_key Key, Key key, event_value Value>
		struct get_event_value<Key, Event<key, Value>> {
			using type = Value;
		};


		template <event_key Key, Key key, event_of_key<Key>... Events>
		struct has_key : std::false_type {};
		template <event_key Key, Key key, event_of_key<Key> FirstEvent, event_of_key<Key>... Events>
		struct has_key<Key, key, FirstEvent, Events...> : std::bool_constant<
			get_event_key<Key, FirstEvent>::value == key || has_key<Key, key, Events...>::value
		> {};

		template <event_key Key, event_of_key<Key>... Events>
		struct has_key_duplicate : std::false_type {};
		template <event_key Key, event_of_key<Key> FirstEvent, event_of_key<Key>... Events>
		struct has_key_duplicate<Key, FirstEvent, Events...> : std::bool_constant<
			has_key<Key, get_event_key<Key, FirstEvent>::value, Events...>::value
			|| has_key_duplicate<Key, Events...>::value
		> {};

		template <event_key Key, Key key, event_of_key<Key>... Events>
		struct get_value_from_key {
			using type = void;
		};
		template <event_key Key, Key key, event_of_key<Key> FirstEvent, event_of_key<Key>... Events>
		struct get_value_from_key<Key, key, FirstEvent, Events...> {
			using type = std::conditional_t<
				get_event_key<Key, FirstEvent>::value == key,
				typename get_event_value<Key, FirstEvent>::type,
				typename get_value_from_key<Key, key, Events...>::type
			>;
		};

		template <typename Visitor, typename Key, typename Event>
		concept visitor_with_specific_event = requires(Visitor visitor, Event event) {
			{visitor.handle(std::move(event))} -> std::same_as<void>;
			requires noexcept(visitor.handle(std::move(event)));
		}
			&& event_key<Key>
			&& event_of_key<Event, Key>;

		template <typename Visitor, typename Key, typename... Events>
		concept visitor_of_events = (visitor_with_specific_event<
				Visitor,
				Key,
				Events
			> && ...)
			&& event_key<Key>
			&& (event_of_key<Events, Key> && ...);
	}


	template <event_key Key, internals::events::event_of_key<Key>... Events>
	requires (!internals::events::has_key_duplicate<Key, Events...>::value)
	class EventQueue final {
		template <Key key>
		using value_from_key = typename internals::events::get_value_from_key<Key, key, Events...>::type;
		public:
			EventQueue(std::string_view name) noexcept :
				m_name {name},
				m_id {},
				m_uuid {0uz},
				m_mutex {},
				m_conditionalVariable {},
				m_eventQueue {}
			{
				static std::size_t id {0uz};
				m_id = id++;
			}
			~EventQueue() noexcept = default;
			EventQueue(const EventQueue&) = delete;
			auto operator=(const EventQueue&) -> EventQueue& = delete;
			constexpr EventQueue(EventQueue&&) noexcept = default;
			auto operator=(EventQueue&&) -> EventQueue& = delete;

			constexpr auto getId() const noexcept -> std::size_t {
				return m_id;
			}

			template <Key key>
			auto push(flex::forward_of<value_from_key<key>> auto&& value) noexcept -> void {
				{
					std::scoped_lock<std::mutex> _ {m_mutex};
					m_eventQueue.push(std::make_unique<Event<key, value_from_key<key>>> (
						m_id, m_uuid++, std::forward<decltype(value)> (value)
					));
				}
				m_conditionalVariable.notify_one();
			}

			auto waitOnEvent(internals::events::visitor_of_events<Key, Events...>auto& visitor) noexcept -> void {
				std::unique_lock<std::mutex> lock {m_mutex};
				m_conditionalVariable.wait(lock, [this]{return m_eventQueue.size() != 0uz;});
				std::unique_ptr<EventBase<Key>> event {std::move(m_eventQueue.front())};
				m_eventQueue.pop();
				[&event, &visitor] <std::size_t I = 0> (this const auto& self) {
					static constexpr Key eventKey {internals::events::get_event_key<Key, Events...[I]>::value};
					if (event->key == eventKey) {
						return visitor.handle(
							std::move(static_cast<Event<eventKey, value_from_key<eventKey>>&> (*event))
						);
					}
					if constexpr (I + 1 < sizeof...(Events))
						self.template operator() <I + 1> ();
				} ();
			}

		private:
			constexpr EventQueue() noexcept = default;

			std::string m_name;
			std::size_t m_id;
			std::size_t m_uuid;
			std::mutex m_mutex;
			std::condition_variable m_conditionalVariable;
			std::queue<std::unique_ptr<EventBase<Key>>> m_eventQueue;
	};
}
