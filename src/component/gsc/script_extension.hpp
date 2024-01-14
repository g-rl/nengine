#pragma once

#include "game/scripting/function.hpp"

namespace gsc
{
	using builtin_function = void(*)(game::scrContext_t* context);
	using builtin_method = void(*)(game::scrContext_t* context, game::scr_entref_t);

	extern builtin_function func_table[0x1000];
	extern builtin_method meth_table[0x1000];

	extern const game::dvar_t* developer_script;

	void scr_error(const char* error, const bool force_print = false);

	namespace function
	{
		void add(const std::string& name, builtin_function function);
	}
}
