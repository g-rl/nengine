#include <std_include.hpp>

#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <utils/flags.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>
#include <utils/nt.hpp>
#include <utils/hook.hpp>

namespace
{
	DECLSPEC_NORETURN void WINAPI exit_hook(const int code)
	{
		component_loader::pre_destroy();
		std::exit(code);
	}

	BOOL WINAPI system_parameters_info_a(const UINT uiAction, const UINT uiParam, const PVOID pvParam, const UINT fWinIni)
	{
		try
		{
			component_loader::post_unpack();
		}
		catch (const std::exception& e)
		{
			MSG_BOX_ERROR(e.what());
			std::exit(1);
		}

		return SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
	}

	void remove_crash_file()
	{
		utils::io::remove_file("__game_dx12_ship_replay");
		utils::io::remove_file("Data/data/CASCRepair.mrk"); // E_REPAIR (28)
	}

	void enable_dpi_awareness()
	{
		const utils::nt::library user32{"user32.dll"};

		{
			const auto set_dpi = user32
				? user32.get_proc<BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT)>(
					"SetProcessDpiAwarenessContext")
				: nullptr;
			if (set_dpi)
			{
				set_dpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
				return;
			}
		}

		{
			const utils::nt::library shcore{"shcore.dll"};
			const auto set_dpi = shcore
				? shcore.get_proc<HRESULT(WINAPI*)(PROCESS_DPI_AWARENESS)>(
					"SetProcessDpiAwareness")
				: nullptr;
			if (set_dpi)
			{
				set_dpi(PROCESS_PER_MONITOR_DPI_AWARE);
				return;
			}
		}

		{
			const auto set_dpi = user32
				? user32.get_proc<BOOL(WINAPI*)()>(
					"SetProcessDPIAware")
				: nullptr;
			if (set_dpi)
			{
				set_dpi();
			}
		}
	}

	void limit_parallel_dll_loading()
	{
		const utils::nt::library self;
		const auto registry_path = R"(Software\Microsoft\Windows NT\CurrentVersion\Image File Execution Options\)" + self.
			get_name();

		HKEY key = nullptr;
		if (RegCreateKeyA(HKEY_LOCAL_MACHINE, registry_path.data(), &key) == ERROR_SUCCESS)
		{
			RegCloseKey(key);
		}

		key = nullptr;
		if (RegOpenKeyExA(
			HKEY_LOCAL_MACHINE, registry_path.data(), 0,
			KEY_ALL_ACCESS, &key) != ERROR_SUCCESS)
		{
			return;
		}

		DWORD value = 1;
		RegSetValueExA(key, "MaxLoaderThreads", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));

		RegCloseKey(key);
	}

	void main()
	{
		enable_dpi_awareness();

		// This requires admin privilege, but I suppose many
		// people will start with admin rights if it crashes.
		limit_parallel_dll_loading();

		srand(uint32_t(time(nullptr)));
		remove_crash_file();

		{
			auto premature_shutdown = true;
			const auto _ = gsl::finally([&premature_shutdown]()
				{
					if (premature_shutdown)
					{
						component_loader::pre_destroy();
					}
				});

			try
			{
				if (!component_loader::post_start())
				{
					return;
				}

				auto* exit_process = utils::nt::library{}.get_iat_entry("kernel32.dll", "ExitProcess");
				if (!exit_process)
				{
					MSG_BOX_ERROR("could not find import ExitProcess");
				}
				utils::hook::set(exit_process, exit_hook);

				auto* system_parameters_info = utils::nt::library{}.get_iat_entry("user32.dll", "SystemParametersInfoA");
				if (!system_parameters_info)
				{
					MSG_BOX_ERROR("could not find import InitializeCriticalSectionEx");
				}
				utils::hook::set(system_parameters_info, system_parameters_info_a);

				if (!component_loader::post_load())
				{
					return;
				}

				premature_shutdown = false;
			}
			catch (std::exception& e)
			{
				MSG_BOX_ERROR(e.what());
			}
		}
	}
}

extern "C" __declspec(dllexport) int DiscordCreate()
{
	//CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)entry_point, 0, 0, 0);
	return 1;
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID lpVoid)
{
	game::load_base_address();

	if (reason == DLL_PROCESS_ATTACH)
	{
		main();
	}

	return TRUE;
}
