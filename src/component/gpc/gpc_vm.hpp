#pragma once

#include "gpc_ast.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace gpc
{
	class runtime_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	// controller button/axis identifiers
	enum class gamepad_input : int
	{
		// Xbox One naming convention
		XB1_A = 0, XB1_B, XB1_X, XB1_Y,
		XB1_UP, XB1_DOWN, XB1_LEFT, XB1_RIGHT,
		XB1_LB, XB1_RB,
		XB1_LS, XB1_RS,    // stick clicks
		XB1_LT, XB1_RT,    // triggers (0-100)
		XB1_LX, XB1_LY,    // left stick axes (-100 to 100)
		XB1_RX, XB1_RY,    // right stick axes (-100 to 100)
		XB1_START, XB1_BACK,
		XB1_GUIDE,

		INPUT_COUNT
	};

	struct combo_state
	{
		std::string name;
		const combo_decl_node* decl = nullptr;
		bool running = false;
		size_t pc = 0;
		std::chrono::steady_clock::time_point wait_until{};
		bool waiting = false;

		// held output values that persist during wait()
		int32_t held_values[static_cast<int>(gamepad_input::INPUT_COUNT)]{};
		bool held_active[static_cast<int>(gamepad_input::INPUT_COUNT)]{};
	};

	// signal types for flow control
	struct signal_break {};
	struct signal_continue {};
	struct signal_return { int64_t value = 0; };

	using exec_signal = std::variant<std::monostate, signal_break, signal_continue, signal_return>;

	class vm
	{
	public:
		vm();

		void load(program_node* program);
		void reset();

		// called once after loading
		void run_init();

		// called every tick (main loop iteration)
		void run_main_tick();

		// called to advance combo timers
		void tick_combos();

		// controller I/O
		void set_input_state(gamepad_input input, int32_t value);
		int32_t get_input_state(gamepad_input input) const;
		void set_output_state(gamepad_input input, int32_t value);
		int32_t get_output_state(gamepad_input input) const;

		// get final output values (input + overrides)
		void get_output_values(int32_t* out, size_t count) const;

		// remap table access
		bool has_output_override() const { return has_output_override_; }

		// error reporting
		std::string last_error() const { return last_error_; }

	private:
		program_node* program_ = nullptr;

		// variable scopes
		struct scope
		{
			std::unordered_map<std::string, int64_t> vars;
			scope* parent = nullptr;

			bool get(const std::string& name, int64_t& out) const;
			void set(const std::string& name, int64_t value);
			bool has_local(const std::string& name) const;
		};

		scope global_scope_;
		scope* current_scope_ = nullptr;

		// defines (compile-time constants)
		std::unordered_map<std::string, int64_t> defines_;

		// data section
		std::vector<int64_t> data_section_;

		// const arrays
		std::unordered_map<std::string, std::vector<int64_t>> const_arrays_;

		// functions
		std::unordered_map<std::string, const function_decl_node*> functions_;

		// combos
		std::unordered_map<std::string, combo_state> combos_;
		combo_state* active_combo_ = nullptr; // combo currently being stepped

		// remapping
		std::unordered_map<int, int> remap_table_; // from -> to

		// controller state
		int32_t input_values_[static_cast<int>(gamepad_input::INPUT_COUNT)]{};
		int32_t output_values_[static_cast<int>(gamepad_input::INPUT_COUNT)]{};
		bool output_overridden_[static_cast<int>(gamepad_input::INPUT_COUNT)]{};
		bool has_output_override_ = false;

		// sections
		const main_section_node* main_section_ = nullptr;
		const init_section_node* init_section_ = nullptr;

		// built-in function table
		using builtin_fn = std::function<int64_t(const std::vector<int64_t>&)>;
		std::unordered_map<std::string, builtin_fn> builtins_;

		// error state
		std::string last_error_;

		// execution
		exec_signal exec_block(const block_stmt* block);
		exec_signal exec_stmt(const stmt* s);
		int64_t eval_expr(const expr* e);

		// helpers
		int resolve_input_name(const std::string& name) const;
		void register_builtins();

		// combo execution
		void step_combo(combo_state& combo);
	};
}
