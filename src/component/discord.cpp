#include <std_include.hpp>
/*
#include "loader/component_loader.hpp"

#include "command.hpp"
//#include "discord.hpp"
#include "scheduler.hpp"

#include "game/game.hpp"

#include <utils/string.hpp>
#include <utils/cryptography.hpp>

#include <discord_rpc.h>

namespace discord
{
	namespace
	{
		struct discord_presence_state_t
		{
			int start_timestamp;
			int party_size;
			int party_max;
		};

		struct discord_presence_strings_t
		{
			std::string state;
			std::string details;
			std::string small_image_key;
			std::string small_image_text;
			std::string large_image_key;
			std::string large_image_text;
			std::string party_id;
			std::string join_secret;
		};

		DiscordRichPresence discord_presence{};
		discord_presence_strings_t discord_strings;

		void update_discord_frontend()
		{
			discord_presence.details = game::G_GAME_MODE_STRINGS_FORMATTED[game::Com_GameMode_GetActiveGameMode()];
			if (game::Com_GameMode_GetActiveGameMode() == game::GAME_MODE_NONE)
			{
				discord_presence.details = game::G_GAME_MODE_STRINGS_FORMATTED[game::GAME_MODE_MP];
			}

			discord_presence.startTimestamp = 0;

			discord_presence.state = "Main Menu";
			discord_presence.largeImageKey = "neura_image";
			//}

			Discord_UpdatePresence(&discord_presence);
		}

		void update_discord_ingame()
		{
			static const game::dvar_t* mapname_dvar = nullptr;
			static const game::dvar_t* gametype_dvar = nullptr;
			static const game::dvar_t* max_clients_dvar = nullptr;

			if (!mapname_dvar) mapname_dvar = game::Dvar_FindVar("ui_mapname");
			if (!gametype_dvar) gametype_dvar = game::Dvar_FindVar("ui_gametype");
			if (!max_clients_dvar) max_clients_dvar = game::Dvar_FindVar("ui_maxclients");

			static std::string mapname_str = "mp_frontend";
			const char* mapname = mapname_str.c_str();

			if (mapname_dvar && strcmp(mapname, mapname_dvar->current.string) != 0)
			{
				mapname_str = utils::string::copy(mapname_dvar->current.string, std::strlen(mapname_dvar->current.string));
				mapname = mapname_str.c_str();
			}

			discord_strings.large_image_key = mapname;

			const auto mode = game::Com_GameMode_GetActiveGameMode();

			if (mode == game::GAME_MODE_CP || mode == game::GAME_MODE_MP)
			{
				const auto* gametype_ui = game::UI_GetGameTypeDisplayName(gametype_dvar != nullptr ? gametype_dvar->current.string : "");
				const auto* mapname_ui = game::UI_GetMapDisplayName(mapname);

				discord_strings.details = std::format("{} on {}", gametype_ui, mapname_ui);

				discord_presence.partySize = *reinterpret_cast<int*>(0x14434FEF0); // probably numClients from snapshot

				if (game::SV_Loaded() && !game::Com_FrontEnd_IsInFrontEnd())
				{
					discord_strings.state = "Private Match";
					discord_presence.partyMax = (max_clients_dvar ? max_clients_dvar->current.integer : 12);
					discord_presence.partyPrivacy = DISCORD_PARTY_PRIVATE;
				}
				else
				{
					auto* server_connection_state = party::get_server_connection_state();

					discord_strings.state = utils::string::strip(server_connection_state->hostname);

					const auto server_ip_port = std::format("{}.{}.{}.{}:{}",
						static_cast<int>(server_connection_state->host.ip[0]),
						static_cast<int>(server_connection_state->host.ip[1]),
						static_cast<int>(server_connection_state->host.ip[2]),
						static_cast<int>(server_connection_state->host.ip[3]),
						static_cast<int>(ntohs(server_connection_state->host.port))
					);

					discord_strings.party_id = utils::cryptography::sha1::compute(server_ip_port, true).substr(0, 8);
					discord_presence.partyMax = server_connection_state->max_clients;
					discord_presence.partyPrivacy = DISCORD_PARTY_PUBLIC;
					discord_strings.join_secret = server_ip_port;
				}

				auto server_discord_info = party::get_server_discord_info();
				if (server_discord_info.has_value())
				{
					discord_strings.small_image_key = server_discord_info->image;
					discord_strings.small_image_text = server_discord_info->image_text;
				}
			}
			else if (mode == game::GAME_MODE_SP)
			{
				discord_strings.details = mapname;
			}

			if (discord_presence.startTimestamp == 0)
			{
				discord_presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
					std::chrono::system_clock::now().time_since_epoch()).count();
			}

			discord_presence.state = discord_strings.state.data();
			discord_presence.details = discord_strings.details.data();
			discord_presence.smallImageKey = discord_strings.small_image_key.data();
			discord_presence.smallImageText = discord_strings.small_image_text.data();
			discord_presence.largeImageKey = discord_strings.large_image_key.data();
			discord_presence.largeImageText = discord_strings.large_image_text.data();
			discord_presence.partyId = discord_strings.party_id.data();
			discord_presence.joinSecret = discord_strings.join_secret.data();

			Discord_UpdatePresence(&discord_presence);
		}

		void update_discord()
		{
			const auto saved_time = discord_presence.startTimestamp;
			discord_presence = {};
			discord_presence.startTimestamp = saved_time;

			if (game::Com_FrontEnd_IsInFrontEnd())
			{
				update_discord_frontend();
			}
			else
			{
				update_discord_ingame();
			}
		}

		void ready(const DiscordUser* request)
		{
			DiscordRichPresence presence{};
			presence.instance = 1;
			presence.state = "";
			printf("Discord: Ready on %s (%s)\n", request->username, request->userId);
			Discord_UpdatePresence(&presence);
		}

		void errored(const int error_code, const char* message)
		{
			printf("Discord: %s (%i)\n", message, error_code);
		}

		void join_request(const DiscordUser* request)
		{
			Discord_Respond(request->userId, DISCORD_REPLY_IGNORE);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			DiscordEventHandlers handlers{};
			handlers.ready = ready;
			handlers.errored = errored;
			handlers.disconnected = errored;
			handlers.spectateGame = nullptr;
			handlers.joinGame = nullptr;;
			handlers.joinRequest = join_request;

			Discord_Initialize("1215500480873103400", &handlers, 1, nullptr);

			scheduler::loop(Discord_RunCallbacks, scheduler::async, 500ms);
			scheduler::loop(update_discord, scheduler::async, 5s);

			initialized_ = true;
		}

		void pre_destroy() override
		{
			if (!initialized_)
			{
				return;
			}

			Discord_Shutdown();
		}

	private:
		bool initialized_ = false;
	};
}
*/

//REGISTER_COMPONENT(discord::component)
