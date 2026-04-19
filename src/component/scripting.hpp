#pragma once
#include <utils/concurrency.hpp>

namespace scripting
{
	struct script_function_info
	{
		std::string file;
		std::string name;
	};

	bool find_script_function(const char* pos, script_function_info* out);
	std::string get_current_file();

	void on_shutdown(const std::function<void(bool, bool)>& callback);
	std::optional<std::string> get_canonical_string(const unsigned int id);
	std::string get_token(unsigned int id);
}
