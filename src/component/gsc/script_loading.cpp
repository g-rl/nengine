#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "component/filesystem.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/memory.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>

namespace gsc
{
	namespace
	{
		std::unordered_map<std::string, std::uint32_t> main_handles;
		//std::unordered_map<std::string, std::uint32_t> init_handles;

		utils::memory::allocator scriptfile_allocator;
		std::unordered_map<const char*, game::ScriptFile*> loaded_scripts;

		char* script_mem_buf = nullptr;

		struct
		{
			char* buf = nullptr;
			char* pos = nullptr;
			const std::uint64_t size = 0x100000i64;
		} script_memory;

		char* allocate_buffer(size_t size)
		{
			if (script_memory.buf == nullptr)
			{
				script_memory.buf = script_mem_buf;
				script_memory.pos = script_memory.buf;
			}

			if (script_memory.pos + size > script_memory.buf + script_memory.size)
			{
				game::Com_Error(game::ERR_FATAL, "Out of custom script memory");
			}

			const auto pos = script_memory.pos;
			script_memory.pos += size;
			return pos;
		}

		void free_script_memory()
		{
			if (script_memory.buf != nullptr)
			{
				memset(script_memory.buf, 0, reinterpret_cast<size_t>(script_memory.pos) - reinterpret_cast<size_t>(script_memory.buf));
				script_memory.buf = nullptr;
				script_memory.pos = nullptr;
			}
		}

		void clear()
		{
			main_handles.clear();
			//init_handles.clear();
			loaded_scripts.clear();
			scriptfile_allocator.clear();
			free_script_memory();
		}

		bool read_raw_script_file(const std::string& name, std::string* data)
		{
			if (filesystem::read_file(name, data))
			{
				return true;
			}

			// TODO: you can store rawfile assets and load them here
			/*
			const auto* name_str = name.data();
			if (game::DB_XAssetExists(game::ASSET_TYPE_RAWFILE, name_str) &&
				!game::DB_IsXAssetDefault(game::ASSET_TYPE_RAWFILE, name_str))
			{
				const auto asset = game::DB_FindXAssetHeader(game::ASSET_TYPE_RAWFILE, name_str, false);
				const auto len = game::DB_GetRawFileLen(asset.rawfile);
				data->resize(len);
				game::DB_GetRawBuffer(asset.rawfile, data->data(), len);
				if (len > 0)
				{
					data->pop_back();
				}

				return true;
			}
			*/

			return false;
		}

		game::ScriptFile* load_custom_script(const char* file_name, const std::string& real_name)
		{
			if (const auto itr = loaded_scripts.find(file_name); itr != loaded_scripts.end())
			{
				return itr->second;
			}

			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				return nullptr;
			}

			std::string source_buffer{};
			if (!read_raw_script_file(real_name, &source_buffer) || source_buffer.empty())
			{
				return nullptr;
			}

			/*
			// filter out "GSC rawfiles" that were used for development usage and are not meant for us.
			// each "GSC rawfile" has a ScriptFile counterpart to be used instead
			if (game::DB_XAssetExists(game::ASSET_TYPE_SCRIPTFILE, file_name) &&
				!game::DB_IsXAssetDefault(game::ASSET_TYPE_SCRIPTFILE, file_name))
			{
				if ((real_name.starts_with("maps/createfx") || real_name.starts_with("maps/createart") || real_name.starts_with("maps/mp"))
					&& (real_name.ends_with("_fx") || real_name.ends_with("_fog") || real_name.ends_with("_hdr")))
				{
					printf("Refusing to compile rawfile '%s'\n", real_name.data());
					return game::DB_FindXAssetHeader(game::ASSET_TYPE_SCRIPTFILE, file_name, false).scriptfile;
				}
			}
			*/

			printf("Loading custom gsc '%s'\n", real_name.data());

			std::vector<std::uint8_t> data;
			data.assign(source_buffer.begin(), source_buffer.end());

			auto pos = std::size_t{0};

			const auto script_file_ptr = static_cast<game::ScriptFile*>(scriptfile_allocator.allocate(sizeof(game::ScriptFile)));

			script_file_ptr->name = file_name;

			const auto name = std::string{ reinterpret_cast<char const*>(data.data()) };
			pos += name.size() + 1;

			script_file_ptr->compressedLen = *reinterpret_cast<std::uint32_t const*>(data.data() + pos);
			pos += 4;

			script_file_ptr->len = *reinterpret_cast<std::uint32_t const*>(data.data() + pos);
			pos += 4;

			script_file_ptr->bytecodeLen = *reinterpret_cast<std::uint32_t const*>(data.data() + pos);
			pos += 4;

			script_file_ptr->buffer = static_cast<char*>(scriptfile_allocator.allocate(script_file_ptr->compressedLen));
			std::memcpy(script_file_ptr->buffer, data.data() + pos, script_file_ptr->compressedLen);
			pos += script_file_ptr->compressedLen;

			script_file_ptr->bytecode = allocate_buffer(script_file_ptr->bytecodeLen);
			std::memcpy(script_file_ptr->bytecode, data.data() + pos, script_file_ptr->bytecodeLen);

			loaded_scripts[file_name] = script_file_ptr;

			return script_file_ptr;
		}

		void db_get_raw_buffer_stub(const game::RawFile* rawfile, char* buf, const int size)
		{
			if (rawfile->len > 0 && rawfile->compressedLen == 0)
			{
				std::memset(buf, 0, size);
				std::memcpy(buf, rawfile->buffer, std::min(rawfile->len, size));
				return;
			}

			game::DB_GetRawBuffer(rawfile, buf, size);
		}

		utils::hook::detour db_alloc_x_zone_memory_internal_hook;
		void db_alloc_x_zone_memory_internal_stub(unsigned __int64* blockSize, const char* filename, game::XZoneMemory* zoneMem, game::XBlock* archiveBlocks, unsigned int type)
		{
			bool patch = false; // ugly fix for script memory allocation

			if (!_stricmp(filename, "code_post_gfx") && type == game::DM_MEMORY_SCRIPT)
			{
				patch = true;
				printf("patching memory for '%s'\n", filename);
			}

			// TODO: type is different all below this
			if (patch)
			{
				blockSize[game::XFILE_BLOCK_SCRIPT] += script_memory.size;
			}

			db_alloc_x_zone_memory_internal_hook.invoke<void>(blockSize, filename, zoneMem, archiveBlocks, type);

			if (patch)
			{
				blockSize[game::XFILE_BLOCK_SCRIPT] -= script_memory.size;
				script_mem_buf = archiveBlocks[game::XFILE_BLOCK_SCRIPT].data + blockSize[game::XFILE_BLOCK_SCRIPT]; // this was changed around, but is 100% wrong
			}
		}

		game::ScriptFile* find_script(game::XAssetType type, const char* name, int allow_create_default)
		{
			std::string real_name = name;
			// TODO: convert ID to name via gsc-tool
			/*
			const auto id = static_cast<std::uint16_t>(std::atoi(name));
			if (id)
			{
				real_name = gsc_ctx->token_name(id);
			}
			*/

			auto* script = load_custom_script(name, real_name);
			if (script)
			{
				return script;
			}

			return game::DB_FindXAssetHeader(type, name, allow_create_default).scriptfile;
		}

		void load_script(const std::string& name)
		{
			const auto scr_context = game::ScriptContext_Server();
			if (!game::Scr_LoadScript(scr_context, name.data()))
			{
				return;
			}

			const auto main_handle = game::Scr_GetFunctionHandle(scr_context, name.data(), 596);
			if (main_handle)
			{
				printf("Loaded '%s::main'\n", name.data());
				main_handles[name] = main_handle;
			}
		}

		void load_scripts(const std::filesystem::path& root_dir, const std::filesystem::path& subfolder)
		{
			std::filesystem::path script_dir = root_dir / subfolder;
			if (!utils::io::directory_exists(script_dir.generic_string()))
			{
				return;
			}

			const auto scripts = utils::io::list_files(script_dir.generic_string());
			for (const auto& script : scripts)
			{
				if (!script.ends_with(".gsc"))
				{
					continue;
				}

				std::filesystem::path path(script);
				const auto relative = path.lexically_relative(root_dir).generic_string();
				load_script(relative);
			}
		}

		int db_is_x_asset_default(game::XAssetType type, const char* name)
		{
			if (loaded_scripts.contains(name))
			{
				return 0;
			}

			return game::DB_IsXAssetDefault(type, name);
		}

		utils::hook::detour load_scripts_hook;
		void load_scripts_stub()
		{
			load_scripts_hook.invoke<void>();

			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				return;
			}

			for (const auto& path : filesystem::get_search_paths())
			{
				load_scripts(path, "scripts/");
			}
		}

		utils::hook::detour g_load_structs_hook;
		void g_load_structs_stub()
		{
			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				g_load_structs_hook.invoke<void>();
				return;
			}

			const auto scr_context = game::ScriptContext_Server();
			for (auto& function_handle : main_handles)
			{
				printf("Executing '%s::main'\n", function_handle.first.data());
				game::Scr_FreeThread(scr_context, game::Scr_ExecThread(scr_context, function_handle.second, 0));
			}

			g_load_structs_hook.invoke<void>();
		}

		utils::hook::detour g_shutdown_game_hook;
		void g_shutdown_game_stub(bool full_clear)
		{
			clear();

			g_shutdown_game_hook.invoke<void>(full_clear);
		}

		void unknown_func_stub(void*, void*, void*)
		{
			game::Com_Error(game::ERR_SCRIPT_DROP, "unknown function (misspelled function or include path is wrong)\n");
		}
	}

	class loading final : public component_interface
	{
	public:
		void post_unpack() override
		{
			// Allocate script memory (PMem doesn't work)
			db_alloc_x_zone_memory_internal_hook.create(0x11A8C90_b, db_alloc_x_zone_memory_internal_stub);

			// TODO: Increase allocated script memory
			//utils::hook::set<uint32_t>(0xA75B5C_b + 1, 0x480000 + static_cast<std::uint32_t>(script_memory.size));
			//utils::hook::set<uint32_t>(0xA75BAA_b + 4, 0x480 + (static_cast<std::uint32_t>(script_memory.size) >> 12));
			//utils::hook::set<uint32_t>(0xA75BBE_b + 6, 0x480 + (static_cast<std::uint32_t>(script_memory.size) >> 12));

			// Load our scripts with an uncompressed stack
			//utils::hook::call(0x13223B6_b, db_get_raw_buffer_stub);

			// ProcessScript: hook xasset functions to return our own custom scripts
			utils::hook::call(0x132230E_b, find_script);
			utils::hook::call(0x132231E_b, db_is_x_asset_default);

			// GScr_LoadScripts: initial loading of scripts (now called GScr_MainMP_LoadScripts)
			load_scripts_hook.create(0x12598A0_b, load_scripts_stub);

			// execute main handle after G_LoadStructs (now called G_Spawn_LoadStructs)
			g_load_structs_hook.create(0xFC80A5_b, g_load_structs_stub);

			// clear memory
			g_shutdown_game_hook.create(0x121F880_b, g_shutdown_game_stub);

			// TODO: move to proper class
			// change Sys_Error -> Com_Error
			utils::hook::call(0x13166DE_b, unknown_func_stub);
			utils::hook::call(0x1316777_b, unknown_func_stub);
		}
	};
}

REGISTER_COMPONENT(gsc::loading)
