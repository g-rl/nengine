#pragma once
#include "game/game.hpp"

namespace scripting
{
	using script_function = void(*)(game::scr_entref_t);

	std::string find_token(std::uint64_t id);
	std::string find_token_single(std::uint64_t id);
}
