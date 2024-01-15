#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include "component/scripting.hpp"

#include "script_error.hpp"
#include "script_extension.hpp"
#include "script_loading.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

namespace gsc
{
	std::uint16_t function_id_start = 1048;
	std::uint16_t method_id_start = 1927 + 0x8000;

	constexpr size_t func_table_count = 0x1000;
	constexpr size_t meth_table_count = 0x1000;

	builtin_function func_table[func_table_count]{};
	builtin_method meth_table[meth_table_count]{};

	const game::dvar_t* developer_script = nullptr;

	namespace
	{
		std::unordered_map<std::uint16_t, builtin_function> functions;
		std::unordered_map<std::uint16_t, builtin_method> methods;

		bool force_error_print = false;
		std::optional<std::string> gsc_error_msg;

		std::unordered_map<const char*, const char*> vm_execute_hooks;
		const char* target_function = nullptr;

		std::uint32_t get_function_id()
		{
			return game::ScriptContext_Server()->top->u.uintValue;
		}

		void execute_custom_function(const std::uint16_t id)
		{
			try
			{
				const auto& function = functions[id];
				function(game::ScriptContext_Server());
			}
			catch (const std::exception& ex)
			{
				scr_error(ex.what());
			}
		}

		void vm_call_builtin_function_internal(int function_id)
		{
			const auto custom_function_id = static_cast<std::uint16_t>(function_id); // cast for gsc-tool & our custom func map
			const auto custom = functions.contains(custom_function_id);
			if (custom)
			{
				execute_custom_function(custom_function_id);
				return;
			}

			const auto context = game::ScriptContext_Server();

			builtin_function func = func_table[function_id - context->m_funcBegin]; // game does this for the stock func table
			if (func == nullptr)
			{
				scr_error(utils::string::va("builtin function \"%s\" doesn't exist", gsc_ctx->func_name(custom_function_id).data()), true);
				return;
			}

			func(context);
		}

		void vm_call_builtin_function_stub(utils::hook::assembler& a)
		{
			a.pushad64();
			a.push(ecx);
			a.mov(ecx, r14d); // function id is in r14d
			a.call_aligned(vm_call_builtin_function_internal);
			a.pop(ecx);
			a.popad64();

			a.jmp(0x1329329_b);
		}

		void builtin_call_error(const std::string& error)
		{
			const auto custom_function_id = static_cast<std::uint16_t>(get_function_id()); // cast for gsc-tool & our custom func map

			if (custom_function_id > func_table_count)
			{
				printf("in call to builtin method \"%s\"%s", gsc_ctx->meth_name(custom_function_id).data(), error.data());
			}
			else
			{
				printf("in call to builtin function \"%s\"%s", gsc_ctx->func_name(custom_function_id).data(), error.data());
			}
		}

		std::optional<std::string> get_opcode_name(const std::uint8_t opcode)
		{
			try
			{
				const auto index = gsc_ctx->opcode_enum(opcode);
				return { gsc_ctx->opcode_name(index) };
			}
			catch (...)
			{
				return {};
			}
		}

		void print_callstack()
		{
			const auto context = game::ScriptContext_Server();
			for (auto frame = context->function_frame; frame != context->function_frame_start; --frame)
			{
				const auto pos = frame == context->function_frame ? context->pos.___u0.m_scriptPos : frame->fs.pos.___u0.m_scriptPos;
				const auto function = find_function(frame->fs.pos.___u0.m_scriptPos);

				if (function.has_value())
				{
					printf("\tat function \"%s\" in file \"%s.gsc\"\n", function.value().first.data(), function.value().second.data());
				}
				else
				{
					printf("\tat unknown location %p\n", pos);
				}
			}
		}

		void vm_error_internal()
		{
			const bool dev_script = developer_script ? developer_script->current.enabled : false;

			if (!dev_script && !force_error_print)
			{
				return;
			}

			printf("*********** script runtime error *************\n");

			const auto opcode_id = *reinterpret_cast<std::uint8_t*>(0xE17F670_b);
			const std::string error_str = gsc_error_msg.has_value()
				? utils::string::va(": %s", gsc_error_msg.value().data())
				: "";

			if ((opcode_id >= gsc_ctx->opcode_id(xsk::gsc::opcode::OP_CallBuiltin0) && opcode_id <= gsc_ctx->opcode_id(xsk::gsc::opcode::OP_CallBuiltin))
				|| (opcode_id >= gsc_ctx->opcode_id(xsk::gsc::opcode::OP_CallBuiltinMethod0) && opcode_id <= gsc_ctx->opcode_id(xsk::gsc::opcode::OP_CallBuiltinMethod)))
			{
				builtin_call_error(error_str);
			}
			else
			{
				const auto opcode = get_opcode_name(opcode_id);
				if (opcode.has_value())
				{
					printf("while processing instruction %s%s\n", opcode.value().data(), error_str.data());
				}
				else
				{
					printf("while processing instruction 0x%X%s\n", opcode_id, error_str.data());
				}
			}

			force_error_print = false;
			gsc_error_msg = {};

			print_callstack();
			printf("**********************************************\n");
		}

		void vm_error_stub(unsigned __int64 mark_pos)
		{
			vm_error_internal();

			utils::hook::invoke<void>(0x1036900_b, mark_pos);
		}

		bool get_replaced_pos(const char* pos)
		{
			if (vm_execute_hooks.contains(pos))
			{
				target_function = vm_execute_hooks[pos];
				return true;
			}
			return false;
		}

		void vm_execute_stub(utils::hook::assembler& a)
		{
			const auto replace = a.newLabel();
			const auto end = a.newLabel();

			a.pushad64();

			a.mov(rcx, rsi);
			a.call_aligned(get_replaced_pos);

			a.cmp(al, 0);
			a.jne(replace);

			a.popad64();
			a.jmp(end);

			a.bind(end);

			a.prefetcht0(byte_ptr(rsi, 0x80));
			a.movzx(r14d, byte_ptr(rsi));
			a.mov(rcx, r12);
			//a.inc(rsi);
			//a.mov(dword_ptr(rbp, 0x94), r14d);

			a.jmp(0x132742E_b);

			a.bind(replace);

			a.popad64();
			a.mov(rax, qword_ptr(reinterpret_cast<int64_t>(&target_function)));
			a.mov(rsi, rax);
			a.jmp(end);
		}

		scripting::script_value get_argument(int index)
		{
			const auto context = game::ScriptContext_Server();
			if (static_cast<std::uint32_t>(index) >= context->outparamcount)
			{
				throw std::runtime_error(std::format("parameter {} does not exist", index + 1));
			}

			return {context->top[-index]};
		}
	}

	void scr_error(const char* error, const bool force_print)
	{
		force_error_print = force_print;
		gsc_error_msg = error;

		printf("scr_error: %s\n", error);
		//game::Scr_ErrorInternal(game::ScriptContext_Server());
	}

	namespace function
	{
		void add(const std::string& name, builtin_function function)
		{
			if (gsc_ctx->func_exists(name))
			{
				const auto id = gsc_ctx->func_id(name);
				functions[id] = function;
			}
			else
			{
				const auto id = ++function_id_start;
				gsc_ctx->func_add(name, id);
				functions[id] = function;
			}
		}
	}

	class extension final : public component_interface
	{
	public:
		void post_unpack() override
		{
			developer_script = game::Dvar_RegisterBool("developer_script", true, game::DVAR_FLAG_NONE, "Enable developer script comments"); // enable by default for now

			gsc::on_begin_scripts([&]()
			{
				const auto context = game::ScriptContext_Server();
				context->m_pFuncTable = func_table;
				context->m_pMethTable = meth_table;
				/*
				funcCount = 1048;
				funcEnd = 1049; (1048 + 1)
				methCount = 1927;
				methEnd = 34695 (1927 + 0x8000)
				*/
				context->m_funcCount = func_table_count;						// 0x1000
				context->m_funcEnd = func_table_count + context->m_funcBegin;	// (0x1000 + 1)
				context->m_methCount = meth_table_count;						// 0x1000
				context->m_methEnd = meth_table_count + context->m_methBegin;	// (0x1000 + 0x8000)

				// done in Scr_BeginLoadScripts when original runs
				//const auto size = static_cast<size_t>(8);
				//memset(context->m_pFuncTable, 0, size * context->m_funcCount);
				//memset(context->m_pMethTable, 0, size * context->m_methCount);

				/*
				printf("=====================================\n");
				printf("begin_load_scripts_stub\n");
				printf("m_funcBegin = %d\n", context->m_funcBegin);
				printf("m_funcCount = %d\n", context->m_funcCount);
				printf("m_funcEnd = %d\n", context->m_funcEnd);
				printf("m_methBegin = %d\n", context->m_methBegin);
				printf("m_methCount = %d\n", context->m_methCount);
				printf("m_methEnd = %d\n", context->m_methEnd);
				printf("=====================================\n");
				*/
			});

			utils::hook::nop(0x1328EF0_b, 23);
			utils::hook::jump(0x1328EF0_b, utils::hook::assemble(vm_call_builtin_function_stub), true);

			/*
			utils::hook::set<uint32_t>(0xBFD182_b + 4, static_cast<uint32_t>(reverse_b((&meth_table))));
			utils::hook::inject(0xBFD5AF_b + 3, &meth_table);
			utils::hook::set<uint32_t>(0xBFD5B6_b + 2, sizeof(meth_table));
			utils::hook::nop(0xC0E8EB_b, 14); // nop the lea & call at the end of call_builtin_method
			utils::hook::jump(0xC0E8EB_b, utils::hook::assemble(vm_call_builtin_method_stub), true);
			*/

			utils::hook::call(0x132ACB9_b, vm_error_stub); // LargeLocalResetToMark

			utils::hook::jump(0x1327420_b, utils::hook::assemble(vm_execute_stub), true);

			function::add("print", [](game::scrContext_t* context) -> void
			{
				printf("%s\n", game::Scr_GetString(context, 0));
			});

			function::add("replacefunc", [](game::scrContext_t* context) -> void
			{
				const auto what = get_argument(0).get_raw();
				const auto with = get_argument(1).get_raw();

				if (what.type != game::VAR_FUNCTION || with.type != game::VAR_FUNCTION)
				{
					throw std::runtime_error("replacefunc: parameter 1 must be a function");
				}

				vm_execute_hooks[what.u.codePosValue] = with.u.codePosValue;
			});
		}
	};
}

REGISTER_COMPONENT(gsc::extension)
