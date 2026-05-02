#pragma once
#include <utils/concurrency.hpp>

namespace scripting
{
	std::string get_current_file();
	std::string get_current_script_file();

	void on_shutdown(const std::function<void(bool, bool)>& callback);
	std::string get_token(std::uint64_t id);

	std::optional<std::pair<std::string, std::string>> find_function(const char* pos);
}
