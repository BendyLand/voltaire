/**************************************************************************/
/*  crypto_mbedtls.h                                                      */
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

#include <mbedtls/ssl.h>
#include "core/crypto/crypto.h"

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
