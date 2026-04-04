#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "component/gsc/script_extension.hpp"
#include "component/gsc/script_loading.hpp"
#include "component/scheduler.hpp"
#include "component/scripting.hpp"

#include "game/game.hpp"

#include "game/scripting/functions.hpp"

#include <utils/hook.hpp>

namespace scripting
{
	std::unordered_map<int, std::unordered_map<std::string, int>> fields_table;

	std::unordered_map<std::string, std::unordered_map<std::string, const char*>> script_function_table;
	std::unordered_map<std::string, std::vector<std::pair<std::string, const char*>>> script_function_table_sort;
	std::unordered_map<const char*, std::pair<std::string, std::string>> script_function_table_rev;

	std::string current_file;

	namespace
	{
		utils::hook::detour scr_add_class_field_hook;

		utils::hook::detour scr_set_thread_position_hook;
		utils::hook::detour process_script_hook;

		std::string current_script_file;
		unsigned int current_file_id{};

		std::vector<std::function<void(bool, bool)>> shutdown_callbacks;

		void scr_add_class_field_stub(game::scrContext_t* context,
			unsigned int classnum, game::scr_string_t name, unsigned int canonical_string, unsigned int offset)
		{
			const auto name_str = game::SL_ConvertToString(name);

			if (fields_table[classnum].find(name_str) == fields_table[classnum].end())
			{
				fields_table[classnum][name_str] = offset;
			}

			scr_add_class_field_hook.invoke<void>(context, classnum, name, canonical_string, offset);
		}

		void process_script_stub(game::scrContext_t* context, const char* filename)
		{
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

		void add_function_sort(unsigned int id, const char* pos)
		{
			std::string filename = current_file;
			if (current_file_id)
			{
				filename = scripting::get_token(current_file_id);
			}

			if (!script_function_table_sort.contains(filename))
			{
				const auto script = gsc::find_script(game::ASSET_TYPE_SCRIPTFILE, current_script_file.data(), false);
				if (script != nullptr)
				{
					const auto end = &script->bytecode[script->bytecodeLen];
					script_function_table_sort[filename].emplace_back("__end__", end);
				}
			}

			const auto name = scripting::get_token(id);
			auto& itr = script_function_table_sort[filename];
			itr.insert(itr.end() - 1, {name, pos});
		}

		void add_function(const std::string& file, unsigned int id, const char* pos)
		{
			const auto name = scripting::get_token(id);
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

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			// TODO: this code should only run on 1.20.4 & 1.20.4-replay!!! iw8-mod's stuff works otherwise.

			scr_add_class_field_hook.create(0x131DBF0_b, scr_add_class_field_stub);
			scr_set_thread_position_hook.create(0x13169D0_b, scr_set_thread_position_stub); // i think this is right
			process_script_hook.create(0x13222F0_b, process_script_stub);

			mp::g_main_mp_shutdowngame_hook.create(0x121F880_b, mp::g_main_mp_shutdowngame_stub);
		}
	};
}

REGISTER_COMPONENT(scripting::component)
