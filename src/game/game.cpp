#include <std_include.hpp>
#include "game.hpp"

#include <identification/game.hpp>

#include <utils/flags.hpp>
#include <utils/string.hpp>

namespace game
{
	uint64_t base_address;

	void load_base_address()
	{
		const auto module = GetModuleHandle(NULL);
		base_address = uint64_t(module);
	}

	DvarValue* get_current(game::dvar_t* dvar) {
		if (!dvar) return nullptr;

		if (identification::game::is("1.20.4-replay")) {
			return &dvar->current;
		}

		// ship has a different dvar layout, just lazy cast the correct type
		auto test = reinterpret_cast<game::dvar_t_ship*>(dvar);
		return &test->current;
	}

	bool dvar_is_enabled_safe(game::dvar_t* dvar)
	{
		if (!dvar)
			return false;

		auto current = get_current(dvar);
		return current->enabled;
	}

	// meme
	dvar_t* Dvar_RegisterString(const char* name, const char* str, game::DvarFlags flags, const char* desc)
	{
		game::DvarLimits domain{};
		game::DvarValue value{};
		value.string = utils::memory::duplicate_string(str); // not sure if needed lmao
		return game::Dvar_RegisterVariant(name, utils::string::dvar_checksum(name), 9, flags, &value, &domain, desc);
	}

	int Cmd_Argc()
	{
		static const auto game_ = identification::game::get_target_game().client_name;
		if (game_ == "iw8-mod"s)
			return game::Cmd_Argc_internal();
		
		return cmd_args->argc[cmd_args->nesting];
	}

	const char* Cmd_Argv(int argIndex)
	{
		static const auto game_ = identification::game::get_target_game().client_name;
		if (game_ == "iw8-mod"s)
			return game::Cmd_Argv_internal(argIndex);
		
		if (argIndex >= cmd_args->argc[cmd_args->nesting])
			return nullptr;
		else
			return cmd_args->argv[cmd_args->nesting][argIndex];
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
