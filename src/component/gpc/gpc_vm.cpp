#include <std_include.hpp>
#include "gpc_vm.hpp"

#include <cstring>
#include <algorithm>
#include <cmath>

namespace gpc
{
	// ── scope ──

	bool vm::scope::get(const std::string& name, int64_t& out) const
	{
		auto it = vars.find(name);
		if (it != vars.end()) { out = it->second; return true; }
		if (parent) return parent->get(name, out);
		return false;
	}

	void vm::scope::set(const std::string& name, int64_t value)
	{
		// walk up scopes to find existing variable
		for (auto* s = this; s; s = s->parent)
		{
			auto it = s->vars.find(name);
			if (it != s->vars.end())
			{
				it->second = value;
				return;
			}
		}
		// new variable in current scope
		vars[name] = value;
	}

	bool vm::scope::has_local(const std::string& name) const
	{
		return vars.find(name) != vars.end();
	}

	// ── input name resolution ──

	static const std::unordered_map<std::string, int>& get_input_name_map()
	{
		static const std::unordered_map<std::string, int> map = {
			{"XB1_A",     static_cast<int>(gamepad_input::XB1_A)},
			{"XB1_B",     static_cast<int>(gamepad_input::XB1_B)},
			{"XB1_X",     static_cast<int>(gamepad_input::XB1_X)},
			{"XB1_Y",     static_cast<int>(gamepad_input::XB1_Y)},
			{"XB1_UP",    static_cast<int>(gamepad_input::XB1_UP)},
			{"XB1_DOWN",  static_cast<int>(gamepad_input::XB1_DOWN)},
			{"XB1_LEFT",  static_cast<int>(gamepad_input::XB1_LEFT)},
			{"XB1_RIGHT", static_cast<int>(gamepad_input::XB1_RIGHT)},
			{"XB1_LB",    static_cast<int>(gamepad_input::XB1_LB)},
			{"XB1_RB",    static_cast<int>(gamepad_input::XB1_RB)},
			{"XB1_LS",    static_cast<int>(gamepad_input::XB1_LS)},
			{"XB1_RS",    static_cast<int>(gamepad_input::XB1_RS)},
			{"XB1_LT",    static_cast<int>(gamepad_input::XB1_LT)},
			{"XB1_RT",    static_cast<int>(gamepad_input::XB1_RT)},
			{"XB1_LX",    static_cast<int>(gamepad_input::XB1_LX)},
			{"XB1_LY",    static_cast<int>(gamepad_input::XB1_LY)},
			{"XB1_RX",    static_cast<int>(gamepad_input::XB1_RX)},
			{"XB1_RY",    static_cast<int>(gamepad_input::XB1_RY)},
			{"XB1_START", static_cast<int>(gamepad_input::XB1_START)},
			{"XB1_BACK",  static_cast<int>(gamepad_input::XB1_BACK)},
			{"XB1_GUIDE", static_cast<int>(gamepad_input::XB1_GUIDE)},
		};
		return map;
	}

	int vm::resolve_input_name(const std::string& name) const
	{
		const auto& map = get_input_name_map();
		auto it = map.find(name);
		if (it != map.end()) return it->second;

		// check defines
		auto dit = defines_.find(name);
		if (dit != defines_.end()) return static_cast<int>(dit->second);

		return -1;
	}

	// ── constructor ──

	vm::vm()
	{
		current_scope_ = &global_scope_;
		register_builtins();
	}

	void vm::register_builtins()
	{
		// get_val(input_id) - read current input value
		builtins_["get_val"] = [this](const std::vector<int64_t>& args) -> int64_t {
			if (args.empty()) throw runtime_error("get_val requires 1 argument");
			const int idx = static_cast<int>(args[0]);
			if (idx < 0 || idx >= static_cast<int>(gamepad_input::INPUT_COUNT)) return 0;
			return input_values_[idx];
		};

		// set_val(input_id, value) - override output value
		builtins_["set_val"] = [this](const std::vector<int64_t>& args) -> int64_t {
			if (args.size() < 2) throw runtime_error("set_val requires 2 arguments");
			const int idx = static_cast<int>(args[0]);
			if (idx < 0 || idx >= static_cast<int>(gamepad_input::INPUT_COUNT)) return 0;
			output_values_[idx] = static_cast<int32_t>(args[1]);
			output_overridden_[idx] = true;
			has_output_override_ = true;
			// if called from a combo, store as held value for wait() persistence
			if (active_combo_)
			{
				active_combo_->held_values[idx] = static_cast<int32_t>(args[1]);
				active_combo_->held_active[idx] = true;
			}
			return 0;
		};

		// combo_run(combo_id) - start a combo
		builtins_["combo_run"] = [this](const std::vector<int64_t>& args) -> int64_t {
			// combo_run is handled specially via name resolution
			return 0;
		};

		// combo_stop(combo_id)
		builtins_["combo_stop"] = [this](const std::vector<int64_t>& args) -> int64_t {
			return 0;
		};

		// combo_restart(combo_id)
		builtins_["combo_restart"] = [this](const std::vector<int64_t>& args) -> int64_t {
			return 0;
		};

		// combo_running(combo_id)
		builtins_["combo_running"] = [this](const std::vector<int64_t>& args) -> int64_t {
			return 0;
		};

		// get_mem(address, size) - read raw bytes
		builtins_["get_mem"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.size() < 2) throw runtime_error("get_mem requires 2 arguments");
			const auto addr = reinterpret_cast<const uint8_t*>(static_cast<uintptr_t>(args[0]));
			const auto size = static_cast<size_t>(args[1]);
			int64_t result = 0;
			if (size > 8) return 0;
			std::memcpy(&result, addr, size);
			return result;
		};

		// set_mem(address, size, value)
		builtins_["set_mem"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.size() < 3) throw runtime_error("set_mem requires 3 arguments");
			auto addr = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(args[0]));
			const auto size = static_cast<size_t>(args[1]);
			if (size > 8) return 0;
			std::memcpy(addr, &args[2], size);
			return 0;
		};

		// get_mem_bool(address)
		builtins_["get_mem_bool"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.empty()) throw runtime_error("get_mem_bool requires 1 argument");
			const auto addr = reinterpret_cast<const uint8_t*>(static_cast<uintptr_t>(args[0]));
			return *addr != 0 ? 1 : 0;
		};

		// set_mem_bool(address, value)
		builtins_["set_mem_bool"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.size() < 2) throw runtime_error("set_mem_bool requires 2 arguments");
			auto addr = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(args[0]));
			*addr = args[1] != 0 ? 1 : 0;
			return 0;
		};

		// get_mem_int(address) - 32-bit int
		builtins_["get_mem_int"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.empty()) throw runtime_error("get_mem_int requires 1 argument");
			const auto addr = reinterpret_cast<const int32_t*>(static_cast<uintptr_t>(args[0]));
			return *addr;
		};

		// set_mem_int(address, value)
		builtins_["set_mem_int"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.size() < 2) throw runtime_error("set_mem_int requires 2 arguments");
			auto addr = reinterpret_cast<int32_t*>(static_cast<uintptr_t>(args[0]));
			*addr = static_cast<int32_t>(args[1]);
			return 0;
		};

		// get_mem_float(address) - reads 32-bit float, returns as int (bit-cast)
		builtins_["get_mem_float"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.empty()) throw runtime_error("get_mem_float requires 1 argument");
			const auto addr = reinterpret_cast<const float*>(static_cast<uintptr_t>(args[0]));
			// store float bits as int for the integer-only VM
			float f = *addr;
			int64_t result;
			// scale by 1000 for fractional precision in integer math
			result = static_cast<int64_t>(f * 1000.0f);
			return result;
		};

		// set_mem_float(address, value) - value is a float passed as raw bits
		builtins_["set_mem_float"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.size() < 2) throw runtime_error("set_mem_float requires 2 arguments");
			auto addr = reinterpret_cast<float*>(static_cast<uintptr_t>(args[0]));
			// the second argument comes from a float_literal which was stored as raw bits
			int64_t raw = args[1];
			float f;
			std::memcpy(&f, &raw, sizeof(float));
			*addr = f;
			return 0;
		};

		// get_ptime() - microseconds since script start
		builtins_["get_ptime"] = [](const std::vector<int64_t>& args) -> int64_t {
			static const auto start = std::chrono::steady_clock::now();
			const auto now = std::chrono::steady_clock::now();
			return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
		};

		// get_rtime() - milliseconds of the current main loop execution time
		builtins_["get_rtime"] = [](const std::vector<int64_t>& args) -> int64_t {
			// placeholder - engine will set this
			return 0;
		};

		// abs(value)
		builtins_["abs"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.empty()) return 0;
			return args[0] < 0 ? -args[0] : args[0];
		};

		// isqrt(value) - integer square root
		builtins_["isqrt"] = [](const std::vector<int64_t>& args) -> int64_t {
			if (args.empty() || args[0] < 0) return 0;
			return static_cast<int64_t>(std::sqrt(static_cast<double>(args[0])));
		};

		// printf-like debug output
		builtins_["printf"] = [](const std::vector<int64_t>& args) -> int64_t {
			for (size_t i = 0; i < args.size(); i++)
			{
				if (i > 0) printf(" ");
				printf("%lld", static_cast<long long>(args[i]));
			}
			printf("\n");
			return 0;
		};
	}

	// ── load / reset ──

	void vm::load(program_node* program)
	{
		reset();
		program_ = program;

		// pre-process all top-level declarations
		for (const auto& decl : program->declarations)
		{
			switch (decl->kind)
			{
			case node_kind::define_decl:
			{
				auto* d = static_cast<const define_decl_stmt*>(decl.get());
				defines_[d->name] = eval_expr(d->value.get());
				break;
			}

			case node_kind::data_section:
			{
				auto* d = static_cast<const data_section_stmt*>(decl.get());
				for (const auto& v : d->values)
					data_section_.push_back(eval_expr(v.get()));
				break;
			}

			case node_kind::remap_decl:
			{
				auto* d = static_cast<const remap_decl_stmt*>(decl.get());
				const int from = resolve_input_name(d->from);
				const int to = resolve_input_name(d->to);
				if (from >= 0 && to >= 0) remap_table_[from] = to;
				break;
			}

			case node_kind::unmap_decl:
			{
				auto* d = static_cast<const unmap_decl_stmt*>(decl.get());
				const int btn = resolve_input_name(d->button);
				if (btn >= 0) remap_table_[btn] = -1; // -1 = unmapped
				break;
			}

			case node_kind::var_decl:
			{
				auto* d = static_cast<const var_decl_stmt*>(decl.get());
				int64_t init_val = 0;
				if (d->initializer) init_val = eval_expr(d->initializer.get());
				global_scope_.vars[d->name] = init_val;
				break;
			}

			case node_kind::block:
			{
				// multi-var declarations produce blocks
				auto* b = static_cast<const block_stmt*>(decl.get());
				for (const auto& s : b->statements)
				{
					if (s->kind == node_kind::var_decl)
					{
						auto* v = static_cast<const var_decl_stmt*>(s.get());
						int64_t init_val = 0;
						if (v->initializer) init_val = eval_expr(v->initializer.get());
						global_scope_.vars[v->name] = init_val;
					}
				}
				break;
			}

			case node_kind::const_array_decl:
			{
				auto* d = static_cast<const const_array_decl_stmt*>(decl.get());
				std::vector<int64_t> arr;
				arr.reserve(d->elements.size());
				for (const auto& e : d->elements)
					arr.push_back(eval_expr(e.get()));
				const_arrays_[d->name] = std::move(arr);
				break;
			}

			case node_kind::init_section:
				init_section_ = static_cast<const init_section_node*>(decl.get());
				break;

			case node_kind::main_section:
				main_section_ = static_cast<const main_section_node*>(decl.get());
				break;

			case node_kind::combo_decl:
			{
				auto* d = static_cast<const combo_decl_node*>(decl.get());
				combo_state cs;
				cs.name = d->name;
				cs.decl = d;
				cs.running = false;
				cs.pc = 0;
				cs.waiting = false;
				combos_[d->name] = std::move(cs);
				break;
			}

			case node_kind::function_decl:
			{
				auto* d = static_cast<const function_decl_node*>(decl.get());
				functions_[d->name] = d;
				break;
			}

			default:
				break;
			}
		}

		// register input constants as defines so scripts can reference them
		for (const auto& [name, idx] : get_input_name_map())
		{
			if (defines_.find(name) == defines_.end())
				defines_[name] = idx;
		}
	}

	void vm::reset()
	{
		program_ = nullptr;
		global_scope_.vars.clear();
		current_scope_ = &global_scope_;
		defines_.clear();
		data_section_.clear();
		const_arrays_.clear();
		functions_.clear();
		combos_.clear();
		remap_table_.clear();
		main_section_ = nullptr;
		init_section_ = nullptr;
		has_output_override_ = false;
		last_error_.clear();
		std::memset(input_values_, 0, sizeof(input_values_));
		std::memset(output_values_, 0, sizeof(output_values_));
		std::memset(output_overridden_, 0, sizeof(output_overridden_));
	}

	// ── execution ──

	void vm::run_init()
	{
		if (!init_section_) return;
		try
		{
			exec_block(init_section_->body.get());
		}
		catch (const std::exception& e)
		{
			last_error_ = std::string("init error: ") + e.what();
			printf("[GPC] %s\n", last_error_.c_str());
		}
	}

	void vm::run_main_tick()
	{
		if (!main_section_) return;

		// reset output overrides each tick
		has_output_override_ = false;
		std::memset(output_overridden_, 0, sizeof(output_overridden_));

		// copy input to output, applying remap/unmap
		for (int i = 0; i < static_cast<int>(gamepad_input::INPUT_COUNT); i++)
		{
			auto it = remap_table_.find(i);
			if (it != remap_table_.end())
			{
				if (it->second < 0)
				{
					output_values_[i] = 0; // unmapped: don't pass to game
				}
				else
				{
					output_values_[it->second] = input_values_[i]; // remapped
					output_values_[i] = 0;
				}
			}
			else
			{
				output_values_[i] = input_values_[i]; // pass-through
			}
		}

		try
		{
			exec_block(main_section_->body.get());
		}
		catch (const std::exception& e)
		{
			last_error_ = std::string("main error: ") + e.what();
			printf("[GPC] %s\n", last_error_.c_str());
		}

		// advance combos
		tick_combos();

		// re-apply held combo values (set_val persists during wait)
		for (const auto& [name, combo] : combos_)
		{
			if (!combo.running) continue;
			for (int i = 0; i < static_cast<int>(gamepad_input::INPUT_COUNT); i++)
			{
				if (combo.held_active[i])
				{
					output_values_[i] = combo.held_values[i];
					output_overridden_[i] = true;
					has_output_override_ = true;
				}
			}
		}
	}

	void vm::tick_combos()
	{
		for (auto& [name, combo] : combos_)
		{
			if (!combo.running) continue;

			if (combo.waiting)
			{
				const auto now = std::chrono::steady_clock::now();
				if (now < combo.wait_until) continue;
				combo.waiting = false;
				combo.pc++;
			}

			step_combo(combo);
		}
	}

	void vm::step_combo(combo_state& combo)
	{
		if (!combo.decl || !combo.decl->body) return;

		const auto& stmts = combo.decl->body->statements;
		const int max_steps = 1000; // prevent infinite loops per tick
		int steps = 0;

		active_combo_ = &combo;

		// clear held values at the start of each new step sequence
		std::memset(combo.held_active, 0, sizeof(combo.held_active));

		while (combo.pc < stmts.size() && steps++ < max_steps)
		{
			const auto* s = stmts[combo.pc].get();

			if (s->kind == node_kind::wait_stmt)
			{
				auto* w = static_cast<const wait_stmt_node*>(s);
				const int64_t ms = eval_expr(w->duration_ms.get());
				combo.wait_until = std::chrono::steady_clock::now() +
					std::chrono::microseconds(ms * 1000); // ms -> us
				combo.waiting = true;
				active_combo_ = nullptr;
				return; // yield
			}

			try
			{
				exec_stmt(s);
			}
			catch (const std::exception& e)
			{
				last_error_ = std::string("combo '") + combo.name + "' error: " + e.what();
				printf("[GPC] %s\n", last_error_.c_str());
				combo.running = false;
				active_combo_ = nullptr;
				return;
			}

			combo.pc++;
		}

		active_combo_ = nullptr;

		// combo finished one cycle - stop
		if (combo.pc >= stmts.size())
		{
			combo.running = false;
			combo.pc = 0;
			combo.waiting = false;
			std::memset(combo.held_active, 0, sizeof(combo.held_active));
		}
	}

	// ── controller I/O ──

	void vm::set_input_state(gamepad_input input, int32_t value)
	{
		const int idx = static_cast<int>(input);
		if (idx >= 0 && idx < static_cast<int>(gamepad_input::INPUT_COUNT))
		{
			// always store raw input so get_val() works
			input_values_[idx] = value;
		}
	}

	int32_t vm::get_input_state(gamepad_input input) const
	{
		const int idx = static_cast<int>(input);
		if (idx >= 0 && idx < static_cast<int>(gamepad_input::INPUT_COUNT))
			return input_values_[idx];
		return 0;
	}

	void vm::set_output_state(gamepad_input input, int32_t value)
	{
		const int idx = static_cast<int>(input);
		if (idx >= 0 && idx < static_cast<int>(gamepad_input::INPUT_COUNT))
		{
			output_values_[idx] = value;
			output_overridden_[idx] = true;
			has_output_override_ = true;
		}
	}

	int32_t vm::get_output_state(gamepad_input input) const
	{
		const int idx = static_cast<int>(input);
		if (idx >= 0 && idx < static_cast<int>(gamepad_input::INPUT_COUNT))
			return output_values_[idx];
		return 0;
	}

	void vm::get_output_values(int32_t* out, size_t count) const
	{
		const size_t n = std::min(count, static_cast<size_t>(gamepad_input::INPUT_COUNT));
		std::memcpy(out, output_values_, n * sizeof(int32_t));
	}

	// ── statement execution ──

	exec_signal vm::exec_block(const block_stmt* block)
	{
		if (!block) return {};

		for (const auto& s : block->statements)
		{
			auto sig = exec_stmt(s.get());
			if (!std::holds_alternative<std::monostate>(sig))
				return sig;
		}
		return {};
	}

	exec_signal vm::exec_stmt(const stmt* s)
	{
		switch (s->kind)
		{
		case node_kind::block:
			return exec_block(static_cast<const block_stmt*>(s));

		case node_kind::var_decl:
		{
			auto* d = static_cast<const var_decl_stmt*>(s);
			int64_t val = 0;
			if (d->initializer) val = eval_expr(d->initializer.get());
			current_scope_->vars[d->name] = val;
			return {};
		}

		case node_kind::expr_stmt:
		{
			auto* e = static_cast<const expr_stmt_node*>(s);
			eval_expr(e->expression.get());
			return {};
		}

		case node_kind::if_stmt:
		{
			auto* n = static_cast<const if_stmt_node*>(s);
			if (eval_expr(n->condition.get()) != 0)
				return exec_stmt(n->then_branch.get());
			else if (n->else_branch)
				return exec_stmt(n->else_branch.get());
			return {};
		}

		case node_kind::while_stmt:
		{
			auto* n = static_cast<const while_stmt_node*>(s);
			while (eval_expr(n->condition.get()) != 0)
			{
				auto sig = exec_stmt(n->body.get());
				if (std::holds_alternative<signal_break>(sig)) break;
				if (std::holds_alternative<signal_return>(sig)) return sig;
				// continue just restarts the loop
			}
			return {};
		}

		case node_kind::do_while_stmt:
		{
			auto* n = static_cast<const do_while_stmt_node*>(s);
			do
			{
				auto sig = exec_stmt(n->body.get());
				if (std::holds_alternative<signal_break>(sig)) break;
				if (std::holds_alternative<signal_return>(sig)) return sig;
			} while (eval_expr(n->condition.get()) != 0);
			return {};
		}

		case node_kind::for_stmt:
		{
			auto* n = static_cast<const for_stmt_node*>(s);
			if (n->init) exec_stmt(n->init.get());

			while (true)
			{
				if (n->condition && eval_expr(n->condition.get()) == 0) break;

				auto sig = exec_stmt(n->body.get());
				if (std::holds_alternative<signal_break>(sig)) break;
				if (std::holds_alternative<signal_return>(sig)) return sig;

				if (n->increment) eval_expr(n->increment.get());
			}
			return {};
		}

		case node_kind::switch_stmt:
		{
			auto* n = static_cast<const switch_stmt_node*>(s);
			const int64_t val = eval_expr(n->condition.get());
			bool matched = false;
			bool fell_through = false;

			for (const auto& c : n->cases)
			{
				if (!fell_through && !matched)
				{
					if (c->value)
					{
						if (eval_expr(c->value.get()) != val) continue;
					}
					matched = true;
				}

				if (matched || fell_through)
				{
					bool should_break = false;
					for (const auto& body_s : c->body)
					{
						auto sig = exec_stmt(body_s.get());
						if (std::holds_alternative<signal_break>(sig)) { should_break = true; break; }
						if (std::holds_alternative<signal_return>(sig)) return sig;
					}
					if (should_break) return {};
					fell_through = true;
				}
			}
			return {};
		}

		case node_kind::return_stmt:
		{
			auto* n = static_cast<const return_stmt_node*>(s);
			int64_t val = 0;
			if (n->value) val = eval_expr(n->value.get());
			return signal_return{val};
		}

		case node_kind::break_stmt:
			return signal_break{};

		case node_kind::continue_stmt:
			return signal_continue{};

		case node_kind::wait_stmt:
			throw runtime_error("wait() is only allowed inside combos");

		default:
			return {};
		}
	}

	// ── expression evaluation ──

	int64_t vm::eval_expr(const expr* e)
	{
		switch (e->kind)
		{
		case node_kind::number_literal:
			return static_cast<const number_literal_expr*>(e)->value;

		case node_kind::float_literal:
		{
			// store float bits in int64 for set_mem_float
			const double d = static_cast<const float_literal_expr*>(e)->value;
			const float f = static_cast<float>(d);
			int64_t result = 0;
			std::memcpy(&result, &f, sizeof(float));
			return result;
		}

		case node_kind::identifier:
		{
			const auto& name = static_cast<const identifier_expr*>(e)->name;

			// check defines first
			auto dit = defines_.find(name);
			if (dit != defines_.end()) return dit->second;

			// then variables
			int64_t val = 0;
			if (current_scope_->get(name, val)) return val;

			// check if it's a combo name (resolve to index for combo_run etc.)
			auto cit = combos_.find(name);
			if (cit != combos_.end()) return 0; // combo names are resolved at call sites

			throw runtime_error("undefined variable: " + name);
		}

		case node_kind::binary_expr:
		{
			auto* n = static_cast<const binary_expr_node*>(e);
			const int64_t left = eval_expr(n->left.get());

			// short-circuit for logical operators
			if (n->op == binary_op::logical_and) return (left != 0 && eval_expr(n->right.get()) != 0) ? 1 : 0;
			if (n->op == binary_op::logical_or) return (left != 0 || eval_expr(n->right.get()) != 0) ? 1 : 0;

			const int64_t right = eval_expr(n->right.get());

			switch (n->op)
			{
			case binary_op::add:         return left + right;
			case binary_op::sub:         return left - right;
			case binary_op::mul:         return left * right;
			case binary_op::div:
				if (right == 0) throw runtime_error("division by zero");
				return left / right; // integer division rounds toward zero
			case binary_op::mod:
				if (right == 0) throw runtime_error("modulo by zero");
				return left % right;
			case binary_op::eq:          return left == right ? 1 : 0;
			case binary_op::neq:         return left != right ? 1 : 0;
			case binary_op::lt:          return left < right ? 1 : 0;
			case binary_op::gt:          return left > right ? 1 : 0;
			case binary_op::lte:         return left <= right ? 1 : 0;
			case binary_op::gte:         return left >= right ? 1 : 0;
			case binary_op::bit_and:     return left & right;
			case binary_op::bit_or:      return left | right;
			case binary_op::bit_xor:     return left ^ right;
			case binary_op::shift_left:  return left << right;
			case binary_op::shift_right: return left >> right;
			default: return 0;
			}
		}

		case node_kind::unary_expr:
		{
			auto* n = static_cast<const unary_expr_node*>(e);
			const int64_t val = eval_expr(n->operand.get());
			switch (n->op)
			{
			case unary_op::negate:      return -val;
			case unary_op::logical_not: return val == 0 ? 1 : 0;
			case unary_op::bit_not:     return ~val;
			}
			return 0;
		}

		case node_kind::call_expr:
		{
			auto* n = static_cast<const call_expr_node*>(e);

			// special handling for combo functions - first arg is a combo name identifier
			if (n->callee == "combo_run" || n->callee == "combo_stop" ||
				n->callee == "combo_restart" || n->callee == "combo_running")
			{
				if (n->args.empty()) throw runtime_error(n->callee + " requires combo name argument");

				// get combo name from first arg (should be identifier)
				std::string combo_name;
				if (n->args[0]->kind == node_kind::identifier)
					combo_name = static_cast<const identifier_expr*>(n->args[0].get())->name;
				else
					throw runtime_error(n->callee + " argument must be a combo name");

				auto it = combos_.find(combo_name);
				if (it == combos_.end())
					throw runtime_error("unknown combo: " + combo_name);

				if (n->callee == "combo_run")
				{
					if (!it->second.running)
					{
						it->second.running = true;
						it->second.pc = 0;
						it->second.waiting = false;
					}
					return 0;
				}
				if (n->callee == "combo_stop")
				{
					it->second.running = false;
					it->second.pc = 0;
					it->second.waiting = false;
					return 0;
				}
				if (n->callee == "combo_restart")
				{
					it->second.running = true;
					it->second.pc = 0;
					it->second.waiting = false;
					return 0;
				}
				if (n->callee == "combo_running")
				{
					return it->second.running ? 1 : 0;
				}
			}

			// evaluate arguments
			std::vector<int64_t> args;
			args.reserve(n->args.size());
			for (const auto& a : n->args)
				args.push_back(eval_expr(a.get()));

			// check builtins
			auto bit = builtins_.find(n->callee);
			if (bit != builtins_.end())
				return bit->second(args);

			// check user functions
			auto fit = functions_.find(n->callee);
			if (fit != functions_.end())
			{
				const auto* func = fit->second;

				// create function scope
				scope func_scope;
				func_scope.parent = &global_scope_; // functions only see globals
				for (size_t i = 0; i < func->params.size() && i < args.size(); i++)
					func_scope.vars[func->params[i]] = args[i];

				auto* prev_scope = current_scope_;
				current_scope_ = &func_scope;

				int64_t result = 0;
				try
				{
					auto sig = exec_block(func->body.get());
					if (std::holds_alternative<signal_return>(sig))
						result = std::get<signal_return>(sig).value;
				}
				catch (...)
				{
					current_scope_ = prev_scope;
					throw;
				}

				current_scope_ = prev_scope;
				return result;
			}

			throw runtime_error("undefined function: " + n->callee);
		}

		case node_kind::index_expr:
		{
			auto* n = static_cast<const index_expr_node*>(e);
			const int64_t idx = eval_expr(n->index.get());

			// check const arrays
			auto ait = const_arrays_.find(n->array_name);
			if (ait != const_arrays_.end())
			{
				if (idx < 0 || idx >= static_cast<int64_t>(ait->second.size()))
					throw runtime_error("array index out of bounds: " + n->array_name + "[" + std::to_string(idx) + "]");
				return ait->second[static_cast<size_t>(idx)];
			}

			// check data section (accessed by index)
			if (n->array_name == "data" || n->array_name == "dbyte")
			{
				if (idx < 0 || idx >= static_cast<int64_t>(data_section_.size()))
					throw runtime_error("data index out of bounds");
				return data_section_[static_cast<size_t>(idx)];
			}

			throw runtime_error("undefined array: " + n->array_name);
		}

		case node_kind::assign_expr:
		{
			auto* n = static_cast<const assign_expr_node*>(e);
			const int64_t rhs = eval_expr(n->value.get());

			// get target name
			std::string target_name;
			if (n->target->kind == node_kind::identifier)
				target_name = static_cast<const identifier_expr*>(n->target.get())->name;
			else
				throw runtime_error("invalid assignment target");

			int64_t current_val = 0;
			current_scope_->get(target_name, current_val);

			int64_t new_val;
			switch (n->op)
			{
			case assign_op::assign:     new_val = rhs; break;
			case assign_op::add_assign: new_val = current_val + rhs; break;
			case assign_op::sub_assign: new_val = current_val - rhs; break;
			case assign_op::mul_assign: new_val = current_val * rhs; break;
			case assign_op::div_assign:
				if (rhs == 0) throw runtime_error("division by zero");
				new_val = current_val / rhs; break;
			case assign_op::mod_assign:
				if (rhs == 0) throw runtime_error("modulo by zero");
				new_val = current_val % rhs; break;
			case assign_op::and_assign: new_val = current_val & rhs; break;
			case assign_op::or_assign:  new_val = current_val | rhs; break;
			case assign_op::xor_assign: new_val = current_val ^ rhs; break;
			default: new_val = rhs; break;
			}

			current_scope_->set(target_name, new_val);
			return new_val;
		}

		case node_kind::increment_expr:
		{
			auto* n = static_cast<const increment_expr_node*>(e);
			if (n->operand->kind != node_kind::identifier)
				throw runtime_error("increment/decrement requires a variable");

			const auto& name = static_cast<const identifier_expr*>(n->operand.get())->name;
			int64_t val = 0;
			current_scope_->get(name, val);

			const int64_t delta = n->is_increment ? 1 : -1;
			if (n->is_prefix)
			{
				val += delta;
				current_scope_->set(name, val);
				return val;
			}
			else
			{
				current_scope_->set(name, val + delta);
				return val; // return old value
			}
		}

		default:
			throw runtime_error("unknown expression type");
		}
	}
}
