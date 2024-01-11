#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>

namespace zones
{
	namespace
	{
		constexpr auto XFILE_VERSION = 4087;
		unsigned int current_xfile_version = 0;

		// TODO
		//std::unordered_map<void*, void*> asset_relocations;
		
		int set_current_zone_version(unsigned int version)
		{
			// TODO: function for resetting asset allocations
			current_xfile_version = version;
		}

		utils::hook::detour check_xfile_version_hook;
		unsigned int check_xfile_version_stub(const game::DB_FFHeader* header)
		{
			set_current_zone_version(header->xfileVersion);

			if (current_xfile_version == 4085) // mp_m_pine is 4085 (1.19.3.7547737_pc)
			{
				return XFILE_VERSION; // expects XFILE_VERSION to load, so we're just gonna force it to pass the checks
			}

			return check_xfile_version_hook.invoke<unsigned int>(header);
		}

		int get_zone_version()
		{
			
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			//utils::hook::nop(0x12AFB62_b, 0x11); // init for CASC
			//utils::hook::set<byte>(get_pattern("E8 ? ? ? ? 80 78 08 00 ? 27", 9), 0xEB); // TODO: disable Bink reading from CASC

			//check_xfile_version_hook.create(0xD8A180_b, check_xfile_version_stub);
		}
	};
}

//REGISTER_COMPONENT(zones::component)
