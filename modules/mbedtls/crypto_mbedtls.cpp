/**************************************************************************/
/*  crypto_mbedtls.cpp                                                    */
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
// --- Inlined from modules/mbedtls/crypto_mbedtls.h ---
#include <mbedtls/debug.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cookie.h>
#include <mbedtls/timing.h>

#include "core/crypto/crypto.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/stream_peer_tls.h"

#if MBEDTLS_VERSION_MAJOR < 4
int godot_mbedtls_random_compat(void* p_rng, unsigned char* r_output, size_t p_output_len);
#define GODOT_MBEDTLS_COMPAT_ARGS(x) x, godot_mbedtls_random_compat, nullptr
#define GODOT_MBEDTLS_COMPAT_SSL_CONF                                                              \
	mbedtls_ssl_conf_rng(&conf, godot_mbedtls_random_compat, nullptr);
#else
#define GODOT_MBEDTLS_COMPAT_ARGS(x) x
#define GODOT_MBEDTLS_COMPAT_SSL_CONF
#endif

struct CryptoKey::Data
{
	psa_key_id_t pk_slot = PSA_KEY_ID_NULL;
	psa_key_type_t pk_type = PSA_KEY_TYPE_NONE;
	size_t pk_size = 0;
	bool public_only = true;

	void clear();
	int parse_key(const uint8_t* p_buf, int p_size, bool p_public_only);
	psa_key_id_t reimport(psa_key_usage_t p_usage,
		psa_algorithm_t p_alg); // You *MUST* call psa_destroy_key when you are done with the key.

	static Ref<CryptoKey> generate(psa_key_type_t p_type, size_t p_size);
};

struct X509Certificate::Data
{
	mbedtls_x509_crt cert;
	int locks = 0;

	Data()
	{
		mbedtls_x509_crt_init(&cert);
	}

	_FORCE_INLINE_ void lock() { locks++; }
	_FORCE_INLINE_ void unlock() { locks--; }
};

struct HMACContext::Data
{
	psa_mac_operation_t mac_op = PSA_MAC_OPERATION_INIT;
	psa_key_id_t key_slot = PSA_KEY_ID_NULL;
	int hash_len = 0;

	void clear();
	static bool is_hash_type_allowed(HashingContext::HashType p_hash_type);
};

namespace MbedTLSModule
{
void initialize_crypto();
void finalize_crypto();
Ref<X509Certificate> get_default_certificates();
psa_algorithm_t psa_alg_from_hashtype(HashingContext::HashType p_hash_type, int& r_size);
mbedtls_md_type_t md_type_from_hashtype(HashingContext::HashType p_hash_type, int& r_size);
} // namespace MbedTLSModule

class TLSContextMbedTLS;

class CookieContextMbedTLS final : public RefCounted
{
	friend class TLSContextMbedTLS;

private:
	bool inited = false;
	mbedtls_ssl_cookie_ctx cookie_ctx;

public:
	Error setup();
	void clear();

	CookieContextMbedTLS();
	~CookieContextMbedTLS();
};

class TLSContextMbedTLS final : public RefCounted
{
private:
	bool inited = false;
	mbedtls_pk_context pk;
	Ref<X509Certificate> certs;
	Ref<CookieContextMbedTLS> cookies;
	mbedtls_ssl_context tls;
	mbedtls_ssl_config conf;

	Error _setup(int p_endpoint, int p_transport, int p_authmode);

public:
	static void print_mbedtls_error(int p_ret);

	Error init_server(int p_transport, Ref<TLSOptions> p_options,
		Ref<CookieContextMbedTLS> p_cookies = Ref<CookieContextMbedTLS>());
	Error init_client(int p_transport, const String& p_hostname, Ref<TLSOptions> p_options);
	void clear();

	mbedtls_ssl_context* get_context();

	TLSContextMbedTLS();
	~TLSContextMbedTLS();
};

struct StreamPeerTLS::Data
{
	Status status = STATUS_DISCONNECTED;
	String hostname;
	Ref<StreamPeer> base;
	Ref<TLSContextMbedTLS> tls_ctx;

	Data()
	{
		tls_ctx.instantiate();
	}

	static int bio_recv(void* ctx, unsigned char* buf, size_t len);
	static int bio_send(void* ctx, const unsigned char* buf, size_t len);
	void cleanup();
	Error do_handshake(StreamPeerTLS* p_owner);
};

struct PacketPeerDTLS::Data
{
	enum
	{
		PACKET_BUFFER_SIZE = 65536
	};

	uint8_t packet_buffer[PACKET_BUFFER_SIZE];
	Status status = STATUS_DISCONNECTED;
	String hostname;
	Ref<PacketPeerUDP> base;
	Ref<TLSContextMbedTLS> tls_ctx;
	mbedtls_timing_delay_context timer;

	Data()
	{
		tls_ctx.instantiate();
	}

	static int bio_recv(void* ctx, unsigned char* buf, size_t len);
	static int bio_send(void* ctx, const unsigned char* buf, size_t len);
	void cleanup();
	Error do_handshake();
	int set_cookie();
	Error accept_peer(Ref<PacketPeerUDP> p_base, Ref<TLSOptions> p_options,
		Ref<CookieContextMbedTLS> p_cookies = Ref<CookieContextMbedTLS>());
};

struct DTLSServer::Data
{
	Ref<TLSOptions> tls_options;
	Ref<CookieContextMbedTLS> cookies;

	Data()
	{
		cookies.instantiate();
	}
};

// --- End modules/mbedtls/crypto_mbedtls.h ---
#include "mbedtls_cpp_compat.h"
#include "register_types.h"

#include <mbedtls/debug.h>
#include <mbedtls/md.h>
#include <mbedtls/pem.h>
#include <mbedtls/psa_util.h>
#include <psa/crypto.h>

#include "core/config/project_settings.h"
#include "core/io/certs_compressed.gen.h"
#include "core/io/compression.h"
#include "core/io/file_access.h"
#include "core/io/stream_peer_socket.h"
#include "core/os/os.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

#define PEM_BEGIN_CRT "-----BEGIN CERTIFICATE-----\n"
#define PEM_END_CRT "-----END CERTIFICATE-----\n"
#define PEM_MIN_SIZE 54

#if MBEDTLS_VERSION_MAJOR < 4
int godot_mbedtls_random_compat(void* p_rng, unsigned char* r_output, size_t p_output_len)
{
	return psa_generate_random(r_output, p_output_len);
}
#endif

static Ref<X509Certificate> default_certs;

/// CryptoKey

CryptoKey::CryptoKey()
{
	data = memnew(Data);
}

CryptoKey::~CryptoKey()
{
	if (data) {
		data->clear();
		memdelete(data);
		data = nullptr;
	}
}

CryptoKey* CryptoKey::create(bool p_notify_postinitialize)
{
	return memnew(CryptoKey);
}

bool CryptoKey::is_available()
{
	return true;
}

bool CryptoKey::is_public_only() const
{
	return data->public_only;
}

Ref<CryptoKey> CryptoKey::Data::generate(psa_key_type_t p_type, size_t p_size)
{
	psa_key_id_t slot = PSA_KEY_ID_NULL;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, p_type);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_COPY);
	psa_set_key_bits(&attr, p_size);
	int ret = psa_generate_key(&attr, &slot);
	ERR_FAIL_COND_V(ret != 0, nullptr);
	Ref<CryptoKey> out;
	out.instantiate();
	out->data->public_only = false;
	out->data->pk_slot = slot;
	out->data->pk_type = p_type;
	out->data->pk_size = p_size;
	return out;
}

Error CryptoKey::load(const String& p_path, bool p_public_only)
{
	PackedByteArray out;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(
		f.is_null(), ERR_INVALID_PARAMETER, "Cannot open CryptoKey file '" + p_path + "'.");

	uint64_t flen = f->get_length();
	out.resize(flen + 1);
	f->get_buffer(out.ptrw(), flen);
	out.write[flen] = 0; // string terminator

	int ret = data->parse_key(out.ptr(), out.size(), p_public_only);
	// We MUST zeroize the memory for safety!
	mbedtls_platform_zeroize(out.ptrw(), out.size());
	ERR_FAIL_COND_V_MSG(ret, FAILED, "Error parsing key '" + itos(ret) + "'.");

	data->public_only = p_public_only;
	return OK;
}

Error CryptoKey::save(const String& p_path, bool p_public_only)
{
	ERR_FAIL_COND_V_MSG(data->public_only && !p_public_only, ERR_INVALID_PARAMETER,
		"The key only has the public part. Cannot save as a private key.");
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(
		f.is_null(), ERR_INVALID_PARAMETER, "Cannot save CryptoKey file '" + p_path + "'.");

	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	int ret = 0;
	if (p_public_only) {
		ret = mbedtls_pk_copy_public_from_psa(data->pk_slot, &ctx);
	}
	else {
		ret = mbedtls_pk_copy_from_psa(data->pk_slot, &ctx);
	}
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(FAILED, "Failed to import key: " + itos(ret));
	}

	unsigned char w[16000];
	memset(w, 0, sizeof(w));
	if (p_public_only) {
		ret = mbedtls_pk_write_pubkey_pem(&ctx, w, sizeof(w));
	}
	else {
		ret = mbedtls_pk_write_key_pem(&ctx, w, sizeof(w));
	}
	mbedtls_pk_free(&ctx);

	if (ret != 0) {
		mbedtls_platform_zeroize(w, sizeof(w)); // Zeroize anything we might have written.
		ERR_FAIL_V_MSG(FAILED, "Error writing key '" + itos(ret) + "'.");
	}

	size_t len = strlen((char*)w);
	f->store_buffer(w, len);
	mbedtls_platform_zeroize(w, sizeof(w)); // Zeroize temporary buffer.
	return OK;
}

Error CryptoKey::load_from_string(const String& p_string_key, bool p_public_only)
{
	const CharString string_key_utf8 = p_string_key.utf8();
	int ret = data->parse_key(
		(const unsigned char*)string_key_utf8.get_data(), string_key_utf8.size(), p_public_only);
	ERR_FAIL_COND_V_MSG(ret, FAILED, "Error parsing key '" + itos(ret) + "'.");

	data->public_only = p_public_only;
	return OK;
}

String CryptoKey::save_to_string(bool p_public_only)
{
	ERR_FAIL_COND_V_MSG(data->public_only && !p_public_only, "",
		"The key only has the public part. Cannot save as a private key.");
	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	int ret = 0;
	if (p_public_only) {
		ret = mbedtls_pk_copy_public_from_psa(data->pk_slot, &ctx);
	}
	else {
		ret = mbedtls_pk_copy_from_psa(data->pk_slot, &ctx);
	}
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG("", "Failed to import key: " + itos(ret));
	}

	unsigned char w[16000];
	memset(w, 0, sizeof(w));
	if (p_public_only) {
		ret = mbedtls_pk_write_pubkey_pem(&ctx, w, sizeof(w));
	}
	else {
		ret = mbedtls_pk_write_key_pem(&ctx, w, sizeof(w));
	}
	mbedtls_pk_free(&ctx);
	if (ret != 0) {
		mbedtls_platform_zeroize(w, sizeof(w));
		ERR_FAIL_V_MSG("", "Error saving key '" + itos(ret) + "'.");
	}
	String s = String::utf8((char*)w);
	mbedtls_platform_zeroize(w, sizeof(w));
	return s;
}

psa_key_id_t CryptoKey::Data::reimport(psa_key_usage_t p_usage, psa_algorithm_t p_alg)
{
	ERR_FAIL_COND_V(pk_slot == PSA_KEY_ID_NULL, PSA_KEY_ID_NULL);

	// We can't copy the key, as we are not allowed to change the usage/algorithm.
	// We will need to export and reimport the key.
	psa_key_id_t slot = PSA_KEY_ID_NULL;
	int size = PSA_EXPORT_KEY_OUTPUT_SIZE(pk_type, pk_size);
	uint8_t* buf = (uint8_t*)alloca(size);
	size_t out_size = 0;
	int ret = psa_export_key(pk_slot, buf, size, &out_size);
	ERR_FAIL_COND_V_MSG(ret, slot, "Failed to export the key: " + itos(ret));

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, pk_type);
	psa_set_key_bits(&attr, pk_size);
	psa_set_key_usage_flags(&attr, p_usage);
	psa_set_key_algorithm(&attr, p_alg);
	ret = psa_import_key(&attr, buf, out_size, &slot);
	mbedtls_platform_zeroize(buf, size);

	ERR_FAIL_COND_V_MSG(ret, slot, "Failed to re-import key: " + itos(ret));
	return slot;
}

void CryptoKey::Data::clear()
{
	psa_destroy_key(pk_slot);
	pk_slot = PSA_KEY_ID_NULL;
	pk_type = PSA_KEY_TYPE_NONE;
	pk_size = 0;
}

int CryptoKey::Data::parse_key(const uint8_t* p_buf, int p_size, bool p_public_only)
{
	clear();

	// Not yet fully supported via PSA
	// See: https://github.com/ARM-software/psa-api/issues/50
	psa_key_usage_t flags = PSA_KEY_USAGE_VERIFY_HASH;
	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	int ret = -1;
	if (p_public_only) {
		ret = mbedtls_pk_parse_public_key(&ctx, p_buf, p_size);
	}
	else {
		flags = PSA_KEY_USAGE_SIGN_HASH;
		ret = mbedtls_pk_parse_key(&ctx, p_buf, p_size, nullptr, GODOT_MBEDTLS_COMPAT_ARGS(0));
	}
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(PSA_ERROR_GENERIC_ERROR, "Failed to parse key " + itos(ret));
	}
	psa_key_attributes_t import_attr = PSA_KEY_ATTRIBUTES_INIT;
	ret = mbedtls_pk_get_psa_attributes(&ctx, flags, &import_attr);
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(PSA_ERROR_GENERIC_ERROR, "Failed to get key attributes " + itos(ret));
	}
	pk_type = psa_get_key_type(&import_attr);
	pk_size = psa_get_key_bits(&import_attr);
	psa_reset_key_attributes(&import_attr);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, pk_type);
	psa_set_key_bits(&attr, pk_size);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_COPY);
	ret = mbedtls_pk_import_into_psa(&ctx, &attr, &pk_slot);
	mbedtls_pk_free(&ctx);
	if (ret != PSA_SUCCESS) {
		clear();
	}
	return ret;
}

/// X509Certificate

X509Certificate::X509Certificate()
{
	data = memnew(Data);
}

X509Certificate::~X509Certificate()
{
	if (data) {
		int locks = data->locks;
		mbedtls_x509_crt_free(&data->cert);
		memdelete(data);
		data = nullptr;
		ERR_FAIL_COND(locks);
	}
}

X509Certificate* X509Certificate::create(bool p_notify_postinitialize)
{
	return memnew(X509Certificate);
}

bool X509Certificate::is_available()
{
	return true;
}

Error X509Certificate::load(const String& p_path)
{
	ERR_FAIL_COND_V_MSG(data->locks, ERR_ALREADY_IN_USE, "Certificate is already in use.");

	PackedByteArray out;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_INVALID_PARAMETER,
		vformat("Cannot open X509Certificate file '%s'.", p_path));

	uint64_t flen = f->get_length();
	out.resize(flen + 1);
	f->get_buffer(out.ptrw(), flen);
	out.write[flen] = 0; // string terminator

	int ret = mbedtls_x509_crt_parse(&data->cert, out.ptr(), out.size());
	ERR_FAIL_COND_V_MSG(ret < 0, FAILED,
		vformat("Error parsing X509 certificates from file '%s': %d.", p_path, ret));
	if (ret > 0) { // Some certs parsed fine, don't error.
		print_verbose(vformat("MbedTLS: Some X509 certificates could not be parsed from file '%s' "
							  "(%d certificates skipped).",
			p_path, ret));
	}

	return OK;
}

Error X509Certificate::load_from_memory(const uint8_t* p_buffer, int p_len)
{
	ERR_FAIL_COND_V_MSG(data->locks, ERR_ALREADY_IN_USE, "Certificate is already in use.");

	int ret = mbedtls_x509_crt_parse(&data->cert, p_buffer, p_len);
	ERR_FAIL_COND_V_MSG(ret < 0, FAILED, vformat("Error parsing X509 certificates: %d.", ret));
	if (ret > 0) { // Some certs parsed fine, don't error.
		print_verbose(vformat(
			"MbedTLS: Some X509 certificates could not be parsed (%d certificates skipped).", ret));
	}
	return OK;
}

Error X509Certificate::save(const String& p_path)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_INVALID_PARAMETER,
		vformat("Cannot save X509Certificate file '%s'.", p_path));

	mbedtls_x509_crt* crt = &data->cert;
	while (crt) {
		unsigned char w[4096];
		size_t wrote = 0;
		int ret = mbedtls_pem_write_buffer(
			PEM_BEGIN_CRT, PEM_END_CRT, data->cert.raw.p, data->cert.raw.len, w, sizeof(w), &wrote);
		if (ret != 0 || wrote == 0) {
			ERR_FAIL_V_MSG(FAILED, "Error writing certificate '" + itos(ret) + "'.");
		}

		f->store_buffer(w, wrote - 1); // don't write the string terminator
		crt = crt->next;
	}
	return OK;
}

String X509Certificate::save_to_string()
{
	String buffer;
	mbedtls_x509_crt* crt = &data->cert;
	while (crt) {
		unsigned char w[4096];
		size_t wrote = 0;
		int ret = mbedtls_pem_write_buffer(
			PEM_BEGIN_CRT, PEM_END_CRT, data->cert.raw.p, data->cert.raw.len, w, sizeof(w), &wrote);
		ERR_FAIL_COND_V_MSG(ret != 0 || wrote == 0, String(), "Error saving the certificate.");

		// PEM is base64, aka ascii
		buffer += String::ascii(Span((char*)w, wrote));
		crt = crt->next;
	}
	if (buffer.length() <= PEM_MIN_SIZE) {
		// When the returned value of variable 'buffer' would consist of no Base-64 data, return an
		// empty String instead.
		return String();
	}
	return buffer;
}

Error X509Certificate::load_from_string(const String& p_string_key)
{
	ERR_FAIL_COND_V_MSG(data->locks, ERR_ALREADY_IN_USE, "Certificate is already in use.");
	CharString cs = p_string_key.utf8();

	int ret = mbedtls_x509_crt_parse(&data->cert, (const unsigned char*)cs.get_data(), cs.size());
	ERR_FAIL_COND_V_MSG(ret < 0, FAILED, vformat("Error parsing X509 certificates: %d.", ret));
	if (ret > 0) { // Some certs parsed fine, don't error.
		print_verbose(vformat(
			"MbedTLS: Some X509 certificates could not be parsed (%d certificates skipped).", ret));
	}

	return OK;
}

/// HMACContext

bool HMACContext::Data::is_hash_type_allowed(HashingContext::HashType p_hash_type)
{
	switch (p_hash_type) {
	case HashingContext::HASH_SHA1:
	case HashingContext::HASH_SHA256:
		return true;
	default:
		return false;
	}
}

HMACContext::HMACContext()
{
	data = memnew(Data);
}

HMACContext::~HMACContext()
{
	if (data) {
		data->clear();
		memdelete(data);
		data = nullptr;
	}
}

HMACContext* HMACContext::create(bool p_notify_postinitialize)
{
	return memnew(HMACContext);
}

bool HMACContext::is_available()
{
	return true;
}

Error HMACContext::start(HashingContext::HashType p_hash_type, const PackedByteArray& p_key)
{
	ERR_FAIL_COND_V_MSG(data->key_slot != PSA_KEY_ID_NULL, ERR_FILE_ALREADY_IN_USE,
		"HMACContext already started.");

	// HMAC keys can be any size.
	ERR_FAIL_COND_V_MSG(p_key.is_empty(), ERR_INVALID_PARAMETER, "Key must not be empty.");

	bool allowed = Data::is_hash_type_allowed(p_hash_type);
	ERR_FAIL_COND_V_MSG(!allowed, ERR_INVALID_PARAMETER, "Unsupported hash type.");

	psa_algorithm_t ht =
		PSA_ALG_HMAC(MbedTLSModule::psa_alg_from_hashtype(p_hash_type, data->hash_len));
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, PSA_KEY_TYPE_HMAC);
	psa_set_key_algorithm(&attr, ht);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_SIGN_HASH);
	int ret = psa_import_key(&attr, p_key.ptr(), p_key.size(), &data->key_slot);
	ERR_FAIL_COND_V_MSG(ret != PSA_SUCCESS, FAILED, "Error import key: " + itos(ret));

	ret = psa_mac_sign_setup(&data->mac_op, data->key_slot, ht);
	if (ret != PSA_SUCCESS) {
		data->clear();
		ERR_FAIL_V_MSG(FAILED, "Failed to setup HMAC signing: " + itos(ret));
	}
	return OK;
}

Error HMACContext::update(const PackedByteArray& p_data)
{
	ERR_FAIL_COND_V_MSG(data->key_slot == PSA_KEY_ID_NULL, ERR_INVALID_DATA,
		"Start must be called before update.");

	ERR_FAIL_COND_V_MSG(p_data.is_empty(), ERR_INVALID_PARAMETER, "Src must not be empty.");

	int ret = psa_mac_update(&data->mac_op, (const uint8_t*)p_data.ptr(), (size_t)p_data.size());
	ERR_FAIL_COND_V_MSG(ret != PSA_SUCCESS, FAILED, "Failed to update HMAC: " + itos(ret));
	return OK;
}

PackedByteArray HMACContext::finish()
{
	ERR_FAIL_COND_V_MSG(data->key_slot == PSA_KEY_ID_NULL, PackedByteArray(),
		"Start must be called before finish.");
	ERR_FAIL_COND_V_MSG(data->hash_len == 0, PackedByteArray(), "Unsupported hash type.");

	PackedByteArray out;
	out.resize(data->hash_len);
	size_t mac_length = 0;
	int ret = psa_mac_sign_finish(&data->mac_op, out.ptrw(), (size_t)out.size(), &mac_length);
	data->clear();
	ERR_FAIL_COND_V_MSG(ret, PackedByteArray(), "Error received while finishing HMAC");
	ERR_FAIL_COND_V(mac_length != (size_t)out.size(), PackedByteArray()); // Bug?
	return out;
}

void HMACContext::Data::clear()
{
	if (key_slot == PSA_KEY_ID_NULL) {
		return;
	}
	hash_len = 0;
	psa_mac_abort(&mac_op);
	mac_op = PSA_MAC_OPERATION_INIT;
	psa_destroy_key(key_slot);
	key_slot = PSA_KEY_ID_NULL;
}

/// Crypto & MbedTLSModule

namespace MbedTLSModule
{

void initialize_crypto() {}

void finalize_crypto()
{
	default_certs = nullptr;
}

Ref<X509Certificate> get_default_certificates()
{
	return default_certs;
}

psa_algorithm_t psa_alg_from_hashtype(HashingContext::HashType p_hash_type, int& r_size)
{
	switch (p_hash_type) {
	case HashingContext::HASH_MD5:
		r_size = 16;
		return PSA_ALG_MD5;
	case HashingContext::HASH_SHA1:
		r_size = 20;
		return PSA_ALG_SHA_1;
	case HashingContext::HASH_SHA256:
		r_size = 32;
		return PSA_ALG_SHA_256;
	default:
		r_size = 0;
		ERR_FAIL_V_MSG(PSA_ALG_NONE, "Invalid hash type.");
	}
}

mbedtls_md_type_t md_type_from_hashtype(HashingContext::HashType p_hash_type, int& r_size)
{
	switch (p_hash_type) {
	case HashingContext::HASH_MD5:
		r_size = 16;
		return MBEDTLS_MD_MD5;
	case HashingContext::HASH_SHA1:
		r_size = 20;
		return MBEDTLS_MD_SHA1;
	case HashingContext::HASH_SHA256:
		r_size = 32;
		return MBEDTLS_MD_SHA256;
	default:
		r_size = 0;
		ERR_FAIL_V_MSG(MBEDTLS_MD_NONE, "Invalid hash type.");
	}
}

} // namespace MbedTLSModule

Crypto* Crypto::create(bool p_notify_postinitialize)
{
	return memnew(Crypto);
}

bool Crypto::is_available()
{
	return true;
}

Crypto::Crypto() {}

Crypto::~Crypto() {}

void Crypto::load_default_certificates(const String& p_path)
{
	ERR_FAIL_COND(default_certs.is_valid());

	default_certs = memnew(X509Certificate);
	ERR_FAIL_COND(default_certs.is_null());

	if (!p_path.is_empty()) {
		// Use certs defined in project settings.
		default_certs->load(p_path);
	}
	else {
		// Try to use system certs otherwise.
		String system_certs = OS::get_system_ca_certificates();
		if (!system_certs.is_empty()) {
			CharString cs = system_certs.utf8();
			default_certs->load_from_memory((const uint8_t*)cs.get_data(), cs.size());
			print_verbose("Loaded system CA certificates");
		}
#ifdef BUILTIN_CERTS_ENABLED
		else {
			// Use builtin certs if there are no system certs.
			PackedByteArray certs;
			certs.resize(_certs_uncompressed_size + 1);
			const int64_t decompressed_size =
				Compression::decompress(certs.ptrw(), _certs_uncompressed_size, _certs_compressed,
					_certs_compressed_size, Compression::MODE_DEFLATE);
			ERR_FAIL_COND_MSG(decompressed_size != _certs_uncompressed_size,
				"Error decompressing builtin CA certificates. Decompressed size did not match "
				"expected size.");
			certs.write[_certs_uncompressed_size] = 0; // Make sure it ends with string terminator
			default_certs->load_from_memory(certs.ptr(), certs.size());
			print_verbose("Loaded builtin CA certificates");
		}
#endif
	}
}

Ref<CryptoKey> Crypto::generate_rsa(int p_bits)
{
	return CryptoKey::Data::generate(PSA_KEY_TYPE_RSA_KEY_PAIR, p_bits);
}

Ref<X509Certificate> Crypto::generate_self_signed_certificate(Ref<CryptoKey> p_key,
	const String& p_issuer_name, const String& p_not_before, const String& p_not_after)
{
	ERR_FAIL_COND_V_MSG(p_key.is_null(), nullptr, "Invalid private key argument.");

	uint8_t rand_serial[20] = {};
	int ret = psa_generate_random(rand_serial, sizeof(rand_serial));
	ERR_FAIL_COND_V(ret, nullptr);

	mbedtls_pk_context ctx;
	mbedtls_pk_init(&ctx);
	ret = mbedtls_pk_copy_from_psa(p_key->get_data()->pk_slot, &ctx);
	if (ret) {
		mbedtls_pk_free(&ctx);
		ERR_FAIL_V_MSG(nullptr, "Failed to import private key: " + itos(ret));
	}

	mbedtls_x509write_cert crt;
	mbedtls_x509write_crt_init(&crt);
	mbedtls_x509write_crt_set_subject_key(&crt, &ctx);
	mbedtls_x509write_crt_set_issuer_key(&crt, &ctx);
	mbedtls_x509write_crt_set_subject_name(&crt, p_issuer_name.utf8().get_data());
	mbedtls_x509write_crt_set_issuer_name(&crt, p_issuer_name.utf8().get_data());
	mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
	mbedtls_x509write_crt_set_md_alg(&crt, MBEDTLS_MD_SHA256);
	mbedtls_x509write_crt_set_serial_raw(&crt, rand_serial, sizeof(rand_serial));
	mbedtls_x509write_crt_set_validity(
		&crt, p_not_before.utf8().get_data(), p_not_after.utf8().get_data());
	mbedtls_x509write_crt_set_basic_constraints(&crt, 1, 0);

	unsigned char buf[4096];
	memset(buf, 0, 4096);

	ret = mbedtls_x509write_crt_pem(&crt, buf, GODOT_MBEDTLS_COMPAT_ARGS(4096));
	mbedtls_x509write_crt_free(&crt);
	mbedtls_pk_free(&ctx);
	ERR_FAIL_COND_V_MSG(ret != 0, nullptr, "Failed to generate certificate: " + itos(ret));
	buf[4095] = '\0'; // Make sure strlen can't fail.

	Ref<X509Certificate> out;
	out.instantiate();
	out->load_from_memory(buf, strlen((char*)buf) + 1); // Use strlen to find correct output size.
	return out;
}

PackedByteArray Crypto::generate_random_bytes(int p_bytes)
{
	ERR_FAIL_COND_V(p_bytes < 0, PackedByteArray());
	PackedByteArray out;
	out.resize(p_bytes);
	int ret = psa_generate_random(out.ptrw(), out.size());
	ERR_FAIL_COND_V(ret != PSA_SUCCESS, PackedByteArray());
	return out;
}

Vector<uint8_t> Crypto::sign(
	HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash, Ref<CryptoKey> p_key)
{
	int size;
	psa_algorithm_t hash_type = MbedTLSModule::psa_alg_from_hashtype(p_hash_type, size);
	ERR_FAIL_COND_V_MSG(hash_type == PSA_ALG_NONE, Vector<uint8_t>(), "Invalid hash type.");
	ERR_FAIL_COND_V_MSG(p_hash.size() != size, Vector<uint8_t>(),
		"Invalid hash provided. Size must be " + itos(size));
	ERR_FAIL_COND_V_MSG(p_key.is_null(), Vector<uint8_t>(), "Invalid key provided.");
	ERR_FAIL_COND_V_MSG(p_key->is_public_only(), Vector<uint8_t>(),
		"Invalid key provided. Cannot sign with public_only keys.");
	psa_key_type_t key_type = p_key->get_data()->pk_type;
	psa_algorithm_t alg = PSA_ALG_NONE;
	if (PSA_KEY_TYPE_IS_RSA(key_type)) {
		alg = PSA_ALG_RSA_PKCS1V15_SIGN(hash_type);
	}
	else if (PSA_KEY_TYPE_IS_ECC(key_type)) {
		alg = PSA_ALG_ECDSA(hash_type);
	}
	ERR_FAIL_COND_V_MSG(
		alg == PSA_ALG_NONE, Vector<uint8_t>(), "Unknown key type: " + itos(key_type));
	unsigned char buf[PSA_SIGNATURE_MAX_SIZE];
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_SIGN_HASH, alg);
	size_t sig_size = 0;
	int ret = psa_sign_hash(pk, alg, p_hash.ptr(), p_hash.size(), buf, sizeof(buf), &sig_size);
	psa_destroy_key(pk);
	ERR_FAIL_COND_V_MSG(ret, Vector<uint8_t>(), "Error while signing: " + itos(ret));

	Vector<uint8_t> out;
	if (PSA_KEY_TYPE_IS_ECC(key_type)) {
		// Historically, mbedTLS used DER format for ECDA signatures.
		// We need to convert the output from raw to DER before returning it for compatility.
		size_t key_size = p_key->get_data()->pk_size;
		size_t out_size = 0;
		uint8_t der[GODOT_MBEDTLS_ECDSA_DER_MAX_LEN];
		ret = godot_mbedtls_ecdsa_raw_to_der(key_size, buf, sig_size, der, sizeof(der), &out_size);
		ERR_FAIL_COND_V_MSG(
			ret, Vector<uint8_t>(), "Failed to convert signature to DER format: " + itos(ret));
		out.resize(out_size);
		memcpy(out.ptrw(), der, out_size);
	}
	else {
		out.resize(sig_size);
		memcpy(out.ptrw(), buf, sig_size);
	}
	return out;
}

bool Crypto::verify(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash,
	const Vector<uint8_t>& p_signature, Ref<CryptoKey> p_key)
{
	int size;
	psa_algorithm_t hash_type = MbedTLSModule::psa_alg_from_hashtype(p_hash_type, size);
	ERR_FAIL_COND_V_MSG(hash_type == PSA_ALG_NONE, false, "Invalid hash type.");
	ERR_FAIL_COND_V_MSG(
		p_hash.size() != size, false, "Invalid hash provided. Size must be " + itos(size));
	ERR_FAIL_COND_V_MSG(p_key.is_null(), false, "Invalid key provided.");
	psa_key_type_t key_type = p_key->get_data()->pk_type;
	psa_algorithm_t alg = PSA_ALG_NONE;
	Vector<uint8_t> sig;
	if (PSA_KEY_TYPE_IS_RSA(key_type)) {
		alg = PSA_ALG_RSA_PKCS1V15_SIGN(hash_type);
		sig = p_signature;
	}
	else if (PSA_KEY_TYPE_IS_ECC(key_type)) {
		alg = PSA_ALG_ECDSA(hash_type);
		// Historically, mbedTLS used DER format for ECDA signatures.
		// We need to convert the input from DER to raw before returning it for compatility.
		size_t key_size = p_key->get_data()->pk_size;
		size_t out_size = 0;
		uint8_t raw_sig[PSA_SIGNATURE_MAX_SIZE];
		int ret = godot_mbedtls_ecdsa_der_to_raw(
			key_size, p_signature.ptr(), p_signature.size(), raw_sig, sizeof(raw_sig), &out_size);
		if (ret) {
			return false;
		}
		sig.resize(out_size);
		memcpy(sig.ptrw(), raw_sig, out_size);
	}
	ERR_FAIL_COND_V_MSG(alg == PSA_ALG_NONE, false, "Unknown key type: " + itos(key_type));
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_VERIFY_HASH, alg);
	int ret = psa_verify_hash(pk, alg, p_hash.ptr(), p_hash.size(), sig.ptr(), sig.size());
	psa_destroy_key(pk);
	return ret == PSA_SUCCESS;
}

Vector<uint8_t> Crypto::encrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_plaintext)
{
	ERR_FAIL_COND_V_MSG(p_key.is_null(), Vector<uint8_t>(), "Invalid key provided.");
	uint8_t buf[PSA_ASYMMETRIC_ENCRYPT_OUTPUT_MAX_SIZE];
	size_t size;
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_ENCRYPT, PSA_ALG_RSA_PKCS1V15_CRYPT);
	int ret = psa_asymmetric_encrypt(pk, PSA_ALG_RSA_PKCS1V15_CRYPT, p_plaintext.ptr(),
		p_plaintext.size(), nullptr, 0, buf, sizeof(buf), &size);
	psa_destroy_key(pk);
	ERR_FAIL_COND_V_MSG(ret, Vector<uint8_t>(), "Error while encrypting: " + itos(ret));
	Vector<uint8_t> out;
	out.resize(size);
	memcpy(out.ptrw(), buf, size);
	return out;
}

Vector<uint8_t> Crypto::decrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_ciphertext)
{
	ERR_FAIL_COND_V_MSG(p_key.is_null(), Vector<uint8_t>(), "Invalid key provided.");
	ERR_FAIL_COND_V_MSG(p_key->is_public_only(), Vector<uint8_t>(),
		"Invalid key provided. Cannot decrypt using a public_only key.");
	psa_key_id_t pk = p_key->get_data()->reimport(PSA_KEY_USAGE_DECRYPT, PSA_ALG_RSA_PKCS1V15_CRYPT);
	uint8_t buf[PSA_ASYMMETRIC_DECRYPT_OUTPUT_MAX_SIZE];
	size_t size;
	int ret = psa_asymmetric_decrypt(pk, PSA_ALG_RSA_PKCS1V15_CRYPT, p_ciphertext.ptr(),
		p_ciphertext.size(), nullptr, 0, buf, sizeof(buf), &size);
	psa_destroy_key(pk);
	ERR_FAIL_COND_V_MSG(ret, Vector<uint8_t>(), "Error while decrypting: " + itos(ret));
	Vector<uint8_t> out;
	out.resize(size);
	memcpy(out.ptrw(), buf, size);
	return out;
}

/// CookieContextMbedTLS & TLSContextMbedTLS

static void my_debug(void* ctx, int level, const char* file, int line, const char* str)
{
	printf("%s:%04d: %s", file, line, str);
	fflush(stdout);
}

void TLSContextMbedTLS::print_mbedtls_error(int p_ret)
{
	printf("mbedtls error: returned -0x%x\n\n", -p_ret);
	fflush(stdout);
}

Error CookieContextMbedTLS::setup()
{
	ERR_FAIL_COND_V_MSG(inited, ERR_ALREADY_IN_USE, "This cookie context is already in use");

	mbedtls_ssl_cookie_init(&cookie_ctx);
	inited = true;

	int ret = mbedtls_ssl_cookie_setup(GODOT_MBEDTLS_COMPAT_ARGS(&cookie_ctx));
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(FAILED, "mbedtls_ssl_cookie_setup returned an error " + itos(ret));
	}
	return OK;
}

void CookieContextMbedTLS::clear()
{
	if (!inited) {
		return;
	}
	mbedtls_ssl_cookie_free(&cookie_ctx);
	inited = false;
}

CookieContextMbedTLS::CookieContextMbedTLS() {}

CookieContextMbedTLS::~CookieContextMbedTLS() { clear(); }

Error TLSContextMbedTLS::_setup(int p_endpoint, int p_transport, int p_authmode)
{
	ERR_FAIL_COND_V_MSG(inited, ERR_ALREADY_IN_USE, "This SSL context is already active");

	mbedtls_ssl_init(&tls);
	mbedtls_ssl_config_init(&conf);
	mbedtls_pk_init(&pk);
	inited = true;

	int ret =
		mbedtls_ssl_config_defaults(&conf, p_endpoint, p_transport, MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(FAILED, "mbedtls_ssl_config_defaults returned an error" + itos(ret));
	}
	mbedtls_ssl_conf_authmode(&conf, p_authmode);
	GODOT_MBEDTLS_COMPAT_SSL_CONF;
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
	return OK;
}

Error TLSContextMbedTLS::init_server(
	int p_transport, Ref<TLSOptions> p_options, Ref<CookieContextMbedTLS> p_cookies)
{
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);

	// Check key and certificate(s)
	Ref<CryptoKey> pkey = p_options->get_private_key();
	certs = p_options->get_own_certificate();
	ERR_FAIL_COND_V(pkey.is_null() || certs.is_null(), ERR_INVALID_PARAMETER);

	Error err =
		_setup(MBEDTLS_SSL_IS_SERVER, p_transport, MBEDTLS_SSL_VERIFY_NONE); // TODO client auth.
	ERR_FAIL_COND_V(err != OK, err);

	// Locking key and certificate(s)
	certs->get_data()->lock();

	// Adding key and certificate
	int ret = mbedtls_pk_copy_from_psa(pkey->get_data()->pk_slot, &pk);
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER, "Failed to import key " + itos(ret));
	}
	ret = mbedtls_ssl_conf_own_cert(&conf, &certs->get_data()->cert, &pk);
	if (ret != 0) {
		clear();
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER, "Invalid cert/key combination " + itos(ret));
	}
	// Adding CA chain if available.
	if (certs->get_data()->cert.next) {
		mbedtls_ssl_conf_ca_chain(&conf, certs->get_data()->cert.next, nullptr);
	}
	// DTLS Cookies
	if (p_transport == MBEDTLS_SSL_TRANSPORT_DATAGRAM) {
		if (p_cookies.is_null() || !p_cookies->inited) {
			clear();
			ERR_FAIL_V(ERR_BUG);
		}
		cookies = p_cookies;
		mbedtls_ssl_conf_dtls_cookies(
			&conf, mbedtls_ssl_cookie_write, mbedtls_ssl_cookie_check, &(cookies->cookie_ctx));
	}

	mbedtls_ssl_setup(&tls, &conf);
	return OK;
}

Error TLSContextMbedTLS::init_client(
	int p_transport, const String& p_hostname, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_options.is_null() || p_options->is_server(), ERR_INVALID_PARAMETER);

	int authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
	bool unsafe = p_options->is_unsafe_client();
	if (unsafe && p_options->get_trusted_ca_chain().is_null()) {
		authmode = MBEDTLS_SSL_VERIFY_NONE;
	}

	Error err = _setup(MBEDTLS_SSL_IS_CLIENT, p_transport, authmode);
	ERR_FAIL_COND_V(err != OK, err);

	if (unsafe) {
		// No hostname verification for unsafe clients.
		mbedtls_ssl_set_hostname(&tls, nullptr);
	}
	else {
		String cn = p_options->get_common_name_override();
		if (cn.is_empty()) {
			cn = p_hostname;
		}
		mbedtls_ssl_set_hostname(&tls, cn.utf8().get_data());
	}

	Ref<X509Certificate> cas;

	if (p_options->get_trusted_ca_chain().is_valid()) {
		// Locking CA certificates
		certs = p_options->get_trusted_ca_chain();
		certs->get_data()->lock();
		cas = certs;
	}
	else {
		// Fall back to default certificates (no need to lock those).
		cas = MbedTLSModule::get_default_certificates();
		if (cas.is_null()) {
			clear();
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "SSL module failed to initialize!");
		}
	}

	// Set valid CAs
	mbedtls_ssl_conf_ca_chain(&conf, &cas->get_data()->cert, nullptr);
	mbedtls_ssl_setup(&tls, &conf);
	return OK;
}

void TLSContextMbedTLS::clear()
{
	if (!inited) {
		return;
	}
	mbedtls_ssl_free(&tls);
	mbedtls_ssl_config_free(&conf);
	mbedtls_pk_free(&pk);
	// Unlock certificates
	if (certs.is_valid()) {
		certs->get_data()->unlock();
	}
	certs = Ref<X509Certificate>();
	cookies = Ref<CookieContextMbedTLS>();
	inited = false;
}

mbedtls_ssl_context* TLSContextMbedTLS::get_context()
{
	ERR_FAIL_COND_V(!inited, nullptr);
	return &tls;
}

TLSContextMbedTLS::TLSContextMbedTLS() {}

TLSContextMbedTLS::~TLSContextMbedTLS() { clear(); }

/// StreamPeerTLS

int StreamPeerTLS::Data::bio_send(void* ctx, const unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	StreamPeerTLS::Data* sp = static_cast<StreamPeerTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	int sent;
	Error err = sp->base->put_partial_data((const uint8_t*)buf, len, sent);
	if (err != OK) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	if (sent == 0) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}
	return sent;
}

int StreamPeerTLS::Data::bio_recv(void* ctx, unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	StreamPeerTLS::Data* sp = static_cast<StreamPeerTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	int got;
	Error err = sp->base->get_partial_data((uint8_t*)buf, len, got);
	if (err != OK) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	if (got == 0) {
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	return got;
}

void StreamPeerTLS::Data::cleanup()
{
	tls_ctx->clear();
	base = Ref<StreamPeer>();
	status = STATUS_DISCONNECTED;
}

Error StreamPeerTLS::Data::do_handshake(StreamPeerTLS* p_owner)
{
	int ret = mbedtls_ssl_handshake(tls_ctx->get_context());
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		// Handshake is still in progress, will retry via poll later.
		return OK;
	}
	else if (ret != 0) {
		// An error occurred.
		ERR_PRINT("TLS handshake error: " + itos(ret));
		TLSContextMbedTLS::print_mbedtls_error(ret);
		p_owner->disconnect_from_stream();
		status = STATUS_ERROR;
		return FAILED;
	}

	status = STATUS_CONNECTED;
	return OK;
}

Error StreamPeerTLS::connect_to_stream(
	Ref<StreamPeer> p_base, const String& p_common_name, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_base.is_null(), ERR_INVALID_PARAMETER);

	Error err = data->tls_ctx->init_client(MBEDTLS_SSL_TRANSPORT_STREAM, p_common_name,
		p_options.is_valid() ? p_options : TLSOptions::client());
	ERR_FAIL_COND_V(err != OK, err);

	data->base = p_base;
	mbedtls_ssl_set_bio(data->tls_ctx->get_context(), data, Data::bio_send, Data::bio_recv, nullptr);

	data->status = STATUS_HANDSHAKING;

	if (data->do_handshake(this) != OK) {
		data->status = STATUS_ERROR_HOSTNAME_MISMATCH;
		return FAILED;
	}

	return OK;
}

Error StreamPeerTLS::accept_stream(Ref<StreamPeer> p_base, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_base.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);

	Error err = data->tls_ctx->init_server(MBEDTLS_SSL_TRANSPORT_STREAM, p_options);
	ERR_FAIL_COND_V(err != OK, err);

	data->base = p_base;

	mbedtls_ssl_set_bio(data->tls_ctx->get_context(), data, Data::bio_send, Data::bio_recv, nullptr);

	data->status = STATUS_HANDSHAKING;

	if (data->do_handshake(this) != OK) {
		return FAILED;
	}

	data->status = STATUS_CONNECTED;
	return OK;
}

Error StreamPeerTLS::put_data(const uint8_t* p_data, int p_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	Error err;
	int sent = 0;

	while (p_bytes > 0) {
		err = put_partial_data(p_data, p_bytes, sent);

		if (err != OK) {
			return err;
		}

		p_data += sent;
		p_bytes -= sent;
	}

	return OK;
}

Error StreamPeerTLS::put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	r_sent = 0;

	if (p_bytes == 0) {
		return OK;
	}

	do {
		int ret =
			mbedtls_ssl_write(data->tls_ctx->get_context(), &p_data[r_sent], p_bytes - r_sent);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			// Non blocking IO.
			break;
		}
		else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Clean close
			disconnect_from_stream();
			return ERR_FILE_EOF;
		}
		else if (ret <= 0) {
			TLSContextMbedTLS::print_mbedtls_error(ret);
			disconnect_from_stream();
			return ERR_CONNECTION_ERROR;
		}
		r_sent += ret;

	} while (r_sent < p_bytes);

	return OK;
}

Error StreamPeerTLS::get_data(uint8_t* p_buffer, int p_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	Error err;

	int got = 0;
	while (p_bytes > 0) {
		err = get_partial_data(p_buffer, p_bytes, got);

		if (err != OK) {
			return err;
		}

		p_buffer += got;
		p_bytes -= got;
	}

	return OK;
}

Error StreamPeerTLS::get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	r_received = 0;

	do {
		int ret = mbedtls_ssl_read(
			data->tls_ctx->get_context(), &p_buffer[r_received], p_bytes - r_received);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			// Non blocking IO.
			break;
		}
		else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Clean close
			disconnect_from_stream();
			return ERR_FILE_EOF;
		}
		else if (ret == MBEDTLS_ERR_SSL_RECEIVED_NEW_SESSION_TICKET) {
			continue; // Re-connection cookies are not currently supported, ignore them.
		}
		else if (ret <= 0) {
			TLSContextMbedTLS::print_mbedtls_error(ret);
			disconnect_from_stream();
			return ERR_CONNECTION_ERROR;
		}

		r_received += ret;

	} while (r_received < p_bytes);

	return OK;
}

void StreamPeerTLS::poll()
{
	ERR_FAIL_COND(data->status != STATUS_CONNECTED && data->status != STATUS_HANDSHAKING);
	ERR_FAIL_COND(data->base.is_null());

	if (data->status == STATUS_HANDSHAKING) {
		data->do_handshake(this);
		return;
	}

	// We could pass nullptr as second parameter, but some behavior sanitizers don't seem to like
	// that. Passing a 1 byte buffer to workaround it.
	uint8_t byte;
	int ret = mbedtls_ssl_read(data->tls_ctx->get_context(), &byte, 0);

	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		// Nothing to read/write (non blocking IO)
	}
	else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
		// Clean close (disconnect)
		disconnect_from_stream();
		return;
	}
	else if (ret < 0) {
		TLSContextMbedTLS::print_mbedtls_error(ret);
		disconnect_from_stream();
		return;
	}

	Ref<StreamPeerSocket> tcp = data->base;
	if (tcp.is_valid() && tcp->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		disconnect_from_stream();
		return;
	}
}

int StreamPeerTLS::get_available_bytes() const
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, 0);

	return mbedtls_ssl_get_bytes_avail(data->tls_ctx->get_context());
}

StreamPeerTLS::StreamPeerTLS()
{
	data = memnew(Data);
}

StreamPeerTLS::~StreamPeerTLS()
{
	if (data) {
		disconnect_from_stream();
		memdelete(data);
		data = nullptr;
	}
}

void StreamPeerTLS::disconnect_from_stream()
{
	if (data->status != STATUS_CONNECTED && data->status != STATUS_HANDSHAKING) {
		return;
	}

	Ref<StreamPeerSocket> tcp = data->base;
	if (tcp.is_valid() && tcp->get_status() == StreamPeerSocket::STATUS_CONNECTED) {
		// We are still connected on the socket, try to send close notify.
		mbedtls_ssl_close_notify(data->tls_ctx->get_context());
	}

	data->cleanup();
}

StreamPeerTLS::Status StreamPeerTLS::get_status() const
{
	return data->status;
}

Ref<StreamPeer> StreamPeerTLS::get_stream() const
{
	return data->base;
}

StreamPeerTLS* StreamPeerTLS::create(bool p_notify_postinitialize)
{
	return memnew(StreamPeerTLS);
}

bool StreamPeerTLS::is_available()
{
	return true;
}

/// PacketPeerDTLS

int PacketPeerDTLS::Data::bio_send(void* ctx, const unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	PacketPeerDTLS::Data* sp = static_cast<PacketPeerDTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	Error err = sp->base->put_packet((const uint8_t*)buf, len);
	if (err == ERR_BUSY) {
		return MBEDTLS_ERR_SSL_WANT_WRITE;
	}
	else if (err != OK) {
		ERR_FAIL_V(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
	}
	return len;
}

int PacketPeerDTLS::Data::bio_recv(void* ctx, unsigned char* buf, size_t len)
{
	if (buf == nullptr || len == 0) {
		return 0;
	}

	PacketPeerDTLS::Data* sp = static_cast<PacketPeerDTLS::Data*>(ctx);

	ERR_FAIL_NULL_V(sp, 0);

	int pc = sp->base->get_available_packet_count();
	if (pc == 0) {
		return MBEDTLS_ERR_SSL_WANT_READ;
	}
	else if (pc < 0) {
		ERR_FAIL_V(MBEDTLS_ERR_SSL_INTERNAL_ERROR);
	}

	const uint8_t* buffer;
	int buffer_size = 0;
	Error err = sp->base->get_packet(&buffer, buffer_size);
	if (err != OK) {
		return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
	}
	memcpy(buf, buffer, buffer_size);
	return buffer_size;
}

void PacketPeerDTLS::Data::cleanup()
{
	tls_ctx->clear();
	base = Ref<PacketPeerUDP>();
	status = STATUS_DISCONNECTED;
}

int PacketPeerDTLS::Data::set_cookie()
{
	// Setup DTLS session cookie for this client
	uint8_t client_id[18];
	IPAddress addr = base->get_packet_address();
	uint16_t port = base->get_packet_port();
	memcpy(client_id, addr.get_ipv6(), 16);
	memcpy(&client_id[16], (uint8_t*)&port, 2);
	return mbedtls_ssl_set_client_transport_id(tls_ctx->get_context(), client_id, 18);
}

Error PacketPeerDTLS::Data::do_handshake()
{
	int ret = 0;
	while ((ret = mbedtls_ssl_handshake(tls_ctx->get_context())) != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			if (ret != MBEDTLS_ERR_SSL_HELLO_VERIFY_REQUIRED) {
				ERR_PRINT("TLS handshake error: " + itos(ret));
				TLSContextMbedTLS::print_mbedtls_error(ret);
			}
			cleanup();
			status = STATUS_ERROR;
			return FAILED;
		}
		// Will retry via poll later
		return OK;
	}

	status = STATUS_CONNECTED;
	return OK;
}

Error PacketPeerDTLS::connect_to_peer(
	Ref<PacketPeerUDP> p_base, const String& p_hostname, Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_base.is_null() || !p_base->is_socket_connected(), ERR_INVALID_PARAMETER);

	Error err = data->tls_ctx->init_client(MBEDTLS_SSL_TRANSPORT_DATAGRAM, p_hostname,
		p_options.is_valid() ? p_options : TLSOptions::client());
	ERR_FAIL_COND_V(err != OK, err);

	data->base = p_base;

	mbedtls_ssl_set_bio(data->tls_ctx->get_context(), data, Data::bio_send, Data::bio_recv, nullptr);
	mbedtls_ssl_set_timer_cb(
		data->tls_ctx->get_context(), &data->timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	data->status = STATUS_HANDSHAKING;

	if (data->do_handshake() != OK) {
		data->status = STATUS_ERROR_HOSTNAME_MISMATCH;
		return FAILED;
	}

	return OK;
}

Error PacketPeerDTLS::Data::accept_peer(
	Ref<PacketPeerUDP> p_base, Ref<TLSOptions> p_options, Ref<CookieContextMbedTLS> p_cookies)
{
	ERR_FAIL_COND_V(p_base.is_null() || !p_base->is_socket_connected(), ERR_INVALID_PARAMETER);

	Error err = tls_ctx->init_server(MBEDTLS_SSL_TRANSPORT_DATAGRAM, p_options, p_cookies);
	ERR_FAIL_COND_V(err != OK, err);

	base = p_base;
	base->set_blocking_mode(false);

	mbedtls_ssl_session_reset(tls_ctx->get_context());

	int ret = set_cookie();
	if (ret != 0) {
		cleanup();
		ERR_FAIL_V_MSG(FAILED, "Error setting DTLS client cookie");
	}

	mbedtls_ssl_set_bio(tls_ctx->get_context(), this, bio_send, bio_recv, nullptr);
	mbedtls_ssl_set_timer_cb(
		tls_ctx->get_context(), &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay);

	status = STATUS_HANDSHAKING;

	if (do_handshake() != OK) {
		status = STATUS_ERROR;
		return FAILED;
	}

	return OK;
}

Error PacketPeerDTLS::put_packet(const uint8_t* p_buffer, int p_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	if (p_bytes == 0) {
		return OK;
	}

	int ret = mbedtls_ssl_write(data->tls_ctx->get_context(), p_buffer, p_bytes);
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		// Non blocking io.
	}
	else if (ret <= 0) {
		TLSContextMbedTLS::print_mbedtls_error(ret);
		data->cleanup();
		return ERR_CONNECTION_ERROR;
	}

	return OK;
}

Error PacketPeerDTLS::get_packet(const uint8_t** r_buffer, int& r_bytes)
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, ERR_UNCONFIGURED);

	r_bytes = 0;

	int ret = mbedtls_ssl_read(
		data->tls_ctx->get_context(), data->packet_buffer, Data::PACKET_BUFFER_SIZE);
	if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
		ret = 0; // non blocking io
	}
	else if (ret <= 0) {
		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Also send close notify back
			disconnect_from_peer();
		}
		else {
			data->cleanup();
			data->status = STATUS_ERROR;
			TLSContextMbedTLS::print_mbedtls_error(ret);
		}
		return ERR_CONNECTION_ERROR;
	}
	*r_buffer = data->packet_buffer;
	r_bytes = ret;

	return OK;
}

void PacketPeerDTLS::poll()
{
	if (data->status == STATUS_HANDSHAKING) {
		data->do_handshake();
		return;
	}
	else if (data->status != STATUS_CONNECTED) {
		return;
	}

	ERR_FAIL_COND(data->base.is_null());

	int ret = mbedtls_ssl_read(data->tls_ctx->get_context(), nullptr, 0);

	if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			// Also send close notify back
			disconnect_from_peer();
		}
		else {
			data->cleanup();
			data->status = STATUS_ERROR;
			TLSContextMbedTLS::print_mbedtls_error(ret);
		}
	}
}

int PacketPeerDTLS::get_available_packet_count() const
{
	ERR_FAIL_COND_V(data->status != STATUS_CONNECTED, 0);

	return mbedtls_ssl_get_bytes_avail(data->tls_ctx->get_context()) > 0 ? 1 : 0;
}

int PacketPeerDTLS::get_max_packet_size() const
{
	return 488; // 512 (UDP in Godot) - 24 (DTLS header)
}

PacketPeerDTLS::PacketPeerDTLS()
{
	data = memnew(Data);
}

PacketPeerDTLS::~PacketPeerDTLS()
{
	if (data) {
		disconnect_from_peer();
		memdelete(data);
		data = nullptr;
	}
}

void PacketPeerDTLS::disconnect_from_peer()
{
	if (data->status != STATUS_CONNECTED && data->status != STATUS_HANDSHAKING) {
		return;
	}

	if (data->status == STATUS_CONNECTED) {
		int ret = 0;
		// Send SSL close notification, blocking, but ignore other errors.
		do {
			ret = mbedtls_ssl_close_notify(data->tls_ctx->get_context());
		} while (ret == MBEDTLS_ERR_SSL_WANT_WRITE);
	}

	data->cleanup();
}

PacketPeerDTLS::Status PacketPeerDTLS::get_status() const
{
	return data->status;
}

PacketPeerDTLS* PacketPeerDTLS::create(bool p_notify_postinitialize)
{
	return memnew(PacketPeerDTLS);
}

bool PacketPeerDTLS::is_available()
{
	return true;
}

/// DTLSServer

Error DTLSServer::setup(Ref<TLSOptions> p_options)
{
	ERR_FAIL_COND_V(p_options.is_null() || !p_options->is_server(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(data->cookies->setup() != OK, ERR_ALREADY_IN_USE);
	data->tls_options = p_options;
	return OK;
}

void DTLSServer::stop()
{
	data->cookies->clear();
}

Ref<PacketPeerDTLS> DTLSServer::take_connection(Ref<PacketPeerUDP> p_udp_peer)
{
	Ref<PacketPeerDTLS> out;

	ERR_FAIL_COND_V(data->tls_options.is_null(), out);
	ERR_FAIL_COND_V(p_udp_peer.is_null(), out);

	out.instantiate();
	out->get_data()->accept_peer(p_udp_peer, data->tls_options, data->cookies);
	return out;
}

DTLSServer* DTLSServer::create(bool p_notify_postinitialize)
{
	return memnew(DTLSServer);
}

bool DTLSServer::is_available()
{
	return true;
}

DTLSServer::DTLSServer()
{
	data = memnew(Data);
}

DTLSServer::~DTLSServer()
{
	if (data) {
		stop();
		memdelete(data);
		data = nullptr;
	}
}

/// Module registration

static bool godot_mbedtls_initialized = false;

void initialize_mbedtls_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

#ifdef DEBUG_ENABLED
	if (OS::is_stdout_verbose()) {
		mbedtls_debug_set_threshold(1);
	}
#endif

	godot_mbedtls_initialized = true;

	MbedTLSModule::initialize_crypto();
}

void uninitialize_mbedtls_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_CORE) {
		return;
	}

	if (!godot_mbedtls_initialized) {
		return;
	}

	MbedTLSModule::finalize_crypto();
	godot_mbedtls_initialized = false;
}
