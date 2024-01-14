#include <std_include.hpp>
#include "script_value.hpp"
#include "function.hpp"

namespace scripting
{
	/***************************************************************
	 * Constructors
	 **************************************************************/

	script_value::script_value(const game::VariableValue& value)
		: value_(value)
	{
	}

	script_value::script_value(const value_wrap& value)
		: value_(value.get_raw())
	{
	}

	script_value::script_value(const int value)
	{
		game::VariableValue variable{};
		variable.type = game::VAR_INTEGER;
		variable.u.intValue = value;

		this->value_ = variable;
	}

	script_value::script_value(const unsigned int value)
	{
		game::VariableValue variable{};
		variable.type = game::VAR_INTEGER;
		variable.u.uintValue = value;

		this->value_ = variable;
	}

	script_value::script_value(const bool value)
		: script_value(static_cast<unsigned>(value))
	{
	}

	script_value::script_value(const float value)
	{
		game::VariableValue variable{};
		variable.type = game::VAR_FLOAT;
		variable.u.floatValue = value;

		this->value_ = variable;
	}

	script_value::script_value(const double value)
		: script_value(static_cast<float>(value))
	{
	}

	script_value::script_value(const char* value)
	{
		game::VariableValue variable{};
		variable.type = game::VAR_STRING;
		variable.u.stringValue = game::SL_GetString(value, 0);

		const auto _ = gsl::finally([&variable]()
		{
			const auto context = game::ScriptContext_Server();
			game::RemoveRefToValue(context, variable.type, variable.u);
		});

		this->value_ = variable;
	}

	script_value::script_value(const std::string& value)
		: script_value(value.data())
	{
	}

	script_value::script_value(const function& value)
	{
		game::VariableValue variable{};
		variable.type = game::VAR_FUNCTION;
		variable.u.codePosValue = value.get_pos();

		this->value_ = variable;
	}

	/***************************************************************
	 * Integer
	 **************************************************************/

	template <>
	bool script_value::is<int>() const
	{
		return this->get_raw().type == game::VAR_INTEGER;
	}

	template <>
	bool script_value::is<unsigned int>() const
	{
		return this->is<int>();
	}

	template <>
	bool script_value::is<bool>() const
	{
		return this->is<int>();
	}

	template <>
	int script_value::get() const
	{
		return this->get_raw().u.intValue;
	}

	template <>
	unsigned int script_value::get() const
	{
		return this->get_raw().u.uintValue;
	}

	template <>
	bool script_value::get() const
	{
		return this->get_raw().u.uintValue != 0;
	}

	/***************************************************************
	 * Float
	 **************************************************************/

	template <>
	bool script_value::is<float>() const
	{
		return this->get_raw().type == game::VAR_FLOAT;
	}

	template <>
	bool script_value::is<double>() const
	{
		return this->is<float>();
	}

	template <>
	float script_value::get() const
	{
		return this->get_raw().u.floatValue;
	}

	template <>
	double script_value::get() const
	{
		return static_cast<double>(this->get_raw().u.floatValue);
	}

	/***************************************************************
	 * String
	 **************************************************************/

	template <>
	bool script_value::is<const char*>() const
	{
		return this->get_raw().type == game::VAR_STRING;
	}

	template <>
	bool script_value::is<std::string>() const
	{
		return this->is<const char*>();
	}

	template <>
	const char* script_value::get() const
	{
		return game::SL_ConvertToString(static_cast<game::scr_string_t>(this->get_raw().u.stringValue));
	}

	template <>
	std::string script_value::get() const
	{
		return this->get<const char*>();
	}

	/***************************************************************
	 * Struct
	 **************************************************************/

	template <>
	bool script_value::is<std::map<std::string, script_value>>() const
	{
		if (this->get_raw().type != game::VAR_POINTER)
		{
			return false;
		}

		const auto id = this->get_raw().u.uintValue;
		const auto context = game::ScriptContext_Server();
		const auto type = context->objectVariableValue[id].w.type;

		return type == game::VAR_OBJECT;
	}

	/***************************************************************
	 * Function
	 **************************************************************/

	template <>
	bool script_value::is<function>() const
	{
		return this->get_raw().type == game::VAR_FUNCTION;
	}

	template <>
	function script_value::get() const
	{
		return function(this->get_raw().u.codePosValue);
	}

	/***************************************************************
	 *
	 **************************************************************/

	const game::VariableValue& script_value::get_raw() const
	{
		return this->value_.get();
	}

	value_wrap::value_wrap(const scripting::script_value& value, int argument_index)
		: value_(value)
		, argument_index_(argument_index)
	{
	}

	std::string script_value::to_string() const
	{
		if (this->is<int>())
		{
			return utils::string::va("%i", this->as<int>());
		}

		if (this->is<float>())
		{
			return utils::string::va("%f", this->as<float>());
		}

		if (this->is<std::string>())
		{
			return this->as<std::string>();
		}

		if (this->is<function>())
		{
			const auto func = this->as<function>();
			return utils::string::va("[[ %s ]]", func.get_name().data());
		}

		return this->type_name();
	}
}
