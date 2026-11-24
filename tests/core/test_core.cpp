
// === Merged from tests/core/config/test_project_settings.cpp ===
/*  test_project_settings.cpp                                             */
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



#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "tests/test_macros.h"

namespace TestProjectSettings {

TEST_CASE("[ProjectSettings] Get existing setting") {
	CHECK(ProjectSettings::has_setting("application/run/main_scene"));

	Variant variant = ProjectSettings::get_setting("application/run/main_scene");
	CHECK_EQ(variant.get_type(), Variant::STRING);

	String name = variant;
	CHECK_EQ(name, String());
}

TEST_CASE("[ProjectSettings] Default value is ignored if setting exists") {
	CHECK(ProjectSettings::has_setting("application/run/main_scene"));

	Variant variant = ProjectSettings::get_setting("application/run/main_scene", "SomeDefaultValue");
	CHECK_EQ(variant.get_type(), Variant::STRING);

	String name = variant;
	CHECK_EQ(name, String());
}

TEST_CASE("[ProjectSettings] Non existing setting is null") {
	CHECK_FALSE(ProjectSettings::has_setting("not_existing_setting"));

	Variant variant = ProjectSettings::get_setting("not_existing_setting");
	CHECK_EQ(variant.get_type(), Variant::NIL);
}

TEST_CASE("[ProjectSettings] Non existing setting should return default value") {
	CHECK_FALSE(ProjectSettings::has_setting("not_existing_setting"));

	Variant variant = ProjectSettings::get_setting("not_existing_setting");
	CHECK_EQ(variant.get_type(), Variant::NIL);

	variant = ProjectSettings::get_setting("not_existing_setting", "my_nice_default_value");
	CHECK_EQ(variant.get_type(), Variant::STRING);

	String name = variant;
	CHECK_EQ(name, "my_nice_default_value");

	CHECK_FALSE(ProjectSettings::has_setting("not_existing_setting"));
}

TEST_CASE("[ProjectSettings] Set value should be returned when retrieved") {
	CHECK_FALSE(ProjectSettings::has_setting("my_custom_setting"));

	Variant variant = ProjectSettings::get_setting("my_custom_setting");
	CHECK_EQ(variant.get_type(), Variant::NIL);

	ProjectSettings::set_setting("my_custom_setting", true);
	CHECK(ProjectSettings::has_setting("my_custom_setting"));

	variant = ProjectSettings::get_setting("my_custom_setting");
	CHECK_EQ(variant.get_type(), Variant::BOOL);

	bool value = variant;
	CHECK_EQ(true, value);

	CHECK(ProjectSettings::has_setting("my_custom_setting"));
}

TEST_CASE("[ProjectSettings] localize_path") {
	String old_resource_path = TestProjectSettingsInternalsAccessor::resource_path();
	TestProjectSettingsInternalsAccessor::resource_path() = DirAccess::create(DirAccess::ACCESS_FILESYSTEM)->get_current_dir();
	String root_path = ProjectSettings::get_resource_path();
#ifdef WINDOWS_ENABLED
	String root_path_win = ProjectSettings::get_resource_path().replace_char('/', '\\');
#endif

	CHECK_EQ(ProjectSettings::localize_path("filename"), "res://filename");
	CHECK_EQ(ProjectSettings::localize_path("path/filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path("path/something/../filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path("path/./filename"), "res://path/filename");
#ifdef WINDOWS_ENABLED
	CHECK_EQ(ProjectSettings::localize_path("path\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path("path\\something\\..\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path("path\\.\\filename"), "res://path/filename");
#endif

	CHECK_EQ(ProjectSettings::localize_path("../filename"), "../filename");
	CHECK_EQ(ProjectSettings::localize_path("../path/filename"), "../path/filename");
	CHECK_EQ(ProjectSettings::localize_path("..\\path\\filename"), "../path/filename");

	CHECK_EQ(ProjectSettings::localize_path("/testroot/filename"), "/testroot/filename");
	CHECK_EQ(ProjectSettings::localize_path("/testroot/path/filename"), "/testroot/path/filename");
	CHECK_EQ(ProjectSettings::localize_path("/testroot/path/something/../filename"), "/testroot/path/filename");
	CHECK_EQ(ProjectSettings::localize_path("/testroot/path/./filename"), "/testroot/path/filename");
#ifdef WINDOWS_ENABLED
	CHECK_EQ(ProjectSettings::localize_path("C:/testroot/filename"), "C:/testroot/filename");
	CHECK_EQ(ProjectSettings::localize_path("C:/testroot/path/filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::localize_path("C:/testroot/path/something/../filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::localize_path("C:/testroot/path/./filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::localize_path("C:\\testroot\\filename"), "C:/testroot/filename");
	CHECK_EQ(ProjectSettings::localize_path("C:\\testroot\\path\\filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::localize_path("C:\\testroot\\path\\something\\..\\filename"), "C:/testroot/path/filename");
	CHECK_EQ(ProjectSettings::localize_path("C:\\testroot\\path\\.\\filename"), "C:/testroot/path/filename");
#endif

	CHECK_EQ(ProjectSettings::localize_path(root_path + "/filename"), "res://filename");
	CHECK_EQ(ProjectSettings::localize_path(root_path + "/path/filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path(root_path + "/path/something/../filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path(root_path + "/path/./filename"), "res://path/filename");
#ifdef WINDOWS_ENABLED
	CHECK_EQ(ProjectSettings::localize_path(root_path_win + "\\filename"), "res://filename");
	CHECK_EQ(ProjectSettings::localize_path(root_path_win + "\\path\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path(root_path_win + "\\path\\something\\..\\filename"), "res://path/filename");
	CHECK_EQ(ProjectSettings::localize_path(root_path_win + "\\path\\.\\filename"), "res://path/filename");
#endif

	TestProjectSettingsInternalsAccessor::resource_path() = old_resource_path;
}

TEST_CASE("[ProjectSettings] get_changed_settings basic functionality") {
	String setting_name = "test_changed_setting";
	ProjectSettings::set_setting(setting_name, "test_value");

	PackedStringArray changes = ProjectSettings::get_changed_settings();
	CHECK(changes.has(setting_name));
}

TEST_CASE("[ProjectSettings] get_changed_settings multiple settings") {
	ProjectSettings::set_setting("test_setting_1", "value1");
	ProjectSettings::set_setting("test_setting_2", "value2");
	ProjectSettings::set_setting("another_group/setting", "value3");

	PackedStringArray changes = ProjectSettings::get_changed_settings();
	CHECK(changes.has("test_setting_1"));
	CHECK(changes.has("test_setting_2"));
	CHECK(changes.has("another_group/setting"));
}

TEST_CASE("[ProjectSettings] check_changed_settings_in_group") {
	ProjectSettings::set_setting("group1/setting1", "value1");
	ProjectSettings::set_setting("group1/setting2", "value2");
	ProjectSettings::set_setting("group2/setting1", "value3");
	ProjectSettings::set_setting("other_setting", "value4");

	CHECK(ProjectSettings::check_changed_settings_in_group("group1/"));
	CHECK(ProjectSettings::check_changed_settings_in_group("group2/"));
	CHECK_FALSE(ProjectSettings::check_changed_settings_in_group("nonexistent/"));

	CHECK(ProjectSettings::check_changed_settings_in_group("group1"));
	CHECK(ProjectSettings::check_changed_settings_in_group("other_setting"));
}

TEST_CASE("[ProjectSettings] No tracking when setting same value") {
	String setting_name = "same_value_test";
	String test_value = "same_value";

	ProjectSettings::set_setting(setting_name, test_value);
	int count_before = ProjectSettings::get_changed_settings().size();

	// Setting the same value should not be tracked due to early return.
	ProjectSettings::set_setting(setting_name, test_value);
	int count_after = ProjectSettings::get_changed_settings().size();

	CHECK_EQ(count_before, count_after);
}

} // namespace TestProjectSettings

// === Merged from tests/core/crypto/test_aes_context.cpp ===
/*  test_aes_context.cpp                                                  */
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



#include "core/io/net_socket.h"

namespace TestAes_context {

bool equals(const PackedByteArray &p_a, const PackedByteArray &p_b) {
	if (p_a.size() != p_b.size()) {
		return false;
	}
	if (p_a.is_empty()) {
		return true;
	}
	return memcmp(p_a.ptr(), p_b.ptr(), p_a.size()) == 0;
}

Vector<uint8_t> to_vec(String s) {
	return s.to_utf8_buffer();
}

TEST_CASE("[AESContext] ECB start") {
	AESContext::Mode modes[2] = { AESContext::MODE_ECB_ENCRYPT, AESContext::MODE_ECB_DECRYPT };
	Ref<AESContext> ctx;
	ctx.instantiate();
	for (int i = 0; i < 2; i++) {
		AESContext::Mode mode = modes[i];
		// ECB has no IV
		PackedByteArray iv;
		// Key must be either 16 or 32 bytes.
		Vector<uint8_t> key;

		// Valid key (16)
		key.resize(16);
		Error err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);
		ctx->finish();

		// Valid key (32)
		key.resize(32);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);

		ERR_PRINT_OFF;

		// Already in use
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_ALREADY_IN_USE);
		ctx->finish();

		// Should ignore IV
		iv.resize(1);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);
		ctx->finish();
		iv.resize(16);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);
		ctx->finish();
		iv.clear();

		// Invalid keys
		key.resize(0);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		key.resize(15);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		key.resize(17);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		key.resize(33);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);

		ERR_PRINT_ON;

		// Valid again
		key.resize(16);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);
		ctx->finish();
	}
}

TEST_CASE("[AESContext] ECB update") {
	// ECB has no IV
	PackedByteArray iv;
	// Key must be either 16 or 32 bytes.
	Vector<uint8_t> key;
	// Data size must be multiple of 16 bytes, apply padding if needed.
	String data = "My secret text!!";
	Ref<AESContext> ctx;
	ctx.instantiate();
	Vector<uint8_t> enc;
	for (int i = 0; i < 2; i++) {
		// Test with 16 and 32 bytes key
		key.resize(i ? 32 : 16);
		Error err = ctx->start(AESContext::MODE_ECB_ENCRYPT, key, iv);
		CHECK_EQ(err, OK);

		enc = ctx->update(to_vec(data));
		CHECK_EQ(enc.size(), to_vec(data).size());

		enc = ctx->update(to_vec(data + data));
		CHECK_EQ(enc.size(), to_vec(data + data).size());

		enc = ctx->update(to_vec(data + data + data));
		CHECK_EQ(enc.size(), to_vec(data + data + data).size());

		// Update size must be a multiple of 16
		ERR_PRINT_OFF;
		enc = ctx->update(to_vec(data + "1"));
		CHECK_EQ(enc.size(), 0);
		enc = ctx->update(to_vec(data.substr(1)));
		CHECK_EQ(enc.size(), 0);
		enc = ctx->update(to_vec(data + data + "1"));
		CHECK_EQ(enc.size(), 0);
		ERR_PRINT_ON;
		ctx->finish();
	}

	// The context must be started
	ERR_PRINT_OFF;
	enc = ctx->update(to_vec(data));
	CHECK_EQ(enc.size(), 0);
	ERR_PRINT_ON;
}

TEST_CASE("[AESContext] ECB encrypt/decrypt") {
	// ECB has no IV
	Vector<uint8_t> iv;
	// Key must be either 16 or 32 bytes.
	Vector<uint8_t> key;
	// Data size must be multiple of 16 bytes, apply padding if needed.
	Vector<uint8_t> data;
	data.resize(64);
	uint8_t *d = data.ptrw();
	for (int i = 0; i < 64; i++) {
		d[i] = i;
	}
	Ref<AESContext> ctx;
	ctx.instantiate();
	for (int i = 0; i < 2; i++) {
		key.resize(i ? 32 : 16);
		// Encrypt
		Error err = ctx->start(AESContext::MODE_ECB_ENCRYPT, key, iv);
		CHECK_EQ(err, OK);
		Vector<uint8_t> enc = ctx->update(data.slice(0, 16));
		CHECK_EQ(enc.size(), 16);
		enc.append_array(ctx->update(data.slice(16)));
		CHECK(!equals(data, enc));
		ctx->finish();
		// Decrypt
		err = ctx->start(AESContext::MODE_ECB_DECRYPT, key, iv);
		CHECK_EQ(err, OK);
		Vector<uint8_t> dec = ctx->update(enc.slice(0, 16));
		CHECK_EQ(dec.size(), 16);
		dec.append_array(ctx->update(enc.slice(16)));
		CHECK(equals(dec, data));
		ctx->finish();
	}

	// Test known values
	String wk_key = "My secret key!!!";
	String wk_data = "My secret text!!My secret text!!";
	Vector<uint8_t> wk_enc = { 103, 81, 126, 234, 18, 0, 206, 76, 28, 20, 158, 138, 208, 20, 117, 53, 103, 81, 126, 234, 18, 0, 206, 76, 28, 20, 158, 138, 208, 20, 117, 53 };
	Error err = ctx->start(AESContext::MODE_ECB_ENCRYPT, to_vec(wk_key));
	Vector<uint8_t> enc = ctx->update(to_vec(wk_data));
	CHECK_EQ(err, OK);
	CHECK(equals(wk_enc, enc));
	ctx->finish();

	wk_key = "My secret key!!!My secret key!!!";
	wk_enc = { 154, 26, 109, 2, 146, 25, 9, 217, 179, 234, 4, 129, 2, 148, 181, 80, 154, 26, 109, 2, 146, 25, 9, 217, 179, 234, 4, 129, 2, 148, 181, 80 };
	err = ctx->start(AESContext::MODE_ECB_ENCRYPT, to_vec(wk_key));
	enc = ctx->update(to_vec(wk_data));
	CHECK_EQ(err, OK);
	CHECK(equals(wk_enc, enc));
	ctx->finish();
}

TEST_CASE("[AESContext] CBC start") {
	AESContext::Mode modes[2] = { AESContext::MODE_CBC_ENCRYPT, AESContext::MODE_CBC_DECRYPT };
	Ref<AESContext> ctx;
	ctx.instantiate();
	for (int i = 0; i < 2; i++) {
		AESContext::Mode mode = modes[i];
		// CBC requires 16 bytes of IV
		PackedByteArray iv;
		iv.resize(16);
		// Key must be either 16 or 32 bytes.
		Vector<uint8_t> key;

		// Valid key (16)
		key.resize(16);
		Error err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);
		ctx->finish();

		// Valid key (32)
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);

		ERR_PRINT_OFF;

		// Already in use
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_ALREADY_IN_USE);
		ctx->finish();

		// Should require exactly 16 bytes of IV
		iv.resize(0);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		iv.resize(15);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		iv.resize(17);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		iv.resize(16);

		// Invalid keys
		key.resize(0);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		key.resize(15);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		key.resize(17);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);
		key.resize(33);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, ERR_INVALID_PARAMETER);

		ERR_PRINT_ON;

		// Valid again
		key.resize(16);
		err = ctx->start(mode, key, iv);
		CHECK_EQ(err, OK);
		ctx->finish();
	}
}

TEST_CASE("[AESContext] CBC encrypt/decrypt") {
	// IV must be exactly 16 bytes.
	Vector<uint8_t> iv;
	iv.resize(16);
	// Key must be either 16 or 32 bytes.
	Vector<uint8_t> key;
	// Data size must be multiple of 16 bytes, apply padding if needed.
	Vector<uint8_t> data;
	data.resize(64);
	uint8_t *d = data.ptrw();
	for (int i = 0; i < 64; i++) {
		d[i] = i;
	}
	Ref<AESContext> ctx;
	ctx.instantiate();
	for (int i = 0; i < 2; i++) {
		key.resize(i ? 32 : 16);
		// Encrypt
		Error err = ctx->start(AESContext::MODE_CBC_ENCRYPT, key, iv);
		CHECK_EQ(err, OK);
		Vector<uint8_t> enc = ctx->update(data.slice(0, 16));
		CHECK_EQ(enc.size(), 16);
		enc.append_array(ctx->update(data.slice(16)));
		CHECK(!equals(data, enc));
		ctx->finish();
		// Decrypt
		err = ctx->start(AESContext::MODE_CBC_DECRYPT, key, iv);
		CHECK_EQ(err, OK);
		Vector<uint8_t> dec = ctx->update(enc.slice(0, 16));
		CHECK_EQ(dec.size(), 16);
		dec.append_array(ctx->update(enc.slice(16)));
		CHECK(equals(dec, data));
		ctx->finish();
	}

	// Test known values
	String wk_key = "My secret key!!!";
	String wk_data = "My secret text!!My secret text!!";
	String wk_iv = "My secret iv!!!!";
	Vector<uint8_t> wk_enc = { 5, 69, 219, 146, 167, 176, 184, 99, 29, 246, 79, 191, 234, 6, 46, 39, 157, 47, 126, 102, 152, 34, 50, 4, 137, 167, 31, 110, 63, 141, 84, 145 };
	Error err = ctx->start(AESContext::MODE_CBC_ENCRYPT, to_vec(wk_key), to_vec(wk_iv));
	Vector<uint8_t> enc = ctx->update(to_vec(wk_data));
	CHECK_EQ(err, OK);
	CHECK(equals(wk_enc, enc));
	ctx->finish();

	wk_key = "My secret key!!!My secret key!!!";
	wk_enc = { 166, 176, 138, 196, 78, 18, 148, 120, 139, 21, 172, 33, 92, 181, 5, 239, 175, 111, 229, 214, 22, 186, 129, 95, 223, 46, 192, 103, 141, 105, 183, 91 };
	err = ctx->start(AESContext::MODE_CBC_ENCRYPT, to_vec(wk_key), to_vec(wk_iv));
	enc = ctx->update(to_vec(wk_data));
	CHECK_EQ(err, OK);
	CHECK(equals(wk_enc, enc));
	ctx->finish();
}

} // namespace TestAes_context

// === Merged from tests/core/crypto/test_crypto.cpp ===
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



#include "core/io/net_socket.h"

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

// === Merged from tests/core/crypto/test_hashing_context.cpp ===
/*  test_hashing_context.cpp                                              */
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



#include "core/io/net_socket.h"

namespace TestHashingContext {

TEST_CASE("[HashingContext] Default - MD5/SHA1/SHA256") {
	HashingContext ctx;

	static const uint8_t md5_expected[] = {
		0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e
	};
	static const uint8_t sha1_expected[] = {
		0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90,
		0xaf, 0xd8, 0x07, 0x09
	};
	static const uint8_t sha256_expected[] = {
		0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
		0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
	};

	CHECK(ctx.start(HashingContext::HASH_MD5) == OK);
	PackedByteArray result = ctx.finish();
	REQUIRE(result.size() == 16);
	CHECK(memcmp(result.ptr(), md5_expected, 16) == 0);

	CHECK(ctx.start(HashingContext::HASH_SHA1) == OK);
	result = ctx.finish();
	REQUIRE(result.size() == 20);
	CHECK(memcmp(result.ptr(), sha1_expected, 20) == 0);

	CHECK(ctx.start(HashingContext::HASH_SHA256) == OK);
	result = ctx.finish();
	REQUIRE(result.size() == 32);
	CHECK(memcmp(result.ptr(), sha256_expected, 32) == 0);
}

TEST_CASE("[HashingContext] Multiple updates - MD5/SHA1/SHA256") {
	HashingContext ctx;

	const PackedByteArray s_byte_parts[] = {
		String("x").to_ascii_buffer(),
		String("y").to_ascii_buffer(),
		String("z").to_ascii_buffer()
	};

	static const uint8_t md5_expected[] = {
		0xd1, 0x6f, 0xb3, 0x6f, 0x09, 0x11, 0xf8, 0x78, 0x99, 0x8c, 0x13, 0x61, 0x91, 0xaf, 0x70, 0x5e
	};
	static const uint8_t sha1_expected[] = {
		0x66, 0xb2, 0x74, 0x17, 0xd3, 0x7e, 0x02, 0x4c, 0x46, 0x52, 0x6c, 0x2f, 0x6d, 0x35, 0x8a, 0x75,
		0x4f, 0xc5, 0x52, 0xf3
	};
	static const uint8_t sha256_expected[] = {
		0x36, 0x08, 0xbc, 0xa1, 0xe4, 0x4e, 0xa6, 0xc4, 0xd2, 0x68, 0xeb, 0x6d, 0xb0, 0x22, 0x60, 0x26,
		0x98, 0x92, 0xc0, 0xb4, 0x2b, 0x86, 0xbb, 0xf1, 0xe7, 0x7a, 0x6f, 0xa1, 0x6c, 0x3c, 0x92, 0x82
	};

	CHECK(ctx.start(HashingContext::HASH_MD5) == OK);
	CHECK(ctx.update(s_byte_parts[0]) == OK);
	CHECK(ctx.update(s_byte_parts[1]) == OK);
	CHECK(ctx.update(s_byte_parts[2]) == OK);
	PackedByteArray result = ctx.finish();
	REQUIRE(result.size() == 16);
	CHECK(memcmp(result.ptr(), md5_expected, 16) == 0);

	CHECK(ctx.start(HashingContext::HASH_SHA1) == OK);
	CHECK(ctx.update(s_byte_parts[0]) == OK);
	CHECK(ctx.update(s_byte_parts[1]) == OK);
	CHECK(ctx.update(s_byte_parts[2]) == OK);
	result = ctx.finish();
	REQUIRE(result.size() == 20);
	CHECK(memcmp(result.ptr(), sha1_expected, 20) == 0);

	CHECK(ctx.start(HashingContext::HASH_SHA256) == OK);
	CHECK(ctx.update(s_byte_parts[0]) == OK);
	CHECK(ctx.update(s_byte_parts[1]) == OK);
	CHECK(ctx.update(s_byte_parts[2]) == OK);
	result = ctx.finish();
	REQUIRE(result.size() == 32);
	CHECK(memcmp(result.ptr(), sha256_expected, 32) == 0);
}

TEST_CASE("[HashingContext] Invalid use of start") {
	HashingContext ctx;

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			ctx.start(static_cast<HashingContext::HashType>(-1)) == ERR_UNAVAILABLE,
			"Using invalid hash types should fail.");
	ERR_PRINT_ON;

	REQUIRE(ctx.start(HashingContext::HASH_MD5) == OK);

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			ctx.start(HashingContext::HASH_MD5) == ERR_ALREADY_IN_USE,
			"Calling 'start' twice before 'finish' should fail.");
	ERR_PRINT_ON;
}

TEST_CASE("[HashingContext] Invalid use of update") {
	HashingContext ctx;

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			ctx.update(PackedByteArray()) == ERR_UNCONFIGURED,
			"Calling 'update' before 'start' should fail.");
	ERR_PRINT_ON;

	REQUIRE(ctx.start(HashingContext::HASH_MD5) == OK);

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			ctx.update(PackedByteArray()) == FAILED,
			"Calling 'update' with an empty byte array should fail.");
	ERR_PRINT_ON;
}

TEST_CASE("[HashingContext] Invalid use of finish") {
	HashingContext ctx;

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			ctx.finish() == PackedByteArray(),
			"Calling 'finish' before 'start' should return an empty byte array.");
	ERR_PRINT_ON;
}

} // namespace TestHashingContext

// === Merged from tests/core/input/test_input_event.cpp ===
/*  test_input_event.cpp                                                  */
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



#include "core/input/input_event.h"
#include "core/os/os.h"
#include "core/types.h"
#include "tests/test_macros.h"

namespace TestInputEvent {

TEST_CASE("[InputEvent] Signal is emitted when device is changed") {
	Ref<InputEventKey> input_event;
	input_event.instantiate();

	SIGNAL_WATCH(*input_event, CoreString(changed));

	Array empty_args = { {} };
	input_event->set_device(1);

	SIGNAL_CHECK("changed", empty_args);
	CHECK(input_event->get_device() == 1);

	SIGNAL_UNWATCH(*input_event, CoreString(changed));
}

TEST_CASE("[InputEvent] Test accumulate") {
	Ref<InputEventMouseMotion> iemm1, iemm2;
	Ref<InputEventKey> iek;

	iemm1.instantiate(), iemm2.instantiate();
	iek.instantiate();

	iemm1->set_button_mask(MouseButtonMask::LEFT);

	CHECK_FALSE(iemm1->accumulate(iemm2));

	iemm2->set_button_mask(MouseButtonMask::LEFT);

	CHECK(iemm1->accumulate(iemm2));

	CHECK_FALSE(iemm1->accumulate(iek));
	CHECK_FALSE(iemm2->accumulate(iek));
}

TEST_CASE("[InputEvent][SceneTree] Test methods that interact with the InputMap") {
	const String mock_action = "mock_action";
	Ref<InputEventJoypadMotion> iejm;
	iejm.instantiate();

	InputMap::initialize();
	InputMap::add_action(mock_action, 0.5);
	InputMap::action_add_event(mock_action, iejm);

	CHECK(iejm->is_action_type());
	CHECK(iejm->is_action(mock_action));

	CHECK(iejm->is_action_released(mock_action));
	CHECK(Math::is_equal_approx(iejm->get_action_strength(mock_action), 0.0f));

	iejm->set_axis_value(0.8f);
	// Since deadzone is 0.5, action_strength grows linearly from 0.5 to 1.0.
	CHECK(Math::is_equal_approx(iejm->get_action_strength(mock_action), 0.6f));
	CHECK(Math::is_equal_approx(iejm->get_action_raw_strength(mock_action), 0.8f));
	CHECK(iejm->is_action_pressed(mock_action));

	InputMap::erase_action(mock_action);
}

TEST_CASE("[InputEvent] Test xformed_by") {
	Ref<InputEventMouseMotion> iemm1;
	iemm1.instantiate();

	iemm1->set_position(Vector2(0.0f, 0.0f));
	Transform2D transform;
	transform = transform.translated(Vector2(2.0f, 3.0f));

	Ref<InputEventMouseMotion> iemm2 = iemm1->xformed_by(transform);

	CHECK(iemm2->get_position().is_equal_approx(Vector2(2.0f, 3.0f)));
}

} // namespace TestInputEvent

// === Merged from tests/core/input/test_input_event_key.cpp ===
/*  test_input_event_key.cpp                                              */
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



#include "core/input/input_event.h"

namespace TestInputEventKey {

TEST_CASE("[InputEventKey] Key correctly registers being pressed") {
	InputEventKey key;
	key.set_pressed(true);
	CHECK(key.is_pressed() == true);

	key.set_pressed(false);
	CHECK(key.is_pressed() == false);
}

TEST_CASE("[InputEventKey] Key correctly stores and retrieves keycode") {
	InputEventKey key;

	key.set_keycode(Key::ENTER);
	CHECK(key.get_keycode() == Key::ENTER);
	CHECK(key.get_keycode() != Key::PAUSE);

	key.set_physical_keycode(Key::BACKSPACE);
	CHECK(key.get_physical_keycode() == Key::BACKSPACE);
	CHECK(key.get_physical_keycode() != Key::PAUSE);
}

TEST_CASE("[InputEventKey] Key correctly stores and retrieves keycode with modifiers") {
	InputEventKey key;

	key.set_keycode(Key::ENTER);
	key.set_ctrl_pressed(true);

	CHECK(key.get_keycode_with_modifiers() == (Key::ENTER | KeyModifierMask::CTRL));
	CHECK(key.get_keycode_with_modifiers() != (Key::ENTER | KeyModifierMask::SHIFT));
	CHECK(key.get_keycode_with_modifiers() != Key::ENTER);

	key.set_physical_keycode(Key::SPACE);
	key.set_ctrl_pressed(true);

	CHECK(key.get_physical_keycode_with_modifiers() == (Key::SPACE | KeyModifierMask::CTRL));
	CHECK(key.get_physical_keycode_with_modifiers() != (Key::SPACE | KeyModifierMask::SHIFT));
	CHECK(key.get_physical_keycode_with_modifiers() != Key::SPACE);
}

TEST_CASE("[InputEventKey] Key correctly stores and retrieves unicode") {
	InputEventKey key;

	key.set_unicode('x');
	CHECK(key.get_unicode() == 'x');
	CHECK(key.get_unicode() != 'y');
}

TEST_CASE("[InputEventKey] Key correctly stores and retrieves location") {
	InputEventKey key;

	CHECK(key.get_location() == KeyLocation::UNSPECIFIED);

	key.set_location(KeyLocation::LEFT);
	CHECK(key.get_location() == KeyLocation::LEFT);
	CHECK(key.get_location() != KeyLocation::RIGHT);
}

TEST_CASE("[InputEventKey] Key correctly stores and checks echo") {
	InputEventKey key;

	key.set_echo(true);
	CHECK(key.is_echo() == true);

	key.set_echo(false);
	CHECK(key.is_echo() == false);
}

TEST_CASE("[InputEventKey] Key correctly converts itself to text") {
	InputEventKey none_key;

	// These next three tests test the functionality of getting a key that is set to None
	// as text. These cases are a bit weird, since None has no textual representation
	// (find_keycode_name(Key::NONE) results in a nullptr). Thus, these tests look weird
	// with only "- Physical" or a lonely modifier with "- Physical" but (as far as I
	// understand the code, that is intended behavior.

	// Key is None without a physical key.
	none_key.set_keycode(Key::NONE);
	CHECK(none_key.as_text() == "(unset)");

	// Key is none and has modifiers.
	none_key.set_ctrl_pressed(true);
	CHECK(none_key.as_text() == "Ctrl+(unset)");

	// Key is None WITH a physical key AND modifiers.
	none_key.set_physical_keycode(Key::ENTER);
	CHECK(none_key.as_text() == "Ctrl+Enter");

	none_key.set_physical_keycode(Key::W);
	CHECK(none_key.as_text() == "Ctrl+W - Physical");

	// Key is None WITH a physical key AND multiple modifiers, checks for correct ordering.
	none_key.set_physical_keycode(Key::ENTER);
	none_key.set_alt_pressed(true);
	none_key.set_shift_pressed(true);
#ifdef MACOS_ENABLED
	CHECK(none_key.as_text() != "Ctrl+Shift+Option+Enter");
	CHECK(none_key.as_text() == "Ctrl+Option+Shift+Enter");
#else
	CHECK(none_key.as_text() != "Ctrl+Shift+Alt+Enter");
	CHECK(none_key.as_text() == "Ctrl+Alt+Shift+Enter");
#endif
	none_key.set_physical_keycode(Key::W);
#ifdef MACOS_ENABLED
	CHECK(none_key.as_text() != "Ctrl+Shift+Option+W - Physical");
	CHECK(none_key.as_text() == "Ctrl+Option+Shift+W - Physical");
#else
	CHECK(none_key.as_text() != "Ctrl+Shift+Alt+W - Physical");
	CHECK(none_key.as_text() == "Ctrl+Alt+Shift+W - Physical");
#endif

	InputEventKey none_key2;

	// Key is None without modifiers with a physical key.
	none_key2.set_keycode(Key::NONE);
	none_key2.set_physical_keycode(Key::W);

	CHECK(none_key2.as_text() == "W - Physical");

	none_key2.set_keycode(Key::NONE);
	none_key2.set_physical_keycode(Key::ENTER);

	CHECK(none_key2.as_text() == "Enter");

	InputEventKey key;

	// Key has keycode.
	key.set_keycode(Key::SPACE);
	CHECK(key.as_text() != "");
	CHECK(key.as_text() == "Space");

	// Key has keycode and modifiers.
	key.set_ctrl_pressed(true);
	CHECK(key.as_text() != "Space");
	CHECK(key.as_text() == "Ctrl+Space");

	// Key has keycode and multiple modifiers, checks for correct ordering.
	key.set_alt_pressed(true);
	key.set_shift_pressed(true);
#ifdef MACOS_ENABLED
	CHECK(key.as_text() != "Ctrl+Shift+Option+Space");
	CHECK(key.as_text() == "Ctrl+Option+Shift+Space");
#else
	CHECK(key.as_text() != "Ctrl+Shift+Alt+Space");
	CHECK(key.as_text() == "Ctrl+Alt+Shift+Space");
#endif

	// Since the keycode is set to Key::NONE upon initialization of the
	// InputEventKey and you can only update it with another Key, the keycode
	// cannot be empty, so the kc.is_empty() case cannot be tested.
}

TEST_CASE("[InputEventKey] Key correctly converts its state to a string representation") {
	InputEventKey none_key;

	CHECK(none_key.to_string() == "InputEventKey: keycode=(unset), mods=none, physical=false, location=unspecified, pressed=false, echo=false");
	// Set physical key to Escape.
	none_key.set_physical_keycode(Key::ESCAPE);
	CHECK(none_key.to_string() == "InputEventKey: keycode=4194305 (Escape), mods=none, physical=true, location=unspecified, pressed=false, echo=false");

	InputEventKey key;

	// Set physical to None, set keycode to Space.
	key.set_keycode(Key::SPACE);
	CHECK(key.to_string() == "InputEventKey: keycode=32 (Space), mods=none, physical=false, location=unspecified, pressed=false, echo=false");

	// Set location
	key.set_location(KeyLocation::RIGHT);
	CHECK(key.to_string() == "InputEventKey: keycode=32 (Space), mods=none, physical=false, location=right, pressed=false, echo=false");

	// Set pressed to true.
	key.set_pressed(true);
	CHECK(key.to_string() == "InputEventKey: keycode=32 (Space), mods=none, physical=false, location=right, pressed=true, echo=false");

	// set echo to true.
	key.set_echo(true);
	CHECK(key.to_string() == "InputEventKey: keycode=32 (Space), mods=none, physical=false, location=right, pressed=true, echo=true");

	// Press Ctrl and Alt.
	key.set_ctrl_pressed(true);
	key.set_alt_pressed(true);
#ifdef MACOS_ENABLED
	CHECK(key.to_string() == "InputEventKey: keycode=32 (Space), mods=Ctrl+Option, physical=false, location=right, pressed=true, echo=true");
#else
	CHECK(key.to_string() == "InputEventKey: keycode=32 (Space), mods=Ctrl+Alt, physical=false, location=right, pressed=true, echo=true");
#endif
}

TEST_CASE("[InputEventKey] Key is correctly converted to reference") {
	InputEventKey base_key;
	Ref<InputEventKey> key_ref = base_key.create_reference(Key::ENTER);

	CHECK(key_ref->get_keycode() == Key::ENTER);
}

TEST_CASE("[InputEventKey] Keys are correctly matched based on action") {
	bool pressed = false;
	float strength, raw_strength = 0.0;

	InputEventKey key;

	// Nullptr.
	CHECK_MESSAGE(key.action_match(nullptr, false, 0.0f, &pressed, &strength, &raw_strength) == false, "nullptr as key reference should result in false");

	// Match on keycode.
	key.set_keycode(Key::SPACE);
	Ref<InputEventKey> match = key.create_reference(Key::SPACE);
	Ref<InputEventKey> no_match = key.create_reference(Key::ENTER);

	CHECK(key.action_match(match, false, 0.0f, &pressed, &strength, &raw_strength) == true);
	CHECK(key.action_match(no_match, false, 0.0f, &pressed, &strength, &raw_strength) == false);

	// Check that values are correctly transferred to the pointers.
	CHECK(pressed == false);
	CHECK(strength < 0.5);
	CHECK(raw_strength < 0.5);

	match->set_pressed(true);
	key.action_match(match, false, 0.0f, &pressed, &strength, &raw_strength);

	CHECK(pressed == true);
	CHECK(strength > 0.5);
	CHECK(raw_strength > 0.5);

	// Tests when keycode is None: Then you rely on physical keycode.
	InputEventKey none_key;
	none_key.set_physical_keycode(Key::SPACE);

	Ref<InputEventKey> match_none = none_key.create_reference(Key::NONE);
	match_none->set_physical_keycode(Key::SPACE);

	Ref<InputEventKey> no_match_none = none_key.create_reference(Key::NONE);
	no_match_none->set_physical_keycode(Key::ENTER);

	CHECK(none_key.action_match(match_none, false, 0.0f, &pressed, &strength, &raw_strength) == true);
	CHECK(none_key.action_match(no_match_none, false, 0.0f, &pressed, &strength, &raw_strength) == false);

	// Test exact match.
	InputEventKey key2, ref_key;
	key2.set_keycode(Key::SPACE);

	Ref<InputEventKey> match2 = ref_key.create_reference(Key::SPACE);

	// Now both press Ctrl and Shift.
	key2.set_ctrl_pressed(true);
	key2.set_shift_pressed(true);

	match2->set_ctrl_pressed(true);
	match2->set_shift_pressed(true);

	// Now they should match.
	bool exact_match = true;
	CHECK(key2.action_match(match2, exact_match, 0.0f, &pressed, &strength, &raw_strength) == true);

	// Modify matching key such that it does no longer match in terms of modifiers: Shift
	// is no longer pressed.
	match2->set_shift_pressed(false);
	CHECK(match2->is_shift_pressed() == false);
	CHECK(key2.action_match(match2, exact_match, 0.0f, &pressed, &strength, &raw_strength) == false);
}

TEST_CASE("[IsMatch] Keys are correctly matched") {
	// Key with NONE as keycode.
	InputEventKey key;
	key.set_keycode(Key::NONE);
	key.set_physical_keycode(Key::SPACE);

	// Nullptr.
	CHECK(key.is_match(nullptr, false) == false);

	Ref<InputEventKey> none_ref = key.create_reference(Key::NONE);

	none_ref->set_physical_keycode(Key::SPACE);
	CHECK(key.is_match(none_ref, false) == true);

	none_ref->set_physical_keycode(Key::ENTER);
	CHECK(key.is_match(none_ref, false) == false);

	none_ref->set_physical_keycode(Key::SPACE);

	key.set_ctrl_pressed(true);
	none_ref->set_ctrl_pressed(false);
	CHECK(key.is_match(none_ref, true) == false);

	none_ref->set_ctrl_pressed(true);
	CHECK(key.is_match(none_ref, true) == true);

	// Ref with actual keycode.
	InputEventKey key2;
	key2.set_keycode(Key::SPACE);

	Ref<InputEventKey> match = key2.create_reference(Key::SPACE);
	Ref<InputEventKey> no_match = key2.create_reference(Key::ENTER);

	CHECK(key2.is_match(match, false) == true);
	CHECK(key2.is_match(no_match, false) == false);

	// Now the keycode is the same, but the modifiers differ.
	no_match->set_keycode(Key::SPACE);

	key2.set_ctrl_pressed(true);
	match->set_ctrl_pressed(true);
	no_match->set_shift_pressed(true);

	CHECK(key2.is_match(match, true) == true);
	CHECK(key2.is_match(no_match, true) == false);

	// Physical key with location.
	InputEventKey key3;
	key3.set_keycode(Key::NONE);
	key3.set_physical_keycode(Key::SHIFT);

	Ref<InputEventKey> loc_ref = key.create_reference(Key::NONE);

	loc_ref->set_keycode(Key::SHIFT);
	loc_ref->set_physical_keycode(Key::SHIFT);

	CHECK(key3.is_match(loc_ref, false) == true);
	key3.set_location(KeyLocation::UNSPECIFIED);
	CHECK(key3.is_match(loc_ref, false) == true);

	loc_ref->set_location(KeyLocation::LEFT);
	CHECK(key3.is_match(loc_ref, false) == true);

	key3.set_location(KeyLocation::LEFT);
	CHECK(key3.is_match(loc_ref, false) == true);

	key3.set_location(KeyLocation::RIGHT);
	CHECK(key3.is_match(loc_ref, false) == false);

	// Keycode key with location.
	key3.set_physical_keycode(Key::NONE);
	key3.set_keycode(Key::SHIFT);
	CHECK(key3.is_match(loc_ref, false) == true);
}

} // namespace TestInputEventKey

// === Merged from tests/core/input/test_input_event_mouse.cpp ===
/*  test_input_event_mouse.cpp                                            */
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



#include "core/input/input_event.h"

namespace TestInputEventMouse {

TEST_CASE("[InputEventMouse] Mouse button mask is set correctly") {
	InputEventMouse mousekey;

	mousekey.set_button_mask(MouseButtonMask::LEFT);
	CHECK(mousekey.get_button_mask().has_flag(MouseButtonMask::LEFT));

	mousekey.set_button_mask(MouseButtonMask::MB_XBUTTON1);
	CHECK(mousekey.get_button_mask().has_flag(MouseButtonMask::MB_XBUTTON1));

	mousekey.set_button_mask(MouseButtonMask::MB_XBUTTON2);
	CHECK(mousekey.get_button_mask().has_flag(MouseButtonMask::MB_XBUTTON2));

	mousekey.set_button_mask(MouseButtonMask::MIDDLE);
	CHECK(mousekey.get_button_mask().has_flag(MouseButtonMask::MIDDLE));

	mousekey.set_button_mask(MouseButtonMask::RIGHT);
	CHECK(mousekey.get_button_mask().has_flag(MouseButtonMask::RIGHT));
}

TEST_CASE("[InputEventMouse] Setting the mouse position works correctly") {
	InputEventMouse mousekey;

	mousekey.set_position(Vector2{ 10, 10 });
	CHECK(mousekey.get_position() == Vector2{ 10, 10 });

	mousekey.set_position(Vector2{ -1, -1 });
	CHECK(mousekey.get_position() == Vector2{ -1, -1 });
}

TEST_CASE("[InputEventMouse] Setting the global mouse position works correctly") {
	InputEventMouse mousekey;

	mousekey.set_global_position(Vector2{ 10, 10 });
	CHECK(mousekey.get_global_position() == Vector2{ 10, 10 });
	CHECK(mousekey.get_global_position() != Vector2{ 1, 1 });

	mousekey.set_global_position(Vector2{ -1, -1 });
	CHECK(mousekey.get_global_position() == Vector2{ -1, -1 });
	CHECK(mousekey.get_global_position() != Vector2{ 1, 1 });
}

} // namespace TestInputEventMouse

// === Merged from tests/core/input/test_shortcut.cpp ===
/*  test_shortcut.cpp                                                     */
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



#include "core/input/input_event.h"

namespace TestShortcut {

TEST_CASE("[Shortcut] Empty shortcut should have no valid events and text equal to None") {
	Shortcut s;

	CHECK(s.get_as_text() == "None");
	CHECK(s.has_valid_event() == false);
}

TEST_CASE("[Shortcut] Setting and getting an event should result in the same event as the input") {
	Ref<InputEventKey> k1;
	Ref<InputEventKey> k2;
	k1.instantiate();
	k2.instantiate();

	k1->set_keycode(Key::ENTER);
	k2->set_keycode(Key::BACKSPACE);

	// Cast to InputEvent so the internal code recognizes the objects.
	Ref<InputEvent> e1 = k1;
	Ref<InputEvent> e2 = k2;
	Array input_array = { e1, e2 };

	Shortcut s;
	s.set_events(input_array);

	// Get result, read it out, check whether it equals the input.
	Array result_array = s.get_events();
	Ref<InputEventKey> result1 = result_array.front();
	Ref<InputEventKey> result2 = result_array.back();

	CHECK(result1->get_keycode() == k1->get_keycode());
	CHECK(result2->get_keycode() == k2->get_keycode());
}

TEST_CASE("[Shortcut] 'set_events_list' should result in the same events as the input") {
	Ref<InputEventKey> k1;
	Ref<InputEventKey> k2;
	k1.instantiate();
	k2.instantiate();

	k1->set_keycode(Key::ENTER);
	k2->set_keycode(Key::BACKSPACE);

	// Cast to InputEvent so the set_events_list() method recognizes the objects.
	Ref<InputEvent> e1 = k1;
	Ref<InputEvent> e2 = k2;

	List<Ref<InputEvent>> list;
	list.push_back(e1);
	list.push_back(e2);

	Shortcut s;
	s.set_events_list(&list);

	// Get result, read it out, check whether it equals the input.
	Array result_array = s.get_events();
	Ref<InputEventKey> result1 = result_array.front();
	Ref<InputEventKey> result2 = result_array.back();

	CHECK(result1->get_keycode() == k1->get_keycode());
	CHECK(result2->get_keycode() == k2->get_keycode());
}

TEST_CASE("[Shortcut] 'matches_event' should correctly match the same event") {
	Ref<InputEventKey> original; // The one we compare with.
	Ref<InputEventKey> similar_but_not_equal; // Same keycode, different event.
	Ref<InputEventKey> different; // Different event, different keycode.
	Ref<InputEventKey> copy; // Copy of original event.

	original.instantiate();
	similar_but_not_equal.instantiate();
	different.instantiate();
	copy.instantiate();

	original->set_keycode(Key::ENTER);
	similar_but_not_equal->set_keycode(Key::ENTER);
	similar_but_not_equal->set_keycode(Key::ESCAPE);
	copy = original;

	// Only the copy is really the same, so only that one should match.
	// The rest should not match.

	Ref<InputEvent> e_original = original;

	Ref<InputEvent> e_similar_but_not_equal = similar_but_not_equal;
	Ref<InputEvent> e_different = different;
	Ref<InputEvent> e_copy = copy;

	Array a = { e_original };
	Shortcut s;
	s.set_events(a);

	CHECK(s.matches_event(e_similar_but_not_equal) == false);
	CHECK(s.matches_event(e_different) == false);

	CHECK(s.matches_event(e_copy) == true);
}

TEST_CASE("[Shortcut] 'get_as_text' text representation should be correct") {
	Ref<InputEventKey> same;
	// k2 will not go into the shortcut but only be used to compare.
	Ref<InputEventKey> different;

	same.instantiate();
	different.instantiate();

	same->set_keycode(Key::ENTER);
	different->set_keycode(Key::ESCAPE);

	Ref<InputEvent> key_event1 = same;
	Array a = { key_event1 };
	Shortcut s;
	s.set_events(a);

	CHECK(s.get_as_text() == same->as_text());
	CHECK(s.get_as_text() != different->as_text());
}

TEST_CASE("[Shortcut] Event validity should be correctly checked.") {
	Ref<InputEventKey> valid;
	// k2 will not go into the shortcut but only be used to compare.
	Ref<InputEventKey> invalid = nullptr;

	valid.instantiate();
	valid->set_keycode(Key::ENTER);

	Ref<InputEvent> valid_event = valid;
	Ref<InputEvent> invalid_event = invalid;

	Array a = { invalid_event, valid_event };

	Shortcut s;
	s.set_events(a);

	CHECK(s.has_valid_event() == true);

	Array b = { invalid_event };

	Shortcut shortcut_with_invalid_event;
	shortcut_with_invalid_event.set_events(b);

	CHECK(shortcut_with_invalid_event.has_valid_event() == false);
}

TEST_CASE("[Shortcut] Equal arrays should be recognized as such.") {
	Ref<InputEventKey> k1;
	// k2 will not go into the shortcut but only be used to compare.
	Ref<InputEventKey> k2;

	k1.instantiate();
	k2.instantiate();

	k1->set_keycode(Key::ENTER);
	k2->set_keycode(Key::ESCAPE);

	Ref<InputEvent> key_event1 = k1;
	Ref<InputEvent> key_event2 = k2;

	Array same;
	same.append(key_event1);

	Array same_as_same;
	same_as_same.append(key_event1);

	Array different1 = { key_event2 };
	Array different2 = { key_event1, key_event2 };
	Array different3;

	Shortcut s;

	CHECK(s.is_event_array_equal(same, same_as_same) == true);
	CHECK(s.is_event_array_equal(same, different1) == false);
	CHECK(s.is_event_array_equal(same, different2) == false);
	CHECK(s.is_event_array_equal(same, different3) == false);
}

} // namespace TestShortcut

// === Merged from tests/core/object/test_class_db.cpp ===
/*  test_class_db.cpp                                                     */
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



#include "core/os/os.h"
#include "core/config/project_settings.h"

namespace TestClassDB {

struct TypeReference {
	String name;
	bool is_enum = false;
};

struct ConstantData {
	String name;
	int64_t value = 0;
};

struct EnumData {
	String name;
	List<ConstantData> constants;

	_FORCE_INLINE_ bool operator==(const EnumData &p_enum) const {
		return p_enum.name == name;
	}
};

struct PropertyData {
	String name;
	int index = 0;

	String getter;
	String setter;
};

struct ArgumentData {
	TypeReference type;
	String name;
	bool has_defval = false;
	Variant defval;
	int position;
};

struct MethodData {
	String name;
	TypeReference return_type;
	List<ArgumentData> arguments;
	bool is_virtual = false;
	bool is_vararg = false;
};

struct SignalData {
	String name;
	List<ArgumentData> arguments;
};

struct ExposedClass {
	String name;
	String base;

	bool is_singleton = false;
	bool is_instantiable = false;
	bool is_ref_counted = false;

	ClassDB::APIType api_type;

	List<ConstantData> constants;
	List<EnumData> enums;
	List<PropertyData> properties;
	List<MethodData> methods;
	List<SignalData> signals_;

	const PropertyData *find_property_by_name(const String &p_name) const {
		for (const PropertyData &E : properties) {
			if (E.name == p_name) {
				return &E;
			}
		}

		return nullptr;
	}

	const MethodData *find_method_by_name(const String &p_name) const {
		for (const MethodData &E : methods) {
			if (E.name == p_name) {
				return &E;
			}
		}

		return nullptr;
	}
};

struct NamesCache {
	String variant_type = String("Variant");
	String object_class = String("Object");
	String ref_counted_class = String("RefCounted");
	String string_type = String("String");
	String string_name_type = String("String");
	String node_path_type = String("NodePath");
	String bool_type = String("bool");
	String int_type = String("int");
	String float_type = String("float");
	String void_type = String("void");
	String vararg_stub_type = String("@VarArg@");
	String vector2_type = String("Vector2");
	String rect2_type = String("Rect2");
	String vector3_type = String("Vector3");
	String vector4_type = String("Vector4");

	// Object not included as it must be checked for all derived classes
	static constexpr int nullable_types_count = 18;
	String nullable_types[nullable_types_count] = {
		string_type,
		string_name_type,
		node_path_type,

		String(_STR(Array)),
		String(_STR(Dictionary)),
		String(_STR(Callable)),
		String(_STR(Signal)),

		String(_STR(PackedByteArray)),
		String(_STR(PackedInt32Array)),
		String(_STR(PackedInt64rray)),
		String(_STR(PackedFloat32Array)),
		String(_STR(PackedFloat64Array)),
		String(_STR(PackedStringArray)),
		String(_STR(PackedVector2Array)),
		String(_STR(PackedVector3Array)),
		String(_STR(PackedColorArray)),
		String(_STR(PackedVector4Array)),
	};

	bool is_nullable_type(const String &p_type) const {
		for (int i = 0; i < nullable_types_count; i++) {
			if (p_type == nullable_types[i]) {
				return true;
			}
		}

		return false;
	}
};

typedef HashMap<String, ExposedClass> ExposedClasses;

struct Context {
	Vector<String> enum_types;
	Vector<String> builtin_types;
	ExposedClasses exposed_classes;
	List<EnumData> global_enums;
	NamesCache names_cache;

	const ExposedClass *find_exposed_class(const String &p_name) const {
		ExposedClasses::ConstIterator elem = exposed_classes.find(p_name);
		return elem ? &elem->value : nullptr;
	}

	const ExposedClass *find_exposed_class(const TypeReference &p_type_ref) const {
		ExposedClasses::ConstIterator elem = exposed_classes.find(p_type_ref.name);
		return elem ? &elem->value : nullptr;
	}

	bool has_type(const TypeReference &p_type_ref) const {
		if (builtin_types.has(p_type_ref.name)) {
			return true;
		}

		if (p_type_ref.is_enum) {
			if (enum_types.has(p_type_ref.name)) {
				return true;
			}

			// Enum not found. Most likely because none of its constants were bound, so it's empty. That's fine. Use int instead.
			return builtin_types.find(names_cache.int_type);
		}

		return false;
	}
};

bool arg_default_value_is_assignable_to_type(const Context &p_context, const Variant &p_val, const TypeReference &p_arg_type, String *r_err_msg = nullptr) {
	if (p_arg_type.name == p_context.names_cache.variant_type) {
		// Variant can take anything
		return true;
	}

	switch (p_val.get_type()) {
		case Variant::NIL:
			return p_context.find_exposed_class(p_arg_type) ||
					p_context.names_cache.is_nullable_type(p_arg_type.name);
		case Variant::BOOL:
			return p_arg_type.name == p_context.names_cache.bool_type;
		case Variant::INT:
			return p_arg_type.name == p_context.names_cache.int_type ||
					p_arg_type.name == p_context.names_cache.float_type ||
					p_arg_type.is_enum;
		case Variant::FLOAT:
			return p_arg_type.name == p_context.names_cache.float_type;
		case Variant::STRING:
		case Variant::STRING_NAME:
			return p_arg_type.name == p_context.names_cache.string_type ||
					p_arg_type.name == p_context.names_cache.string_name_type ||
					p_arg_type.name == p_context.names_cache.node_path_type;
		case Variant::NODE_PATH:
			return p_arg_type.name == p_context.names_cache.node_path_type;
		case Variant::TRANSFORM3D:
		case Variant::TRANSFORM2D:
		case Variant::BASIS:
		case Variant::QUATERNION:
		case Variant::PLANE:
		case Variant::AABB:
		case Variant::COLOR:
		case Variant::VECTOR2:
		case Variant::RECT2:
		case Variant::VECTOR3:
		case Variant::VECTOR4:
		case Variant::PROJECTION:
		case Variant::RID:
		case Variant::ARRAY:
		case Variant::DICTIONARY:
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY:
		case Variant::PACKED_VECTOR4_ARRAY:
		case Variant::CALLABLE:
		case Variant::SIGNAL:
			return p_arg_type.name == Variant::get_type_name(p_val.get_type());
		case Variant::OBJECT:
			return p_context.find_exposed_class(p_arg_type);
		case Variant::VECTOR2I:
			return p_arg_type.name == p_context.names_cache.vector2_type ||
					p_arg_type.name == Variant::get_type_name(p_val.get_type());
		case Variant::RECT2I:
			return p_arg_type.name == p_context.names_cache.rect2_type ||
					p_arg_type.name == Variant::get_type_name(p_val.get_type());
		case Variant::VECTOR3I:
			return p_arg_type.name == p_context.names_cache.vector3_type ||
					p_arg_type.name == Variant::get_type_name(p_val.get_type());
		case Variant::VECTOR4I:
			return p_arg_type.name == p_context.names_cache.vector4_type ||
					p_arg_type.name == Variant::get_type_name(p_val.get_type());
		case Variant::VARIANT_MAX:
			break;
	}
	if (r_err_msg) {
		*r_err_msg = "Unexpected Variant type: " + itos(p_val.get_type());
	}
	return false;
}

bool arg_default_value_is_valid_data(const Variant &p_val, String *r_err_msg = nullptr) {
	switch (p_val.get_type()) {
		case Variant::RID:
		case Variant::ARRAY:
		case Variant::DICTIONARY:
		case Variant::PACKED_BYTE_ARRAY:
		case Variant::PACKED_INT32_ARRAY:
		case Variant::PACKED_INT64_ARRAY:
		case Variant::PACKED_FLOAT32_ARRAY:
		case Variant::PACKED_FLOAT64_ARRAY:
		case Variant::PACKED_STRING_ARRAY:
		case Variant::PACKED_VECTOR2_ARRAY:
		case Variant::PACKED_VECTOR3_ARRAY:
		case Variant::PACKED_COLOR_ARRAY:
		case Variant::PACKED_VECTOR4_ARRAY:
		case Variant::CALLABLE:
		case Variant::SIGNAL:
		case Variant::OBJECT:
			if (p_val.is_zero()) {
				return true;
			}
			if (r_err_msg) {
				*r_err_msg = "Must be zero.";
			}
			break;
		default:
			return true;
	}

	return false;
}

void validate_property(const Context &p_context, const ExposedClass &p_class, const PropertyData &p_prop) {
	const MethodData *setter = p_class.find_method_by_name(p_prop.setter);

	// Search it in base classes too
	const ExposedClass *top = &p_class;
	while (!setter && top->base != String()) {
		top = p_context.find_exposed_class(top->base);
		TEST_FAIL_COND(!top, "Class not found '", top->base, "'. Inherited by '", top->name, "'.");
		setter = top->find_method_by_name(p_prop.setter);
	}

	const MethodData *getter = p_class.find_method_by_name(p_prop.getter);

	// Search it in base classes too
	top = &p_class;
	while (!getter && top->base != String()) {
		top = p_context.find_exposed_class(top->base);
		TEST_FAIL_COND(!top, "Class not found '", top->base, "'. Inherited by '", top->name, "'.");
		getter = top->find_method_by_name(p_prop.getter);
	}

	TEST_FAIL_COND((!setter && !getter),
			"Couldn't find neither the setter nor the getter for property: '", p_class.name, ".", String(p_prop.name), "'.");

	if (setter) {
		int setter_argc = p_prop.index != -1 ? 2 : 1;
		TEST_FAIL_COND(setter->arguments.size() != setter_argc,
				"Invalid property setter argument count: '", p_class.name, ".", String(p_prop.name), "'.");
	}

	if (getter) {
		int getter_argc = p_prop.index != -1 ? 1 : 0;
		TEST_FAIL_COND(getter->arguments.size() != getter_argc,
				"Invalid property setter argument count: '", p_class.name, ".", String(p_prop.name), "'.");
	}

	if (getter && setter) {
		const ArgumentData &setter_first_arg = setter->arguments.back()->get();
		if (getter->return_type.name != setter_first_arg.type.name) {
			TEST_FAIL(
					"Return type from getter doesn't match first argument of setter, for property: '", p_class.name, ".", String(p_prop.name), "'.");
		}
	}

	const TypeReference &prop_type_ref = getter ? getter->return_type : setter->arguments.back()->get().type;

	const ExposedClass *prop_class = p_context.find_exposed_class(prop_type_ref);
	if (prop_class) {
		TEST_COND(prop_class->is_singleton,
				"Property type is a singleton: '", p_class.name, ".", String(p_prop.name), "'.");

		if (p_class.api_type == ClassDB::API_CORE) {
			TEST_COND(prop_class->api_type == ClassDB::API_EDITOR,
					"Property '", p_class.name, ".", p_prop.name, "' has type '", prop_class->name,
					"' from the editor API. Core API cannot have dependencies on the editor API.");
		}
	} else {
		// Look for types that don't inherit Object
		TEST_FAIL_COND(!p_context.has_type(prop_type_ref),
				"Property type '", prop_type_ref.name, "' not found: '", p_class.name, ".", String(p_prop.name), "'.");
	}

	if (getter) {
		if (p_prop.index != -1) {
			const ArgumentData &idx_arg = getter->arguments.front()->get();
			if (idx_arg.type.name != p_context.names_cache.int_type) {
				// If not an int, it can be an enum
				TEST_COND(!p_context.enum_types.has(idx_arg.type.name),
						"Invalid type '", idx_arg.type.name, "' for index argument of property getter: '", p_class.name, ".", String(p_prop.name), "'.");
			}
		}
	}

	if (setter) {
		if (p_prop.index != -1) {
			const ArgumentData &idx_arg = setter->arguments.front()->get();
			if (idx_arg.type.name != p_context.names_cache.int_type) {
				// Assume the index parameter is an enum
				// If not an int, it can be an enum
				TEST_COND(!p_context.enum_types.has(idx_arg.type.name),
						"Invalid type '", idx_arg.type.name, "' for index argument of property setter: '", p_class.name, ".", String(p_prop.name), "'.");
			}
		}
	}
}

void validate_argument(const Context &p_context, const ExposedClass &p_class, const String &p_owner_name, const String &p_owner_type, const ArgumentData &p_arg) {
#ifdef DEBUG_ENABLED
	TEST_COND((p_arg.name.is_empty() || p_arg.name.begins_with("_unnamed_arg")),
			vformat("Unnamed argument in position %d of %s '%s.%s'.", p_arg.position, p_owner_type, p_class.name, p_owner_name));

	TEST_FAIL_COND((p_arg.name != "@varargs@" && !p_arg.name.is_valid_ascii_identifier()),
			vformat("Invalid argument name '%s' of %s '%s.%s'.", p_arg.name, p_owner_type, p_class.name, p_owner_name));
#endif // DEBUG_ENABLED

	const ExposedClass *arg_class = p_context.find_exposed_class(p_arg.type);
	if (arg_class) {
		TEST_COND(arg_class->is_singleton,
				vformat("Argument type is a singleton: '%s' of %s '%s.%s'.", p_arg.name, p_owner_type, p_class.name, p_owner_name));

		if (p_class.api_type == ClassDB::API_CORE) {
			TEST_COND(arg_class->api_type == ClassDB::API_EDITOR,
					vformat("Argument '%s' of %s '%s.%s' has type '%s' from the editor API. Core API cannot have dependencies on the editor API.",
							p_arg.name, p_owner_type, p_class.name, p_owner_name, arg_class->name));
		}
	} else {
		// Look for types that don't inherit Object.
		TEST_FAIL_COND(!p_context.has_type(p_arg.type),
				vformat("Argument type '%s' not found: '%s' of %s '%s.%s'.", p_arg.type.name, p_arg.name, p_owner_type, p_class.name, p_owner_name));
	}

	if (p_arg.has_defval) {
		String type_error_msg;
		bool arg_defval_assignable_to_type = arg_default_value_is_assignable_to_type(p_context, p_arg.defval, p_arg.type, &type_error_msg);

		String err_msg = vformat("Invalid default value for parameter '%s' of %s '%s.%s'.", p_arg.name, p_owner_type, p_class.name, p_owner_name);
		if (!type_error_msg.is_empty()) {
			err_msg += " " + type_error_msg;
		}

		TEST_COND(!arg_defval_assignable_to_type, err_msg);

		bool arg_defval_valid_data = arg_default_value_is_valid_data(p_arg.defval, &type_error_msg);

		if (!type_error_msg.is_empty()) {
			err_msg += " " + type_error_msg;
		}

		TEST_COND(!arg_defval_valid_data, err_msg);
	}
}

void validate_method(const Context &p_context, const ExposedClass &p_class, const MethodData &p_method) {
	if (p_method.return_type.name != String()) {
		const ExposedClass *return_class = p_context.find_exposed_class(p_method.return_type);
		if (return_class) {
			if (p_class.api_type == ClassDB::API_CORE) {
				TEST_COND(return_class->api_type == ClassDB::API_EDITOR,
						"Method '", p_class.name, ".", p_method.name, "' has return type '", return_class->name,
						"' from the editor API. Core API cannot have dependencies on the editor API.");
			}
		} else {
			// Look for types that don't inherit Object
			TEST_FAIL_COND(!p_context.has_type(p_method.return_type),
					"Method return type '", p_method.return_type.name, "' not found: '", p_class.name, ".", p_method.name, "'.");
		}
	}

	for (const ArgumentData &F : p_method.arguments) {
		const ArgumentData &arg = F;
		validate_argument(p_context, p_class, p_method.name, "method", arg);
	}
}

void validate_signal(const Context &p_context, const ExposedClass &p_class, const SignalData &p_signal) {
	for (const ArgumentData &F : p_signal.arguments) {
		const ArgumentData &arg = F;
		validate_argument(p_context, p_class, p_signal.name, "signal", arg);
	}
}

void validate_class(const Context &p_context, const ExposedClass &p_exposed_class) {
	bool is_derived_type = p_exposed_class.base != String();

	if (!is_derived_type) {
		// Asserts about the base Object class
		TEST_FAIL_COND(p_exposed_class.name != p_context.names_cache.object_class,
				"Class '", p_exposed_class.name, "' has no base class.");
		TEST_FAIL_COND(!p_exposed_class.is_instantiable,
				"Object class is not instantiable.");
		TEST_FAIL_COND(p_exposed_class.api_type != ClassDB::API_CORE,
				"Object class is API is not API_CORE.");
		TEST_FAIL_COND(p_exposed_class.is_singleton,
				"Object class is registered as a singleton.");
	}

	TEST_FAIL_COND((is_derived_type && !p_context.exposed_classes.has(p_exposed_class.base)),
			"Base type '", p_exposed_class.base.string(), "' does not exist, for class '", p_exposed_class.name, "'.");

	for (const PropertyData &F : p_exposed_class.properties) {
		validate_property(p_context, p_exposed_class, F);
	}

	for (const MethodData &F : p_exposed_class.methods) {
		validate_method(p_context, p_exposed_class, F);
	}

	for (const SignalData &F : p_exposed_class.signals_) {
		validate_signal(p_context, p_exposed_class, F);
	}
}

void add_exposed_classes(Context &r_context) {
	LocalVector<String> class_list;
	ClassDB::get_class_list(class_list);

	for (uint32_t class_list_idx = 0; class_list_idx < class_list.size(); class_list_idx++) {
		String class_name = class_list[class_list_idx];

		ClassDB::APIType api_type = ClassDB::get_api_type(class_name);

		if (api_type == ClassDB::API_NONE) {
			continue;
		}

		if (!ClassDB::is_class_exposed(class_name)) {
			INFO(vformat("Ignoring class '%s' because it's not exposed.", class_name));
			continue;
		}

		if (!ClassDB::is_class_enabled(class_name)) {
			INFO(vformat("Ignoring class '%s' because it's not enabled.", class_name));
			continue;
		}

		ClassDB::ClassInfo *class_info = ClassDB::classes.getptr(class_name);

		ExposedClass exposed_class;
		exposed_class.name = class_name;
		exposed_class.api_type = api_type;
		exposed_class.is_singleton = Engine::has_singleton(class_name);
		exposed_class.is_instantiable = class_info->creation_func && !exposed_class.is_singleton;
		exposed_class.is_ref_counted = ClassDB::is_parent_class(class_name, "RefCounted");
		exposed_class.base = ClassDB::get_parent_class(class_name);

		// Add properties

		List<PropertyInfo> property_list;
		ClassDB::get_property_list(class_name, &property_list, true);

		HashMap<String, String> accessor_methods;

		for (const PropertyInfo &property : property_list) {
			if (property.usage & PROPERTY_USAGE_GROUP || property.usage & PROPERTY_USAGE_SUBGROUP || property.usage & PROPERTY_USAGE_CATEGORY || (property.type == Variant::NIL && property.usage & PROPERTY_USAGE_ARRAY)) {
				continue;
			}

			PropertyData prop;
			prop.name = property.name;
			prop.setter = ClassDB::get_property_setter(class_name, prop.name);
			prop.getter = ClassDB::get_property_getter(class_name, prop.name);

			if (prop.setter != String()) {
				accessor_methods[prop.setter] = prop.name;
			}
			if (prop.getter != String()) {
				accessor_methods[prop.getter] = prop.name;
			}

			bool valid = false;
			prop.index = ClassDB::get_property_index(class_name, prop.name, &valid);
			TEST_FAIL_COND(!valid, "Invalid property: '", exposed_class.name, ".", String(prop.name), "'.");

			exposed_class.properties.push_back(prop);
		}

		// Add methods

		List<MethodInfo> virtual_method_list;
		ClassDB::get_virtual_methods(class_name, &virtual_method_list, true);

		List<MethodInfo> method_list;
		ClassDB::get_method_list(class_name, &method_list, true);
		method_list.sort();

		for (const MethodInfo &E : method_list) {
			const MethodInfo &method_info = E;

			if (method_info.name.is_empty()) {
				continue;
			}

			MethodData method;
			method.name = method_info.name;
			TEST_FAIL_COND(!String(method.name).is_valid_ascii_identifier(),
					"Method name is not a valid identifier: '", exposed_class.name, ".", method.name, "'.");

			if (method_info.flags & METHOD_FLAG_VIRTUAL) {
				method.is_virtual = true;
			}

			PropertyInfo return_info = method_info.return_val;

			MethodBind *m = method.is_virtual ? nullptr : ClassDB::get_method(class_name, method_info.name);

			method.is_vararg = m && m->is_vararg();

			if (!m && !method.is_virtual) {
				TEST_FAIL_COND(!virtual_method_list.find(method_info),
						"Missing MethodBind for non-virtual method: '", exposed_class.name, ".", method.name, "'.");

				// A virtual method without the virtual flag. This is a special case.

				// The method Object.free is registered as a virtual method, but without the virtual flag.
				// This is because this method is not supposed to be overridden, but called.
				// We assume the return type is void.
				method.return_type.name = r_context.names_cache.void_type;

				// Actually, more methods like this may be added in the future, which could return
				// something different. Let's put this check to notify us if that ever happens.
				String warn_msg = vformat(
						"Notification: New unexpected virtual non-overridable method found. "
						"We only expected Object.free, but found '%s.%s'.",
						exposed_class.name, method.name);
				TEST_FAIL_COND_WARN(
						(exposed_class.name != r_context.names_cache.object_class || String(method.name) != "free"),
						warn_msg);

			} else if (return_info.type == Variant::INT && return_info.usage & (PROPERTY_USAGE_CLASS_IS_ENUM | PROPERTY_USAGE_CLASS_IS_BITFIELD)) {
				method.return_type.name = return_info.class_name;
				method.return_type.is_enum = true;
			} else if (return_info.class_name != String()) {
				method.return_type.name = return_info.class_name;

				bool bad_reference_hint = !method.is_virtual && return_info.hint != PROPERTY_HINT_RESOURCE_TYPE &&
						ClassDB::is_parent_class(return_info.class_name, r_context.names_cache.ref_counted_class);
				TEST_COND(bad_reference_hint, "Return type is reference but hint is not '" _STR(PROPERTY_HINT_RESOURCE_TYPE) "'.", " Are you returning a reference type by pointer? Method: '",
						exposed_class.name, ".", method.name, "'.");
			} else if (return_info.hint == PROPERTY_HINT_RESOURCE_TYPE) {
				method.return_type.name = return_info.hint_string;
			} else if (return_info.type == Variant::NIL && return_info.usage & PROPERTY_USAGE_NIL_IS_VARIANT) {
				method.return_type.name = r_context.names_cache.variant_type;
			} else if (return_info.type == Variant::NIL) {
				method.return_type.name = r_context.names_cache.void_type;
			} else {
				// NOTE: We don't care about the size and sign of int and float in these tests
				method.return_type.name = Variant::get_type_name(return_info.type);
			}

			for (int64_t i = 0; i < method_info.arguments.size(); ++i) {
				const PropertyInfo &arg_info = method_info.arguments[i];

				String orig_arg_name = arg_info.name;

				ArgumentData arg;
				arg.name = orig_arg_name;
				arg.position = i;

				if (arg_info.type == Variant::INT && arg_info.usage & (PROPERTY_USAGE_CLASS_IS_ENUM | PROPERTY_USAGE_CLASS_IS_BITFIELD)) {
					arg.type.name = arg_info.class_name;
					arg.type.is_enum = true;
				} else if (arg_info.class_name != String()) {
					arg.type.name = arg_info.class_name;
				} else if (arg_info.hint == PROPERTY_HINT_RESOURCE_TYPE) {
					arg.type.name = arg_info.hint_string;
				} else if (arg_info.type == Variant::NIL) {
					arg.type.name = r_context.names_cache.variant_type;
				} else {
					// NOTE: We don't care about the size and sign of int and float in these tests
					arg.type.name = Variant::get_type_name(arg_info.type);
				}

				if (m && m->has_default_argument(i)) {
					arg.has_defval = true;
					arg.defval = m->get_default_argument(i);
				}

				method.arguments.push_back(arg);
			}

			if (method.is_vararg) {
				ArgumentData vararg;
				vararg.type.name = r_context.names_cache.vararg_stub_type;
				vararg.name = "@varargs@";
				method.arguments.push_back(vararg);
			}

			TEST_COND(exposed_class.find_property_by_name(method.name),
					"Method name conflicts with property: '", String(class_name), ".", String(method.name), "'.");

			// Methods starting with an underscore are ignored unless they're virtual or used as a property setter or getter.
			if (!method.is_virtual && String(method.name)[0] == '_') {
				for (const PropertyData &F : exposed_class.properties) {
					const PropertyData &prop = F;

					if (prop.setter == method.name || prop.getter == method.name) {
						exposed_class.methods.push_back(method);
						break;
					}
				}
			} else {
				exposed_class.methods.push_back(method);
			}

			if (method.is_virtual) {
				TEST_COND(String(method.name)[0] != '_', "Virtual method ", String(method.name), " does not start with underscore.");
			}
		}

		// Add signals

		const AHashMap<String, const MethodInfo *> &signal_map = class_info->vltrtype->get_signal_map(true);

		for (const KeyValue<String, const MethodInfo *> &K : signal_map) {
			SignalData signal;

			const MethodInfo &method_info = *signal_map.get(K.key);

			signal.name = method_info.name;
			TEST_FAIL_COND(!String(signal.name).is_valid_ascii_identifier(),
					"Signal name is not a valid identifier: '", exposed_class.name, ".", signal.name, "'.");

			for (int64_t i = 0; i < method_info.arguments.size(); ++i) {
				const PropertyInfo &arg_info = method_info.arguments[i];

				String orig_arg_name = arg_info.name;

				ArgumentData arg;
				arg.name = orig_arg_name;
				arg.position = i;

				if (arg_info.type == Variant::INT && arg_info.usage & (PROPERTY_USAGE_CLASS_IS_ENUM | PROPERTY_USAGE_CLASS_IS_BITFIELD)) {
					arg.type.name = arg_info.class_name;
					arg.type.is_enum = true;
				} else if (arg_info.class_name != String()) {
					arg.type.name = arg_info.class_name;
				} else if (arg_info.hint == PROPERTY_HINT_RESOURCE_TYPE) {
					arg.type.name = arg_info.hint_string;
				} else if (arg_info.type == Variant::NIL) {
					arg.type.name = r_context.names_cache.variant_type;
				} else {
					// NOTE: We don't care about the size and sign of int and float in these tests
					arg.type.name = Variant::get_type_name(arg_info.type);
				}

				signal.arguments.push_back(arg);
			}

			bool method_conflict = exposed_class.find_property_by_name(signal.name);

			String warn_msg = vformat(
					"Signal name conflicts with %s: '%s.%s.",
					method_conflict ? "method" : "property", class_name, signal.name);
			TEST_FAIL_COND((method_conflict || exposed_class.find_method_by_name(signal.name)),
					warn_msg);

			exposed_class.signals_.push_back(signal);
		}

		// Add enums and constants

		List<String> constants;
		ClassDB::get_integer_constant_list(class_name, &constants, true);

		const AHashMap<String, const GDType::EnumInfo *> &enum_map = class_info->vltrtype->get_enum_map(true);

		for (const KeyValue<String, const GDType::EnumInfo *> &kv_enum : enum_map) {
			EnumData enum_;
			enum_.name = kv_enum.key;

			for (const KeyValue<String, int64_t> &kv_case : kv_enum.value->values) {
				const String &constant_name = kv_case.key;
				TEST_FAIL_COND(String(constant_name).contains("::"),
						"Enum constant contains '::', check bindings to remove the scope: '",
						String(class_name), ".", String(enum_.name), ".", String(constant_name), "'.");
				const int64_t *value = class_info->vltrtype->get_integer_constant_map(false).getptr(constant_name);
				TEST_FAIL_COND(!value, "Missing enum constant value: '",
						String(class_name), ".", String(enum_.name), ".", String(constant_name), "'.");
				constants.erase(constant_name);

				ConstantData constant;
				constant.name = constant_name;
				constant.value = *value;

				enum_.constants.push_back(constant);
			}

			exposed_class.enums.push_back(enum_);

			r_context.enum_types.push_back(String(class_name) + "." + String(kv_enum.key));
		}

		for (const String &E : constants) {
			const String &constant_name = E;
			TEST_FAIL_COND(constant_name.contains("::"),
					"Constant contains '::', check bindings to remove the scope: '",
					String(class_name), ".", constant_name, "'.");
			const int64_t *value = class_info->vltrtype->get_integer_constant_map(false).getptr(String(E));
			TEST_FAIL_COND(!value, "Missing constant value: '", String(class_name), ".", String(constant_name), "'.");

			ConstantData constant;
			constant.name = constant_name;
			constant.value = *value;

			exposed_class.constants.push_back(constant);
		}

		r_context.exposed_classes.insert(class_name, exposed_class);
	}
}

void add_builtin_types(Context &r_context) {
	// NOTE: We don't care about the size and sign of int and float in these tests
	for (int i = 0; i < Variant::VARIANT_MAX; i++) {
		r_context.builtin_types.push_back(Variant::get_type_name(Variant::Type(i)));
	}

	r_context.builtin_types.push_back(_STR(Variant));
	r_context.builtin_types.push_back(r_context.names_cache.vararg_stub_type);
	r_context.builtin_types.push_back("void");
}

void add_global_enums(Context &r_context) {
	int global_constants_count = CoreConstants::get_global_constant_count();

	if (global_constants_count > 0) {
		for (int i = 0; i < global_constants_count; i++) {
			String enum_name = CoreConstants::get_global_constant_enum(i);

			if (enum_name != String()) {
				ConstantData constant;
				constant.name = CoreConstants::get_global_constant_name(i);
				constant.value = CoreConstants::get_global_constant_value(i);

				EnumData enum_;
				enum_.name = enum_name;
				List<EnumData>::Element *enum_match = r_context.global_enums.find(enum_);
				if (enum_match) {
					enum_match->get().constants.push_back(constant);
				} else {
					enum_.constants.push_back(constant);
					r_context.global_enums.push_back(enum_);
				}
			}
		}

		for (const EnumData &E : r_context.global_enums) {
			r_context.enum_types.push_back(E.name);
		}
	}

	for (int i = 0; i < Variant::VARIANT_MAX; i++) {
		if (i == Variant::OBJECT) {
			continue;
		}

		const Variant::Type type = Variant::Type(i);

		List<String> enum_names;
		Variant::get_enums_for_type(type, &enum_names);

		for (const String &enum_name : enum_names) {
			r_context.enum_types.push_back(Variant::get_type_name(type) + "." + enum_name);
		}
	}
}

TEST_SUITE("[ClassDB]") {
	TEST_CASE("[ClassDB] Add exposed classes, builtin types, and global enums") {
		Context context;

		add_exposed_classes(context);
		add_builtin_types(context);
		add_global_enums(context);

		SUBCASE("[ClassDB] Validate exposed classes") {
			const ExposedClass *object_class = context.find_exposed_class(context.names_cache.object_class);
			TEST_FAIL_COND(!object_class, "Object class not found.");
			TEST_FAIL_COND(object_class->base != String(),
					"Object class derives from another class: '", object_class->base, "'.");

			for (const KeyValue<String, ExposedClass> &E : context.exposed_classes) {
				validate_class(context, E.value);
			}
		}
	}
}

} // namespace TestClassDB

// === Merged from tests/core/object/test_method_bind.cpp ===
/*  test_method_bind.cpp                                                  */
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




namespace TestMethodBind
{

class MethodBindTester
{
public:
	mem_unique_ptr<Object> obj;
	enum Test
	{
		TEST_METHOD,
		TEST_METHOD_ARGS,
		TEST_METHODC,
		TEST_METHODC_ARGS,
		TEST_METHODR,
		TEST_METHODR_ARGS,
		TEST_METHODRC,
		TEST_METHODRC_ARGS,
		TEST_METHOD_DEFARGS,
		TEST_METHOD_OBJECT_CAST,
		TEST_MAX
	};

	class ObjectSubclass
	{
	public:
		mem_unique_ptr<Object> obj;
		int value = 1;
	};

	int test_num = 0;

	bool test_valid[TEST_MAX];

	void test_method() { test_valid[TEST_METHOD] = true; }

	void test_method_args(int p_arg) { test_valid[TEST_METHOD_ARGS] = p_arg == test_num; }

	void test_methodc() { test_valid[TEST_METHODC] = true; }

	void test_methodc_args(int p_arg) { test_valid[TEST_METHODC_ARGS] = p_arg == test_num; }

	int test_methodr()
	{
		test_valid[TEST_METHODR] = true; // temporary
		return test_num;
	}

	int test_methodr_args(int p_arg)
	{
		test_valid[TEST_METHODR_ARGS] = true; // temporary
		return p_arg;
	}

	int test_methodrc()
	{
		test_valid[TEST_METHODRC] = true; // temporary
		return test_num;
	}

	int test_methodrc_args(int p_arg)
	{
		test_valid[TEST_METHODRC_ARGS] = true; // temporary
		return p_arg;
	}

	void test_method_default_args(int p_arg1, int p_arg2, int p_arg3, int p_arg4, int p_arg5)
	{
		test_valid[TEST_METHOD_DEFARGS] =
			p_arg1 == 1 && p_arg2 == 2 && p_arg3 == 3 && p_arg4 == 4 && p_arg5 == 5; // temporary
	}

	void test_method_object_cast(ObjectSubclass* p_object)
	{
		test_valid[TEST_METHOD_OBJECT_CAST] = p_object->value == 1;
	}

	static void _bind_methods() {}

	virtual void run_tests()
	{
		for (int i = 0; i < TEST_MAX; i++) {
			test_valid[i] = false;
		}
		// regular
		test_num = Math::rand();
		this->obj->call("test_method");
		test_num = Math::rand();
		this->obj->call("test_method_args", test_num);
		test_num = Math::rand();
		this->obj->call("test_methodc");
		test_num = Math::rand();
		this->obj->call("test_methodc_args", test_num);
		// return
		test_num = Math::rand();
		test_valid[TEST_METHODR] =
			int(this->obj->call("test_methodr")) == test_num && test_valid[TEST_METHODR];
		test_num = Math::rand();
		test_valid[TEST_METHODR_ARGS] =
			int(this->obj->call("test_methodr_args", test_num)) == test_num && test_valid[TEST_METHODR_ARGS];
		test_num = Math::rand();
		test_valid[TEST_METHODRC] =
			int(this->obj->call("test_methodrc")) == test_num && test_valid[TEST_METHODRC];
		test_num = Math::rand();
		test_valid[TEST_METHODRC_ARGS] =
			int(this->obj->call("test_methodrc_args", test_num)) == test_num && test_valid[TEST_METHODRC_ARGS];

		this->obj->call("test_method_default_args", 1, 2, 3, 4);

		ObjectSubclass* obj = memnew(ObjectSubclass);
		this->obj->call("test_method_object_cast", obj);
		memdelete(obj);
	}
};

TEST_CASE("[MethodBind] check all method binds")
{
	MethodBindTester* mbt = memnew(MethodBindTester);

	mbt->run_tests();

	CHECK(mbt->test_valid[MethodBindTester::TEST_METHOD]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHOD_ARGS]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHODC]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHODC_ARGS]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHODR]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHODR_ARGS]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHODRC]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHODRC_ARGS]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHOD_DEFARGS]);
	CHECK(mbt->test_valid[MethodBindTester::TEST_METHOD_OBJECT_CAST]);

	memdelete(mbt);
}

} // namespace TestMethodBind

// === Merged from tests/core/object/test_object.cpp ===
/*  test_object.cpp                                                       */
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




namespace TestObject
{

class _TestDerivedObject
{
	int property_value;

protected:
	static void _bind_methods() {}

public:
	mem_unique_ptr<Object> obj;

	void set_property(int value) { property_value = value; }

	int get_property() const { return property_value; }
};

class _MockScriptInstance : public ScriptInstance
{
	String property_name = "NO_NAME";
	Variant property_value;

public:
	bool set(const String& p_name, const Variant& p_value) override
	{
		property_name = p_name;
		property_value = p_value;
		return true;
	}

	bool get(const String& p_name, Variant& r_ret) const override
	{
		if (property_name == p_name) {
			r_ret = property_value;
			return true;
		}
		return false;
	}

	void get_property_list(List<PropertyInfo>* p_properties) const override {}

	Variant::Type get_property_type(const String& p_name, bool* r_is_valid) const override
	{
		return Variant::PACKED_FLOAT32_ARRAY;
	}

	virtual void validate_property(PropertyInfo& p_property) const override {}

	bool property_can_revert(const String& p_name) const override { return false; }

	bool property_get_revert(const String& p_name, Variant& r_ret) const override
	{
		return false;
	}

	void get_method_list(List<MethodInfo>* p_list) const override {}

	bool has_method(const String& p_method) const override { return false; }

	int get_method_argument_count(
		const String& p_method, bool* r_is_valid = nullptr) const override
	{
		if (r_is_valid) {
			*r_is_valid = false;
		}
		return 0;
	}

	Variant callp(const String& p_method, const Variant** p_args, int p_argcount,
		Callable::CallError& r_error) override
	{
		return Variant();
	}

	void notification(int p_notification, bool p_reversed = false) override {}

	Ref<Script> get_script() const override { return Ref<Script>(); }

	const Variant get_rpc_config() const override { return Variant(); }

	ScriptLanguage* get_language() override { return nullptr; }
};

} // namespace TestObject

// === Merged from tests/core/object/test_undo_redo.cpp ===
/*  test_undo_redo.cpp                                                    */
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



namespace TestUndoRedo
{

class _TestUndoRedoObject
{
	int property_value = 0;

public:
	void set_property(int value) { property_value = value; }

	int get_property() const { return property_value; }

	void add_to_property(int value) { property_value += value; }

	void subtract_from_property(int value) { property_value -= value; }
};

} // namespace TestUndoRedo

// === Merged from tests/core/os/test_os.cpp ===
/*  test_os.cpp                                                           */
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



#include "core/os/os.h"

namespace TestOS {

TEST_CASE("[OS] Environment variables") {
#ifdef WINDOWS_ENABLED
	CHECK_MESSAGE(
			OS::has_environment("USERPROFILE"),
			"The USERPROFILE environment variable should be present.");
#else
	CHECK_MESSAGE(
			OS::has_environment("HOME"),
			"The HOME environment variable should be present.");
#endif
}

TEST_CASE("[OS] UTF-8 environment variables") {
	String value = String::utf8("hell\xc3\xb6"); // "hellö", UTF-8 encoded

	OS::set_environment("HELLO", value);
	String val = OS::get_environment("HELLO");
	CHECK_MESSAGE(
			val == value,
			"The previously-set HELLO environment variable should return the expected value.");
	CHECK_MESSAGE(
			val.length() == 5,
			"The previously-set HELLO environment variable was decoded as UTF-8 and should have a length of 5.");
	OS::unset_environment("HELLO");
}

TEST_CASE("[OS] Non-UTF-8 environment variables") {
	String value = String("\xff t\xf6rkylempij\xe4vongahdus"); // hex FF and a Finnish pangram, latin-1
	OS::set_environment("HELLO", value);
	String val = OS::get_environment("HELLO");
	CHECK_MESSAGE(
			val == value,
			"The previously-set HELLO environment variable should return the expected value.");
	CHECK_MESSAGE(
			val.length() == 23,
			"The previously-set HELLO environment variable was not decoded from Latin-1.");
	OS::unset_environment("HELLO");
}

TEST_CASE("[OS] Command line arguments") {
	List<String> arguments = OS::get_cmdline_args();
	bool found = false;
	for (const String &arg : arguments) {
		if (arg == "--test") {
			found = true;
			break;
		}
	}
	CHECK_MESSAGE(
			found,
			"The `--test` option must be present in the list of command line arguments.");
}

TEST_CASE("[OS] Executable and data paths") {
	CHECK_MESSAGE(
			OS::get_executable_path().is_absolute_path(),
			"The executable path returned should be an absolute path.");
	CHECK_MESSAGE(
			OS::get_data_path().is_absolute_path(),
			"The user data path returned should be an absolute path.");
	CHECK_MESSAGE(
			OS::get_config_path().is_absolute_path(),
			"The user configuration path returned should be an absolute path.");
	CHECK_MESSAGE(
			OS::get_cache_path().is_absolute_path(),
			"The cache path returned should be an absolute path.");
}

TEST_CASE("[OS] Ticks") {
	CHECK_MESSAGE(
			OS::get_ticks_usec() > 1000,
			"The returned ticks (in microseconds) must be greater than 1,000.");
	CHECK_MESSAGE(
			OS::get_ticks_msec() > 1,
			"The returned ticks (in milliseconds) must be greater than 1.");
}

TEST_CASE("[OS] Feature tags") {
#ifdef TOOLS_ENABLED
	CHECK_MESSAGE(
			OS::has_feature("editor"),
			"The binary has the \"editor\" feature tag.");
	CHECK_MESSAGE(
			!OS::has_feature("template"),
			"The binary does not have the \"template\" feature tag.");
	CHECK_MESSAGE(
			!OS::has_feature("template_debug"),
			"The binary does not have the \"template_debug\" feature tag.");
	CHECK_MESSAGE(
			!OS::has_feature("template_release"),
			"The binary does not have the \"template_release\" feature tag.");
#else
	CHECK_MESSAGE(
			!OS::has_feature("editor"),
			"The binary does not have the \"editor\" feature tag.");
	CHECK_MESSAGE(
			OS::has_feature("template"),
			"The binary has the \"template\" feature tag.");
#ifdef DEBUG_ENABLED
	CHECK_MESSAGE(
			OS::has_feature("template_debug"),
			"The binary has the \"template_debug\" feature tag.");
	CHECK_MESSAGE(
			!OS::has_feature("template_release"),
			"The binary does not have the \"template_release\" feature tag.");
#else
	CHECK_MESSAGE(
			!OS::has_feature("template_debug"),
			"The binary does not have the \"template_debug\" feature tag.");
	CHECK_MESSAGE(
			OS::has_feature("template_release"),
			"The binary has the \"template_release\" feature tag.");
#endif // DEBUG_ENABLED
#endif // TOOLS_ENABLED
}

TEST_CASE("[OS] Process ID") {
	CHECK_MESSAGE(
			OS::get_process_id() >= 1,
			"The returned process ID should be greater than zero.");
}

TEST_CASE("[OS] Processor count and memory information") {
	CHECK_MESSAGE(
			OS::get_processor_count() >= 1,
			"The returned processor count should be greater than zero.");
#ifdef DEBUG_ENABLED
	CHECK_MESSAGE(
			OS::get_static_memory_usage() >= 1,
			"The returned static memory usage should be greater than zero.");
	CHECK_MESSAGE(
			OS::get_static_memory_peak_usage() >= 1,
			"The returned static memory peak usage should be greater than zero.");
#endif // DEBUG_ENABLED
}

TEST_CASE("[OS] Execute") {
#ifdef WINDOWS_ENABLED
	List<String> arguments;
	arguments.push_back("/C");
	arguments.push_back("dir > NUL");
	int exit_code;
	const Error err = OS::execute("cmd", arguments, nullptr, &exit_code);
	CHECK_MESSAGE(
			err == OK,
			"(Running the command `cmd /C \"dir > NUL\"` returns the expected Godot error code (OK).");
	CHECK_MESSAGE(
			exit_code == 0,
			"Running the command `cmd /C \"dir > NUL\"` returns a zero (successful) exit code.");
#else
	List<String> arguments;
	arguments.push_back("-c");
	arguments.push_back("ls > /dev/null");
	int exit_code;
	const Error err = OS::execute("sh", arguments, nullptr, &exit_code);
	CHECK_MESSAGE(
			err == OK,
			"(Running the command `sh -c \"ls > /dev/null\"` returns the expected Godot error code (OK).");
	CHECK_MESSAGE(
			exit_code == 0,
			"Running the command `sh -c \"ls > /dev/null\"` returns a zero (successful) exit code.");
#endif
}

} // namespace TestOS

// === Merged from tests/core/string/test_fuzzy_search.cpp ===
/*  test_fuzzy_search.cpp                                                 */
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



#include "core/io/file_access.h"
#include "core/config/project_settings.h"
#include "tests/test_macros.h"

namespace TestFuzzySearch {

struct FuzzySearchTestCase {
	String query;
	String expected;
};

// Ideally each of these test queries should represent a different aspect, and potentially bottleneck, of the search process.
const FuzzySearchTestCase test_cases[] = {
	// Short query, many matches, few adjacent characters
	{ "///gd", "./menu/hud/hud.gd" },
	// Filename match with typo
	{ "sm.png", "./entity/blood_sword/sam.png" },
	// Multipart filename word matches
	{ "ham ", "./entity/game_trap/ha_missed_me.wav" },
	// Single word token matches
	{ "push background", "./entity/background_zone1/background/push.png" },
	// Long token matches
	{ "background_freighter background png", "./entity/background_freighter/background/background.png" },
	// Many matches, many short tokens
	{ "menu menu characters wav", "./menu/menu/characters/smoker/0.wav" },
	// Maximize total matches
	{ "entity gd", "./entity/entity_man.gd" }
};

Vector<String> load_test_data() {
	Ref<FileAccess> fp = FileAccess::open(TestUtils::get_data_path("fuzzy_search/project_dir_tree.txt"), FileAccess::READ);
	REQUIRE(fp.is_valid());
	return fp->get_as_utf8_string().split("\n");
}

TEST_CASE("[FuzzySearch] Test fuzzy search results") {
	FuzzySearch search;
	Vector<String> targets = load_test_data();

	for (FuzzySearchTestCase test_case : test_cases) {
		Vector<Ref<FuzzySearchMatch>> results = search.search_all(test_case.query, targets);
		CHECK_GT(results.size(), 0);
		CHECK_EQ(results[0]->get_target(), test_case.expected);
	}
}

} // namespace TestFuzzySearch

// === Merged from tests/core/string/test_node_path.cpp ===
/*  test_node_path.cpp                                                    */
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



#include "core/types.h"

namespace TestNodePath {

TEST_CASE("[NodePath] Relative path") {
	const NodePath node_path_relative = NodePath("Path2D/PathFollow2D/Sprite2D:position:x");

	CHECK_MESSAGE(
			node_path_relative.get_as_property_path() == NodePath(":Path2D/PathFollow2D/Sprite2D:position:x"),
			"The returned property path should match the expected value.");
	CHECK_MESSAGE(
			node_path_relative.get_concatenated_subnames() == "position:x",
			"The returned concatenated subnames should match the expected value.");

	CHECK_MESSAGE(
			node_path_relative.get_name(0) == "Path2D",
			"The returned name at index 0 should match the expected value.");
	CHECK_MESSAGE(
			node_path_relative.get_name(1) == "PathFollow2D",
			"The returned name at index 1 should match the expected value.");
	CHECK_MESSAGE(
			node_path_relative.get_name(2) == "Sprite2D",
			"The returned name at index 2 should match the expected value.");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			node_path_relative.get_name(3) == "",
			"The returned name at invalid index 3 should match the expected value.");
	CHECK_MESSAGE(
			node_path_relative.get_name(-1) == "",
			"The returned name at invalid index -1 should match the expected value.");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			node_path_relative.get_name_count() == 3,
			"The returned number of names should match the expected value.");

	CHECK_MESSAGE(
			node_path_relative.get_subname(0) == "position",
			"The returned subname at index 0 should match the expected value.");
	CHECK_MESSAGE(
			node_path_relative.get_subname(1) == "x",
			"The returned subname at index 1 should match the expected value.");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			node_path_relative.get_subname(2) == "",
			"The returned subname at invalid index 2 should match the expected value.");
	CHECK_MESSAGE(
			node_path_relative.get_subname(-1) == "",
			"The returned subname at invalid index -1 should match the expected value.");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			node_path_relative.get_subname_count() == 2,
			"The returned number of subnames should match the expected value.");

	CHECK_MESSAGE(
			!node_path_relative.is_absolute(),
			"The node path should be considered relative.");

	CHECK_MESSAGE(
			!node_path_relative.is_empty(),
			"The node path shouldn't be considered empty.");
}

TEST_CASE("[NodePath] Absolute path") {
	const NodePath node_path_absolute = NodePath("/root/Sprite2D");

	CHECK_MESSAGE(
			node_path_absolute.get_as_property_path() == NodePath(":root/Sprite2D"),
			"The returned property path should match the expected value.");
	CHECK_MESSAGE(
			node_path_absolute.get_concatenated_subnames() == "",
			"The returned concatenated subnames should match the expected value.");

	CHECK_MESSAGE(
			node_path_absolute.get_name(0) == "root",
			"The returned name at index 0 should match the expected value.");
	CHECK_MESSAGE(
			node_path_absolute.get_name(1) == "Sprite2D",
			"The returned name at index 1 should match the expected value.");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			node_path_absolute.get_name(2) == "",
			"The returned name at invalid index 2 should match the expected value.");
	CHECK_MESSAGE(
			node_path_absolute.get_name(-1) == "",
			"The returned name at invalid index -1 should match the expected value.");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			node_path_absolute.get_name_count() == 2,
			"The returned number of names should match the expected value.");

	CHECK_MESSAGE(
			node_path_absolute.get_subname_count() == 0,
			"The returned number of subnames should match the expected value.");

	CHECK_MESSAGE(
			node_path_absolute.is_absolute(),
			"The node path should be considered absolute.");

	CHECK_MESSAGE(
			!node_path_absolute.is_empty(),
			"The node path shouldn't be considered empty.");
}

TEST_CASE("[NodePath] Empty path") {
	const NodePath node_path_empty = NodePath();

	CHECK_MESSAGE(
			node_path_empty.get_as_property_path() == NodePath(),
			"The returned property path should match the expected value.");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			node_path_empty.get_concatenated_subnames() == "",
			"The returned concatenated subnames should match the expected value.");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			node_path_empty.get_name_count() == 0,
			"The returned number of names should match the expected value.");

	CHECK_MESSAGE(
			node_path_empty.get_subname_count() == 0,
			"The returned number of subnames should match the expected value.");

	CHECK_MESSAGE(
			!node_path_empty.is_absolute(),
			"The node path shouldn't be considered absolute.");

	CHECK_MESSAGE(
			node_path_empty.is_empty(),
			"The node path should be considered empty.");
}

TEST_CASE("[NodePath] Slice") {
	const NodePath node_path_relative = NodePath("Parent/Child:prop:subprop");
	const NodePath node_path_absolute = NodePath("/root/Parent/Child:prop");
	CHECK_MESSAGE(
			node_path_relative.slice(0, 2) == NodePath("Parent/Child"),
			"The slice lower bound should be inclusive and the slice upper bound should be exclusive.");
	CHECK_MESSAGE(
			node_path_relative.slice(3) == NodePath(":subprop"),
			"Slicing on the last index (length - 1) should return the last entry.");
	CHECK_MESSAGE(
			node_path_relative.slice(1) == NodePath("Child:prop:subprop"),
			"Slicing without upper bound should return remaining entries after index.");
	CHECK_MESSAGE(
			node_path_relative.slice(1, 3) == NodePath("Child:prop"),
			"Slicing should include names and subnames.");
	CHECK_MESSAGE(
			node_path_relative.slice(-1) == NodePath(":subprop"),
			"Slicing on -1 should return the last entry.");
	CHECK_MESSAGE(
			node_path_relative.slice(0, -1) == NodePath("Parent/Child:prop"),
			"Slicing up to -1 should include the second-to-last entry.");
	CHECK_MESSAGE(
			node_path_relative.slice(-2, -1) == NodePath(":prop"),
			"Slicing from negative to negative should treat lower bound as inclusive and upper bound as exclusive.");
	CHECK_MESSAGE(
			node_path_relative.slice(0, 10) == NodePath("Parent/Child:prop:subprop"),
			"Slicing past the length of the path should work like slicing up to the last entry.");
	CHECK_MESSAGE(
			node_path_relative.slice(-10, 2) == NodePath("Parent/Child"),
			"Slicing negatively past the length of the path should work like slicing from the first entry.");
	CHECK_MESSAGE(
			node_path_relative.slice(1, 1) == NodePath(""),
			"Slicing with a lower bound equal to upper bound should return empty path.");

	CHECK_MESSAGE(
			node_path_absolute.slice(0, 2) == NodePath("/root/Parent"),
			"Slice from beginning of an absolute path should be an absolute path.");
	CHECK_MESSAGE(
			node_path_absolute.slice(1, 4) == NodePath("Parent/Child:prop"),
			"Slice of an absolute path that does not start at the beginning should be a relative path.");
	CHECK_MESSAGE(
			node_path_absolute.slice(3, 4) == NodePath(":prop"),
			"Slice of an absolute path that does not start at the beginning should be a relative path.");

	CHECK_MESSAGE(
			NodePath("").slice(0, 1) == NodePath(""),
			"Slice of an empty path should be an empty path.");
	CHECK_MESSAGE(
			NodePath("").slice(-1, 2) == NodePath(""),
			"Slice of an empty path should be an empty path.");
	CHECK_MESSAGE(
			NodePath("/").slice(-1, 2) == NodePath("/"),
			"Slice of an empty absolute path should be an empty absolute path.");
}

} // namespace TestNodePath

// === Merged from tests/core/string/test_string.cpp ===
/*  test_string.cpp                                                       */
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



#include "core/string/ustring.h"

namespace TestString {

int u32scmp(const char32_t *l, const char32_t *r) {
	for (; *l == *r && *l && *r; l++, r++) {
		// Continue.
	}
	return *l - *r;
}

TEST_CASE("[String] Assign Latin-1 char string") {
	String s = "Hello";
	CHECK(u32scmp(s.get_data(), U"Hello") == 0);
}

TEST_CASE("[String] Assign from Latin-1 char string (operator=)") {
	String s = "Dolly";
	const String &t = s;
	CHECK(u32scmp(t.get_data(), U"Dolly") == 0);
}

TEST_CASE("[String] Assign from Latin-1 char string (copycon)") {
	String s("Sheep");
	const String &t1(s);
	CHECK(u32scmp(t1.get_data(), U"Sheep") == 0);

	String t2 = String::latin1(Span("Sheep", 3));
	CHECK(u32scmp(t2.get_data(), U"She") == 0);
}

TEST_CASE("[String] Assign from wchar_t string (operator=)") {
	String s = L"Give me";
	CHECK(u32scmp(s.get_data(), U"Give me") == 0);
}

TEST_CASE("[String] Assign from wchar_t string (copycon)") {
	String s(L"Wool");
	CHECK(u32scmp(s.get_data(), U"Wool") == 0);
}

TEST_CASE("[String] Assign from char32_t string (operator=)") {
	String s = U"Give me";
	CHECK(u32scmp(s.get_data(), U"Give me") == 0);
}

TEST_CASE("[String] Assign from char32_t string (copycon)") {
	String s(U"Wool");
	CHECK(u32scmp(s.get_data(), U"Wool") == 0);
}

TEST_CASE("[String] UTF8") {
	/* how can i embed UTF in here? */
	static const char32_t u32str[] = { 0x0045, 0x0020, 0x304A, 0x360F, 0x3088, 0x3046, 0x1F3A4, 0 };
	static const uint8_t u8str[] = { 0x45, 0x20, 0xE3, 0x81, 0x8A, 0xE3, 0x98, 0x8F, 0xE3, 0x82, 0x88, 0xE3, 0x81, 0x86, 0xF0, 0x9F, 0x8E, 0xA4, 0 };
	String expected = u32str;
	String parsed;
	Error err = parsed.append_utf8(expected.utf8().get_data());
	CHECK(err == OK);
	CHECK(parsed == u32str);

	parsed.clear();
	err = parsed.append_utf8((const char *)u8str);
	CHECK(err == OK);
	CHECK(parsed == u32str);

	CharString cs = (const char *)u8str;
	CHECK(String::utf8(cs) == parsed);
}

TEST_CASE("[String] UTF16") {
	/* how can i embed UTF in here? */
	static const char32_t u32str[] = { 0x0045, 0x0020, 0x304A, 0x360F, 0x3088, 0x3046, 0x1F3A4, 0 };
	static const char16_t u16str[] = { 0x0045, 0x0020, 0x304A, 0x360F, 0x3088, 0x3046, 0xD83C, 0xDFA4, 0 };
	String expected = u32str;
	String parsed;
	Error err = parsed.append_utf16(expected.utf16().get_data());
	CHECK(err == OK);
	CHECK(parsed == u32str);

	parsed.clear();
	err = parsed.append_utf16(u16str);
	CHECK(err == OK);
	CHECK(parsed == u32str);

	Char16String cs = u16str;
	CHECK(String::utf16(cs) == parsed);
}

TEST_CASE("[String] UTF8 with BOM") {
	/* how can i embed UTF in here? */
	static const char32_t u32str[] = { 0x0045, 0x0020, 0x304A, 0x360F, 0x3088, 0x3046, 0x1F3A4, 0 };
	static const uint8_t u8str[] = { 0xEF, 0xBB, 0xBF, 0x45, 0x20, 0xE3, 0x81, 0x8A, 0xE3, 0x98, 0x8F, 0xE3, 0x82, 0x88, 0xE3, 0x81, 0x86, 0xF0, 0x9F, 0x8E, 0xA4, 0 };
	String s;
	Error err = s.append_utf8((const char *)u8str);
	CHECK(err == OK);
	CHECK(s == u32str);

	CharString cs = (const char *)u8str;
	CHECK(String::utf8(cs) == s);
}

TEST_CASE("[String] UTF16 with BOM") {
	/* how can i embed UTF in here? */
	static const char32_t u32str[] = { 0x0020, 0x0045, 0x304A, 0x360F, 0x3088, 0x3046, 0x1F3A4, 0 };
	static const char16_t u16str[] = { 0xFEFF, 0x0020, 0x0045, 0x304A, 0x360F, 0x3088, 0x3046, 0xD83C, 0xDFA4, 0 };
	static const char16_t u16str_swap[] = { 0xFFFE, 0x2000, 0x4500, 0x4A30, 0x0F36, 0x8830, 0x4630, 0x3CD8, 0xA4DF, 0 };
	String s;
	Error err = s.append_utf16(u16str);
	CHECK(err == OK);
	CHECK(s == u32str);

	s.clear();
	err = s.append_utf16(u16str_swap);
	CHECK(err == OK);
	CHECK(s == u32str);

	Char16String cs = u16str;
	CHECK(String::utf16(cs) == s);

	cs = u16str_swap;
	CHECK(String::utf16(cs) == s);
}

TEST_CASE("[String] Invalid UTF8 (non shortest form sequence)") {
	ERR_PRINT_OFF
	// Examples from the unicode standard : 3.9 Unicode Encoding Forms - Table 3.8.
	static const uint8_t u8str[] = { 0xC0, 0xAF, 0xE0, 0x80, 0xBF, 0xF0, 0x81, 0x82, 0x41, 0 };
	static const char32_t u32str[] = { 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x41, 0 };
	String s;
	Error err = s.append_utf8((const char *)u8str);
	CHECK(err == ERR_INVALID_DATA);
	CHECK(s == u32str);

	CharString cs = (const char *)u8str;
	CHECK(String::utf8(cs) == s);
	ERR_PRINT_ON
}

TEST_CASE("[String] Invalid UTF8 (ill formed sequences for surrogates)") {
	ERR_PRINT_OFF
	// Examples from the unicode standard : 3.9 Unicode Encoding Forms - Table 3.9.
	static const uint8_t u8str[] = { 0xED, 0xA0, 0x80, 0xED, 0xBF, 0xBF, 0xED, 0xAF, 0x41, 0 };
	static const char32_t u32str[] = { 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x41, 0 };
	String s;
	Error err = s.append_utf8((const char *)u8str);
	CHECK(err == ERR_INVALID_DATA);
	CHECK(s == u32str);

	CharString cs = (const char *)u8str;
	CHECK(String::utf8(cs) == s);
	ERR_PRINT_ON
}

TEST_CASE("[String] Invalid UTF8 (other ill formed sequences)") {
	ERR_PRINT_OFF
	// Examples from the unicode standard : 3.9 Unicode Encoding Forms - Table 3.10.
	static const uint8_t u8str[] = { 0xF4, 0x91, 0x92, 0x93, 0xFF, 0x41, 0x80, 0xBF, 0x42, 0 };
	static const char32_t u32str[] = { 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x41, 0xFFFD, 0xFFFD, 0x42, 0 };
	String s;
	Error err = s.append_utf8((const char *)u8str);
	CHECK(err == ERR_INVALID_DATA);
	CHECK(s == u32str);

	CharString cs = (const char *)u8str;
	CHECK(String::utf8(cs) == s);
	ERR_PRINT_ON
}

TEST_CASE("[String] Invalid UTF8 (truncated sequences)") {
	ERR_PRINT_OFF
	// Examples from the unicode standard : 3.9 Unicode Encoding Forms - Table 3.11.
	static const uint8_t u8str[] = { 0xE1, 0x80, 0xE2, 0xF0, 0x91, 0x92, 0xF1, 0xBF, 0x41, 0 };
	static const char32_t u32str[] = { 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0x41, 0 };
	String s;
	Error err = s.append_utf8((const char *)u8str);
	CHECK(err == ERR_INVALID_DATA);
	CHECK(s == u32str);

	CharString cs = (const char *)u8str;
	CHECK(String::utf8(cs) == s);
	ERR_PRINT_ON
}

TEST_CASE("[String] Invalid UTF16 (non-standard)") {
	ERR_PRINT_OFF
	static const char16_t u16str[] = { 0x0045, 0x304A, 0x3088, 0x3046, 0xDFA4, 0 };
	//                                 +       +       +       +       unpaired
	static const char32_t u32str[] = { 0x0045, 0x304A, 0x3088, 0x3046, 0xDFA4, 0 };
	String s;
	Error err = s.append_utf16(u16str);
	CHECK(err == ERR_PARSE_ERROR);
	CHECK(s == u32str);

	Char16String cs = u16str;
	CHECK(String::utf16(cs) == s);
	ERR_PRINT_ON
}

TEST_CASE("[String] ASCII") {
	String s = U"Primero Leche";
	String t = s.ascii(false).get_data();
	CHECK(s == t);

	t = s.ascii(true).get_data();
	CHECK(s == t);
}

TEST_CASE("[String] Comparisons (equal)") {
	String s = "Test Compare";
	CHECK(s == "Test Compare");
	CHECK(s == U"Test Compare");
	CHECK(s == L"Test Compare");
	CHECK(s == String("Test Compare"));

	CharString empty = "";
	CharString cs = "Test Compare";
	CHECK(!(empty == cs));
	CHECK(!(cs == empty));
	CHECK(cs == CharString("Test Compare"));
}

TEST_CASE("[String] Comparisons (not equal)") {
	String s = "Test Compare";
	CHECK(s != "Peanut");
	CHECK(s != U"Coconut");
	CHECK(s != L"Coconut");
	CHECK(s != String("Butter"));
}

TEST_CASE("[String] Comparisons (operator <)") {
	String s = "Bees";
	CHECK(s < "Elephant");
	CHECK(!(s < U"Amber"));
	CHECK(!(s < L"Amber"));
	CHECK(!(s < String("Beatrix")));
}

TEST_CASE("[String] Concatenation") {
	String s;

	s += "Have";
	s += ' ';
	s += 'a';
	s += String(" ");
	s = s + U"Nice";
	s = s + " ";
	s = s + String("Day");

	CHECK(s == "Have a Nice Day");
}

TEST_CASE("[String] Testing size and length of string") {
	// todo: expand this test to do more tests on size() as it is complicated under the hood.
	CHECK(String("Mellon").size() == 7);
	CHECK(String("Mellon1").size() == 8);

	// length works fine and is easier to test
	CHECK(String("Mellon").length() == 6);
	CHECK(String("Mellon1").length() == 7);
	CHECK(String("Mellon2").length() == 7);
	CHECK(String("Mellon3").length() == 7);
}

TEST_CASE("[String] Testing for empty string") {
	CHECK(!String("Mellon").is_empty());
	// do this more than once, to check for string corruption
	CHECK(String("").is_empty());
	CHECK(String("").is_empty());
	CHECK(String("").is_empty());
}

TEST_CASE("[String] Contains") {
	String s = "C:\\Godot\\project\\string_test.tscn";
	CHECK(s.contains(":\\"));
	CHECK(s.contains("Godot"));
	CHECK(s.contains(String("project\\string_test")));
	CHECK(s.contains(String("\\string_test.tscn")));

	CHECK(!s.contains("://"));
	CHECK(!s.contains("Godoh"));
	CHECK(!s.contains(String("project\\string test")));
	CHECK(!s.contains(String("\\char_test.tscn")));
}

TEST_CASE("[String] Contains case insensitive") {
	String s = "C:\\Godot\\project\\string_test.tscn";
	CHECK(s.containsn("Godot"));
	CHECK(s.containsn("godot"));
	CHECK(s.containsn(String("Project\\string_test")));
	CHECK(s.containsn(String("\\string_Test.tscn")));

	CHECK(!s.containsn("Godoh"));
	CHECK(!s.containsn("godoh"));
	CHECK(!s.containsn(String("project\\string test")));
	CHECK(!s.containsn(String("\\char_test.tscn")));
}

TEST_CASE("[String] Test chr") {
	CHECK(String::chr('H') == "H");
	CHECK(String::chr(0x3012)[0] == 0x3012);
	ERR_PRINT_OFF
	CHECK(String::chr(0xd812)[0] == 0xfffd); // Unpaired UTF-16 surrogate
	CHECK(String::chr(0x20d812)[0] == 0xfffd); // Outside UTF-32 range
	ERR_PRINT_ON
}

TEST_CASE("[String] Operator []") {
	String a = "Kugar Sane";
	a[0] = 'S';
	a[6] = 'C';
	CHECK(a == "Sugar Cane");
	CHECK(a[1] == 'u');
	CHECK(a.unicode_at(1) == 'u');
}

TEST_CASE("[String] Case function test") {
	String a = "MoMoNgA";

	CHECK(a.to_upper() == "MOMONGA");
	CHECK(a.to_lower() == "momonga");
}

TEST_CASE("[String] Case compare function test") {
	String a = "MoMoNgA";

	CHECK(a.casecmp_to("momonga") != 0);
	CHECK(a.nocasecmp_to("momonga") == 0);
}

TEST_CASE("[String] Natural compare function test") {
	String a = "img2.png";

	CHECK(a.nocasecmp_to("img10.png") > 0);
	CHECK(a.naturalnocasecmp_to("img10.png") < 0);
}

TEST_CASE("[String] File compare function test") {
	String a = "_img2.png";
	String b = "img2.png";

	CHECK(a.nocasecmp_to("img10.png") > 0);
	CHECK_MESSAGE(a.filenocasecmp_to("img10.png") < 0, "Should sort before letters.");
	CHECK_MESSAGE(a.filenocasecmp_to(".img10.png") > 0, "Should sort after period.");
	CHECK(b.filenocasecmp_to("img3.png") < 0);
}

TEST_CASE("[String] hex_encode_buffer") {
	static const uint8_t u8str[] = { 0x45, 0xE3, 0x81, 0x8A, 0x8F, 0xE3 };
	String s = String::hex_encode_buffer(u8str, 6);
	CHECK(s == U"45e3818a8fe3");
}

TEST_CASE("[String] Substr") {
	String s = "Killer Baby";
	CHECK(s.substr(3, 4) == "ler ");
	CHECK(s.substr(3) == "ler Baby");
}

TEST_CASE("[String] Find") {
	String s = "Pretty Woman Woman";
	MULTICHECK_STRING_EQ(s, find, "tty", 3);
	MULTICHECK_STRING_EQ(s, find, "Revenge of the Monster Truck", -1);
	MULTICHECK_STRING_INT_EQ(s, find, "Wo", 9, 13);
	MULTICHECK_STRING_INT_EQ(s, find, "Wo", 1000, -1);
	MULTICHECK_STRING_INT_EQ(s, find, "Wo", -1, -1);
	MULTICHECK_STRING_EQ(s, find, "", -1);
	MULTICHECK_STRING_EQ(s, find, "Pretty Woman Woman", 0);
	MULTICHECK_STRING_EQ(s, find, "WOMAN", -1);
	MULTICHECK_STRING_INT_EQ(s, find, "", 9, -1);

	MULTICHECK_STRING_EQ(s, rfind, "", -1);
	MULTICHECK_STRING_EQ(s, rfind, "foo", -1);
	MULTICHECK_STRING_EQ(s, rfind, "Pretty Woman Woman", 0);
	MULTICHECK_STRING_EQ(s, rfind, "man", 15);
	MULTICHECK_STRING_EQ(s, rfind, "WOMAN", -1);
	MULTICHECK_STRING_INT_EQ(s, rfind, "", 15, -1);
	MULTICHECK_STRING_INT_EQ(s, rfind, "Wo", 1000, -1);
	MULTICHECK_STRING_INT_EQ(s, rfind, "Wo", -1, 13);
}

TEST_CASE("[String] Find character") {
	String s = "racecar";
	CHECK_EQ(s.find_char('r'), 0);
	CHECK_EQ(s.find_char('r', 1), 6);
	CHECK_EQ(s.find_char('e'), 3);
	CHECK_EQ(s.find_char('e', 4), -1);

	CHECK_EQ(s.rfind_char('r'), 6);
	CHECK_EQ(s.rfind_char('r', 5), 0);
	CHECK_EQ(s.rfind_char('e'), 3);
	CHECK_EQ(s.rfind_char('e', 2), -1);
}

TEST_CASE("[String] Find case insensitive") {
	String s = "Pretty Whale Whale";
	MULTICHECK_STRING_EQ(s, findn, "WHA", 7);
	MULTICHECK_STRING_INT_EQ(s, findn, "WHA", 9, 13);
	MULTICHECK_STRING_INT_EQ(s, findn, "WHA", 1000, -1);
	MULTICHECK_STRING_INT_EQ(s, findn, "WHA", -1, -1);
	MULTICHECK_STRING_EQ(s, findn, "Revenge of the Monster SawFish", -1);
	MULTICHECK_STRING_EQ(s, findn, "", -1);
	MULTICHECK_STRING_EQ(s, findn, "wha", 7);
	MULTICHECK_STRING_EQ(s, findn, "Wha", 7);
	MULTICHECK_STRING_INT_EQ(s, findn, "", 3, -1);

	MULTICHECK_STRING_EQ(s, rfindn, "WHA", 13);
	MULTICHECK_STRING_EQ(s, rfindn, "", -1);
	MULTICHECK_STRING_EQ(s, rfindn, "wha", 13);
	MULTICHECK_STRING_EQ(s, rfindn, "Wha", 13);
	MULTICHECK_STRING_INT_EQ(s, rfindn, "", 13, -1);
	MULTICHECK_STRING_INT_EQ(s, rfindn, "WHA", 1000, -1);
	MULTICHECK_STRING_INT_EQ(s, rfindn, "WHA", -1, 13);
}

TEST_CASE("[String] Find MK") {
	Vector<String> keys;
	keys.push_back("sty");
	keys.push_back("tty");
	keys.push_back("man");

	String s = "Pretty Woman";
	int key = 0;

	CHECK(s.findmk(keys, 0, &key) == 3);
	CHECK(key == 1);

	CHECK(s.findmk(keys, 5, &key) == 9);
	CHECK(key == 2);

	CHECK(s.findmk(keys, -1, &key) == -1);
	CHECK(s.findmk(keys, 1000, &key) == -1);
}

TEST_CASE("[String] Find and replace") {
	String s = "Happy Birthday, Anna!";
	MULTICHECK_STRING_STRING_EQ(s, replace, "Birthday", "Halloween", "Happy Halloween, Anna!");
	MULTICHECK_STRING_STRING_EQ(s, replace_first, "y", "Y", "HappY Birthday, Anna!");
	MULTICHECK_STRING_STRING_EQ(s, replacen, "Y", "Y", "HappY BirthdaY, Anna!");
}

TEST_CASE("[String] replace_char") {
	String s = "Banana";
	CHECK(s.replace_char('n', 'x') == "Baxaxa");
	CHECK(s.replace_char('\0', 'x') == "Banana");
	ERR_PRINT_OFF
	CHECK(s.replace_char('n', '\0') == "Banana");
	ERR_PRINT_ON
}

TEST_CASE("[String] replace_chars") {
	String s = "Banana";
	CHECK(s.replace_chars(String("Bn"), 'x') == "xaxaxa");
	CHECK(s.replace_chars("Bn", 'x') == "xaxaxa");
	CHECK(s.replace_chars(String(), 'x') == "Banana");
	CHECK(s.replace_chars("", 'x') == "Banana");
	ERR_PRINT_OFF
	CHECK(s.replace_chars(String("Bn"), '\0') == "Banana");
	CHECK(s.replace_chars("Bn", '\0') == "Banana");
	ERR_PRINT_ON
}

TEST_CASE("[String] Insertion") {
	String s = "Who is Frederic?";
	s = s.insert(s.find("?"), " Chopin");
	CHECK(s == "Who is Frederic Chopin?");

	s = "foobar";
	CHECK(s.insert(0, "X") == "Xfoobar");
	CHECK(s.insert(-100, "X") == "foobar");
	CHECK(s.insert(6, "X") == "foobarX");
	CHECK(s.insert(100, "X") == "foobarX");
	CHECK(s.insert(2, "") == "foobar");

	s = "";
	CHECK(s.insert(0, "abc") == "abc");
	CHECK(s.insert(100, "abc") == "abc");
	CHECK(s.insert(-100, "abc") == "");
	CHECK(s.insert(0, "") == "");
}

TEST_CASE("[String] Erasing") {
	String s = "Josephine is such a cute girl!";
	s = s.erase(s.find("cute "), String("cute ").length());
	CHECK(s == "Josephine is such a girl!");
}

TEST_CASE("[String] remove_char") {
	String s = "Banana";
	CHECK(s.remove_char('a') == "Bnn");
	CHECK(s.remove_char('\0') == "Banana");
	CHECK(s.remove_char('x') == "Banana");
}

TEST_CASE("[String] remove_chars") {
	String s = "Banana";
	CHECK(s.remove_chars("Ba") == "nn");
	CHECK(s.remove_chars(String("Ba")) == "nn");
	CHECK(s.remove_chars("") == "Banana");
	CHECK(s.remove_chars(String()) == "Banana");
	CHECK(s.remove_chars("xy") == "Banana");
	CHECK(s.remove_chars(String("xy")) == "Banana");
}

TEST_CASE("[String] Number to string") {
	CHECK(String::num(0) == "0.0"); // The method takes double, so always add zeros.
	CHECK(String::num(0.0) == "0.0");
	CHECK(String::num(-0.0) == "-0.0"); // Includes sign even for zero.
	CHECK(String::num(3.141593) == "3.141593");
	CHECK(String::num(3.141593, 3) == "3.142");
	CHECK(String::num(42.100023, 4) == "42.1"); // No trailing zeros.

	// String::num_int64 tests.
	CHECK(String::num_int64(3141593) == "3141593");
	CHECK(String::num_int64(-3141593) == "-3141593");
	CHECK(String::num_int64(0xA141593, 16) == "a141593");
	CHECK(String::num_int64(0xA141593, 16, true) == "A141593");
	ERR_PRINT_OFF;
	CHECK(String::num_int64(3141593, 1) == ""); // Invalid base < 2.
	CHECK(String::num_int64(3141593, 37) == ""); // Invalid base > 36.
	ERR_PRINT_ON;

	// String::num_uint64 tests.
	CHECK(String::num_uint64(4294967295) == "4294967295");
	CHECK(String::num_uint64(0xF141593, 16) == "f141593");
	CHECK(String::num_uint64(0xF141593, 16, true) == "F141593");
	ERR_PRINT_OFF;
	CHECK(String::num_uint64(4294967295, 1) == ""); // Invalid base < 2.
	CHECK(String::num_uint64(4294967295, 37) == ""); // Invalid base > 36.
	ERR_PRINT_ON;

	// String::num_scientific tests.
	CHECK(String::num_scientific(30000000.0) == "30000000");
	CHECK(String::num_scientific(1234567890.0) == "1234567890");
	CHECK(String::num_scientific(3e100) == "3e+100");
	CHECK(String::num_scientific(7e-100) == "7e-100");
	CHECK(String::num_scientific(Math::TAU) == "6.283185307179586");
	CHECK(String::num_scientific(Math::INF) == "inf");
	CHECK(String::num_scientific(-Math::INF) == "-inf");
	CHECK(String::num_scientific(Math::NaN) == "nan");
	CHECK(String::num_scientific(2.0) == "2");
	CHECK(String::num_scientific(1.0) == "1");
	CHECK(String::num_scientific(0.0) == "0");
	CHECK(String::num_scientific(-0.0) == "-0");

	// String::num_real tests.
	CHECK(String::num_real(1.0) == "1.0");
	CHECK(String::num_real(1.0, false) == "1");
	CHECK(String::num_real(9.9) == "9.9");
	CHECK(String::num_real(9.99) == "9.99");
	CHECK(String::num_real(9.999) == "9.999");
	CHECK(String::num_real(9.9999) == "9.9999");
	CHECK(String::num_real(3.141593) == "3.141593");
	CHECK(String::num_real(3.141) == "3.141"); // No trailing zeros.
#ifdef REAL_T_IS_DOUBLE
	CHECK_MESSAGE(String::num_real(real_t(123.456789)) == "123.456789", "Prints the appropriate amount of digits for real_t = double.");
	CHECK_MESSAGE(String::num_real(real_t(-123.456789)) == "-123.456789", "Prints the appropriate amount of digits for real_t = double.");
	CHECK_MESSAGE(String::num_real(real_t(Math::PI)) == "3.14159265358979", "Prints the appropriate amount of digits for real_t = double.");
	CHECK_MESSAGE(String::num_real(real_t(3.1415f)) == "3.1414999961853", "Prints more digits of 32-bit float when real_t = double (ones that would be reliable for double) and no trailing zero.");
#else
	CHECK_MESSAGE(String::num_real(real_t(123.456789)) == "123.4568", "Prints the appropriate amount of digits for real_t = float.");
	CHECK_MESSAGE(String::num_real(real_t(-123.456789)) == "-123.4568", "Prints the appropriate amount of digits for real_t = float.");
	CHECK_MESSAGE(String::num_real(real_t(Math::PI)) == "3.141593", "Prints the appropriate amount of digits for real_t = float.");
	CHECK_MESSAGE(String::num_real(real_t(3.1415f)) == "3.1415", "Prints only reliable digits of 32-bit float when real_t = float.");
#endif // REAL_T_IS_DOUBLE

	// Checks doubles with many decimal places.
	CHECK(String::num(0.0000012345432123454321, -1) == "0.00000123454321"); // -1 uses 14 as sane default.
	CHECK(String::num(0.0000012345432123454321) == "0.00000123454321"); // -1 is the default value.
	CHECK(String::num(-0.0000012345432123454321) == "-0.00000123454321");
	CHECK(String::num(-10000.0000012345432123454321) == "-10000.0000012345");
	CHECK(String::num(0.0000000000012345432123454321) == "0.00000000000123");
	CHECK(String::num(0.0000000000012345432123454321, 3) == "0.0");

	// Note: When relevant (remainder > 0.5), the last digit gets rounded up,
	// which can also lead to not include a trailing zero, e.g. "...89" -> "...9".
	CHECK(String::num(0.0000056789876567898765) == "0.00000567898766"); // Should round last digit.
	CHECK(String::num(10000.000005678999999999) == "10000.000005679"); // We cut at ...789|99 which is rounded to ...79, so only 13 decimals.
	CHECK(String::num(42.12999999, 6) == "42.13"); // Also happens with lower decimals count.

	// 32 is MAX_DECIMALS. We can't reliably store that many so we can't compare against a string,
	// but we can check that the string length is 34 (32 + 2 for "0.").
	CHECK(String::num(0.00000123456789987654321123456789987654321, 32).length() == 34);
	CHECK(String::num(0.00000123456789987654321123456789987654321, 42).length() == 34); // Should enforce MAX_DECIMALS.
	CHECK(String::num(10000.00000123456789987654321123456789987654321, 42).length() == 38); // 32 decimals + "10000.".
}

TEST_CASE("[String] String to integer") {
	static const char *nums[14] = { "1237461283", "- 22", "0", " - 1123412", "", "10_000_000", "-1_2_3_4", "10__000", "  1  2  34 ", "-0", "007", "--45", "---46", "-7-2" };
	static const int num[14] = { 1237461283, -22, 0, -1123412, 0, 10000000, -1234, 10000, 1234, 0, 7, 45, -46, -72 };

	for (int i = 0; i < 14; i++) {
		CHECK(String(nums[i]).to_int() == num[i]);
	}
	CHECK(String("0b1011").to_int() == 1011); // Looks like a binary number, but to_int() handles this as a base-10 number, "b" is just ignored.
	CHECK(String("0B1011").to_int() == 1011);

	CHECK(String("0x1012").to_int() == 1012); // Looks like a hexadecimal number, but to_int() handles this as a base-10 number, "x" is just ignored.
	CHECK(String("0X1012").to_int() == 1012);

	ERR_PRINT_OFF
	CHECK(String("999999999999999999999999999999999999999999999999999999999").to_int() == INT64_MAX); // Too large, largest possible is returned.
	CHECK(String("-999999999999999999999999999999999999999999999999999999999").to_int() == INT64_MIN); // Too small, smallest possible is returned.
	ERR_PRINT_ON
}

TEST_CASE("[String] Hex to integer") {
	static const char *nums[13] = { "0xFFAE", "22", "0", "AADDAD", "0x7FFFFFFFFFFFFFFF", "-0xf", "", "000", "000f", "0xaA", "-ff", "-", "0XFFAE" };
	static const int64_t num[13] = { 0xFFAE, 0x22, 0, 0xAADDAD, 0x7FFFFFFFFFFFFFFF, -0xf, 0, 0, 0xf, 0xaa, -0xff, 0x0, 0xFFAE };

	for (int i = 0; i < 13; i++) {
		CHECK(String(nums[i]).hex_to_int() == num[i]);
	}

	// Invalid hex strings should return 0.
	static const char *invalid_nums[15] = { "qwerty", "QWERTY", "0xqwerty", "0x00qwerty", "qwerty00", "0x", "0x__", "__", "x12", "+", " ff", "ff ", "f f", "+ff", "--0x78" };

	ERR_PRINT_OFF
	for (int i = 0; i < 15; i++) {
		CHECK(String(invalid_nums[i]).hex_to_int() == 0);
	}

	CHECK(String("0xFFFFFFFFFFFFFFFFFFFFFFF").hex_to_int() == INT64_MAX); // Too large, largest possible is returned.
	CHECK(String("-0xFFFFFFFFFFFFFFFFFFFFFFF").hex_to_int() == INT64_MIN); // Too small, smallest possible is returned.
	ERR_PRINT_ON
}

TEST_CASE("[String] Bin to integer") {
	static const char *nums[11] = { "", "0", "0b0", "0b1", "0b", "1", "0b1010", "-0b11", "-1010", "0b0111111111111111111111111111111111111111111111111111111111111111", "0B1010" };
	static const int64_t num[11] = { 0, 0, 0, 1, 0, 1, 10, -3, -10, 0x7FFFFFFFFFFFFFFF, 10 };

	for (int i = 0; i < 11; i++) {
		CHECK(String(nums[i]).bin_to_int() == num[i]);
	}

	// Invalid bin strings should return 0. The long "0x11...11" is just too long for a 64 bit int.
	static const char *invalid_nums[16] = { "qwerty", "QWERTY", "0bqwerty", "0b00qwerty", "qwerty00", "0x__", "0b__", "__", "b12", "+", "-", "0x12ab", " 11", "11 ", "1 1", "--0b11" };

	for (int i = 0; i < 16; i++) {
		CHECK(String(invalid_nums[i]).bin_to_int() == 0);
	}

	ERR_PRINT_OFF
	CHECK(String("0b111111111111111111111111111111111111111111111111111111111111111111111111111111111").bin_to_int() == INT64_MAX); // Too large, largest possible is returned.
	CHECK(String("-0b111111111111111111111111111111111111111111111111111111111111111111111111111111111").bin_to_int() == INT64_MIN); // Too small, smallest possible is returned.
	ERR_PRINT_ON
}

TEST_CASE("[String] String to float") {
	static const char *nums[12] = { "-12348298412.2", "0.05", "2.0002", " -0.0001", "0", "000", "123", "0.0", "000.000", "000.007", "234__", "3..14" };
	static const double num[12] = { -12348298412.2, 0.05, 2.0002, -0.0001, 0.0, 0.0, 123.0, 0.0, 0.0, 0.007, 234.0, 3.0 };

	for (int i = 0; i < 12; i++) {
		CHECK(!(Math::abs(String(nums[i]).to_float() - num[i]) > 0.00001));
	}

	// Invalid float strings should return 0.
	static const char *invalid_nums[6] = { "qwerty", "qwerty123", "0xffff", "0b1010", "--3.13", "__345" };

	for (int i = 0; i < 6; i++) {
		CHECK(String(invalid_nums[i]).to_float() == 0);
	}

	// Very large exponents.
	CHECK(String("1e308").to_float() == 1e308);
	CHECK(String("-1e308").to_float() == -1e308);

	// Exponent is so high that value is INFINITY/-INFINITY.
	CHECK(String("1e309").to_float() == Math::INF);
	CHECK(String("1e511").to_float() == Math::INF);
	CHECK(String("-1e309").to_float() == -Math::INF);
	CHECK(String("-1e511").to_float() == -Math::INF);

	// Exponent is so high that a warning message is printed. Value is INFINITY/-INFINITY.
	ERR_PRINT_OFF
	CHECK(String("1e512").to_float() == Math::INF);
	CHECK(String("-1e512").to_float() == -Math::INF);
	ERR_PRINT_ON
}

TEST_CASE("[String] Slicing") {
	String s = "Mars,Jupiter,Saturn,Uranus";
	const char *slices[4] = { "Mars", "Jupiter", "Saturn", "Uranus" };
	MULTICHECK_GET_SLICE(s, ",", slices);
}

TEST_CASE("[String] Begins with") {
	// Test cases for true:
	MULTICHECK_STRING_EQ(String("res://foobar"), begins_with, "res://", true);
	MULTICHECK_STRING_EQ(String("abc"), begins_with, "abc", true);
	MULTICHECK_STRING_EQ(String("abc"), begins_with, "", true);
	MULTICHECK_STRING_EQ(String(""), begins_with, "", true);

	// Test cases for false:
	MULTICHECK_STRING_EQ(String("res"), begins_with, "res://", false);
	MULTICHECK_STRING_EQ(String("abcdef"), begins_with, "foo", false);
	MULTICHECK_STRING_EQ(String("abc"), begins_with, "ax", false);
	MULTICHECK_STRING_EQ(String(""), begins_with, "abc", false);

	// Test "const char *" version also with nullptr.
	String s("foo");
	bool state = s.begins_with(nullptr) == false;
	CHECK_MESSAGE(state, "nullptr check failed");

	String empty("");
	state = empty.begins_with(nullptr) == false;
	CHECK_MESSAGE(state, "nullptr check with empty string failed");
}

TEST_CASE("[String] Ends with") {
	// Test cases for true:
	MULTICHECK_STRING_EQ(String("res://foobar"), ends_with, "foobar", true);
	MULTICHECK_STRING_EQ(String("abc"), ends_with, "abc", true);
	MULTICHECK_STRING_EQ(String("abc"), ends_with, "", true);
	MULTICHECK_STRING_EQ(String(""), ends_with, "", true);

	// Test cases for false:
	MULTICHECK_STRING_EQ(String("res"), ends_with, "res://", false);
	MULTICHECK_STRING_EQ(String("abcdef"), ends_with, "foo", false);
	MULTICHECK_STRING_EQ(String("abc"), ends_with, "ax", false);
	MULTICHECK_STRING_EQ(String(""), ends_with, "abc", false);

	// Test "const char *" version also with nullptr.
	String s("foo");
	bool state = s.ends_with(nullptr) == false;
	CHECK_MESSAGE(state, "nullptr check failed");

	String empty("");
	state = empty.ends_with(nullptr) == false;
	CHECK_MESSAGE(state, "nullptr check with empty string failed");
}

TEST_CASE("[String] Splitting") {
	{
		const String s = "Mars,Jupiter,Saturn,Uranus";

		const char *slices_l[3] = { "Mars", "Jupiter", "Saturn,Uranus" };
		MULTICHECK_SPLIT(s, split, ",", true, 2, slices_l, 3);

		const char *slices_r[3] = { "Mars,Jupiter", "Saturn", "Uranus" };
		MULTICHECK_SPLIT(s, rsplit, ",", true, 2, slices_r, 3);
	}

	{
		const String s = "test";
		const char *slices[4] = { "t", "e", "s", "t" };
		MULTICHECK_SPLIT(s, split, "", true, 0, slices, 4);
	}

	{
		const String s = "";
		const char *slices[1] = { "" };
		MULTICHECK_SPLIT(s, split, "", true, 0, slices, 1);
		MULTICHECK_SPLIT(s, split, "", false, 0, slices, 0);
	}

	{
		const String s = "Mars Jupiter Saturn Uranus";
		const char *slices[4] = { "Mars", "Jupiter", "Saturn", "Uranus" };
		Vector<String> l = s.split_spaces();
		for (int i = 0; i < l.size(); i++) {
			CHECK(l[i] == slices[i]);
		}
	}
	{
		const String s = "Mars Jupiter Saturn Uranus";
		const char *slices[2] = { "Mars", "Jupiter Saturn Uranus" };
		Vector<String> l = s.split_spaces(1);
		for (int i = 0; i < l.size(); i++) {
			CHECK(l[i] == slices[i]);
		}
	}

	{
		const String s = "1.2;2.3 4.5";
		const double slices[3] = { 1.2, 2.3, 4.5 };

		const Vector<double> d_arr = s.split_floats(";");
		CHECK(d_arr.size() == 2);
		for (int i = 0; i < d_arr.size(); i++) {
			CHECK(Math::abs(d_arr[i] - slices[i]) <= 0.00001);
		}

		const Vector<String> keys = { ";", " " };
		const Vector<float> f_arr = s.split_floats_mk(keys);
		CHECK(f_arr.size() == 3);
		for (int i = 0; i < f_arr.size(); i++) {
			CHECK(Math::abs(f_arr[i] - slices[i]) <= 0.00001);
		}
	}

	{
		const String s = " -2.0        5";
		const double slices[10] = { 0, -2, 0, 0, 0, 0, 0, 0, 0, 5 };

		const Vector<double> arr = s.split_floats(" ");
		CHECK(arr.size() == 10);
		for (int i = 0; i < arr.size(); i++) {
			CHECK(Math::abs(arr[i] - slices[i]) <= 0.00001);
		}

		const Vector<String> keys = { ";", " " };
		const Vector<float> mk = s.split_floats_mk(keys);
		CHECK(mk.size() == 10);
		for (int i = 0; i < mk.size(); i++) {
			CHECK(mk[i] == slices[i]);
		}
	}

	{
		const String s = "1;2 4";
		const int slices[3] = { 1, 2, 4 };

		const Vector<int> arr = s.split_ints(";");
		CHECK(arr.size() == 2);
		for (int i = 0; i < arr.size(); i++) {
			CHECK(arr[i] == slices[i]);
		}

		const Vector<String> keys = { ";", " " };
		const Vector<int> mk = s.split_ints_mk(keys);
		CHECK(mk.size() == 3);
		for (int i = 0; i < mk.size(); i++) {
			CHECK(mk[i] == slices[i]);
		}
	}
}

TEST_CASE("[String] format") {
	const String value_format = "red=\"$red\" green=\"$green\" blue=\"$blue\" alpha=\"$alpha\"";

	Dictionary value_dictionary;
	value_dictionary["red"] = 10;
	value_dictionary["green"] = 20;
	value_dictionary["blue"] = "bla";
	value_dictionary["alpha"] = 0.4;
	String value = value_format.format(value_dictionary, "$_");

	CHECK(value == "red=\"10\" green=\"20\" blue=\"bla\" alpha=\"0.4\"");
}

TEST_CASE("[String] sprintf") {
	String format, output;
	Array args;
	bool error;

	// %%
	format = "fish %% frog";
	args.clear();
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish % frog"));
	///// Ints

	// Int
	format = "fish %d frog";
	args.clear();
	args.push_back(5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 5 frog"));

	// Int left padded with zeroes.
	format = "fish %05d frog";
	args.clear();
	args.push_back(5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 00005 frog"));

	// Int left padded with spaces.
	format = "fish %5d frog";
	args.clear();
	args.push_back(5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish     5 frog"));

	// Int right padded with spaces.
	format = "fish %-5d frog";
	args.clear();
	args.push_back(5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 5     frog"));

	// Int with sign (positive).
	format = "fish %+d frog";
	args.clear();
	args.push_back(5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish +5 frog"));

	// Negative int.
	format = "fish %d frog";
	args.clear();
	args.push_back(-5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -5 frog"));

	// Negative int left padded with spaces.
	format = "fish %5d frog";
	args.clear();
	args.push_back(-5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish    -5 frog"));

	// Negative int left padded with zeros.
	format = "fish %05d frog";
	args.clear();
	args.push_back(-5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -0005 frog"));

	// Negative int right padded with spaces.
	format = "fish %-5d frog";
	args.clear();
	args.push_back(-5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -5    frog"));

	// Negative int right padded with zeros. (0 ignored)
	format = "fish %-05d frog";
	args.clear();
	args.push_back(-5);
	ERR_PRINT_OFF; // Silence warning about 0 ignored.
	output = format.sprintf(args, &error);
	ERR_PRINT_ON;
	REQUIRE(error == false);
	CHECK(output == String("fish -5    frog"));

	// Hex (lower)
	format = "fish %x frog";
	args.clear();
	args.push_back(45);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 2d frog"));

	// Hex (upper)
	format = "fish %X frog";
	args.clear();
	args.push_back(45);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 2D frog"));

	// Octal
	format = "fish %o frog";
	args.clear();
	args.push_back(99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 143 frog"));

	// INT64_MIN
	format = "fish %d frog";
	args.clear();
	args.push_back(INT64_MIN);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -9223372036854775808 frog"));

	// INT64_MIN hex (lower)
	format = "fish %x frog";
	args.clear();
	args.push_back(INT64_MIN);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -8000000000000000 frog"));

	// INT64_MIN hex (upper)
	format = "fish %X frog";
	args.clear();
	args.push_back(INT64_MIN);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -8000000000000000 frog"));

	// INT64_MIN octal
	format = "fish %o frog";
	args.clear();
	args.push_back(INT64_MIN);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -1000000000000000000000 frog"));

	///// Reals

	// Real
	format = "fish %f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 99.990000 frog"));

	// Real left-padded.
	format = "fish %11f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish   99.990000 frog"));

	// Real (infinity) left-padded
	format = "fish %11f frog";
	args.clear();
	args.push_back(Math::INF);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish         inf frog"));

	// Real right-padded.
	format = "fish %-11f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 99.990000   frog"));

	// Real given int.
	format = "fish %f frog";
	args.clear();
	args.push_back(99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 99.000000 frog"));

	// Real with sign (positive).
	format = "fish %+f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish +99.990000 frog"));

	// Real with sign (negative zero).
	format = "fish %+f frog";
	args.clear();
	args.push_back(-0.0);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish -0.000000 frog"));

	// Real with sign (positive zero).
	format = "fish %+f frog";
	args.clear();
	args.push_back(0.0);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish +0.000000 frog"));

	// Real with 1 decimal.
	format = "fish %.1f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 100.0 frog"));

	// Real with 12 decimals.
	format = "fish %.12f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 99.990000000000 frog"));

	// Real with no decimals.
	format = "fish %.f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 100 frog"));

	// Negative real right padded with zeros. (0 ignored)
	format = "fish %-011f frog";
	args.clear();
	args.push_back(-99.99);
	ERR_PRINT_OFF; // Silence warning about 0 ignored.
	output = format.sprintf(args, &error);
	ERR_PRINT_ON;
	REQUIRE(error == false);
	CHECK(output == String("fish -99.990000  frog"));

	///// Vectors

	// Vector2
	format = "fish %v frog";
	args.clear();
	args.push_back(Variant(Vector2(19.99, 1.00)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (19.990000, 1.000000) frog"));

	// Vector3
	format = "fish %v frog";
	args.clear();
	args.push_back(Variant(Vector3(19.99, 1.00, -2.05)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (19.990000, 1.000000, -2.050000) frog"));

	// Vector4
	format = "fish %v frog";
	args.clear();
	args.push_back(Variant(Vector4(19.99, 1.00, -2.05, 5.5)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (19.990000, 1.000000, -2.050000, 5.500000) frog"));

	// Vector with negative values.
	format = "fish %v frog";
	args.clear();
	args.push_back(Variant(Vector2(-19.99, -1.00)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (-19.990000, -1.000000) frog"));

	// Vector left-padded.
	format = "fish %11v frog";
	args.clear();
	args.push_back(Variant(Vector3(19.99, 1.00, -2.05)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (  19.990000,    1.000000,   -2.050000) frog"));

	// Vector left-padded with inf/nan
	format = "fish %11v frog";
	args.clear();
	args.push_back(Variant(Vector2(Math::INF, Math::NaN)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (        inf,         nan) frog"));

	// Vector right-padded.
	format = "fish %-11v frog";
	args.clear();
	args.push_back(Variant(Vector3(19.99, 1.00, -2.05)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (19.990000  , 1.000000   , -2.050000  ) frog"));

	// Vector left-padded with zeros.
	format = "fish %011v frog";
	args.clear();
	args.push_back(Variant(Vector3(19.99, 1.00, -2.05)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (0019.990000, 0001.000000, -002.050000) frog"));

	// Vector given Vector3i.
	format = "fish %v frog";
	args.clear();
	args.push_back(Variant(Vector3i(19, 1, -2)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (19.000000, 1.000000, -2.000000) frog"));

	// Vector with 1 decimal.
	format = "fish %.1v frog";
	args.clear();
	args.push_back(Variant(Vector3(19.99, 1.00, -2.05)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (20.0, 1.0, -2.0) frog"));

	// Vector with 12 decimals.
	format = "fish %.12v frog";
	args.clear();
	args.push_back(Variant(Vector3(19.00, 1.00, -2.00)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (19.000000000000, 1.000000000000, -2.000000000000) frog"));

	// Vector with no decimals.
	format = "fish %.v frog";
	args.clear();
	args.push_back(Variant(Vector3(19.99, 1.00, -2.05)));
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish (20, 1, -2) frog"));

	///// Strings

	// String
	format = "fish %s frog";
	args.clear();
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish cheese frog"));

	// String left-padded.
	format = "fish %10s frog";
	args.clear();
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish     cheese frog"));

	// String right-padded.
	format = "fish %-10s frog";
	args.clear();
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish cheese     frog"));

	///// Characters

	// Character as string.
	format = "fish %c frog";
	args.clear();
	args.push_back("A");
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish A frog"));

	// Character as int.
	format = "fish %c frog";
	args.clear();
	args.push_back(65);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish A frog"));

	///// Dynamic width

	// String dynamic width.
	format = "fish %*s frog";
	args.clear();
	args.push_back(10);
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	REQUIRE(output == String("fish     cheese frog"));

	// Int dynamic width.
	format = "fish %*d frog";
	args.clear();
	args.push_back(10);
	args.push_back(99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	REQUIRE(output == String("fish         99 frog"));

	// Float dynamic width.
	format = "fish %*.*f frog";
	args.clear();
	args.push_back(10);
	args.push_back(3);
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish     99.990 frog"));

	///// Argument indices.
	format = "fish %2$d frog %1$s xx";
	args.clear();
	args.push_back("test");
	args.push_back(5);
	output = format.sprintf(args, &error);
	REQUIRE(error == false);
	CHECK(output == String("fish 5 frog test xx"));

	///// Errors

	// More formats than arguments.
	format = "fish %s %s frog";
	args.clear();
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "not enough arguments for format string");

	// More arguments than formats.
	format = "fish %s frog";
	args.clear();
	args.push_back("hello");
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "not all arguments converted during string formatting");

	// Incomplete format.
	format = "fish %10";
	args.clear();
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "incomplete format");

	// Bad character in format string.
	format = "fish %&f frog";
	args.clear();
	args.push_back("cheese");
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "unsupported format character");

	// Too many decimals.
	format = "fish %2.2.2f frog";
	args.clear();
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "too many decimal points in format");

	// * not a number or vector.
	format = "fish %*f frog";
	args.clear();
	args.push_back("cheese");
	args.push_back(99.99);
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "* wants number or vector");

	// Character too long.
	format = "fish %c frog";
	args.clear();
	args.push_back("sc");
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "%c requires number or single-character string");

	// Character bad type.
	format = "fish %c frog";
	args.clear();
	args.push_back(Array());
	output = format.sprintf(args, &error);
	REQUIRE(error);
	CHECK(output == "%c requires number or single-character string");
}

TEST_CASE("[String] is_numeric") {
	CHECK(String("12").is_numeric());
	CHECK(String("1.2").is_numeric());
	CHECK(!String("AF").is_numeric());
	CHECK(String("-12").is_numeric());
	CHECK(String("-1.2").is_numeric());
}

TEST_CASE("[String] pad") {
	String s = String("test");
	CHECK(s.lpad(10, "x") == U"xxxxxxtest");
	CHECK(s.rpad(10, "x") == U"testxxxxxx");

	s = String("10.10");
	CHECK(s.pad_decimals(4) == U"10.1000");
	CHECK(s.pad_decimals(1) == U"10.1");
	CHECK(s.pad_zeros(4) == U"0010.10");
	CHECK(s.pad_zeros(1) == U"10.10");
}

TEST_CASE("[String] is_subsequence_of") {
	String a = "is subsequence of";
	CHECK(String("sub").is_subsequence_of(a));
	CHECK(!String("Sub").is_subsequence_of(a));
	CHECK(String("Sub").is_subsequence_ofn(a));
}

TEST_CASE("[String] is_lowercase") {
	CHECK(String("abcd1234 !@#$%^&*()_-=+,.<>/\\|[]{};':\"`~").is_lowercase());
	CHECK(String("").is_lowercase());
	CHECK(!String("abc_ABC").is_lowercase());
}

TEST_CASE("[String] match") {
	CHECK(String("img1.png").match("*.png"));
	CHECK(!String("img1.jpeg").match("*.png"));
	CHECK(!String("img1.Png").match("*.png"));
	CHECK(String("img1.Png").matchn("*.png"));
}

TEST_CASE("[String] IPVX address to string") {
	IPAddress ip0("2001:0db8:85a3:0000:0000:8a2e:0370:7334");
	CHECK(ip0.is_valid());

	IPAddress ip(0x0123, 0x4567, 0x89ab, 0xcdef, true);
	CHECK(ip.is_valid());

	IPAddress ip2("fe80::52e5:49ff:fe93:1baf");
	CHECK(ip2.is_valid());

	IPAddress ip3("::ffff:192.168.0.1");
	CHECK(ip3.is_valid());

	String ip4 = "192.168.0.1";
	CHECK(ip4.is_valid_ip_address());

	ip4 = "192.368.0.1";
	CHECK(!ip4.is_valid_ip_address());

	String ip6 = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
	CHECK(ip6.is_valid_ip_address());

	ip6 = "2001:0db8:85j3:0000:0000:8a2e:0370:7334";
	CHECK(!ip6.is_valid_ip_address());

	ip6 = "2001:0db8:85f345:0000:0000:8a2e:0370:7334";
	CHECK(!ip6.is_valid_ip_address());

	ip6 = "2001:0db8::0:8a2e:370:7334";
	CHECK(ip6.is_valid_ip_address());

	ip6 = "::ffff:192.168.0.1";
	CHECK(ip6.is_valid_ip_address());
}

TEST_CASE("[String] Capitalize against many strings") {
	String input = "2D";
	String output = "2d";
	CHECK(input.capitalize() == output);

	input = "2d";
	output = "2d";
	CHECK(input.capitalize() == output);

	input = "2db";
	output = "2 Db";
	CHECK(input.capitalize() == output);

	input = "HTML5 Html5 html5 html_5";
	output = "Html 5 Html 5 Html 5 Html 5";
	CHECK(input.capitalize() == output);

	input = "Node2D Node2d NODE2D NODE_2D node_2d";
	output = "Node 2d Node 2d Node 2d Node 2d Node 2d";
	CHECK(input.capitalize() == output);

	input = "Node2DPosition";
	output = "Node 2d Position";
	CHECK(input.capitalize() == output);

	input = "Number2Digits";
	output = "Number 2 Digits";
	CHECK(input.capitalize() == output);

	input = "bytes2var";
	output = "Bytes 2 Var";
	CHECK(input.capitalize() == output);

	input = "linear2db";
	output = "Linear 2 Db";
	CHECK(input.capitalize() == output);

	input = "vector3";
	output = "Vector 3";
	CHECK(input.capitalize() == output);

	input = "sha256";
	output = "Sha 256";
	CHECK(input.capitalize() == output);

	input = "PascalCase";
	output = "Pascal Case";
	CHECK(input.capitalize() == output);

	input = "PascalPascalCase";
	output = "Pascal Pascal Case";
	CHECK(input.capitalize() == output);

	input = "snake_case";
	output = "Snake Case";
	CHECK(input.capitalize() == output);

	input = "snake_snake_case";
	output = "Snake Snake Case";
	CHECK(input.capitalize() == output);

	input = "kebab-case";
	output = "Kebab Case";
	CHECK(input.capitalize() == output);

	input = "kebab-kebab-case";
	output = "Kebab Kebab Case";
	CHECK(input.capitalize() == output);

	input = "sha256sum";
	output = "Sha 256 Sum";
	CHECK(input.capitalize() == output);

	input = "cat2dog";
	output = "Cat 2 Dog";
	CHECK(input.capitalize() == output);

	input = "function(name)";
	output = "Function(name)";
	CHECK(input.capitalize() == output);

	input = "snake_case_function(snake_case_arg)";
	output = "Snake Case Function(snake Case Arg)";
	CHECK(input.capitalize() == output);

	input = "snake_case_function( snake_case_arg )";
	output = "Snake Case Function( Snake Case Arg )";
	CHECK(input.capitalize() == output);

	input = "kebab-case-function( kebab-case-arg )";
	output = "Kebab Case Function( Kebab Case Arg )";
	CHECK(input.capitalize() == output);

	input = "kebab_case_function( kebab_case_arg )";
	output = "Kebab Case Function( Kebab Case Arg )";
	CHECK(input.capitalize() == output);

	input = U"словоСлово_слово слово";
	output = U"Слово Слово Слово Слово";
	CHECK(input.capitalize() == output);

	input = U"λέξηΛέξη_λέξη λέξη";
	output = U"Λέξη Λέξη Λέξη Λέξη";
	CHECK(input.capitalize() == output);

	input = U"բառԲառ_բառ բառ";
	output = U"Բառ Բառ Բառ Բառ";
	CHECK(input.capitalize() == output);
}

struct StringCasesTestCase {
	const char32_t *input;
	const char32_t *camel_case;
	const char32_t *pascal_case;
	const char32_t *snake_case;
	const char32_t *kebab_case;
};

TEST_CASE("[String] Checking case conversion methods") {
	StringCasesTestCase test_cases[] = {
		/* clang-format off */
		{ U"2D",                     U"2d",                   U"2d",                   U"2d",                      U"2d"                      },
		{ U"2d",                     U"2d",                   U"2d",                   U"2d",                      U"2d"                      },
		{ U"2db",                    U"2Db",                  U"2Db",                  U"2_db",                    U"2-db"                    },
		{ U"Vector3",                U"vector3",              U"Vector3",              U"vector_3",                U"vector-3"                },
		{ U"sha256",                 U"sha256",               U"Sha256",               U"sha_256",                 U"sha-256"                 },
		{ U"Node2D",                 U"node2d",               U"Node2d",               U"node_2d",                 U"node-2d"                 },
		{ U"RichTextLabel",          U"richTextLabel",        U"RichTextLabel",        U"rich_text_label",         U"rich-text-label"         },
		{ U"HTML5",                  U"html5",                U"Html5",                U"html_5",                  U"html-5"                  },
		{ U"Node2DPosition",         U"node2dPosition",       U"Node2dPosition",       U"node_2d_position",        U"node-2d-position"        },
		{ U"Number2Digits",          U"number2Digits",        U"Number2Digits",        U"number_2_digits",         U"number-2-digits"         },
		{ U"get_property_list",      U"getPropertyList",      U"GetPropertyList",      U"get_property_list",       U"get-property-list"       },
		{ U"get_camera_2d",          U"getCamera2d",          U"GetCamera2d",          U"get_camera_2d",           U"get-camera-2d"           },
		{ U"_physics_process",       U"physicsProcess",       U"PhysicsProcess",       U"_physics_process",        U"-physics-process"        },
		{ U"bytes2var",              U"bytes2Var",            U"Bytes2Var",            U"bytes_2_var",             U"bytes-2-var"             },
		{ U"linear2db",              U"linear2Db",            U"Linear2Db",            U"linear_2_db",             U"linear-2-db"             },
		{ U"sha256sum",              U"sha256Sum",            U"Sha256Sum",            U"sha_256_sum",             U"sha-256-sum"             },
		{ U"camelCase",              U"camelCase",            U"CamelCase",            U"camel_case",              U"camel-case"              },
		{ U"PascalCase",             U"pascalCase",           U"PascalCase",           U"pascal_case",             U"pascal-case"             },
		{ U"snake_case",             U"snakeCase",            U"SnakeCase",            U"snake_case",              U"snake-case"              },
		{ U"kebab-case",             U"kebabCase",            U"KebabCase",            U"kebab_case",              U"kebab-case"              },
		{ U"Test TEST test",         U"testTestTest",         U"TestTestTest",         U"test_test_test",          U"test-test-test"          },
		{ U"словоСлово_слово слово", U"словоСловоСловоСлово", U"СловоСловоСловоСлово", U"слово_слово_слово_слово", U"слово-слово-слово-слово" },
		{ U"λέξηΛέξη_λέξη λέξη",     U"λέξηΛέξηΛέξηΛέξη",     U"ΛέξηΛέξηΛέξηΛέξη",     U"λέξη_λέξη_λέξη_λέξη",     U"λέξη-λέξη-λέξη-λέξη"     },
		{ U"բառԲառ_բառ բառ",         U"բառԲառԲառԲառ",         U"ԲառԲառԲառԲառ",         U"բառ_բառ_բառ_բառ",         U"բառ-բառ-բառ-բառ"         },
		{ nullptr,                   nullptr,                 nullptr,                 nullptr,                    nullptr                    },
		/* clang-format on */
	};

	int idx = 0;
	while (test_cases[idx].input != nullptr) {
		String input = test_cases[idx].input;
		CHECK(input.to_camel_case() == test_cases[idx].camel_case);
		CHECK(input.to_pascal_case() == test_cases[idx].pascal_case);
		CHECK(input.to_snake_case() == test_cases[idx].snake_case);
		CHECK(input.to_kebab_case() == test_cases[idx].kebab_case);
		idx++;
	}
}

TEST_CASE("[String] Checking string is empty when it should be") {
	bool state = true;
	bool success;

	String a = "";
	success = a[0] == 0;
	if (!success) {
		state = false;
	}
	String b = "Godot";
	success = b[b.size()] == 0;
	if (!success) {
		state = false;
	}
	const String c = "";
	success = c[0] == 0;
	if (!success) {
		state = false;
	}

	const String d = "Godot";
	success = d[d.size()] == 0;
	if (!success) {
		state = false;
	}

	CHECK(state);
}

TEST_CASE("[String] lstrip and rstrip") {
#define STRIP_TEST(x) \
	{ \
		bool success = x; \
		state = state && success; \
	}

	bool state = true;

	// strip none
	STRIP_TEST(String("abc").lstrip("") == "abc");
	STRIP_TEST(String("abc").rstrip("") == "abc");
	// strip one
	STRIP_TEST(String("abc").lstrip("a") == "bc");
	STRIP_TEST(String("abc").rstrip("c") == "ab");
	// strip lots
	STRIP_TEST(String("bababbababccc").lstrip("ab") == "ccc");
	STRIP_TEST(String("aaabcbcbcbbcbbc").rstrip("cb") == "aaa");
	// strip empty string
	STRIP_TEST(String("").lstrip("") == "");
	STRIP_TEST(String("").rstrip("") == "");
	// strip to empty string
	STRIP_TEST(String("abcabcabc").lstrip("bca") == "");
	STRIP_TEST(String("abcabcabc").rstrip("bca") == "");
	// don't strip wrong end
	STRIP_TEST(String("abc").lstrip("c") == "abc");
	STRIP_TEST(String("abca").lstrip("a") == "bca");
	STRIP_TEST(String("abc").rstrip("a") == "abc");
	STRIP_TEST(String("abca").rstrip("a") == "abc");
	// in utf-8 "¿" (\u00bf) has the same first byte as "µ" (\u00b5)
	// and the same second as "ÿ" (\u00ff)
	STRIP_TEST(String::utf8("¿").lstrip(String::utf8("µÿ")) == String::utf8("¿"));
	STRIP_TEST(String::utf8("¿").rstrip(String::utf8("µÿ")) == String::utf8("¿"));
	STRIP_TEST(String::utf8("µ¿ÿ").lstrip(String::utf8("µÿ")) == String::utf8("¿ÿ"));
	STRIP_TEST(String::utf8("µ¿ÿ").rstrip(String::utf8("µÿ")) == String::utf8("µ¿"));

	// the above tests repeated with additional superfluous strip chars

	// strip none
	STRIP_TEST(String("abc").lstrip("qwjkl") == "abc");
	STRIP_TEST(String("abc").rstrip("qwjkl") == "abc");
	// strip one
	STRIP_TEST(String("abc").lstrip("qwajkl") == "bc");
	STRIP_TEST(String("abc").rstrip("qwcjkl") == "ab");
	// strip lots
	STRIP_TEST(String("bababbababccc").lstrip("qwabjkl") == "ccc");
	STRIP_TEST(String("aaabcbcbcbbcbbc").rstrip("qwcbjkl") == "aaa");
	// strip empty string
	STRIP_TEST(String("").lstrip("qwjkl") == "");
	STRIP_TEST(String("").rstrip("qwjkl") == "");
	// strip to empty string
	STRIP_TEST(String("abcabcabc").lstrip("qwbcajkl") == "");
	STRIP_TEST(String("abcabcabc").rstrip("qwbcajkl") == "");
	// don't strip wrong end
	STRIP_TEST(String("abc").lstrip("qwcjkl") == "abc");
	STRIP_TEST(String("abca").lstrip("qwajkl") == "bca");
	STRIP_TEST(String("abc").rstrip("qwajkl") == "abc");
	STRIP_TEST(String("abca").rstrip("qwajkl") == "abc");
	// in utf-8 "¿" (\u00bf) has the same first byte as "µ" (\u00b5)
	// and the same second as "ÿ" (\u00ff)
	STRIP_TEST(String::utf8("¿").lstrip(String::utf8("qwaµÿjkl")) == String::utf8("¿"));
	STRIP_TEST(String::utf8("¿").rstrip(String::utf8("qwaµÿjkl")) == String::utf8("¿"));
	STRIP_TEST(String::utf8("µ¿ÿ").lstrip(String::utf8("qwaµÿjkl")) == String::utf8("¿ÿ"));
	STRIP_TEST(String::utf8("µ¿ÿ").rstrip(String::utf8("qwaµÿjkl")) == String::utf8("µ¿"));

	CHECK(state);

#undef STRIP_TEST
}

TEST_CASE("[String] Ensuring empty string into extend_utf8 passes empty string") {
	String empty;
	CHECK(empty.append_utf8(nullptr, -1) == ERR_INVALID_DATA);
}

TEST_CASE("[String] Cyrillic to_lower()") {
	String upper = U"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ";
	String lower = U"абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

	String test = upper.to_lower();

	bool state = test == lower;

	CHECK(state);
}

TEST_CASE("[String] Count and countn functionality") {
	String s = String("");
	MULTICHECK_STRING_EQ(s, count, "Test", 0);

	s = "Test";
	MULTICHECK_STRING_EQ(s, count, "", 0);

	s = "Test";
	MULTICHECK_STRING_EQ(s, count, "test", 0);

	s = "Test";
	MULTICHECK_STRING_EQ(s, count, "TEST", 0);

	s = "TEST";
	MULTICHECK_STRING_EQ(s, count, "TEST", 1);

	s = "Test";
	MULTICHECK_STRING_EQ(s, count, "Test", 1);

	s = "aTest";
	MULTICHECK_STRING_EQ(s, count, "Test", 1);

	s = "Testa";
	MULTICHECK_STRING_EQ(s, count, "Test", 1);

	s = "TestTestTest";
	MULTICHECK_STRING_EQ(s, count, "Test", 3);

	s = "TestTestTest";
	MULTICHECK_STRING_EQ(s, count, "TestTest", 1);

	s = "TestGodotTestGodotTestGodot";
	MULTICHECK_STRING_EQ(s, count, "Test", 3);

	s = "TestTestTestTest";
	MULTICHECK_STRING_INT_INT_EQ(s, count, "Test", 4, 8, 1);

	s = "TestTestTestTest";
	MULTICHECK_STRING_INT_INT_EQ(s, count, "Test", 4, 12, 2);

	s = "TestTestTestTest";
	MULTICHECK_STRING_INT_INT_EQ(s, count, "Test", 4, 16, 3);

	s = "TestTestTestTest";
	MULTICHECK_STRING_INT_EQ(s, count, "Test", 4, 3);

	s = "Test";
	MULTICHECK_STRING_EQ(s, countn, "test", 1);

	s = "Test";
	MULTICHECK_STRING_EQ(s, countn, "TEST", 1);

	s = "testTest-Testatest";
	MULTICHECK_STRING_EQ(s, countn, "tEst", 4);

	s = "testTest-TeStatest";
	MULTICHECK_STRING_INT_INT_EQ(s, countn, "tEsT", 4, 16, 2);
}

TEST_CASE("[String] Bigrams") {
	String s = "abcd";
	Vector<String> bigr = s.bigrams();

	CHECK(bigr.size() == 3);
	CHECK(bigr[0] == "ab");
	CHECK(bigr[1] == "bc");
	CHECK(bigr[2] == "cd");
}

TEST_CASE("[String] c-escape/unescape") {
	String s = "\\1\a2\b\f3\n45\r6\t7\v8\'9\?0\"";
	CHECK(s.c_escape().c_unescape() == s);
}

TEST_CASE("[String] indent") {
	static const char *input[] = {
		"",
		"aaa\nbbb",
		"\tcontains\n\tindent",
		"empty\n\nline",
	};
	static const char *expected[] = {
		"",
		"\taaa\n\tbbb",
		"\t\tcontains\n\t\tindent",
		"\tempty\n\n\tline",
	};

	for (int i = 0; i < 3; i++) {
		CHECK(String(input[i]).indent("\t") == expected[i]);
	}
}

TEST_CASE("[String] dedent") {
	String s = "      aaa\n    bbb";
	String t = "aaa\nbbb";
	CHECK(s.dedent() == t);
}

TEST_CASE("[String] Path functions") {
	static const char *path[8] = { "C:\\Godot\\project\\test.tscn", "/Godot/project/test.xscn", "../Godot/project/test.scn", "Godot\\test.doc", "C:\\test.", "res://test", "user://test", "/.test" };
	static const char *base_dir[8] = { "C:\\Godot\\project", "/Godot/project", "../Godot/project", "Godot", "C:\\", "res://", "user://", "/" };
	static const char *base_name[8] = { "C:\\Godot\\project\\test", "/Godot/project/test", "../Godot/project/test", "Godot\\test", "C:\\test", "res://test", "user://test", "/" };
	static const char *ext[8] = { "tscn", "xscn", "scn", "doc", "", "", "", "test" };
	static const char *file[8] = { "test.tscn", "test.xscn", "test.scn", "test.doc", "test.", "test", "test", ".test" };
	static const char *simplified[8] = { "C:/Godot/project/test.tscn", "/Godot/project/test.xscn", "../Godot/project/test.scn", "Godot/test.doc", "C:/test.", "res://test", "user://test", "/.test" };
	static const bool abs[8] = { true, true, false, false, true, true, true, true };

	for (int i = 0; i < 8; i++) {
		CHECK(String(path[i]).get_base_dir() == base_dir[i]);
		CHECK(String(path[i]).get_basename() == base_name[i]);
		CHECK(String(path[i]).get_extension() == ext[i]);
		CHECK(String(path[i]).get_file() == file[i]);
		CHECK(String(path[i]).is_absolute_path() == abs[i]);
		CHECK(String(path[i]).is_relative_path() != abs[i]);
		CHECK(String(path[i]).simplify_path() == String(simplified[i]));
		CHECK(String(path[i]).simplify_path().get_base_dir().path_join(file[i]) == String(path[i]).simplify_path());
	}

	CHECK(String("res://test.png").has_extension("png"));
	CHECK(String("res://test.PNG").has_extension("png"));
	CHECK_FALSE(String("res://test.png").has_extension("jpg"));
	CHECK_FALSE(String("res://test.png/README").has_extension("png"));
	CHECK_FALSE(String("res://test.").has_extension("png"));
	CHECK_FALSE(String("res://test").has_extension("png"));

	static const char *file_name[3] = { "test.tscn", "test://.xscn", "?tes*t.scn" };
	static const bool valid[3] = { true, false, false };
	for (int i = 0; i < 3; i++) {
		CHECK(String(file_name[i]).is_valid_filename() == valid[i]);
	}

	CHECK(String("res://texture.png") == String("res://folder/../folder/../texture.png").simplify_path());
	CHECK(String("res://texture.png") == String("res://folder/sub/../../texture.png").simplify_path());
	CHECK(String("res://../../texture.png") == String("res://../../texture.png").simplify_path());
}

TEST_CASE("[String] hash") {
	String a = "Test";
	String b = "Test";
	String c = "West";
	CHECK(a.hash() == b.hash());
	CHECK(a.hash() != c.hash());

	CHECK(a.hash64() == b.hash64());
	CHECK(a.hash64() != c.hash64());
}

TEST_CASE("[String] uri_encode/unescape") {
	String s = "Godot Engine:'docs'";
	String t = "Godot%20Engine%3A%27docs%27";

	String x1 = "T%C4%93%C5%A1t";
	static const uint8_t u8str[] = { 0x54, 0xC4, 0x93, 0xC5, 0xA1, 0x74, 0x00 };
	String x2 = String::utf8((const char *)u8str);
	String x3 = U"Tēšt";
	String x4 = U"file+name";

	CHECK(x1.uri_decode() == x2);
	CHECK(x1.uri_decode() == x3);
	CHECK((x1 + x3).uri_decode() == (x2 + x3)); // Mixed unicode and URL encoded string, e.g. GTK+ bookmark.
	CHECK(x2.uri_encode() == x1);
	CHECK(x3.uri_encode() == x1);

	CHECK(s.uri_encode() == t);
	CHECK(t.uri_decode() == s);
	CHECK(x4.uri_file_decode() == x4);
	CHECK(x4.uri_decode() == U"file name");
}

TEST_CASE("[String] xml_escape/unescape") {
	String s = "\"Test\" <test@test&'test'>";
	CHECK(s.xml_escape(true).xml_unescape() == s);
	CHECK(s.xml_escape(false).xml_unescape() == s);
}

TEST_CASE("[String] xml_unescape") {
	// Named entities
	String input = "&quot;&amp;&apos;&lt;&gt;";
	CHECK(input.xml_unescape() == "\"&\'<>");

	// Numeric entities
	input = "&#x41;&#66;";
	CHECK(input.xml_unescape() == "AB");

	input = "&#0;&x#0;More text";
	String result = input.xml_unescape();
	// Didn't put in a leading NUL and terminate the string
	CHECK(input.length() > 0);
	CHECK(input[0] != '\0');
	// Entity should be left as-is if invalid
	CHECK(input.xml_unescape() == input);

	// Check near char32_t range
	input = "&#xFFFFFFFF;";
	result = input.xml_unescape();
	CHECK(result.length() == 1);
	CHECK(result[0] == 0xFFFFFFFF);
	input = "&#4294967295;";
	result = input.xml_unescape();
	CHECK(result.length() == 1);
	CHECK(result[0] == 0xFFFFFFFF);

	// Check out of range of char32_t
	input = "&#xFFFFFFFFF;";
	CHECK(input.xml_unescape() == input);
	input = "&#4294967296;";
	CHECK(input.xml_unescape() == input);

	// Shouldn't consume without ending in a ';'
	input = "&#66";
	CHECK(input.xml_unescape() == input);
	input = "&#x41";
	CHECK(input.xml_unescape() == input);

	// Invalid characters should make the entity ignored
	input = "&#x41SomeIrrelevantText;";
	CHECK(input.xml_unescape() == input);
	input = "&#66SomeIrrelevantText;";
	CHECK(input.xml_unescape() == input);
}

TEST_CASE("[String] Strip escapes") {
	String s = "\t\tTest Test\r\n Test";
	CHECK(s.strip_escapes() == "Test Test Test");
}

TEST_CASE("[String] Similarity") {
	String a = "Test";
	String b = "West";
	String c = "Toad";
	CHECK(a.similarity(b) > a.similarity(c));
}

TEST_CASE("[String] Strip edges") {
	String s = "\t Test Test   ";
	CHECK(s.strip_edges(true, false) == "Test Test   ");
	CHECK(s.strip_edges(false, true) == "\t Test Test");
	CHECK(s.strip_edges(true, true) == "Test Test");
}

TEST_CASE("[String] Trim") {
	String s = "aaaTestbbb";
	MULTICHECK_STRING_EQ(s, trim_prefix, "aaa", "Testbbb");
	MULTICHECK_STRING_EQ(s, trim_prefix, "Test", s);
	MULTICHECK_STRING_EQ(s, trim_prefix, "", s);
	MULTICHECK_STRING_EQ(s, trim_prefix, "aaaTestbbb", "");
	MULTICHECK_STRING_EQ(s, trim_prefix, "bbb", s);
	MULTICHECK_STRING_EQ(s, trim_prefix, "AAA", s);

	MULTICHECK_STRING_EQ(s, trim_suffix, "bbb", "aaaTest");
	MULTICHECK_STRING_EQ(s, trim_suffix, "Test", s);
	MULTICHECK_STRING_EQ(s, trim_suffix, "", s);
	MULTICHECK_STRING_EQ(s, trim_suffix, "aaaTestbbb", "");
	MULTICHECK_STRING_EQ(s, trim_suffix, "aaa", s);
	MULTICHECK_STRING_EQ(s, trim_suffix, "BBB", s);
}

TEST_CASE("[String] Right/Left") {
	String s = "aaaTestbbb";
	//                ^
	CHECK(s.right(6) == "estbbb");
	CHECK(s.right(-6) == "tbbb");
	CHECK(s.left(6) == "aaaTes");
	CHECK(s.left(-6) == "aaaT");
}

TEST_CASE("[String] Repeat") {
	String s = "abababab";
	String x = "ab";
	String t = x.repeat(4);
	CHECK(t == s);
}

TEST_CASE("[String] Reverse") {
	String s = "Abcd";
	CHECK(s.reverse() == "dcbA");
}

TEST_CASE("[String] SHA1/SHA256/MD5") {
	String s = "Godot";
	String sha1 = "a1e91f39b9fce6a9998b14bdbe2aa2b39dc2d201";
	static uint8_t sha1_buf[20] = {
		0xA1, 0xE9, 0x1F, 0x39, 0xB9, 0xFC, 0xE6, 0xA9, 0x99, 0x8B, 0x14, 0xBD, 0xBE, 0x2A, 0xA2, 0xB3,
		0x9D, 0xC2, 0xD2, 0x01
	};
	String sha256 = "2a02b2443f7985d89d09001086ae3dcfa6eb0f55c6ef170715d42328e16e6cb8";
	static uint8_t sha256_buf[32] = {
		0x2A, 0x02, 0xB2, 0x44, 0x3F, 0x79, 0x85, 0xD8, 0x9D, 0x09, 0x00, 0x10, 0x86, 0xAE, 0x3D, 0xCF,
		0xA6, 0xEB, 0x0F, 0x55, 0xC6, 0xEF, 0x17, 0x07, 0x15, 0xD4, 0x23, 0x28, 0xE1, 0x6E, 0x6C, 0xB8
	};
	String md5 = "4a336d087aeb0390da10ee2ea7cb87f8";
	static uint8_t md5_buf[16] = {
		0x4A, 0x33, 0x6D, 0x08, 0x7A, 0xEB, 0x03, 0x90, 0xDA, 0x10, 0xEE, 0x2E, 0xA7, 0xCB, 0x87, 0xF8
	};

	PackedByteArray buf = s.sha1_buffer();
	CHECK(memcmp(sha1_buf, buf.ptr(), 20) == 0);
	CHECK(s.sha1_text() == sha1);

	buf = s.sha256_buffer();
	CHECK(memcmp(sha256_buf, buf.ptr(), 32) == 0);
	CHECK(s.sha256_text() == sha256);

	buf = s.md5_buffer();
	CHECK(memcmp(md5_buf, buf.ptr(), 16) == 0);
	CHECK(s.md5_text() == md5);
}

TEST_CASE("[String] Join") {
	String comma = ", ";
	String empty = "";
	Vector<String> parts;

	CHECK(comma.join(parts) == "");
	CHECK(empty.join(parts) == "");

	parts.push_back("One");
	CHECK(comma.join(parts) == "One");
	CHECK(empty.join(parts) == "One");

	parts.push_back("B");
	parts.push_back("C");
	CHECK(comma.join(parts) == "One, B, C");
	CHECK(empty.join(parts) == "OneBC");

	parts.push_back("");
	CHECK(comma.join(parts) == "One, B, C, ");
	CHECK(empty.join(parts) == "OneBC");
}

TEST_CASE("[String] Is_*") {
	static const char *data[] = { "-30", "100", "10.1", "10,1", "1e2", "1e-2", "1e2e3", "0xAB", "AB", "Test1", "1Test", "Test*1", "文字", "1E2", "1E-2" };
	static bool isnum[] = { true, true, true, false, false, false, false, false, false, false, false, false, false, false, false };
	static bool isint[] = { true, true, false, false, false, false, false, false, false, false, false, false, false, false, false };
	static bool ishex[] = { true, true, false, false, true, false, true, false, true, false, false, false, false, true, false };
	static bool ishex_p[] = { false, false, false, false, false, false, false, true, false, false, false, false, false, false, false };
	static bool isflt[] = { true, true, true, false, true, true, false, false, false, false, false, false, false, true, true };
	static bool isaid[] = { false, false, false, false, false, false, false, false, true, true, false, false, false, false, false };
	static bool isuid[] = { false, false, false, false, false, false, false, false, true, true, false, false, true, false, false };
	for (unsigned int i = 0; i < std_size(data); i++) {
		String s = String::utf8(data[i]);
		CHECK(s.is_numeric() == isnum[i]);
		CHECK(s.is_valid_int() == isint[i]);
		CHECK(s.is_valid_hex_number(false) == ishex[i]);
		CHECK(s.is_valid_hex_number(true) == ishex_p[i]);
		CHECK(s.is_valid_float() == isflt[i]);
		CHECK(s.is_valid_ascii_identifier() == isaid[i]);
		CHECK(s.is_valid_unicode_identifier() == isuid[i]);
	}
}

TEST_CASE("[String] humanize_size") {
	CHECK(String::humanize_size(1000) == "1000 B");
	CHECK(String::humanize_size(1025) == "1.00 KiB");
	CHECK(String::humanize_size(1025300) == "1001.2 KiB");
	CHECK(String::humanize_size(100523550) == "95.86 MiB");
	CHECK(String::humanize_size(5345555000) == "4.97 GiB");
}

TEST_CASE("[String] validate_node_name") {
	String numeric_only = "12345";
	CHECK(numeric_only.validate_node_name() == "12345");

	String name_with_spaces = "Name with spaces";
	CHECK(name_with_spaces.validate_node_name() == "Name with spaces");

	String name_with_kana = U"Name with kana ゴドツ";
	CHECK(name_with_kana.validate_node_name() == U"Name with kana ゴドツ");

	String name_with_invalid_chars = "Name with invalid characters :.@%removed!";
	CHECK(name_with_invalid_chars.validate_node_name() == "Name with invalid characters ____removed!");
}

TEST_CASE("[String] validate_ascii_identifier") {
	String empty_string;
	CHECK(empty_string.validate_ascii_identifier() == "_");

	String numeric_only = "12345";
	CHECK(numeric_only.validate_ascii_identifier() == "_12345");

	String name_with_spaces = "Name with spaces";
	CHECK(name_with_spaces.validate_ascii_identifier() == "Name_with_spaces");

	String name_with_invalid_chars = U"Invalid characters:@*#&世界";
	CHECK(name_with_invalid_chars.validate_ascii_identifier() == "Invalid_characters_______");
}

TEST_CASE("[String] validate_unicode_identifier") {
	String empty_string;
	CHECK(empty_string.validate_unicode_identifier() == "_");

	String numeric_only = "12345";
	CHECK(numeric_only.validate_unicode_identifier() == "_12345");

	String name_with_spaces = "Name with spaces";
	CHECK(name_with_spaces.validate_unicode_identifier() == "Name_with_spaces");

	String name_with_invalid_chars = U"Invalid characters:@*#&世界";
	CHECK(name_with_invalid_chars.validate_unicode_identifier() == U"Invalid_characters_____世界");
}

TEST_CASE("[String] Variant indexed get") {
	Variant s = String("abcd");
	bool valid = false;
	bool oob = true;

	String r = s.get_indexed(1, valid, oob);

	CHECK(valid);
	CHECK_FALSE(oob);
	CHECK_EQ(r, String("b"));
}

TEST_CASE("[String] Variant validated indexed get") {
	Variant s = String("abcd");

	Variant::ValidatedIndexedGetter getter = Variant::get_member_validated_indexed_getter(Variant::STRING);

	Variant r;
	bool oob = true;
	getter(&s, 1, &r, &oob);

	CHECK_FALSE(oob);
	CHECK_EQ(r, String("b"));
}

TEST_CASE("[String] Variant ptr indexed get") {
	String s("abcd");

	Variant::PTRIndexedGetter getter = Variant::get_member_ptr_indexed_getter(Variant::STRING);

	String r;
	getter(&s, 1, &r);

	CHECK_EQ(r, String("b"));
}

TEST_CASE("[String] Variant indexed set") {
	Variant s = String("abcd");
	bool valid = false;
	bool oob = true;

	s.set_indexed(1, String("z"), valid, oob);

	CHECK(valid);
	CHECK_FALSE(oob);
	CHECK_EQ(s, String("azcd"));
}

TEST_CASE("[String] Variant validated indexed set") {
	Variant s = String("abcd");

	Variant::ValidatedIndexedSetter setter = Variant::get_member_validated_indexed_setter(Variant::STRING);

	Variant v = String("z");
	bool oob = true;
	setter(&s, 1, &v, &oob);

	CHECK_FALSE(oob);
	CHECK_EQ(s, String("azcd"));
}

TEST_CASE("[String] Variant ptr indexed set") {
	String s("abcd");

	Variant::PTRIndexedSetter setter = Variant::get_member_ptr_indexed_setter(Variant::STRING);

	String v("z");
	setter(&s, 1, &v);

	CHECK_EQ(s, String("azcd"));
}

TEST_CASE("[String][URL] Parse URL") {
#define CHECK_URL(m_url_to_parse, m_expected_schema, m_expected_host, m_expected_port, m_expected_path, m_expected_fragment, m_expected_error) \
	if (true) { \
		int port; \
		String url(m_url_to_parse), schema, host, path, fragment; \
\
		CHECK_EQ(url.parse_url(schema, host, port, path, fragment), m_expected_error); \
		CHECK_EQ(schema, m_expected_schema); \
		CHECK_EQ(host, m_expected_host); \
		CHECK_EQ(path, m_expected_path); \
		CHECK_EQ(fragment, m_expected_fragment); \
		CHECK_EQ(port, m_expected_port); \
	} else \
		((void)0)

	// All elements.
	CHECK_URL("https://www.example.com:8080/path/to/file.html#fragment", "https://", "www.example.com", 8080, "/path/to/file.html", "fragment", Error::OK);

	// Valid URLs.
	CHECK_URL("https://godotengine.org", "https://", "godotengine.org", 0, "", "", Error::OK);
	CHECK_URL("https://godotengine.org/", "https://", "godotengine.org", 0, "/", "", Error::OK);
	CHECK_URL("godotengine.org/", "", "godotengine.org", 0, "/", "", Error::OK);
	CHECK_URL("HTTPS://godotengine.org/", "https://", "godotengine.org", 0, "/", "", Error::OK);
	CHECK_URL("https://GODOTENGINE.ORG/", "https://", "godotengine.org", 0, "/", "", Error::OK);
	CHECK_URL("http://godotengine.org", "http://", "godotengine.org", 0, "", "", Error::OK);
	CHECK_URL("https://godotengine.org:8080", "https://", "godotengine.org", 8080, "", "", Error::OK);
	CHECK_URL("https://godotengine.org/blog", "https://", "godotengine.org", 0, "/blog", "", Error::OK);
	CHECK_URL("https://godotengine.org/blog/", "https://", "godotengine.org", 0, "/blog/", "", Error::OK);
	CHECK_URL("https://docs.godotengine.org/en/stable", "https://", "docs.godotengine.org", 0, "/en/stable", "", Error::OK);
	CHECK_URL("https://docs.godotengine.org/en/stable/", "https://", "docs.godotengine.org", 0, "/en/stable/", "", Error::OK);
	CHECK_URL("https://me:secret@godotengine.org", "https://", "godotengine.org", 0, "", "", Error::OK);
	CHECK_URL("https://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]/ipv6", "https://", "fedc:ba98:7654:3210:fedc:ba98:7654:3210", 0, "/ipv6", "", Error::OK);

	// Scheme vs Fragment.
	CHECK_URL("google.com/#goto=http://redirect_url/", "", "google.com", 0, "/", "goto=http://redirect_url/", Error::OK);

	// Invalid URLs.

	// Invalid Scheme.
	CHECK_URL("https_://godotengine.org", "", "https_", 0, "//godotengine.org", "", Error::ERR_INVALID_PARAMETER);

	// Multiple ports.
	CHECK_URL("https://godotengine.org:8080:433", "https://", "", 0, "", "", Error::ERR_INVALID_PARAMETER);
	// Missing ] on literal IPv6.
	CHECK_URL("https://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210/ipv6", "https://", "", 0, "/ipv6", "", Error::ERR_INVALID_PARAMETER);
	// Missing host.
	CHECK_URL("https:///blog", "https://", "", 0, "/blog", "", Error::ERR_INVALID_PARAMETER);
	// Invalid ports.
	CHECK_URL("https://godotengine.org:notaport", "https://", "godotengine.org", 0, "", "", Error::ERR_INVALID_PARAMETER);
	CHECK_URL("https://godotengine.org:-8080", "https://", "godotengine.org", -8080, "", "", Error::ERR_INVALID_PARAMETER);
	CHECK_URL("https://godotengine.org:88888", "https://", "godotengine.org", 88888, "", "", Error::ERR_INVALID_PARAMETER);

#undef CHECK_URL
}

} // namespace TestString

// === Merged from tests/core/string/test_translation.cpp ===
/*  test_translation.cpp                                                  */
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



#include "core/io/resource_loader.h"
#include "core/config/project_settings.h"

#ifdef TOOLS_ENABLED
#include "modules/gltf/gltf_document.h"
#endif

#include "tests/test_macros.h"

namespace TestTranslation {

TEST_CASE("[Translation] Messages") {
	Ref<Translation> translation;
	translation.instantiate();
	translation->set_locale("fr");
	translation->add_message("Hello", "Bonjour");
	CHECK(translation->get_message("Hello") == "Bonjour");

	translation->erase_message("Hello");
	// The message no longer exists, so it returns an empty string instead.
	CHECK(translation->get_message("Hello") == "");

	List<String> messages;
	translation->get_message_list(&messages);
	CHECK(translation->get_message_count() == 0);
	CHECK(messages.size() == 0);

	translation->add_message("Hello2", "Bonjour2");
	translation->add_message("Hello3", "Bonjour3");
	messages.clear();
	translation->get_message_list(&messages);
	CHECK(translation->get_message_count() == 2);
	CHECK(messages.size() == 2);
	// Messages are stored in a Map, don't assume ordering.
	CHECK(messages.find("Hello2"));
	CHECK(messages.find("Hello3"));
}

TEST_CASE("[Translation] Messages with context") {
	Ref<Translation> translation;
	translation.instantiate();
	translation->set_locale("fr");
	translation->add_message("Hello", "Bonjour");
	translation->add_message("Hello", "Salut", "friendly");
	CHECK(translation->get_message("Hello") == "Bonjour");
	CHECK(translation->get_message("Hello", "friendly") == "Salut");
	CHECK(translation->get_message("Hello", "nonexistent_context") == "");

	// Only remove the message for the default context, not the "friendly" context.
	translation->erase_message("Hello");
	// The message no longer exists, so it returns an empty string instead.
	CHECK(translation->get_message("Hello") == "");
	CHECK(translation->get_message("Hello", "friendly") == "Salut");
	CHECK(translation->get_message("Hello", "nonexistent_context") == "");

	List<String> messages;
	translation->get_message_list(&messages);

	CHECK(translation->get_message_count() == 1);
	CHECK(messages.size() == 1);

	translation->add_message("Hello2", "Bonjour2");
	translation->add_message("Hello2", "Salut2", "friendly");
	translation->add_message("Hello3", "Bonjour3");
	messages.clear();
	translation->get_message_list(&messages);

	CHECK(translation->get_message_count() == 4);
	CHECK(messages.size() == 4);
	// Messages are stored in a Map, don't assume ordering.
	CHECK(messages.find("Hello2"));
	CHECK(messages.find("Hello3"));
	// Context and untranslated string are separated by EOT.
	CHECK(messages.find("friendly\x04Hello2"));
}

TEST_CASE("[Translation] Plural messages") {
	{
		Ref<Translation> translation;
		translation.instantiate();
		translation->set_locale("fr");
		CHECK(translation->get_nplurals() == 2);
	}

	{
		Ref<Translation> translation;
		translation.instantiate();
		translation->set_locale("invalid");
		CHECK(translation->get_nplurals() == 2);
	}

	{
		Ref<Translation> translation;
		translation.instantiate();
		translation->set_plural_rules_override("Plural-Forms: nplurals=2; plural=(n >= 2);");
		CHECK(translation->get_nplurals() == 2);

		PackedStringArray plurals;
		plurals.push_back("Il y a %d pomme");
		plurals.push_back("Il y a %d pommes");
		translation->add_plural_message("There are %d apples", plurals);
		ERR_PRINT_OFF;
		// This is invalid, as the number passed to `get_plural_message()` may not be negative.
		CHECK(vformat(translation->get_plural_message("There are %d apples", "", -1), -1) == "");
		ERR_PRINT_ON;
		CHECK(vformat(translation->get_plural_message("There are %d apples", "", 0), 0) == "Il y a 0 pomme");
		CHECK(vformat(translation->get_plural_message("There are %d apples", "", 1), 1) == "Il y a 1 pomme");
		CHECK(vformat(translation->get_plural_message("There are %d apples", "", 2), 2) == "Il y a 2 pommes");
	}
}

TEST_CASE("[Translation] Plural rules parsing") {
	ERR_PRINT_OFF;
	{
		CHECK(PluralRules::parse("") == nullptr);

		CHECK(PluralRules::parse("plurals=(n != 1);") == nullptr);
		CHECK(PluralRules::parse("nplurals; plurals=(n != 1);") == nullptr);
		CHECK(PluralRules::parse("nplurals=; plurals=(n != 1);") == nullptr);
		CHECK(PluralRules::parse("nplurals=0; plurals=(n != 1);") == nullptr);
		CHECK(PluralRules::parse("nplurals=-1; plurals=(n != 1);") == nullptr);

		CHECK(PluralRules::parse("nplurals=2;") == nullptr);
		CHECK(PluralRules::parse("nplurals=2; plurals;") == nullptr);
		CHECK(PluralRules::parse("nplurals=2; plurals=;") == nullptr);
	}
	ERR_PRINT_ON;

	{
		PluralRules *pr = PluralRules::parse("nplurals=3; plural=(n==0 ? 0 : n==1 ? 1 : 2);");
		REQUIRE(pr != nullptr);

		CHECK(pr->get_nplurals() == 3);
		CHECK(pr->get_plural() == "(n==0 ? 0 : n==1 ? 1 : 2)");

		CHECK(pr->evaluate(0) == 0);
		CHECK(pr->evaluate(1) == 1);
		CHECK(pr->evaluate(2) == 2);
		CHECK(pr->evaluate(3) == 2);

		memdelete(pr);
	}

	{
		PluralRules *pr = PluralRules::parse("nplurals=1; plural=0;");
		REQUIRE(pr != nullptr);

		CHECK(pr->get_nplurals() == 1);
		CHECK(pr->get_plural() == "0");

		CHECK(pr->evaluate(0) == 0);
		CHECK(pr->evaluate(1) == 0);
		CHECK(pr->evaluate(2) == 0);
		CHECK(pr->evaluate(3) == 0);

		memdelete(pr);
	}
}

#ifdef TOOLS_ENABLED
TEST_CASE("[Translation] Generate optimized from Translation and read messages") {
	Ref<Translation> translation = memnew(Translation);
	translation->set_locale("fr");
	translation->add_message("Hello", "Bonjour");
	translation->add_message("Hello2", "Bonjour2");
	translation->add_message("Hello3", "Bonjour3");

	Ref<Translation> optimized_translation = memnew(Translation);
	optimized_translation->generate(translation);
	CHECK(optimized_translation->get_message("Hello") == "Bonjour");
	CHECK(optimized_translation->get_message("Hello2") == "Bonjour2");
	CHECK(optimized_translation->get_message("Hello3") == "Bonjour3");
	CHECK(optimized_translation->get_message("DoesNotExist") == "");

	List<String> messages;
	// `get_message_list()` can't return the list of messages stored in an optimized Translation.
	optimized_translation->get_message_list(&messages);
	CHECK(optimized_translation->get_message_count() == 0);
	CHECK(messages.size() == 0);
}

TEST_CASE("[TranslationCSV] CSV import") {
	Ref<ResourceImporterCSVTranslation> import_csv_translation = memnew(ResourceImporterCSVTranslation);

	HashMap<String, Variant> options;
	options["compress"] = false;
	options["delimiter"] = 0;

	List<String> gen_files;

	Error result = import_csv_translation->import(0, TestUtils::get_data_path("translations.csv"),
			"", options, nullptr, &gen_files);
	CHECK(result == OK);
	CHECK(gen_files.size() == 4);

	Ref<TranslationDomain> td = TranslationServer::get_or_add_domain("godot.test");
	for (const String &file : gen_files) {
		Ref<Translation> translation = ResourceLoader::load(file);
		CHECK(translation.is_valid());
		td->add_translation(translation);
	}

	td->set_locale_override("en");

	CHECK(td->translate("GOOD_MORNING", String()) == "Good Morning");
	CHECK(td->translate("GOOD_EVENING", String()) == "Good Evening");

	td->set_locale_override("de");

	CHECK(td->translate("GOOD_MORNING", String()) == "Guten Morgen");
	CHECK(td->translate("GOOD_EVENING", String()) == "Good Evening"); // Left blank in CSV, should source from 'en'.

	td->set_locale_override("ja");

	CHECK(td->translate("GOOD_MORNING", String()) == String::utf8("おはよう"));
	CHECK(td->translate("GOOD_EVENING", String()) == String::utf8("こんばんは"));

	/* FIXME: This passes, but triggers a chain reaction that makes test_viewport
	 * and test_text_edit explode in a billion glittery Unicode particles.
	td->set_locale_override("fa");

	CHECK(td->translate("GOOD_MORNING", String()) == String::utf8("صبح بخیر"));
	CHECK(td->translate("GOOD_EVENING", String()) == String::utf8("عصر بخیر"));
	*/

	TranslationServer::remove_domain("godot.test");
}

#endif // TOOLS_ENABLED

} // namespace TestTranslation

// === Merged from tests/core/string/test_translation_server.cpp ===
/*  test_translation_server.cpp                                           */
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



#include "core/config/project_settings.h"

namespace TestTranslationServer {

TEST_CASE("[TranslationServer] Translation operations") {
	Ref<TranslationDomain> td = TranslationServer::get_or_add_domain("godot.test");
	CHECK(td->get_translations().is_empty());

	Ref<Translation> t1 = memnew(Translation);
	t1->set_locale("uk"); // Ukrainian.
	t1->add_message("Good Morning", String(U"Добрий ранок"));
	td->add_translation(t1);
	CHECK(td->get_translations().size() == 1);
	CHECK(td->has_translation_for_locale("uk", true));
	CHECK(td->has_translation_for_locale("uk", false));
	CHECK_FALSE(td->has_translation_for_locale("uk_UA", true));
	CHECK(td->has_translation_for_locale("uk_UA", false));
	CHECK(td->find_translations("uk", false).size() == 1);
	CHECK(td->find_translations("uk", true).size() == 1);
	CHECK(td->find_translations("uk_UA", false).size() == 1);
	CHECK(td->find_translations("uk_UA", true).size() == 0);

	Ref<Translation> t2 = memnew(Translation);
	t2->set_locale("uk_UA"); // Ukrainian in Ukraine.
	t2->add_message("Hello Godot", String(U"Привіт, Годо."));
	td->add_translation(t2);
	CHECK(td->get_translations().size() == 2);
	CHECK(td->has_translation_for_locale("uk", true));
	CHECK(td->has_translation_for_locale("uk", false));
	CHECK(td->has_translation_for_locale("uk_UA", true));
	CHECK(td->has_translation_for_locale("uk_UA", false));
	CHECK(td->find_translations("uk", false).size() == 2);
	CHECK(td->find_translations("uk", true).size() == 1);
	CHECK(td->find_translations("uk_UA", false).size() == 2);
	CHECK(td->find_translations("uk_UA", true).size() == 1);

	td->set_locale_override("uk");
	CHECK(td->translate("Good Morning", String()) == String::utf8("Добрий ранок"));

	td->remove_translation(t1);
	CHECK(td->get_translations().size() == 1);
	CHECK_FALSE(td->has_translation_for_locale("uk", true));
	CHECK(td->has_translation_for_locale("uk", false));
	CHECK(td->has_translation_for_locale("uk_UA", true));
	CHECK(td->has_translation_for_locale("uk_UA", false));
	CHECK(td->find_translations("uk", true).size() == 0);
	CHECK(td->find_translations("uk", false).size() == 1);
	CHECK(td->find_translations("uk_UA", true).size() == 1);
	CHECK(td->find_translations("uk_UA", false).size() == 1);

	// If no suitable Translation object has been found - the original message should be returned.
	CHECK(td->translate("Good Morning", String()) == "Good Morning");

	TranslationServer::remove_domain("godot.test");
}

TEST_CASE("[TranslationServer] Locale operations") {

	// Language variant test; we supplied the variant of Español and the result should be the same string.
	String loc = "es_Hani_ES_tradnl";
	String res = TranslationServer::standardize_locale(loc);

	CHECK(res == loc);

	// No such variant in variant_map; should return everything except the variant.
	loc = "es_Hani_ES_missing";
	res = TranslationServer::standardize_locale(loc);

	CHECK(res == "es_Hani_ES");

	// Non-ISO language name check (Windows issue).
	loc = "iw_Hani_IL";
	res = TranslationServer::standardize_locale(loc);

	CHECK(res == "he_Hani_IL");

	// Country rename check.
	loc = "uk_Hani_UK";
	res = TranslationServer::standardize_locale(loc);

	CHECK(res == "uk_Hani_GB");

	// Supplying a script name that is not in the list.
	loc = "de_Wrong_DE";
	res = TranslationServer::standardize_locale(loc);

	CHECK(res == "de_DE");

	// No added defaults.
	loc = "es_ES";
	res = TranslationServer::standardize_locale(loc, true);

	CHECK(res == "es_ES");

	// Add default script.
	loc = "az_AZ";
	res = TranslationServer::standardize_locale(loc, true);

	CHECK(res == "az_Latn_AZ");

	// Add default country.
	loc = "pa_Arab";
	res = TranslationServer::standardize_locale(loc, true);

	CHECK(res == "pa_Arab_PK");

	// Add default script and country.
	loc = "zh";
	res = TranslationServer::standardize_locale(loc, true);

	CHECK(res == "zh_Hans_CN");

	// Explicitly don't add defaults.
	loc = "zh";
	res = TranslationServer::standardize_locale(loc, false);

	CHECK(res == "zh");
}

TEST_CASE("[TranslationServer] Comparing locales") {

	String locale_a = "es";
	String locale_b = "es";

	// Exact match check.
	int res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 10);

	locale_a = "sr-Latn-CS";
	locale_b = "sr-Latn-RS";

	// Script matches (+1) but country doesn't (-1).
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 5);

	locale_a = "uz-Cyrl-UZ";
	locale_b = "uz-Latn-UZ";

	// Country matches (+1) but script doesn't (-1).
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 5);

	locale_a = "aa-Latn-ER";
	locale_b = "aa-Latn-ER-saaho";

	// Script and country match (+2) with variant on one locale (+0).
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 7);

	locale_a = "uz-Cyrl-UZ";
	locale_b = "uz-Latn-KG";

	// Both script and country mismatched (-2).
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 3);

	locale_a = "es-ES";
	locale_b = "es-AR";

	// Mismatched country (-1).
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 4);

	locale_a = "es";
	locale_b = "es-AR";

	// No country for one locale (+0).
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 5);

	locale_a = "es-EC";
	locale_b = "fr-LU";

	// No match.
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 0);

	locale_a = "zh-HK";
	locale_b = "zh";

	// In full standardization, zh-HK becomes zh_Hant_HK and zh becomes
	// zh_Hans_CN. Both script and country mismatch (-2).
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 3);

	locale_a = "zh-CN";
	locale_b = "zh";

	// In full standardization, zh and zh-CN both become zh_Hans_CN for an
	// exact match.
	res = TranslationServer::compare_locales(locale_a, locale_b);

	CHECK(res == 10);
}

} // namespace TestTranslationServer

// === Merged from tests/core/test_time.cpp ===
/*  test_time.cpp                                                         */
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



#include "core/os/time.h"

namespace TestTime {

#define YEAR_KEY "year"
#define MONTH_KEY "month"
#define DAY_KEY "day"
#define WEEKDAY_KEY "weekday"
#define HOUR_KEY "hour"
#define MINUTE_KEY "minute"
#define SECOND_KEY "second"
#define DST_KEY "dst"

TEST_CASE("[Time] Unix time conversion to/from datetime string") {
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1970-01-01T00:00:00") == 0, "Time get_unix_time_from_datetime_string: The timestamp for Unix epoch is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1970-01-01 00:00:00") == 0, "Time get_unix_time_from_datetime_string: The timestamp for Unix epoch with space is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1970-01-01") == 0, "Time get_unix_time_from_datetime_string: The timestamp for Unix epoch without time is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("00:00:00") == 0, "Time get_unix_time_from_datetime_string: The timestamp for zero time without date is zero.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1969-12-31T23:59:59") == -1, "Time get_unix_time_from_datetime_string: The timestamp for just before Unix epoch is negative one.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1234-05-06T07:08:09") == -23215049511, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary datetime is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1234-05-06 07:08:09") == -23215049511, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary datetime with space is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1234-05-06") == -23215075200, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary date without time is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("07:08:09") == 25689, "Time get_unix_time_from_datetime_string: The timestamp for an arbitrary time without date is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("2014-02-09T22:10:30") == 1391983830, "Time get_unix_time_from_datetime_string: The timestamp for GODOT IS OPEN SOURCE is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("2014-02-09 22:10:30") == 1391983830, "Time get_unix_time_from_datetime_string: The timestamp for GODOT IS OPEN SOURCE with space is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("2014-02-09") == 1391904000, "Time get_unix_time_from_datetime_string: The date for GODOT IS OPEN SOURCE without time is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("22:10:30") == 79830, "Time get_unix_time_from_datetime_string: The time for GODOT IS OPEN SOURCE without date is as expected.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("-1000000000-01-01T00:00:00") == -31557014167219200, "Time get_unix_time_from_datetime_string: In the year negative a billion, Japan might not have been here.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string("1000000-01-01T00:00:00") == 31494784780800, "Time get_unix_time_from_datetime_string: The timestamp for the year a million is as expected.");

	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(0) == "1970-01-01T00:00:00", "Time get_datetime_string_from_unix_time: The timestamp string for Unix epoch is zero.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(0, true) == "1970-01-01 00:00:00", "Time get_datetime_string_from_unix_time: The timestamp string for Unix epoch with space is zero.");
	CHECK_MESSAGE(Time::get_date_string_from_unix_time(0) == "1970-01-01", "Time get_date_string_from_unix_time: The date string for zero is Unix epoch date.");
	CHECK_MESSAGE(Time::get_time_string_from_unix_time(0) == "00:00:00", "Time get_time_string_from_unix_time: The date for zero zero is Unix epoch date.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(-1) == "1969-12-31T23:59:59", "Time get_time_string_from_unix_time: The timestamp string for just before Unix epoch is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(-23215049511) == "1234-05-06T07:08:09", "Time get_datetime_string_from_unix_time: The timestamp for an arbitrary datetime is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(-23215049511, true) == "1234-05-06 07:08:09", "Time get_datetime_string_from_unix_time: The timestamp for an arbitrary datetime with space is as expected.");
	CHECK_MESSAGE(Time::get_date_string_from_unix_time(-23215075200) == "1234-05-06", "Time get_date_string_from_unix_time: The timestamp for an arbitrary date without time is as expected.");
	CHECK_MESSAGE(Time::get_time_string_from_unix_time(25689) == "07:08:09", "Time get_time_string_from_unix_time: The timestamp for an arbitrary time without date is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(1391983830) == "2014-02-09T22:10:30", "Time get_datetime_string_from_unix_time: The timestamp for GODOT IS OPEN SOURCE is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(1391983830, true) == "2014-02-09 22:10:30", "Time get_datetime_string_from_unix_time: The timestamp for GODOT IS OPEN SOURCE with space is as expected.");
	CHECK_MESSAGE(Time::get_date_string_from_unix_time(1391904000) == "2014-02-09", "Time get_date_string_from_unix_time: The date for GODOT IS OPEN SOURCE without time is as expected.");
	CHECK_MESSAGE(Time::get_time_string_from_unix_time(79830) == "22:10:30", "Time get_time_string_from_unix_time: The time for GODOT IS OPEN SOURCE without date is as expected.");
	CHECK_MESSAGE(Time::get_datetime_string_from_unix_time(31494784780800) == "1000000-01-01T00:00:00", "Time get_datetime_string_from_unix_time: The timestamp for the year a million is as expected.");
	CHECK_MESSAGE(Time::get_offset_string_from_offset_minutes(0) == "+00:00", "Time get_offset_string_from_offset_minutes: The offset string is as expected.");
	CHECK_MESSAGE(Time::get_offset_string_from_offset_minutes(-600) == "-10:00", "Time get_offset_string_from_offset_minutes: The offset string is as expected.");
	CHECK_MESSAGE(Time::get_offset_string_from_offset_minutes(345) == "+05:45", "Time get_offset_string_from_offset_minutes: The offset string is as expected.");
}

TEST_CASE("[Time] System time methods") {
	const uint64_t ticks_msec = Time::get_ticks_msec();
	const uint64_t ticks_usec = Time::get_ticks_usec();

	CHECK_MESSAGE(Time::get_unix_time_from_system() > 1000000000, "Time get_unix_time_from_system: The timestamp from system time doesn't fail and is very positive.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string(Time::get_datetime_string_from_system()) > 1000000000, "Time get_datetime_string_from_system: The timestamp from system time doesn't fail and is very positive.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string(Time::get_date_string_from_system()) > 1000000000, "Time get_datetime_string_from_system: The date from system time doesn't fail and is very positive.");
	CHECK_MESSAGE(Time::get_unix_time_from_datetime_string(Time::get_time_string_from_system()) < 86400, "Time get_datetime_string_from_system: The time from system time doesn't fail and is within the acceptable range.");

	CHECK_MESSAGE(Time::get_ticks_msec() >= ticks_msec, "Time get_ticks_msec: The value has not decreased.");
	CHECK_MESSAGE(Time::get_ticks_usec() > ticks_usec, "Time get_ticks_usec: The value has increased.");
}

} // namespace TestTime

// === Merged from tests/core/test_validate_testing.cpp ===
/*  test_validate_testing.cpp                                             */
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



#include "core/types.h"
#include "tests/test_macros.h"

namespace TestValidateTesting {

TEST_SUITE("Validate tests") {
	TEST_CASE("Always pass") {
		CHECK(true);
	}
	TEST_CASE_PENDING("Pending tests are skipped") {
		if (!doctest::getContextOptions()->no_skip) { // Normal run.
			FAIL("This should be skipped if `--no-skip` is NOT set (missing `doctest::skip()` decorator?)");
		} else {
			CHECK_MESSAGE(true, "Pending test is run with `--no-skip`");
		}
	}
	TEST_CASE("Muting Godot error messages") {
		ERR_PRINT_OFF;
		CHECK_MESSAGE(!CoreGlobals::print_error_enabled, "Error printing should be disabled.");
		ERR_PRINT("Still waiting for Godot!"); // This should never get printed!
		ERR_PRINT_ON;
		CHECK_MESSAGE(CoreGlobals::print_error_enabled, "Error printing should be re-enabled.");
	}
	TEST_CASE("Stringify Variant types") {
		Variant var;
		INFO(var);

		String string("Godot is finally here!");
		INFO(string);

		Vector2 vec2(0.5, 1.0);
		INFO(vec2);

		Vector2i vec2i(1, 2);
		INFO(vec2i);

		Rect2 rect2(0.5, 0.5, 100.5, 100.5);
		INFO(rect2);

		Rect2i rect2i(0, 0, 100, 100);
		INFO(rect2i);

		Vector3 vec3(0.5, 1.0, 2.0);
		INFO(vec3);

		Vector3i vec3i(1, 2, 3);
		INFO(vec3i);

		Transform2D trans2d(0.5, Vector2(100, 100));
		INFO(trans2d);

		Plane plane(Vector3(1, 1, 1), 1.0);
		INFO(plane);

		Quaternion quat = Quaternion::from_euler(Vector3(0.5, 1.0, 2.0));
		INFO(quat);

		AABB aabb(Vector3(), Vector3(100, 100, 100));
		INFO(aabb);

		Basis basis(quat);
		INFO(basis);

		Transform3D trans(basis);
		INFO(trans);

		Color color(1, 0.5, 0.2, 0.3);
		INFO(color);

		String string_name("has_method");
		INFO(string_name);

		NodePath node_path("godot/sprite");
		INFO(node_path);

		INFO(RID());

		Object *obj = memnew(Object);
		INFO(obj);

		Callable callable(obj, "has_method");
		INFO(callable);

		Signal signal(obj, "script_changed");
		INFO(signal);

		memdelete(obj);

		Dictionary dict;
		dict["string"] = string;
		dict["color"] = color;
		INFO(dict);

		Array arr = { string, color };
		INFO(arr);

		PackedByteArray byte_arr;
		byte_arr.push_back(0);
		byte_arr.push_back(1);
		byte_arr.push_back(2);
		INFO(byte_arr);

		PackedInt32Array int32_arr;
		int32_arr.push_back(0);
		int32_arr.push_back(1);
		int32_arr.push_back(2);
		INFO(int32_arr);

		PackedInt64Array int64_arr;
		int64_arr.push_back(0);
		int64_arr.push_back(1);
		int64_arr.push_back(2);
		INFO(int64_arr);

		PackedFloat32Array float32_arr;
		float32_arr.push_back(0.5);
		float32_arr.push_back(1.5);
		float32_arr.push_back(2.5);
		INFO(float32_arr);

		PackedFloat64Array float64_arr;
		float64_arr.push_back(0.5);
		float64_arr.push_back(1.5);
		float64_arr.push_back(2.5);
		INFO(float64_arr);

		PackedStringArray str_arr = string.split(" ");
		INFO(str_arr);

		PackedVector2Array vec2_arr;
		vec2_arr.push_back(Vector2(0, 0));
		vec2_arr.push_back(Vector2(1, 1));
		vec2_arr.push_back(Vector2(2, 2));
		INFO(vec2_arr);

		PackedVector3Array vec3_arr;
		vec3_arr.push_back(Vector3(0, 0, 0));
		vec3_arr.push_back(Vector3(1, 1, 1));
		vec3_arr.push_back(Vector3(2, 2, 2));
		INFO(vec3_arr);

		PackedColorArray color_arr;
		color_arr.push_back(Color(0, 0, 0));
		color_arr.push_back(Color(1, 1, 1));
		color_arr.push_back(Color(2, 2, 2));
		INFO(color_arr);

		PackedVector4Array vec4_arr;
		vec4_arr.push_back(Vector4(0, 0, 0, 0));
		vec4_arr.push_back(Vector4(1, 1, 1, 1));
		vec4_arr.push_back(Vector4(2, 2, 2, 2));
		vec4_arr.push_back(Vector4(3, 3, 3, 3));
		INFO(vec4_arr);

		// doctest string concatenation.
		CHECK_MESSAGE(true, var, " ", vec2, " ", rect2, " ", color);
	}
	TEST_CASE("Detect error messages") {
		ErrorDetector ed;

		REQUIRE_FALSE(ed.has_error);

		ERR_PRINT_OFF;
		ERR_PRINT("Still waiting for Godot!");
		ERR_PRINT_ON;

		REQUIRE(ed.has_error);
	}
}

} // namespace TestValidateTesting

// === Merged from tests/core/variant/test_array.cpp ===
/*  test_array.cpp                                                        */
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




namespace TestArray {

TEST_CASE("[Array] initializer list") {
	Array arr = { 0, 1, "test", true, { 0.0, 1.0 } };
	CHECK(arr.size() == 5);
	CHECK(arr[0] == Variant(0));
	CHECK(arr[1] == Variant(1));
	CHECK(arr[2] == Variant("test"));
	CHECK(arr[3] == Variant(true));
	CHECK(arr[4] == Variant({ 0.0, 1.0 }));

	arr = { "reassign" };
	CHECK(arr.size() == 1);
	CHECK(arr[0] == Variant("reassign"));

	TypedArray<int> typed_arr = { 0, 1, 2 };
	CHECK(typed_arr.size() == 3);
	CHECK(typed_arr[0] == Variant(0));
	CHECK(typed_arr[1] == Variant(1));
	CHECK(typed_arr[2] == Variant(2));
}

TEST_CASE("[Array] size(), clear(), and is_empty()") {
	Array arr;
	CHECK(arr.size() == 0);
	CHECK(arr.is_empty());
	arr.push_back(1);
	CHECK(arr.size() == 1);
	arr.clear();
	CHECK(arr.is_empty());
	CHECK(arr.size() == 0);
}

TEST_CASE("[Array] fill()") {
	Array arr;
	arr.resize(5);
	arr.fill(7);
	Array expected = { 7, 7, 7, 7, 7 };
	CHECK_EQ(arr, expected);

	Array empty;
	empty.fill(7);
	Array empty_expected;
	CHECK_EQ(empty, empty_expected);
}

TEST_CASE("[Array] reverse()") {
	Array odd_sized = { 1, 2, 3 };
	odd_sized.reverse();
	Array odd_sized_expected = { 3, 2, 1 };
	CHECK_EQ(odd_sized, odd_sized_expected);

	Array even_sized = { "a", "b", "c", "d" };
	even_sized.reverse();
	Array even_sized_expected = { "d", "c", "b", "a" };
	CHECK_EQ(even_sized, even_sized_expected);

	Array empty;
	empty.reverse();
	Array empty_expected;
	CHECK_EQ(empty, empty_expected);
}

TEST_CASE("[Array] Assignment and comparison operators") {
	Array arr1;
	Array arr2;
	arr1.push_back(1);
	CHECK(arr1 != arr2);
	CHECK(arr1 > arr2);
	CHECK(arr1 >= arr2);
	arr2.push_back(2);
	CHECK(arr1 != arr2);
	CHECK(arr1 < arr2);
	CHECK(arr1 <= arr2);
	CHECK(arr2 > arr1);
	CHECK(arr2 >= arr1);
	Array arr3 = arr2;
	CHECK(arr3 == arr2);
}

TEST_CASE("[Array] append()") {
	Array arr;
	arr.append(1);
	arr.append(2);
	arr.append(3);
	arr.append("test");
	Array expected = { 1, 2, 3, "test" };
	CHECK_EQ(arr, expected);
}

TEST_CASE("[Array] append_array()") {
	Array arr1;
	Array arr2;
	arr1.push_back(1);
	arr1.append_array(arr2);
	CHECK(arr1.size() == 1);
	arr2.push_back(2);
	arr1.append_array(arr2);
	CHECK(arr1.size() == 2);
	CHECK(int(arr1[0]) == 1);
	CHECK(int(arr1[1]) == 2);
}

TEST_CASE("[Array] resize(), insert(), and erase()") {
	Array arr;
	arr.resize(2);
	CHECK(arr.size() == 2);
	arr.insert(0, 1);
	CHECK(int(arr[0]) == 1);
	arr.insert(0, 2);
	CHECK(int(arr[0]) == 2);
	arr.erase(2);
	CHECK(int(arr[0]) == 1);
	arr.resize(0);
	CHECK(arr.size() == 0);
	arr.insert(0, 8);
	CHECK(arr.size() == 1);
	arr.insert(1, 16);
	CHECK(int(arr[1]) == 16);
	arr.insert(-1, 3);
	CHECK(int(arr[1]) == 3);
}

TEST_CASE("[Array] front() and back()") {
	Array arr;
	arr.push_back(1);
	CHECK(int(arr.front()) == 1);
	CHECK(int(arr.back()) == 1);
	arr.push_back(3);
	CHECK(int(arr.front()) == 1);
	CHECK(int(arr.back()) == 3);
}

TEST_CASE("[Array] has() and count()") {
	Array arr = { 1, 1 };
	CHECK(arr.has(1));
	CHECK(!arr.has(2));
	CHECK(arr.count(1) == 2);
	CHECK(arr.count(2) == 0);
}

TEST_CASE("[Array] remove_at()") {
	Array arr = { 1, 2 };
	arr.remove_at(0);
	CHECK(arr.size() == 1);
	CHECK(int(arr[0]) == 2);
	arr.remove_at(0);
	CHECK(arr.size() == 0);

	// Negative index.
	arr.push_back(3);
	arr.push_back(4);
	arr.remove_at(-1);
	CHECK(arr.size() == 1);
	CHECK(int(arr[0]) == 3);
	arr.remove_at(-1);
	CHECK(arr.size() == 0);

	// The array is now empty; try to use `remove_at()` again.
	// Normally, this prints an error message so we silence it.
	ERR_PRINT_OFF;
	arr.remove_at(0);
	ERR_PRINT_ON;

	CHECK(arr.size() == 0);
}

TEST_CASE("[Array] get() and set()") {
	Array arr = { 1, 2, 3 };
	CHECK_EQ(int(arr.get(0)), 1);
	CHECK_EQ(int(arr.get(1)), 2);
	CHECK_EQ(int(arr.get(2)), 3);

	arr.set(1, 5);
	CHECK_EQ(int(arr.get(1)), 5);
}

TEST_CASE("[Array] sort() and bsearch()") {
	Array arr = { 3, 4, 2, 1 };
	arr.sort();
	Array expected = { 1, 2, 3, 4 };
	CHECK_EQ(arr, expected);

	CHECK_EQ(arr.bsearch(1), 0);
	CHECK_EQ(arr.bsearch(3), 2);
	CHECK_EQ(arr.bsearch(-100), 0);
	CHECK_EQ(arr.bsearch(100), 4);
}

static bool _order_descending(int p_a, int p_b) {
	return p_b < p_a;
}

TEST_CASE("[Array] sort_custom() and bsearch_custom()") {
	Array arr = { 3, 4, 2, 1 };
	arr.sort_custom(callable_mp_static(_order_descending));
	Array expected = { 4, 3, 2, 1 };
	CHECK_EQ(arr, expected);

	CHECK_EQ(arr.bsearch_custom(1, callable_mp_static(_order_descending)), 3);
	CHECK_EQ(arr.bsearch_custom(4, callable_mp_static(_order_descending)), 0);
	CHECK_EQ(arr.bsearch_custom(100, callable_mp_static(_order_descending)), 0);
	CHECK_EQ(arr.bsearch_custom(-100, callable_mp_static(_order_descending)), 4);
}

static bool _is_even(int p_num) {
	return p_num % 2 == 0;
}

static bool _is_odd(int p_num) {
	return p_num % 2 == 1;
}

TEST_CASE("[Array] filter(), any(), all()") {
	Array nums = { 1, 2, 3, 4, 5, 6, 7 };
	CHECK(nums.any(callable_mp_static(_is_odd)));
	CHECK(nums.any(callable_mp_static(_is_even)));
	CHECK(!nums.all(callable_mp_static(_is_odd)));
	CHECK(!nums.all(callable_mp_static(_is_even)));

	Array odd = nums.filter(callable_mp_static(_is_odd));
	Array odd_expected = { 1, 3, 5, 7 };
	CHECK_EQ(odd, odd_expected);

	Array even = nums.filter(callable_mp_static(_is_even));
	Array even_expected = { 2, 4, 6 };
	CHECK_EQ(even, even_expected);

	CHECK(odd.all(callable_mp_static(_is_odd)));
	CHECK(odd.any(callable_mp_static(_is_odd)));
	CHECK(!odd.all(callable_mp_static(_is_even)));
	CHECK(!odd.any(callable_mp_static(_is_even)));
}

static int _add(int p_a, int p_b) {
	return p_a + p_b;
}

TEST_CASE("[Array] map() and reduce()") {
	Array array = { 1, 2, 3, 4, 5 };
	Array mapped = array.map(callable_mp_static(_add).bind(5));
	Array mapped_expected = { 6, 7, 8, 9, 10 };
	CHECK_EQ(mapped, mapped_expected);

	Variant sum = 0;
	CHECK_EQ(int(array.reduce(callable_mp_static(_add), sum)), 15);
}

TEST_CASE("[Array] push_front(), pop_front(), pop_back()") {
	Array arr;
	arr.push_front(1);
	arr.push_front(2);
	CHECK(int(arr[0]) == 2);
	arr.pop_front();
	CHECK(int(arr[0]) == 1);
	CHECK(arr.size() == 1);
	arr.push_front(2);
	arr.push_front(3);
	arr.pop_back();
	CHECK(int(arr[1]) == 2);
	CHECK(arr.size() == 2);
}

TEST_CASE("[Array] pop_at()") {
	ErrorDetector ed;

	Array arr = { 2, 4, 6, 8, 10 };

	REQUIRE(int(arr.pop_at(2)) == 6);
	REQUIRE(arr.size() == 4);
	CHECK(int(arr[0]) == 2);
	CHECK(int(arr[1]) == 4);
	CHECK(int(arr[2]) == 8);
	CHECK(int(arr[3]) == 10);

	REQUIRE(int(arr.pop_at(2)) == 8);
	REQUIRE(arr.size() == 3);
	CHECK(int(arr[0]) == 2);
	CHECK(int(arr[1]) == 4);
	CHECK(int(arr[2]) == 10);

	// Negative index.
	REQUIRE(int(arr.pop_at(-1)) == 10);
	REQUIRE(arr.size() == 2);
	CHECK(int(arr[0]) == 2);
	CHECK(int(arr[1]) == 4);

	// Invalid pop.
	ed.clear();
	ERR_PRINT_OFF;
	const Variant ret = arr.pop_at(-15);
	ERR_PRINT_ON;
	REQUIRE(ret.is_null());
	CHECK(ed.has_error);

	REQUIRE(int(arr.pop_at(0)) == 2);
	REQUIRE(arr.size() == 1);
	CHECK(int(arr[0]) == 4);

	REQUIRE(int(arr.pop_at(0)) == 4);
	REQUIRE(arr.is_empty());

	// Pop from empty array.
	ed.clear();
	REQUIRE(arr.pop_at(24).is_null());
	CHECK_FALSE(ed.has_error);
}

TEST_CASE("[Array] max() and min()") {
	Array arr;
	arr.push_back(3);
	arr.push_front(4);
	arr.push_back(5);
	arr.push_back(2);
	int max = int(arr.max());
	int min = int(arr.min());
	CHECK(max == 5);
	CHECK(min == 2);
}

TEST_CASE("[Array] slice()") {
	Array array = { 0, 1, 2, 3, 4, 5 };

	Array slice0 = array.slice(0, 0);
	CHECK(slice0.size() == 0);

	Array slice1 = array.slice(1, 3);
	CHECK(slice1.size() == 2);
	CHECK(slice1[0] == Variant(1));
	CHECK(slice1[1] == Variant(2));

	Array slice2 = array.slice(1, -1);
	CHECK(slice2.size() == 4);
	CHECK(slice2[0] == Variant(1));
	CHECK(slice2[1] == Variant(2));
	CHECK(slice2[2] == Variant(3));
	CHECK(slice2[3] == Variant(4));

	Array slice3 = array.slice(3);
	CHECK(slice3.size() == 3);
	CHECK(slice3[0] == Variant(3));
	CHECK(slice3[1] == Variant(4));
	CHECK(slice3[2] == Variant(5));

	Array slice4 = array.slice(2, -2);
	CHECK(slice4.size() == 2);
	CHECK(slice4[0] == Variant(2));
	CHECK(slice4[1] == Variant(3));

	Array slice5 = array.slice(-2);
	CHECK(slice5.size() == 2);
	CHECK(slice5[0] == Variant(4));
	CHECK(slice5[1] == Variant(5));

	Array slice6 = array.slice(2, 42);
	CHECK(slice6.size() == 4);
	CHECK(slice6[0] == Variant(2));
	CHECK(slice6[1] == Variant(3));
	CHECK(slice6[2] == Variant(4));
	CHECK(slice6[3] == Variant(5));

	Array slice7 = array.slice(4, 0, -2);
	CHECK(slice7.size() == 2);
	CHECK(slice7[0] == Variant(4));
	CHECK(slice7[1] == Variant(2));

	Array slice8 = array.slice(5, 0, -2);
	CHECK(slice8.size() == 3);
	CHECK(slice8[0] == Variant(5));
	CHECK(slice8[1] == Variant(3));
	CHECK(slice8[2] == Variant(1));

	Array slice9 = array.slice(10, 0, -2);
	CHECK(slice9.size() == 3);
	CHECK(slice9[0] == Variant(5));
	CHECK(slice9[1] == Variant(3));
	CHECK(slice9[2] == Variant(1));

	Array slice10 = array.slice(2, -10, -1);
	CHECK(slice10.size() == 3);
	CHECK(slice10[0] == Variant(2));
	CHECK(slice10[1] == Variant(1));
	CHECK(slice10[2] == Variant(0));

	ERR_PRINT_OFF;
	Array slice11 = array.slice(4, 1);
	CHECK(slice11.size() == 0);

	Array slice12 = array.slice(3, -4);
	CHECK(slice12.size() == 0);
	ERR_PRINT_ON;

	Array slice13 = Array().slice(1);
	CHECK(slice13.size() == 0);

	Array slice14 = array.slice(6);
	CHECK(slice14.size() == 0);
}

TEST_CASE("[Array] Duplicate array") {
	// a = [1, [2, 2], {3: 3}]
	Array a = { 1, { 2, 2 }, Dictionary({ { 3, 3 } }) };

	// Deep copy
	Array deep_a = a.duplicate(true);
	CHECK_MESSAGE(deep_a.id() != a.id(), "Should create a new array");
	CHECK_MESSAGE(Array(deep_a[1]).id() != Array(a[1]).id(), "Should clone nested array");
	CHECK_MESSAGE(Dictionary(deep_a[2]).id() != Dictionary(a[2]).id(), "Should clone nested dictionary");
	CHECK_EQ(deep_a, a);
	deep_a.push_back(1);
	CHECK_NE(deep_a, a);
	deep_a.pop_back();
	Array(deep_a[1]).push_back(1);
	CHECK_NE(deep_a, a);
	Array(deep_a[1]).pop_back();
	CHECK_EQ(deep_a, a);

	// Shallow copy
	Array shallow_a = a.duplicate(false);
	CHECK_MESSAGE(shallow_a.id() != a.id(), "Should create a new array");
	CHECK_MESSAGE(Array(shallow_a[1]).id() == Array(a[1]).id(), "Should keep nested array");
	CHECK_MESSAGE(Dictionary(shallow_a[2]).id() == Dictionary(a[2]).id(), "Should keep nested dictionary");
	CHECK_EQ(shallow_a, a);
	Array(shallow_a).push_back(1);
	CHECK_NE(shallow_a, a);
}

TEST_CASE("[Array] Duplicate recursive array") {
	// Self recursive
	Array a;
	a.push_back(a);

	Array a_shallow = a.duplicate(false);
	CHECK_EQ(a, a_shallow);

	// Deep copy of recursive array ends up with recursion limit and return
	// an invalid result (multiple nested arrays), the point is we should
	// not end up with a segfault and an error log should be printed
	ERR_PRINT_OFF;
	a.duplicate(true);
	ERR_PRINT_ON;

	// Nested recursive
	Array a1;
	Array a2;
	a2.push_back(a1);
	a1.push_back(a2);

	Array a1_shallow = a1.duplicate(false);
	CHECK_EQ(a1, a1_shallow);

	// Same deep copy issue as above
	ERR_PRINT_OFF;
	a1.duplicate(true);
	ERR_PRINT_ON;

	// Break the recursivity otherwise Array teardown will leak memory
	a.clear();
	a1.clear();
	a2.clear();
}

TEST_CASE("[Array] Hash array") {
	// a = [1, [2, 2], {3: 3}]
	Array a = { 1, { 2, 2 }, Dictionary({ { 3, 3 } }) };
	uint32_t original_hash = a.hash();

	a.push_back(1);
	CHECK_NE(a.hash(), original_hash);

	a.pop_back();
	CHECK_EQ(a.hash(), original_hash);

	Array(a[1]).push_back(1);
	CHECK_NE(a.hash(), original_hash);
	Array(a[1]).pop_back();
	CHECK_EQ(a.hash(), original_hash);

	(Dictionary(a[2]))[1] = 1;
	CHECK_NE(a.hash(), original_hash);
	Dictionary(a[2]).erase(1);
	CHECK_EQ(a.hash(), original_hash);

	Array a2 = a.duplicate(true);
	CHECK_EQ(a2.hash(), a.hash());
}

TEST_CASE("[Array] Hash recursive array") {
	Array a1;
	a1.push_back(a1);

	Array a2;
	a2.push_back(a2);

	// Hash should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_EQ(a1.hash(), a2.hash());
	ERR_PRINT_ON;

	// Break the recursivity otherwise Array teardown will leak memory
	a1.clear();
	a2.clear();
}

TEST_CASE("[Array] Empty comparison") {
	Array a1;
	Array a2;

	// test both operator== and operator!=
	CHECK_EQ(a1, a2);
	CHECK_FALSE(a1 != a2);
}

TEST_CASE("[Array] Flat comparison") {
	Array a1 = { 1 };
	Array a2 = { 1 };
	Array other_a = { 2 };

	// test both operator== and operator!=
	CHECK_EQ(a1, a1); // compare self
	CHECK_FALSE(a1 != a1);
	CHECK_EQ(a1, a2); // different equivalent arrays
	CHECK_FALSE(a1 != a2);
	CHECK_NE(a1, other_a); // different arrays with different content
	CHECK_FALSE(a1 == other_a);
}

TEST_CASE("[Array] Nested array comparison") {
	// a1 = [[[1], 2], 3]
	Array a1 = { { { 1 }, 2 }, 3 };

	Array a2 = a1.duplicate(true);

	// other_a = [[[1, 0], 2], 3]
	Array other_a = { { { 1, 0 }, 2 }, 3 };

	// test both operator== and operator!=
	CHECK_EQ(a1, a1); // compare self
	CHECK_FALSE(a1 != a1);
	CHECK_EQ(a1, a2); // different equivalent arrays
	CHECK_FALSE(a1 != a2);
	CHECK_NE(a1, other_a); // different arrays with different content
	CHECK_FALSE(a1 == other_a);
}

TEST_CASE("[Array] Nested dictionary comparison") {
	// a1 = [{1: 2}, 3]
	Array a1 = { Dictionary({ { 1, 2 } }), 3 };

	Array a2 = a1.duplicate(true);

	// other_a = [{1: 0}, 3]
	Array other_a = { Dictionary({ { 1, 0 } }), 3 };

	// test both operator== and operator!=
	CHECK_EQ(a1, a1); // compare self
	CHECK_FALSE(a1 != a1);
	CHECK_EQ(a1, a2); // different equivalent arrays
	CHECK_FALSE(a1 != a2);
	CHECK_NE(a1, other_a); // different arrays with different content
	CHECK_FALSE(a1 == other_a);
}

TEST_CASE("[Array] Recursive comparison") {
	Array a1;
	a1.push_back(a1);

	Array a2;
	a2.push_back(a2);

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_EQ(a1, a2);
	CHECK_FALSE(a1 != a2);
	ERR_PRINT_ON;

	a1.push_back(1);
	a2.push_back(1);

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_EQ(a1, a2);
	CHECK_FALSE(a1 != a2);
	ERR_PRINT_ON;

	a1.push_back(1);
	a2.push_back(2);

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_NE(a1, a2);
	CHECK_FALSE(a1 == a2);
	ERR_PRINT_ON;

	// Break the recursivity otherwise Array tearndown will leak memory
	a1.clear();
	a2.clear();
}

TEST_CASE("[Array] Recursive self comparison") {
	Array a1;
	Array a2;
	a2.push_back(a1);
	a1.push_back(a2);

	CHECK_EQ(a1, a1);
	CHECK_FALSE(a1 != a1);

	// Break the recursivity otherwise Array tearndown will leak memory
	a1.clear();
	a2.clear();
}

TEST_CASE("[Array] Iteration") {
	Array a1 = { 1, 2, 3 };
	Array a2 = { 1, 2, 3 };

	int idx = 0;
	for (Variant &E : a1) {
		CHECK_EQ(int(a2[idx]), int(E));
		idx++;
	}

	CHECK_EQ(idx, a1.size());

	idx = 0;

	for (const Variant &E : (const Array &)a1) {
		CHECK_EQ(int(a2[idx]), int(E));
		idx++;
	}

	CHECK_EQ(idx, a1.size());

	a1.clear();
}

TEST_CASE("[Array] Iteration and modification") {
	Array a1 = { 1, 2, 3 };
	Array a2 = { 2, 3, 4 };
	Array a3 = { 1, 2, 3 };
	Array a4 = { 1, 2, 3 };
	a3.make_read_only();

	int idx = 0;
	for (Variant &E : a1) {
		E = a2[idx];
		idx++;
	}

	CHECK_EQ(a1, a2);

	// Ensure read-only is respected.
	idx = 0;
	for (Variant &E : a3) {
		E = a2[idx];
	}

	CHECK_EQ(a3, a4);

	a1.clear();
	a2.clear();
	a4.clear();
}

TEST_CASE("[Array] Typed copying") {
	TypedArray<int> a1 = { 1 };
	TypedArray<double> a2 = { 1.0 };

	Array a3 = a1;
	TypedArray<int> a4 = a3;

	Array a5 = a2;
	TypedArray<int> a6 = a5;

	a3[0] = 2;
	a4[0] = 3;

	// Same typed TypedArray should be shared.
	CHECK_EQ(a1[0], Variant(3));
	CHECK_EQ(a3[0], Variant(3));
	CHECK_EQ(a4[0], Variant(3));

	a5[0] = 2.0;
	a6[0] = 3.0;

	// Different typed TypedArray should not be shared.
	CHECK_EQ(a2[0], Variant(2.0));
	CHECK_EQ(a5[0], Variant(2.0));
	CHECK_EQ(a6[0], Variant(3.0));

	a1.clear();
	a2.clear();
	a3.clear();
	a4.clear();
	a5.clear();
	a6.clear();
}

TEST_CASE("[Array] find() and rfind()") {
	Array array = { "a", "b", "c", "a", "b", "c" };

	CHECK_EQ(array.find("a"), 0);
	CHECK_EQ(array.find("c"), 2);
	CHECK_EQ(array.find("a", 1), 3);

	CHECK_EQ(array.rfind("b"), 4);
	CHECK_EQ(array.rfind("c", -2), 2);
}

static bool _find_custom_callable(const Variant &p_val) {
	return (int)p_val % 2 == 0;
}

TEST_CASE("[Array] Test find_custom") {
	Array a1 = { 1, 3, 4, 5, 8, 9 };
	// Find first even number.
	int index = a1.find_custom(callable_mp_static(_find_custom_callable));
	CHECK_EQ(index, 2);
}

TEST_CASE("[Array] Test rfind_custom") {
	Array a1 = { 1, 3, 4, 5, 8, 9 };
	// Find last even number.
	int index = a1.rfind_custom(callable_mp_static(_find_custom_callable));
	CHECK_EQ(index, 4);
}

TEST_CASE("[Array] Test typed arrays") {
	Array arr1;
	CHECK_FALSE(arr1.is_typed());

	arr1.set_typed(Variant::FLOAT, String(), Variant());
	CHECK(arr1.is_typed());
	CHECK_EQ(arr1.get_typed_builtin(), Variant::FLOAT);

	arr1.push_back(1);
	CHECK_EQ(arr1.size(), 1);

	ERR_PRINT_OFF;
	arr1.push_back("test wrong type");
	CHECK_EQ(arr1.size(), 1);
	ERR_PRINT_ON;

	Array arr2;
	arr2.set_typed(Variant::INT, String(), Variant());
	CHECK_FALSE(arr1.is_same_typed(arr2));

	Array arr3;
	arr3.set_typed(Variant::OBJECT, "Node", Variant());
	CHECK_EQ(arr3.get_typed_class_name(), "Node");
}

} // namespace TestArray

// === Merged from tests/core/variant/test_dictionary.cpp ===
/*  test_dictionary.cpp                                                   */
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




namespace TestDictionary {

TEST_CASE("[Dictionary] Assignment using bracket notation ([])") {
	Dictionary map;
	map["Hello"] = 0;
	CHECK(int(map["Hello"]) == 0);
	map["Hello"] = 3;
	CHECK(int(map["Hello"]) == 3);
	map["World!"] = 4;
	CHECK(int(map["World!"]) == 4);

	map[String("HelloName")] = 6;
	CHECK(int(map[String("HelloName")]) == 6);
	CHECK(int(map.find_key(6).get_type()) == Variant::STRING_NAME);
	map[String("HelloName")] = 7;
	CHECK(int(map[String("HelloName")]) == 7);

	// Test String and String are equivalent.
	map[String("Hello")] = 8;
	CHECK(int(map["Hello"]) == 8);
	map["Hello"] = 9;
	CHECK(int(map[String("Hello")]) == 9);

	// Test non-string keys, since keys can be of any Variant type.
	map[12345] = -5;
	CHECK(int(map[12345]) == -5);
	map[false] = 128;
	CHECK(int(map[false]) == 128);
	map[Vector2(10, 20)] = 30;
	CHECK(int(map[Vector2(10, 20)]) == 30);
	map[0] = 400;
	CHECK(int(map[0]) == 400);
	// Check that assigning 0 doesn't overwrite the value for `false`.
	CHECK(int(map[false]) == 128);

	// Ensure read-only maps aren't modified by non-existing keys.
	const int length = map.size();
	map.make_read_only();
	CHECK(int(map["This key does not exist"].get_type()) == Variant::NIL);
	CHECK(map.size() == length);
}

TEST_CASE("[Dictionary] List init") {
	Dictionary dict{
		{ 0, "int" },
		{ "packed_string_array", PackedStringArray({ "array", "of", "values" }) },
		{ "key", Dictionary({ { "nested", 200 } }) },
		{ Vector2(), "v2" },
	};
	CHECK(dict.size() == 4);
	CHECK(dict[0] == "int");
	CHECK(PackedStringArray(dict["packed_string_array"])[2] == "values");
	CHECK(Dictionary(dict["key"])["nested"] == Variant(200));
	CHECK(dict[Vector2()] == "v2");

	TypedDictionary<double, double> tdict{
		{ 0.0, 1.0 },
		{ 5.0, 2.0 },
	};
	CHECK_EQ(tdict[0.0], Variant(1.0));
	CHECK_EQ(tdict[5.0], Variant(2.0));
}

TEST_CASE("[Dictionary] get_key_list()") {
	Dictionary map;
	LocalVector<Variant> keys;
	keys = map.get_key_list();
	CHECK(keys.is_empty());
	map[1] = 3;
	keys = map.get_key_list();
	CHECK(keys.size() == 1);
	CHECK(int(keys[0]) == 1);
	map[2] = 4;
	keys = map.get_key_list();
	CHECK(keys.size() == 2);
}

TEST_CASE("[Dictionary] get_key_at_index()") {
	Dictionary map;
	map[4] = 3;
	Variant val = map.get_key_at_index(0);
	CHECK(int(val) == 4);
	map[3] = 1;
	val = map.get_key_at_index(0);
	CHECK(int(val) == 4);
	val = map.get_key_at_index(1);
	CHECK(int(val) == 3);
}

TEST_CASE("[Dictionary] getptr()") {
	Dictionary map;
	map[1] = 3;
	Variant *key = map.getptr(1);
	CHECK(int(*key) == 3);
	key = map.getptr(2);
	CHECK(key == nullptr);
}

TEST_CASE("[Dictionary] get_valid()") {
	Dictionary map;
	map[1] = 3;
	Variant val = map.get_valid(1);
	CHECK(int(val) == 3);
}

TEST_CASE("[Dictionary] set(), get(), and get_or_add()") {
	Dictionary map;

	map.set(1, 3);
	Variant val = map.get(1, -1);
	CHECK(int(val) == 3);

	map.set(1, 5);
	val = map.get(1, -1);
	CHECK(int(val) == 5);

	CHECK(int(map.get_or_add(1, 7)) == 5);
	CHECK(int(map.get_or_add(2, 7)) == 7);
}

TEST_CASE("[Dictionary] make_read_only() and is_read_only()") {
	Dictionary map;
	CHECK_FALSE(map.is_read_only());
	CHECK(map.set(1, 1));

	map.make_read_only();
	CHECK(map.is_read_only());

	ERR_PRINT_OFF;
	CHECK_FALSE(map.set(1, 2));
	ERR_PRINT_ON;
}

TEST_CASE("[Dictionary] size(), is_empty() and clear()") {
	Dictionary map;
	CHECK(map.size() == 0);
	CHECK(map.is_empty());
	map[1] = 3;
	CHECK(map.size() == 1);
	CHECK(!map.is_empty());
	map.clear();
	CHECK(map.size() == 0);
	CHECK(map.is_empty());
}

TEST_CASE("[Dictionary] has() and has_all()") {
	Dictionary map;
	CHECK(map.has(1) == false);
	map[1] = 3;
	CHECK(map.has(1));
	Array keys;
	keys.push_back(1);
	CHECK(map.has_all(keys));
	keys.push_back(2);
	CHECK(map.has_all(keys) == false);
}

TEST_CASE("[Dictionary] keys() and values()") {
	Dictionary map;
	Array keys = map.keys();
	Array values = map.values();
	CHECK(keys.is_empty());
	CHECK(values.is_empty());
	map[1] = 3;
	keys = map.keys();
	values = map.values();
	CHECK(int(keys[0]) == 1);
	CHECK(int(values[0]) == 3);
}

TEST_CASE("[Dictionary] merge() and merged()") {
	Dictionary d1 = {
		{ "key1", 1 },
		{ "key2", 2 },
	};
	Dictionary d2 = {
		{ "key2", 200 },
		{ "key3", 300 },
	};
	Dictionary expected_no_overwrite = {
		{ "key1", 1 },
		{ "key2", 2 },
		{ "key3", 300 },
	};
	Dictionary expected_overwrite = {
		{ "key1", 1 },
		{ "key2", 200 },
		{ "key3", 300 },
	};

	Dictionary d_test = d1.duplicate();
	d_test.merge(d2, false);
	CHECK_EQ(d_test, expected_no_overwrite);

	d_test = d1.duplicate();
	d_test.merge(d2, true);
	CHECK_EQ(d_test, expected_overwrite);

	CHECK_EQ(d1.merged(d2, false), expected_no_overwrite);
	CHECK_EQ(d1.merged(d2, true), expected_overwrite);
}

TEST_CASE("[Dictionary] Duplicate dictionary") {
	// d = {1: {1: 1}, {2: 2}: [2], [3]: 3}
	Dictionary k2 = { { 2, 2 } };
	Array k3 = { 3 };
	Dictionary d = {
		{ 1, Dictionary({ { 1, 1 } }) },
		{ k2, Array({ 2 }) },
		{ k3, 3 }
	};

	// Deep copy
	Dictionary deep_d = d.duplicate(true);
	CHECK_MESSAGE(deep_d.id() != d.id(), "Should create a new dictionary");
	CHECK_MESSAGE(Dictionary(deep_d[1]).id() != Dictionary(d[1]).id(), "Should clone nested dictionary");
	CHECK_MESSAGE(Array(deep_d[k2]).id() != Array(d[k2]).id(), "Should clone nested array");
	CHECK_EQ(deep_d, d);

	// Check that duplicate_deep matches duplicate(true)
	Dictionary deep_d2 = d.duplicate_deep();
	CHECK_EQ(deep_d, deep_d2);

	deep_d[0] = 0;
	CHECK_NE(deep_d, d);
	deep_d.erase(0);
	Dictionary(deep_d[1]).operator[](0) = 0;
	CHECK_NE(deep_d, d);
	Dictionary(deep_d[1]).erase(0);
	CHECK_EQ(deep_d, d);
	// Keys should also be copied
	k2[0] = 0;
	CHECK_NE(deep_d, d);
	k2.erase(0);
	CHECK_EQ(deep_d, d);
	k3.push_back(0);
	CHECK_NE(deep_d, d);
	k3.pop_back();
	CHECK_EQ(deep_d, d);

	// Shallow copy
	Dictionary shallow_d = d.duplicate(false);
	CHECK_MESSAGE(shallow_d.id() != d.id(), "Should create a new array");
	CHECK_MESSAGE(Dictionary(shallow_d[1]).id() == Dictionary(d[1]).id(), "Should keep nested dictionary");
	CHECK_MESSAGE(Array(shallow_d[k2]).id() == Array(d[k2]).id(), "Should keep nested array");
	CHECK_EQ(shallow_d, d);
	shallow_d[0] = 0;
	CHECK_NE(shallow_d, d);
	shallow_d.erase(0);
#if 0 // TODO: recursion in dict key currently is buggy
	// Keys should also be shallowed
	k2[0] = 0;
	CHECK_EQ(shallow_d, d);
	k2.erase(0);
	k3.push_back(0);
	CHECK_EQ(shallow_d, d);
#endif
}

TEST_CASE("[Dictionary] Duplicate recursive dictionary") {
	// Self recursive
	Dictionary d;
	d[1] = d;

	Dictionary d_shallow = d.duplicate(false);
	CHECK_EQ(d, d_shallow);

	// Deep copy of recursive dictionary endup with recursion limit and return
	// an invalid result (multiple nested dictionaries), the point is we should
	// not end up with a segfault and an error log should be printed
	ERR_PRINT_OFF;
	d.duplicate(true);
	ERR_PRINT_ON;

	// Nested recursive
	Dictionary d1;
	Dictionary d2;
	d1[2] = d2;
	d2[1] = d1;

	Dictionary d1_shallow = d1.duplicate(false);
	CHECK_EQ(d1, d1_shallow);

	// Same deep copy issue as above
	ERR_PRINT_OFF;
	d1.duplicate(true);
	ERR_PRINT_ON;

	// Break the recursivity otherwise Dictionary teardown will leak memory
	d.clear();
	d1.clear();
	d2.clear();
}

#if 0 // TODO: duplicate recursion in dict key is currently buggy
TEST_CASE("[Dictionary] Duplicate recursive dictionary on keys") {
	// Self recursive
	Dictionary d;
	d[d] = d;

	Dictionary d_shallow = d.duplicate(false);
	CHECK_EQ(d, d_shallow);

	// Deep copy of recursive dictionary endup with recursion limit and return
	// an invalid result (multiple nested dictionaries), the point is we should
	// not end up with a segfault and an error log should be printed
	ERR_PRINT_OFF;
	d.duplicate(true);
	ERR_PRINT_ON;

	// Nested recursive
	Dictionary d1;
	Dictionary d2;
	d1[d2] = d2;
	d2[d1] = d1;

	Dictionary d1_shallow = d1.duplicate(false);
	CHECK_EQ(d1, d1_shallow);

	// Same deep copy issue as above
	ERR_PRINT_OFF;
	d1.duplicate(true);
	ERR_PRINT_ON;

	// Break the recursivity otherwise Dictionary teardown will leak memory
	d.clear();
	d1.clear();
	d2.clear();
}
#endif

TEST_CASE("[Dictionary] Hash dictionary") {
	// d = {1: {1: 1}, {2: 2}: [2], [3]: 3}
	Dictionary k2 = { { 2, 2 } };
	Array k3 = { 3 };
	Dictionary d = {
		{ 1, Dictionary({ { 1, 1 } }) },
		{ k2, Array({ 2 }) },
		{ k3, 3 }
	};
	uint32_t original_hash = d.hash();

	// Modify dict change the hash
	d[0] = 0;
	CHECK_NE(d.hash(), original_hash);
	d.erase(0);
	CHECK_EQ(d.hash(), original_hash);

	// Modify nested item change the hash
	Dictionary(d[1]).operator[](0) = 0;
	CHECK_NE(d.hash(), original_hash);
	Dictionary(d[1]).erase(0);
	Array(d[k2]).push_back(0);
	CHECK_NE(d.hash(), original_hash);
	Array(d[k2]).pop_back();

	// Modify a key change the hash
	k2[0] = 0;
	CHECK_NE(d.hash(), original_hash);
	k2.erase(0);
	CHECK_EQ(d.hash(), original_hash);
	k3.push_back(0);
	CHECK_NE(d.hash(), original_hash);
	k3.pop_back();
	CHECK_EQ(d.hash(), original_hash);

	// Duplication doesn't change the hash
	Dictionary d2 = d.duplicate(true);
	CHECK_EQ(d2.hash(), original_hash);
}

TEST_CASE("[Dictionary] Hash recursive dictionary") {
	Dictionary d;
	d[1] = d;

	// Hash should reach recursion limit, we just make sure this doesn't blow up
	ERR_PRINT_OFF;
	d.hash();
	ERR_PRINT_ON;

	// Break the recursivity otherwise Dictionary teardown will leak memory
	d.clear();
}

#if 0 // TODO: recursion in dict key is currently buggy
TEST_CASE("[Dictionary] Hash recursive dictionary on keys") {
	Dictionary d;
	d[d] = 1;

	// Hash should reach recursion limit, we just make sure this doesn't blow up
	ERR_PRINT_OFF;
	d.hash();
	ERR_PRINT_ON;

	// Break the recursivity otherwise Dictionary teardown will leak memory
	d.clear();
}
#endif

TEST_CASE("[Dictionary] Empty comparison") {
	Dictionary d1;
	Dictionary d2;

	// test both operator== and operator!=
	CHECK_EQ(d1, d2);
	CHECK_FALSE(d1 != d2);
}

TEST_CASE("[Dictionary] Flat comparison") {
	Dictionary d1 = { { 1, 1 } };
	Dictionary d2 = { { 1, 1 } };
	Dictionary other_d = { { 2, 1 } };

	// test both operator== and operator!=
	CHECK_EQ(d1, d1); // compare self
	CHECK_FALSE(d1 != d1);
	CHECK_EQ(d1, d2); // different equivalent arrays
	CHECK_FALSE(d1 != d2);
	CHECK_NE(d1, other_d); // different arrays with different content
	CHECK_FALSE(d1 == other_d);
}

TEST_CASE("[Dictionary] Nested dictionary comparison") {
	// d1 = {1: {2: {3: 4}}}
	Dictionary d1 = { { 1, Dictionary({ { 2, Dictionary({ { 3, 4 } }) } }) } };

	Dictionary d2 = d1.duplicate(true);

	// other_d = {1: {2: {3: 0}}}
	Dictionary other_d = { { 1, Dictionary({ { 2, Dictionary({ { 3, 0 } }) } }) } };

	// test both operator== and operator!=
	CHECK_EQ(d1, d1); // compare self
	CHECK_FALSE(d1 != d1);
	CHECK_EQ(d1, d2); // different equivalent arrays
	CHECK_FALSE(d1 != d2);
	CHECK_NE(d1, other_d); // different arrays with different content
	CHECK_FALSE(d1 == other_d);
}

TEST_CASE("[Dictionary] Nested array comparison") {
	// d1 = {1: [2, 3]}
	Dictionary d1 = { { 1, { 2, 3 } } };

	Dictionary d2 = d1.duplicate(true);

	// other_d = {1: [2, 0]}
	Dictionary other_d = { { 1, { 2, 0 } } };

	// test both operator== and operator!=
	CHECK_EQ(d1, d1); // compare self
	CHECK_FALSE(d1 != d1);
	CHECK_EQ(d1, d2); // different equivalent arrays
	CHECK_FALSE(d1 != d2);
	CHECK_NE(d1, other_d); // different arrays with different content
	CHECK_FALSE(d1 == other_d);
}

TEST_CASE("[Dictionary] Recursive comparison") {
	Dictionary d1;
	d1[1] = d1;

	Dictionary d2;
	d2[1] = d2;

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_EQ(d1, d2);
	CHECK_FALSE(d1 != d2);
	ERR_PRINT_ON;

	d1[2] = 2;
	d2[2] = 2;

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_EQ(d1, d2);
	CHECK_FALSE(d1 != d2);
	ERR_PRINT_ON;

	d1[3] = 3;
	d2[3] = 0;

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_NE(d1, d2);
	CHECK_FALSE(d1 == d2);
	ERR_PRINT_ON;

	// Break the recursivity otherwise Dictionary teardown will leak memory
	d1.clear();
	d2.clear();
}

#if 0 // TODO: recursion in dict key is currently buggy
TEST_CASE("[Dictionary] Recursive comparison on keys") {
	Dictionary d1;
	// Hash computation should reach recursion limit
	ERR_PRINT_OFF;
	d1[d1] = 1;
	ERR_PRINT_ON;

	Dictionary d2;
	// Hash computation should reach recursion limit
	ERR_PRINT_OFF;
	d2[d2] = 1;
	ERR_PRINT_ON;

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_EQ(d1, d2);
	CHECK_FALSE(d1 != d2);
	ERR_PRINT_ON;

	d1[2] = 2;
	d2[2] = 2;

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_EQ(d1, d2);
	CHECK_FALSE(d1 != d2);
	ERR_PRINT_ON;

	d1[3] = 3;
	d2[3] = 0;

	// Comparison should reach recursion limit
	ERR_PRINT_OFF;
	CHECK_NE(d1, d2);
	CHECK_FALSE(d1 == d2);
	ERR_PRINT_ON;

	// Break the recursivity otherwise Dictionary teardown will leak memory
	d1.clear();
	d2.clear();
}
#endif

TEST_CASE("[Dictionary] Recursive self comparison") {
	Dictionary d1;
	Dictionary d2;
	d1[1] = d2;
	d2[1] = d1;

	CHECK_EQ(d1, d1);
	CHECK_FALSE(d1 != d1);

	// Break the recursivity otherwise Dictionary teardown will leak memory
	d1.clear();
	d2.clear();
}

TEST_CASE("[Dictionary] Order and find") {
	Dictionary d;
	d[4] = "four";
	d[8] = "eight";
	d[12] = "twelve";
	d["4"] = "four";

	Array keys = { 4, 8, 12, "4" };

	CHECK_EQ(d.keys(), keys);
	CHECK_EQ(d.find_key("four"), Variant(4));
	CHECK_EQ(d.find_key("does not exist"), Variant());
}

TEST_CASE("[Dictionary] sort()") {
	Dictionary d;
	d[3] = 3;
	d[2] = 2;
	d[4] = 4;
	d[1] = 1;

	Array expected_unsorted = { 3, 2, 4, 1 };
	CHECK_EQ(d.keys(), expected_unsorted);

	d.sort();
	Array expected_sorted = { 1, 2, 3, 4 };
	CHECK_EQ(d.keys(), expected_sorted);

	Dictionary d_str;
	d_str["b"] = 2;
	d_str["c"] = 3;
	d_str["a"] = 1;

	d_str.sort();
	Array expected_str_sorted = { "a", "b", "c" };
	CHECK_EQ(d_str.keys(), expected_str_sorted);
}

TEST_CASE("[Dictionary] assign()") {
	Dictionary untyped;
	untyped["key1"] = "value";
	CHECK(untyped.size() == 1);

	Dictionary typed;
	typed.set_typed(Variant::STRING, String(), Variant(), Variant::STRING, String(), Variant());
	typed.assign(untyped);
	CHECK(typed.size() == 1);
	typed["key2"] = "value";

	untyped.assign(typed);
	CHECK(untyped.size() == 2);
	untyped["key3"] = 5;
	CHECK(untyped.size() == 3);

	ERR_PRINT_OFF;
	typed.assign(untyped);
	ERR_PRINT_ON;
	CHECK(typed.size() == 2);
}

TEST_CASE("[Dictionary] Typed copying") {
	TypedDictionary<int, int> d1;
	d1[0] = 1;

	TypedDictionary<double, double> d2;
	d2[0] = 1.0;

	Dictionary d3 = d1;
	TypedDictionary<int, int> d4 = d3;

	Dictionary d5 = d2;
	TypedDictionary<int, int> d6 = d5;

	d3[0] = 2;
	d4[0] = 3;

	// Same typed TypedDictionary should be shared.
	CHECK_EQ(d1[0], Variant(3));
	CHECK_EQ(d3[0], Variant(3));
	CHECK_EQ(d4[0], Variant(3));

	d5[0] = 2.0;
	d6[0] = 3.0;

	// Different typed TypedDictionary should not be shared.
	CHECK_EQ(d2[0], Variant(2.0));
	CHECK_EQ(d5[0], Variant(2.0));
	CHECK_EQ(d6[0], Variant(3.0));

	d1.clear();
	d2.clear();
	d3.clear();
	d4.clear();
	d5.clear();
	d6.clear();
}

TEST_CASE("[Dictionary] Type checks/comparisons") {
	Dictionary d1;
	CHECK_FALSE(d1.is_typed());
	CHECK_FALSE(d1.is_typed_key());
	CHECK_FALSE(d1.is_typed_value());

	d1.set_typed(Variant::STRING, String(), Variant(), Variant::OBJECT, "Node", Variant());
	CHECK(d1.is_typed());
	CHECK(d1.is_typed_key());
	CHECK(d1.is_typed_value());
	CHECK_EQ(d1.get_typed_key_builtin(), Variant::STRING);
	CHECK_EQ(d1.get_typed_value_builtin(), Variant::OBJECT);
	CHECK_EQ(d1.get_typed_value_class_name(), "Node");

	Dictionary d2;
	CHECK_FALSE(d1.is_same_typed(d2));
	CHECK_FALSE(d1.is_same_typed_key(d2));
	CHECK_FALSE(d1.is_same_typed_value(d2));

	d2.set_typed(Variant::STRING, String(), Variant(), Variant::STRING, String(), Variant());
	CHECK_FALSE(d1.is_same_typed(d2));
	CHECK(d1.is_same_typed_key(d2));
	CHECK_FALSE(d1.is_same_typed_value(d2));
}

TEST_CASE("[Dictionary] Iteration") {
	Dictionary a1 = { { 1, 2 }, { 3, 4 }, { 5, 6 } };
	Dictionary a2 = { { 1, 2 }, { 3, 4 }, { 5, 6 } };

	int idx = 0;

	for (const KeyValue<Variant, Variant> &kv : (const Dictionary &)a1) {
		CHECK_EQ(int(a2[kv.key]), int(kv.value));
		idx++;
	}

	CHECK_EQ(idx, a1.size());

	a1.clear();
	a2.clear();
}

TEST_CASE("[Dictionary] Object value init") {
	Object *a = memnew(Object);
	Object *b = memnew(Object);
	TypedDictionary<double, Object *> tdict = {
		{ 0.0, a },
		{ 5.0, b },
	};
	CHECK_EQ(tdict[0.0], Variant(a));
	CHECK_EQ(tdict[5.0], Variant(b));
	memdelete(a);
	memdelete(b);
}

TEST_CASE("[Dictionary] RefCounted value init") {
	Ref<RefCounted> a = memnew(RefCounted);
	Ref<RefCounted> b = memnew(RefCounted);
	TypedDictionary<double, Ref<RefCounted>> tdict = {
		{ 0.0, a },
		{ 5.0, b },
	};
	CHECK_EQ(tdict[0.0], Variant(a));
	CHECK_EQ(tdict[5.0], Variant(b));
}

} // namespace TestDictionary

// === Merged from tests/core/variant/test_variant_initialization.cpp ===
/*  test_variant_initialization.cpp                                       */
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




namespace TestVariantInitialization {

TEST_CASE("[VariantInitialization] change_and_reset") {
	// Garbage data
	Variant *p = new Variant();
	std::memset((void *)p, -1, sizeof(Variant));
	VariantInternal::set_type(*p, Variant::Type::NIL);

	SUBCASE("bool") {
		VariantTypeChanger<bool>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::BOOL);
		CHECK(*p == Variant(false));

		*p = Variant(true);
		VariantTypeChanger<bool>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::BOOL);
		CHECK(*p == Variant(false));

		*p = Variant(1);
		VariantTypeChanger<bool>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::BOOL);
		CHECK(*p == Variant(false));
	}

	SUBCASE("int") {
		VariantTypeChanger<int64_t>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::INT);
		CHECK(*p == Variant(0));

		*p = Variant(1);
		VariantTypeChanger<int64_t>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::INT);
		CHECK(*p == Variant(0));

		*p = Variant(true);
		VariantTypeChanger<int64_t>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::INT);
		CHECK(*p == Variant(0));
	}

	SUBCASE("float") {
		VariantTypeChanger<double>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::FLOAT);
		CHECK(*p == Variant(0.));

		*p = Variant(1.0);
		VariantTypeChanger<double>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::FLOAT);
		CHECK(*p == Variant(0.));

		*p = Variant(true);
		VariantTypeChanger<double>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::FLOAT);
		CHECK(*p == Variant(0.));
	}

	SUBCASE("String") {
		VariantTypeChanger<String>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::STRING);
		CHECK(*p == Variant(""));

		*p = Variant(1.0);
		VariantTypeChanger<String>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::STRING);
		CHECK(*p == Variant(""));

		*p = Variant(true);
		VariantTypeChanger<String>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::STRING);
		CHECK(*p == Variant(""));
	}

	SUBCASE("Vector2") {
		VariantTypeChanger<Vector2>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR2);
		CHECK(*p == Variant(Vector2()));

		*p = Variant(Vector2(1., 1.));
		VariantTypeChanger<Vector2>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR2);
		CHECK(*p == Variant(Vector2()));

		*p = Variant(true);
		VariantTypeChanger<Vector2>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR2);
		CHECK(*p == Variant(Vector2()));
	}

	SUBCASE("Vector2i") {
		VariantTypeChanger<Vector2i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR2I);
		CHECK(*p == Variant(Vector2i()));

		*p = Variant(Vector2i(1, 1));
		VariantTypeChanger<Vector2i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR2I);
		CHECK(*p == Variant(Vector2i()));

		*p = Variant(true);
		VariantTypeChanger<Vector2i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR2I);
		CHECK(*p == Variant(Vector2i()));
	}

	SUBCASE("Vector3") {
		VariantTypeChanger<Vector3>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR3);
		CHECK(*p == Variant(Vector3()));

		*p = Variant(Vector3(1., 1., 1.));
		VariantTypeChanger<Vector3>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR3);
		CHECK(*p == Variant(Vector3()));

		*p = Variant(true);
		VariantTypeChanger<Vector3>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR3);
		CHECK(*p == Variant(Vector3()));
	}

	SUBCASE("Vector3i") {
		VariantTypeChanger<Vector3i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR3I);
		CHECK(*p == Variant(Vector3i()));

		*p = Variant(Vector3i(1, 1, 1));
		VariantTypeChanger<Vector3i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR3I);
		CHECK(*p == Variant(Vector3i()));

		*p = Variant(true);
		VariantTypeChanger<Vector3i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR3I);
		CHECK(*p == Variant(Vector3i()));
	}

	SUBCASE("Vector4") {
		VariantTypeChanger<Vector4>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR4);
		CHECK(*p == Variant(Vector4()));

		*p = Variant(Vector4(1., 1., 1., 1.));
		VariantTypeChanger<Vector4>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR4);
		CHECK(*p == Variant(Vector4()));

		*p = Variant(true);
		VariantTypeChanger<Vector4>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR4);
		CHECK(*p == Variant(Vector4()));
	}

	SUBCASE("Vector4i") {
		VariantTypeChanger<Vector4i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR4I);
		CHECK(*p == Variant(Vector4i()));

		*p = Variant(Vector4i(1, 1, 1, 1));
		VariantTypeChanger<Vector4i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR4I);
		CHECK(*p == Variant(Vector4i()));

		*p = Variant(true);
		VariantTypeChanger<Vector4i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::VECTOR4I);
		CHECK(*p == Variant(Vector4i()));
	}

	SUBCASE("Rect2") {
		VariantTypeChanger<Rect2>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RECT2);
		CHECK(*p == Variant(Rect2()));

		*p = Variant(Rect2(1., 1., 1., 1.));
		VariantTypeChanger<Rect2>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RECT2);
		CHECK(*p == Variant(Rect2()));

		*p = Variant(true);
		VariantTypeChanger<Rect2>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RECT2);
		CHECK(*p == Variant(Rect2()));
	}

	SUBCASE("Rect2i") {
		VariantTypeChanger<Rect2i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RECT2I);
		CHECK(*p == Variant(Rect2i()));

		*p = Variant(Rect2i(1, 1, 1, 1));
		VariantTypeChanger<Rect2i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RECT2I);
		CHECK(*p == Variant(Rect2i()));

		*p = Variant(true);
		VariantTypeChanger<Rect2i>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RECT2I);
		CHECK(*p == Variant(Rect2i()));
	}

	SUBCASE("Transform3D") {
		VariantTypeChanger<Transform3D>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::TRANSFORM3D);
		CHECK(*p == Variant(Transform3D()));

		Transform3D t;
		t.origin = Vector3(1, 1, 1);
		*p = Variant(t);
		VariantTypeChanger<Transform3D>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::TRANSFORM3D);
		CHECK(*p == Variant(Transform3D()));

		*p = Variant(true);
		VariantTypeChanger<Transform3D>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::TRANSFORM3D);
		CHECK(*p == Variant(Transform3D()));
	}

	SUBCASE("Projection") {
		VariantTypeChanger<Projection>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PROJECTION);
		CHECK(*p == Variant(Projection()));

		Projection pj;
		pj.columns[0].x = 1;
		*p = Variant(pj);
		VariantTypeChanger<Projection>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PROJECTION);
		CHECK(*p == Variant(Projection()));

		*p = Variant(true);
		VariantTypeChanger<Projection>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PROJECTION);
		CHECK(*p == Variant(Projection()));
	}

	SUBCASE("Transform2D") {
		VariantTypeChanger<Transform2D>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::TRANSFORM2D);
		CHECK(*p == Variant(Transform2D()));

		Transform2D t;
		t *= 2;
		*p = Variant(t);
		VariantTypeChanger<Transform2D>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::TRANSFORM2D);
		CHECK(*p == Variant(Transform2D()));

		*p = Variant(true);
		VariantTypeChanger<Transform2D>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::TRANSFORM2D);
		CHECK(*p == Variant(Transform2D()));
	}

	SUBCASE("Quaternion") {
		VariantTypeChanger<Quaternion>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::QUATERNION);
		CHECK(*p == Variant(Quaternion()));

		*p = Variant(Quaternion(1., 2., 3., 4.));
		VariantTypeChanger<Quaternion>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::QUATERNION);
		CHECK(*p == Variant(Quaternion()));

		*p = Variant(true);
		VariantTypeChanger<Quaternion>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::QUATERNION);
		CHECK(*p == Variant(Quaternion()));
	}

	SUBCASE("Plane") {
		VariantTypeChanger<Plane>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PLANE);
		CHECK(*p == Variant(Plane()));

		*p = Variant(Plane(Vector3(), 1.));
		VariantTypeChanger<Plane>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PLANE);
		CHECK(*p == Variant(Plane()));

		*p = Variant(true);
		VariantTypeChanger<Plane>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PLANE);
		CHECK(*p == Variant(Plane()));
	}

	SUBCASE("Basis") {
		VariantTypeChanger<Basis>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::BASIS);
		CHECK(*p == Variant(Basis()));

		*p = Variant(Basis::from_scale(Vector3::UP));
		VariantTypeChanger<Basis>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::BASIS);
		CHECK(*p == Variant(Basis()));

		*p = Variant(true);
		VariantTypeChanger<Basis>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::BASIS);
		CHECK(*p == Variant(Basis()));
	}

	SUBCASE("AABB") {
		VariantTypeChanger<AABB>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::AABB);
		CHECK(*p == Variant(AABB()));

		*p = Variant(AABB(Vector3::UP, Vector3::RIGHT));
		VariantTypeChanger<AABB>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::AABB);
		CHECK(*p == Variant(AABB()));

		*p = Variant(true);
		VariantTypeChanger<AABB>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::AABB);
		CHECK(*p == Variant(AABB()));
	}

	SUBCASE("Color") {
		VariantTypeChanger<Color>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::COLOR);
		CHECK(*p == Variant(Color()));

		*p = Variant(Color::from_hsv(0.5, 0.5, 0.5));
		VariantTypeChanger<Color>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::COLOR);
		CHECK(*p == Variant(Color()));

		*p = Variant(true);
		VariantTypeChanger<Color>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::COLOR);
		CHECK(*p == Variant(Color()));
	}

	SUBCASE("String") {
		VariantTypeChanger<String>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::STRING_NAME);
		CHECK(*p == Variant(String()));

		*p = Variant(String("abc"));
		VariantTypeChanger<String>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::STRING_NAME);
		CHECK(*p == Variant(String()));

		*p = Variant(true);
		VariantTypeChanger<String>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::STRING_NAME);
		CHECK(*p == Variant(String()));
	}

	SUBCASE("NodePath") {
		VariantTypeChanger<NodePath>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::NODE_PATH);
		CHECK(*p == Variant(NodePath()));

		*p = Variant(NodePath("abc"));
		VariantTypeChanger<NodePath>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::NODE_PATH);
		CHECK(*p == Variant(NodePath()));

		*p = Variant(true);
		VariantTypeChanger<NodePath>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::NODE_PATH);
		CHECK(*p == Variant(NodePath()));
	}

	SUBCASE("RID") {
		VariantTypeChanger<RID>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RID);
		CHECK(*p == Variant(RID()));

		*p = Variant(RID::from_uint64(123ull));
		VariantTypeChanger<RID>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RID);
		CHECK(*p == Variant(RID()));

		*p = Variant(true);
		VariantTypeChanger<RID>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::RID);
		CHECK(*p == Variant(RID()));
	}

	SUBCASE("Callable") {
		VariantTypeChanger<Callable>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::CALLABLE);
		CHECK(*p == Variant(Callable()));

		Object o;
		*p = Variant(Callable(&o, "abc"));
		VariantTypeChanger<Callable>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::CALLABLE);
		CHECK(*p == Variant(Callable()));

		*p = Variant(true);
		VariantTypeChanger<Callable>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::CALLABLE);
		CHECK(*p == Variant(Callable()));
	}

	SUBCASE("Signal") {
		VariantTypeChanger<Signal>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::SIGNAL);
		CHECK(*p == Variant(Signal()));

		Object o;
		*p = Variant(Signal(&o, "abc"));
		VariantTypeChanger<Signal>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::SIGNAL);
		CHECK(*p == Variant(Signal()));

		*p = Variant(true);
		VariantTypeChanger<Signal>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::SIGNAL);
		CHECK(*p == Variant(Signal()));
	}

	SUBCASE("Dictionary") {
		VariantTypeChanger<Dictionary>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::DICTIONARY);
		CHECK(*p == Variant(Dictionary()));

		*p = Variant(Dictionary{ KeyValue{ Variant(1), Variant(2) } });
		VariantTypeChanger<Dictionary>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::DICTIONARY);
		CHECK(*p == Variant(Dictionary()));

		*p = Variant(true);
		VariantTypeChanger<Dictionary>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::DICTIONARY);
		CHECK(*p == Variant(Dictionary()));
	}

	SUBCASE("Array") {
		VariantTypeChanger<Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::ARRAY);
		CHECK(*p == Variant(Array()));

		*p = Variant(Array{ Variant(1) });
		VariantTypeChanger<Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::ARRAY);
		CHECK(*p == Variant(Array()));

		*p = Variant(true);
		VariantTypeChanger<Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::ARRAY);
		CHECK(*p == Variant(Array()));
	}

	SUBCASE("PackedByteArray") {
		VariantTypeChanger<PackedByteArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_BYTE_ARRAY);
		CHECK(*p == Variant(PackedByteArray()));

		*p = Variant(PackedByteArray{ 1, 2 });
		VariantTypeChanger<PackedByteArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_BYTE_ARRAY);
		CHECK(*p == Variant(PackedByteArray()));

		*p = Variant(true);
		VariantTypeChanger<PackedByteArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_BYTE_ARRAY);
		CHECK(*p == Variant(PackedByteArray()));
	}

	SUBCASE("PackedInt32Array") {
		VariantTypeChanger<PackedInt32Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_INT32_ARRAY);
		CHECK(*p == Variant(PackedInt32Array()));

		*p = Variant(PackedInt32Array{ 1, 2 });
		VariantTypeChanger<PackedInt32Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_INT32_ARRAY);
		CHECK(*p == Variant(PackedInt32Array()));

		*p = Variant(true);
		VariantTypeChanger<PackedInt32Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_INT32_ARRAY);
		CHECK(*p == Variant(PackedInt32Array()));
	}

	SUBCASE("PackedInt64Array") {
		VariantTypeChanger<PackedInt64Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_INT64_ARRAY);
		CHECK(*p == Variant(PackedInt64Array()));

		*p = Variant(PackedInt64Array{ 1, 2 });
		VariantTypeChanger<PackedInt64Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_INT64_ARRAY);
		CHECK(*p == Variant(PackedInt64Array()));

		*p = Variant(true);
		VariantTypeChanger<PackedInt64Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_INT64_ARRAY);
		CHECK(*p == Variant(PackedInt64Array()));
	}

	SUBCASE("PackedFloat32Array") {
		VariantTypeChanger<PackedFloat32Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_FLOAT32_ARRAY);
		CHECK(*p == Variant(PackedFloat32Array()));

		*p = Variant(PackedFloat32Array{ 1, 2 });
		VariantTypeChanger<PackedFloat32Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_FLOAT32_ARRAY);
		CHECK(*p == Variant(PackedFloat32Array()));

		*p = Variant(true);
		VariantTypeChanger<PackedFloat32Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_FLOAT32_ARRAY);
		CHECK(*p == Variant(PackedFloat32Array()));
	}

	SUBCASE("PackedFloat64Array") {
		VariantTypeChanger<PackedFloat64Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_FLOAT64_ARRAY);
		CHECK(*p == Variant(PackedFloat64Array()));

		*p = Variant(PackedFloat64Array{ 1, 2 });
		VariantTypeChanger<PackedFloat64Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_FLOAT64_ARRAY);
		CHECK(*p == Variant(PackedFloat64Array()));

		*p = Variant(true);
		VariantTypeChanger<PackedFloat64Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_FLOAT64_ARRAY);
		CHECK(*p == Variant(PackedFloat64Array()));
	}

	SUBCASE("PackedStringArray") {
		VariantTypeChanger<PackedStringArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_STRING_ARRAY);
		CHECK(*p == Variant(PackedStringArray()));

		*p = Variant(PackedStringArray{ "1", "2" });
		VariantTypeChanger<PackedStringArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_STRING_ARRAY);
		CHECK(*p == Variant(PackedStringArray()));

		*p = Variant(true);
		VariantTypeChanger<PackedStringArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_STRING_ARRAY);
		CHECK(*p == Variant(PackedStringArray()));
	}

	SUBCASE("PackedVector2Array") {
		VariantTypeChanger<PackedVector2Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR2_ARRAY);
		CHECK(*p == Variant(PackedVector2Array()));

		*p = Variant(PackedVector2Array{ Vector2::UP, Vector2::RIGHT });
		VariantTypeChanger<PackedVector2Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR2_ARRAY);
		CHECK(*p == Variant(PackedVector2Array()));

		*p = Variant(true);
		VariantTypeChanger<PackedVector2Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR2_ARRAY);
		CHECK(*p == Variant(PackedVector2Array()));
	}

	SUBCASE("PackedVector3Array") {
		VariantTypeChanger<PackedVector3Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR3_ARRAY);
		CHECK(*p == Variant(PackedVector3Array()));

		*p = Variant(PackedVector3Array{ Vector3::UP, Vector3::RIGHT });
		VariantTypeChanger<PackedVector3Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR3_ARRAY);
		CHECK(*p == Variant(PackedVector3Array()));

		*p = Variant(true);
		VariantTypeChanger<PackedVector3Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR3_ARRAY);
		CHECK(*p == Variant(PackedVector3Array()));
	}

	SUBCASE("PackedColorArray") {
		VariantTypeChanger<PackedColorArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_COLOR_ARRAY);
		CHECK(*p == Variant(PackedColorArray()));

		*p = Variant(PackedColorArray{ Color::from_hsv(0., .1, .2) });
		VariantTypeChanger<PackedColorArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_COLOR_ARRAY);
		CHECK(*p == Variant(PackedColorArray()));

		*p = Variant(true);
		VariantTypeChanger<PackedColorArray>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_COLOR_ARRAY);
		CHECK(*p == Variant(PackedColorArray()));
	}

	SUBCASE("PackedVector4Array") {
		VariantTypeChanger<PackedVector4Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR4_ARRAY);
		CHECK(*p == Variant(PackedVector4Array()));

		*p = Variant(PackedVector4Array{ Vector4(1, 2, 3, 4) });
		VariantTypeChanger<PackedVector4Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR4_ARRAY);
		CHECK(*p == Variant(PackedVector4Array()));

		*p = Variant(true);
		VariantTypeChanger<PackedVector4Array>::change_and_reset(p);
		CHECK(p->get_type() == Variant::Type::PACKED_VECTOR4_ARRAY);
		CHECK(*p == Variant(PackedVector4Array()));
	}

	delete p;
}

} // namespace TestVariantInitialization

// === Merged from tests/core/variant/test_variant_utility.cpp ===
/*  test_variant_utility.cpp                                              */
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




namespace TestVariantUtility {

TEST_CASE("[VariantUtility] Type conversion") {
	Variant converted;
	converted = VariantUtilityFunctions::type_convert("Hi!", Variant::Type::NIL);
	CHECK(converted.get_type() == Variant::Type::NIL);
	CHECK(converted == Variant());

	converted = VariantUtilityFunctions::type_convert("Hi!", Variant::Type::INT);
	CHECK(converted.get_type() == Variant::Type::INT);
	CHECK(converted == Variant(0));

	converted = VariantUtilityFunctions::type_convert("123", Variant::Type::INT);
	CHECK(converted.get_type() == Variant::Type::INT);
	CHECK(converted == Variant(123));

	converted = VariantUtilityFunctions::type_convert(123, Variant::Type::STRING);
	CHECK(converted.get_type() == Variant::Type::STRING);
	CHECK(converted == Variant("123"));

	converted = VariantUtilityFunctions::type_convert(123.4, Variant::Type::INT);
	CHECK(converted.get_type() == Variant::Type::INT);
	CHECK(converted == Variant(123));

	converted = VariantUtilityFunctions::type_convert(5, Variant::Type::VECTOR2);
	CHECK(converted.get_type() == Variant::Type::VECTOR2);
	CHECK(converted == Variant(Vector2(0, 0)));

	converted = VariantUtilityFunctions::type_convert(Vector3(1, 2, 3), Variant::Type::VECTOR2);
	CHECK(converted.get_type() == Variant::Type::VECTOR2);
	CHECK(converted == Variant(Vector2(1, 2)));

	converted = VariantUtilityFunctions::type_convert(Vector2(1, 2), Variant::Type::VECTOR4);
	CHECK(converted.get_type() == Variant::Type::VECTOR4);
	CHECK(converted == Variant(Vector4(1, 2, 0, 0)));

	converted = VariantUtilityFunctions::type_convert(Vector4(1.2, 3.4, 5.6, 7.8), Variant::Type::VECTOR3I);
	CHECK(converted.get_type() == Variant::Type::VECTOR3I);
	CHECK(converted == Variant(Vector3i(1, 3, 5)));

	{
		Basis basis = Basis::from_scale(Vector3(1.2, 3.4, 5.6));
		Transform3D transform = Transform3D(basis, Vector3());

		converted = VariantUtilityFunctions::type_convert(transform, Variant::Type::BASIS);
		CHECK(converted.get_type() == Variant::Type::BASIS);
		CHECK(converted == basis);

		converted = VariantUtilityFunctions::type_convert(basis, Variant::Type::TRANSFORM3D);
		CHECK(converted.get_type() == Variant::Type::TRANSFORM3D);
		CHECK(converted == transform);

		converted = VariantUtilityFunctions::type_convert(basis, Variant::Type::STRING);
		CHECK(converted.get_type() == Variant::Type::STRING);
		CHECK(converted == Variant("[X: (1.2, 0.0, 0.0), Y: (0.0, 3.4, 0.0), Z: (0.0, 0.0, 5.6)]"));
	}

	{
		Array arr = { 1.2, 3.4, 5.6 };
		PackedFloat64Array packed = { 1.2, 3.4, 5.6 };

		converted = VariantUtilityFunctions::type_convert(arr, Variant::Type::PACKED_FLOAT64_ARRAY);
		CHECK(converted.get_type() == Variant::Type::PACKED_FLOAT64_ARRAY);
		CHECK(converted == packed);

		converted = VariantUtilityFunctions::type_convert(packed, Variant::Type::ARRAY);
		CHECK(converted.get_type() == Variant::Type::ARRAY);
		CHECK(converted == arr);
	}

	{
		// Check that using Variant::call_utility_function also works.
		Vector<const Variant *> args;
		Variant data_arg = "Hi!";
		args.push_back(&data_arg);
		Variant type_arg = Variant::Type::NIL;
		args.push_back(&type_arg);
		Callable::CallError call_error;
		Variant::call_utility_function("type_convert", &converted, (const Variant **)args.ptr(), 2, call_error);
		CHECK(converted.get_type() == Variant::Type::NIL);
		CHECK(converted == Variant());

		type_arg = Variant::Type::INT;
		Variant::call_utility_function("type_convert", &converted, (const Variant **)args.ptr(), 2, call_error);
		CHECK(converted.get_type() == Variant::Type::INT);
		CHECK(converted == Variant(0));

		data_arg = "123";
		Variant::call_utility_function("type_convert", &converted, (const Variant **)args.ptr(), 2, call_error);
		CHECK(converted.get_type() == Variant::Type::INT);
		CHECK(converted == Variant(123));
	}
}

} // namespace TestVariantUtility

TEST_FORCE_LINK(test_core)

