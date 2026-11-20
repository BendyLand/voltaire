/**************************************************************************/
/*  test_crypto.cpp                                                       */
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

#include "tests/test_macros.h"

TEST_FORCE_LINK(test_crypto)

#include "core/crypto/crypto.h"

namespace TestCrypto {

PackedByteArray raw_to_pba(const uint8_t *arr, size_t len) {
	PackedByteArray pba;
	pba.resize(len);
	for (size_t i = 0; i < len; i++) {
		pba.set(i, arr[i]);
	}
	return pba;
}

TEST_CASE("[Crypto] PackedByteArray constant time compare") {
	const uint8_t hm1[] = { 144, 140, 176, 38, 88, 113, 101, 45, 71, 105, 10, 91, 248, 16, 117, 244, 189, 30, 238, 29, 219, 134, 82, 130, 212, 114, 161, 166, 188, 169, 200, 106 };
	const uint8_t hm2[] = { 80, 30, 144, 228, 108, 38, 188, 125, 150, 64, 165, 127, 221, 118, 144, 232, 45, 100, 15, 248, 193, 244, 245, 34, 116, 147, 132, 200, 110, 27, 38, 75 };
	PackedByteArray p1 = raw_to_pba(hm1, std_size(hm1));
	PackedByteArray p2 = raw_to_pba(hm2, std_size(hm2));
	Crypto crypto;
	bool equal = crypto.constant_time_compare(p1, p1);
	CHECK(equal);
	equal = crypto.constant_time_compare(p1, p2);
	CHECK(!equal);
}

} // namespace TestCrypto
