#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "component/call_spoofer.hpp"

#include "component/gsc/script_extension.hpp"
#include "component/gsc/script_loading.hpp"
#include "component/filesystem.hpp"
#include "component/scheduler.hpp"
#include "component/scripting.hpp"

#include "game/game.hpp"

#include "game/scripting/functions.hpp"

#include <utils/hook.hpp>
#include <utils/io.hpp>

#include <identification/game.hpp>

namespace scripting
{
	namespace
	{
		std::unordered_map<int, std::unordered_map<std::string, int>> fields_table;

		std::unordered_map<std::string, std::unordered_map<std::string, const char*>> script_function_table;
		std::unordered_map<std::string, std::vector<std::pair<std::string, const char*>>> script_function_table_sort;
		std::unordered_map<const char*, std::pair<std::string, std::string>> script_function_table_rev;

		utils::hook::detour scr_add_class_field_hook;
		utils::hook::detour scr_set_thread_position_hook;
		utils::hook::detour process_script_hook;

		// asset key for find_script(): raw filename ptr (IW8/S4) or hash (IW9)
		game::name_or_hash current_script_asset_key{};
		// raw filename, std::string-owned so it stays valid for error messages
		std::string current_script_file;
		// resolved readable filename, used as table key
		std::string current_file;

		std::vector<std::function<void(bool, bool)>> shutdown_callbacks;

		bool is_iw9()
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			return game_ == "iw9-mod"s;
		}

		bool is_s4()
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			return game_ == "s4-mod"s;
		}

		game::XAssetType get_scriptfile_type()
		{
			if (is_s4()) return game::ASSET_TYPE_SCRIPTFILE_S4;
			if (is_iw9()) return game::ASSET_TYPE_SCRIPTFILE_IW9;
			return game::ASSET_TYPE_SCRIPTFILE;
		}

		const char* get_script_bytecode_end(void* script)
		{
			if (is_s4())
			{
				auto* s4 = static_cast<game::ScriptFile_S4*>(script);
				return &s4->bytecode[s4->bytecodeLen];
			}
			auto* sf = static_cast<game::ScriptFile*>(script);
			return &sf->bytecode[sf->bytecodeLen];
		}

		std::string resolve_function_name(std::uint64_t id)
		{
			if (is_iw9())
			{
				game::name_or_hash key{};
				key.hash = id;
				return gsc::get_function_name(key);
			}
			return scripting::get_token(id);
		}

		void add_function_sort(const std::string& file, const std::string& name, const char* pos)
		{
			if (!script_function_table_sort.contains(file))
			{
				auto* script = gsc::find_script(get_scriptfile_type(), current_script_asset_key, false);
				if (script == nullptr)
				{
					return;
				}

				const auto* end = get_script_bytecode_end(script);
				script_function_table_sort[file].emplace_back("__end__", end);
			}

			auto& v = script_function_table_sort[file];
			v.insert(v.end() - 1, { name, pos });
		}

		void add_function(const std::string& file, const std::string& name, const char* pos)
		{
			script_function_table[file][name] = pos;
			script_function_table_rev[pos] = { file, name };
		}

		void scr_add_class_field_stub(game::scrContext_t* context,
			unsigned int classnum, game::scr_string_t name, unsigned int canonical_string, unsigned int offset)
		{
			const auto* name_str = game::SL_ConvertToString(name);
			if (!fields_table[classnum].contains(name_str))
			{
				fields_table[classnum][name_str] = offset;
			}
			scr_add_class_field_hook.invoke<void>(context, classnum, name, canonical_string, offset);
		}

		void scr_add_class_field_stub_iw9(game::scrContext_t* context,
			unsigned int classnum, std::uint64_t name, unsigned int offset)
		{
			const auto name_str = gsc::gsc_ctx_iw9->path_name(name);
			if (!fields_table[classnum].contains(name_str))
			{
				fields_table[classnum][name_str] = offset;
			}
			scr_add_class_field_hook.invoke<void>(context, classnum, name, offset);
		}

		void process_script_stub(game::scrContext_t* context, game::ScriptFile* scriptfile)
		{
			const auto* filename = scriptfile && scriptfile->raw_name.name ? scriptfile->raw_name.name : "";
			current_script_file = filename;
			current_script_asset_key.name = current_script_file.c_str();
			current_file = gsc::get_script_name(current_script_file.c_str());

			call_spoofer::spoof_hook_invoke<void>(process_script_hook, context, scriptfile);
		}

		void process_script_stub_iw9(game::scrContext_t* context, game::ScriptFile* scriptfile)
		{
			current_script_asset_key.hash = scriptfile->raw_name.hash;
			current_file = gsc::get_script_name_iw9(scriptfile->raw_name.hash);
			current_script_file = current_file;

			call_spoofer::spoof_hook_invoke<void>(process_script_hook, context, scriptfile);
		}

		void scr_set_thread_position_stub(game::scrContext_t* context, unsigned int thread_name, const char* code_pos)
		{
			const auto name = resolve_function_name(thread_name);
			add_function_sort(current_file, name, code_pos);
			add_function(current_file, name, code_pos);

			scr_set_thread_position_hook.invoke<void>(context, thread_name, code_pos);
		}

		void scr_set_thread_position_stub_iw9(game::scrContext_t* context, std::uint64_t thread_name, const char* code_pos)
		{
			const auto name = resolve_function_name(thread_name);
			add_function_sort(current_file, name, code_pos);
			add_function(current_file, name, code_pos);

			scr_set_thread_position_hook.invoke<void>(context, thread_name, code_pos);
		}

		void shutdown_game_pre(bool free_scripts)
		{
			if (free_scripts)
			{
				script_function_table_sort.clear();
				script_function_table.clear();
				script_function_table_rev.clear();
			}

			for (const auto& callback : shutdown_callbacks)
			{
				callback(free_scripts, false);
			}
		}

		void shutdown_game_post(bool free_scripts)
		{
			for (const auto& callback : shutdown_callbacks)
			{
				callback(free_scripts, true);
			}
		}

		namespace mp
		{
			utils::hook::detour g_main_mp_shutdowngame_hook;

			void g_main_mp_shutdowngame_stub(bool full_clear)
			{
				shutdown_game_pre(full_clear);
				g_main_mp_shutdowngame_hook.invoke<void>(full_clear);
				shutdown_game_post(full_clear);
			}
		}
	}

	std::string get_token(std::uint64_t id)
	{
		return scripting::find_token(id);
	}

	void on_shutdown(const std::function<void(bool, bool)>& callback)
	{
		shutdown_callbacks.push_back(callback);
	}

	std::string get_current_file()
	{
		return current_file;
	}

	std::string get_current_script_file()
	{
		return current_script_file;
	}

	std::optional<std::pair<std::string, std::string>> find_function(const char* pos)
	{
		for (const auto& [file, vec] : script_function_table_sort)
		{
			for (auto i = vec.begin(); i != vec.end() && std::next(i) != vec.end(); ++i)
			{
				const auto next = std::next(i);
				if (pos >= i->second && pos < next->second)
				{
					return std::make_pair(i->first, file);
				}
			}
		}

		return {};
	}

	class component final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw9-mod"s)
				batch.add(SETUP_POINTER(game::G_MainMP_ShutdownGame),
					"48 89 5C 24 10 48 89 6C 24 18 48 89 7C 24 20 41 56 48 83 ec 20 0F ? ? B9 12 00 00 00");
			else if (game_ == "s4-mod"s)
				batch.add(SETUP_POINTER(game::G_MainMP_ShutdownGame), "40 53 57 41 56 48 83 EC ? 44 0F B6 F1");
			else
			{
				if (identification::game::is_greater_or_eq("1.46.0"))
					batch.add(SETUP_POINTER(game::G_MainMP_ShutdownGame), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 8B 05 ? ? ? ? 0F B6 F9");
				else
					batch.add(SETUP_POINTER(game::G_MainMP_ShutdownGame), "E8 ? ? ? ? 65 48 8B 04 25 ? ? ? ? 48 8B CF 48 8B 14 18 33 C0", SETUP_MOD(add(1).rip()));
			}

			batch.add(SETUP_POINTER(game::Scr_AddClassField), "E8 ? ? ? 00 FF ? 48 8D ? ? 83 ? 0E 72 ? 48 8B 5C 24 ? 48 8B 74 24", GRAB_CALL);

			batch.add(SETUP_POINTER(game::Scr_SetThreadPosition), "E8 ? ? ? 00 4C 8D 44 24 20 C6 44 24 28 ? 8B D0 48 89 5C 24 20 48 8B CF E8 ? ? ? ? 48", [](memory::scanned_result<void> r) {
				std::uintptr_t offset = 0;
				while (true) {
					offset++;
					auto buf = r.sub(offset).as<std::uint8_t*>();
					if (buf[0] == 0x48 && buf[1] == 0x89 && buf[2] == 0x5C && buf[3] == 0x24 && buf[4] == 0x08) {
						break;
					}
				}
				return r.sub(offset);
				});

			batch.add(SETUP_POINTER(game::ProcessScript), "E8 ? ? ? FF ? C0 75 0B 48 8B ? 48 8B ? E8 ? 00 00 00", SETUP_MOD(add(16).rip()));
		}

		void post_unpack() override
		{
			mp::g_main_mp_shutdowngame_hook.create(game::G_MainMP_ShutdownGame, mp::g_main_mp_shutdowngame_stub);

			if (is_iw9())
			{
				scr_add_class_field_hook.create(game::Scr_AddClassField, scr_add_class_field_stub_iw9);
				scr_set_thread_position_hook.create(game::Scr_SetThreadPosition, scr_set_thread_position_stub_iw9);
				process_script_hook.create(game::ProcessScript, process_script_stub_iw9);
			}
			else
			{
				scr_add_class_field_hook.create(game::Scr_AddClassField, scr_add_class_field_stub);
				scr_set_thread_position_hook.create(game::Scr_SetThreadPosition, scr_set_thread_position_stub);
				process_script_hook.create(game::ProcessScript, process_script_stub);
			}
		}
	};
}

REGISTER_COMPONENT(scripting::component)
