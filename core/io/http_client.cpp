/**************************************************************************/
/*  http_client.cpp                                                       */
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

#include "core/object/class_db.h"
#include "http_client.h"

const char* HTTPClient::_methods[METHOD_MAX] = {
	"GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE", "CONNECT", "PATCH"};

HTTPClient* HTTPClient::create(bool p_notify_postinitialize)
{
	if (_create) {
		return _create(p_notify_postinitialize);
	}
	return nullptr;
}

void HTTPClient::set_http_proxy(const String& p_host, int p_port)
{
	WARN_PRINT("HTTP proxy feature is not available");
}

void HTTPClient::set_https_proxy(const String& p_host, int p_port)
{
	WARN_PRINT("HTTPS proxy feature is not available");
}

Error HTTPClient::_request_raw(Method p_method, const String& p_url,
	const Vector<String>& p_headers, const Vector<uint8_t>& p_body)
{
	int size = p_body.size();
	return request(p_method, p_url, p_headers, size > 0 ? p_body.ptr() : nullptr, size);
}

Error HTTPClient::_request(
	Method p_method, const String& p_url, const Vector<String>& p_headers, const String& p_body)
{
	CharString body_utf8 = p_body.utf8();
	int size = body_utf8.length();
	return request(p_method, p_url, p_headers,
		size > 0 ? (const uint8_t*)body_utf8.get_data() : nullptr, size);
}

String HTTPClient::query_string_from_dict(const Dictionary& p_dict)
{
	String query = "";
	for (const KeyValue<Variant, Variant>& kv : p_dict) {
		String encoded_key = String(kv.key).uri_encode();
		const Variant& value = kv.value;
		switch (value.get_type()) {
		case Variant::ARRAY: {
			// Repeat the key with every values
			Array values = value;
			for (int j = 0; j < values.size(); ++j) {
				query += "&" + encoded_key + "=" + String(values[j]).uri_encode();
			}
			break;
		}
		case Variant::NIL: {
			// Add the key with no value
			query += "&" + encoded_key;
			break;
		}
		default: {
			// Add the key-value pair
			query += "&" + encoded_key + "=" + String(value).uri_encode();
		}
		}
	}
	return query.substr(1);
}

Error HTTPClient::verify_headers(const Vector<String>& p_headers)
{
	for (int i = 0; i < p_headers.size(); i++) {
		String sanitized = p_headers[i].strip_edges();
		ERR_FAIL_COND_V_MSG(sanitized.is_empty(), ERR_INVALID_PARAMETER,
			vformat("Invalid HTTP header at index %d: empty.", i));
		ERR_FAIL_COND_V_MSG(sanitized.find_char(':') < 1, ERR_INVALID_PARAMETER,
			vformat("Invalid HTTP header at index %d: String must contain header-value pair, "
					"delimited by ':', but was: '%s'.",
				i, p_headers[i]));
	}

	return OK;
}

Dictionary HTTPClient::_get_response_headers_as_dictionary()
{
	List<String> rh;
	get_response_headers(&rh);
	Dictionary ret;
	for (const String& s : rh) {
		int sp = s.find_char(':');
		if (sp == -1) {
			continue;
		}
		String key = s.substr(0, sp).strip_edges();
		String value = s.substr(sp + 1).strip_edges();
		ret[key] = value;
	}

	return ret;
}

PackedStringArray HTTPClient::_get_response_headers()
{
	List<String> rh;
	get_response_headers(&rh);
	PackedStringArray ret;
	ret.resize(rh.size());
	int idx = 0;
	for (const String& E : rh) {
		ret.set(idx++, E);
	}

	return ret;
}

void HTTPClient::_bind_methods() {}

