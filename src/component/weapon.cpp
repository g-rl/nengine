#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include "scheduler.hpp"

#include <utils/hook.hpp>

namespace weapon
{
	const game::dvar_t* sprintswaps = nullptr;
	const game::dvar_t* instashoots = nullptr;
	const game::dvar_t* alwayscanswap = nullptr;
	const game::dvar_t* freezeanim = nullptr;

	// =========================================================================
	// Detour: PM_BeginWeaponChange
	//
	// Preserves weapon animation state when switching weapons while sprinting.
	// Without this, the sprint-to-swap transition plays incorrect anims.
	// =========================================================================
	utils::hook::detour PM_BeginWeaponChange_hook;
	void PM_BeginWeaponChange_stub(game::pmove_t* pm, game::pml_t* pml,
		const game::Weapon* newweapon, bool isNewAlternate, bool quick)
	{
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

	// =========================================================================
	// Instashoot check
	//
	// If the weapon is still in RAISING state and the fire button is held,
	// skip the raise animation and go straight to idle (ready to fire).
	// =========================================================================
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

	// =========================================================================
	// Detour: PM_Weapon_ProcessHand
	//
	// Runs the instashoot check before each hand's weapon processing.
	// =========================================================================
	utils::hook::detour PM_Weapon_ProcessHand_hook;
	void PM_Weapon_ProcessHand_stub(game::pmove_t* pm, game::pml_t* pml,
		int delayedAction, int hand)
	{
		instashoots_check(pm, hand);
		PM_Weapon_ProcessHand_hook.invoke<void>(pm, pml, delayedAction, hand);
	}

	// =========================================================================
	// Detour: PM_Weapon
	//
	// Clears the "usedBefore" flag on all 15 weapon slots each frame,
	// allowing re-equip without restrictions.
	// =========================================================================
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
			// ── Functions ──────────────────────────────────────────────
			// Patterns from game_dx12_ship_replay_dump.exe prologue bytes.
			// ?? marks RIP-relative offsets / short jump targets that
			// change between builds.

			// PM_Weapon (2 matches without cookie tail — include stack store to disambiguate)
			batch.add(SETUP_POINTER(game::PM_Weapon_sig),
				"40 53 55 56 57 41 56 41 57 48 81 EC A8 00 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 90 00");

			// PM_Weapon_ProcessHand
			batch.add(SETUP_POINTER(game::PM_Weapon_ProcessHand_sig),
				"44 89 44 24 18 55 56 57 41 55 41 56 48 83 EC 70");

			// PM_BeginWeaponChange
			batch.add(SETUP_POINTER(game::PM_BeginWeaponChange_sig),
				"40 55 57 41 54 41 55 41 57 48 81 EC F0 00 00 00");

			// CG_UpdateViewWeaponAnim
			batch.add(SETUP_POINTER(game::CG_UpdateViewWeaponAnim),
				"48 89 5C 24 ?? 56 57 41 55 41 56 41 57 48 81 EC F0 04 00 00");

			// PM_Weapon_Idle (thunk — struct offsets 0x380 and 0x348 are the anchor)
			batch.add(SETUP_POINTER(game::PM_Weapon_Idle_sig),
				"4C 8B 81 80 03 00 00 44 8B CA");

			// PM_GetWeaponFireButton
			batch.add(SETUP_POINTER(game::PM_GetWeaponFireButton_sig),
				"48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B 59 08 41 0F B6 E9");

			// BG_GetCurrentWeaponForPlayer (struct offsets 0x704 and 0x6D8 are the anchor)
			batch.add(SETUP_POINTER(game::BG_GetCurrentWeaponForPlayer_sig),
				"8B 82 04 07 00 00 4C 8B C1 D1 E8");

			// BG_PlayerLastWeaponHand
			batch.add(SETUP_POINTER(game::BG_PlayerLastWeaponHand_sig),
				"40 53 48 83 EC 20 0F B7 82 F8 06 00 00 48 8B DA");

			// ── NOP targets ────────────────────────────────────────────
			// Both inside PM_Weapon_CheckForChangeWeapon.
			// Anchor on its prologue, then offset to each instruction.
			//
			// +0x365 = mov [r14], eax  (3 bytes to NOP)
			// +0x3A1 = add r14, 50h   (4 bytes to NOP)

			batch.add("nop_target_1", reinterpret_cast<void**>(&nop_target_1),
				"48 89 54 24 10 53 55 56 57 41 55 41 56 41 57 48 83 EC 70",
				SETUP_MOD(add(0x365)));

			batch.add("nop_target_2", reinterpret_cast<void**>(&nop_target_2),
				"48 89 54 24 10 53 55 56 57 41 55 41 56 41 57 48 83 EC 70",
				SETUP_MOD(add(0x3A1)));
		}

		void post_unpack() override
		{
			scheduler::once([]
			{
				sprintswaps = game::Dvar_RegisterBool("pan_sprintswaps", false, game::DVAR_FLAG_NONE, "");
				instashoots = game::Dvar_RegisterBool("pan_instashoots", false, game::DVAR_FLAG_NONE, "");
				alwayscanswap = game::Dvar_RegisterBool("pan_alwayscanswap", false, game::DVAR_FLAG_NONE, "");
				freezeanim = game::Dvar_RegisterBool("pan_freezeanim", false, game::DVAR_FLAG_NONE, "");
			}, scheduler::main);

			// NOP patches — only apply if signatures resolved
			if (nop_target_1)
				utils::hook::nop(nop_target_1, 3);
			if (nop_target_2)
				utils::hook::nop(nop_target_2, 4);

			// Detour hooks — only create if function pointers resolved
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
