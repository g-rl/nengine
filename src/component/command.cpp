#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "command.hpp"

#include "game/game.hpp"
#include <identification/game.hpp>
//#include "game/dvars.hpp"

//#include "console.hpp"
//#include "game_console.hpp"
#include "scheduler.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/memory.hpp>
#include <utils/io.hpp>

namespace command
{
	namespace
	{
		std::unordered_map<std::string, std::function<void()>> handlers;

		void main_handler()
		{
#ifdef _DEBUG
			printf("main_handler\n");
#endif
			const auto command = utils::string::to_lower(game::Cmd_Argv(0));
			if (handlers.find(command) != handlers.end())
			{
#ifdef _DEBUG
				printf("handling %s\n", command.c_str());
#endif
				handlers[command]();
			}
		}
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
		void find_signatures(memory::signature_store& batch) override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;

			if (game_ == "iw9-mod"s || game_ == "s4-mod"s || identification::game::is("1.20.4-replay"))
				batch.add(SETUP_POINTER(game::SV_CmdsMP_RequestMapRestart),
					"40 55 53 57 48 8D 6C 24 F0 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 00");
			else
				batch.add(SETUP_POINTER(game::SV_CmdsMP_RequestMapRestart),
					"48 83 EC 28 E8 ? ? ? ? 84 ? ? ? E8 ? ? ? ? 84 ? ? ? 48 8B 05 ? ? ? ? 80");
		
			// iw8 & iw9
			batch.add(SETUP_POINTER(game::Cmd_AddCommandInternal),
				"4C 8D 05 ? ? ? ? 48 8D 15 ? ? 00 00 48 8D 0D ? ? ? ? E8 ? ? ? 00",
				SETUP_MOD(add(22).rip()));

			if (game_ == "iw8-mod"s)
			{
				batch.add(SETUP_POINTER(game::Cmd_Argc_internal),
					"48 83 EC 28 E8 ? ? ? ? 83 F8 03 7C 33",
					SETUP_MOD(add(5).rip()));

				batch.add(SETUP_POINTER(game::Cmd_Argv_internal),
					"48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 33 C9 E8 ? ? FF FF",
					SETUP_MOD(add(27).rip()));
			}
			else if (game_ == "iw9-mod"s)
			{
				batch.add(SETUP_POINTER(game::cmd_args),
					"48 63 ? ? ? ? ? 48 8d ? ? ? ? ? 83 ? ? ? ? 7C 44",
					SETUP_MOD(add(3).rip()));
			}
		}

		void post_start() override
		{
			SetConsoleTitleA("neura engine");

			static const auto& game_ = identification::game::get_target_game().client_name;
			//if (game_ == "iw8-mod"s)
			//	return;

			//FreeConsole();
			AllocConsole();
			SetConsoleTitleA("neura engine");

			fflush(stdout);
			fflush(stderr);

			FILE* f;
			freopen_s(&f, "CONOUT$", "w", stdout);
			freopen_s(&f, "CONOUT$", "w", stderr);
			freopen_s(&f, "CONIN$", "r", stdin);

			std::ios::sync_with_stdio(true);
			setvbuf(stdout, nullptr, _IONBF, 0);

			const auto out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD out_mode{};
			GetConsoleMode(out_handle, &out_mode);
			SetConsoleMode(out_handle, out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

			const auto in_handle = GetStdHandle(STD_INPUT_HANDLE);
			DWORD in_mode{};
			GetConsoleMode(in_handle, &in_mode);
			in_mode &= ~(ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT);
			in_mode |= ENABLE_WINDOW_INPUT;
			SetConsoleMode(in_handle, in_mode);

			printf("neura patch started!\n");
		}

		void post_unpack() override
		{
			add("fast_restart", []()
			{
				game::SV_CmdsMP_RequestMapRestart(0, 0);
			});

			add("map_restart", []()
			{
#ifdef _DEBUG
				printf("attempting restart\n");
#endif
				game::SV_CmdsMP_RequestMapRestart(1, 0);
			});
		}
	};
}

REGISTER_COMPONENT(command::component)
