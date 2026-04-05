#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace gpc
{
	enum class token_type
	{
		// literals
		integer_literal,
		hex_literal,
		float_literal,
		string_literal,

		// identifier
		identifier,

		// keywords
		kw_define,
		kw_data,
		kw_remap,
		kw_unmap,
		kw_int,
		kw_int8,
		kw_int16,
		kw_int32,
		kw_const,
		kw_init,
		kw_main,
		kw_combo,
		kw_function,
		kw_if,
		kw_else,
		kw_while,
		kw_do,
		kw_for,
		kw_switch,
		kw_case,
		kw_default,
		kw_break,
		kw_continue,
		kw_return,
		kw_wait,
		kw_true,
		kw_false,
		kw_not,

		// operators
		op_plus,        // +
		op_minus,       // -
		op_multiply,    // *
		op_divide,      // /
		op_modulo,      // %
		op_assign,      // =
		op_equal,       // ==
		op_not_equal,   // !=
		op_less,        // <
		op_greater,     // >
		op_less_eq,     // <=
		op_greater_eq,  // >=
		op_and,         // &&
		op_or,          // ||
		op_not,         // !
		op_bit_and,     // &
		op_bit_or,      // |
		op_bit_xor,     // ^
		op_bit_not,     // ~
		op_shift_left,  // <<
		op_shift_right, // >>
		op_increment,   // ++
		op_decrement,   // --
		op_plus_assign, // +=
		op_minus_assign,// -=
		op_mul_assign,  // *=
		op_div_assign,  // /=
		op_mod_assign,  // %=
		op_and_assign,  // &=
		op_or_assign,   // |=
		op_xor_assign,  // ^=

		// delimiters
		left_paren,     // (
		right_paren,    // )
		left_brace,     // {
		right_brace,    // }
		left_bracket,   // [
		right_bracket,  // ]
		semicolon,      // ;
		comma,          // ,
		dot,            // .
		colon,          // :

		// special
		eof,
		unknown,
	};

	struct token
	{
		token_type type;
		std::string value;
		int line;
		int column;
	};

	class lexer
	{
	public:
		lexer();

		std::vector<token> tokenize(const std::string& source, const std::string& filename = "<unknown>");

	private:
		std::unordered_map<std::string, token_type> keywords_;
		std::string source_;
		std::string filename_;
		size_t pos_;
		int line_;
		int column_;

		char current() const;
		char peek(int offset = 1) const;
		char advance();
		void skip_whitespace_and_comments();
		token read_number();
		token read_identifier_or_keyword();
		token read_string();
	};
}
