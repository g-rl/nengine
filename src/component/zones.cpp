#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/memory.hpp>

namespace zones
{
	namespace
	{
		constexpr unsigned int IW8_1_19_XFILE_VERSION	= 4085;
		constexpr unsigned int XFILE_VERSION			= 4087;

		unsigned int current_zone_version = 0;

		utils::memory::allocator asset_allocator;

		// TODO
		//std::unordered_map<void*, void*> asset_relocations;
		
		void set_current_zone_version(unsigned int version)
		{
			// TODO: function for resetting asset allocations
			current_zone_version = version;
		}

		utils::hook::detour check_xfile_version_hook;
		unsigned int check_xfile_version_stub(const game::DB_FFHeader* header)
		{
			set_current_zone_version(header->xfileVersion);

			if (current_zone_version == IW8_1_19_XFILE_VERSION) // mp_m_cargo is 4085 (1.19.3.7547737_pc)
			{
				return IW8_1_19_XFILE_VERSION; // expects 4085 to load, so we're just gonna force it to pass the checks
			}

			return check_xfile_version_hook.invoke<unsigned int>(header);
		}

		bool bdiff_stub()
		{
			return true;
		}

		void weapondef_load_stream_stuib(const int streamStart, void* ptr, std::uint64_t size)
		{
			if (current_zone_version <= IW8_1_19_XFILE_VERSION)
			{
				size -= 24; // 24 byte difference from 4085 -> 4087
			}

			game::Load_Stream(streamStart, ptr, size);

			if (current_zone_version == IW8_1_19_XFILE_VERSION)
			{
				game::iw8_1_19::WeaponDef* varWeaponDef = *reinterpret_cast<game::iw8_1_19::WeaponDef**>(0x5D404A8_b);

				auto new_weapon_def_var = static_cast<game::WeaponDef*>(asset_allocator.allocate(sizeof(game::WeaponDef)));

				new_weapon_def_var->szOverlayName = varWeaponDef->szOverlayName;	// 0
				memcpy(new_weapon_def_var->__pad0, varWeaponDef->__pad0, 112);		// 8
				new_weapon_def_var->playerShadowModel = nullptr;					// 120
				new_weapon_def_var->playerShadowModelLeftHand = nullptr;			// 128
				new_weapon_def_var->playerShadowModelRightHand = nullptr;			// 136
				new_weapon_def_var->szXAnims = varWeaponDef->szXAnims;				// 144
				new_weapon_def_var->szXAnimsRightHanded = varWeaponDef->szXAnimsRightHanded;	// 152
				new_weapon_def_var->szXAnimsLeftHanded = varWeaponDef->szXAnimsLeftHanded;		// 160
				memcpy(new_weapon_def_var->__pad1, varWeaponDef->__pad1, 5128);		// 168

				size += 24; // fixed struct size
				memcpy(ptr, new_weapon_def_var, size);
				asset_allocator.clear();
				//game::Load_Stream(streamStart, ptr, size);
			}
		}

		void gfxworld_load_stream_stub(const int streamStart, void* ptr, std::uint64_t size)
		{
			if (current_zone_version <= IW8_1_19_XFILE_VERSION)
			{
				size -= 32; // 32 byte difference from 4085 -> 4087
			}

			game::Load_Stream(streamStart, ptr, size);

			if (current_zone_version == IW8_1_19_XFILE_VERSION)
			{
				game::iw8_1_19::GfxWorld* varGfxWorld = *reinterpret_cast<game::iw8_1_19::GfxWorld**>(0x5D40D00_b);

				auto new_gfx_world_var = static_cast<game::GfxWorld*>(asset_allocator.allocate(sizeof(game::GfxWorld)));
				new_gfx_world_var->name = varGfxWorld->name;
				new_gfx_world_var->baseName = varGfxWorld->baseName;
			}
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			//utils::hook::nop(0x12AFB62_b, 0x11); // init for CASC
			//utils::hook::set<byte>(get_pattern("E8 ? ? ? ? 80 78 08 00 ? 27", 9), 0xEB); // TODO: disable Bink reading from CASC

			check_xfile_version_hook.create(0xD8A180_b, check_xfile_version_stub);

			utils::hook::nop(0xD89091_b, 5); // prevent Dirty disk error from occuring on bad assets (remove)

			// TODO: Postload_WeaponDef

			// WeaponDef
			utils::hook::call(0xDB647F_b, weapondef_load_stream_stuib); // Preload_WeaponDef
			utils::hook::call(0xD9A5A1_b, weapondef_load_stream_stuib); // Load_WeaponDef

			// GfxWorld
			//utils::hook::call(0xDAF4F8_b, gfxworld_load_stream_stub); // Preload_GfxWorld
		}
	};
}

REGISTER_COMPONENT(zones::component)
