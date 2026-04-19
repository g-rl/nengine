#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "component/command.hpp"
#include "component/filesystem.hpp"
#include "component/scripting.hpp"

#include "game/scripting/function.hpp"
#include "game/scripting/functions.hpp"

#include "script_extension.hpp"
#include "script_loading.hpp"

#include <utils/compression.hpp>
#include <utils/hook.hpp>
#include <utils/memory.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>

#include <identification/game.hpp>

namespace gsc
{
	std::unique_ptr<xsk::gsc::iw8::context> gsc_ctx = std::make_unique<xsk::gsc::iw8::context>();

	std::unordered_map<std::string, loaded_script_t> loaded_scripts;

	namespace
	{
		void* DB_GetRawBuffer_call{};
		void* FindXAssetHeaderScript_call{};
		void* IsXAssetDefaultScript_call{};
		void* DB_AllocXZoneMemory_call{};

		utils::hook::detour scr_begin_load_scripts_hook;
		utils::hook::detour scr_end_load_scripts_hook;
		utils::hook::detour db_is_x_asset_default_hook;

		std::unordered_map<std::string, std::uint32_t> main_handles;
		std::unordered_map<std::string, std::uint32_t> init_handles;

		utils::memory::allocator scriptfile_allocator;

		std::unordered_map<std::uint32_t, std::string> cached_ids;
		std::unordered_map<std::uint32_t, std::string> script_function_names;

		char* script_mem_buf = nullptr;

		std::vector<std::function<void()>> begin_scripts_callbacks;

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
			init_handles.clear();
			loaded_scripts.clear();
			scriptfile_allocator.clear();
			free_script_memory();
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
				script_mem_buf = archiveBlocks[game::XFILE_BLOCK_SCRIPT].data + blockSize[game::XFILE_BLOCK_SCRIPT];
			}
		}

		bool read_raw_script_file(const std::string& name, std::string* data)
		{
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

			return filesystem::read_file(name, data);
		}

		std::map<std::uint32_t, col_line_t> parse_devmap(const xsk::gsc::buffer& devmap)
		{
			auto data = devmap.data;

			const auto read_32 = [&]()
			{
				const auto val = *reinterpret_cast<const std::uint32_t*>(data);
				data += sizeof(std::uint32_t);
				return val;
			};

			const auto read_16 = [&]()
			{
				const auto val = *reinterpret_cast<const std::uint16_t*>(data);
				data += sizeof(std::uint16_t);
				return val;
			};

			std::map<std::uint32_t, col_line_t> pos_map;

			const auto devmap_count = read_32();
			for (auto i = 0u; i < devmap_count; i++)
			{
				const auto script_pos = read_32();
				const auto line = read_16();
				const auto col = read_16();

				pos_map[script_pos] = { line, col };
			}

			return pos_map;
		}

		game::ScriptFile* load_custom_script(const char* file_name, const std::string& real_name)
		{
			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				return nullptr;
			}

			if (const auto itr = loaded_scripts.find(file_name); itr != loaded_scripts.end())
			{
				return itr->second.ptr;
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
				if ((real_name.starts_with("scripts/createfx") || real_name.starts_with("scripts/createart") || real_name.starts_with("scripts/mp"))
					&& (real_name.ends_with("_fx") || real_name.ends_with("_fog") || real_name.ends_with("_hdr")))
				{
					printf("Refusing to compile rawfile '%s'\n", real_name.data());
					return game::DB_FindXAssetHeader(game::ASSET_TYPE_SCRIPTFILE, file_name, false).scriptfile;
				}
			}
			*/

			try
			{
				auto& compiler = gsc_ctx->compiler();
				auto& assembler = gsc_ctx->assembler();

				std::vector<std::uint8_t> data;
				data.assign(source_buffer.begin(), source_buffer.end());

				const auto assembly_ptr = compiler.compile(real_name, data);
				const auto& [bytecode, stack, devmap] = assembler.assemble(*assembly_ptr);

				const auto script_file_ptr = static_cast<game::ScriptFile*>(scriptfile_allocator.allocate(sizeof(game::ScriptFile)));
				script_file_ptr->name = file_name;

				script_file_ptr->len = static_cast<int>(stack.size);
				script_file_ptr->bytecodeLen = static_cast<int>(bytecode.size);

				const auto stack_size = static_cast<std::uint32_t>(stack.size + 1);
				const auto byte_code_size = static_cast<std::uint32_t>(bytecode.size + 1);

				script_file_ptr->buffer = static_cast<char*>(scriptfile_allocator.allocate(stack_size));
				std::memcpy(const_cast<char*>(script_file_ptr->buffer), stack.data, stack.size);

				script_file_ptr->bytecode = allocate_buffer(byte_code_size);
				std::memcpy(script_file_ptr->bytecode, bytecode.data, bytecode.size);

				script_file_ptr->compressedLen = 0;

				loaded_script_t loaded_script{};
				loaded_script.ptr = script_file_ptr;
				loaded_script.devmap = parse_devmap(devmap);
				loaded_scripts.insert(std::make_pair(file_name, loaded_script));

				// precache all functions in their hashed form for later - this helps us with human readable errors
				// a std::uint64_t should map to a gsc_ctx->path_name
				// this is cleared on shutdown next to loaded_scripts a
				for (const auto& func : assembly_ptr->functions)
				{
					auto bruh = gsc_ctx->token_id(func->name);
					printf("caching function '%s' with id %u\n", func->name.data(), bruh);
					script_function_names[bruh] = func->name;
				}

				//
				printf("Loaded custom gsc '%s'\n", real_name.data());

				return script_file_ptr;
			}
			catch (const std::exception& e)
			{
				printf("*********** script compile error *************\n");
				printf("failed to compile '%s':\n%s\n", real_name.data(), e.what());
				printf("**********************************************\n");
				return nullptr;
			}

			return nullptr;
		}

		std::string get_script_file_name(const std::string& name)
		{
			const auto id = gsc_ctx->token_id(name);
			if (!id)
			{
				return name;
			}

			return std::to_string(id);
		}

		std::pair<xsk::gsc::buffer, std::vector<std::uint8_t>> read_compiled_script_file(const std::string& name, const std::string& real_name)
		{
			const auto* script_file = game::DB_FindXAssetHeader(game::ASSET_TYPE_SCRIPTFILE, name.data(), false).scriptfile;
			if (script_file == nullptr)
			{
				throw std::runtime_error(std::format("Could not load scriptfile '{}'", real_name));
			}

			printf("Decompiling scriptfile '%s'\n", real_name.data());

			const auto len = script_file->compressedLen;
			const std::string stack{script_file->buffer, static_cast<std::uint32_t>(len)};

			const auto decompressed_stack = utils::compression::zlib::decompress(stack);

			std::vector<std::uint8_t> stack_data;
			stack_data.assign(decompressed_stack.begin(), decompressed_stack.end());

			return {{reinterpret_cast<std::uint8_t*>(script_file->bytecode), static_cast<std::uint32_t>(script_file->bytecodeLen)}, stack_data};
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

		void load_script(const std::string& name)
		{
			const auto scr_context = game::ScriptContext_Server();

			auto token_id = gsc_ctx->token_id(name.data());
			cached_ids[token_id] = name;

			if (!game::Scr_LoadScript(scr_context, name.data()))
			{
				return;
			}

			const auto main_handle = game::Scr_GetFunctionHandle(scr_context, name.data(), gsc_ctx->token_id("main"));
			if (main_handle)
			{
				printf("Loaded '%s::main'\n", name.data());
				main_handles[name] = main_handle;
			}

			const auto init_handle = game::Scr_GetFunctionHandle(scr_context, name.data(), gsc_ctx->token_id("init"));
			if (init_handle)
			{
				printf("Loaded '%s::init'\n", name.data());
				init_handles[name] = init_handle;
			}
		}

		int db_is_x_asset_default_stub(game::XAssetType type, const char* name)
		{
			if (loaded_scripts.contains(name))
			{
				return 0;
			}

			return db_is_x_asset_default_hook.invoke<int>(type, name);
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

			for (auto& function_handle : init_handles)
			{
				printf("Executing '%s::init'\n", function_handle.first.data());
				game::Scr_FreeThread(scr_context, game::Scr_ExecThread(scr_context, function_handle.second, 0));
			}

			g_load_structs_hook.invoke<void>();
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

		void load_scripts()
		{
			if (!game::Com_FrontEnd_IsInFrontEnd())
			{
				for (const auto& path : filesystem::get_search_paths())
				{
					load_scripts(path, "scripts/"); // meant to override stock GSC
					load_scripts(path, "custom_scripts/"); // for no issues, use custom_scripts/
				}
			}
		}

		using fs_callback = std::pair<xsk::gsc::buffer, std::vector<std::uint8_t>>;
		fs_callback init_compiler_internal(const xsk::gsc::context*, const std::string& include_name)
		{
			std::string file_buffer;
			if (!read_raw_script_file(include_name, &file_buffer) || file_buffer.empty())
			{
				const auto name = get_script_file_name(include_name);
				if (game::DB_XAssetExists(game::ASSET_TYPE_SCRIPTFILE, name.data()))
				{
					return read_compiled_script_file(name, include_name);
				}

				throw std::runtime_error(std::format("Could not load gsc file '{}'", include_name));
			}

			std::vector<std::uint8_t> script_data;
			script_data.assign(file_buffer.begin(), file_buffer.end());

			return { {}, script_data };
		}

		void init_compiler()
		{
			const bool dev_script = developer_script ? developer_script->current.enabled : false;
			const auto comp_mode = dev_script ?
				xsk::gsc::build::dev :
				xsk::gsc::build::prod;

			gsc_ctx->init(comp_mode, init_compiler_internal);
		}

		void scr_begin_load_scripts_stub(game::scrContext_t* context, char threadMode, unsigned int a3)
		{
			// callbacks to begin load scripts
			for (const auto& callback : begin_scripts_callbacks)
			{
				callback();
			}

			// start the compiler
			init_compiler();

			scr_begin_load_scripts_hook.invoke<void>(context, threadMode, a3);

			// load scripts
			load_scripts();
		}

		void scr_end_load_scripts_stub(game::scrContext_t* context)
		{
			// cleanup the compiler
			gsc_ctx->cleanup();

			scr_end_load_scripts_hook.invoke<void>(context);
		}

		struct custom_text_slot
		{
			const char* prefix;       // "MP/NEURA_STR1_"
			unsigned int id;
			std::string value;        // the extracted custom text
			std::string value_copy;   // kept alive for c_str() in the output hook
		};

		static std::array<custom_text_slot, 17> custom_text_slots = { {
			{ "MP/NEURA_TITLE_",	790, "", ""},
			{ "MP/NEURA_INFO_",		791, "", ""},
			{ "MP/NEURA_ADDITIONAL_",	792, "", ""},
			{ "MP/NEURA_STR1_",	787, "", ""},
			{ "MP/NEURA_STR2_",	794, "", ""},
			{ "MP/NEURA_STR3_",	795, "", ""},
			{ "MP/NEURA_STR4_",	796, "", ""},
			{ "MP/NEURA_STR5_",	797, "", ""},
			{ "MP/NEURA_STR6_",	830, "", ""},
			{ "MP/NEURA_STR7_",	831, "", ""},
			{ "MP/NEURA_STR8_",	832, "", ""},
			{ "MP/NEURA_STR9_",	833, "", ""},
			{ "MP/NEURA_STR10_",	834, "", ""},
			{ "MP/NEURA_STR11_",	835, "", ""},
			{ "MP/NEURA_STR12_",	836, "", ""},
			{ "MP/NEURA_STR13_",	837, "", ""},
			{ "MP/NEURA_STR14_",	838, "", ""}
		} };

		utils::hook::detour NetConstStrings_GetIndexPlusOneFromName_hook;
		bool NetConstStrings_GetIndexPlusOneFromName(int type, const char* string, unsigned int* outIndex)
		{
			bool res = NetConstStrings_GetIndexPlusOneFromName_hook.invoke<bool>(type, string, outIndex);
			if (res)
			{
				return res;
			}

			for (auto& slot : custom_text_slots)
			{
				if (strstr(string, slot.prefix))
				{
					size_t base_len = strlen(slot.prefix) + 1;
					slot.value = std::string(string + base_len, strlen(string) - base_len);

					// set the out index to our overrided string, and then return true
					*outIndex = slot.id;
					return true;
				}
			}

			return res;
		}

		utils::hook::detour NetConstStrings_GetNameFromIndexPlusOne_hook;
		bool NetConstStrings_GetNameFromIndexPlusOne(int type, const unsigned int index, const char** outName)
		{
			bool res = NetConstStrings_GetNameFromIndexPlusOne_hook.invoke<bool>(type, index, outName);

			if (res && (type == 7))
			{
				for (auto& slot : custom_text_slots)
				{
					if (index == slot.id)
					{
						slot.value_copy = slot.value;
						*outName = slot.value_copy.c_str();
						break;
					}
				}
			}

			return res;
		}
	}

	game::ScriptFile* find_script(game::XAssetType type, const char* name, int allow_create_default)
	{
		auto real_name = get_script_name(name, true);

		auto* script = load_custom_script(name, real_name);
		if (script)
		{
			return script;
		}

		return game::DB_FindXAssetHeader(type, name, allow_create_default).scriptfile;
	}

	loaded_script_t* get_loaded_script(const std::string& name)
	{
		if (loaded_scripts.contains(name))
		{
			return &loaded_scripts[name];
		}
		return nullptr;
	}

	void on_begin_scripts(const std::function<void()>& callback)
	{
		begin_scripts_callbacks.push_back(callback);
	}

	inline std::string get_script_name(const char* name, bool ignore_cache)
	{
		std::string real_name = name;
		const auto id = static_cast<std::uint16_t>(std::atoi(name));

		if (id)
		{
			// check if the id passed through is actually our script
			if (!ignore_cache && cached_ids.contains(id))
			{
				return cached_ids[id];
			}

			real_name = gsc_ctx->token_name(id);
		}

		return real_name;
	}

	std::string get_function_name(std::uint32_t id)
	{
		if (const auto itr = script_function_names.find(id); itr != script_function_names.end())
		{
			return itr->second;
		}

		return scripting::find_token(id);
	}

	class loading final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override 
		{
			if (identification::game::is("1.20.4") || identification::game::is("1.20.4-replay"))
			{
				batch.add(SETUP_POINTER(game::DB_AllocXZoneMemory), "E8 ? ? ? ? 4C 8B 7C 24 ? 33 D2 41 B8", GRAB_CALL);
				batch.add(SETUP_POINTER(game::DB_AllocXZoneMemoryInternal), "E8 ? ? ? ? 48 8B 8F ? ? ? ? 4C 8B C6", GRAB_CALL);
				batch.add(SETUP_POINTER(game::G_Spawn_LoadStructs), "48 89 5C 24 ? 57 48 83 EC ? 48 8B 1D ? ? ? ? E8 ? ? ? ? 8B 53 ? 45 33 C0 48 8B C8 48 8B F8 E8 ? ? ? ?"
					" 8B D0 48 8B CF E8");

				batch.add(SETUP_POINTER(game::Scr_BeginLoadScripts), "E8 ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 85 C0", GRAB_CALL);
				batch.add(SETUP_POINTER(game::Scr_EndLoadScripts), "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B CF E8");

				if (identification::game::is_greater_or_eq("1.53.0")) {
					batch.add(SETUP_POINTER(DB_GetRawBuffer_call), "E8 ? ? ? ? 48 8B 47 ? 4C 63 67");
					batch.add(SETUP_POINTER(game::DB_GetRawBuffer), "E8 ? ? ? ? 48 8B 47 ? 4C 63 67", GRAB_CALL);
				}
				else {
					// this also works on IW9
					batch.add(SETUP_POINTER(DB_GetRawBuffer_call), "E8 ? ? ? ? 41 6B ? ? ? 00 00 1F 48 8B 4F 20 ? 63 ? 10");
					batch.add(SETUP_POINTER(game::DB_GetRawBuffer), "E8 ? ? ? ? 41 6B ? ? ? 00 00 1F 48 8B 4F 20 ? 63 ? 10", GRAB_CALL);
				}

				// inside ProcessScript
				batch.add(SETUP_POINTER(FindXAssetHeaderScript_call), "E8 ? ? ? FF 48 8B D3 B9 ? 00 00 00 48 8B F0 E8 ? ? ? FF 85 C0 75 0B 48 8B ? 48 8B ? E8 1E 00 00 00");
				
				if (identification::game::is("1.20.4-replay"))
					batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "E8 9D 42 E9 FF 85 C0 74 12 33 C0 48 8B 5C 24 30");
				else
					batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "E8 ?? ?? ?? FF 85 C0 ?? ?? 48 8B ?? 48 8B ?? E8 1E 00 00 00");
			}
		}

		void post_unpack() override
		{
			// TODO: this code should only run on 1.20.4 & 1.20.4-replay!!! iw8-mod's stuff works otherwise, but we have a 1.20 compiler A
			if (identification::game::is("1.20.4-replay"))
			{
				// Allocate script memory (PMem doesn't work)
				db_alloc_x_zone_memory_internal_hook.create(game::DB_AllocXZoneMemoryInternal, db_alloc_x_zone_memory_internal_stub);

				// Load our scripts with an uncompressed stack
				utils::hook::call(DB_GetRawBuffer_call, db_get_raw_buffer_stub);

				// Compiler start and cleanup, also loads scripts
				scr_begin_load_scripts_hook.create(game::Scr_BeginLoadScripts, scr_begin_load_scripts_stub);
				scr_end_load_scripts_hook.create(game::Scr_EndLoadScripts, scr_end_load_scripts_stub);

				// ProcessScript: hook xasset functions to return our own custom scripts
				utils::hook::call(FindXAssetHeaderScript_call, find_script);
				
				db_is_x_asset_default_hook.create(game::DB_IsXAssetDefault, db_is_x_asset_default_stub);

				g_load_structs_hook.create(game::G_Spawn_LoadStructs, g_load_structs_stub);

				// clear memory (SV_GameMP_ShutdownGameVM)
				scripting::on_shutdown([](bool free_scripts, bool is_post_shutdown)
				{
					if (free_scripts && is_post_shutdown)
					{
						printf("clearing script memory...\n");
						clear();
					}
				});
			}

			// disable patchStrings from ZeroProxy
			static const auto& game_ = identification::game::get_target_game().client_name;

			if (game_ == "iw8-mod"s)
			{
				auto patch_strings_dvar = game::Dvar_FindVarByName("ncs_patchStrings");
				if (patch_strings_dvar)
				{
#ifdef _DEBUG
					printf("setting ZeroProxy ncs_patchStrings to 0\n");
#endif

					game::Dvar_SetBool_Internal(patch_strings_dvar, false);
				}
			}

			// disable xp dec
			[[maybe_unused]] game::dvar_t* xp_dec_dvar = nullptr;

			if (game_ == "iw8-mod"s)
				xp_dec_dvar = game::Dvar_FindVarByName("NTTRLOPQKS");
			else if (game_ == "s4-mod"s)
				xp_dec_dvar = game::Dvar_FindVarByName_IW9(0xB403CABB1673EEB5);

			if (xp_dec_dvar)
			{
#ifdef _DEBUG
				printf("setting xp dec to 0\n");
#endif
				//game::Dvar_SetBool_Internal(xp_dec_dvar, false);
			}

			// fix settext
			//NetConstStrings_GetIndexPlusOneFromName_hook.create(game::NetConstStrings_GetIndexPlusOneFromName, NetConstStrings_GetIndexPlusOneFromName); // return our hardcoded ID we override
			//NetConstStrings_GetNameFromIndexPlusOne_hook.create(game::NetConstStrings_GetNameFromIndexPlusOne, NetConstStrings_GetNameFromIndexPlusOne); // return custom name for index

			// TODO: add iprintln printing to external console for ez debugging on any game
		}
	};
}

REGISTER_COMPONENT(gsc::loading)
