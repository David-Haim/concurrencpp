#ifndef CONCURRENCPP_SCOPE_GUARD_H
#define CONCURRENCPP_SCOPE_GUARD_H

namespace concurrencpp::details {

	template<class cleaner_type>
	class scope_guard {

	private:
		cleaner_type m_cleaner;

	public:
		template<class given_cleaner_type>
		scope_guard(given_cleaner_type&& cleaner) : m_cleaner(std::forward<given_cleaner_type>(cleaner)) {}
		~scope_guard() noexcept { m_cleaner(); }
	};

	template<class cleaner_type>
	auto make_scope_guard(cleaner_type&& cleaner) {
		return scope_guard<typename std::decay_t<cleaner_type>>(std::forward<cleaner_type>(cleaner));
	}

}

#endif