#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "scheduler.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/flags.hpp>
#include <utils/io.hpp>

namespace patches
{
	namespace
	{
		int tick = 0;

		const game::dvar_t* name_dvar = nullptr;
		const char* live_get_local_client_name_stub()
		{
			return name_dvar->current.string;
		}

		std::string get_login_username()
		{
			char username[UNLEN + 1];
			DWORD username_len = UNLEN + 1;
			if (!GetUserNameA(username, &username_len))
			{
				return "Unknown Soldier";
			}

			return std::string{username, username_len - 1};
		}

		utils::hook::detour com_register_dvars_hook;
		void com_register_dvars_stub()
		{
			// make name save + default to login username
			name_dvar = game::Dvar_RegisterString("name", get_login_username().data(), game::DVAR_FLAG_SAVED, "Player name.");

			com_register_dvars_hook.invoke<void>();
		}

		int signin_state_stub(uintptr_t luaVM)
		{
			game::lua_pushnumber(luaVM, 2); // BattleNetSignInState.signedIn
			return 1;
		}

		int is_paid_user_stub()
		{
			return 1;
		}

		bool live_is_offline_tool()
		{
			return true;
		}

		bool live_is_user_signed_into_dw_stub()
		{
			return true;
		}

		int dw_log_on_status_stub()
		{
			return 2;
		}

		int get_activate_stats_source_stub()
		{
			return 1;
		}

		int lui_is_demo_build_stub(uintptr_t luaVM)
		{
			game::lua_pushboolean(luaVM, 1);
			return 1;
		}
		
		utils::hook::detour dvar_register_hook;
		game::dvar_t* dvar_register_stub(const char* name, unsigned int checksum, unsigned __int8 type, 
			game::DvarFlags flags, game::DvarValue* value, void* domain, const char* desc)
		{
			if (!strcmp(name, "MPSSOTQQPM")		// force_offline_enabled
				|| !strcmp(name, "LSTQOKLTRN")	// force_offline_menus
				|| !strcmp(name, "LSSRRSMNMR")	// lui_dev_features_enabled
				|| !strcmp(name, "NRSSTQQSKK")	// r_preloadShaders
				|| !strcmp(name, "intro"))
			{
				if (!strcmp(name, "NRSSTQQSKK")) // r_preloadShaders
				{
					value->enabled = false;
				}
				else
				{
					value->enabled = true;
				}

				/*
				// dedicated server dvar patches
				if (game::environment::is_dedi())
				{
					if (!strcmp(name, "NRSSTQQSKK")		// r_preloadShaders
						|| !strcmp(name, "NPQTOLNKQK"))	// Enable/Disable sharing of shaders/PSOs during warmup on boot (uses more memory)
						//|| !strcmp(name, "NRSSTQQSKK")
					{
						value->integer = 0;
					}

					// skip splash screen dvar
					if (!strcmp(name, "NPLRKNKKOP"))
					{
						value->enabled = true;
					}

					// frontEndSceneEnabled & frontEndScenePreload
					if (!strcmp(name, "LOTLQRLOMK") || !strcmp(name, "MOKLQNMLMS"))
					{
						value->enabled = false;
					}
				}
				*/
			}

			return dvar_register_hook.invoke<game::dvar_t*>(name, checksum, type, flags, value, domain, desc);
		}

		void set_transient_mode_stub(int mode)
		{
			if (!strcmp(game::Dvar_GetStringSafe("NSQLTTMRMP"), "mp_donetsk"))
			{
				*reinterpret_cast<int*>(0x5CC7534_b) = 1;
			}
			else 
			{
				*reinterpret_cast<int*>(0x5CC7534_b) = mode;
			}
		}

		const char* invalid_map_references[23] = {
			"LUA_MENU/MAPNAME_ANIYAH",		"LUA_MENU/MAPNAME_DEADZONE",	"LUA_MENU/MAPNAME_M_CAGE",
			"LUA_MENU/MAPNAME_CAVE_AM",		"LUA_MENU/MAPNAME_CAVE",		//"LUA_MENU/MAPNAME_M_CARGO",
			"LUA_MENU/MAPNAME_CRASH2",		"LUA_MENU/MAPNAME_M_OVERUNDER", "LUA_MENU/MAPNAME_EUPHRATES",
			"LUA_MENU/MAPNAME_RAID",		"LUA_MENU/MAPNAME_M_SHOWERS",	"LUA_MENU/MAPNAME_RUNNER_AM",
			"LUA_MENU/MAPNAME_RUNNER",		"LUA_MENU/MAPNAME_HACKNEY_AM",	"LUA_MENU/MAPNAME_HACKNEY_YARD",
			"LUA_MENU/MAPNAME_M_HILL",		"LUA_MENU/MAPNAME_PICCADILLY",	"LUA_MENU/MAPNAME_M_PINE",
			"LUA_MENU/MAPNAME_SPEAR_AM",	"LUA_MENU/MAPNAME_SPEAR",		"LUA_MENU/MAPNAME_PETROGRAD",
			"LUA_MENU/MAPNAME_M_STACK",		"LUA_MENU/MAPNAME_PICCADILLY",	"LUA_MENU/MAPNAME_VACANT"
		};

		utils::hook::detour seh_string_ed_get_string_hook;
		const char* seh_string_ed_get_string_stub(const char* ref)
		{
			for (const char* invalid_ref : invalid_map_references)
			{
				if (!strcmp(ref, invalid_ref))
				{
					return "^1missing";
				}
			}

			if (!strcmp(ref, "LUA_MENU/CAMPAIGN_DESC") || !strcmp(ref, "LUA_MENU/LOCAL_COOP_DESC"))
			{
				return "^1The required content is not available in this build.";
			}
			else if (!strcmp(ref, "MENU_SP/CAMPAIGN"))
			{
				return "^1CAMPAIGN"; // use red to show invalid
			}
			else if (!strcmp(ref, "LUA_MENU/LOCAL_COOP_CAPS"))
			{
				return "^1LOCAL CO-OP";
			}

			return seh_string_ed_get_string_hook.invoke<const char*>(ref);
		}

		std::string current_event_name = "none";
		void set_table_string_stub(const char* name, const char* value, void* lua_vm)
		{
			current_event_name = value;
			utils::hook::invoke<void>(0x19B7840_b, name, value, lua_vm);
		}

		void report_error_with_info_stub(const char* error, const char* error_info, void* lua_vm)
		{
			error = utils::string::va("Error processing event '%s'\n", current_event_name.data());
			utils::hook::invoke<void>(0x19CDD30_b, error, error_info, lua_vm);
		}

		utils::hook::detour g_find_config_string_index_hook;
		unsigned int g_find_config_string_index_stub(const char* name, unsigned int start, unsigned int max, int create, const char* errormsg)
		{
			create = 1;
			return g_find_config_string_index_hook.invoke<unsigned int>(name, start, max, create, errormsg);
		}

		static_assert(sizeof(char) == 1);

		struct LuaFile
		{
			const char* name;	// 0
			int len;			// 8
			char strippingType;	// 12
			const char* buffer;	// 16
		}; static_assert(sizeof(LuaFile) == 24);

		void dump_lua_file(LuaFile** lua_file_)
		{
			auto lua_file = *lua_file_;

			std::string buffer;
			if (lua_file->len > 0)
			{
				buffer.append(lua_file->buffer, lua_file->len);
			}

			const auto out_name = utils::string::va("lua_dump/%s", lua_file->name);
			utils::io::write_file(out_name, buffer);

			printf("Dumped %s\n", lua_file->name);
		}

		utils::hook::detour load_luafileasset_hook;
		void load_luafileasset_stub(LuaFile** lua_file)
		{
			dump_lua_file(lua_file);
			load_luafileasset_hook.invoke<void>(lua_file);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_start() override
		{
			utils::hook::set<uint8_t>(0x3061A0_b, 0xC3); // mystery function for Windows 11 users

			// name dvar
			com_register_dvars_hook.create(0x12B0CD0_b, com_register_dvars_stub);

			// force offline menus + dev dvars
			dvar_register_hook.create(0x13E7D40_b, dvar_register_stub);

			auto pipeline = scheduler::renderer;
			if (game::environment::is_dedi())
			{
				pipeline = scheduler::main;
			}

			schedule([=]()
			{
				// funny Donetsk workaround to get into menus lmfao
				if (tick != 500)
				{
					tick += 1;
					return scheduler::cond_continue;
				}

				printf("running authentication and profile hooks\n");

				// go straight to main menu
				game::GamerProfile_SetDataByName(0, "acceptedEULA", 1);
				game::GamerProfile_SetDataByName(0, "hasEverPlayed_MainMenu", 1);

				// bunch of auth stuff copy & pasted from codUPLOADER (gets us in lobby)
				game::XUID xuid{};
				xuid.random_xuid();

				utils::hook::set<int>(0x4622BE0_b, 1);

				utils::hook::set<uintptr_t>(0xE5C07C0_b, 0x11CB1243B8D7C31E | xuid.m_id * xuid.m_id);
				utils::hook::set<uintptr_t>(0xF05ACE8_b, 0x11CB1243B8D7C31E | xuid.m_id * xuid.m_id);

				utils::hook::set<uintptr_t>(0xE5C07E8_b, 0x11CB1243B8D7C31E | (xuid.m_id * xuid.m_id) / 6); // s_presenceData

				utils::hook::set<int>(0xE371231_b, 1);
				utils::hook::set<int>(0x4622910_b, 2);
				utils::hook::set<int>(0x4622BE0_b, 1);

				utils::hook::set<char>(*reinterpret_cast<uintptr_t*>(0xEE560B0_b) + 0x28, 0); // dont disconnect if xp discreases
				utils::hook::set(0xE5C0730_b, 2);

				auto get_bnet_class = reinterpret_cast<uintptr_t(*)()>(0x1660280_b);
				uintptr_t bnet_class = get_bnet_class();
				*(DWORD*)(bnet_class + 0x2F4) = 0x795230F0;
				*(DWORD*)(bnet_class + 0x2FC) = 0;
				*(BYTE*)(bnet_class + 0x2F8) = 31;

				return scheduler::cond_end;
			}, pipeline);
		}

		void post_unpack() override
		{
			// allows settext method to work with strings that are not localized
			//g_find_config_string_index_hook.create(0x10E9140_b, g_find_config_string_index_stub);

			// use name dvar
			utils::hook::jump(0x13FD3A0_b, live_get_local_client_name_stub);

			// dw
			utils::hook::jump(0x1A04BD0_b, signin_state_stub); // LUI_CoD_LuaCall_betSignInState
			utils::hook::jump(0x1AC2570_b, is_paid_user_stub); // LiveStorage_IsPaidUser

			utils::hook::jump(0x1528470_b, live_is_offline_tool);				// Live_IsOfflineTool
			utils::hook::jump(0x1528490_b, live_is_user_signed_into_dw_stub);	// Live_IsUserSignedInToDw
			utils::hook::jump(0x17EC930_b, dw_log_on_status_stub);				// dwGetLogOnStatus
			utils::hook::jump(0x12A1EB0_b, get_activate_stats_source_stub);
			utils::hook::jump(0x19B96A0_b, lui_is_demo_build_stub);				// LUI_IsDemoBuild

			// bgs
			utils::hook::nop(0x12AFAE5_b, 40); // BGS init (Com_Init_Try_Block_Function)
			utils::hook::set<uint8_t>(0x1665AC0_b, 0xC3); // BGS connect
			utils::hook::set<uint8_t>(0x165F300_b, 0xC3); // BGS shutdown

			// patch ui_maxclients limit
			utils::hook::nop(0x0F30210_b, 5);
			utils::hook::nop(0x119E51D_b, 5);
			utils::hook::nop(0x136B8F8_b, 5);
			utils::hook::nop(0x16029F0_b, 5);
			utils::hook::nop(0x19E19A3_b, 5);

			// patch party_maxplayers limit
			utils::hook::nop(0x0F252EE_b, 5);
			utils::hook::nop(0x119D23F_b, 5);
			utils::hook::nop(0x10769B9_b, 5);
			utils::hook::set(0x10769B9_b, 0xC3);
			utils::hook::nop(0x0F24B4B_b, 5);
			utils::hook::set(0x0F24B4B_b, 0xC3);
			utils::hook::nop(0x16029E2_b, 5);
			utils::hook::nop(0x119E52B_b, 5);
			utils::hook::nop(0x0f252EE_b, 5);
			utils::hook::nop(0x119F13A_b, 5);
			utils::hook::nop(0x10D32E2_b, 5);

			// removes "Services aren't ready yet." print
			utils::hook::nop(0x1504374_b, 5);

			// bypass wz collision (missing file)
			utils::hook::jump(0xD6B7D0_b, set_transient_mode_stub); // CL_TransientsCollisionMP_SetTransientMode

			// add commands
			//game::Cmd_AddCommandInternal("addbot", Cmd_AddBot_f, &addbot_f_VAR);
			//game::Cmd_AddCommandInternal("addtestclient", Cmd_AddTestClient_f, &addTestClient_f_VAR);

			// modify strings to reveal maps not working
			seh_string_ed_get_string_hook.create(0x13CC2A0_b, seh_string_ed_get_string_stub);

			// debug LUI errors more in depth
			utils::hook::call(0x19BCD56_b, set_table_string_stub); // get name for event in LuaShared_SetTableString
			utils::hook::call(0x19BD9C4_b, report_error_with_info_stub); // LUI_ReportErrorWithInfo

			//load_luafileasset_hook.create(0xF61630_b, load_luafileasset_stub);
		}
	};
}

REGISTER_COMPONENT(patches::component)
