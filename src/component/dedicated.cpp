#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "command.hpp"
#include "scheduler.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/flags.hpp>
#include <utils/string.hpp>

//#define REMOVE_RENDERER

namespace dedicated
{
	namespace
	{
		utils::hook::detour com_quit_f_hook;

		void kill_server()
		{
			utils::hook::invoke<void>(0x137D6F0_b); //game::SV_MainMP_KillLocalServer();

			com_quit_f_hook.invoke<void>();
		}

		bool party_is_server_dedicated_stub()
		{
			return true;
		}

		utils::hook::detour game_state_info_hook;
		void* game_state_info_stub()
		{
			static auto s_gameStateInfo = game::s_gameStateInfo;
			s_gameStateInfo->usingRecipe = false;
			return s_gameStateInfo;
		}

		utils::hook::detour sync_gpu_hook;
		void sync_gpu_stub()
		{
			// R_SyncGpu_Full (R_SyncGpu renamed)
			sync_gpu_hook.invoke<void>();

			std::this_thread::sleep_for(1ms);
		}

		void init_dedicated_server()
		{
			printf("init_dedicated_server\n");

			// R_RegisterDvars
			utils::hook::invoke<void>(0x192A130_b);

			// R_RegisterCmds
			utils::hook::invoke<void>(0x18E3D70_b);

			// RB_Tonemap_RegisterDvars
			//utils::hook::invoke<void>(0x1876C20_b);

			static bool initialized = false;
			if (initialized) return;
			initialized = true;

			// R_LoadGraphicsAssets
			utils::hook::invoke<void>(0x1941A50_b);
		}

		std::vector<std::string>& get_startup_command_queue()
		{
			static std::vector<std::string> startup_command_queue;
			return startup_command_queue;
		}

		void execute_startup_command(int client, int /*controllerIndex*/, const char* command)
		{
			if (game::Live_SyncOnlineDataFlags(0) == 0)
			{
				game::Cbuf_ExecuteBufferInternal(0, 0, command, 0);
			}
			else
			{
				get_startup_command_queue().emplace_back(command);
			}
		}

		void execute_startup_command_queue()
		{
			const auto queue = get_startup_command_queue();
			get_startup_command_queue().clear();

			for (const auto& command : queue)
			{
				game::Cbuf_ExecuteBufferInternal(0, 0, command.data(), 0);
			}
		}

		bool ret_true()
		{
			return true;
		}

		void initialize()
		{
			const auto initialize_gamemode = []() -> void
			{
				if (game::Com_GameMode_GetActiveGameMode() == game::GAME_MODE_CP)
				{
					game::Cbuf_ExecuteBufferInternal(0, 0, "exec default_systemlink_cp.cfg", 1);
					game::Cbuf_ExecuteBufferInternal(0, 0, "exec default_cp.cfg", 1);
				}
				else if (game::Com_GameMode_GetActiveGameMode() == game::GAME_MODE_MP)
				{
					game::Cbuf_ExecuteBufferInternal(0, 0, "exec default_systemlink_mp.cfg", 1);
					game::Cbuf_ExecuteBufferInternal(0, 0, "exec default_mp.cfg", 1);
				}
			};

			initialize_gamemode();
		}
	}

	class component final : public component_interface
	{
	public:
		void post_start() override
		{
			if (!game::environment::is_dedi())
			{
				return;
			}

			// r_loadForRenderer reimplemented into IW8 partially
			//utils::hook::set<uint8_t>(0x193BCC0_b, 0xC3); // Image_Setup
		}

		void post_unpack() override
		{
			if (!game::environment::is_dedi())
			{
				return;
			}

#ifdef DEBUG
			printf("Starting dedicated server\n");
#endif

			// Register dedicated dvar
			game::Dvar_RegisterBool("dedicated", true, game::DVAR_FLAG_READ, "Dedicated server");

			// Add lanonly mode
			game::Dvar_RegisterBool("sv_lanOnly", false, game::DVAR_FLAG_NONE, "Don't send heartbeat");

			// Disable frontend
			utils::hook::set<uint8_t>(0x10B29C0_b, 0xC3); // Com_FastFile_Frame_FrontEnd

#ifdef REMOVE_RENDERER
			// Disable load for renderer
			game::Dvar_RegisterBool("r_loadForRenderer", false, game::DVAR_FLAG_READ, "Disable dx allocations (not on IW8?)"); // NOT FOUND!!! :(
#endif

			// Is party dedicated
			utils::hook::jump(0x118EC70_b, party_is_server_dedicated_stub);

			// Make GScr_IsUsingMatchRulesData return 0 so the game doesn't override the cfg
			//utils::hook::jump(0xB53950_b, gscr_is_using_match_rules_data_stub);
			game_state_info_hook.create(0x15990E0_b, game_state_info_stub);

#ifdef REMOVE_RENDERER
			// Hook R_SyncGpu
			sync_gpu_hook.create(0x1946240_b, sync_gpu_stub);

			utils::hook::nop(0x193E33B_b, 5); // R_CreateWindow
#endif

			utils::hook::jump(0x15C35C0_b, init_dedicated_server, true);

			// TODO: delay startup commands until the initialization is done
			//utils::hook::call(0x12AA329_b, execute_startup_command);

			/*
			utils::hook::nop(0x13DAC88_b, 5);			// don't load config file
			//utils::hook::nop(0xB7CE46_b, 5);			// ^
			utils::hook::call(0x1297644_b, ret_true);	// ^ (new version of above)
			utils::hook::set<uint8_t>(0x12B58D0_b, 0xC3); // don't save config file
			*/

			utils::hook::set<uint8_t>(0x11977A0_b, 0xC3);	// disable self-registration (PartyHost_AddLocalPlayer)
			//utils::hook::set<uint8_t>(0x199B830_b, 0xC3);	// render thread
			//utils::hook::set<uint8_t>(0x15CAA50_b, 0xC3);	// called from Com_Frame, seems to do renderer stuff
			//utils::hook::set<uint8_t>(0x15E4FC0_b, 0xC3);	// CL_CheckForResend, which tries to connect to the local server constantly
			//utils::hook::set<uint8_t>(0x13FCA00_b, 0xC3);	// recommended settings check

			scheduler::schedule([=]()
			{
				const auto data_flags = game::Live_SyncOnlineDataFlags(0);
				//printf("data_flags: %d\n", data_flags);
				const auto initial_lobby_ready = data_flags == 994934;
				if (initial_lobby_ready && game::Sys_IsDatabaseReady())
				{
					static auto lobby_msg_showed = false;
					if (!lobby_msg_showed)
					{
						printf("game initialized completed\n");
						lobby_msg_showed = true;
					}

					return scheduler::cond_end;
				}

				return scheduler::cond_continue;
			}, scheduler::pipeline::main);

			/*
			utils::hook::nop(0x136FFC6_b, 2);	// unknown check in SV_ExecuteClientMessage
			//utils::hook::nop(0xC4F407_b, 3);	// allow first slot to be occupied (can't find???)
			utils::hook::nop(0x15C52D3_b, 2);	// properly shut down dedicated servers
			utils::hook::nop(0x15C5440_b, 2);	// ^
			utils::hook::set<uint8_t>(0x1943A80_b, 0xC3); // don't shutdown renderer

			// SOUND patches
			//utils::hook::nop(0xC93213_b, 5); // snd stream thread
			//utils::hook::set<uint8_t>(0xC93206_b, 0); // snd_active
			//utils::hook::set<uint8_t>(0xCB9150_b, 0xC3); // sound queue thing
			//utils::hook::set<uint8_t>(0xC75550_b, 0xC3); // SD_AllocInit
			//utils::hook::set<uint8_t>(0xC75CA0_b, 0xC3); // SD_Init

			utils::hook::set<uint8_t>(0x1BD01E0_b, 0xC3); // Voice_Init
			//utils::hook::set(0xCFDC40_b, 0xC3C033); // sound stream reading (can't find?)

			// lookup the length from our list
			//snd_lookup_sound_length_hook.create(0xC9BCE0_b, snd_lookup_sound_length_stub);

			// check the sounddata when server is launched
			//start_server_hook.create(0xC56050_b, start_server_stub);

			// IMAGE patches
			// image stream (pak)
			//utils::hook::set<uint8_t>(0xA7DB10_b, 0xC3); // DB_CreateGfxImageStreamInternal

			// UI patches
			utils::hook::set<uint8_t>(0x19D1E70_b, 0xC3); // LUI_CoD_Init

			// IW8 patches (to IW7)
			//utils::hook::set(0xE06060_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE06060_b, 0xC3); // directx
			//utils::hook::set(0xE05B80_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE05B80_b, 0xC3); // ^
			//utils::hook::set(0xDD2760_b, 0xC3C033); //utils::hook::set<uint8_t>(0xDD2760_b, 0xC3); // ^
			//utils::hook::set(0xE05E20_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE05E20_b, 0xC3); // ^ buffer
			//utils::hook::set(0x194BB80_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE11270_b, 0xC3); // ^
			//utils::hook::set(0xDD3C50_b, 0xC3C033); //utils::hook::set<uint8_t>(0xDD3C50_b, 0xC3); // ^
			//utils::hook::set(0x0C1210_b, 0xC3C033); //utils::hook::set<uint8_t>(0x0C1210_b, 0xC3); // ^ idk
			//utils::hook::set(0x0C12B0_b, 0xC3C033); //utils::hook::set<uint8_t>(0x0C12B0_b, 0xC3); // ^ idk
			//utils::hook::set(0xE423A0_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE423A0_b, 0xC3); // directx
			//utils::hook::set(0xE04680_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE04680_b, 0xC3); // ^

			// 0x1881750

			/*
			utils::hook::set(0xE00ED0_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE00ED0_b, 0xC3); // Image_Create1DTexture_PC
			utils::hook::set(0xE00FC0_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE00FC0_b, 0xC3); // Image_Create2DTexture_PC
			utils::hook::set(0xE011A0_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE011A0_b, 0xC3); // Image_Create3DTexture_PC
			utils::hook::set(0xE015C0_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE015C0_b, 0xC3); // Image_CreateCubeTexture_PC
			utils::hook::set(0xE01300_b, 0xC3C033); //utils::hook::set<uint8_t>(0xE01300_b, 0xC3); // Image_CreateArrayTexture_PC

			//utils::hook::set(0x5F1EA0_b, 0xC3C033); //utils::hook::set<uint8_t>(0x5F1EA0_b, 0xC3); // renderer
			//utils::hook::set(0x0C1370_b, 0xC3C033); //utils::hook::set<uint8_t>(0x0C1370_b, 0xC3); // ^
			utils::hook::set(0xDD26E0_b, 0xC3C033); //utils::hook::set<uint8_t>(0xDD26E0_b, 0xC3); // directx
			utils::hook::set(0x5F0610_b, 0xC3C033); //utils::hook::set<uint8_t>(0x5F0610_b, 0xC3); // ^
			utils::hook::set(0x5F0580_b, 0xC3C033); //utils::hook::set<uint8_t>(0x5F0580_b, 0xC3); // ^
			utils::hook::set(0x5F0820_b, 0xC3C033); //utils::hook::set<uint8_t>(0x5F0820_b, 0xC3); // ^
			utils::hook::set(0x5F0790_b, 0xC3C033); //utils::hook::set<uint8_t>(0x5F0790_b, 0xC3); // ^

			utils::hook::set(0xDD42A0_b, 0xC3C033); // shutdown
			utils::hook::set(0xDD42E0_b, 0xC3C033); // ^
			utils::hook::set(0xDD42E0_b, 0xC3C033); // ^
			utils::hook::set(0xDD4280_b, 0xC3C033); // ^

			utils::hook::set(0xDD4230_b, 0xC3C033); // ^

			// skip R_GetFrameIndex check in DB_LoadLevelXAssets
			utils::hook::set<uint8_t>(0xF63934_b, 0xEB);

			// don't release buffer
			utils::hook::set<uint8_t>(0x18DCB80_b, 0xC3);

			// R_LoadWorld
			utils::hook::set<uint8_t>(0x18D8FB0_b, 0xC3);

			// something to do with vls?
			//utils::hook::set<uint8_t>(0xD02CB0_b, 0xC3);

			// recipe save threads
			utils::hook::set<uint8_t>(0xCC9550_b, 0xC3);

			// set game mode
			scheduler::once([]()
			{
				if (utils::flags::has_flag("sp"))
				{
					game::Com_GameMode_SetDesiredGameMode(game::GAME_MODE_SP, 0);
				}
				else if (utils::flags::has_flag("cp"))
				{
					game::Com_GameMode_SetDesiredGameMode(game::GAME_MODE_CP, 0);
				}
				else
				{
					game::Com_GameMode_SetDesiredGameMode(game::GAME_MODE_MP, 0);
				}
			}, scheduler::pipeline::main);

			// initialization
			scheduler::on_game_initialized([]()
			{
				initialize();

				printf("==================================\n");
				printf("Server started!\n");
				printf("==================================\n");

				// remove disconnect command
				utils::hook::invoke<void>(0x1298270_b, "disconnect"); // Cmd_RemoveCommand

				execute_startup_command_queue();
			}, scheduler::pipeline::main, 1s);

			// dedicated info
			scheduler::loop([]()
			{
				auto* sv_running = game::Dvar_FindVarByName("NLTTMLSQRQ"); // sv_running
				if (!sv_running || !sv_running->current.enabled)
				{
					SetConsoleTitleA("IW8-Mod Dedicated Server");
					return;
				}

				auto* const sv_hostname = game::Dvar_FindVarByName("name");
				auto* const mapname = game::Dvar_FindVarByName("mapname");

				if (!sv_hostname || !mapname)
				{
					return;
				}

				std::string cleaned_hostname;
				cleaned_hostname.resize(static_cast<int>(strlen(sv_hostname->current.string) + 1));

				utils::string::strip(sv_hostname->current.string, cleaned_hostname.data(),
					static_cast<int>(strlen(sv_hostname->current.string)) + 1);

				SetConsoleTitleA(utils::string::va("%s on %s", cleaned_hostname.data(), mapname->current.string));
			}, scheduler::pipeline::main, 1s);

			scheduler::once([]()
			{
				command::add("killserver", kill_server);
				com_quit_f_hook.create(0x12B09D0_b, kill_server);
			}, scheduler::server);
			*/
		}
	};
}

//REGISTER_COMPONENT(dedicated::component)
