#include <std_include.hpp>
#include "game.hpp"

#include <identification/game.hpp>

#include <utils/flags.hpp>

namespace game
{
	uint64_t base_address;

	void load_base_address()
	{
		const auto module = GetModuleHandle(NULL);
		base_address = uint64_t(module);
	}

	DvarValue& get_current(game::dvar_t* dvar) {
		if (identification::game::is("1.20.4-replay")) {
			return dvar->current;
		}

		// ship has a different dvar layout, just lazy cast the correct type
		auto test = reinterpret_cast<game::dvar_t_ship*>(dvar);
		return test->current;
	}

	bool dvar_is_enabled_safe(game::dvar_t* dvar)
	{
		if (!dvar)
			return false;

		auto current = get_current(dvar);
		return current.enabled;
	}
}

size_t operator"" _b(const size_t ptr)
{
	return game::base_address + ptr;
}

size_t reverse_b(const size_t ptr)
{
	return ptr - game::base_address;
}

size_t reverse_b(const void* ptr)
{
	return reverse_b(reinterpret_cast<size_t>(ptr));
}
