/**************************************************************************/
/*  test_file_access.cpp                                                  */
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



#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "tests/test_macros.h"

namespace TestFileAccess {

TEST_CASE("[FileAccess] CSV read") {
	Ref<FileAccess> f = FileAccess::open(TestUtils::get_data_path("testdata.csv"), FileAccess::READ);
	REQUIRE(f.is_valid());

	Vector<String> header = f->get_csv_line(); // Default delimiter: ",".
	REQUIRE(header.size() == 4);

	Vector<String> row1 = f->get_csv_line(","); // Explicit delimiter, should be the same.
	REQUIRE(row1.size() == 4);
	CHECK(row1[0] == "GOOD_MORNING");
	CHECK(row1[1] == "Good Morning");
	CHECK(row1[2] == "Guten Morgen");
	CHECK(row1[3] == "Bonjour");

	Vector<String> row2 = f->get_csv_line();
	REQUIRE(row2.size() == 4);
	CHECK(row2[0] == "GOOD_EVENING");
	CHECK(row2[1] == "Good Evening");
	CHECK(row2[2].is_empty()); // Use case: not yet translated!
	// https://github.com/godotengine/godot/issues/44269
	CHECK_MESSAGE(row2[2] != "\"", "Should not parse empty string as a single double quote.");
	CHECK(row2[3] == "\"\""); // Intentionally testing only escaped double quotes.

	Vector<String> row3 = f->get_csv_line();
	REQUIRE(row3.size() == 6);
	CHECK(row3[0] == "Without quotes");
	CHECK(row3[1] == "With, comma");
	CHECK(row3[2] == "With \"inner\" quotes");
	CHECK(row3[3] == "With \"inner\", quotes\",\" and comma");
	CHECK(row3[4] == "With \"inner\nsplit\" quotes and\nline breaks");
	CHECK(row3[5] == "With \\nnewline chars"); // Escaped, not an actual newline.

	Vector<String> row4 = f->get_csv_line("~"); // Custom delimiter, makes inline commas easier.
	REQUIRE(row4.size() == 3);
	CHECK(row4[0] == "Some other");
	CHECK(row4[1] == "delimiter");
	CHECK(row4[2] == "should still work, shouldn't it?");

	Vector<String> row5 = f->get_csv_line("\t"); // Tab separated variables.
	REQUIRE(row5.size() == 3);
	CHECK(row5[0] == "What about");
	CHECK(row5[1] == "tab separated");
	CHECK(row5[2] == "lines, good?");
}

TEST_CASE("[FileAccess] Get as UTF-8 String") {
	SUBCASE("Newline == \\n (Unix)") {
		Ref<FileAccess> f_lf = FileAccess::open(TestUtils::get_data_path("line_endings_lf.test.txt"), FileAccess::READ);
		REQUIRE(f_lf.is_valid());
		String s_lf = f_lf->get_as_utf8_string();
		CHECK(s_lf == "Hello darkness\nMy old friend\nI've come to talk\nWith you again\n");
		f_lf->seek(0);
		CHECK(f_lf->get_line() == "Hello darkness");
		CHECK(f_lf->get_line() == "My old friend");
		CHECK(f_lf->get_line() == "I've come to talk");
		CHECK(f_lf->get_line() == "With you again");
		CHECK(f_lf->get_error() == Error::OK);
	}

	SUBCASE("Newline == \\r\\n (Windows)") {
		Ref<FileAccess> f_crlf = FileAccess::open(TestUtils::get_data_path("line_endings_crlf.test.txt"), FileAccess::READ);
		REQUIRE(f_crlf.is_valid());
		String s_crlf = f_crlf->get_as_utf8_string();
		CHECK(s_crlf == "Hello darkness\r\nMy old friend\r\nI've come to talk\r\nWith you again\r\n");
		f_crlf->seek(0);
		CHECK(f_crlf->get_line() == "Hello darkness");
		CHECK(f_crlf->get_line() == "My old friend");
		CHECK(f_crlf->get_line() == "I've come to talk");
		CHECK(f_crlf->get_line() == "With you again");
		CHECK(f_crlf->get_error() == Error::OK);
	}

	SUBCASE("Newline == \\r (Legacy macOS)") {
		Ref<FileAccess> f_cr = FileAccess::open(TestUtils::get_data_path("line_endings_cr.test.txt"), FileAccess::READ);
		REQUIRE(f_cr.is_valid());
		String s_cr = f_cr->get_as_utf8_string();
		CHECK(s_cr == "Hello darkness\rMy old friend\rI've come to talk\rWith you again\r");
		f_cr->seek(0);
		CHECK(f_cr->get_line() == "Hello darkness");
		CHECK(f_cr->get_line() == "My old friend");
		CHECK(f_cr->get_line() == "I've come to talk");
		CHECK(f_cr->get_line() == "With you again");
		CHECK(f_cr->get_error() == Error::OK);
	}

	SUBCASE("Newline == Mixed") {
		Ref<FileAccess> f_mix = FileAccess::open(TestUtils::get_data_path("line_endings_mixed.test.txt"), FileAccess::READ);
		REQUIRE(f_mix.is_valid());
		String s_mix = f_mix->get_as_utf8_string();
		CHECK(s_mix == "Hello darkness\nMy old friend\r\nI've come to talk\rWith you again");
		f_mix->seek(0);
		CHECK(f_mix->get_line() == "Hello darkness");
		CHECK(f_mix->get_line() == "My old friend");
		CHECK(f_mix->get_line() == "I've come to talk");
		CHECK(f_mix->get_line() == "With you again");
		CHECK(f_mix->get_error() == Error::ERR_FILE_EOF); // Not a bug; the file lacks a final newline.
	}
}

TEST_CASE("[FileAccess] Get/Store floating point values") {
	// BigEndian Hex: 0x40490E56
	// LittleEndian Hex: 0x560E4940
	float value = 3.1415f;

	SUBCASE("Little Endian") {
		const String file_path = TestUtils::get_data_path("floating_point_little_endian.bin");
		const String file_path_new = TestUtils::get_data_path("floating_point_little_endian_new.bin");

		Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
		REQUIRE(f.is_valid());
		CHECK_EQ(f->get_float(), value);

		Ref<FileAccess> fw = FileAccess::open(file_path_new, FileAccess::WRITE);
		REQUIRE(fw.is_valid());
		fw->store_float(value);
		fw->close();

		CHECK_EQ(FileAccess::get_sha256(file_path_new), FileAccess::get_sha256(file_path));

		DirAccess::remove_file_or_error(file_path_new);
	}

	SUBCASE("Big Endian") {
		const String file_path = TestUtils::get_data_path("floating_point_big_endian.bin");
		const String file_path_new = TestUtils::get_data_path("floating_point_big_endian_new.bin");

		Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
		REQUIRE(f.is_valid());
		f->set_big_endian(true);
		CHECK_EQ(f->get_float(), value);

		Ref<FileAccess> fw = FileAccess::open(file_path_new, FileAccess::WRITE);
		REQUIRE(fw.is_valid());
		fw->set_big_endian(true);
		fw->store_float(value);
		fw->close();

		CHECK_EQ(FileAccess::get_sha256(file_path_new), FileAccess::get_sha256(file_path));

		DirAccess::remove_file_or_error(file_path_new);
	}
}

TEST_CASE("[FileAccess] Get/Store floating point half precision values") {
	// IEEE 754 half-precision binary floating-point format:
	// sign exponent (5 bits)    fraction (10 bits)
	//  0        01101               0101010101
	// BigEndian Hex: 0x3555
	// LittleEndian Hex: 0x5535
	float value = 0.33325195f;

	SUBCASE("Little Endian") {
		const String file_path = TestUtils::get_data_path("half_precision_floating_point_little_endian.bin");
		const String file_path_new = TestUtils::get_data_path("half_precision_floating_point_little_endian_new.bin");

		Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
		REQUIRE(f.is_valid());
		CHECK_EQ(f->get_half(), value);

		Ref<FileAccess> fw = FileAccess::open(file_path_new, FileAccess::WRITE);
		REQUIRE(fw.is_valid());
		fw->store_half(value);
		fw->close();

		CHECK_EQ(FileAccess::get_sha256(file_path_new), FileAccess::get_sha256(file_path));

		DirAccess::remove_file_or_error(file_path_new);
	}

	SUBCASE("Big Endian") {
		const String file_path = TestUtils::get_data_path("half_precision_floating_point_big_endian.bin");
		const String file_path_new = TestUtils::get_data_path("half_precision_floating_point_big_endian_new.bin");

		Ref<FileAccess> f = FileAccess::open(file_path, FileAccess::READ);
		REQUIRE(f.is_valid());
		f->set_big_endian(true);
		CHECK_EQ(f->get_half(), value);

		Ref<FileAccess> fw = FileAccess::open(file_path_new, FileAccess::WRITE);
		REQUIRE(fw.is_valid());
		fw->set_big_endian(true);
		fw->store_half(value);
		fw->close();

		CHECK_EQ(FileAccess::get_sha256(file_path_new), FileAccess::get_sha256(file_path));

		DirAccess::remove_file_or_error(file_path_new);
	}

	SUBCASE("4096 bytes fastlz compressed") {
		const String file_path = TestUtils::get_data_path("exactly_4096_bytes_fastlz.bin");

		Ref<FileAccess> f = FileAccess::open_compressed(file_path, FileAccess::READ, FileAccess::COMPRESSION_FASTLZ);
		const Vector<uint8_t> full_data = f->get_buffer(4096 * 2);
		CHECK(full_data.size() == 4096);
		CHECK(f->eof_reached());

		// Data should be empty.
		PackedByteArray reference;
		reference.resize_initialized(4096);
		CHECK(reference == full_data);

		f->seek(0);
		const Vector<uint8_t> partial_data = f->get_buffer(4095);
		CHECK(partial_data.size() == 4095);
		CHECK(!f->eof_reached());

		reference.resize_initialized(4095);
		CHECK(reference == partial_data);
	}
}

TEST_CASE("[FileAccess] Cursor positioning") {
	Ref<FileAccess> f = FileAccess::open(TestUtils::get_data_path("line_endings_lf.test.txt"), FileAccess::READ);
	REQUIRE(f.is_valid());

	String full = f->get_as_utf8_string();
	int64_t len = full.length();

	SUBCASE("Initial position is zero") {
		f->seek(0);
		CHECK(f->get_position() == 0);
	}

	SUBCASE("seek() moves cursor to absolute position") {
		f->seek(5);
		CHECK(f->get_position() == 5);
	}

	SUBCASE("seek() moves cursor beyond file size") {
		f->seek(len + 10);
		CHECK(f->get_position() == len + 10);
	}

	SUBCASE("seek_end() moves cursor to end of file") {
		f->seek_end(0);
		CHECK(f->get_position() == len);
	}

	SUBCASE("seek_end() with positive offset") {
		f->seek_end(1);
		CHECK(f->get_position() == len + 1);
	}

	SUBCASE("seek_end() with negative offset") {
		f->seek_end(-1);
		CHECK(f->get_position() == len - 1);

		char last_char = full[full.length() - 1];
		CHECK(f->get_8() == last_char);
	}

	SUBCASE("seek_end() beyond file size") {
		f->seek(5);
		f->seek_end(-len - 10); // seeking to a position below 0; ignored.
		CHECK(f->get_position() == 5);
	}
}

} // namespace TestFileAccess

// === Merged from tests/core/io/test_config_file.cpp ===
/*  test_config_file.cpp                                                  */
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

#ifdef WINDOWS_ENABLED
#include "core/os/os.h"
#endif

namespace TestConfigFile {

TEST_CASE("[ConfigFile] Parsing well-formatted files") {
	ConfigFile config_file;
	// Formatting is intentionally hand-edited to see how human-friendly the parser is.
	const Error error = config_file.parse(R"(
[player]

name = "Unnamed Player"
tagline="Waiting
for
Godot"

color =Color(   0, 0.5,1, 1) ; Inline comment
position= Vector2(
	3,
	4
)

[graphics]

antialiasing = true

; Testing comments and case-sensitivity...
antiAliasing = false
)");

	CHECK_MESSAGE(error == OK, "The configuration file should parse successfully.");
	CHECK_MESSAGE(
			String(config_file.get_value("player", "name")) == "Unnamed Player",
			"Reading `player/name` should return the expected value.");
	CHECK_MESSAGE(
			String(config_file.get_value("player", "tagline")) == "Waiting\nfor\nGodot",
			"Reading `player/tagline` should return the expected value.");
	CHECK_MESSAGE(
			Color(config_file.get_value("player", "color")).is_equal_approx(Color(0, 0.5, 1)),
			"Reading `player/color` should return the expected value.");
	CHECK_MESSAGE(
			Vector2(config_file.get_value("player", "position")).is_equal_approx(Vector2(3, 4)),
			"Reading `player/position` should return the expected value.");
	CHECK_MESSAGE(
			bool(config_file.get_value("graphics", "antialiasing")),
			"Reading `graphics/antialiasing` should return `true`.");
	CHECK_MESSAGE(
			bool(config_file.get_value("graphics", "antiAliasing")) == false,
			"Reading `graphics/antiAliasing` should return `false`.");

	// An empty ConfigFile is valid.
	const Error error_empty = config_file.parse("");
	CHECK_MESSAGE(error_empty == OK,
			"An empty configuration file should parse successfully.");
}

TEST_CASE("[ConfigFile] Parsing malformatted file") {
	ConfigFile config_file;
	ERR_PRINT_OFF;
	const Error error = config_file.parse(R"(
[player]

name = "Unnamed Player"" ; Extraneous closing quote.
tagline = "Waiting\nfor\nGodot"

color = Color(0, 0.5, 1) ; Missing 4th parameter.
position = Vector2(
	3,,
	4
) ; Extraneous comma.

[graphics]

antialiasing = true
antialiasing = false ; Duplicate key.
)");
	ERR_PRINT_ON;

	CHECK_MESSAGE(error == ERR_PARSE_ERROR,
			"The configuration file shouldn't parse successfully.");
}

TEST_CASE("[ConfigFile] Saving file") {
	ConfigFile config_file;
	config_file.set_value("player", "name", "Unnamed Player");
	config_file.set_value("player", "tagline", "Waiting\nfor\nGodot");
	config_file.set_value("player", "color", Color(0, 0.5, 1));
	config_file.set_value("player", "position", Vector2(3, 4));
	config_file.set_value("graphics", "antialiasing", true);
	config_file.set_value("graphics", "antiAliasing", false);
	config_file.set_value("quoted", String::utf8("静音"), 42);
	config_file.set_value("quoted", "a=b", 7);

#ifdef WINDOWS_ENABLED
	const String config_path = OS::get_environment("TEMP").path_join("config.ini");
#else
	const String config_path = "/tmp/config.ini";
#endif

	config_file.save(config_path);

	// Expected contents of the saved ConfigFile.
	const String contents = String::utf8(R"([player]

name="Unnamed Player"
tagline="Waiting
for
Godot"
color=Color(0, 0.5, 1, 1)
position=Vector2(3, 4)

[graphics]

antialiasing=true
antiAliasing=false

[quoted]

"静音"=42
"a=b"=7
)");

	Ref<FileAccess> file = FileAccess::open(config_path, FileAccess::READ);
	CHECK_MESSAGE(file->get_as_utf8_string() == contents,
			"The saved configuration file should match the expected format.");
}

} // namespace TestConfigFile

// === Merged from tests/core/io/test_http_client.cpp ===
/*  test_http_client.cpp                                                  */
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

#include "modules/modules_enabled.gen.h" // For mbedtls.

namespace TestHTTPClient {

TEST_CASE("[HTTPClient] Instantiation") {
	Ref<HTTPClient> client = HTTPClient::create();
	CHECK_MESSAGE(client.is_valid(), "A HTTP Client created should not be a null pointer");
}

TEST_CASE("[HTTPClient] query_string_from_dict") {
	Ref<HTTPClient> client = HTTPClient::create();
	Dictionary empty_dict;
	String empty_query = client->query_string_from_dict(empty_dict);
	CHECK_MESSAGE(empty_query.is_empty(), "A empty dictionary should return a empty string");

	Dictionary dict1;
	dict1["key"] = "value";
	String single_key = client->query_string_from_dict(dict1);
	CHECK_MESSAGE(single_key == "key=value", "The query should return key=value for every string in the dictionary");

	// Check Dictionary with multiple values of different types.
	Dictionary dict2;
	dict2["key1"] = "value";
	dict2["key2"] = 123;
	Array values = { 1, 2, 3 };
	dict2["key3"] = values;
	dict2["key4"] = Variant();
	String multiple_keys = client->query_string_from_dict(dict2);
	CHECK_MESSAGE(multiple_keys == "key1=value&key2=123&key3=1&key3=2&key3=3&key4",
			"The query should return key=value for every string in the dictionary. Pairs should be separated by &, arrays should be have a query for every element, and variants should have empty values");
}

TEST_CASE("[HTTPClient] verify_headers") {
	Ref<HTTPClient> client = HTTPClient::create();
	Vector<String> headers = { "Accept: text/html", "Content-Type: application/json", "Authorization: Bearer abc123" };

	Error err = client->verify_headers(headers);
	CHECK_MESSAGE(err == OK, "Expected OK for valid headers");

	ERR_PRINT_OFF;
	Vector<String> empty_header = { "" };
	err = client->verify_headers(empty_header);
	CHECK_MESSAGE(err == ERR_INVALID_PARAMETER, "Expected ERR_INVALID_PARAMETER for empty header");

	Vector<String> invalid_header = { "InvalidHeader", "Header: " };
	err = client->verify_headers(invalid_header);
	CHECK_MESSAGE(err == ERR_INVALID_PARAMETER, "Expected ERR_INVALID_PARAMETER for header with no colon");

	Vector<String> invalid_header_b = { ":", "Header: " };
	err = client->verify_headers(invalid_header_b);
	CHECK_MESSAGE(err == ERR_INVALID_PARAMETER, "Expected ERR_INVALID_PARAMETER for header with colon in first position");
	ERR_PRINT_ON;
}

#if defined(MODULE_MBEDTLS_ENABLED) || defined(WEB_ENABLED)
TEST_CASE("[HTTPClient] connect_to_host") {
	Ref<HTTPClient> client = HTTPClient::create();
	String host = "https://www.example.com";
	int port = 443;
	Ref<TLSOptions> tls_options;

	// Connect to host.
	Error err = client->connect_to_host(host, port, tls_options);
	CHECK_MESSAGE(err == OK, "Expected OK for successful connection");
}
#endif // defined(MODULE_MBEDTLS_ENABLED) || defined(WEB_ENABLED)

} // namespace TestHTTPClient

// === Merged from tests/core/io/test_image.cpp ===
/*  test_image.cpp                                                        */
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
#include "core/io/image.h"
#include "tests/test_macros.h"

#include "modules/modules_enabled.gen.h" // For bmp, jpg, svg, webp, tga.

namespace TestImage {

TEST_CASE("[Image] Instantiation") {
	Ref<Image> image = memnew(Image(8, 4, false, Image::FORMAT_RGBA8));
	CHECK_MESSAGE(
			!image->is_empty(),
			"An image created with specified size and format should not be empty at first.");
	CHECK_MESSAGE(
			image->is_invisible(),
			"A newly created image should be invisible.");
	CHECK_MESSAGE(
			!image->is_compressed(),
			"A newly created image should not be compressed.");
	CHECK(!image->has_mipmaps());

	PackedByteArray image_data = image->get_data();
	for (int i = 0; i < image_data.size(); i++) {
		CHECK_MESSAGE(
				image_data[i] == 0,
				"An image created without data specified should have its data zeroed out.");
	}

	Ref<Image> image_copy = memnew(Image());
	CHECK_MESSAGE(
			image_copy->is_empty(),
			"An image created without any specified size and format be empty at first.");
	image_copy->copy_internals_from(image);

	CHECK_MESSAGE(
			image->get_data() == image_copy->get_data(),
			"Duplicated images should have the same data.");

	image_data = image->get_data();
	Ref<Image> image_from_data = memnew(Image(8, 4, false, Image::FORMAT_RGBA8, image_data));
	CHECK_MESSAGE(
			image->get_data() == image_from_data->get_data(),
			"An image created from data of another image should have the same data of the original image.");
}

TEST_CASE("[Image] Saving and loading") {
	Ref<Image> image = memnew(Image(4, 4, false, Image::FORMAT_RGBA8));
	const String save_path_png = TestUtils::get_temp_path("image.png");
#ifdef TOOLS_ENABLED
	const String save_path_exr = TestUtils::get_temp_path("image.exr");
#endif // TOOLS_ENABLED

	// Save PNG
	Error err;
	err = image->save_png(save_path_png);
	CHECK_MESSAGE(
			err == OK,
			"The image should be saved successfully as a .png file.");

	// Only available on editor builds.
#ifdef TOOLS_ENABLED
	// Save EXR
	err = image->save_exr(save_path_exr, false);
	CHECK_MESSAGE(
			err == OK,
			"The image should be saved successfully as an .exr file.");
#endif // TOOLS_ENABLED

	// Load using load()
	Ref<Image> image_load = memnew(Image());
	err = image_load->load(save_path_png);
	CHECK_MESSAGE(
			err == OK,
			"The image should load successfully using load().");
	CHECK_MESSAGE(
			image->get_data() == image_load->get_data(),
			"The loaded image should have the same data as the one that got saved.");

#ifdef MODULE_BMP_ENABLED
	// Load BMP
	Ref<Image> image_bmp = memnew(Image());
	Ref<FileAccess> f_bmp = FileAccess::open(TestUtils::get_data_path("images/icon.bmp"), FileAccess::READ, &err);
	REQUIRE(f_bmp.is_valid());
	PackedByteArray data_bmp;
	data_bmp.resize(f_bmp->get_length() + 1);
	f_bmp->get_buffer(data_bmp.ptrw(), f_bmp->get_length());
	CHECK_MESSAGE(
			image_bmp->load_bmp_from_buffer(data_bmp) == OK,
			"The BMP image should load successfully.");
#endif // MODULE_BMP_ENABLED

#ifdef MODULE_EXR_ENABLED
	// Load EXR
	Ref<Image> image_exr;
	image_exr.instantiate();
	Ref<FileAccess> f_exr = FileAccess::open(TestUtils::get_data_path("images/icon.exr"), FileAccess::READ, &err);
	REQUIRE(f_exr.is_valid());
	PackedByteArray data_exr;
	data_exr.resize(f_exr->get_length() + 1);
	f_exr->get_buffer(data_exr.ptrw(), f_exr->get_length());
	CHECK_MESSAGE(
			image_exr->load_exr_from_buffer(data_exr) == OK,
			"The EXR image should load successfully.");
#endif // MODULE_EXR_ENABLED

#ifdef MODULE_JPG_ENABLED
	// Load JPG
	Ref<Image> image_jpg = memnew(Image());
	Ref<FileAccess> f_jpg = FileAccess::open(TestUtils::get_data_path("images/icon.jpg"), FileAccess::READ, &err);
	REQUIRE(f_jpg.is_valid());
	PackedByteArray data_jpg;
	data_jpg.resize(f_jpg->get_length() + 1);
	f_jpg->get_buffer(data_jpg.ptrw(), f_jpg->get_length());
	CHECK_MESSAGE(
			image_jpg->load_jpg_from_buffer(data_jpg) == OK,
			"The JPG image should load successfully.");

	Ref<Image> image_grayscale_jpg = memnew(Image());
	Ref<FileAccess> f_grayscale_jpg = FileAccess::open(TestUtils::get_data_path("images/grayscale.jpg"), FileAccess::READ, &err);
	REQUIRE(f_grayscale_jpg.is_valid());
	PackedByteArray data_grayscale_jpg;
	data_grayscale_jpg.resize(f_grayscale_jpg->get_length() + 1);
	f_grayscale_jpg->get_buffer(data_grayscale_jpg.ptrw(), f_grayscale_jpg->get_length());
	CHECK_MESSAGE(
			image_jpg->load_jpg_from_buffer(data_grayscale_jpg) == OK,
			"The grayscale JPG image should load successfully.");

	// Save JPG
	const String save_path_jpg = TestUtils::get_temp_path("image.jpg");
	CHECK_MESSAGE(image->save_jpg(save_path_jpg) == OK,
			"The image should be saved successfully as a .jpg file.");

#ifdef MODULE_SVG_ENABLED
	// Load SVG with embedded jpg image
	Ref<Image> image_svg = memnew(Image());
	Ref<FileAccess> f_svg = FileAccess::open(TestUtils::get_data_path("images/embedded_jpg.svg"), FileAccess::READ, &err);
	REQUIRE(f_svg.is_valid());
	PackedByteArray data_svg;
	data_svg.resize(f_svg->get_length() + 1);
	f_svg->get_buffer(data_svg.ptrw(), f_svg->get_length());
	CHECK_MESSAGE(
			image_svg->load_svg_from_buffer(data_svg) == OK,
			"The SVG image should load successfully.");
#endif // MODULE_SVG_ENABLED
#endif // MODULE_JPG_ENABLED

#ifdef MODULE_WEBP_ENABLED
	// Load WebP
	Ref<Image> image_webp = memnew(Image());
	Ref<FileAccess> f_webp = FileAccess::open(TestUtils::get_data_path("images/icon.webp"), FileAccess::READ, &err);
	REQUIRE(f_webp.is_valid());
	PackedByteArray data_webp;
	data_webp.resize(f_webp->get_length() + 1);
	f_webp->get_buffer(data_webp.ptrw(), f_webp->get_length());
	CHECK_MESSAGE(
			image_webp->load_webp_from_buffer(data_webp) == OK,
			"The WebP image should load successfully.");
#endif // MODULE_WEBP_ENABLED

	// Load PNG
	Ref<Image> image_png = memnew(Image());
	Ref<FileAccess> f_png = FileAccess::open(TestUtils::get_data_path("images/icon.png"), FileAccess::READ, &err);
	REQUIRE(f_png.is_valid());
	PackedByteArray data_png;
	data_png.resize(f_png->get_length() + 1);
	f_png->get_buffer(data_png.ptrw(), f_png->get_length());
	CHECK_MESSAGE(
			image_png->load_png_from_buffer(data_png) == OK,
			"The PNG image should load successfully.");

#ifdef MODULE_TGA_ENABLED
	// Load TGA
	Ref<Image> image_tga = memnew(Image());
	Ref<FileAccess> f_tga = FileAccess::open(TestUtils::get_data_path("images/icon.tga"), FileAccess::READ, &err);
	REQUIRE(f_tga.is_valid());
	PackedByteArray data_tga;
	data_tga.resize(f_tga->get_length() + 1);
	f_tga->get_buffer(data_tga.ptrw(), f_tga->get_length());
	CHECK_MESSAGE(
			image_tga->load_tga_from_buffer(data_tga) == OK,
			"The TGA image should load successfully.");
#endif // MODULE_TGA_ENABLED
}

TEST_CASE("[Image] Basic getters") {
	Ref<Image> image = memnew(Image(8, 4, false, Image::FORMAT_LA8));
	CHECK(image->get_width() == 8);
	CHECK(image->get_height() == 4);
	CHECK(image->get_size() == Vector2(8, 4));
	CHECK(image->get_format() == Image::FORMAT_LA8);
	CHECK(image->get_used_rect() == Rect2i(0, 0, 0, 0));
	Ref<Image> image_get_rect = image->get_region(Rect2i(0, 0, 2, 1));
	CHECK(image_get_rect->get_size() == Vector2(2, 1));
}

TEST_CASE("[Image] Resizing") {
	Ref<Image> image = memnew(Image(8, 8, false, Image::FORMAT_RGBA8));
	// Crop
	image->crop(4, 4);
	CHECK_MESSAGE(
			image->get_size() == Vector2(4, 4),
			"get_size() should return the correct size after cropping.");
	image->set_pixel(0, 0, Color(1, 1, 1, 1));

	// Resize
	for (int i = 0; i < 5; i++) {
		Ref<Image> image_resized = memnew(Image());
		image_resized->copy_internals_from(image);
		Image::Interpolation interpolation = static_cast<Image::Interpolation>(i);
		image_resized->resize(8, 8, interpolation);
		CHECK_MESSAGE(
				image_resized->get_size() == Vector2(8, 8),
				"get_size() should return the correct size after resizing.");
		CHECK_MESSAGE(
				image_resized->get_pixel(1, 1).a > 0,
				"Resizing an image should also affect its content.");
	}

	// shrink_x2()
	image->shrink_x2();
	CHECK_MESSAGE(
			image->get_size() == Vector2(2, 2),
			"get_size() should return the correct size after shrink_x2().");

	// resize_to_po2()
	Ref<Image> image_po_2 = memnew(Image(14, 28, false, Image::FORMAT_RGBA8));
	image_po_2->resize_to_po2();
	CHECK_MESSAGE(
			image_po_2->get_size() == Vector2(16, 32),
			"get_size() should return the correct size after resize_to_po2().");
}

TEST_CASE("[Image] Modifying pixels of an image") {
	Ref<Image> image = memnew(Image(3, 3, false, Image::FORMAT_RGBA8));
	image->set_pixel(0, 0, Color(1, 1, 1, 1));
	CHECK_MESSAGE(
			!image->is_invisible(),
			"Image should not be invisible after drawing on it.");
	CHECK_MESSAGE(
			image->get_pixelv(Vector2(0, 0)).is_equal_approx(Color(1, 1, 1, 1)),
			"Image's get_pixel() should return the same color value as the one being set with set_pixel() in the same position.");
	CHECK_MESSAGE(
			image->get_used_rect() == Rect2i(0, 0, 1, 1),
			"Image's get_used_rect should return the expected value, larger than Rect2i(0, 0, 0, 0) if it's visible.");

	image->set_pixelv(Vector2(0, 0), Color(0.5, 0.5, 0.5, 0.5));
	Ref<Image> image2 = memnew(Image(3, 3, false, Image::FORMAT_RGBA8));

	// Fill image with color
	image2->fill(Color(0.5, 0.5, 0.5, 0.5));
	for (int m = 0; m < image2->get_mipmap_count(); m++) {
		Ref<Image> mip_image = image2->get_image_from_mipmap(m);

		for (int y = 0; y < mip_image->get_height(); y++) {
			for (int x = 0; x < mip_image->get_width(); x++) {
				CHECK_MESSAGE(
						mip_image->get_pixel(x, y).r > 0.49,
						"fill() should colorize all pixels of the image.");
			}
		}
	}

	// Fill rect with color
	{
		const int img_width = 3;
		const int img_height = 3;
		Vector<Rect2i> rects;
		rects.push_back(Rect2i());
		rects.push_back(Rect2i(-5, -5, 3, 3));
		rects.push_back(Rect2i(img_width, 0, 12, 12));
		rects.push_back(Rect2i(0, img_height, 12, 12));
		rects.push_back(Rect2i(img_width + 1, img_height + 2, 12, 12));
		rects.push_back(Rect2i(1, 1, 1, 1));
		rects.push_back(Rect2i(0, 1, 2, 3));
		rects.push_back(Rect2i(-5, 0, img_width + 10, 2));
		rects.push_back(Rect2i(0, -5, 2, img_height + 10));
		rects.push_back(Rect2i(-1, -1, img_width + 1, img_height + 1));

		for (const Rect2i &rect : rects) {
			Ref<Image> img = memnew(Image(img_width, img_height, false, Image::FORMAT_RGBA8));
			img->fill_rect(rect, Color(1, 1, 1, 1));
			for (int y = 0; y < img->get_height(); y++) {
				for (int x = 0; x < img->get_width(); x++) {
					if (rect.abs().has_point(Point2(x, y))) {
						CHECK_MESSAGE(
								img->get_pixel(x, y).is_equal_approx(Color(1, 1, 1, 1)),
								"fill_rect() should colorize all image pixels within rect bounds.");
					} else {
						CHECK_MESSAGE(
								!img->get_pixel(x, y).is_equal_approx(Color(1, 1, 1, 1)),
								"fill_rect() shouldn't colorize any image pixel out of rect bounds.");
					}
				}
			}
		}
	}

	// Blend two images together
	image->blend_rect(image2, Rect2i(Vector2i(0, 0), image2->get_size()), Vector2i(0, 0));
	CHECK_MESSAGE(
			image->get_pixel(0, 0).a > 0.7,
			"blend_rect() should blend the alpha values of the two images.");
	CHECK_MESSAGE(
			image->get_used_rect().size == image->get_size(),
			"get_used_rect() should return the expected value, its Rect size should be the same as get_size() if there are no transparent pixels.");

	Ref<Image> image3 = memnew(Image(2, 2, false, Image::FORMAT_RGBA8));
	image3->set_pixel(0, 0, Color(0, 1, 0, 1));

	//blit_rect() two images together
	image->blit_rect(image3, Rect2i(Vector2i(0, 0), image3->get_size()), Vector2i(0, 0));
	CHECK_MESSAGE(
			image->get_pixel(0, 0).is_equal_approx(Color(0, 1, 0, 1)),
			"blit_rect() should replace old colors and not blend them.");
	CHECK_MESSAGE(
			!image->get_pixel(2, 2).is_equal_approx(Color(0, 1, 0, 1)),
			"blit_rect() should not affect the area of the image that is outside src_rect.");

	// Flip image
	image3->flip_x();
	CHECK(image3->get_pixel(1, 0).is_equal_approx(Color(0, 1, 0, 1)));
	CHECK_MESSAGE(
			image3->get_pixel(0, 0).is_equal_approx(Color(0, 0, 0, 0)),
			"flip_x() should not leave old pixels behind.");
	image3->flip_y();
	CHECK(image3->get_pixel(1, 1).is_equal_approx(Color(0, 1, 0, 1)));
	CHECK_MESSAGE(
			image3->get_pixel(1, 0).is_equal_approx(Color(0, 0, 0, 0)),
			"flip_y() should not leave old pixels behind.");

	// Pre-multiply Alpha then Convert from RGBA to L8, checking alpha
	{
		Ref<Image> gray_image = memnew(Image(3, 3, false, Image::FORMAT_RGBA8));
		gray_image->fill_rect(Rect2i(0, 0, 3, 3), Color(1, 1, 1, 0));
		gray_image->set_pixel(1, 1, Color(1, 1, 1, 1));
		gray_image->set_pixel(1, 2, Color(0.5, 0.5, 0.5, 0.5));
		gray_image->set_pixel(2, 1, Color(0.25, 0.05, 0.5, 1.0));
		gray_image->set_pixel(2, 2, Color(0.5, 0.25, 0.95, 0.75));
		gray_image->premultiply_alpha();
		gray_image->convert(Image::FORMAT_L8);
		CHECK_MESSAGE(gray_image->get_pixel(0, 0).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(0, 1).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(0, 2).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(1, 0).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(1, 1).is_equal_approx(Color(1, 1, 1, 1)), "convert() RGBA to L8 should be white.");
		CHECK_MESSAGE(gray_image->get_pixel(1, 2).is_equal_approx(Color(0.250980407, 0.250980407, 0.250980407, 1)), "convert() RGBA to L8 should be around 0.250980407 (64).");
		CHECK_MESSAGE(gray_image->get_pixel(2, 0).is_equal_approx(Color(0, 0, 0, 1)), "convert() RGBA to L8 should be black.");
		CHECK_MESSAGE(gray_image->get_pixel(2, 1).is_equal_approx(Color(0.121568628, 0.121568628, 0.121568628, 1)), "convert() RGBA to L8 should be around 0.121568628 (31).");
		CHECK_MESSAGE(gray_image->get_pixel(2, 2).is_equal_approx(Color(0.266666681, 0.266666681, 0.266666681, 1)), "convert() RGBA to L8 should be around 0.266666681 (68).");
	}
}

TEST_CASE("[Image] Custom mipmaps") {
	Ref<Image> image = memnew(Image(100, 100, false, Image::FORMAT_RGBA8));

	REQUIRE(!image->has_mipmaps());
	image->generate_mipmaps();
	REQUIRE(image->has_mipmaps());

	const int mipmaps = image->get_mipmap_count() + 1;
	REQUIRE(mipmaps == 7);

	// Initialize reference mipmap data.
	// Each byte is given value "mipmap_index * 5".

	{
		PackedByteArray data = image->get_data();
		uint8_t *data_ptr = data.ptrw();

		for (int mip = 0; mip < mipmaps; mip++) {
			int64_t mip_offset = 0;
			int64_t mip_size = 0;
			image->get_mipmap_offset_and_size(mip, mip_offset, mip_size);

			for (int i = 0; i < mip_size; i++) {
				data_ptr[mip_offset + i] = mip * 5;
			}
		}
		image->set_data(image->get_width(), image->get_height(), image->has_mipmaps(), image->get_format(), data);
	}

	// Byte format conversion.

	for (int format = Image::FORMAT_L8; format <= Image::FORMAT_RGBA8; format++) {
		Ref<Image> image_bytes = memnew(Image());
		image_bytes->copy_internals_from(image);
		image_bytes->convert((Image::Format)format);
		REQUIRE(image_bytes->has_mipmaps());

		PackedByteArray data = image_bytes->get_data();
		const uint8_t *data_ptr = data.ptr();

		for (int mip = 0; mip < mipmaps; mip++) {
			int64_t mip_offset = 0;
			int64_t mip_size = 0;
			image_bytes->get_mipmap_offset_and_size(mip, mip_offset, mip_size);

			for (int i = 0; i < mip_size; i++) {
				if (data_ptr[mip_offset + i] != mip * 5) {
					REQUIRE_MESSAGE(false, "Byte format conversion error.");
				}
			}
		}
	}

	// Floating point format conversion.

	for (int format = Image::FORMAT_RF; format <= Image::FORMAT_RGBAF; format++) {
		Ref<Image> image_rgbaf = memnew(Image());
		image_rgbaf->copy_internals_from(image);
		image_rgbaf->convert((Image::Format)format);
		REQUIRE(image_rgbaf->has_mipmaps());

		PackedByteArray data = image_rgbaf->get_data();
		const uint8_t *data_ptr = data.ptr();

		for (int mip = 0; mip < mipmaps; mip++) {
			int64_t mip_offset = 0;
			int64_t mip_size = 0;
			image_rgbaf->get_mipmap_offset_and_size(mip, mip_offset, mip_size);

			for (int i = 0; i < mip_size; i += 4) {
				float value = *(float *)(data_ptr + mip_offset + i);
				if (!Math::is_equal_approx(value * 255.0f, mip * 5)) {
					REQUIRE_MESSAGE(false, "Floating point conversion error.");
				}
			}
		}
	}
}

TEST_CASE("[Image] Convert image") {
	for (int format = Image::FORMAT_RF; format < Image::FORMAT_RGBE9995; format++) {
		for (int new_format = Image::FORMAT_RF; new_format < Image::FORMAT_RGBE9995; new_format++) {
			Ref<Image> image = memnew(Image(4, 4, false, (Image::Format)format));
			image->convert((Image::Format)new_format);
			String format_string = Image::format_names[(Image::Format)format];
			String new_format_string = Image::format_names[(Image::Format)new_format];
			format_string = "Error converting from " + format_string + " to " + new_format_string + ".";
			CHECK_MESSAGE(image->get_format() == new_format, format_string);
		}
	}

	Ref<Image> image = memnew(Image(4, 4, false, Image::FORMAT_RGBA8));
	PackedByteArray image_data = image->get_data();
	ERR_PRINT_OFF;
	image->convert((Image::Format)-1);
	ERR_PRINT_ON;
	CHECK_MESSAGE(image->get_data() == image_data, "Image conversion to invalid type (-1) should not alter image.");
	Ref<Image> image2 = memnew(Image(4, 4, false, Image::FORMAT_RGBA8));
	image_data = image2->get_data();
	ERR_PRINT_OFF;
	image2->convert((Image::Format)(Image::FORMAT_MAX + 1));
	ERR_PRINT_ON;
	CHECK_MESSAGE(image2->get_data() == image_data, "Image conversion to invalid type (Image::FORMAT_MAX + 1) should not alter image.");
}

} // namespace TestImage

// === Merged from tests/core/io/test_ip.cpp ===
/*  test_ip.cpp                                                           */
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



#include "core/io/ip.h"

namespace TestIP {

TEST_CASE("[IP] resolve_hostname") {
	for (int x = 0; x < 1000; x++) {
		IPAddress IPV4 = IP::resolve_hostname("localhost", IP::TYPE_IPV4);
		CHECK("127.0.0.1" == String(IPV4));
		IPAddress IPV6 = IP::resolve_hostname("localhost", IP::TYPE_IPV6);
		CHECK("0:0:0:0:0:0:0:1" == String(IPV6));
	}
}

} // namespace TestIP

// === Merged from tests/core/io/test_ip_address.cpp ===
/*  test_ip_address.cpp                                                   */
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



#include "core/io/ip.h"

namespace TestIPAddress {

struct IPTester : public IPAddress {
public:
	static bool _test_v4(const char *p_ip) {
		uint8_t ip[4];
		return _parse_ipv4(String::utf8(p_ip), 0, ip);
	}

	static bool _test_v6(const char *p_ip) {
		IPAddress addr;
		return _parse_ipv6(String::utf8(p_ip), addr);
	}

	static bool test_v4(const char *p_ip, bool p_valid) {
		bool is_6 = _test_v6(p_ip);
		bool is_4 = _test_v4(p_ip);
		bool is_valid = IPAddress::is_valid_ip_address(String::utf8(p_ip));
		IPAddress ip(p_ip);
		return is_4 == p_valid && is_6 == false && is_valid == p_valid && ip.is_valid() == p_valid && ip.is_wildcard() == false;
	}

	static bool test_v6(const char *p_ip, bool p_valid) {
		bool is_6 = _test_v6(p_ip);
		bool is_4 = _test_v4(p_ip);
		bool is_valid = is_valid_ip_address(String::utf8(p_ip));
		IPAddress ip(p_ip);
		return is_4 == false && is_6 == p_valid && is_valid == p_valid && ip.is_valid() == p_valid && ip.is_wildcard() == false;
	}

	static bool test_wildcard(const char *p_ip) {
		bool is_6 = _test_v6(p_ip);
		bool is_4 = _test_v4(p_ip);
		bool is_valid = is_valid_ip_address(String::utf8(p_ip));
		IPAddress ip(p_ip);
		return is_6 == false && is_4 == false && ip.is_valid() == false && is_valid == false && ip.is_wildcard() == true;
	}
};

TEST_CASE("[IPAddress] Wildcard and misc") {
	ERR_PRINT_OFF;

	auto test_ip = [](const char *l_ip, bool l_valid) {
		return IPAddress(l_ip).is_wildcard() == false && l_valid == (IPTester::test_v4(l_ip, true) || IPTester::test_v6(l_ip, true));
	};

	CHECK(IPTester::test_wildcard("*"));

	CHECK(test_ip("", false));

	CHECK(test_ip(" ", false));

	CHECK(test_ip("::", true));
	CHECK(test_ip("0.0.0.0", true));

	CHECK(test_ip("not an ip", false));
	CHECK(test_ip("surely.not:an:ip", false));

	ERR_PRINT_ON;
}

TEST_CASE("[IPAddress] IPv4 is_valid") {
	ERR_PRINT_OFF;

	auto test_ip = [](const char *l_ip, bool l_valid) {
		return IPTester::test_v4(l_ip, l_valid);
	};

	// Valid IPs
	CHECK(test_ip("127.0.0.1", true));
	CHECK(test_ip("255.255.255.255", true));
	CHECK(test_ip("0.0.0.0", true));

	// Invalid IPs
	CHECK(test_ip(" 127.0.0.1", false));
	CHECK(test_ip("127.0.0.1 ", false));
	CHECK(test_ip(" 127.0.0.1 ", false));
	CHECK(test_ip("127.0.0.-1", false));
	CHECK(test_ip("127.0.0.256", false));
	CHECK(test_ip("127.0.0.", false));
	CHECK(test_ip(".0.0.1", false));
	CHECK(test_ip("127.0.0.1.", false));
	CHECK(test_ip(".127.0.0.1", false));
	CHECK(test_ip("0.127.0.0.1", false));
	CHECK(test_ip("127.0.0.1.0", false));
	CHECK(test_ip(".....", false));
	CHECK(test_ip("....", false));
	CHECK(test_ip("...", false));
	CHECK(test_ip("..", false));
	CHECK(test_ip(".", false));
	CHECK(test_ip("", false));
	ERR_PRINT_ON;
}

TEST_CASE("[IPAddress] IPv4 Parsing") {
	auto test_ip = [](const char *l_ip, uint32_t l_test) {
		l_test = BSWAP32(l_test);
		return memcmp(IPAddress(l_ip).get_ipv4(), &l_test, 4) == 0 && IPTester::test_v4(l_ip, true);
	};
	CHECK(test_ip("127.0.0.1", 2130706433));
	CHECK(test_ip("255.0.0.0", 255 << 24));
	CHECK(test_ip("0.255.0.0", 255 << 16));
	CHECK(test_ip("0.0.255.0", 255 << 8));
	CHECK(test_ip("0.0.0.255", 255));
	CHECK(test_ip("127.0.0.0", 127 << 24));
	CHECK(test_ip("0.127.0.0", 127 << 16));
	CHECK(test_ip("0.0.127.0", 127 << 8));
	CHECK(test_ip("0.0.0.127", 127));
	CHECK(test_ip("1.0.0.0", 1 << 24));
	CHECK(test_ip("0.1.0.0", 1 << 16));
	CHECK(test_ip("0.0.1.0", 1 << 8));
	CHECK(test_ip("0.0.0.1", 1));
}

TEST_CASE("[IPAddress] IPv6 is_valid") {
	ERR_PRINT_OFF;

	auto test_ip = [](const char *l_ip, bool l_valid) {
		return IPTester::test_v6(l_ip, l_valid);
	};

	// Valid IPs
	CHECK(test_ip("::", true));

	CHECK(test_ip("::1", true));
	CHECK(test_ip("::1:1", true));
	CHECK(test_ip("::1:1:1", true));
	CHECK(test_ip("::1:1:1:1", true));
	CHECK(test_ip("::1:1:1:1:1", true));
	CHECK(test_ip("::1:1:1:1:1:1", true));
	CHECK(test_ip("::1:1:1:1:1:1:1", true));
	CHECK(test_ip("1:1:1:1:1:1:1:1", true));
	CHECK(test_ip("1:1:1:1:1:1:1::", true));
	CHECK(test_ip("1:1:1:1:1:1::", true));
	CHECK(test_ip("1:1:1:1:1::", true));
	CHECK(test_ip("1:1:1:1::", true));
	CHECK(test_ip("1:1:1::", true));
	CHECK(test_ip("1:1::", true));
	CHECK(test_ip("1::", true));

	CHECK(test_ip("1::1:1:1:1:1:1", true));
	CHECK(test_ip("1:1::1:1:1:1:1", true));
	CHECK(test_ip("1:1:1::1:1:1:1", true));
	CHECK(test_ip("1:1:1:1::1:1:1", true));
	CHECK(test_ip("1:1:1:1:1::1:1", true));
	CHECK(test_ip("1:1:1:1:1:1::1", true));
	CHECK(test_ip("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", true));
	CHECK(test_ip("FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF", true));
	CHECK(test_ip("::ffff", true));
	CHECK(test_ip("::FFFF", true));
	CHECK(test_ip("ffff::", true));
	CHECK(test_ip("FFFF::", true));

	// IPv4-mapped address
	CHECK(test_ip("::ffff:127.0.0.1", true));
	CHECK(test_ip("::FFFF:127.0.0.1", true));
	CHECK(test_ip("0::ffff:127.0.0.1", true));
	CHECK(test_ip("0:0::ffff:127.0.0.1", true));
	CHECK(test_ip("0:0:0::ffff:127.0.0.1", true));
	CHECK(test_ip("0:0:0:0::ffff:127.0.0.1", true));
	CHECK(test_ip("0:0:0:0:0:ffff:127.0.0.1", true));

	// Invalid IPs
	CHECK(test_ip(" ::", false));
	CHECK(test_ip(":: ", false));
	CHECK(test_ip("::-0", false));
	CHECK(test_ip("1:1:1:1:1:1:1:g", false));
	CHECK(test_ip(" 1:1:1:1:1:1:1:1", false));
	CHECK(test_ip("1:1:1:1:1:1:1:1 ", false));
	CHECK(test_ip(" 1:1:1:1:1:1:1:1 ", false));
	CHECK(test_ip("1:1:1:1:1:1:1:1:1", false));
	CHECK(test_ip("1:1:1:1:1:1:1:1::", false));
	CHECK(test_ip("::1:1:1:1:1:1:1:1", false));
	CHECK(test_ip("::1::1", false));
	CHECK(test_ip(":", false));
	CHECK(test_ip(":::", false));
	CHECK(test_ip("::::", false));
	CHECK(test_ip(":::::", false));
	CHECK(test_ip("::::::", false));
	CHECK(test_ip(":::::::", false));
	CHECK(test_ip("::::::::", false));

	// IPv4-mapped address
	CHECK(test_ip("1::ffff:127.0.0.1", false));
	CHECK(test_ip("::1:ffff:127.0.0.1", false));
	CHECK(test_ip("::ffff:127.0.256.1", false));
	CHECK(test_ip("::ffff:127.0.0.256", false));
	CHECK(test_ip("::ffff:256.0.0.1", false));
	CHECK(test_ip("::ffff:127.0.0.1.1", false));
	CHECK(test_ip("::ffff:127.0.0.1.", false));
	CHECK(test_ip("::ffff:127.0.0.", false));
	CHECK(test_ip("::ffff:127.0.0", false));
	CHECK(test_ip("::ffff:127.0.", false));
	CHECK(test_ip("::ffff:127.0", false));
	CHECK(test_ip("::ffff:127.", false));
	CHECK(test_ip("::ffff:", false));

	// This is a valid IPv6 address (non IPv4-mapped)
	CHECK(test_ip("::ffff:127", true));

	ERR_PRINT_ON;
}

TEST_CASE("[IPAddress] IPv6 Parsing") {
	struct InitIP {
		uint16_t data[8];
	};
	auto test_ip = [](const char *l_ip, InitIP l_test) {
		for (int i = 0; i < 8; i++) {
			l_test.data[i] = BSWAP16(l_test.data[i]);
		}
		return memcmp(IPAddress(l_ip).get_ipv6(), l_test.data, 16) == 0 && IPTester::test_v6(l_ip, true);
	};
	CHECK(IPAddress("::").is_valid() == true);

	CHECK(test_ip("::1", { 0, 0, 0, 0, 0, 0, 0, 1 }));
	CHECK(test_ip("::1:1", { 0, 0, 0, 0, 0, 0, 1, 1 }));
	CHECK(test_ip("::1:1:1", { 0, 0, 0, 0, 0, 1, 1, 1 }));
	CHECK(test_ip("::1:1:1:1", { 0, 0, 0, 0, 1, 1, 1, 1 }));
	CHECK(test_ip("::1:1:1:1:1", { 0, 0, 0, 1, 1, 1, 1, 1 }));
	CHECK(test_ip("::1:1:1:1:1:1", { 0, 0, 1, 1, 1, 1, 1, 1 }));
	CHECK(test_ip("::1:1:1:1:1:1:1", { 0, 1, 1, 1, 1, 1, 1, 1 }));
	CHECK(test_ip("1:1:1:1:1:1:1:1", { 1, 1, 1, 1, 1, 1, 1, 1 }));
	CHECK(test_ip("1:1:1:1:1:1:1::", { 1, 1, 1, 1, 1, 1, 1, 0 }));
	CHECK(test_ip("1:1:1:1:1:1::", { 1, 1, 1, 1, 1, 1, 0, 0 }));
	CHECK(test_ip("1:1:1:1:1::", { 1, 1, 1, 1, 1, 0, 0, 0 }));
	CHECK(test_ip("1:1:1:1::", { 1, 1, 1, 1, 0, 0, 0, 0 }));
	CHECK(test_ip("1:1:1::", { 1, 1, 1, 0, 0, 0, 0, 0 }));
	CHECK(test_ip("1:1::", { 1, 1, 0, 0, 0, 0, 0, 0 }));
	CHECK(test_ip("1::", { 1, 0, 0, 0, 0, 0, 0, 0 }));

	CHECK(test_ip("1::1", { 1, 0, 0, 0, 0, 0, 0, 1 }));
	CHECK(test_ip("1:1:1:1:1::1", { 1, 1, 1, 1, 1, 0, 0, 1 }));
	CHECK(test_ip("1:1:1::1:1:1", { 1, 1, 1, 0, 0, 1, 1, 1 }));
	CHECK(test_ip("1::1:1:1:1:1", { 1, 0, 0, 1, 1, 1, 1, 1 }));

	CHECK(test_ip("ffff::", { 0xFFFF, 0, 0, 0, 0, 0, 0, 0 }));
	CHECK(test_ip("::ffff", { 0, 0, 0, 0, 0, 0, 0, 0xFFFF }));
	CHECK(test_ip("::fffe:0", { 0, 0, 0, 0, 0, 0, 0xFFFE, 0 }));
	CHECK(test_ip("0:fffe::", { 0, 0xFFFE, 0, 0, 0, 0, 0, 0 }));

	// IPv4-mapped address
	CHECK(test_ip("::ffff:127.0.0.1", { 0, 0, 0, 0, 0, 0xFFFF, 0x7F00, 1 }));
	CHECK(test_ip("::FFFF:127.0.0.1", { 0, 0, 0, 0, 0, 0xFFFF, 0x7F00, 1 }));
	CHECK(test_ip("0::ffff:127.0.0.1", { 0, 0, 0, 0, 0, 0xFFFF, 0x7F00, 1 }));
	CHECK(test_ip("0:0::ffff:127.0.0.1", { 0, 0, 0, 0, 0, 0xFFFF, 0x7F00, 1 }));
	CHECK(test_ip("0:0:0::ffff:127.0.0.1", { 0, 0, 0, 0, 0, 0xFFFF, 0x7F00, 1 }));
	CHECK(test_ip("0:0:0:0::ffff:127.0.0.1", { 0, 0, 0, 0, 0, 0xFFFF, 0x7F00, 1 }));
	CHECK(test_ip("0:0:0:0:0:ffff:127.0.0.1", { 0, 0, 0, 0, 0, 0xFFFF, 0x7F00, 1 }));
}

} // namespace TestIPAddress

// === Merged from tests/core/io/test_json.cpp ===
/*  test_json.cpp                                                         */
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

#include <cfloat> // DBL_MAX

namespace TestJSON {

TEST_CASE("[JSON] Stringify single data types") {
	CHECK(JSON::stringify(Variant()) == "null");
	CHECK(JSON::stringify(false) == "false");
	CHECK(JSON::stringify(true) == "true");
	CHECK(JSON::stringify(0) == "0");
	CHECK(JSON::stringify(12345) == "12345");
	CHECK(JSON::stringify(0.75) == "0.75");
	CHECK(JSON::stringify("test") == "\"test\"");
	CHECK(JSON::stringify("\\\b\f\n\r\t\v\"") == "\"\\\\\\b\\f\\n\\r\\t\\v\\\"\"");
}

TEST_CASE("[JSON] Stringify arrays") {
	CHECK(JSON::stringify(Array()) == "[]");

	Array int_array;
	for (int i = 0; i < 10; i++) {
		int_array.push_back(i);
	}
	CHECK(JSON::stringify(int_array) == "[0,1,2,3,4,5,6,7,8,9]");

	Array str_array;
	str_array.push_back("Hello");
	str_array.push_back("World");
	str_array.push_back("!");
	CHECK(JSON::stringify(str_array) == "[\"Hello\",\"World\",\"!\"]");

	Array indented_array;
	Array nested_array;
	for (int i = 0; i < 5; i++) {
		indented_array.push_back(i);
		nested_array.push_back(i);
	}
	indented_array.push_back(nested_array);
	CHECK(JSON::stringify(indented_array, "\t") == "[\n\t0,\n\t1,\n\t2,\n\t3,\n\t4,\n\t[\n\t\t0,\n\t\t1,\n\t\t2,\n\t\t3,\n\t\t4\n\t]\n]");

	Array full_precision_array;
	full_precision_array.push_back(0.12345678901234568);
	CHECK(JSON::stringify(full_precision_array, "", true, true) == "[0.12345678901234568]");

	Array non_finite_array;
	non_finite_array.push_back(Math::INF);
	non_finite_array.push_back(-Math::INF);
	non_finite_array.push_back(Math::NaN);
	ERR_PRINT_OFF
	CHECK(JSON::stringify(non_finite_array) == "[1e99999,-1e99999,null]");

	Array non_finite_round_trip = JSON::parse_string(JSON::stringify(non_finite_array));
	CHECK(non_finite_round_trip[0] == Variant(Math::INF));
	CHECK(non_finite_round_trip[1] == Variant(-Math::INF));
	CHECK(non_finite_round_trip[2].get_type() == Variant::NIL);

	Array self_array;
	self_array.push_back(self_array);
	CHECK(JSON::stringify(self_array) == "[\"[...]\"]");
	self_array.clear();

	Array max_recursion_array;
	for (int i = 0; i < Variant::MAX_RECURSION_DEPTH + 1; i++) {
		Array next;
		next.push_back(max_recursion_array);
		max_recursion_array = next;
	}
	CHECK(JSON::stringify(max_recursion_array).contains("[...]"));
	ERR_PRINT_ON
}

TEST_CASE("[JSON] Stringify dictionaries") {
	CHECK(JSON::stringify(Dictionary()) == "{}");

	Dictionary single_entry;
	single_entry["key"] = "value";
	CHECK(JSON::stringify(single_entry) == "{\"key\":\"value\"}");

	Dictionary indented;
	indented["key1"] = "value1";
	indented["key2"] = 2;
	CHECK(JSON::stringify(indented, "\t") == "{\n\t\"key1\": \"value1\",\n\t\"key2\": 2\n}");

	Dictionary outer;
	Dictionary inner;
	inner["key"] = "value";
	outer["inner"] = inner;
	CHECK(JSON::stringify(outer) == "{\"inner\":{\"key\":\"value\"}}");

	Dictionary full_precision_dictionary;
	full_precision_dictionary["key"] = 0.12345678901234568;
	CHECK(JSON::stringify(full_precision_dictionary, "", true, true) == "{\"key\":0.12345678901234568}");

	Dictionary non_finite_dictionary;
	non_finite_dictionary["-inf"] = -Math::INF;
	non_finite_dictionary["inf"] = Math::INF;
	non_finite_dictionary["nan"] = Math::NaN;
	ERR_PRINT_OFF
	CHECK(JSON::stringify(non_finite_dictionary) == "{\"-inf\":-1e99999,\"inf\":1e99999,\"nan\":null}");

	Dictionary non_finite_round_trip = JSON::parse_string(JSON::stringify(non_finite_dictionary));
	CHECK(non_finite_round_trip["-inf"] == Variant(-Math::INF));
	CHECK(non_finite_round_trip["inf"] == Variant(Math::INF));
	CHECK(non_finite_round_trip["nan"].get_type() == Variant::NIL);

	Dictionary self_dictionary;
	self_dictionary["key"] = self_dictionary;
	CHECK(JSON::stringify(self_dictionary) == "{\"key\":\"{...}\"}");
	self_dictionary.clear();

	Dictionary max_recursion_dictionary;
	for (int i = 0; i < Variant::MAX_RECURSION_DEPTH + 1; i++) {
		Dictionary next;
		next["key"] = max_recursion_dictionary;
		max_recursion_dictionary = next;
	}
	CHECK(JSON::stringify(max_recursion_dictionary).contains("{...:...}"));
	ERR_PRINT_ON
}

// NOTE: The current JSON parser accepts many non-conformant strings such as
// single-quoted strings, duplicate commas and trailing commas.
// This is intentionally not tested as users shouldn't rely on this behavior.

TEST_CASE("[JSON] Parsing single data types") {
	// Parsing a single data type as JSON is valid per the JSON specification.

	JSON json;

	json.parse("null");
	CHECK_MESSAGE(
			json.get_error_line() == 0,
			"Parsing `null` as JSON should parse successfully.");
	CHECK_MESSAGE(
			json.get_data() == Variant(),
			"Parsing a double quoted string as JSON should return the expected value.");

	json.parse("true");
	CHECK_MESSAGE(
			json.get_error_line() == 0,
			"Parsing boolean `true` as JSON should parse successfully.");
	CHECK_MESSAGE(
			json.get_data(),
			"Parsing boolean `true` as JSON should return the expected value.");

	json.parse("false");
	CHECK_MESSAGE(
			json.get_error_line() == 0,
			"Parsing boolean `false` as JSON should parse successfully.");
	CHECK_MESSAGE(
			!json.get_data(),
			"Parsing boolean `false` as JSON should return the expected value.");

	json.parse("123456");
	CHECK_MESSAGE(
			json.get_error_line() == 0,
			"Parsing an integer number as JSON should parse successfully.");
	CHECK_MESSAGE(
			(int)(json.get_data()) == 123456,
			"Parsing an integer number as JSON should return the expected value.");

	json.parse("0.123456");
	CHECK_MESSAGE(
			json.get_error_line() == 0,
			"Parsing a floating-point number as JSON should parse successfully.");
	CHECK_MESSAGE(
			double(json.get_data()) == doctest::Approx(0.123456),
			"Parsing a floating-point number as JSON should return the expected value.");

	json.parse("\"hello\"");
	CHECK_MESSAGE(
			json.get_error_line() == 0,
			"Parsing a double quoted string as JSON should parse successfully.");
	CHECK_MESSAGE(
			json.get_data() == "hello",
			"Parsing a double quoted string as JSON should return the expected value.");
}

TEST_CASE("[JSON] Parsing arrays") {
	JSON json;

	// JSON parsing fails if it's split over several lines (even if leading indentation is removed).
	json.parse(R"(["Hello", "world.", "This is",["a","json","array.",[]], "Empty arrays ahoy:", [[["Gotcha!"]]]])");

	const Array array = json.get_data();
	CHECK_MESSAGE(
			json.get_error_line() == 0,
			"Parsing a JSON array should parse successfully.");
	CHECK_MESSAGE(
			array[0] == "Hello",
			"The parsed JSON should contain the expected values.");
	const Array sub_array = array[3];
	CHECK_MESSAGE(
			sub_array.size() == 4,
			"The parsed JSON should contain the expected values.");
	CHECK_MESSAGE(
			sub_array[1] == "json",
			"The parsed JSON should contain the expected values.");
	CHECK_MESSAGE(
			sub_array[3].hash() == Array().hash(),
			"The parsed JSON should contain the expected values.");
	const Array deep_array = Array(Array(array[5])[0])[0];
	CHECK_MESSAGE(
			deep_array[0] == "Gotcha!",
			"The parsed JSON should contain the expected values.");
}

TEST_CASE("[JSON] Parsing objects (dictionaries)") {
	JSON json;

	json.parse(R"({"name": "Godot Engine", "is_free": true, "bugs": null, "apples": {"red": 500, "green": 0, "blue": -20}, "empty_object": {}})");

	const Dictionary dictionary = json.get_data();
	CHECK_MESSAGE(
			dictionary["name"] == "Godot Engine",
			"The parsed JSON should contain the expected values.");
	CHECK_MESSAGE(
			dictionary["is_free"],
			"The parsed JSON should contain the expected values.");
	CHECK_MESSAGE(
			dictionary["bugs"] == Variant(),
			"The parsed JSON should contain the expected values.");
	CHECK_MESSAGE(
			(int)Dictionary(dictionary["apples"])["blue"] == -20,
			"The parsed JSON should contain the expected values.");
	CHECK_MESSAGE(
			dictionary["empty_object"].hash() == Dictionary().hash(),
			"The parsed JSON should contain the expected values.");
}

TEST_CASE("[JSON] Parsing escape sequences") {
	// Only certain escape sequences are valid according to the JSON specification.
	// Others must result in a parsing error instead.

	JSON json;

	TypedArray<String> valid_escapes = { "\";\"", "\\;\\", "/;/", "b;\b", "f;\f", "n;\n", "r;\r", "t;\t" };

	SUBCASE("Basic valid escape sequences") {
		for (int i = 0; i < valid_escapes.size(); i++) {
			String valid_escape = valid_escapes[i];
			String valid_escape_string = valid_escape.get_slicec(';', 0);
			String valid_escape_value = valid_escape.get_slicec(';', 1);

			String json_string = "\"\\";
			json_string += valid_escape_string;
			json_string += "\"";
			json.parse(json_string);

			CHECK_MESSAGE(
					json.get_error_line() == 0,
					vformat("Parsing valid escape sequence `%s` as JSON should parse successfully.", valid_escape_string));

			String json_value = json.get_data();
			CHECK_MESSAGE(
					json_value == valid_escape_value,
					vformat("Parsing valid escape sequence `%s` as JSON should return the expected value.", valid_escape_string));
		}
	}

	SUBCASE("Valid unicode escape sequences") {
		String json_string = "\"\\u0020\"";
		json.parse(json_string);

		CHECK_MESSAGE(
				json.get_error_line() == 0,
				vformat("Parsing valid unicode escape sequence with value `0020` as JSON should parse successfully."));

		String json_value = json.get_data();
		CHECK_MESSAGE(
				json_value == " ",
				vformat("Parsing valid unicode escape sequence with value `0020` as JSON should return the expected value."));
	}

	SUBCASE("Invalid escape sequences") {
		ERR_PRINT_OFF
		for (char32_t i = 0; i < 128; i++) {
			bool skip = false;
			for (int j = 0; j < valid_escapes.size(); j++) {
				String valid_escape = valid_escapes[j];
				String valid_escape_string = valid_escape.get_slicec(';', 0);
				if (valid_escape_string[0] == i) {
					skip = true;
					break;
				}
			}

			if (skip) {
				continue;
			}

			String json_string = "\"\\";
			json_string += i;
			json_string += "\"";
			Error err = json.parse(json_string);

			// TODO: Line number is currently kept on 0, despite an error occurring. This should be fixed in the JSON parser.
			// CHECK_MESSAGE(
			// 		json.get_error_line() != 0,
			// 		vformat("Parsing invalid escape sequence with ASCII value `%d` as JSON should fail to parse.", i));
			CHECK_MESSAGE(
					err == ERR_PARSE_ERROR,
					vformat("Parsing invalid escape sequence with ASCII value `%d` as JSON should fail to parse with ERR_PARSE_ERROR.", i));
		}
		ERR_PRINT_ON
	}
}

TEST_CASE("[JSON] Serialization") {
	JSON json;

	struct FpTestCase {
		double number;
		String json;
	};

	struct IntTestCase {
		int64_t number;
		String json;
	};

	struct UIntTestCase {
		uint64_t number;
		String json;
	};

	static FpTestCase fp_tests_default_precision[] = {
		{ 0.0, "0.0" },
		{ 1000.1234567890123456789, "1000.12345678901" },
		{ -1000.1234567890123456789, "-1000.12345678901" },
		{ DBL_MAX, "179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0" },
		{ DBL_MAX - 1, "179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368.0" },
		{ std::pow(2, 53), "9007199254740992.0" },
		{ -std::pow(2, 53), "-9007199254740992.0" },
		{ 0.00000000000000011, "0.00000000000000011" },
		{ -0.00000000000000011, "-0.00000000000000011" },
		{ 1.0 / 3.0, "0.333333333333333" },
		{ 0.9999999999999999, "1.0" },
		{ 1.0000000000000001, "1.0" },
	};

	static FpTestCase fp_tests_full_precision[] = {
		{ 0.0, "0.0" },
		{ 1000.1234567890123456789, "1000.1234567890124" },
		{ -1000.1234567890123456789, "-1000.1234567890124" },
		{ DBL_MAX, "1.7976931348623157e+308" },
		{ DBL_MAX - 1, "1.7976931348623157e+308" },
		{ std::pow(2, 53), "9.007199254740992e+15" },
		{ -std::pow(2, 53), "-9.007199254740992e+15" },
		{ 0.00000000000000011, "1.1e-16" },
		{ -0.00000000000000011, "-1.1e-16" },
		{ 1.0 / 3.0, "0.3333333333333333" },
		{ 0.9999999999999999, "0.9999999999999999" },
		{ 1.0000000000000001, "1.0" },
	};

	static IntTestCase int_tests[] = {
		{ 0, "0" },
		{ INT64_MAX, "9223372036854775807" },
		{ INT64_MIN, "-9223372036854775808" },
	};

	SUBCASE("Floating point default precision") {
		for (FpTestCase &test : fp_tests_default_precision) {
			String json_value = json.stringify(test.number, "", true, false);

			CHECK_MESSAGE(
					json_value == test.json,
					vformat("Serializing `%.20d` to JSON should return the expected value.", test.number));
		}
	}

	SUBCASE("Floating point full precision") {
		for (FpTestCase &test : fp_tests_full_precision) {
			String json_value = json.stringify(test.number, "", true, true);

			CHECK_MESSAGE(
					json_value == test.json,
					vformat("Serializing `%20f` to JSON should return the expected value.", test.number));
		}
	}

	SUBCASE("Signed integer") {
		for (IntTestCase &test : int_tests) {
			String json_value = json.stringify(test.number, "", true, true);

			CHECK_MESSAGE(
					json_value == test.json,
					vformat("Serializing `%d` to JSON should return the expected value.", test.number));
		}
	}
}

} // namespace TestJSON

// === Merged from tests/core/io/test_json_native.cpp ===
/*  test_json_native.cpp                                                  */
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

namespace TestJSONNative {

String encode(const Variant &p_variant, bool p_full_objects = false) {
	return JSON::stringify(JSON::from_native(p_variant, p_full_objects), "", false);
}

Variant decode(const String &p_string, bool p_allow_objects = false) {
	return JSON::to_native(JSON::parse_string(p_string), p_allow_objects);
}

void test(const Variant &p_variant, const String &p_string, bool p_with_objects = false) {
	CHECK(encode(p_variant, p_with_objects) == p_string);
	CHECK(decode(p_string, p_with_objects).get_construct_string() == p_variant.get_construct_string());
}

TEST_CASE("[JSON][Native] Conversion between native and JSON formats") {
	// `Nil` and `bool` (represented as JSON keyword literals).
	test(Variant(), "null");
	test(false, "false");
	test(true, "true");

	// Numbers and strings (represented as JSON strings).
	test(1, R"("i:1")");
	test(1.0, R"("f:1.0")");
	test(Math::INF, R"("f:inf")");
	test(-Math::INF, R"("f:-inf")");
	test(Math::NaN, R"("f:nan")");
	test(String("abc"), R"("s:abc")");
	test(String("abc"), R"("sn:abc")");
	test(NodePath("abc"), R"("np:abc")");

	// Non-serializable types (always empty after deserialization).
	test(RID(), R"({"type":"RID"})");
	test(Callable(), R"({"type":"Callable"})");
	test(Signal(), R"({"type":"Signal"})");

	// Math types.

	test(Vector2(1, 2), R"({"type":"Vector2","args":[1.0,2.0]})");
	test(Vector2i(1, 2), R"({"type":"Vector2i","args":[1,2]})");
	test(Rect2(1, 2, 3, 4), R"({"type":"Rect2","args":[1.0,2.0,3.0,4.0]})");
	test(Rect2i(1, 2, 3, 4), R"({"type":"Rect2i","args":[1,2,3,4]})");
	test(Vector3(1, 2, 3), R"({"type":"Vector3","args":[1.0,2.0,3.0]})");
	test(Vector3i(1, 2, 3), R"({"type":"Vector3i","args":[1,2,3]})");
	test(Transform2D(1, 2, 3, 4, 5, 6), R"({"type":"Transform2D","args":[1.0,2.0,3.0,4.0,5.0,6.0]})");
	test(Vector4(1, 2, 3, 4), R"({"type":"Vector4","args":[1.0,2.0,3.0,4.0]})");
	test(Vector4i(1, 2, 3, 4), R"({"type":"Vector4i","args":[1,2,3,4]})");
	test(Plane(1, 2, 3, 4), R"({"type":"Plane","args":[1.0,2.0,3.0,4.0]})");
	test(Quaternion(1, 2, 3, 4), R"({"type":"Quaternion","args":[1.0,2.0,3.0,4.0]})");
	test(AABB(Vector3(1, 2, 3), Vector3(4, 5, 6)), R"({"type":"AABB","args":[1.0,2.0,3.0,4.0,5.0,6.0]})");

	const Basis b(Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9));
	test(b, R"({"type":"Basis","args":[1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0]})");

	const Transform3D tr3d(Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9), Vector3(10, 11, 12));
	test(tr3d, R"({"type":"Transform3D","args":[1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12.0]})");

	const Projection p(Vector4(1, 2, 3, 4), Vector4(5, 6, 7, 8), Vector4(9, 10, 11, 12), Vector4(13, 14, 15, 16));
	test(p, R"({"type":"Projection","args":[1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12.0,13.0,14.0,15.0,16.0]})");

	test(Color(1, 2, 3, 4), R"({"type":"Color","args":[1.0,2.0,3.0,4.0]})");

	// `Object`.

	Ref<Resource> res;
	res.instantiate();

	// The properties are stored in an array because the order in which they are assigned may be important during initialization.
	const String res_repr = R"({"type":"Resource","props":["resource_local_to_scene",false,"resource_name","s:","script",null]})";

	test(res, res_repr, true);
	ERR_PRINT_OFF;
	CHECK(encode(res) == "null");
	CHECK(decode(res_repr).get_type() == Variant::NIL);
	ERR_PRINT_ON;

	// `Dictionary`.

	Dictionary dict;
	dict[false] = true;
	dict[0] = 1;
	dict[0.0] = 1.0;

	// Godot dictionaries preserve insertion order, so an array is used for keys/values.
	test(dict, R"({"type":"Dictionary","args":[false,true,"i:0","i:1","f:0.0","f:1.0"]})");

	TypedDictionary<int64_t, int64_t> int_int_dict;
	int_int_dict[1] = 2;
	int_int_dict[3] = 4;

	test(int_int_dict, R"({"type":"Dictionary","key_type":"int","value_type":"int","args":["i:1","i:2","i:3","i:4"]})");

	TypedDictionary<int64_t, Variant> int_var_dict;
	int_var_dict[1] = "2";
	int_var_dict[3] = "4";

	test(int_var_dict, R"({"type":"Dictionary","key_type":"int","args":["i:1","s:2","i:3","s:4"]})");

	TypedDictionary<Variant, int64_t> var_int_dict;
	var_int_dict["1"] = 2;
	var_int_dict["3"] = 4;

	test(var_int_dict, R"({"type":"Dictionary","value_type":"int","args":["s:1","i:2","s:3","i:4"]})");

	Dictionary dict2;
	dict2["x"] = res;

	const String dict2_repr = vformat(R"({"type":"Dictionary","args":["s:x",%s]})", res_repr);

	test(dict2, dict2_repr, true);
	ERR_PRINT_OFF;
	CHECK(encode(dict2) == R"({"type":"Dictionary","args":["s:x",null]})");
	CHECK(decode(dict2_repr).get_construct_string() == "{\n\"x\": null\n}");
	ERR_PRINT_ON;

	TypedDictionary<String, Resource> res_dict;
	res_dict["x"] = res;

	const String res_dict_repr = vformat(R"({"type":"Dictionary","key_type":"String","value_type":"Resource","args":["s:x",%s]})", res_repr);

	test(res_dict, res_dict_repr, true);
	ERR_PRINT_OFF;
	CHECK(encode(res_dict) == "null");
	CHECK(decode(res_dict_repr).get_type() == Variant::NIL);
	ERR_PRINT_ON;

	// `Array`.

	Array arr = { true, 1, "abc" };
	test(arr, R"([true,"i:1","s:abc"])");

	TypedArray<int64_t> int_arr = { 1, 2, 3 };
	test(int_arr, R"({"type":"Array","elem_type":"int","args":["i:1","i:2","i:3"]})");

	Array arr2 = { 1, res, 9 };
	const String arr2_repr = vformat(R"(["i:1",%s,"i:9"])", res_repr);

	test(arr2, arr2_repr, true);
	ERR_PRINT_OFF;
	CHECK(encode(arr2) == R"(["i:1",null,"i:9"])");
	CHECK(decode(arr2_repr).get_construct_string() == "[1, null, 9]");
	ERR_PRINT_ON;

	TypedArray<Resource> res_arr = { res };
	const String res_arr_repr = vformat(R"({"type":"Array","elem_type":"Resource","args":[%s]})", res_repr);

	test(res_arr, res_arr_repr, true);
	ERR_PRINT_OFF;
	CHECK(encode(res_arr) == "null");
	CHECK(decode(res_arr_repr).get_type() == Variant::NIL);
	ERR_PRINT_ON;

	// Packed arrays.

	test(PackedByteArray({ 1, 2, 3 }), R"({"type":"PackedByteArray","args":[1,2,3]})");
	test(PackedInt32Array({ 1, 2, 3 }), R"({"type":"PackedInt32Array","args":[1,2,3]})");
	test(PackedInt64Array({ 1, 2, 3 }), R"({"type":"PackedInt64Array","args":[1,2,3]})");
	test(PackedFloat32Array({ 1, 2, 3 }), R"({"type":"PackedFloat32Array","args":[1.0,2.0,3.0]})");
	test(PackedFloat64Array({ 1, 2, 3 }), R"({"type":"PackedFloat64Array","args":[1.0,2.0,3.0]})");
	test(PackedStringArray({ "a", "b", "c" }), R"({"type":"PackedStringArray","args":["a","b","c"]})");

	const PackedVector2Array pv2arr({ Vector2(1, 2), Vector2(3, 4), Vector2(5, 6) });
	test(pv2arr, R"({"type":"PackedVector2Array","args":[1.0,2.0,3.0,4.0,5.0,6.0]})");

	const PackedVector3Array pv3arr({ Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9) });
	test(pv3arr, R"({"type":"PackedVector3Array","args":[1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0]})");

	const PackedColorArray pcolarr({ Color(1, 2, 3, 4), Color(5, 6, 7, 8), Color(9, 10, 11, 12) });
	test(pcolarr, R"({"type":"PackedColorArray","args":[1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12.0]})");

	const PackedVector4Array pv4arr({ Vector4(1, 2, 3, 4), Vector4(5, 6, 7, 8), Vector4(9, 10, 11, 12) });
	test(pv4arr, R"({"type":"PackedVector4Array","args":[1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12.0]})");
}

} // namespace TestJSONNative

// === Merged from tests/core/io/test_logger.cpp ===
/*  test_logger.cpp                                                       */
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
#include "core/io/file_access.h"
#include "core/os/os.h"

namespace TestLogger {

constexpr int sleep_duration = 1200000;

void initialize_logs() {
	ProjectSettings::set_setting("application/config/name", "godot_tests");
	DirAccess::make_dir_recursive_absolute(OS::get_user_data_dir().path_join("logs"));
}

void cleanup_logs() {
	ProjectSettings::set_setting("application/config/name", "godot_tests");
	Ref<DirAccess> dir = DirAccess::open("user://logs");
	dir->list_dir_begin();
	String file = dir->get_next();
	while (file != "") {
		if (file.match("*.log")) {
			dir->remove(file);
		}
		file = dir->get_next();
	}
	DirAccess::remove_absolute(OS::get_user_data_dir().path_join("logs"));
	DirAccess::remove_absolute(OS::get_user_data_dir());
}

TEST_CASE("[Logger][RotatedFileLogger] Creates the first log file and logs on it") {
	initialize_logs();

	String waiting_for_godot = "Waiting for Godot";
	RotatedFileLogger logger("user://logs/godot.log");
	logger.logf("%s", "Waiting for Godot");

	Error err = Error::OK;
	Ref<FileAccess> log = FileAccess::open("user://logs/godot.log", FileAccess::READ, &err);
	CHECK_EQ(err, Error::OK);
	CHECK_EQ(log->get_as_text(), waiting_for_godot);

	cleanup_logs();
}

void get_log_files(Vector<String> &log_files) {
	Ref<DirAccess> dir = DirAccess::open("user://logs");
	dir->list_dir_begin();
	String file = dir->get_next();
	while (file != "") {
		// Filtering godot.log because ordered_insert will put it first and should be the last.
		if (file.match("*.log") && file != "godot.log") {
			log_files.ordered_insert(file);
		}
		file = dir->get_next();
	}
	if (FileAccess::exists("user://logs/godot.log")) {
		log_files.push_back("godot.log");
	}
}

// All things related to log file rotation are in the same test because testing it require some sleeps.
TEST_CASE("[Logger][RotatedFileLogger] Rotates logs files") {
	initialize_logs();

	Vector<String> all_waiting_for_godot;

	const int number_of_files = 3;
	for (int i = 0; i < number_of_files; i++) {
		String waiting_for_godot = "Waiting for Godot " + itos(i);
		RotatedFileLogger logger("user://logs/godot.log", number_of_files);
		logger.logf("%s", waiting_for_godot.ascii().get_data());
		all_waiting_for_godot.push_back(waiting_for_godot);

		// Required to ensure the rotation of the log file.
		OS::delay_usec(sleep_duration);
	}

	Vector<String> log_files;
	get_log_files(log_files);
	CHECK_MESSAGE(log_files.size() == number_of_files, "Did not rotate all files");

	for (int i = 0; i < log_files.size(); i++) {
		Error err = Error::OK;
		Ref<FileAccess> log_file = FileAccess::open("user://logs/" + log_files[i], FileAccess::READ, &err);
		REQUIRE_EQ(err, Error::OK);
		CHECK_EQ(log_file->get_as_text(), all_waiting_for_godot[i]);
	}

	// Required to ensure the rotation of the log file.
	OS::delay_usec(sleep_duration);

	// This time the oldest log must be removed and godot.log updated.
	String new_waiting_for_godot = "Waiting for Godot " + itos(number_of_files);
	all_waiting_for_godot = all_waiting_for_godot.slice(1, all_waiting_for_godot.size());
	all_waiting_for_godot.push_back(new_waiting_for_godot);
	RotatedFileLogger logger("user://logs/godot.log", number_of_files);
	logger.logf("%s", new_waiting_for_godot.ascii().get_data());

	log_files.clear();
	get_log_files(log_files);
	CHECK_MESSAGE(log_files.size() == number_of_files, "Did not remove old log file");

	for (int i = 0; i < log_files.size(); i++) {
		Error err = Error::OK;
		Ref<FileAccess> log_file = FileAccess::open("user://logs/" + log_files[i], FileAccess::READ, &err);
		REQUIRE_EQ(err, Error::OK);
		CHECK_EQ(log_file->get_as_text(), all_waiting_for_godot[i]);
	}

	cleanup_logs();
}

TEST_CASE("[Logger][CompositeLogger] Logs the same into multiple loggers") {
	initialize_logs();

	Vector<Logger *> all_loggers;
	all_loggers.push_back(memnew(RotatedFileLogger("user://logs/godot_logger_1.log", 1)));
	all_loggers.push_back(memnew(RotatedFileLogger("user://logs/godot_logger_2.log", 1)));

	String waiting_for_godot = "Waiting for Godot";
	CompositeLogger logger(all_loggers);
	logger.logf("%s", "Waiting for Godot");

	Error err = Error::OK;
	Ref<FileAccess> log = FileAccess::open("user://logs/godot_logger_1.log", FileAccess::READ, &err);
	CHECK_EQ(err, Error::OK);
	CHECK_EQ(log->get_as_text(), waiting_for_godot);
	log = FileAccess::open("user://logs/godot_logger_2.log", FileAccess::READ, &err);
	CHECK_EQ(err, Error::OK);
	CHECK_EQ(log->get_as_text(), waiting_for_godot);

	cleanup_logs();
}

} // namespace TestLogger

// === Merged from tests/core/io/test_marshalls.cpp ===
/*  test_marshalls.cpp                                                    */
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

namespace TestMarshalls {

TEST_CASE("[Marshalls] Unsigned 16 bit integer encoding") {
	uint8_t arr[2];

	unsigned int actual_size = encode_uint16(0x1234, arr);
	CHECK(actual_size == sizeof(uint16_t));
	CHECK_MESSAGE(arr[0] == 0x34, "First encoded byte value should be equal to low order byte value.");
	CHECK_MESSAGE(arr[1] == 0x12, "Last encoded byte value should be equal to high order byte value.");
}

TEST_CASE("[Marshalls] Unsigned 32 bit integer encoding") {
	uint8_t arr[4];

	unsigned int actual_size = encode_uint32(0x12345678, arr);
	CHECK(actual_size == sizeof(uint32_t));
	CHECK_MESSAGE(arr[0] == 0x78, "First encoded byte value should be equal to low order byte value.");
	CHECK(arr[1] == 0x56);
	CHECK(arr[2] == 0x34);
	CHECK_MESSAGE(arr[3] == 0x12, "Last encoded byte value should be equal to high order byte value.");
}

TEST_CASE("[Marshalls] Unsigned 64 bit integer encoding") {
	uint8_t arr[8];

	unsigned int actual_size = encode_uint64(0x0f123456789abcdef, arr);
	CHECK(actual_size == sizeof(uint64_t));
	CHECK_MESSAGE(arr[0] == 0xef, "First encoded byte value should be equal to low order byte value.");
	CHECK(arr[1] == 0xcd);
	CHECK(arr[2] == 0xab);
	CHECK(arr[3] == 0x89);
	CHECK(arr[4] == 0x67);
	CHECK(arr[5] == 0x45);
	CHECK(arr[6] == 0x23);
	CHECK_MESSAGE(arr[7] == 0xf1, "Last encoded byte value should be equal to high order byte value.");
}

TEST_CASE("[Marshalls] Unsigned 16 bit integer decoding") {
	uint8_t arr[] = { 0x34, 0x12 };

	CHECK(decode_uint16(arr) == 0x1234);
}

TEST_CASE("[Marshalls] Unsigned 32 bit integer decoding") {
	uint8_t arr[] = { 0x78, 0x56, 0x34, 0x12 };

	CHECK(decode_uint32(arr) == 0x12345678);
}

TEST_CASE("[Marshalls] Unsigned 64 bit integer decoding") {
	uint8_t arr[] = { 0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0xf1 };

	CHECK(decode_uint64(arr) == 0x0f123456789abcdef);
}

TEST_CASE("[Marshalls] Floating point half precision encoding") {
	uint8_t arr[2];

	// Decimal: 0.33325195
	// IEEE 754 half-precision binary floating-point format:
	// sign exponent (5 bits)    fraction (10 bits)
	//  0        01101               0101010101
	// Hexadecimal: 0x3555
	unsigned int actual_size = encode_half(0.33325195f, arr);
	CHECK(actual_size == sizeof(uint16_t));
	CHECK(arr[0] == 0x55);
	CHECK(arr[1] == 0x35);
}

TEST_CASE("[Marshalls] Floating point single precision encoding") {
	uint8_t arr[4];

	// Decimal: 0.15625
	// IEEE 754 single-precision binary floating-point format:
	// sign exponent (8 bits)    fraction (23 bits)
	//  0       01111100      01000000000000000000000
	// Hexadecimal: 0x3E200000
	unsigned int actual_size = encode_float(0.15625f, arr);
	CHECK(actual_size == sizeof(uint32_t));
	CHECK(arr[0] == 0x00);
	CHECK(arr[1] == 0x00);
	CHECK(arr[2] == 0x20);
	CHECK(arr[3] == 0x3e);
}

TEST_CASE("[Marshalls] Floating point double precision encoding") {
	uint8_t arr[8];

	// Decimal: 0.333333333333333314829616256247390992939472198486328125
	// IEEE 754 double-precision binary floating-point format:
	// sign exponent (11 bits)                  fraction (52 bits)
	//  0      01111111101     0101010101010101010101010101010101010101010101010101
	// Hexadecimal: 0x3FD5555555555555
	unsigned int actual_size = encode_double(0.33333333333333333, arr);
	CHECK(actual_size == sizeof(uint64_t));
	CHECK(arr[0] == 0x55);
	CHECK(arr[1] == 0x55);
	CHECK(arr[2] == 0x55);
	CHECK(arr[3] == 0x55);
	CHECK(arr[4] == 0x55);
	CHECK(arr[5] == 0x55);
	CHECK(arr[6] == 0xd5);
	CHECK(arr[7] == 0x3f);
}

TEST_CASE("[Marshalls] Floating point half precision decoding") {
	uint8_t arr[] = { 0x55, 0x35 };

	// See floating point half precision encoding test case for details behind expected values.
	CHECK(decode_half(arr) == 0.33325195f);
}

TEST_CASE("[Marshalls] Floating point single precision decoding") {
	uint8_t arr[] = { 0x00, 0x00, 0x20, 0x3e };

	// See floating point encoding test case for details behind expected values
	CHECK(decode_float(arr) == 0.15625f);
}

TEST_CASE("[Marshalls] Floating point double precision decoding") {
	uint8_t arr[] = { 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xd5, 0x3f };

	// See floating point encoding test case for details behind expected values
	CHECK(decode_double(arr) == 0.33333333333333333);
}

TEST_CASE("[Marshalls] C string encoding") {
	char cstring[] = "Godot"; // 5 characters
	uint8_t data[6];

	int actual_size = encode_cstring(cstring, data);
	CHECK(actual_size == 6);
	CHECK(data[0] == 'G');
	CHECK(data[1] == 'o');
	CHECK(data[2] == 'd');
	CHECK(data[3] == 'o');
	CHECK(data[4] == 't');
	CHECK(data[5] == '\0');
}

TEST_CASE("[Marshalls] NIL Variant encoding") {
	int r_len;
	Variant variant;
	uint8_t buffer[4];

	CHECK(encode_variant(variant, buffer, r_len) == OK);
	CHECK_MESSAGE(r_len == 4, "Length == 4 bytes for header.");
	CHECK_MESSAGE(buffer[0] == 0x00, "Variant::NIL");
	CHECK(buffer[1] == 0x00);
	CHECK(buffer[2] == 0x00);
	CHECK(buffer[3] == 0x00);
	// No value
}

TEST_CASE("[Marshalls] INT 32 bit Variant encoding") {
	int r_len;
	Variant variant(0x12345678);
	uint8_t buffer[8];

	CHECK(encode_variant(variant, buffer, r_len) == OK);
	CHECK_MESSAGE(r_len == 8, "Length == 4 bytes for header + 4 bytes for `int32_t`.");
	CHECK_MESSAGE(buffer[0] == 0x02, "Variant::INT");
	CHECK(buffer[1] == 0x00);
	CHECK(buffer[2] == 0x00);
	CHECK(buffer[3] == 0x00);
	// Check value
	CHECK(buffer[4] == 0x78);
	CHECK(buffer[5] == 0x56);
	CHECK(buffer[6] == 0x34);
	CHECK(buffer[7] == 0x12);
}

TEST_CASE("[Marshalls] INT 64 bit Variant encoding") {
	int r_len;
	Variant variant(uint64_t(0x0f123456789abcdef));
	uint8_t buffer[12];

	CHECK(encode_variant(variant, buffer, r_len) == OK);
	CHECK_MESSAGE(r_len == 12, "Length == 4 bytes for header + 8 bytes for `int64_t`.");
	CHECK_MESSAGE(buffer[0] == 0x02, "Variant::INT");
	CHECK(buffer[1] == 0x00);
	CHECK_MESSAGE(buffer[2] == 0x01, "HEADER_DATA_FLAG_64");
	CHECK(buffer[3] == 0x00);
	// Check value
	CHECK(buffer[4] == 0xef);
	CHECK(buffer[5] == 0xcd);
	CHECK(buffer[6] == 0xab);
	CHECK(buffer[7] == 0x89);
	CHECK(buffer[8] == 0x67);
	CHECK(buffer[9] == 0x45);
	CHECK(buffer[10] == 0x23);
	CHECK(buffer[11] == 0xf1);
}

TEST_CASE("[Marshalls] FLOAT single precision Variant encoding") {
	int r_len;
	Variant variant(0.15625f);
	uint8_t buffer[8];

	CHECK(encode_variant(variant, buffer, r_len) == OK);
	CHECK_MESSAGE(r_len == 8, "Length == 4 bytes for header + 4 bytes for `float`.");
	CHECK_MESSAGE(buffer[0] == 0x03, "Variant::FLOAT");
	CHECK(buffer[1] == 0x00);
	CHECK(buffer[2] == 0x00);
	CHECK(buffer[3] == 0x00);
	// Check value
	CHECK(buffer[4] == 0x00);
	CHECK(buffer[5] == 0x00);
	CHECK(buffer[6] == 0x20);
	CHECK(buffer[7] == 0x3e);
}

TEST_CASE("[Marshalls] FLOAT double precision Variant encoding") {
	int r_len;
	Variant variant(0.33333333333333333);
	uint8_t buffer[12];

	CHECK(encode_variant(variant, buffer, r_len) == OK);
	CHECK_MESSAGE(r_len == 12, "Length == 4 bytes for header + 8 bytes for `double`.");
	CHECK_MESSAGE(buffer[0] == 0x03, "Variant::FLOAT");
	CHECK(buffer[1] == 0x00);
	CHECK_MESSAGE(buffer[2] == 0x01, "HEADER_DATA_FLAG_64");
	CHECK(buffer[3] == 0x00);
	// Check value
	CHECK(buffer[4] == 0x55);
	CHECK(buffer[5] == 0x55);
	CHECK(buffer[6] == 0x55);
	CHECK(buffer[7] == 0x55);
	CHECK(buffer[8] == 0x55);
	CHECK(buffer[9] == 0x55);
	CHECK(buffer[10] == 0xd5);
	CHECK(buffer[11] == 0x3f);
}

TEST_CASE("[Marshalls] Invalid data Variant decoding") {
	Variant variant;
	int r_len = 0;
	uint8_t some_buffer[1] = { 0x00 };
	uint8_t out_of_range_type_buffer[4] = { 0xff }; // Greater than Variant::VARIANT_MAX

	ERR_PRINT_OFF;
	CHECK(decode_variant(variant, some_buffer, /* less than 4 */ 1, &r_len) == ERR_INVALID_DATA);
	CHECK(r_len == 0);

	CHECK(decode_variant(variant, out_of_range_type_buffer, 4, &r_len) == ERR_INVALID_DATA);
	CHECK(r_len == 0);
	ERR_PRINT_ON;
}

TEST_CASE("[Marshalls] NIL Variant decoding") {
	Variant variant;
	int r_len;
	uint8_t buffer[] = {
		0x00, 0x00, 0x00, 0x00 // Variant::NIL
	};

	CHECK(decode_variant(variant, buffer, 4, &r_len) == OK);
	CHECK(r_len == 4);
	CHECK(variant == Variant());
}

TEST_CASE("[Marshalls] INT 32 bit Variant decoding") {
	Variant variant;
	int r_len;
	uint8_t buffer[] = {
		0x02, 0x00, 0x00, 0x00, // Variant::INT
		0x78, 0x56, 0x34, 0x12 // value
	};

	CHECK(decode_variant(variant, buffer, 8, &r_len) == OK);
	CHECK(r_len == 8);
	CHECK(variant == Variant(0x12345678));
}

TEST_CASE("[Marshalls] INT 64 bit Variant decoding") {
	Variant variant;
	int r_len;
	uint8_t buffer[] = {
		0x02, 0x00, 0x01, 0x00, // Variant::INT, HEADER_DATA_FLAG_64
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0xf1 // value
	};

	CHECK(decode_variant(variant, buffer, 12, &r_len) == OK);
	CHECK(r_len == 12);
	CHECK(variant == Variant(uint64_t(0x0f123456789abcdef)));
}

TEST_CASE("[Marshalls] FLOAT single precision Variant decoding") {
	Variant variant;
	int r_len;
	uint8_t buffer[] = {
		0x03, 0x00, 0x00, 0x00, // Variant::FLOAT
		0x00, 0x00, 0x20, 0x3e // value
	};

	CHECK(decode_variant(variant, buffer, 8, &r_len) == OK);
	CHECK(r_len == 8);
	CHECK(variant == Variant(0.15625f));
}

TEST_CASE("[Marshalls] FLOAT double precision Variant decoding") {
	Variant variant;
	int r_len;
	uint8_t buffer[] = {
		0x03, 0x00, 0x01, 0x00, // Variant::FLOAT, HEADER_DATA_FLAG_64
		0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0xd5, 0x3f // value
	};

	CHECK(decode_variant(variant, buffer, 12, &r_len) == OK);
	CHECK(r_len == 12);
	CHECK(variant == Variant(0.33333333333333333));
}

TEST_CASE("[Marshalls] Typed array encoding") {
	int r_len;
	Array array;
	array.set_typed(Variant::INT, String(), Ref<Script>());
	array.push_back(Variant(uint64_t(0x0f123456789abcdef)));
	uint8_t buffer[24];

	CHECK(encode_variant(array, buffer, r_len) == OK);
	CHECK_MESSAGE(r_len == 24, "Length == 4 bytes for header + 4 bytes for array type + 4 bytes for array size + 12 bytes for element.");
	CHECK_MESSAGE(buffer[0] == 0x1c, "Variant::ARRAY");
	CHECK(buffer[1] == 0x00);
	CHECK_MESSAGE(buffer[2] == 0x01, "CONTAINER_TYPE_KIND_BUILTIN");
	CHECK(buffer[3] == 0x00);
	// Check array type.
	CHECK_MESSAGE(buffer[4] == 0x02, "Variant::INT");
	CHECK(buffer[5] == 0x00);
	CHECK(buffer[6] == 0x00);
	CHECK(buffer[7] == 0x00);
	// Check array size.
	CHECK(buffer[8] == 0x01);
	CHECK(buffer[9] == 0x00);
	CHECK(buffer[10] == 0x00);
	CHECK(buffer[11] == 0x00);
	// Check element type.
	CHECK_MESSAGE(buffer[12] == 0x02, "Variant::INT");
	CHECK(buffer[13] == 0x00);
	CHECK_MESSAGE(buffer[14] == 0x01, "HEADER_DATA_FLAG_64");
	CHECK(buffer[15] == 0x00);
	// Check element value.
	CHECK(buffer[16] == 0xef);
	CHECK(buffer[17] == 0xcd);
	CHECK(buffer[18] == 0xab);
	CHECK(buffer[19] == 0x89);
	CHECK(buffer[20] == 0x67);
	CHECK(buffer[21] == 0x45);
	CHECK(buffer[22] == 0x23);
	CHECK(buffer[23] == 0xf1);
}

TEST_CASE("[Marshalls] Typed array decoding") {
	Variant variant;
	int r_len;
	uint8_t buffer[] = {
		0x1c, 0x00, 0x01, 0x00, // Variant::ARRAY, CONTAINER_TYPE_KIND_BUILTIN
		0x02, 0x00, 0x00, 0x00, // Array type (Variant::INT).
		0x01, 0x00, 0x00, 0x00, // Array size.
		0x02, 0x00, 0x01, 0x00, // Element type (Variant::INT, HEADER_DATA_FLAG_64).
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0xf1 // Element value.
	};

	CHECK(decode_variant(variant, buffer, 24, &r_len) == OK);
	CHECK(r_len == 24);
	CHECK(variant.get_type() == Variant::ARRAY);
	Array array = variant;
	CHECK(array.get_typed_builtin() == Variant::INT);
	CHECK(array.size() == 1);
	CHECK(array[0] == Variant(uint64_t(0x0f123456789abcdef)));
}

TEST_CASE("[Marshalls] Typed dicttionary encoding") {
	int r_len;
	Dictionary dictionary;
	dictionary.set_typed(Variant::INT, String(), Ref<Script>(), Variant::INT, String(), Ref<Script>());
	dictionary[Variant(uint64_t(0x0f123456789abcdef))] = Variant(uint64_t(0x0f123456789abcdef));
	uint8_t buffer[40];

	CHECK(encode_variant(dictionary, buffer, r_len) == OK);
	CHECK_MESSAGE(r_len == 40, "Length == 4 bytes for header + 8 bytes for dictionary type + 4 bytes for dictionary size + 24 bytes for key-value pair.");
	CHECK_MESSAGE(buffer[0] == 0x1b, "Variant::DICTIONARY");
	CHECK(buffer[1] == 0x00);
	CHECK_MESSAGE(buffer[2] == 0x05, "key: CONTAINER_TYPE_KIND_BUILTIN | value: CONTAINER_TYPE_KIND_BUILTIN");
	CHECK(buffer[3] == 0x00);
	// Check dictionary key type.
	CHECK_MESSAGE(buffer[4] == 0x02, "Variant::INT");
	CHECK(buffer[5] == 0x00);
	CHECK(buffer[6] == 0x00);
	CHECK(buffer[7] == 0x00);
	// Check dictionary value type.
	CHECK_MESSAGE(buffer[8] == 0x02, "Variant::INT");
	CHECK(buffer[9] == 0x00);
	CHECK(buffer[10] == 0x00);
	CHECK(buffer[11] == 0x00);
	// Check dictionary size.
	CHECK(buffer[12] == 0x01);
	CHECK(buffer[13] == 0x00);
	CHECK(buffer[14] == 0x00);
	CHECK(buffer[15] == 0x00);
	// Check key type.
	CHECK_MESSAGE(buffer[16] == 0x02, "Variant::INT");
	CHECK(buffer[17] == 0x00);
	CHECK_MESSAGE(buffer[18] == 0x01, "HEADER_DATA_FLAG_64");
	CHECK(buffer[19] == 0x00);
	// Check key value.
	CHECK(buffer[20] == 0xef);
	CHECK(buffer[21] == 0xcd);
	CHECK(buffer[22] == 0xab);
	CHECK(buffer[23] == 0x89);
	CHECK(buffer[24] == 0x67);
	CHECK(buffer[25] == 0x45);
	CHECK(buffer[26] == 0x23);
	CHECK(buffer[27] == 0xf1);
	// Check value type.
	CHECK_MESSAGE(buffer[28] == 0x02, "Variant::INT");
	CHECK(buffer[29] == 0x00);
	CHECK_MESSAGE(buffer[30] == 0x01, "HEADER_DATA_FLAG_64");
	CHECK(buffer[31] == 0x00);
	// Check value value.
	CHECK(buffer[32] == 0xef);
	CHECK(buffer[33] == 0xcd);
	CHECK(buffer[34] == 0xab);
	CHECK(buffer[35] == 0x89);
	CHECK(buffer[36] == 0x67);
	CHECK(buffer[37] == 0x45);
	CHECK(buffer[38] == 0x23);
	CHECK(buffer[39] == 0xf1);
}

TEST_CASE("[Marshalls] Typed dictionary decoding") {
	Variant variant;
	int r_len;
	uint8_t buffer[] = {
		0x1b, 0x00, 0x05, 0x00, // Variant::DICTIONARY, key: CONTAINER_TYPE_KIND_BUILTIN | value: CONTAINER_TYPE_KIND_BUILTIN
		0x02, 0x00, 0x00, 0x00, // Dictionary key type (Variant::INT).
		0x02, 0x00, 0x00, 0x00, // Dictionary value type (Variant::INT).
		0x01, 0x00, 0x00, 0x00, // Dictionary size.
		0x02, 0x00, 0x01, 0x00, // Key type (Variant::INT, HEADER_DATA_FLAG_64).
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0xf1, // Key value.
		0x02, 0x00, 0x01, 0x00, // Value type (Variant::INT, HEADER_DATA_FLAG_64).
		0xef, 0xcd, 0xab, 0x89, 0x67, 0x45, 0x23, 0xf1 // Value value.
	};

	CHECK(decode_variant(variant, buffer, 40, &r_len) == OK);
	CHECK(r_len == 40);
	CHECK(variant.get_type() == Variant::DICTIONARY);
	Dictionary dictionary = variant;
	CHECK(dictionary.get_typed_key_builtin() == Variant::INT);
	CHECK(dictionary.get_typed_value_builtin() == Variant::INT);
	CHECK(dictionary.size() == 1);
	CHECK(dictionary.has(Variant(uint64_t(0x0f123456789abcdef))));
	CHECK(dictionary[Variant(uint64_t(0x0f123456789abcdef))] == Variant(uint64_t(0x0f123456789abcdef)));
}

} // namespace TestMarshalls

// === Merged from tests/core/io/test_packet_peer.cpp ===
/*  test_packet_peer.cpp                                                  */
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

namespace TestPacketPeer {

TEST_CASE("[PacketPeer][PacketPeerStream] Encode buffer max size") {
	Ref<PacketPeerStream> pps;
	pps.instantiate();

	SUBCASE("Default value") {
		CHECK_EQ(pps->get_encode_buffer_max_size(), 8 * 1024 * 1024);
	}

	SUBCASE("Max encode buffer must be at least 1024 bytes") {
		ERR_PRINT_OFF;
		pps->set_encode_buffer_max_size(42);
		ERR_PRINT_ON;

		CHECK_EQ(pps->get_encode_buffer_max_size(), 8 * 1024 * 1024);
	}

	SUBCASE("Max encode buffer cannot exceed 256 MiB") {
		ERR_PRINT_OFF;
		pps->set_encode_buffer_max_size((256 * 1024 * 1024) + 42);
		ERR_PRINT_ON;

		CHECK_EQ(pps->get_encode_buffer_max_size(), 8 * 1024 * 1024);
	}

	SUBCASE("Should be next power of two") {
		pps->set_encode_buffer_max_size(2000);

		CHECK_EQ(pps->get_encode_buffer_max_size(), 2048);
	}
}

TEST_CASE("[PacketPeer][PacketPeerStream] Read a variant from peer") {
	String godot_rules = "Godot Rules!!!";

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	spb->put_var(godot_rules);
	spb->seek(0);

	Ref<PacketPeerStream> pps;
	pps.instantiate();
	pps->set_stream_peer(spb);

	Variant value;
	CHECK_EQ(pps->get_var(value), Error::OK);
	CHECK_EQ(String(value), godot_rules);
}

TEST_CASE("[PacketPeer][PacketPeerStream] Read a variant from peer fails") {
	Ref<PacketPeerStream> pps;
	pps.instantiate();

	Variant value;
	ERR_PRINT_OFF;
	CHECK_EQ(pps->get_var(value), Error::ERR_UNCONFIGURED);
	ERR_PRINT_ON;
}

TEST_CASE("[PacketPeer][PacketPeerStream] Put a variant to peer") {
	String godot_rules = "Godot Rules!!!";

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	Ref<PacketPeerStream> pps;
	pps.instantiate();
	pps->set_stream_peer(spb);

	CHECK_EQ(pps->put_var(godot_rules), Error::OK);

	spb->seek(0);
	CHECK_EQ(String(spb->get_var()), godot_rules);
}

TEST_CASE("[PacketPeer][PacketPeerStream] Put a variant to peer out of memory failure") {
	String more_than_1mb = String("*").repeat(1024 + 1);

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	Ref<PacketPeerStream> pps;
	pps.instantiate();
	pps->set_stream_peer(spb);
	pps->set_encode_buffer_max_size(1024);

	ERR_PRINT_OFF;
	CHECK_EQ(pps->put_var(more_than_1mb), Error::ERR_OUT_OF_MEMORY);
	ERR_PRINT_ON;
}

TEST_CASE("[PacketPeer][PacketPeerStream] Get packet buffer") {
	String godot_rules = "Godot Rules!!!";

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	// First 4 bytes are the length of the string.
	CharString cs = godot_rules.ascii();
	Vector<uint8_t> buffer = { (uint8_t)(cs.length() + 1), 0, 0, 0 };
	buffer.resize_initialized(4 + cs.length() + 1);
	memcpy(buffer.ptrw() + 4, cs.get_data(), cs.length());
	spb->set_data_array(buffer);

	Ref<PacketPeerStream> pps;
	pps.instantiate();
	pps->set_stream_peer(spb);

	buffer.clear();
	CHECK_EQ(pps->get_packet_buffer(buffer), Error::OK);

	CHECK_EQ(String(reinterpret_cast<const char *>(buffer.ptr())), godot_rules);
}

TEST_CASE("[PacketPeer][PacketPeerStream] Get packet buffer from an empty peer") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	Ref<PacketPeerStream> pps;
	pps.instantiate();
	pps->set_stream_peer(spb);

	Vector<uint8_t> buffer;
	ERR_PRINT_OFF;
	CHECK_EQ(pps->get_packet_buffer(buffer), Error::ERR_UNAVAILABLE);
	ERR_PRINT_ON;
	CHECK_EQ(buffer.size(), 0);
}

TEST_CASE("[PacketPeer][PacketPeerStream] Put packet buffer") {
	String godot_rules = "Godot Rules!!!";

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	Ref<PacketPeerStream> pps;
	pps.instantiate();
	pps->set_stream_peer(spb);

	CHECK_EQ(pps->put_packet_buffer(godot_rules.to_ascii_buffer()), Error::OK);

	spb->seek(0);
	CHECK_EQ(spb->get_string(), godot_rules);
	// First 4 bytes are the length of the string.
	CharString cs = godot_rules.ascii();
	Vector<uint8_t> buffer = { (uint8_t)cs.length(), 0, 0, 0 };
	buffer.resize(4 + cs.length());
	memcpy(buffer.ptrw() + 4, cs.get_data(), cs.length());
	CHECK_EQ(spb->get_data_array(), buffer);
}

TEST_CASE("[PacketPeer][PacketPeerStream] Put packet buffer when is empty") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	Ref<PacketPeerStream> pps;
	pps.instantiate();
	pps->set_stream_peer(spb);

	Vector<uint8_t> buffer;
	CHECK_EQ(pps->put_packet_buffer(buffer), Error::OK);

	CHECK_EQ(spb->get_size(), 0);
}

} // namespace TestPacketPeer

// === Merged from tests/core/io/test_pck_packer.cpp ===
/*  test_pck_packer.cpp                                                   */
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
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "tests/test_macros.h"

namespace TestPCKPacker {

TEST_CASE("[PCKPacker] Pack an empty PCK file") {
	PCKPacker pck_packer;
	const String output_pck_path = TestUtils::get_temp_path("output_empty.pck");
	CHECK_MESSAGE(
			pck_packer.pck_start(output_pck_path) == OK,
			"Starting a PCK file should return an OK error code.");

	CHECK_MESSAGE(
			pck_packer.flush() == OK,
			"Flushing the PCK should return an OK error code.");

	Error err;
	Ref<FileAccess> f = FileAccess::open(output_pck_path, FileAccess::READ, &err);
	CHECK_MESSAGE(
			err == OK,
			"The generated empty PCK file should be opened successfully.");
	CHECK_MESSAGE(
			f->get_length() >= 100,
			"The generated empty PCK file shouldn't be too small (it should have the PCK header).");
	CHECK_MESSAGE(
			f->get_length() <= 500,
			"The generated empty PCK file shouldn't be too large.");
}

TEST_CASE("[PCKPacker] Pack empty with zero alignment invalid") {
	PCKPacker pck_packer;
	const String output_pck_path = TestUtils::get_temp_path("output_empty.pck");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(pck_packer.pck_start(output_pck_path, 0) != OK, "PCK with zero alignment should fail.");
	ERR_PRINT_ON;
}

TEST_CASE("[PCKPacker] Pack empty with invalid key") {
	PCKPacker pck_packer;
	const String output_pck_path = TestUtils::get_temp_path("output_empty.pck");
	ERR_PRINT_OFF;
	CHECK_MESSAGE(pck_packer.pck_start(output_pck_path, 32, "") != OK, "PCK with invalid key should fail.");
	ERR_PRINT_ON;
}

TEST_CASE("[PCKPacker] Pack a PCK file with some files and directories") {
	PCKPacker pck_packer;
	const String output_pck_path = TestUtils::get_temp_path("output_with_files.pck");
	CHECK_MESSAGE(
			pck_packer.pck_start(output_pck_path) == OK,
			"Starting a PCK file should return an OK error code.");

	const String base_dir = OS::get_executable_path().get_base_dir();

	CHECK_MESSAGE(
			pck_packer.add_file("version.py", base_dir.path_join("../version.py"), "version.py") == OK,
			"Adding a file to the PCK should return an OK error code.");
	CHECK_MESSAGE(
			pck_packer.add_file("some/directories with spaces/to/create/icon.png", base_dir.path_join("../misc/logo/icon.png")) == OK,
			"Adding a file to a new subdirectory in the PCK should return an OK error code.");
	CHECK_MESSAGE(
			pck_packer.add_file("some/directories with spaces/to/create/icon.svg", base_dir.path_join("../misc/logo/icon.svg")) == OK,
			"Adding a file to an existing subdirectory in the PCK should return an OK error code.");
	CHECK_MESSAGE(
			pck_packer.add_file("some/directories with spaces/to/create/icon.png", base_dir.path_join("../misc/logo/logo.png")) == OK,
			"Overriding a non-flushed file to an existing subdirectory in the PCK should return an OK error code.");
	CHECK_MESSAGE(
			pck_packer.add_file_from_buffer("buffer/new.txt", String("Hello world!").to_utf8_buffer()) == OK,
			"Adding a file from a buffer to the PCK in a new subdirectory should return an OK error code.");
	CHECK_MESSAGE(
			pck_packer.flush() == OK,
			"Flushing the PCK should return an OK error code.");

	Error err;
	Ref<FileAccess> f = FileAccess::open(output_pck_path, FileAccess::READ, &err);
	CHECK_MESSAGE(
			err == OK,
			"The generated non-empty PCK file should be opened successfully.");
	CHECK_MESSAGE(
			f->get_length() >= 18000,
			"The generated non-empty PCK file should be large enough to actually hold the contents specified above.");
	CHECK_MESSAGE(
			f->get_length() <= 27000,
			"The generated non-empty PCK file shouldn't be too large.");
}

} // namespace TestPCKPacker

// === Merged from tests/core/io/test_resource.cpp ===
/*  test_resource.cpp                                                     */
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

// === Merged from tests/core/io/test_resource_uid.cpp ===
/*  test_resource_uid.cpp                                                 */
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

namespace TestResourceUID {

TEST_CASE("[ResourceUID] Must encode/decode maximum/minimum UID correctly") {
	CHECK_MESSAGE(ResourceUID::id_to_text(0x7fffffffffffffff) == "uid://d4n4ub6itg400", "Maximum UID must encode correctly.");
	CHECK_MESSAGE(ResourceUID::text_to_id("uid://d4n4ub6itg400") == 0x7fffffffffffffff, "Maximum UID must decode correctly.");

	CHECK_MESSAGE(ResourceUID::id_to_text(0) == "uid://a", "Minimum UID must encode correctly.");
	CHECK_MESSAGE(ResourceUID::text_to_id("uid://a") == 0, "Minimum UID must decode correctly.");
}

TEST_CASE("[ResourceUID] Must encode and decode invalid UIDs correctly") {
	CHECK_MESSAGE(ResourceUID::id_to_text(-1) == "uid://<invalid>", "Invalid UID -1 must encode correctly.");
	CHECK_MESSAGE(ResourceUID::text_to_id("uid://<invalid>") == -1, "Invalid UID -1 must decode correctly.");

	CHECK_MESSAGE(ResourceUID::id_to_text(-2) == ResourceUID::id_to_text(-1), "Invalid UID -2 must encode to the same as -1.");

	CHECK_MESSAGE(ResourceUID::text_to_id("dm3rdgs30kfci") == -1, "UID without scheme must decode correctly.");
}

TEST_CASE("[ResourceUID] Must encode and decode various UIDs correctly") {
	CHECK_MESSAGE(ResourceUID::id_to_text(1) == "uid://b", "UID 1 must encode correctly.");
	CHECK_MESSAGE(ResourceUID::text_to_id("uid://b") == 1, "UID 1 must decode correctly.");

	CHECK_MESSAGE(ResourceUID::id_to_text(8060368642360689600) == "uid://dm3rdgs30kfci", "A normal UID must encode correctly.");
	CHECK_MESSAGE(ResourceUID::text_to_id("uid://dm3rdgs30kfci") == 8060368642360689600, "A normal UID must decode correctly.");
}

} // namespace TestResourceUID

// === Merged from tests/core/io/test_stream_peer.cpp ===
/*  test_stream_peer.cpp                                                  */
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

namespace TestStreamPeer {

TEST_CASE("[StreamPeer] Initialization through StreamPeerBuffer") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	CHECK_EQ(spb->is_big_endian_enabled(), false);
}

TEST_CASE("[StreamPeer] Get and sets through StreamPeerBuffer") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	SUBCASE("A int8_t value") {
		int8_t value = 42;

		spb->clear();
		spb->put_8(value);
		spb->seek(0);

		CHECK_EQ(spb->get_8(), value);
	}

	SUBCASE("A uint8_t value") {
		uint8_t value = 42;

		spb->clear();
		spb->put_u8(value);
		spb->seek(0);

		CHECK_EQ(spb->get_u8(), value);
	}

	SUBCASE("A int16_t value") {
		int16_t value = 42;

		spb->clear();
		spb->put_16(value);
		spb->seek(0);

		CHECK_EQ(spb->get_16(), value);
	}

	SUBCASE("A uint16_t value") {
		uint16_t value = 42;

		spb->clear();
		spb->put_u16(value);
		spb->seek(0);

		CHECK_EQ(spb->get_u16(), value);
	}

	SUBCASE("A int32_t value") {
		int32_t value = 42;

		spb->clear();
		spb->put_32(value);
		spb->seek(0);

		CHECK_EQ(spb->get_32(), value);
	}

	SUBCASE("A uint32_t value") {
		uint32_t value = 42;

		spb->clear();
		spb->put_u32(value);
		spb->seek(0);

		CHECK_EQ(spb->get_u32(), value);
	}

	SUBCASE("A int64_t value") {
		int64_t value = 42;

		spb->clear();
		spb->put_64(value);
		spb->seek(0);

		CHECK_EQ(spb->get_64(), value);
	}

	SUBCASE("A int64_t value") {
		uint64_t value = 42;

		spb->clear();
		spb->put_u64(value);
		spb->seek(0);

		CHECK_EQ(spb->get_u64(), value);
	}

	SUBCASE("A half-precision float value") {
		float value = 3.1415927f;
		float expected = 3.14062f;

		spb->clear();
		spb->put_half(value);
		spb->seek(0);

		CHECK(spb->get_half() == doctest::Approx(expected));
	}

	SUBCASE("A float value") {
		float value = 42.0f;

		spb->clear();
		spb->put_float(value);
		spb->seek(0);

		CHECK_EQ(spb->get_float(), value);
	}

	SUBCASE("A double value") {
		double value = 42.0;

		spb->clear();
		spb->put_double(value);
		spb->seek(0);

		CHECK_EQ(spb->get_double(), value);
	}

	SUBCASE("A string value") {
		String value = "Hello, World!";

		spb->clear();
		spb->put_string(value);
		spb->seek(0);

		CHECK_EQ(spb->get_string(), value);
	}

	SUBCASE("A utf8 string value") {
		String value = String::utf8("Hello✩, World✩!");

		spb->clear();
		spb->put_utf8_string(value);
		spb->seek(0);

		CHECK_EQ(spb->get_utf8_string(), value);
	}

	SUBCASE("A variant value") {
		Array value;
		value.push_front(42);
		value.push_front("Hello, World!");

		spb->clear();
		spb->put_var(value);
		spb->seek(0);

		CHECK_EQ(spb->get_var(), value);
	}
}

TEST_CASE("[StreamPeer] Get and sets big endian through StreamPeerBuffer") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	spb->set_big_endian(true);

	SUBCASE("A int16_t value") {
		int16_t value = 42;

		spb->clear();
		spb->put_16(value);
		spb->seek(0);

		CHECK_EQ(spb->get_16(), value);
	}

	SUBCASE("A uint16_t value") {
		uint16_t value = 42;

		spb->clear();
		spb->put_u16(value);
		spb->seek(0);

		CHECK_EQ(spb->get_u16(), value);
	}

	SUBCASE("A int32_t value") {
		int32_t value = 42;

		spb->clear();
		spb->put_32(value);
		spb->seek(0);

		CHECK_EQ(spb->get_32(), value);
	}

	SUBCASE("A uint32_t value") {
		uint32_t value = 42;

		spb->clear();
		spb->put_u32(value);
		spb->seek(0);

		CHECK_EQ(spb->get_u32(), value);
	}

	SUBCASE("A int64_t value") {
		int64_t value = 42;

		spb->clear();
		spb->put_64(value);
		spb->seek(0);

		CHECK_EQ(spb->get_64(), value);
	}

	SUBCASE("A int64_t value") {
		uint64_t value = 42;

		spb->clear();
		spb->put_u64(value);
		spb->seek(0);

		CHECK_EQ(spb->get_u64(), value);
	}

	SUBCASE("A float value") {
		float value = 42.0f;

		spb->clear();
		spb->put_float(value);
		spb->seek(0);

		CHECK_EQ(spb->get_float(), value);
	}

	SUBCASE("A half-precision float value") {
		float value = 3.1415927f;
		float expected = 3.14062f;

		spb->clear();
		spb->put_half(value);
		spb->seek(0);

		CHECK(spb->get_half() == doctest::Approx(expected));
	}

	SUBCASE("A double value") {
		double value = 42.0;

		spb->clear();
		spb->put_double(value);
		spb->seek(0);

		CHECK_EQ(spb->get_double(), value);
	}
}

TEST_CASE("[StreamPeer] Get string when there is no string") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	ERR_PRINT_OFF;
	CHECK_EQ(spb->get_string(), "");
	ERR_PRINT_ON;
}

TEST_CASE("[StreamPeer] Get UTF8 string when there is no string") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();

	ERR_PRINT_OFF;
	CHECK_EQ(spb->get_utf8_string(), "");
	ERR_PRINT_ON;
}

} // namespace TestStreamPeer

// === Merged from tests/core/io/test_stream_peer_buffer.cpp ===
/*  test_stream_peer_buffer.cpp                                           */
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

namespace TestStreamPeerBuffer {

TEST_CASE("[StreamPeerBuffer] Initialization") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	CHECK_EQ(spb->get_size(), 0);
	CHECK_EQ(spb->get_position(), 0);
	CHECK_EQ(spb->get_available_bytes(), 0);
}

TEST_CASE("[StreamPeerBuffer] Seek") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	uint8_t first = 5;
	uint8_t second = 7;
	uint8_t third = 11;

	spb->put_u8(first);
	spb->put_u8(second);
	spb->put_u8(third);

	spb->seek(0);
	CHECK_EQ(spb->get_u8(), first);
	CHECK_EQ(spb->get_u8(), second);
	CHECK_EQ(spb->get_u8(), third);

	spb->seek(1);
	CHECK_EQ(spb->get_position(), 1);
	CHECK_EQ(spb->get_u8(), second);

	spb->seek(1);
	ERR_PRINT_OFF;
	spb->seek(-1);
	ERR_PRINT_ON;
	CHECK_EQ(spb->get_position(), 1);
	ERR_PRINT_OFF;
	spb->seek(5);
	ERR_PRINT_ON;
	CHECK_EQ(spb->get_position(), 1);
}

TEST_CASE("[StreamPeerBuffer] Resize") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	CHECK_EQ(spb->get_size(), 0);
	CHECK_EQ(spb->get_position(), 0);
	CHECK_EQ(spb->get_available_bytes(), 0);

	spb->resize(42);
	CHECK_EQ(spb->get_size(), 42);
	CHECK_EQ(spb->get_position(), 0);
	CHECK_EQ(spb->get_available_bytes(), 42);

	spb->seek(21);
	CHECK_EQ(spb->get_size(), 42);
	CHECK_EQ(spb->get_position(), 21);
	CHECK_EQ(spb->get_available_bytes(), 21);
}

TEST_CASE("[StreamPeerBuffer] Get underlying data array") {
	uint8_t first = 5;
	uint8_t second = 7;
	uint8_t third = 11;

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	spb->put_u8(first);
	spb->put_u8(second);
	spb->put_u8(third);

	Vector<uint8_t> data_array = spb->get_data_array();

	CHECK_EQ(data_array[0], first);
	CHECK_EQ(data_array[1], second);
	CHECK_EQ(data_array[2], third);
}

TEST_CASE("[StreamPeerBuffer] Set underlying data array") {
	uint8_t first = 5;
	uint8_t second = 7;
	uint8_t third = 11;

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	spb->put_u8(1);
	spb->put_u8(2);
	spb->put_u8(3);

	Vector<uint8_t> new_data_array;
	new_data_array.push_back(first);
	new_data_array.push_back(second);
	new_data_array.push_back(third);

	spb->set_data_array(new_data_array);

	CHECK_EQ(spb->get_u8(), first);
	CHECK_EQ(spb->get_u8(), second);
	CHECK_EQ(spb->get_u8(), third);
}

TEST_CASE("[StreamPeerBuffer] Duplicate") {
	uint8_t first = 5;
	uint8_t second = 7;
	uint8_t third = 11;

	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	spb->put_u8(first);
	spb->put_u8(second);
	spb->put_u8(third);

	Ref<StreamPeerBuffer> spb2 = spb->duplicate();

	CHECK_EQ(spb2->get_u8(), first);
	CHECK_EQ(spb2->get_u8(), second);
	CHECK_EQ(spb2->get_u8(), third);
}

TEST_CASE("[StreamPeerBuffer] Put data with size equal to zero does nothing") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	uint8_t data = 42;

	Error error = spb->put_data((const uint8_t *)&data, 0);

	CHECK_EQ(error, OK);
	CHECK_EQ(spb->get_size(), 0);
	CHECK_EQ(spb->get_position(), 0);
	CHECK_EQ(spb->get_available_bytes(), 0);
}

TEST_CASE("[StreamPeerBuffer] Get data with invalid size returns an error") {
	Ref<StreamPeerBuffer> spb;
	spb.instantiate();
	uint8_t data = 42;
	spb->put_u8(data);
	spb->seek(0);

	uint8_t data_out = 0;
	Error error = spb->get_data(&data_out, 3);

	CHECK_EQ(error, ERR_INVALID_PARAMETER);
	CHECK_EQ(spb->get_size(), 1);
	CHECK_EQ(spb->get_position(), 1);
}

} // namespace TestStreamPeerBuffer

// === Merged from tests/core/io/test_stream_peer_gzip.cpp ===
/*  test_stream_peer_gzip.cpp                                             */
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

namespace TestStreamPeerGZIP {

const String hello = "Hello World!!!";

TEST_CASE("[StreamPeerGZIP] Initialization") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();
	CHECK_EQ(spgz->get_available_bytes(), 0);
}

TEST_CASE("[StreamPeerGZIP] Compress/Decompress") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();

	bool is_deflate = false;

	SUBCASE("GZIP") {
		is_deflate = false;
	}

	SUBCASE("DEFLATE") {
		is_deflate = true;
	}

	CHECK_EQ(spgz->start_compression(is_deflate), Error::OK);
	CHECK_EQ(spgz->put_data(hello.to_ascii_buffer().ptr(), hello.to_ascii_buffer().size()), Error::OK);
	CHECK_EQ(spgz->finish(), Error::OK);

	Vector<uint8_t> hello_compressed;
	int hello_compressed_size = spgz->get_available_bytes();
	hello_compressed.resize(hello_compressed_size);
	CHECK_EQ(spgz->get_data(hello_compressed.ptrw(), hello_compressed_size), Error::OK);

	spgz->clear();

	CHECK_EQ(spgz->start_decompression(is_deflate), Error::OK);
	CHECK_EQ(spgz->put_data(hello_compressed.ptr(), hello_compressed.size()), Error::OK);

	Vector<uint8_t> hello_decompressed;
	int hello_decompressed_size = spgz->get_available_bytes();
	hello_decompressed.resize(hello_decompressed_size);
	CHECK_EQ(spgz->get_data(hello_decompressed.ptrw(), hello_decompressed_size), Error::OK);
	CHECK_EQ(hello_decompressed, hello.to_ascii_buffer());
}

TEST_CASE("[StreamPeerGZIP] Compress/Decompress big chunks of data") { // GH-97201
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();
	CHECK_EQ(spgz->start_compression(false), Error::OK);

	Vector<uint8_t> big_data;
	big_data.resize(2500);
	// Filling it with random data because the issue is related to the size of the data when it's compress.
	// Random data results in bigger compressed data size.
	for (int i = 0; i < big_data.size(); i++) {
		big_data.write[i] = Math::random(48, 122);
	}
	CHECK_EQ(spgz->put_data(big_data.ptr(), big_data.size()), Error::OK);
	CHECK_EQ(spgz->finish(), Error::OK);

	Vector<uint8_t> big_data_compressed;
	int big_data_compressed_size = spgz->get_available_bytes();
	big_data_compressed.resize(big_data_compressed_size);
	CHECK_EQ(spgz->get_data(big_data_compressed.ptrw(), big_data_compressed_size), Error::OK);

	spgz->clear();

	CHECK_EQ(spgz->start_decompression(false), Error::OK);
	CHECK_EQ(spgz->put_data(big_data_compressed.ptr(), big_data_compressed.size()), Error::OK);

	Vector<uint8_t> big_data_decompressed;
	int big_data_decompressed_size = spgz->get_available_bytes();
	big_data_decompressed.resize(big_data_decompressed_size);
	CHECK_EQ(spgz->get_data(big_data_decompressed.ptrw(), big_data_decompressed_size), Error::OK);
	CHECK_EQ(big_data_decompressed, big_data);
}

TEST_CASE("[StreamPeerGZIP] Can't start twice") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();
	CHECK_EQ(spgz->start_compression(false), Error::OK);

	ERR_PRINT_OFF;
	CHECK_EQ(spgz->start_compression(false), Error::ERR_ALREADY_IN_USE);
	CHECK_EQ(spgz->start_decompression(false), Error::ERR_ALREADY_IN_USE);
	ERR_PRINT_ON;
}

TEST_CASE("[StreamPeerGZIP] Can't start with a buffer size equal or less than zero") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();

	ERR_PRINT_OFF;
	CHECK_EQ(spgz->start_compression(false, 0), Error::ERR_INVALID_PARAMETER);
	CHECK_EQ(spgz->start_compression(false, -1), Error::ERR_INVALID_PARAMETER);
	CHECK_EQ(spgz->start_decompression(false, 0), Error::ERR_INVALID_PARAMETER);
	CHECK_EQ(spgz->start_decompression(false, -1), Error::ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
}

TEST_CASE("[StreamPeerGZIP] Can't put/get data with a buffer size less than zero") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();
	CHECK_EQ(spgz->start_compression(false), Error::OK);

	ERR_PRINT_OFF;
	CHECK_EQ(spgz->put_data(hello.to_ascii_buffer().ptr(), -1), Error::ERR_INVALID_PARAMETER);

	Vector<uint8_t> hello_compressed;
	hello_compressed.resize(5);
	CHECK_EQ(spgz->get_data(hello_compressed.ptrw(), -1), Error::ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
}

TEST_CASE("[StreamPeerGZIP] Needs to be started before use") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();

	ERR_PRINT_OFF;
	CHECK_EQ(spgz->put_data(hello.to_ascii_buffer().ptr(), hello.to_ascii_buffer().size()), Error::ERR_UNCONFIGURED);
	ERR_PRINT_ON;
}

TEST_CASE("[StreamPeerGZIP] Can't be finished after clear or if it's decompressing") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();

	CHECK_EQ(spgz->start_compression(false), Error::OK);
	spgz->clear();
	ERR_PRINT_OFF;
	CHECK_EQ(spgz->finish(), Error::ERR_UNAVAILABLE);
	ERR_PRINT_ON;

	spgz->clear();
	CHECK_EQ(spgz->start_decompression(false), Error::OK);
	ERR_PRINT_OFF;
	CHECK_EQ(spgz->finish(), Error::ERR_UNAVAILABLE);
	ERR_PRINT_ON;
}

TEST_CASE("[StreamPeerGZIP] Fails to get if nothing was compress/decompress") {
	Ref<StreamPeerGZIP> spgz;
	spgz.instantiate();

	SUBCASE("Compression") {
		CHECK_EQ(spgz->start_compression(false), Error::OK);
	}

	SUBCASE("Decompression") {
		CHECK_EQ(spgz->start_decompression(false), Error::OK);
	}

	ERR_PRINT_OFF;
	Vector<uint8_t> hello_compressed;
	hello_compressed.resize(5);
	CHECK_EQ(spgz->get_data(hello_compressed.ptrw(), hello_compressed.size()), Error::ERR_UNAVAILABLE);
	ERR_PRINT_ON;
}

} // namespace TestStreamPeerGZIP

// === Merged from tests/core/io/test_stream_peer_tcp.cpp ===
/*  test_stream_peer_tcp.cpp                                              */
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
#include "core/io/net_socket.h"

namespace TestStreamPeerTCP {

class MockNetSocket : public NetSocket {

public:
	static void make_default();
	static void cleanup();

	virtual Error open(Family p_family, Type p_type, IP::Type &ip_type) override;
	virtual void close() override;
	virtual Error bind(Address p_addr) override;
	virtual Error listen(int p_max_pending) override;
	virtual Error connect_to_host(Address p_addr) override;
	virtual Error poll(PollType p_type, int timeout) const override;
	virtual Error recv(uint8_t *p_buffer, int p_len, int &r_read) override;
	virtual Error recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek) override;
	virtual Error send(const uint8_t *p_buffer, int p_len, int &r_sent) override;
	virtual Error sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) override;
	virtual Ref<NetSocket> accept(Address &r_addr) override;

	virtual bool is_open() const override;
	virtual int get_available_bytes() const override;
	virtual Error get_socket_address(Address *r_ip) const override;

	virtual Error set_broadcasting_enabled(bool p_enabled) override;
	virtual void set_blocking_enabled(bool p_enabled) override;
	virtual void set_ipv6_only_enabled(bool p_enabled) override;
	virtual void set_tcp_no_delay_enabled(bool p_enabled) override;
	virtual void set_reuse_address_enabled(bool p_enabled) override;
	virtual Error join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override;
	virtual Error leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) override;

	Address host_addr;
	Address dest_ip;
	bool blocking_enabled = true;

	// Helper methods for testing.
	void _set_available_bytes(int p_available_bytes);
	void _set_send_data(uint8_t *p_sent_data);
	void _set_recv_data(uint8_t *p_recv_data);

	MockNetSocket();
	~MockNetSocket() override;

protected:
	static Ref<NetSocket> _create_func();

private:
	bool _is_open = false;
	int _available_bytes = 0;
	uint8_t *_sent_data = nullptr;
	uint8_t *_recv_data = nullptr;
};

Ref<NetSocket> MockNetSocket::_create_func() {
	return memnew(MockNetSocket);
}

void MockNetSocket::_set_available_bytes(int p_available_bytes) {
	_available_bytes = p_available_bytes;
}

void MockNetSocket::_set_send_data(uint8_t *p_sent_data) {
	_sent_data = p_sent_data;
}

void MockNetSocket::_set_recv_data(uint8_t *p_recv_data) {
	_recv_data = p_recv_data;
}

void MockNetSocket::make_default() {
	ERR_FAIL_COND(_create != nullptr);

	_create = _create_func;
}

void MockNetSocket::cleanup() {
	ERR_FAIL_NULL(_create);

	_create = nullptr;
}

Error MockNetSocket::open(Family p_family, Type p_type, IP::Type &ip_type) {
	_is_open = true;
	return OK;
}

void MockNetSocket::close() {
	_is_open = false;
}

Error MockNetSocket::bind(Address p_addr) {
	host_addr = p_addr;
	return OK;
}

Error MockNetSocket::listen(int p_max_pending) {
	return OK;
}

Error MockNetSocket::connect_to_host(Address p_addr) {
	dest_ip = p_addr;
	return OK;
}

Error MockNetSocket::poll(PollType p_type, int timeout) const {
	return OK;
}

Error MockNetSocket::recv(uint8_t *p_buffer, int p_len, int &r_read) {
	// Receives one byte of _recv_data on each invocation.
	p_buffer[0] = _recv_data[0];
	_recv_data += 1;
	r_read = 1;
	return OK;
}

Error MockNetSocket::recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek = false) {
	return OK;
}

Error MockNetSocket::send(const uint8_t *p_buffer, int p_len, int &r_sent) {
	// Sends one byte to _sent_data on each invocation.
	_sent_data[0] = p_buffer[0];
	_sent_data += 1;
	r_sent = 1;
	return OK;
}

Error MockNetSocket::sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) {
	return OK;
}

Ref<NetSocket> MockNetSocket::accept(Address &r_ip) {
	return this;
}

bool MockNetSocket::is_open() const {
	return _is_open;
}

int MockNetSocket::get_available_bytes() const {
	return _available_bytes;
}

Error MockNetSocket::get_socket_address(Address *r_ip) const {
	return OK;
}

Error MockNetSocket::set_broadcasting_enabled(bool p_enabled) {
	return OK;
}

void MockNetSocket::set_blocking_enabled(bool p_enabled) {
	blocking_enabled = p_enabled;
}

void MockNetSocket::set_ipv6_only_enabled(bool p_enabled) {}

void MockNetSocket::set_tcp_no_delay_enabled(bool p_enabled) {}

void MockNetSocket::set_reuse_address_enabled(bool p_enabled) {}

Error MockNetSocket::join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return OK;
}

Error MockNetSocket::leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return OK;
}

MockNetSocket::MockNetSocket() {}

MockNetSocket::~MockNetSocket() {}

TEST_CASE("[StreamPeerTCP] Basics") {
	Ref<MockNetSocket> ns;
	ns.instantiate();
	Ref<StreamPeerSocket> spt;
	spt.instantiate();
	NetSocket::Address peer_ip = NetSocket::Address(IPAddress("127.0.1.1"), 5678);
	spt->accept_socket(ns, peer_ip);
	REQUIRE(ns->blocking_enabled == false);

	SUBCASE("Invalid port numbers returns an Error") {
		ERR_PRINT_OFF;
		Error negative_ret = spt->bind(-901, IPAddress("127.0.2.1"));
		REQUIRE(negative_ret != OK);

		Error too_high_ret = spt->bind(70000, IPAddress("127.0.3.1"));
		REQUIRE(too_high_ret != OK);
		ERR_PRINT_ON;
	}

	SUBCASE("Invoking bind calls opens and binds NetSocket") {
		int bind_port = 7890;
		IPAddress bind_ip = IPAddress("127.0.4.1");
		Error bind_ret = spt->bind(bind_port, bind_ip);
		REQUIRE(bind_ret == OK);
		REQUIRE(ns->is_open() == true);
		REQUIRE(ns->host_addr.ip() == bind_ip);
		REQUIRE(ns->host_addr.port() == bind_port);
	}

	SUBCASE("Invoking disconnect_from_host closes NetSocket") {
		spt->disconnect_from_host();
		REQUIRE(ns->is_open() == false);
	}
}

TEST_CASE("[StreamPeerTCP] Poll") {
	Ref<MockNetSocket> ns;
	ns.instantiate();
	Ref<StreamPeerSocket> spt;
	spt.instantiate();
	IPAddress peer_ip = IPAddress("127.2.2.2");
	int peer_port = 45878;
	spt->accept_socket(ns, NetSocket::Address(peer_ip, peer_port));
	IPAddress bind_ip = IPAddress("127.0.0.1");
	int bind_port = 9043;
	Error bind_ret = spt->bind(bind_port, bind_ip);
	REQUIRE(bind_ret == OK);
	ns->_set_available_bytes(100);

	SUBCASE("Unconnected status causes connect_to_host") {
		Error poll_ret = spt->poll();
		REQUIRE(poll_ret == OK);
		REQUIRE(spt->get_status() == StreamPeerSocket::STATUS_CONNECTED);
		REQUIRE(ns->is_open() == true);
	}

	SUBCASE("FIN causes disconnect_from_host") {
		// This is the condition for FIN.
		ns->_set_available_bytes(0);
		Error fin_ret = spt->poll();
		REQUIRE(fin_ret == OK);
		REQUIRE(ns->is_open() == false);
	}
}

TEST_CASE("[StreamPeerTCP] Data") {
	Ref<MockNetSocket> ns;
	ns.instantiate();
	Ref<StreamPeerSocket> spt;
	spt.instantiate();
	IPAddress peer_ip = IPAddress("127.5.4.3");
	int peer_port = 8908;
	spt->accept_socket(ns, NetSocket::Address(peer_ip, peer_port));
	IPAddress bind_ip = IPAddress("127.0.0.1");
	int bind_port = 2039;
	Error bind_ret = spt->bind(bind_port, bind_ip);
	REQUIRE(bind_ret == OK);

	SUBCASE("Invoking put_data on an ascii-encoded stream") {
		const String expected = "hello, world";
		Vector<uint8_t> send_data;
		send_data.resize_initialized(expected.length());
		ns->_set_send_data(send_data.ptrw());
		REQUIRE_EQ(spt->put_data(expected.to_ascii_buffer().ptrw(), expected.length()), Error::OK);
		CHECK_EQ(send_data, expected.to_ascii_buffer());
	}

	SUBCASE("Invoking get_data on an ascii-encoded stream") {
		const String expected = "I - too - say hello!";
		Vector<uint8_t> recv_data = expected.to_ascii_buffer();
		ns->_set_recv_data(recv_data.ptrw());
		Vector<uint8_t> in_data;
		in_data.resize(expected.length());
		REQUIRE_EQ(spt->get_data(in_data.ptrw(), expected.length()), Error::OK);
		CHECK_EQ(in_data, expected.to_ascii_buffer());
	}
}

} // namespace TestStreamPeerTCP

// === Merged from tests/core/io/test_tcp_server.cpp ===
/*  test_tcp_server.cpp                                                   */
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
#include "core/os/os.h"

#include <functional>

namespace TestTCPServer {

const int PORT = 12345;
const IPAddress LOCALHOST("127.0.0.1");
const uint32_t SLEEP_DURATION = 1000;
const uint64_t MAX_WAIT_USEC = 2000000;

void wait_for_condition(std::function<bool()> f_test) {
	const uint64_t time = OS::get_ticks_usec();
	while (!f_test() && (OS::get_ticks_usec() - time) < MAX_WAIT_USEC) {
		OS::delay_usec(SLEEP_DURATION);
	}
}

Ref<SocketServer> create_server(const IPAddress &p_address, int p_port) {
	Ref<SocketServer> server;
	server.instantiate();

	REQUIRE_EQ(server->listen(PORT, LOCALHOST), Error::OK);
	REQUIRE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());

	return server;
}

Ref<StreamPeerSocket> create_client(const IPAddress &p_address, int p_port) {
	Ref<StreamPeerSocket> client;
	client.instantiate();

	REQUIRE_EQ(client->connect_to_host(LOCALHOST, PORT), Error::OK);
	CHECK_EQ(client->get_connected_host(), LOCALHOST);
	CHECK_EQ(client->get_connected_port(), PORT);
	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTING);

	return client;
}

Ref<StreamPeerSocket> accept_connection(Ref<SocketServer> &p_server) {
	wait_for_condition([&]() {
		return p_server->is_connection_available();
	});

	REQUIRE(p_server->is_connection_available());
	Ref<StreamPeerSocket> client_from_server = p_server->take_connection();
	REQUIRE(client_from_server.is_valid());
	CHECK_EQ(client_from_server->get_connected_host(), LOCALHOST);
	CHECK_EQ(client_from_server->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	return client_from_server;
}

TEST_CASE("[TCPServer] Instantiation") {
	Ref<SocketServer> server;
	server.instantiate();

	REQUIRE(server.is_valid());
	CHECK_EQ(false, server->is_listening());
}

TEST_CASE("[TCPServer] Accept a connection and receive/send data") {
	Ref<SocketServer> server = create_server(LOCALHOST, PORT);
	Ref<StreamPeerSocket> client = create_client(LOCALHOST, PORT);
	Ref<StreamPeerSocket> client_from_server = accept_connection(server);

	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_CONNECTED;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	client->put_string(hello_world);
	CHECK_EQ(client_from_server->get_string(), hello_world);

	// Sending data from server to client.
	const float pi = 3.1415;
	client_from_server->put_float(pi);
	CHECK_EQ(client->get_float(), pi);

	client->disconnect_from_host();
	server->stop();
	CHECK_FALSE(server->is_listening());
}

TEST_CASE("[TCPServer] Handle multiple clients at the same time") {
	Ref<SocketServer> server = create_server(LOCALHOST, PORT);

	Vector<Ref<StreamPeerSocket>> clients;
	for (int i = 0; i < 5; i++) {
		clients.push_back(create_client(LOCALHOST, PORT));
	}

	Vector<Ref<StreamPeerSocket>> clients_from_server;
	for (int i = 0; i < clients.size(); i++) {
		clients_from_server.push_back(accept_connection(server));
	}

	wait_for_condition([&]() {
		bool should_exit = true;
		for (Ref<StreamPeerSocket> &c : clients) {
			if (c->poll() != Error::OK) {
				return true;
			}
			StreamPeerSocket::Status status = c->get_status();
			if (status != StreamPeerSocket::STATUS_CONNECTED && status != StreamPeerSocket::STATUS_CONNECTING) {
				return true;
			}
			if (status != StreamPeerSocket::STATUS_CONNECTED) {
				should_exit = false;
			}
		}
		return should_exit;
	});

	for (Ref<StreamPeerSocket> &c : clients) {
		REQUIRE_EQ(c->get_status(), StreamPeerSocket::STATUS_CONNECTED);
	}

	// Sending data from each client to server.
	for (int i = 0; i < clients.size(); i++) {
		String hello_client = "Hello " + itos(i);
		clients[i]->put_string(hello_client);
		CHECK_EQ(clients_from_server[i]->get_string(), hello_client);
	}

	for (Ref<StreamPeerSocket> &c : clients) {
		c->disconnect_from_host();
	}
	server->stop();
}

TEST_CASE("[TCPServer] When stopped shouldn't accept new connections") {
	Ref<SocketServer> server = create_server(LOCALHOST, PORT);
	Ref<StreamPeerSocket> client = create_client(LOCALHOST, PORT);
	Ref<StreamPeerSocket> client_from_server = accept_connection(server);

	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_CONNECTED;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	client->put_string(hello_world);
	CHECK_EQ(client_from_server->get_string(), hello_world);

	client->disconnect_from_host();
	server->stop();
	CHECK_FALSE(server->is_listening());

	Ref<StreamPeerSocket> new_client = create_client(LOCALHOST, PORT);

	CHECK_FALSE(server->is_connection_available());

	wait_for_condition([&]() {
		return new_client->poll() != Error::OK || new_client->get_status() == StreamPeerSocket::STATUS_ERROR;
	});

	CHECK_FALSE(server->is_connection_available());

	CHECK_EQ(new_client->get_status(), StreamPeerSocket::STATUS_ERROR);
	new_client->disconnect_from_host();
	CHECK_EQ(new_client->get_status(), StreamPeerSocket::STATUS_NONE);
}

TEST_CASE("[TCPServer] Should disconnect client") {
	Ref<SocketServer> server = create_server(LOCALHOST, PORT);
	Ref<StreamPeerSocket> client = create_client(LOCALHOST, PORT);
	Ref<StreamPeerSocket> client_from_server = accept_connection(server);

	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_CONNECTED;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	client->put_string(hello_world);
	CHECK_EQ(client_from_server->get_string(), hello_world);

	client_from_server->disconnect_from_host();
	server->stop();
	CHECK_FALSE(server->is_listening());

	// Wait for disconnection
	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_NONE;
	});

	// Wait for disconnection
	wait_for_condition([&]() {
		return client_from_server->poll() != Error::OK || client_from_server->get_status() == StreamPeerSocket::STATUS_NONE;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_NONE);
	CHECK_EQ(client_from_server->get_status(), StreamPeerSocket::STATUS_NONE);

	ERR_PRINT_OFF;
	CHECK_EQ(client->get_string(), String());
	CHECK_EQ(client_from_server->get_string(), String());
	ERR_PRINT_ON;
}

} // namespace TestTCPServer

// === Merged from tests/core/io/test_udp_server.cpp ===
/*  test_udp_server.cpp                                                   */
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



#include "core/io/packet_peer_udp.h"
#include "core/os/os.h"

#include <functional>

namespace TestUDPServer {

const int PORT = 12345;
const IPAddress LOCALHOST("127.0.0.1");
const uint32_t SLEEP_DURATION = 1000;
const uint64_t MAX_WAIT_USEC = 100000;

void wait_for_condition(std::function<bool()> f_test) {
	const uint64_t time = OS::get_ticks_usec();
	while (!f_test() && (OS::get_ticks_usec() - time) < MAX_WAIT_USEC) {
		OS::delay_usec(SLEEP_DURATION);
	}
}

Ref<UDPServer> create_server(const IPAddress &p_address, int p_port) {
	Ref<UDPServer> server;
	server.instantiate();

	Error err = server->listen(PORT, LOCALHOST);
	REQUIRE_EQ(Error::OK, err);
	REQUIRE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());
	CHECK_EQ(server->get_max_pending_connections(), 16);

	return server;
}

Ref<PacketPeerUDP> create_client(const IPAddress &p_address, int p_port) {
	Ref<PacketPeerUDP> client;
	client.instantiate();

	Error err = client->connect_to_host(LOCALHOST, PORT);
	REQUIRE_EQ(Error::OK, err);
	CHECK(client->is_bound());
	CHECK(client->is_socket_connected());

	return client;
}

Ref<PacketPeerUDP> accept_connection(Ref<UDPServer> &p_server) {
	wait_for_condition([&]() {
		return p_server->poll() != Error::OK || p_server->is_connection_available();
	});

	CHECK_EQ(p_server->poll(), Error::OK);
	REQUIRE(p_server->is_connection_available());
	Ref<PacketPeerUDP> client_from_server = p_server->take_connection();
	REQUIRE(client_from_server.is_valid());
	CHECK(client_from_server->is_bound());
	CHECK(client_from_server->is_socket_connected());

	return client_from_server;
}

TEST_CASE("[UDPServer] Instantiation") {
	Ref<UDPServer> server;
	server.instantiate();

	REQUIRE(server.is_valid());
	CHECK_EQ(false, server->is_listening());
}

TEST_CASE("[UDPServer] Accept a connection and receive/send data") {
	Ref<UDPServer> server = create_server(LOCALHOST, PORT);
	Ref<PacketPeerUDP> client = create_client(LOCALHOST, PORT);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	CHECK_EQ(client->put_var(hello_world), Error::OK);

	Variant hello_world_received;
	Ref<PacketPeerUDP> client_from_server = accept_connection(server);
	CHECK_EQ(client_from_server->get_var(hello_world_received), Error::OK);
	CHECK_EQ(String(hello_world_received), hello_world);

	// Sending data from server to client.
	const Variant pi = 3.1415;
	CHECK_EQ(client_from_server->put_var(pi), Error::OK);

	wait_for_condition([&]() {
		return client->get_available_packet_count() > 0;
	});

	CHECK_GT(client->get_available_packet_count(), 0);

	Variant pi_received;
	CHECK_EQ(client->get_var(pi_received), Error::OK);
	CHECK_EQ(pi_received, pi);

	client->close();
	server->stop();
	CHECK_FALSE(server->is_listening());
}

TEST_CASE("[UDPServer] Handle multiple clients at the same time") {
	Ref<UDPServer> server = create_server(LOCALHOST, PORT);

	Vector<Ref<PacketPeerUDP>> clients;
	for (int i = 0; i < 5; i++) {
		Ref<PacketPeerUDP> c = create_client(LOCALHOST, PORT);

		// Sending data from client to server.
		const String hello_client = itos(i);
		CHECK_EQ(c->put_var(hello_client), Error::OK);

		clients.push_back(c);
	}

	Array packets;
	for (int i = 0; i < clients.size(); i++) {
		Ref<PacketPeerUDP> cfs = accept_connection(server);

		Variant received_var;
		CHECK_EQ(cfs->get_var(received_var), Error::OK);
		CHECK_EQ(received_var.get_type(), Variant::STRING);
		packets.push_back(received_var);

		// Sending data from server to client.
		const float sent_float = 3.1415 + received_var.operator String().to_float();
		CHECK_EQ(cfs->put_var(sent_float), Error::OK);
	}

	CHECK_EQ(packets.size(), clients.size());

	packets.sort();
	for (int i = 0; i < clients.size(); i++) {
		CHECK_EQ(packets[i].operator String(), itos(i));
	}

	wait_for_condition([&]() {
		bool should_exit = true;
		for (Ref<PacketPeerUDP> &c : clients) {
			int count = c->get_available_packet_count();
			if (count < 0) {
				return true;
			}
			if (count == 0) {
				should_exit = false;
			}
		}
		return should_exit;
	});

	for (int i = 0; i < clients.size(); i++) {
		CHECK_GT(clients[i]->get_available_packet_count(), 0);

		Variant received_var;
		const float expected = 3.1415 + i;
		CHECK_EQ(clients[i]->get_var(received_var), Error::OK);
		CHECK_EQ(received_var.get_type(), Variant::FLOAT);
		CHECK_EQ(received_var.operator float(), expected);
	}

	for (Ref<PacketPeerUDP> &c : clients) {
		c->close();
	}
	server->stop();
}

TEST_CASE("[UDPServer] Should not accept new connections after stop") {
	Ref<UDPServer> server = create_server(LOCALHOST, PORT);
	Ref<PacketPeerUDP> client = create_client(LOCALHOST, PORT);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	CHECK_EQ(client->put_var(hello_world), Error::OK);

	wait_for_condition([&]() {
		return server->poll() != Error::OK || server->is_connection_available();
	});

	REQUIRE(server->is_connection_available());

	server->stop();

	CHECK_FALSE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());
}

} // namespace TestUDPServer

// === Merged from tests/core/io/test_uds_server.cpp ===
/*  test_uds_server.cpp                                                   */
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



#ifdef UNIX_ENABLED

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/net_socket.h"
#include "core/os/os.h"

#include <functional>

namespace TestUDSServer {

const String SOCKET_PATH = "/tmp/godot_test_uds_socket";
const uint32_t SLEEP_DURATION = 1000;
const uint64_t MAX_WAIT_USEC = 2000000;

void wait_for_condition(std::function<bool()> f_test) {
	const uint64_t time = OS::get_ticks_usec();
	while (!f_test() && (OS::get_ticks_usec() - time) < MAX_WAIT_USEC) {
		OS::delay_usec(SLEEP_DURATION);
	}
}

void cleanup_socket_file() {
	// Remove socket file if it exists
	if (FileAccess::exists(SOCKET_PATH)) {
		DirAccess::remove_absolute(SOCKET_PATH);
	}
}

Ref<SocketServer> create_server(const String &p_path) {
	cleanup_socket_file();

	Ref<SocketServer> server;
	server.instantiate();

	REQUIRE_EQ(server->listen(p_path), Error::OK);
	REQUIRE(server->is_listening());
	CHECK_FALSE(server->is_connection_available());

	return server;
}

Ref<StreamPeerSocket> create_client(const String &p_path) {
	Ref<StreamPeerSocket> client;
	client.instantiate(StreamPeerSocket::DOMAIN_UDS);

	Error err = client->connect_to_path(p_path);
	REQUIRE_EQ(err, Error::OK);

	// UDS connections may be immediately connected or in connecting state
	StreamPeerSocket::Status status = client->get_status();
	REQUIRE((status == StreamPeerSocket::STATUS_CONNECTED || status == StreamPeerSocket::STATUS_CONNECTING));

	if (status == StreamPeerSocket::STATUS_CONNECTED) {
		CHECK_EQ(client->get_connected_path(), p_path);
	}

	return client;
}

Ref<StreamPeerSocket> accept_connection(Ref<SocketServer> &p_server) {
	wait_for_condition([&]() {
		return p_server->is_connection_available();
	});

	REQUIRE(p_server->is_connection_available());
	Ref<StreamPeerSocket> client_from_server = p_server->take_connection();
	REQUIRE(client_from_server.is_valid());
	CHECK_EQ(client_from_server->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	return client_from_server;
}

TEST_CASE("[UDSServer] Instantiation") {
	Ref<SocketServer> server;
	server.instantiate();

	REQUIRE(server.is_valid());
	CHECK_FALSE(server->is_listening());
}

TEST_CASE("[UDSServer] Accept a connection and receive/send data") {
	Ref<SocketServer> server = create_server(SOCKET_PATH);
	Ref<StreamPeerSocket> client = create_client(SOCKET_PATH);
	Ref<StreamPeerSocket> client_from_server = accept_connection(server);

	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_CONNECTED;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	client->put_string(hello_world);
	CHECK_EQ(client_from_server->get_string(), hello_world);

	// Sending data from server to client.
	const float pi = 3.1415;
	client_from_server->put_float(pi);
	CHECK_EQ(client->get_float(), pi);

	client->disconnect_from_host();
	server->stop();
	CHECK_FALSE(server->is_listening());

	cleanup_socket_file();
}

TEST_CASE("[UDSServer] Handle multiple clients at the same time") {
	Ref<SocketServer> server = create_server(SOCKET_PATH);

	Vector<Ref<StreamPeerSocket>> clients;
	for (int i = 0; i < 5; i++) {
		clients.push_back(create_client(SOCKET_PATH));
	}

	Vector<Ref<StreamPeerSocket>> clients_from_server;
	for (int i = 0; i < clients.size(); i++) {
		clients_from_server.push_back(accept_connection(server));
	}

	wait_for_condition([&]() {
		bool should_exit = true;
		for (Ref<StreamPeerSocket> &c : clients) {
			if (c->poll() != Error::OK) {
				return true;
			}
			StreamPeerSocket::Status status = c->get_status();
			if (status != StreamPeerSocket::STATUS_CONNECTED && status != StreamPeerSocket::STATUS_CONNECTING) {
				return true;
			}
			if (status != StreamPeerSocket::STATUS_CONNECTED) {
				should_exit = false;
			}
		}
		return should_exit;
	});

	for (Ref<StreamPeerSocket> &c : clients) {
		REQUIRE_EQ(c->get_status(), StreamPeerSocket::STATUS_CONNECTED);
	}

	// Sending data from each client to server.
	for (int i = 0; i < clients.size(); i++) {
		String hello_client = "Hello " + itos(i);
		clients[i]->put_string(hello_client);
		CHECK_EQ(clients_from_server[i]->get_string(), hello_client);
	}

	for (Ref<StreamPeerSocket> &c : clients) {
		c->disconnect_from_host();
	}
	server->stop();

	cleanup_socket_file();
}

TEST_CASE("[UDSServer] When stopped shouldn't accept new connections") {
	Ref<SocketServer> server = create_server(SOCKET_PATH);
	Ref<StreamPeerSocket> client = create_client(SOCKET_PATH);
	Ref<StreamPeerSocket> client_from_server = accept_connection(server);

	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_CONNECTED;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	client->put_string(hello_world);
	CHECK_EQ(client_from_server->get_string(), hello_world);

	client->disconnect_from_host();
	server->stop();
	CHECK_FALSE(server->is_listening());

	// Clean up the socket file after server stops
	cleanup_socket_file();

	// Try to connect to non-existent socket
	Ref<StreamPeerSocket> new_client;
	new_client.instantiate(StreamPeerSocket::DOMAIN_UDS);
	Error err = new_client->connect_to_path(SOCKET_PATH);

	// Connection should fail since socket doesn't exist
	CHECK_NE(err, Error::OK);
	CHECK_FALSE(server->is_connection_available());

	cleanup_socket_file();
}

TEST_CASE("[UDSServer] Should disconnect client") {
	Ref<SocketServer> server = create_server(SOCKET_PATH);
	Ref<StreamPeerSocket> client = create_client(SOCKET_PATH);
	Ref<StreamPeerSocket> client_from_server = accept_connection(server);

	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_CONNECTED;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	// Sending data from client to server.
	const String hello_world = "Hello World!";
	client->put_string(hello_world);
	CHECK_EQ(client_from_server->get_string(), hello_world);

	client_from_server->disconnect_from_host();
	server->stop();
	CHECK_FALSE(server->is_listening());

	// Wait for disconnection
	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_NONE;
	});

	// Wait for disconnection
	wait_for_condition([&]() {
		return client_from_server->poll() != Error::OK || client_from_server->get_status() == StreamPeerSocket::STATUS_NONE;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_NONE);
	CHECK_EQ(client_from_server->get_status(), StreamPeerSocket::STATUS_NONE);

	ERR_PRINT_OFF;
	CHECK_EQ(client->get_string(), String());
	CHECK_EQ(client_from_server->get_string(), String());
	ERR_PRINT_ON;

	cleanup_socket_file();
}

TEST_CASE("[UDSServer] Test with different socket paths") {
	// Test with a different socket path
	const String alt_socket_path = "/tmp/godot_test_uds_socket_alt";

	// Clean up before test
	if (FileAccess::exists(alt_socket_path)) {
		DirAccess::remove_absolute(alt_socket_path);
	}

	Ref<SocketServer> server = create_server(alt_socket_path);
	Ref<StreamPeerSocket> client = create_client(alt_socket_path);
	Ref<StreamPeerSocket> client_from_server = accept_connection(server);

	wait_for_condition([&]() {
		return client->poll() != Error::OK || client->get_status() == StreamPeerSocket::STATUS_CONNECTED;
	});

	CHECK_EQ(client->get_status(), StreamPeerSocket::STATUS_CONNECTED);

	// Test data exchange
	const int test_number = 42;
	client->put_32(test_number);
	CHECK_EQ(client_from_server->get_32(), test_number);

	client->disconnect_from_host();
	server->stop();

	// Clean up
	if (FileAccess::exists(alt_socket_path)) {
		DirAccess::remove_absolute(alt_socket_path);
	}
}

} // namespace TestUDSServer

#endif // UNIX_ENABLED

// === Merged from tests/core/io/test_xml_parser.cpp ===
/*  test_xml_parser.cpp                                                   */
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

namespace TestXMLParser {

TEST_CASE("[XMLParser] End-to-end") {
	String source = "<?xml version = \"1.0\" encoding=\"UTF-8\" ?>\
<top attr=\"attr value\">\
  Text&lt;&#65;&#x42;&gt;\
</top>";
	Vector<uint8_t> buff = source.to_utf8_buffer();

	XMLParser parser;
	parser.open_buffer(buff);

	// <?xml ...?> gets parsed as NODE_UNKNOWN
	CHECK(parser.read() == OK);
	CHECK(parser.get_node_type() == XMLParser::NodeType::NODE_UNKNOWN);

	CHECK(parser.read() == OK);
	CHECK(parser.get_node_type() == XMLParser::NodeType::NODE_ELEMENT);
	CHECK(parser.get_node_name() == "top");
	CHECK(parser.has_attribute("attr"));
	CHECK(parser.get_named_attribute_value("attr") == "attr value");

	CHECK(parser.read() == OK);
	CHECK(parser.get_node_type() == XMLParser::NodeType::NODE_TEXT);
	CHECK(parser.get_node_data().lstrip(" \t") == "Text<AB>");

	CHECK(parser.read() == OK);
	CHECK(parser.get_node_type() == XMLParser::NodeType::NODE_ELEMENT_END);
	CHECK(parser.get_node_name() == "top");

	parser.close();
}

TEST_CASE("[XMLParser] Comments") {
	XMLParser parser;

	SUBCASE("Missing end of comment") {
		const String input = "<first></first><!-- foo";
		REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
		REQUIRE_EQ(parser.read(), OK);
		REQUIRE_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT);
		REQUIRE_EQ(parser.read(), OK);
		REQUIRE_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT_END);
		REQUIRE_EQ(parser.read(), OK);
		CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_COMMENT);
		CHECK_EQ(parser.get_node_name(), " foo");
	}
	SUBCASE("Bad start of comment") {
		const String input = "<first></first><!-";
		REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
		REQUIRE_EQ(parser.read(), OK);
		REQUIRE_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT);
		REQUIRE_EQ(parser.read(), OK);
		REQUIRE_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT_END);
		REQUIRE_EQ(parser.read(), OK);
		CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_COMMENT);
		CHECK_EQ(parser.get_node_name(), "-");
	}
	SUBCASE("Unblanced angle brackets in comment") {
		const String input = "<!-- example << --><next-tag></next-tag>";
		REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
		REQUIRE_EQ(parser.read(), OK);
		CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_COMMENT);
		CHECK_EQ(parser.get_node_name(), " example << ");
	}
	SUBCASE("Doctype") {
		const String input = "<!DOCTYPE greeting [<!ELEMENT greeting (#PCDATA)>]>";
		REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
		REQUIRE_EQ(parser.read(), OK);
		CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_COMMENT);
		CHECK_EQ(parser.get_node_name(), "DOCTYPE greeting [<!ELEMENT greeting (#PCDATA)>]");
	}
}

TEST_CASE("[XMLParser] Premature endings") {
	SUBCASE("Simple cases") {
		String input;
		String expected_name;
		XMLParser::NodeType expected_type;

		SUBCASE("Incomplete Unknown") {
			input = "<first></first><?xml";
			expected_type = XMLParser::NodeType::NODE_UNKNOWN;
			expected_name = "?xml";
		}
		SUBCASE("Incomplete CDStart") {
			input = "<first></first><![CD";
			expected_type = XMLParser::NodeType::NODE_CDATA;
			expected_name = "";
		}
		SUBCASE("Incomplete CData") {
			input = "<first></first><![CDATA[example";
			expected_type = XMLParser::NodeType::NODE_CDATA;
			expected_name = "example";
		}
		SUBCASE("Incomplete CDEnd") {
			input = "<first></first><![CDATA[example]]";
			expected_type = XMLParser::NodeType::NODE_CDATA;
			expected_name = "example]]";
		}
		SUBCASE("Incomplete start-tag name") {
			input = "<first></first><second";
			expected_type = XMLParser::NodeType::NODE_ELEMENT;
			expected_name = "second";
		}

		XMLParser parser;
		REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
		REQUIRE_EQ(parser.read(), OK);
		REQUIRE_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT);
		REQUIRE_EQ(parser.read(), OK);
		REQUIRE_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT_END);
		REQUIRE_EQ(parser.read(), OK);
		CHECK_EQ(parser.get_node_type(), expected_type);
		CHECK_EQ(parser.get_node_name(), expected_name);
	}

	SUBCASE("With attributes and texts") {
		XMLParser parser;

		SUBCASE("Incomplete start-tag attribute name") {
			const String input = "<first></first><second attr1=\"foo\" attr2";
			REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT);
			CHECK_EQ(parser.get_node_name(), "second");
			CHECK_EQ(parser.get_attribute_count(), 1);
			CHECK_EQ(parser.get_attribute_name(0), "attr1");
			CHECK_EQ(parser.get_attribute_value(0), "foo");
		}

		SUBCASE("Incomplete start-tag attribute unquoted value") {
			const String input = "<first></first><second attr1=\"foo\" attr2=bar";
			REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT);
			CHECK_EQ(parser.get_node_name(), "second");
			CHECK_EQ(parser.get_attribute_count(), 1);
			CHECK_EQ(parser.get_attribute_name(0), "attr1");
			CHECK_EQ(parser.get_attribute_value(0), "foo");
		}

		SUBCASE("Incomplete start-tag attribute quoted value") {
			const String input = "<first></first><second attr1=\"foo\" attr2=\"bar";
			REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT);
			CHECK_EQ(parser.get_node_name(), "second");
			CHECK_EQ(parser.get_attribute_count(), 2);
			CHECK_EQ(parser.get_attribute_name(0), "attr1");
			CHECK_EQ(parser.get_attribute_value(0), "foo");
			CHECK_EQ(parser.get_attribute_name(1), "attr2");
			CHECK_EQ(parser.get_attribute_value(1), "bar");
		}

		SUBCASE("Incomplete end-tag name") {
			const String input = "<first></fir";
			REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT_END);
			CHECK_EQ(parser.get_node_name(), "fir");
		}

		SUBCASE("Trailing text") {
			const String input = "<first></first>example";
			REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			REQUIRE_EQ(parser.read(), OK);
			CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_TEXT);
			CHECK_EQ(parser.get_node_data(), "example");
		}
	}
}

TEST_CASE("[XMLParser] CDATA") {
	const String input = "<a><![CDATA[my cdata content goes here]]></a>";
	XMLParser parser;
	REQUIRE_EQ(parser.open_buffer(input.to_utf8_buffer()), OK);
	REQUIRE_EQ(parser.read(), OK);
	CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT);
	CHECK_EQ(parser.get_node_name(), "a");
	REQUIRE_EQ(parser.read(), OK);
	CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_CDATA);
	CHECK_EQ(parser.get_node_name(), "my cdata content goes here");
	REQUIRE_EQ(parser.read(), OK);
	CHECK_EQ(parser.get_node_type(), XMLParser::NodeType::NODE_ELEMENT_END);
	CHECK_EQ(parser.get_node_name(), "a");
}

} // namespace TestXMLParser

TEST_FORCE_LINK(test_file_access)

