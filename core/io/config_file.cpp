/**************************************************************************/
/*  config_file.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "config_file.h"
#include "core/io/file_access_wrappers.h"
#include "core/string/string_builder.h"

void ConfigFile::erase_section_key(const String& p_section, const String& p_key) {}

bool ConfigFile::has_section_key(const String& p_section, const String& p_key) const
{
	return true;
}

Error ConfigFile::save(const String& p_path)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err) {
		return err;
	}

	return _internal_save(file);
}

Error ConfigFile::save_encrypted(const String& p_path, const Vector<uint8_t>& p_key)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse(f, p_key, FileAccessEncrypted::MODE_WRITE_AES256);
	if (err) {
		return err;
	}
	return _internal_save(fae);
}

Error ConfigFile::save_encrypted_pass(const String& p_path, const String& p_pass)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse_password(f, p_pass, FileAccessEncrypted::MODE_WRITE_AES256);
	if (err) {
		return err;
	}

	return _internal_save(fae);
}

Error ConfigFile::load(const String& p_path)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (f.is_null()) {
		return err;
	}

	return _internal_load(p_path, f);
}

Error ConfigFile::load_encrypted(const String& p_path, const Vector<uint8_t>& p_key)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse(f, p_key, FileAccessEncrypted::MODE_READ);
	if (err) {
		return err;
	}
	return _internal_load(p_path, fae);
}

Error ConfigFile::load_encrypted_pass(const String& p_path, const String& p_pass)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (err) {
		return err;
	}

	Ref<FileAccessEncrypted> fae;
	fae.instantiate();
	err = fae->open_and_parse_password(f, p_pass, FileAccessEncrypted::MODE_READ);
	if (err) {
		return err;
	}

	return _internal_load(p_path, fae);
}



Error ConfigFile::_internal_save(Ref<FileAccess>) {}

Error ConfigFile::_internal_load(String const&, Ref<FileAccess>) {}

void ConfigFile::erase_section(String const&) {}

bool ConfigFile::has_section(String const&) const {}

// ==========================================================================
// json.cpp
// ==========================================================================

/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "core/io/resource_loader.h"
#include "json.h"

const char* JSON::tk_name[TK_MAX] = {
	"'{'",
	"'}'",
	"'['",
	"']'",
	"identifier",
	"string",
	"number",
	"':'",
	"','",
	"EOF",
};

void JSON::_add_indent(String& r_result, const String& p_indent, int p_size)
{
	for (int i = 0; i < p_size; i++) {
		r_result += p_indent;
	}
}

Error JSON::_get_token(
	const char32_t* p_str, int& r_index, int p_len, Token& r_token, int& r_line, String& r_err_str)
{
	while (p_len > 0) {
		switch (p_str[r_index]) {
		case '\n': {
			r_line++;
			r_index++;
			break;
		}
		case 0: {
			r_token.type = TK_EOF;
			return OK;
		} break;
		case '{': {
			r_token.type = TK_CURLY_BRACKET_OPEN;
			r_index++;
			return OK;
		}
		case '}': {
			r_token.type = TK_CURLY_BRACKET_CLOSE;
			r_index++;
			return OK;
		}
		case '[': {
			r_token.type = TK_BRACKET_OPEN;
			r_index++;
			return OK;
		}
		case ']': {
			r_token.type = TK_BRACKET_CLOSE;
			r_index++;
			return OK;
		}
		case ':': {
			r_token.type = TK_COLON;
			r_index++;
			return OK;
		}
		case ',': {
			r_token.type = TK_COMMA;
			r_index++;
			return OK;
		}
		case '"': {
			r_index++;
			String str;
			while (true) {
				if (p_str[r_index] == 0) {
					r_err_str = "Unterminated string";
					return ERR_PARSE_ERROR;
				}
				else if (p_str[r_index] == '"') {
					r_index++;
					break;
				}
				else if (p_str[r_index] == '\\') {
					// escaped characters...
					r_index++;
					char32_t next = p_str[r_index];
					if (next == 0) {
						r_err_str = "Unterminated string";
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
					case 'u': {
						// hex number
						for (int j = 0; j < 4; j++) {
							char32_t c = p_str[r_index + j + 1];
							if (c == 0) {
								r_err_str = "Unterminated string";
								return ERR_PARSE_ERROR;
							}
							if (!is_hex_digit(c)) {
								r_err_str = "Malformed hex constant in string";
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
						r_index += 4; // will add at the end anyway

						if ((res & 0xfffffc00) == 0xd800) {
							if (p_str[r_index + 1] != '\\' || p_str[r_index + 2] != 'u') {
								r_err_str =
									"Invalid UTF-16 sequence in string, unpaired lead surrogate";
								return ERR_PARSE_ERROR;
							}
							r_index += 2;
							char32_t trail = 0;
							for (int j = 0; j < 4; j++) {
								char32_t c = p_str[r_index + j + 1];
								if (c == 0) {
									r_err_str = "Unterminated string";
									return ERR_PARSE_ERROR;
								}
								if (!is_hex_digit(c)) {
									r_err_str = "Malformed hex constant in string";
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

								trail <<= 4;
								trail |= v;
							}
							if ((trail & 0xfffffc00) == 0xdc00) {
								res = (res << 10UL) + trail - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
								r_index += 4; // will add at the end anyway
							}
							else {
								r_err_str =
									"Invalid UTF-16 sequence in string, unpaired lead surrogate";
								return ERR_PARSE_ERROR;
							}
						}
						else if ((res & 0xfffffc00) == 0xdc00) {
							r_err_str =
								"Invalid UTF-16 sequence in string, unpaired trail surrogate";
							return ERR_PARSE_ERROR;
						}

					} break;
					case '"':
					case '\\':
					case '/': {
						res = next;
					} break;
					default: {
						r_err_str = "Invalid escape sequence";
						return ERR_PARSE_ERROR;
					}
					}

					str += res;

				}
				else {
					if (p_str[r_index] == '\n') {
						r_line++;
					}
					str += p_str[r_index];
				}
				r_index++;
			}

			r_token.type = TK_STRING;
			return OK;

		} break;
		default: {
			if (p_str[r_index] <= 32) {
				r_index++;
				break;
			}

			if (p_str[r_index] == '-' || is_digit(p_str[r_index])) {
				// a number
				const char32_t* rptr;
				double number = String::to_float(&p_str[r_index], &rptr);
				r_index += (rptr - &p_str[r_index]);
				r_token.type = TK_NUMBER;
				return OK;

			}
			else if (is_ascii_alphabet_char(p_str[r_index])) {
				String id;

				while (is_ascii_alphabet_char(p_str[r_index])) {
					id += p_str[r_index];
					r_index++;
				}

				r_token.type = TK_IDENTIFIER;
				return OK;
			}
			else {
				r_err_str = "Unexpected character";
				return ERR_PARSE_ERROR;
			}
		}
		}
	}

	r_err_str = "Unknown error getting token";
	return ERR_PARSE_ERROR;
}

String JSON::get_parsed_text() const { return text; }



Error JSON::parse(String const&, bool) {}

// ==========================================================================
// json_resource_format.cpp
// ==========================================================================

/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "core/config/engine.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "json_resource_format.h"

Ref<Resource> ResourceFormatLoaderJSON::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	if (!FileAccess::exists(p_path)) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		return Ref<Resource>();
	}

	Ref<JSON> json;
	json.instantiate();

	Error err = json->parse(
		FileAccess::get_file_as_string(p_path), Engine::is_editor_hint());
	if (err != OK) {
		String err_text = "Error parsing JSON file at '" + p_path + "', on line " +
						  itos(json->get_error_line()) + ": " + json->get_error_message();

		if (Engine::is_editor_hint()) {
			// If running on editor, still allow opening the JSON so the code editor can edit it.
			WARN_PRINT(err_text);
		}
		else {
			if (r_error) {
				*r_error = err;
			}
			ERR_PRINT(err_text);
			return Ref<Resource>();
		}
	}

	if (r_error) {
		*r_error = OK;
	}

	return json;
}

void ResourceFormatLoaderJSON::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("json");
}

bool ResourceFormatLoaderJSON::handles_type(const String& p_type) const
{
	return (p_type == "JSON");
}

String ResourceFormatLoaderJSON::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("json")) {
		return "JSON";
	}
	return "";
}

Error ResourceFormatSaverJSON::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Ref<JSON> json = p_resource;
	ERR_FAIL_COND_V(json.is_null(), ERR_INVALID_PARAMETER);

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	ERR_FAIL_COND_V_MSG(err, err, vformat("Cannot save json '%s'.", p_path));

	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

void ResourceFormatSaverJSON::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	Ref<JSON> json = p_resource;
	if (json.is_valid()) {
		p_extensions->push_back("json");
	}
}

bool ResourceFormatSaverJSON::recognize(const Ref<Resource>& p_resource) const
{
	return false;
}

// ==========================================================================
// plist.cpp
// ==========================================================================

/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "core/crypto/crypto_core.h"
#include "core/os/time.h"
#include "plist.h"

PList::PLNodeType PListNode::get_type() const { return data_type; }

Ref<PListNode> PListNode::new_array()
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_ARRAY;
	return node;
}

Ref<PListNode> PListNode::new_dict()
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_DICT;
	return node;
}

Ref<PListNode> PListNode::new_string(const String& p_string)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_STRING;
	node->data_string = p_string.utf8();
	return node;
}

Ref<PListNode> PListNode::new_data(const String& p_string)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_DATA;
	node->data_string = p_string.utf8();
	return node;
}

Ref<PListNode> PListNode::new_date(const String& p_string)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_DATE;
	node->data_string = p_string.utf8();
	node->data_real =
		(double)Time::get_unix_time_from_datetime_string(p_string) - 978307200.0;
	return node;
}

Ref<PListNode> PListNode::new_bool(bool p_bool)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_BOOLEAN;
	node->data_bool = p_bool;
	return node;
}

Ref<PListNode> PListNode::new_int(int64_t p_int)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_INTEGER;
	node->data_int = p_int;
	return node;
}

Ref<PListNode> PListNode::new_real(double p_real)
{
	Ref<PListNode> node = memnew(PListNode());
	ERR_FAIL_COND_V(node.is_null(), Ref<PListNode>());
	node->data_type = PList::PLNodeType::PL_NODE_TYPE_REAL;
	node->data_real = p_real;
	return node;
}

bool PListNode::push_subnode(const Ref<PListNode>& p_node, const String& p_key)
{
	ERR_FAIL_COND_V(p_node.is_null(), false);
	if (data_type == PList::PLNodeType::PL_NODE_TYPE_DICT) {
		ERR_FAIL_COND_V(p_key.is_empty(), false);
		ERR_FAIL_COND_V(data_dict.has(p_key), false);
		data_dict[p_key] = p_node;
		return true;
	}
	else if (data_type == PList::PLNodeType::PL_NODE_TYPE_ARRAY) {
		data_array.push_back(p_node);
		return true;
	}
	else {
		ERR_FAIL_V_MSG(false, "PList: Invalid parent node type, should be DICT or ARRAY.");
	}
}

size_t PListNode::get_asn1_size(uint8_t p_len_octets) const
{
	// Get size of all data, excluding type and size information.
	switch (data_type) {
	case PList::PLNodeType::PL_NODE_TYPE_NIL: {
		return 0;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATA:
	case PList::PLNodeType::PL_NODE_TYPE_DATE: {
		ERR_FAIL_V_MSG(0, "PList: DATE and DATA nodes are not supported by ASN.1 serialization.");
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_STRING: {
		return data_string.length();
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_BOOLEAN: {
		return 1;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_INTEGER:
	case PList::PLNodeType::PL_NODE_TYPE_REAL: {
		return 4;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_ARRAY: {
		size_t size = 0;
		for (int i = 0; i < data_array.size(); i++) {
			size += 1 + _asn1_size_len(p_len_octets) + data_array[i]->get_asn1_size(p_len_octets);
		}
		return size;
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DICT: {
		size_t size = 0;

		for (const KeyValue<String, Ref<PListNode>>& E : data_dict) {
			size += 1 + _asn1_size_len(p_len_octets);						  // Sequence.
			size += 1 + _asn1_size_len(p_len_octets) + E.key.utf8().length(); // Key.
			size +=
				1 + _asn1_size_len(p_len_octets) + E.value->get_asn1_size(p_len_octets); // Value.
		}
		return size;
	} break;
	default: {
		return 0;
	} break;
	}
}

int PListNode::_asn1_size_len(uint8_t p_len_octets)
{
	if (p_len_octets > 1) {
		return p_len_octets + 1;
	}
	else {
		return 1;
	}
}

void PListNode::store_asn1_size(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const
{
	uint32_t size = get_asn1_size(p_len_octets);
	if (p_len_octets > 1) {
		p_stream.push_back(0x80 + p_len_octets);
	}
	for (int i = p_len_octets - 1; i >= 0; i--) {
		uint8_t x = (size >> i * 8) & 0xFF;
		p_stream.push_back(x);
	}
}

bool PListNode::store_asn1(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const
{
	// Convert to binary ASN1 stream.
	bool valid = true;
	switch (data_type) {
	case PList::PLNodeType::PL_NODE_TYPE_NIL: {
		// Nothing to store.
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATE:
	case PList::PLNodeType::PL_NODE_TYPE_DATA: {
		ERR_FAIL_V_MSG(
			false, "PList: DATE and DATA nodes are not supported by ASN.1 serialization.");
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_STRING: {
		p_stream.push_back(0x0C);
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 0; i < data_string.size(); i++) {
			p_stream.push_back(data_string[i]);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_BOOLEAN: {
		p_stream.push_back(0x01);
		store_asn1_size(p_stream, p_len_octets);
		if (data_bool) {
			p_stream.push_back(0x01);
		}
		else {
			p_stream.push_back(0x00);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_INTEGER: {
		p_stream.push_back(0x02);
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 4; i >= 0; i--) {
			uint8_t x = (data_int >> i * 8) & 0xFF;
			p_stream.push_back(x);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_REAL: {
		p_stream.push_back(0x03);
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 4; i >= 0; i--) {
			uint8_t x = (data_int >> i * 8) & 0xFF;
			p_stream.push_back(x);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_ARRAY: {
		p_stream.push_back(0x30); // Sequence.
		store_asn1_size(p_stream, p_len_octets);
		for (int i = 0; i < data_array.size(); i++) {
			valid = valid && data_array[i]->store_asn1(p_stream, p_len_octets);
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DICT: {
		p_stream.push_back(0x31); // Set.
		store_asn1_size(p_stream, p_len_octets);
		for (const KeyValue<String, Ref<PListNode>>& E : data_dict) {
			CharString cs = E.key.utf8();
			uint32_t size = cs.length();

			// Sequence.
			p_stream.push_back(0x30);
			uint32_t seq_size = 2 * (1 + _asn1_size_len(p_len_octets)) + size +
								E.value->get_asn1_size(p_len_octets);
			if (p_len_octets > 1) {
				p_stream.push_back(0x80 + p_len_octets);
			}
			for (int i = p_len_octets - 1; i >= 0; i--) {
				uint8_t x = (seq_size >> i * 8) & 0xFF;
				p_stream.push_back(x);
			}
			// Key.
			p_stream.push_back(0x0C);
			if (p_len_octets > 1) {
				p_stream.push_back(0x80 + p_len_octets);
			}
			for (int i = p_len_octets - 1; i >= 0; i--) {
				uint8_t x = (size >> i * 8) & 0xFF;
				p_stream.push_back(x);
			}
			for (uint32_t i = 0; i < size; i++) {
				p_stream.push_back(cs[i]);
			}
			// Value.
			valid = valid && E.value->store_asn1(p_stream, p_len_octets);
		}
	} break;
	}
	return valid;
}

void PListNode::store_text(String& p_stream, uint8_t p_indent) const
{
	// Convert to text XML stream.
	switch (data_type) {
	case PList::PLNodeType::PL_NODE_TYPE_NIL: {
		// Nothing to store.
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATA: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<data>\n";
		p_stream += String("\t").repeat(p_indent);
		// Data should be Base64 (i.e. ASCII only).
		p_stream += String::ascii(data_string) + "\n";
		p_stream += String("\t").repeat(p_indent);
		p_stream += "</data>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DATE: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<date>";
		// Data should be ISO 8601 (i.e. ASCII only).
		p_stream += String::ascii(data_string);
		p_stream += "</date>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_STRING: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<string>";
		p_stream += String::utf8(data_string);
		p_stream += "</string>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_BOOLEAN: {
		p_stream += String("\t").repeat(p_indent);
		if (data_bool) {
			p_stream += "<true/>\n";
		}
		else {
			p_stream += "<false/>\n";
		}
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_INTEGER: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<integer>";
		p_stream += itos(data_int);
		p_stream += "</integer>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_REAL: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<real>";
		p_stream += rtos(data_real);
		p_stream += "</real>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_ARRAY: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<array>\n";
		for (int i = 0; i < data_array.size(); i++) {
			data_array[i]->store_text(p_stream, p_indent + 1);
		}
		p_stream += String("\t").repeat(p_indent);
		p_stream += "</array>\n";
	} break;
	case PList::PLNodeType::PL_NODE_TYPE_DICT: {
		p_stream += String("\t").repeat(p_indent);
		p_stream += "<dict>\n";
		for (const KeyValue<String, Ref<PListNode>>& E : data_dict) {
			p_stream += String("\t").repeat(p_indent + 1);
			p_stream += "<key>";
			p_stream += E.key;
			p_stream += "</key>\n";
			E.value->store_text(p_stream, p_indent + 1);
		}
		p_stream += String("\t").repeat(p_indent);
		p_stream += "</dict>\n";
	} break;
	}
}

/*************************************************************************/

PList::PList() { root = PListNode::new_dict(); }

PList::PList(const String& p_string)
{
	String err_str;
	bool ok = load_string(p_string, err_str);
	ERR_FAIL_COND_MSG(!ok, vformat("PList: %s.", err_str));
}

uint64_t PList::read_bplist_var_size_int(Ref<FileAccess> p_file, uint8_t p_size)
{
	uint64_t pos = p_file->get_position();
	uint64_t ret = 0;
	switch (p_size) {
	case 1: {
		ret = p_file->get_8();
	} break;
	case 2: {
		ret = BSWAP16(p_file->get_16());
	} break;
	case 3: {
		ret = BSWAP32(p_file->get_32() & 0x00FFFFFF);
	} break;
	case 4: {
		ret = BSWAP32(p_file->get_32());
	} break;
	case 5: {
		ret = BSWAP64(p_file->get_64() & 0x000000FFFFFFFFFF);
	} break;
	case 6: {
		ret = BSWAP64(p_file->get_64() & 0x0000FFFFFFFFFFFF);
	} break;
	case 7: {
		ret = BSWAP64(p_file->get_64() & 0x00FFFFFFFFFFFFFF);
	} break;
	case 8: {
		ret = BSWAP64(p_file->get_64());
	} break;
	default: {
		ret = 0;
	}
	}
	p_file->seek(pos + p_size);

	return ret;
}

Ref<PListNode> PList::read_bplist_obj(Ref<FileAccess> p_file, uint64_t p_offset_idx)
{
	Ref<PListNode> node;
	node.instantiate();

	uint64_t ot_off = trailer.offset_table_start + p_offset_idx * trailer.offset_size;
	p_file->seek(ot_off);
	uint64_t marker_off = read_bplist_var_size_int(p_file, trailer.offset_size);
	ERR_FAIL_COND_V_MSG(marker_off == 0, Ref<PListNode>(), "Invalid marker size.");

	p_file->seek(marker_off);
	uint8_t marker = p_file->get_8();
	uint8_t marker_type = marker & 0xF0;
	uint64_t marker_size = marker & 0x0F;

	switch (marker_type) {
	case 0x00: {
		if (marker_size == 0x00) {
			node->data_type = PL_NODE_TYPE_NIL;
		}
		else if (marker_size == 0x08) {
			node->data_type = PL_NODE_TYPE_BOOLEAN;
			node->data_bool = false;
		}
		else if (marker_size == 0x09) {
			node->data_type = PL_NODE_TYPE_BOOLEAN;
			node->data_bool = true;
		}
		else {
			ERR_FAIL_V_MSG(Ref<PListNode>(), "Invalid nil/bool marker value.");
		}
	} break;
	case 0x10: {
		node->data_type = PL_NODE_TYPE_INTEGER;
		node->data_int =
			static_cast<int64_t>(read_bplist_var_size_int(p_file, std::pow(2, marker_size)));
	} break;
	case 0x20: {
		node->data_type = PL_NODE_TYPE_REAL;
		node->data_int =
			static_cast<int64_t>(read_bplist_var_size_int(p_file, std::pow(2, marker_size)));
	} break;
	case 0x30: {
		node->data_type = PL_NODE_TYPE_DATE;
		node->data_int = BSWAP64(p_file->get_64());
		node->data_string =
			Time::get_datetime_string_from_unix_time(node->data_real + 978307200.0).utf8();
	} break;
	case 0x40: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		node->data_type = PL_NODE_TYPE_DATA;
		Vector<uint8_t> buf;
		buf.resize(marker_size + 1);
		p_file->get_buffer(reinterpret_cast<uint8_t*>(buf.ptrw()), marker_size);
		node->data_string = CryptoCore::b64_encode_str(buf.ptr(), buf.size()).utf8();
	} break;
	case 0x50: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		node->data_type = PL_NODE_TYPE_STRING;
		node->data_string.resize_uninitialized(marker_size + 1);
		p_file->get_buffer(reinterpret_cast<uint8_t*>(node->data_string.ptrw()), marker_size);
	} break;
	case 0x60: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		Char16String cs16;
		cs16.resize_uninitialized(marker_size + 1);
		for (uint64_t i = 0; i < marker_size; i++) {
			cs16[i] = BSWAP16(p_file->get_16());
		}
		node->data_type = PL_NODE_TYPE_STRING;
		node->data_string = String::utf16(cs16.ptr(), cs16.length()).utf8();
	} break;
	case 0x80: {
		node->data_type = PL_NODE_TYPE_INTEGER;
		node->data_int = static_cast<int64_t>(read_bplist_var_size_int(p_file, marker_size + 1));
	} break;
	case 0xA0:
	case 0xC0: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		uint64_t pos = p_file->get_position();

		node->data_type = PL_NODE_TYPE_ARRAY;
		for (uint64_t i = 0; i < marker_size; i++) {
			p_file->seek(pos + trailer.ref_size * i);
			uint64_t ref = read_bplist_var_size_int(p_file, trailer.ref_size);

			Ref<PListNode> element = read_bplist_obj(p_file, ref);
			ERR_FAIL_COND_V(element.is_null(), Ref<PListNode>());
			node->data_array.push_back(element);
		}
	} break;
	case 0xD0: {
		if (marker_size == 0x0F) {
			uint8_t ext = p_file->get_8() & 0xF;
			marker_size = read_bplist_var_size_int(p_file, std::pow(2, ext));
		}
		uint64_t pos = p_file->get_position();

		node->data_type = PL_NODE_TYPE_DICT;
		for (uint64_t i = 0; i < marker_size; i++) {
			p_file->seek(pos + trailer.ref_size * i);
			uint64_t key_ref = read_bplist_var_size_int(p_file, trailer.ref_size);

			p_file->seek(pos + trailer.ref_size * (i + marker_size));
			uint64_t obj_ref = read_bplist_var_size_int(p_file, trailer.ref_size);

			Ref<PListNode> element_key = read_bplist_obj(p_file, key_ref);
			ERR_FAIL_COND_V(element_key.is_null() || element_key->data_type != PL_NODE_TYPE_STRING,
				Ref<PListNode>());
			Ref<PListNode> element = read_bplist_obj(p_file, obj_ref);
			ERR_FAIL_COND_V(element.is_null(), Ref<PListNode>());
			node->data_dict[String::utf8(
				element_key->data_string.ptr(), element_key->data_string.length())] = element;
		}
	} break;
	default: {
		ERR_FAIL_V_MSG(Ref<PListNode>(), "Invalid marker type.");
	}
	}
	return node;
}

bool PList::load_file(const String& p_filename)
{
	root = Ref<PListNode>();

	Ref<FileAccess> fb = FileAccess::open(p_filename, FileAccess::READ);
	if (fb.is_null()) {
		return false;
	}

	unsigned char magic[8];
	fb->get_buffer(magic, 8);

	if (String::ascii(Span((const char*)magic, 8)) == "bplist00") {
		fb->seek_end(-26);
		trailer.offset_size = fb->get_8();
		trailer.ref_size = fb->get_8();
		trailer.object_num = BSWAP64(fb->get_64());
		trailer.root_offset_idx = BSWAP64(fb->get_64());
		trailer.offset_table_start = BSWAP64(fb->get_64());
		root = read_bplist_obj(fb, trailer.root_offset_idx);

		return root.is_valid();
	}
	else {
		// Load text plist.
		Error err;
		Vector<uint8_t> array = FileAccess::get_file_as_bytes(p_filename, &err);
		ERR_FAIL_COND_V(err != OK, false);

		String err_str;
		bool ok = load_string(String::utf8((const char*)array.ptr(), array.size()), err_str);
		ERR_FAIL_COND_V_MSG(!ok, false, "PList: " + err_str);

		return true;
	}
}

bool PList::load_string(const String& p_string, String& r_err_out)
{
	root = Ref<PListNode>();

	int pos = 0;
	bool in_plist = false;
	bool done_plist = false;
	List<Ref<PListNode>> stack;
	String key;
	while (pos >= 0) {
		int open_token_s = p_string.find_char('<', pos);
		if (open_token_s == -1) {
			r_err_out = "Unexpected end of data. No tags found.";
			return false;
		}
		int open_token_e = p_string.find_char('>', open_token_s);
		pos = open_token_e;

		String token = p_string.substr(open_token_s + 1, open_token_e - open_token_s - 1);
		if (token.is_empty()) {
			r_err_out = "Invalid token name.";
			return false;
		}
		String value;
		if (token[0] == '?' || token[0] == '!') { // Skip <?xml ... ?> and <!DOCTYPE ... >
			int end_token_e = p_string.find_char('>', open_token_s);
			pos = end_token_e;
			continue;
		}

		if (token.find("plist", 0) == 0) {
			in_plist = true;
			continue;
		}

		if (token == "/plist") {
			done_plist = true;
			break;
		}

		if (!in_plist) {
			r_err_out = "Node outside of <plist> tag.";
			return false;
		}

		if (token == "dict") {
			if (!stack.is_empty()) {
				// Add subnode end enter it.
				Ref<PListNode> dict = PListNode::new_dict();
				dict->data_type = PList::PLNodeType::PL_NODE_TYPE_DICT;
				if (!stack.back()->get()->push_subnode(dict, key)) {
					r_err_out = "Can't push subnode, invalid parent type.";
					return false;
				}
				stack.push_back(dict);
			}
			else {
				// Add root node.
				if (root.is_valid()) {
					r_err_out = "Root node already set.";
					return false;
				}
				Ref<PListNode> dict = PListNode::new_dict();
				stack.push_back(dict);
				root = dict;
			}
			continue;
		}

		if (token == "/dict") {
			// Exit current dict.
			if (stack.is_empty() ||
				stack.back()->get()->data_type != PList::PLNodeType::PL_NODE_TYPE_DICT) {
				r_err_out = "Mismatched </dict> tag.";
				return false;
			}
			stack.pop_back();
			continue;
		}

		if (token == "array") {
			if (!stack.is_empty()) {
				// Add subnode end enter it.
				Ref<PListNode> arr = PListNode::new_array();
				if (!stack.back()->get()->push_subnode(arr, key)) {
					r_err_out
= "Can't push subnode, invalid parent type.";
					return false;
				}
				stack.push_back(arr);
			}
			else {
				// Add root node.
				if (root.is_valid()) {
					r_err_out = "Root node already set.";
					return false;
				}
				Ref<PListNode> arr = PListNode::new_array();
				stack.push_back(arr);
				root = arr;
			}
			continue;
		}

		if (token == "/array") {
			// Exit current array.
			if (stack.is_empty() ||
				stack.back()->get()->data_type != PList::PLNodeType::PL_NODE_TYPE_ARRAY) {
				r_err_out = "Mismatched </array> tag.";
				return false;
			}
			stack.pop_back();
			continue;
		}

		if (token[token.length() - 1] == '/') {
			token = token.substr(0, token.length() - 1);
		}
		else {
			int end_token_s = p_string.find("</", pos);
			if (end_token_s == -1) {
				r_err_out = vformat("Mismatched <%s> tag.", token);
				return false;
			}
			int end_token_e = p_string.find_char('>', end_token_s);
			pos = end_token_e;
			String end_token = p_string.substr(end_token_s + 2, end_token_e - end_token_s - 2);
			if (end_token != token) {
				r_err_out = vformat("Mismatched <%s> and <%s> token pair.", token, end_token);
				return false;
			}
			value = p_string.substr(open_token_e + 1, end_token_s - open_token_e - 1);
		}
		if (token == "key") {
			key = value;
		}
		else {
			Ref<PListNode> var = nullptr;
			if (token == "true") {
				var = PListNode::new_bool(true);
			}
			else if (token == "false") {
				var = PListNode::new_bool(false);
			}
			else if (token == "integer") {
				var = PListNode::new_int(value.to_int());
			}
			else if (token == "real") {
				var = PListNode::new_real(value.to_float());
			}
			else if (token == "string") {
				var = PListNode::new_string(value);
			}
			else if (token == "data") {
				var = PListNode::new_data(value);
			}
			else if (token == "date") {
				var = PListNode::new_date(value);
			}
			else {
				r_err_out = vformat("Invalid value type: %s.", token);
				return false;
			}
			if (stack.is_empty() || !stack.back()->get()->push_subnode(var, key)) {
				r_err_out = "Can't push subnode, invalid parent type.";
				return false;
			}
		}
	}
	if (!stack.is_empty() || !done_plist) {
		r_err_out = "Unexpected end of data. Root node is not closed.";
		return false;
	}
	return true;
}

Vector<uint8_t> PList::save_asn1() const
{
	if (root.is_null()) {
		ERR_FAIL_V_MSG(Vector<uint8_t>(), "PList: Invalid PList, no root node.");
	}
	size_t size = root->get_asn1_size(1);
	uint8_t len_octets = 0;
	if (size < 0x80) {
		len_octets = 1;
	}
	else {
		size = root->get_asn1_size(2);
		if (size < 0xFFFF) {
			len_octets = 2;
		}
		else {
			size = root->get_asn1_size(3);
			if (size < 0xFFFFFF) {
				len_octets = 3;
			}
			else {
				size = root->get_asn1_size(4);
				if (size < 0xFFFFFFFF) {
					len_octets = 4;
				}
				else {
					ERR_FAIL_V_MSG(Vector<uint8_t>(),
						"PList: Data is too big for ASN.1 serializer, should be < 4 GiB.");
				}
			}
		}
	}

	Vector<uint8_t> ret;
	if (!root->store_asn1(ret, len_octets)) {
		ERR_FAIL_V_MSG(Vector<uint8_t>(), "PList: ASN.1 serializer error.");
	}
	return ret;
}

String PList::save_text() const
{
	if (root.is_null()) {
		ERR_FAIL_V_MSG(String(), "PList: Invalid PList, no root node.");
	}

	String ret;
	ret += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
	ret += "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
		   "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
	ret += "<plist version=\"1.0\">\n";

	root->store_text(ret, 0);

	ret += "</plist>\n\n";
	return ret;
}

Ref<PListNode> PList::get_root() { return root; }

// ==========================================================================
// xml_parser.cpp
// ==========================================================================

/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "core/io/file_access.h"
#include "xml_parser.h"

static inline bool _is_white_space(char p_char)
{
	return (p_char == ' ' || p_char == '\t' || p_char == '\n' || p_char == '\r');
}

//! sets the state that text was found. Returns true if set should be set
bool XMLParser::_set_text(const char* p_start, const char* p_end)
{
	// check if text is more than 2 characters, and if not, check if there is
	// only white space, so that this text won't be reported
	if (p_end - p_start < 3) {
		const char* p = p_start;
		for (; p != p_end; ++p) {
			if (!_is_white_space(*p)) {
				break;
			}
		}

		if (p == p_end) {
			return false;
		}
	}

	// set current text to the parsed text, and replace xml special characters
	String s = String::utf8(p_start, (int)(p_end - p_start));
	node_name = s.xml_unescape();

	// current XML node type is text
	node_type = NODE_TEXT;

	return true;
}

void XMLParser::_parse_closing_xml_element()
{
	node_type = NODE_ELEMENT_END;
	node_empty = false;
	attributes.clear();

	next_char();
	const char* pBeginClose = P;

	while (*P && *P != '>') {
		next_char();
	}

	node_name = String::utf8(pBeginClose, (int)(P - pBeginClose));
#ifdef DEBUG_XML
	print_line("XML CLOSE: " + node_name);
#endif

	if (*P) {
		next_char();
	}
}

void XMLParser::_ignore_definition()
{
	node_type = NODE_UNKNOWN;

	const char* F = P;
	// move until end marked with '>' reached
	while (*P && *P != '>') {
		next_char();
	}
	node_name.clear();
	node_name.append_utf8(F, P - F);

	if (*P) {
		next_char();
	}
}

bool XMLParser::_parse_cdata()
{
	if (*(P + 1) != '[') {
		return false;
	}

	node_type = NODE_CDATA;

	// skip '<![CDATA['
	int count = 0;
	while (*P && count < 8) {
		next_char();
		++count;
	}

	if (!*P) {
		node_name = "";
		return true;
	}

	const char* cDataBegin = P;
	const char* cDataEnd = nullptr;

	// find end of CDATA
	while (*P && !cDataEnd) {
		if (*P == '>' && (*(P - 1) == ']') && (*(P - 2) == ']')) {
			cDataEnd = P - 2;
		}

		next_char();
	}

	if (!cDataEnd) {
		cDataEnd = P;
	}
	node_name = String::utf8(cDataBegin, (int)(cDataEnd - cDataBegin));
#ifdef DEBUG_XML
	print_line("XML CDATA: " + node_name);
#endif

	return true;
}

void XMLParser::_parse_comment()
{
	node_type = NODE_COMMENT;
	P += 1;

	const char* pEndOfInput = data + length;
	const char* pCommentBegin;
	const char* pCommentEnd;

	if (P + 1 < pEndOfInput && P[0] == '-' && P[1] == '-') {
		// Comment, use '-->' as end.
		pCommentBegin = P + 2;
		for (pCommentEnd = pCommentBegin; pCommentEnd + 2 < pEndOfInput; pCommentEnd++) {
			if (pCommentEnd[0] == '-' && pCommentEnd[1] == '-' && pCommentEnd[2] == '>') {
				break;
			}
		}
		if (pCommentEnd + 2 < pEndOfInput) {
			P = pCommentEnd + 3;
		}
		else {
			P = pCommentEnd = pEndOfInput;
		}
	}
	else {
		// Like document type definition, match angle brackets.
		pCommentBegin = P;

		int count = 1;
		while (*P && count) {
			if (*P == '>') {
				--count;
			}
			else if (*P == '<') {
				++count;
			}
			next_char();
		}

		if (count) {
			pCommentEnd = P;
		}
		else {
			pCommentEnd = P - 1;
		}
	}

	node_name = String::utf8(pCommentBegin, (int)(pCommentEnd - pCommentBegin));
#ifdef DEBUG_XML
	print_line("XML COMMENT: " + node_name);
#endif
}

void XMLParser::_parse_opening_xml_element()
{
	node_type = NODE_ELEMENT;
	node_empty = false;
	attributes.clear();

	// find name
	const char* startName = P;

	// find end of element
	while (*P && *P != '>' && !_is_white_space(*P)) {
		next_char();
	}

	const char* endName = P;

	// find attributes
	while (*P && *P != '>') {
		if (_is_white_space(*P)) {
			next_char();
		}
		else {
			if (*P != '/') {
				// we've got an attribute

				// read the attribute names
				const char* attributeNameBegin = P;

				while (*P && !_is_white_space(*P) && *P != '=') {
					next_char();
				}

				if (!*P) {
					break;
				}

				const char* attributeNameEnd = P;
				next_char();

				// read the attribute value
				// check for quotes and single quotes, thx to murphy
				while ((*P != '\"') && (*P != '\'') && *P) {
					next_char();
				}

				if (!*P) { // malformatted xml file
					break;
				}

				const char attributeQuoteChar = *P;

				next_char();
				const char* attributeValueBegin = P;

				while (*P != attributeQuoteChar && *P) {
					next_char();
				}

				const char* attributeValueEnd = P;
				if (*P) {
					next_char();
				}

				Attribute attr;
				attr.name =
					String::utf8(attributeNameBegin, (int)(attributeNameEnd - attributeNameBegin));

				String s = String::utf8(
					attributeValueBegin, (int)(attributeValueEnd - attributeValueBegin));

				attr.value = s.xml_unescape();
				attributes.push_back(attr);
			}
			else {
				// tag is closed directly
				next_char();
				node_empty = true;
				break;
			}
		}
	}

	// check if this tag is closing directly
	if (endName > startName && *(endName - 1) == '/') {
		// directly closing tag
		node_empty = true;
		endName--;
	}

	node_name = String::utf8(startName, (int)(endName - startName));
#ifdef DEBUG_XML
	print_line("XML OPEN: " + node_name);
#endif

	if (*P) {
		next_char();
	}
}

void XMLParser::_parse_current_node()
{
	const char* start = P;
	node_offset = P - data;

	// more forward until '<' found
	while (*P != '<' && *P) {
		next_char();
	}

	if (P - start > 0) {
		// we found some text, store it
		if (_set_text(start, P)) {
			return;
		}
	}

	if (!*P) {
		return;
	}

	next_char();

	// based on current token, parse and report next element
	switch (*P) {
	case '/':
		_parse_closing_xml_element();
		break;
	case '?':
		_ignore_definition();
		break;
	case '!':
		if (!_parse_cdata()) {
			_parse_comment();
		}
		break;
	default:
		_parse_opening_xml_element();
		break;
	}
}

uint64_t XMLParser::get_node_offset() const { return node_offset; }

Error XMLParser::seek(uint64_t p_pos)
{
	ERR_FAIL_NULL_V(data, ERR_FILE_EOF);
	ERR_FAIL_COND_V(p_pos >= length, ERR_FILE_EOF);

	P = data + p_pos;

	return read();
}

Error XMLParser::read()
{
	// if end not reached, parse the node
	if (P && (P - data) < (int64_t)length - 1 && *P != 0) {
		_parse_current_node();
		return OK;
	}

	return ERR_FILE_EOF;
}

XMLParser::NodeType XMLParser::get_node_type() { return node_type; }

String XMLParser::get_node_data() const
{
	ERR_FAIL_COND_V(node_type != NODE_TEXT, "");
	return node_name;
}

String XMLParser::get_node_name() const
{
	ERR_FAIL_COND_V(node_type == NODE_TEXT, "");
	return node_name;
}

int XMLParser::get_attribute_count() const { return attributes.size(); }

String XMLParser::get_attribute_name(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, attributes.size(), "");
	return attributes[p_idx].name;
}

String XMLParser::get_attribute_value(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, attributes.size(), "");
	return attributes[p_idx].value;
}

bool XMLParser::has_attribute(const String& p_name) const
{
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == p_name) {
			return true;
		}
	}

	return false;
}

String XMLParser::get_named_attribute_value(const String& p_name) const
{
	int idx = -1;
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == p_name) {
			idx = i;
			break;
		}
	}

	ERR_FAIL_COND_V_MSG(idx < 0, "", vformat("Attribute not found: '%s'.", p_name));

	return attributes[idx].value;
}

String XMLParser::get_named_attribute_value_safe(const String& p_name) const
{
	int idx = -1;
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].name == p_name) {
			idx = i;
			break;
		}
	}

	if (idx < 0) {
		return "";
	}
	return attributes[idx].value;
}

bool XMLParser::is_empty() const { return node_empty; }

Error XMLParser::open_buffer(const Vector<uint8_t>& p_buffer)
{
	ERR_FAIL_COND_V(p_buffer.is_empty(), ERR_INVALID_DATA);

	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}

	length = p_buffer.size();
	data_copy = memnew_arr(char, length + 1);
	memcpy(data_copy, p_buffer.ptr(), length);
	data_copy[length] = 0;
	data = data_copy;
	P = data;
	current_line = 0;

	return OK;
}

Error XMLParser::_open_buffer(const uint8_t* p_buffer, size_t p_size)
{
	ERR_FAIL_COND_V(p_size == 0, ERR_INVALID_DATA);
	ERR_FAIL_NULL_V(p_buffer, ERR_INVALID_DATA);

	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}

	length = p_size;
	data = (const char*)p_buffer;
	P = data;
	current_line = 0;

	return OK;
}

Error XMLParser::open(const String& p_path)
{
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);

	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot open file '%s'.", p_path));

	length = file->get_length();
	ERR_FAIL_COND_V(length < 1, ERR_FILE_CORRUPT);

	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}

	data_copy = memnew_arr(char, length + 1);
	file->get_buffer((uint8_t*)data_copy, length);
	data_copy[length] = 0;
	data = data_copy;
	P = data;
	current_line = 0;

	return OK;
}

void XMLParser::skip_section()
{
	// skip if this element is empty anyway.
	if (is_empty()) {
		return;
	}

	// read until we've reached the last element in this section
	int tagcount = 1;

	while (tagcount && read() == OK) {
		if (get_node_type() == XMLParser::NODE_ELEMENT && !is_empty()) {
			++tagcount;
		}
		else if (get_node_type() == XMLParser::NODE_ELEMENT_END) {
			--tagcount;
		}
	}
}

void XMLParser::close()
{
	if (data_copy) {
		memdelete_arr(data);
		data_copy = nullptr;
	}
	data = nullptr;
	length = 0;
	P = nullptr;
	node_empty = false;
	node_type = NODE_NONE;
	node_offset = 0;
}

int XMLParser::get_current_line() const { return current_line; }

XMLParser::~XMLParser()
{
	if (data_copy) {
		memdelete_arr(data_copy);
		data_copy = nullptr;
	}
}

// ==========================================================================
// translation_loader_po.cpp
// ==========================================================================

/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "translation_loader_po.h"

#include "core/io/file_access.h"
#include "core/string/translation.h"

Ref<Resource> TranslationLoaderPO::load_translation(Ref<FileAccess> r_file, Error *r_error) {
	if (r_error) {
		*r_error = ERR_FILE_CORRUPT;
	}

	const String path = r_file->get_path();
	Ref<Translation> translation;
	translation.instantiate();
	String config;

	uint32_t magic = r_file->get_32();
	if (magic == 0x950412de) {
		// Load binary MO file.

		uint16_t version_maj = r_file->get_16();
		uint16_t version_min = r_file->get_16();
		ERR_FAIL_COND_V_MSG(version_maj > 1, Ref<Resource>(), vformat("Unsupported MO file %s, version %d.%d.", path, version_maj, version_min));

		uint32_t num_strings = r_file->get_32();
		uint32_t id_table_offset = r_file->get_32();
		uint32_t trans_table_offset = r_file->get_32();

		// Read string tables.
		for (uint32_t i = 0; i < num_strings; i++) {
			String msg_id;
			String msg_id_plural;
			String msg_context;

			// Read id strings and context.
			{
				Vector<uint8_t> data;
				r_file->seek(id_table_offset + i * 8);
				uint32_t str_start = 0;
				uint32_t str_len = r_file->get_32();
				uint32_t str_offset = r_file->get_32();

				data.resize(str_len + 1);
				r_file->seek(str_offset);
				r_file->get_buffer(data.ptrw(), str_len);
				data.write[str_len] = 0;

				bool is_plural = false;
				for (uint32_t j = 0; j < str_len + 1; j++) {
					if (data[j] == 0x04) {
						msg_context.clear();
						msg_context.append_utf8((const char *)data.ptr(), j);
						str_start = j + 1;
					}
					if (data[j] == 0x00) {
						if (is_plural) {
							msg_id_plural.clear();
							msg_id_plural.append_utf8((const char *)(data.ptr() + str_start), j - str_start);
						} else {
							msg_id.clear();
							msg_id.append_utf8((const char *)(data.ptr() + str_start), j - str_start);
							is_plural = true;
						}
						str_start = j + 1;
					}
				}
			}

			// Read translated strings.
			{
				Vector<uint8_t> data;
				r_file->seek(trans_table_offset + i * 8);
				uint32_t str_len = r_file->get_32();
				uint32_t str_offset = r_file->get_32();

				data.resize(str_len + 1);
				r_file->seek(str_offset);
				r_file->get_buffer(data.ptrw(), str_len);
				data.write[str_len] = 0;

				if (msg_id.is_empty()) {
					config = String::utf8((const char *)data.ptr(), str_len);
					// Record plural rule.
					int p_start = config.find("Plural-Forms");
					if (p_start != -1) {
						int p_end = config.find_char('\n', p_start);
						translation->set_plural_rules_override(config.substr(p_start, p_end - p_start));
					}
				} else {
					uint32_t str_start = 0;
					Vector<String> plural_msg;
					for (uint32_t j = 0; j < str_len + 1; j++) {
						if (data[j] == 0x00) {
							if (msg_id_plural.is_empty()) {
								translation->add_message(msg_id, String::utf8((const char *)(data.ptr() + str_start), j - str_start), msg_context);
							} else {
								plural_msg.push_back(String::utf8((const char *)(data.ptr() + str_start), j - str_start));
							}
							str_start = j + 1;
						}
					}
					if (!plural_msg.is_empty()) {
						translation->add_plural_message(msg_id, plural_msg, msg_context);
					}
				}
			}
		}

	} else {
		// Try to load as text PO file.
		r_file->seek(0);

		enum Status {
			STATUS_NONE,
			STATUS_READING_ID,
			STATUS_READING_STRING,
			STATUS_READING_CONTEXT,
			STATUS_READING_PLURAL,
		};

		Status status = STATUS_NONE;

		String msg_id;
		String msg_str;
		String msg_context;
		Vector<String> msgs_plural;

		if (r_error) {
			*r_error = ERR_FILE_CORRUPT;
		}

		int line = 1;
		int plural_forms = 0;
		int plural_index = -1;
		bool entered_context = false;
		bool skip_this = false;
		bool skip_next = false;
		bool is_eof = false;

		while (!is_eof) {
			String l = r_file->get_line().strip_edges();
			is_eof = r_file->eof_reached();

			// If we reached last line and it's not a content line, break, otherwise let processing that last loop
			if (is_eof && l.is_empty()) {
				if (status == STATUS_READING_ID || status == STATUS_READING_CONTEXT || (status == STATUS_READING_PLURAL && plural_index != plural_forms - 1)) {
					ERR_FAIL_V_MSG(Ref<Resource>(), vformat("Unexpected EOF while reading PO file at: %s:%d.", path, line));
				} else {
					break;
				}
			}

			if (l.begins_with("msgctxt")) {
				ERR_FAIL_COND_V_MSG(status != STATUS_READING_STRING && status != STATUS_READING_PLURAL, Ref<Resource>(), vformat("Unexpected 'msgctxt', was expecting 'msgid_plural' or 'msgstr' before 'msgctxt' while parsing: %s:%d.", path, line));

				// In PO file, "msgctxt" appears before "msgid". If we encounter a "msgctxt", we add what we have read
				// and set "entered_context" to true to prevent adding twice.
				if (!skip_this && !msg_id.is_empty()) {
					if (status == STATUS_READING_STRING) {
						translation->add_message(msg_id, msg_str, msg_context);
					} else if (status == STATUS_READING_PLURAL) {
						ERR_FAIL_COND_V_MSG(plural_index != plural_forms - 1, Ref<Resource>(), vformat("Number of 'msgstr[]' doesn't match with number of plural forms: %s:%d.", path, line));
						translation->add_plural_message(msg_id, msgs_plural, msg_context);
					}
				}
				msg_context = "";
				l = l.substr(7).strip_edges();
				status = STATUS_READING_CONTEXT;
				entered_context = true;
			}

			if (l.begins_with("msgid_plural")) {
				if (plural_forms == 0) {
					ERR_FAIL_V_MSG(Ref<Resource>(), vformat("PO file uses 'msgid_plural' but 'Plural-Forms' is invalid or missing in header: %s:%d.", path, line));
				} else if (status != STATUS_READING_ID) {
					ERR_FAIL_V_MSG(Ref<Resource>(), vformat("Unexpected 'msgid_plural', was expecting 'msgid' before 'msgid_plural' while parsing: %s:%d.", path, line));
				}
				// We don't record the message in "msgid_plural" itself as tr_n(), TTRN(), RTRN() interfaces provide the plural string already.
				// We just have to reset variables related to plurals for "msgstr[]" later on.
				l = l.substr(12).strip_edges();
				plural_index = -1;
				msgs_plural.clear();
				msgs_plural.resize(plural_forms);
				status = STATUS_READING_PLURAL;
			} else if (l.begins_with("msgid")) {
				ERR_FAIL_COND_V_MSG(status == STATUS_READING_ID, Ref<Resource>(), vformat("Unexpected 'msgid', was expecting 'msgstr' while parsing: %s:%d.", path, line));

				if (!msg_id.is_empty()) {
					if (!skip_this && !entered_context) {
						if (status == STATUS_READING_STRING) {
							translation->add_message(msg_id, msg_str, msg_context);
						} else if (status == STATUS_READING_PLURAL) {
							ERR_FAIL_COND_V_MSG(plural_index != plural_forms - 1, Ref<Resource>(), vformat("Number of 'msgstr[]' doesn't match with number of plural forms: %s:%d.", path, line));
							translation->add_plural_message(msg_id, msgs_plural, msg_context);
						}
					}
				} else if (config.is_empty()) {
					config = msg_str;
					// Record plural rule.
					int p_start = config.find("Plural-Forms");
					if (p_start != -1) {
						int p_end = config.find_char('\n', p_start);
						translation->set_plural_rules_override(config.substr(p_start, p_end - p_start));
						plural_forms = translation->get_nplurals();
					}
				}

				l = l.substr(5).strip_edges();
				status = STATUS_READING_ID;
				// If we did not encounter msgctxt, we reset context to empty to reset it.
				if (!entered_context) {
					msg_context = "";
				}
				msg_id = "";
				msg_str = "";
				skip_this = skip_next;
				skip_next = false;
				entered_context = false;
			}

			if (l.begins_with("msgstr[")) {
				ERR_FAIL_COND_V_MSG(status != STATUS_READING_PLURAL, Ref<Resource>(), vformat("Unexpected 'msgstr[]', was expecting 'msgid_plural' before 'msgstr[]' while parsing: %s:%d.", path, line));
				plural_index++; // Increment to add to the next slot in vector msgs_plural.
				l = l.substr(9).strip_edges();
			} else if (l.begins_with("msgstr")) {
				ERR_FAIL_COND_V_MSG(status != STATUS_READING_ID, Ref<Resource>(), vformat("Unexpected 'msgstr', was expecting 'msgid' before 'msgstr' while parsing: %s:%d.", path, line));
				l = l.substr(6).strip_edges();
				status = STATUS_READING_STRING;
			}

			if (l.is_empty() || l.begins_with("#")) {
				if (l.contains("fuzzy")) {
					skip_next = true;
				}
				line++;
				continue; // Nothing to read or comment.
			}

			ERR_FAIL_COND_V_MSG(!l.begins_with("\"") || status == STATUS_NONE, Ref<Resource>(), vformat("Invalid line '%s' while parsing: %s:%d.", l, path, line));

			l = l.substr(1);
			// Find final quote, ignoring escaped ones (\").
			// The escape_next logic is necessary to properly parse things like \\"
			// where the backslash is the one being escaped, not the quote.
			int end_pos = -1;
			bool escape_next = false;
			for (int i = 0; i < l.length(); i++) {
				if (l[i] == '\\' && !escape_next) {
					escape_next = true;
					continue;
				}

				if (l[i] == '"' && !escape_next) {
					end_pos = i;
					break;
				}

				escape_next = false;
			}

			ERR_FAIL_COND_V_MSG(end_pos == -1, Ref<Resource>(), vformat("Expected '\"' at end of message while parsing: %s:%d.", path, line));

			l = l.substr(0, end_pos);
			l = l.c_unescape();

			if (status == STATUS_READING_ID) {
				msg_id += l;
			} else if (status == STATUS_READING_STRING) {
				msg_str += l;
			} else if (status == STATUS_READING_CONTEXT) {
				msg_context += l;
			} else if (status == STATUS_READING_PLURAL && plural_index >= 0) {
				ERR_FAIL_COND_V_MSG(plural_index >= plural_forms, Ref<Resource>(), vformat("Unexpected plural form while parsing: %s:%d.", path, line));
				msgs_plural.write[plural_index] = msgs_plural[plural_index] + l;
			}

			line++;
		}

		// Add the last set of data from last iteration.
		if (status == STATUS_READING_STRING) {
			if (!msg_id.is_empty()) {
				if (!skip_this) {
					translation->add_message(msg_id, msg_str, msg_context);
				}
			} else if (config.is_empty()) {
				config = msg_str;
			}
		} else if (status == STATUS_READING_PLURAL) {
			if (!skip_this && !msg_id.is_empty()) {
				ERR_FAIL_COND_V_MSG(plural_index != plural_forms - 1, Ref<Resource>(), vformat("Number of 'msgstr[]' doesn't match with number of plural forms: %s:%d.", path, line));
				translation->add_plural_message(msg_id, msgs_plural, msg_context);
			}
		}
	}

	ERR_FAIL_COND_V_MSG(config.is_empty(), Ref<Resource>(), vformat("No config found in file: '%s'.", path));

	Vector<String> configs = config.split("\n");
	for (int i = 0; i < configs.size(); i++) {
		String c = configs[i].strip_edges();
		int p = c.find_char(':');
		if (p == -1) {
			continue;
		}
		String prop = c.substr(0, p).strip_edges();
		String value = c.substr(p + 1).strip_edges();

		if (prop == "X-Language" || prop == "Language") {
			translation->set_locale(value);
		}
	}

	if (r_error) {
		*r_error = OK;
	}

	return translation;
}

Ref<Resource> TranslationLoaderPO::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	if (r_error) {
		*r_error = ERR_CANT_OPEN;
	}

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), Ref<Resource>(), vformat("Cannot open file '%s'.", p_path));

	return load_translation(f, r_error);
}

void TranslationLoaderPO::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("po");
	p_extensions->push_back("mo");
}

bool TranslationLoaderPO::handles_type(const String &p_type) const {
	return p_type == "Translation";
}

String TranslationLoaderPO::get_resource_type(const String &p_path) const {
	if (p_path.has_extension("po") || p_path.has_extension("mo")) {
		return "Translation";
	}
	return "";
}
