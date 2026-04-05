#include <std_include.hpp>
#include "gpc_parser.hpp"

namespace gpc
{
	std::unique_ptr<program_node> parser::parse(const std::vector<token>& tokens, const std::string& filename)
	{
		tokens_ = &tokens;
		pos_ = 0;
		filename_ = filename;

		auto program = std::make_unique<program_node>();

		while (!check(token_type::eof))
		{
			program->declarations.push_back(parse_top_level());
		}

		return program;
	}

	// ── token helpers ──

	const token& parser::current() const
	{
		return (*tokens_)[pos_];
	}

	const token& parser::peek(int offset) const
	{
		const size_t idx = pos_ + offset;
		if (idx >= tokens_->size()) return tokens_->back();
		return (*tokens_)[idx];
	}

	const token& parser::advance()
	{
		const auto& tok = current();
		if (pos_ < tokens_->size() - 1) pos_++;
		return tok;
	}

	bool parser::check(token_type type) const
	{
		return current().type == type;
	}

	bool parser::match(token_type type)
	{
		if (check(type)) { advance(); return true; }
		return false;
	}

	const token& parser::expect(token_type type, const std::string& msg)
	{
		if (check(type)) return advance();
		error(msg);
	}

	void parser::error(const std::string& msg) const
	{
		error(msg, current());
	}

	void parser::error(const std::string& msg, const token& tok) const
	{
		throw parse_error(
			filename_ + "(" + std::to_string(tok.line) + "," + std::to_string(tok.column) + "): " + msg,
			tok.line, tok.column);
	}

	bool parser::is_type_keyword() const
	{
		const auto t = current().type;
		return t == token_type::kw_int || t == token_type::kw_int8 ||
		       t == token_type::kw_int16 || t == token_type::kw_int32;
	}

	std::string parser::get_type_name()
	{
		const auto& tok = current();
		if (tok.type == token_type::kw_int) { advance(); return "int"; }
		if (tok.type == token_type::kw_int8) { advance(); return "int8"; }
		if (tok.type == token_type::kw_int16) { advance(); return "int16"; }
		if (tok.type == token_type::kw_int32) { advance(); return "int32"; }
		error("expected type name");
	}

	// ── top-level parsing ──

	stmt_ptr parser::parse_top_level()
	{
		const auto& tok = current();

		switch (tok.type)
		{
		case token_type::kw_define:   return parse_define();
		case token_type::kw_data:     return parse_data_section();
		case token_type::kw_remap:    return parse_remap();
		case token_type::kw_unmap:    return parse_unmap();
		case token_type::kw_init:     return parse_init_section();
		case token_type::kw_main:     return parse_main_section();
		case token_type::kw_combo:    return parse_combo();
		case token_type::kw_function: return parse_function();

		case token_type::kw_const:    return parse_const_array();

		case token_type::kw_int:
		case token_type::kw_int8:
		case token_type::kw_int16:
		case token_type::kw_int32:
		{
			auto type = get_type_name();
			return parse_global_var_decl(type);
		}

		default:
			error("unexpected token '" + tok.value + "' at top level");
		}
	}

	// define MY_CONST = 10;
	stmt_ptr parser::parse_define()
	{
		advance(); // 'define'
		const auto& name_tok = expect(token_type::identifier, "expected identifier after 'define'");
		expect(token_type::op_assign, "expected '=' in define");
		auto value = parse_expression();
		expect(token_type::semicolon, "expected ';' after define");
		return std::make_unique<define_decl_stmt>(name_tok.value, std::move(value));
	}

	// data(0, 10, 20);
	stmt_ptr parser::parse_data_section()
	{
		advance(); // 'data'
		expect(token_type::left_paren, "expected '(' after 'data'");

		std::vector<expr_ptr> values;
		if (!check(token_type::right_paren))
		{
			values.push_back(parse_expression());
			while (match(token_type::comma))
				values.push_back(parse_expression());
		}

		expect(token_type::right_paren, "expected ')' after data values");
		expect(token_type::semicolon, "expected ';' after data section");
		return std::make_unique<data_section_stmt>(std::move(values));
	}

	// remap XB1_A -> XB1_B;
	stmt_ptr parser::parse_remap()
	{
		advance(); // 'remap'
		const auto& from = expect(token_type::identifier, "expected button name after 'remap'");
		expect(token_type::op_minus, "expected '->' in remap");
		expect(token_type::op_greater, "expected '->' in remap");
		const auto& to = expect(token_type::identifier, "expected button name in remap");
		expect(token_type::semicolon, "expected ';' after remap");
		return std::make_unique<remap_decl_stmt>(from.value, to.value);
	}

	// unmap XB1_A;
	stmt_ptr parser::parse_unmap()
	{
		advance(); // 'unmap'
		const auto& btn = expect(token_type::identifier, "expected button name after 'unmap'");
		expect(token_type::semicolon, "expected ';' after unmap");
		return std::make_unique<unmap_decl_stmt>(btn.value);
	}

	// int my_var; or int x = 10, y = 20;
	stmt_ptr parser::parse_global_var_decl(const std::string& type_name)
	{
		// for the first variable in a potential comma-separated list we create a block
		auto block = std::make_unique<block_stmt>();

		auto parse_single = [&]() {
			const auto& name_tok = expect(token_type::identifier, "expected variable name");
			expr_ptr init;
			if (match(token_type::op_assign))
				init = parse_expression();
			block->statements.push_back(
				std::make_unique<var_decl_stmt>(type_name, name_tok.value, std::move(init)));
		};

		parse_single();
		while (match(token_type::comma))
			parse_single();

		expect(token_type::semicolon, "expected ';' after variable declaration");

		if (block->statements.size() == 1)
			return std::move(block->statements[0]);
		return block;
	}

	// const int8 arr[] = {1, 2, 3};
	stmt_ptr parser::parse_const_array()
	{
		advance(); // 'const'
		auto type = get_type_name();
		const auto& name_tok = expect(token_type::identifier, "expected array name");
		expect(token_type::left_bracket, "expected '[' after array name");
		expect(token_type::right_bracket, "expected ']'");
		expect(token_type::op_assign, "expected '=' in const array");
		expect(token_type::left_brace, "expected '{' for array initializer");

		std::vector<expr_ptr> elements;
		if (!check(token_type::right_brace))
		{
			elements.push_back(parse_expression());
			while (match(token_type::comma))
			{
				if (check(token_type::right_brace)) break; // trailing comma
				elements.push_back(parse_expression());
			}
		}

		expect(token_type::right_brace, "expected '}' after array elements");
		expect(token_type::semicolon, "expected ';' after const array");
		return std::make_unique<const_array_decl_stmt>(type, name_tok.value, std::move(elements));
	}

	// init { ... }
	stmt_ptr parser::parse_init_section()
	{
		advance(); // 'init'
		auto body = parse_block();
		return std::make_unique<init_section_node>(std::move(body));
	}

	// main { ... }
	stmt_ptr parser::parse_main_section()
	{
		advance(); // 'main'
		auto body = parse_block();
		return std::make_unique<main_section_node>(std::move(body));
	}

	// combo Name { ... }
	stmt_ptr parser::parse_combo()
	{
		advance(); // 'combo'
		const auto& name_tok = expect(token_type::identifier, "expected combo name");
		auto body = parse_block();
		return std::make_unique<combo_decl_node>(name_tok.value, std::move(body));
	}

	// function name(params) { ... }
	stmt_ptr parser::parse_function()
	{
		advance(); // 'function'
		const auto& name_tok = expect(token_type::identifier, "expected function name");
		expect(token_type::left_paren, "expected '(' after function name");

		std::vector<std::string> params;
		if (!check(token_type::right_paren))
		{
			params.push_back(expect(token_type::identifier, "expected parameter name").value);
			while (match(token_type::comma))
				params.push_back(expect(token_type::identifier, "expected parameter name").value);
		}

		expect(token_type::right_paren, "expected ')' after parameters");
		auto body = parse_block();
		return std::make_unique<function_decl_node>(name_tok.value, std::move(params), std::move(body));
	}

	// ── block and statements ──

	std::unique_ptr<block_stmt> parser::parse_block()
	{
		expect(token_type::left_brace, "expected '{'");
		auto block = std::make_unique<block_stmt>();

		while (!check(token_type::right_brace) && !check(token_type::eof))
		{
			block->statements.push_back(parse_statement());
		}

		expect(token_type::right_brace, "expected '}'");
		return block;
	}

	stmt_ptr parser::parse_statement()
	{
		const auto& tok = current();

		switch (tok.type)
		{
		case token_type::kw_if:       return parse_if();
		case token_type::kw_while:    return parse_while();
		case token_type::kw_do:       return parse_do_while();
		case token_type::kw_for:      return parse_for();
		case token_type::kw_switch:   return parse_switch();
		case token_type::kw_return:   return parse_return();
		case token_type::kw_break:    return parse_break();
		case token_type::kw_continue: return parse_continue();
		case token_type::kw_wait:     return parse_wait();
		case token_type::left_brace:  return parse_block();

		case token_type::kw_int:
		case token_type::kw_int8:
		case token_type::kw_int16:
		case token_type::kw_int32:
		{
			auto type = get_type_name();
			return parse_local_var_decl(type);
		}

		default:
			return parse_expr_statement();
		}
	}

	stmt_ptr parser::parse_local_var_decl(const std::string& type_name)
	{
		auto block = std::make_unique<block_stmt>();

		auto parse_single = [&]() {
			const auto& name_tok = expect(token_type::identifier, "expected variable name");
			expr_ptr init;
			if (match(token_type::op_assign))
				init = parse_expression();
			block->statements.push_back(
				std::make_unique<var_decl_stmt>(type_name, name_tok.value, std::move(init)));
		};

		parse_single();
		while (match(token_type::comma))
			parse_single();

		expect(token_type::semicolon, "expected ';' after variable declaration");

		if (block->statements.size() == 1)
			return std::move(block->statements[0]);
		return block;
	}

	stmt_ptr parser::parse_if()
	{
		advance(); // 'if'
		expect(token_type::left_paren, "expected '(' after 'if'");
		auto cond = parse_expression();
		expect(token_type::right_paren, "expected ')' after if condition");
		auto then_branch = parse_statement();

		stmt_ptr else_branch;
		if (match(token_type::kw_else))
			else_branch = parse_statement();

		return std::make_unique<if_stmt_node>(std::move(cond), std::move(then_branch), std::move(else_branch));
	}

	stmt_ptr parser::parse_while()
	{
		advance(); // 'while'
		expect(token_type::left_paren, "expected '(' after 'while'");
		auto cond = parse_expression();
		expect(token_type::right_paren, "expected ')' after while condition");
		auto body = parse_statement();
		return std::make_unique<while_stmt_node>(std::move(cond), std::move(body));
	}

	stmt_ptr parser::parse_do_while()
	{
		advance(); // 'do'
		auto body = parse_statement();
		expect(token_type::kw_while, "expected 'while' after do body");
		expect(token_type::left_paren, "expected '(' after 'while'");
		auto cond = parse_expression();
		expect(token_type::right_paren, "expected ')' after while condition");
		expect(token_type::semicolon, "expected ';' after do-while");
		return std::make_unique<do_while_stmt_node>(std::move(body), std::move(cond));
	}

	stmt_ptr parser::parse_for()
	{
		advance(); // 'for'
		expect(token_type::left_paren, "expected '(' after 'for'");

		// init
		stmt_ptr init;
		if (is_type_keyword())
		{
			auto type = get_type_name();
			init = parse_local_var_decl(type);
		}
		else if (!check(token_type::semicolon))
		{
			init = parse_expr_statement();
		}
		else
		{
			advance(); // empty init, consume ';'
		}

		// condition
		expr_ptr cond;
		if (!check(token_type::semicolon))
			cond = parse_expression();
		expect(token_type::semicolon, "expected ';' after for condition");

		// increment
		expr_ptr inc;
		if (!check(token_type::right_paren))
			inc = parse_expression();
		expect(token_type::right_paren, "expected ')' after for clauses");

		auto body = parse_statement();
		return std::make_unique<for_stmt_node>(std::move(init), std::move(cond), std::move(inc), std::move(body));
	}

	stmt_ptr parser::parse_switch()
	{
		advance(); // 'switch'
		expect(token_type::left_paren, "expected '(' after 'switch'");
		auto cond = parse_expression();
		expect(token_type::right_paren, "expected ')' after switch expression");
		expect(token_type::left_brace, "expected '{'");

		std::vector<std::unique_ptr<case_clause_node>> cases;

		while (!check(token_type::right_brace) && !check(token_type::eof))
		{
			expr_ptr case_val;
			if (match(token_type::kw_case))
			{
				case_val = parse_expression();
				expect(token_type::colon, "expected ':' after case value");
			}
			else if (match(token_type::kw_default))
			{
				expect(token_type::colon, "expected ':' after 'default'");
			}
			else
			{
				error("expected 'case' or 'default' in switch");
			}

			std::vector<stmt_ptr> body;
			while (!check(token_type::kw_case) && !check(token_type::kw_default) &&
			       !check(token_type::right_brace) && !check(token_type::eof))
			{
				body.push_back(parse_statement());
			}

			cases.push_back(std::make_unique<case_clause_node>(std::move(case_val), std::move(body)));
		}

		expect(token_type::right_brace, "expected '}'");
		return std::make_unique<switch_stmt_node>(std::move(cond), std::move(cases));
	}

	stmt_ptr parser::parse_return()
	{
		advance(); // 'return'
		expr_ptr value;
		if (!check(token_type::semicolon))
			value = parse_expression();
		expect(token_type::semicolon, "expected ';' after return");
		return std::make_unique<return_stmt_node>(std::move(value));
	}

	stmt_ptr parser::parse_break()
	{
		advance(); // 'break'
		expect(token_type::semicolon, "expected ';' after break");
		return std::make_unique<break_stmt_node>();
	}

	stmt_ptr parser::parse_continue()
	{
		advance(); // 'continue'
		expect(token_type::semicolon, "expected ';' after continue");
		return std::make_unique<continue_stmt_node>();
	}

	stmt_ptr parser::parse_wait()
	{
		advance(); // 'wait'
		expect(token_type::left_paren, "expected '(' after 'wait'");
		auto duration = parse_expression();
		expect(token_type::right_paren, "expected ')' after wait duration");
		expect(token_type::semicolon, "expected ';' after wait");
		return std::make_unique<wait_stmt_node>(std::move(duration));
	}

	stmt_ptr parser::parse_expr_statement()
	{
		auto e = parse_expression();
		expect(token_type::semicolon, "expected ';' after expression");
		return std::make_unique<expr_stmt_node>(std::move(e));
	}

	// ── expression parsing (precedence climbing) ──

	expr_ptr parser::parse_expression()
	{
		return parse_assignment();
	}

	expr_ptr parser::parse_assignment()
	{
		auto left = parse_logical_or();

		assign_op op;
		bool is_assign = true;

		switch (current().type)
		{
		case token_type::op_assign:       op = assign_op::assign; break;
		case token_type::op_plus_assign:  op = assign_op::add_assign; break;
		case token_type::op_minus_assign: op = assign_op::sub_assign; break;
		case token_type::op_mul_assign:   op = assign_op::mul_assign; break;
		case token_type::op_div_assign:   op = assign_op::div_assign; break;
		case token_type::op_mod_assign:   op = assign_op::mod_assign; break;
		case token_type::op_and_assign:   op = assign_op::and_assign; break;
		case token_type::op_or_assign:    op = assign_op::or_assign; break;
		case token_type::op_xor_assign:   op = assign_op::xor_assign; break;
		default: is_assign = false; break;
		}

		if (is_assign)
		{
			advance();
			auto right = parse_assignment(); // right-associative
			return std::make_unique<assign_expr_node>(op, std::move(left), std::move(right));
		}

		return left;
	}

	expr_ptr parser::parse_logical_or()
	{
		auto left = parse_logical_and();
		while (check(token_type::op_or))
		{
			advance();
			auto right = parse_logical_and();
			left = std::make_unique<binary_expr_node>(binary_op::logical_or, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_logical_and()
	{
		auto left = parse_bitwise_or();
		while (check(token_type::op_and))
		{
			advance();
			auto right = parse_bitwise_or();
			left = std::make_unique<binary_expr_node>(binary_op::logical_and, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_bitwise_or()
	{
		auto left = parse_bitwise_xor();
		while (check(token_type::op_bit_or))
		{
			advance();
			auto right = parse_bitwise_xor();
			left = std::make_unique<binary_expr_node>(binary_op::bit_or, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_bitwise_xor()
	{
		auto left = parse_bitwise_and();
		while (check(token_type::op_bit_xor))
		{
			advance();
			auto right = parse_bitwise_and();
			left = std::make_unique<binary_expr_node>(binary_op::bit_xor, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_bitwise_and()
	{
		auto left = parse_equality();
		while (check(token_type::op_bit_and))
		{
			advance();
			auto right = parse_equality();
			left = std::make_unique<binary_expr_node>(binary_op::bit_and, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_equality()
	{
		auto left = parse_comparison();
		while (true)
		{
			binary_op op;
			if (check(token_type::op_equal))       op = binary_op::eq;
			else if (check(token_type::op_not_equal)) op = binary_op::neq;
			else break;

			advance();
			auto right = parse_comparison();
			left = std::make_unique<binary_expr_node>(op, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_comparison()
	{
		auto left = parse_shift();
		while (true)
		{
			binary_op op;
			if (check(token_type::op_less))            op = binary_op::lt;
			else if (check(token_type::op_greater))    op = binary_op::gt;
			else if (check(token_type::op_less_eq))    op = binary_op::lte;
			else if (check(token_type::op_greater_eq)) op = binary_op::gte;
			else break;

			advance();
			auto right = parse_shift();
			left = std::make_unique<binary_expr_node>(op, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_shift()
	{
		auto left = parse_additive();
		while (true)
		{
			binary_op op;
			if (check(token_type::op_shift_left))      op = binary_op::shift_left;
			else if (check(token_type::op_shift_right)) op = binary_op::shift_right;
			else break;

			advance();
			auto right = parse_additive();
			left = std::make_unique<binary_expr_node>(op, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_additive()
	{
		auto left = parse_multiplicative();
		while (true)
		{
			binary_op op;
			if (check(token_type::op_plus))      op = binary_op::add;
			else if (check(token_type::op_minus)) op = binary_op::sub;
			else break;

			advance();
			auto right = parse_multiplicative();
			left = std::make_unique<binary_expr_node>(op, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_multiplicative()
	{
		auto left = parse_unary();
		while (true)
		{
			binary_op op;
			if (check(token_type::op_multiply))    op = binary_op::mul;
			else if (check(token_type::op_divide)) op = binary_op::div;
			else if (check(token_type::op_modulo)) op = binary_op::mod;
			else break;

			advance();
			auto right = parse_unary();
			left = std::make_unique<binary_expr_node>(op, std::move(left), std::move(right));
		}
		return left;
	}

	expr_ptr parser::parse_unary()
	{
		if (check(token_type::op_minus))
		{
			advance();
			return std::make_unique<unary_expr_node>(unary_op::negate, parse_unary());
		}
		if (check(token_type::op_not) || check(token_type::kw_not))
		{
			advance();
			return std::make_unique<unary_expr_node>(unary_op::logical_not, parse_unary());
		}
		if (check(token_type::op_bit_not))
		{
			advance();
			return std::make_unique<unary_expr_node>(unary_op::bit_not, parse_unary());
		}
		// prefix increment/decrement
		if (check(token_type::op_increment))
		{
			advance();
			auto operand = parse_postfix();
			return std::make_unique<increment_expr_node>(true, true, std::move(operand));
		}
		if (check(token_type::op_decrement))
		{
			advance();
			auto operand = parse_postfix();
			return std::make_unique<increment_expr_node>(false, true, std::move(operand));
		}

		return parse_postfix();
	}

	expr_ptr parser::parse_postfix()
	{
		auto left = parse_primary();

		while (true)
		{
			// function call
			if (check(token_type::left_paren) && left->kind == node_kind::identifier)
			{
				auto* id = static_cast<identifier_expr*>(left.get());
				std::string name = id->name;
				advance(); // '('

				std::vector<expr_ptr> args;
				if (!check(token_type::right_paren))
				{
					args.push_back(parse_expression());
					while (match(token_type::comma))
						args.push_back(parse_expression());
				}

				expect(token_type::right_paren, "expected ')' after function arguments");
				left = std::make_unique<call_expr_node>(name, std::move(args));
				continue;
			}

			// array index
			if (check(token_type::left_bracket) && left->kind == node_kind::identifier)
			{
				auto* id = static_cast<identifier_expr*>(left.get());
				std::string name = id->name;
				advance(); // '['
				auto index = parse_expression();
				expect(token_type::right_bracket, "expected ']' after index");
				left = std::make_unique<index_expr_node>(name, std::move(index));
				continue;
			}

			// postfix increment/decrement
			if (check(token_type::op_increment))
			{
				advance();
				left = std::make_unique<increment_expr_node>(true, false, std::move(left));
				continue;
			}
			if (check(token_type::op_decrement))
			{
				advance();
				left = std::make_unique<increment_expr_node>(false, false, std::move(left));
				continue;
			}

			break;
		}

		return left;
	}

	expr_ptr parser::parse_primary()
	{
		const auto& tok = current();

		switch (tok.type)
		{
		case token_type::integer_literal:
		{
			advance();
			return std::make_unique<number_literal_expr>(std::stoll(tok.value));
		}

		case token_type::hex_literal:
		{
			advance();
			return std::make_unique<number_literal_expr>(std::stoll(tok.value, nullptr, 16));
		}

		case token_type::float_literal:
		{
			advance();
			return std::make_unique<float_literal_expr>(std::stod(tok.value));
		}

		case token_type::kw_true:
		{
			advance();
			return std::make_unique<number_literal_expr>(1);
		}

		case token_type::kw_false:
		{
			advance();
			return std::make_unique<number_literal_expr>(0);
		}

		case token_type::identifier:
		{
			advance();
			return std::make_unique<identifier_expr>(tok.value);
		}

		case token_type::left_paren:
		{
			advance();
			auto e = parse_expression();
			expect(token_type::right_paren, "expected ')'");
			return e;
		}

		default:
			error("unexpected token '" + tok.value + "' in expression");
		}
	}
}
