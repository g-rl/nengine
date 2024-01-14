#pragma once
#include "script_value.hpp"

namespace scripting
{
	class function
	{
	public:
		function(const char*);

		script_value get_raw() const;
		const char* get_pos() const;
		std::string get_name() const;

	private:
		const char* pos_;
	};
}
