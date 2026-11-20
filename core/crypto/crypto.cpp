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
