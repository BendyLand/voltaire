/**************************************************************************/
/*  ip.h                                                                  */
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

#pragma once

// --- Inlined/Folded from core/io/ip_address.h ---
#include "core/string/ustring.h"

struct [[nodiscard]] _WARN_UNUSED_ IPAddress {
private:
	union {
		uint8_t field8[16];
		uint16_t field16[8];
		uint32_t field32[4];
	};

	enum {
		IPV4_MAX_STRING_LENGTH = 16,
		IPV6_MAX_STRING_LENGTH = 40,
	};

	bool valid;
	bool wildcard;

protected:
	static bool _parse_ipv6(const String &p_string, IPAddress &r_ip);
	static bool _parse_ipv4(const String &p_string, int p_start, uint8_t *r_dest);

public:
	//operator Variant() const;
	bool operator==(const IPAddress &p_ip) const {
		if (p_ip.valid != valid) {
			return false;
		}
		if (!valid) {
			return false;
		}
		for (int i = 0; i < 4; i++) {
			if (field32[i] != p_ip.field32[i]) {
				return false;
			}
		}
		return true;
	}

	bool operator!=(const IPAddress &p_ip) const {
		if (p_ip.valid != valid) {
			return true;
		}
		if (!valid) {
			return true;
		}
		for (int i = 0; i < 4; i++) {
			if (field32[i] != p_ip.field32[i]) {
				return true;
			}
		}
		return false;
	}

	static bool is_valid_ip_address(const String &p_string);

	void clear();
	bool is_wildcard() const { return wildcard; }
	bool is_valid() const { return valid; }
	bool is_ipv4() const;
	const uint8_t *get_ipv4() const;
	void set_ipv4(const uint8_t *p_ip);

	const uint8_t *get_ipv6() const;
	void set_ipv6(const uint8_t *p_buf);

	explicit operator String() const;
	IPAddress(const String &p_string);
	IPAddress(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d, bool p_is_v6 = false);
	IPAddress() { clear(); }
};

// Zero-constructing IPAddress initializes field, valid, and wildcard to 0 (and thus empty).
template <>
struct is_zero_constructible<IPAddress> : std::true_type {};
#include "core/types.h"

struct _IP_ResolverPrivate;

class IP final
{
public:
	enum ResolverStatus
	{
		RESOLVER_STATUS_NONE,
		RESOLVER_STATUS_WAITING,
		RESOLVER_STATUS_DONE,
		RESOLVER_STATUS_ERROR,
	};

	enum Type
	{
		TYPE_NONE = 0,
		TYPE_IPV4 = 1,
		TYPE_IPV6 = 2,
		TYPE_ANY = 3,
	};

	enum
	{
		RESOLVER_MAX_QUERIES = 256,
		RESOLVER_INVALID_ID = -1
	};

	typedef int ResolverID;

	struct Interface_Info
	{
		String name;
		String name_friendly;
		String index;
		List<IPAddress> ip_addresses;
	};

	struct Data
	{
		_IP_ResolverPrivate* resolver = nullptr;
	};

	static inline Data* data = nullptr;

private:
	static void _default_resolve_hostname(
		List<IPAddress>& r_addresses, const String& p_hostname, Type p_type)
	{
		(void)r_addresses;
		(void)p_hostname;
		(void)p_type;
	}
	static void _default_get_local_interfaces(HashMap<String, Interface_Info>* r_interfaces)
	{
		(void)r_interfaces;
	}

	static Vector<String> _get_local_addresses();

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	inline static void (*_resolve_hostname)(List<IPAddress>& r_addresses, const String& p_hostname,
		Type p_type) = &_default_resolve_hostname;
	inline static void (*get_local_interfaces)(
		HashMap<String, Interface_Info>* r_interfaces) = &_default_get_local_interfaces;

	template <typename Impl> static void bind()
	{
		_resolve_hostname = &Impl::_resolve_hostname;
		get_local_interfaces = &Impl::get_local_interfaces;
	}

	static void reset_backend();

	static IPAddress resolve_hostname(const String& p_hostname, Type p_type = TYPE_ANY);
	static Vector<String> resolve_hostname_addresses(
		const String& p_hostname, Type p_type = TYPE_ANY);
	// async resolver hostname
	static ResolverID resolve_hostname_queue_item(const String& p_hostname, Type p_type = TYPE_ANY);
	static ResolverStatus get_resolve_item_status(ResolverID p_id);
	static IPAddress get_resolve_item_address(ResolverID p_id);
	static void get_local_addresses(List<IPAddress>* r_addresses);

	static void erase_resolve_item(ResolverID p_id);

	static void clear_cache(const String& p_hostname = "");

	IP() = delete;
	IP(const IP&) = delete;
	IP& operator=(const IP&) = delete;
	~IP() = delete;
};


