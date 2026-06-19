#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "call_spoofer.hpp"
#include "scheduler.hpp"
#include "game/game.hpp"
#include <identification/game.hpp>

#include <utils/hook.hpp>
#include <utils/concurrency.hpp>
#include <utils/string.hpp>
#include <utils/thread.hpp>

namespace scheduler
{
	namespace
	{
		struct task
		{
			std::function<bool()> handler{};
			std::chrono::milliseconds interval{};
			std::chrono::high_resolution_clock::time_point last_call{};
		};

		using task_list = std::vector<task>;

		class task_pipeline
		{
		public:
			void add(task&& task)
			{
				new_callbacks_.access([&task](task_list& tasks)
				{
					tasks.emplace_back(std::move(task));
				});
			}

			void execute()
			{
				callbacks_.access([&](task_list& tasks)
				{
					this->merge_callbacks();

					for (auto i = tasks.begin(); i != tasks.end();)
					{
						const auto now = std::chrono::high_resolution_clock::now();
						const auto diff = now - i->last_call;

						if (diff < i->interval)
						{
							++i;
							continue;
						}

						i->last_call = now;

						const auto res = i->handler();
						if (res == cond_end)
						{
							i = tasks.erase(i);
						}
						else
						{
							++i;
						}
					}
				});
			}

		private:
			utils::concurrency::container<task_list> new_callbacks_;
			utils::concurrency::container<task_list, std::recursive_mutex> callbacks_;

			void merge_callbacks()
			{
				callbacks_.access([&](task_list& tasks)
				{
					new_callbacks_.access([&](task_list& new_tasks)
					{
							tasks.insert(tasks.end(), std::move_iterator<task_list::iterator>(new_tasks.begin()),
								std::move_iterator<task_list::iterator>(new_tasks.end()));
						new_tasks = {};
					});
				});
			}
		};

		volatile bool kill = false;
		std::thread thread;
		task_pipeline pipelines[pipeline::count];
		utils::hook::detour r_end_frame_hook;
		utils::hook::detour main_frame_hook;

		void execute(const pipeline type)
		{
			assert(type >= 0 && type < pipeline::count);
			pipelines[type].execute();
		}

		void r_end_frame_stub()
		{
			execute(pipeline::renderer);
			call_spoofer::spoof_hook_invoke<void>(r_end_frame_hook);
		}

		void main_frame_stub()
		{
			call_spoofer::spoof_hook_invoke<void>(main_frame_hook);
			execute(pipeline::main);
		}
	}

	void schedule(const std::function<bool()>& callback, const pipeline type,
	              const std::chrono::milliseconds delay)
	{
		assert(type >= 0 && type < pipeline::count);

		task task;
		task.handler = callback;
		task.interval = delay;
		task.last_call = std::chrono::high_resolution_clock::now();

		pipelines[type].add(std::move(task));
	}

	void loop(const std::function<void()>& callback, const pipeline type,
	          const std::chrono::milliseconds delay)
	{
		schedule([callback]()
		{
			callback();
			return cond_continue;
		}, type, delay);
	}

	void once(const std::function<void()>& callback, const pipeline type,
	          const std::chrono::milliseconds delay)
	{
		schedule([callback]()
		{
			callback();
			return cond_end;
		}, type, delay);
	}

	void on_game_initialized(const std::function<void()>& callback, const pipeline type,
	                         const std::chrono::milliseconds delay)
	{
		schedule([=]()
		{
			const auto data_flags = game::Live_SyncOnlineDataFlags(0);
			printf("data_flags: %d\n", data_flags);
			const auto dw_init = data_flags == 0;
			if (dw_init && game::Sys_IsDatabaseReady())
			{
				once(callback, type, delay);
				return cond_end;
			}

			return cond_continue;
		}, pipeline::main);
	}

	class component final : public component_interface
	{
	public:
		void find_signatures(memory::signature_store& batch) override
		{
			/*
			if (identification::game::is_less_or_eq("1.36.1") || identification::game::is("1.20.4-replay")) {
				batch.add(SETUP_POINTER(game::R_EndFrame), "48 8B 15 ? ? ? ? 45 33 D2 4C 8B 0D");
			}
			else {
				batch.add(SETUP_POINTER(game::R_EndFrame), "48 83 EC ? E8 ? ? ? ? 48 8B 15 ? ? ? ? 45 33 D2");
			}
			*/

			static const auto& game_ = identification::game::get_target_game().client_name;

			if (game_ == "iw9-mod"s)
			{
				// slightly different on IW9, so we just add known bytes instead
				batch.add(SETUP_POINTER(game::R_EndFrame), "E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? FF 48 8B ? ? ? 84 C0 74", GRAB_CALL);
			}
			else
			{
				// works on all IW8 & S4
				batch.add(SETUP_POINTER(game::R_EndFrame), "E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? FF 48 8B ? ? ? 01", GRAB_CALL);
			}

			if (identification::game::is("1.20.4-replay"))
			{
				batch.add(SETUP_POINTER(game::FenceManager_Frame), "E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8", GRAB_CALL);
			}
			else
			{
				if (game_ == "s4-mod"s) // add 8B to end of sig for S4
					batch.add(SETUP_POINTER(game::FenceManager_Frame), "48 89 5C 24 20 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 20 33 ED");
				else if (game_ == "iw9-mod"s) // IW9 is missing 2 calls, but we change it anyways on here to be arxan safe
					batch.add(SETUP_POINTER(game::FenceManager_Frame), "8B CB E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? 8B D3", SETUP_MOD(add(8).rip()));
				else
					batch.add(SETUP_POINTER(game::FenceManager_Frame), "E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? E8 ? ? ? ? E8", GRAB_CALL);
			}
		}

		void post_start() override
		{
			thread = utils::thread::create_named_thread("Async Scheduler", []()
			{
				while (!kill)
				{
					execute(pipeline::async);
					std::this_thread::sleep_for(10ms);
				}
			});
		}

		void post_unpack() override
		{
			if (game::R_EndFrame)
				r_end_frame_hook.create(game::R_EndFrame, r_end_frame_stub);

			if (game::FenceManager_Frame)
				main_frame_hook.create(game::FenceManager_Frame, main_frame_stub);
		}

		void pre_destroy() override
		{
			kill = true;
			if (thread.joinable())
			{
				thread.join();
			}
		}
	};
}

REGISTER_COMPONENT(scheduler::component)
