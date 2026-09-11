/**************************************************************************/
/*  aes_context.cpp                                                       */
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

void AESContext::_bind_methods() {}


