#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/memory.hpp>

// TODO: this is NOT a solution lol
//#define APE_SHIT_MODE

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

#define COPY_NEW_VALUE(new_value_ptr, old_value_ptr) new_value_ptr = old_value_ptr;

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

				COPY_NEW_VALUE(new_weapon_def_var->szOverlayName, varWeaponDef->szOverlayName)	// 0
				memcpy(new_weapon_def_var->__pad0, varWeaponDef->__pad0, 112);					// 8
				COPY_NEW_VALUE(new_weapon_def_var->playerShadowModel, nullptr)					// 120
				COPY_NEW_VALUE(new_weapon_def_var->playerShadowModelLeftHand, nullptr)			// 128
				COPY_NEW_VALUE(new_weapon_def_var->playerShadowModelRightHand, nullptr)			// 136
				COPY_NEW_VALUE(new_weapon_def_var->szXAnims, varWeaponDef->szXAnims)			// 144
				COPY_NEW_VALUE(new_weapon_def_var->szXAnimsRightHanded, varWeaponDef->szXAnimsRightHanded)	// 152
				COPY_NEW_VALUE(new_weapon_def_var->szXAnimsLeftHanded, varWeaponDef->szXAnimsLeftHanded)	// 160
				memcpy(new_weapon_def_var->__pad1, varWeaponDef->__pad1, 5128);					// 168

				size += 24; // actual struct size
				memcpy(ptr, new_weapon_def_var, size);
				asset_allocator.clear();
				//game::Load_Stream(streamStart, ptr, size);
			}
		}

		void gfxworld_load_stream_stub(const int streamStart, void* ptr, std::uint64_t size)
		{
			if (current_zone_version <= IW8_1_19_XFILE_VERSION)
			{
				size -= 32; // 32 byte difference in GfxWorld::frustumLights from 4085 -> 4087
			}

			game::Load_Stream(streamStart, ptr, size);

			if (current_zone_version == IW8_1_19_XFILE_VERSION)
			{
				game::iw8_1_19::GfxWorld* varGfxWorld = *reinterpret_cast<game::iw8_1_19::GfxWorld**>(0x5D40D00_b);

				auto new_gfx_world_var = static_cast<game::GfxWorld*>(asset_allocator.allocate(sizeof(game::GfxWorld)));

				COPY_NEW_VALUE(new_gfx_world_var->name, varGfxWorld->name)				// 0
				COPY_NEW_VALUE(new_gfx_world_var->baseName, varGfxWorld->baseName)		// 8
				COPY_NEW_VALUE(new_gfx_world_var->bspVersion, varGfxWorld->bspVersion)	// 16
				memcpy(new_gfx_world_var->__pad0, varGfxWorld->__pad0, 14460);			// 20
				memcpy(new_gfx_world_var->dynamicLightset, varGfxWorld->dynamicLightset, 928); // 14480
				memcpy(new_gfx_world_var->mayhemSelfVis, varGfxWorld->mayhemSelfVis, 112); // 15408

				// TODO: fix up frustumLights??????
				/*
				memcpy(new_gfx_world_var->frustumLights.__pad0, varGfxWorld->frustumLights.__pad0, 64); // 15520

				// this is NOT the right way to do it, i know. spare me
				new_gfx_world_var->frustumLights.indexBuffer.buffer = nullptr; // 15584
				strcpy_s(new_gfx_world_var->frustumLights.indexBuffer.view, 48, ""); // 15592
				new_gfx_world_var->frustumLights.indexBuffer.data = nullptr; // 15640
				// 

				COPY_NEW_VALUE(new_gfx_world_var->frustumLights.vertexBuffer, varGfxWorld->frustumLights.vertexBuffer);
				*/

#ifdef APE_SHIT_MODE
				memcpy(new_gfx_world_var->frustumLights.__pad0, varGfxWorld->frustumLights.__pad0, 64);

				game::GfxWrappedBuffer blank_buffer{};
				new_gfx_world_var->frustumLights.indexBuffer = blank_buffer;
				new_gfx_world_var->frustumLights.vertexBuffer = varGfxWorld->frustumLights.vertexBuffer;
#endif

#ifdef APE_SHIT_MODE
				new_gfx_world_var->lightViewFrustums = nullptr;
				new_gfx_world_var->primaryLights = nullptr;
				new_gfx_world_var->voxelTreeCount = 0;
				new_gfx_world_var->voxelTree = nullptr;
#else
				memcpy(new_gfx_world_var->lightViewFrustums, varGfxWorld->lightViewFrustums, 8);
				memcpy(new_gfx_world_var->primaryLights, varGfxWorld->primaryLights, 8);
				COPY_NEW_VALUE(new_gfx_world_var->voxelTreeCount, varGfxWorld->voxelTreeCount)
				memcpy(new_gfx_world_var->voxelTree, varGfxWorld->voxelTree, 8);
#endif

				memcpy(new_gfx_world_var->__pad1, varGfxWorld->__pad1, 2064);

				size += 32; // actual struct size
				memcpy(ptr, new_gfx_world_var, size);
				asset_allocator.clear();
				//game::Load_Stream(streamStart, ptr, size);
			}
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			check_xfile_version_hook.create(0xD8A180_b, check_xfile_version_stub);

			//utils::hook::nop(0xD89091_b, 5); // prevent Dirty disk error from occuring on bad assets (kj)

			// WeaponDef
			// TODO: Postload_WeaponDef
			utils::hook::call(0xDB647F_b, weapondef_load_stream_stuib); // Preload_WeaponDef
			utils::hook::call(0xD9A5A1_b, weapondef_load_stream_stuib); // Load_WeaponDef

			// GfxWorld
			utils::hook::call(0xDAF4F8_b, gfxworld_load_stream_stub); // Preload_GfxWorld
		}
	};
}

// TODO: do this component in the future maybe lmfao
#ifdef APE_SHIT_MODE
//REGISTER_COMPONENT(zones::component)
#endif
