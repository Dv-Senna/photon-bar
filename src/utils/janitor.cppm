module;

#include <utility>

export module photon.utils.janitor;


namespace photon::utils {
	export template <typename Callback>
	class Janitor final {
		public:
			constexpr Janitor(Callback&& callback) noexcept :
				m_callback {std::forward<Callback> (callback)}
			{}
			constexpr ~Janitor() noexcept {
				m_callback();
			}

		private:
			Callback m_callback;
	};
}
