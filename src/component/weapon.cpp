#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <identification/game.hpp>

#include "scheduler.hpp"

#include <utils/hook.hpp>

namespace weapon
{
	const game::dvar_t* sprintswaps = nullptr;
	const game::dvar_t* instashoots = nullptr;
	const game::dvar_t* alwayscanswap = nullptr;
	const game::dvar_t* freezeanim = nullptr;
	const game::dvar_t* canzooms = nullptr;
	const game::dvar_t* alwaysaltswap = nullptr;

	utils::hook::detour PM_BeginWeaponChange_hook;
	void PM_BeginWeaponChange_stub(game::pmove_t* pm, game::pml_t* pml,
		const game::Weapon* newweapon, bool isNewAlternate, bool quick)
	{
		if (alwaysaltswap && alwaysaltswap->current.enabled)
		{
			quick = true;
		}

		if (!sprintswaps || !sprintswaps->current.enabled)
		{
			PM_BeginWeaponChange_hook.invoke<void>(pm, pml, newweapon, isNewAlternate, quick);
			return;
		}

		game::PlayerActiveWeaponState prevWeapState[2] = {
			pm->ps->weapState[0],
			pm->ps->weapState[1]
		};

		PM_BeginWeaponChange_hook.invoke<void>(pm, pml, newweapon, isNewAlternate, quick);

		const bool isSprinting = pm->ps->sprintState.lastSprintStart
			&& pm->ps->sprintState.lastSprintStart > pm->ps->sprintState.lastSprintEnd;

		if (isSprinting)
		{
			for (int i = 0; i < 2; i++)
			{
				pm->ps->weapState[i].weapAnim = prevWeapState[i].weapAnim;
				pm->ps->weapState[i].prevWeapAnim = prevWeapState[i].prevWeapAnim;
			}
		}
	}

	void instashoots_check(game::pmove_t* pm, int hand)
	{
		if (!instashoots || !instashoots->current.enabled)
			return;

		int state = pm->ps->weapState[hand].weaponState;
		if (state != game::WEAPON_RAISING && state != game::WEAPON_RAISING_ALTSWITCH)
			return;

		const game::Weapon* currentWeapon = game::BG_GetCurrentWeaponForPlayer_sig(
			pm->weaponMap, pm->ps);

		uint64_t fireButton = game::PM_GetWeaponFireButton_sig(
			pm, currentWeapon, hand, pm->cmd.inputFromGamepad);

		if (pm->cmd.buttons & fireButton)
		{
			int lastHand = game::BG_PlayerLastWeaponHand_sig(pm->weaponMap, pm->ps);
			for (int i = 0; i <= lastHand; i++)
				game::PM_Weapon_Idle_sig(pm, i);
		}
	}

	utils::hook::detour PM_Weapon_ProcessHand_hook;
	void PM_Weapon_ProcessHand_stub(game::pmove_t* pm, game::pml_t* pml,
		int delayedAction, int hand)
	{
		instashoots_check(pm, hand);
		PM_Weapon_ProcessHand_hook.invoke<void>(pm, pml, delayedAction, hand);
	}

	void canzooms_check(game::pmove_t* pm)
	{
		if (!canzooms || !canzooms->current.enabled)
			return;

		if (pm->ps->weapState[0].weaponState != game::WEAPON_RAISING)
			return;

		const game::Weapon* currentWeapon = game::BG_GetCurrentWeaponForPlayer_sig(pm->weaponMap, pm->ps);
		const bool dualWielding = game::BG_PlayerDualWieldingWeapon(pm->weaponMap, pm->ps, currentWeapon);
		if (dualWielding)
			return;

		unsigned int zoomButton = 0x20000;
		if (pm->cmd.buttons & zoomButton)
		{
			pm->ps->weapState[0].weaponState = game::WEAPON_READY;
			pm->ps->weapState[0].weaponTime = 0;
			pm->ps->weapState[0].weaponDelay = 0;
		}
	}

	utils::hook::detour PM_Weapon_hook;
	void PM_Weapon_stub(game::pmove_t* pm, game::pml_t* pml)
	{
		if (alwayscanswap && alwayscanswap->current.enabled)
		{
			for (int i = 0; i < 15; i++)
			{
				pm->ps->weapEquippedData[i].usedBefore = false;
			}
		}

		canzooms_check(pm);
		PM_Weapon_hook.invoke<void>(pm, pml);
	}

	utils::hook::detour CG_UpdateViewWeaponAnim_hook;
	void CG_UpdateViewWeaponAnim_stub(unsigned int localClientNum)
	{
		if (freezeanim && freezeanim->current.enabled)
		{
			return;
		}

		CG_UpdateViewWeaponAnim_hook.invoke<void>(localClientNum);
	}

	// =========================================================================
	// Sig-scanned NOP targets
	//
	// These point to instructions that need to be patched out.
	// In the old codebase these were hardcoded as base_address + RVA.
	// Now they're resolved via signature scanning.
	// =========================================================================
	void* nop_target_1 = nullptr; // was base + 0x11440A5, NOP 3 bytes
	void* nop_target_2 = nullptr; // was base + 0x11440E1, NOP 4 bytes

	class component final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			const bool is_ship_replay = identification::game::is("1.20.4-replay");

			batch.add(SETUP_POINTER(game::PM_Weapon_sig),
				"48 8B D5 48 8B CF E8 ? ? ? ? 48 8B 4F 08 4C 8B 74 24 40 48 8B 74 24 38 8B 41 14 C1 E8 1D"
				" A8 01",
				SETUP_MOD(add(7).rip()));

			batch.add(SETUP_POINTER(game::PM_Weapon_ProcessHand_sig),
				"48 8B CF E8 ? ? ? ? 48 8B 8F ?? 03 00 00 48 8B D6 41 FF ?? 49 83 ?? ?? 48 83 C3 04 E8 ? ?"
				" ? ? 44 3B ?? 7E ?? 48 8B 4F 08",
				SETUP_MOD(add(4).rip()));

			batch.add(SETUP_POINTER(game::PM_BeginWeaponChange_sig),
				"48 8B 94 24 88 00 00 00 4C 8B C0 41 B1 01 C6 44 24 20 00 48 8B CE E8 ? ? ? ? 0F 28 74 24 30"
				" 48 8B AC 24 90 00 00 00",
				SETUP_MOD(add(23).rip()));

			batch.add(SETUP_POINTER(game::CG_UpdateViewWeaponAnim),
				"E8 ? ? ? ? 8B CF E8 ? ? ? ? 8B CF E8 ? ? ? ? 48 8B 93 ? ? 00 00",
				SETUP_MOD(add(8).rip()));

			batch.add(SETUP_POINTER(game::PM_Weapon_Idle_sig),
				"83 FA 3B 77 ?? 48 B9 01 00 00 00 00 00 01 0C 48 0F A3 D1 72 ?? EB ?? 45 85 FF 7F ?? 33 D2 48"
				" 8B CF E8 ? ? ? ?",
				SETUP_MOD(add(34).rip()));

			batch.add(SETUP_POINTER(game::PM_GetWeaponFireButton_sig),
				"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B 59 08 41 0F B6 E9");

			batch.add(SETUP_POINTER(game::BG_GetCurrentWeaponForPlayer_sig),
				"48 8B 51 08 48 8B 89 ?? ?? 00 00 E8 ? ? ? ? 48 8B C8 E8 ? ? ? ? 85 C0 B9 18 00 00 00 BA 16 00"
				" 00 00 0F 44 CA",
				SETUP_MOD(add(12).rip()));

			batch.add(SETUP_POINTER(game::BG_PlayerLastWeaponHand_sig),
				"E8 ? ? ? ? 48 85 C0 74 ?? 48 8B D5 48 8B C8 E8 ? ? ? ? 0F B6 F8",
				SETUP_MOD(add(17).rip()));

			batch.add(SETUP_POINTER(game::BG_PlayerDualWieldingWeapon),
				"48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B F1 49 8B F8 48 8B CA 48 8B DA E8 ?? ?? ?? ?? 84 C0");

			// NOP targets
			// Both inside PM_Weapon_CheckForChangeWeapon.
			// Anchor directly on the patch sites so this stays one signature call.
			if (is_ship_replay)
			{
				batch.add("nop_target_1", reinterpret_cast<void**>(&nop_target_1),
					"41 89 06 85 F6 75 33 48 8B 4C 24 40 41 0F B6 D4 E8 ?? ?? ?? ?? 84 C0 74 21 F3 0F 10 05 ?? ?? ?? ?? F3 0F 59 C6 F3 0F 2C C8 85 C9 7E 0D 48 8B 47 08 03 4F 1C 89 88 CC 10 00 00");
				batch.add("nop_target_2", reinterpret_cast<void**>(&nop_target_2),
					"89 88 CC 10 00 00 FF C6 49 83 C6 50 49 83 EF 01 0F 85 ?? ?? ?? ?? 4C 8B AC 24 C0 00 00 00 0F 28 74 24 50 B9 4A 00 00 00 E8 ?? ?? ?? ??");
			}
			else
			{
				//who knows bruh
				batch.add("nop_target_1", reinterpret_cast<void**>(&nop_target_1),
					"89 43 10 85 ED 75 33 48 8B 4C 24 40 41 0F B6 D4 E8 ?? ?? ?? ?? 84 C0 74 21 F3 0F 10 05 ?? ?? ?? ?? F3 0F 59 C6 F3 0F 2C C8 85 C9 7E 0D 48 8B 46 08 03 4E 1C 89 88 38 11 00 00");
				batch.add("nop_target_2", reinterpret_cast<void**>(&nop_target_2),
					"89 88 38 11 00 00 FF C5 48 83 C3 54 49 83 EF 01 0F 85 ?? ?? ?? ?? 4C 8B AC 24 C0 00 00 00 0F 28 74 24 50 48 8B 9E C8 03 00 00");
			}
		}

		void post_unpack() override
		{
			scheduler::once([]
			{
				sprintswaps = game::Dvar_RegisterBool("pan_sprintswaps", false, game::DVAR_FLAG_NONE, "");
				instashoots = game::Dvar_RegisterBool("pan_instashoots", false, game::DVAR_FLAG_NONE, "");
				alwayscanswap = game::Dvar_RegisterBool("pan_alwayscanswap", false, game::DVAR_FLAG_NONE, "");
				freezeanim = game::Dvar_RegisterBool("pan_freezeanim", false, game::DVAR_FLAG_NONE, "");
				canzooms = game::Dvar_RegisterBool("pan_canzooms", false, game::DVAR_FLAG_NONE, "");
				alwaysaltswap = game::Dvar_RegisterBool("pan_alwaysaltswap", false, game::DVAR_FLAG_NONE, "");
			}, scheduler::main);

			// NOP patches - only apply if signatures resolved
			if (nop_target_1)
				utils::hook::nop(nop_target_1, 3);
			if (nop_target_2)
				utils::hook::nop(nop_target_2, 4);

			// Detour hooks - only create if function pointers resolved
			if (game::PM_Weapon_sig)
				PM_Weapon_hook.create(game::PM_Weapon_sig, PM_Weapon_stub);

			if (game::PM_Weapon_ProcessHand_sig)
				PM_Weapon_ProcessHand_hook.create(game::PM_Weapon_ProcessHand_sig, PM_Weapon_ProcessHand_stub);

			if (game::PM_BeginWeaponChange_sig)
				PM_BeginWeaponChange_hook.create(game::PM_BeginWeaponChange_sig, PM_BeginWeaponChange_stub);

			if (game::CG_UpdateViewWeaponAnim)
				CG_UpdateViewWeaponAnim_hook.create(game::CG_UpdateViewWeaponAnim, CG_UpdateViewWeaponAnim_stub);
		}
	};
}

REGISTER_COMPONENT(weapon::component)
