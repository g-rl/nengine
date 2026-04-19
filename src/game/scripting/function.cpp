#include <std_include.hpp>

#include "game/scripting/function.hpp"

#include "component/scripting.hpp"

namespace scripting
{
	function::function(const char* pos)
		: pos_(pos)
	{
	}

	script_value function::get_raw() const
	{
		game::VariableValue value;
		value.type = game::VAR_FUNCTION;
		value.u.codePosValue = this->pos_;

		return value;
	}

	const char* function::get_pos() const
	{
		return this->pos_;
	}

	std::string function::get_name() const
	{
		scripting::script_function_info info;
		if (scripting::find_script_function(this->pos_, &info))
		{
			return utils::string::va("%s::%s", info.file.c_str(), info.name.c_str());
		}

		return "unknown function";
	}
}
