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

#include "core/io/ip_address.h"
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


