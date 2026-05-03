#include <std_include.hpp>

#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <identification/game.hpp>

#include "script_error.hpp"
#include "script_extension.hpp"
#include "script_loading.hpp"

#include "component/scripting.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

using namespace utils::string;

namespace gsc
{
	namespace
	{
		utils::hook::detour scr_emit_function_hook;

		std::uint32_t current_filename;
		std::uint64_t current_filename_iw9;

		std::string unknown_function_error;

		void* FindVariable_call{};
		void* CompileError_bonus_call{};

		std::vector<void*> compile_error_sites_common;     // S4 (2) / IW8 (1)
		std::vector<void*> compile_error_sites_iw8_extra;  // IW8 (2)
		std::vector<void*> compile_error_sites_iw9;        // IW9 (2)

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

		void scr_emit_function_stub(game::scrContext_t* context, std::uint32_t filename, std::uint32_t thread_name, char* code_pos)
		{
			current_filename = filename;
			scr_emit_function_hook.invoke<void>(context, filename, thread_name, code_pos);
		}

		void scr_emit_function_stub_iw9(game::scrContext_t* context, std::uint64_t filename, std::uint64_t thread_name, char* code_pos)
		{
			current_filename_iw9 = filename;
			scr_emit_function_hook.invoke<void>(context, filename, thread_name, code_pos);
		}

		std::string get_emit_filename()
		{
			if (is_iw9())
				return gsc::get_script_name_iw9(current_filename_iw9);

			return scripting::get_token(current_filename);
		}

		std::string resolve_function_name(std::uint64_t thread_name)
		{
			if (is_iw9())
			{
				game::name_or_hash key{};
				key.hash = thread_name;
				return gsc::get_function_name(key);
			}
			return scripting::get_token(thread_name);
		}

		void build_unknown_function_error(const char* code_pos)
		{
			const auto current = scripting::get_current_file();
			const auto function = scripting::find_function(code_pos);
			if (function.has_value())
			{
				unknown_function_error = std::format(
					"while processing function '{}' in script '{}':\nunknown script '{}'",
					function->first, function->second, current
				);
			}
			else
			{
				unknown_function_error = std::format("unknown script '{}'", current);
			}
		}

		void build_unknown_function_error(std::uint64_t thread_name)
		{
			const auto filename = get_emit_filename();
			const auto name = resolve_function_name(thread_name);

			unknown_function_error = std::format(
				"while processing script '{}':\nunknown function '{}::{}'",
				scripting::get_current_file(), filename, name
			);
		}

		void compile_error_stub(game::scrContext_t* context, const char* code_pos)
		{
			build_unknown_function_error(code_pos);
			const auto error_msg = utils::string::va("script link error\n%s", unknown_function_error.data());
			printf("%s\n", error_msg);
			game::Com_Error(game::ERR_SCRIPT_DROP, "%s\n", error_msg);
		}

		std::uint32_t find_variable_stub(game::scrContext_t* context, std::uint32_t parent_id, std::uint32_t thread_name)
		{
			const auto res = game::FindVariable(context, parent_id, thread_name);
			if (!res)
			{
				build_unknown_function_error(thread_name);
				const auto error_msg = utils::string::va("(FindVariable) script link error\n%s", unknown_function_error.data());
				printf("%s\n", error_msg);
				game::Com_Error(game::ERR_SCRIPT_DROP, "%s\n", error_msg);
			}
			return res;
		}

		std::uint64_t find_variable_stub_iw9(game::scrContext_t* context, std::uint32_t parent_id, char a3, std::uint64_t thread_name)
		{
			const auto res = game::FindVariable_IW9(context, parent_id, a3, thread_name);
			if (!res)
			{
				build_unknown_function_error(thread_name);
				const auto error_msg = utils::string::va("(FindVariable) script link error\n%s", unknown_function_error.data());
				printf("%s\n", error_msg);
				game::Com_Error(game::ERR_SCRIPT_DROP, "%s\n", error_msg);
			}
			return res;
		}

		void apply_compile_error_hooks(const std::vector<void*>& sites)
		{
			for (auto* site : sites)
			{
				if (site)
				{
					utils::hook::call(site, compile_error_stub);
				}
			}
		}
	}

	class error final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			batch.add(SETUP_POINTER(FindVariable_call), "E8 ? ? ? 00 8B ? 85 C0 75 15 41 B8 75 04 00 00 48");

			if (is_iw9())
			{
				batch.add(SETUP_POINTER(game::Scr_EmitFunction),
					"48 89 5C 24 10 48 89 6C 24 18 56 57 41 56 48 83 EC 60 48 B8 FF");

				batch.add(SETUP_POINTER(CompileError_bonus_call),
					"48 8B 5C 24 50 48 8B D1 48 8B CB 4D ? C5 E8 30 0D 00 00", SETUP_MOD(add(14)));

				// IW9 Sys_Error call sites
				batch.add_multi(SETUP_MULTI_POINTER(compile_error_sites_iw9),
					"18 ? ? ? ? ? FF 48 8B 5C 24 38 4D 8B C7 48 8B CB 48 ? D7 E8 ? ? 00 00", 2, SETUP_MOD(add(21)));
			}
			else
			{
				batch.add(SETUP_POINTER(game::Scr_EmitFunction),
					"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC ? 41 8B E8 8B ? 44 8B C2");

				// S4 has 2, IW8 has 1
				batch.add_multi(SETUP_MULTI_POINTER(compile_error_sites_common),
					"4C 8D 05 ? ? ? ? 48 8B CB E8 ? ? 00 00 ? 8B 16 44 8B C7 8B",
					(is_s4() ? 2 : 1), SETUP_MOD(add(10)));

				if (!is_s4())
				{
					// extra CompileError sites on IW8
					batch.add_multi(SETUP_MULTI_POINTER(compile_error_sites_iw8_extra),
						"4C 8D 05 ? ? ? ? 48 8B D6 48 8B CB E8 ? ? 00 00", 2, SETUP_MOD(add(13)));
				}
			}
		}

		void post_unpack() override
		{
			if (is_iw9())
			{
				scr_emit_function_hook.create(game::Scr_EmitFunction, scr_emit_function_stub_iw9);
				utils::hook::call(FindVariable_call, find_variable_stub_iw9);

				apply_compile_error_hooks(compile_error_sites_iw9);
				if (CompileError_bonus_call)
					utils::hook::call(CompileError_bonus_call, compile_error_stub);
				return;
			}

			// IW8 + S4
			scr_emit_function_hook.create(game::Scr_EmitFunction, scr_emit_function_stub);
			utils::hook::call(FindVariable_call, find_variable_stub);

			apply_compile_error_hooks(compile_error_sites_common);
			if (!is_s4())
			{
				apply_compile_error_hooks(compile_error_sites_iw8_extra);
			}
		}
	};
}

REGISTER_COMPONENT(gsc::error)
