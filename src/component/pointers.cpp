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

		batch.add(SETUP_POINTER(game::DB_IsXAssetDefault), "48 89 5C 24 ? 48 89 74 24 ? 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ?"
			" 48 8B DA 8B F1");

		batch.add(SETUP_POINTER(game::NetConstStrings_GetIndexPlusOneFromName), "E8 ? ? ? ? 33 C9 84 C0 0F 45 4C 24 ? 89 0B", SETUP_MOD(add(1).rip()));
		batch.add(SETUP_POINTER(game::NetConstStrings_GetNameFromIndexPlusOne), "8B D7 B9 0C 00 00 00 E8 ? ? ? ?? 84 C0 74 ?? ", SETUP_MOD(add(6).rip()));
	}
};

REGISTER_COMPONENT(pointers)
