#include <std_include.hpp>
#include "game.hpp"

#include <utils/flags.hpp>

namespace game
{
	uint64_t base_address;

	void load_base_address()
	{
		const auto module = GetModuleHandle(NULL);
		base_address = uint64_t(module);
	}

	namespace environment
	{
		bool is_dedi()
		{
			static const auto dedicated = utils::flags::has_flag("dedicated");
			return dedicated;
		}
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
