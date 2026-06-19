#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "scheduler.hpp"
#include "scripting.hpp"

#include "game/game.hpp"
#include <identification/game.hpp>

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/io.hpp>

namespace patches
{
	namespace
	{
		inline game::FontGlowStyle s_legacyShadow = {
			-0.4f, 0.f,
			{ -0.001f, -0.001f },
			{ 0.f, 0.f, 0.f, 1.f },
			0.f, 0.f,
			{ 0.f, 0.f, 0.f, 0.f }
		};

		game::dvar_t* neura_session_should_save = nullptr;
		game::dvar_t* neura_session_read_complete = nullptr;
		game::dvar_t* neura_session_data_count = nullptr;
		game::dvar_t* neura_session_data_current = nullptr;

		game::dvar_t* neura_session_should_load = nullptr;
		game::dvar_t* neura_session_write_complete = nullptr;
		std::vector<std::pair<std::string, std::string>> neura_load_entries;
		size_t neura_load_index = 0;

		void* GameMessageVA_Stub{};

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

			const float scale = 0.80f;
			const float line_h = static_cast<float>(overlay_font->height) * scale;

			game::cg_t* cg = *reinterpret_cast<game::cg_t**>(0xF26F940_b);
			if (!cg || !cg->predictedPlayerstate)
			{
				return;
			}

			// yeee
			const auto flags = cg->predictedPlayerstate->pm_flags[0];

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

	utils::hook::detour sv_kick_client_num_hook;
	void sv_kick_client_num_stub(const int client_num, const char* reason, bool kicked_for_inactivity)
	{
		printf("%s\n", reason);
		if (!strcmp(reason, "EXE/PLAYERKICKED_BOT_BALANCE"))
		{
			return;
		}

		sv_kick_client_num_hook.invoke<void>(client_num, reason, kicked_for_inactivity);
	}

	char* make_game_message_stub(const char* a2, int a3, const char* a4)
	{
		printf("[GScr_MakeGameMessage] %s\n", a4);
		return utils::hook::invoke<char*>(0x13F3010_b, a2, a3, a4);
	}

	utils::hook::detour make_game_message_hook;
	char* make_game_message_stub_iw9(char* a1, char* a2, int a3, const char* a4)
	{
		printf("[GScr_MakeGameMessage] %s\n", a4);
		return game::Core_strcpy_va(a1, a2, a3, a4);
	}

	class component final : public component_interface
	{
	public:
		component()
		{
			const auto version_dll = GetModuleHandleA("version.dll");
			if (!version_dll)
				return;

			const auto set_output_callback = reinterpret_cast<void(*)(void(*)(const char*))>(
				GetProcAddress(version_dll, "set_output_callback"));

			if (set_output_callback)
			{
				set_output_callback([](const char* msg)
				{
					static bool done = false;
					if (done)
						return;

					// skip past ANSI escape sequences to find the actual text
					const char* p = msg;
					std::string stripped;
					while (*p)
					{
						if (*p == '\x1b')
						{
							while (*p && *p != 'm') p++;
							if (*p) p++;
						}
						else
						{
							stripped += *p++;
						}
					}

					if (strstr(stripped.c_str(), "Found") && strstr(stripped.c_str(), "out of"))
					{
						component_loader::find_signatures();
					}

					if (identification::game::get_target_game().client_name == "iw9-mod"s && strstr(stripped.c_str(), "Created inline hooks for checksums"))
					{
						//component_loader::post_unpack();
						done = true;
					}
				});
			}
		}

		void find_signatures(memory::signature_store& batch) override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw8-mod"s)
			{
				batch.add(SETUP_POINTER(GameMessageVA_Stub),
					"48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B ? 24 ? ? 00 00 4C 8B C0", SETUP_MOD(add(7)));

				batch.add(SETUP_POINTER(game::Core_strcpy_va),
					"48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B ? 24 ? ? 00 00 4C 8B C0", SETUP_MOD(add(7).rip()));
			}
			else
			{
				batch.add(SETUP_POINTER(GameMessageVA_Stub),
					"E8 ? ? ? ? 48 8B BC 24 ? ? ? ? 48 8B B4 24 ? ? ? ? 48 8B 9C 24 ? ? 00 00 83");

				batch.add(SETUP_POINTER(game::Core_strcpy_va),
					"E8 ? ? ? ? 48 8B BC 24 ? ? ? ? 48 8B B4 24 ? ? ? ? 48 8B 9C 24 ? ? 00 00 83",
					GRAB_CALL);
			}
		}

		void post_unpack() override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;

			if (game_ == "iw9-mod"s)
			{
				utils::hook::call(GameMessageVA_Stub, make_game_message_stub_iw9);
			}
			else if (identification::game::is("1.20.4-replay"))
			{
				//sv_kick_client_num_hook.create(0x36C160_b, sv_kick_client_num_stub);

				// show console prints from iprintln[bold] from GSC
#ifdef _DEBUG
				utils::hook::call(0x1259AD2_b, make_game_message_stub);
#endif
			}

			scheduler::once([]
			{
				static auto version_str = identification::game::get_version(false);
				static auto version_str_full = identification::game::get_version(true);

				// create a simplified version dvar for script to read
				game::Dvar_RegisterString("build_version", version_str.c_str(), game::DVAR_NOFLAG, "");
				game::Dvar_RegisterString("build_version_full", version_str_full.c_str(), game::DVAR_NOFLAG, "");
			
			}, scheduler::main);
		}
	};
}

REGISTER_COMPONENT(patches::component)
