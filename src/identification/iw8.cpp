#include <std_include.hpp>
#include "game.hpp"

namespace identification::game
{
	game_iden get_target_info_game()
	{
		return game_iden{
			"iw8-mod", "Modern Warfare®",
			{
				// prerelease builds
				{ { 0, 01,  2,  7089334 }, AIO, BattleNet, Beta,   0xDCA903BC, XXH32 }, // 20/09/2019 00:47:24
				{ { 1, 01,  0,  7199386 }, AIO, BattleNet, Ship,   0x4C556A52, XXH32 }, // 23/10/2019 10:56:07 (NZST)
				{ { 1, 03,  0,  7209368 }, AIO, BattleNet, Ship,   0xB51FA3A9, XXH32 }, // 24/10/2019 03:30:49

				{ { 1, 16,  1,  7496249 }, AIO, BattleNet, Ship,   0x2153537C, XXH32 }, // 08/03/2020 00:15:39
				{ { 1, 19,  1,  7547737 }, AIO, BattleNet, Ship,   0xE05AD7B8, XXH32 }, // 09/04/2020 21:01:15
				{ { 1, 20,  4,  7623265 }, AIO, BattleNet, Replay, 0x2DF6FFE0, XXH32 }, // 19/04/2020 02:55:12
				{ { 1, 20,  4,  7623265 }, AIO, BattleNet, Ship,   0xEA549A79, XXH32 }, // 04/05/2020 19:06:16
				{ { 1, 21,  1,  7647827 }, AIO, BattleNet, Ship,   0xBB472E97, XXH32 }, // 15/05/2020 23:28:21
				{ { 1, 23,  0,  7776274 }, AIO, BattleNet, Ship,   0xE2465A64, XXH32 }, // 27/06/2020 15:41:18
				{ { 1, 23,  2,  7787568 }, AIO, BattleNet, Ship,   0xA13C9E4D, XXH32 }, // --/--/---- --:--:--
				{ { 1, 24,  0,  7837446 }, AIO, BattleNet, Ship,   0x3FE45E35, XXH32 }, // 19/07/2020 05:45:26
				{ { 1, 26,  3,  7961954 }, AIO, BattleNet, Ship,   0x335F0F61, XXH32 }, // --/--/---- --:--:--
				{ { 1, 27,  4,  8126717 }, AIO, BattleNet, Ship,   0x40037E73, XXH32 }, // 07/10/2020 18:39:41
				{ { 1, 28,  3,  8175163 }, AIO, BattleNet, Ship,   0x51EEA0C6, XXH32 }, // 19/10/2020 20:54:56
				{ { 1, 30,  0,  8403677 }, AIO, BattleNet, Ship,   0x7E80EEB2, XXH32 }, // 11/12/2020 08:33:41
				{ { 1, 31,  5,  8643996 }, AIO, BattleNet, Ship,   0x21293365, XXH32 }, // 17/02/2021 17:38:12
				{ { 1, 34,  1,  8774611 }, AIO, BattleNet, Ship,   0x729EC95E, XXH32 }, // 22/03/2021 16:04:17
				{ { 1, 36,  1,  8976727 }, AIO, BattleNet, Ship,   0x005887EE, XXH32 }, // 17/04/2021 21:07:13
				{ { 1, 37,  7,  9348574 }, AIO, BattleNet, Ship,   0x1ADC910A, XXH32 }, // 09/06/2021 23:29:52
				{ { 1, 38,  3,  9489393 }, AIO, BattleNet, Ship,   0x3F0FDD65, XXH32 }, // 22/06/2021 18:34:30
				{ { 1, 39,  3,  9786493 }, AIO, BattleNet, Ship,   0x9FC668C2, XXH32 }, // 22/07/2021 18:43:16
				{ { 1, 39,  6,  9873021 }, AIO, BattleNet, Ship,   0x55C7D4AC, XXH32 }, // 03/08/2021 22:48:50
				{ { 1, 40,  0,  9926265 }, AIO, BattleNet, Ship,   0x819BBADA, XXH32 }, // 09/08/2021 16:29:41
				{ { 1, 41,  2, 10056273 }, AIO, BattleNet, Ship,   0x94FA4C02, XXH32 }, // 25/08/2021 18:41:14
				{ { 1, 42,  1, 10125479 }, AIO, BattleNet, Ship,   0x7289FED9, XXH32 }, // 07/09/2021 03:26:04
				{ { 1, 43,  1, 10402486 }, AIO, BattleNet, Ship,   0x09FAB5D8, XXH32 }, // 07/10/2021 23:47:30
				{ { 1, 44,  0, 10435696 }, AIO, BattleNet, Ship,   0xAFCEFB09, XXH32 }, // 13/10/2021 18:52:40
				{ { 1, 45,  0, 10517085 }, AIO, BattleNet, Ship,   0x41C910A9, XXH32 }, // 27/10/2021 00:26:42
				{ { 1, 46,  0, 10750827 }, AIO, BattleNet, Ship,   0x51E33B0C, XXH32 }, // 02/12/2021 09:09:18
				{ { 1, 50,  2, 10821704 }, AIO, BattleNet, Ship,   0xCDE5C7CC, XXH32 }, // --/--/---- --:--:--
				{ { 1, 51,  0, 10885062 }, AIO, BattleNet, Ship,   0x8D0D23F3, XXH32 }, // 08/01/2022 00:50:55
				{ { 1, 51,  1, 10909157 }, AIO, BattleNet, Ship,   0x6698AA9B, XXH32 }, // 14/01/2022 20:12:44
				{ { 1, 51,  2, 10930490 }, AIO, BattleNet, Ship,   0xDCF0F7F1, XXH32 }, // 18/01/2022 21:23:44
				{ { 1, 52,  3, 10923266 }, AIO, BattleNet, Ship,   0xB60B8280, XXH32 }, // 21/01/2022 20:05:56
				{ { 1, 53,  2, 11115544 }, AIO, BattleNet, Ship,   0x38682F92, XXH32 }, // 23/02/2022 21:31:40
				{ { 1, 54,  0, 11116736 }, AIO, BattleNet, Ship,   0x562E17C3, XXH32 }, // 23/02/2022 22:57:12
				{ { 1, 54,  1, 11155708 }, AIO, BattleNet, Ship,   0xDBFE2345, XXH32 }, // 02/03/2022 23:41:06
				{ { 1, 54,  4, 11222167 }, AIO, BattleNet, Ship,   0x6C534718, XXH32 }, // 15/03/2022 17:23:30
				{ { 1, 55,  0, 11227632 }, AIO, BattleNet, Ship,   0x84067E0D, XXH32 }, // 16/03/2022 02:19:49
				{ { 1, 55,  3, 11303845 }, AIO, BattleNet, Ship,   0xEB2C6AFE, XXH32 }, // 30/03/2022 20:58:55
				{ { 1, 55,  4, 11343887 }, AIO, BattleNet, Ship,   0x8723ACA6, XXH32 }, // 05/04/2022 19:40:57
				{ { 1, 55,  5, 11364056 }, AIO, BattleNet, Ship,   0x446345AC, XXH32 }, // 08/04/2022 17:31:30
				{ { 1, 55,  6, 11383760 }, AIO, BattleNet, Ship,   0x8C6C3695, XXH32 }, // 12/04/2022 23:16:54
				{ { 1, 56,  0, 11330025 }, AIO, BattleNet, Ship,   0x006C3FB7, XXH32 }, // 02/04/2022 00:53:56
				{ { 1, 56,  1, 11419693 }, AIO, BattleNet, Ship,   0xC35C418D, XXH32 }, // 20/04/2022 03:16:26
				{ { 1, 57,  0, 11399652 }, AIO, BattleNet, Ship,   0xB29A33B4, XXH32 }, // 15/04/2022 01:59:06
				{ { 1, 57,  1, 11464871 }, AIO, BattleNet, Ship,   0xB480D5F0, XXH32 }, // 28/04/2022 18:27:34
				{ { 1, 57,  2, 11494759 }, AIO, BattleNet, Ship,   0xF86ACAC9, XXH32 }, // 03/05/2022 17:41:51
				{ { 1, 57,  3, 11521872 }, AIO, BattleNet, Ship,   0xD0D0FDEF, XXH32 }, // 06/05/2022 20:41:20
				{ { 1, 57,  4, 11531527 }, AIO, BattleNet, Ship,   0xD224A3BC, XXH32 }, // 09/05/2022 19:58:16
				{ { 1, 57,  5, 11558592 }, AIO, BattleNet, Ship,   0x281DD7A4, XXH32 }, // 12/05/2022 20:14:00
				{ { 1, 57,  6, 11588296 }, AIO, BattleNet, Ship,   0xA0911285, XXH32 }, // 17/05/2022 21:14:48
				{ { 1, 58,  0, 11591958 }, AIO, BattleNet, Ship,   0x71F06292, XXH32 }, // 18/05/2022 03:31:20
				{ { 1, 58,  1, 11642448 }, AIO, BattleNet, Ship,   0x241A5DFF, XXH32 }, // 26/05/2022 18:17:09
				{ { 1, 58,  2, 11674731 }, AIO, BattleNet, Ship,   0xE94D9652, XXH32 }, // 31/05/2022 19:24:02
				{ { 1, 58,  3, 11691177 }, AIO, BattleNet, Ship,   0x1BA54A45, XXH32 }, // 02/06/2022 18:49:22
				{ { 1, 58,  4, 11715736 }, AIO, BattleNet, Ship,   0xF7DCB0BB, XXH32 }, // 07/06/2022 18:28:08
				{ { 1, 58,  5, 11751758 }, AIO, BattleNet, Ship,   0xC0183379, XXH32 }, // 13/06/2022 22:15:16
				{ { 1, 59,  0, 11744825 }, AIO, BattleNet, Ship,   0x9E6D7CFC, XXH32 }, // 11/06/2022 04:16:50
				{ { 1, 59,  1, 11810757 }, AIO, BattleNet, Ship,   0x95F50774, XXH32 }, // 22/06/2022 18:52:05
				{ { 1, 59,  2, 11846511 }, AIO, BattleNet, Ship,   0x09827BC4, XXH32 }, // 28/06/2022 18:38:09
				{ { 1, 59,  3, 11888139 }, AIO, BattleNet, Ship,   0xAFEBA1BC, XXH32 }, // 05/07/2022 23:40:07
				{ { 1, 59,  4, 11920874 }, AIO, BattleNet, Ship,   0x0A79A7D1, XXH32 }, // 11/07/2022 18:48:31
				{ { 1, 59,  5, 11948640 }, AIO, BattleNet, Ship,   0x2268EE2D, XXH32 }, // 14/07/2022 18:03:17
				{ { 1, 60,  0, 11967659 }, AIO, BattleNet, Ship,   0xB5788AEC, XXH32 }, // 19/07/2022 03:18:02
				{ { 1, 60,  1, 12047793 }, AIO, BattleNet, Ship,   0xDB593C22, XXH32 }, // 27/07/2022 19:07:32
				{ { 1, 61,  0, 12087051 }, AIO, BattleNet, Ship,   0x7733D630, XXH32 }, // 02/08/2022 01:55:52
				{ { 1, 61,  2, 12145825 }, AIO, BattleNet, Ship,   0xCFE5A69D, XXH32 }, // 09/08/2022 17:23:51
				{ { 1, 61,  3, 12181591 }, AIO, BattleNet, Ship,   0x707CFB10, XXH32 }, // 13/08/2022 01:29:19
				{ { 1, 62,  0, 12194754 }, AIO, BattleNet, Ship,   0x320182FB, XXH32 }, // 15/08/2022 17:05:26
				{ { 1, 62,  1, 12283188 }, AIO, BattleNet, Ship,   0x9E20A1AD, XXH32 }, // 25/08/2022 22:33:44
				{ { 1, 63,  0, 12273715 }, AIO, BattleNet, Ship,   0x5C11D1D9, XXH32 }, // 25/08/2022 00:05:45
				{ { 1, 63,  1, 12374094 }, AIO, BattleNet, Ship,   0x4CB1ED4B, XXH32 }, // 07/09/2022 02:56:15
				{ { 1, 63,  2, 12405035 }, AIO, BattleNet, Ship,   0x1ECC0463, XXH32 }, // 09/09/2022 21:04:01
				{ { 1, 63,  3, 12428344 }, AIO, BattleNet, Ship,   0x26BCEB4D, XXH32 }, // 13/09/2022 21:51:44
				{ { 1, 63,  4, 12484851 }, AIO, BattleNet, Ship,   0x826BDA50, XXH32 }, // 20/09/2022 17:35:51
				{ { 1, 64,  0, 12452741 }, AIO, BattleNet, Ship,   0x9DEA3C97, XXH32 }, // 16/09/2022 01:49:25
				{ { 1, 64,  1, 12552903 }, AIO, BattleNet, Ship,   0x52F88892, XXH32 }, // 28/09/2022 22:58:47
				{ { 1, 64,  2, 12658874 }, AIO, BattleNet, Ship,   0x173F94B3, XXH32 }, // 11/10/2022 18:59:40
				{ { 1, 64,  3, 12881783 }, AIO, BattleNet, Ship,   0x7E23CABE, XXH32 }, // 18/10/2022 18:06:24
				{ { 1, 64,  4, 13050304 }, AIO, BattleNet, Ship,   0x69EA09B7, XXH32 }, // 25/10/2022 19:58:51
				{ { 1, 64,  5, 13113888 }, AIO, BattleNet, Ship,   0xE70F67E6, XXH32 }, // 02/11/2022 18:36:06
				{ { 1, 65,  0, 13142214 }, AIO, BattleNet, Ship,   0xCC9C2F23, XXH32 }, // 04/11/2022 23:44:30
				{ { 1, 65,  1, 13273688 }, AIO, BattleNet, Ship,   0x876FFEF1, XXH32 }, // 28/11/2022 19:04:31
				{ { 1, 65,  2, 13356919 }, AIO, BattleNet, Ship,   0x001A561A, XXH32 }, // 06/12/2022 21:28:59
				{ { 1, 65,  3, 13395434 }, AIO, BattleNet, Ship,   0xCEA281AA, XXH32 }, // 12/12/2022 17:51:36
				{ { 1, 65,  4, 13402798 }, AIO, BattleNet, Ship,   0x34076D0E, XXH32 }, // 13/12/2022 18:49:10
				{ { 1, 65,  8, 13668260 }, AIO, BattleNet, Ship,   0x61B61D7F, XXH32 }, // 02/02/2023 21:46:59
				{ { 1, 65,  9, 13789124 }, AIO, BattleNet, Ship,   0xCB815841, XXH32 }, // 14/02/2023 00:53:36
				{ { 1, 65, 10, 13848720 }, AIO, BattleNet, Ship,   0x889D6BDE, XXH32 }, // 20/02/2023 19:16:49
				{ { 1, 66,  0, 13858333 }, AIO, BattleNet, Ship,   0x86942028, XXH32 }, // 21/02/2023 23:38:12
				{ { 1, 66,  0, 13858333 }, AIO, Steam,     Ship,   0x68561673, XXH32 }, // 28/02/2023 01:48:10
				{ { 1, 66,  0, 13858333 }, AIO, Steam,     Ship,   0xD883E82A, XXH32 }, // 28/02/2023 01:48:10
				{ { 1, 66,  1, 14023464 }, AIO, BattleNet, Ship,   0xD7E4BE32, XXH32 }, // 17/03/2023 00:03:46
				{ { 1, 66,  1, 14023464 }, AIO, Steam,     Ship,   0x33111734, XXH32 }, // 17/03/2023 00:10:20
				{ { 1, 66,  3, 14228804 }, AIO, BattleNet, Ship,   0x39D59933, XXH32 }, // 05/04/2023 22:30:41
				{ { 1, 66,  3, 14228804 }, AIO, Steam,     Ship,   0x670C0C0F, XXH32 }, // 05/04/2023 22:33:07
				{ { 1, 66,  4, 14299935 }, AIO, BattleNet, Ship,   0x3447D188, XXH32 }, // 11/04/2023 18:31:03
				{ { 1, 66,  4, 14299935 }, AIO, Steam,     Ship,   0x492E80C6, XXH32 }, // 11/04/2023 18:26:46
				{ { 1, 66,  5, 14360630 }, AIO, BattleNet, Ship,   0x8A1A90D3, XXH32 }, // 19/04/2023 01:33:28
				{ { 1, 66,  5, 14360630 }, AIO, Steam,     Ship,   0x424D509F, XXH32 }, // 19/04/2023 01:44:13
				{ { 1, 66,  7, 14580756 }, AIO, BattleNet, Ship,   0x4D467B5C, XXH32 }, // 10/05/2023 00:55:52
				{ { 1, 66,  7, 14580756 }, AIO, Steam,     Ship,   0x0F79312D, XXH32 }, // 10/05/2023 01:01:55
				{ { 1, 66,  8, 14633877 }, AIO, BattleNet, Ship,   0x8C44C93B, XXH32 }, // 15/05/2023 22:28:44
				{ { 1, 66,  8, 14633877 }, AIO, Steam,     Ship,   0x17E2591D, XXH32 }, // 15/05/2023 22:21:57
				{ { 1, 66,  9, 14699633 }, AIO, BattleNet, Ship,   0x9C127D9B, XXH32 }, // 22/05/2023 18:06:28
				{ { 1, 66,  9, 14699633 }, AIO, Steam,     Ship,   0x9316CBAC, XXH32 }, // 22/05/2023 17:53:33
				{ { 1, 66, 10, 14843327 }, AIO, BattleNet, Ship,   0x785BE218, XXH32 }, // 05/06/2023 21:52:47
				{ { 1, 66, 10, 14843327 }, AIO, Steam,     Ship,   0xB11D8C16, XXH32 }, // 05/06/2023 21:25:54
				{ { 1, 66, 11, 14910993 }, AIO, BattleNet, Ship,   0x254672B4, XXH32 }, // 12/06/2023 18:27:09
				{ { 1, 66, 11, 14910993 }, AIO, Steam,     Ship,   0x5F60E2DD, XXH32 }, // 12/06/2023 18:24:05
				{ { 1, 66, 13, 15079557 }, AIO, BattleNet, Ship,   0x6355BE57, XXH32 }, // 29/06/2023 23:15:33
				{ { 1, 66, 13, 15079557 }, AIO, Steam,     Ship,   0x684AEB60, XXH32 }, // 29/06/2023 23:07:49
				{ { 1, 66, 14, 15119212 }, AIO, BattleNet, Ship,   0x0275BCD9, XXH32 }, // 05/07/2023 18:04:03
				{ { 1, 66, 14, 15119212 }, AIO, Steam,     Ship,   0xB4927F8E, XXH32 }, // 05/07/2023 18:06:22
				{ { 1, 66, 15, 15244192 }, AIO, BattleNet, Ship,   0x3A92C1D6, XXH32 }, // 17/07/2023 22:24:26
				{ { 1, 66, 15, 15244192 }, AIO, Steam,     Ship,   0xC58B195A, XXH32 }, // 17/07/2023 22:40:40
				{ { 1, 66, 16, 15321234 }, AIO, BattleNet, Ship,   0x6DE0ACA4, XXH32 }, // 24/07/2023 19:48:34
				{ { 1, 66, 16, 15321234 }, AIO, Steam,     Ship,   0x3CEFFF67, XXH32 }, // 24/07/2023 19:40:13
				{ { 1, 66, 17, 15473234 }, AIO, BattleNet, Ship,   0x8EAEB299, XXH32 }, // 31/07/2023 21:00:59
				{ { 1, 66, 17, 15473234 }, AIO, Steam,     Ship,   0xDC8E5985, XXH32 }, // 31/07/2023 21:52:00
				{ { 1, 66, 18, 15546934 }, AIO, BattleNet, Ship,   0x24D61034, XXH32 }, // 07/08/2023 18:24:45
				{ { 1, 66, 18, 15546934 }, AIO, Steam,     Ship,   0xCBCF7025, XXH32 }, // 07/08/2023 18:17:33
				{ { 1, 66, 19, 15629918 }, AIO, BattleNet, Ship,   0x5F1E9939, XXH32 }, // 14/08/2023 20:07:39
				{ { 1, 66, 19, 15629918 }, AIO, Steam,     Ship,   0x3F9D890C, XXH32 }, // 14/08/2023 20:02:41
				{ { 1, 66, 20, 15708959 }, AIO, BattleNet, Ship,   0x9BBD0BA4, XXH32 }, // 21/08/2023 20:05:08
				{ { 1, 66, 20, 15708959 }, AIO, Steam,     Ship,   0xACA6F0B1, XXH32 }, // 21/08/2023 19:50:27
				{ { 1, 66, 21, 15791281 }, AIO, BattleNet, Ship,   0x3FB36872, XXH32 }, // 29/08/2023 01:25:32
				{ { 1, 66, 21, 15791281 }, AIO, Steam,     Ship,   0xFE6FDFDE, XXH32 }, // 29/08/2023 01:05:28
				{ { 1, 66, 22, 15889661 }, AIO, BattleNet, Ship,   0x9524AEBA, XXH32 }, // 06/09/2023 16:33:21
				{ { 1, 66, 22, 15889661 }, AIO, Steam,     Ship,   0x6F85739C, XXH32 }, // 06/09/2023 16:26:41
				{ { 1, 66, 23, 15947459 }, AIO, BattleNet, Ship,   0x18981517, XXH32 }, // 11/09/2023 19:48:00
				{ { 1, 66, 23, 15947459 }, AIO, Steam,     Ship,   0x2879565C, XXH32 }, // 11/09/2023 21:05:53
				{ { 1, 67,  0, 15967738 }, AIO, BattleNet, Ship,   0x2EAD7337, XXH32 }, // 13/09/2023 01:19:22
				{ { 1, 67,  0, 15967738 }, AIO, Steam,     Ship,   0x76E39A40, XXH32 }, // 13/09/2023 02:50:11
				{ { 1, 67,  1, 17008517 }, AIO, BattleNet, Ship,   0xC0293F73, XXH32 }, // 16/01/2024 20:40:23
				{ { 1, 67,  1, 17008517 }, AIO, Steam,     Ship,   0xF76F8574, XXH32 }, // 16/01/2024 20:27:01
				{ { 1, 67,  2, 17008517 }, AIO, Steam,     Ship,   0xB0F0FE76, XXH32 }, // 11/04/2024 21:22:39
				{ { 1, 67,  3, 18290054 }, AIO, BattleNet, Ship,   0x5959AD84, XXH32 }, // 13/05/2024 21:53:26
				{ { 1, 67,  3, 18290054 }, AIO, Steam,     Ship,   0x48D61B3B, XXH32 }, // 13/05/2024 20:28:59
				{ { 1, 68,  0, 26168382 }, AIO, BattleNet, Ship,   0x90C3DB89, XXH32 }, // 09/03/2026 14:05:46
				{ { 1, 68,  0, 26168382 }, AIO, MsStore,   Ship,   0x24616D7E, XXH32 }, // 09/03/2026 15:00:54
				{ { 1, 68,  0, 26168382 }, AIO, Steam,     Ship,   0x7B8AF418, XXH32 }, // 09/03/2026 15:35:21
			}
		};
	}
}

DEF_GAME_IMPL_SPECIFIC()
