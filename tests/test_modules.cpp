#include "modules/modules_enabled.gen.h"
#include "tests/test_macros.h"

// === Merged from modules/mbedtls/tests/test_crypto_mbedtls.cpp ===
#ifdef MODULE_MBEDTLS_ENABLED
/*  test_crypto_mbedtls.cpp                                               */
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

// --- Inlined from modules/mbedtls/tests/test_crypto_mbedtls.h ---
#include "core/io/net_socket.h"
#include "tests/test_macros.h"

namespace TestCryptoMbedTLS {

// HMACContext
void hmac_digest_test(HashingContext::HashType ht, String expected_hex);

TEST_CASE("[Crypto] HMAC digest") {
	// SHA-256
	hmac_digest_test(HashingContext::HashType::HASH_SHA256, "fe442023f8a7d36a810e1e7cd8a8e2816457f350a008fbf638296afa12085e59");

	// SHA-1
	hmac_digest_test(HashingContext::HashType::HASH_SHA1, "a0ac4cd68a2f4812c355983d94e8d025afe7dddf");
}

void hmac_context_digest_test(HashingContext::HashType ht, String expected_hex);

TEST_CASE("[HMACContext] HMAC digest") {
	// SHA-256
	hmac_context_digest_test(HashingContext::HashType::HASH_SHA256, "fe442023f8a7d36a810e1e7cd8a8e2816457f350a008fbf638296afa12085e59");

	// SHA-1
	hmac_context_digest_test(HashingContext::HashType::HASH_SHA1, "a0ac4cd68a2f4812c355983d94e8d025afe7dddf");
}

// CryptoKey
void crypto_key_public_only_test(const String &p_key_path, bool public_only);

TEST_CASE("[Crypto] CryptoKey is_public_only") {
	crypto_key_public_only_test(TestUtils::get_data_path("crypto/in.key"), false);
	crypto_key_public_only_test(TestUtils::get_data_path("crypto/in.pub"), true);
	crypto_key_public_only_test(TestUtils::get_data_path("crypto/in.prime256v1.key"), false);
	crypto_key_public_only_test(TestUtils::get_data_path("crypto/in.prime256v1.pub"), true);
}

void crypto_key_save_test(const String &p_in_path, const String &p_out_path, bool public_only);

TEST_CASE("[Crypto] CryptoKey save RSA") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.key");
	const String out_priv_path = TestUtils::get_data_path("crypto/out.key");
	crypto_key_save_test(in_priv_path, out_priv_path, false);

	const String in_pub_path = TestUtils::get_data_path("crypto/in.pub");
	const String out_pub_path = TestUtils::get_data_path("crypto/out.pub");
	crypto_key_save_test(in_pub_path, out_pub_path, true);
}

TEST_CASE("[Crypto] CryptoKey save EC") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.prime256v1.key");
	const String out_priv_path = TestUtils::get_data_path("crypto/out.prime256v1.key");
	crypto_key_save_test(in_priv_path, out_priv_path, false);

	const String in_pub_path = TestUtils::get_data_path("crypto/in.prime256v1.pub");
	const String out_pub_path = TestUtils::get_data_path("crypto/out.prime256v1.pub");
	crypto_key_save_test(in_pub_path, out_pub_path, true);
}

void crypto_key_save_public_only_test(const String &p_in_priv_path, const String &p_in_pub_path, const String &p_out_path);

TEST_CASE("[Crypto] CryptoKey save public_only RSA") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.key");
	const String in_pub_path = TestUtils::get_data_path("crypto/in.pub");
	const String out_path = TestUtils::get_data_path("crypto/out_public_only.pub");
	crypto_key_save_public_only_test(in_priv_path, in_pub_path, out_path);
}

TEST_CASE("[Crypto] CryptoKey save public_only EC") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.prime256v1.key");
	const String in_pub_path = TestUtils::get_data_path("crypto/in.prime256v1.pub");
	const String out_path = TestUtils::get_data_path("crypto/out_public_only.prime256v1.pub");
	crypto_key_save_public_only_test(in_priv_path, in_pub_path, out_path);
}

void crypto_key_generate_save_load(const String &p_out_priv_path, const String &p_out_pub_path);

TEST_CASE("[Crypto] CryptoKey RSA generate/save/load") {
	const String out_priv_path = TestUtils::get_data_path("crypto/out_generated.key");
	const String out_pub_path = TestUtils::get_data_path("crypto/out_generated.pub");
	crypto_key_generate_save_load(out_priv_path, out_pub_path);
}

// Crypto
void crypto_sign_verify_test(const String &p_in_priv_path, const String &p_in_pub_path, HashingContext::HashType p_hash_type, const Vector<uint8_t> &p_hash);

void crypto_generate_sign_verify_test(HashingContext::HashType p_hash_type, const Vector<uint8_t> &p_hash);

TEST_CASE("[Crypto] RSA generate/sign/verify") {
	const String msg("Some message to sign.");
	crypto_generate_sign_verify_test(HashingContext::HASH_MD5, msg.md5_buffer());
	crypto_generate_sign_verify_test(HashingContext::HASH_SHA1, msg.sha1_buffer());
	crypto_generate_sign_verify_test(HashingContext::HASH_SHA256, msg.sha256_buffer());
}

TEST_CASE("[Crypto] RSA load/sign/verify") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.key");
	const String in_pub_path = TestUtils::get_data_path("crypto/in.pub");
	const String msg("Some message to sign.");
	crypto_sign_verify_test(in_priv_path, in_pub_path, HashingContext::HASH_MD5, msg.md5_buffer());
	crypto_sign_verify_test(in_priv_path, in_pub_path, HashingContext::HASH_SHA1, msg.sha1_buffer());
	crypto_sign_verify_test(in_priv_path, in_pub_path, HashingContext::HASH_SHA256, msg.sha256_buffer());
}

TEST_CASE("[Crypto] EC load/sign/verify") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.prime256v1.key");
	const String in_pub_path = TestUtils::get_data_path("crypto/in.prime256v1.pub");
	const String msg("Some message to sign.");
	crypto_sign_verify_test(in_priv_path, in_pub_path, HashingContext::HASH_MD5, msg.md5_buffer());
	crypto_sign_verify_test(in_priv_path, in_pub_path, HashingContext::HASH_SHA1, msg.sha1_buffer());
	crypto_sign_verify_test(in_priv_path, in_pub_path, HashingContext::HASH_SHA256, msg.sha256_buffer());
}

void crypto_verify_test(const String &p_in_pub_path, HashingContext::HashType p_hash_type, const Vector<uint8_t> &p_hash, const Vector<uint8_t> &p_signature, bool p_expected);

TEST_CASE("[Crypto] RSA verify well-known signatures") {
	const String in_pub_path = TestUtils::get_data_path("crypto/in.pub");
	const String msg("This is a message to verify");

	const char *wk_sig_md5 = "7524b3be41d7c40e4e4f4e3c3a072e5cfe4c66f6f87fa8f11e77e72941a3c601a04281ec2200fadc5d7362807679099dc374d5d8c02f1b6f47a1a3dd2f8f5808b2b7265c04c0da01a144ddc06379994209d5e5e731719e73a90741b3435706b43f0ff28bf707a4d7e17e610ef44dbfe303b52e74118c3a6696d09bf94b92673f8a7b49a391a7c1bdfbd5a8b7e3fad747517d9fe25720072217a29c1ab06c74b97a7464574dc509dd896df129d051823c6d25a1b31a16bdad8be84e8557b31193efe55c5f4d28ef50153c5129ac7d42ea560eb2ac10874c565e0a8fc61067cfc178bad824afbbbb8a659f7ad50a35728d08c30b2ec52eb9627704f6bc4620d0ad1d3a6f28017350f2e470c1a7dd2723ef92732aa9b226d4df241231b0a2c0ac7867bee2f352f853a2812a63b7ee34eec9abb76213d306bf9726dfce483a6f9ade18dee017a1373f26b45ceb8c5754f3e0f565db1d2d2c49b286dd2aef254e05c0041b78347586020492c3199b0c865b5341c95e465fe7bcd83d0ea44bd750f863597fa2ce970ab165a97d3e2978bbbba2eef9135dbe86c53d223d4303a19353c8f9a4f1bed0ac69f53e09dad836f38cdca5d6fe46407de95413bf5e9e551ea84f8ca097b36932432eefe49be707a3999e0f4064933a2a39552b16b2466ee64d72bec3c52337f3326c2acdeee5f3ada69cb11060ab5c97eace333d29d6fcbdf06b";
	const char *wk_sig_sha1 = "9b81157dc6dae7bdd6dfbc3e77bc3714119a9ddef5f73a1d765c9478629eea937d23c450c496e1220fcff32eaf8aadc070739ced0f22691c2b88aafc694f129773d1fd9fa5fc62d7d1c5b9feb7701ea6c451a6b51f5d980a8f13e73eea4d3ebf04da6b5d3c047413d8985fcc6dfccc77714e33e8fefe7a04e98b21c41b1bfe2f78f902ab6acf9fb50db4ff25e79374ae212537099482088b46a4769ca511c9880370698a2e69a3b6427d67e79afb4d830f2e4f85ba380663c7fe188f878f57420ab5584b01cede502dd015e642fe533d62cefb267aea77d2f55e4fe6d93b1dab8dfe2b5c70135f993b41ea1b7a3f627f0293a035bb3ad08b3118f8398a55accad7616a89645f929fafbfad7c22a4e449112b57792dd3480252865c12d1348abcdfe8a8096e254f516d6ffd5d6d40c9ed66c8a82ad2cca2dc2526aab06b1436db9984aea624051e72f949dd163a4a364902930de44deab84e3b5048c5fe67b97dcc858f3334895d68c35dd7821446676ae240a846362878d490b9385437ed0a75ad4f1cc30e4553337629e4b31cede5dfd93aa9ee5fd98d2a2875084212c9264c751350e28b32630befebc1bfb04d46a82d50f9465e76d535c9159b9a902ac3cc437c46928d6839504e66154ee711c9000d99b928753216622cb6ee52ff93e972a8069676953e8d0d3b7306c603ee8e483a94a8733ee9debe020832b14f433307";
	const char *wk_sig_sha256 = "8bd353af89d0c1f3ce39541f2b0e80f40e1e361fbeb754db4b4a532df4821c87f93e13abe397a3e661fb69f0423c2a03bdf7d20903116122315d53baa8885fe26fd4e3f85a83dd3d5d3d526fec9039487e67938b141c7eec23cfa09fad03eeb3e4772b78e768ada24ae79126595b816b748201a6549bf4d967f6c88c830eb77b4afc23990a2b0ab0c82afd9a93d2ee4131d17382cd16dfa228e9a0fbcb22383da347bcbd1baff1efa7ac93590a910977784b37dd2824e600481fc986012d5151a502508e621568283c084c071154d4048ece461993c0831ed5979a100d76659482c0714ceb02d1e2d76d0bc8540d92409ab73bb0688d633c2ff1881f50302ac134b87f0031ed2a67412a9e09617a2b4f6c7b6dc5d82d1cba1b2634d8e70caf786fe91de11b8a57d2d506b7b84e9317687b2bf2c8418c9c7e555a7254cec2b6a1aaf7e614d94b0c170eddf7c981de92a87799c949d4091f99f1c3faee6df86fb701328c990892cec290e810ace2ea64b0f93d01888a6defb09212137ed36746b7149ff81bddaf58012e62ddc49b7f4e7d87b450df2be005f7b5f4c6d06c5e640b4f017f9091a84a91e8ddac6f09ef7349c3ec9959e1b7648d74944b4dea1773f52fbca7436f3704cb5906d9e03a59ff3e40c66eac3dcd7990bf4103dfe10c896dd6897ca3916cd0fa61ece22b54b7c752f03b267b975e72ac187d5ed007c3b26b";

	// Valid
	crypto_verify_test(in_pub_path, HashingContext::HASH_MD5, msg.md5_buffer(), String(wk_sig_md5).hex_decode(), true);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA1, msg.sha1_buffer(), String(wk_sig_sha1).hex_decode(), true);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA256, msg.sha256_buffer(), String(wk_sig_sha256).hex_decode(), true);

	// Incorrect hash
	ERR_PRINT_OFF;
	crypto_verify_test(in_pub_path, HashingContext::HASH_MD5, msg.sha256_buffer(), String(wk_sig_sha256).hex_decode(), false);
	ERR_PRINT_ON;

	// Incorrect signature
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA256, msg.sha256_buffer(), String(wk_sig_sha256).replace("a", "b").hex_decode(), false);

	// Tampered message
	crypto_verify_test(in_pub_path, HashingContext::HASH_MD5, (msg + "!").md5_buffer(), String(wk_sig_md5).hex_decode(), false);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA1, (msg + "!").sha1_buffer(), String(wk_sig_sha1).hex_decode(), false);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA256, (msg + "!").sha256_buffer(), String(wk_sig_sha256).hex_decode(), false);
}

TEST_CASE("[Crypto] EC verify well-known signatures") {
	const String in_pub_path = TestUtils::get_data_path("crypto/in.prime256v1.pub");
	const String msg("This is a message to verify");

	const char *wk_sig_md5 = "3044022047f4782bc7319d7562eb9f20e9b8a924fe9a58f0fac0710d609bbe448f368188022035d5c332388f577c5ef7c829d1c58cca817a6ade4406c931a7cfafc367895c8a";
	const char *wk_sig_sha1 = "3044022023f1e922dc51e88960dd133bd192a405fcb7f3b71fd59d49b9943d2037be1c140220053e6ad1230b422556db3bd8dbaa05358399db24f5ad4a044be4c53dc6b2a491";
	const char *wk_sig_sha256 = "3046022100a1b768db040dea7a79c81f10fd0b3d34d53fe941cdd46ee1a382ce27310a9826022100c038ce88b59ff6f9fbe7837f0ff5e5079adf7995ac2d86e2baf03bfa1cda5cd4";

	// Valid
	crypto_verify_test(in_pub_path, HashingContext::HASH_MD5, msg.md5_buffer(), String(wk_sig_md5).hex_decode(), true);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA1, msg.sha1_buffer(), String(wk_sig_sha1).hex_decode(), true);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA256, msg.sha256_buffer(), String(wk_sig_sha256).hex_decode(), true);

	// Incorrect hash
	ERR_PRINT_OFF;
	crypto_verify_test(in_pub_path, HashingContext::HASH_MD5, msg.sha256_buffer(), String(wk_sig_sha256).hex_decode(), false);
	ERR_PRINT_ON;

	// Incorrect signature
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA256, msg.sha256_buffer(), String(wk_sig_sha256).replace("a", "b").hex_decode(), false);

	// Tampered message
	crypto_verify_test(in_pub_path, HashingContext::HASH_MD5, (msg + "!").md5_buffer(), String(wk_sig_md5).hex_decode(), false);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA1, (msg + "!").sha1_buffer(), String(wk_sig_sha1).hex_decode(), false);
	crypto_verify_test(in_pub_path, HashingContext::HASH_SHA256, (msg + "!").sha256_buffer(), String(wk_sig_sha256).hex_decode(), false);
}

void crypto_encrypt_decrypt_test(const String &p_in_priv_path, const String &p_in_pub_path, const Vector<uint8_t> &p_message);

TEST_CASE("[Crypto] Crypto encrypt/decrypt") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.key");
	const String in_pub_path = TestUtils::get_data_path("crypto/in.pub");
	crypto_encrypt_decrypt_test(in_priv_path, in_pub_path, String("Some message").to_utf8_buffer());
	crypto_encrypt_decrypt_test(in_priv_path, in_pub_path, String("Some very long message").repeat(16).to_utf8_buffer());
}

void crypto_decrypt_test(const String &p_in_priv_path, const Vector<uint8_t> &p_ciphertext, const Vector<uint8_t> &p_message, bool p_valid, bool p_try_pub_key = false);

TEST_CASE("[Crypto] Crypto decrypt well-known") {
	const String in_priv_path = TestUtils::get_data_path("crypto/in.key");
	const String in_pub_path = TestUtils::get_data_path("crypto/in.pub");

	const char *wk_ciphertext = "85ca5bf60eac80831b9927c5dba1a4dcf253a5f019e56b71262a6b5476c8294418c59ad30d2815fc26f1ce1e1860637ade746c262c44443cef84467babbba456c0dec7093481fc56795ebe170372fccb39744590e32201752981e39b81d24915af90240632878b96617103eff46c999ee5b4f9aa009eed6a1f9a8be4fde142c754e8f71cddac8405329d8ae6d9f98eb23547b60c416f01cd9199bbfec5e6fe076b032d8900ac7c6b14112ceebfef67962814b9090c49d0ca2fe14adf40242759e374c7944a413743fcaf655ede33d57cfb93cf2c33a292894b6450f34faf1b2772813c8007dd19d2fceaa1c9a8632dba021ecd80305a26b8b0ecfb3c26edca2d81bf48381db0b3f7eb17e6513b4e691d1965ab2b1b7d0dff1c6f717cd498233367081bb989ed8e1836d7229ec8323d7cd8d8fd68ee434c8e703b1215acd10dbd5ca7d52599503d1775f53d726396ac7bed171810de498ff5364bcb2f09c6b75775d0974a20563c51530ca41addabe99f46076ff11d037c23be13bb195e5416c73a3471d7ffec3f0bb65ce4b0674be8e3f4c7da2fe89a5a7d2994cefd936a0a4443d38d8490353640598725da7e8ab85b038ec81d558d56df0d4fa733ddc8972104c0ded969908d3d9833cbb4fd34e528368fe7b2b988a222707671af11972e98e6ec7f38d9847843c6adfb46ff66c888a037c4affd6d7c7a6e6437b53d2fdfdb";

	const char *wk_empty_ciphertext = "a63f7948a1c5ac185c893b1e783571b31b34678462ea413fcbc3cb531b575b3c49cc7547dcfcaaf37015785ad917be1b08cd25adb1ff1a448bf6b76eadf57b26236644e6aadb60a04e41da7add9238c5c211af8a3dacaf64fba47125314189defd9e305e7264d156f3a25972acc2195d33257fa427d519d349f21319fdd6d8a6d33a64ee174a61af92a999ae4fc9034286b307c6872c6e8bd809595bb586958e3362b52460b6bb83edcc94c38a90c799ce4905f0f2efade25b17ac1d2795b15eedc99ea1212ef9922b629f1ca3e0288bf493be6bed4066cb29f75cd37af9eca7c012514907970ba59ed803759bb2d4235ed9fdce12183b703246625ee221c51c9a0bc341a7bb3d45720f6072fffa2d7233bd94ff79fbdd90138137cb00d069f82be63c5fe20df7e23bbcd7d77f1a5a71b5936557a76703c3aa4fdbd4950d3aca68db28d117ca26a5ff0cda4b1d4841a8814ce93f097ac2c94381f77a29de4dadbca7daefaebbd03097f74b36644453bfce5c7a3aee2d99ca552389d9c8895d2933b874201b15fa63af37ee722e8a0d848665f85a00b234b2927b06e12861a539d9e20549af07bfa9db68775bf821007363015b09e2d8ed3b9c2fbf431c44619152159106368a9c04c6e8c0eb5295f11e1dcebaf5691a70dbb9c520bd186e13b6cbaf0381377a7f367eebd2c45c636e9cc1116729e23614001b92ebc26a899226";

	const Vector<uint8_t> message = String("This is a secret message").to_utf8_buffer();
	crypto_decrypt_test(in_priv_path, String(wk_ciphertext).hex_decode(), message, true);
	crypto_decrypt_test(in_priv_path, String(wk_empty_ciphertext).hex_decode(), Vector<uint8_t>(), true);

	// Invalid ciphertext
	ERR_PRINT_OFF;
	crypto_decrypt_test(in_priv_path, String(wk_ciphertext).replace("a", "b").hex_decode(), message, false);
	ERR_PRINT_ON;

	// Can't decrypt with public key
	ERR_PRINT_OFF;
	crypto_decrypt_test(in_pub_path, String(wk_ciphertext).hex_decode(), message, false, true);
	ERR_PRINT_ON;
}

} // namespace TestCryptoMbedTLS
// --- End modules/mbedtls/tests/test_crypto_mbedtls.h ---

#include "core/io/net_socket.h"
#include "core/io/file_access.h"
#include "tests/test_macros.h"

namespace TestCryptoMbedTLS {

void hmac_digest_test(HashingContext::HashType ht, String expected_hex) {
	Crypto crypto;
	PackedByteArray key = String("supersecretkey").to_utf8_buffer();
	PackedByteArray msg = String("Return of the MAC!").to_utf8_buffer();
	PackedByteArray digest = crypto.hmac_digest(ht, key, msg);
	String hex = String::hex_encode_buffer(digest.ptr(), digest.size());
	CHECK(hex == expected_hex);
}

void hmac_context_digest_test(HashingContext::HashType ht, String expected_hex) {
	HMACContext ctx;
	PackedByteArray key = String("supersecretkey").to_utf8_buffer();
	PackedByteArray msg1 = String("Return of ").to_utf8_buffer();
	PackedByteArray msg2 = String("the MAC!").to_utf8_buffer();
	Error err = ctx.start(ht, key);
	CHECK(err == OK);
	err = ctx.update(msg1);
	CHECK(err == OK);
	err = ctx.update(msg2);
	CHECK(err == OK);
	PackedByteArray digest = ctx.finish();
	String hex = String::hex_encode_buffer(digest.ptr(), digest.size());
	CHECK(hex == expected_hex);
}

Ref<CryptoKey> load_key(const String &p_key_path, bool p_public_only) {
	Ref<CryptoKey> key(memnew(CryptoKey));
	REQUIRE(key.is_valid());
	Error err = key->load(p_key_path, p_public_only);
	REQUIRE(err == OK);
	return key;
}

String read_file_s(const String &p_file_path) {
	Ref<FileAccess> file_access = FileAccess::open(p_file_path, FileAccess::READ);
	REQUIRE(file_access.is_valid());
	return file_access->get_as_utf8_string();
}

bool files_equal(const String &p_in_path, const String &p_out_path) {
	const String s_in = read_file_s(p_in_path);
	const String s_out = read_file_s(p_out_path);
	return s_in == s_out;
}

void crypto_key_public_only_test(const String &p_key_path, bool p_public_only) {
	Ref<CryptoKey> crypto_key = load_key(p_key_path, p_public_only);
	bool is_equal = crypto_key->is_public_only() == p_public_only;
	CHECK(is_equal);
}

void crypto_key_save_test(const String &p_in_path, const String &p_out_path, bool p_public_only) {
	Ref<CryptoKey> crypto_key = load_key(p_in_path, p_public_only);
	crypto_key->save(p_out_path, p_public_only);
	bool is_equal = files_equal(p_in_path, p_out_path);
	CHECK(is_equal);
}

void crypto_key_save_public_only_test(const String &p_in_priv_path, const String &p_in_pub_path, const String &p_out_path) {
	Ref<CryptoKey> crypto_key = load_key(p_in_priv_path, false);
	crypto_key->save(p_out_path, true);
	bool is_equal = files_equal(p_in_pub_path, p_out_path);
	CHECK(is_equal);
}

void crypto_key_generate_save_load(const String &p_out_priv_path, const String &p_out_pub_path) {
	Ref<Crypto> c(memnew(Crypto));
	REQUIRE(c.is_valid());
	Ref<CryptoKey> key = c->generate_rsa(1024);
	CHECK(key->save_to_string() != key->save_to_string(true));
	Error err = key->save(p_out_priv_path);
	REQUIRE(err == OK);
	err = key->save(p_out_pub_path, true);
	REQUIRE(err == OK);
	Ref<CryptoKey> priv = load_key(p_out_priv_path, false);
	Ref<CryptoKey> pub = load_key(p_out_pub_path, true);
	CHECK(pub->save_to_string(true) != priv->save_to_string());
	CHECK(pub->save_to_string(true) == priv->save_to_string(true));
	CHECK(pub->save_to_string(true) == key->save_to_string(true));
	CHECK(key->save_to_string() == priv->save_to_string());
}

void crypto_generate_sign_verify_test(HashingContext::HashType p_hash_type, const Vector<uint8_t> &p_hash) {
	Ref<Crypto> c(memnew(Crypto));
	REQUIRE(c.is_valid());
	Ref<CryptoKey> key = c->generate_rsa(1024);
	Vector<uint8_t> signature = c->sign(p_hash_type, p_hash, key);
	CHECK(signature.size() > 0);
	bool valid = c->verify(p_hash_type, p_hash, signature, key);
	CHECK(valid);
	Ref<CryptoKey> pub(memnew(CryptoKey));
	pub->load_from_string(key->save_to_string(true), true);
	valid = c->verify(p_hash_type, p_hash, signature, pub);
	CHECK(valid);
}

void crypto_sign_verify_test(const String &p_in_priv_path, const String &p_in_pub_path, HashingContext::HashType p_hash_type, const Vector<uint8_t> &p_hash) {
	Ref<CryptoKey> priv = load_key(p_in_priv_path, false);
	Ref<CryptoKey> pub = load_key(p_in_pub_path, true);
	Ref<Crypto> c(memnew(Crypto));
	REQUIRE(c.is_valid());
	Vector<uint8_t> signature = c->sign(p_hash_type, p_hash, priv);
	CHECK(signature.size() > 0);
	bool valid = c->verify(p_hash_type, p_hash, signature, pub);
	CHECK(valid);
}

void crypto_verify_test(const String &p_in_pub_path, HashingContext::HashType p_hash_type, const Vector<uint8_t> &p_hash, const Vector<uint8_t> &p_signature, bool p_expected) {
	Ref<CryptoKey> pub = load_key(p_in_pub_path, true);
	Ref<Crypto> c(memnew(Crypto));
	REQUIRE(c.is_valid());
	bool valid = c->verify(p_hash_type, p_hash, p_signature, pub);
	CHECK_EQ(valid, p_expected);
}

bool equals(const PackedByteArray &p_a, const PackedByteArray &p_b) {
	if (p_a.size() != p_b.size()) {
		return false;
	}
	if (p_a.is_empty()) {
		return true;
	}
	return memcmp(p_a.ptr(), p_b.ptr(), p_a.size()) == 0;
}

void crypto_encrypt_decrypt_test(const String &p_in_priv_path, const String &p_in_pub_path, const Vector<uint8_t> &p_message) {
	Ref<CryptoKey> priv = load_key(p_in_priv_path, false);
	Ref<CryptoKey> pub = load_key(p_in_pub_path, true);
	Ref<Crypto> c(memnew(Crypto));
	REQUIRE(c.is_valid());
	Vector<uint8_t> ciphertext = c->encrypt(pub, p_message);
	Vector<uint8_t> plaintext = c->decrypt(priv, ciphertext);
	CHECK(equals(plaintext, p_message));
}

void crypto_decrypt_test(const String &p_in_priv_path, const Vector<uint8_t> &p_ciphertext, const Vector<uint8_t> &p_message, bool p_valid, bool p_try_pub_key) {
	Ref<CryptoKey> priv = load_key(p_in_priv_path, p_try_pub_key);
	Ref<Crypto> c(memnew(Crypto));
	REQUIRE(c.is_valid());
	Vector<uint8_t> plaintext = c->decrypt(priv, p_ciphertext);
	CHECK_EQ(equals(plaintext, p_message), p_valid);
}

} // namespace TestCryptoMbedTLS
#endif // MODULE_MBEDTLS_ENABLED

// === Merged from modules/zip/tests/test_zip.cpp ===
#ifdef MODULE_ZIP_ENABLED
/*  test_zip.cpp                                                          */
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

// --- Inlined from modules/zip/tests/test_zip.h ---
#include "core/io/net_socket.h"

#include "tests/test_macros.h"

namespace TestZip {

void check_file_size(const String &p_path, int p_expected_size);

TEST_CASE("[ZIPPacker] default compression") {
	const String path = TestUtils::get_temp_path("compressed.zip");
	Ref<ZIPPacker> packer;
	packer.instantiate();
	Error open_result = packer->open(path, ZIPPacker::APPEND_CREATE);
	CHECK(open_result == OK);
	Error start_file_result = packer->start_file("demo.txt");
	CHECK(start_file_result == OK);
	String text = "hello world!";
	Error write_file_result = packer->write_file(text.to_utf8_buffer());
	CHECK(write_file_result == OK);
	Error close_file_result = packer->close_file();
	CHECK(close_file_result == OK);
	Error close_result = packer->close();
	CHECK(close_result == OK);
	check_file_size(path, 128);
}

TEST_CASE("[ZIPPacker] no compression") {
	const String path = TestUtils::get_temp_path("uncompressed.zip");
	Ref<ZIPPacker> packer;
	packer.instantiate();
	Error open_result = packer->open(path, ZIPPacker::APPEND_CREATE);
	CHECK(open_result == OK);
	packer->set_compression_level(ZIPPacker::COMPRESSION_NONE);
	Error start_file_result = packer->start_file("demo.txt");
	CHECK(start_file_result == OK);
	String text = "hello world!";
	Error write_file_result = packer->write_file(text.to_utf8_buffer());
	CHECK(write_file_result == OK);
	Error close_file_result = packer->close_file();
	CHECK(close_file_result == OK);
	Error close_result = packer->close();
	CHECK(close_result == OK);
	check_file_size(path, 131);
}

TEST_CASE("[ZIPReader] read files") {
	String test_data = String("modules/zip/tests/data/").path_join("test.zip");
	Ref<ZIPReader> reader;
	reader.instantiate();
	Error open_result = reader->open(test_data);
	CHECK(open_result == OK);

	const String hello_path = "hello.txt";
	const String world_path = "world.txt";
	PackedStringArray expected_files;
	expected_files.push_back(hello_path);
	expected_files.push_back(world_path);
	CHECK(reader->get_files() == expected_files);

	const String expected_hello_text = "hello world!";
	const String expected_world_text = "game over!";
	PackedByteArray hello_bytes = reader->read_file(hello_path, false);
	PackedByteArray world_bytes = reader->read_file(world_path, true);
	CHECK(hello_bytes == expected_hello_text.to_utf8_buffer());
	CHECK(world_bytes == expected_world_text.to_utf8_buffer());

	CHECK(reader->get_compression_level(hello_path, true) == 6);
	CHECK(reader->get_compression_level(world_path, false) == 9);
}

} // namespace TestZip
// --- End modules/zip/tests/test_zip.h ---

namespace TestZip {

void check_file_size(const String &p_path, int p_expected_size) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	CHECK(f.is_valid());
	CHECK(f->get_length() == p_expected_size);
}

} // namespace TestZip
#endif // MODULE_ZIP_ENABLED

// === Merged from modules/csg/tests/test_csg.h ===
#ifdef MODULE_CSG_ENABLED
#include "scene/main/scene_tree.h"

#include "tests/test_macros.h"

namespace TestCSG {

TEST_CASE("[SceneTree][CSG] CSGPolygon3D") {
	SUBCASE("[SceneTree][CSG] CSGPolygon3D: using accurate path tangent for polygon rotation") {
		const float polygon_radius = 10.0f;

		const Vector3 expected_min_bounds = Vector3(-polygon_radius, -polygon_radius, 0);
		const Vector3 expected_max_bounds = Vector3(100 + polygon_radius, polygon_radius, 100);
		const AABB expected_aabb = AABB(expected_min_bounds, expected_max_bounds - expected_min_bounds);

		Ref<Curve3D> curve;
		curve.instantiate();
		curve->add_point(
				// p_position
				Vector3(0, 0, 0),
				// p_in
				Vector3(),
				// p_out
				Vector3(0, 0, 60));
		curve->add_point(
				// p_position
				Vector3(100, 0, 100),
				// p_in
				Vector3(0, 0, -60),
				// p_out
				Vector3());

		Path3D *path = memnew(Path3D);
		path->set_curve(curve);

		CSGShape3D *csg_polygon_3d = memnew(CSGShape3D(CSGShape3D::SHAPE_POLYGON));
		SceneTree::get_root()->add_child(csg_polygon_3d);

		csg_polygon_3d->add_child(path);
		csg_polygon_3d->set_path_node(csg_polygon_3d->get_path_to(path));
		csg_polygon_3d->set_polygon_mode(CSGShape3D::POLYGON_MODE_PATH);

		PackedVector2Array polygon;
		polygon.append(Vector2(-polygon_radius, 0));
		polygon.append(Vector2(0, polygon_radius));
		polygon.append(Vector2(polygon_radius, 0));
		polygon.append(Vector2(0, -polygon_radius));
		csg_polygon_3d->set_polygon(polygon);

		csg_polygon_3d->set_path_rotation(CSGShape3D::PATH_ROTATION_PATH);
		csg_polygon_3d->set_path_rotation_accurate(true);

		// Minimize the number of extrusions.
		// This decreases the number of samples taken from the curve.
		// Having fewer samples increases the inaccuracy of the line between samples as an approximation of the tangent of the curve.
		// With correct polygon orientation, the bounding box for the given curve should be independent of the number of extrusions.
		csg_polygon_3d->set_path_interval_type(CSGShape3D::PATH_INTERVAL_DISTANCE);
		csg_polygon_3d->set_path_interval(1000.0f);

		// Call get_brush_faces to force the bounding box to update.
		csg_polygon_3d->get_brush_faces();

		CHECK(csg_polygon_3d->get_aabb().is_equal_approx(expected_aabb));

		// Perform the bounding box check again with a greater number of extrusions.
		csg_polygon_3d->set_path_interval(1.0f);
		csg_polygon_3d->get_brush_faces();

		CHECK(csg_polygon_3d->get_aabb().is_equal_approx(expected_aabb));

		csg_polygon_3d->remove_child(path);
		SceneTree::get_root()->remove_child(csg_polygon_3d);

		memdelete(csg_polygon_3d);
		memdelete(path);
	}
}

} // namespace TestCSG

// --- Folded from modules/gridmap/tests/test_grid_map.h ---
#include "scene/main/scene_tree.h"

#include "tests/test_macros.h"

namespace TestGridMap {

TEST_CASE("[SceneTree][GridMap][MeshLibrary] Octant querying") {
	Ref<MeshLibrary> mesh_library;
	mesh_library.instantiate();

	int item1 = 0;
	int item2 = 1;

	{
		Vector<int> item_list = mesh_library->get_item_list();
		CHECK(item_list.size() == 0);
	}

	mesh_library->create_item(item1);
	mesh_library->create_item(item2);

	int last_unused_item_id = mesh_library->get_last_unused_item_id();
	CHECK(last_unused_item_id == 2);

	mesh_library->set_item_name(item1, "Item1");
	mesh_library->set_item_name(item2, "Item2");

	Ref<PrimitiveMesh> box_mesh;
	box_mesh.instantiate(PrimitiveMesh::MESH_BOX);
	box_mesh->set_size(Vector3(1.0, 1.0, 1.0));

	mesh_library->set_item_mesh(item1, box_mesh);
	mesh_library->set_item_mesh(item2, box_mesh);

	{
		Vector<int> item_list = mesh_library->get_item_list();
		CHECK(item_list.size() == 2);
		CHECK(item_list[0] == item1);
		CHECK(item_list[1] == item2);
	}

	GridMap *grid_map = memnew(GridMap);
	grid_map->set_mesh_library(mesh_library);

	SceneTree::get_root()->add_child(grid_map);

	grid_map->set_octant_size(8);
	grid_map->set_cell_size(Vector3(1.0, 1.0, 1.0));

	grid_map->set_cell_item(Vector3i(0, 0, 0), -1);

	CHECK(grid_map->get_used_cells().size() == 0);
	CHECK(grid_map->get_used_octants().size() == 0);

	grid_map->set_cell_item(Vector3i(0, 0, 0), item1);
	grid_map->set_cell_item(Vector3i(7, 7, 7), item2);

	CHECK(grid_map->get_used_cells().size() == 2);
	CHECK(grid_map->get_used_octants().size() == 1);
	CHECK(grid_map->get_used_octants()[0] == Vector3i(0, 0, 0));

	grid_map->set_cell_item(Vector3i(8, 8, 8), item2);

	CHECK(grid_map->get_used_cells().size() == 3);
	CHECK(grid_map->get_used_octants().size() == 2);
	CHECK(grid_map->get_used_octants()[0] == Vector3i(0, 0, 0));
	CHECK(grid_map->get_used_octants()[1] == Vector3i(1, 1, 1));

	AABB bounds;
	bounds.position = Vector3(0.0, 0.0, 0.0);
	bounds.size = Vector3(8.0, 8.0, 8.0);

	CHECK(grid_map->get_octants_in_bounds(bounds).size() == 1);
	CHECK(grid_map->get_used_octants_in_bounds(bounds).size() == 1);
	CHECK(grid_map->get_octants_in_bounds(bounds)[0] == Vector3i(0, 0, 0));

	// Edge margin is -CMP_EPSILON.
	bounds.size = Vector3(8.001, 8.001, 8.001);
	CHECK(grid_map->get_octants_in_bounds(bounds).size() == 8);
	CHECK(grid_map->get_used_octants_in_bounds(bounds).size() == 2);

	SceneTree::get_root()->remove_child(grid_map);

	memdelete(grid_map);
}

} // namespace TestGridMap
// --- End modules/gridmap/tests/test_grid_map.h ---
#endif // MODULE_CSG_ENABLED

// === Merged from modules/dds/tests/test_dds.h ===
#ifdef MODULE_DDS_ENABLED
#include "servers/rendering/renderer.h"

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "tests/test_macros.h"

namespace TestDDS {
String init(const String &p_test, const String &p_copy_target = String()) {
	String old_resource_path = TestProjectSettingsInternalsAccessor::resource_path();
	Error err;
	// Setup project settings with `res://` set to a temporary path.
	String project_folder = TestUtils::get_temp_path(p_test.get_file().get_basename());
	TestProjectSettingsInternalsAccessor::resource_path() = project_folder;
	err = ProjectSettings::setup(project_folder, String(), true);

	// Create the imported files folder as the editor import process expects it to exist.
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	da->make_dir_recursive(ProjectSettings::globalize_path(ProjectSettings::get_imported_files_path()));

	if (p_copy_target.is_empty()) {
		return old_resource_path;
	}

	// Copy all the necessary test data files to the res:// directory.
	String test_data = String("tests/data").path_join(p_test);
	da = DirAccess::open(test_data);
	CHECK_MESSAGE(da.is_valid(), "Unable to open folder.");
	da->list_dir_begin();
	for (String item = da->get_next(); !item.is_empty(); item = da->get_next()) {
		if (!FileAccess::exists(test_data.path_join(item))) {
			continue;
		}
		Ref<FileAccess> output = FileAccess::open(p_copy_target.path_join(item), FileAccess::WRITE, &err);
		CHECK_MESSAGE(err == OK, "Unable to open output file.");
		output->store_buffer(FileAccess::get_file_as_bytes(test_data.path_join(item)));
		output->close();
	}
	da->list_dir_end();
	return old_resource_path;
}

TEST_CASE("[SceneTree][DDSSaver] Save DDS - Save valid image with mipmap" * doctest::skip(true)) {
	String old_resource_path = init("save_dds_valid_image_with_mipmap");
	Ref<Image> image = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
	image->fill(Color(1, 0, 0)); // Fill with red color
	image->generate_mipmaps();
	image->compress_from_channels(Image::COMPRESS_S3TC, Image::USED_CHANNELS_RGBA);
	Error err = save_dds("res://valid_image_with_mipmap.dds", image);
	CHECK(err == OK);

	Ref<Image> loaded_image;
	loaded_image.instantiate();
	Vector<uint8_t> buffer = FileAccess::get_file_as_bytes("res://valid_image_with_mipmap.dds", &err);
	CHECK(err == OK);
	err = loaded_image->load_dds_from_buffer(buffer);
	CHECK(err == OK);
	Dictionary metrics = image->compute_image_metrics(loaded_image, false);
	CHECK(metrics.size() > 0);
	CHECK_MESSAGE(metrics.has("root_mean_squared"), "Metrics dictionary contains 'root_mean_squared'.");
	float rms = metrics["root_mean_squared"];
	CHECK(rms == 0.0f);
	TestProjectSettingsInternalsAccessor::resource_path() = old_resource_path;
}

TEST_CASE("[SceneTree][DDSSaver] Save DDS - Save valid image with BPTC and S3TC compression" * doctest::skip(true)) {
	String old_resource_path = init("save_dds_valid_image_bptc_s3tc");
	Ref<Image> image_bptc = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
	image_bptc->fill(Color(0, 0, 1)); // Fill with blue color
	image_bptc->compress_from_channels(Image::COMPRESS_BPTC, Image::USED_CHANNELS_RGBA);
	Error err_bptc = image_bptc->save_dds("res://valid_image_bptc.dds");
	CHECK(err_bptc == OK);

	Ref<Image> image_s3tc = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
	image_s3tc->fill(Color(1, 1, 1)); // Fill with white color
	image_s3tc->compress_from_channels(Image::COMPRESS_S3TC, Image::USED_CHANNELS_RGBA);
	Error err_s3tc = image_s3tc->save_dds("res://valid_image_s3tc_combined.dds");
	CHECK(err_s3tc == OK);

	// Validate BPTC image
	Ref<Image> loaded_image_bptc;
	loaded_image_bptc.instantiate();
	Vector<uint8_t> buffer_bptc = FileAccess::get_file_as_bytes("res://valid_image_bptc.dds", &err_bptc);
	CHECK(err_bptc == OK);
	err_bptc = loaded_image_bptc->load_dds_from_buffer(buffer_bptc);
	CHECK(err_bptc == OK);
	Dictionary metrics_bptc = image_bptc->compute_image_metrics(loaded_image_bptc, false);
	CHECK(metrics_bptc.size() > 0);
	CHECK_MESSAGE(metrics_bptc.has("root_mean_squared"), "Metrics dictionary contains 'root_mean_squared' for BPTC.");
	float rms_bptc = metrics_bptc["root_mean_squared"];
	CHECK(rms_bptc == 0.0f);

	// Validate S3TC image
	Ref<Image> loaded_image_s3tc;
	loaded_image_s3tc.instantiate();
	Vector<uint8_t> buffer_s3tc = FileAccess::get_file_as_bytes("res://valid_image_s3tc_combined.dds", &err_s3tc);
	CHECK(err_s3tc == OK);
	err_s3tc = loaded_image_s3tc->load_dds_from_buffer(buffer_s3tc);
	CHECK(err_s3tc == OK);
	Dictionary metrics_s3tc = image_s3tc->compute_image_metrics(loaded_image_s3tc, false);
	CHECK(metrics_s3tc.size() > 0);
	CHECK_MESSAGE(metrics_s3tc.has("root_mean_squared"), "Metrics dictionary contains 'root_mean_squared' for S3TC.");
	float rms_s3tc = metrics_s3tc["root_mean_squared"];
	CHECK(rms_s3tc == 0.0f);
	TestProjectSettingsInternalsAccessor::resource_path() = old_resource_path;
}

TEST_CASE("[SceneTree][DDSSaver] Save DDS - Save valid uncompressed image") {
	String old_resource_path = init("save_dds_valid_uncompressed");
	Ref<Image> image = Image::create_empty(4, 4, false, Image::FORMAT_RGBA8);
	image->fill(Color(0, 0, 1)); // Fill with blue color
	Error err = image->save_dds("res://valid_image_uncompressed.dds");
	CHECK(err == OK);
	Vector<uint8_t> buffer = FileAccess::get_file_as_bytes("res://valid_image_uncompressed.dds", &err);
	CHECK(err == OK);
	Ref<Image> loaded_image;
	loaded_image.instantiate();
	err = loaded_image->load_dds_from_buffer(buffer);
	CHECK(err == OK);
	Dictionary metrics = image->compute_image_metrics(loaded_image, false);
	CHECK(metrics.size() > 0);
	CHECK_MESSAGE(metrics.has("root_mean_squared"), "Metrics dictionary contains 'root_mean_squared' for uncompressed.");
	float rms = metrics["root_mean_squared"];
	CHECK(rms == 0.0f);
	TestProjectSettingsInternalsAccessor::resource_path() = old_resource_path;
}
} //namespace TestDDS
#endif // MODULE_DDS_ENABLED

// === Merged from modules/gltf/tests/test_gltf.h ===
#ifdef MODULE_GLTF_ENABLED
#include "tests/test_macros.h"

#ifdef TOOLS_ENABLED

#include "scene/main/scene_tree.h"

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"
#include "scene/main/node.h"
#include "tests/test_macros.h"

namespace TestGltf {

static Node *gltf_import(const String &p_file) {
	// Setting up importers.
	Ref<ResourceImporterScene> import_scene;
	import_scene.instantiate("PackedScene");
	ResourceFormatImporter::add_importer(import_scene);
	Ref<EditorSceneFormatImporterGLTF> import_gltf;
	import_gltf.instantiate();
	ResourceImporterScene::add_scene_importer(import_gltf);

	// Support processing png files in editor import.
	Ref<ResourceImporterTexture> import_texture;
	import_texture.instantiate(true);
	ResourceFormatImporter::add_importer(import_texture);

	// Once editor import convert pngs to ctex, we will need to load it as ctex resource.
	Ref<ResourceFormatLoaderCompressedTexture> resource_loader_stream_texture;
	if constexpr (GD_IS_CLASS_ENABLED(CompressedTexture2D)) {
		resource_loader_stream_texture.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_stream_texture);
	}

	HashMap<String, Variant> options(21);
	options["nodes/root_type"] = "";
	options["nodes/root_name"] = "";
	options["nodes/root_script"] = Variant();
	options["nodes/apply_root_scale"] = true;
	options["nodes/root_scale"] = 1.0;
	options["meshes/ensure_tangents"] = true;
	options["meshes/generate_lods"] = false;
	options["meshes/create_shadow_meshes"] = true;
	options["meshes/light_baking"] = 1;
	options["meshes/lightmap_texel_size"] = 0.2;
	options["meshes/force_disable_compression"] = false;
	options["skins/use_named_skins"] = true;
	options["animation/import"] = true;
	options["animation/fps"] = 30;
	options["animation/trimming"] = false;
	options["animation/remove_immutable_tracks"] = true;
	options["import_script/path"] = "";
	options["extract_path"] = "res://";
	options["_subresources"] = Dictionary();
	options["gltf/naming_version"] = 1;

	// Process gltf file, note that this generates `.scn` resource from the 2nd argument.
	String scene_file = "res://" + p_file.get_file().get_basename();
	Error err = import_scene->import(0, p_file, scene_file, options, nullptr, nullptr, nullptr);
	CHECK_MESSAGE(err == OK, "GLTF import failed.");

	Ref<PackedScene> packed_scene = ResourceLoader::load(scene_file + ".scn", "", ResourceFormatLoader::CACHE_MODE_REPLACE, &err);
	CHECK_MESSAGE(err == OK, "Loading scene failed.");
	Node *p_scene = packed_scene->instantiate();

	ResourceImporterScene::remove_scene_importer(import_gltf);
	ResourceFormatImporter::remove_importer(import_texture);
	if constexpr (GD_IS_CLASS_ENABLED(CompressedTexture2D)) {
		ResourceLoader::remove_resource_format_loader(resource_loader_stream_texture);
		resource_loader_stream_texture.unref();
	}
	return p_scene;
}

static Node *gltf_export_then_import(Node *p_root, const String &p_test_name) {
	String tempfile = TestUtils::get_temp_path(p_test_name);

	Ref<GLTFDocument> doc;
	doc.instantiate();
	Ref<GLTFState> state;
	state.instantiate();
	Error err = doc->append_from_scene(p_root, state, EditorSceneFormatImporter::IMPORT_USE_NAMED_SKIN_BINDS);
	CHECK_MESSAGE(err == OK, "GLTF state generation failed.");

	err = doc->write_to_filesystem(state, tempfile + ".gltf");
	CHECK_MESSAGE(err == OK, "Writing GLTF to cache dir failed.");

	return gltf_import(tempfile + ".gltf");
}

void init(const String &p_test, const String &p_copy_target = String()) {
	Error err;

	// Setup project settings with `res://` set to a temporary path.
	String project_folder = TestUtils::get_temp_path(p_test.get_file().get_basename());
	TestProjectSettingsInternalsAccessor::resource_path() = project_folder;
	err = ProjectSettings::setup(project_folder, String(), true);

	// Create the imported files folder as the editor import process expects it to exist.
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	da->make_dir_recursive(ProjectSettings::globalize_path(ProjectSettings::get_imported_files_path()));

	if (p_copy_target.is_empty()) {
		return;
	}

	// Copy all the necessary test data files to the res:// directory.
	String test_data = String("modules/gltf/tests/data/").path_join(p_test);
	da = DirAccess::open(test_data);
	CHECK_MESSAGE(da.is_valid(), "Unable to open folder.");
	da->list_dir_begin();
	for (String item = da->get_next(); !item.is_empty(); item = da->get_next()) {
		if (!FileAccess::exists(test_data.path_join(item))) {
			continue;
		}
		Ref<FileAccess> output = FileAccess::open(p_copy_target.path_join(item), FileAccess::WRITE, &err);
		CHECK_MESSAGE(err == OK, "Unable to open output file.");
		output->store_buffer(FileAccess::get_file_as_bytes(test_data.path_join(item)));
		output->close();
	}
	da->list_dir_end();
}

} //namespace TestGltf

#endif // TOOLS_ENABLED

// --- Folded from modules/gltf/tests/test_gltf_emissive.h ---


#ifdef TOOLS_ENABLED

namespace TestGltf {

TEST_CASE("[SceneTree][Node] GLTF emissiveTexture without emissiveFactor uses white emission") {
	init("gltf_emissive_no_factor", "res://");

	Node *loaded = gltf_import("res://emissive_no_factor.gltf");
	CHECK_MESSAGE(loaded != nullptr, "Failed to load GLB.");

	MeshInstance3D *mesh = dynamic_cast<MeshInstance3D *>(loaded->find_child("Cube", true, true));
	CHECK_MESSAGE(mesh != nullptr, "Mesh not found.");

	Ref<StandardMaterial3D> mat = mesh->get_active_material(0);
	CHECK_MESSAGE(mat.is_valid(), "Material not found.");

	// Emission should be enabled.
	CHECK(mat->get_feature(StandardMaterial3D::FEATURE_EMISSION));

	// Emission operator should be MULTIPLY per glTF spec.
	CHECK(mat->get_emission_operator() == StandardMaterial3D::EMISSION_OP_MULTIPLY);

	// Without emissiveFactor, emission color should be WHITE, not BLACK.
	Color c = mat->get_emission();
	CHECK_MESSAGE(c.r > 0.9f, "Emission red should be ~1.0 when emissiveFactor is absent.");
	CHECK_MESSAGE(c.g > 0.9f, "Emission green should be ~1.0 when emissiveFactor is absent.");
	CHECK_MESSAGE(c.b > 0.9f, "Emission blue should be ~1.0 when emissiveFactor is absent.");

	memdelete(loaded);
}

} // namespace TestGltf

#endif // TOOLS_ENABLED
// --- End modules/gltf/tests/test_gltf_emissive.h ---

// --- Folded from modules/gltf/tests/test_gltf_extras.h ---


#ifdef TOOLS_ENABLED

#include "scene/main/scene_tree.h"

#include "servers/rendering/renderer.h"
#include "tests/test_macros.h"

namespace TestGltf {

TEST_CASE("[SceneTree][Node] GLTF test mesh and material meta export and import") {
	init("gltf_mesh_material_extras");
	// Setup scene.
	Ref<StandardMaterial3D> original_material = memnew(StandardMaterial3D);
	original_material->set_albedo(Color(1.0, .0, .0));
	original_material->set_name("material");
	Dictionary material_dict;
	material_dict["node_type"] = "material";
	original_material->set_meta("extras", material_dict);

	Ref<PrimitiveMesh> original_meshdata = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
	original_meshdata->set_name("planemesh");
	Dictionary meshdata_dict;
	meshdata_dict["node_type"] = "planemesh";
	original_meshdata->set_meta("extras", meshdata_dict);
	original_meshdata->surface_set_material(0, original_material);

	MeshInstance3D *original_mesh_instance = memnew(MeshInstance3D);
	original_mesh_instance->set_mesh(original_meshdata);
	original_mesh_instance->set_name("mesh_instance_3d");
	Dictionary mesh_instance_dict;
	mesh_instance_dict["node_type"] = "mesh_instance_3d";
	original_mesh_instance->set_meta("extras", mesh_instance_dict);

	Node3D *original = memnew(Node3D);
	SceneTree::get_root()->add_child(original);
	original->add_child(original_mesh_instance);
	original->set_name("node3d");
	Dictionary node_dict;
	node_dict["node_type"] = "node3d";
	original->set_meta("extras", node_dict);
	original->set_meta("meta_not_nested_under_extras", "should not propagate");

	original->set_owner(SceneTree::get_root());
	original_mesh_instance->set_owner(SceneTree::get_root());

	// Convert to GLFT and back.
	Node *loaded = gltf_export_then_import(original, "gltf_extras");

	// Compare the results.
	CHECK(loaded->get_name() == "node3d");
	CHECK(Dictionary(loaded->get_meta("extras")).size() == 1);
	CHECK(Dictionary(loaded->get_meta("extras"))["node_type"] == "node3d");
	CHECK_FALSE(loaded->has_meta("meta_not_nested_under_extras"));
	CHECK_FALSE(Dictionary(loaded->get_meta("extras")).has("meta_not_nested_under_extras"));

	MeshInstance3D *mesh_instance_3d = Object::cast_to<MeshInstance3D>(loaded->find_child("mesh_instance_3d", false, true));
	CHECK(mesh_instance_3d->get_name() == "mesh_instance_3d");
	CHECK(Dictionary(mesh_instance_3d->get_meta("extras"))["node_type"] == "mesh_instance_3d");

	Ref<Mesh> mesh = mesh_instance_3d->get_mesh();
	CHECK(Dictionary(mesh->get_meta("extras"))["node_type"] == "planemesh");

	Ref<Material> material = mesh->surface_get_material(0);
	CHECK(material->get_name() == "material");
	CHECK(Dictionary(material->get_meta("extras"))["node_type"] == "material");

	memdelete(original_mesh_instance);
	memdelete(original);
	memdelete(loaded);
}

TEST_CASE("[SceneTree][Node] GLTF test skeleton and bone export and import") {
	init("gltf_skeleton_extras");
	// Setup scene.
	Skeleton3D *skeleton = memnew(Skeleton3D);
	skeleton->set_name("skeleton");
	Dictionary skeleton_extras;
	skeleton_extras["node_type"] = "skeleton";
	skeleton->set_meta("extras", skeleton_extras);

	skeleton->add_bone("parent");
	skeleton->set_bone_rest(0, Transform3D());
	Dictionary parent_bone_extras;
	parent_bone_extras["bone"] = "i_am_parent_bone";
	skeleton->set_bone_meta(0, "extras", parent_bone_extras);

	skeleton->add_bone("child");
	skeleton->set_bone_rest(1, Transform3D());
	skeleton->set_bone_parent(1, 0);
	Dictionary child_bone_extras;
	child_bone_extras["bone"] = "i_am_child_bone";
	skeleton->set_bone_meta(1, "extras", child_bone_extras);

	// We have to have a mesh to link with skeleton or it will not get imported.
	Ref<PrimitiveMesh> meshdata = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
	meshdata->set_name("planemesh");

	MeshInstance3D *mesh = memnew(MeshInstance3D);
	mesh->set_mesh(meshdata);
	mesh->set_name("mesh_instance_3d");

	Node3D *original = memnew(Node3D);
	SceneTree::get_root()->add_child(original);
	original->add_child(skeleton);
	original->add_child(mesh);
	original->set_name("node3d");

	// Now that both skeleton and mesh are part of scene, link them.
	mesh->set_skeleton_path(mesh->get_path_to(skeleton));

	mesh->set_owner(SceneTree::get_root());
	original->set_owner(SceneTree::get_root());

	// Convert to GLFT and back.
	Node *loaded = gltf_export_then_import(original, "gltf_bone_extras");

	// Compare the results.
	CHECK(loaded->get_name() == "node3d");
	Skeleton3D *result = Object::cast_to<Skeleton3D>(loaded->find_child("Skeleton3D", false, true));
	CHECK(result->get_bone_name(0) == "parent");
	CHECK(Dictionary(result->get_bone_meta(0, "extras"))["bone"] == "i_am_parent_bone");
	CHECK(result->get_bone_name(1) == "child");
	CHECK(Dictionary(result->get_bone_meta(1, "extras"))["bone"] == "i_am_child_bone");

	memdelete(skeleton);
	memdelete(mesh);
	memdelete(original);
	memdelete(loaded);
}
} //namespace TestGltf

#endif // TOOLS_ENABLED
// --- End modules/gltf/tests/test_gltf_extras.h ---

// --- Folded from modules/gltf/tests/test_gltf_images.h ---
#ifdef TOOLS_ENABLED



#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

namespace TestGltf {
Ref<Texture2D> _check_texture(Node *p_node) {
	MeshInstance3D *mesh_instance_3d = Object::cast_to<MeshInstance3D>(p_node->find_child("mesh_instance_3d", true, true));
	Ref<StandardMaterial3D> material = mesh_instance_3d->get_active_material(0);
	Ref<Texture2D> texture = material->get_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO);

	CHECK_MESSAGE(texture->get_size().x == 2, "Texture width not correct.");
	CHECK_MESSAGE(texture->get_size().y == 2, "Texture height not correct.");

	// Check if the loaded texture pixels are exactly as we expect.
	for (int x = 0; x < 2; ++x) {
		for (int y = 0; y < 2; ++y) {
			Color c = texture->get_image()->get_pixel(x, y);
			CHECK_MESSAGE(c == Color(x, y, y), "Texture content is incorrect.");
		}
	}
	return texture;
}

TEST_CASE("[SceneTree][Node] Export GLTF with external texture and import") {
	init("gltf_images_external_export_import");
	// Setup scene.
	Ref<ImageTexture> original_texture;
	original_texture.instantiate();
	Ref<Image> image;
	image.instantiate();
	image->initialize_data(2, 2, false, Image::FORMAT_RGBA8);
	for (int x = 0; x < 2; ++x) {
		for (int y = 0; y < 2; ++y) {
			image->set_pixel(x, y, Color(x, y, y));
		}
	}

	original_texture->set_image(image);

	Ref<StandardMaterial3D> original_material;
	original_material.instantiate();
	original_material->set_texture(StandardMaterial3D::TextureParam::TEXTURE_ALBEDO, original_texture);
	original_material->set_name("material");

	Ref<PrimitiveMesh> original_meshdata;
	original_meshdata.instantiate(PrimitiveMesh::MESH_PLANE);
	original_meshdata->set_name("planemesh");
	original_meshdata->surface_set_material(0, original_material);

	MeshInstance3D *original_mesh_instance = memnew(MeshInstance3D);
	original_mesh_instance->set_mesh(original_meshdata);
	original_mesh_instance->set_name("mesh_instance_3d");

	Node3D *original = memnew(Node3D);
	SceneTree::get_root()->add_child(original);
	original->add_child(original_mesh_instance);
	original->set_owner(SceneTree::get_root());
	original_mesh_instance->set_owner(SceneTree::get_root());

	// Convert to GLFT and back.
	Node *loaded = gltf_export_then_import(original, "gltf_images");
	_check_texture(loaded);

	memdelete(original_mesh_instance);
	memdelete(original);
	memdelete(loaded);
}

TEST_CASE("[SceneTree][Node][Editor] Import GLTF from .godot/imported folder with external texture") {
	init("gltf_placed_in_dot_godot_imported", "res://.godot/imported");

	EditorFileSystem *efs = memnew(EditorFileSystem);
	EditorResourcePreview *erp = memnew(EditorResourcePreview);

	ERR_PRINT_OFF
	Node *loaded = gltf_import("res://.godot/imported/gltf_placed_in_dot_godot_imported.gltf");
	Ref<Texture2D> texture = _check_texture(loaded);
	ERR_PRINT_ON

	// In-editor imports of gltf and texture from .godot/imported folder should end up in res:// if extract_path is defined.
	CHECK_MESSAGE(texture->get_path() == "res://gltf_placed_in_dot_godot_imported_material_albedo000.png", "Texture not parsed as resource.");

	memdelete(loaded);
	memdelete(erp);
	memdelete(efs);
}

TEST_CASE("[SceneTree][Node][Editor] Import GLTF with texture outside of res:// directory") {
	init("gltf_pointing_to_texture_outside_of_res_folder", "res://");

	EditorFileSystem *efs = memnew(EditorFileSystem);
	EditorResourcePreview *erp = memnew(EditorResourcePreview);

	// Copy texture to the parent folder of res:// - i.e. to res://.. where we can't import from.
	String oneup = TestUtils::get_temp_path("texture.png");
	Error err;
	Ref<FileAccess> output = FileAccess::open(oneup, FileAccess::WRITE, &err);
	CHECK_MESSAGE(err == OK, "Unable to open texture file.");
	output->store_buffer(FileAccess::get_file_as_bytes("res://texture_source.png"));
	output->close();

	ERR_PRINT_OFF
	Node *loaded = gltf_import("res://gltf_pointing_to_texture_outside_of_res_folder.gltf");
	Ref<Texture2D> texture = _check_texture(loaded);
	ERR_PRINT_ON

	// Imports of gltf with texture from outside of res:// folder should end up being copied to res://
	CHECK_MESSAGE(texture->get_path() == "res://gltf_pointing_to_texture_outside_of_res_folder_material_albedo000.png", "Texture not parsed as resource.");

	memdelete(loaded);
	memdelete(erp);
	memdelete(efs);
}

TEST_CASE("[SceneTree][Node][Editor] Import GLTF with embedded texture, check how it got extracted") {
	init("gltf_embedded_texture", "res://");

	EditorFileSystem *efs = memnew(EditorFileSystem);
	EditorResourcePreview *erp = memnew(EditorResourcePreview);

	ERR_PRINT_OFF
	Node *loaded = gltf_import("res://embedded_texture.gltf");
	Ref<Texture2D> texture = _check_texture(loaded);
	ERR_PRINT_ON

	// In-editor imports of texture embedded in file should end up with a resource.
	CHECK_MESSAGE(texture->get_path() == "res://embedded_texture_material_albedo000.png", "Texture not parsed as resource.");

	memdelete(loaded);
	memdelete(erp);
	memdelete(efs);
}

} //namespace TestGltf

#endif // TOOLS_ENABLED
// --- End modules/gltf/tests/test_gltf_images.h ---
#endif // MODULE_GLTF_ENABLED

// === Merged from modules/multiplayer/tests/test_scene_multiplayer.h ===
#ifdef MODULE_MULTIPLAYER_ENABLED
#include "scene/main/scene_tree.h"

#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestSceneMultiplayer {

TEST_CASE("[Multiplayer][MultiplayerAPI] Defaults") {
	Ref<MultiplayerAPI> multiplayer;
	multiplayer.instantiate();

	REQUIRE(multiplayer->has_multiplayer_peer());
	Ref<MultiplayerPeer> multiplayer_peer = multiplayer->get_multiplayer_peer();
	REQUIRE_MESSAGE(dynamic_cast<OfflineMultiplayerPeer *>(multiplayer_peer.ptr()) != nullptr, "By default it must be an OfflineMultiplayerPeer instance.");
	CHECK_EQ(multiplayer->poll(), Error::OK);
	CHECK_EQ(multiplayer->get_unique_id(), MultiplayerPeer::TARGET_PEER_SERVER);
	CHECK_EQ(multiplayer->get_peer_ids(), Vector<int>());
	CHECK_EQ(multiplayer->get_remote_sender_id(), 0);
	CHECK_EQ(multiplayer->get_root_path(), NodePath());
	CHECK(multiplayer->get_connected_peers().is_empty());
	CHECK_FALSE(multiplayer->is_refusing_new_connections());
	CHECK_FALSE(multiplayer->is_object_decoding_allowed());
	CHECK(multiplayer->is_server_relay_enabled());
	CHECK_EQ(multiplayer->get_max_sync_packet_size(), 1350);
	CHECK_EQ(multiplayer->get_max_delta_packet_size(), 65535);
	CHECK(multiplayer->is_server());
}

TEST_CASE("[Multiplayer][MultiplayerAPI][SceneTree] SceneTree has an OfflineMultiplayerPeer by default") {
	Ref<MultiplayerAPI> multiplayer = SceneTree::get_multiplayer();
	REQUIRE(multiplayer->has_multiplayer_peer());

	Ref<MultiplayerPeer> multiplayer_peer = multiplayer->get_multiplayer_peer();
	REQUIRE_MESSAGE(dynamic_cast<OfflineMultiplayerPeer *>(multiplayer_peer.ptr()) != nullptr, "By default it must be an OfflineMultiplayerPeer instance.");
}

TEST_CASE("[Multiplayer][MultiplayerAPI] Root Path") {
	Ref<MultiplayerAPI> multiplayer;
	multiplayer.instantiate();

	SUBCASE("Is set") {
		NodePath foo_path("/Foo");
		multiplayer->set_root_path(foo_path);

		CHECK_EQ(multiplayer->get_root_path(), foo_path);
	}

	SUBCASE("Allows empty path") {
		multiplayer->set_root_path(NodePath());
		CHECK_EQ(multiplayer->get_root_path(), NodePath());
	}

	SUBCASE("Fails when path is relative") {
		NodePath foo_path("Foo");
		ERR_PRINT_OFF;
		multiplayer->set_root_path(foo_path);
		ERR_PRINT_ON;

		CHECK_EQ(multiplayer->get_root_path(), NodePath());
	}
}

TEST_CASE("[Multiplayer][MultiplayerAPI] Authentication validation") {
	Ref<MultiplayerAPI> multiplayer;
	multiplayer.instantiate();

	SUBCASE("Send auth fails when there is no MultiplayerPeer configured") {
		multiplayer->set_multiplayer_peer(nullptr);

		ERR_PRINT_OFF;
		CHECK_EQ(multiplayer->send_auth(42, Vector<uint8_t>()), Error::ERR_UNCONFIGURED);
		CHECK_EQ(multiplayer->complete_auth(42), Error::ERR_UNCONFIGURED);
		ERR_PRINT_ON;
	}

	SUBCASE("Send/complete auth fails when peer is not pending") {
		ERR_PRINT_OFF;
		CHECK_EQ(multiplayer->send_auth(42, String("It's me").to_ascii_buffer()), Error::ERR_INVALID_PARAMETER);
		CHECK_EQ(multiplayer->complete_auth(42), Error::ERR_INVALID_PARAMETER);
		ERR_PRINT_ON;
	}
}

} // namespace TestSceneMultiplayer

// --- Folded from modules/multiplayer/tests/test_multiplayer_spawner.h ---
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestMultiplayerSpawner
{
class Wasp : public Node
{
	int _size = 0;

public:
	int get_size() const { return _size; }

	void set_size(int p_size) { _size = p_size; }

	Wasp()
	{
		set_name("Wasp");
		set_scene_file_path("wasp.tscn");
	}
};

class SpawnWasps
{
public:
	Wasp* create_wasps(int p_size)
	{
		Wasp* wasp = memnew(Wasp);
		wasp->set_size(p_size);
		return wasp;
	}

	int echo_size(int p_size) { return p_size; }
};

} // namespace TestMultiplayerSpawner
// --- End modules/multiplayer/tests/test_multiplayer_spawner.h ---
#endif // MODULE_MULTIPLAYER_ENABLED

// === Merged from modules/noise/tests/test_fastnoise_lite.h ===
#ifdef MODULE_NOISE_ENABLED
#include "servers/rendering/renderer.h"

#include "tests/test_macros.h"

namespace TestFastNoiseLite {

// Uitility functions for finding differences in noise generation

bool all_equal_approx(const Vector<real_t> &p_values_1, const Vector<real_t> &p_values_2) {
	ERR_FAIL_COND_V_MSG(p_values_1.size() != p_values_2.size(), false, "Arrays must be the same size. This is a error in the test code.");

	for (int i = 0; i < p_values_1.size(); i++) {
		if (!Math::is_equal_approx(p_values_1[i], p_values_2[i])) {
			return false;
		}
	}
	return true;
}

Vector<Pair<size_t, size_t>> find_approx_equal_vec_pairs(std::initializer_list<Vector<real_t>> inputs) {
	Vector<Vector<real_t>> p_array = Vector<Vector<real_t>>(inputs);

	Vector<Pair<size_t, size_t>> result;
	for (int i = 0; i < p_array.size(); i++) {
		for (int j = i + 1; j < p_array.size(); j++) {
			if (all_equal_approx(p_array[i], p_array[j])) {
				result.push_back(Pair<size_t, size_t>(i, j));
			}
		}
	}
	return result;
}

#define CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(...) \
	{ \
		Vector<Pair<size_t, size_t>> equal_pairs = find_approx_equal_vec_pairs({ __VA_ARGS__ }); \
		for (Pair<size_t, size_t> p : equal_pairs) { \
			MESSAGE("Argument with index ", p.first, " is approximately equal to argument with index ", p.second); \
		} \
		CHECK_MESSAGE(equal_pairs.size() == 0, "All arguments should be pairwise distinct."); \
	}

Vector<real_t> get_noise_samples_1d(const FastNoiseLite &p_noise, size_t p_count = 32) {
	Vector<real_t> result;
	result.resize(p_count);
	for (size_t i = 0; i < p_count; i++) {
		result.write[i] = p_noise.get_noise_1d(i);
	}
	return result;
}

Vector<real_t> get_noise_samples_2d(const FastNoiseLite &p_noise, size_t p_count = 32) {
	Vector<real_t> result;
	result.resize(p_count);
	for (size_t i = 0; i < p_count; i++) {
		result.write[i] = p_noise.get_noise_2d(i, i);
	}
	return result;
}

Vector<real_t> get_noise_samples_3d(const FastNoiseLite &p_noise, size_t p_count = 32) {
	Vector<real_t> result;
	result.resize(p_count);
	for (size_t i = 0; i < p_count; i++) {
		result.write[i] = p_noise.get_noise_3d(i, i, i);
	}
	return result;
}

// The following test suite is rather for testing the wrapper code than the actual noise generation.

TEST_CASE("[FastNoiseLite] Getter and setter") {
	FastNoiseLite noise;

	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
	CHECK(noise.get_noise_type() == FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);

	noise.set_seed(123);
	CHECK(noise.get_seed() == 123);

	noise.set_frequency(0.123);
	CHECK(noise.get_frequency() == doctest::Approx(0.123));

	noise.set_offset(Vector3(1, 2, 3));
	CHECK(noise.get_offset() == Vector3(1, 2, 3));

	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_PING_PONG);
	CHECK(noise.get_fractal_type() == FastNoiseLite::FractalType::FRACTAL_PING_PONG);

	noise.set_fractal_octaves(2);
	CHECK(noise.get_fractal_octaves() == 2);

	noise.set_fractal_lacunarity(1.123);
	CHECK(noise.get_fractal_lacunarity() == doctest::Approx(1.123));

	noise.set_fractal_gain(0.123);
	CHECK(noise.get_fractal_gain() == doctest::Approx(0.123));

	noise.set_fractal_weighted_strength(0.123);
	CHECK(noise.get_fractal_weighted_strength() == doctest::Approx(0.123));

	noise.set_fractal_ping_pong_strength(0.123);
	CHECK(noise.get_fractal_ping_pong_strength() == doctest::Approx(0.123));

	noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_MANHATTAN);
	CHECK(noise.get_cellular_distance_function() == FastNoiseLite::CellularDistanceFunction::DISTANCE_MANHATTAN);

	noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_SUB);
	CHECK(noise.get_cellular_return_type() == FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_SUB);

	noise.set_cellular_jitter(0.123);
	CHECK(noise.get_cellular_jitter() == doctest::Approx(0.123));

	noise.set_domain_warp_enabled(true);
	CHECK(noise.is_domain_warp_enabled() == true);
	noise.set_domain_warp_enabled(false);
	CHECK(noise.is_domain_warp_enabled() == false);

	noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX_REDUCED);
	CHECK(noise.get_domain_warp_type() == FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX_REDUCED);

	noise.set_domain_warp_amplitude(0.123);
	CHECK(noise.get_domain_warp_amplitude() == doctest::Approx(0.123));

	noise.set_domain_warp_frequency(0.123);
	CHECK(noise.get_domain_warp_frequency() == doctest::Approx(0.123));

	noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_INDEPENDENT);
	CHECK(noise.get_domain_warp_fractal_type() == FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_INDEPENDENT);

	noise.set_domain_warp_fractal_octaves(2);
	CHECK(noise.get_domain_warp_fractal_octaves() == 2);

	noise.set_domain_warp_fractal_lacunarity(1.123);
	CHECK(noise.get_domain_warp_fractal_lacunarity() == doctest::Approx(1.123));

	noise.set_domain_warp_fractal_gain(0.123);
	CHECK(noise.get_domain_warp_fractal_gain() == doctest::Approx(0.123));
}

TEST_CASE("[FastNoiseLite] Basic noise generation") {
	FastNoiseLite noise;
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
	noise.set_seed(123);
	noise.set_offset(Vector3(10, 10, 10));

	// 1D noise will be checked just in the cases where there's the possibility of
	// finding a bug/regression in the wrapper function.
	// (since it uses FastNoise's 2D noise generator with the Y coordinate set to 0).

	SUBCASE("Determinacy of noise generation (all noise types)") {
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_PERLIN);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE_CUBIC);
		CHECK(noise.get_noise_2d(0, 0) == doctest::Approx(noise.get_noise_2d(0, 0)));
		CHECK(noise.get_noise_3d(0, 0, 0) == doctest::Approx(noise.get_noise_3d(0, 0, 0)));
	}

	SUBCASE("Different seeds should produce different noise") {
		noise.set_seed(456);
		Vector<real_t> noise_seed_1_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_seed_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_seed_1_3d = get_noise_samples_3d(noise);
		noise.set_seed(123);
		Vector<real_t> noise_seed_2_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_seed_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_seed_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(noise_seed_1_1d, noise_seed_2_1d));
		CHECK_FALSE(all_equal_approx(noise_seed_1_2d, noise_seed_2_2d));
		CHECK_FALSE(all_equal_approx(noise_seed_1_3d, noise_seed_2_3d));
	}

	SUBCASE("Different frequencies should produce different noise") {
		noise.set_frequency(0.1);
		Vector<real_t> noise_frequency_1_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_frequency_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_frequency_1_3d = get_noise_samples_3d(noise);
		noise.set_frequency(1.0);
		Vector<real_t> noise_frequency_2_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_frequency_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_frequency_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(noise_frequency_1_1d, noise_frequency_2_1d));
		CHECK_FALSE(all_equal_approx(noise_frequency_1_2d, noise_frequency_2_2d));
		CHECK_FALSE(all_equal_approx(noise_frequency_1_3d, noise_frequency_2_3d));
	}

	SUBCASE("Noise should be offset by the offset parameter") {
		noise.set_offset(Vector3(1, 2, 3));
		Vector<real_t> noise_offset_1_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_offset_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_offset_1_3d = get_noise_samples_3d(noise);
		noise.set_offset(Vector3(4, 5, 6));
		Vector<real_t> noise_offset_2_1d = get_noise_samples_1d(noise);
		Vector<real_t> noise_offset_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_offset_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(noise_offset_1_1d, noise_offset_2_1d));
		CHECK_FALSE(all_equal_approx(noise_offset_1_2d, noise_offset_2_2d));
		CHECK_FALSE(all_equal_approx(noise_offset_1_3d, noise_offset_2_3d));
	}

	SUBCASE("Different noise types should produce different noise") {
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
		Vector<real_t> noise_type_simplex_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_simplex_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
		Vector<real_t> noise_type_simplex_smooth_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_simplex_smooth_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
		Vector<real_t> noise_type_cellular_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_cellular_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_PERLIN);
		Vector<real_t> noise_type_perlin_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_perlin_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE);
		Vector<real_t> noise_type_value_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_value_3d = get_noise_samples_3d(noise);
		noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_VALUE_CUBIC);
		Vector<real_t> noise_type_value_cubic_2d = get_noise_samples_2d(noise);
		Vector<real_t> noise_type_value_cubic_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(noise_type_simplex_2d,
				noise_type_simplex_smooth_2d,
				noise_type_cellular_2d,
				noise_type_perlin_2d,
				noise_type_value_2d,
				noise_type_value_cubic_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(noise_type_simplex_3d,
				noise_type_simplex_smooth_3d,
				noise_type_cellular_3d,
				noise_type_perlin_3d,
				noise_type_value_3d,
				noise_type_value_cubic_3d);
	}
}

TEST_CASE("[FastNoiseLite] Fractal noise") {
	FastNoiseLite noise;
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
	noise.set_offset(Vector3(10, 10, 10));
	noise.set_frequency(0.01);
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_FBM);
	noise.set_fractal_octaves(4);
	noise.set_fractal_lacunarity(2.0);
	noise.set_fractal_gain(0.5);
	noise.set_fractal_weighted_strength(0.5);
	noise.set_fractal_ping_pong_strength(2.0);

	SUBCASE("Different fractal types should produce different results") {
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
		Vector<real_t> fractal_type_none_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_none_3d = get_noise_samples_3d(noise);
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_FBM);
		Vector<real_t> fractal_type_fbm_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_fbm_3d = get_noise_samples_3d(noise);
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_RIDGED);
		Vector<real_t> fractal_type_ridged_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_ridged_3d = get_noise_samples_3d(noise);
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_PING_PONG);
		Vector<real_t> fractal_type_ping_pong_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_type_ping_pong_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(fractal_type_none_2d,
				fractal_type_fbm_2d,
				fractal_type_ridged_2d,
				fractal_type_ping_pong_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(fractal_type_none_3d,
				fractal_type_fbm_3d,
				fractal_type_ridged_3d,
				fractal_type_ping_pong_3d);
	}

	SUBCASE("Different octaves should produce different results") {
		noise.set_fractal_octaves(1.0);
		Vector<real_t> fractal_octaves_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_octaves_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_octaves(8.0);
		Vector<real_t> fractal_octaves_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_octaves_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_octaves_1_2d, fractal_octaves_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_octaves_1_3d, fractal_octaves_2_3d));
	}

	SUBCASE("Different lacunarity should produce different results") {
		noise.set_fractal_lacunarity(1.0);
		Vector<real_t> fractal_lacunarity_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_lacunarity_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_lacunarity(2.0);
		Vector<real_t> fractal_lacunarity_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_lacunarity_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_lacunarity_1_2d, fractal_lacunarity_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_lacunarity_1_3d, fractal_lacunarity_2_3d));
	}

	SUBCASE("Different gain should produce different results") {
		noise.set_fractal_gain(0.5);
		Vector<real_t> fractal_gain_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_gain_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_gain(0.75);
		Vector<real_t> fractal_gain_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_gain_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_gain_1_2d, fractal_gain_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_gain_1_3d, fractal_gain_2_3d));
	}

	SUBCASE("Different weights should produce different results") {
		noise.set_fractal_weighted_strength(0.5);
		Vector<real_t> fractal_weighted_strength_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_weighted_strength_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_weighted_strength(0.75);
		Vector<real_t> fractal_weighted_strength_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_weighted_strength_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_weighted_strength_1_2d, fractal_weighted_strength_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_weighted_strength_1_3d, fractal_weighted_strength_2_3d));
	}

	SUBCASE("Different ping pong strength should produce different results") {
		noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_PING_PONG);
		noise.set_fractal_ping_pong_strength(0.5);
		Vector<real_t> fractal_ping_pong_strength_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_ping_pong_strength_1_3d = get_noise_samples_3d(noise);
		noise.set_fractal_ping_pong_strength(0.75);
		Vector<real_t> fractal_ping_pong_strength_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> fractal_ping_pong_strength_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(fractal_ping_pong_strength_1_2d, fractal_ping_pong_strength_2_2d));
		CHECK_FALSE(all_equal_approx(fractal_ping_pong_strength_1_3d, fractal_ping_pong_strength_2_3d));
	}
}

TEST_CASE("[FastNoiseLite] Cellular noise") {
	FastNoiseLite noise;
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
	noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN);
	noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE);
	noise.set_frequency(1.0);

	SUBCASE("Different distance functions should produce different results") {
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN);
		Vector<real_t> cellular_distance_function_euclidean_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_euclidean_3d = get_noise_samples_3d(noise);
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN_SQUARED);
		Vector<real_t> cellular_distance_function_euclidean_squared_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_euclidean_squared_3d = get_noise_samples_3d(noise);
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_MANHATTAN);
		Vector<real_t> cellular_distance_function_manhattan_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_manhattan_3d = get_noise_samples_3d(noise);
		noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_HYBRID);
		Vector<real_t> cellular_distance_function_hybrid_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_distance_function_hybrid_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_distance_function_euclidean_2d,
				cellular_distance_function_euclidean_squared_2d,
				cellular_distance_function_manhattan_2d,
				cellular_distance_function_hybrid_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_distance_function_euclidean_3d,
				cellular_distance_function_euclidean_squared_3d,
				cellular_distance_function_manhattan_3d,
				cellular_distance_function_hybrid_3d);
	}

	SUBCASE("Different return function types should produce different results") {
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_CELL_VALUE);
		Vector<real_t> cellular_return_type_cell_value_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_cell_value_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE);
		Vector<real_t> cellular_return_type_distance_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2);
		Vector<real_t> cellular_return_type_distance2_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_ADD);
		Vector<real_t> cellular_return_type_distance2_add_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_add_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_SUB);
		Vector<real_t> cellular_return_type_distance2_sub_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_sub_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_MUL);
		Vector<real_t> cellular_return_type_distance2_mul_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_mul_3d = get_noise_samples_3d(noise);
		noise.set_cellular_return_type(FastNoiseLite::CellularReturnType::RETURN_DISTANCE2_DIV);
		Vector<real_t> cellular_return_type_distance2_div_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_return_type_distance2_div_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_return_type_cell_value_2d,
				cellular_return_type_distance_2d,
				cellular_return_type_distance2_2d,
				cellular_return_type_distance2_add_2d,
				cellular_return_type_distance2_sub_2d,
				cellular_return_type_distance2_mul_2d,
				cellular_return_type_distance2_div_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(cellular_return_type_cell_value_3d,
				cellular_return_type_distance_3d,
				cellular_return_type_distance2_3d,
				cellular_return_type_distance2_add_3d,
				cellular_return_type_distance2_sub_3d,
				cellular_return_type_distance2_mul_3d,
				cellular_return_type_distance2_div_3d);
	}

	SUBCASE("Different cellular jitter should produce different results") {
		noise.set_cellular_jitter(0.0);
		Vector<real_t> cellular_jitter_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_jitter_1_3d = get_noise_samples_3d(noise);
		noise.set_cellular_jitter(0.5);
		Vector<real_t> cellular_jitter_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> cellular_jitter_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(cellular_jitter_1_2d, cellular_jitter_2_2d));
		CHECK_FALSE(all_equal_approx(cellular_jitter_1_3d, cellular_jitter_2_3d));
	}
}

TEST_CASE("[FastNoiseLite] Domain warp") {
	FastNoiseLite noise;
	noise.set_frequency(1.0);
	noise.set_domain_warp_amplitude(200.0);
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_SIMPLEX);
	noise.set_domain_warp_enabled(true);

	SUBCASE("Different domain warp types should produce different results") {
		noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX);
		Vector<real_t> domain_warp_type_simplex_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_type_simplex_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_SIMPLEX_REDUCED);
		Vector<real_t> domain_warp_type_simplex_reduced_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_type_simplex_reduced_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_type(FastNoiseLite::DomainWarpType::DOMAIN_WARP_BASIC_GRID);
		Vector<real_t> domain_warp_type_basic_grid_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_type_basic_grid_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_type_simplex_2d,
				domain_warp_type_simplex_reduced_2d,
				domain_warp_type_basic_grid_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_type_simplex_3d,
				domain_warp_type_simplex_reduced_3d,
				domain_warp_type_basic_grid_3d);
	}

	SUBCASE("Different domain warp amplitude should produce different results") {
		noise.set_domain_warp_amplitude(0.0);
		Vector<real_t> domain_warp_amplitude_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_amplitude_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_amplitude(100.0);
		Vector<real_t> domain_warp_amplitude_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_amplitude_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_amplitude_1_2d, domain_warp_amplitude_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_amplitude_1_3d, domain_warp_amplitude_2_3d));
	}

	SUBCASE("Different domain warp frequency should produce different results") {
		noise.set_domain_warp_frequency(0.1);
		Vector<real_t> domain_warp_frequency_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_frequency_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_frequency(2.0);
		Vector<real_t> domain_warp_frequency_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_frequency_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_frequency_1_2d, domain_warp_frequency_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_frequency_1_3d, domain_warp_frequency_2_3d));
	}

	SUBCASE("Different domain warp fractal type should produce different results") {
		noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_NONE);
		Vector<real_t> domain_warp_fractal_type_none_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_type_none_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_PROGRESSIVE);
		Vector<real_t> domain_warp_fractal_type_progressive_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_type_progressive_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_type(FastNoiseLite::DomainWarpFractalType::DOMAIN_WARP_FRACTAL_INDEPENDENT);
		Vector<real_t> domain_warp_fractal_type_independent_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_type_independent_3d = get_noise_samples_3d(noise);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_fractal_type_none_2d,
				domain_warp_fractal_type_progressive_2d,
				domain_warp_fractal_type_independent_2d);

		CHECK_ARGS_APPROX_PAIRWISE_DISTINCT_VECS(domain_warp_fractal_type_none_3d,
				domain_warp_fractal_type_progressive_3d,
				domain_warp_fractal_type_independent_3d);
	}

	SUBCASE("Different domain warp fractal octaves should produce different results") {
		noise.set_domain_warp_fractal_octaves(1);
		Vector<real_t> domain_warp_fractal_octaves_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_octaves_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_octaves(6);
		Vector<real_t> domain_warp_fractal_octaves_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_octaves_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_fractal_octaves_1_2d, domain_warp_fractal_octaves_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_fractal_octaves_1_3d, domain_warp_fractal_octaves_2_3d));
	}

	SUBCASE("Different domain warp fractal lacunarity should produce different results") {
		noise.set_domain_warp_fractal_lacunarity(0.5);
		Vector<real_t> domain_warp_fractal_lacunarity_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_lacunarity_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_lacunarity(5.0);
		Vector<real_t> domain_warp_fractal_lacunarity_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_lacunarity_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_fractal_lacunarity_1_2d, domain_warp_fractal_lacunarity_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_fractal_lacunarity_1_3d, domain_warp_fractal_lacunarity_2_3d));
	}

	SUBCASE("Different domain warp fractal gain should produce different results") {
		noise.set_domain_warp_fractal_gain(0.1);
		Vector<real_t> domain_warp_fractal_gain_1_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_gain_1_3d = get_noise_samples_3d(noise);
		noise.set_domain_warp_fractal_gain(0.9);
		Vector<real_t> domain_warp_fractal_gain_2_2d = get_noise_samples_2d(noise);
		Vector<real_t> domain_warp_fractal_gain_2_3d = get_noise_samples_3d(noise);

		CHECK_FALSE(all_equal_approx(domain_warp_fractal_gain_1_2d, domain_warp_fractal_gain_2_2d));
		CHECK_FALSE(all_equal_approx(domain_warp_fractal_gain_1_3d, domain_warp_fractal_gain_2_3d));
	}
}

// Raw image data for the reference images used in the regression tests.
// Generated with the following code:
//     for (int y = 0; y < img->get_data().size(); y++) {
//         printf("0x%x,", img->get_data()[y]);
//     }

const Vector<uint8_t> ref_img_1_data = { 0xff, 0xe6, 0xd2, 0xc2, 0xb7, 0xb4, 0xb4, 0xb7, 0xc2, 0xd2, 0xe6, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcb, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x65, 0x82, 0xa1, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x32, 0x50, 0x72, 0x94, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x33, 0x50, 0x72, 0x94, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x66, 0x82, 0xa1, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcc };
const Vector<uint8_t> ref_img_2_data = { 0xff, 0xe6, 0xd2, 0xc2, 0xb7, 0xb4, 0xb4, 0xb7, 0xc2, 0xd2, 0xe6, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcb, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x65, 0x82, 0xa1, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x32, 0x50, 0x72, 0x94, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x33, 0x50, 0x72, 0x94, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x66, 0x82, 0xa1, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcc };
const Vector<uint8_t> ref_img_3_data = { 0xff, 0xe6, 0xd2, 0xc2, 0xb7, 0xb4, 0xb4, 0xb7, 0xc2, 0xd2, 0xe6, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcb, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x65, 0x82, 0xa1, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x32, 0x50, 0x72, 0x94, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb4, 0x90, 0x6c, 0x48, 0x24, 0x0, 0x0, 0x24, 0x48, 0x6c, 0x90, 0xb7, 0x94, 0x72, 0x50, 0x32, 0x24, 0x24, 0x33, 0x50, 0x72, 0x94, 0xc2, 0xa1, 0x82, 0x65, 0x50, 0x48, 0x48, 0x50, 0x66, 0x82, 0xa1, 0xd2, 0xb4, 0x99, 0x82, 0x72, 0x6c, 0x6c, 0x72, 0x82, 0x99, 0xb4, 0xe6, 0xcb, 0xb4, 0xa1, 0x94, 0x90, 0x90, 0x94, 0xa1, 0xb4, 0xcc };

// Utility function to compare two images pixel by pixel (for easy debugging of regressions)
void compare_image_with_reference(const Ref<Image> &p_img, const Ref<Image> &p_reference_img) {
	for (int y = 0; y < p_img->get_height(); y++) {
		for (int x = 0; x < p_img->get_width(); x++) {
			CHECK(p_img->get_pixel(x, y) == p_reference_img->get_pixel(x, y));
		}
	}
}

TEST_CASE("[FastNoiseLite] Generating seamless 2D images (11x11px) and compare to reference images") {
	FastNoiseLite noise;
	noise.set_noise_type(FastNoiseLite::NoiseType::TYPE_CELLULAR);
	noise.set_fractal_type(FastNoiseLite::FractalType::FRACTAL_NONE);
	noise.set_cellular_distance_function(FastNoiseLite::CellularDistanceFunction::DISTANCE_EUCLIDEAN);
	noise.set_frequency(0.1);
	noise.set_cellular_jitter(0.0);

	SUBCASE("Blend skirt 0.0") {
		Ref<Image> img = noise.get_seamless_image(11, 11, false, false, 0.0);

		Ref<Image> ref_img_1 = memnew(Image);
		ref_img_1->set_data(11, 11, false, Image::FORMAT_L8, ref_img_1_data);

		compare_image_with_reference(img, ref_img_1);
	}

	SUBCASE("Blend skirt 0.1") {
		Ref<Image> img = noise.get_seamless_image(11, 11, false, false, 0.1);

		Ref<Image> ref_img_2 = memnew(Image);
		ref_img_2->set_data(11, 11, false, Image::FORMAT_L8, ref_img_2_data);

		compare_image_with_reference(img, ref_img_2);
	}

	SUBCASE("Blend skirt 1.0") {
		Ref<Image> img = noise.get_seamless_image(11, 11, false, false, 0.1);

		Ref<Image> ref_img_3 = memnew(Image);
		ref_img_3->set_data(11, 11, false, Image::FORMAT_L8, ref_img_3_data);

		compare_image_with_reference(img, ref_img_3);
	}
}

} //namespace TestFastNoiseLite

// --- Folded from modules/noise/tests/test_noise_texture_2d.h ---
#include "servers/rendering/renderer.h"

#include "tests/test_macros.h"

namespace TestNoiseTexture2D {

class NoiseTextureTester : public RefCounted {

	const NoiseTexture2D *const texture;

public:
	NoiseTextureTester(const NoiseTexture2D *const p_texture) :
			texture{ p_texture } {}

	Color compute_average_color(const Ref<Image> &p_noise_image) {
		Color r_avg_color{};

		for (int i = 0; i < p_noise_image->get_width(); ++i) {
			for (int j = 0; j < p_noise_image->get_height(); ++j) {
				const Color pixel = p_noise_image->get_pixel(i, j);
				r_avg_color += pixel;
			}
		}

		int pixel_count = p_noise_image->get_width() * p_noise_image->get_height();
		r_avg_color /= pixel_count;
		return r_avg_color;
	}

	void check_mip_and_color_ramp() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);
		CHECK(noise_image->has_mipmaps());

		Color avg_color = compute_average_color(noise_image);

		// Check that the noise texture is modulated correctly by the color ramp (Gradient).
		CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(0.0), "The noise texture should not be all black");
		CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(noise_image->get_width() * noise_image->get_height() * 3.0), "The noise texture should not be all white");
		CHECK_MESSAGE(avg_color.g == doctest::Approx(0.0), "The noise texture should not have any green when modulated correctly by the color ramp");
	}

	void check_normal_map() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);
		CHECK_FALSE(noise_image->has_mipmaps());

		Color avg_color = compute_average_color(noise_image);

		// Check for the characteristic color distribution (for tangent space) of a normal map.
		CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.g == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.b == doctest::Approx(1.0).epsilon(0.05));
	}

	void check_seamless_texture_grayscale() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_L8);

		Color avg_color = compute_average_color(noise_image);

		// Since it's a grayscale image and every channel except the alpha channel has the
		// same values (conversion happens in Image::get_pixel) we only need to test one channel.
		CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
	}

	void check_seamless_texture_rgba() {
		const Ref<Image> noise_image = texture->get_image();
		CHECK(noise_image.is_valid());
		CHECK(noise_image->get_width() == texture->get_width());
		CHECK(noise_image->get_height() == texture->get_height());

		CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);

		// Check that the noise texture is modulated correctly by the color ramp (Gradient).
		Color avg_color = compute_average_color(noise_image);

		// We use a default (black to white) gradient, so the average of the red, green and blue channels should be the same.
		CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.g == doctest::Approx(0.5).epsilon(0.05));
		CHECK(avg_color.b == doctest::Approx(0.5).epsilon(0.05));
	}
};

TEST_CASE("[NoiseTexture][SceneTree] Getter and setter") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);
	CHECK(noise_texture->get_noise() == noise);
	noise_texture->set_noise(nullptr);
	CHECK(noise_texture->get_noise().is_null());

	noise_texture->set_width(8);
	noise_texture->set_height(4);
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);

	ERR_PRINT_OFF;
	noise_texture->set_width(-1);
	noise_texture->set_height(-1);
	ERR_PRINT_ON;
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);

	noise_texture->set_invert(true);
	CHECK(noise_texture->get_invert() == true);
	noise_texture->set_invert(false);
	CHECK(noise_texture->get_invert() == false);

	noise_texture->set_in_3d_space(true);
	CHECK(noise_texture->is_in_3d_space() == true);
	noise_texture->set_in_3d_space(false);
	CHECK(noise_texture->is_in_3d_space() == false);

	noise_texture->set_generate_mipmaps(true);
	CHECK(noise_texture->is_generating_mipmaps() == true);
	noise_texture->set_generate_mipmaps(false);
	CHECK(noise_texture->is_generating_mipmaps() == false);

	noise_texture->set_seamless(true);
	CHECK(noise_texture->get_seamless() == true);
	noise_texture->set_seamless(false);
	CHECK(noise_texture->get_seamless() == false);

	noise_texture->set_seamless_blend_skirt(0.45);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));

	ERR_PRINT_OFF;
	noise_texture->set_seamless_blend_skirt(-1.0);
	noise_texture->set_seamless_blend_skirt(2.0);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));
	ERR_PRINT_ON;

	noise_texture->set_as_normal_map(true);
	CHECK(noise_texture->is_normal_map() == true);
	noise_texture->set_as_normal_map(false);
	CHECK(noise_texture->is_normal_map() == false);

	noise_texture->set_bump_strength(0.168);
	CHECK(noise_texture->get_bump_strength() == doctest::Approx(0.168));

	Ref<Gradient> gradient = memnew(Gradient);
	noise_texture->set_color_ramp(gradient);
	CHECK(noise_texture->get_color_ramp() == gradient);
	noise_texture->set_color_ramp(nullptr);
	CHECK(noise_texture->get_color_ramp().is_null());
}

TEST_CASE("[NoiseTexture2D][SceneTree] Generating a basic noise texture with mipmaps and color ramp modulation") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);

	Ref<Gradient> gradient = memnew(Gradient);
	Vector<float> offsets = { 0.0, 1.0 };
	Vector<Color> colors = { Color(1, 0, 0), Color(0, 0, 1) };
	gradient->set_offsets(offsets);
	gradient->set_colors(colors);

	noise_texture->set_color_ramp(gradient);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_generate_mipmaps(true);

	Ref<NoiseTextureTester> tester = memnew(NoiseTextureTester(noise_texture.ptr()));
	noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_mip_and_color_ramp));
}

TEST_CASE("[NoiseTexture2D][SceneTree] Generating a normal map without mipmaps") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise->set_frequency(0.5);
	noise_texture->set_noise(noise);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_as_normal_map(true);
	noise_texture->set_bump_strength(0.5);
	noise_texture->set_generate_mipmaps(false);

	Ref<NoiseTextureTester> tester = memnew(NoiseTextureTester(noise_texture.ptr()));
	noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_normal_map));
}

TEST_CASE("[NoiseTexture2D][SceneTree] Generating a seamless noise texture") {
	Ref<NoiseTexture2D> noise_texture = memnew(NoiseTexture2D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise->set_frequency(0.5);
	noise_texture->set_noise(noise);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_seamless(true);

	Ref<NoiseTextureTester> tester = memnew(NoiseTextureTester(noise_texture.ptr()));

	SUBCASE("Grayscale(L8) 16x16, with seamless blend skirt of 0.05") {
		noise_texture->set_seamless_blend_skirt(0.05);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_seamless_texture_grayscale));
	}

	SUBCASE("16x16 modulated with default (transparent)black and white gradient (RGBA8), with seamless blend skirt of 1.0") {
		Ref<Gradient> gradient = memnew(Gradient);

		Vector<float> offsets = { 0.0, 1.0 };
		Vector<Color> colors = { Color(0, 0, 0, 0), Color(1, 1, 1, 1) };
		gradient->set_offsets(offsets);
		gradient->set_colors(colors);

		noise_texture->set_color_ramp(gradient);
		noise_texture->set_seamless_blend_skirt(1.0);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTextureTester::check_seamless_texture_rgba));
	}
}

} //namespace TestNoiseTexture2D
// --- End modules/noise/tests/test_noise_texture_2d.h ---

// --- Folded from modules/noise/tests/test_noise_texture_3d.h ---
#include "servers/rendering/renderer.h"

#include "tests/test_macros.h"

namespace TestNoiseTexture3D {

class NoiseTexture3DTester : public RefCounted {

	const NoiseTexture3D *const texture;

public:
	NoiseTexture3DTester(const NoiseTexture3D *const p_texture) :
			texture{ p_texture } {}

	Color compute_average_color(const Ref<Image> &p_noise_image) {
		Color r_avg_color{};

		for (int i = 0; i < p_noise_image->get_width(); ++i) {
			for (int j = 0; j < p_noise_image->get_height(); ++j) {
				const Color pixel = p_noise_image->get_pixel(i, j);
				r_avg_color += pixel;
			}
		}

		int pixel_count = p_noise_image->get_width() * p_noise_image->get_height();
		r_avg_color /= pixel_count;
		return r_avg_color;
	}

	void check_mip_and_color_ramp() {
		const Vector<Ref<Image>> noise_data = texture->get_data();

		for (int i = 0; i < noise_data.size(); i++) {
			const Ref<Image> noise_image = noise_data[i];

			CHECK(noise_image.is_valid());
			CHECK(noise_image->get_width() == texture->get_width());
			CHECK(noise_image->get_height() == texture->get_height());

			CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);
			CHECK(!noise_image->has_mipmaps());

			Color avg_color = compute_average_color(noise_image);

			// Check that the noise texture is modulated correctly by the color ramp (Gradient).
			CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(0.0), "The noise texture should not be all black");
			CHECK_FALSE_MESSAGE((avg_color.r + avg_color.g + avg_color.b) == doctest::Approx(noise_image->get_width() * noise_image->get_height() * 3.0), "The noise texture should not be all white");
			CHECK_MESSAGE(avg_color.g == doctest::Approx(0.0), "The noise texture should not have any green when modulated correctly by the color ramp");
		}
	}

	void check_seamless_texture_grayscale() {
		const Vector<Ref<Image>> noise_data = texture->get_data();

		for (int i = 0; i < noise_data.size(); i++) {
			const Ref<Image> noise_image = noise_data[i];

			CHECK(noise_image.is_valid());
			CHECK(noise_image->get_width() == texture->get_width());
			CHECK(noise_image->get_height() == texture->get_height());

			CHECK(noise_image->get_format() == Image::FORMAT_L8);

			Color avg_color = compute_average_color(noise_image);

			// Since it's a grayscale image and every channel except the alpha channel has the
			// same values (conversion happens in Image::get_pixel) we only need to test one channel.
			CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
		}
	}

	void check_seamless_texture_rgba() {
		const Vector<Ref<Image>> noise_data = texture->get_data();

		for (int i = 0; i < noise_data.size(); i++) {
			const Ref<Image> noise_image = noise_data[i];

			CHECK(noise_image.is_valid());
			CHECK(noise_image->get_width() == texture->get_width());
			CHECK(noise_image->get_height() == texture->get_height());

			CHECK(noise_image->get_format() == Image::FORMAT_RGBA8);

			// Check that the noise texture is modulated correctly by the color ramp (Gradient).
			Color avg_color = compute_average_color(noise_image);

			// We use a default (black to white) gradient, so the average of the red, green and blue channels should be the same.
			CHECK(avg_color.r == doctest::Approx(0.5).epsilon(0.05));
			CHECK(avg_color.g == doctest::Approx(0.5).epsilon(0.05));
			CHECK(avg_color.b == doctest::Approx(0.5).epsilon(0.05));
		}
	}
};

TEST_CASE("[NoiseTexture][SceneTree] Getter and setter") {
	Ref<NoiseTexture3D> noise_texture = memnew(NoiseTexture3D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);
	CHECK(noise_texture->get_noise() == noise);
	noise_texture->set_noise(nullptr);
	CHECK(noise_texture->get_noise().is_null());

	noise_texture->set_width(8);
	noise_texture->set_height(4);
	noise_texture->set_depth(2);
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);
	CHECK(noise_texture->get_depth() == 2);

	ERR_PRINT_OFF;
	noise_texture->set_width(-1);
	noise_texture->set_height(-1);
	noise_texture->set_depth(-1);
	ERR_PRINT_ON;
	CHECK(noise_texture->get_width() == 8);
	CHECK(noise_texture->get_height() == 4);
	CHECK(noise_texture->get_depth() == 2);

	noise_texture->set_invert(true);
	CHECK(noise_texture->get_invert() == true);
	noise_texture->set_invert(false);
	CHECK(noise_texture->get_invert() == false);

	noise_texture->set_seamless(true);
	CHECK(noise_texture->get_seamless() == true);
	noise_texture->set_seamless(false);
	CHECK(noise_texture->get_seamless() == false);

	noise_texture->set_seamless_blend_skirt(0.45);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));

	ERR_PRINT_OFF;
	noise_texture->set_seamless_blend_skirt(-1.0);
	noise_texture->set_seamless_blend_skirt(2.0);
	CHECK(noise_texture->get_seamless_blend_skirt() == doctest::Approx(0.45));
	ERR_PRINT_ON;

	Ref<Gradient> gradient = memnew(Gradient);
	noise_texture->set_color_ramp(gradient);
	CHECK(noise_texture->get_color_ramp() == gradient);
	noise_texture->set_color_ramp(nullptr);
	CHECK(noise_texture->get_color_ramp().is_null());
}

TEST_CASE("[NoiseTexture3D][SceneTree] Generating a basic noise texture with mipmaps and color ramp modulation") {
	Ref<NoiseTexture3D> noise_texture = memnew(NoiseTexture3D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise_texture->set_noise(noise);

	Ref<Gradient> gradient = memnew(Gradient);
	Vector<float> offsets = { 0.0, 1.0 };
	Vector<Color> colors = { Color(1, 0, 0), Color(0, 0, 1) };
	gradient->set_offsets(offsets);
	gradient->set_colors(colors);

	noise_texture->set_color_ramp(gradient);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_depth(16);

	Ref<NoiseTexture3DTester> tester = memnew(NoiseTexture3DTester(noise_texture.ptr()));
	noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTexture3DTester::check_mip_and_color_ramp));
}

TEST_CASE("[NoiseTexture3D][SceneTree] Generating a seamless noise texture") {
	Ref<NoiseTexture3D> noise_texture = memnew(NoiseTexture3D);

	Ref<FastNoiseLite> noise = memnew(FastNoiseLite);
	noise->set_frequency(0.5);
	noise_texture->set_noise(noise);
	noise_texture->set_width(16);
	noise_texture->set_height(16);
	noise_texture->set_depth(16);
	noise_texture->set_seamless(true);

	Ref<NoiseTexture3DTester> tester = memnew(NoiseTexture3DTester(noise_texture.ptr()));

	SUBCASE("Grayscale(L8) 16x16x16, with seamless blend skirt of 0.05") {
		noise_texture->set_seamless_blend_skirt(0.05);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTexture3DTester::check_seamless_texture_grayscale));
	}

	SUBCASE("16x16x16 modulated with default (transparent)black and white gradient (RGBA8), with seamless blend skirt of 1.0") {
		Ref<Gradient> gradient = memnew(Gradient);

		Vector<float> offsets = { 0.0, 1.0 };
		Vector<Color> colors = { Color(0, 0, 0, 0), Color(1, 1, 1, 1) };
		gradient->set_offsets(offsets);
		gradient->set_colors(colors);

		noise_texture->set_color_ramp(gradient);
		noise_texture->set_seamless_blend_skirt(1.0);
		noise_texture->connect_changed(callable_mp(tester.ptr(), &NoiseTexture3DTester::check_seamless_texture_rgba));
	}
}

} //namespace TestNoiseTexture3D
// --- End modules/noise/tests/test_noise_texture_3d.h ---
#endif // MODULE_NOISE_ENABLED

// === Merged from modules/regex/tests/test_regex.h ===
#ifdef MODULE_REGEX_ENABLED
#include "core/os/os.h"

#include "core/string/ustring.h"
#include "tests/test_macros.h"

namespace TestRegEx {

TEST_CASE("[RegEx] Initialization") {
	const String pattern = "(?<vowel>[aeiou])";

	RegEx re1(pattern);
	CHECK(re1.is_valid());
	CHECK(re1.get_pattern() == pattern);
	CHECK(re1.get_group_count() == 1);

	PackedStringArray names = re1.get_names();
	CHECK(names.size() == 1);
	CHECK(names[0] == "vowel");

	RegEx re2;
	CHECK(re2.is_valid() == false);
	CHECK(re2.compile(pattern) == OK);
	CHECK(re2.is_valid());

	CHECK(re1.get_pattern() == re2.get_pattern());
	CHECK(re1.get_group_count() == re2.get_group_count());

	names = re2.get_names();
	CHECK(names.size() == 1);
	CHECK(names[0] == "vowel");
}

TEST_CASE("[RegEx] Clearing") {
	RegEx re("Godot");
	REQUIRE(re.is_valid());
	re.clear();
	CHECK(re.is_valid() == false);
}

TEST_CASE("[RegEx] Searching") {
	const String s = "Searching";
	const String vowels = "[aeiou]{1,2}";
	const String numerics = "\\d";

	RegEx re(vowels);
	REQUIRE(re.is_valid());

	Ref<RegExMatch> match = re.search(s);
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == "ea");

	match = re.search(s, 1, 2);
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == "e");
	match = re.search(s, 2, 4);
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == "a");
	match = re.search(s, 3, 5);
	CHECK(match.is_null());
	match = re.search(s, 6, 2);
	CHECK(match.is_null());

	const Array all_results = re.search_all(s);
	CHECK(all_results.size() == 2);
	match = all_results[0];
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == "ea");
	match = all_results[1];
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == "i");

	CHECK(re.compile(numerics) == OK);
	CHECK(re.is_valid());
	CHECK(re.search(s).is_null());
	CHECK(re.search_all(s).size() == 0);
}

TEST_CASE("[RegEx] Substitution") {
	const String s1 = "Double all the vowels.";

	RegEx re1("(?<vowel>[aeiou])");
	REQUIRE(re1.is_valid());
	CHECK(re1.sub(s1, "$0$vowel", true) == "Doouublee aall thee vooweels.");

	const String s2 = "Substitution with group.";

	RegEx re2("Substitution (.+)");
	REQUIRE(re2.is_valid());
	CHECK(re2.sub(s2, "Test ${1}") == "Test with group.");

	const String s3 = "Useless substitution";

	RegEx re3("Anything");
	REQUIRE(re3.is_valid());
	CHECK(re3.sub(s3, "Something") == "Useless substitution");

	const String s4 = "acacac";

	RegEx re4("(a)(b){0}(c)");
	REQUIRE(re4.is_valid());
	CHECK(re4.sub(s4, "${1}.${3}.", true) == "a.c.a.c.a.c.");

	const String s5 = "aaaa";

	RegEx re5("a");
	REQUIRE(re5.is_valid());
	CHECK(re5.sub(s5, "b", true, 0, 2) == "bbaa");
	CHECK(re5.sub(s5, "b", true, 1, 3) == "abba");
	CHECK(re5.sub(s5, "b", true, 0, 0) == "aaaa");
	CHECK(re5.sub(s5, "b", true, 1, 1) == "aaaa");
	CHECK(re5.sub(s5, "cc", true, 0, 2) == "ccccaa");
	CHECK(re5.sub(s5, "cc", true, 1, 3) == "acccca");
	CHECK(re5.sub(s5, "", true, 0, 2) == "aa");

	const String s6 = "property get_property set_property";

	RegEx re6("(get_|set_)?property");
	REQUIRE(re6.is_valid());
	CHECK(re6.sub(s6, "$1new_property", true) == "new_property get_new_property set_new_property");
	ERR_PRINT_OFF;
	CHECK(re6.sub(s6, "$5new_property", true) == "new_property new_property new_property");
	ERR_PRINT_ON;
}

TEST_CASE("[RegEx] Substitution with empty input and/or replacement") {
	const String s1 = "";
	const String s2 = "gogogo";

	RegEx re1("");
	REQUIRE(re1.is_valid());
	CHECK(re1.sub(s1, "") == "");
	CHECK(re1.sub(s1, "a") == "a");
	CHECK(re1.sub(s2, "") == "gogogo");

	RegEx re2("go");
	REQUIRE(re2.is_valid());
	CHECK(re2.sub(s2, "") == "gogo");
	CHECK(re2.sub(s2, "", true) == "");
}

TEST_CASE("[RegEx] Uninitialized use") {
	const String s = "Godot";

	RegEx re;
	ERR_PRINT_OFF;
	CHECK(re.search(s).is_null());
	CHECK(re.search_all(s).size() == 0);
	CHECK(re.sub(s, "") == "");
	CHECK(re.get_group_count() == 0);
	CHECK(re.get_names().size() == 0);
	ERR_PRINT_ON
}

TEST_CASE("[RegEx] Empty pattern") {
	RegEx re;
	CHECK(re.compile("") == OK);
	CHECK(re.is_valid());
}

TEST_CASE("[RegEx] Complex Grouping") {
	const String test = "https://docs.godotengine.org/en/latest/contributing/";

	// Ignored protocol in grouping.
	RegEx re("^(?:https?://)([a-zA-Z]{2,4})\\.([a-zA-Z][a-zA-Z0-9_\\-]{2,64})\\.([a-zA-Z]{2,4})");
	REQUIRE(re.is_valid());
	Ref<RegExMatch> expr = re.search(test);

	CHECK(expr->get_group_count() == 3);

	CHECK(expr->get_string(0) == "https://docs.godotengine.org");

	CHECK(expr->get_string(1) == "docs");
	CHECK(expr->get_string(2) == "godotengine");
	CHECK(expr->get_string(3) == "org");
}

TEST_CASE("[RegEx] Number Expression") {
	const String test = "(2.5e-3 + 35 + 46) / 2.8e0 = 28.9294642857";

	// Not an exact regex for number but a good test.
	RegEx re("([+-]?\\d+)(\\.\\d+([eE][+-]?\\d+)?)?");
	REQUIRE(re.is_valid());
	Array number_match = re.search_all(test);

	CHECK(number_match.size() == 5);

	Ref<RegExMatch> number = number_match[0];
	CHECK(number->get_string(0) == "2.5e-3");
	CHECK(number->get_string(1) == "2");
	number = number_match[1];
	CHECK(number->get_string(0) == "35");
	number = number_match[2];
	CHECK(number->get_string(0) == "46");
	number = number_match[3];
	CHECK(number->get_string(0) == "2.8e0");
	number = number_match[4];
	CHECK(number->get_string(0) == "28.9294642857");
	CHECK(number->get_string(1) == "28");
	CHECK(number->get_string(2) == ".9294642857");
}

TEST_CASE("[RegEx] Invalid end position") {
	const String s = "Godot";

	RegEx re("o");
	REQUIRE(re.is_valid());
	Ref<RegExMatch> match = re.search(s, 0, 10);
	CHECK(match->get_string(0) == "o");

	const Array all_results = re.search_all(s, 0, 10);
	CHECK(all_results.size() == 2);
	match = all_results[0];
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == String("o"));
	match = all_results[1];
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == String("o"));

	CHECK(re.sub(s, "", true, 0, 10) == "Gdt");
}

TEST_CASE("[RegEx] Get match string list") {
	const String s = "Godot Engine";

	RegEx re("(Go)(dot)");
	Ref<RegExMatch> match = re.search(s);
	REQUIRE(match.is_valid());
	PackedStringArray result;
	result.append("Godot");
	result.append("Go");
	result.append("dot");
	CHECK(match->get_strings() == result);
}

TEST_CASE("[RegEx] Match start and end positions") {
	const String s = "Whole pattern";

	RegEx re1("pattern");
	REQUIRE(re1.is_valid());
	Ref<RegExMatch> match = re1.search(s);
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 6);
	CHECK(match->get_end(0) == 13);

	RegEx re2("(?<vowel>[aeiou])");
	REQUIRE(re2.is_valid());
	match = re2.search(s);
	REQUIRE(match.is_valid());
	CHECK(match->get_start("vowel") == 2);
	CHECK(match->get_end("vowel") == 3);
}

TEST_CASE("[RegEx] Asterisk search all") {
	const String s = "Godot Engine";

	RegEx re("o*");
	REQUIRE(re.is_valid());
	Ref<RegExMatch> match;
	const Array all_results = re.search_all(s);
	CHECK(all_results.size() == 13);

	match = all_results[0];
	CHECK(match->get_string(0) == "");
	match = all_results[1];
	CHECK(match->get_string(0) == "o");
	match = all_results[2];
	CHECK(match->get_string(0) == "");
	match = all_results[3];
	CHECK(match->get_string(0) == "o");

	for (int i = 4; i < 13; i++) {
		match = all_results[i];
		CHECK(match->get_string(0) == "");
	}
}

TEST_CASE("[RegEx] Simple lookahead") {
	const String s = "Godot Engine";

	RegEx re("o(?=t)");
	REQUIRE(re.is_valid());
	Ref<RegExMatch> match = re.search(s);
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 3);
	CHECK(match->get_end(0) == 4);
}

TEST_CASE("[RegEx] Lookahead groups empty matches") {
	const String s = "12";

	RegEx re("(?=(\\d+))");
	REQUIRE(re.is_valid());
	Ref<RegExMatch> match = re.search(s);
	CHECK(match->get_string(0) == "");
	CHECK(match->get_string(1) == "12");

	const Array all_results = re.search_all(s);
	CHECK(all_results.size() == 2);

	match = all_results[0];
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == String(""));
	CHECK(match->get_string(1) == String("12"));

	match = all_results[1];
	REQUIRE(match.is_valid());
	CHECK(match->get_string(0) == String(""));
	CHECK(match->get_string(1) == String("2"));
}

TEST_CASE("[RegEx] Simple lookbehind") {
	const String s = "Godot Engine";

	RegEx re("(?<=d)o");
	REQUIRE(re.is_valid());
	Ref<RegExMatch> match = re.search(s);
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 3);
	CHECK(match->get_end(0) == 4);
}

TEST_CASE("[RegEx] Simple lookbehind search all") {
	const String s = "ababbaabab";

	RegEx re("(?<=a)b");
	REQUIRE(re.is_valid());
	const Array all_results = re.search_all(s);
	CHECK(all_results.size() == 4);

	Ref<RegExMatch> match = all_results[0];
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 1);
	CHECK(match->get_end(0) == 2);

	match = all_results[1];
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 3);
	CHECK(match->get_end(0) == 4);

	match = all_results[2];
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 7);
	CHECK(match->get_end(0) == 8);

	match = all_results[3];
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 9);
	CHECK(match->get_end(0) == 10);
}

TEST_CASE("[RegEx] Lookbehind groups empty matches") {
	const String s = "abaaabab";

	RegEx re("(?<=(b))");
	REQUIRE(re.is_valid());
	Ref<RegExMatch> match;

	const Array all_results = re.search_all(s);
	CHECK(all_results.size() == 3);

	match = all_results[0];
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 2);
	CHECK(match->get_end(0) == 2);
	CHECK(match->get_start(1) == 1);
	CHECK(match->get_end(1) == 2);
	CHECK(match->get_string(0) == String(""));
	CHECK(match->get_string(1) == String("b"));

	match = all_results[1];
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 6);
	CHECK(match->get_end(0) == 6);
	CHECK(match->get_start(1) == 5);
	CHECK(match->get_end(1) == 6);
	CHECK(match->get_string(0) == String(""));
	CHECK(match->get_string(1) == String("b"));

	match = all_results[2];
	REQUIRE(match.is_valid());
	CHECK(match->get_start(0) == 8);
	CHECK(match->get_end(0) == 8);
	CHECK(match->get_start(1) == 7);
	CHECK(match->get_end(1) == 8);
	CHECK(match->get_string(0) == String(""));
	CHECK(match->get_string(1) == String("b"));
}

} // namespace TestRegEx
#endif // MODULE_REGEX_ENABLED

TEST_FORCE_LINK(test_modules)

