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

class NetSocket : public RefCounted
{
protected:
	static Ref<NetSocket> (*_create)();

public:
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

		Address(const IPAddress& p_addr, uint16_t p_port) : _family(Family::INET)
		{
			_ip = p_addr;
			_port = p_port;
		}

		Address(const String& p_path) : _family(Family::UNIX), _path(p_path.utf8()) {}

		Address(const CharString& p_path) : _family(Family::UNIX), _path(p_path) {}
	};

	virtual Error open(Family p_family, Type p_type, IP::Type& r_ip_type) = 0;
	virtual void close() = 0;
	virtual Error bind(Address p_addr) = 0;
	virtual Error listen(int p_max_pending) = 0;
	virtual Error connect_to_host(Address p_addr) = 0;
	virtual Error poll(PollType p_type, int p_timeout) const = 0;
	virtual Error recv(uint8_t* p_buffer, int p_len, int& r_read) = 0;
	virtual Error recvfrom(uint8_t* p_buffer, int p_len, int& r_read, IPAddress& r_ip,
		uint16_t& r_port, bool p_peek = false) = 0;
	virtual Error send(const uint8_t* p_buffer, int p_len, int& r_sent) = 0;
	virtual Error sendto(
		const uint8_t* p_buffer, int p_len, int& r_sent, IPAddress p_ip, uint16_t p_port) = 0;
	virtual Ref<NetSocket> accept(Address& r_addr) = 0;

	virtual bool is_open() const = 0;
	virtual int get_available_bytes() const = 0;
	virtual Error get_socket_address(Address* r_addr) const = 0;

	virtual Error set_broadcasting_enabled(
		bool p_enabled) = 0; // Returns OK if the socket option has been set successfully.
	virtual void set_blocking_enabled(bool p_enabled) = 0;
	virtual void set_ipv6_only_enabled(bool p_enabled) = 0;
	virtual void set_tcp_no_delay_enabled(bool p_enabled) = 0;
	virtual void set_reuse_address_enabled(bool p_enabled) = 0;
	virtual Error join_multicast_group(
		const IPAddress& p_multi_address, const String& p_if_name) = 0;
	virtual Error leave_multicast_group(
		const IPAddress& p_multi_address, const String& p_if_name) = 0;

	virtual ~NetSocket() {}
};

// --- Folded from core/crypto/crypto.h ---
// --- Inlined/Folded from core/crypto/hashing_context.h ---
#include "core/types.h"

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
#include "core/types.h"

class CryptoKey : public Resource
{
public:
	struct Data;

private:
	Data* data = nullptr;

public:
	static CryptoKey* create(bool p_notify_postinitialize = true);
	static bool is_available();

	Data* get_data() const { return data; }

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
public:
	struct Data;

private:
	Data* data = nullptr;

public:
	static X509Certificate* create(bool p_notify_postinitialize = true);
	static bool is_available();

	Data* get_data() const { return data; }

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
public:
	struct Data;

private:
	Data* data = nullptr;

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

// --- Folded from core/crypto/aes_context.h ---
// --- Folded from core/crypto/crypto_core.h ---
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

	class MD5Context {
	private:
		void *ctx = nullptr;

	public:
		MD5Context();
		~MD5Context();

		Error start();
		Error update(const uint8_t *p_src, size_t p_len);
		Error finish(unsigned char r_hash[16]);
	};

	class SHA1Context {
	private:
		void *ctx = nullptr;

	public:
		SHA1Context();
		~SHA1Context();

		Error start();
		Error update(const uint8_t *p_src, size_t p_len);
		Error finish(unsigned char r_hash[20]);
	};

	class SHA256Context {
	private:
		void *ctx = nullptr;

	public:
		SHA256Context();
		~SHA256Context();

		Error start();
		Error update(const uint8_t *p_src, size_t p_len);
		Error finish(unsigned char r_hash[32]);
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
// --- End core/crypto/crypto_core.h ---
#include "core/types.h"

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


// --- End core/crypto/aes_context.h ---

// --- Folded from core/crypto/crypto_resource_format.h ---
#include "core/io/resource_loader.h"
#include "core/types.h"

class ResourceFormatLoaderCrypto : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;

	// Treat certificates as text files, do not generate a `*.{crt,key,pub}.uid` file.
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceUID::INVALID_ID;
	}

	virtual bool has_custom_uid_support() const override { return true; }
};

class ResourceFormatSaverCrypto : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
};



// --- End core/crypto/crypto_resource_format.h ---
// --- End core/crypto/crypto.h ---

// --- Folded from core/io/stream_peer.h ---
#include "core/types.h"

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

class StreamPeerExtension : public StreamPeer
{
protected:

public:
	virtual Error put_data(const uint8_t* p_data, int p_bytes) override;
	virtual Error put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent) override;
	virtual Error get_data(uint8_t* p_buffer, int p_bytes) override;
	virtual Error get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received) override;
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

// --- Folded from core/io/stream_peer_gzip.h ---

#include "core/types.h"

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



// --- End core/io/stream_peer_gzip.h ---

// --- Folded from core/io/http_client.h ---


#ifndef WEB_ENABLED
#include "core/io/ip.h"

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

// --- Folded from core/io/stream_peer_tls.h ---


class StreamPeerTLS : public StreamPeer
{
public:
	struct Data;

private:
	Data* data = nullptr;

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

// --- Folded from core/io/packet_peer.h ---

#include "core/types.h"

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

class PacketPeerExtension : public PacketPeer
{
protected:
	static void _bind_methods();

public:
	virtual Error get_packet(const uint8_t** r_buffer,
		int& r_buffer_size) override; ///< buffer is GONE after next get_packet
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;
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
// --- End core/io/packet_peer.h ---

// --- Folded from core/io/stream_peer_socket.h ---
#include "core/io/ip.h"



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

// --- Folded from core/io/socket_server.h ---
#include "core/io/ip.h"


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
// --- End core/io/stream_peer_socket.h ---
// --- End core/io/stream_peer.h ---

// --- Folded from core/io/dir_access.h ---
#include "core/string/ustring.h"
#include "core/typedefs.h"
#include "core/types.h"

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

// --- Folded from core/io/file_access_wrappers.h ---

#include "core/io/file_access.h"

#include "core/types.h"
#include "core/string/ustring.h"
#include "core/types.h"
#include "core/templates/list.h"

// ============================================================================
// FileAccessMemory
// ============================================================================

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

// ============================================================================
// FileAccessPatched
// ============================================================================

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

// ============================================================================
// FileAccessCompressed
// ============================================================================

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

// ============================================================================
// FileAccessEncrypted
// ============================================================================

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

// ============================================================================
// PackedData / PackSource / FileAccessPack / DirAccessPack
// ============================================================================

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

class PackSource
{
public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) = 0;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) = 0;

	virtual ~PackSource() {}
};

class PackedSourcePCK final : public PackSource
{
public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
};

class PackedSourceDirectory final : public PackSource
{
	void add_directory(const String& p_path, bool p_replace_files);

public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
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

// ============================================================================
// ZipArchive / PackedSourceZIP / FileAccessZip
// ============================================================================

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

class PackedSourceZIP final : public PackSource
{
public:
	virtual bool try_open_pack(const String& p_path, bool p_replace_files, uint64_t p_offset,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
	virtual Ref<FileAccess> get_file(const String& p_path, PackedData::PackedFile* p_file,
		const Vector<uint8_t>& p_decryption_key = Vector<uint8_t>()) override;
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

// --- Folded from modules/zip/zip_reader.h ---
#include <thirdparty/minizip/unzip.h>
#include <thirdparty/minizip/zip.h>
#include "core/io/file_access.h"

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
// --- End modules/zip/zip_reader.h ---
// --- End core/io/file_access_wrappers.h ---
// --- End core/io/dir_access.h ---
