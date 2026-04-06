#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <identification/game.hpp>

#include "scheduler.hpp"

#include <utils/hook.hpp>

namespace weapon
{
	const game::dvar_t* sprint_swaps_dvar = nullptr;
	const game::dvar_t* instashoots_dvar = nullptr;
	const game::dvar_t* always_canswap_dvar = nullptr;
	const game::dvar_t* freeze_anim_dvar = nullptr;
	const game::dvar_t* canzooms_dvar = nullptr;
	const game::dvar_t* always_altswap_dvar = nullptr;

	utils::hook::detour PM_BeginWeaponChange_hook;
	void PM_BeginWeaponChange_stub(game::pmove_t* pm, game::pml_t* pml,
		const game::Weapon* newweapon, bool isNewAlternate, bool quick)
	{
		printf("PM_BeginWeaponChange_stub\n");

		if (always_altswap_dvar && always_altswap_dvar->current.enabled)
		{
			quick = true;
		}

		if (!sprint_swaps_dvar || !sprint_swaps_dvar->current.enabled)
		{
			PM_BeginWeaponChange_hook.invoke<void>(pm, pml, newweapon, isNewAlternate, quick);
			return;
		}

		printf("PM_BeginWeaponChange_stub 2\n");

		game::PlayerActiveWeaponState prevWeapState[2] = {
			pm->ps->weapState[0],
			pm->ps->weapState[1]
		};

		printf("PM_BeginWeaponChange_stub 3\n");
		PM_BeginWeaponChange_hook.invoke<void>(pm, pml, newweapon, isNewAlternate, quick);

		printf("PM_BeginWeaponChange_stub 4\n");

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

		printf("PM_BeginWeaponChange_stub final\n");
	}

	void instashoots_check(game::pmove_t* pm, int hand)
	{
		if (!instashoots_dvar || !instashoots_dvar->current.enabled)
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
		printf("PM_Weapon_ProcessHand_stub\n");
		instashoots_check(pm, hand);
		printf("PM_Weapon_ProcessHand_stub end 1\n");
		PM_Weapon_ProcessHand_hook.invoke<void>(pm, pml, delayedAction, hand);
		printf("PM_Weapon_ProcessHand_stub end final\n");
	}

	void canzooms_check(game::pmove_t* pm)
	{
		if (!canzooms_dvar || !canzooms_dvar->current.enabled)
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
		printf("PM_Weapon start\n");

		if (always_canswap_dvar && always_canswap_dvar->current.enabled)
		{
			for (int i = 0; i < 15; i++)
			{
				pm->ps->weapEquippedData[i].usedBefore = false;
			}
		}

		canzooms_check(pm);

		printf("PM_Weapon end 1\n");
		PM_Weapon_hook.invoke<void>(pm, pml);
		printf("PM_Weapon end final\n");
	}

	utils::hook::detour CG_UpdateViewWeaponAnim_hook;
	void CG_UpdateViewWeaponAnim_stub(unsigned int localClientNum)
	{
		printf("CG_UpdateViewWeaponAnim_stub\n");
		if (freeze_anim_dvar && freeze_anim_dvar->current.enabled)
		{
			return;
		}

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
				"48 8B 94 24 88 00 00 00 4C 8B C0 41 B1 01 C6 44 24 20 00 48 8B CE E8 ? ? ? ? 0F 28 74 24 30"
				" 48 8B AC 24 90 00 00 00",
				SETUP_MOD(add(23).rip()));

			batch.add(SETUP_POINTER(game::CG_UpdateViewWeaponAnim),
				"E8 ? ? ? ? 8B CF E8 ? ? ? ? 48 8B 93 ? ? ? ? 41 B0",
				SETUP_MOD(add(1).rip()));

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
				"40 53 48 83 EC 20 0F B7 82 ?? ?? 00 00 48 8B DA 4C 6B C0 3E 49 83 C0 02 4C 03 41 08 E8 ? ? ? ?");

			batch.add(SETUP_POINTER(game::BG_PlayerDualWieldingWeapon),
				"48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 8B F1 49 8B F8 48 8B CA 48 8B DA E8 ?? ?? ?? ?? 84 C0");

			// nop targets that are both inside PM_Weapon_CheckForChangeWeapon
			batch.add("nop_target_1", reinterpret_cast<void**>(&nop_target_1),
				"?? ?? ?? 85 ?? 75 33 48 8B 4C 24 40 41 0F B6 D4 E8 ?? ?? ?? ?? 84 C0 74 21 F3 0F 10 05 ?? ?? ?? ?? F3 0F 59 C6 F3 0F 2C C8 85 C9 7E 0D 48 8B ?? 08 03 ?? 1C 89 88 ?? ?? ?? ??");
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

			if (game::CG_UpdateViewWeaponAnim)
				CG_UpdateViewWeaponAnim_hook.create(game::CG_UpdateViewWeaponAnim, CG_UpdateViewWeaponAnim_stub);
		}
	};
}

REGISTER_COMPONENT(weapon::component)
