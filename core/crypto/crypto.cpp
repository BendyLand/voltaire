/**************************************************************************/
/*  crypto.cpp                                                            */
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

#include "crypto.h"

#include "modules/modules_enabled.gen.h"

Ref<TLSOptions> TLSOptions::client(
	Ref<X509Certificate> p_trusted_chain, const String& p_common_name_override)
{
	Ref<TLSOptions> opts;
	opts.instantiate();
	opts->mode = MODE_CLIENT;
	opts->trusted_ca_chain = p_trusted_chain;
	opts->common_name = p_common_name_override;
	return opts;
}

Ref<TLSOptions> TLSOptions::client_unsafe(Ref<X509Certificate> p_trusted_chain)
{
	Ref<TLSOptions> opts;
	opts.instantiate();
	opts->mode = MODE_CLIENT_UNSAFE;
	opts->trusted_ca_chain = p_trusted_chain;
	return opts;
}

Ref<TLSOptions> TLSOptions::server(Ref<CryptoKey> p_own_key, Ref<X509Certificate> p_own_certificate)
{
	Ref<TLSOptions> opts;
	opts.instantiate();
	opts->mode = MODE_SERVER;
	opts->own_certificate = p_own_certificate;
	opts->private_key = p_own_key;
	return opts;
}

Vector<uint8_t> Crypto::hmac_digest(HashingContext::HashType p_hash_type,
	const Vector<uint8_t>& p_key, const Vector<uint8_t>& p_msg)
{
	Ref<HMACContext> ctx = Ref<HMACContext>(HMACContext::create());
	ERR_FAIL_COND_V_MSG(
		ctx.is_null(), Vector<uint8_t>(), "HMAC is not available without mbedtls module.");
	Error err = ctx->start(p_hash_type, p_key);
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	err = ctx->update(p_msg);
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	return ctx->finish();
}

// Compares two HMACS for equality without leaking timing information in order to prevent timing
// attacks.
// @see:
// https://paragonie.com/blog/2015/11/preventing-timing-attacks-on-string-comparison-with-double-hmac-strategy
bool Crypto::constant_time_compare(
	const Vector<uint8_t>& p_trusted, const Vector<uint8_t>& p_received)
{
	const uint8_t* t = p_trusted.ptr();
	const uint8_t* r = p_received.ptr();
	int tlen = p_trusted.size();
	int rlen = p_received.size();
	// If the lengths are different then nothing else matters.
	if (tlen != rlen) {
		return false;
	}

	uint8_t v = 0;
	for (int i = 0; i < tlen; i++) {
		v |= t[i] ^ r[i];
	}
	return v == 0;
}

#ifndef MODULE_MBEDTLS_ENABLED

CryptoKey* CryptoKey::create(bool p_notify_postinitialize) { return nullptr; }
bool CryptoKey::is_available() { return false; }
Error CryptoKey::load(const String& p_path, bool p_public_only) { return ERR_UNAVAILABLE; }
Error CryptoKey::save(const String& p_path, bool p_public_only) { return ERR_UNAVAILABLE; }
String CryptoKey::save_to_string(bool p_public_only) { return String(); }
Error CryptoKey::load_from_string(const String& p_string_key, bool p_public_only)
{
	return ERR_UNAVAILABLE;
}
bool CryptoKey::is_public_only() const { return true; }
CryptoKey::CryptoKey() {}
CryptoKey::~CryptoKey() {}

X509Certificate* X509Certificate::create(bool p_notify_postinitialize) { return nullptr; }
bool X509Certificate::is_available() { return false; }
Error X509Certificate::load(const String& p_path) { return ERR_UNAVAILABLE; }
Error X509Certificate::load_from_memory(const uint8_t* p_buffer, int p_len)
{
	return ERR_UNAVAILABLE;
}
Error X509Certificate::save(const String& p_path) { return ERR_UNAVAILABLE; }
String X509Certificate::save_to_string() { return String(); }
Error X509Certificate::load_from_string(const String& p_string) { return ERR_UNAVAILABLE; }
X509Certificate::X509Certificate() {}
X509Certificate::~X509Certificate() {}

HMACContext* HMACContext::create(bool p_notify_postinitialize)
{
	ERR_FAIL_V_MSG(nullptr, "HMACContext is not available when the mbedtls module is disabled.");
}
bool HMACContext::is_available() { return false; }
Error HMACContext::start(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_key)
{
	return ERR_UNAVAILABLE;
}
Error HMACContext::update(const Vector<uint8_t>& p_data) { return ERR_UNAVAILABLE; }
Vector<uint8_t> HMACContext::finish() { return Vector<uint8_t>(); }
HMACContext::HMACContext() {}
HMACContext::~HMACContext() {}

Crypto* Crypto::create(bool p_notify_postinitialize)
{
	ERR_FAIL_V_MSG(nullptr, "Crypto is not available when the mbedtls module is disabled.");
}
bool Crypto::is_available() { return false; }
void Crypto::load_default_certificates(const String& p_path) {}
Vector<uint8_t> Crypto::generate_random_bytes(int p_bytes) { return Vector<uint8_t>(); }
Ref<CryptoKey> Crypto::generate_rsa(int p_bytes) { return nullptr; }
Ref<X509Certificate> Crypto::generate_self_signed_certificate(Ref<CryptoKey> p_key,
	const String& p_issuer_name, const String& p_not_before, const String& p_not_after)
{
	return nullptr;
}
Vector<uint8_t> Crypto::sign(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash,
	Ref<CryptoKey> p_key)
{
	return Vector<uint8_t>();
}
bool Crypto::verify(HashingContext::HashType p_hash_type, const Vector<uint8_t>& p_hash,
	const Vector<uint8_t>& p_signature, Ref<CryptoKey> p_key)
{
	return false;
}
Vector<uint8_t> Crypto::encrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_plaintext)
{
	return Vector<uint8_t>();
}
Vector<uint8_t> Crypto::decrypt(Ref<CryptoKey> p_key, const Vector<uint8_t>& p_ciphertext)
{
	return Vector<uint8_t>();
}
Crypto::Crypto() {}
Crypto::~Crypto() {}

#endif // !MODULE_MBEDTLS_ENABLED

// ==========================================================================
// crypto_core.cpp
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

#include "crypto_core.h"

#include "core/os/os.h"
#include "core/string/ustring.h"

#include <mbedtls/base64.h>
#include <psa/crypto.h>

// MD5
CryptoCore::MD5Context::MD5Context() {
	ctx = memalloc_zeroed(sizeof(psa_hash_operation_t));
}

CryptoCore::MD5Context::~MD5Context() {
	psa_hash_abort((psa_hash_operation_t *)ctx);
	memfree((psa_hash_operation_t *)ctx);
}

Error CryptoCore::MD5Context::start() {
	int ret = psa_hash_setup((psa_hash_operation_t *)ctx, PSA_ALG_MD5);
	return ret ? FAILED : OK;
}

Error CryptoCore::MD5Context::update(const uint8_t *p_src, size_t p_len) {
	int ret = psa_hash_update((psa_hash_operation_t *)ctx, p_src, p_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::MD5Context::finish(unsigned char r_hash[16]) {
	size_t size = 0;
	int ret = psa_hash_finish((psa_hash_operation_t *)ctx, r_hash, 16, &size);
	if (ret) {
		psa_hash_abort((psa_hash_operation_t *)ctx);
		return FAILED;
	}
	ERR_FAIL_COND_V(size != 16, ERR_BUG);
	return OK;
}

// SHA1
CryptoCore::SHA1Context::SHA1Context() {
	ctx = memalloc_zeroed(sizeof(psa_hash_operation_t));
}

CryptoCore::SHA1Context::~SHA1Context() {
	psa_hash_abort((psa_hash_operation_t *)ctx);
	memfree((psa_hash_operation_t *)ctx);
}

Error CryptoCore::SHA1Context::start() {
	int ret = psa_hash_setup((psa_hash_operation_t *)ctx, PSA_ALG_SHA_1);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA1Context::update(const uint8_t *p_src, size_t p_len) {
	int ret = psa_hash_update((psa_hash_operation_t *)ctx, p_src, p_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA1Context::finish(unsigned char r_hash[20]) {
	size_t size = 0;
	int ret = psa_hash_finish((psa_hash_operation_t *)ctx, r_hash, 20, &size);
	if (ret) {
		psa_hash_abort((psa_hash_operation_t *)ctx);
		return FAILED;
	}
	ERR_FAIL_COND_V(size != 20, ERR_BUG);
	return OK;
}

// SHA256
CryptoCore::SHA256Context::SHA256Context() {
	ctx = memalloc_zeroed(sizeof(psa_hash_operation_t));
}

CryptoCore::SHA256Context::~SHA256Context() {
	psa_hash_abort((psa_hash_operation_t *)ctx);
	memfree((psa_hash_operation_t *)ctx);
}

Error CryptoCore::SHA256Context::start() {
	int ret = psa_hash_setup((psa_hash_operation_t *)ctx, PSA_ALG_SHA_256);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA256Context::update(const uint8_t *p_src, size_t p_len) {
	int ret = psa_hash_update((psa_hash_operation_t *)ctx, p_src, p_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::SHA256Context::finish(unsigned char r_hash[32]) {
	size_t size = 0;
	int ret = psa_hash_finish((psa_hash_operation_t *)ctx, r_hash, 32, &size);
	if (ret) {
		psa_hash_abort((psa_hash_operation_t *)ctx);
		return FAILED;
	}
	ERR_FAIL_COND_V(size != 32, ERR_BUG);
	return OK;
}

// AES256
CryptoCore::AESContext::AESContext() {
	ctx = memalloc_zeroed(sizeof(psa_cipher_operation_t));
}

CryptoCore::AESContext::~AESContext() {
	psa_cipher_abort((psa_cipher_operation_t *)ctx);
	psa_destroy_key(key_id);
	memfree((psa_cipher_operation_t *)ctx);
}

void CryptoCore::AESContext::reset() {
	psa_cipher_abort((psa_cipher_operation_t *)ctx);
	psa_destroy_key(key_id);
	key_id = PSA_KEY_ID_NULL;
	alg = PSA_ALG_NONE;
}

Error CryptoCore::AESContext::setup(Mode p_mode, Cipher p_cipher, const uint8_t *p_key, size_t p_key_length, const uint8_t *p_iv, size_t p_iv_size) {
	reset();
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	switch (p_mode) {
		case Mode::NONE:
			return OK;
		case Mode::ENCRYPT:
		case Mode::DECRYPT:
			break;
		default:
			return ERR_INVALID_PARAMETER;
	}
	switch (p_cipher) {
		case Cipher::CBC:
			alg = PSA_ALG_CBC_NO_PADDING;
			break;
		case Cipher::ECB:
			alg = PSA_ALG_ECB_NO_PADDING;
			break;
		case Cipher::CFB:
			alg = PSA_ALG_CFB;
			break;
		default:
			return ERR_INVALID_PARAMETER;
	}
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attr, alg);
	psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attr, p_key_length << 3);
	int ret = psa_import_key(&attr, p_key, p_key_length, &key_id);
	if (ret) {
		return FAILED;
	}
	if (p_mode == Mode::ENCRYPT) {
		ret = psa_cipher_encrypt_setup((psa_cipher_operation_t *)ctx, key_id, alg);
	} else {
		ret = psa_cipher_decrypt_setup((psa_cipher_operation_t *)ctx, key_id, alg);
	}
	if (ret == PSA_SUCCESS && p_iv_size) {
		ret = psa_cipher_set_iv((psa_cipher_operation_t *)ctx, p_iv, p_iv_size);
	}
	if (ret) {
		reset();
		return FAILED;
	}
	return OK;
}

Error CryptoCore::AESContext::update(const uint8_t *p_src, size_t p_src_size, uint8_t *r_dst, size_t p_dst_size) {
	size_t size = 0;
	int ret = psa_cipher_update((psa_cipher_operation_t *)ctx, p_src, p_src_size, r_dst, p_dst_size, &size);
	if (ret) {
		return FAILED;
	}
	ERR_FAIL_COND_V(p_dst_size != size, ERR_BUG);
	return OK;
}

Error CryptoCore::AESContext::finish(uint8_t *r_dst, size_t p_dst_size) {
	size_t size = 0;
	int ret = psa_cipher_finish((psa_cipher_operation_t *)ctx, r_dst, p_dst_size, &size);
	reset();
	if (ret) {
		return FAILED;
	}
	ERR_FAIL_COND_V(p_dst_size != size, ERR_BUG);
	return OK;
}

// CryptoCore
Error CryptoCore::generate_random(uint8_t *r_buffer, size_t p_buffer_len) {
	if (unlikely(!initialized)) {
		// Fall back to raw get_entropy (might affect performance).
		return OS::get_entropy(r_buffer, p_buffer_len);
	}
	int ret = psa_generate_random(r_buffer, p_buffer_len);
	ERR_FAIL_COND_V(ret, FAILED);
	return OK;
}

String CryptoCore::b64_encode_str(const uint8_t *p_src, size_t p_src_len) {
	size_t b64len = p_src_len / 3 * 4 + 4 + 1;
	Vector<uint8_t> b64buff;
	b64buff.resize(b64len);
	uint8_t *w64 = b64buff.ptrw();
	size_t strlen = 0;
	int ret = b64_encode(&w64[0], b64len, &strlen, p_src, p_src_len);
	w64[strlen] = 0;
	return ret ? String() : (const char *)&w64[0];
}

Error CryptoCore::b64_encode(uint8_t *r_dst, size_t p_dst_len, size_t *r_len, const uint8_t *p_src, size_t p_src_len) {
	int ret = mbedtls_base64_encode(r_dst, p_dst_len, r_len, p_src, p_src_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::b64_decode(uint8_t *r_dst, size_t p_dst_len, size_t *r_len, const uint8_t *p_src, size_t p_src_len) {
	int ret = mbedtls_base64_decode(r_dst, p_dst_len, r_len, p_src, p_src_len);
	return ret ? FAILED : OK;
}

Error CryptoCore::md5(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[16]) {
	ERR_FAIL_COND_V(PSA_HASH_LENGTH(PSA_ALG_MD5) != 16, ERR_BUG);
	size_t size = 0;
	int ret = psa_hash_compute(PSA_ALG_MD5, p_src, p_src_len, r_hash, 16, &size);
	return ret ? FAILED : OK;
}

Error CryptoCore::sha1(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[20]) {
	ERR_FAIL_COND_V(PSA_HASH_LENGTH(PSA_ALG_SHA_1) != 20, ERR_BUG);
	size_t size = 0;
	int ret = psa_hash_compute(PSA_ALG_SHA_1, p_src, p_src_len, r_hash, 20, &size);
	return ret ? FAILED : OK;
}

Error CryptoCore::sha256(const uint8_t *p_src, size_t p_src_len, unsigned char r_hash[32]) {
	ERR_FAIL_COND_V(PSA_HASH_LENGTH(PSA_ALG_SHA_256) != 32, ERR_BUG);
	size_t size = 0;
	int ret = psa_hash_compute(PSA_ALG_SHA_256, p_src, p_src_len, r_hash, 32, &size);
	return ret ? FAILED : OK;
}

Error CryptoCore::encrypt_cfb(const uint8_t *p_src, uint8_t *r_dst, size_t p_length, const uint8_t *p_key, size_t p_key_length, const uint8_t p_iv[16]) {
	AESContext ctx;
	Error err = ctx.setup(AESContext::Mode::ENCRYPT, AESContext::Cipher::CFB, p_key, p_key_length, p_iv, 16);
	ERR_FAIL_COND_V(err, err);
	err = ctx.update(p_src, p_length, r_dst, p_length);
	ERR_FAIL_COND_V(err, err);
	err = ctx.finish(nullptr, 0);
	ERR_FAIL_COND_V(err, err);
	return OK;
}

Error CryptoCore::decrypt_cfb(const uint8_t *p_src, uint8_t *r_dst, size_t p_length, const uint8_t *p_key, size_t p_key_length, const uint8_t p_iv[16]) {
	AESContext ctx;
	Error err = ctx.setup(AESContext::Mode::DECRYPT, AESContext::Cipher::CFB, p_key, p_key_length, p_iv, 16);
	ERR_FAIL_COND_V(err, err);
	err = ctx.update(p_src, p_length, r_dst, p_length);
	ERR_FAIL_COND_V(err, err);
	err = ctx.finish(nullptr, 0);
	ERR_FAIL_COND_V(err, err);
	return OK;
}

bool CryptoCore::initialized = false;
void CryptoCore::initialize() {
	ERR_FAIL_COND(initialized);
#ifdef GODOT_MBEDTLS_PLATFORM
	godot_mbedtls_platform_init();
#endif

	int status = psa_crypto_init();
	ERR_FAIL_COND_MSG(status != PSA_SUCCESS, "Failed to initialize psa crypto. Cryptographic functions will not work.");
	initialized = true;
}
void CryptoCore::finalize() {
	if (!initialized) {
		return;
	}
	initialized = false;
	mbedtls_psa_crypto_free(); // Not part of PSA for reasons.
#ifdef GODOT_MBEDTLS_PLATFORM
	godot_mbedtls_platform_free();
#endif
}

#ifndef VOLTAIRE_BUILD
extern "C" {
psa_status_t psa_crypto_init(void) { return 0; }
psa_status_t psa_hash_abort(psa_hash_operation_t *operation) { return 0; }
psa_status_t psa_hash_setup(psa_hash_operation_t *operation, psa_algorithm_t alg) { return 0; }
psa_status_t psa_hash_update(psa_hash_operation_t *operation, const uint8_t *input, size_t input_length) { return 0; }
psa_status_t psa_hash_finish(psa_hash_operation_t *operation, uint8_t *hash, size_t hash_size, size_t *hash_length) { return 0; }
psa_status_t psa_hash_compute(psa_algorithm_t alg, const uint8_t *input, size_t input_length, uint8_t *hash, size_t hash_size, size_t *hash_length) { return 0; }
psa_status_t psa_cipher_abort(psa_cipher_operation_t *operation) { return 0; }
psa_status_t psa_destroy_key(psa_key_id_t key) { return 0; }
psa_status_t psa_import_key(const psa_key_attributes_t *attributes, const uint8_t *data, size_t data_length, psa_key_id_t *key) { return 0; }
psa_status_t psa_cipher_decrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key, psa_algorithm_t alg) { return 0; }
psa_status_t psa_cipher_encrypt_setup(psa_cipher_operation_t *operation, psa_key_id_t key, psa_algorithm_t alg) { return 0; }
psa_status_t psa_cipher_set_iv(psa_cipher_operation_t *operation, const uint8_t *iv, size_t iv_length) { return 0; }
psa_status_t psa_cipher_update(psa_cipher_operation_t *operation, const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size, size_t *output_length) { return 0; }
psa_status_t psa_cipher_finish(psa_cipher_operation_t *operation, uint8_t *output, size_t output_size, size_t *output_length) { return 0; }
psa_status_t psa_generate_random(uint8_t *output, size_t output_size) { return 0; }
void mbedtls_psa_crypto_free(void) {}
int mbedtls_base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen) { return 0; }
int mbedtls_base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen) { return 0; }
}
#endif

// ==========================================================================
// hashing_context.cpp
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
#include "hashing_context.h"

Error HashingContext::start(HashType p_type)
{
	ERR_FAIL_COND_V(ctx != nullptr, ERR_ALREADY_IN_USE);
	_create_ctx(p_type);
	ERR_FAIL_NULL_V(ctx, ERR_UNAVAILABLE);
	switch (type) {
	case HASH_MD5:
		return ((CryptoCore::MD5Context*)ctx)->start();
	case HASH_SHA1:
		return ((CryptoCore::SHA1Context*)ctx)->start();
	case HASH_SHA256:
		return ((CryptoCore::SHA256Context*)ctx)->start();
	}
	return ERR_UNAVAILABLE;
}

Error HashingContext::update(const Vector<uint8_t>& p_chunk)
{
	ERR_FAIL_NULL_V(ctx, ERR_UNCONFIGURED);
	size_t len = p_chunk.size();
	ERR_FAIL_COND_V(len == 0, FAILED);
	const uint8_t* r = p_chunk.ptr();
	switch (type) {
	case HASH_MD5:
		return ((CryptoCore::MD5Context*)ctx)->update(&r[0], len);
	case HASH_SHA1:
		return ((CryptoCore::SHA1Context*)ctx)->update(&r[0], len);
	case HASH_SHA256:
		return ((CryptoCore::SHA256Context*)ctx)->update(&r[0], len);
	}
	return ERR_UNAVAILABLE;
}

Vector<uint8_t> HashingContext::finish()
{
	ERR_FAIL_NULL_V(ctx, Vector<uint8_t>());
	Vector<uint8_t> out;
	Error err = FAILED;
	switch (type) {
	case HASH_MD5:
		out.resize(16);
		err = ((CryptoCore::MD5Context*)ctx)->finish(out.ptrw());
		break;
	case HASH_SHA1:
		out.resize(20);
		err = ((CryptoCore::SHA1Context*)ctx)->finish(out.ptrw());
		break;
	case HASH_SHA256:
		out.resize(32);
		err = ((CryptoCore::SHA256Context*)ctx)->finish(out.ptrw());
		break;
	}
	_delete_ctx();
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	return out;
}

void HashingContext::_create_ctx(HashType p_type)
{
	type = p_type;
	switch (type) {
	case HASH_MD5:
		ctx = memnew(CryptoCore::MD5Context);
		break;
	case HASH_SHA1:
		ctx = memnew(CryptoCore::SHA1Context);
		break;
	case HASH_SHA256:
		ctx = memnew(CryptoCore::SHA256Context);
		break;
	default:
		ctx = nullptr;
	}
}

void HashingContext::_delete_ctx()
{
	switch (type) {
	case HASH_MD5:
		memdelete((CryptoCore::MD5Context*)ctx);
		break;
	case HASH_SHA1:
		memdelete((CryptoCore::SHA1Context*)ctx);
		break;
	case HASH_SHA256:
		memdelete((CryptoCore::SHA256Context*)ctx);
		break;
	}
	ctx = nullptr;
}

HashingContext::~HashingContext()
{
	if (ctx != nullptr) {
		_delete_ctx();
	}
}

// ==========================================================================
// aes_context.cpp
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

#include "aes_context.h"

using AESMode = CryptoCore::AESContext::Mode;
using AESCipher = CryptoCore::AESContext::Cipher;

Error AESContext::start(Mode p_mode, const Vector<uint8_t>& p_key, const Vector<uint8_t>& p_iv)
{
	ERR_FAIL_COND_V_MSG(mode != MODE_MAX, ERR_ALREADY_IN_USE,
		"AESContext already started. Call 'finish' before starting a new one.");
	ERR_FAIL_COND_V_MSG(
		p_mode < 0 || p_mode >= MODE_MAX, ERR_INVALID_PARAMETER, "Invalid mode requested.");
	// Key check.
	int key_bits = p_key.size() << 3;
	ERR_FAIL_COND_V_MSG(key_bits != 128 && key_bits != 256, ERR_INVALID_PARAMETER,
		"AES key must be either 16 or 32 bytes");

	AESMode ctx_mode = AESMode::NONE;
	AESCipher ctx_cipher = AESCipher::NONE;
	switch (p_mode) {
	case MODE_ECB_ENCRYPT:
		ctx_mode = AESMode::ENCRYPT;
		ctx_cipher = AESCipher::ECB;
		break;
	case MODE_ECB_DECRYPT:
		ctx_mode = AESMode::DECRYPT;
		ctx_cipher = AESCipher::ECB;
		break;
	case MODE_CBC_ENCRYPT:
		ctx_mode = AESMode::ENCRYPT;
		ctx_cipher = AESCipher::CBC;
		break;
	case MODE_CBC_DECRYPT:
		ctx_mode = AESMode::DECRYPT;
		ctx_cipher = AESCipher::CBC;
		break;
	default:
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	}

	// Initialization vector.
	Vector<uint8_t> iv = p_iv;
	ERR_FAIL_COND_V_MSG(ctx_cipher == AESCipher::CBC && iv.size() != 16, ERR_INVALID_PARAMETER,
		"The initialization vector (IV) must be exactly 16 bytes.");
	if (unlikely(ctx_cipher == AESCipher::ECB && iv.size())) {
		WARN_PRINT("ECB mode does not require an initialization vector (IV). Pass an empty IV "
				   "argument when using ECB.");
		iv.clear();
	}
	Error err = ctx.setup(
		ctx_mode, ctx_cipher, p_key.ptr(), p_key.size(), iv.size() ? iv.ptr() : nullptr, iv.size());
	if (err != OK) {
		return err;
	}
	mode = p_mode;
	return OK;
}

Vector<uint8_t> AESContext::update(const Vector<uint8_t>& p_src)

{
	ERR_FAIL_COND_V_MSG(mode < 0 || mode >= MODE_MAX, Vector<uint8_t>(),
		"AESContext not started. Call 'start' before calling 'update'.");
	int len = p_src.size();
	ERR_FAIL_COND_V_MSG(len % 16, Vector<uint8_t>(),
		"The number of bytes to be encrypted must be multiple of 16. Add padding if needed");
	Vector<uint8_t> out;
	out.resize(len);
	Error err = ctx.update(p_src.ptr(), len, out.ptrw(), len);
	ERR_FAIL_COND_V(err != OK, Vector<uint8_t>());
	return out;
}

Vector<uint8_t> AESContext::get_iv_state()
{
	ERR_FAIL_V_MSG(Vector<uint8_t>(), "Calling 'get_iv_state' is no longer supported.");
}

void AESContext::finish()
{
	ctx.finish(nullptr, 0);
	mode = MODE_MAX;
}

// ==========================================================================
// crypto_resource_format.cpp
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

#include "core/crypto/crypto.h"
#include "crypto_resource_format.h"

Ref<Resource> ResourceFormatLoaderCrypto::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	String el = p_path.get_extension().to_lower();
	if (el == "crt") {
		X509Certificate* cert = X509Certificate::create();
		if (cert) {
			cert->load(p_path);
		}
		return cert;
	}
	else if (el == "key") {
		CryptoKey* key = CryptoKey::create();
		if (key) {
			key->load(p_path, false);
		}
		return key;
	}
	else if (el == "pub") {
		CryptoKey* key = CryptoKey::create();
		if (key) {
			key->load(p_path, true);
		}
		return key;
	}
	return nullptr;
}

void ResourceFormatLoaderCrypto::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("crt");
	p_extensions->push_back("key");
	p_extensions->push_back("pub");
}

bool ResourceFormatLoaderCrypto::handles_type(const String& p_type) const
{
	return p_type == "X509Certificate" || p_type == "CryptoKey";
}

String ResourceFormatLoaderCrypto::get_resource_type(const String& p_path) const
{
	String el = p_path.get_extension().to_lower();
	if (el == "crt") {
		return "X509Certificate";
	}
	else if (el == "key" || el == "pub") {
		return "CryptoKey";
	}
	return "";
}

Error ResourceFormatSaverCrypto::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Error err;
	Ref<X509Certificate> cert = p_resource;
	Ref<CryptoKey> key = p_resource;
	if (cert.is_valid()) {
		err = cert->save(p_path);
	}
	else if (key.is_valid()) {
		err = key->save(p_path, p_path.has_extension("pub"));
	}
	else {
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	}
	ERR_FAIL_COND_V_MSG(
		err != OK, err, vformat("Cannot save Crypto resource to file '%s'.", p_path));
	return OK;
}

bool ResourceFormatSaverCrypto::recognize(const Ref<Resource>& p_resource) const { return false; }

void ResourceFormatSaverCrypto::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
}
