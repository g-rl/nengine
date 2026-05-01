#pragma once
#include <xsk/gsc/engine/iw8.hpp>
#include <xsk/gsc/engine/s4.hpp>
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
	extern std::unique_ptr<xsk::gsc::iw9::context> gsc_ctx_iw9;

	game::ScriptFile* find_script(game::XAssetType type, game::name_or_hash name, int allow_create_default);

	loaded_script_t* get_loaded_script(const std::string& name);

	void on_begin_scripts(const std::function<void()>& callback);

	std::uint32_t token_id(const std::string& name);
	std::string token_name(std::uint64_t id);

	std::string builtin_function_name(std::uint64_t id);
	std::string builtin_method_name(std::uint64_t id);
	bool builtin_function_exists(const std::string& name);
	bool builtin_method_exists(const std::string& name);
	std::uint16_t builtin_function_id(const std::string& name);
	std::uint16_t builtin_method_id(const std::string& name);
	void add_builtin_function(const std::string& name, std::uint16_t id);
	void add_builtin_method(const std::string& name, std::uint16_t id);
	int find_builtin_index(const std::string& name, bool prefer_global);

	std::optional<std::string> opcode_name(std::uint8_t opcode);
	bool is_builtin_call_opcode(std::uint8_t opcode);

	std::string get_function_name(game::name_or_hash raw_name);

	std::string get_script_name(const char* name);
	std::string get_script_name(game::name_or_hash raw_name);
	std::string get_script_name_iw9(std::uint64_t hash);
}
