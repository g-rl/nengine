#pragma once

#include "gpc_lexer.hpp"
#include "gpc_ast.hpp"

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

namespace gpc
{
	class parse_error : public std::runtime_error
	{
	public:
		int line;
		int column;
		parse_error(const std::string& msg, int l, int c)
			: std::runtime_error(msg), line(l), column(c) {}
	};

	class parser
	{
	public:
		std::unique_ptr<program_node> parse(const std::vector<token>& tokens, const std::string& filename = "<unknown>");

	private:
		const std::vector<token>* tokens_;
		size_t pos_;
		std::string filename_;

		const token& current() const;
		const token& peek(int offset = 1) const;
		const token& advance();
		bool check(token_type type) const;
		bool match(token_type type);
		const token& expect(token_type type, const std::string& msg);

		[[noreturn]] void error(const std::string& msg) const;
		[[noreturn]] void error(const std::string& msg, const token& tok) const;

		// top-level
		stmt_ptr parse_top_level();
		stmt_ptr parse_define();
		stmt_ptr parse_data_section();
		stmt_ptr parse_remap();
		stmt_ptr parse_unmap();
		stmt_ptr parse_global_var_decl(const std::string& type_name);
		stmt_ptr parse_const_array();
		stmt_ptr parse_init_section();
		stmt_ptr parse_main_section();
		stmt_ptr parse_combo();
		stmt_ptr parse_function();

		// statements
		stmt_ptr parse_statement();
		stmt_ptr parse_local_var_decl(const std::string& type_name);
		std::unique_ptr<block_stmt> parse_block();
		stmt_ptr parse_if();
		stmt_ptr parse_while();
		stmt_ptr parse_do_while();
		stmt_ptr parse_for();
		stmt_ptr parse_switch();
		stmt_ptr parse_return();
		stmt_ptr parse_break();
		stmt_ptr parse_continue();
		stmt_ptr parse_wait();
		stmt_ptr parse_expr_statement();

		// expressions (precedence climbing)
		expr_ptr parse_expression();
		expr_ptr parse_assignment();
		expr_ptr parse_logical_or();
		expr_ptr parse_logical_and();
		expr_ptr parse_bitwise_or();
		expr_ptr parse_bitwise_xor();
		expr_ptr parse_bitwise_and();
		expr_ptr parse_equality();
		expr_ptr parse_comparison();
		expr_ptr parse_shift();
		expr_ptr parse_additive();
		expr_ptr parse_multiplicative();
		expr_ptr parse_unary();
		expr_ptr parse_postfix();
		expr_ptr parse_primary();

		bool is_type_keyword() const;
		std::string get_type_name();
	};
}
