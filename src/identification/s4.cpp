#include <std_include.hpp>
#include "game.hpp"

namespace identification::game
{
	game_iden get_s4_game_iden()
	{
		return game_iden{
			"s4-mod", "Vanguard", {
				{ { 0, 02, 7, 10140263 }, MP,  BattleNet, Beta, 0xAF79E042, XXH32 }, // 16/09/2021 23:04:55
				{ { 1, 10, 0, 10819777 }, AIO, BattleNet, Ship, 0x357A41F0, XXH32 }, // 15/12/2021 04:48:47
				{ { 1, 14, 0, 11077351 }, AIO, BattleNet, Ship, 0xAB2F41BC, XXH32 }, // 17/02/2022 17:10:29
				{ { 1, 16, 0, 11283250 }, AIO, BattleNet, Ship, 0xAABADD37, XXH32 }, // 25/03/2022 01:28:08
				{ { 1, 24, 1, 12510470 }, CDL, BattleNet, Ship, 0x6399DB19, XXH32 }, // 11/10/2022 20:49:30
				{ { 1, 26, 1, 13860558 }, AIO, BattleNet, Ship, 0xCFFEE091, XXH32 }, // 29/01/2024 23:00:47
				{ { 1, 26, 1, 13860558 }, AIO, Steam,     Ship, 0xEB933916, XXH32 }  // 30/01/2024 00:54:55
			}
		};
	}
}
