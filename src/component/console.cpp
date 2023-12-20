#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/thread.hpp>

namespace console
{
	namespace
	{

		static volatile bool exit = false;

		DWORD WINAPI console(LPVOID)
		{
			AllocConsole();
			AttachConsole(GetCurrentProcessId());
			SetConsoleTitleA("iw8-mod");

			freopen("CONIN$", "r", stdin);
			freopen("CONOUT$", "w", stdout);

			std::string cmd;

			while (!exit)
			{
				std::getline(std::cin, cmd);
			}

			return 0;
		}

		const game::dvar_t* output_console = nullptr;
		utils::hook::detour print_message_hook;

		void print_message_stub(int channel, const char* text, int unk)
		{
			if (output_console != nullptr && output_console->current.enabled)
			{
				printf("[%d] %s", channel, text);
			}

			print_message_hook.invoke<void>(channel, text, unk);
		}

		utils::hook::detour cl_keys_event_hook;

		void cl_keys_event_stub(const int local_client_num, const int key, const bool down, unsigned int time, int v_key, int index)
		{
			if (down)
			{
				if (key == game::K_GRAVE)
				{
					if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
					{
						if (!game::Con_IsActive(local_client_num))
						{
							game::Con_ToggleConsole();
						}
						game::Con_ToggleConsoleOutput();
					}
					else
					{
						game::Con_ToggleConsole();
					}
				}
				else if (key == game::keyNum_t::K_F1)
				{
					game::DevGui_Toggle();
				}
			}

			cl_keys_event_hook.invoke<void>(local_client_num, key, down, time, v_key, index);
		}

		const char* console_text_stub(const char*, const char*, const char*)
		{
			return "^1iw8-mod: > ^7";
		}

		void draw_overlay_stub()
		{
			game::Con_DrawConsole(0);
			game::DevGui_Draw(0);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_start() override
		{
			const auto handle = CreateThread(0, 0, console, 0, 0, 0);
			utils::thread::set_name(handle, "console");
		}

		void post_unpack() override
		{
			utils::hook::jump(0x15E1340_b, draw_overlay_stub);

			// toggle ingame console
			cl_keys_event_hook.create(0x15BEB80_b, cl_keys_event_stub);

			// toggle ingame console output to console
			output_console = game::Dvar_RegisterBool("output_console", false, game::DVAR_FLAG_SAVED, "Output messages to external console.");
			print_message_hook.create(0x12B0660_b, print_message_stub);

			// change console text
			utils::hook::call(0x15AE45B_b, console_text_stub);
		}

		void pre_destroy() override
		{
			exit = true;
		}
	};
}

REGISTER_COMPONENT(console::component)
