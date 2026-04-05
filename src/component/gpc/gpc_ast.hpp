#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace gpc
{
	// forward declarations
	struct node;
	struct expr;
	struct stmt;

	using node_ptr = std::unique_ptr<node>;
	using expr_ptr = std::unique_ptr<expr>;
	using stmt_ptr = std::unique_ptr<stmt>;

	enum class node_kind
	{
		// expressions
		number_literal,
		float_literal,
		identifier,
		binary_expr,
		unary_expr,
		call_expr,
		index_expr,
		assign_expr,
		increment_expr,

		// statements
		var_decl,
		define_decl,
		const_array_decl,
		data_section,
		remap_decl,
		unmap_decl,
		block,
		expr_stmt,
		if_stmt,
		while_stmt,
		do_while_stmt,
		for_stmt,
		switch_stmt,
		case_clause,
		return_stmt,
		break_stmt,
		continue_stmt,
		wait_stmt,

		// top-level
		init_section,
		main_section,
		combo_decl,
		function_decl,
		program,
	};

	struct node
	{
		node_kind kind;
		int line = 0;
		int column = 0;
		virtual ~node() = default;

	protected:
		explicit node(node_kind k) : kind(k) {}
	};

	// ── expressions ──

	struct expr : node
	{
	protected:
		using node::node;
	};

	struct number_literal_expr : expr
	{
		int64_t value;
		number_literal_expr(int64_t v) : expr(node_kind::number_literal), value(v) {}
	};

	struct float_literal_expr : expr
	{
		double value;
		float_literal_expr(double v) : expr(node_kind::float_literal), value(v) {}
	};

	struct identifier_expr : expr
	{
		std::string name;
		identifier_expr(const std::string& n) : expr(node_kind::identifier), name(n) {}
	};

	enum class binary_op
	{
		add, sub, mul, div, mod,
		eq, neq, lt, gt, lte, gte,
		logical_and, logical_or,
		bit_and, bit_or, bit_xor,
		shift_left, shift_right,
	};

	struct binary_expr_node : expr
	{
		binary_op op;
		expr_ptr left;
		expr_ptr right;
		binary_expr_node(binary_op o, expr_ptr l, expr_ptr r)
			: expr(node_kind::binary_expr), op(o), left(std::move(l)), right(std::move(r)) {}
	};

	enum class unary_op
	{
		negate,      // -
		logical_not, // ! or NOT
		bit_not,     // ~
	};

	struct unary_expr_node : expr
	{
		unary_op op;
		expr_ptr operand;
		unary_expr_node(unary_op o, expr_ptr e)
			: expr(node_kind::unary_expr), op(o), operand(std::move(e)) {}
	};

	struct call_expr_node : expr
	{
		std::string callee;
		std::vector<expr_ptr> args;
		call_expr_node(const std::string& name, std::vector<expr_ptr> a)
			: expr(node_kind::call_expr), callee(name), args(std::move(a)) {}
	};

	struct index_expr_node : expr
	{
		std::string array_name;
		expr_ptr index;
		index_expr_node(const std::string& name, expr_ptr idx)
			: expr(node_kind::index_expr), array_name(name), index(std::move(idx)) {}
	};

	enum class assign_op
	{
		assign,     // =
		add_assign, // +=
		sub_assign, // -=
		mul_assign, // *=
		div_assign, // /=
		mod_assign, // %=
		and_assign, // &=
		or_assign,  // |=
		xor_assign, // ^=
	};

	struct assign_expr_node : expr
	{
		assign_op op;
		expr_ptr target; // identifier or index expr
		expr_ptr value;
		assign_expr_node(assign_op o, expr_ptr t, expr_ptr v)
			: expr(node_kind::assign_expr), op(o), target(std::move(t)), value(std::move(v)) {}
	};

	struct increment_expr_node : expr
	{
		bool is_increment; // true = ++, false = --
		bool is_prefix;
		expr_ptr operand;
		increment_expr_node(bool inc, bool pre, expr_ptr e)
			: expr(node_kind::increment_expr), is_increment(inc), is_prefix(pre), operand(std::move(e)) {}
	};

	// ── statements ──

	struct stmt : node
	{
	protected:
		using node::node;
	};

	struct block_stmt : stmt
	{
		std::vector<stmt_ptr> statements;
		block_stmt() : stmt(node_kind::block) {}
	};

	struct var_decl_stmt : stmt
	{
		std::string type_name; // "int", "int8", "int16", "int32"
		std::string name;
		expr_ptr initializer; // may be null
		var_decl_stmt(const std::string& type, const std::string& n, expr_ptr init = nullptr)
			: stmt(node_kind::var_decl), type_name(type), name(n), initializer(std::move(init)) {}
	};

	struct define_decl_stmt : stmt
	{
		std::string name;
		expr_ptr value;
		define_decl_stmt(const std::string& n, expr_ptr v)
			: stmt(node_kind::define_decl), name(n), value(std::move(v)) {}
	};

	struct const_array_decl_stmt : stmt
	{
		std::string element_type; // "int8", "int16", "int32", "int"
		std::string name;
		std::vector<expr_ptr> elements;
		const_array_decl_stmt(const std::string& type, const std::string& n, std::vector<expr_ptr> elems)
			: stmt(node_kind::const_array_decl), element_type(type), name(n), elements(std::move(elems)) {}
	};

	struct data_section_stmt : stmt
	{
		std::vector<expr_ptr> values;
		data_section_stmt(std::vector<expr_ptr> vals)
			: stmt(node_kind::data_section), values(std::move(vals)) {}
	};

	struct remap_decl_stmt : stmt
	{
		std::string from;
		std::string to;
		remap_decl_stmt(const std::string& f, const std::string& t)
			: stmt(node_kind::remap_decl), from(f), to(t) {}
	};

	struct unmap_decl_stmt : stmt
	{
		std::string button;
		unmap_decl_stmt(const std::string& b)
			: stmt(node_kind::unmap_decl), button(b) {}
	};

	struct expr_stmt_node : stmt
	{
		expr_ptr expression;
		expr_stmt_node(expr_ptr e) : stmt(node_kind::expr_stmt), expression(std::move(e)) {}
	};

	struct if_stmt_node : stmt
	{
		expr_ptr condition;
		stmt_ptr then_branch;
		stmt_ptr else_branch; // may be null
		if_stmt_node(expr_ptr cond, stmt_ptr then_b, stmt_ptr else_b = nullptr)
			: stmt(node_kind::if_stmt), condition(std::move(cond)),
			  then_branch(std::move(then_b)), else_branch(std::move(else_b)) {}
	};

	struct while_stmt_node : stmt
	{
		expr_ptr condition;
		stmt_ptr body;
		while_stmt_node(expr_ptr cond, stmt_ptr b)
			: stmt(node_kind::while_stmt), condition(std::move(cond)), body(std::move(b)) {}
	};

	struct do_while_stmt_node : stmt
	{
		stmt_ptr body;
		expr_ptr condition;
		do_while_stmt_node(stmt_ptr b, expr_ptr cond)
			: stmt(node_kind::do_while_stmt), body(std::move(b)), condition(std::move(cond)) {}
	};

	struct for_stmt_node : stmt
	{
		stmt_ptr init;      // var decl or expr stmt
		expr_ptr condition;
		expr_ptr increment;
		stmt_ptr body;
		for_stmt_node(stmt_ptr i, expr_ptr c, expr_ptr inc, stmt_ptr b)
			: stmt(node_kind::for_stmt), init(std::move(i)), condition(std::move(c)),
			  increment(std::move(inc)), body(std::move(b)) {}
	};

	struct case_clause_node : stmt
	{
		expr_ptr value; // null for default
		std::vector<stmt_ptr> body;
		case_clause_node(expr_ptr v, std::vector<stmt_ptr> b)
			: stmt(node_kind::case_clause), value(std::move(v)), body(std::move(b)) {}
	};

	struct switch_stmt_node : stmt
	{
		expr_ptr condition;
		std::vector<std::unique_ptr<case_clause_node>> cases;
		switch_stmt_node(expr_ptr cond, std::vector<std::unique_ptr<case_clause_node>> c)
			: stmt(node_kind::switch_stmt), condition(std::move(cond)), cases(std::move(c)) {}
	};

	struct return_stmt_node : stmt
	{
		expr_ptr value; // may be null
		return_stmt_node(expr_ptr v = nullptr)
			: stmt(node_kind::return_stmt), value(std::move(v)) {}
	};

	struct break_stmt_node : stmt
	{
		break_stmt_node() : stmt(node_kind::break_stmt) {}
	};

	struct continue_stmt_node : stmt
	{
		continue_stmt_node() : stmt(node_kind::continue_stmt) {}
	};

	struct wait_stmt_node : stmt
	{
		expr_ptr duration_ms;
		wait_stmt_node(expr_ptr d) : stmt(node_kind::wait_stmt), duration_ms(std::move(d)) {}
	};

	// ── top-level ──

	struct init_section_node : stmt
	{
		std::unique_ptr<block_stmt> body;
		init_section_node(std::unique_ptr<block_stmt> b)
			: stmt(node_kind::init_section), body(std::move(b)) {}
	};

	struct main_section_node : stmt
	{
		std::unique_ptr<block_stmt> body;
		main_section_node(std::unique_ptr<block_stmt> b)
			: stmt(node_kind::main_section), body(std::move(b)) {}
	};

	struct combo_decl_node : stmt
	{
		std::string name;
		std::unique_ptr<block_stmt> body;
		combo_decl_node(const std::string& n, std::unique_ptr<block_stmt> b)
			: stmt(node_kind::combo_decl), name(n), body(std::move(b)) {}
	};

	struct function_decl_node : stmt
	{
		std::string name;
		std::vector<std::string> params;
		std::unique_ptr<block_stmt> body;
		function_decl_node(const std::string& n, std::vector<std::string> p, std::unique_ptr<block_stmt> b)
			: stmt(node_kind::function_decl), name(n), params(std::move(p)), body(std::move(b)) {}
	};

	struct program_node : node
	{
		std::vector<stmt_ptr> declarations;
		program_node() : node(node_kind::program) {}
	};
}
