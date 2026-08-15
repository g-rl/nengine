#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "component/command.hpp"
#include "component/filesystem.hpp"
#include "component/scripting.hpp"
#include "game/scripting/function.hpp"
#include "game/scripting/functions.hpp"
#include "script_extension.hpp"
#include "script_loading.hpp"
#include "script_error.hpp"
#include <utils/compression.hpp>
#include <utils/hook.hpp>
#include <utils/memory.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>
#include <regex>
#include <identification/game.hpp>

namespace gsc
{
    std::unique_ptr<xsk::gsc::iw8::context> gsc_ctx = std::make_unique<xsk::gsc::iw8::context>(xsk::gsc::instance::server);
    std::unique_ptr<xsk::gsc::s4::context> gsc_ctx_s4 = std::make_unique<xsk::gsc::s4::context>(xsk::gsc::instance::server);
    std::unique_ptr<xsk::gsc::iw9::context> gsc_ctx_iw9 = std::make_unique<xsk::gsc::iw9::context>(xsk::gsc::instance::server);

    std::unordered_map<std::string, loaded_script_t> loaded_scripts;
    std::unordered_map<std::uint64_t, loaded_script_t> loaded_scripts_iw9;

    namespace
    {
        template <typename Callback>
        decltype(auto) with_current_context(Callback&& callback)
        {
            static const auto& game_ = identification::game::get_target_game().client_name;
            if (game_ == "s4-mod"s)
                return callback(*gsc_ctx_s4);
            if (game_ == "iw9-mod"s)
                return callback(*gsc_ctx_iw9);
            return callback(*gsc_ctx);
        }

        template <typename Callback>
        decltype(auto) with_token_context(Callback&& callback)
        {
            static const auto& game_ = identification::game::get_target_game().client_name;
            if (game_ == "s4-mod"s)
                return callback(*gsc_ctx_s4);
            return callback(*gsc_ctx);
        }

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
            bool patch = false;
            if (!_stricmp(filename, ALLOCATE_FASTFILE) && type == game::DM_MEMORY_SCRIPT)
            {
                patch = true;
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

        game::ScriptFile_S4* load_custom_script(game::name_or_hash file_name, const std::string& real_name)
        {
            if (game::Com_FrontEnd_IsInFrontEnd())
                return nullptr;

            static const auto& game_ = identification::game::get_target_game().client_name;

            static const auto find_loaded = [](game::name_or_hash file_name) -> void*
                {
                    if (game_ == "iw9-mod"s)
                    {
                        const auto itr = loaded_scripts_iw9.find(file_name.hash);
                        return itr != loaded_scripts_iw9.end() ? itr->second.ptr : nullptr;
                    }
                    const auto itr = loaded_scripts.find(file_name.name);
                    return itr != loaded_scripts.end() ? itr->second.ptr : nullptr;
                };

            if (const auto ptr = find_loaded(file_name))
                return reinterpret_cast<game::ScriptFile_S4*>(ptr);

            std::string source_buffer{};
            if (!read_raw_script_file(real_name, &source_buffer) || source_buffer.empty())
                return nullptr;

            printf("Loading custom gsc '%s'\n", real_name.data());

            try
            {
                return with_current_context([&](auto& ctx) -> game::ScriptFile_S4*
                    {
                        auto& compiler = ctx.compiler();
                        auto& assembler = ctx.assembler();

                        std::vector<std::uint8_t> data;
                        data.assign(source_buffer.begin(), source_buffer.end());

                        const auto assembly_ptr = compiler.compile(real_name, data);
                        const auto& [bytecode, stack, devmap] = assembler.assemble(*assembly_ptr);

                        const auto stack_size = static_cast<std::uint32_t>(stack.size + 1);
                        const auto byte_code_size = static_cast<std::uint32_t>(bytecode.size + 1);

                        auto* stack_buffer = static_cast<char*>(scriptfile_allocator.allocate(stack_size));
                        auto* bytecode_buffer = allocate_buffer(byte_code_size);

                        std::memcpy(stack_buffer, stack.data, stack.size);
                        std::memcpy(bytecode_buffer, bytecode.data, bytecode.size);

                        void* script_ptr{};

                        if (game_ == "s4-mod"s)
                        {
                            auto* script_file_ptr = static_cast<game::ScriptFile_S4*>(scriptfile_allocator.allocate(sizeof(game::ScriptFile_S4)));
                            script_file_ptr->name = file_name.name;
                            script_file_ptr->idk = nullptr;
                            script_file_ptr->compressedLen = 0;
                            script_file_ptr->len = static_cast<int>(stack.size);
                            script_file_ptr->bytecodeLen = static_cast<int>(bytecode.size);
                            script_file_ptr->buffer = stack_buffer;
                            script_file_ptr->bytecode = bytecode_buffer;
                            script_ptr = script_file_ptr;
                        }
                        else
                        {
                            auto* script_file_ptr = static_cast<game::ScriptFile*>(scriptfile_allocator.allocate(sizeof(game::ScriptFile)));
                            if (game_ == "iw9-mod"s)
                                script_file_ptr->raw_name.hash = gsc_ctx_iw9->path_id(real_name.data());
                            else
                                script_file_ptr->raw_name.name = file_name.name;

                            script_file_ptr->compressedLen = 0;
                            script_file_ptr->len = static_cast<int>(stack.size);
                            script_file_ptr->bytecodeLen = static_cast<int>(bytecode.size);
                            script_file_ptr->buffer = stack_buffer;
                            script_file_ptr->bytecode = bytecode_buffer;
                            script_ptr = script_file_ptr;
                        }

                        loaded_script_t loaded_script{};
                        loaded_script.ptr = script_ptr;
                        loaded_script.devmap = parse_devmap(devmap);

                        static const auto store_loaded = [](game::name_or_hash file_name, loaded_script_t loaded_script)
                            {
                                if (game_ == "iw9-mod"s)
                                    loaded_scripts_iw9.insert(std::make_pair(file_name.hash, loaded_script));
                                else
                                    loaded_scripts.insert(std::make_pair(file_name.name, loaded_script));
                            };

                        store_loaded(file_name, loaded_script);

                        if (game_ == "iw9-mod"s)
                        {
                            for (const auto& func : assembly_ptr->functions)
                            {
                                auto bruh = gsc_ctx_iw9->hash_id(func->name);
                                script_function_names[bruh] = func->name;
                            }
                        }

                        printf("Loaded custom gsc '%s'\n", real_name.data());
                        return reinterpret_cast<game::ScriptFile_S4*>(script_ptr);
                    });
            }
            catch (const std::exception& e)
            {
                // Try to parse compiler message for file:line:col prefixes so we
                // don't repeat filenames in the final in-game message.
                const std::string what = e.what();
                std::smatch m;
                // pattern: file:line:col: message
                static const std::regex re_file_line_col(R"(^([^:]+):(\d+):(\d+):\s*(.*)$)");
                // pattern: file:line: message
                static const std::regex re_file_line(R"(^([^:]+):(\d+):\s*(.*)$)");

                if (std::regex_match(what, m, re_file_line_col) && m.size() == 5)
                {
                    const std::string file = m[1].str();
                    const int line = std::stoi(m[2].str());
                    const int col = std::stoi(m[3].str());
                    const std::string msg = m[4].str();
                    ReportCompileError(real_name, file, line, col, msg);
                }
                else if (std::regex_match(what, m, re_file_line) && m.size() == 4)
                {
                    const std::string file = m[1].str();
                    const int line = std::stoi(m[2].str());
                    const std::string msg = m[3].str();
                    ReportCompileError(real_name, file, line, -1, msg);
                }
                else
                {
                    // Fallback: just show whole exception text once
                    ReportCompileError(real_name, real_name, what);
                }

                return nullptr;
            }
        }

        std::string get_script_file_name(const std::string& name)
        {
            static const auto& game_ = identification::game::get_target_game().client_name;
            if (game_ == "iw9-mod"s)
            {
                const auto id = gsc_ctx_iw9->hash_id(name);
                if (id)
                    return std::to_string(id);
            }

            const auto id = token_id(name);
            if (!id)
                return name;
            return std::to_string(id);
        }

        std::pair<xsk::gsc::buffer, std::vector<std::uint8_t>> read_compiled_script_file(const std::string& name, const std::string& real_name)
        {
            const auto* script_file = game::DB_FindXAssetHeader(ASSET_TYPE_SCRIPTFILE, name.data(), false).scriptfile;
            if (script_file == nullptr)
                throw std::runtime_error(std::format("Could not load scriptfile '{}'", real_name));

            printf("Decompiling scriptfile '%s'\n", real_name.data());

            const auto len = script_file->compressedLen;
            const std::string stack{ script_file->buffer, static_cast<std::uint32_t>(len) };
            const auto decompressed_stack = utils::compression::zlib::decompress(stack);

            std::vector<std::uint8_t> stack_data;
            stack_data.assign(decompressed_stack.begin(), decompressed_stack.end());

            return { {reinterpret_cast<std::uint8_t*>(script_file->bytecode), static_cast<std::uint32_t>(script_file->bytecodeLen)}, stack_data };
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
                return;

            const auto main_handle = game::Scr_GetFunctionHandle_IW9(scr_context, path_id, gsc_ctx_iw9->hash_id("main"));
            if (main_handle)
                main_handles[name] = main_handle;

            const auto init_handle = game::Scr_GetFunctionHandle_IW9(scr_context, path_id, gsc_ctx_iw9->hash_id("init"));
            if (init_handle)
                init_handles[name] = init_handle;
        }

        void load_script(const std::string& name)
        {
            static const auto& game_ = identification::game::get_target_game().client_name;
            auto* scr_context = game::ScriptContext_Server();

            if (game_ == "iw9-mod"s)
            {
                const auto path_id = gsc_ctx_iw9->path_id(name.data());
                cached_ids[path_id] = name;
                load_script_iw9(scr_context, path_id, name);
            }
            else
            {
                printf("loading script '%s'\n", name.data());
                if (!game::Scr_LoadScript(scr_context, name.data()))
                    return;

                const auto main_handle = game::Scr_GetFunctionHandle(scr_context, name.data(), token_id("main"));
                if (main_handle)
                    main_handles[name] = main_handle;

                const auto init_handle = game::Scr_GetFunctionHandle(scr_context, name.data(), token_id("init"));
                if (init_handle)
                    init_handles[name] = init_handle;
            }
        }

        int db_is_x_asset_default_stub(game::XAssetType type, game::name_or_hash name)
        {
            static const auto& game_ = identification::game::get_target_game().client_name;

            if (game_ == "iw9-mod"s)
            {
                if (loaded_scripts_iw9.contains(name.hash))
                    return 0;
                return db_is_x_asset_default_hook.invoke<int>(type, name.hash);
            }

            if (loaded_scripts.contains(name.name))
                return 0;

            return db_is_x_asset_default_hook.invoke<int>(type, name.name);
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
                auto exec = game::Scr_ExecThread(scr_context, function_handle.second, 0);
                game::Scr_FreeThread(scr_context, exec);
            }

            for (auto& function_handle : init_handles)
            {
                printf("Executing '%s::init'\n", function_handle.first.data());
                auto exec = game::Scr_ExecThread(scr_context, function_handle.second, 0);
                game::Scr_FreeThread(scr_context, exec);
            }

            gscr_load_level_hook.invoke<void>();
        }

        void load_scripts(const std::filesystem::path& root_dir, const std::filesystem::path& subfolder)
        {
            std::filesystem::path script_dir = root_dir / subfolder;
            if (!utils::io::directory_exists(script_dir.generic_string()))
                return;

            const auto scripts = utils::io::list_files(script_dir.generic_string());
            for (const auto& script : scripts)
            {
                if (!script.ends_with(".gsc"))
                    continue;

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
                    load_scripts(path, "scripts/");
                    load_scripts(path, "custom_scripts/");
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
                    return read_compiled_script_file(name, include_name);

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

            const bool dev_script = true;
            const auto comp_mode = dev_script ? xsk::gsc::build::dev : xsk::gsc::build::prod;

            with_current_context([&](auto& ctx)
                {
                    ctx.init(comp_mode, init_compiler_internal);
                });
        }

        void scr_begin_load_scripts_stub(game::scrContext_t* context, char threadMode, unsigned int a3)
        {
            init_compiler();
            scr_begin_load_scripts_hook.invoke<void>(context, threadMode, a3);
            load_scripts();
        }

        void scr_begin_load_scripts_stub_s4(void* loadArray, int scriptThreadMode, const char* gameType, bool isFrontEnd,
            const char* mapName, int gamemode, bool botsEnabled, bool agentsEnabled)
        {
            init_compiler();
            scr_begin_load_scripts_hook.invoke<void>(loadArray, scriptThreadMode, gameType, isFrontEnd, mapName, gamemode, botsEnabled, agentsEnabled);
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
            gsc_ctx_s4->cleanup();
            gsc_ctx_iw9->cleanup();
            scr_end_load_scripts_hook.invoke<void>(context);
        }

        // ==================== Fixed CUSTOM TEXT SYSTEM ====================
        struct custom_text_slot
        {
            unsigned int id;
            std::string value;
            std::string value_copy;
        };

        static std::array<custom_text_slot, 32> custom_text_slots = { {
            {790, "", ""}, {791, "", ""}, {792, "", ""}, {787, "", ""},
            {794, "", ""}, {795, "", ""}, {796, "", ""}, {797, "", ""},
            {830, "", ""}, {831, "", ""}, {832, "", ""}, {833, "", ""},
            {834, "", ""}, {835, "", ""}, {836, "", ""}, {837, "", ""},
            {838, "", ""}, {839, "", ""}, {840, "", ""}, {841, "", ""},
            {842, "", ""}, {843, "", ""}, {844, "", ""}, {845, "", ""},
            {846, "", ""}, {847, "", ""}, {848, "", ""}, {849, "", ""},
            {850, "", ""}, {851, "", ""}, {852, "", ""}, {853, "", ""}
        } };

        static size_t next_slot = 0;

        bool looks_like_hud_text(const char* str)
        {
            if (!str || !str[0])
                return false;

            // Only intercept strings that look like HUD text (color codes or newlines)
            return strchr(str, '^') != nullptr || strchr(str, '\n') != nullptr;
        }

        unsigned int get_or_alloc_custom_text(const char* string)
        {
            // Reuse existing slot if same text
            for (auto& slot : custom_text_slots)
            {
                if (!slot.value.empty() && slot.value == string)
                    return slot.id;
            }

            // Allocate next slot (round-robin)
            auto& slot = custom_text_slots[next_slot];
            slot.value = string;
            next_slot = (next_slot + 1) % custom_text_slots.size();
            return slot.id;
        }

        utils::hook::detour NetConstStrings_GetIndexPlusOneFromName_hook;
        bool NetConstStrings_GetIndexPlusOneFromName(int type, const char* string, unsigned int* outIndex)
        {
            bool res = NetConstStrings_GetIndexPlusOneFromName_hook.invoke<bool>(type, string, outIndex);
            if (res)
                return true;

            // Only hijack strings that look like HUD text
            if (looks_like_hud_text(string) && outIndex)
            {
                *outIndex = get_or_alloc_custom_text(string);
                return true;
            }

            return false;
        }

        utils::hook::detour NetConstStrings_GetNameFromIndexPlusOne_hook;
        bool NetConstStrings_GetNameFromIndexPlusOne(int type, const unsigned int index, const char** outName)
        {
            bool res = NetConstStrings_GetNameFromIndexPlusOne_hook.invoke<bool>(type, index, outName);

            if (type == 7)
            {
                for (auto& slot : custom_text_slots)
                {
                    if (index == slot.id && !slot.value.empty())
                    {
                        slot.value_copy = slot.value;
                        *outName = slot.value_copy.c_str();
                        return true;
                    }
                }
            }

            return res;
        }
    }

    game::ScriptFile_S4* find_script(game::XAssetType type, game::name_or_hash name, int allow_create_default)
    {
        auto real_name = get_script_name(name);
        auto* script = load_custom_script(name, real_name);
        if (script)
            return script;

        static const auto& game_ = identification::game::get_target_game().client_name;
        if (game_ == "iw9-mod"s)
            return game::DB_FindXAssetHeader_IW9(type, name.hash, allow_create_default).scriptfile;

        return game::DB_FindXAssetHeader(type, name.name, allow_create_default).scriptfile;
    }

    loaded_script_t* get_loaded_script(const std::string& name)
    {
        if (loaded_scripts.contains(name))
            return &loaded_scripts[name];
        return nullptr;
    }

    void on_begin_scripts(const std::function<void()>& callback)
    {
        begin_scripts_callbacks.push_back(callback);
    }

    std::uint32_t token_id(const std::string& name)
    {
        return with_token_context([&](auto& ctx) -> std::uint32_t
            {
                return static_cast<std::uint32_t>(ctx.token_id(name));
            });
    }

    std::string token_name(std::uint64_t id)
    {
        return with_token_context([&](auto& ctx) -> std::string
            {
                return ctx.token_name(static_cast<std::uint32_t>(id));
            });
    }

    std::string builtin_function_name(std::uint64_t id)
    {
        return with_current_context([&](auto& ctx) -> std::string
            {
                return ctx.func_name(static_cast<std::uint16_t>(id));
            });
    }

    std::string builtin_method_name(std::uint64_t id)
    {
        return with_current_context([&](auto& ctx) -> std::string
            {
                return ctx.meth_name(static_cast<std::uint16_t>(id));
            });
    }

    bool builtin_function_exists(const std::string& name)
    {
        return with_current_context([&](auto& ctx) -> bool
            {
                return ctx.func_exists(name);
            });
    }

    bool builtin_method_exists(const std::string& name)
    {
        return with_current_context([&](auto& ctx) -> bool
            {
                return ctx.meth_exists(name);
            });
    }

    std::uint16_t builtin_function_id(const std::string& name)
    {
        return with_current_context([&](auto& ctx) -> std::uint16_t
            {
                return static_cast<std::uint16_t>(ctx.func_id(name));
            });
    }

    std::uint16_t builtin_method_id(const std::string& name)
    {
        return with_current_context([&](auto& ctx) -> std::uint16_t
            {
                return static_cast<std::uint16_t>(ctx.meth_id(name));
            });
    }

    void add_builtin_function(const std::string& name, std::uint16_t id)
    {
        with_current_context([&](auto& ctx)
            {
                ctx.func_add(name, id);
            });
    }

    void add_builtin_method(const std::string& name, std::uint16_t id)
    {
        with_current_context([&](auto& ctx)
            {
                ctx.meth_add(name, id);
            });
    }

    int find_builtin_index(const std::string& name, const bool prefer_global)
    {
        const auto target = utils::string::to_lower(name);
        return with_current_context([&](auto& ctx) -> int
            {
                const auto& functions = ctx.func_map();
                const auto& methods = ctx.meth_map();

                if (!prefer_global)
                {
                    if (const auto itr = methods.find(target); itr != methods.end())
                        return static_cast<int>(itr->second);
                    if (const auto itr = functions.find(target); itr != functions.end())
                        return static_cast<int>(itr->second);
                }

                if (const auto itr = functions.find(target); itr != functions.end())
                    return static_cast<int>(itr->second);
                if (const auto itr = methods.find(target); itr != methods.end())
                    return static_cast<int>(itr->second);

                return -1;
            });
    }

    std::optional<std::string> opcode_name(const std::uint8_t opcode)
    {
        try
        {
            return with_current_context([&](auto& ctx) -> std::optional<std::string>
                {
                    const auto index = ctx.opcode_enum(opcode);
                    return { ctx.opcode_name(index) };
                });
        }
        catch (...)
        {
            return {};
        }
    }

    bool is_builtin_call_opcode(const std::uint8_t opcode)
    {
        return with_current_context([&](auto& ctx) -> bool
            {
                return (opcode >= ctx.opcode_id(xsk::gsc::opcode::OP_CallBuiltin0) && opcode <= ctx.opcode_id(xsk::gsc::opcode::OP_CallBuiltin))
                    || (opcode >= ctx.opcode_id(xsk::gsc::opcode::OP_CallBuiltinMethod0) && opcode <= ctx.opcode_id(xsk::gsc::opcode::OP_CallBuiltinMethod));
            });
    }

    inline std::string get_script_name(const char* name)
    {
        std::string real_name = name;
        const auto id = static_cast<std::uint16_t>(std::atoi(name));
        if (id)
            real_name = token_name(id);
        return real_name;
    }

    inline std::string get_script_name_iw9(std::uint64_t hash)
    {
        if (cached_ids.contains(hash))
            return cached_ids[hash];
        return gsc_ctx_iw9->path_name(hash);
    }

    std::string get_script_name(game::name_or_hash raw_name)
    {
        static const auto& game_ = identification::game::get_target_game().client_name;
        if (game_ == "iw9-mod"s)
            return get_script_name_iw9(raw_name.hash);
        return get_script_name(raw_name.name);
    }

    std::string get_function_name(game::name_or_hash raw_name)
    {
        static const auto& game_ = identification::game::get_target_game().client_name;
        if (game_ != "iw9-mod"s)
            return raw_name.name;

        if (const auto itr = script_function_names.find(raw_name.hash); itr != script_function_names.end())
            return itr->second;

        return "<unknown>";
    }

    class loading final : public component_interface
    {
    public:
        void find_signatures(memory::signature_store& batch) override
        {
            static const auto& game_ = identification::game::get_target_game().client_name;
            if (game_ == "iw8-mod"s)
                return;

            batch.add(SETUP_POINTER(game::DB_AllocXZoneMemory), "E8 ? ? 00 00 4C 8B ? ? ? 33 D2 41 B8", GRAB_CALL);
            batch.add(SETUP_POINTER(game::DB_AllocXZoneMemoryInternal), "E8 ? ? ? ? 48 8B 8F ? ? ? ? 4C 8B C6", GRAB_CALL);
            batch.add(SETUP_POINTER(game::GScr_LoadLevel), "E8 ? ? ? ? 33 D2 33 C9 E8 ? ? 00 00 83 ? 01 75 0C 48 8B", GRAB_CALL);

            if (game_ == "iw9-mod"s)
            {
                batch.add(SETUP_POINTER(game::Scr_BeginLoadScripts), "48 89 5C 24 08 57 48 83 EC 20 48 8B D9 C6 81 7C 12 00 00 01 88 91 7D 12 00 00 E8");
                batch.add(SETUP_POINTER(FindXAssetHeaderScript_call), "E8 ? ? ? FF B9 ? 00 00 00 48 8B D8 48 8B 10 E8 ? ? ? ? ? C0 75 0B 48");
            }
            else
            {
                batch.add(SETUP_POINTER(game::Scr_BeginLoadScripts), "E8 ? ? ? ? C7 44 24 ? ? ? ? ? E8 ? ? ? ? 85 C0", GRAB_CALL);
                batch.add(SETUP_POINTER(FindXAssetHeaderScript_call), "E8 ? ? ? FF 48 8B D3 B9 ? 00 00 00 48 8B F0 E8 ? ? ? FF 85 C0 75 0B 48 8B ? 48 8B ? E8 1E 00 00 00");
            }

            if (game_ == "s4-mod"s)
            {
                batch.add(SETUP_POINTER(game::Scr_BeginLoadScripts_S4), "E8 ? ? ? ? 33 FF 39 ? ? ? 0F ? ? 00 00 00 48 ? ? ? ? ? 00 00 48 8D", GRAB_CALL);
            }

            batch.add(SETUP_POINTER(game::Scr_EndLoadScripts), "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B CF E8");

            if (identification::game::is_greater_or_eq("1.53.0"))
            {
                batch.add(SETUP_POINTER(DB_GetRawBuffer_call), "E8 ? ? ? ? 48 8B 47 ? 4C 63 67");
                batch.add(SETUP_POINTER(game::DB_GetRawBuffer), "E8 ? ? ? ? 48 8B 47 ? 4C 63 67", GRAB_CALL);
            }
            else
            {
                batch.add(SETUP_POINTER(DB_GetRawBuffer_call), "E8 ? ? ? ? 41 6B ? ? ? 00 00 1F 48 8B 4F ? ? 63");
                batch.add(SETUP_POINTER(game::DB_GetRawBuffer), "E8 ? ? ? ? 41 6B ? ? ? 00 00 1F 48 8B 4F ? ? 63", GRAB_CALL);
            }

            if (game_ == "iw9-mod"s)
                batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "8B 10 E8 ? ? ? FF ? C0 ? ? 48 8B ? 48 8B ? E8 ? 00 00 00", SETUP_MOD(add(2)));
            else if (identification::game::is("1.20.4-replay"))
                batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "E8 9D 42 E9 FF 85 C0 74 12 33 C0 48 8B 5C 24 30");
            else
                batch.add(SETUP_POINTER(IsXAssetDefaultScript_call), "E8 ?? ?? ?? FF 85 C0 ?? ?? 48 8B ?? 48 8B ?? E8 1E 00 00 00");
        }

        void post_unpack() override
        {
            static const auto& game_ = identification::game::get_target_game().client_name;

            if (game_ != "iw8-mod"s)
            {
                const auto is_game_s4 = game_ == "s4-mod"s;
                const auto is_game_iw9 = game_ == "iw9-mod"s;

                ALLOCATE_FASTFILE = "code_post_gfx";
                ALLOCATE_SCRIPT_POOL = 6;
                ASSET_TYPE_SCRIPTFILE = game::ASSET_TYPE_SCRIPTFILE;

                if (is_game_s4)
                {
                    ALLOCATE_FASTFILE = "global_mp";
                    ALLOCATE_SCRIPT_POOL = 8;
                    ASSET_TYPE_SCRIPTFILE = game::ASSET_TYPE_SCRIPTFILE_S4;
                }
                else if (is_game_iw9)
                {
                    ALLOCATE_FASTFILE = "global_shared_mp";
                    ALLOCATE_SCRIPT_POOL = 10;
                    ASSET_TYPE_SCRIPTFILE = game::ASSET_TYPE_SCRIPTFILE_IW9;
                }

                db_alloc_x_zone_memory_internal_hook.create(game::DB_AllocXZoneMemory, db_alloc_x_zone_memory_internal_stub);
                scr_end_load_scripts_hook.create(game::Scr_EndLoadScripts, scr_end_load_scripts_stub);

                if (is_game_s4)
                {
                    utils::hook::call(DB_GetRawBuffer_call, db_get_raw_buffer_stub);
                    scr_begin_load_scripts_hook.create(game::Scr_BeginLoadScripts_S4, scr_begin_load_scripts_stub_s4);
                }
                else if (is_game_iw9)
                {
                    utils::hook::call(DB_GetRawBuffer_call, db_get_raw_buffer_stub_iw9);
                    scr_begin_load_scripts_hook.create(game::Scr_BeginLoadScripts, scr_begin_load_scripts_stub_iw9);
                }

                utils::hook::call(FindXAssetHeaderScript_call, find_script);
                db_is_x_asset_default_hook.create(game::DB_IsXAssetDefault, db_is_x_asset_default_stub);
                gscr_load_level_hook.create(game::GScr_LoadLevel, gscr_load_level_stub);

                scripting::on_shutdown([](bool free_scripts, bool is_post_shutdown)
                    {
                        if (free_scripts && is_post_shutdown)
                            clear();
                    });
            }

            if (game_ == "iw8-mod"s)
            {
                auto patch_strings_dvar = game::Dvar_FindVarByName("ncs_patchStrings");
                if (patch_strings_dvar)
                    game::Dvar_SetBool_Internal(patch_strings_dvar, false);
            }

            // Fixed custom setText system
            NetConstStrings_GetIndexPlusOneFromName_hook.create(game::NetConstStrings_GetIndexPlusOneFromName, NetConstStrings_GetIndexPlusOneFromName);
            NetConstStrings_GetNameFromIndexPlusOne_hook.create(game::NetConstStrings_GetNameFromIndexPlusOne, NetConstStrings_GetNameFromIndexPlusOne);
        }
    };
}

REGISTER_COMPONENT(gsc::loading)
