/**************************************************************************/
/*  crypto.cpp                                                            */
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
#include "crypto.h"

/// Resources

CryptoKey* (*CryptoKey::_create)(bool p_notify_postinitialize) = nullptr;

CryptoKey* CryptoKey::create(bool p_notify_postinitialize)
{
	if (_create) {
		return _create(p_notify_postinitialize);
	}
	return nullptr;
}

void CryptoKey::_bind_methods() {}

X509Certificate* (*X509Certificate::_create)(bool p_notify_postinitialize) = nullptr;

X509Certificate* X509Certificate::create(bool p_notify_postinitialize)
{
	if (_create) {
		return _create(p_notify_postinitialize);
	}
	return nullptr;
}

void X509Certificate::_bind_methods() {}

/// TLSOptions

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

void TLSOptions::_bind_methods() {}

/// HMACContext

void HMACContext::_bind_methods() {}

HMACContext* (*HMACContext::_create)(bool p_notify_postinitialize) = nullptr;

HMACContext* HMACContext::create(bool p_notify_postinitialize)
{
	if (_create) {
		return _create(p_notify_postinitialize);
	}
	ERR_FAIL_V_MSG(nullptr, "HMACContext is not available when the mbedtls module is disabled.");
}

/// Crypto

void (*Crypto::_load_default_certificates)(const String& p_path) = nullptr;
Crypto* (*Crypto::_create)(bool p_notify_postinitialize) = nullptr;

Crypto* Crypto::create(bool p_notify_postinitialize)
{
	if (_create) {
		return _create(p_notify_postinitialize);
	}
	ERR_FAIL_V_MSG(nullptr, "Crypto is not available when the mbedtls module is disabled.");
}

void Crypto::load_default_certificates(const String& p_path)
{
	if (_load_default_certificates) {
		_load_default_certificates(p_path);
	}
}

PackedByteArray Crypto::hmac_digest(HashingContext::HashType p_hash_type,
	const PackedByteArray& p_key, const PackedByteArray& p_msg)
{
	Ref<HMACContext> ctx = Ref<HMACContext>(HMACContext::create());
	ERR_FAIL_COND_V_MSG(
		ctx.is_null(), PackedByteArray(), "HMAC is not available without mbedtls module.");
	Error err = ctx->start(p_hash_type, p_key);
	ERR_FAIL_COND_V(err != OK, PackedByteArray());
	err = ctx->update(p_msg);
	ERR_FAIL_COND_V(err != OK, PackedByteArray());
	return ctx->finish();
}

// Compares two HMACS for equality without leaking timing information in order to prevent timing
// attacks.
// @see:
// https://paragonie.com/blog/2015/11/preventing-timing-attacks-on-string-comparison-with-double-hmac-strategy
bool Crypto::constant_time_compare(
	const PackedByteArray& p_trusted, const PackedByteArray& p_received)
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

void Crypto::_bind_methods() {}


