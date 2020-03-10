#ifndef CONCURRENCPP_FAST_RANDOMIZER_H
#define CONCURRENCPP_FAST_RANDOMIZER_H

#include <chrono>

namespace concurrencpp::tests {
	class random {

	private:
		int64_t m_state;

		inline static size_t time_from_epoch() noexcept {
			return std::chrono::duration_cast<std::chrono::seconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		}

		inline static int64_t xor_shift_64(int64_t i) noexcept {
			i ^= (i << 21);
			i ^= (i >> 35);
			i ^= (i << 4);
			return i;
		}

	public:
		random() noexcept : m_state(static_cast<int64_t>(time_from_epoch() | 0xCAFEBABE)) {}

		int64_t operator()() {
			const auto res = m_state;
			m_state = xor_shift_64(m_state);
			return res;
		}

		int64_t operator()(int64_t min, int64_t max) {
			auto r = (*this)();
			auto upper_limit = max - min + 1;
			return std::abs(r) % upper_limit + min;
		}
	};
}



#endif //CONCURRENCPP_FAST_RANDOMIZER_H
