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
	std::unique_ptr<xsk::gsc::iw8::context> gsc_ctx = std::make_unique<xsk::gsc::iw8::context>(xsk::gsc::instance::server);
	std::unique_ptr<xsk::gsc::iw9::context> gsc_ctx_iw9 = std::make_unique<xsk::gsc::iw9::context>(xsk::gsc::instance::server);

	std::unordered_map<std::string, loaded_script_t> loaded_scripts;
	std::unordered_map<std::uint64_t, loaded_script_t> loaded_scripts_iw9;

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

		std::unordered_map<std::uint64_t, std::string> cached_ids;
		std::unordered_map<std::uint64_t, std::string> script_function_names;

		char* script_mem_buf = nullptr;

		std::vector<std::function<void()>> begin_scripts_callbacks;

		const char* ALLOCATE_FASTFILE;
		int ALLOCATE_SCRIPT_POOL;
		game::XAssetType ASSET_TYPE_SCRIPTFILE;

		struct
		{
			char* buf = nullptr;
			char* pos = nullptr;
			const std::uint64_t size = 0x50000i64;
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
			loaded_scripts_iw9.clear();
			scriptfile_allocator.clear();
			free_script_memory();
		}

		void db_alloc_x_zone_memory_internal(unsigned __int64* blockSize, const char* filename, game::XZoneMemory* zoneMem, game::XBlock* archiveBlocks, int type)
		{
			bool patch = false; // ugly fix for script memory allocation

			if (!_stricmp(filename, ALLOCATE_FASTFILE) && type == game::DM_MEMORY_SCRIPT)
			{
				patch = true;
				printf("patching memory for '%s'\n", ALLOCATE_FASTFILE);
			}

			if (patch)
			{
				blockSize[ALLOCATE_SCRIPT_POOL] += script_memory.size;
			}

			game::DB_AllocXZoneMemoryInternal(blockSize, filename, zoneMem, archiveBlocks, type);

			if (patch)
			{
				blockSize[ALLOCATE_SCRIPT_POOL] -= script_memory.size;
				script_mem_buf = archiveBlocks[ALLOCATE_SCRIPT_POOL].data + blockSize[ALLOCATE_SCRIPT_POOL];
			}
		}

		utils::hook::detour db_alloc_x_zone_memory_internal_hook;
		void db_alloc_x_zone_memory_internal_stub(unsigned __int64* blockSize, const char* filename, game::XZoneMemory* zoneMem, game::XBlock* archiveBlocks)
		{
			for (auto i = 0; i < 4; ++i)
			{
				db_alloc_x_zone_memory_internal(blockSize, filename, zoneMem, archiveBlocks, i);
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
				return reinterpret_cast<game::ScriptFile*>(itr->second.ptr);
			}

			std::string source_buffer{};
			if (!read_raw_script_file(real_name, &source_buffer) || source_buffer.empty())
			{
				return nullptr;
			}

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

				// TODO
				for (const auto& func : assembly_ptr->functions)
				{
					auto bruh = 0; // gsc_ctx->token_id(func->name);
					printf("caching function '%s' with id %u\n", func->name.data(), bruh);
					script_function_names[bruh] = func->name;
				}

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

		game::ScriptFile_IW9* load_custom_script_iw9(std::uint64_t path_id, const std::string& real_name)
		{
			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				return nullptr;
			}

			// put this above to override vlobby scripts
			if (const auto itr = loaded_scripts_iw9.find(path_id); itr != loaded_scripts_iw9.end())
			{
				return reinterpret_cast<game::ScriptFile_IW9*>(itr->second.ptr);
			}

			std::string source_buffer{};
			if (!read_raw_script_file(real_name, &source_buffer) || source_buffer.empty())
			{
				return nullptr;
			}

			printf("Loading custom gsc '%s'\n", real_name.data());

			try
			{
				auto& compiler = gsc_ctx_iw9->compiler();
				auto& assembler = gsc_ctx_iw9->assembler();

				std::vector<std::uint8_t> data;
				data.assign(source_buffer.begin(), source_buffer.end());

				const auto assembly_ptr = compiler.compile(real_name, data);
				const auto& [bytecode, stack, devmap] = assembler.assemble(*assembly_ptr);

				auto* script_file_ptr = static_cast<game::ScriptFile_IW9*>(scriptfile_allocator.allocate(sizeof(game::ScriptFile_IW9)));
				auto file_name = gsc_ctx_iw9->path_id(real_name.data());
				script_file_ptr->name = file_name;

				script_file_ptr->len = static_cast<int>(stack.size);
				script_file_ptr->bytecodeLen = static_cast<int>(bytecode.size);

				const auto stack_size = static_cast<std::uint32_t>(stack.size + 1);
				const auto byte_code_size = static_cast<std::uint32_t>(bytecode.size + 1);

				script_file_ptr->buffer = allocate_buffer(stack_size);
				std::memcpy(script_file_ptr->buffer, stack.data, stack.size);

				script_file_ptr->bytecode = allocate_buffer(byte_code_size);
				std::memcpy(script_file_ptr->bytecode, bytecode.data, bytecode.size);

				script_file_ptr->pad = 0; // idk what this is, just 0 it
				script_file_ptr->compressedLen = 0;

				loaded_script_t loaded_script{};
				loaded_script.ptr = script_file_ptr;
				loaded_script.devmap = parse_devmap(devmap);
				loaded_scripts_iw9.insert(std::make_pair(file_name, loaded_script));

				// precache all functions in their hashed form for later - this helps us with human readable errors
				// a std::uint64_t should map to a gsc_ctx_iw9->path_name
				// this is cleared on shutdown next to loaded_scripts
				for (const auto& func : assembly_ptr->functions)
				{
					auto bruh = gsc_ctx_iw9->hash_id(func->name);
					script_function_names[bruh] = func->name;
				}

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
			static const auto& game_ = identification::game::get_target_game().client_name;

			if (game_ == "iw9-mod"s)
			{
				const auto id = gsc_ctx_iw9->hash_id(name);
				if (id)
				{
					return std::to_string(id);
				}
			}

			const auto id = gsc_ctx->token_id(name);
			if (!id)
			{
				return name;
			}

			return std::to_string(id);
		}

		std::pair<xsk::gsc::buffer, std::vector<std::uint8_t>> read_compiled_script_file(const std::string& name, const std::string& real_name)
		{
			const auto* script_file = game::DB_FindXAssetHeader(ASSET_TYPE_SCRIPTFILE, name.data(), false).scriptfile;
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

		void db_get_raw_buffer_stub(game::RawFile* rawfile, char* buf, const int size)
		{
			if (rawfile->len > 0 && rawfile->compressedLen == 0)
			{
				std::memset(buf, 0, size);
				std::memcpy(buf, rawfile->buffer, std::min(rawfile->len, size));
				return;
			}

			game::DB_GetRawBuffer(rawfile, buf, size);
		}

		void db_get_raw_buffer_stub_iw9(game::RawFile_IW9* rawfile, char* buf, const int size)
		{
			if (rawfile->len > 0 && rawfile->compressedLen == 0)
			{
				std::memset(buf, 0, size);
				std::memcpy(buf, rawfile->buffer, std::min(rawfile->len, size));
				return;
			}

			game::DB_GetRawBuffer(rawfile, buf, size);
		}

		void load_script_iw9(game::scrContext_t* scr_context, std::uint64_t path_id, const std::string& name)
		{
			if (!game::Scr_LoadScript_IW9(scr_context, path_id))
			{
				return;
			}

			const auto main_handle = game::Scr_GetFunctionHandle_IW9(scr_context, path_id, gsc_ctx_iw9->hash_id("main"));
			if (main_handle)
			{
				printf("Loaded '%s::main'\n", name.data());
				main_handles[name] = main_handle;
			}

			const auto init_handle = game::Scr_GetFunctionHandle_IW9(scr_context, path_id, gsc_ctx_iw9->hash_id("init"));
			if (init_handle)
			{
				printf("Loaded '%s::init'\n", name.data());
				init_handles[name] = init_handle;
			}
		}

		void load_script(const std::string& name)
		{
			static const auto& game_ = identification::game::get_target_game().client_name;

			auto* scr_context = game::ScriptContext_Server();

			if (game_ == "iw9-mod"s)
			{
				const auto path_id = gsc_ctx_iw9->path_id(name.data());
				//printf("[load_script] caching and loading script %" PRIu64 " (%s)\n", path_id, name.data());
				cached_ids[path_id] = name;

				load_script_iw9(scr_context, path_id, name);
			}
			else
			{
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
		}

		int db_is_x_asset_default_stub(game::XAssetType type, const char* name)
		{
			if (loaded_scripts.contains(name))
				return 0;
			return db_is_x_asset_default_hook.invoke<int>(type, name);
		}

		int db_is_x_asset_default_stub_iw9(game::XAssetType type, std::uint64_t name)
		{
			if (loaded_scripts_iw9.contains(name))
				return 0;
			return db_is_x_asset_default_hook.invoke<int>(type, name);
		}

		utils::hook::detour gscr_load_level_hook;
		void gscr_load_level_stub()
		{
			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				gscr_load_level_hook.invoke<void>();
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

			gscr_load_level_hook.invoke<void>();
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
				if (game::DB_XAssetExists(ASSET_TYPE_SCRIPTFILE, name.data()))
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
			for (const auto& callback : begin_scripts_callbacks)
				callback();

			const bool dev_script = developer_script ? developer_script->current.enabled : false;
			const auto comp_mode = dev_script ?
				xsk::gsc::build::dev :
				xsk::gsc::build::prod;

			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw9-mod"s)
				gsc_ctx_iw9->init(comp_mode, init_compiler_internal);
			else
				gsc_ctx->init(comp_mode, init_compiler_internal);
		}

		void scr_begin_load_scripts_stub(game::scrContext_t* context, char threadMode, unsigned int a3)
		{
			init_compiler();
			scr_begin_load_scripts_hook.invoke<void>(context, threadMode, a3);
			load_scripts();
		}

		void scr_begin_load_scripts_stub_iw9(void* a1, char a2)
		{
			init_compiler();
			scr_begin_load_scripts_hook.invoke<void>(a1, a2);
			load_scripts();
		}

		void scr_end_load_scripts_stub(game::scrContext_t* context)
		{
			gsc_ctx->cleanup();
			gsc_ctx_iw9->cleanup();
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
		auto real_name = get_script_name(name);

		auto* script = load_custom_script(name, real_name);
		if (script)
		{
			return script;
		}

		return game::DB_FindXAssetHeader(type, name, allow_create_default).scriptfile;
	}

	game::ScriptFile_IW9* find_script_iw9(game::XAssetType type, std::uint64_t name, int allow_create_default)
	{
		auto real_name = get_script_name_iw9(name);

		auto* script = load_custom_script_iw9(name, real_name);
		if (script)
		{
			return script;
		}

		return game::DB_FindXAssetHeader_IW9(type, name, allow_create_default).scriptfile;
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

	inline std::string get_script_name_iw9(std::uint64_t hash)
	{
		if (cached_ids.contains(hash))
		{
			return cached_ids[hash];
		}
		return gsc_ctx_iw9->path_name(hash);
	}

	std::string get_function_name(std::uint64_t id)
	{
		if (const auto itr = script_function_names.find(id); itr != script_function_names.end())
		{
			return itr->second;
		}

		return "<unknown>";
	}

	class loading final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override 
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw9-mod"s || identification::game::is("1.20.4") || identification::game::is("1.20.4-replay"))
			{
				batch.add(SETUP_POINTER(game::DB_AllocXZoneMemory), "E8 ? ? ? ? 4C 8B 7C 24 ? 33 D2 41 B8", GRAB_CALL);
				batch.add(SETUP_POINTER(game::DB_AllocXZoneMemoryInternal), "E8 ? ? ? ? 48 8B 8F ? ? ? ? 4C 8B C6", GRAB_CALL);
				batch.add(SETUP_POINTER(game::GScr_LoadLevel), "E8 ? ? ? ? 33 D2 33 C9 E8 ? ? 00 00 83 ? 01 75 0C 48 8B", GRAB_CALL);

				if (game_ == "iw9-mod"s)
				{
					batch.add(SETUP_POINTER(game::Scr_BeginLoadScripts), "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 C6 81 7C 12 00 00 01 88 91 7D 12 00 00 E8");
					batch.add(SETUP_POINTER(FindXAssetHeaderScript_call), "E8 ? ? ? FF B9 ? 00 00 00 48 8B D8 48 8B 10 E8 ? ? ? ? ? C0 75 0B 48"); // ProcessScript
				}
				else
				{
					batch.add(SETUP_POINTER(game::Scr_BeginLoadScripts), "E8 ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 85 C0", GRAB_CALL);
					batch.add(SETUP_POINTER(FindXAssetHeaderScript_call), "E8 ? ? ? FF 48 8B D3 B9 ? 00 00 00 48 8B F0 E8 ? ? ? FF 85 C0 75 0B 48 8B ? 48 8B ? E8 1E 00 00 00");
				}

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

				if (game_ == "iw9-mod"s)
					batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "8B 10 E8 ? ? ? FF ? C0 ? ? 48 8B ? 48 8B ? E8 ? 00 00 00", SETUP_MOD(add(2))); // ProcessScript
				else if (identification::game::is("1.20.4-replay"))
					batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "E8 9D 42 E9 FF 85 C0 74 12 33 C0 48 8B 5C 24 30");
				else
					batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "E8 ?? ?? ?? FF 85 C0 ?? ?? 48 8B ?? 48 8B ?? E8 1E 00 00 00");
			}
		}

		void post_unpack() override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			const auto is_game_iw9 = game_ == "iw9-mod"s;

			ALLOCATE_FASTFILE = "code_post_gfx";
			ALLOCATE_SCRIPT_POOL = 6;
			ASSET_TYPE_SCRIPTFILE = game::ASSET_TYPE_SCRIPTFILE;

			if (game_ == "s4-mod"s)
			{
				ALLOCATE_FASTFILE = "global_shared";
				ALLOCATE_SCRIPT_POOL = 8;
				ASSET_TYPE_SCRIPTFILE = game::ASSET_TYPE_SCRIPTFILE_S4;
			}
			else if (is_game_iw9)
			{
				ALLOCATE_FASTFILE = "global_shared_mp";
				ALLOCATE_SCRIPT_POOL = 10;
				ASSET_TYPE_SCRIPTFILE = game::ASSET_TYPE_SCRIPTFILE_IW9;
			}

			if (is_game_iw9 // support IW9 with duplicate functions
				|| identification::game::is("1.20.4-replay") // we handle 1.20.4-replay seperately
				)
			{
				// IW9 & S4 hook the original and handle the < 4 check ourselves
				db_alloc_x_zone_memory_internal_hook.create(game::DB_AllocXZoneMemory, db_alloc_x_zone_memory_internal_stub); // allocation

				scr_end_load_scripts_hook.create(game::Scr_EndLoadScripts, scr_end_load_scripts_stub);

				if (is_game_iw9)
				{
					utils::hook::call(DB_GetRawBuffer_call, db_get_raw_buffer_stub_iw9); // load our scripts with an uncompressed stack
					scr_begin_load_scripts_hook.create(game::Scr_BeginLoadScripts, scr_begin_load_scripts_stub_iw9);
					db_is_x_asset_default_hook.create(game::DB_IsXAssetDefault, db_is_x_asset_default_stub_iw9);
					utils::hook::call(FindXAssetHeaderScript_call, find_script_iw9); // ProcessScript: hook xasset functions to return our own custom scripts
				}
				else
				{
					utils::hook::call(DB_GetRawBuffer_call, db_get_raw_buffer_stub);
					scr_begin_load_scripts_hook.create(game::Scr_BeginLoadScripts, scr_begin_load_scripts_stub);
					db_is_x_asset_default_hook.create(game::DB_IsXAssetDefault, db_is_x_asset_default_stub);
					utils::hook::call(FindXAssetHeaderScript_call, find_script);
				}

				gscr_load_level_hook.create(game::GScr_LoadLevel, gscr_load_level_stub); // execute handles

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
			NetConstStrings_GetIndexPlusOneFromName_hook.create(game::NetConstStrings_GetIndexPlusOneFromName, NetConstStrings_GetIndexPlusOneFromName); // return our hardcoded ID we override
			NetConstStrings_GetNameFromIndexPlusOne_hook.create(game::NetConstStrings_GetNameFromIndexPlusOne, NetConstStrings_GetNameFromIndexPlusOne); // return custom name for index
		}
	};
}

REGISTER_COMPONENT(gsc::loading)
