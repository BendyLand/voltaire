/**************************************************************************/
/*  ip.cpp                                                                */
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

#include "core/os/semaphore.h"
#include "core/os/thread.h"
#include "core/templates/hash_map.h"
#include "ip.h"

/************* RESOLVER ******************/

struct _IP_ResolverPrivate
{
	struct QueueItem
	{
		SafeNumeric<IP::ResolverStatus> status;

		List<IPAddress> response;

		String hostname;
		IP::Type type;

		void clear()
		{
			status.set(IP::RESOLVER_STATUS_NONE);
			response.clear();
			type = IP::TYPE_NONE;
			hostname = "";
		}

		QueueItem() { clear(); }
	};

	QueueItem queue[IP::RESOLVER_MAX_QUERIES];

	IP::ResolverID find_empty_id() const
	{
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() == IP::RESOLVER_STATUS_NONE) {
				return i;
			}
		}
		return IP::RESOLVER_INVALID_ID;
	}

	Mutex mutex;
	Semaphore sem;

	Thread thread;
	SafeFlag thread_abort;

	void resolve_queues()
	{
		for (int i = 0; i < IP::RESOLVER_MAX_QUERIES; i++) {
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}

			MutexLock lock(mutex);
			List<IPAddress> response;
			String hostname = queue[i].hostname;
			IP::Type type = queue[i].type;
			lock.temp_unlock();

			// We should not lock while resolving the hostname,
			// only when modifying the queue.
			IP::_resolve_hostname(response, hostname, type);

			lock.temp_relock();
			// Could have been completed by another function, or deleted.
			if (queue[i].status.get() != IP::RESOLVER_STATUS_WAITING) {
				continue;
			}
			// We might be overriding another result, but we don't care as long as the result is
			// valid.
			if (response.size()) {
				String key = get_cache_key(hostname, type);
				cache[key] = response;
			}
			queue[i].response = response;
			queue[i].status.set(
				response.is_empty() ? IP::RESOLVER_STATUS_ERROR : IP::RESOLVER_STATUS_DONE);
		}
	}

	static void _thread_function(void* p_self)
	{
		_IP_ResolverPrivate* ipr = static_cast<_IP_ResolverPrivate*>(p_self);

		while (!ipr->thread_abort.is_set()) {
			ipr->sem.wait();
			ipr->resolve_queues();
		}
	}

	HashMap<String, List<IPAddress>> cache;

	static String get_cache_key(const String& p_hostname, IP::Type p_type)
	{
		return itos(p_type) + p_hostname;
	}
};

IPAddress IP::resolve_hostname(const String& p_hostname, IP::Type p_type)
{
	const Vector<String> addresses = resolve_hostname_addresses(p_hostname, p_type);
	return addresses.size() ? (IPAddress)addresses[0] : IPAddress();
}

Vector<String> IP::resolve_hostname_addresses(const String& p_hostname, Type p_type)
{
	ERR_FAIL_NULL_V(data, Vector<String>());
	_IP_ResolverPrivate* resolver = data->resolver;

	List<IPAddress> res;
	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);

	{
		MutexLock lock(resolver->mutex);
		if (resolver->cache.has(key)) {
			res = resolver->cache[key];
		}
		else {
			// This should be run unlocked so the resolver thread can keep resolving
			// other requests.
			lock.temp_unlock();
			_resolve_hostname(res, p_hostname, p_type);
			lock.temp_relock();
			// We might be overriding another result, but we don't care as long as the result is
			// valid.
			if (res.size()) {
				resolver->cache[key] = res;
			}
		}
	}

	Vector<String> result;
	for (const IPAddress& E : res) {
		result.push_back(String(E));
	}
	return result;
}

IP::ResolverID IP::resolve_hostname_queue_item(const String& p_hostname, IP::Type p_type)
{
	ERR_FAIL_NULL_V(data, RESOLVER_INVALID_ID);
	_IP_ResolverPrivate* resolver = data->resolver;

	MutexLock lock(resolver->mutex);

	ResolverID id = resolver->find_empty_id();

	if (id == RESOLVER_INVALID_ID) {
		WARN_PRINT("Out of resolver queries");
		return id;
	}

	String key = _IP_ResolverPrivate::get_cache_key(p_hostname, p_type);
	resolver->queue[id].hostname = p_hostname;
	resolver->queue[id].type = p_type;
	if (resolver->cache.has(key)) {
		resolver->queue[id].response = resolver->cache[key];
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_DONE);
	}
	else {
		resolver->queue[id].response = List<IPAddress>();
		resolver->queue[id].status.set(IP::RESOLVER_STATUS_WAITING);
		if (resolver->thread.is_started()) {
			resolver->sem.post();
		}
		else {
			resolver->resolve_queues();
		}
	}

	return id;
}

IP::ResolverStatus IP::get_resolve_item_status(ResolverID p_id)
{
	ERR_FAIL_NULL_V(data, IP::RESOLVER_STATUS_NONE);
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IP::RESOLVER_STATUS_NONE,
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	IP::ResolverStatus res = data->resolver->queue[p_id].status.get();
	if (res == IP::RESOLVER_STATUS_NONE) {
		ERR_PRINT("Condition status == IP::RESOLVER_STATUS_NONE");
		return IP::RESOLVER_STATUS_NONE;
	}
	return res;
}

IPAddress IP::get_resolve_item_address(ResolverID p_id)
{
	ERR_FAIL_NULL_V(data, IPAddress());
	ERR_FAIL_INDEX_V_MSG(p_id, IP::RESOLVER_MAX_QUERIES, IPAddress(),
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	_IP_ResolverPrivate* resolver = data->resolver;
	MutexLock lock(resolver->mutex);

	if (resolver->queue[p_id].status.get() != IP::RESOLVER_STATUS_DONE) {
		ERR_PRINT(vformat("Resolve of '%s' didn't complete yet.", resolver->queue[p_id].hostname));
		return IPAddress();
	}

	List<IPAddress> res(resolver->queue[p_id].response);

	for (const IPAddress& E : res) {
		if (E.is_valid()) {
			return E;
		}
	}
	return IPAddress();
}

void IP::erase_resolve_item(ResolverID p_id)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_INDEX_MSG(p_id, IP::RESOLVER_MAX_QUERIES,
		vformat("Too many concurrent DNS resolver queries (%d, but should be %d at most). Try "
				"performing less network requests at once.",
			p_id, IP::RESOLVER_MAX_QUERIES));

	data->resolver->queue[p_id].status.set(IP::RESOLVER_STATUS_NONE);
}

void IP::clear_cache(const String& p_hostname)
{
	ERR_FAIL_NULL(data);
	_IP_ResolverPrivate* resolver = data->resolver;
	MutexLock lock(resolver->mutex);

	if (p_hostname.is_empty()) {
		resolver->cache.clear();
	}
	else {
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_NONE));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV4));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_IPV6));
		resolver->cache.erase(_IP_ResolverPrivate::get_cache_key(p_hostname, IP::TYPE_ANY));
	}
}

Vector<String> IP::_get_local_addresses()
{
	Vector<String> addresses;
	List<IPAddress> ip_addresses;
	get_local_addresses(&ip_addresses);
	for (const IPAddress& E : ip_addresses) {
		addresses.push_back(String(E));
	}

	return addresses;
}

void IP::get_local_addresses(List<IPAddress>* r_addresses)
{
	HashMap<String, Interface_Info> interfaces;
	get_local_interfaces(&interfaces);
	for (const KeyValue<String, Interface_Info>& E : interfaces) {
		for (const IPAddress& F : E.value.ip_addresses) {
			r_addresses->push_front(F);
		}
	}
}

void IP::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	data->resolver = memnew(_IP_ResolverPrivate);
	data->resolver->thread_abort.clear();
	data->resolver->thread.start(_IP_ResolverPrivate::_thread_function, data->resolver);
}

void IP::finalize()
{
	if (data == nullptr) {
		return;
	}
	if (data->resolver != nullptr) {
		data->resolver->thread_abort.set();
		data->resolver->sem.post();
		if (data->resolver->thread.is_started()) {
			data->resolver->thread.wait_to_finish();
		}
		memdelete(data->resolver);
		data->resolver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

void IP::reset_backend()
{
	_resolve_hostname = &_default_resolve_hostname;
	get_local_interfaces = &_default_get_local_interfaces;
}

// ==========================================================================
// ip_address.cpp
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

#include "ip_address.h"
/*
IPAddress::operator Variant() const {
	return operator String();
}*/

IPAddress::operator String() const {
	if (wildcard) {
		return "*";
	}

	if (!valid) {
		return "";
	}

	if (is_ipv4()) {
		// IPv4 address mapped to IPv6.
		return itos(field8[12]) + "." + itos(field8[13]) + "." + itos(field8[14]) + "." + itos(field8[15]);
	}
	String ret;
	for (int i = 0; i < 8; i++) {
		if (i > 0) {
			ret = ret + ":";
		}
		uint16_t num = (field8[i * 2] << 8) + field8[i * 2 + 1];
		ret = ret + String::num_int64(num, 16);
	}

	return ret;
}

bool IPAddress::_parse_ipv6(const String &p_string, IPAddress &r_ip) {
	int len = p_string.length();
	const char32_t *buf = p_string.ptr();

	int cur = 0;
	int shift = -1;
	for (int i = 0; i < len; i++) {
		for (int j = i; j < len; j++) {
			char32_t c = buf[j];
			if (c == ':') {
				if (j + 1 == len) {
					return false; // Can't end with a column (unless part of shortening).
				}
				if (buf[j + 1] == ':') {
					if (shift > -1) {
						return false; // Only one shortening allowed.
					} else if (j == 0) {
						shift = cur;
					} else {
						shift = cur + 1;
					}
					j++;
				} else if (i == j) {
					return false; // Stray column.
				}
				i = j;
				break;
			}
			if (j - i > 3) {
				return false;
			}
			if (c >= '0' && c <= '9') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= c - '0';
			} else if (c >= 'a' && c <= 'f') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= 10 + (c - 'a');
			} else if (c >= 'A' && c <= 'F') {
				r_ip.field16[cur] = r_ip.field16[cur] << 4;
				r_ip.field16[cur] |= 10 + (c - 'A');
			} else if (c == '.') {
				// IPv4 mapped IPv6 (e.g. "::FFFF:127.0.0.1").
				if (cur < 1 || r_ip.field16[cur - 1] != 0xFFFF) {
					return false; // IPv6 part must end with FFFF.
				}
				if (shift < 0 && cur != 6) {
					return false; // Needs 5 zeros, and FFFF "0:0:0:0:0:FFFF:127.0.0.1".
				}
				// Only empty bytes allowed before FFFF.
				r_ip.field16[cur] = 0;
				r_ip.field16[cur - 1] = 0;
				while (cur > 0) {
					cur--;
					if (r_ip.field16[cur] != 0) {
						return false;
					}
				}
				r_ip.field16[5] = 0xFFFF;
				return _parse_ipv4(p_string, i, &r_ip.field8[12]);
			} else {
				return false;
			}
			if (j + 1 == len) {
				i = j;
			}
		}
		r_ip.field16[cur] = BSWAP16(r_ip.field16[cur]);
		cur += 1;
		if (cur > 8 || (cur == 8 && i + 1 != len)) {
			return false;
		}
	}
	if (shift < 0) {
		return cur == 8; // Should have parsed 8 16-bits ints.
	} else if (shift > 7) {
		return false; // Can't shorten more than this.
	} else if (shift == cur || cur == 8) {
		return true; // Nothing to do (fields are assumed zeroized).
	}
	// Shift bytes.
	int pad = 8 - cur;
	int blank_end = shift + pad;
	for (int i = 7; i > shift; i--) {
		if (i < blank_end) {
			r_ip.field16[i] = 0;
		} else {
			r_ip.field16[i] = r_ip.field16[i - pad];
			r_ip.field16[i - pad] = 0;
		}
	}
	return true;
}

bool IPAddress::_parse_ipv4(const String &p_string, int p_start, uint8_t *r_dest) {
	int len = p_string.length();
	const char32_t *buf = p_string.ptr();

	int cur = 0;
	uint16_t next = 0;
	bool parsed = false;
	for (int i = p_start; i < len; i++) {
		char32_t c = buf[i];
		if (c == '.') {
			if (!parsed) {
				return false;
			}
			parsed = false;
			r_dest[cur] = next;
			next = 0;
			cur++;
			if (cur > 3) {
				return false;
			}
		} else if (c >= '0' && c <= '9') {
			parsed = true;
			next *= 10;
			next += c - '0';
			if (next > 255) {
				return false;
			}
		} else {
			return false; // Invalid char.
		}
	}
	if (!parsed) {
		return false;
	}
	r_dest[cur] = next;
	return parsed && cur == 3;
}

void IPAddress::clear() {
	memset(&field8[0], 0, sizeof(field8));
	valid = false;
	wildcard = false;
}

bool IPAddress::is_ipv4() const {
	return (field32[0] == 0 && field32[1] == 0 && field16[4] == 0 && field16[5] == 0xffff);
}

const uint8_t *IPAddress::get_ipv4() const {
	ERR_FAIL_COND_V_MSG(!is_ipv4(), &(field8[12]), "IPv4 requested, but current IP is IPv6."); // Not the correct IPv4 (it's an IPv6), but we don't want to return a null pointer risking an engine crash.
	return &(field8[12]);
}

void IPAddress::set_ipv4(const uint8_t *p_ip) {
	clear();
	valid = true;
	field16[5] = 0xffff;
	field32[3] = *((const uint32_t *)p_ip);
}

const uint8_t *IPAddress::get_ipv6() const {
	return field8;
}

void IPAddress::set_ipv6(const uint8_t *p_buf) {
	clear();
	valid = true;
	for (int i = 0; i < 16; i++) {
		field8[i] = p_buf[i];
	}
}

bool IPAddress::is_valid_ip_address(const String &p_string) {
	IPAddress addr;
	if (p_string.length() < IPV6_MAX_STRING_LENGTH && p_string.contains_char(':')) {
		return _parse_ipv6(p_string, addr);
	} else if (p_string.length() < IPV4_MAX_STRING_LENGTH) { // Try IPv4.
		return _parse_ipv4(p_string, 0, &addr.field8[12]);
	}
	return false;
}

IPAddress::IPAddress(const String &p_string) {
	clear();

	if (p_string == "*") {
		// Wildcard (not a valid IP).
		wildcard = true;

	} else if (p_string.length() < IPV6_MAX_STRING_LENGTH && p_string.contains_char(':')) {
		// IPv6.
		valid = _parse_ipv6(p_string, *this);
		ERR_FAIL_COND_MSG(!valid, "Invalid IPv6 address: " + p_string);

	} else if (p_string.length() < IPV4_MAX_STRING_LENGTH) {
		// IPv4 (mapped to IPv6 internally).
		field16[5] = 0xffff;
		valid = _parse_ipv4(p_string, 0, &field8[12]);
		ERR_FAIL_COND_MSG(!valid, "Invalid IPv4 address: " + p_string);

	} else {
		ERR_PRINT("Invalid IP address.");
	}
}

_FORCE_INLINE_ static void _32_to_buf(uint8_t *p_dst, uint32_t p_n) {
	p_dst[0] = (p_n >> 24) & 0xff;
	p_dst[1] = (p_n >> 16) & 0xff;
	p_dst[2] = (p_n >> 8) & 0xff;
	p_dst[3] = (p_n >> 0) & 0xff;
}

IPAddress::IPAddress(uint32_t p_a, uint32_t p_b, uint32_t p_c, uint32_t p_d, bool p_is_v6) {
	clear();
	valid = true;
	if (!p_is_v6) {
		// Mapped to IPv6.
		field16[5] = 0xffff;
		field8[12] = p_a;
		field8[13] = p_b;
		field8[14] = p_c;
		field8[15] = p_d;
	} else {
		_32_to_buf(&field8[0], p_a);
		_32_to_buf(&field8[4], p_b);
		_32_to_buf(&field8[8], p_c);
		_32_to_buf(&field8[12], p_d);
	}
}

// ==========================================================================
// net_socket.cpp
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

#include "net_socket.h"

Ref<NetSocket> (*NetSocket::_create)() = nullptr;

Ref<NetSocket> NetSocket::create() {
	if (_create) {
		return _create();
	}

	ERR_PRINT("Unable to create network socket, platform not supported");
	return nullptr;
}

// ==========================================================================
// remote_filesystem_client.cpp
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

#include "remote_filesystem_client.h"

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/stream_peer_socket.h"
#include "core/os/os.h"
#include "core/string/string_builder.h"

#define FILESYSTEM_CACHE_VERSION 1
#define FILESYSTEM_PROTOCOL_VERSION 1
#define PASSWORD_LENGTH 32

#define FILES_SUBFOLDER "remote_filesystem_files"
#define FILES_CACHE_FILE "remote_filesystem.cache"

Vector<RemoteFilesystemClient::FileCache> RemoteFilesystemClient::_load_cache_file() {
	Ref<FileAccess> fa = FileAccess::open(cache_path.path_join(FILES_CACHE_FILE), FileAccess::READ);
	if (fa.is_null()) {
		return Vector<FileCache>(); // No cache, return empty
	}

	int version = fa->get_line().to_int();
	if (version != FILESYSTEM_CACHE_VERSION) {
		return Vector<FileCache>(); // Version mismatch, ignore everything.
	}

	String file_path = cache_path.path_join(FILES_SUBFOLDER);

	Vector<FileCache> file_cache;

	while (!fa->eof_reached()) {
		String l = fa->get_line();
		Vector<String> fields = l.split("::");
		if (fields.size() != 3) {
			break;
		}
		FileCache fc;
		fc.path = fields[0];
		fc.server_modified_time = fields[1].to_int();
		fc.modified_time = fields[2].to_int();

		String full_path = file_path.path_join(fc.path);
		if (!FileAccess::exists(full_path)) {
			continue; // File is gone.
		}

		if (FileAccess::get_modified_time(full_path) != fc.modified_time) {
			DirAccess::remove_absolute(full_path); // Take the chance to remove this file and assume we no longer have it.
			continue;
		}

		file_cache.push_back(fc);
	}

	return file_cache;
}

Error RemoteFilesystemClient::_store_file(const String &p_path, const LocalVector<uint8_t> &p_file, uint64_t &r_modified_time) {
	r_modified_time = 0;
	String full_path = cache_path.path_join(FILES_SUBFOLDER).path_join(p_path);
	String base_file_dir = full_path.get_base_dir();

	if (!validated_directories.has(base_file_dir)) {
		// Verify that path exists before writing file, but only verify once for performance.
		DirAccess::make_dir_recursive_absolute(base_file_dir);
		validated_directories.insert(base_file_dir);
	}

	Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_FILE_CANT_OPEN, vformat("Unable to open file for writing to remote filesystem cache: '%s'.", p_path));
	f->store_buffer(p_file.ptr(), p_file.size());
	Error err = f->get_error();
	if (err) {
		return err;
	}
	f.unref(); // Unref to ensure file is not locked and modified time can be obtained.

	r_modified_time = FileAccess::get_modified_time(full_path);
	return OK;
}

Error RemoteFilesystemClient::_remove_file(const String &p_path) {
	return DirAccess::remove_absolute(cache_path.path_join(FILES_SUBFOLDER).path_join(p_path));
}
Error RemoteFilesystemClient::_store_cache_file(const Vector<FileCache> &p_cache) {
	String full_path = cache_path.path_join(FILES_CACHE_FILE);
	String base_file_dir = full_path.get_base_dir();
	Error err = DirAccess::make_dir_recursive_absolute(base_file_dir);
	ERR_FAIL_COND_V_MSG(err != OK && err != ERR_ALREADY_EXISTS, err, vformat("Unable to create base directory to store cache file: '%s'.", base_file_dir));

	Ref<FileAccess> f = FileAccess::open(full_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_FILE_CANT_OPEN, vformat("Unable to open the remote cache file for writing: '%s'.", full_path));
	f->store_line(itos(FILESYSTEM_CACHE_VERSION));
	for (int i = 0; i < p_cache.size(); i++) {
		String l = p_cache[i].path + "::" + itos(p_cache[i].server_modified_time) + "::" + itos(p_cache[i].modified_time);
		f->store_line(l);
	}
	return OK;
}

Error RemoteFilesystemClient::synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path) {
	Error err = _synchronize_with_server(p_host, p_port, p_password, r_cache_path);
	// Ensure no memory is kept
	validated_directories.reset();
	cache_path = String();
	return err;
}

void RemoteFilesystemClient::_update_cache_path(String &r_cache_path) {
	r_cache_path = cache_path.path_join(FILES_SUBFOLDER);
}

Error RemoteFilesystemClient::_synchronize_with_server(const String &p_host, int p_port, const String &p_password, String &r_cache_path) {
	cache_path = r_cache_path;
	{
		Ref<DirAccess> dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
		dir->change_dir(cache_path);
		cache_path = dir->get_current_dir();
	}

	Ref<StreamPeerSocket> tcp_client;
	tcp_client.instantiate();

	IPAddress ip = p_host.is_valid_ip_address() ? IPAddress(p_host) : IP::resolve_hostname(p_host);
	ERR_FAIL_COND_V_MSG(!ip.is_valid(), ERR_INVALID_PARAMETER, vformat("Unable to resolve remote filesystem server hostname: '%s'.", p_host));
	print_verbose(vformat("Remote Filesystem: Connecting to host %s, port %d.", ip, p_port));
	Error err = tcp_client->connect_to_host(ip, p_port);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Unable to open connection to remote file server (%s, port %d) failed.", String(p_host), p_port));

	while (tcp_client->get_status() == StreamPeerSocket::STATUS_CONNECTING) {
		tcp_client->poll();
		OS::delay_usec(100);
	}

	if (tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		ERR_FAIL_V_MSG(ERR_CANT_CONNECT, vformat("Connection to remote file server (%s, port %d) failed.", String(p_host), p_port));
	}

	// Connection OK, now send the current file state.
	print_verbose("Remote Filesystem: Connection OK.");

	// Header (GRFS) - Godot Remote File System
	print_verbose("Remote Filesystem: Sending header");
	tcp_client->put_u8('G');
	tcp_client->put_u8('R');
	tcp_client->put_u8('F');
	tcp_client->put_u8('S');
	// Protocol version
	tcp_client->put_32(FILESYSTEM_PROTOCOL_VERSION);
	print_verbose("Remote Filesystem: Sending password");
	uint8_t password[PASSWORD_LENGTH]; // Send fixed size password, since it's easier and safe to validate.
	for (int i = 0; i < PASSWORD_LENGTH; i++) {
		if (i < p_password.length()) {
			password[i] = p_password[i];
		} else {
			password[i] = 0;
		}
	}
	tcp_client->put_data(password, PASSWORD_LENGTH);
	print_verbose("Remote Filesystem: Tags.");
	Vector<String> tags;
	{
		tags.push_back(OS::get_identifier());
		switch (OS::get_preferred_texture_format()) {
			case OS::PREFERRED_TEXTURE_FORMAT_S3TC_BPTC: {
				tags.push_back("bptc");
				tags.push_back("s3tc");
			} break;
			case OS::PREFERRED_TEXTURE_FORMAT_ETC2_ASTC: {
				tags.push_back("etc2");
				tags.push_back("astc");
			} break;
		}
	}

	tcp_client->put_32(tags.size());
	for (int i = 0; i < tags.size(); i++) {
		tcp_client->put_utf8_string(tags[i]);
	}
	// Size of compressed list of files
	print_verbose("Remote Filesystem: Sending file list");

	Vector<FileCache> file_cache = _load_cache_file();

	// Encode file cache to send it via network.
	Vector<uint8_t> file_cache_buffer;
	if (file_cache.size()) {
		StringBuilder sbuild;
		for (int64_t i = 0; i < file_cache.size(); i++) {
			sbuild.append(file_cache[i].path);
			sbuild.append("::");
			sbuild.append(itos(file_cache[i].server_modified_time));
			sbuild.append("\n");
		}
		String s = sbuild.as_string();
		CharString cs = s.utf8();
		file_cache_buffer.resize(Compression::get_max_compressed_buffer_size(cs.length(), Compression::MODE_ZSTD));
		const int64_t res_len = Compression::compress(file_cache_buffer.ptrw(), (const uint8_t *)cs.ptr(), cs.length(), Compression::MODE_ZSTD);
		file_cache_buffer.resize(res_len);

		tcp_client->put_32(cs.length()); // Size of buffer uncompressed
		tcp_client->put_32(file_cache_buffer.size()); // Size of buffer compressed
		tcp_client->put_data(file_cache_buffer.ptr(), file_cache_buffer.size()); // Buffer
	} else {
		tcp_client->put_32(0); // No file cache buffer
	}

	tcp_client->poll();
	ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected after sending header.");

	uint32_t file_count = tcp_client->get_u32();

	ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected while waiting for file list");

	LocalVector<uint8_t> file_buffer;

	Vector<FileCache> temp_file_cache;

	HashSet<String> files_processed;
	for (uint32_t i = 0; i < file_count; i++) {
		String file = tcp_client->get_utf8_string();
		ERR_FAIL_COND_V_MSG(file == String(), ERR_CONNECTION_ERROR, "Invalid file name received from remote filesystem.");
		uint64_t server_modified_time = tcp_client->get_u64();
		ERR_FAIL_COND_V_MSG(tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED, ERR_CONNECTION_ERROR, "Remote filesystem server disconnected while waiting for file info.");

		FileCache fc;
		fc.path = file;
		fc.server_modified_time = server_modified_time;
		temp_file_cache.push_back(fc);

		files_processed.insert(file);
	}

	Vector<FileCache> new_file_cache;

	// Get the actual files. As a robustness measure, if the connection is interrupted here, any file not yet received will be considered removed.
	// Since the file changed anyway, this makes it the easiest way to keep robustness.

	bool server_disconnected = false;
	for (uint32_t i = 0; i < file_count; i++) {
		String file = temp_file_cache[i].path;

		if (temp_file_cache[i].server_modified_time == 0 || server_disconnected) {
			// File was removed, or server disconnected before transferring it. Since it's no longer valid, remove anyway.
			_remove_file(file);
			continue;
		}

		uint64_t file_size = tcp_client->get_u64();
		file_buffer.resize(file_size);

		err = tcp_client->get_data(file_buffer.ptr(), file_size);
		if (err != OK) {
			ERR_PRINT(vformat("Error retrieving file from remote filesystem: '%s'.", file));
			server_disconnected = true;
		}

		if (tcp_client->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
			// Early disconnect, stop accepting files.
			server_disconnected = true;
		}

		if (server_disconnected) {
			// No more server, transfer is invalid, remove this file.
			_remove_file(file);
			continue;
		}

		uint64_t modified_time = 0;
		err = _store_file(file, file_buffer, modified_time);
		if (err != OK) {
			server_disconnected = true;
			continue;
		}
		FileCache fc = temp_file_cache[i];
		fc.modified_time = modified_time;
		new_file_cache.push_back(fc);
	}

	print_verbose("Remote Filesystem: Updating the cache file.");

	// Go through the list of local files read initially (file_cache) and see which ones are
	// unchanged (not sent again from the server).
	// These need to be re-saved in the new list (new_file_cache).

	for (int i = 0; i < file_cache.size(); i++) {
		if (files_processed.has(file_cache[i].path)) {
			continue; // This was either added or removed, so skip.
		}
		new_file_cache.push_back(file_cache[i]);
	}

	err = _store_cache_file(new_file_cache);
	ERR_FAIL_COND_V_MSG(err != OK, ERR_FILE_CANT_OPEN, "Error writing the remote filesystem file cache.");

	print_verbose("Remote Filesystem: Update success.");

	_update_cache_path(r_cache_path);
	return OK;
}
#ifdef FILESYSTEM_CACHE_VERSION
#undef FILESYSTEM_CACHE_VERSION
#endif
#ifdef FILESYSTEM_PROTOCOL_VERSION
#undef FILESYSTEM_PROTOCOL_VERSION
#endif
#ifdef PASSWORD_LENGTH
#undef PASSWORD_LENGTH
#endif
#ifdef FILES_SUBFOLDER
#undef FILES_SUBFOLDER
#endif
#ifdef FILES_CACHE_FILE
#undef FILES_CACHE_FILE
#endif
