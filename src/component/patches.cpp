#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "scheduler.hpp"

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
						//printf("find_signatures call\n");
						component_loader::find_signatures();
					}
				});
			}
		}

		void post_unpack() override
		{
			if (identification::game::is("1.20.4-replay"))
			{
				/*
				scheduler::loop([]()
				{
					render_pm_debug();
				}, scheduler::renderer);
				*/
			}
		}
	};
}

REGISTER_COMPONENT(patches::component)
