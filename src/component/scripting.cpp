#include <std_include.hpp>
#include "loader/component_loader.hpp"

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

		std::string current_file;

		struct zp_gsc_script_info
		{
			const char* file;
			const char* name;
		};

		using zp_gsc_find_function_t = int (*)(const char*, zp_gsc_script_info*);
		using zp_gsc_get_current_file_t = const char* (*)();

		zp_gsc_find_function_t zp_find_function = nullptr;
		zp_gsc_get_current_file_t zp_get_current_file = nullptr;

		utils::hook::detour scr_add_class_field_hook;

		utils::hook::detour scr_set_thread_position_hook;
		utils::hook::detour process_script_hook;

		std::string current_script_file;
		unsigned int current_file_id{};

		std::vector<std::function<void(bool, bool)>> shutdown_callbacks;

		void scr_add_class_field_stub(game::scrContext_t* context,
			unsigned int classnum, game::scr_string_t name, unsigned int canonical_string, unsigned int offset)
		{
			//printf("scr_add_class_field_stub");

			const auto name_str = game::SL_ConvertToString(name);

			if (fields_table[classnum].find(name_str) == fields_table[classnum].end())
			{
				fields_table[classnum][name_str] = offset;
			}

			scr_add_class_field_hook.invoke<void>(context, classnum, name, canonical_string, offset);
		}

		void process_script_stub(game::scrContext_t* context, const char* filename)
		{
			//printf("process_script_stub\n");

			current_script_file = filename;
			
			const auto file_id = atoi(filename);
			if (file_id)
			{
				current_file_id = static_cast<std::uint16_t>(file_id);
			}
			else
			{
				current_file_id = 0;
				current_file = filename;
			}

			process_script_hook.invoke<void>(context, filename);
		}

		inline game::XAssetType get_scriptfile_type(const std::string* name)
		{
			if (*name == "s4-mod"s)
				return game::ASSET_TYPE_SCRIPTFILE_S4;
			else if (*name == "iw9-mod"s)
				return game::ASSET_TYPE_SCRIPTFILE_IW9;
			return game::ASSET_TYPE_SCRIPTFILE;
		}

		void add_function_sort(unsigned int id, const char* pos)
		{
			std::string filename = current_file;
			if (current_file_id)
			{
				filename = scripting::get_token(current_file_id);
			}

			if (!script_function_table_sort.contains(filename))
			{
				static const auto& game_ = identification::game::get_target_game().client_name;

				auto* script = gsc::find_script(get_scriptfile_type(&game_), current_script_file.data(), false);

				if (script != nullptr)
				{
					const char* end = nullptr;
					if (game_ == "s4-mod"s)
					{
						auto* s4 = reinterpret_cast<game::ScriptFile_S4*>(script);
						end = &s4->bytecode[s4->bytecodeLen];
					}
					else if (game_ == "iw9-mod"s)
					{
						auto* iw9 = reinterpret_cast<game::ScriptFile_IW9*>(script);
						end = &iw9->bytecode[iw9->bytecodeLen];
					}
					else
					{
						end = &script->bytecode[script->bytecodeLen];
					}
					script_function_table_sort[filename].emplace_back("__end__", end);
				}
			}

			const auto name = gsc::get_function_name(id);
			auto& itr = script_function_table_sort[filename];
			itr.insert(itr.end() - 1, {name, pos});
		}

		void add_function(const std::string& file, unsigned int id, const char* pos)
		{
			const auto name = gsc::get_function_name(id);
			script_function_table[file][name] = pos;
			script_function_table_rev[pos] = {file, name};
		}

		void scr_set_thread_position_stub(game::scrContext_t* context, unsigned int thread_name, const char* code_pos)
		{
			add_function_sort(thread_name, code_pos);

			if (current_file_id)
			{
				const auto name = get_token(current_file_id);
				add_function(name, thread_name, code_pos);
			}
			else
			{
				add_function(current_file, thread_name, code_pos);
			}

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

			//scripting::notify(*game::levelEntityId, "shutdownGame_called", { 1 });
		}

		void shutdown_game_post(const bool free_scripts)
		{
			for (const auto& callback : shutdown_callbacks)
			{
				callback(free_scripts, true);
			}
		}

		namespace mp
		{
			utils::hook::detour sv_initgame_vm_hook;
			utils::hook::detour g_main_mp_shutdowngame_hook;

			void g_main_mp_shutdowngame_stub(bool full_clear)
			{
				shutdown_game_pre(full_clear);
				g_main_mp_shutdowngame_hook.invoke<void>(full_clear);
				shutdown_game_post(full_clear);
			}
		}
	}

	std::string get_token(unsigned int id)
	{
		return scripting::find_token(id);
	}

	void on_shutdown(const std::function<void(bool, bool)>& callback)
	{
		shutdown_callbacks.push_back(callback);
	}

	bool find_script_function(const char* pos, script_function_info* out)
	{
		if (zp_find_function)
		{
			zp_gsc_script_info info{};
			if (!zp_find_function(pos, &info))
				return false;
			if (out)
			{
				out->file = info.file ? info.file : "";
				out->name = info.name ? info.name : "";
			}
			return true;
		}

		const auto rev_it = script_function_table_rev.find(pos);
		if (rev_it != script_function_table_rev.end())
		{
			if (out)
			{
				out->file = rev_it->second.first;
				out->name = rev_it->second.second;
			}
			return true;
		}

		for (const auto& file : script_function_table_sort)
		{
			if (file.first.find("/asm/") != std::string::npos)
				continue;

			for (auto i = file.second.begin(); i != file.second.end() && std::next(i) != file.second.end(); ++i)
			{
				const auto next = std::next(i);
				if (pos >= i->second && pos < next->second)
				{
					if (out)
					{
						out->file = file.first;
						out->name = i->first;
					}
					return true;
				}
			}
		}

		return false;
	}

	std::string get_current_file()
	{
		if (zp_get_current_file)
		{
			const auto cf = zp_get_current_file();
			return cf ? cf : "";
		}
		return current_file;
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

			batch.add(SETUP_POINTER(game::Scr_AddClassField), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 0F B6 C2");

			batch.add(SETUP_POINTER(game::Scr_SetThreadPosition), "48 89 5C 24 ? 57 48 83 EC ? 49 8B D8 48 8B F9 44 8B C2");

			batch.add(SETUP_POINTER(game::ProcessScript), "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F9 48 8B DA B9 ? ? ? ? 44 8D 41 ? E8 ? ? ? ?"
				" 48 8B D3 B9 ? ? ? ? 48 8B F0 E8 ? ? ? ? 85 C0 75");
		}

		void post_unpack() override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw9-mod"s)
			{
				scr_add_class_field_hook.create(game::Scr_AddClassField, scr_add_class_field_stub);
				scr_set_thread_position_hook.create(game::Scr_SetThreadPosition, scr_set_thread_position_stub);
				process_script_hook.create(game::ProcessScript, process_script_stub);

				mp::g_main_mp_shutdowngame_hook.create(game::G_MainMP_ShutdownGame, mp::g_main_mp_shutdowngame_stub);
			}
			else
			{
				const auto version_dll = GetModuleHandleA("version.dll");
				if (!version_dll)
					return;

				zp_find_function = reinterpret_cast<zp_gsc_find_function_t>(
					GetProcAddress(version_dll, "zp_gsc_find_function"));
				zp_get_current_file = reinterpret_cast<zp_gsc_get_current_file_t>(
					GetProcAddress(version_dll, "zp_gsc_get_current_file"));
			}
		}
	};
}

REGISTER_COMPONENT(scripting::component)
