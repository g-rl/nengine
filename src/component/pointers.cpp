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

		if (game_ == "iw9-mod"s)
		{
			batch.add(SETUP_POINTER(game::DB_FindXAssetHeader_IW9), "45 8B C8 45 33 C0 E9 ? 00 00 00"); // uses std::uint64_t duplicate for a2 name
		}
		
		batch.add(SETUP_POINTER(game::DB_FindXAssetHeader), "E8 ? ? ? ? 44 8B C5 8D 4D", GRAB_CALL); // this resolves for IW9 for the real one

		batch.add(SETUP_POINTER(game::DB_IsXAssetDefault), "E8 ? ? ? FF ? C0 75 0B 48 8B ? 48 8B CF E8 ? 00 00 00", GRAB_CALL); // inside ProcessScriptFile (IW8, S4, IW9)
		batch.add(SETUP_POINTER(game::DB_XAssetExists), 
			"B9 ? 00 00 00 E8 ? ? ? 00 ? C0 0F 84 ? ? 00 00 45 33 C0 ? 89 ? ? ? 01 00 00 48",
			SETUP_MOD(add(6).rip()));

		batch.add(SETUP_POINTER(game::NetConstStrings_GetIndexPlusOneFromName), "B9 ? 00 00 00 E8 ? ? ? ? 33 C9 84 C0 0F 45 4C 24 ? 89 0B", SETUP_MOD(add(6).rip()));
		batch.add(SETUP_POINTER(game::NetConstStrings_GetNameFromIndexPlusOne), "B9 0C 00 00 00 E8 ? ? ? ?? 84 C0 74 ? 48 ? ? ? ? B2", SETUP_MOD(add(6).rip()));

		batch.add(SETUP_POINTER(game::Com_FrontEnd_IsInFrontEnd), "E8 ? ? ? 00 84 C0 74 ? E8 ? ? FF FF 84 C0 75 ? E8", GRAB_CALL);

		batch.add(SETUP_POINTER(game::ScriptContext_Server), "48 83 EC ? E8 ? ? ? ? 48 8B F0 48 8D ? ? ? ? ? 33 ? ? 8B", SETUP_MOD(add(5).rip()));

		if (game_ == "s4-mod"s)
		{
			// no clue
			batch.add(SETUP_POINTER(game::Scr_LoadScript), "40 53 55 56 57 41 54 48 81 EC 70 04 00 00 48 8B 05 ? ? ? ? 48");
		}
		else
		{
			batch.add(SETUP_POINTER(game::Scr_LoadScript), "48 8B DA 48 8B F9 BA ?? ?? 00 00 48 8D 4C 24 ?? E8 ?? ?? ?? FF 4C 8B", [](memory::scanned_result<void> r) {
				std::uintptr_t offset = 0;
				while (true) {
					offset++;
					auto buf = r.sub(offset).as<std::uint8_t*>();
					if (buf[0] == 0x48 && buf[1] == 0x89 && buf[2] == 0x5C && buf[3] == 0x24) {
						break;
					}
				}
				return r.sub(offset);
				});
		}

		// this is the bottom of the function, and we need to make something to scan up and find the beginning of it
		batch.add(SETUP_POINTER(game::Scr_GetFunctionHandle), "49 2B D0 48 3B D1 73 ? 41 2B C0", [](memory::scanned_result<void> r) {
			std::uintptr_t offset = 0;
			while (true) {
				offset++;
				auto buf = r.sub(offset).as<std::uint8_t*>();
				if (buf[0] == 0x48 && buf[1] == 0x89 && buf[2] == 0x5C && buf[3] == 0x24) {
					break;
				}
			}
			return r.sub(offset);
			});

		//Scr_LoadScript_IW9
		if (game_ == "iw9-mod"s)
		{
			batch.add(SETUP_POINTER(game::Scr_LoadScript_IW9), "48 8B DA 48 8B F9 BA ?? ?? 00 00 48 8D 4C 24 ?? E8 ?? ?? ?? FF 4C 8B", [](memory::scanned_result<void> r) {
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

			// this is the bottom of the function, and we need to make something to scan up and find the beginning of it
			batch.add(SETUP_POINTER(game::Scr_GetFunctionHandle_IW9), "49 2B D0 48 3B D1 73 ? 41 2B C0", [](memory::scanned_result<void> r) {
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
		}

		batch.add(SETUP_POINTER(game::Scr_ExecThread), "48 83 EC ? 33 C0 45 8B C8");
		if (game_ == "s4-mod"s)
			batch.add(SETUP_POINTER(game::Scr_FreeThread), "E8 ? ? ? ? 48 8B 4F ? 48 63 81 ? ? ? ? 48 63 91", GRAB_CALL);
		else
			batch.add(SETUP_POINTER(game::Scr_FreeThread), "E8 ? ? ? ? 48 8B 4F ? 48 63 81", GRAB_CALL);

		if (game_ == "iw8-mod"s)
			batch.add(SETUP_POINTER(game::Dvar_FindVarByName), "E8 ? ? ? ? 48 8B CB 48 63 50", GRAB_CALL);
		else
			batch.add(SETUP_POINTER(game::Dvar_FindVarByName_IW9), "E8 ? ? ? ? 48 8B CB 48 63 50", GRAB_CALL);

		if (game_ == "iw8-mod"s)
			batch.add(SETUP_POINTER(game::Dvar_RegisterBool_), "E8 ? ? ? ? 48 8B F0 F6 46", GRAB_CALL);
		else if (game_ == "s4-mod"s)
			batch.add(SETUP_POINTER(game::Dvar_RegisterBool_IW9), "E8 ? ? ? ? 48 89 05 ? ? ? ? 48 83 C4 ? 5F E9", SETUP_MOD(add(1).rip()));
		else // s4 & iw9
			batch.add(SETUP_POINTER(game::Dvar_RegisterBool_IW9), "E8 ? ? ? ? C5 FA 10 0D ? ? ? ? C5 FA 10 1D ? ? ? ? C5 F8 28 D1", GRAB_CALL);

		if (game_ == "iw8-mod"s)
		{
			if (identification::game::is("1.20.4-replay")) {
				batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "8B D0 48 8B CE E8 ? ? 00 00 48 8B 5C 24 70 48",
					SETUP_MOD(add(6).rip()));
			}
			else if (identification::game::is_less_or_eq("1.24.0")) {
				batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ?"
					" 4C 8B F9");
			}
			else {
				// every other version
				batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ? ? 8B ? B9 ? ? 00 00");
			}

			batch.add(SETUP_POINTER(game::Dvar_SetBool_Internal_IW8), "B2 01 48 83 C4 28 E9 ? ? ? 00 48 83 C4 28 C3", SETUP_MOD(add(7).rip()));
		}
		else if (game_ == "s4-mod"s)
		{
			// 48 89 44 24 20 ? 8B ? 33 D2 C5 F8 77 E8 ? ? ? ? 4C 8D 5C 24 60
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "E8 ? ? ? ? EB ? 41 B8 ? ? ? ? 48 8B D6 48 8B CF", SETUP_MOD(add(1).rip()));
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant_IW9), "E8 ? ? ? ? EB ? 41 B8 ? ? ? ? 48 8B D6 48 8B CF", SETUP_MOD(add(1).rip()));
			batch.add(SETUP_POINTER(game::Dvar_SetBool_Internal_IW9), "B2 01 48 83 C4 28 E9 ? ? ? 00 48 83 C4 28 C3", SETUP_MOD(add(7).rip()));
		}
		else {
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ? ? 8B ? B9 ? ? 00 00");
			batch.add(SETUP_POINTER(game::Dvar_RegisterVariant_IW9), "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 54 41 56 41 57 48 83 EC ? 8B 05 ? ? ? ? ? 8B ? B9 ? ? 00 00");
			batch.add(SETUP_POINTER(game::Dvar_SetBool_Internal_IW9), "B2 01 48 83 C4 28 E9 ? ? ? 00 48 83 C4 28 C3", SETUP_MOD(add(7).rip()));
		}
	
		// same func
		batch.add(SETUP_POINTER(game::Core_strcpy),
			"24 40 49 81 C0 ? ? 00 00 BA 40 00 00 00 E8", SETUP_MOD(add(15).rip()));
		batch.add(SETUP_POINTER(game::I_CleanStr),
			"24 40 49 81 C0 ? ? 00 00 BA 40 00 00 00 E8", SETUP_MOD(add(25).rip()));

		if (game_ == "iw8-mod"s)
		{
			if (identification::game::is("1.20.4-replay"))
				batch.add(SETUP_POINTER(game::FindVariable), "E8 ? ? 00 00 8B ? 85 C0 75 ? 48 8D ? ? ? ? ? 8D ? 01 E8", GRAB_CALL);
			else // every other iw8 version uses this
				batch.add(SETUP_POINTER(game::FindVariable), "E8 ? ? ? 00 8B ? 85 C0 75 15 41 B8 75 04 00 00 48", GRAB_CALL);
		
			batch.add(SETUP_POINTER(game::SL_GetString), "C6 40 08 02 E8 ? ? ? ? 89 03 48", SETUP_MOD(add(5).rip()));
		}
		else if (game_ == "iw9-mod"s)	// IW9: E8 ? ? 00 00 8B ? 85 C0 75 ? 48 8D ? ? ? ? ? 8D ? 01 E8
			batch.add(SETUP_POINTER(game::FindVariable_IW9), "E8 ? ? ? 00 8B ? 85 C0 75 15 41 B8 75 04 00 00 48", GRAB_CALL);
		else if (game_ == "s4-mod"s)	// S4: uses IW8 sig, has IW9 definition
			batch.add(SETUP_POINTER(game::FindVariable_IW9), "E8 ? ? ? 00 8B ? 85 C0 75 15 41 B8 75 04 00 00 48", GRAB_CALL);

		batch.add(SETUP_POINTER(game::SL_ConvertToString), "E8 ? ? ? ? 45 33 F6 4C 8B E0", GRAB_CALL);

		batch.add(SETUP_POINTER(game::Sys_Milliseconds), "E8 ? ? ? 00 89 87 ? ? 00 00 FF 87", GRAB_CALL);

	}
};

REGISTER_COMPONENT(pointers)
