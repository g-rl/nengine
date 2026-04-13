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

		static const auto& game_ = identification::game::get_target_game().client_name;

		// IW9: [sig] MISS: game::Com_Error (E8 ? ? ? ? 41 8D 46 ? 44 3B F8)
		if (game_ == "iw8-mod"s)
		{
			if (identification::game::is("1.20.4-replay")) {
				batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 4C 8B EF", GRAB_CALL);
			}
			else if (identification::game::is_less_or_eq("1.24.0")) {
				batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 48 8B 8B ? ? ? ? 8B 49", GRAB_CALL);
			}
			else {
				batch.add(SETUP_POINTER(game::Com_Error), "E8 ? ? ? ? 41 8D 46 ? 44 3B F8", GRAB_CALL);
			}
		}
		else
		{
			// 48 8D 15 ? ? ? ? 8D 4F 01 E8 ? ? ? ? 48 8B ? E8 ? ? ? 00
			// 1.20.4-replay, S4, IW9
			batch.add(SETUP_POINTER(game::Com_Error), "48 8D 15 ? ? ? ? 8D 4F 01 E8 ? ? ? ? 48 8B ? E8 ? ? ? 00",
				SETUP_MOD(add(11).rip()));
		}

		batch.add(SETUP_POINTER(game::DB_FindXAssetHeader), "E8 ? ? ? ? 44 8B C5 8D 4D", GRAB_CALL);
		batch.add(SETUP_POINTER(game::DB_IsXAssetDefault), "E8 ? ? ? FF ? C0 75 0B 48 8B ? 48 8B CF E8 ? 00 00 00", GRAB_CALL); // inside ProcessScriptFile (IW8, S4, IW9)
		batch.add(SETUP_POINTER(game::DB_XAssetExists), 
			"B9 ? 00 00 00 E8 ? ? ? 00 ? C0 0F 84 ? ? 00 00 45 33 C0 ? 89 ? ? ? 01 00 00 48",
			SETUP_MOD(add(6).rip()));

		batch.add(SETUP_POINTER(game::NetConstStrings_GetIndexPlusOneFromName), "E8 ? ? ? ? 33 C9 84 C0 0F 45 4C 24 ? 89 0B", GRAB_CALL);
		batch.add(SETUP_POINTER(game::NetConstStrings_GetNameFromIndexPlusOne), "B9 0C 00 00 00 E8 ? ? ? ?? 84 C0 74 ? 48 ? ? ? ? B2", SETUP_MOD(add(6).rip()));

		batch.add(SETUP_POINTER(game::Com_FrontEnd_IsInFrontEnd), "E8 ? ? ? 00 84 C0 74 ? E8 ? ? FF FF 84 C0 75 ? E8", GRAB_CALL);

		batch.add(SETUP_POINTER(game::ScriptContext_Server), "E8 ? ? ? ? 4C 8B C3 41 8B D7", GRAB_CALL);
		batch.add(SETUP_POINTER(game::Scr_LoadScript), "48 89 5C 24 ? 57 48 83 EC ? 48 8B DA 48 8B F9 BA ? ? ? ? 48 8D 4C 24");

		// this is the bottom of the function, and we need to make something to scan up and find the beginning of it
		batch.add(SETUP_POINTER(game::Scr_GetFunctionHandle), "49 2B D0 48 3B D1 73 ? 41 2B C0", [](memory::scanned_result<void> r) {
			std::uintptr_t offset = 0;
			while (true) {
				offset++;
				auto buf = r.sub(offset).as<std::uint8_t*>();
				if (buf[0] == 0x48 && buf[1] == 0x89 && buf[2] == 0x5C && buf[3] == 0x24 && buf[4] == 0x08) {
					break;
				}
			}
			return r.sub(offset);
		});

		batch.add(SETUP_POINTER(game::Scr_ExecThread), "48 83 EC ? 33 C0 45 8B C8");
		batch.add(SETUP_POINTER(game::Scr_FreeThread), "E8 ? ? ? ? 48 8B 4F ? 48 63 81", GRAB_CALL);

		batch.add(SETUP_POINTER(game::Dvar_FindVarByName), "E8 ? ? ? ? 48 8B CB 48 63 50", GRAB_CALL);
		//batch.add(SETUP_POINTER(game::Dvar_GetIntSafe), "E8 ? ? ? ? 8B D0 85 C0 75 ? 38 05", GRAB_CALL);

		if (game_ == "iw8-mod"s)
			batch.add(SETUP_POINTER(game::Dvar_RegisterBool), "E8 ? ? ? ? 48 8B F0 F6 46", GRAB_CALL);
		else
			batch.add(SETUP_POINTER(game::Dvar_RegisterBool), "E8 ? ? AD 00 F6 40 ? 08", GRAB_CALL);

		if (identification::game::is("1.20.4-replay")) {
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4"
				" 48 89 84 24 ? ? ? ? 8B 05");
		}
		else if (identification::game::is_less_or_eq("1.24.0")) {
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ?"
				" 4C 8B F9");
		}
		else {
			if (game_ == "iw9-mod"s) {
				batch.add(SETUP_POINTER(game::Dvar_RegisterVariant_IW9), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ? ? 8B ? B9 ? ? 00 00");
			}
			else {
				// works for IW8, S4, and IW9 technically but its different parameters
				batch.add(SETUP_POINTER(game::Dvar_RegisterVariant),
					"48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ? ? 8B ? B9 ? ? 00 00");
			}
		}

		batch.add(SETUP_POINTER(game::Dvar_SetBool_Internal), "75 1C 48 8B 0D ? ? ? ? B2 01 E8", SETUP_MOD(add(12).rip()));
	}
};

REGISTER_COMPONENT(pointers)
