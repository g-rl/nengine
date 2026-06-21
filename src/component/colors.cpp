#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "call_spoofer.hpp"
#include "scheduler.hpp"

#include "game/game.hpp"
#include <identification/game.hpp>

#include <utils/hook.hpp>
#include <utils/string.hpp>

constexpr auto MAX_COLOR_INDEX = 28; // 24 is default max IW8

namespace colors
{
	struct hsv_color
	{
		unsigned char h;
		unsigned char s;
		unsigned char v;
	};

	namespace
	{
		game::dvar_t* r_color_mode = nullptr;
		
		std::vector<DWORD> color_table;

		void* rainbow_color_override_addr{};

		DWORD hsv_to_rgb(const hsv_color hsv)
		{
			DWORD rgb;

			if (hsv.s == 0)
			{
				return RGB(hsv.v, hsv.v, hsv.v);
			}

			// converting to 16 bit to prevent overflow
			const unsigned int h = hsv.h;
			const unsigned int s = hsv.s;
			const unsigned int v = hsv.v;

			const auto region = static_cast<uint8_t>(h / 43);
			const auto remainder = (h - (region * 43)) * 6;

			const auto p = static_cast<uint8_t>((v * (255 - s)) >> 8);
			const auto q = static_cast<uint8_t>(
				(v * (255 - ((s * remainder) >> 8))) >> 8);
			const auto t = static_cast<uint8_t>(
				(v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);

			switch (region)
			{
			case 0:
				rgb = RGB(v, t, p);
				break;
			case 1:
				rgb = RGB(q, v, p);
				break;
			case 2:
				rgb = RGB(p, v, t);
				break;
			case 3:
				rgb = RGB(p, q, v);
				break;
			case 4:
				rgb = RGB(t, p, v);
				break;
			default:
				rgb = RGB(v, p, q);
				break;
			}

			return rgb;
		}

		int color_index(const char c)
		{
			const auto index = c - 39;
			return (index > MAX_COLOR_INDEX ? 16 : index);
		}

		char add(const uint8_t r, const uint8_t g, const uint8_t b)
		{
			const char index = '0' + static_cast<char>(color_table.size());
			color_table.emplace_back(RGB(r, g, b));
			return index;
		}

		void com_clean_name_stub(const char* in, char* out, const int out_size)
		{
			// check that the name is at least 3 char without colors
			char name[32]{};

			game::Core_strcpy(out, std::min<int>(out_size, sizeof(name)), in);

			utils::string::strip(out, name, std::min<int>(out_size, sizeof(name)));
			if (std::strlen(name) < 3)
			{
				game::Core_strcpy(out, std::min<int>(out_size, sizeof(name)), "UnnamedPlayer");
			}
		}

		char* i_clean_str_stub(char* string)
		{
			utils::string::strip(string, string, static_cast<int>(strlen(string)) + 1);
			return string;
		}

		utils::hook::detour ColorIndex_hook;
		__int64 color_index_stub(char a1)
		{
			/*
			if (!rainbow_color_override_addr)
			{
				ColorIndex_hook.clear();
				return call_spoofer::spoof_hook_invoke<__int64>(ColorIndex_hook, a1);
			}
			*/

			if (rainbow_color_override_addr)
			{
				// set the ^: value to rainbow here - this check is inside RB_LookupColor, which is inlined on replay..?
				const auto rgb = hsv_to_rgb({ static_cast<uint8_t>((game::Sys_Milliseconds() / 30) % 256), 255, 255 });
				*reinterpret_cast<DWORD*>(rainbow_color_override_addr) = 0xFF000000u | rgb;
			}

			return call_spoofer::spoof_hook_invoke<__int64>(ColorIndex_hook, a1);
		}
	}

	class component final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			batch.add(SETUP_POINTER(game::Com_CleanName),
				"41 B8 24 00 00 00 ? 8D ? ? ? 00 00 48 8B C8 E8", SETUP_MOD(add(17).rip()));

			//batch.add(SETUP_POINTER(game::CL_LookupColor),
			//	"48 89 5C 24 08 57 48 83 EC 20 0F B6 CA 49 8B D8 0F B6 FA E8");

			static const auto& game_ = identification::game::get_target_game().client_name;

			if (game_ != "iw8-mod"s)
			{
				batch.add(SETUP_POINTER(game::ColorIndex), "? ? ? ? ? C7 02 FF FF FF FF E8 ? ? ? FE", SETUP_MOD(add(12).rip()));
				batch.add(SETUP_POINTER(rainbow_color_override_addr), "8B 84 ? ? ? ? 04 ? 03 ? 41 FF E0 8B 05",
					SETUP_MOD(add(15).rip().add(8)));
			}
			else
			{
				batch.add(SETUP_POINTER(game::ColorIndex), "80 E9 ?? B8 ?? 00 00 00 0F B6 ?? 80 FA");

				if (identification::game::is("1.20.4-replay"))
				{
					rainbow_color_override_addr = reinterpret_cast<void*>(0x10C793BC_b);
				}
				else
				{
					if (identification::game::is_in_range("1.19.1", "1.23.0"))
					{
						batch.add(SETUP_POINTER(rainbow_color_override_addr), "E8 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B ? ? ? E8 ? ? ? ? 48 ? ? 28 C3",
							SETUP_MOD(add(8).rip()));
					}
					else
					{
						// 1.03, 1.16.1, 1.23.0 to 1.44.0
						batch.add(SETUP_POINTER(rainbow_color_override_addr), "48 8B ? E8 ? ? ? ? 48 8D ? ? ? ? ? 48 8B CF 48 8B ? ? ? 48 83 C4 20",
							SETUP_MOD(add(11).rip()));
					}

					// this isnt working
					/*
					batch.add(SETUP_POINTER(rainbow_color_override_addr), "83 E9 38 74 ? 83 E9 01 74 ? 83 E9 01 74 ? 83",
						[](memory::scanned_result<void> r) {
							auto buf = r.add(13).as<std::uint8_t*>();
							auto jz_target = r.add(15).add(static_cast<std::int8_t>(buf[1]));
							return jz_target.add(2).rip();
						});
					*/
				}
			}
		}

		void post_unpack() override
		{
			// allows colored name in-game
			utils::hook::jump(game::Com_CleanName, com_clean_name_stub, true);

			// don't apply colors to overhead names
			//utils::hook::call(0x1406843FE, get_client_name_stub);

			// patch I_CleanStr
			utils::hook::jump(game::I_CleanStr, i_clean_str_stub, true);
			
			// make color index higher for more colors
			//utils::hook::jump(0x140CFA6F0, color_index, true);
			//utils::hook::set<uint8_t>(0x140E4F64B, MAX_COLOR_INDEX);

			// force new colors
			ColorIndex_hook.create(game::ColorIndex, color_index_stub);

			// prevent name mismatch check
			//utils::hook::set<uint8_t>(0x140805C10, 0xC3);

			// add colors
			add(0, 0, 0);		// ^0 black (original)
			add(255, 0, 0);		// ^1 red (original)
			add(0, 255, 0);		// ^2 green (original)
			add(255, 255, 0);	// ^3 yellow (original)
			add(0, 135, 193);	// ^4 blue (easier to see)
			add(25, 200, 230);	// ^5 light blue (original)
			add(255, 92, 255);	// ^6 pink (original)
			add(255, 255, 255);	// ^7 white (original)

			// these are all handled in rb_lookup_color_stub
			add(0, 0, 0);		// ^8 friendly team color (original)
			add(0, 0, 0);		// ^9 enemy team color (original)
			add(0, 0, 0);		// ^: rainbow color code (original is "my party")
			add(0, 0, 0);		// ^; facebook blue (original, ';' is an illegal character for infostrings)
			add(0, 0, 0);		// ^< sky blue (idek where this comes from)
		}
	};
}

REGISTER_COMPONENT(colors::component)
