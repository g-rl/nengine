#pragma once
#include <xsk/gsc/engine/iw8.hpp>
#include <xsk/gsc/engine/iw9.hpp>

namespace gsc
{
	struct col_line_t
	{
		std::uint16_t line;
		std::uint16_t column;
	};

	struct loaded_script_t
	{
		void* ptr;
		std::map<std::uint32_t, col_line_t> devmap;
	};

	extern std::unique_ptr<xsk::gsc::iw8::context> gsc_ctx;

	game::ScriptFile* find_script(game::XAssetType type, const char* name, int allow_create_default);
	game::ScriptFile_IW9* find_script_iw9(game::XAssetType type, std::uint64_t name, int allow_create_default);

	loaded_script_t* get_loaded_script(const std::string& name);

	void on_begin_scripts(const std::function<void()>& callback);

	std::string get_function_name(std::uint64_t id);

	std::string get_script_name(const char* name, bool ignore_cache = false);
	std::string get_script_name_iw9(std::uint64_t hash);
}
