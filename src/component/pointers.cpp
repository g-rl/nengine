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
		//	SETUP_MOD(add(1).rip()));

		if (identification::game::is("1.20.4-replay")) {
			batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 4C 8B EF", SETUP_MOD(add(1).rip()));
		}
		else if (identification::game::is_less_or_eq("1.24.0")) {
			batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 48 8B 8B ? ? ? ? 8B 49", SETUP_MOD(add(1).rip()));
		}
		else {
			batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 41 8D 46 ? 44 3B F8", SETUP_MOD(add(1).rip()));
		}

		batch.add(SETUP_POINTER(game::DB_FindXAssetHeader), "E8 ? ? ? ? 44 8B C5 8D 4D", SETUP_MOD(add(1).rip()));
		batch.add(SETUP_POINTER(game::DB_IsXAssetDefault), "48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ?"
			" 48 8B DA 8B F1");
		batch.add(SETUP_POINTER(game::DB_XAssetExists), "E8 ? ? ? FF 85 ? 75 0B 48 8B D6 48 8B CF E8 1E 00 00 00", SETUP_MOD(add(1).rip()));


		batch.add(SETUP_POINTER(game::NetConstStrings_GetIndexPlusOneFromName), "E8 ? ? ? ? 33 C9 84 C0 0F 45 4C 24 ? 89 0B", SETUP_MOD(add(1).rip()));
		batch.add(SETUP_POINTER(game::NetConstStrings_GetNameFromIndexPlusOne), "B9 0C 00 00 00 E8 ? ? ? ?? 84 C0 74 ? 48 ? ? ? ? B2", SETUP_MOD(add(6).rip()));

		batch.add(SETUP_POINTER(game::Com_FrontEnd_IsInFrontEnd), "0F ? ? 83 ? ? E8 ? ? ? 00 84 ? 74 ? E8 ? ? ? FF 84", SETUP_MOD(add(7).rip()));

		batch.add(SETUP_POINTER(game::ScriptContext_Server), "E8 ? ? ? ? 4C 8B C3 41 8B D7", SETUP_MOD(add(1).rip()));
		batch.add(SETUP_POINTER(game::Scr_LoadScript), "48 89 5C 24 ? 57 48 83 EC ? 48 8B DA 48 8B F9 BA ? ? ? ? 48 8D 4C 24");
		batch.add(SETUP_POINTER(game::Scr_GetFunctionHandle), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8B E8 48 8B D9 E8");

		batch.add(SETUP_POINTER(game::Scr_ExecThread), "48 83 EC ? 33 C0 45 8B C8");
		batch.add(SETUP_POINTER(game::Scr_FreeThread), "E8 ? ? ? ? 48 8B 4F ? 48 63 81", SETUP_MOD(add(1).rip()));

		batch.add(SETUP_POINTER(game::Dvar_RegisterBool), "E8 ? ? ? ? 48 8B F0 F6 46", SETUP_MOD(add(1).rip()));
		batch.add(SETUP_POINTER(game::Dvar_FindVarByName), "E8 ? ? ? ? 48 8B CB 48 63 50", SETUP_MOD(add(1).rip()));
	}
};

REGISTER_COMPONENT(pointers)
