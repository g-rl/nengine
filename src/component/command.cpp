#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "command.hpp"

#include "game/game.hpp"
#include <identification/game.hpp>
//#include "game/dvars.hpp"

//#include "console.hpp"
//#include "game_console.hpp"
#include "scheduler.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/memory.hpp>
#include <utils/io.hpp>

namespace command
{
	namespace
	{
		std::unordered_map<std::string, std::function<void()>> handlers;

		void main_handler()
		{
			const auto command = utils::string::to_lower(game::Cmd_Argv(0));
			if (handlers.find(command) != handlers.end())
			{
				handlers[command]();
			}
		}
	}

	void add_raw(const char* name, void (*callback)())
	{
		game::Cmd_AddCommandInternal(name, callback, utils::memory::get_allocator()->allocate<game::cmd_function_s>());
	}

	void add(const char* name, const std::function<void()>& callback)
	{
		const auto command = utils::string::to_lower(name);

		if (handlers.find(command) == handlers.end())
			add_raw(name, main_handler);

		handlers[command] = callback;
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (identification::game::is("1.20.4-replay"))
			{
				add("map_restart", []()
				{
					auto SV_CmdsMP_RequestMapRestart = reinterpret_cast<void(*)(bool load_scripts, bool migrate)>(0x136C310_b);
					SV_CmdsMP_RequestMapRestart(1, 0);
				});

				add("test", []()
				{
					printf("test\n");
				});
			}
		}
	};
}

REGISTER_COMPONENT(command::component)
