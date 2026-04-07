#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <identification/game.hpp>

#include "scheduler.hpp"

#include <utils/hook.hpp>

namespace weapon
{
	game::dvar_t* sprint_swaps_dvar = nullptr;
	game::dvar_t* instashoots_dvar = nullptr;
	game::dvar_t* always_canswap_dvar = nullptr;
	game::dvar_t* freeze_anim_dvar = nullptr;
	game::dvar_t* canzooms_dvar = nullptr;
	game::dvar_t* always_altswap_dvar = nullptr;

	std::uint32_t pmove_weaponMap_offset  = 0x380; // default: 1.20
	std::uint32_t pmove_weapState_offset  = 0x50C; // default: 1.20
	std::uint32_t ps_sprintState_offset   = 0x31C; // default: 1.20

	void* get_weapon_map(game::pmove_t* pm)
	{
		return *reinterpret_cast<void**>(reinterpret_cast<std::uint8_t*>(pm) + pmove_weaponMap_offset);
	}

	game::SprintState* get_sprint_state_ptr(game::pmove_t* pm)
	{
		return reinterpret_cast<game::SprintState*>(reinterpret_cast<std::uint8_t*>(pm->ps) + ps_sprintState_offset);
	}

	game::PlayerActiveWeaponState get_weap_state(game::pmove_t* pm, int index)
	{
		return *reinterpret_cast<game::PlayerActiveWeaponState*>(reinterpret_cast<std::uint8_t*>(pm->ps) + pmove_weapState_offset + (index * sizeof(game::PlayerActiveWeaponState)));
	}

	game::PlayerActiveWeaponState* get_weap_state_ptr(game::pmove_t* pm, int index)
	{
		return reinterpret_cast<game::PlayerActiveWeaponState*>(reinterpret_cast<std::uint8_t*>(pm->ps) + pmove_weapState_offset + (index * sizeof(game::PlayerActiveWeaponState)));
	}

	// this is so so so so extremely ugly, but i think it works sadly :p
	// too lazy to sig this rn but i think its always the same lmfao
	game::PlayerEquippedWeaponState* get_weap_equipped_data_ptr(game::pmove_t* pm, int index)
	{
		//printf("get_weap_equipped_data_ptr\n");
		constexpr auto offset = (2 * sizeof(game::PlayerActiveWeaponState)) + (15 * sizeof(game::BgWeaponHandle));
		return reinterpret_cast<game::PlayerEquippedWeaponState*>(reinterpret_cast<std::uint8_t*>(pm->ps) + pmove_weapState_offset + offset + (index * sizeof(game::PlayerEquippedWeaponState)));
	}

	/*
		ok now actual code :D
	*/

	utils::hook::detour PM_BeginWeaponChange_hook;
	void PM_BeginWeaponChange_stub(game::pmove_t* pm, game::pml_t* pml,
		const game::Weapon* newweapon, bool isNewAlternate, bool quick)
	{
		if (game::dvar_is_enabled_safe(always_canswap_dvar))
		{
			quick = true;
		}

		//printf("beginweaponchange 1\n");

		if (!game::dvar_is_enabled_safe(sprint_swaps_dvar))
		{
			//printf("its off bruh\n");
			utils::hook::spoof_hook_invoke<void>(PM_BeginWeaponChange_hook, pm, pml, newweapon, isNewAlternate, quick);
			return;
		}

		game::PlayerActiveWeaponState prevWeapState[2] = {
			*get_weap_state_ptr(pm, 0),
			*get_weap_state_ptr(pm, 1)
		};

		utils::hook::spoof_hook_invoke<void>(PM_BeginWeaponChange_hook, pm, pml, newweapon, isNewAlternate, quick);

		const auto* sprint_state = get_sprint_state_ptr(pm);
		const bool isSprinting = sprint_state->lastSprintStart
			&& sprint_state->lastSprintStart > sprint_state->lastSprintEnd;

		if (isSprinting)
		{
			for (int i = 0; i < 2; i++)
			{
				auto weap_state = get_weap_state_ptr(pm, i);
				weap_state->weapAnim = prevWeapState[i].weapAnim;
				weap_state->prevWeapAnim = prevWeapState[i].prevWeapAnim;
			}
		}
	}

	void instashoots_check(game::pmove_t* pm, int hand)
	{
		if (!game::dvar_is_enabled_safe(instashoots_dvar))
			return;

		int state = get_weap_state_ptr(pm, hand)->weaponState;
		if (state != game::WEAPON_RAISING && state != game::WEAPON_RAISING_ALTSWITCH)
			return;

		const game::Weapon* currentWeapon = game::BG_GetCurrentWeaponForPlayer_sig(
			get_weapon_map(pm), pm->ps);

		uint64_t fireButton = game::PM_GetWeaponFireButton_sig(
			pm, currentWeapon, hand, pm->cmd.inputFromGamepad);

		if (pm->cmd.buttons & fireButton)
		{
			int lastHand = game::BG_PlayerLastWeaponHand_sig(get_weapon_map(pm), pm->ps);
			for (int i = 0; i <= lastHand; i++)
				game::PM_Weapon_Idle_sig(pm, i);
		}
	}

	utils::hook::detour PM_Weapon_ProcessHand_hook;
	void PM_Weapon_ProcessHand_stub(game::pmove_t* pm, game::pml_t* pml,
		int delayedAction, int hand)
	{
		//printf("PM_Weapon_ProcessHand_stub\n");
		instashoots_check(pm, hand);
		//printf("PM_Weapon_ProcessHand_stub end 1\n");
		PM_Weapon_ProcessHand_hook.invoke<void>(pm, pml, delayedAction, hand);
		//printf("PM_Weapon_ProcessHand_stub end final\n");
	}

	void canzooms_check(game::pmove_t* pm)
	{
		if (!game::dvar_is_enabled_safe(canzooms_dvar))
			return;

		auto weap_state = get_weap_state_ptr(pm, 0);

		if (weap_state->weaponState != game::WEAPON_RAISING)
			return;

		const game::Weapon* currentWeapon = game::BG_GetCurrentWeaponForPlayer_sig(get_weapon_map(pm), pm->ps);
		const bool dualWielding = game::BG_PlayerDualWieldingWeapon(get_weapon_map(pm), pm->ps, currentWeapon);
		if (dualWielding)
			return;

		unsigned int zoomButton = 0x20000;
		if (pm->cmd.buttons & zoomButton)
		{
			weap_state->weaponState = game::WEAPON_READY;
			weap_state->weaponTime = 0;
			weap_state->weaponDelay = 0;
		}
	}

	utils::hook::detour PM_Weapon_hook;
	void PM_Weapon_stub(game::pmove_t* pm, game::pml_t* pml)
	{
		//printf("PM_Weapon start\n");

		if (game::dvar_is_enabled_safe(always_canswap_dvar))
		{
			//printf("always canswap is enabled\n");
			for (int i = 0; i < 15; i++)
			{
				get_weap_equipped_data_ptr(pm, i)->usedBefore = false;
			}
		}

		//printf("PM_Weapon start 2\n");

		canzooms_check(pm);

		//printf("PM_Weapon end 1\n");
		PM_Weapon_hook.invoke<void>(pm, pml);
		//printf("PM_Weapon end final\n");
	}

	utils::hook::detour CG_UpdateViewWeaponAnim_hook;
	void CG_UpdateViewWeaponAnim_stub(unsigned int localClientNum)
	{
		if (game::dvar_is_enabled_safe(freeze_anim_dvar))
		{
			return;
		}

		//printf("CG_UpdateViewWeaponAnim_stub normally call\n");
		CG_UpdateViewWeaponAnim_hook.invoke<void>(localClientNum);
	}

	void* nop_target_1 = nullptr; // was base + 0x11440A5, NOP 3 bytes

	class component final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			batch.add(SETUP_POINTER(game::PM_Weapon_sig),
				"48 8B D5 48 8B CF E8 ? ? ? ? 48 8B 4F 08 4C 8B 74 24 40 48 8B 74 24 38 8B 41 14 C1 E8 1D"
				" A8 01",
				SETUP_MOD(add(7).rip()));

			batch.add(SETUP_POINTER(game::PM_Weapon_ProcessHand_sig),
				"48 8B CF E8 ? ? ? ? 48 8B 8F ?? 03 00 00 48 8B D6 41 FF ?? 49 83 ?? ?? 48 83 C3 04 E8 ? ?"
				" ? ? 44 3B ?? 7E ?? 48 8B 4F 08",
				SETUP_MOD(add(4).rip()));

			batch.add(SETUP_POINTER(game::PM_BeginWeaponChange_sig),
				"41 B1 01 C6 44 24 20 00 48 8B CE E8 ? ? ? FF",
				SETUP_MOD(add(12).rip()));

			batch.add(SETUP_POINTER(game::CG_UpdateViewWeaponAnim),
				"8B CF E8 ? ? ? ? 8B CF E8 ? ? ? ? 48 8B 93",
				SETUP_MOD(add(3).rip()));

			batch.add(SETUP_POINTER(game::PM_Weapon_Idle_sig),
				"7F 0A 33 ? ? ? ? E8 ? ? ? 00 4C",
				SETUP_MOD(add(8).rip()));

			batch.add(SETUP_POINTER(game::PM_GetWeaponFireButton_sig),
				"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B 59 08 41 0F B6 E9");

			batch.add(SETUP_POINTER(game::BG_GetCurrentWeaponForPlayer_sig),
				"48 8B 51 08 48 8B 89 ?? ?? 00 00 E8 ? ? ? ? 48 8B C8 E8 ? ? ? ? 85 C0 B9 18 00 00 00 BA 16 00"
				" 00 00 0F 44 CA",
				SETUP_MOD(add(12).rip()));

			batch.add(SETUP_POINTER(game::BG_PlayerLastWeaponHand_sig),
				"40 53 48 83 EC 20 0F B7 82 ?? ?? 00 00 48 8B DA 4C 6B C0 3E 49 83 C0 02 4C 03 41 08 E8 ? ? ? ?");

			batch.add(SETUP_POINTER(game::BG_PlayerDualWieldingWeapon),
				"48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B F1 49 8B F8 48 8B CA 48 8B DA E8 ?? ?? ?? ?? 84 C0");

			// nop targets that are both inside PM_Weapon_CheckForChangeWeapon
			batch.add("nop_target_1", reinterpret_cast<void**>(&nop_target_1),
				"?? ?? ?? 85 ?? 75 33 48 8B 4C 24 40 41 0F B6 D4 E8 ?? ?? ?? ?? 84 C0 74 21 F3 0F 10 05 ?? ?? ?? ?? F3 0F 59 C6 F3 0F 2C C8 85 C9 7E 0D 48 8B ?? 08 03 ?? 1C 89 88 ?? ?? ?? ??");
		
			// offsets
			batch.add(SETUP_OFFSET(pmove_weaponMap_offset),
				"48 8B 8F ? ? 00 00 48 8B D6 E8 ? ? ? FF",
				SETUP_OFFSET_MOD(add(3).as<std::uint32_t&>()));

			// 83 ? ? ? ? ? ? 48 8B ? E8 ? ? ? FF 84 setspawnweapon gsc func

			// weapState offset: sig finds a mov that writes into [rdi+weapState+0x18],
			// so we read the displacement and subtract 24 (0x18) to land on weapState[0].
			if (identification::game::is("1.20.4-replay"))
			{
				// 44 89 BF [disp32] = mov [rdi+disp32], r15d  — disp at byte 3
				batch.add(SETUP_OFFSET(pmove_weapState_offset),
					"44 89 ? ? ? 00 00 48 8D 0D ? ? ? ? 44 89 ? ? ? 00 00 0F",
					SETUP_OFFSET_MOD(add(3).as<std::uint32_t&>() - 36));
			}
			else // ship builds
			{
				// 89 AF [disp32] = mov [rdi+disp32], ebp  — disp at byte 2
				batch.add(SETUP_OFFSET(pmove_weapState_offset),
					"89 ? ? ? 00 00 48 8D 0D ? ? ? ? 89 ? ? ? 00 00 0F",
					SETUP_OFFSET_MOD(add(2).as<std::uint32_t&>() - 36));
			}

			// ps_sprintState_offset: cmp [rbx+disp32], 0 — disp at byte 12
			// 1.20: 0x31C, 1.38: 0x32C — single sig covers both
			batch.add(SETUP_OFFSET(ps_sprintState_offset),
				"F6 ? ? ? 0F ? ? ? ? ? 83 ? ? ? 00 00 00 0F",
				SETUP_OFFSET_MOD(add(12).as<std::uint32_t&>()));
		}

		void post_unpack() override
		{
			scheduler::once([]
			{
				sprint_swaps_dvar = game::Dvar_RegisterBool("pan_sprintswaps", false, game::DVAR_FLAG_NONE, "");
				instashoots_dvar = game::Dvar_RegisterBool("pan_instashoots", false, game::DVAR_FLAG_NONE, "");
				always_canswap_dvar = game::Dvar_RegisterBool("pan_alwayscanswap", false, game::DVAR_FLAG_NONE, "");
				freeze_anim_dvar = game::Dvar_RegisterBool("pan_freezeanim", false, game::DVAR_FLAG_NONE, "");
				canzooms_dvar = game::Dvar_RegisterBool("pan_canzooms", false, game::DVAR_FLAG_NONE, "");
				always_altswap_dvar = game::Dvar_RegisterBool("pan_alwaysaltswap", false, game::DVAR_FLAG_NONE, "");
			}, scheduler::main);

			if (nop_target_1)
				utils::hook::nop(nop_target_1, 3);

			if (game::PM_Weapon_sig)
				PM_Weapon_hook.create(game::PM_Weapon_sig, PM_Weapon_stub);

			if (game::PM_Weapon_ProcessHand_sig)
				PM_Weapon_ProcessHand_hook.create(game::PM_Weapon_ProcessHand_sig, PM_Weapon_ProcessHand_stub);

			if (game::PM_BeginWeaponChange_sig)
				PM_BeginWeaponChange_hook.create(game::PM_BeginWeaponChange_sig, PM_BeginWeaponChange_stub);

			//if (game::CG_UpdateViewWeaponAnim)
			//	CG_UpdateViewWeaponAnim_hook.create(game::CG_UpdateViewWeaponAnim, CG_UpdateViewWeaponAnim_stub);
		}
	};
}

REGISTER_COMPONENT(weapon::component)
