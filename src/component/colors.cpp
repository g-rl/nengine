#include <std_include.hpp>
#include "loader/component_loader.hpp"

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

		void* ColorIndex_call{};

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

		__int64 color_index_stub(char a1)
		{
			unsigned __int8 v1; // cl
			__int64 result; // rax

			// set the ^: value to rainbow here - this check is inside RB_LookupColor, which is inlined on replay..?
			const auto rgb = hsv_to_rgb({ static_cast<uint8_t>((game::Sys_Milliseconds() / 100) % 256), 255, 255 });
			*reinterpret_cast<DWORD*>(0x10C793BC_b) = 0xFF000000u | rgb;

			v1 = a1 - 39;
			result = 16LL;
			if (v1 < 24u)
				return v1;
			return result;
		}
	}

	class component final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			batch.add(SETUP_POINTER(game::Com_CleanName),
				"40 53 45 33 DB 41 FF C8 44 88 1A 45 33 D2 44 0F B6 09 48 8B DA 48 FF C1 45 84 C9 74 3A");

			batch.add(SETUP_POINTER(game::ColorIndex),
				"48 83 EC 20 0F ? ? 49 ? ? 0F ? ? E8", SETUP_MOD(add(14).rip()));

			batch.add(SETUP_POINTER(game::CL_LookupColor),
				"48 89 5C 24 08 57 48 83 EC 20 0F B6 CA 49 8B D8 0F B6 FA E8");

			if (identification::game::is("1.20.4-replay"))
				batch.add(SETUP_POINTER(ColorIndex_call), "E8 ? ? ? ? 0F ? ? 83 ? 11 73 19");
			else
				batch.add(SETUP_POINTER(ColorIndex_call), "48 8B FA 0F B6 D9 E8 ? ? ? ? 44 0F ? ? 41 ? ? ? 0F", SETUP_MOD(add(7).rip()));
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
			utils::hook::call(ColorIndex_call, color_index_stub);

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
