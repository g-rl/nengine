#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "gpc_engine.hpp"

#include <utils/io.hpp>
#include <utils/string.hpp>

namespace gpc
{
	engine& engine::instance()
	{
		static engine inst;
		return inst;
	}

	void engine::initialize(const std::string& scripts_dir)
	{
		std::lock_guard lock(mutex_);
		scripts_dir_ = scripts_dir;
		initialized_ = true;
		load_scripts();
	}

	void engine::shutdown()
	{
		std::lock_guard lock(mutex_);
		scripts_.clear();
		initialized_ = false;
	}

	void engine::reload()
	{
		std::lock_guard lock(mutex_);
		scripts_.clear();
		if (initialized_)
			load_scripts();
	}

	std::vector<std::string> engine::get_loaded_scripts() const
	{
		std::lock_guard lock(mutex_);
		std::vector<std::string> names;
		for (const auto& s : scripts_)
			names.push_back(s->filename);
		return names;
	}

	void engine::load_scripts()
	{
		if (!utils::io::directory_exists(scripts_dir_))
		{
			printf("[GPC] Scripts directory not found: %s\n", scripts_dir_.c_str());
			utils::io::create_directory(scripts_dir_);
			printf("[GPC] Created scripts directory: %s\n", scripts_dir_.c_str());
			return;
		}

		const auto files = utils::io::list_files(scripts_dir_);
		for (const auto& file : files)
		{
			if (file.size() >= 4 && file.substr(file.size() - 4) == ".gpc")
			{
				load_script_file(file);
			}
		}

		printf("[GPC] Loaded %zu script(s)\n", scripts_.size());
	}

	void engine::load_script_file(const std::string& path)
	{
		printf("[GPC] Loading script: %s\n", path.c_str());

		auto script = std::make_unique<loaded_script>();
		script->filename = path;

		if (!utils::io::read_file(path, &script->source))
		{
			printf("[GPC] Failed to read file: %s\n", path.c_str());
			return;
		}

		try
		{
			// tokenize
			lexer lex;
			script->tokens = lex.tokenize(script->source, path);

			// parse
			parser p;
			script->ast = p.parse(script->tokens, path);

			// create VM
			script->runtime = std::make_unique<vm>();
			script->runtime->load(script->ast.get());

			// run init section
			script->runtime->run_init();

			script->active = true;
			try { script->last_modified = std::filesystem::last_write_time(path); }
			catch (...) {}
			printf("[GPC] Script loaded successfully: %s\n", path.c_str());
		}
		catch (const parse_error& e)
		{
			printf("[GPC] Parse error in %s: %s\n", path.c_str(), e.what());
			return;
		}
		catch (const std::exception& e)
		{
			printf("[GPC] Error loading %s: %s\n", path.c_str(), e.what());
			return;
		}

		scripts_.push_back(std::move(script));
	}

	void engine::check_for_updates()
	{
		static auto last_check = std::chrono::steady_clock::now();
		const auto now = std::chrono::steady_clock::now();

		// only check every 500ms to avoid filesystem spam
		if (now - last_check < std::chrono::milliseconds(500)) return;
		last_check = now;

		for (auto& script : scripts_)
		{
			try
			{
				const auto current_time = std::filesystem::last_write_time(script->filename);
				if (current_time != script->last_modified)
				{
					printf("[GPC] File changed, reloading: %s\n", script->filename.c_str());

					std::string source;
					if (!utils::io::read_file(script->filename, &source)) continue;

					lexer lex;
					auto tokens = lex.tokenize(source, script->filename);

					parser p;
					auto ast = p.parse(tokens, script->filename);

					auto runtime = std::make_unique<vm>();
					runtime->load(ast.get());
					runtime->run_init();

					script->source = std::move(source);
					script->tokens = std::move(tokens);
					script->ast = std::move(ast);
					script->runtime = std::move(runtime);
					script->active = true;
					script->last_modified = current_time;

					printf("[GPC] Reloaded successfully: %s\n", script->filename.c_str());
				}
			}
			catch (const std::exception& e)
			{
				printf("[GPC] Reload error: %s\n", e.what());
			}
		}
	}

	void engine::on_xinput_get_state(DWORD user_index, XINPUT_STATE* state)
	{
		std::lock_guard lock(mutex_);

		check_for_updates();

		for (auto& script : scripts_)
		{
			if (!script->active || !script->runtime) continue;

			// feed XInput state into VM
			xinput_state_to_vm(state, *script->runtime);

			// run one tick
			script->runtime->run_main_tick();

			// write VM output back if there were overrides
			if (script->runtime->has_output_override())
				vm_to_xinput_state(*script->runtime, state);
		}
	}

	void engine::xinput_state_to_vm(const XINPUT_STATE* state, vm& runtime)
	{
		const auto& gp = state->Gamepad;

		// digital buttons (0 or 100)
		runtime.set_input_state(gamepad_input::XB1_A,     (gp.wButtons & XINPUT_GAMEPAD_A) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_B,     (gp.wButtons & XINPUT_GAMEPAD_B) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_X,     (gp.wButtons & XINPUT_GAMEPAD_X) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_Y,     (gp.wButtons & XINPUT_GAMEPAD_Y) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_UP,    (gp.wButtons & XINPUT_GAMEPAD_DPAD_UP) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_DOWN,  (gp.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_LEFT,  (gp.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_RIGHT, (gp.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_LB,    (gp.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_RB,    (gp.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_LS,    (gp.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_RS,    (gp.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_START, (gp.wButtons & XINPUT_GAMEPAD_START) ? 100 : 0);
		runtime.set_input_state(gamepad_input::XB1_BACK,  (gp.wButtons & XINPUT_GAMEPAD_BACK) ? 100 : 0);

		// triggers (0-255 -> 0-100)
		runtime.set_input_state(gamepad_input::XB1_LT, static_cast<int32_t>(gp.bLeftTrigger * 100 / 255));
		runtime.set_input_state(gamepad_input::XB1_RT, static_cast<int32_t>(gp.bRightTrigger * 100 / 255));

		// sticks (-32768 to 32767 -> -100 to 100)
		runtime.set_input_state(gamepad_input::XB1_LX, static_cast<int32_t>(gp.sThumbLX * 100 / 32767));
		runtime.set_input_state(gamepad_input::XB1_LY, static_cast<int32_t>(gp.sThumbLY * 100 / 32767));
		runtime.set_input_state(gamepad_input::XB1_RX, static_cast<int32_t>(gp.sThumbRX * 100 / 32767));
		runtime.set_input_state(gamepad_input::XB1_RY, static_cast<int32_t>(gp.sThumbRY * 100 / 32767));
	}

	void engine::vm_to_xinput_state(const vm& runtime, XINPUT_STATE* state)
	{
		auto& gp = state->Gamepad;

		// helper: set or clear a button bit based on VM output
		auto set_button = [&](gamepad_input input, WORD flag) {
			const int32_t val = runtime.get_output_state(input);
			if (val > 0) gp.wButtons |= flag;
			else gp.wButtons &= ~flag;
		};

		set_button(gamepad_input::XB1_A,     XINPUT_GAMEPAD_A);
		set_button(gamepad_input::XB1_B,     XINPUT_GAMEPAD_B);
		set_button(gamepad_input::XB1_X,     XINPUT_GAMEPAD_X);
		set_button(gamepad_input::XB1_Y,     XINPUT_GAMEPAD_Y);
		set_button(gamepad_input::XB1_UP,    XINPUT_GAMEPAD_DPAD_UP);
		set_button(gamepad_input::XB1_DOWN,  XINPUT_GAMEPAD_DPAD_DOWN);
		set_button(gamepad_input::XB1_LEFT,  XINPUT_GAMEPAD_DPAD_LEFT);
		set_button(gamepad_input::XB1_RIGHT, XINPUT_GAMEPAD_DPAD_RIGHT);
		set_button(gamepad_input::XB1_LB,    XINPUT_GAMEPAD_LEFT_SHOULDER);
		set_button(gamepad_input::XB1_RB,    XINPUT_GAMEPAD_RIGHT_SHOULDER);
		set_button(gamepad_input::XB1_LS,    XINPUT_GAMEPAD_LEFT_THUMB);
		set_button(gamepad_input::XB1_RS,    XINPUT_GAMEPAD_RIGHT_THUMB);
		set_button(gamepad_input::XB1_START, XINPUT_GAMEPAD_START);
		set_button(gamepad_input::XB1_BACK,  XINPUT_GAMEPAD_BACK);

		// triggers (0-100 -> 0-255)
		gp.bLeftTrigger  = static_cast<BYTE>(std::clamp(runtime.get_output_state(gamepad_input::XB1_LT) * 255 / 100, 0, 255));
		gp.bRightTrigger = static_cast<BYTE>(std::clamp(runtime.get_output_state(gamepad_input::XB1_RT) * 255 / 100, 0, 255));

		// sticks (-100 to 100 -> -32768 to 32767)
		gp.sThumbLX = static_cast<SHORT>(std::clamp(runtime.get_output_state(gamepad_input::XB1_LX) * 32767 / 100, -32768, 32767));
		gp.sThumbLY = static_cast<SHORT>(std::clamp(runtime.get_output_state(gamepad_input::XB1_LY) * 32767 / 100, -32768, 32767));
		gp.sThumbRX = static_cast<SHORT>(std::clamp(runtime.get_output_state(gamepad_input::XB1_RX) * 32767 / 100, -32768, 32767));
		gp.sThumbRY = static_cast<SHORT>(std::clamp(runtime.get_output_state(gamepad_input::XB1_RY) * 32767 / 100, -32768, 32767));
	}

	// ── component registration ──

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			// determine game directory and set up gpad_scripts path
			char exe_path[MAX_PATH]{};
			GetModuleFileNameA(nullptr, exe_path, MAX_PATH);

			std::string game_dir = exe_path;
			const auto last_slash = game_dir.find_last_of("\\/");
			if (last_slash != std::string::npos)
				game_dir = game_dir.substr(0, last_slash);

			const auto scripts_path = game_dir + "/gpad_scripts";
			engine::instance().initialize(scripts_path);
		}

		void pre_destroy() override
		{
			engine::instance().shutdown();
		}
	};
}

REGISTER_COMPONENT(gpc::component)
