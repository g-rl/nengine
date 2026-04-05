#include <std_include.hpp>
#include "gpc_lexer.hpp"

namespace gpc
{
	lexer::lexer()
		: pos_(0), line_(1), column_(1)
	{
		keywords_["define"]   = token_type::kw_define;
		keywords_["data"]     = token_type::kw_data;
		keywords_["remap"]    = token_type::kw_remap;
		keywords_["unmap"]    = token_type::kw_unmap;
		keywords_["int"]      = token_type::kw_int;
		keywords_["int8"]     = token_type::kw_int8;
		keywords_["int16"]    = token_type::kw_int16;
		keywords_["int32"]    = token_type::kw_int32;
		keywords_["const"]    = token_type::kw_const;
		keywords_["init"]     = token_type::kw_init;
		keywords_["main"]     = token_type::kw_main;
		keywords_["combo"]    = token_type::kw_combo;
		keywords_["function"] = token_type::kw_function;
		keywords_["if"]       = token_type::kw_if;
		keywords_["else"]     = token_type::kw_else;
		keywords_["while"]    = token_type::kw_while;
		keywords_["do"]       = token_type::kw_do;
		keywords_["for"]      = token_type::kw_for;
		keywords_["switch"]   = token_type::kw_switch;
		keywords_["case"]     = token_type::kw_case;
		keywords_["default"]  = token_type::kw_default;
		keywords_["break"]    = token_type::kw_break;
		keywords_["continue"] = token_type::kw_continue;
		keywords_["return"]   = token_type::kw_return;
		keywords_["wait"]     = token_type::kw_wait;
		keywords_["TRUE"]     = token_type::kw_true;
		keywords_["FALSE"]    = token_type::kw_false;
		keywords_["NOT"]      = token_type::kw_not;
	}

	std::vector<token> lexer::tokenize(const std::string& source, const std::string& filename)
	{
		source_ = source;
		filename_ = filename;
		pos_ = 0;
		line_ = 1;
		column_ = 1;

		std::vector<token> tokens;
		tokens.reserve(source.size() / 4);

		while (pos_ < source_.size())
		{
			skip_whitespace_and_comments();
			if (pos_ >= source_.size()) break;

			const char c = current();
			const int tok_line = line_;
			const int tok_col = column_;

			// numbers
			if (c >= '0' && c <= '9')
			{
				tokens.push_back(read_number());
				continue;
			}

			// identifiers and keywords
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
			{
				tokens.push_back(read_identifier_or_keyword());
				continue;
			}

			// string literals
			if (c == '"')
			{
				tokens.push_back(read_string());
				continue;
			}

			// two-character operators
			const char n = peek();

			switch (c)
			{
			case '+':
				advance();
				if (n == '+') { advance(); tokens.push_back({token_type::op_increment, "++", tok_line, tok_col}); }
				else if (n == '=') { advance(); tokens.push_back({token_type::op_plus_assign, "+=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_plus, "+", tok_line, tok_col}); }
				continue;

			case '-':
				advance();
				if (n == '-') { advance(); tokens.push_back({token_type::op_decrement, "--", tok_line, tok_col}); }
				else if (n == '=') { advance(); tokens.push_back({token_type::op_minus_assign, "-=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_minus, "-", tok_line, tok_col}); }
				continue;

			case '*':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_mul_assign, "*=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_multiply, "*", tok_line, tok_col}); }
				continue;

			case '/':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_div_assign, "/=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_divide, "/", tok_line, tok_col}); }
				continue;

			case '%':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_mod_assign, "%=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_modulo, "%", tok_line, tok_col}); }
				continue;

			case '=':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_equal, "==", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_assign, "=", tok_line, tok_col}); }
				continue;

			case '!':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_not_equal, "!=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_not, "!", tok_line, tok_col}); }
				continue;

			case '<':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_less_eq, "<=", tok_line, tok_col}); }
				else if (n == '<') { advance(); tokens.push_back({token_type::op_shift_left, "<<", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_less, "<", tok_line, tok_col}); }
				continue;

			case '>':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_greater_eq, ">=", tok_line, tok_col}); }
				else if (n == '>') { advance(); tokens.push_back({token_type::op_shift_right, ">>", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_greater, ">", tok_line, tok_col}); }
				continue;

			case '&':
				advance();
				if (n == '&') { advance(); tokens.push_back({token_type::op_and, "&&", tok_line, tok_col}); }
				else if (n == '=') { advance(); tokens.push_back({token_type::op_and_assign, "&=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_bit_and, "&", tok_line, tok_col}); }
				continue;

			case '|':
				advance();
				if (n == '|') { advance(); tokens.push_back({token_type::op_or, "||", tok_line, tok_col}); }
				else if (n == '=') { advance(); tokens.push_back({token_type::op_or_assign, "|=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_bit_or, "|", tok_line, tok_col}); }
				continue;

			case '^':
				advance();
				if (n == '=') { advance(); tokens.push_back({token_type::op_xor_assign, "^=", tok_line, tok_col}); }
				else { tokens.push_back({token_type::op_bit_xor, "^", tok_line, tok_col}); }
				continue;

			case '~': advance(); tokens.push_back({token_type::op_bit_not, "~", tok_line, tok_col}); continue;

			// delimiters
			case '(': advance(); tokens.push_back({token_type::left_paren, "(", tok_line, tok_col}); continue;
			case ')': advance(); tokens.push_back({token_type::right_paren, ")", tok_line, tok_col}); continue;
			case '{': advance(); tokens.push_back({token_type::left_brace, "{", tok_line, tok_col}); continue;
			case '}': advance(); tokens.push_back({token_type::right_brace, "}", tok_line, tok_col}); continue;
			case '[': advance(); tokens.push_back({token_type::left_bracket, "[", tok_line, tok_col}); continue;
			case ']': advance(); tokens.push_back({token_type::right_bracket, "]", tok_line, tok_col}); continue;
			case ';': advance(); tokens.push_back({token_type::semicolon, ";", tok_line, tok_col}); continue;
			case ',': advance(); tokens.push_back({token_type::comma, ",", tok_line, tok_col}); continue;
			case '.': advance(); tokens.push_back({token_type::dot, ".", tok_line, tok_col}); continue;
		case ':': advance(); tokens.push_back({token_type::colon, ":", tok_line, tok_col}); continue;

			default:
				advance();
				tokens.push_back({token_type::unknown, std::string(1, c), tok_line, tok_col});
				continue;
			}
		}

		tokens.push_back({token_type::eof, "", line_, column_});
		return tokens;
	}

	char lexer::current() const
	{
		if (pos_ >= source_.size()) return '\0';
		return source_[pos_];
	}

	char lexer::peek(int offset) const
	{
		const size_t idx = pos_ + offset;
		if (idx >= source_.size()) return '\0';
		return source_[idx];
	}

	char lexer::advance()
	{
		if (pos_ >= source_.size()) return '\0';
		const char c = source_[pos_++];
		if (c == '\n') { line_++; column_ = 1; }
		else { column_++; }
		return c;
	}

	void lexer::skip_whitespace_and_comments()
	{
		while (pos_ < source_.size())
		{
			const char c = current();

			if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
			{
				advance();
				continue;
			}

			// line comment
			if (c == '/' && peek() == '/')
			{
				while (pos_ < source_.size() && current() != '\n')
					advance();
				continue;
			}

			// block comment
			if (c == '/' && peek() == '*')
			{
				advance(); // /
				advance(); // *
				while (pos_ < source_.size())
				{
					if (current() == '*' && peek() == '/')
					{
						advance(); // *
						advance(); // /
						break;
					}
					advance();
				}
				continue;
			}

			break;
		}
	}

	token lexer::read_number()
	{
		const int tok_line = line_;
		const int tok_col = column_;
		std::string num;

		// hex literal
		if (current() == '0' && (peek() == 'x' || peek() == 'X'))
		{
			num += advance(); // 0
			num += advance(); // x
			while (pos_ < source_.size())
			{
				const char c = current();
				if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
					num += advance();
				else
					break;
			}
			return {token_type::hex_literal, num, tok_line, tok_col};
		}

		// decimal or float
		bool is_float = false;
		while (pos_ < source_.size())
		{
			const char c = current();
			if (c >= '0' && c <= '9')
			{
				num += advance();
			}
			else if (c == '.' && !is_float)
			{
				is_float = true;
				num += advance();
			}
			else
			{
				break;
			}
		}

		if (is_float)
			return {token_type::float_literal, num, tok_line, tok_col};
		return {token_type::integer_literal, num, tok_line, tok_col};
	}

	token lexer::read_identifier_or_keyword()
	{
		const int tok_line = line_;
		const int tok_col = column_;
		std::string id;

		while (pos_ < source_.size())
		{
			const char c = current();
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
				id += advance();
			else
				break;
		}

		auto it = keywords_.find(id);
		if (it != keywords_.end())
			return {it->second, id, tok_line, tok_col};

		return {token_type::identifier, id, tok_line, tok_col};
	}

	token lexer::read_string()
	{
		const int tok_line = line_;
		const int tok_col = column_;
		advance(); // opening quote

		std::string str;
		while (pos_ < source_.size() && current() != '"')
		{
			if (current() == '\\')
			{
				advance();
				switch (current())
				{
				case 'n': str += '\n'; break;
				case 't': str += '\t'; break;
				case '\\': str += '\\'; break;
				case '"': str += '"'; break;
				default: str += current(); break;
				}
				advance();
			}
			else
			{
				str += advance();
			}
		}

		if (pos_ < source_.size()) advance(); // closing quote
		return {token_type::string_literal, str, tok_line, tok_col};
	}
}
