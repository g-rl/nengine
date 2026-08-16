#pragma once

#include "std_include.hpp"

#include <string>

namespace gsc
{
	inline void report_compile_error(const std::string& script_name, const std::string& file, const int line, const int col, const std::string& msg)
	{
		printf("*********** script compile error *************\n");

		if (col >= 0)
		{
			printf("failed to compile '%s' at %s:%d:%d:\n%s\n", script_name.data(), file.data(), line, col, msg.data());
		}
		else
		{
			printf("failed to compile '%s' at %s:%d:\n%s\n", script_name.data(), file.data(), line, msg.data());
		}
		
		printf("**********************************************\n");
	}

	inline void report_compile_error(const std::string& script_name, [[maybe_unused]] const std::string& file, const std::string& raw_msg)
	{
		printf("*********** script compile error *************\n");
		printf("failed to compile '%s':\n%s\n", script_name.data(), raw_msg.data());
		printf("**********************************************\n");
	}
}
