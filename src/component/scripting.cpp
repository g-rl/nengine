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

		bool is_iw9()
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			return game_ == "iw9-mod"s;
		}

		struct name_or_hash_hash
		{
			std::size_t operator()(const game::name_or_hash value) const
			{
				if (is_iw9())
				{
					return std::hash<std::uint64_t>{}(value.hash);
				}

				return std::hash<std::string>{}(value.name ? value.name : "");
			}
		};

		struct name_or_hash_equal
		{
			bool operator()(const game::name_or_hash lhs, const game::name_or_hash rhs) const
			{
				if (is_iw9())
				{
					return lhs.hash == rhs.hash;
				}

				if (lhs.name == rhs.name)
				{
					return true;
				}

				if (!lhs.name || !rhs.name)
				{
					return false;
				}

				return std::strcmp(lhs.name, rhs.name) == 0;
			}
		};

		using script_function_map = std::unordered_map<game::name_or_hash, const char*, name_or_hash_hash, name_or_hash_equal>;

		std::unordered_map<game::name_or_hash, script_function_map, name_or_hash_hash, name_or_hash_equal> script_function_table;
		std::unordered_map<game::name_or_hash, std::vector<std::pair<game::name_or_hash, const char*>>, name_or_hash_hash, name_or_hash_equal> script_function_table_sort;
		std::unordered_map<const char*, std::pair<game::name_or_hash, game::name_or_hash>> script_function_table_rev;
		std::unordered_set<std::string> script_name_pool;

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

		game::name_or_hash current_script_file;
		game::name_or_hash current_script_file_asset;
		std::string current_file;

		std::vector<std::function<void(bool, bool)>> shutdown_callbacks;

		const char* intern_script_name(std::string name)
		{
			return script_name_pool.emplace(std::move(name)).first->c_str();
		}

		game::name_or_hash make_name_key(const char* name)
		{
			game::name_or_hash result{};
			result.name = name;
			return result;
		}

		game::name_or_hash make_name_key(std::string name)
		{
			return make_name_key(intern_script_name(std::move(name)));
		}

		game::name_or_hash make_hash_key(const std::uint64_t hash)
		{
			game::name_or_hash result{};
			result.hash = hash;
			return result;
		}

		game::name_or_hash make_function_key(const std::uint64_t id)
		{
			return is_iw9() ? make_hash_key(id) : make_name_key(get_token(id));
		}

		game::name_or_hash make_end_key()
		{
			return is_iw9() ? make_hash_key(0) : make_name_key("__end__");
		}

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

		void scr_add_class_field_stub_iw9(game::scrContext_t* context,
			unsigned int classnum, std::uint64_t name, unsigned int offset)
		{
			const auto name_str = gsc::gsc_ctx_iw9->path_name(name);
			if (fields_table[classnum].find(name_str) == fields_table[classnum].end())
			{
				fields_table[classnum][name_str] = offset;
			}
			scr_add_class_field_hook.invoke<void>(context, classnum, name, offset);
		}

		void process_script_stub(game::scrContext_t* context, const char* filename)
		{
			current_script_file_asset = make_name_key(filename);
			current_file = gsc::get_script_name(filename);
			current_script_file = make_name_key(current_file);

			call_spoofer::spoof_hook_invoke<void>(process_script_hook, context, filename);
		}

		void process_script_stub_iw9(game::scrContext_t* context, game::ScriptFile* scriptfile)
		{
			current_script_file = make_hash_key(scriptfile->raw_name.hash);
			current_script_file_asset = current_script_file;
			current_file = gsc::get_script_name(current_script_file);

			//printf("process_script_stub: script file is %s (%" PRIu64 ")\n", gsc::gsc_ctx->path_name(scriptfile->name).data(), scriptfile->name);
			call_spoofer::spoof_hook_invoke<void>(process_script_hook, context, scriptfile);
		}

		inline game::XAssetType get_scriptfile_type(const std::string* name)
		{
			if (*name == "s4-mod"s)
				return game::ASSET_TYPE_SCRIPTFILE_S4;
			else if (*name == "iw9-mod"s)
				return game::ASSET_TYPE_SCRIPTFILE_IW9;
			return game::ASSET_TYPE_SCRIPTFILE;
		}

		void add_function_sort(game::name_or_hash id, const char* pos)
		{
			if (!script_function_table_sort.contains(current_script_file))
			{
				static const auto& game_ = identification::game::get_target_game().client_name;

				void* script = gsc::find_script(get_scriptfile_type(&game_), current_script_file_asset, false);

				if (script == nullptr)
				{
					return;
				}

				const char* end = nullptr;
				if (game_ == "s4-mod"s)
				{
					auto* s4 = reinterpret_cast<game::ScriptFile_S4*>(script);
					end = &s4->bytecode[s4->bytecodeLen];
				}
				else
				{
					auto* script_ = reinterpret_cast<game::ScriptFile*>(script);
					end = &script_->bytecode[script_->bytecodeLen];
				}

				script_function_table_sort[current_script_file].emplace_back(make_end_key(), end);
			}

			const auto name = make_function_key(id.token);
			auto& itr = script_function_table_sort[current_script_file];
			itr.insert(itr.end() - 1, { name, pos });
		}

		void add_function(game::name_or_hash file, game::name_or_hash id, const char* pos)
		{
			script_function_table[file][id] = pos;
			script_function_table_rev[pos] = {file, id};
		}

		void add_function_sort_iw9(std::uint64_t id, const char* pos)
		{
			if (!script_function_table_sort.contains(current_script_file))
			{
				const auto script = gsc::find_script(game::ASSET_TYPE_SCRIPTFILE_IW9, current_script_file_asset, false);
				if (script == nullptr)
				{
					return;
				}

				const auto end = &script->bytecode[script->bytecodeLen];
				script_function_table_sort[current_script_file].emplace_back(make_end_key(), end);
			}

			const auto name = make_function_key(id);
			auto& itr = script_function_table_sort[current_script_file];
			itr.insert(itr.end() - 1, { name, pos });
		}

		void add_function_iw9(const std::uint64_t file, std::uint64_t id, const char* pos)
		{
			add_function(make_hash_key(file), make_hash_key(id), pos);
		}

		void scr_set_thread_position_stub(game::scrContext_t* context, game::name_or_hash thread_name, const char* code_pos)
		{
			add_function_sort(thread_name, code_pos);
			add_function(current_script_file, make_function_key(thread_name.token), code_pos);

			scr_set_thread_position_hook.invoke<void>(context, thread_name, code_pos);
		}

		void scr_set_thread_position_stub_iw9(game::scrContext_t* context, std::uint64_t thread_name, const char* code_pos)
		{
			add_function_sort_iw9(thread_name, code_pos);
			add_function_iw9(current_script_file.hash, thread_name, code_pos);
			scr_set_thread_position_hook.invoke<void>(context, thread_name, code_pos);
		}

		void shutdown_game_pre(bool free_scripts)
		{
			if (free_scripts)
			{
				script_function_table_sort.clear();
				script_function_table.clear();
				script_function_table_rev.clear();
				script_name_pool.clear();
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
		if (zp_get_current_file)
		{
			const auto cf = zp_get_current_file();
			if (cf) return cf;
		}
		return current_file;
	}

	std::optional<std::pair<game::name_or_hash, game::name_or_hash>> find_function(const char* pos)
	{
		for (const auto& file : script_function_table_sort)
		{
			for (auto i = file.second.begin(); i != file.second.end() && std::next(i) != file.second.end(); ++i)
			{
				const auto next = std::next(i);
				if (pos >= i->second && pos < next->second)
				{
					return { std::make_pair(i->first, file.first) };
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
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw9-mod"s)
			{
				scr_add_class_field_hook.create(game::Scr_AddClassField, scr_add_class_field_stub_iw9);
				scr_set_thread_position_hook.create(game::Scr_SetThreadPosition, scr_set_thread_position_stub_iw9);
				process_script_hook.create(game::ProcessScript, process_script_stub_iw9);

				mp::g_main_mp_shutdowngame_hook.create(game::G_MainMP_ShutdownGame, mp::g_main_mp_shutdowngame_stub);
			}
			else
			{
				scr_add_class_field_hook.create(game::Scr_AddClassField, scr_add_class_field_stub);
				scr_set_thread_position_hook.create(game::Scr_SetThreadPosition, scr_set_thread_position_stub);
				process_script_hook.create(game::ProcessScript, process_script_stub);
				mp::g_main_mp_shutdowngame_hook.create(game::G_MainMP_ShutdownGame, mp::g_main_mp_shutdowngame_stub);

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
