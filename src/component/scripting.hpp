#pragma once
#include <utils/concurrency.hpp>

namespace scripting
{
	std::string get_current_file();

	void on_shutdown(const std::function<void(bool, bool)>& callback);
	//std::optional<std::string> get_canonical_string(const unsigned int id);
	std::string get_token(std::uint64_t id);

	std::optional<std::pair<std::uint64_t, std::uint64_t>> find_function_iw9(const char* pos);
}
