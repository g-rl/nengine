#include <std_include.hpp>
#include "game.hpp"

namespace identification::game
{
	game_iden get_iw9_game_iden()
	{
		return game_iden{
			"iw9-mod", "Modern Warfare® II", {
				{ { 1, 03, 2, 12480788 }, MP, BattleNet, Beta, 0x07D14DD5, XXH32 },	// 23/09/2022 22:46:21
				{ { 1, 07, 0, 13200072 }, MP, Steam,     Ship, 0xE0F635A3 },		// 13/11/2022 16:56:27 (NZST)
				{ { 1, 12, 0, 13758007 }, MP, Steam,     Ship, 0xE4F7CCB2 },		// 09/02/2023 17:31:39
				{ { 1, 14, 0, 14265121 }, MP, Steam,     Ship, 0xE560F9D9 },		// 07/04/2023 05:08:45
				{ { 1, 18, 0, 15257229 }, MP, Steam,     Ship, 0x58879A9A },		// 18/07/2023 22:24:50
				{ { 1, 18, 0, 15257229 }, MP, BattleNet, Ship, 0xF3A1CB7F, XXH32 },	// 18/07/2023 22:30:50
				{ { 1, 19, 0, 15332815 }, MP, Steam,     Ship, 0xFCABC600 },		// 25/07/2023 18:29:41
				{ { 1, 25, 0, 16245188 }, MP, Steam,     Ship, 0xC26D5FD7, XXH32 },	// 07/10/2023 03:29:42
				{ { 1, 40, 0, 23226476 }, MP, Steam,     Ship, 0x23301CA2 },		// 18/07/2025 15:08:33
				
				// i think this is the build i'm on, but this wasn't what ZeroProxy had
				// i calculated XXH32 myself and got this
				{ { 1, 40, 0, 23226476 }, MP, BattleNet, Ship, 0xA079B405, XXH32 },	// 18/07/2025 15:25:33
				
				{ { 1, 40, 0, 23226476 }, MP, MsStore,   Ship, 0xE61E38D1, XXH32 } 	// 18/07/2025 15:25:33

				// sp22 found in 1.18 BNET - 0x8084C556
			}
		};
	}
}

