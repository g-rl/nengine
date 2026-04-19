#include <std_include.hpp>
#include "game.hpp"

#include <xxhash32.h>

#include <format>
#include <fstream>
#include <sstream>
#include <vector>

namespace identification::game
{
	std::uint32_t get_game_xxh32_checksum()
	{
		static std::uint32_t cached = []() -> std::uint32_t
		{
			utils::nt::library self{};
			std::ifstream file(self.get_path(), std::ios::binary);
			if (!file)
			{
				return 0;
			}

			XXHash32 hasher(0);

			constexpr std::size_t streaming_buffer_size = 4 * (1024 * 1024);
			std::vector<char> buffer(streaming_buffer_size);

			while (file)
			{
				file.read(buffer.data(), streaming_buffer_size);
				const std::streamsize bytes_read = file.gcount();
				if (bytes_read > 0)
				{
					hasher.add(buffer.data(), static_cast<uint64_t>(bytes_read));
				}
			}

			return hasher.hash();
		}();
		return cached;
	}

	std::uint32_t get_game_nt_timestamp()
	{
		static std::uint32_t timestamp = []() -> std::uint32_t
		{
			utils::nt::library self{};
			const auto headers = self.get_nt_headers();
			return headers ? headers->FileHeader.TimeDateStamp : 0u;
		}();
		return timestamp;
	}

	std::uint32_t get_target_checksum(iden_game_checksum_type type)
	{
		switch (type)
		{
		case iden_game_checksum_type::XXH32:
			return get_game_xxh32_checksum();
		case iden_game_checksum_type::NtTimestamp:
			return get_game_nt_timestamp();
		case iden_game_checksum_type::ImplSpecific:
			return get_game_impl_specific();
		default:
			return UINT32_MAX;
		}
	}

	std::uint32_t get_game_impl_specific()
	{
		return UINT32_MAX;
	}

	static game_iden_target resolve_target()
	{
		const game_iden idens[] = { get_iw8_game_iden(), get_iw9_game_iden(), get_s4_game_iden()};
		game_iden_target target{};

		for (const auto& iden : idens)
		{
			for (const auto& it : iden.game_sets)
			{
				if (get_target_checksum(it.game_checksum_type) == it.game_hash)
				{
					target.client_name = iden.client_name;
					target.game_name = iden.game_name;
					target.game_version = it.game_version;
					target.game_mode = it.game_mode;
					target.game_platform = it.game_platform;
					target.game_config = it.game_config;
					target.game_hash = it.game_hash;
					target.game_checksum_type = it.game_checksum_type;
					return target;
				}
			}
		}

		// no match found — print the actual checksum so it can be added to the database
		char buf[64];
		_snprintf_s(buf, _TRUNCATE, "[iden] unknown exe: xxh32=0x%08X nt_ts=0x%08X\n",
			get_game_xxh32_checksum(), get_game_nt_timestamp());
		printf("%s", buf);

		return target;
	}

	game_iden_target get_target_game()
	{
		static game_iden_target target = resolve_target();
		return target;
	}

	std::string get_full_display_name()
	{
		static auto target = get_target_game();

		const std::string version = std::format(" v{}.{:02}.{}.{}",
			target.game_version[0], target.game_version[1], target.game_version[2], target.game_version[3]);

		std::string optional_config;
		if (target.game_config != iden_game_config::Ship)
		{
			std::string name;
			switch (target.game_config)
			{
			case Replay: name = "Replay Feature Branch"; break;
			case Beta:   name = "Beta"; break;
			case Alpha:  name = "Alpha"; break;
			default:     name = "Unknown"; break;
			}
			optional_config = std::format(" [{}]", name);
		}

		std::string optional_mode;
		if (target.game_mode != iden_game_modes::AIO)
		{
			std::string name;
			switch (target.game_mode)
			{
			case iden_game_modes::MP:  name = "Multiplayer"; break;
			case iden_game_modes::SP:  name = "Singleplayer"; break;
			case iden_game_modes::ZM:  name = "Zombies"; break;
			case iden_game_modes::CDL: name = "E-Sports"; break;
			default:                   name = "Unknown"; break;
			}
			optional_mode = std::format(" ({})", name);
		}

		return std::format("Call of Duty®: {}{}{}{}",
			target.game_name, version, optional_config, optional_mode);
	}

	std::string get_version(bool show_delta)
	{
		static auto target = get_target_game();

		std::string config;
		if (target.game_config != iden_game_config::Ship)
		{
			switch (target.game_config)
			{
			case Replay: config = "-replay"; break;
			case Beta:   config = "-beta"; break;
			case Alpha:  config = "-alpha"; break;
			default:     config = ""; break;
			}
		}

		std::string mode;
		if (!show_delta)
		{
			switch (target.game_mode)
			{
			case iden_game_modes::MP:  mode = "-MP"; break;
			case iden_game_modes::SP:  mode = "-SP"; break;
			case iden_game_modes::ZM:  mode = "-ZM"; break;
			case iden_game_modes::CDL: mode = "-ES"; break;
			default: break;
			}
		}

		if (show_delta)
		{
			return std::format("{}.{:02}.{}.{}{}",
				target.game_version[0], target.game_version[1], target.game_version[2], target.game_version[3], config);
		}

		return std::format("{}.{:02}.{}{}{}",
			target.game_version[0], target.game_version[1], target.game_version[2], config, mode);
	}

	std::tuple<int, int, int> parse_str_version(const std::string& version)
	{
		std::string ver = version;
		if (ver.ends_with("-replay"))
		{
			ver = ver.substr(0, ver.size() - 7);
		}
		if (ver.ends_with("-beta"))
		{
			ver = ver.substr(0, ver.size() - 5);
		}
		if (ver.ends_with("-alpha"))
		{
			ver = ver.substr(0, ver.size() - 6);
		}

		std::istringstream iss(ver);
		std::string token;
		int major = 0, minor = 0, patch = 0;

		if (std::getline(iss, token, '.')) major = std::stoi(token);
		if (std::getline(iss, token, '.')) minor = std::stoi(token);
		if (std::getline(iss, token, '.')) patch = std::stoi(token);

		return { major, minor, patch };
	}
}
