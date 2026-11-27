/**************************************************************************/
/*  net_socket.h                                                          */
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

#include "core/io/ip.h"

class NetSocket final : public RefCounted
{
public:
	static Ref<NetSocket> (*_create)();
	static Ref<NetSocket> create();

	enum PollType : int32_t
	{
		POLL_TYPE_IN,
		POLL_TYPE_OUT,
		POLL_TYPE_IN_OUT
	};

	enum Type : int32_t
	{
		TYPE_NONE,
		TYPE_TCP,
		TYPE_UDP,
	};

	enum class Family
	{
		NONE,
		INET,
		UNIX,
	};

	class Address
	{
		Family _family = Family::NONE;
		CharString _path;
		IPAddress _ip;
		uint16_t _port = 0;

	public:
		_FORCE_INLINE_ Family get_family() const { return _family; }
		_FORCE_INLINE_ bool is_inet() const { return _family == Family::INET; }
		_FORCE_INLINE_ bool is_unix() const { return _family == Family::UNIX; }
		_FORCE_INLINE_ bool is_valid() const { return is_inet() || is_unix(); }
		_FORCE_INLINE_ const IPAddress& ip() const { return _ip; }
		_FORCE_INLINE_ const uint16_t& port() const { return _port; }
		_FORCE_INLINE_ const CharString& get_path() const { return _path; }

		Address() {}
		Address(const IPAddress& p_addr, uint16_t p_port) : _family(Family::INET), _ip(p_addr), _port(p_port) {}
		Address(const String& p_path) : _family(Family::UNIX), _path(p_path.utf8()) {}
		Address(const CharString& p_path) : _family(Family::UNIX), _path(p_path) {}
	};

#if defined(WINDOWS_ENABLED) || (defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE))
private:
	enum NetError
	{
		ERR_NET_WOULD_BLOCK,
		ERR_NET_IS_CONNECTED,
		ERR_NET_IN_PROGRESS,
		ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE,
		ERR_NET_UNAUTHORIZED,
		ERR_NET_BUFFER_TOO_SMALL,
		ERR_NET_OTHER,
	};

#if defined(WINDOWS_ENABLED)
	uintptr_t _sock = ~(uintptr_t)0;
	IP::Type _ip_type = IP::TYPE_NONE;
	bool _is_stream = false;

	NetError _get_socket_error() const;
	void _set_socket(uintptr_t p_sock, IP::Type p_ip_type, bool p_is_stream);
	_FORCE_INLINE_ Error _change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add);
	bool _can_use_ip(const IPAddress& p_ip, const bool p_for_bind) const;
#else
	int _sock = -1;
	Family _family = Family::NONE;
	IP::Type _ip_type = IP::TYPE_NONE;
	bool _is_stream = false;
	CharString _unix_path;
	bool _unlink_on_close = false;
	bool wants_broadcast = false;
	int multicast_groups = 0;

	NetError _get_socket_error() const;
	void _set_socket(int p_sock, IP::Type p_ip_type, bool p_is_stream);
	_FORCE_INLINE_ Error _change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add);
	_FORCE_INLINE_ void _set_close_exec_enabled(bool p_enabled);

	bool _can_use_ip(const IPAddress& p_ip, const bool p_for_bind) const;
	bool _can_use_path(const CharString& p_path) const;

	Error _inet_open(Type p_sock_type, IP::Type& r_ip_type);
	Error _inet_bind(IPAddress p_addr, uint16_t p_port);
	Error _inet_connect_to_host(IPAddress p_addr, uint16_t p_port);
	Error _inet_get_socket_address(IPAddress* r_ip, uint16_t* r_port) const;
	Ref<NetSocket> _inet_accept(IPAddress& r_ip, uint16_t& r_port);

	Error _unix_open();
	Error _unix_bind(const CharString& p_path);
	Error _unix_connect_to_host(const CharString& p_path);
	Ref<NetSocket> _unix_accept();
#endif

	static Ref<NetSocket> _create_func();

public:
	static inline void (*multicast_lock_acquire_fn)() = nullptr;
	static inline void (*multicast_lock_release_fn)() = nullptr;

	static void make_default();
	static void cleanup();

	Error open(Family p_family, Type p_type, IP::Type& r_ip_type);
	void close();
	Error bind(Address p_addr);
	Error listen(int p_max_pending);
	Error connect_to_host(Address p_addr);
	Error poll(PollType p_type, int p_timeout) const;
	Error recv(uint8_t* p_buffer, int p_len, int& r_read);
	Error recvfrom(uint8_t* p_buffer, int p_len, int& r_read, IPAddress& r_ip,
		uint16_t& r_port, bool p_peek = false);
	Error send(const uint8_t* p_buffer, int p_len, int& r_sent);
	Error sendto(
		const uint8_t* p_buffer, int p_len, int& r_sent, IPAddress p_ip, uint16_t p_port);
	Ref<NetSocket> accept(Address& r_addr);

	bool is_open() const;
	int get_available_bytes() const;
	Error get_socket_address(Address* r_addr) const;

	Error set_broadcasting_enabled(bool p_enabled);
	void set_blocking_enabled(bool p_enabled);
	void set_ipv6_only_enabled(bool p_enabled);
	void set_tcp_no_delay_enabled(bool p_enabled);
	void set_reuse_address_enabled(bool p_enabled);
	Error join_multicast_group(const IPAddress& p_multi_address, const String& p_if_name);
	Error leave_multicast_group(const IPAddress& p_multi_address, const String& p_if_name);

	NetSocket();
	~NetSocket();
#else
private:
	static Ref<NetSocket> _create_func() { return memnew(NetSocket); }

public:
	static void make_default() { _create = _create_func; }
	static void cleanup() { _create = nullptr; }

	Error open(Family p_family, Type p_type, IP::Type& r_ip_type) { return ERR_UNAVAILABLE; }
	void close() {}
	Error bind(Address p_addr) { return ERR_UNAVAILABLE; }
	Error listen(int p_max_pending) { return ERR_UNAVAILABLE; }
	Error connect_to_host(Address p_addr) { return ERR_UNAVAILABLE; }
	Error poll(PollType p_type, int p_timeout) const { return ERR_UNAVAILABLE; }
	Error recv(uint8_t* p_buffer, int p_len, int& r_read) { return ERR_UNAVAILABLE; }
	Error recvfrom(uint8_t* p_buffer, int p_len, int& r_read, IPAddress& r_ip,
		uint16_t& r_port, bool p_peek = false) { return ERR_UNAVAILABLE; }
	Error send(const uint8_t* p_buffer, int p_len, int& r_sent) { return ERR_UNAVAILABLE; }
	Error sendto(const uint8_t* p_buffer, int p_len, int& r_sent, IPAddress p_ip, uint16_t p_port) { return ERR_UNAVAILABLE; }
	Ref<NetSocket> accept(Address& r_addr) { return Ref<NetSocket>(); }

	bool is_open() const { return false; }
	int get_available_bytes() const { return -1; }
	Error get_socket_address(Address* r_addr) const { return ERR_UNAVAILABLE; }

	Error set_broadcasting_enabled(bool p_enabled) { return ERR_UNAVAILABLE; }
	void set_blocking_enabled(bool p_enabled) {}
	void set_ipv6_only_enabled(bool p_enabled) {}
	void set_tcp_no_delay_enabled(bool p_enabled) {}
	void set_reuse_address_enabled(bool p_enabled) {}
	Error join_multicast_group(const IPAddress& p_multi_address, const String& p_if_name) { return ERR_UNAVAILABLE; }
	Error leave_multicast_group(const IPAddress& p_multi_address, const String& p_if_name) { return ERR_UNAVAILABLE; }

	NetSocket() = default;
	~NetSocket() = default;
#endif
};

using NetSocketUnix = NetSocket;
using NetSocketWinSock = NetSocket;

class HashingContext : public RefCounted
{
public:
	enum HashType : int32_t
	{
		HASH_MD5,
		HASH_SHA1,
		HASH_SHA256
	};

private:
	void* ctx = nullptr;
	HashType type = HASH_MD5;

protected:
	void _create_ctx(HashType p_type);
	void _delete_ctx();

public:
	Error start(HashType p_type);
	Error update(const Vector<uint8_t>& p_chunk);
	Vector<uint8_t> finish();

	~HashingContext();
};

struct MbedTLSState;

class CryptoKey : public Resource
{
private:
	MbedTLSState* data = nullptr;

public:
	static CryptoKey* create(bool p_notify_postinitialize = true);
	static bool is_available();

	MbedTLSState* get_data() const { return data; }

	Error load(const String& p_path, bool p_public_only = false);
	Error save(const String& p_path, bool p_public_only = false);
	String save_to_string(bool p_public_only = false);
	Error load_from_string(const String& p_string_key, bool p_public_only = false);
	bool is_public_only() const;

	CryptoKey();
	~CryptoKey();
};

class X509Certificate : public Resource
{
private:
	MbedTLSState* data = nullptr;

public:
	static X509Certificate* create(bool p_notify_postinitialize = true);
	static bool is_available();

	MbedTLSState* get_data() const { return data; }

	Error load(const String& p_path);
	Error load_from_memory(const uint8_t* p_buffer, int p_len);
	Error save(const String& p_path);
	String save_to_string();
	Error load_from_string(const String& p_string);

	X509Certificate();
	~X509Certificate();
};

class TLSOptions : public RefCounted
{
private:
	enum Mode
	{
		MODE_CLIENT = 0,
		MODE_CLIENT_UNSAFE = 1,
		MODE_SERVER = 2,
	};

	Mode mode = MODE_CLIENT;
	String common_name;
	Ref<X509Certificate> trusted_ca_chain;
	Ref<X509Certificate> own_certificate;
	Ref<CryptoKey> private_key;

public:
	static Ref<TLSOptions> client(Ref<X509Certificate> p_trusted_chain = Ref<X509Certificate>(),
		const String& p_common_name_override = String());
	static Ref<TLSOptions> client_unsafe(Ref<X509Certificate> p_trusted_chain);
	static Ref<TLSOptions> server(Ref<CryptoKey> p_own_key, Ref<X509Certificate> p_own_certificate);

	String get_common_name_override() const { return common_name; }

	Ref<X509Certificate> get_trusted_ca_chain() const { return trusted_ca_chain; }

	Ref<X509Certificate> get_own_certificate() const { return own_certificate; }

	Ref<CryptoKey> get_private_key() const { return private_key; }

	bool is_server() const { return mode == MODE_SERVER; }

	bool is_unsafe_client() const { return mode == MODE_CLIENT_UNSAFE; }
};

class HMACContext : public RefCounted
{
private:
	MbedTLSState* data = nullptr;

public:
	static HMACContext* create(bool p_notify_postinitialize = true);
	static bool is_available();

	Error start(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_key);
	Error update(const Vector<uint8_t>& p_data);
	Vector<uint8_t> finish();

	HMACContext();
	~HMACContext();
};

class Crypto : public RefCounted
{
public:
	static Crypto* create(bool p_notify_postinitialize = true);
	static bool is_available();
	static void load_default_certificates(const String& p_path);

	Vector<uint8_t> generate_random_bytes(int p_bytes);
	Ref<CryptoKey> generate_rsa(int p_bytes);
	Ref<X509Certificate> generate_self_signed_certificate(Ref<CryptoKey> p_key,
		const String& p_issuer_name, const String& p_not_before, const String& p_not_after);

	Vector<uint8_t> sign(HashingContext::HashType p_hash_type,
		const Vector<uint8_t>& p_hash, Ref<CryptoKey> p_key);
	bool verify(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash,
		const Vector<uint8_t>& p_signature, Ref<CryptoKey> p_key);
	Vector<uint8_t> encrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_plaintext);
	Vector<uint8_t> decrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_ciphertext);

	Vector<uint8_t> hmac_digest(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_key,
		const Vector<uint8_t>& p_msg);

	// Compares two PackedByteArrays for equality without leaking timing information in order to
	// prevent timing attacks.
	// @see:
	// https://paragonie.com/blog/2015/11/preventing-timing-attacks-on-string-comparison-with-double-hmac-strategy
	bool constant_time_compare(const Vector<uint8_t>& p_trusted, const Vector<uint8_t>& p_received);

	Crypto();
	~Crypto();
};

#include "core/typedefs.h"

#include <cstddef>
#include <cstdint>

class String;

class CryptoCore {
private:
	static bool initialized;

public:
	static void initialize();
	static void finalize();

	enum HashAlgorithm {
		HASH_ALG_MD5,
		HASH_ALG_SHA1,
		HASH_ALG_SHA256,
	};

	class HashContext {
	private:
		void *ctx = nullptr;
		HashAlgorithm alg = HASH_ALG_SHA256;

	public:
		explicit HashContext(HashAlgorithm p_alg);
		~HashContext();

		Error start();
		Error update(const uint8_t *p_src, size_t p_len);
		Error finish(unsigned char *r_hash);
	};

	class AESContext {
	public:
		enum class Mode {
			NONE,
			ENCRYPT,
			DECRYPT,
		};
		enum class Cipher {
			NONE,
			CBC,
			CFB,
			ECB,
		};

	private:
		uint32_t key_id = 0;
		uint32_t alg = 0;
		void *ctx = nullptr;

		void reset();

	public:
		AESContext();
		~AESContext();

		Error setup(Mode p_mode, Cipher p_cipher, const uint8_t *p_key, size_t p_key_length, const uint8_t *p_iv, size_t p_iv_size);
		Error update(const uint8_t *p_src, size_t p_src_size, uint8_t *r_dst, size_t p_dst_size);
		Error finish(uint8_t *r_dst, size_t p_dst_size);
	};

	static Error generate_random(uint8_t *r_buffer, size_t p_buffer_len);

	static String b64_encode_str(const uint8_t *p_src, size_t p_src_len);
	static Error b64_encode(uint8_t *r_dst, size_t p_dst_len, size_t *r_len, const uint8_t *p_src, size_t p_src_len);
	static Error b64_decode(uint8_t *r_dst, size_t p_dst_len, size_t *r_len, const uint8_t *p_src, size_t p_src_len);

	static Error md5(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[16]);
	static Error sha1(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[20]);
	static Error sha256(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[32]);

	static Error encrypt_cfb(const uint8_t *p_src, uint8_t *r_dst, size_t p_length, const uint8_t *p_key, size_t p_key_length, const uint8_t p_iv[16]);
	static Error decrypt_cfb(const uint8_t *p_src, uint8_t *r_dst, size_t p_length, const uint8_t *p_key, size_t p_key_length, const uint8_t p_iv[16]);
};

class AESContext : public RefCounted
{
public:
	enum Mode : int32_t
	{
		MODE_ECB_ENCRYPT,
		MODE_ECB_DECRYPT,
		MODE_CBC_ENCRYPT,
		MODE_CBC_DECRYPT,
		MODE_MAX
	};

private:
	Mode mode = MODE_MAX;
	CryptoCore::AESContext ctx;

protected:

public:
	Error start(
		Mode p_mode, const Vector<uint8_t>& p_key, const Vector<uint8_t>& p_iv = Vector<uint8_t>());
	Vector<uint8_t> update(const Vector<uint8_t>& p_src);
	Vector<uint8_t> get_iv_state();
	void finish();
};

namespace ResourceLoaderConstants
{

enum CacheMode
{
	CACHE_MODE_IGNORE,
	CACHE_MODE_REUSE,
	CACHE_MODE_REPLACE,
	CACHE_MODE_IGNORE_DEEP,
	CACHE_MODE_REPLACE_DEEP,
};

} // namespace ResourceLoaderConstants

#include "core/os/mutex.h"

namespace CoreBind
{
class ResourceLoader;
}

class ConditionVariable;

template <int Tag> class SafeBinaryMutex;

class ResourceFormatLoader : public RefCounted
{
public:
	using CacheMode = ResourceLoaderConstants::CacheMode;
	static constexpr CacheMode CACHE_MODE_IGNORE = ResourceLoaderConstants::CACHE_MODE_IGNORE;
	static constexpr CacheMode CACHE_MODE_REUSE = ResourceLoaderConstants::CACHE_MODE_REUSE;
	static constexpr CacheMode CACHE_MODE_REPLACE = ResourceLoaderConstants::CACHE_MODE_REPLACE;
	static constexpr CacheMode CACHE_MODE_IGNORE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_IGNORE_DEEP;
	static constexpr CacheMode CACHE_MODE_REPLACE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_REPLACE_DEEP;

	using LoadFn = Ref<Resource> (*)(const String&, const String&, Error*, bool, float*, CacheMode);
	using HandlesTypeFn = bool (*)(const String&);
	using GetResourceTypeFn = String (*)(const String&);
	using GetExtensionsForTypeFn = void (*)(const String&, List<String>*);

private:
	LoadFn load_fn = nullptr;
	const char* const* extensions = nullptr;
	HandlesTypeFn handles_type_fn = nullptr;
	GetResourceTypeFn get_resource_type_fn = nullptr;
	GetExtensionsForTypeFn get_exts_for_type_fn = nullptr;
	bool disable_uid_file = false;
	bool is_importer = false;

public:
	explicit ResourceFormatLoader(bool p_is_importer = false) : is_importer(p_is_importer) {}
	ResourceFormatLoader(LoadFn p_load_fn, const char* const* p_extensions,
		HandlesTypeFn p_handles_fn, GetResourceTypeFn p_get_type_fn,
		bool p_disable_uid_file = false, GetExtensionsForTypeFn p_exts_for_type_fn = nullptr) :
		load_fn(p_load_fn),
		extensions(p_extensions),
		handles_type_fn(p_handles_fn),
		get_resource_type_fn(p_get_type_fn),
		get_exts_for_type_fn(p_exts_for_type_fn),
		disable_uid_file(p_disable_uid_file)
	{
	}

	static Ref<ResourceFormatLoader> create_compressed_texture_loader();
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE);
	virtual bool exists(const String& p_path) const;
	virtual void get_recognized_extensions(List<String>* p_extensions) const;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const;
	virtual bool recognize_path(const String& p_path, const String& p_for_type = String()) const;
	virtual bool handles_type(const String& p_type) const;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	virtual String get_resource_type(const String& p_path) const;
	virtual String get_resource_script_class(const String& p_path) const;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const;
	virtual bool has_custom_uid_support() const;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);
	virtual Error rename_dependencies(const String& p_path, const HashMap<String, String>& p_map);

	virtual bool is_import_valid(const String& p_path) const;

	virtual bool is_imported(const String& p_path) const;

	virtual int get_import_order(const String& p_path) const;

	virtual String get_import_group_file(const String& p_path) const;

	virtual ~ResourceFormatLoader() = default;
};

typedef void (*ResourceLoadErrorNotify)(const String& p_text);
typedef void (*DependencyErrorNotify)(
	const String& p_loading, const String& p_which, const String& p_type);

typedef Error (*ResourceLoaderImport)(const String& p_path);
typedef void (*ResourceLoadedCallback)(Ref<Resource> p_resource, const String& p_path);

class ResourceLoader
{
	friend class LoadToken;
	friend class CoreBind::ResourceLoader;

	enum
	{
		MAX_LOADERS = 64
	};

	struct ThreadLoadTask;

public:
	using CacheMode = ResourceLoaderConstants::CacheMode;
	static constexpr CacheMode CACHE_MODE_IGNORE = ResourceLoaderConstants::CACHE_MODE_IGNORE;
	static constexpr CacheMode CACHE_MODE_REUSE = ResourceLoaderConstants::CACHE_MODE_REUSE;
	static constexpr CacheMode CACHE_MODE_REPLACE = ResourceLoaderConstants::CACHE_MODE_REPLACE;
	static constexpr CacheMode CACHE_MODE_IGNORE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_IGNORE_DEEP;
	static constexpr CacheMode CACHE_MODE_REPLACE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_REPLACE_DEEP;

	enum ThreadLoadStatus
	{
		THREAD_LOAD_INVALID_RESOURCE,
		THREAD_LOAD_IN_PROGRESS,
		THREAD_LOAD_FAILED,
		THREAD_LOAD_LOADED
	};

	enum LoadThreadMode
	{
		LOAD_THREAD_FROM_CURRENT,
		LOAD_THREAD_SPAWN_SINGLE,
		LOAD_THREAD_DISTRIBUTE,
	};

	struct LoadToken : public RefCounted
	{
		String local_path;
		String user_path;
		uint32_t user_rc = 0; // Having user RC implies regular RC incremented in one, until the
							  // user RC reaches zero.
		ThreadLoadTask* task_if_unregistered = nullptr;

		void clear();

		virtual ~LoadToken();
	};

	static const int BINARY_MUTEX_TAG = 1;

	static Ref<LoadToken> _load_start(const String& p_path, const String& p_type_hint,
		LoadThreadMode p_thread_mode, CacheMode p_cache_mode, bool p_for_user = false);
	static Ref<Resource> _load_complete(LoadToken& p_load_token, Error* r_error);

private:
	static LoadToken* _load_threaded_request_reuse_user_token(const String& p_path);
	static void _load_threaded_request_setup_user_token(LoadToken* p_token, const String& p_path);

	static Ref<Resource> _load_complete_inner(LoadToken& p_load_token, Error* r_error,
		MutexLock<SafeBinaryMutex<BINARY_MUTEX_TAG>>& p_thread_load_lock);

	static Ref<ResourceFormatLoader> loader[MAX_LOADERS];
	static int loader_count;
	static bool timestamp_on_load;

	static void* err_notify_ud;
	static ResourceLoadErrorNotify err_notify;
	static void* dep_err_notify_ud;
	static DependencyErrorNotify dep_err_notify;
	static bool abort_on_missing_resource;
	static bool create_missing_resources_if_class_unavailable;
	static HashMap<String, Vector<String>> translation_remaps;

	static String _path_remap(const String& p_path, bool* r_translation_remapped = nullptr);
	friend class Resource;

	static SelfList<Resource>::List remapped_list;

	friend class ResourceFormatImporter;

	static Ref<Resource> _load(const String& p_path, const String& p_original_path,
		const String& p_type_hint, CacheMode p_cache_mode, Error* r_error, bool p_use_sub_threads,
		float* r_progress);

	static ResourceLoadedCallback _loaded_callback;

	static Ref<ResourceFormatLoader> _find_custom_resource_format_loader(const String& p_path);

	struct ThreadLoadTask
	{
		Thread::ID thread_id =
			0; // Used if running on an user thread (e.g., simple non-threaded load).
		int thread_index = -1;
		ConditionVariable* cond_var = nullptr; // In not in the worker pool or already awaiting,
											   // this is used as a secondary awaiting mechanism.
		uint32_t awaiters_count = 0;
		LoadToken* load_token = nullptr;
		String local_path;
		String type_hint;
		float progress = 0.0f;
		float max_reported_progress = 0.0f;
		uint64_t last_progress_check_main_thread_frame = UINT64_MAX;
		ThreadLoadStatus status = THREAD_LOAD_IN_PROGRESS;
		CacheMode cache_mode = CACHE_MODE_REUSE;
		Error error = OK;
		Ref<Resource> resource;
		LocalVector<Ref<Resource>> resource_dependencies; // We need to keep these alive for as long
														  // as the task is alive at least.
		ThreadLoadTask* parent_task = nullptr;
		HashSet<String> sub_tasks;

		bool awaited : 1; // If it's in the pool, this helps not awaiting from more than one
						  // dependent thread.
		bool need_wait : 1;
		bool in_progress_check : 1; // Measure against recursion cycles in progress reporting.
									// Cycles are not expected, but can happen due to how it's
									// currently implemented.
		bool use_sub_threads : 1;
		bool started_load : 1;
		bool finished_load : 1;
		bool connections_propagated : 1;

		struct ResourceChangedConnection
		{
			Resource* source = nullptr;
			uint32_t flags = 0;
		};

		LocalVector<ResourceChangedConnection> resource_changed_connections;

		ThreadLoadTask()
			: awaited(false), need_wait(true), in_progress_check(false), use_sub_threads(false),
			  started_load(false), finished_load(false), connections_propagated(false)
		{
		}
	};

	static void _run_load_task(void* p_userdata);

	static thread_local bool import_thread;
	static thread_local int load_nesting;
	static thread_local HashMap<int, HashMap<String, Ref<Resource>>>
		res_ref_overrides; // Outermost key is nesting level.
	static thread_local ThreadLoadTask* curr_load_task;

	static SafeBinaryMutex<BINARY_MUTEX_TAG> thread_load_mutex;
	friend SafeBinaryMutex<BINARY_MUTEX_TAG>& _get_res_loader_mutex();

	static HashMap<String, ThreadLoadTask> thread_load_tasks;
	static HashMap<int, String> thread_waiting_on;
	static LocalVector<int> yielders;
	static bool cleaning_tasks;

	static HashMap<String, LoadToken*> user_load_tokens;

	static float _dependency_get_progress(const String& p_path);

	static bool _ensure_load_progress();

	static String _validate_local_path(const String& p_path);

public:
	static Error load_threaded_request(const String& p_path, const String& p_type_hint = "",
		bool p_use_sub_threads = false, CacheMode p_cache_mode = CACHE_MODE_REUSE);
	static ThreadLoadStatus load_threaded_get_status(
		const String& p_path, float* r_progress = nullptr);
	static Ref<Resource> load_threaded_get(const String& p_path, Error* r_error = nullptr);

	static bool is_within_load() { return load_nesting > 0; }

	static void resource_changed_emit(Resource* p_source);

	static Ref<Resource> load(const String& p_path, const String& p_type_hint = "",
		CacheMode p_cache_mode = CACHE_MODE_REUSE, Error* r_error = nullptr);
	static bool exists(const String& p_path, const String& p_type_hint = "");

	static void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions);
	static void add_resource_format_loader(
		Ref<ResourceFormatLoader> p_format_loader, bool p_at_front = false);
	static void remove_resource_format_loader(Ref<ResourceFormatLoader> p_format_loader);
	static void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	static String get_resource_type(const String& p_path);
	static String get_resource_script_class(const String& p_path);
	static ResourceUID::ID get_resource_uid(const String& p_path);
	static bool should_create_uid_file(const String& p_path);
	static void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);
	static Error rename_dependencies(const String& p_path, const HashMap<String, String>& p_map);
	static bool is_import_valid(const String& p_path);
	static String get_import_group_file(const String& p_path);
	static bool is_imported(const String& p_path);

	static void set_is_import_thread(bool p_import_thread);

	static void set_timestamp_on_load(bool p_timestamp) { timestamp_on_load = p_timestamp; }

	static bool get_timestamp_on_load() { return timestamp_on_load; }

	// Loaders can safely use this regardless which thread they are running on.
	static void notify_load_error(const String& p_err);

	static void set_error_notify_func(ResourceLoadErrorNotify p_err_notify)
	{
		err_notify = p_err_notify;
	}

	// Loaders can safely use this regardless which thread they are running on.
	static void notify_dependency_error(
		const String& p_path, const String& p_dependency, const String& p_type);

	static void set_dependency_error_notify_func(DependencyErrorNotify p_err_notify)
	{
		dep_err_notify = p_err_notify;
	}

	static void set_abort_on_missing_resources(bool p_abort)
	{
		abort_on_missing_resource = p_abort;
	}

	static bool get_abort_on_missing_resources() { return abort_on_missing_resource; }

	static String path_remap(const String& p_path);
	static String import_remap(const String& p_path);

	static void reload_translation_remaps();
	static void load_translation_remaps();
	static Ref<Resource> load_translation(Ref<FileAccess> p_f, Error* r_error = nullptr);
	static void clear_translation_remaps();

	static void clear_thread_load_tasks();

	static void set_load_callback(ResourceLoadedCallback p_callback);
	static ResourceLoaderImport import;

	static bool add_custom_resource_format_loader(const String& p_script_path);
	static void add_custom_loaders();
	static void remove_custom_loaders();

	static void set_create_missing_resources_if_class_unavailable(bool p_enable);

	_FORCE_INLINE_ static bool is_creating_missing_resources_if_class_unavailable_enabled()
	{
		return create_missing_resources_if_class_unavailable;
	}

	static Ref<Resource> ensure_resource_ref_override_for_outer_load(
		const String& p_path, const String& p_res_type);
	static Ref<Resource> get_resource_ref_override(const String& p_path);

	static bool is_cleaning_tasks();

	static Vector<String> list_directory(const String& p_directory);

	static void initialize();
	static void finalize();
};

#include "core/io/file_access.h"
#include "core/math/vector2.h"

/**
 * Image storage class. This is used to store an image in user memory, as well as
 * providing some basic methods for image manipulation.
 * Images can be loaded from a file, or registered into the Render object as textures.
 */

class Image;

// Function pointer prototypes.

typedef Error (*SavePNGFunc)(const String& p_path, const Ref<Image>& p_img);
typedef Vector<uint8_t> (*SavePNGBufferFunc)(const Ref<Image>& p_img);

typedef Error (*SaveJPGFunc)(const String& p_path, const Ref<Image>& p_img, float p_quality);
typedef Vector<uint8_t> (*SaveJPGBufferFunc)(const Ref<Image>& p_img, float p_quality);

typedef Ref<Image> (*ImageMemLoadFunc)(const uint8_t* p_data, int p_size);
typedef Ref<Image> (*ScalableImageMemLoadFunc)(const uint8_t* p_data, int p_size, float p_scale);

typedef Error (*SaveWebPFunc)(
	const String& p_path, const Ref<Image>& p_img, const bool p_lossy, const float p_quality);
typedef Vector<uint8_t> (*SaveWebPBufferFunc)(
	const Ref<Image>& p_img, const bool p_lossy, const float p_quality);

typedef Error (*SaveEXRFunc)(const String& p_path, const Ref<Image>& p_img, bool p_grayscale,
	bool p_color_image, float p_max_value);
typedef Vector<uint8_t> (*SaveEXRBufferFunc)(
	const Ref<Image>& p_img, bool p_grayscale, bool p_color_image, float p_max_value);

typedef Error (*SaveDDSFunc)(const String& p_path, const Ref<Image>& p_img);
typedef Vector<uint8_t> (*SaveDDSBufferFunc)(const Ref<Image>& p_img);

class Image : public Resource
{
public:
	enum
	{
		MAX_WIDTH = (1 << 24),	// Force a limit somehow.
		MAX_HEIGHT = (1 << 24), // Force a limit somehow.
		MAX_PIXELS = 268435456	// 16384 ^ 2
	};

	enum Format : int32_t
	{
		FORMAT_L8,	// Luminance
		FORMAT_LA8, // Luminance-Alpha
		FORMAT_R8,
		FORMAT_RG8,
		FORMAT_RGB8,
		FORMAT_RGBA8,
		FORMAT_RGBA4444,
		FORMAT_RGB565,
		FORMAT_RF, // Float
		FORMAT_RGF,
		FORMAT_RGBF,
		FORMAT_RGBAF,
		FORMAT_RH, // Half
		FORMAT_RGH,
		FORMAT_RGBH,
		FORMAT_RGBAH,
		FORMAT_RGBE9995,
		FORMAT_DXT1,	   // BC1
		FORMAT_DXT3,	   // BC2
		FORMAT_DXT5,	   // BC3
		FORMAT_RGTC_R,	   // BC4
		FORMAT_RGTC_RG,	   // BC5
		FORMAT_BPTC_RGBA,  // BC7
		FORMAT_BPTC_RGBF,  // BC6 Signed
		FORMAT_BPTC_RGBFU, // BC6 Unsigned
		FORMAT_ETC,		   // ETC1
		FORMAT_ETC2_R11,
		FORMAT_ETC2_R11S, // Signed, NOT srgb.
		FORMAT_ETC2_RG11,
		FORMAT_ETC2_RG11S, // Signed, NOT srgb.
		FORMAT_ETC2_RGB8,
		FORMAT_ETC2_RGBA8,
		FORMAT_ETC2_RGB8A1,
		FORMAT_ETC2_RA_AS_RG, // ETC2 RGBA with a RA-RG swizzle for normal maps.
		FORMAT_DXT5_RA_AS_RG, // BC3 with a RA-RG swizzle for normal maps.
		FORMAT_ASTC_4x4,
		FORMAT_ASTC_4x4_HDR,
		FORMAT_ASTC_8x8,
		FORMAT_ASTC_8x8_HDR,
		FORMAT_R16,
		FORMAT_RG16,
		FORMAT_RGB16,
		FORMAT_RGBA16,
		FORMAT_R16I,
		FORMAT_RG16I,
		FORMAT_RGB16I,
		FORMAT_RGBA16I,
		FORMAT_ASTC_6x6,
		FORMAT_ASTC_6x6_HDR,
		FORMAT_MAX
	};

	static const char* format_names[FORMAT_MAX];

	enum Interpolation
	{
		INTERPOLATE_NEAREST,
		INTERPOLATE_BILINEAR,
		INTERPOLATE_CUBIC,
		INTERPOLATE_TRILINEAR,
		INTERPOLATE_LANCZOS,
		// INTERPOLATE_TRICUBIC,
		// INTERPOLATE_GAUSS
	};

	// Used for obtaining optimal compression quality.
	enum UsedChannels
	{
		USED_CHANNELS_L,
		USED_CHANNELS_LA,
		USED_CHANNELS_R,
		USED_CHANNELS_RG,
		USED_CHANNELS_RGB,
		USED_CHANNELS_RGBA,
	};

#ifndef DISABLE_DEPRECATED
	// ASTC supports block formats other than 4x4.
	enum ASTCFormat
	{
		ASTC_FORMAT_4x4,
		ASTC_FORMAT_8x8,
	};
#endif

	enum BPTCFormat
	{
		BPTC_DETECT,
		BPTC_FORCE_SIGNED,
		BPTC_FORCE_UNSIGNED,
	};

	enum RoughnessChannel
	{
		ROUGHNESS_CHANNEL_R,
		ROUGHNESS_CHANNEL_G,
		ROUGHNESS_CHANNEL_B,
		ROUGHNESS_CHANNEL_A,
		ROUGHNESS_CHANNEL_L,
	};

	enum Image3DValidateError
	{
		VALIDATE_3D_OK,
		VALIDATE_3D_ERR_IMAGE_EMPTY,
		VALIDATE_3D_ERR_MISSING_IMAGES,
		VALIDATE_3D_ERR_EXTRA_IMAGES,
		VALIDATE_3D_ERR_IMAGE_SIZE_MISMATCH,
		VALIDATE_3D_ERR_IMAGE_FORMAT_MISMATCH,
		VALIDATE_3D_ERR_IMAGE_HAS_MIPMAPS,
	};

	enum CompressMode
	{
		COMPRESS_S3TC,
		COMPRESS_ETC,
		COMPRESS_ETC2,
		COMPRESS_BPTC,
		COMPRESS_ASTC,
		COMPRESS_MAX,
	};

	enum CompressSource
	{
		COMPRESS_SOURCE_GENERIC,
		COMPRESS_SOURCE_SRGB,
		COMPRESS_SOURCE_NORMAL,
		COMPRESS_SOURCE_MAX,
	};

	enum CompressProfile
	{
		COMPRESS_PROFILE_AUTOMATIC,
		COMPRESS_PROFILE_MAX_QUALITY,
		COMPRESS_PROFILE_COMPRESSED,
		COMPRESS_PROFILE_MAX_COMPRESSION,
		COMPRESS_PROFILE_MAX
	};

	enum AlphaMode
	{
		ALPHA_NONE,
		ALPHA_BIT,
		ALPHA_BLEND
	};

	struct BasisUniversalPackerParams
	{
		int uastc_level = 0;
		float rdo_quality_loss = 0;
	};

	// External saver function pointers.

	static inline SavePNGFunc save_png_func = nullptr;
	static inline SaveJPGFunc save_jpg_func = nullptr;
	static inline SaveEXRFunc save_exr_func = nullptr;
	static inline SaveWebPFunc save_webp_func = nullptr;
	static inline SaveDDSFunc save_dds_func = nullptr;
	static inline SavePNGBufferFunc save_png_buffer_func = nullptr;
	static inline SaveEXRBufferFunc save_exr_buffer_func = nullptr;
	static inline SaveJPGBufferFunc save_jpg_buffer_func = nullptr;
	static inline SaveWebPBufferFunc save_webp_buffer_func = nullptr;
	static inline SaveDDSBufferFunc save_dds_buffer_func = nullptr;

	// External loader function pointers.

	static inline ImageMemLoadFunc _png_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _png_mem_unpacker_func = nullptr;
	static inline ImageMemLoadFunc _jpg_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _webp_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _tga_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _bmp_mem_loader_func = nullptr;
	static inline ScalableImageMemLoadFunc _svg_scalable_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _ktx_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _dds_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _exr_mem_loader_func = nullptr;

	// External VRAM compression function pointers.

	static void (*_image_compress_bc_func)(Image*, UsedChannels p_channels);
	static void (*_image_compress_bptc_func)(
		Image*, UsedChannels p_channels, BPTCFormat p_bptc_format);
	static void (*_image_compress_etc1_func)(Image*);
	static void (*_image_compress_etc2_func)(Image*, UsedChannels p_channels);
	static void (*_image_compress_astc_func)(
		Image*, UsedChannels p_channels, CompressProfile p_profile);

	static Error (*_image_compress_bptc_rd_func)(
		Image*, UsedChannels p_channels, BPTCFormat p_bptc_format);
	static Error (*_image_compress_bc_rd_func)(Image*, UsedChannels p_channels);

	// External VRAM decompression function pointers.

	static void (*_image_decompress_bc)(Image*);
	static void (*_image_decompress_bptc)(Image*);
	static void (*_image_decompress_etc1)(Image*);
	static void (*_image_decompress_etc2)(Image*);
	static void (*_image_decompress_astc)(Image*);

	// External packer function pointers.

	static Vector<uint8_t> (*webp_lossy_packer)(const Ref<Image>& p_image, float p_quality);
	static Vector<uint8_t> (*webp_lossless_packer)(const Ref<Image>& p_image);
	static Vector<uint8_t> (*png_packer)(const Ref<Image>& p_image);
	static Vector<uint8_t> (*basis_universal_packer)(const Ref<Image>& p_image,
		UsedChannels p_channels, const BasisUniversalPackerParams& p_basisu_params);

	static Ref<Image> (*webp_unpacker)(const Vector<uint8_t>& p_buffer);
	static Ref<Image> (*png_unpacker)(const Vector<uint8_t>& p_buffer);
	static Ref<Image> (*basis_universal_unpacker)(const Vector<uint8_t>& p_buffer);
	static Ref<Image> (*basis_universal_unpacker_ptr)(const uint8_t* p_data, int p_size);

protected:

#ifndef DISABLE_DEPRECATED
	Error _compress_bind_compat_115003(
		CompressMode p_mode, CompressSource p_source, ASTCFormat p_format);
	Error _compress_from_channels_bind_compat_115003(
		CompressMode p_mode, UsedChannels p_channels, ASTCFormat p_format);
	Vector<uint8_t> _save_exr_to_buffer_bind_compat_117800(bool p_grayscale = false) const;
	Error _save_exr_bind_compat_117800(const String& p_path, bool p_grayscale = false) const;

	static void _bind_compatibility_methods();
#endif

private:
	Format format = FORMAT_L8;
	Vector<uint8_t> data;
	int width = 0;
	int height = 0;
	bool mipmaps = false;

	void _copy_internals_from(const Image& p_image);

	_FORCE_INLINE_ Color _get_color_at_ofs(const uint8_t* p_ptr, uint32_t p_ofs) const;
	_FORCE_INLINE_ void _set_color_at_ofs(uint8_t* r_ptr, uint32_t p_ofs, const Color& p_color);

	_FORCE_INLINE_ void _get_mipmap_offset_and_size(int p_mipmap, int64_t& r_offset, int& r_width,
		int& r_height) const; // Get where the mipmap begins in data.

	static int64_t _get_dst_image_size(int p_width, int p_height, Format p_format, int& r_mipmaps,
		int p_mipmaps = -1, int* r_mm_width = nullptr, int* r_mm_height = nullptr);

	_FORCE_INLINE_ void _get_clipped_src_and_dest_rects(const Ref<Image>& p_src,
		const Rect2i& p_src_rect, const Point2i& p_dest, Rect2i& r_clipped_src_rect,
		Rect2i& r_clipped_dest_rect) const;

	_FORCE_INLINE_ void _put_pixelb(
		int p_x, int p_y, uint32_t p_pixel_size, uint8_t* p_data, const uint8_t* p_pixel);
	_FORCE_INLINE_ void _get_pixelb(
		int p_x, int p_y, uint32_t p_pixel_size, const uint8_t* p_data, uint8_t* p_pixel);

	_FORCE_INLINE_ void _repeat_pixel_over_subsequent_memory(
		uint8_t* p_pixel, int p_pixel_size, int p_count);

	Error _load_from_buffer(const Vector<uint8_t>& p_array, ImageMemLoadFunc p_loader);

	_FORCE_INLINE_ void _generate_mipmap_from_format(Image::Format p_format, const uint8_t* p_src,
		uint8_t* p_dst, uint32_t p_width, uint32_t p_height, bool p_renormalize = false);

	static void average_4_uint8(uint8_t& p_out, const uint8_t& p_a, const uint8_t& p_b,
		const uint8_t& p_c, const uint8_t& p_d);
	static void average_4_float(
		float& p_out, const float& p_a, const float& p_b, const float& p_c, const float& p_d);
	static void average_4_half(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);
	static void average_4_rgbe9995(uint32_t& p_out, const uint32_t& p_a, const uint32_t& p_b,
		const uint32_t& p_c, const uint32_t& p_d);
	static void average_4_uint16(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);
	static void average_4_rgba4444(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);
	static void average_4_rgb565(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);

	static void renormalize_uint8(uint8_t* p_rgb);
	static void renormalize_float(float* p_rgb);
	static void renormalize_half(uint16_t* p_rgb);
	static void renormalize_uint16(uint16_t* p_rgb);

public:
	int get_width() const;
	int get_height() const;
	Size2i get_size() const;
	bool has_mipmaps() const;
	int get_mipmap_count() const;

	// Convert the image to another format, conversion only to raw byte format.
	void convert(Format p_new_format);

	Format get_format() const;

	// Get where the mipmap begins in data.
	int64_t get_mipmap_offset(int p_mipmap) const;
	void get_mipmap_offset_and_size(int p_mipmap, int64_t& r_ofs, int64_t& r_size) const;
	void get_mipmap_offset_size_and_dimensions(
		int p_mipmap, int64_t& r_ofs, int64_t& r_size, int& p_w, int& p_h) const;

	static Image3DValidateError validate_3d_image(Format p_format, int p_width, int p_height,
		int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_images);
	static String get_3d_image_validation_error_text(Image3DValidateError p_error);

	// Resize the image, using the preferred interpolation method.
	void resize_to_po2(bool p_square = false, Interpolation p_interpolation = INTERPOLATE_BILINEAR);
	void resize(int p_width, int p_height, Interpolation p_interpolation = INTERPOLATE_BILINEAR);
	void shrink_x2();
	bool is_size_po2() const;

	// Crop the image to a specific size, if larger, then the image is filled by black.
	void crop_from_point(int p_x, int p_y, int p_width, int p_height);
	void crop(int p_width, int p_height);

	void rotate_90(ClockDirection p_direction);
	void rotate_180();

	void flip_x();
	void flip_y();

	// Generate a mipmap chain of an image (creates an image 1/4 the size, with averaging of 4->1).
	Error generate_mipmaps(bool p_renormalize = false);

	void clear_mipmaps();
	void normalize();

	// Creates new internal image data of a given size and format. Current image will be lost.
	void initialize_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format);
	void initialize_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format,
		const Vector<uint8_t>& p_data);
	void initialize_data(const char** p_xpm);

	// Returns true when the image is empty (0,0) in size.
	bool is_empty() const;

	const Vector<uint8_t>& get_data() const _LIFETIME_BOUND_;

	Error load(const String& p_path);
	static Ref<Image> load_from_file(const String& p_path);
	Error save_png(const String& p_path) const;
	Error save_jpg(const String& p_path, float p_quality = 0.75) const;
	Error save_dds(const String& p_path) const;
	Vector<uint8_t> save_png_to_buffer() const;
	Vector<uint8_t> save_jpg_to_buffer(float p_quality = 0.75) const;
	Vector<uint8_t> save_exr_to_buffer(
		bool p_grayscale = false, bool p_color_image = false, float p_max_value = -1.0f) const;
	Vector<uint8_t> save_dds_to_buffer() const;
	Error save_exr(const String& p_path, bool p_grayscale = false, bool p_color_image = false,
		float p_max_value = -1.0f) const;
	Error save_webp(
		const String& p_path, const bool p_lossy = false, const float p_quality = 0.75f) const;
	Vector<uint8_t> save_webp_to_buffer(
		const bool p_lossy = false, const float p_quality = 0.75f) const;

	static Ref<Image> create_empty(int p_width, int p_height, bool p_use_mipmaps, Format p_format);
	static Ref<Image> create_from_data(int p_width, int p_height, bool p_use_mipmaps,
		Format p_format, const Vector<uint8_t>& p_data);
	void set_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format,
		const Vector<uint8_t>& p_data);

	Image() = default; // Create an empty image.
	Image(int p_width, int p_height, bool p_use_mipmaps,
		Format p_format); // Create an empty image of a specific size and format.
	Image(int p_width, int p_height, bool p_mipmaps, Format p_format,
		const Vector<uint8_t>&
			p_data); // Import an image of a specific size and format from a byte vector.
	Image(
		const uint8_t* p_mem_png_jpg, int p_len = -1); // Import either a png or jpg from a pointer.
	Image(const char** p_xpm);						   // Import an XPM image.

	AlphaMode detect_alpha() const;
	bool is_invisible() const;

	static int get_format_pixel_size(Format p_format);
	static uint64_t get_format_pixels_shifted(Format p_format, uint64_t p_pixels);
	static int get_format_block_size(Format p_format);
	static void get_format_min_pixel_size(Format p_format, int& r_w, int& r_h);

	static int64_t get_image_data_size(
		int p_width, int p_height, Format p_format, bool p_mipmaps = false);
	static int get_image_required_mipmaps(int p_width, int p_height, Format p_format);
	static Size2i get_image_mipmap_size(int p_width, int p_height, Format p_format, int p_mipmap);
	static int64_t get_image_mipmap_offset(
		int p_width, int p_height, Format p_format, int p_mipmap);
	static int64_t get_image_mipmap_offset_and_dimensions(
		int p_width, int p_height, Format p_format, int p_mipmap, int& r_w, int& r_h);

	Error compress(CompressMode p_mode, CompressSource p_source = COMPRESS_SOURCE_GENERIC,
		CompressProfile p_profile = COMPRESS_PROFILE_AUTOMATIC);
	Error compress_from_channels(CompressMode p_mode, UsedChannels p_channels,
		CompressProfile p_profile = COMPRESS_PROFILE_AUTOMATIC);
	Error _compress_from_channels(CompressMode p_mode, UsedChannels p_channels,
		CompressProfile p_profile, BPTCFormat p_bptc_format);
	Error decompress();
	bool is_compressed() const;
	static bool is_format_compressed(Format p_format);

	static bool can_decompress(const String& p_format_tag);

	void fix_alpha_edges();
	void premultiply_alpha();
	void srgb_to_linear();
	void linear_to_srgb();
	void normal_map_to_xy();
	Ref<Image> rgbe_to_srgb();
	Ref<Image> get_image_from_mipmap(int p_mipmap) const;
	void bump_map_to_normal_map(float p_bump_scale = 1.0);

	bool detect_signed(bool p_include_mips = true) const;

	void blit_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest);
	void blit_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask, const Rect2i& p_src_rect,
		const Point2i& p_dest);
	void blend_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest);
	void blend_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask,
		const Rect2i& p_src_rect, const Point2i& p_dest);
	void fill(const Color& p_color);
	void fill_rect(const Rect2i& p_rect, const Color& p_color);

	Rect2i get_used_rect() const;
	Ref<Image> get_region(const Rect2i& p_area) const;

	static String get_format_name(Format p_format);
	static uint32_t get_format_component_mask(Format p_format);

	Error load_png_from_buffer(const Vector<uint8_t>& p_array);
	Error load_jpg_from_buffer(const Vector<uint8_t>& p_array);
	Error load_webp_from_buffer(const Vector<uint8_t>& p_array);
	Error load_tga_from_buffer(const Vector<uint8_t>& p_array);
	Error load_bmp_from_buffer(const Vector<uint8_t>& p_array);
	Error load_ktx_from_buffer(const Vector<uint8_t>& p_array);
	Error load_dds_from_buffer(const Vector<uint8_t>& p_array);
	Error load_exr_from_buffer(const Vector<uint8_t>& p_array);

	Error load_svg_from_buffer(const Vector<uint8_t>& p_array, float p_scale = 1.0);
	Error load_svg_from_string(const String& p_svg_str, float p_scale = 1.0);

	void convert_rg_to_ra_rgba8();
	void convert_ra_rgba8_to_rg();
	void convert_rgba8_to_bgra8();

	UsedChannels detect_used_channels(CompressSource p_source = COMPRESS_SOURCE_GENERIC) const;
	void optimize_channels();

	Color get_pixelv(const Point2i& p_point) const;
	Color get_pixel(int p_x, int p_y) const;
	void set_pixelv(const Point2i& p_point, const Color& p_color);
	void set_pixel(int p_x, int p_y, const Color& p_color);

	const uint8_t* ptr() const;
	uint8_t* ptrw();
	int64_t get_data_size() const;

	void adjust_bcs(float p_brightness, float p_contrast, float p_saturation);

	void set_as_black();

	void copy_internals_from(const Ref<Image>& p_image);
};
#include "core/string/ustring.h"
#include "core/templates/list.h"

class ImageLoader;

class ImageFormatLoader final : public RefCounted
{
	friend class ImageLoader;

public:
	enum LoaderFlags
	{
		FLAG_NONE = 0,
		FLAG_FORCE_LINEAR = 1,
		FLAG_CONVERT_COLORS = 2,
	};
	using LoadImageFn = Error (*)(Ref<Image>, Ref<FileAccess>, uint32_t, float);

private:
	LoadImageFn load_fn = nullptr;
	const char* const* extensions = nullptr;

public:
	ImageFormatLoader() = default;
	ImageFormatLoader(LoadImageFn p_load_fn, const char* const* p_extensions) :
		load_fn(p_load_fn), extensions(p_extensions) {}

	Error load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess,
		uint32_t p_flags = FLAG_NONE, float p_scale = 1.0f)
	{
		return load_fn ? load_fn(p_image, p_fileaccess, p_flags, p_scale) : ERR_UNAVAILABLE;
	}
	void get_recognized_extensions(List<String>* p_extensions) const;
	bool recognize(const String& p_extension) const;
};

class ImageLoader
{
public:
	static inline Vector<Ref<ImageFormatLoader>> loader;
	static HashMap<Color, Color> svg_forced_color_map;

	static void _svg_replace_color_property(
		const HashMap<Color, Color>& p_color_map, const String& p_prefix, String& r_string);
	static Ref<Image> load_mem_svg(const uint8_t* p_svg, int p_size, float p_scale);
	static void set_forced_color_map(const HashMap<Color, Color>& p_color_map);
	static Error create_image_from_utf8_buffer(
		Ref<Image> p_image, const uint8_t* p_buffer, int p_buffer_size, float p_scale, bool p_upsample);
	static Error create_image_from_utf8_buffer(
		Ref<Image> p_image, const PackedByteArray& p_buffer, float p_scale, bool p_upsample);
	static Error create_image_from_string(Ref<Image> p_image, String p_string, float p_scale,
		bool p_upsample, const HashMap<Color, Color>& p_color_map = HashMap<Color, Color>());

	static Error load_image(const String& p_file, Ref<Image> p_image,
		Ref<FileAccess> p_custom = Ref<FileAccess>(),
		uint32_t p_flags = ImageFormatLoader::FLAG_NONE,
		float p_scale = 1.0);
	static void get_recognized_extensions(List<String>* p_extensions);
	static Ref<ImageFormatLoader> recognize(const String& p_extension);

	static void add_image_format_loader(Ref<ImageFormatLoader> p_loader);
	static void remove_image_format_loader(Ref<ImageFormatLoader> p_loader);

	static void cleanup();
};

class ResourceFormatSaver : public RefCounted
{
public:
	using SaveFn = Error (*)(const Ref<Resource>&, const String&, uint32_t);
	using RecognizeFn = bool (*)(const Ref<Resource>&);
	using GetExtensionsFn = void (*)(const Ref<Resource>&, List<String>*);
	using SetUidFn = Error (*)(const String&, ResourceUID::ID);

private:
	SaveFn save_fn = nullptr;
	RecognizeFn recognize_fn = nullptr;
	GetExtensionsFn get_exts_fn = nullptr;
	SetUidFn set_uid_fn = nullptr;

public:
	ResourceFormatSaver() = default;
	ResourceFormatSaver(SaveFn p_save_fn, RecognizeFn p_recognize_fn = nullptr,
		GetExtensionsFn p_exts_fn = nullptr, SetUidFn p_set_uid_fn = nullptr) :
		save_fn(p_save_fn),
		recognize_fn(p_recognize_fn),
		get_exts_fn(p_exts_fn),
		set_uid_fn(p_set_uid_fn)
	{
	}

	virtual Error save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0);
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid);
	virtual bool recognize(const Ref<Resource>& p_resource) const;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const;
	virtual bool recognize_path(const Ref<Resource>& p_resource, const String& p_path) const;

	virtual ~ResourceFormatSaver() {}
};

typedef void (*ResourceSavedCallback)(Ref<Resource> p_resource, const String& p_path);
typedef ResourceUID::ID (*ResourceSaverGetResourceIDForPath)(const String& p_path, bool p_generate);

class ResourceSaver
{
	enum
	{
		MAX_SAVERS = 64
	};

	static Ref<ResourceFormatSaver> saver[MAX_SAVERS];
	static int saver_count;
	static bool timestamp_on_save;
	static ResourceSavedCallback save_callback;
	static ResourceSaverGetResourceIDForPath save_get_id_for_path;

	static Ref<ResourceFormatSaver> _find_custom_resource_format_saver(const String& p_path);

public:
	enum SaverFlags
	{
		FLAG_NONE = 0,
		FLAG_RELATIVE_PATHS = 1,
		FLAG_BUNDLE_RESOURCES = 2,
		FLAG_CHANGE_PATH = 4,
		FLAG_OMIT_EDITOR_PROPERTIES = 8,
		FLAG_SAVE_BIG_ENDIAN = 16,
		FLAG_COMPRESS = 32,
		FLAG_REPLACE_SUBRESOURCE_PATHS = 64,
	};

	static Error save(
		Resource* rp_resource, const String& p_path = "", uint32_t p_flags = (uint32_t)FLAG_NONE);
	static void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions);
	static void add_resource_format_saver(
		Ref<ResourceFormatSaver> p_format_saver, bool p_at_front = false);
	static void remove_resource_format_saver(Ref<ResourceFormatSaver> p_format_saver);

	static Error set_uid(const String& p_path, ResourceUID::ID p_uid);

	static void set_timestamp_on_save(bool p_timestamp) { timestamp_on_save = p_timestamp; }

	static bool get_timestamp_on_save() { return timestamp_on_save; }

	static ResourceUID::ID get_resource_id_for_path(const String& p_path, bool p_generate = false);

	static void set_save_callback(ResourceSavedCallback p_callback);
	static void set_get_resource_id_for_path(ResourceSaverGetResourceIDForPath p_callback);

	static bool add_custom_resource_format_saver(const String& p_script_path);
	static void add_custom_savers();
	static void remove_custom_savers();
};

class ResourceLoaderBinary
{
	bool translation_remapped = false;
	String local_path;
	String res_path;
	String type;
	Ref<Resource> resource;
	uint32_t ver_format = 0;

	Ref<FileAccess> f;

	uint64_t importmd_ofs = 0;

	ResourceUID::ID uid = ResourceUID::INVALID_ID;

	Vector<char> str_buf;
	List<Ref<Resource>> resource_cache;

	Vector<String> string_map;

	String _get_string();

	struct ExtResource
	{
		String path;
		String type;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
		Ref<ResourceLoader::LoadToken> load_token;
	};

	bool using_named_scene_ids = false;
	bool using_uids = false;
	String script_class;
	bool use_sub_threads = false;
	float* progress = nullptr;
	Vector<ExtResource> external_resources;

	struct IntResource
	{
		String path;
		uint64_t offset;
	};

	Vector<IntResource> internal_resources;
	HashMap<String, Ref<Resource>> internal_index_cache;

	String get_unicode_string();
	void _advance_padding(uint32_t p_len);

	HashMap<String, String> remaps;
	Error error = OK;

	ResourceFormatLoader::CacheMode cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
	ResourceFormatLoader::CacheMode cache_mode_for_external =
		ResourceFormatLoader::CACHE_MODE_REUSE;

	friend class ResourceFormatLoaderBinary;

	HashMap<String, Ref<Resource>> dependency_cache;

public:
	Ref<Resource> get_resource();
	Error load();
	void set_translation_remapped(bool p_remapped);

	void set_remaps(const HashMap<String, String>& p_remaps) { remaps = p_remaps; }

	void open(Ref<FileAccess> p_file, bool p_no_resources = false, bool p_keep_uuid_paths = false);
	String recognize(Ref<FileAccess> p_file);
	String recognize_script_class(Ref<FileAccess> p_file);
	void get_dependencies(Ref<FileAccess> p_file, List<String>* p_dependencies, bool p_add_types);
	void get_classes_used(Ref<FileAccess> p_file, HashSet<String>* p_classes);
};

class ResourceFormatLoaderBinary : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
	virtual String get_resource_script_class(const String& p_path) const override;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override;
	virtual bool has_custom_uid_support() const override;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override;
	virtual Error rename_dependencies(
		const String& p_path, const HashMap<String, String>& p_map) override;
};

struct ResourceNonPersistentKey
{ // for resource properties generated on the fly
	Ref<Resource> base;
	String property;

	bool operator<(const ResourceNonPersistentKey& p_key) const
	{
		return base == p_key.base ? property < p_key.property : base < p_key.base;
	}
};

class ResourceFormatSaverBinaryInstance
{
	String local_path;
	String path;

	bool relative_paths;
	bool bundle_resources;
	bool skip_editor;
	bool big_endian;
	bool takeover_paths;
	String magic;
	HashSet<Ref<Resource>> resource_set;

	using NonPersistentKey = ResourceNonPersistentKey;

	HashMap<String, int> string_map;
	Vector<String> strings;

	HashMap<Ref<Resource>, int> external_resources;
	List<Ref<Resource>> saved_resources;

	struct Property
	{
		int name_idx;
	};

	struct ResourceData
	{
		String type;
		List<Property> properties;
	};

	static void _pad_buffer(Ref<FileAccess> r_file, int p_bytes);
	static void save_unicode_string(
		Ref<FileAccess> r_file, const String& p_string, bool p_bit_on_len = false);
	int get_string_index(const String& p_string);

public:
	enum
	{
		FORMAT_FLAG_NAMED_SCENE_IDS = 1,
		FORMAT_FLAG_UIDS = 2,
		FORMAT_FLAG_REAL_T_IS_DOUBLE = 4,
		FORMAT_FLAG_HAS_SCRIPT_CLASS = 8,

		// Amount of reserved 32-bit fields in resource header
		RESERVED_FIELDS = 11
	};

	Error save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags = 0);
	Error set_uid(const String& p_path, ResourceUID::ID p_uid);
};

class ResourceFormatSaverBinary : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceFormatSaverBinary() = default;
};

#define META_PROPERTY_MISSING_RESOURCES "metadata/_missing_resources"
#define META_MISSING_RESOURCES "_missing_resources"

class MissingResource : public Resource
{
	String original_class;
	bool recording_properties = false;

protected:

public:
	void set_original_class(const String& p_class);
	String get_original_class() const;

	void set_recording_properties(bool p_enable);
	bool is_recording_properties() const;

	virtual String get_save_class() const;
};

namespace PNGDriverCommon {

// Attempt to load png from buffer (p_source, p_size) into p_image
Error png_to_image(const uint8_t *p_source, size_t p_size, bool p_force_linear, Ref<Image> p_image);

// Append p_image, as a png, to p_buffer.
// Contents of p_buffer is unspecified if error returned.
Error image_to_png(const Ref<Image> &p_image, Vector<uint8_t> &p_buffer);
} // namespace PNGDriverCommon

class ResourceImporter;
class ResourceFormatImporter;

typedef Ref<Resource> (*ResourceFormatImporterLoadOnStartup)(
	const String& p_path, Error* r_error, bool p_use_sub_threads, float* r_progress,
	ResourceFormatLoader::CacheMode p_cache_mode);

class ResourceFormatImporter final
{
public:
	using CacheMode = ResourceFormatLoader::CacheMode;

private:
	struct PathAndType
	{
		String path;
		String type;
		String importer;
		String group_file;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
	};

	static Error _get_path_and_type(const String& p_path, PathAndType& r_path_and_type, bool p_load,
		bool* r_valid = nullptr);


	struct Data
	{
		Vector<Ref<ResourceImporter>> importers;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE);
	static Ref<Resource> load_internal(const String& p_path, Error* r_error, bool p_use_sub_threads,
		float* r_progress, CacheMode p_cache_mode, bool p_silence_errors);
	static void get_recognized_extensions(List<String>* p_extensions);
	static void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions);
	static bool recognize_path(
		const String& p_path, const String& p_for_type = String());
	static bool handles_type(const String& p_type);
	static String get_resource_type(const String& p_path);
	static String get_resource_script_class(const String& p_path);
	static ResourceUID::ID get_resource_uid(const String& p_path);
	static bool has_custom_uid_support();
	static bool is_import_valid(const String& p_path);
	static void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);

	static bool is_imported(const String& p_path) { return recognize_path(p_path); }

	static String get_import_group_file(const String& p_path);
	static void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	static bool exists(const String& p_path);

	static void get_build_dependencies(const String& p_path, HashSet<String>* r_dependencies);

	static int get_import_order(const String& p_path);

	static Error get_import_order_threads_and_importer(
		const String& p_path, int& r_order, bool& r_can_threads, String& r_importer);

	static String get_internal_resource_path(const String& p_path);
	static void get_internal_resource_path_list(const String& p_path, List<String>* r_paths);

	static void add_importer(const Ref<ResourceImporter>& p_importer, bool p_first_priority = false);

	static void remove_importer(const Ref<ResourceImporter>& p_importer)
	{
		if (data) {
			data->importers.erase(p_importer);
		}
	}

	static Ref<ResourceImporter> get_importer_by_name(const String& p_name);
	static Ref<ResourceImporter> get_importer_by_file(const String& p_file);
	static void get_importers_for_file(const String& p_file, List<Ref<ResourceImporter>>* r_importers);
	static void get_importers(List<Ref<ResourceImporter>>* r_importers);

	static bool are_import_settings_valid(const String& p_path);
	static String get_import_settings_hash();

	static String get_import_base_path(const String& p_for_file);
	static Error get_resource_import_info(const String& p_path, String& r_type, ResourceUID::ID& r_uid,
		String& r_import_group_file);

	ResourceFormatImporter() = delete;
	ResourceFormatImporter(const ResourceFormatImporter&) = delete;
	ResourceFormatImporter& operator=(const ResourceFormatImporter&) = delete;
	~ResourceFormatImporter() = delete;
};


class ResourceImporter : public RefCounted
{
public:
	enum BuiltinFormat
	{
		FORMAT_CUSTOM = 0,
		FORMAT_MP3,
		FORMAT_OGG_VORBIS,
	};

protected:
	BuiltinFormat builtin_format = FORMAT_CUSTOM;

public:
	ResourceImporter(BuiltinFormat p_format = FORMAT_CUSTOM) : builtin_format(p_format) {}

	static inline ResourceFormatImporterLoadOnStartup load_on_startup = nullptr;

	virtual String get_importer_name() const
	{
		if (builtin_format == FORMAT_MP3) return "mp3";
		if (builtin_format == FORMAT_OGG_VORBIS) return "oggvorbisstr";
		return String();
	}
	virtual String get_visible_name() const
	{
		if (builtin_format == FORMAT_MP3) return "MP3";
		if (builtin_format == FORMAT_OGG_VORBIS) return "Ogg Vorbis";
		return String();
	}
	virtual void get_recognized_extensions(List<String>* p_extensions) const
	{
		if (builtin_format == FORMAT_MP3) {
#ifndef DR_MP3_ONLY_MP3
			p_extensions->push_back("mp1");
			p_extensions->push_back("mp2");
#endif
			p_extensions->push_back("mp3");
		} else if (builtin_format == FORMAT_OGG_VORBIS) {
			p_extensions->push_back("ogg");
		}
	}
	virtual String get_save_extension() const
	{
		if (builtin_format == FORMAT_MP3) return "mp3str";
		if (builtin_format == FORMAT_OGG_VORBIS) return "oggvorbisstr";
		return String();
	}
	virtual String get_resource_type() const
	{
		if (builtin_format == FORMAT_MP3) return "AudioStreamMP3";
		if (builtin_format == FORMAT_OGG_VORBIS) return "AudioStreamOggVorbis";
		return String();
	}

	virtual float get_priority() const { return 1.0; }

	virtual int get_import_order() const { return IMPORT_ORDER_DEFAULT; }

	virtual int get_format_version() const { return 0; }

	struct ImportOption
	{
		ImportOption() {}
	};

	enum ImportOrder
	{
		IMPORT_ORDER_DEFAULT = 0,
		IMPORT_ORDER_SCENE = 100,
	};

	virtual bool has_advanced_options() const { return false; }

	virtual void show_advanced_options(const String& p_path) {}

	virtual int get_preset_count() const { return 0; }

	virtual String get_preset_name(int p_idx) const { return String(); }

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const {}

	virtual String get_option_group_file() const { return String(); }

	virtual bool can_import_threaded() const { return builtin_format != FORMAT_CUSTOM; }

	virtual void import_threaded_begin() {}

	virtual void import_threaded_end() {}

	virtual String get_import_settings_string() const { return String(); }

	virtual void get_build_dependencies(
		const String& p_path, HashSet<String>* r_build_dependencies);
};

// uintr_t is only for pairing with real_t, and we only need it in here.
#ifdef REAL_T_IS_DOUBLE
typedef uint64_t uintr_t;
#else
typedef uint32_t uintr_t;
#endif

/**
 * Miscellaneous helpers for marshaling data types, and encoding
 * in an endian independent way
 */

union MarshallFloat
{
	uint32_t i; ///< int
	float f;	///< float
};

union MarshallDouble
{
	uint64_t l; ///< long long
	double d;	///< double
};

// Behaves like one of the above, depending on compilation setting.
union MarshallReal
{
	uintr_t i;
	real_t r;
};

static inline unsigned int encode_uint16(uint16_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 2; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint16_t);
}

static inline unsigned int encode_uint32(uint32_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 4; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint32_t);
}

static inline unsigned int encode_half(float p_float, uint8_t* p_arr)
{
	encode_uint16(Math::make_half_float(p_float), p_arr);

	return sizeof(uint16_t);
}

static inline unsigned int encode_float(float p_float, uint8_t* p_arr)
{
	MarshallFloat mf;
	mf.f = p_float;
	encode_uint32(mf.i, p_arr);

	return sizeof(uint32_t);
}

static inline unsigned int encode_uint64(uint64_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 8; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint64_t);
}

static inline unsigned int encode_double(double p_double, uint8_t* p_arr)
{
	MarshallDouble md;
	md.d = p_double;
	encode_uint64(md.l, p_arr);

	return sizeof(uint64_t);
}

static inline unsigned int encode_uintr(uintr_t p_uint, uint8_t* p_arr)
{
	for (size_t i = 0; i < sizeof(uintr_t); i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uintr_t);
}

static inline unsigned int encode_real(real_t p_real, uint8_t* p_arr)
{
	MarshallReal mr;
	mr.r = p_real;
	encode_uintr(mr.i, p_arr);

	return sizeof(uintr_t);
}

static inline int encode_cstring(const char* p_string, uint8_t* p_data)
{
	int len = 0;

	while (*p_string) {
		if (p_data) {
			*p_data = (uint8_t)*p_string;
			p_data++;
		}
		p_string++;
		len++;
	}

	if (p_data) {
		*p_data = 0;
	}
	return len + 1;
}

static inline uint16_t decode_uint16(const uint8_t* p_arr)
{
	uint16_t u = 0;

	for (int i = 0; i < 2; i++) {
		uint16_t b = *p_arr;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline uint32_t decode_uint32(const uint8_t* p_arr)
{
	uint32_t u = 0;

	for (int i = 0; i < 4; i++) {
		uint32_t b = *p_arr;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline float decode_half(const uint8_t* p_arr)
{
	return Math::half_to_float(decode_uint16(p_arr));
}

static inline float decode_float(const uint8_t* p_arr)
{
	MarshallFloat mf;
	mf.i = decode_uint32(p_arr);
	return mf.f;
}

static inline uint64_t decode_uint64(const uint8_t* p_arr)
{
	uint64_t u = 0;

	for (int i = 0; i < 8; i++) {
		uint64_t b = (*p_arr) & 0xFF;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline double decode_double(const uint8_t* p_arr)
{
	MarshallDouble md;
	md.l = decode_uint64(p_arr);
	return md.d;
}

Vector<float> vector3_to_float32_array(const Vector3* p_vecs, size_t p_count);

class ConfigFile : public RefCounted
{
	Error _internal_load(const String& p_path, Ref<FileAccess> p_file);
	Error _internal_save(Ref<FileAccess> p_file);

	Error _parse(const String& p_path);

public:
	bool has_section(const String& p_section) const;
	bool has_section_key(const String& p_section, const String& p_key) const;

	Vector<String> get_sections() const;
	Vector<String> get_section_keys(const String& p_section) const;

	void erase_section(const String& p_section);
	void erase_section_key(const String& p_section, const String& p_key);

	Error save(const String& p_path);
	Error load(const String& p_path);
	Error parse(const String& p_data);

	String encode_to_text() const; // used by exporter

	void clear();

	Error load_encrypted(const String& p_path, const Vector<uint8_t>& p_key);
	Error load_encrypted_pass(const String& p_path, const String& p_pass);

	Error save_encrypted(const String& p_path, const Vector<uint8_t>& p_key);
	Error save_encrypted_pass(const String& p_path, const String& p_pass);
};

// Property list file format (application/x-plist) parser, property list ASN-1 serialization.

class PListNode;

class PList : public RefCounted
{
	friend class PListNode;

public:
	enum PLNodeType
	{
		PL_NODE_TYPE_NIL,
		PL_NODE_TYPE_STRING,
		PL_NODE_TYPE_ARRAY,
		PL_NODE_TYPE_DICT,
		PL_NODE_TYPE_BOOLEAN,
		PL_NODE_TYPE_INTEGER,
		PL_NODE_TYPE_REAL,
		PL_NODE_TYPE_DATA,
		PL_NODE_TYPE_DATE,
	};

private:
	struct PListTrailer
	{
		uint8_t offset_size;
		uint8_t ref_size;
		uint64_t object_num;
		uint64_t root_offset_idx;
		uint64_t offset_table_start;
	};

	PListTrailer trailer;
	Ref<PListNode> root;

	uint64_t read_bplist_var_size_int(Ref<FileAccess> p_file, uint8_t p_size);
	Ref<PListNode> read_bplist_obj(Ref<FileAccess> p_file, uint64_t p_offset_idx);

public:
	PList();
	PList(const String& p_string);

	bool load_file(const String& p_filename);
	bool load_string(const String& p_string, String& r_err_out);

	Vector<uint8_t> save_asn1() const;
	String save_text() const;

	Ref<PListNode> get_root();
};

/*************************************************************************/

class PListNode : public RefCounted
{
	static int _asn1_size_len(uint8_t p_len_octets);

public:
	PList::PLNodeType data_type = PList::PLNodeType::PL_NODE_TYPE_NIL;

	CharString data_string;
	Vector<Ref<PListNode>> data_array;
	HashMap<String, Ref<PListNode>> data_dict;

	union
	{
		int64_t data_int;
		bool data_bool;
		double data_real;
	};

	PList::PLNodeType get_type() const;

	static Ref<PListNode> new_array();
	static Ref<PListNode> new_dict();
	static Ref<PListNode> new_string(const String& p_string);
	static Ref<PListNode> new_data(const String& p_string);
	static Ref<PListNode> new_date(const String& p_string);
	static Ref<PListNode> new_bool(bool p_bool);
	static Ref<PListNode> new_int(int64_t p_int);
	static Ref<PListNode> new_real(double p_real);

	bool push_subnode(const Ref<PListNode>& p_node, const String& p_key = "");

	size_t get_asn1_size(uint8_t p_len_octets) const;

	void store_asn1_size(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const;
	bool store_asn1(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const;
	void store_text(String& p_stream, uint8_t p_indent) const;
};

class JSON : public Resource
{
	enum TokenType
	{
		TK_CURLY_BRACKET_OPEN,
		TK_CURLY_BRACKET_CLOSE,
		TK_BRACKET_OPEN,
		TK_BRACKET_CLOSE,
		TK_IDENTIFIER,
		TK_STRING,
		TK_NUMBER,
		TK_COLON,
		TK_COMMA,
		TK_EOF,
		TK_MAX
	};

	enum Expecting
	{
		EXPECT_OBJECT,
		EXPECT_OBJECT_KEY,
		EXPECT_COLON,
		EXPECT_OBJECT_VALUE,
	};

	struct Token
	{
		TokenType type;
	};

	String text;
	String err_str;
	int err_line = 0;

	static const char* tk_name[];

	static void _add_indent(String& r_result, const String& p_indent, int p_size);
	static Error _get_token(const char32_t* p_str, int& r_index, int p_len, Token& r_token,
		int& r_line, String& r_err_str);

protected:

public:
	Error parse(const String& p_json_string, bool p_keep_text = false);
	String get_parsed_text() const;

	_FORCE_INLINE_ int get_error_line() const { return err_line; }

	_FORCE_INLINE_ String get_error_message() const { return err_str; }
};

/*
  Based on irrXML (see their zlib license). Added mainly for compatibility with their Collada
  loader.
*/

class XMLParser : public RefCounted
{
public:
	//! Enumeration of all supported source text file formats
	enum SourceFormat
	{
		SOURCE_ASCII,
		SOURCE_UTF8,
		SOURCE_UTF16_BE,
		SOURCE_UTF16_LE,
		SOURCE_UTF32_BE,
		SOURCE_UTF32_LE
	};

	enum NodeType
	{
		NODE_NONE,
		NODE_ELEMENT,
		NODE_ELEMENT_END,
		NODE_TEXT,
		NODE_COMMENT,
		NODE_CDATA,
		NODE_UNKNOWN
	};

private:
	char* data_copy = nullptr;
	const char* data = nullptr;
	const char* P = nullptr;
	uint64_t length = 0;
	uint64_t current_line = 0;
	String node_name;
	bool node_empty = false;
	NodeType node_type = NODE_NONE;
	uint64_t node_offset = 0;

	struct Attribute
	{
		String name;
		String value;
	};

	Vector<Attribute> attributes;

	bool _set_text(const char* p_start, const char* p_end);
	void _parse_closing_xml_element();
	void _ignore_definition();
	bool _parse_cdata();
	void _parse_comment();
	void _parse_opening_xml_element();
	void _parse_current_node();

	_FORCE_INLINE_ void next_char()
	{
		if (*P == '\n') {
			current_line++;
		}
		P++;
	}

public:
	Error read();
	NodeType get_node_type();
	String get_node_name() const;
	String get_node_data() const;
	uint64_t get_node_offset() const;
	int get_attribute_count() const;
	String get_attribute_name(int p_idx) const;
	String get_attribute_value(int p_idx) const;
	bool has_attribute(const String& p_name) const;
	String get_named_attribute_value(const String& p_name) const;
	String get_named_attribute_value_safe(
		const String& p_name) const; // do not print error if doesn't exist
	bool is_empty() const;
	int get_current_line() const;

	void skip_section();
	Error seek(uint64_t p_pos);

	Error open(const String& p_path);
	Error open_buffer(const Vector<uint8_t>& p_buffer);
	Error _open_buffer(const uint8_t* p_buffer, size_t p_size);

	void close();

	~XMLParser();
};

class StreamPeer : public RefCounted
{
protected:

	// bind helpers
	Error _put_data(const Vector<uint8_t>& p_data);
	bool big_endian = false;

public:
	virtual Error put_data(const uint8_t* p_data,
		int p_bytes) = 0; ///< put a whole chunk of data, blocking until it sent
	virtual Error put_partial_data(const uint8_t* p_data, int p_bytes,
		int& r_sent) = 0; ///< put as much data as possible, without blocking.

	virtual Error get_data(uint8_t* p_buffer,
		int p_bytes) = 0; ///< read p_bytes of data, if p_bytes > available, it will block
	virtual Error get_partial_data(uint8_t* p_buffer, int p_bytes,
		int& r_received) = 0; ///< read as much data as p_bytes into buffer, if less was read,
							  ///< return in r_received

	virtual int get_available_bytes() const = 0;

	/* helpers */
	void set_big_endian(bool p_big_endian);
	bool is_big_endian_enabled() const;

	void put_8(int8_t p_val);
	void put_u8(uint8_t p_val);
	void put_16(int16_t p_val);
	void put_u16(uint16_t p_val);
	void put_32(int32_t p_val);
	void put_u32(uint32_t p_val);
	void put_64(int64_t p_val);
	void put_u64(uint64_t p_val);
	void put_half(float p_val);
	void put_float(float p_val);
	void put_double(double p_val);
	void put_string(const String& p_string);
	void put_utf8_string(const String& p_string);

	uint8_t get_u8();
	int8_t get_8();
	uint16_t get_u16();
	int16_t get_16();
	uint32_t get_u32();
	int32_t get_32();
	uint64_t get_u64();
	int64_t get_64();
	float get_half();
	float get_float();
	double get_double();
	String get_string(int p_bytes = -1);
	String get_utf8_string(int p_bytes = -1);
};

class StreamPeerBuffer : public StreamPeer
{
	Vector<uint8_t> data;
	int pointer = 0;

protected:

public:
	Error put_data(const uint8_t* p_data, int p_bytes) override;
	Error put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent) override;

	Error get_data(uint8_t* p_buffer, int p_bytes) override;
	Error get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received) override;

	virtual int get_available_bytes() const override;

	void seek(int p_pos);
	int get_size() const;
	int get_position() const;
	void resize(int p_size);

	void set_data_array(const Vector<uint8_t>& p_data);
	const Vector<uint8_t>& get_data_array() const _LIFETIME_BOUND_;

	void clear();

	Ref<StreamPeerBuffer> duplicate() const;
};

class StreamPeerGZIP : public StreamPeer
{
private:
	void* ctx = nullptr; // Will hold our z_stream instance.
	bool compressing = true;

	RingBuffer<uint8_t> rb;
	Vector<uint8_t> buffer;

	Error _process(uint8_t* p_dst, int p_dst_size, const uint8_t* p_src, int p_src_size,
		int& r_consumed, int& r_out, bool p_close = false);
	void _close();
	Error _start(bool p_compress, bool p_is_deflate, int p_buffer_size = 65535);

protected:

public:
	Error start_compression(bool p_is_deflate, int p_buffer_size = 65535);
	Error start_decompression(bool p_is_deflate, int p_buffer_size = 65535);

	Error finish();
	void clear();

	virtual Error put_data(const uint8_t* p_data, int p_bytes) override;
	virtual Error put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent) override;

	virtual Error get_data(uint8_t* p_buffer, int p_bytes) override;
	virtual Error get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received) override;

	virtual int get_available_bytes() const override;

	~StreamPeerGZIP();
};

#ifndef WEB_ENABLED

class StreamPeerSocket;
#endif

class HTTPClient
#ifndef WEB_ENABLED
	final
#endif
	: public RefCounted
{
public:
	enum ResponseCode
	{
		// 1xx informational
		RESPONSE_CONTINUE = 100,
		RESPONSE_SWITCHING_PROTOCOLS = 101,
		RESPONSE_PROCESSING = 102,

		// 2xx successful
		RESPONSE_OK = 200,
		RESPONSE_CREATED = 201,
		RESPONSE_ACCEPTED = 202,
		RESPONSE_NON_AUTHORITATIVE_INFORMATION = 203,
		RESPONSE_NO_CONTENT = 204,
		RESPONSE_RESET_CONTENT = 205,
		RESPONSE_PARTIAL_CONTENT = 206,
		RESPONSE_MULTI_STATUS = 207,
		RESPONSE_ALREADY_REPORTED = 208,
		RESPONSE_IM_USED = 226,

		// 3xx redirection
		RESPONSE_MULTIPLE_CHOICES = 300,
		RESPONSE_MOVED_PERMANENTLY = 301,
		RESPONSE_FOUND = 302,
		RESPONSE_SEE_OTHER = 303,
		RESPONSE_NOT_MODIFIED = 304,
		RESPONSE_USE_PROXY = 305,
		RESPONSE_SWITCH_PROXY = 306,
		RESPONSE_TEMPORARY_REDIRECT = 307,
		RESPONSE_PERMANENT_REDIRECT = 308,

		// 4xx client error
		RESPONSE_BAD_REQUEST = 400,
		RESPONSE_UNAUTHORIZED = 401,
		RESPONSE_PAYMENT_REQUIRED = 402,
		RESPONSE_FORBIDDEN = 403,
		RESPONSE_NOT_FOUND = 404,
		RESPONSE_METHOD_NOT_ALLOWED = 405,
		RESPONSE_NOT_ACCEPTABLE = 406,
		RESPONSE_PROXY_AUTHENTICATION_REQUIRED = 407,
		RESPONSE_REQUEST_TIMEOUT = 408,
		RESPONSE_CONFLICT = 409,
		RESPONSE_GONE = 410,
		RESPONSE_LENGTH_REQUIRED = 411,
		RESPONSE_PRECONDITION_FAILED = 412,
		RESPONSE_REQUEST_ENTITY_TOO_LARGE = 413,
		RESPONSE_REQUEST_URI_TOO_LONG = 414,
		RESPONSE_UNSUPPORTED_MEDIA_TYPE = 415,
		RESPONSE_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
		RESPONSE_EXPECTATION_FAILED = 417,
		RESPONSE_IM_A_TEAPOT = 418,
		RESPONSE_MISDIRECTED_REQUEST = 421,
		RESPONSE_UNPROCESSABLE_ENTITY = 422,
		RESPONSE_LOCKED = 423,
		RESPONSE_FAILED_DEPENDENCY = 424,
		RESPONSE_UPGRADE_REQUIRED = 426,
		RESPONSE_PRECONDITION_REQUIRED = 428,
		RESPONSE_TOO_MANY_REQUESTS = 429,
		RESPONSE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
		RESPONSE_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

		// 5xx server error
		RESPONSE_INTERNAL_SERVER_ERROR = 500,
		RESPONSE_NOT_IMPLEMENTED = 501,
		RESPONSE_BAD_GATEWAY = 502,
		RESPONSE_SERVICE_UNAVAILABLE = 503,
		RESPONSE_GATEWAY_TIMEOUT = 504,
		RESPONSE_HTTP_VERSION_NOT_SUPPORTED = 505,
		RESPONSE_VARIANT_ALSO_NEGOTIATES = 506,
		RESPONSE_INSUFFICIENT_STORAGE = 507,
		RESPONSE_LOOP_DETECTED = 508,
		RESPONSE_NOT_EXTENDED = 510,
		RESPONSE_NETWORK_AUTH_REQUIRED = 511,
	};

	enum Method
	{
		METHOD_GET,
		METHOD_HEAD,
		METHOD_POST,
		METHOD_PUT,
		METHOD_DELETE,
		METHOD_OPTIONS,
		METHOD_TRACE,
		METHOD_CONNECT,
		METHOD_PATCH,
		METHOD_MAX
	};

	enum Status
	{
		STATUS_DISCONNECTED,
		STATUS_RESOLVING, // Resolving hostname (if passed a hostname)
		STATUS_CANT_RESOLVE,
		STATUS_CONNECTING, // Connecting to IP
		STATUS_CANT_CONNECT,
		STATUS_CONNECTED,  // Connected, requests can be made
		STATUS_REQUESTING, // Request in progress
		STATUS_BODY,	   // Request resulted in body, which must be read
		STATUS_CONNECTION_ERROR,
		STATUS_TLS_HANDSHAKE_ERROR,
	};

protected:
	static const char* _methods[METHOD_MAX];
	static const int HOST_MIN_LEN = 4;

	enum Port
	{
		PORT_HTTP = 80,
		PORT_HTTPS = 443,
	};

	Vector<String> _get_response_headers();
	Error _request_raw(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const Vector<uint8_t>& p_body);
	Error _request(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const String& p_body = String());

#ifdef WEB_ENABLED
	static HTTPClient* (*_create)(bool p_notify_postinitialize);
#else
private:
	Status status = STATUS_DISCONNECTED;
	IP::ResolverID resolving = IP::RESOLVER_INVALID_ID;
	int conn_port = -1; // Server to make requests to.
	String conn_host;
	int server_port = -1; // Server to connect to (might be a proxy server).
	String server_host;
	int http_proxy_port = -1; // Proxy server for http requests.
	String http_proxy_host;
	int https_proxy_port = -1; // Proxy server for https requests.
	String https_proxy_host;
	bool blocking = false;
	bool handshaking = false;
	bool head_request = false;
	Ref<TLSOptions> tls_options;

	Vector<uint8_t> response_str;

	bool chunked = false;
	Vector<uint8_t> chunk;
	int chunk_left = 0;
	bool chunk_trailer_part = false;
	int64_t body_size = -1;
	int64_t body_left = 0;
	bool read_until_eof = false;

	Ref<StreamPeerBuffer> request_buffer;
	Ref<StreamPeerSocket> tcp_connection;
	Ref<StreamPeer> connection;
	Ref<HTTPClient> proxy_client; // Negotiate with proxy server.

	int response_num = 0;
	Vector<String> response_headers;
	// 64 KiB by default (favors fast download speeds at the cost of memory usage).
	int read_chunk_size = 65536;

	Error _get_http_data(uint8_t* p_buffer, int p_bytes, int& r_received);
#endif

public:
	static Ref<HTTPClient> create(bool p_notify_postinitialize = true);

	Error verify_headers(const Vector<String>& p_headers);

#ifdef WEB_ENABLED
	virtual Error request(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const uint8_t* p_body, int p_body_size) = 0;
	virtual Error connect_to_host(const String& p_host, int p_port = -1,
		Ref<TLSOptions> p_tls_options = Ref<TLSOptions>()) = 0;

	virtual void set_connection(const Ref<StreamPeer>& p_connection) = 0;
	virtual Ref<StreamPeer> get_connection() const = 0;

	virtual void close() = 0;

	virtual Status get_status() const = 0;

	virtual bool has_response() const = 0;
	virtual bool is_response_chunked() const = 0;
	virtual int get_response_code() const = 0;
	virtual Error get_response_headers(List<String>* r_response) = 0;
	virtual int64_t get_response_body_length() const = 0;

	virtual Vector<uint8_t> read_response_body_chunk() = 0;

	virtual void set_blocking_mode(bool p_enable) = 0;
	virtual bool is_blocking_mode_enabled() const = 0;

	virtual void set_read_chunk_size(int p_size) = 0;
	virtual int get_read_chunk_size() const = 0;

	virtual Error poll() = 0;

	// Use empty string or -1 to unset
	virtual void set_http_proxy(const String& p_host, int p_port);
	virtual void set_https_proxy(const String& p_host, int p_port);

	virtual ~HTTPClient() {}
#else
	Error request(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const uint8_t* p_body, int p_body_size);
	Error connect_to_host(const String& p_host, int p_port = -1,
		Ref<TLSOptions> p_tls_options = Ref<TLSOptions>());

	void set_connection(const Ref<StreamPeer>& p_connection);
	Ref<StreamPeer> get_connection() const;

	void close();

	Status get_status() const;

	bool has_response() const;
	bool is_response_chunked() const;
	int get_response_code() const;
	Error get_response_headers(List<String>* r_response);
	int64_t get_response_body_length() const;

	Vector<uint8_t> read_response_body_chunk();

	void set_blocking_mode(bool p_enable);
	bool is_blocking_mode_enabled() const;

	void set_read_chunk_size(int p_size);
	int get_read_chunk_size() const;

	Error poll();

	// Use empty string or -1 to unset
	void set_http_proxy(const String& p_host, int p_port);
	void set_https_proxy(const String& p_host, int p_port);

	HTTPClient();
	virtual ~HTTPClient();
#endif
};

class StreamPeerTLS : public StreamPeer
{
private:
	MbedTLSState* data = nullptr;

public:
	enum Status
	{
		STATUS_DISCONNECTED,
		STATUS_HANDSHAKING,
		STATUS_CONNECTED,
		STATUS_ERROR,
		STATUS_ERROR_HOSTNAME_MISMATCH
	};

	void poll();
	Error accept_stream(Ref<StreamPeer> p_base, Ref<TLSOptions> p_options);
	Error connect_to_stream(Ref<StreamPeer> p_base, const String& p_common_name,
		Ref<TLSOptions> p_options = Ref<TLSOptions>());
	Status get_status() const;
	Ref<StreamPeer> get_stream() const;

	void disconnect_from_stream();

	virtual Error put_data(const uint8_t* p_data, int p_bytes) override;
	virtual Error put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent) override;

	virtual Error get_data(uint8_t* p_buffer, int p_bytes) override;
	virtual Error get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received) override;

	virtual int get_available_bytes() const override;

	static StreamPeerTLS* create(bool p_notify_postinitialize = true);
	static bool is_available();

	StreamPeerTLS();
	~StreamPeerTLS();
};

class PacketPeer : public RefCounted
{
	Error _put_packet(const Vector<uint8_t>& p_buffer);
	Vector<uint8_t> _get_packet();
	Error _get_packet_error() const;

	mutable Error last_get_error = OK;

	int encode_buffer_max_size = 8 * 1024 * 1024;
	Vector<uint8_t> encode_buffer;

public:
	virtual int get_available_packet_count() const = 0;
	virtual Error get_packet(
		const uint8_t** r_buffer, int& r_buffer_size) = 0; ///< buffer is GONE after next get_packet
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) = 0;

	virtual int get_max_packet_size() const = 0;

	/* helpers / binders */

	virtual Error get_packet_buffer(Vector<uint8_t>& r_buffer);
	virtual Error put_packet_buffer(const Vector<uint8_t>& p_buffer);

	void set_encode_buffer_max_size(int p_max_size);
	int get_encode_buffer_max_size() const;
};

class PacketPeerStream : public PacketPeer
{
	// the way the buffers work sucks, will change later

	mutable Ref<StreamPeer> peer;
	mutable RingBuffer<uint8_t> ring_buffer;
	mutable Vector<uint8_t> input_buffer;
	mutable Vector<uint8_t> output_buffer;

	Error _poll_buffer() const;

public:
	virtual int get_available_packet_count() const override;
	virtual Error get_packet(const uint8_t** r_buffer, int& r_buffer_size) override;
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;

	virtual int get_max_packet_size() const override;

	void set_stream_peer(const Ref<StreamPeer>& p_peer);
	Ref<StreamPeer> get_stream_peer() const;
	void set_input_buffer_max_size(int p_max_size);
	int get_input_buffer_max_size() const;
	void set_output_buffer_max_size(int p_max_size);
	int get_output_buffer_max_size() const;
	PacketPeerStream();
};

class StreamPeerSocket final : public StreamPeer
{
public:
	enum Domain
	{
		DOMAIN_TCP,
		DOMAIN_UDS,
	};

	enum Status
	{
		STATUS_NONE,
		STATUS_CONNECTING,
		STATUS_CONNECTED,
		STATUS_ERROR,
	};

private:
	Domain domain = DOMAIN_TCP;
	Ref<NetSocket> _sock;
	uint64_t timeout = 0;
	Status status = STATUS_NONE;
	NetSocket::Address peer_address;
	String peer_path;

	Error write(const uint8_t* p_data, int p_bytes, int& r_sent, bool p_block);
	Error read(uint8_t* p_buffer, int p_bytes, int& r_received, bool p_block);

public:
	Domain get_domain() const { return domain; }

	void accept_socket(Ref<NetSocket> p_sock, const NetSocket::Address& p_addr);

	// TCP operations.
	Error bind(int p_port, const IPAddress& p_host = IPAddress("*"));
	Error connect_to_host(const IPAddress& p_host, int p_port);
	void set_no_delay(bool p_enabled);
	IPAddress get_connected_host() const;
	int get_connected_port() const;
	int get_local_port() const;

	// UDS operations.
	Error bind(const String& p_path);
	Error bind_path(const String& p_path) { return bind(p_path); }
	Error connect_to_path(const String& p_path);
	String get_connected_path() const;

	void disconnect_from_host();

	int get_available_bytes() const override;
	Status get_status() const;

	// Poll socket updating its state.
	Error poll();

	// Wait or check for writable, readable.
	Error wait(NetSocket::PollType p_type, int p_timeout = 0);

	// Read/Write from StreamPeer.
	Error put_data(const uint8_t* p_data, int p_bytes) override;
	Error put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent) override;
	Error get_data(uint8_t* p_buffer, int p_bytes) override;
	Error get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received) override;

	StreamPeerSocket(Domain p_domain = DOMAIN_TCP);
	~StreamPeerSocket() override;
};

class SocketServer final : public RefCounted
{
private:
	enum
	{
		MAX_PENDING_CONNECTIONS = 8,
	};

	StreamPeerSocket::Domain domain = StreamPeerSocket::DOMAIN_TCP;
	Ref<NetSocket> _sock;

	Error _listen(const NetSocket::Address& p_addr);

public:
	Error listen(uint16_t p_port, const IPAddress& p_bind_address = IPAddress("*"));
	Error listen_path(const String& p_path);
	Error listen(const String& p_path) { return listen_path(p_path); }

	int get_local_port() const;

	bool is_listening() const;
	bool is_connection_available() const;
	Ref<StreamPeerSocket> take_connection();
	Ref<StreamPeerSocket> take_socket_connection() { return take_connection(); }

	void stop(); // Stop listening

	SocketServer();
	~SocketServer() override;
};

//@ TODO, excellent candidate for THREAD_SAFE MACRO, should go through all these and add THREAD_SAFE
// where it applies
class DirAccess : public RefCounted
{
public:
	enum AccessType : int32_t
	{
		ACCESS_RESOURCES,
		ACCESS_USERDATA,
		ACCESS_FILESYSTEM,
		ACCESS_MAX
	};

	typedef Ref<DirAccess> (*CreateFunc)();

private:
	AccessType _access_type = ACCESS_FILESYSTEM;
	static CreateFunc create_func[ACCESS_MAX]; ///< set this to instance a filesystem object
	static Ref<DirAccess> _open(const String& p_path);

	Error _copy_dir(
		Ref<DirAccess>& p_target_da, const String& p_to, int p_chmod_flags, bool p_copy_links);
	Vector<String> _get_contents(bool p_directories);

	static inline thread_local Error last_dir_open_error = OK;
	bool include_navigational = false;
	bool include_hidden = false;

	bool _is_temp = false;
	bool _temp_keep_after_free = false;
	String _temp_path;
	void _delete_temp();

	static Ref<DirAccess> _create_temp(const String& p_prefix = "", bool p_keep = false);

protected:

	String _get_root_path() const;
	virtual String _get_root_string() const;

	AccessType get_access_type() const;
	virtual String fix_path(const String& p_path) const;

	template <typename T> static Ref<DirAccess> _create_builtin() { return memnew(T); }

public:
	virtual Error list_dir_begin() = 0; ///< This starts dir listing
	virtual String get_next() = 0;
	virtual bool current_is_dir() const = 0;
	virtual bool current_is_hidden() const = 0;

	virtual void list_dir_end() = 0; ///<

	virtual int get_drive_count() = 0;
	virtual String get_drive(int p_drive) = 0;

	virtual String get_drive_label(int p_drive) { return String(); }

	virtual int get_current_drive();
	virtual bool drives_are_shortcuts();

	virtual Error change_dir(
		String p_dir) = 0; ///< can be relative or absolute, return false on success
	virtual String get_current_dir(
		bool p_include_drive = true) const = 0; ///< return current dir location
	virtual Error make_dir(String p_dir) = 0;
	virtual Error make_dir_recursive(const String& p_dir);
	virtual Error erase_contents_recursive(); // super dangerous, use with care!

	virtual bool file_exists(String p_file) = 0;
	virtual bool dir_exists(String p_dir) = 0;

	virtual bool is_readable(String p_dir) { return true; }

	virtual bool is_writable(String p_dir) { return true; }

	static bool exists(const String& p_dir);
	virtual uint64_t get_space_left() = 0;

	Error copy_dir(
		const String& p_from, String p_to, int p_chmod_flags = -1, bool p_copy_links = false);
	virtual Error copy(const String& p_from, const String& p_to, int p_chmod_flags = -1);
	virtual Error rename(String p_from, String p_to) = 0;
	virtual Error remove(String p_name) = 0;

	virtual bool is_link(String p_file) = 0;
	virtual String read_link(String p_file) = 0;
	virtual Error create_link(String p_source, String p_target) = 0;

	// Meant for editor code when we want to quickly remove a file without custom
	// handling (e.g. removing a cache file).
	static void remove_file_or_error(const String& p_path)
	{
		Ref<DirAccess> da = create(ACCESS_FILESYSTEM);
	}

	virtual String get_filesystem_type() const = 0;
	static String get_full_path(const String& p_path, AccessType p_access);
	static Ref<DirAccess> create_for_path(const String& p_path);

	static Ref<DirAccess> create(AccessType p_access);
	static Error get_open_error();

	template <typename T> static void make_default(AccessType p_access)
	{
		create_func[p_access] = _create_builtin<T>;
	}

	static Ref<DirAccess> open(const String& p_path, Error* r_error = nullptr);
	static Ref<DirAccess> create_temp(
		const String& p_prefix = "", bool p_keep = false, Error* r_error = nullptr);

	static int _get_drive_count();
	static String _get_drive_name(int p_idx);
	static String _get_drive_label(int p_idx);

	static Error make_dir_absolute(const String& p_dir);
	static Error make_dir_recursive_absolute(const String& p_dir);
	static bool dir_exists_absolute(const String& p_dir);

	static Error copy_absolute(const String& p_from, const String& p_to, int p_chmod_flags = -1);
	static Error rename_absolute(const String& p_from, const String& p_to);
	static Error remove_absolute(const String& p_path);

	Vector<String> get_files();
	static Vector<String> get_files_at(const String& p_path);
	Vector<String> get_directories();
	static Vector<String> get_directories_at(const String& p_path);
	String _get_next();

	void set_include_navigational(bool p_enable);
	bool get_include_navigational() const;
	void set_include_hidden(bool p_enable);
	bool get_include_hidden() const;

	virtual bool is_case_sensitive(const String& p_path) const;

	virtual bool is_bundle(const String& p_file) const { return false; }

	virtual bool is_equivalent(const String& p_path_a, const String& p_path_b) const;

public:
	virtual ~DirAccess();
};

// FileAccessMemory

class FileAccessMemory final : public FileAccess
{
	uint8_t* data = nullptr;
	uint64_t length = 0;
	mutable uint64_t pos = 0;

	static Ref<FileAccess> create();

public:
	static void register_file(const String& p_name, const Vector<uint8_t>& p_data);
	static void cleanup();

	Error open_custom(const uint8_t* p_data, uint64_t p_len);					  ///< open a file
	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual void seek(uint64_t p_position) override;	///< seek to a given position
	virtual void seek_end(int64_t p_position) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;		///< get position in the file
	virtual uint64_t get_length() const override;		///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(
		uint8_t* p_dst, uint64_t p_length) const override; ///< get an array of bytes

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(
		const uint8_t* p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual void close() override {}
};

// FileAccessPatched

class FileAccessPatched final : public FileAccess
{
	Ref<FileAccess> old_file;
	mutable Vector<uint8_t> patched_file_data;
	mutable Ref<FileAccessMemory> patched_file;
	mutable Error last_error = OK;

	Error _apply_patch() const;
	bool _try_apply_patch() const;

protected:
	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual Error open_internal(const String& p_path, int p_mode_flags) override
	{
		return ERR_UNAVAILABLE;
	}

public:
	Error open_custom(const Ref<FileAccess>& p_old_file);

	virtual bool is_open() const override;

	virtual void seek(uint64_t p_position) override;
	virtual void seek_end(int64_t p_position = 0) override;

	virtual uint64_t get_position() const override;
	virtual uint64_t get_length() const override;
	virtual bool eof_reached() const override;
	virtual Error get_error() const override;

	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;
	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual void close() override;

	virtual bool file_exists(const String& p_name) override;
};

// FileAccessCompressed

class FileAccessCompressed final : public FileAccess
{
	Compression::Mode cmode = Compression::MODE_ZSTD;
	bool writing = false;
	uint64_t write_pos = 0;
	uint8_t* write_ptr = nullptr;
	uint64_t write_buffer_size = 0;
	uint64_t write_max = 0;
	uint32_t block_size = 0;
	mutable bool read_eof = false;
	mutable bool at_end = false;

	struct ReadBlock
	{
		uint32_t csize;
		uint64_t offset;
	};

	mutable Vector<uint8_t> comp_buffer;
	uint8_t* read_ptr = nullptr;
	mutable uint32_t read_block = 0;
	uint32_t read_block_count = 0;
	mutable uint32_t read_block_size = 0;
	mutable uint64_t read_pos = 0;
	Vector<ReadBlock> read_blocks;
	uint64_t read_total = 0;

	String magic = "GCMP";
	mutable Vector<uint8_t> buffer;
	Ref<FileAccess> f;

	void _close();

public:
	void configure(const String& p_magic, Compression::Mode p_mode = Compression::MODE_ZSTD,
		uint32_t p_block_size = 4096);

	Error open_after_magic(Ref<FileAccess> p_base);

	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String
	get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override;		///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;			///< get position in the file
	virtual uint64_t get_length() const override;			///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override;
	virtual uint64_t _get_access_time(const String& p_file) override;
	virtual int64_t _get_size(const String& p_file) override;
	virtual uint32_t _get_unix_permissions(const String& p_file) override;
	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override;

	virtual bool _get_hidden_attribute(const String& p_file) override;
	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override;
	virtual bool _get_read_only_attribute(const String& p_file) override;
	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override;

	virtual void close() override;

	virtual ~FileAccessCompressed();
};

// FileAccessEncrypted

#define ENCRYPTED_HEADER_MAGIC 0x43454447

class FileAccessEncrypted final : public FileAccess
{
public:
	enum Mode : int32_t
	{
		MODE_READ,
		MODE_WRITE_AES256,
		MODE_MAX
	};

private:
	Vector<uint8_t> iv;
	Vector<uint8_t> key;
	bool writing = false;
	Ref<FileAccess> file;
	uint64_t base = 0;
	uint64_t length = 0;
	Vector<uint8_t> data;
	mutable uint64_t pos = 0;
	mutable bool eofed = false;
	bool use_magic = true;

	void _close();

public:
	Error open_and_parse(Ref<FileAccess> p_base, const Vector<uint8_t>& p_key, Mode p_mode,
		bool p_with_magic = true, const Vector<uint8_t>& p_iv = Vector<uint8_t>());
	Error open_and_parse_password(Ref<FileAccess> p_base, const String& p_key, Mode p_mode);

	Vector<uint8_t> get_iv() const { return iv; }

	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String
	get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override;		///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;			///< get position in the file
	virtual uint64_t get_length() const override;			///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(
		const uint8_t* p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override;
	virtual uint64_t _get_access_time(const String& p_file) override;
	virtual int64_t _get_size(const String& p_file) override;
	virtual uint32_t _get_unix_permissions(const String& p_file) override;
	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override;

	virtual bool _get_hidden_attribute(const String& p_file) override;
	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override;
	virtual bool _get_read_only_attribute(const String& p_file) override;
	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override;

	virtual void close() override;

	~FileAccessEncrypted();
};

// PackedData / PackSource / FileAccessPack / DirAccessPack

// Godot's packed file magic header ("GDPC" in ASCII).
#define PACK_HEADER_MAGIC 0x43504447

#define PACK_FORMAT_VERSION_V2 2
#define PACK_FORMAT_VERSION_V3 3
#define PACK_FORMAT_VERSION_V4 4

// The current packed file format version number.
#define PACK_FORMAT_VERSION PACK_FORMAT_VERSION_V4

enum PackFlags
{
	PACK_DIR_ENCRYPTED = 1 << 0,
	PACK_REL_FILEBASE = 1 << 1,
	PACK_SPARSE_BUNDLE = 1 << 2,
};

enum PackFileFlags
{
	PACK_FILE_ENCRYPTED = 1 << 0,
	PACK_FILE_REMOVAL = 1 << 1,
	PACK_FILE_DELTA = 1 << 2,
};

class PackSource;

class PackedData final
{
	friend class FileAccessPack;
	friend class DirAccessPack;
	friend class PackSource;

public:
	struct PackedFile
	{
		String pack;
		uint64_t offset; // if offset is ZERO, the file was ERASED
		uint64_t size;
		uint8_t md5[16];
		PackSource* src = nullptr;
		bool encrypted;
		bool bundle;
		bool delta;
		String salt;
	};

private:
	struct PackedDir
	{
		PackedDir* parent = nullptr;
		String name;
		HashMap<String, PackedDir*> subdirs;
		HashSet<String> files;
	};

	struct PathMD5
	{
		uint64_t a = 0;
		uint64_t b = 0;

		bool operator==(const PathMD5& p_val) const { return (a == p_val.a) && (b == p_val.b); }

		static uint32_t hash(const PathMD5& p_val)
		{
			uint32_t h = hash_murmur3_one_32(p_val.a);
			return hash_fmix32(hash_murmur3_one_32(p_val.b, h));
		}

		PathMD5() {}

		explicit PathMD5(const Vector<uint8_t>& p_buf)
		{
			a = *((uint64_t*)&p_buf[0]);
			b = *((uint64_t*)&p_buf[8]);
		}
	};

public:
	struct Data
	{
		HashMap<PathMD5, PackedFile, PathMD5> files;
		HashMap<PathMD5, Vector<PackedFile>, PathMD5> delta_patches;
		Vector<PackSource*> sources;
		PackedDir* root = nullptr;
		bool disabled = false;
	};

	static inline Data* data = nullptr;

private:
	static void _free_packed_dirs(PackedDir* p_dir);
	static void _get_file_paths(
		PackedDir* p_dir, const String& p_parent_dir, HashSet<String>& r_paths);

	_FORCE_INLINE_ static PathMD5 _get_simplified_path(const String& p_path)
	{
		String simplified_path = p_path;
		if (simplified_path.begins_with("uid://")) {
			simplified_path = ResourceUID::uid_to_path(simplified_path);
		}
		simplified_path = simplified_path.simplify_path().trim_prefix("res://");
		return PathMD5(simplified_path.md5_buffer());
	}

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void add_pack_source(PackSource* p_source);
	static void add_path(const String& p_pkg_path, const String& p_path, uint64_t p_ofs,
		uint64_t p_size, const uint8_t* p_md5, PackSource* p_src, bool p_replace_files,
		bool p_encrypted = false, bool p_bundle = false, bool p_delta = false,
		const String& p_salt = String()); // for PackSource
	static void remove_path(const String& p_path);
	static uint8_t* get_file_hash(const String& p_path);
	static Vector<PackedFile> get_delta_patches(const String& p_path);
	static bool has_delta_patches(const String& p_path);
	static HashSet<String> get_file_paths();

	static void set_disabled(bool p_disabled)
	{
		if (data != nullptr) {
			data->disabled = p_disabled;
		}
	}

	_FORCE_INLINE_ static bool is_disabled() { return data != nullptr ? data->disabled : true; }

	static Error add_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());

	static void clear();

	_FORCE_INLINE_ static Ref<FileAccess> try_open_path(
		const String& p_path, const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
	_FORCE_INLINE_ static bool has_path(const String& p_path);

	_FORCE_INLINE_ static int64_t get_size(const String& p_path);

	_FORCE_INLINE_ static Ref<DirAccess> try_open_directory(const String& p_path);
	_FORCE_INLINE_ static bool has_directory(const String& p_path);

	PackedData() = delete;
	PackedData(const PackedData&) = delete;
	PackedData& operator=(const PackedData&) = delete;
	~PackedData() = delete;
};

class PackSource final
{
public:
	enum Type
	{
		SOURCE_PCK,
		SOURCE_DIRECTORY,
		SOURCE_ZIP,
	};

private:
	Type type = SOURCE_PCK;
	void add_directory(const String& p_path, bool p_replace_files);
	bool _try_open_pck(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key);

public:
	explicit PackSource(Type p_type = SOURCE_PCK) : type(p_type) {}
	bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
	Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
};

class FileAccessPack final : public FileAccess
{
	PackedData::PackedFile pf;

	String path;
	mutable uint64_t pos;
	mutable bool eof;
	uint64_t off;

	Ref<FileAccess> f;
	virtual Error open_internal(const String& p_path, int p_mode_flags) override;

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

public:
	virtual bool is_open() const override;

	virtual String get_path() const override { return path; }

	virtual String get_path_absolute() const override { return path; }

	virtual void seek(uint64_t p_position) override;
	virtual void seek_end(int64_t p_position = 0) override;
	virtual uint64_t get_position() const override;
	virtual uint64_t get_length() const override;

	virtual bool eof_reached() const override;

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual void set_big_endian(bool p_big_endian) override;

	virtual Error get_error() const override;

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;

	virtual bool file_exists(const String& p_name) override;

	virtual void close() override;

	FileAccessPack(const String& p_path, const PackedData::PackedFile& p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
};

int64_t PackedData::get_size(const String& p_path)
{
	if (data == nullptr) {
		return -1;
	}
	HashMap<PathMD5, PackedFile, PathMD5>::Iterator E =
		data->files.find(_get_simplified_path(p_path));
	if (!E) {
		return -1; // File not found.
	}
	if (E->value.offset == 0) {
		return -1; // File was erased.
	}
	return E->value.size;
}

Ref<FileAccess> PackedData::try_open_path(
	const String& p_path, const Vector<uint8_t>& p_decryption_key)
{
	if (data == nullptr) {
		return nullptr;
	}
	HashMap<PathMD5, PackedFile, PathMD5>::Iterator E =
		data->files.find(_get_simplified_path(p_path));
	if (!E) {
		return nullptr; // Not found.
	}

	return E->value.src->get_file(p_path, &E->value, p_decryption_key);
}

bool PackedData::has_path(const String& p_path)
{
	return data != nullptr && data->files.has(_get_simplified_path(p_path));
}

bool PackedData::has_directory(const String& p_path)
{
	if (data == nullptr) {
		return false;
	}
	Ref<DirAccess> da = try_open_directory(p_path);
	if (da.is_valid()) {
		return true;
	}
	else {
		return false;
	}
}

class DirAccessPack final : public DirAccess
{
	PackedData::PackedDir* current;

	List<String> list_dirs;
	List<String> list_files;
	bool cdir = false;

	PackedData::PackedDir* _find_dir(const String& p_dir);

public:
	virtual Error list_dir_begin() override;
	virtual String get_next() override;
	virtual bool current_is_dir() const override;
	virtual bool current_is_hidden() const override;
	virtual void list_dir_end() override;

	virtual int get_drive_count() override;
	virtual String get_drive(int p_drive) override;

	virtual Error change_dir(String p_dir) override;
	virtual String get_current_dir(bool p_include_drive = true) const override;

	virtual bool file_exists(String p_file) override;
	virtual bool dir_exists(String p_dir) override;

	virtual Error make_dir(String p_dir) override;

	virtual Error rename(String p_from, String p_to) override;
	virtual Error remove(String p_name) override;

	uint64_t get_space_left() override;

	virtual bool is_link(String p_file) override { return false; }

	virtual String read_link(String p_file) override { return p_file; }

	virtual Error create_link(String p_source, String p_target) override { return FAILED; }

	virtual String get_filesystem_type() const override;

	DirAccessPack();
};

Ref<DirAccess> PackedData::try_open_directory(const String& p_path)
{
	Ref<DirAccess> da = memnew(DirAccessPack());
	if (da->change_dir(p_path) != OK) {
		da = Ref<DirAccess>();
	}
	return da;
}

// ZipArchive / PackedSourceZIP / FileAccessZip

#ifdef MINIZIP_ENABLED

#include <thirdparty/minizip/unzip.h>

class ZipArchive final
{
public:
	struct File
	{
		int package = -1;
		unz_file_pos file_pos;
	};

private:
	struct Package
	{
		String filename;
	};

public:
	struct Data
	{
		Vector<Package> packages;
		HashMap<String, File> files;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void close_handle(unzFile p_file);
	static unzFile get_file_handle(const String& p_file);

	static bool file_exists(const String& p_name);

	static bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		PackSource* p_source, const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());
	static Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>());

	ZipArchive() = delete;
	ZipArchive(const ZipArchive&) = delete;
	ZipArchive& operator=(const ZipArchive&) = delete;
	~ZipArchive() = delete;
};

class FileAccessZip final : public FileAccess
{
	unzFile zfile = nullptr;
	unz_file_info64 file_info;

	mutable bool at_eof = false;

	void _close();

public:
	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file
	virtual bool is_open() const override; ///< true when file is open

	virtual void seek(uint64_t p_position) override;		///< seek to a given position
	virtual void seek_end(int64_t p_position = 0) override; ///< seek from the end of file
	virtual uint64_t get_position() const override;			///< get position in the file
	virtual uint64_t get_length() const override;			///< get size of the file

	virtual bool eof_reached() const override; ///< reading passed EOF

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override;
	virtual bool store_buffer(const uint8_t* p_src, uint64_t p_length) override;

	virtual bool file_exists(const String& p_name) override; ///< return true if a file exists

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(const String& p_file) override { return 0; }

	virtual Error _set_unix_permissions(const String& p_file, uint32_t p_permissions) override
	{
		return FAILED;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual void close() override;

	FileAccessZip(const String& p_path, const PackedData::PackedFile& p_file);
	~FileAccessZip();
};

#endif // MINIZIP_ENABLED

#include <thirdparty/minizip/unzip.h>
#include <thirdparty/minizip/zip.h>

class ZIPPacker final : public RefCounted
{
	Ref<FileAccess> fa;
	zipFile zf = nullptr;
	int compression_level = Z_DEFAULT_COMPRESSION;
	HashSet<String> directories;

public:
	enum ZipAppend
	{
		APPEND_CREATE = 0,
		APPEND_CREATEAFTER = 1,
		APPEND_ADDINZIP = 2,
	};

	enum CompressionLevel
	{
		COMPRESSION_DEFAULT = Z_DEFAULT_COMPRESSION,
		COMPRESSION_NONE = Z_NO_COMPRESSION,
		COMPRESSION_FAST = Z_BEST_SPEED,
		COMPRESSION_BEST = Z_BEST_COMPRESSION,
	};

	Error open(const String& p_path, ZipAppend p_append);
	Error close();

	void set_compression_level(int p_compression_level);
	int get_compression_level() const;

	Error start_file(
		const String& p_path, uint32_t p_permissions = 0644, uint64_t p_modified_time = 0);
	Error write_file(const Vector<uint8_t>& p_data);
	Error close_file();

	Error add_directory(
		const String& p_path, uint32_t p_permissions = 0755, uint64_t p_modified_time = 0);

	ZIPPacker();
	~ZIPPacker();
};

class ZIPReader final : public RefCounted
{
	Ref<FileAccess> fa;
	unzFile uzf = nullptr;

public:
	Error open(const String& p_path);
	Error close();

	PackedStringArray get_files();
	PackedByteArray read_file(const String& p_path, bool p_case_sensitive);
	bool file_exists(const String& p_path, bool p_case_sensitive);
	int get_compression_level(const String& p_path, bool p_case_sensitive);

	ZIPReader();
	~ZIPReader();
};
