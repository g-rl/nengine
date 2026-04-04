#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "scheduler.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/flags.hpp>
#include <utils/io.hpp>
#include <utils/nt.hpp>

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

		utils::hook::detour com_register_dvars_hook;
		void com_register_dvars_stub()
		{
			// make name save + default to login username
			name_dvar = game::Dvar_RegisterString("name", utils::nt::get_login_username().data(), game::DVAR_FLAG_SAVED, "Player name.");

			com_register_dvars_hook.invoke<void>();
		}

		int signin_state_stub(uintptr_t state)
		{
			game::lua_pushnumber(state, 2); // BattleNetSignInState.signedIn
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
				//|| !strcmp(name, "LSSRRSMNMR")	// lui_dev_features_enabled
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

			if (!strcmp(name, "MTRLPQOPSR") || !strcmp(name, "NLNTMRRQML"))
			{
				value->integer = 300000;
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
		void load_luafileasset_stub(LuaFile** lua_file_)
		{
			auto lua_file = *lua_file_;
			if (lua_file->name)
			{
				std::string data;
				if (utils::io::read_file(lua_file->name, &data))
				{
					const auto data_ = data.data();
					lua_file->buffer = data_;
					lua_file->len = strlen(data_);
					printf("overriding \"%s\"\n", lua_file->name);
				}
			}

			load_luafileasset_hook.invoke<void>(lua_file);
		}

		bool live_is_signed_in_stub(int index)
		{
			return true;
		}

		inline game::FontGlowStyle s_legacyShadow = {
			-0.4f, 0.f,
			{ -0.001f, -0.001f },
			{ 0.f, 0.f, 0.f, 1.f },
			0.f, 0.f,
			{ 0.f, 0.f, 0.f, 0.f }
		};

		void render_pm_debug()
		{
			// if nothing is going on, we dont use this
			if (!utils::hook::invoke<bool>(0x12B0290_b)) // Com_IsGameLocalServerRunning
			{
				return;
			}

			// we ignore vlobby too
			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				return;
			}

			static game::GfxFont* overlay_font = utils::hook::invoke<game::GfxFont*>(0x19329B0_b, "fonts/fira_mono_bold.ttf", 36); // R_RegisterFont
			if (!overlay_font) return;

			auto* font_glow_style = &s_legacyShadow;
			const bool font_use_post = false;

			auto draw_text = [&](const char* text, float x, float y, float scale, float* color) {
				// R_AddCmdDrawText
				utils::hook::invoke<void>(0x1965330_b, text, std::numeric_limits<int>::max(), overlay_font, overlay_font->height, x, y, scale, scale, 0.f,
					color, font_glow_style, font_use_post);
			};

			//int width = *reinterpret_cast<int*>(0xEF2DEC0_b);
			//int height = *reinterpret_cast<int*>(0xEF2DEC4_b);

			//printf("trying 1\n");

			const float scale = 0.80f;
			const float line_h = static_cast<float>(overlay_font->height) * scale;

			game::cg_t* cg = *reinterpret_cast<game::cg_t**>(0xF26F940_b);
			if (!cg || !cg->predictedPlayerstate)
			{
				return;
			}

			// yeee
			const auto flags = cg->predictedPlayerstate->pm_flags;

			game::vec4_t on_col  = { 0.f, 1.f, 0.f, 1.f };
			game::vec4_t off_col = { 0.6f, 0.6f, 0.6f, 1.f };

			const float x = 50.f;
			float y = 100.f;

#define DRAW_FLAG(flag_val, flag_name) \
			draw_text(#flag_name, x, y, scale, ( (flags & flag_val) != 0 ) ? (float*)on_col : (float*)off_col); \
			y += line_h;

			DRAW_FLAG(0x1,        PMF_PRONE)
			DRAW_FLAG(0x2,        PMF_DUCKED)
			DRAW_FLAG(0x4,        PMF_MANTLE)
			DRAW_FLAG(0x8,        PMF_LADDER)
			DRAW_FLAG(0x10,       PMF_SIGHT_AIMING)
			DRAW_FLAG(0x20,       PMF_BACKWARDS_RUN)
			DRAW_FLAG(0x40,       PMF_WALKING)
			DRAW_FLAG(0x80,       PMF_TIME_HARDLANDING)
			DRAW_FLAG(0x100,      PMF_TIME_KNOCKBACK)
			DRAW_FLAG(0x200,      PMF_PRONEMOVE_OVERRIDDEN)
			DRAW_FLAG(0x400,      PMF_RESPAWNED)
			DRAW_FLAG(0x800,      PMF_FROZEN)
			DRAW_FLAG(0x1000,     PMF_LADDER_FALL)
			DRAW_FLAG(0x2000,     PMF_JUMPING)
			DRAW_FLAG(0x4000,     PMF_SPRINTING)
			DRAW_FLAG(0x8000,     PMF_SHELLSHOCKED)
			DRAW_FLAG(0x10000,    PMF_MELEE_CHARGE)
			DRAW_FLAG(0x20000,    PMF_NO_SPRINT)
			DRAW_FLAG(0x40000,    PMF_NO_JUMP)
			DRAW_FLAG(0x80000,    PMF_REMOTE_CONTROLLING)
			DRAW_FLAG(0x100000,   PMF_SLIDE)
			DRAW_FLAG(0x800000,   PMF_NO_STAND)
			DRAW_FLAG(0x1000000,  PMF_NO_CROUCH)
			DRAW_FLAG(0x2000000,  PMF_NO_PRONE)
			DRAW_FLAG(0x4000000,  PMF_NO_LEAN)
			DRAW_FLAG(0x8000000,  PMF_NO_MELEE)
			DRAW_FLAG(0x10000000, PMF_NO_FIRE)
			DRAW_FLAG(0x20000000, PMF_NO_LADDER)
			DRAW_FLAG(0x40000000, PMF_NO_MANTLE)
#undef DRAW_FLAG
		}
	}

	class component final : public component_interface
	{
	public:
		void post_start() override
		{
			/*
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

			// go straight to main menu
			//game::GamerProfile_SetDataByName(0, "acceptedEULA", 1);
			//game::GamerProfile_SetDataByName(0, "hasEverPlayed_MainMenu", 1);

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
				*(DWORD*)(bnet_class + 756) = 0x795230F0;
				*(BYTE*)(bnet_class + 760) = 31;
				*(DWORD*)(bnet_class + 764) = 0;

				return scheduler::cond_end;
			}, pipeline);
			*/
		}

		void post_unpack() override
		{
			scheduler::loop([]()
			{
				render_pm_debug();
			}, scheduler::renderer);

			// add data for pmove stuff

			// 
		}
	};
}

REGISTER_COMPONENT(patches::component)
