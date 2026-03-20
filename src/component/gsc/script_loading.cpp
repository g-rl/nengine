#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "component/command.hpp"
#include "component/filesystem.hpp"
#include "component/scripting.hpp"

#include "game/scripting/function.hpp"

#include "script_extension.hpp"
#include "script_loading.hpp"

#include <utils/compression.hpp>
#include <utils/hook.hpp>
#include <utils/memory.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>

namespace gsc
{
	std::unique_ptr<xsk::gsc::iw8::context> gsc_ctx = std::make_unique<xsk::gsc::iw8::context>();
	std::unordered_map<std::string, loaded_script_t> loaded_scripts;

	namespace
	{
		utils::hook::detour scr_begin_load_scripts_hook;
		utils::hook::detour scr_end_load_scripts_hook;

		std::unordered_map<std::string, std::uint32_t> main_handles;
		std::unordered_map<std::string, std::uint32_t> init_handles;

		utils::memory::allocator scriptfile_allocator;

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

		std::string get_raw_script_file_name(const std::string& name)
		{
			if (name.ends_with(".gsh"))
			{
				return name;
			}

			return name + ".gsc";
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

		int db_is_x_asset_default(game::XAssetType type, const char* name)
		{
			if (loaded_scripts.contains(name))
			{
				return 0;
			}

			return game::DB_IsXAssetDefault(type, name);
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
			const auto real_name = get_raw_script_file_name(include_name);

			std::string file_buffer;
			if (!read_raw_script_file(real_name, &file_buffer) || file_buffer.empty())
			{
				const auto name = get_script_file_name(include_name);
				if (game::DB_XAssetExists(game::ASSET_TYPE_SCRIPTFILE, name.data()))
				{
					return read_compiled_script_file(name, real_name);
				}

				throw std::runtime_error(std::format("Could not load gsc file '{}'", real_name));
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
		std::string real_name = name;
		const auto id = static_cast<std::uint16_t>(std::atoi(name));
		if (id)
		{
			real_name = gsc_ctx->token_name(id);
		}

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
			utils::hook::call(0x13223B6_b, db_get_raw_buffer_stub);

			// Compiler start and cleanup, also loads scripts
			scr_begin_load_scripts_hook.create(0x1316DE0_b, scr_begin_load_scripts_stub);
			scr_end_load_scripts_hook.create(0x1316FE0_b, scr_end_load_scripts_stub);

			// ProcessScript: hook xasset functions to return our own custom scripts
			utils::hook::call(0x132230E_b, find_script);
			utils::hook::call(0x132231E_b, db_is_x_asset_default);

			// execute main handle after G_LoadStructs (now called G_Spawn_LoadStructs)
			g_load_structs_hook.create(0xFC80A0_b, g_load_structs_stub);

			// fix settext
			NetConstStrings_GetIndexPlusOneFromName_hook.create(0x10F0F20_b, NetConstStrings_GetIndexPlusOneFromName); // return our hardcoded ID we override
			NetConstStrings_GetNameFromIndexPlusOne_hook.create(0x10F1030_b, NetConstStrings_GetNameFromIndexPlusOne); // return custom name for index

			command::add("dumplocalization", []()
			{
				std::string data;
				int count = 0;
				for (unsigned int i = 1; ; ++i)
				{
					const char* name = nullptr;
					auto res = NetConstStrings_GetNameFromIndexPlusOne(7, i, &name);
					if (!res)
						break;
					data += std::format("type 7, index {}, name: {}\n", i, name ? name : "(null)");
					count++;
				}
				printf("Dumped %d locstrings\n", count);
				utils::io::write_file("iw8-mod/loc_strings.txt", data);
			});

			// clear memory (SV_GameMP_ShutdownGameVM)
			scripting::on_shutdown([](bool free_scripts, bool is_post_shutdown)
			{
				if (!is_post_shutdown)
				{
					clear();
				}
			});
		}
	};
}

REGISTER_COMPONENT(gsc::loading)
