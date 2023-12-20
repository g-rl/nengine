#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/flags.hpp>

namespace patches
{
	namespace
	{
		const game::dvar_t* name_dvar = nullptr;

		const char* live_bet_local_client_name()
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
			// make name save
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

		bool bgs_init_jnz_stub()
		{
			// no clue what this does, but it shouldn't initialize BGS stuff
			return true;
		}
		
		utils::hook::detour dvar_register_hook;
		game::dvar_t* dvar_register_stub(const char* name, unsigned int checksum, unsigned __int8 type, 
			game::DvarFlags flags, game::DvarValue* value, void* domain, const char* desc)
		{
			if (strcmp(name, "MPSSOTQQPM") == 0		// force_offline_enabled
				|| strcmp(name, "LSTQOKLTRN") == 0	// force_offline_menus
				|| strcmp(name, "LKRTMSRPRO") == 0)	// online_check_online_data_fence_before_showing_signin_error
			{
				const auto val1 = value->enabled;
				const auto val2 = value->integer;
				value->enabled = true;
				value->integer = 1;
				printf("dvar '%s' overrided with new values (old: %d-%d, new: %d-%d)\n", name, val1, val2, value->enabled, value->integer);
			}

			return dvar_register_hook.invoke<game::dvar_t*>(name, checksum, type, flags, value, domain, desc);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_start() override
		{
			//utils::hook::set<uint8_t>(0x3061A0_b, 0xC3); // mystery function 1

			// name dvar
			com_register_dvars_hook.create(0x12B0CD0_b, com_register_dvars_stub);

			// force offline menus + text chat
			dvar_register_hook.create(0x13E7D40_b, dvar_register_stub);
		}

		void post_unpack() override
		{
			// use name dvar
			utils::hook::jump(0x13FD3A0_b, live_bet_local_client_name);

			// dw stuff
			utils::hook::jump(0x1A04BD0_b, signin_state_stub); // LUI_CoD_LuaCall_GetSignInState
			utils::hook::jump(0x1AC2570_b, is_paid_user_stub); // LiveStorage_IsPaidUser

			utils::hook::jump(0x1528470_b, live_is_offline_tool);				// Live_IsOfflineTool
			utils::hook::jump(0x1528490_b, live_is_user_signed_into_dw_stub);	// Live_IsUserSignedInToDw
			utils::hook::jump(0x17EC930_b, dw_log_on_status_stub);				// dwGetLogOnStatus
			utils::hook::jump(0x12A1EB0_b, get_activate_stats_source_stub);
			utils::hook::jump(0x19B96A0_b, lui_is_demo_build_stub);				// LUI_IsDemoBuild
			utils::hook::call(0x12AFAEA_b, bgs_init_jnz_stub);	// BGS init (Com_Init_Try_Block_Function)
			utils::hook::set<uint8_t>(0x1665AC0_b, 0xC3);		// BGS connect
			utils::hook::set<uint8_t>(0x165F300_b, 0xC3);		// BGS shutdown
		}
	};
}

REGISTER_COMPONENT(patches::component)
