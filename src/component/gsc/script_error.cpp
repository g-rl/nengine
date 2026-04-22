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

		void get_unknown_function_error(const char* code_pos)
		{
			const auto current = scripting::get_current_file();
			const auto function = scripting::find_function_iw9(code_pos);
			if (function.has_value())
			{
				const auto& pos = function.value();
				unknown_function_error = std::format(
					"while processing function '{}' in script '{}':\nunknown script '{}'", get_function_name(pos.first), get_script_name_iw9(pos.second), current
				);
			}
			else
			{
				unknown_function_error = std::format("unknown script '{}'", current);
			}
		}

		std::string get_filename_name()
		{
			const auto filename_str = game::SL_ConvertToString(static_cast<game::scr_string_t>(current_filename));
			const auto id = std::atoi(filename_str);
			if (!id)
			{
				return filename_str;
			}

			return scripting::get_token(id);
		}

		void get_unknown_function_error(std::uint32_t thread_name)
		{
			const auto filename = get_filename_name();
			const auto name = scripting::get_token(thread_name);

			unknown_function_error = std::format(
				"while processing script '{}':\nunknown function '{}::{}'", scripting::get_current_file(), filename, name
			);
		}

		void get_unknown_function_error(std::uint64_t thread_name)
		{
			const auto filename = gsc::get_script_name_iw9(current_filename_iw9);
			const auto name = scripting::get_token(thread_name);

			unknown_function_error = std::format(
				"while processing script '{}':\nunknown function '{}::{}'", scripting::get_current_file(), filename, name
			);
		}

		void compile_error_stub(game::scrContext_t* context, const char* code_pos)
		{
			get_unknown_function_error(code_pos);
			const auto error_msg = utils::string::va("script link error\n%s", unknown_function_error.data());
			game::Com_Error(game::ERR_SCRIPT_DROP, "%s\n", error_msg);
			printf("%s\n", error_msg);
		}
		
		std::uint32_t find_variable_stub(game::scrContext_t* context, std::uint32_t parent_id, std::uint32_t thread_name)
		{
			const auto res = game::FindVariable(context, parent_id, thread_name);
			if (!res)
			{
				get_unknown_function_error(thread_name);
				const auto error_msg = utils::string::va("script link error\n%s", unknown_function_error.data());
				game::Com_Error(game::ERR_SCRIPT_DROP, "%s\n", error_msg);
			}
			return res;
		}

		std::uint64_t find_variable_stub_iw9(game::scrContext_t* context, std::uint32_t parent_id, char a3, std::uint64_t thread_name)
		{
			auto res = game::FindVariable_IW9(context, parent_id, a3, thread_name);
			if (!res)
			{
				get_unknown_function_error(thread_name);
				const auto error_msg = utils::string::va("DEV ERROR 1141 FindVariable\n%s", unknown_function_error.data());
				game::Com_Error(1, "%s\n", error_msg);
			}
			return res;
		}

		utils::hook::detour scr_get_object_hook;
		utils::hook::detour scr_get_const_string_hook;
		utils::hook::detour scr_get_const_istring_hook;
		utils::hook::detour scr_validate_localized_string_ref_hook;
		utils::hook::detour scr_get_vector_hook;
		utils::hook::detour scr_get_int_hook;
		utils::hook::detour scr_get_float_hook;
		utils::hook::detour scr_get_pointer_type_hook;
		utils::hook::detour scr_get_type_hook;
		utils::hook::detour scr_get_type_name_hook;

		unsigned int scr_get_object(game::scrContext_t* context, unsigned int index)
		{
			if (index < context->outparamcount)
			{
				auto* value = context->top - index;
				if (value->type == game::VAR_POINTER)
				{
					return value->u.pointerValue;
				}

				scr_error(va(__FUNCTION__ ": type %s is not an object", game::Scr_GetNameForType(value->type)));
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return 0;
		}

		unsigned int scr_get_const_string(game::scrContext_t* context, unsigned int index)
		{
			if (index < context->outparamcount)
			{
				auto* value = context->top - index;
				if (utils::hook::invoke<bool>(0x131E480_b, context, value)) // Scr_CastString
				{
					assert(value->type == game::VAR_STRING);
					return value->u.stringValue;
				}

				//game::Scr_ErrorInternal(context);
				*(DWORD*)(context + 13536) = index + 1;
				scr_error(va(__FUNCTION__ ": type %s is not a string", game::Scr_GetNameForType(value->type)));
			}

			*(DWORD*)(context + 13536) = index + 1;
			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return 0;
		}

		__int64 scr_get_const_istring(game::scrContext_t* context, unsigned int index)
		{
			if (index < context->outparamcount)
			{
				auto* value = context->top - index;
				if (value->type == game::VAR_ISTRING)
				{
					return value->u.uintValue;
				}

				*(DWORD*)(context + 13536) = index + 1;
				scr_error(va(__FUNCTION__ ": type %s is not a localized string", game::Scr_GetNameForType(value->type)));
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return 0;
		}

		void scr_validate_localized_string_ref(game::scrContext_t* context, int parm_index, const char* token, int token_len)
		{
			assert(token);
			assert(token_len >= 0);

			if (token_len < 2)
			{
				return;
			}

			for (auto char_iter = 0; char_iter < token_len; ++char_iter)
			{
				if (!std::isalnum(static_cast<unsigned char>(token[char_iter])) && token[char_iter] != '_' && token[char_iter] != '/')
				{
					scr_error(va(__FUNCTION__ ": Illegal localized string reference: %s must contain only alpha-numeric characters and underscore or '/'", token));
				}
			}
		}

		void scr_get_vector(game::scrContext_t* context, unsigned int index, float* vector_value)
		{
			if (index < context->outparamcount)
			{
				auto* value = context->top - index;
				if (value->type == game::VAR_VECTOR)
				{
					std::memcpy(vector_value, value->u.vectorValue, sizeof(std::float_t[3]));
					return;
				}

				*(DWORD*)(context + 13536) = index + 1;
				scr_error(va(__FUNCTION__ ": type %s is not a vector", game::Scr_GetNameForType(value->type)));
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
		}

		int scr_get_int(game::scrContext_t* context, unsigned int index)
		{
			printf("scr_get_int: %d < %d\n", index, context->outparamcount);
			if (index < context->outparamcount)
			{
				auto* value = context->top - index;
				if (value->type == game::VAR_INTEGER)
				{
					return value->u.intValue;
				}

				*(DWORD*)(context + 13536) = index + 1;
				scr_error(va(__FUNCTION__ ": type %s is not an int", game::Scr_GetNameForType(value->type)));
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return 0;
		}

		float scr_get_float(game::scrContext_t* context, unsigned int index)
		{
			if (index < context->outparamcount)
			{
				auto* value = context->top - index;
				if (value->type == game::VAR_FLOAT)
				{
					return value->u.floatValue;
				}

				if (value->type == game::VAR_INTEGER)
				{
					return static_cast<float>(value->u.intValue);
				}

				*(DWORD*)(context + 13536) = index + 1;
				scr_error(va(__FUNCTION__ ": type %s is not a float", game::Scr_GetNameForType(value->type)));
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return 0.0f;
		}

		int scr_get_pointer_type(game::scrContext_t* context, unsigned int index)
		{
			if (index < context->outparamcount)
			{
				if ((context->top - index)->type == game::VAR_POINTER)
				{
					return utils::hook::invoke<int>(0x131D2C0_b, context, (context->top - index)->u.uintValue); // GetObjectType
				}

				*(DWORD*)(context + 13536) = index + 1;
				scr_error(va(__FUNCTION__ ": type %s is not an object", game::Scr_GetNameForType((context->top - index)->type)));
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return 0;
		}

		int scr_get_type(game::scrContext_t* context, unsigned int index)
		{
			if (index < context->outparamcount)
			{
				return (context->top - index)->type;
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return 0;
		}

		const char* scr_get_type_name(game::scrContext_t* context, unsigned int index)
		{
			if (index < context->outparamcount)
			{
				return game::Scr_GetNameForType((context->top - index)->type);
			}

			scr_error(va(__FUNCTION__ ": parameter %u does not exist", index + 1));
			return nullptr;
		}
	}

	class error final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			batch.add(SETUP_POINTER(FindVariable_call), "E8 ? ? ? 00 8B ? 85 C0 75 15 41 B8 75 04 00 00 48");
		}

		void post_unpack() override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "s4-mod"s)
			{
				scr_emit_function_hook.create(0x21AE6F0_b, scr_emit_function_stub);

				// change Sys_Error -> Com_Error + advanced messages A
				utils::hook::call(0x21AE566_b, compile_error_stub); // CompileError (LinkFile)
				utils::hook::call(0x21AE656_b, compile_error_stub); // ^
				utils::hook::call(FindVariable_call, find_variable_stub); // Scr_EmitFunction_Precompiled
				return;
			}
			else if (game_ == "iw9-mod"s)
			{
				scr_emit_function_hook.create(0x2796D20_b, scr_emit_function_stub_iw9);

				// change Sys_Error -> Com_Error + advanced messages
				utils::hook::call(0x279658B_b, compile_error_stub); // CompileError (LinkFile)
				utils::hook::call(0x27968F7_b, compile_error_stub); // ^
				utils::hook::call(0x2796AAA_b, compile_error_stub); // idk 3rd one
				utils::hook::call(0x2796E17_b, find_variable_stub_iw9); // Scr_EmitFunction_Precompiled
				return;
			}

			if (!identification::game::is("1.20.4-replay"))
			{
#ifdef _DEBUG
				printf("script errors are not included in this verison of the game\n");
#endif
				return;
			}

			// TODO: this works great in IW8, but we need to make it multi-game now
			scr_emit_function_hook.create(0x1316800_b, scr_emit_function_stub);

			// change Sys_Error -> Com_Error + advanced messages
			utils::hook::call(0x13166DE_b, compile_error_stub); // CompileError (LinkFile)
			utils::hook::call(0x1316777_b, compile_error_stub); // ^
			utils::hook::call(0x13168DD_b, find_variable_stub); // Scr_EmitFunction_Precompiled

			/*
			// Restore basic error messages for commonly used scr functions
#define MEME_DETOUR(address, func) //func##_hook.create(address, func);

			MEME_DETOUR(0x1325000_b, scr_get_object);
			MEME_DETOUR(0x13247F0_b, scr_get_const_string);
			//MEME_DETOUR(0x1324E10_b, scr_get_const_istring);
			MEME_DETOUR(0x125CDA0_b, scr_validate_localized_string_ref);
			MEME_DETOUR(0x1325740_b, scr_get_vector);
			//MEME_DETOUR(0x1324F00_b, scr_get_int); // works, but has meme cases?
			MEME_DETOUR(0x1324C00_b, scr_get_float);

			MEME_DETOUR(0x1325220_b, scr_get_pointer_type);
			MEME_DETOUR(0x1325580_b, scr_get_type);
			MEME_DETOUR(0x1325610_b, scr_get_type_name);
			*/
		}
	};
}

//REGISTER_COMPONENT(gsc::error)
