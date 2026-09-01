/**************************************************************************/
/*  expression.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "expression.h"

Error Expression::_get_token(Token& r_token)
{
	while (true) {
#define GET_CHAR() (str_ofs >= expression.length() ? 0 : expression[str_ofs++])

		char32_t cchar = GET_CHAR();

		switch (cchar) {
		case 0: {
			r_token.type = TK_EOF;
			return OK;
		}
		case '{': {
			r_token.type = TK_CURLY_BRACKET_OPEN;
			return OK;
		}
		case '}': {
			r_token.type = TK_CURLY_BRACKET_CLOSE;
			return OK;
		}
		case '[': {
			r_token.type = TK_BRACKET_OPEN;
			return OK;
		}
		case ']': {
			r_token.type = TK_BRACKET_CLOSE;
			return OK;
		}
		case '(': {
			r_token.type = TK_PARENTHESIS_OPEN;
			return OK;
		}
		case ')': {
			r_token.type = TK_PARENTHESIS_CLOSE;
			return OK;
		}
		case ',': {
			r_token.type = TK_COMMA;
			return OK;
		}
		case ':': {
			r_token.type = TK_COLON;
			return OK;
		}
		case '$': {
			r_token.type = TK_INPUT;
			int index = 0;
			do {
				if (!is_digit(expression[str_ofs])) {
					_set_error("Expected number after '$'");
					r_token.type = TK_ERROR;
					return ERR_PARSE_ERROR;
				}
				index *= 10;
				index += expression[str_ofs] - '0';
				str_ofs++;

			} while (is_digit(expression[str_ofs]));
			return OK;
		}
		case '=': {
			cchar = GET_CHAR();
			if (cchar == '=') {
				r_token.type = TK_OP_EQUAL;
			}
			else {
				_set_error("Expected '='");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}
			return OK;
		}
		case '!': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_NOT_EQUAL;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_NOT;
			}
			return OK;
		}
		case '>': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_GREATER_EQUAL;
				str_ofs++;
			}
			else if (expression[str_ofs] == '>') {
				r_token.type = TK_OP_SHIFT_RIGHT;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_GREATER;
			}
			return OK;
		}
		case '<': {
			if (expression[str_ofs] == '=') {
				r_token.type = TK_OP_LESS_EQUAL;
				str_ofs++;
			}
			else if (expression[str_ofs] == '<') {
				r_token.type = TK_OP_SHIFT_LEFT;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_LESS;
			}
			return OK;
		}
		case '+': {
			r_token.type = TK_OP_ADD;
			return OK;
		}
		case '-': {
			r_token.type = TK_OP_SUB;
			return OK;
		}
		case '/': {
			r_token.type = TK_OP_DIV;
			return OK;
		}
		case '*': {
			if (expression[str_ofs] == '*') {
				r_token.type = TK_OP_POW;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_MUL;
			}
			return OK;
		}
		case '%': {
			r_token.type = TK_OP_MOD;
			return OK;
		}
		case '&': {
			if (expression[str_ofs] == '&') {
				r_token.type = TK_OP_AND;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_BIT_AND;
			}
			return OK;
		}
		case '|': {
			if (expression[str_ofs] == '|') {
				r_token.type = TK_OP_OR;
				str_ofs++;
			}
			else {
				r_token.type = TK_OP_BIT_OR;
			}
			return OK;
		}
		case '^': {
			r_token.type = TK_OP_BIT_XOR;

			return OK;
		}
		case '~': {
			r_token.type = TK_OP_BIT_INVERT;

			return OK;
		}
		case '\'':
		case '"': {
			String str;
			char32_t prev = 0;
			while (true) {
				char32_t ch = GET_CHAR();

				if (ch == 0) {
					_set_error("Unterminated String");
					r_token.type = TK_ERROR;
					return ERR_PARSE_ERROR;
				}
				else if (ch == cchar) {
					// cchar contain a corresponding quote symbol
					break;
				}
				else if (ch == '\\') {
					// escaped characters...

					char32_t next = GET_CHAR();
					if (next == 0) {
						_set_error("Unterminated String");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					char32_t res = 0;

					switch (next) {
					case 'b':
						res = 8;
						break;
					case 't':
						res = 9;
						break;
					case 'n':
						res = 10;
						break;
					case 'f':
						res = 12;
						break;
					case 'r':
						res = 13;
						break;
					case 'U':
					case 'u': {
						// Hexadecimal sequence.
						int hex_len = (next == 'U') ? 6 : 4;
						for (int j = 0; j < hex_len; j++) {
							char32_t c = GET_CHAR();

							if (c == 0) {
								_set_error("Unterminated String");
								r_token.type = TK_ERROR;
								return ERR_PARSE_ERROR;
							}
							if (!is_hex_digit(c)) {
								_set_error("Malformed hex constant in string");
								r_token.type = TK_ERROR;
								return ERR_PARSE_ERROR;
							}
							char32_t v;
							if (is_digit(c)) {
								v = c - '0';
							}
							else if (c >= 'a' && c <= 'f') {
								v = c - 'a';
								v += 10;
							}
							else if (c >= 'A' && c <= 'F') {
								v = c - 'A';
								v += 10;
							}
							else {
								ERR_PRINT("Bug parsing hex constant.");
								v = 0;
							}

							res <<= 4;
							res |= v;
						}

					} break;
					default: {
						res = next;
					} break;
					}

					// Parse UTF-16 pair.
					if ((res & 0xfffffc00) == 0xd800) {
						if (prev == 0) {
							prev = res;
							continue;
						}
						else {
							_set_error(
								"Invalid UTF-16 sequence in string, unpaired lead surrogate");
							r_token.type = TK_ERROR;
							return ERR_PARSE_ERROR;
						}
					}
					else if ((res & 0xfffffc00) == 0xdc00) {
						if (prev == 0) {
							_set_error(
								"Invalid UTF-16 sequence in string, unpaired trail surrogate");
							r_token.type = TK_ERROR;
							return ERR_PARSE_ERROR;
						}
						else {
							res = (prev << 10UL) + res - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
							prev = 0;
						}
					}
					if (prev != 0) {
						_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					str += res;
				}
				else {
					if (prev != 0) {
						_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
						r_token.type = TK_ERROR;
						return ERR_PARSE_ERROR;
					}
					str += ch;
				}
			}
			if (prev != 0) {
				_set_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}

			r_token.type = TK_CONSTANT;
			return OK;

		} break;
		default: {
			if (cchar <= 32) {
				break;
			}

			char32_t next_char = (str_ofs >= expression.length()) ? 0 : expression[str_ofs];
			if (is_digit(cchar) || (cchar == '.' && is_digit(next_char))) {
				// a number

				String num;
#define READING_SIGN 0
#define READING_INT 1
#define READING_HEX 2
#define READING_BIN 3
#define READING_DEC 4
#define READING_EXP 5
#define READING_DONE 6
				int reading = READING_INT;

				char32_t c = cchar;
				bool exp_sign = false;
				bool exp_beg = false;
				bool bin_beg = false;
				bool hex_beg = false;
				bool is_float = false;
				bool is_first_char = true;

				while (true) {
					switch (reading) {
					case READING_INT: {
						if (is_digit(c)) {
							if (is_first_char && c == '0') {
								if (next_char == 'b' || next_char == 'B') {
									reading = READING_BIN;
								}
								else if (next_char == 'x' || next_char == 'X') {
									reading = READING_HEX;
								}
							}
						}
						else if (c == '.') {
							reading = READING_DEC;
							is_float = true;
						}
						else if (c == 'e' || c == 'E') {
							reading = READING_EXP;
							is_float = true;
						}
						else {
							reading = READING_DONE;
						}

					} break;
					case READING_BIN: {
						if (bin_beg && !is_binary_digit(c)) {
							reading = READING_DONE;
						}
						else if (c == 'b' || c == 'B') {
							bin_beg = true;
						}

					} break;
					case READING_HEX: {
						if (hex_beg && !is_hex_digit(c)) {
							reading = READING_DONE;
						}
						else if (c == 'x' || c == 'X') {
							hex_beg = true;
						}

					} break;
					case READING_DEC: {
						if (is_digit(c)) {
						}
						else if (c == 'e' || c == 'E') {
							reading = READING_EXP;
						}
						else {
							reading = READING_DONE;
						}

					} break;
					case READING_EXP: {
						if (is_digit(c)) {
							exp_beg = true;

						}
						else if ((c == '-' || c == '+') && !exp_sign && !exp_beg) {
							exp_sign = true;

						}
						else {
							reading = READING_DONE;
						}
					} break;
					}

					if (reading == READING_DONE) {
						break;
					}
					num += c;
					c = GET_CHAR();
					is_first_char = false;
				}

				if (c != 0) {
					str_ofs--;
				}

				r_token.type = TK_CONSTANT;

				return OK;

			}
			else if (is_unicode_identifier_start(cchar)) {
				String id = String::chr(cchar);
				cchar = GET_CHAR();

				while (is_unicode_identifier_continue(cchar)) {
					id += cchar;
					cchar = GET_CHAR();
				}

				str_ofs--; // go back one

				if (id == "in") {
					r_token.type = TK_OP_IN;
				}
				else if (id == "null") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "true") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "false") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "PI") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "TAU") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "INF") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "NAN") {
					r_token.type = TK_CONSTANT;
				}
				else if (id == "not") {
					r_token.type = TK_OP_NOT;
				}
				else if (id == "or") {
					r_token.type = TK_OP_OR;
				}
				else if (id == "and") {
					r_token.type = TK_OP_AND;
				}
				else if (id == "self") {
					r_token.type = TK_SELF;
				}
				else {
					r_token.type = TK_IDENTIFIER;
				}
				return OK;

			}
			else if (cchar == '.') {
				// Handled down there as we support '.[0-9]' as numbers above
				r_token.type = TK_PERIOD;
				return OK;

			}
			else {
				_set_error("Unexpected character.");
				r_token.type = TK_ERROR;
				return ERR_PARSE_ERROR;
			}
		}
		}
#undef GET_CHAR
	}

	r_token.type = TK_ERROR;
	return ERR_PARSE_ERROR;
}

const char* Expression::token_name[TK_MAX] = {"CURLY BRACKET OPEN", "CURLY BRACKET CLOSE",
	"BRACKET OPEN", "BRACKET CLOSE", "PARENTHESIS OPEN", "PARENTHESIS CLOSE", "IDENTIFIER",
	"BUILTIN FUNC", "SELF", "CONSTANT", "BASIC TYPE", "COLON", "COMMA", "PERIOD", "OP IN",
	"OP EQUAL", "OP NOT EQUAL", "OP LESS", "OP LESS EQUAL", "OP GREATER", "OP GREATER EQUAL",
	"OP AND", "OP OR", "OP NOT", "OP ADD", "OP SUB", "OP MUL", "OP DIV", "OP MOD", "OP POW",
	"OP SHIFT LEFT", "OP SHIFT RIGHT", "OP BIT AND", "OP BIT OR", "OP BIT XOR", "OP BIT INVERT",
	"OP INPUT", "EOF", "ERROR"};

bool Expression::_compile_expression()
{
	if (!expression_dirty) {
		return error_set;
	}

	if (nodes) {
		memdelete(nodes);
		nodes = nullptr;
		root = nullptr;
	}

	error_str = String();
	error_set = false;
	str_ofs = 0;

	root = _parse_expression();

	if (error_set) {
		root = nullptr;
		memdelete(nodes);
		nodes = nullptr;
		return true;
	}

	expression_dirty = false;
	return false;
}

Error Expression::parse(const String& p_expression, const Vector<String>& p_input_names)
{
	if (nodes) {
		memdelete(nodes);
		nodes = nullptr;
		root = nullptr;
	}

	error_str = String();
	error_set = false;
	str_ofs = 0;
	input_names = p_input_names;

	expression = p_expression;
	root = _parse_expression();

	if (error_set) {
		root = nullptr;
		memdelete(nodes);
		nodes = nullptr;
		return ERR_INVALID_PARAMETER;
	}

	return OK;
}

bool Expression::has_execute_failed() const { return execution_error; }

String Expression::get_error_text() const { return error_str; }

void Expression::_bind_methods() {}

Expression::~Expression() { memdelete(nodes); }


