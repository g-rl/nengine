#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include <identification/game.hpp>

#include "component/scripting.hpp"

#include "script_error.hpp"
#include "script_extension.hpp"
#include "script_loading.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

namespace gsc
{
	std::unordered_map<std::string, uint64_t> cached_names;

	utils::hook::detour Scr_PeekXHashDvar_hook;
	uint64_t* Scr_PeekXHashDvar_stub(uint64_t* a1, __int64 scr_context, unsigned int index)
	{
		if (index < *(int*)(scr_context + 49732))
		{
			auto v6 = *(uint64_t*)(scr_context + 49720) - 16LL * index; // gets the variable at the index
			auto variable_type = *(BYTE*)(v6 + 8);

			// 9 is dvar hash for IW9 (11 on JUP), the normal game logic
			if (variable_type == 9)
			{
				*a1 = *(uint64_t*)v6;
				return a1;
			}

			// CastString should be used, but we aren't doing lazy casting and expect only strings
			if (variable_type == 2)
			{
				// get string value easily
				auto dvar_name_raw = game::Scr_GetString((game::scrContext_t*)scr_context, 0);
				if (cached_names.contains(dvar_name_raw))
				{
					*a1 = cached_names[dvar_name_raw];
					return a1;
				}

				// convert it to hash, and then store in cache
				auto hash = game::hash_scr_dvar_iw9(dvar_name_raw);
				printf("caching dvar -> hash for \"%s\" (%llx)\n", dvar_name_raw, hash);
				cached_names.insert_or_assign(dvar_name_raw, hash);

				*a1 = hash;
				return a1;
			}
		}

		return Scr_PeekXHashDvar_hook.invoke<uint64_t*>(a1, scr_context, index);
	}

	class extension final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ != "iw9-mod"s)
				return;

			batch.add(SETUP_POINTER(game::Scr_PeekXHashDvar), "48 8D ? ? ? 48 8B D3 E8 ? ? ? ? 48 8B D7 48 8B 08 48",
				SETUP_MOD(add(9).rip()));

			batch.add(SETUP_POINTER(game::Scr_GetString), "48 83 EC ? 44 8B C2 48 8B D1 48 ? ? ? ? E8 ? ? ? ? 8B 08 E8");
		}

		void post_unpack() override
		{
			Scr_PeekXHashDvar_hook.create(game::Scr_PeekXHashDvar, Scr_PeekXHashDvar_stub);
		}
	};
}

REGISTER_COMPONENT(gsc::extension)
