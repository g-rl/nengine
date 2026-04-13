#include <std_include.hpp>

#include "game/game.hpp"
#include <identification/game.hpp>
#include "loader/component_loader.hpp"

class pointers final : public component_interface
{
public:
	void find_signatures(memory::signature_store& batch) override
	{
		// Example: direct function address
		//batch.add(SETUP_POINTER(game::Cbuf_AddText_sig),
		//	"48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F2 41 8B F8");

		// Example: E8 <rel32> call — grab operand via add(1).rip()
		//batch.add(SETUP_POINTER(game::Com_Error_sig),
		//	"E8 ? ? ? ? 84 C0 74 ? 48 8B CB",
		//	GRAB_CALL);

		static const auto game_ = identification::game::get_target_game().client_name;

		// IW9: [sig] MISS: game::Com_Error (E8 ? ? ? ? 41 8D 46 ? 44 3B F8)
		if (identification::game::is("1.20.4-replay")) {
			batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 4C 8B EF", GRAB_CALL);
		}
		else if (identification::game::is_less_or_eq("1.24.0")) {
			batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 48 8B 8B ? ? ? ? 8B 49", GRAB_CALL);
		}
		else {
			batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 41 8D 46 ? 44 3B F8", GRAB_CALL);
		}

		batch.add(SETUP_POINTER(game::DB_FindXAssetHeader), "E8 ? ? ? ? 44 8B C5 8D 4D", GRAB_CALL);
		batch.add(SETUP_POINTER(game::DB_IsXAssetDefault), "E8 ? ? ? FF ? C0 75 0B 48 8B ? 48 8B CF E8 ? 00 00 00", GRAB_CALL); // inside ProcessScriptFile (IW8, S4, IW9)
		batch.add(SETUP_POINTER(game::DB_XAssetExists), "E8 ? ? ? FF 85 ? 75 0B 48 8B D6 48 8B CF E8 1E 00 00 00", GRAB_CALL); // IW9 [sig] MISS: game::DB_XAssetExists (E8 ? ? ? FF 85 ? 75 0B 48 8B D6 48 8B CF E8 1E 00 00 00)

		batch.add(SETUP_POINTER(game::NetConstStrings_GetIndexPlusOneFromName), "E8 ? ? ? ? 33 C9 84 C0 0F 45 4C 24 ? 89 0B", GRAB_CALL);
		batch.add(SETUP_POINTER(game::NetConstStrings_GetNameFromIndexPlusOne), "B9 0C 00 00 00 E8 ? ? ? ?? 84 C0 74 ? 48 ? ? ? ? B2", SETUP_MOD(add(6).rip()));

		batch.add(SETUP_POINTER(game::Com_FrontEnd_IsInFrontEnd), "0F ? ? 83 ? ? E8 ? ? ? 00 84 ? 74 ? E8 ? ? ? FF 84", SETUP_MOD(add(7).rip())); // IW9 [sig] MISS: game::Com_FrontEnd_IsInFrontEnd (0F ? ? 83 ? ? E8 ? ? ? 00 84 ? 74 ? E8 ? ? ? FF 84)

		batch.add(SETUP_POINTER(game::ScriptContext_Server), "E8 ? ? ? ? 4C 8B C3 41 8B D7", GRAB_CALL);
		batch.add(SETUP_POINTER(game::Scr_LoadScript), "48 89 5C 24 ? 57 48 83 EC ? 48 8B DA 48 8B F9 BA ? ? ? ? 48 8D 4C 24");
		batch.add(SETUP_POINTER(game::Scr_GetFunctionHandle), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B E8 48 8B D9 E8"); // IW9 [sig] MISS: game::Scr_GetFunctionHandle (48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B E8 48 8B D9 E8)

		batch.add(SETUP_POINTER(game::Scr_ExecThread), "48 83 EC ? 33 C0 45 8B C8");
		batch.add(SETUP_POINTER(game::Scr_FreeThread), "E8 ? ? ? ? 48 8B 4F ? 48 63 81", GRAB_CALL);

		batch.add(SETUP_POINTER(game::Dvar_FindVarByName), "E8 ? ? ? ? 48 8B CB 48 63 50", GRAB_CALL);
		//batch.add(SETUP_POINTER(game::Dvar_GetIntSafe), "E8 ? ? ? ? 8B D0 85 C0 75 ? 38 05", GRAB_CALL);
		batch.add(SETUP_POINTER(game::Dvar_RegisterBool), "E8 ? ? ? ? 48 8B F0 F6 46", GRAB_CALL);

		if (identification::game::is("1.20.4-replay")) {
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4"
				" 48 89 84 24 ? ? ? ? 8B 05");
		}
		else if (identification::game::is_less_or_eq("1.24.0")) {
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ?"
				" 4C 8B F9");
		}
		else {
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ?"
				" 4C 8B E1");
		}

		// IW9 sig results
		/*
		[sig] found: game::SV_CmdsMP_RequestMapRestart at 0x35a47f0
		[sig] found: game::Cmd_AddCommandInternal at 0x31fed40
		[sig] found: game::cmd_args at 0xcb93670
		[sig] MISS: game::Com_Error (E8 ? ? ? ? 41 8D 46 ? 44 3B F8)
		[sig] found: game::DB_FindXAssetHeader at 0x31f4a10
		[sig] MISS: game::DB_IsXAssetDefault (48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 48 8B DA 8B F1)
		[sig] MISS: game::DB_XAssetExists (E8 ? ? ? FF 85 ? 75 0B 48 8B D6 48 8B CF E8 1E 00 00 00)
		[sig] found: game::NetConstStrings_GetIndexPlusOneFromName at 0x31ba400
		[sig] found: game::NetConstStrings_GetNameFromIndexPlusOne at 0x31ba5f0
		[sig] MISS: game::Com_FrontEnd_IsInFrontEnd (0F ? ? 83 ? ? E8 ? ? ? 00 84 ? 74 ? E8 ? ? ? FF 84)
		[sig] found: game::ScriptContext_Server at 0x3f445a0
		[sig] found: game::Scr_LoadScript at 0x3359a30
		[sig] MISS: game::Scr_GetFunctionHandle (48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B E8 48 8B D9 E8)
		[sig] found: game::Scr_ExecThread at 0x3372480
		[sig] found: game::Scr_FreeThread at 0x3372ee0
		[sig] found: game::Dvar_FindVarByName at 0x3ccf150
		[sig] MISS: game::Dvar_RegisterBool (E8 ? ? ? ? 48 8B F0 F6 46)
		[sig] MISS: game::Dvar_RegisterVariant (48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ? 4C 8B E1)
		[sig] MISS: game::R_EndFrame (48 83 EC ? E8 ? ? ? ? 48 8B 15 ? ? ? ? 45 33 D2)
		[sig] found: game::FenceManager_Frame at 0x206d180
		[sig] MISS: game::PM_Weapon_sig (48 8B D5 48 8B CF E8 ? ? ? ? 48 8B 4F 08 4C 8B 74 24 40 48 8B 74 24 38 8B 41 14 C1 E8 1D A8 01)
		[sig] MISS: game::PM_Weapon_ProcessHand_sig (48 8B CF E8 ? ? ? ? 48 8B 8F ?? 03 00 00 48 8B D6 41 FF ?? 49 83 ?? ?? 48 83 C3 04 E8 ? ? ? ? 44 3B ?? 7E ?? 48 8B 4F 08)
		[sig] MISS: game::PM_BeginWeaponChange_sig (41 B1 01 C6 44 24 20 00 48 8B CE E8 ? ? ? FF)
		[sig] found: game::CG_UpdateViewWeaponAnim at 0x47eeed0
		[sig] MISS: game::PM_Weapon_Idle_sig (7F 0A 33 ? ? ? ? E8 ? ? ? 00 4C)
		[sig] found: game::PM_GetWeaponFireButton_sig at 0x573af90
		[sig] found: game::BG_GetCurrentWeaponForPlayer_sig at 0x575acd0
		[sig] MISS: game::BG_PlayerLastWeaponHand_sig (40 53 48 83 EC 20 0F B7 82 ?? ?? 00 00 48 8B DA 4C 6B C0 3E 49 83 C0 02 4C 03 41 08 E8 ? ? ? ?)
		[sig] found: game::BG_PlayerDualWieldingWeapon at 0x20a7230
		[sig] MISS: nop_target_1 (?? ?? ?? 85 ?? 75 33 48 8B 4C 24 40 41 0F B6 D4 E8 ?? ?? ?? ?? 84 C0 74 21 F3 0F 10 05 ?? ?? ?? ?? F3 0F 59 C6 F3 0F 2C C8 85 C9 7E 0D 48 8B ?? 08 03 ?? 1C 89 88 ?? ?? ?? ??)
		[sig] offset: pmove_weaponMap_offset = 0x6a0
		[sig] offset: pmove_weapState_offset = 0x860
		[sig] offset: ps_sprintState_offset = 0x370
		[sig] resolved 19/33
*/
	}
};

REGISTER_COMPONENT(pointers)
