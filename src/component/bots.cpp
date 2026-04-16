#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "command.hpp"
#include "scheduler.hpp"
#include "scripting.hpp"
#include "call_spoofer.hpp"

#include "game/game.hpp"
#include <identification/game.hpp>

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/io.hpp>

namespace bots
{
	namespace
	{
		utils::hook::detour get_bot_name_hook;
		std::vector<std::string> bot_names{};

		void load_bot_data()
		{
			static const char* bots_txt = "neura/bots.txt";

			std::string bots_content;
			if (!utils::io::read_file(bots_txt, &bots_content))
			{
				// add cool people n contributors
				bot_names = {
					"mjkzy", "alicealys", "Joelrau",
					"momo5502", "skkuull", "yoyothebest",
					"Lierrmm", "Wanted", "January",
					"diamante0018", "efinst0rm"
				};

				return;
			}

			auto names = utils::string::split(bots_content, '\n');
			for (auto& name : names)
			{
				name = utils::string::replace(name, "\r", "");
				if (!name.empty())
				{
					bot_names.emplace_back(name);
				}
			}
		}

		size_t bot_id = 0;

		const char* get_random_bot_name()
		{
			if (bot_names.empty())
			{
				load_bot_data();
			}

			// only use bot names once, no dupes in names
			if (!bot_names.empty() && bot_id < bot_names.size())
			{
				bot_id %= bot_names.size();
				const auto& entry = bot_names.at(bot_id++);
				return utils::string::va("%.*s", static_cast<int>(entry.size()), entry.data());
			}

			return call_spoofer::spoof_hook_invoke<const char*>(get_bot_name_hook);
		}

		utils::hook::detour sv_kick_client_num_hook;
		void sv_kick_client_num_stub(const int client_num, const char* reason, bool kicked_for_inactivity)
		{
			if (!strcmp(reason, "EXE_PLAYERKICKED_BOT_BALANCE"))
			{
				return;
			}

			sv_kick_client_num_hook.invoke<void>(client_num, reason, kicked_for_inactivity);
		}

		utils::hook::detour SV_ClientMP_ConnectBot_hook;
		void* SV_ClientMP_ConnectBot_call(void* result, const char* name, const int headModelIndex, const int bodyModelIndex, __int64 lol)
		{
			if (bot_names.empty())
			{
				load_bot_data();
			}

			// only use bot names once, no dupes in names
			if (!bot_names.empty() && bot_id < bot_names.size())
			{
				bot_id %= bot_names.size();
				const auto& entry = bot_names.at(bot_id++);
				name = ("%.*s", static_cast<int>(entry.size()), entry.data());
			}

			return SV_ClientMP_ConnectBot_hook.invoke<void*>(result, name, headModelIndex, bodyModelIndex, lol);
		}
	}

	class component final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw9-mod"s)
				batch.add(SETUP_POINTER(game::SV_ClientMP_ConnectBot),
					"C5 ? ? E8 ? ? 00 00 48 ? ? ? ? ? ? ? 4C ? ? ? ? ? ? C5", SETUP_MOD(add(4).rip()));
			else
				batch.add(SETUP_POINTER(game::SV_BotGetRandomName), 
					"48 8B C4 48 83 EC 48 83 3D ? ? ? ? 00 0F 8C ? ? 00 00 48 8B 0D");
		}

		void post_unpack() override
		{
			// don't kick bot to equalize team balance
			//sv_kick_client_num_hook.create(game::SV_CmdsMP_KickClientNum, sv_kick_client_num_stub);

			static const auto& game_ = identification::game::get_target_game().client_name;
			if (game_ == "iw9-mod"s)
				SV_ClientMP_ConnectBot_hook.create(game::SV_ClientMP_ConnectBot, SV_ClientMP_ConnectBot_call);
			else
				get_bot_name_hook.create(game::SV_BotGetRandomName, get_random_bot_name);

			// clear bot names and reset ID on game shutdown to allow new names to be added without restarting
			scripting::on_shutdown([](bool /*free_scripts*/, bool post_shutdown)
			{
				if (!post_shutdown)
				{
					bot_names.clear();
					bot_id = 0;
				}
			});
		}
	};
}

REGISTER_COMPONENT(bots::component)
