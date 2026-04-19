#pragma once

#include <utils/nt.hpp>

#include <array>
#include <string>
#include <tuple>
#include <vector>

namespace identification::game
{
	enum iden_game_modes : std::uint8_t
	{
		SP, MP, ZM, AIO, CDL
	};

	enum iden_game_platform : std::uint8_t
	{
		Standalone, BattleNet, Steam, MsStore,
	};

	enum iden_game_config : std::uint8_t
	{
		Ship, Replay, Beta, Alpha
	};

	enum iden_game_checksum_type : std::uint8_t
	{
		XXH32, NtTimestamp, ImplSpecific
	};

	struct game_iden_set
	{
		std::array<int, 4> game_version;
		iden_game_modes game_mode;
		iden_game_platform game_platform;
		iden_game_config game_config;
		std::uint32_t game_hash;
		iden_game_checksum_type game_checksum_type;
	};

	struct game_iden
	{
		std::string client_name;
		std::string game_name;
		std::vector<game_iden_set> game_sets;
	};

	struct game_iden_target : public game_iden_set
	{
		std::string client_name;
		std::string game_name;
	};

	game_iden get_iw8_game_iden();
	game_iden get_iw9_game_iden();
	game_iden get_s4_game_iden();

	std::uint32_t get_game_xxh32_checksum();
	std::uint32_t get_game_nt_timestamp();
	std::uint32_t get_game_impl_specific();

#define DEF_GAME_IMPL_SPECIFIC() \
	std::uint32_t identification::game::get_game_impl_specific() { return UINT32_MAX; }

	std::uint32_t get_target_checksum(iden_game_checksum_type type);

	game_iden_target get_target_game();

	std::string get_full_display_name();
	std::string get_version(bool show_delta = true);

	std::tuple<int, int, int> parse_str_version(const std::string& version);

	inline bool is_less_or_eq(const std::string& str)
	{
		static game_iden_target g = get_target_game();
		static auto target_version = std::make_tuple(g.game_version[0], g.game_version[1], g.game_version[2]);
		return target_version <= parse_str_version(str);
	}

	inline bool is_greater_or_eq(const std::string& str)
	{
		static game_iden_target g = get_target_game();
		static auto target_version = std::make_tuple(g.game_version[0], g.game_version[1], g.game_version[2]);
		return target_version >= parse_str_version(str);
	}

	inline bool is_greater(const std::string& str)
	{
		static game_iden_target g = get_target_game();
		static auto target_version = std::make_tuple(g.game_version[0], g.game_version[1], g.game_version[2]);
		return target_version > parse_str_version(str);
	}

	inline bool is_less(const std::string& str)
	{
		static game_iden_target g = get_target_game();
		static auto target_version = std::make_tuple(g.game_version[0], g.game_version[1], g.game_version[2]);
		return target_version < parse_str_version(str);
	}

	inline bool is_in_range(const std::string& min_version, const std::string& max_version)
	{
		return is_greater_or_eq(min_version) && is_less(max_version);
	}

	template <typename... Args>
	inline bool is(Args... args)
	{
		return ((std::string(args) == get_version(false)) || ...);
	}

	inline bool is_target_game()
	{
		return get_target_game().game_hash != 0x0;
	}

	inline bool is_mode(const iden_game_modes mode)
	{
		return get_target_game().game_mode == mode;
	}

	inline bool is_config(const iden_game_config config)
	{
		return get_target_game().game_config == config;
	}

	inline bool is_platform(const iden_game_platform plat)
	{
		return get_target_game().game_platform == plat;
	}
}
