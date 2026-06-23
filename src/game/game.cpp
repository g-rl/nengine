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
		if (!dvar) 
			return nullptr;

		static const auto& game_ = identification::game::get_target_game().client_name;

		if (game_ == "s4-mod"s)
		{
			auto dvar_ship_S4 = reinterpret_cast<game::dvar_t_S4*>(dvar);
			return &dvar_ship_S4->current;
		}

		// IW9 ship
		if (game_ == "iw9-mod"s)
		{
			auto dvar_ship_IW9 = reinterpret_cast<game::dvar_t_IW9*>(dvar);
			return &dvar_ship_IW9->current;
		}

		// IW8 1.20.4-replay dev
		if (identification::game::is("1.20.4-replay")) {
			return &dvar->current;
		}

		// IW8 ship
		auto dvar_ship = reinterpret_cast<game::dvar_t_ship*>(dvar);
		return &dvar_ship->current;
	}

	bool dvar_is_enabled_safe(game::dvar_t* dvar)
	{
		if (!dvar)
			return false;

		auto current = get_current(dvar);
		if (!current)
			return false;

		return current->enabled;
	}

	// meme
	dvar_t* Dvar_RegisterString(const char* name, const char* str, game::DvarFlags flags, const char* desc)
	{
		game::DvarLimits domain{};
		game::DvarValue value{};
		value.string = utils::memory::duplicate_string(str); // not sure if needed lmao

		static const auto& game_ = identification::game::get_target_game().client_name;
		if (game_ == "s4-mod"s)
		{
			auto hash = game::hash_scr_dvar(name);
			printf("registering \"%s\" with hash %llx\n", name, hash);
			auto res = game::Dvar_RegisterVariant_IW9(hash, 9, flags, &value, &domain, desc);

			/*
			if (res)
			{
				auto bruh = get_current(res);
				printf("current value: %s\n", bruh->string);
			}
			else
				printf("failed to register dvar \"%s\"!\n", name);
			*/

			return res;
		}
		else if (game_ == "iw9-mod"s)
		{
			auto hash = game::hash_scr_dvar_iw9(name);
			printf("registering \"%s\" with hash %llx\n", name, hash);
			return game::Dvar_RegisterVariant_IW9(hash, 10, flags, &value, &domain, desc);
		}

		return game::Dvar_RegisterVariant(name, utils::string::dvar_checksum(name), 9, flags, &value, &domain, desc);
	}

	dvar_t* Dvar_RegisterBool(const char* name, bool value, DvarFlags flags, const char* desc)
	{
		static const auto& game_ = identification::game::get_target_game().client_name;
		if (game_ == "s4-mod"s)
		{
			auto hash = game::hash_scr_dvar(name);
			printf("registering \"%s\" with hash %llx\n", name, hash);
			auto res = game::Dvar_RegisterBool_IW9(hash, value, flags, desc);
			
			/*
			if (res)
			{
				auto bruh = get_current(res);
				printf("current value: %d (%d)\n", bruh->enabled, bruh->integer);
			}
			else
				printf("failed to register dvar \"%s\"!\n", name);
			*/

			return res;
		}
		else if (game_ == "iw9-mod"s)
		{
			auto hash = game::hash_scr_dvar_iw9(name);
			printf("registering \"%s\" with hash %llx\n", name, hash);
			return game::Dvar_RegisterBool_IW9(hash, value, flags, desc);
		}

		return game::Dvar_RegisterBool_(name, value, flags, desc);
	}

	void Dvar_SetBool_Internal(game::dvar_t* dvar, bool value)
	{
		static const auto& game_ = identification::game::get_target_game().client_name;
		if (game_ == "iw8-mod"s)
			game::Dvar_SetBool_Internal_IW8(dvar, value);
		else
			game::Dvar_SetBool_Internal_IW9(dvar, value, 0); // changed for IW9 in S4 too, probably source
	}

	int Cmd_Argc()
	{
		static const auto& game_ = identification::game::get_target_game().client_name;
		if (game_ == "iw9-mod"s)
			return cmd_args->argc[cmd_args->nesting];
			
		//return game::Cmd_Argc_internal();
		return 0;
	}

	const char* Cmd_Argv(int argIndex)
	{
		static const auto& game_ = identification::game::get_target_game().client_name;
		if (game_ != "iw9-mod"s)
			return ""; // game::Cmd_Argv_internal(argIndex);
		
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
