#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "command.hpp"

#include "game/game.hpp"
//#include "game/dvars.hpp"

//#include "console.hpp"
//#include "game_console.hpp"
#include "scheduler.hpp"
#include "dvars.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/memory.hpp>
#include <utils/io.hpp>

namespace command
{
	namespace
	{
		//utils::hook::detour client_command_mp_hook;
		//utils::hook::detour client_command_sp_hook;
		utils::hook::detour parse_commandline_hook;

		std::unordered_map<std::string, std::function<void()>> handlers;
		//std::unordered_map<std::string, std::function<void(int)>> handlers_sv;

		void main_handler()
		{
			const auto command = utils::string::to_lower(game::Cmd_Argv(0));
			if (handlers.find(command) != handlers.end())
			{
				handlers[command]();
			}
		}

		/*
		void client_command_mp(const int client_num)
		{
			const auto command = utils::string::to_lower(game::SV_Cmd_Argv(0));
			if (handlers_sv.find(command) != handlers_sv.end())
			{
				handlers_sv[command](client_num);
			}

			client_command_mp_hook.invoke<void>(client_num);
		}

		void client_command_sp(const int client_num, const char* s)
		{
			game::SV_Cmd_TokenizeString(s);
			const auto command = utils::string::to_lower(s);
			if (handlers_sv.find(command) != handlers_sv.end())
			{
				handlers_sv[command](client_num);
			}
			game::SV_Cmd_EndTokenizedString();

			client_command_sp_hook.invoke<void>(client_num, s);
		}

		game::dvar_t* dvar_command_stub()
		{
			const params args;

			if (args.size() <= 0)
			{
				return 0;
			}

			auto* dvar = game::Dvar_FindVarByName(args[0]);
			if (dvar == nullptr)
			{
				dvar = game::Dvar_FindMalleableVar(atoi(args[0]));
			}

			if (dvar)
			{
				if (args.size() == 1)
				{
					const auto current = game::Dvar_ValueToString(dvar, dvar->current);
					const auto reset = game::Dvar_ValueToString(dvar, dvar->reset);

					console::info("\"%s\" is: \"%s\" default: \"%s\" checksum: %d type: %i\n",
						dvars::dvar_get_name(dvar).data(), current, reset, dvar->checksum, dvar->type);

					const auto dvar_info = dvars::dvar_get_description(dvar);

					if (!dvar_info.empty())
						console::info("%s\n", dvar_info.data());

					console::info("   %s\n", dvars::dvar_get_domain(dvar->type, dvar->domain).data());
				}
				else
				{
					char command[0x1000] = { 0 };
					game::Dvar_GetCombinedString(command, 1);
					game::Dvar_SetCommand(args[0], command);
				}

				return dvar;
			}

			return 0;
		}
		*/
	}

	void add_raw(const char* name, void (*callback)())
	{
		game::Cmd_AddCommandInternal(name, callback, utils::memory::get_allocator()->allocate<game::cmd_function_s>());
	}

	void add(const char* name, const std::function<void()>& callback)
	{
		const auto command = utils::string::to_lower(name);

		if (handlers.find(command) == handlers.end())
			add_raw(name, main_handler);

		handlers[command] = callback;
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			//utils::hook::jump(0xBB1DC0_b, dvar_command_stub, true);
			//client_command_mp_hook.create(0x120B6A0_b, &client_command_mp);
			//client_command_sp_hook.create(0x483130_b, &client_command_sp);

			add("map_restart", []()
			{
				auto SV_CmdsMP_RequestMapRestart = reinterpret_cast<void(*)(bool load_scripts, bool migrate)>(0x136C310_b);
				SV_CmdsMP_RequestMapRestart(1, 0);
			});

			add("test", []()
			{
				printf("test\n");
			});
		}
	};
}

REGISTER_COMPONENT(command::component)
