#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include "platform.hpp"
#include "scheduler.hpp"

#include <utils/cryptography.hpp>
#include <utils/hook.hpp>
#include <utils/nt.hpp>

namespace platform
{
	std::uint64_t bnet_get_user_id()
	{
		static std::uint32_t default_xuid = utils::cryptography::xxh32::compute(utils::nt::get_login_username());
		return default_xuid;
	}

	const char* bnet_get_username()
	{
		static std::string default_name = utils::nt::get_login_username();
		return default_name.data();
	}

	std::string get_userdata_directory()
	{
		return std::format("iw8-mod/bnet-{}", bnet_get_user_id());
	}

	std::uintptr_t bnet_get_class()
	{
		return utils::hook::invoke<std::uintptr_t>(0x1660280_b);
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			auto bnet_class = bnet_get_class();

			*reinterpret_cast<BYTE*>(bnet_class + 720)	= 1; // cross auth check
			*reinterpret_cast<DWORD*>(bnet_class + 756) = 0x795230F0;
			*reinterpret_cast<BYTE*>(bnet_class + 760)	= 31;
			*reinterpret_cast<DWORD*>(bnet_class + 764) = 0;

			//utils::hook::jump(0x13FD550_b, bnet_get_user_id);
			utils::hook::jump(0x13FD5E0_b, bnet_get_user_id);
		}
	};
}

//REGISTER_COMPONENT(platform::component)
