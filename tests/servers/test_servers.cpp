
// === Merged from tests/servers/rendering/test_shader_preprocessor.cpp ===
/*  test_shader_preprocessor.cpp                                          */
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



#include "servers/rendering/renderer.h"

#include <cctype>

namespace TestShaderPreprocessor {

void erase_all_empty(Vector<String> &p_vec) {
	int idx = p_vec.find(" ");
	while (idx >= 0) {
		p_vec.remove_at(idx);
		idx = p_vec.find(" ");
	}
}

bool is_variable_char(unsigned char c) {
	return std::isalnum(c) || c == '_';
}

bool is_operator_char(unsigned char c) {
	return (c == '*') || (c == '+') || (c == '-') || (c == '/') || ((c >= '<') && (c <= '>'));
}

// Remove unnecessary spaces from a line.
String remove_spaces(String &p_str) {
	String res;
	// Result is guaranteed to not be longer than the input.
	res.resize_uninitialized(p_str.size());
	int wp = 0;
	char32_t last = 0;
	bool has_removed = false;

	for (int n = 0; n < p_str.size(); n++) {
		// These test cases only use ASCII.
		unsigned char c = static_cast<unsigned char>(p_str[n]);
		if (std::isblank(c)) {
			has_removed = true;
		} else {
			if (has_removed) {
				// Insert a space to avoid joining things that could potentially form a new token.
				// E.g. "float x" or "- -".
				if ((is_variable_char(c) && is_variable_char(last)) ||
						(is_operator_char(c) && is_operator_char(last))) {
					res[wp++] = ' ';
				}
				has_removed = false;
			}
			res[wp++] = c;
			last = c;
		}
	}
	res.resize_uninitialized(wp);
	return res;
}

// The pre-processor changes indentation and inserts spaces when inserting macros.
// Re-format the code, without changing its meaning, to make it easier to compare.
String compact_spaces(String &p_str) {
	Vector<String> lines = p_str.split("\n", false);
	erase_all_empty(lines);
	for (String &line : lines) {
		line = remove_spaces(line);
	}
	return String("\n").join(lines);
}

#define CHECK_SHADER_EQ(a, b) CHECK_EQ(compact_spaces(a), compact_spaces(b))
#define CHECK_SHADER_NE(a, b) CHECK_NE(compact_spaces(a), compact_spaces(b))

TEST_CASE("[ShaderPreprocessor] Simple defines") {
	String code(
			"#define X 1.0 // comment\n"
			"#define Y mix\n"
			"#define Z X\n"
			"\n"
			"#define func0 \\\n"
			"  vec3 my_fun(vec3 arg) {\\\n"
			"    return pow(arg, 2.2);\\\n"
			"  }\n"
			"\n"
			"func0\n"
			"\n"
			"fragment() {\n"
			"  ALBEDO = vec3(X);\n"
			"  float x = Y(0., Z, X);\n"
			"  #undef X\n"
			"  float X = x;\n"
			"  x = -Z;\n"
			"}\n");
	String expected(
			"vec3 my_fun(vec3 arg) { return pow(arg, 2.2); }\n"
			"\n"
			"fragment() {\n"
			"  ALBEDO = vec3( 1.0 );\n"
			"  float x = mix(0., 1.0 , 1.0 );\n"
			"  float X = x;\n"
			"  x = -X;\n"
			"}\n");
	String result;

	ShaderPreprocessor preprocessor;
	CHECK_EQ(preprocessor.preprocess(code, String("file.gdshader"), result), Error::OK);

	CHECK_SHADER_EQ(result, expected);
}

TEST_CASE("[ShaderPreprocessor] Avoid merging adjacent tokens") {
	String code(
			"#define X -10\n"
			"#define Y(s) s\n"
			"\n"
			"fragment() {\n"
			"  float v = 1.0-X-Y(-2);\n"
			"}\n");
	String expected(
			"fragment() {\n"
			"  float v = 1.0 - -10 - -2;\n"
			"}\n");
	String result;

	ShaderPreprocessor preprocessor;
	CHECK_EQ(preprocessor.preprocess(code, String("file.gdshader"), result), Error::OK);

	CHECK_SHADER_EQ(result, expected);
}

TEST_CASE("[ShaderPreprocessor] Complex defines") {
	String code(
			"const float X = 2.0;\n"
			"#define A(X) X*2.\n"
			"#define X 1.0\n"
			"#define Y Z(X, W)\n"
			"#define Z max\n"
			"#define C(X, Y) Z(A(Y), B(X))\n"
			"#define W -X\n"
			"#define B(X) X*3.\n"
			"\n"
			"fragment() {\n"
			"  float x = Y;\n"
			"  float y = C(5., 7.0);\n"
			"}\n");
	String expected(
			"const float X = 2.0;\n"
			"fragment() {\n"
			"  float x = max(1.0, - 1.0);\n"
			"  float y = max(7.0*2. , 5.*3.);\n"
			"}\n");
	String result;

	ShaderPreprocessor preprocessor;
	CHECK_EQ(preprocessor.preprocess(code, String("file.gdshader"), result), Error::OK);

	CHECK_SHADER_EQ(result, expected);
}

TEST_CASE("[ShaderPreprocessor] Concatenation") {
	String code(
			"fragment() {\n"
			"  #define X 1 // this is fine ##\n"
			"  #define y 2\n"
			"  #define z 3##.## 1## 4 ## 59\n"
			"  #define Z(y) X ## y\n"
			"  #define Z2(y) y##X\n"
			"  #define W(y) X, y\n"
			"  #define A(x) fl## oat a = 1##x ##.3  ##  x\n"
			"  #define C(x, y) x##.##y\n"
			"  #define J(x) x##=\n"
			"  float Z(y) = 1.2;\n"
			"  float Z(z) = 2.3;\n"
			"  float Z2(y) = z;\n"
			"  float Z2(z) = 2.3;\n"
			"  int b = max(W(3));\n"
			"  Xy J(+) b J(=) 3 ? 0.1 : 0.2;\n"
			"  A(9);\n"
			"  Xy = C(X, y);\n"
			"}\n");
	String expected(
			"fragment() {\n"
			"  float Xy = 1.2;\n"
			"  float Xz = 2.3;\n"
			"  float yX = 3.1459;\n"
			"  float zX = 2.3;\n"
			"  int b = max(1, 3);\n"
			"  Xy += b == 3 ? 0.1 : 0.2;\n"
			"  float a = 19.39;\n"
			"  Xy = 1.2;\n"
			"}\n");
	String result;

	ShaderPreprocessor preprocessor;
	CHECK_EQ(preprocessor.preprocess(code, String("file.gdshader"), result), Error::OK);

	CHECK_SHADER_EQ(result, expected);
}

TEST_CASE("[ShaderPreprocessor] Nested concatenation") {
	// Concatenation ## should not expand adjacent tokens if they are macros,
	// but this is currently not implemented in Godot's shader preprocessor.
	// To force expanding, an extra macro should be required (B in this case).

	String code(
			"fragment() {\n"
			"  vec2 X = vec2(0);\n"
			"  #define X 1\n"
			"  #define y 2\n"
			"  #define B(x, y) C(x, y)\n"
			"  #define C(x, y) x##.##y\n"
			"  C(X, y) = B(X, y);\n"
			"}\n");
	String expected(
			"fragment() {\n"
			"  vec2 X = vec2(0);\n"
			"  X.y = 1.2;\n"
			"}\n");
	String result;

	ShaderPreprocessor preprocessor;
	CHECK_EQ(preprocessor.preprocess(code, String("file.gdshader"), result), Error::OK);

	// TODO: Reverse the check when/if this is changed.
	CHECK_SHADER_NE(result, expected);
}

TEST_CASE("[ShaderPreprocessor] Concatenation sorting network") {
	String code(
			"fragment() {\n"
			"  #define ARR(X) test##X\n"
			"  #define ACMP(a, b) ARR(a) > ARR(b)\n"
			"  #define ASWAP(a, b) tmp = ARR(b); ARR(b) = ARR(a); ARR(a) = tmp;\n"
			"  #define ACSWAP(a, b) if(ACMP(a, b)) { ASWAP(a, b) }\n"
			"  float test0 = 1.2;\n"
			"  float test1 = 0.34;\n"
			"  float test3 = 0.8;\n"
			"  float test4 = 2.9;\n"
			"  float tmp;\n"
			"  ACSWAP(0,2)\n"
			"  ACSWAP(1,3)\n"
			"  ACSWAP(0,1)\n"
			"  ACSWAP(2,3)\n"
			"  ACSWAP(1,2)\n"
			"}\n");
	String expected(
			"fragment() {\n"
			"  float test0 = 1.2;\n"
			"  float test1 = 0.34;\n"
			"  float test3 = 0.8;\n"
			"  float test4 = 2.9;\n"
			"  float tmp;\n"
			"  if(test0 > test2) { tmp = test2; test2 = test0; test0 = tmp; }\n"
			"  if(test1 > test3) { tmp = test3; test3 = test1; test1 = tmp; }\n"
			"  if(test0 > test1) { tmp = test1; test1 = test0; test0 = tmp; }\n"
			"  if(test2 > test3) { tmp = test3; test3 = test2; test2 = tmp; }\n"
			"  if(test1 > test2) { tmp = test2; test2 = test1; test1 = tmp; }\n"
			"}\n");
	String result;

	ShaderPreprocessor preprocessor;
	CHECK_EQ(preprocessor.preprocess(code, String("file.gdshader"), result), Error::OK);

	CHECK_SHADER_EQ(result, expected);
}

TEST_CASE("[ShaderPreprocessor] Undefined behavior") {
	// None of these are valid concatenation, nor valid shader code.
	// Don't care about results, just make sure there's no crash.
	const String filename("somefile.gdshader");
	String result;
	ShaderPreprocessor preprocessor;

	preprocessor.preprocess("#define X ###\nX\n", filename, result);
	preprocessor.preprocess("#define X ####\nX\n", filename, result);
	preprocessor.preprocess("#define X #####\nX\n", filename, result);
	preprocessor.preprocess("#define X 1 ### 2\nX\n", filename, result);
	preprocessor.preprocess("#define X 1 #### 2\nX\n", filename, result);
	preprocessor.preprocess("#define X 1 ##### 2\nX\n", filename, result);
	preprocessor.preprocess("#define X ### 2\nX\n", filename, result);
	preprocessor.preprocess("#define X #### 2\nX\n", filename, result);
	preprocessor.preprocess("#define X ##### 2\nX\n", filename, result);
	preprocessor.preprocess("#define X 1 ###\nX\n", filename, result);
	preprocessor.preprocess("#define X 1 ####\nX\n", filename, result);
	preprocessor.preprocess("#define X 1 #####\nX\n", filename, result);
}

TEST_CASE("[ShaderPreprocessor] Invalid concatenations") {
	const String filename("somefile.gdshader");
	String result;
	ShaderPreprocessor preprocessor;

	CHECK_NE(preprocessor.preprocess("#define X ##", filename, result), Error::OK);
	CHECK_NE(preprocessor.preprocess("#define X 1 ##", filename, result), Error::OK);
	CHECK_NE(preprocessor.preprocess("#define X ## 1", filename, result), Error::OK);
	CHECK_NE(preprocessor.preprocess("#define X(y)   ##  ", filename, result), Error::OK);
	CHECK_NE(preprocessor.preprocess("#define X(y) y ##  ", filename, result), Error::OK);
	CHECK_NE(preprocessor.preprocess("#define X(y) ## y", filename, result), Error::OK);
}

} // namespace TestShaderPreprocessor

// === Merged from tests/servers/test_nav_heap.cpp ===
/*  test_nav_heap.cpp                                                     */
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



#include "scene/main/scene_tree.h"

namespace TestNavHeap {

struct GreaterThan {
	bool operator()(int p_a, int p_b) const { return p_a > p_b; }
};

struct CompareArrayValues {
	const int *array;

	CompareArrayValues(const int *p_array) :
			array(p_array) {}

	bool operator()(uint32_t p_index_a, uint32_t p_index_b) const {
		return array[p_index_a] < array[p_index_b];
	}
};

struct RegisterHeapIndexes {
	uint32_t *indexes;

	RegisterHeapIndexes(uint32_t *p_indexes) :
			indexes(p_indexes) {}

	void operator()(uint32_t p_vector_index, uint32_t p_heap_index) {
		indexes[p_vector_index] = p_heap_index;
	}
};

TEST_CASE("[Heap] size") {
	Heap<int> heap;

	CHECK(heap.size() == 0);

	heap.push(0);
	CHECK(heap.size() == 1);

	heap.push(1);
	CHECK(heap.size() == 2);

	heap.pop();
	CHECK(heap.size() == 1);

	heap.pop();
	CHECK(heap.size() == 0);
}

TEST_CASE("[Heap] is_empty") {
	Heap<int> heap;

	CHECK(heap.is_empty() == true);

	heap.push(0);
	CHECK(heap.is_empty() == false);

	heap.pop();
	CHECK(heap.is_empty() == true);
}

TEST_CASE("[Heap] push/pop") {
	SUBCASE("Default comparator") {
		Heap<int> heap;

		heap.push(2);
		heap.push(7);
		heap.push(5);
		heap.push(3);
		heap.push(4);

		CHECK(heap.pop() == 7);
		CHECK(heap.pop() == 5);
		CHECK(heap.pop() == 4);
		CHECK(heap.pop() == 3);
		CHECK(heap.pop() == 2);
	}

	SUBCASE("Custom comparator") {
		GreaterThan greaterThan;
		Heap<int, GreaterThan> heap(greaterThan);

		heap.push(2);
		heap.push(7);
		heap.push(5);
		heap.push(3);
		heap.push(4);

		CHECK(heap.pop() == 2);
		CHECK(heap.pop() == 3);
		CHECK(heap.pop() == 4);
		CHECK(heap.pop() == 5);
		CHECK(heap.pop() == 7);
	}

	SUBCASE("Intermediate pops") {
		Heap<int> heap;

		heap.push(0);
		heap.push(3);
		heap.pop();
		heap.push(1);
		heap.push(2);

		CHECK(heap.pop() == 2);
		CHECK(heap.pop() == 1);
		CHECK(heap.pop() == 0);
	}
}

TEST_CASE("[Heap] shift") {
	int values[] = { 5, 3, 6, 7, 1 };
	uint32_t heap_indexes[] = { 0, 0, 0, 0, 0 };
	CompareArrayValues comparator(values);
	RegisterHeapIndexes indexer(heap_indexes);
	Heap<uint32_t, CompareArrayValues, RegisterHeapIndexes> heap(comparator, indexer);

	heap.push(0);
	heap.push(1);
	heap.push(2);
	heap.push(3);
	heap.push(4);

	// Shift down: 6 -> 2
	values[2] = 2;
	heap.shift(heap_indexes[2]);

	// Shift up: 5 -> 8
	values[0] = 8;
	heap.shift(heap_indexes[0]);

	CHECK(heap.pop() == 0);
	CHECK(heap.pop() == 3);
	CHECK(heap.pop() == 1);
	CHECK(heap.pop() == 2);
	CHECK(heap.pop() == 4);

	CHECK(heap_indexes[0] == Heap<uint32_t, CompareArrayValues, RegisterHeapIndexes>::INVALID_INDEX);
	CHECK(heap_indexes[1] == Heap<uint32_t, CompareArrayValues, RegisterHeapIndexes>::INVALID_INDEX);
	CHECK(heap_indexes[2] == Heap<uint32_t, CompareArrayValues, RegisterHeapIndexes>::INVALID_INDEX);
	CHECK(heap_indexes[3] == Heap<uint32_t, CompareArrayValues, RegisterHeapIndexes>::INVALID_INDEX);
	CHECK(heap_indexes[4] == Heap<uint32_t, CompareArrayValues, RegisterHeapIndexes>::INVALID_INDEX);
}

TEST_CASE("[Heap] clear") {
	uint32_t heap_indexes[] = { 0, 0, 0, 0 };
	RegisterHeapIndexes indexer(heap_indexes);
	Heap<uint32_t, Comparator<uint32_t>, RegisterHeapIndexes> heap(indexer);

	heap.push(0);
	heap.push(2);
	heap.push(1);
	heap.push(3);

	heap.clear();

	CHECK(heap.size() == 0);

	CHECK(heap_indexes[0] == Heap<uint32_t, Comparator<uint32_t>, RegisterHeapIndexes>::INVALID_INDEX);
	CHECK(heap_indexes[1] == Heap<uint32_t, Comparator<uint32_t>, RegisterHeapIndexes>::INVALID_INDEX);
	CHECK(heap_indexes[2] == Heap<uint32_t, Comparator<uint32_t>, RegisterHeapIndexes>::INVALID_INDEX);
	CHECK(heap_indexes[3] == Heap<uint32_t, Comparator<uint32_t>, RegisterHeapIndexes>::INVALID_INDEX);
}

} // namespace TestNavHeap

// === Merged from tests/servers/test_navigation_server_2d.cpp ===
/*  test_navigation_server_2d.cpp                                         */
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



#include "modules/modules_enabled.gen.h" // For navigation 2D.

#ifdef MODULE_NAVIGATION_2D_ENABLED

#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestNavigationServer2D
{

// TODO: Find a more generic way to create `Callable` mocks.
class CallableMock
{
public:
	mem_unique_ptr<Object> obj;
	void function1(Variant arg0)
	{
		function1_calls++;
		function1_latest_arg0 = arg0;
	}

	unsigned function1_calls{0};
	Variant function1_latest_arg0;
};

struct GreaterThan
{
	bool operator()(int p_a, int p_b) const { return p_a > p_b; }
};

struct CompareArrayValues
{
	const int* array;

	CompareArrayValues(const int* p_array) : array(p_array) {}

	bool operator()(uint32_t p_index_a, uint32_t p_index_b) const
	{
		return array[p_index_a] < array[p_index_b];
	}
};

struct RegisterHeapIndexes
{
	uint32_t* indexes;

	RegisterHeapIndexes(uint32_t* p_indexes) : indexes(p_indexes) {}

	void operator()(uint32_t p_vector_index, uint32_t p_heap_index)
	{
		indexes[p_vector_index] = p_heap_index;
	}
};

TEST_SUITE("[Navigation2D]")
{
	TEST_CASE("[NavigationServer2D] Server should be empty when initialized")
	{
		CHECK_EQ(NavigationServer2D::get_maps().size(), 0);

		SUBCASE("'ProcessInfo' should report all counters empty as well")
		{
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_ACTIVE_MAPS), 0);
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_REGION_COUNT), 0);
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_AGENT_COUNT), 0);
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_LINK_COUNT), 0);
			CHECK_EQ(
				NavigationServer2D::get_process_info(NavigationServer2D::INFO_POLYGON_COUNT), 0);
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_EDGE_COUNT), 0);
			CHECK_EQ(
				NavigationServer2D::get_process_info(NavigationServer2D::INFO_EDGE_MERGE_COUNT), 0);
			CHECK_EQ(
				NavigationServer2D::get_process_info(NavigationServer2D::INFO_EDGE_CONNECTION_COUNT),
				0);
			CHECK_EQ(
				NavigationServer2D::get_process_info(NavigationServer2D::INFO_EDGE_FREE_COUNT), 0);
		}
	}

	TEST_CASE("[NavigationServer2D] Server should manage agent properly")
	{

		RID agent = NavigationServer2D::agent_create();
		CHECK(agent.is_valid());

		SUBCASE("'ProcessInfo' should not report dangling agent")
		{
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_AGENT_COUNT), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			bool initial_avoidance_enabled = NavigationServer2D::agent_get_avoidance_enabled(agent);
			NavigationServer2D::agent_set_avoidance_enabled(agent, !initial_avoidance_enabled);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(
				NavigationServer2D::agent_get_avoidance_enabled(agent), !initial_avoidance_enabled);
			// TODO: Add remaining setters/getters once the missing getters are added.
		}

		SUBCASE("'ProcessInfo' should report agent with active map")
		{
			RID map = NavigationServer2D::map_create();
			CHECK(map.is_valid());
			NavigationServer2D::map_set_active(map, true);
			NavigationServer2D::agent_set_map(agent, map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_AGENT_COUNT), 1);
			NavigationServer2D::agent_set_map(agent, RID());
			NavigationServer2D::free_rid(map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_AGENT_COUNT), 0);
		}

		NavigationServer2D::free_rid(agent);
	}

	TEST_CASE("[NavigationServer2D] Server should manage map properly")
	{

		RID map;
		CHECK_FALSE(map.is_valid());

		SUBCASE("Queries against invalid map should return empty or invalid values")
		{
			ERR_PRINT_OFF;
			CHECK_EQ(NavigationServer2D::map_get_closest_point(map, Vector2(7, 7)), Vector2());
			CHECK_FALSE(
				NavigationServer2D::map_get_closest_point_owner(map, Vector2(7, 7)).is_valid());
			CHECK_EQ(
				NavigationServer2D::map_get_path(map, Vector2(7, 7), Vector2(8, 8), true).size(), 0);
			CHECK_EQ(
				NavigationServer2D::map_get_path(map, Vector2(7, 7), Vector2(8, 8), false).size(),
				0);

			Ref<NavigationPathQueryParameters2D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector2(7, 7));
			query_parameters->set_target_position(Vector2(8, 8));
			Ref<NavigationPathQueryResult2D> query_result;
			query_result.instantiate();
			NavigationServer2D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
			ERR_PRINT_ON;
		}

		map = NavigationServer2D::map_create();
		CHECK(map.is_valid());
		CHECK_EQ(NavigationServer2D::get_maps().size(), 1);

		SUBCASE("'ProcessInfo' should not report inactive map")
		{
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_ACTIVE_MAPS), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			NavigationServer2D::map_set_cell_size(map, 0.55);
			NavigationServer2D::map_set_edge_connection_margin(map, 0.66);
			NavigationServer2D::map_set_link_connection_radius(map, 0.77);
			bool initial_use_edge_connections =
				NavigationServer2D::map_get_use_edge_connections(map);
			NavigationServer2D::map_set_use_edge_connections(map, !initial_use_edge_connections);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(NavigationServer2D::map_get_cell_size(map), doctest::Approx(0.55));
			CHECK_EQ(NavigationServer2D::map_get_edge_connection_margin(map), doctest::Approx(0.66));
			CHECK_EQ(NavigationServer2D::map_get_link_connection_radius(map), doctest::Approx(0.77));
			CHECK_EQ(NavigationServer2D::map_get_use_edge_connections(map),
				!initial_use_edge_connections);
		}

		SUBCASE("'ProcessInfo' should report map iff active")
		{
			NavigationServer2D::map_set_active(map, true);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK(NavigationServer2D::map_is_active(map));
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_ACTIVE_MAPS), 1);
			NavigationServer2D::map_set_active(map, false);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_ACTIVE_MAPS), 0);
		}

		SUBCASE("Number of agents should be reported properly")
		{
			RID agent = NavigationServer2D::agent_create();
			CHECK(agent.is_valid());
			NavigationServer2D::agent_set_map(agent, map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_agents(map).size(), 1);
			NavigationServer2D::free_rid(agent);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_agents(map).size(), 0);
		}

		SUBCASE("Number of links should be reported properly")
		{
			RID link = NavigationServer2D::link_create();
			CHECK(link.is_valid());
			NavigationServer2D::link_set_map(link, map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_links(map).size(), 1);
			NavigationServer2D::free_rid(link);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_links(map).size(), 0);
		}

		SUBCASE("Number of obstacles should be reported properly")
		{
			RID obstacle = NavigationServer2D::obstacle_create();
			CHECK(obstacle.is_valid());
			NavigationServer2D::obstacle_set_map(obstacle, map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_obstacles(map).size(), 1);
			NavigationServer2D::free_rid(obstacle);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_obstacles(map).size(), 0);
		}

		SUBCASE("Number of regions should be reported properly")
		{
			RID region = NavigationServer2D::region_create();
			CHECK(region.is_valid());
			NavigationServer2D::region_set_map(region, map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_regions(map).size(), 1);
			NavigationServer2D::free_rid(region);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::map_get_regions(map).size(), 0);
		}

		SUBCASE("Queries against empty map should return empty or invalid values")
		{
			NavigationServer2D::map_set_active(map, true);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

			ERR_PRINT_OFF;
			CHECK_EQ(NavigationServer2D::map_get_closest_point(map, Vector2(7, 7)), Vector2());
			CHECK_FALSE(
				NavigationServer2D::map_get_closest_point_owner(map, Vector2(7, 7)).is_valid());
			CHECK_EQ(
				NavigationServer2D::map_get_path(map, Vector2(7, 7), Vector2(8, 8), true).size(), 0);
			CHECK_EQ(
				NavigationServer2D::map_get_path(map, Vector2(7, 7), Vector2(8, 8), false).size(),
				0);

			Ref<NavigationPathQueryParameters2D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector2(7, 7));
			query_parameters->set_target_position(Vector2(8, 8));
			Ref<NavigationPathQueryResult2D> query_result;
			query_result.instantiate();
			NavigationServer2D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
			ERR_PRINT_ON;

			NavigationServer2D::map_set_active(map, false);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
		}

		NavigationServer2D::free_rid(map);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to actually remove map.
		CHECK_EQ(NavigationServer2D::get_maps().size(), 0);
	}

	TEST_CASE("[NavigationServer2D] Server should manage link properly")
	{

		RID link = NavigationServer2D::link_create();
		CHECK(link.is_valid());

		SUBCASE("'ProcessInfo' should not report dangling link")
		{
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_LINK_COUNT), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			bool initial_bidirectional = NavigationServer2D::link_is_bidirectional(link);
			NavigationServer2D::link_set_bidirectional(link, !initial_bidirectional);
			NavigationServer2D::link_set_end_position(link, Vector2(7, 7));
			NavigationServer2D::link_set_enter_cost(link, 0.55);
			NavigationServer2D::link_set_navigation_layers(link, 6);
			NavigationServer2D::link_set_owner_id(link, ObjectID((int64_t)7));
			NavigationServer2D::link_set_start_position(link, Vector2(8, 8));
			NavigationServer2D::link_set_travel_cost(link, 0.66);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(NavigationServer2D::link_is_bidirectional(link), !initial_bidirectional);
			CHECK_EQ(NavigationServer2D::link_get_end_position(link), Vector2(7, 7));
			CHECK_EQ(NavigationServer2D::link_get_enter_cost(link), doctest::Approx(0.55));
			CHECK_EQ(NavigationServer2D::link_get_navigation_layers(link), 6);
			CHECK_EQ(NavigationServer2D::link_get_owner_id(link), ObjectID((int64_t)7));
			CHECK_EQ(NavigationServer2D::link_get_start_position(link), Vector2(8, 8));
			CHECK_EQ(NavigationServer2D::link_get_travel_cost(link), doctest::Approx(0.66));
		}

		SUBCASE("'ProcessInfo' should report link with active map")
		{
			RID map = NavigationServer2D::map_create();
			CHECK(map.is_valid());
			NavigationServer2D::map_set_active(map, true);
			NavigationServer2D::link_set_map(link, map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_LINK_COUNT), 1);
			NavigationServer2D::link_set_map(link, RID());
			NavigationServer2D::free_rid(map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_LINK_COUNT), 0);
		}

		NavigationServer2D::free_rid(link);
	}

	TEST_CASE("[NavigationServer2D] Server should manage obstacles properly")
	{

		RID obstacle = NavigationServer2D::obstacle_create();
		CHECK(obstacle.is_valid());

		// TODO: Add tests for setters/getters once getters are added.

		NavigationServer2D::free_rid(obstacle);
	}

	TEST_CASE("[NavigationServer2D] Server should manage regions properly")
	{

		RID region = NavigationServer2D::region_create();
		CHECK(region.is_valid());

		SUBCASE("'ProcessInfo' should not report dangling region")
		{
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_REGION_COUNT), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			bool initial_use_edge_connections =
				NavigationServer2D::region_get_use_edge_connections(region);
			NavigationServer2D::region_set_enter_cost(region, 0.55);
			NavigationServer2D::region_set_navigation_layers(region, 5);
			NavigationServer2D::region_set_owner_id(region, ObjectID((int64_t)7));
			NavigationServer2D::region_set_travel_cost(region, 0.66);
			NavigationServer2D::region_set_use_edge_connections(
				region, !initial_use_edge_connections);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(NavigationServer2D::region_get_enter_cost(region), doctest::Approx(0.55));
			CHECK_EQ(NavigationServer2D::region_get_navigation_layers(region), 5);
			CHECK_EQ(NavigationServer2D::region_get_owner_id(region), ObjectID((int64_t)7));
			CHECK_EQ(NavigationServer2D::region_get_travel_cost(region), doctest::Approx(0.66));
			CHECK_EQ(NavigationServer2D::region_get_use_edge_connections(region),
				!initial_use_edge_connections);
		}

		SUBCASE("'ProcessInfo' should report region with active map")
		{
			RID map = NavigationServer2D::map_create();
			CHECK(map.is_valid());
			NavigationServer2D::map_set_active(map, true);
			NavigationServer2D::region_set_map(region, map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_REGION_COUNT), 1);
			NavigationServer2D::region_set_map(region, RID());
			NavigationServer2D::free_rid(map);
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer2D::get_process_info(NavigationServer2D::INFO_REGION_COUNT), 0);
		}

		SUBCASE("Queries against empty region should return empty or invalid values")
		{
			ERR_PRINT_OFF;
			CHECK_EQ(NavigationServer2D::region_get_connections_count(region), 0);
			CHECK_EQ(NavigationServer2D::region_get_connection_pathway_end(region, 55), Vector2());
			CHECK_EQ(NavigationServer2D::region_get_connection_pathway_start(region, 55), Vector2());
			ERR_PRINT_ON;
		}

		NavigationServer2D::free_rid(region);
	}

	// This test case does not check precise values on purpose - to not be too sensitivte.
	TEST_CASE("[NavigationServer2D] Server should move agent properly")
	{

		RID map = NavigationServer2D::map_create();
		RID agent = NavigationServer2D::agent_create();

		NavigationServer2D::map_set_active(map, true);
		NavigationServer2D::agent_set_map(agent, map);
		NavigationServer2D::agent_set_avoidance_enabled(agent, true);
		NavigationServer2D::agent_set_velocity(agent, Vector2(1, 1));
		CallableMock agent_avoidance_callback_mock;
		NavigationServer2D::agent_set_avoidance_callback(
			agent, callable_mp(&agent_avoidance_callback_mock, &CallableMock::function1));
		CHECK_EQ(agent_avoidance_callback_mock.function1_calls, 0);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_avoidance_callback_mock.function1_calls, 1);
		CHECK_NE(agent_avoidance_callback_mock.function1_latest_arg0, Vector2(0, 0));

		NavigationServer2D::free_rid(agent);
		NavigationServer2D::free_rid(map);
	}

	// This test case does not check precise values on purpose - to not be too sensitivte.
	TEST_CASE(
		"[NavigationServer2D] Server should make agents avoid each other when avoidance enabled")
	{

		RID map = NavigationServer2D::map_create();
		RID agent_1 = NavigationServer2D::agent_create();
		RID agent_2 = NavigationServer2D::agent_create();

		NavigationServer2D::map_set_active(map, true);

		NavigationServer2D::agent_set_map(agent_1, map);
		NavigationServer2D::agent_set_avoidance_enabled(agent_1, true);
		NavigationServer2D::agent_set_position(agent_1, Vector2(0, 0));
		NavigationServer2D::agent_set_radius(agent_1, 1);
		NavigationServer2D::agent_set_velocity(agent_1, Vector2(1, 0));
		CallableMock agent_1_avoidance_callback_mock;
		NavigationServer2D::agent_set_avoidance_callback(
			agent_1, callable_mp(&agent_1_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer2D::agent_set_map(agent_2, map);
		NavigationServer2D::agent_set_avoidance_enabled(agent_2, true);
		NavigationServer2D::agent_set_position(agent_2, Vector2(2.5, 0.5));
		NavigationServer2D::agent_set_radius(agent_2, 1);
		NavigationServer2D::agent_set_velocity(agent_2, Vector2(-1, 0));
		CallableMock agent_2_avoidance_callback_mock;
		NavigationServer2D::agent_set_avoidance_callback(
			agent_2, callable_mp(&agent_2_avoidance_callback_mock, &CallableMock::function1));

		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 0);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 0);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 1);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 1);
		Vector2 agent_1_safe_velocity = agent_1_avoidance_callback_mock.function1_latest_arg0;
		Vector2 agent_2_safe_velocity = agent_2_avoidance_callback_mock.function1_latest_arg0;
		CHECK_MESSAGE(
			agent_1_safe_velocity.x > 0, "agent 1 should move a bit along desired velocity (+X)");
		CHECK_MESSAGE(
			agent_2_safe_velocity.x < 0, "agent 2 should move a bit along desired velocity (-X)");
		CHECK_MESSAGE(agent_1_safe_velocity.y < 0,
			"agent 1 should move a bit to the side so that it avoids agent 2");
		CHECK_MESSAGE(agent_2_safe_velocity.y > 0,
			"agent 2 should move a bit to the side so that it avoids agent 1");

		NavigationServer2D::free_rid(agent_2);
		NavigationServer2D::free_rid(agent_1);
		NavigationServer2D::free_rid(map);
	}

	TEST_CASE("[NavigationServer2D] Server should make agents avoid dynamic obstacles when "
			  "avoidance enabled")
	{

		RID map = NavigationServer2D::map_create();
		RID agent_1 = NavigationServer2D::agent_create();
		RID obstacle_1 = NavigationServer2D::obstacle_create();

		NavigationServer2D::map_set_active(map, true);

		NavigationServer2D::agent_set_map(agent_1, map);
		NavigationServer2D::agent_set_avoidance_enabled(agent_1, true);
		NavigationServer2D::agent_set_position(agent_1, Vector2(0, 0));
		NavigationServer2D::agent_set_radius(agent_1, 1);
		NavigationServer2D::agent_set_velocity(agent_1, Vector2(1, 0));
		CallableMock agent_1_avoidance_callback_mock;
		NavigationServer2D::agent_set_avoidance_callback(
			agent_1, callable_mp(&agent_1_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer2D::obstacle_set_map(obstacle_1, map);
		NavigationServer2D::obstacle_set_avoidance_enabled(obstacle_1, true);
		NavigationServer2D::obstacle_set_position(obstacle_1, Vector2(2.5, 0.5));
		NavigationServer2D::obstacle_set_radius(obstacle_1, 1);

		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 0);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 1);
		Vector2 agent_1_safe_velocity = agent_1_avoidance_callback_mock.function1_latest_arg0;
		CHECK_MESSAGE(
			agent_1_safe_velocity.x > 0, "Agent 1 should move a bit along desired velocity (+X).");
		CHECK_MESSAGE(agent_1_safe_velocity.y < 0,
			"Agent 1 should move a bit to the side so that it avoids obstacle.");

		NavigationServer2D::free_rid(obstacle_1);
		NavigationServer2D::free_rid(agent_1);
		NavigationServer2D::free_rid(map);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
	}

	TEST_CASE("[NavigationServer2D] Server should make agents avoid static obstacles when "
			  "avoidance enabled")
	{

		RID map = NavigationServer2D::map_create();
		RID agent_1 = NavigationServer2D::agent_create();
		RID agent_2 = NavigationServer2D::agent_create();
		RID obstacle_1 = NavigationServer2D::obstacle_create();

		NavigationServer2D::map_set_active(map, true);

		NavigationServer2D::agent_set_map(agent_1, map);
		NavigationServer2D::agent_set_avoidance_enabled(agent_1, true);
		NavigationServer2D::agent_set_radius(agent_1, 1.6); // Have hit the obstacle already.
		NavigationServer2D::agent_set_velocity(agent_1, Vector2(1, 0));
		CallableMock agent_1_avoidance_callback_mock;
		NavigationServer2D::agent_set_avoidance_callback(
			agent_1, callable_mp(&agent_1_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer2D::agent_set_map(agent_2, map);
		NavigationServer2D::agent_set_avoidance_enabled(agent_2, true);
		NavigationServer2D::agent_set_radius(agent_2, 1.4); // Haven't hit the obstacle yet.
		NavigationServer2D::agent_set_velocity(agent_2, Vector2(1, 0));
		CallableMock agent_2_avoidance_callback_mock;
		NavigationServer2D::agent_set_avoidance_callback(
			agent_2, callable_mp(&agent_2_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer2D::obstacle_set_map(obstacle_1, map);
		NavigationServer2D::obstacle_set_avoidance_enabled(obstacle_1, true);
		PackedVector2Array obstacle_1_vertices;

		SUBCASE("Static obstacles should work on ground level")
		{
			NavigationServer2D::agent_set_position(agent_1, Vector2(0, 0));
			NavigationServer2D::agent_set_position(agent_2, Vector2(0, 5));
			obstacle_1_vertices.push_back(Vector2(1.5, 0.5));
			obstacle_1_vertices.push_back(Vector2(1.5, 4.5));
		}

		NavigationServer2D::obstacle_set_vertices(obstacle_1, obstacle_1_vertices);

		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 0);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 0);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 1);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 1);
		Vector2 agent_1_safe_velocity = agent_1_avoidance_callback_mock.function1_latest_arg0;
		Vector2 agent_2_safe_velocity = agent_2_avoidance_callback_mock.function1_latest_arg0;
		CHECK_MESSAGE(
			agent_1_safe_velocity.x > 0, "Agent 1 should move a bit along desired velocity (+X).");
		CHECK_MESSAGE(agent_1_safe_velocity.y < 0,
			"Agent 1 should move a bit to the side so that it avoids obstacle.");
		CHECK_MESSAGE(
			agent_2_safe_velocity.x > 0, "Agent 2 should move a bit along desired velocity (+X).");
		CHECK_MESSAGE(agent_2_safe_velocity.y == 0, "Agent 2 should not move to the side.");

		NavigationServer2D::free_rid(obstacle_1);
		NavigationServer2D::free_rid(agent_2);
		NavigationServer2D::free_rid(agent_1);
		NavigationServer2D::free_rid(map);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
	}

	TEST_CASE("[NavigationServer2D][SceneTree] Server should be able to parse geometry")
	{

		// Prepare scene tree with simple mesh to serve as an input geometry.
		Node2D* node_2d = memnew(Node2D);
		SceneTree::get_root()->add_child(node_2d);
		Polygon2D* polygon = memnew(Polygon2D);
		polygon->set_polygon(PackedVector2Array({Vector2(200.0, 200.0), Vector2(400.0, 200.0),
			Vector2(400.0, 400.0), Vector2(200.0, 400.0)}));
		node_2d->add_child(polygon);

		// TODO: Use MeshInstance2D as well?

		Ref<NavigationPolygon> navigation_polygon;
		navigation_polygon.instantiate();
		Ref<NavigationMeshSourceGeometryData2D> source_geometry;
		source_geometry.instantiate();
		CHECK_EQ(source_geometry->get_traversable_outlines().size(), 0);
		CHECK_EQ(source_geometry->get_obstruction_outlines().size(), 0);

		NavigationServer2D::parse_source_geometry_data(navigation_polygon, source_geometry, polygon);
		CHECK_EQ(source_geometry->get_traversable_outlines().size(), 0);
		REQUIRE_EQ(source_geometry->get_obstruction_outlines().size(), 1);
		CHECK_EQ(((PackedVector2Array)source_geometry->get_obstruction_outlines()[0]).size(), 4);

		SUBCASE("By default, parsing should remove any data that was parsed before")
		{
			NavigationServer2D::parse_source_geometry_data(
				navigation_polygon, source_geometry, polygon);
			CHECK_EQ(source_geometry->get_traversable_outlines().size(), 0);
			REQUIRE_EQ(source_geometry->get_obstruction_outlines().size(), 1);
			CHECK_EQ(
				((PackedVector2Array)source_geometry->get_obstruction_outlines()[0]).size(), 4);
		}

		SUBCASE("Parsed geometry should be extendible with other geometry")
		{
			source_geometry->merge(source_geometry); // Merging with itself.
			CHECK_EQ(source_geometry->get_traversable_outlines().size(), 0);
			REQUIRE_EQ(source_geometry->get_obstruction_outlines().size(), 2);
			const PackedVector2Array obstruction_outline_1 =
				source_geometry->get_obstruction_outlines()[0];
			const PackedVector2Array obstruction_outline_2 =
				source_geometry->get_obstruction_outlines()[1];
			REQUIRE_EQ(obstruction_outline_1.size(), 4);
			REQUIRE_EQ(obstruction_outline_2.size(), 4);
			CHECK_EQ(obstruction_outline_1[0], obstruction_outline_2[0]);
			CHECK_EQ(obstruction_outline_1[1], obstruction_outline_2[1]);
			CHECK_EQ(obstruction_outline_1[2], obstruction_outline_2[2]);
			CHECK_EQ(obstruction_outline_1[3], obstruction_outline_2[3]);
		}

		memdelete(polygon);
		memdelete(node_2d);
	}

	// This test case uses only public APIs on purpose - other test cases use simplified baking.
	TEST_CASE("[NavigationServer2D][SceneTree] Server should be able to bake map correctly")
	{

		// Prepare scene tree with simple mesh to serve as an input geometry.
		Node2D* node_2d = memnew(Node2D);
		SceneTree::get_root()->add_child(node_2d);
		Polygon2D* polygon = memnew(Polygon2D);
		polygon->set_polygon(PackedVector2Array({Vector2(-200.0, -200.0), Vector2(200.0, -200.0),
			Vector2(200.0, 200.0), Vector2(-200.0, 200.0)}));
		node_2d->add_child(polygon);

		// TODO: Use MeshInstance2D as well?

		// Prepare anything necessary to bake navigation polygon.
		RID map = NavigationServer2D::map_create();
		RID region = NavigationServer2D::region_create();
		Ref<NavigationPolygon> navigation_polygon;
		navigation_polygon.instantiate();
		navigation_polygon->add_outline(PackedVector2Array({Vector2(-1000.0, -1000.0),
			Vector2(1000.0, -1000.0), Vector2(1000.0, 1000.0), Vector2(-1000.0, 1000.0)}));
		NavigationServer2D::map_set_active(map, true);
		NavigationServer2D::map_set_use_async_iterations(map, false);
		NavigationServer2D::region_set_use_async_iterations(region, false);
		NavigationServer2D::region_set_map(region, map);
		NavigationServer2D::region_set_navigation_polygon(region, navigation_polygon);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

		CHECK_EQ(navigation_polygon->get_polygon_count(), 0);
		CHECK_EQ(navigation_polygon->get_vertices().size(), 0);
		CHECK_EQ(navigation_polygon->get_outline_count(), 1);

		Ref<NavigationMeshSourceGeometryData2D> source_geometry;
		source_geometry.instantiate();
		NavigationServer2D::parse_source_geometry_data(navigation_polygon, source_geometry, node_2d);
		NavigationServer2D::bake_from_source_geometry_data(
			navigation_polygon, source_geometry, Callable());
		// FIXME: The above line should trigger the update (line below) under the hood.
		NavigationServer2D::region_set_navigation_polygon(
			region, navigation_polygon); // Force update.
		CHECK_EQ(navigation_polygon->get_polygon_count(), 4);
		CHECK_EQ(navigation_polygon->get_vertices().size(), 8);
		CHECK_EQ(navigation_polygon->get_outline_count(), 1);

		SUBCASE("Map should take newly baked navigation mesh into account")
		{
			NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_NE(NavigationServer2D::map_get_closest_point(map, Vector2(0, 0)), Vector2(0, 0));
		}

		NavigationServer2D::free_rid(region);
		NavigationServer2D::free_rid(map);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

		memdelete(polygon);
		memdelete(node_2d);
	}

	// This test case does not check precise values on purpose - to not be too sensitivte.
	TEST_CASE("[NavigationServer2D] Server should respond to queries against valid map properly")
	{
		Ref<NavigationPolygon> navigation_polygon;
		navigation_polygon.instantiate();
		Ref<NavigationMeshSourceGeometryData2D> source_geometry;
		source_geometry.instantiate();

		navigation_polygon->add_outline(PackedVector2Array({Vector2(-1000.0, -1000.0),
			Vector2(1000.0, -1000.0), Vector2(1000.0, 1000.0), Vector2(-1000.0, 1000.0)}));

		// TODO: Other input?
		source_geometry->add_obstruction_outline(PackedVector2Array({Vector2(-200.0, -200.0),
			Vector2(200.0, -200.0), Vector2(200.0, 200.0), Vector2(-200.0, 200.0)}));

		NavigationServer2D::bake_from_source_geometry_data(
			navigation_polygon, source_geometry, Callable());
		CHECK_NE(navigation_polygon->get_polygon_count(), 0);
		CHECK_NE(navigation_polygon->get_vertices().size(), 0);
		CHECK_NE(navigation_polygon->get_outline_count(), 0);

		RID map = NavigationServer2D::map_create();
		RID region = NavigationServer2D::region_create();
		NavigationServer2D::map_set_active(map, true);
		NavigationServer2D::map_set_use_async_iterations(map, false);
		NavigationServer2D::region_set_use_async_iterations(region, false);
		NavigationServer2D::region_set_map(region, map);
		NavigationServer2D::region_set_navigation_polygon(region, navigation_polygon);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.

		SUBCASE("Simple queries should return non-default values")
		{
			CHECK_NE(
				NavigationServer2D::map_get_closest_point(map, Vector2(0.0, 0.0)), Vector2(0, 0));
			CHECK(
				NavigationServer2D::map_get_closest_point_owner(map, Vector2(0.0, 0.0)).is_valid());
			CHECK_NE(
				NavigationServer2D::map_get_path(map, Vector2(0, 0), Vector2(10, 10), true).size(),
				0);
			CHECK_NE(
				NavigationServer2D::map_get_path(map, Vector2(0, 0), Vector2(10, 10), false).size(),
				0);
		}

		SUBCASE(
			"Elaborate query with 'CORRIDORFUNNEL' post-processing should yield non-empty result")
		{
			Ref<NavigationPathQueryParameters2D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector2(0, 0));
			query_parameters->set_target_position(Vector2(10, 10));
			query_parameters->set_path_postprocessing(
				NavigationPathQueryParameters2D::PATH_POSTPROCESSING_CORRIDORFUNNEL);
			Ref<NavigationPathQueryResult2D> query_result;
			query_result.instantiate();
			NavigationServer2D::query_path(query_parameters, query_result);
			CHECK_NE(query_result->get_path().size(), 0);
			CHECK_NE(query_result->get_path_types().size(), 0);
			CHECK_NE(query_result->get_path_rids().size(), 0);
			CHECK_NE(query_result->get_path_owner_ids().size(), 0);
		}

		SUBCASE("Elaborate query with 'EDGECENTERED' post-processing should yield non-empty result")
		{
			Ref<NavigationPathQueryParameters2D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector2(10, 10));
			query_parameters->set_target_position(Vector2(0, 0));
			query_parameters->set_path_postprocessing(
				NavigationPathQueryParameters2D::PATH_POSTPROCESSING_EDGECENTERED);
			Ref<NavigationPathQueryResult2D> query_result;
			query_result.instantiate();
			NavigationServer2D::query_path(query_parameters, query_result);
			CHECK_NE(query_result->get_path().size(), 0);
			CHECK_NE(query_result->get_path_types().size(), 0);
			CHECK_NE(query_result->get_path_rids().size(), 0);
			CHECK_NE(query_result->get_path_owner_ids().size(), 0);
		}

		SUBCASE("Elaborate query with non-matching navigation layer mask should yield empty result")
		{
			Ref<NavigationPathQueryParameters2D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector2(10, 10));
			query_parameters->set_target_position(Vector2(0, 0));
			query_parameters->set_navigation_layers(2);
			Ref<NavigationPathQueryResult2D> query_result;
			query_result.instantiate();
			NavigationServer2D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
		}

		SUBCASE("Elaborate query without metadata flags should yield path only")
		{
			Ref<NavigationPathQueryParameters2D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector2(10, 10));
			query_parameters->set_target_position(Vector2(0, 0));
			query_parameters->set_metadata_flags(0);
			Ref<NavigationPathQueryResult2D> query_result;
			query_result.instantiate();
			NavigationServer2D::query_path(query_parameters, query_result);
			CHECK_NE(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
		}

		NavigationServer2D::free_rid(region);
		NavigationServer2D::free_rid(map);
		NavigationServer2D::physics_process(0.0); // Give server some cycles to commit.
	}

	TEST_CASE("[NavigationServer2D] Server should simplify path properly")
	{
		real_t simplify_epsilon = 0.2;
		Vector<Vector2> source_path;
		source_path.resize(7);
		source_path.write[0] = Vector2(0.0, 0.0);
		source_path.write[1] = Vector2(0.0, 1.0); // This point needs to go.
		source_path.write[2] = Vector2(0.0, 2.0); // This point needs to go.
		source_path.write[3] = Vector2(0.0, 2.0);
		source_path.write[4] = Vector2(2.0, 3.0);
		source_path.write[5] = Vector2(2.5, 4.0); // This point needs to go.
		source_path.write[6] = Vector2(3.0, 5.0);
		Vector<Vector2> simplified_path =
			NavigationServer2D::simplify_path(source_path, simplify_epsilon);
		CHECK_EQ(simplified_path.size(), 4);
	}
}

} // namespace TestNavigationServer2D

#endif // MODULE_NAVIGATION_2D_ENABLED

// === Merged from tests/servers/test_navigation_server_3d.cpp ===
/*  test_navigation_server_3d.cpp                                         */
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



#include "modules/modules_enabled.gen.h" // For navigation 3D.

#ifdef MODULE_NAVIGATION_3D_ENABLED

#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestNavigationServer3D
{
// TODO: Find a more generic way to create `Callable` mocks.
class CallableMock
{
public:
	mem_unique_ptr<Object> obj;
	void function1(Variant arg0)
	{
		function1_calls++;
		function1_latest_arg0 = arg0;
	}

	unsigned function1_calls{0};
	Variant function1_latest_arg0;
};

TEST_SUITE("[Navigation3D]")
{
	TEST_CASE("[NavigationServer3D] Server should be empty when initialized")
	{
		CHECK_EQ(NavigationServer3D::get_maps().size(), 0);

		SUBCASE("'ProcessInfo' should report all counters empty as well")
		{
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_ACTIVE_MAPS), 0);
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_REGION_COUNT), 0);
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_AGENT_COUNT), 0);
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_LINK_COUNT), 0);
			CHECK_EQ(
				NavigationServer3D::get_process_info(NavigationServer3D::INFO_POLYGON_COUNT), 0);
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_EDGE_COUNT), 0);
			CHECK_EQ(
				NavigationServer3D::get_process_info(NavigationServer3D::INFO_EDGE_MERGE_COUNT), 0);
			CHECK_EQ(
				NavigationServer3D::get_process_info(NavigationServer3D::INFO_EDGE_CONNECTION_COUNT),
				0);
			CHECK_EQ(
				NavigationServer3D::get_process_info(NavigationServer3D::INFO_EDGE_FREE_COUNT), 0);
		}
	}

	TEST_CASE("[NavigationServer3D] Server should manage agent properly")
	{

		RID agent = NavigationServer3D::agent_create();
		CHECK(agent.is_valid());

		SUBCASE("'ProcessInfo' should not report dangling agent")
		{
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_AGENT_COUNT), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			bool initial_use_3d_avoidance = NavigationServer3D::agent_get_use_3d_avoidance(agent);
			NavigationServer3D::agent_set_use_3d_avoidance(agent, !initial_use_3d_avoidance);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(
				NavigationServer3D::agent_get_use_3d_avoidance(agent), !initial_use_3d_avoidance);
			// TODO: Add remaining setters/getters once the missing getters are added.
		}

		SUBCASE("'ProcessInfo' should report agent with active map")
		{
			RID map = NavigationServer3D::map_create();
			CHECK(map.is_valid());
			NavigationServer3D::map_set_active(map, true);
			NavigationServer3D::agent_set_map(agent, map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_AGENT_COUNT), 1);
			NavigationServer3D::agent_set_map(agent, RID());
			NavigationServer3D::free_rid(map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_AGENT_COUNT), 0);
		}

		NavigationServer3D::free_rid(agent);
	}

	TEST_CASE("[NavigationServer3D] Server should manage map properly")
	{

		RID map;
		CHECK_FALSE(map.is_valid());

		SUBCASE("Queries against invalid map should return empty or invalid values")
		{
			ERR_PRINT_OFF;
			CHECK_EQ(NavigationServer3D::map_get_closest_point(map, Vector3(7, 7, 7)), Vector3());
			CHECK_EQ(
				NavigationServer3D::map_get_closest_point_normal(map, Vector3(7, 7, 7)), Vector3());
			CHECK_FALSE(
				NavigationServer3D::map_get_closest_point_owner(map, Vector3(7, 7, 7)).is_valid());
			CHECK_EQ(NavigationServer3D::map_get_closest_point_to_segment(
						 map, Vector3(7, 7, 7), Vector3(8, 8, 8), true),
				Vector3());
			CHECK_EQ(NavigationServer3D::map_get_closest_point_to_segment(
						 map, Vector3(7, 7, 7), Vector3(8, 8, 8), false),
				Vector3());
			CHECK_EQ(NavigationServer3D::map_get_path(map, Vector3(7, 7, 7), Vector3(8, 8, 8), true)
						 .size(),
				0);
			CHECK_EQ(NavigationServer3D::map_get_path(map, Vector3(7, 7, 7), Vector3(8, 8, 8), false)
						 .size(),
				0);

			Ref<NavigationPathQueryParameters3D> query_parameters =
				memnew(NavigationPathQueryParameters3D);
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(7, 7, 7));
			query_parameters->set_target_position(Vector3(8, 8, 8));
			Ref<NavigationPathQueryResult3D> query_result = memnew(NavigationPathQueryResult3D);
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
			ERR_PRINT_ON;
		}

		map = NavigationServer3D::map_create();
		CHECK(map.is_valid());
		CHECK_EQ(NavigationServer3D::get_maps().size(), 1);

		SUBCASE("'ProcessInfo' should not report inactive map")
		{
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_ACTIVE_MAPS), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			NavigationServer3D::map_set_cell_size(map, 0.55);
			NavigationServer3D::map_set_edge_connection_margin(map, 0.66);
			NavigationServer3D::map_set_link_connection_radius(map, 0.77);
			NavigationServer3D::map_set_up(map, Vector3(1, 0, 0));
			bool initial_use_edge_connections =
				NavigationServer3D::map_get_use_edge_connections(map);
			NavigationServer3D::map_set_use_edge_connections(map, !initial_use_edge_connections);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(NavigationServer3D::map_get_cell_size(map), doctest::Approx(0.55));
			CHECK_EQ(NavigationServer3D::map_get_edge_connection_margin(map), doctest::Approx(0.66));
			CHECK_EQ(NavigationServer3D::map_get_link_connection_radius(map), doctest::Approx(0.77));
			CHECK_EQ(NavigationServer3D::map_get_up(map), Vector3(1, 0, 0));
			CHECK_EQ(NavigationServer3D::map_get_use_edge_connections(map),
				!initial_use_edge_connections);
		}

		SUBCASE("'ProcessInfo' should report map iff active")
		{
			NavigationServer3D::map_set_active(map, true);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK(NavigationServer3D::map_is_active(map));
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_ACTIVE_MAPS), 1);
			NavigationServer3D::map_set_active(map, false);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_ACTIVE_MAPS), 0);
		}

		SUBCASE("Number of agents should be reported properly")
		{
			RID agent = NavigationServer3D::agent_create();
			CHECK(agent.is_valid());
			NavigationServer3D::agent_set_map(agent, map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_agents(map).size(), 1);
			NavigationServer3D::free_rid(agent);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_agents(map).size(), 0);
		}

		SUBCASE("Number of links should be reported properly")
		{
			RID link = NavigationServer3D::link_create();
			CHECK(link.is_valid());
			NavigationServer3D::link_set_map(link, map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_links(map).size(), 1);
			NavigationServer3D::free_rid(link);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_links(map).size(), 0);
		}

		SUBCASE("Number of obstacles should be reported properly")
		{
			RID obstacle = NavigationServer3D::obstacle_create();
			CHECK(obstacle.is_valid());
			NavigationServer3D::obstacle_set_map(obstacle, map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_obstacles(map).size(), 1);
			NavigationServer3D::free_rid(obstacle);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_obstacles(map).size(), 0);
		}

		SUBCASE("Number of regions should be reported properly")
		{
			RID region = NavigationServer3D::region_create();
			CHECK(region.is_valid());
			NavigationServer3D::region_set_map(region, map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_regions(map).size(), 1);
			NavigationServer3D::free_rid(region);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::map_get_regions(map).size(), 0);
		}

		SUBCASE("Queries against empty map should return empty or invalid values")
		{
			NavigationServer3D::map_set_active(map, true);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

			ERR_PRINT_OFF;
			CHECK_EQ(NavigationServer3D::map_get_closest_point(map, Vector3(7, 7, 7)), Vector3());
			CHECK_EQ(
				NavigationServer3D::map_get_closest_point_normal(map, Vector3(7, 7, 7)), Vector3());
			CHECK_FALSE(
				NavigationServer3D::map_get_closest_point_owner(map, Vector3(7, 7, 7)).is_valid());
			CHECK_EQ(NavigationServer3D::map_get_closest_point_to_segment(
						 map, Vector3(7, 7, 7), Vector3(8, 8, 8), true),
				Vector3());
			CHECK_EQ(NavigationServer3D::map_get_closest_point_to_segment(
						 map, Vector3(7, 7, 7), Vector3(8, 8, 8), false),
				Vector3());
			CHECK_EQ(NavigationServer3D::map_get_path(map, Vector3(7, 7, 7), Vector3(8, 8, 8), true)
						 .size(),
				0);
			CHECK_EQ(NavigationServer3D::map_get_path(map, Vector3(7, 7, 7), Vector3(8, 8, 8), false)
						 .size(),
				0);

			Ref<NavigationPathQueryParameters3D> query_parameters =
				memnew(NavigationPathQueryParameters3D);
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(7, 7, 7));
			query_parameters->set_target_position(Vector3(8, 8, 8));
			Ref<NavigationPathQueryResult3D> query_result = memnew(NavigationPathQueryResult3D);
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
			ERR_PRINT_ON;

			NavigationServer3D::map_set_active(map, false);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
		}

		NavigationServer3D::free_rid(map);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to actually remove map.
		CHECK_EQ(NavigationServer3D::get_maps().size(), 0);
	}

	TEST_CASE("[NavigationServer3D] Server should manage link properly")
	{

		RID link = NavigationServer3D::link_create();
		CHECK(link.is_valid());

		SUBCASE("'ProcessInfo' should not report dangling link")
		{
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_LINK_COUNT), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			bool initial_bidirectional = NavigationServer3D::link_is_bidirectional(link);
			NavigationServer3D::link_set_bidirectional(link, !initial_bidirectional);
			NavigationServer3D::link_set_end_position(link, Vector3(7, 7, 7));
			NavigationServer3D::link_set_enter_cost(link, 0.55);
			NavigationServer3D::link_set_navigation_layers(link, 6);
			NavigationServer3D::link_set_owner_id(link, ObjectID((int64_t)7));
			NavigationServer3D::link_set_start_position(link, Vector3(8, 8, 8));
			NavigationServer3D::link_set_travel_cost(link, 0.66);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(NavigationServer3D::link_is_bidirectional(link), !initial_bidirectional);
			CHECK_EQ(NavigationServer3D::link_get_end_position(link), Vector3(7, 7, 7));
			CHECK_EQ(NavigationServer3D::link_get_enter_cost(link), doctest::Approx(0.55));
			CHECK_EQ(NavigationServer3D::link_get_navigation_layers(link), 6);
			CHECK_EQ(NavigationServer3D::link_get_owner_id(link), ObjectID((int64_t)7));
			CHECK_EQ(NavigationServer3D::link_get_start_position(link), Vector3(8, 8, 8));
			CHECK_EQ(NavigationServer3D::link_get_travel_cost(link), doctest::Approx(0.66));
		}

		SUBCASE("'ProcessInfo' should report link with active map")
		{
			RID map = NavigationServer3D::map_create();
			CHECK(map.is_valid());
			NavigationServer3D::map_set_active(map, true);
			NavigationServer3D::link_set_map(link, map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_LINK_COUNT), 1);
			NavigationServer3D::link_set_map(link, RID());
			NavigationServer3D::free_rid(map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_LINK_COUNT), 0);
		}

		NavigationServer3D::free_rid(link);
	}

	TEST_CASE("[NavigationServer3D] Server should manage obstacles properly")
	{

		RID obstacle = NavigationServer3D::obstacle_create();
		CHECK(obstacle.is_valid());

		// TODO: Add tests for setters/getters once getters are added.

		NavigationServer3D::free_rid(obstacle);
	}

	TEST_CASE("[NavigationServer3D] Server should manage regions properly")
	{

		RID region = NavigationServer3D::region_create();
		CHECK(region.is_valid());

		SUBCASE("'ProcessInfo' should not report dangling region")
		{
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_REGION_COUNT), 0);
		}

		SUBCASE("Setters/getters should work")
		{
			bool initial_use_edge_connections =
				NavigationServer3D::region_get_use_edge_connections(region);
			NavigationServer3D::region_set_enter_cost(region, 0.55);
			NavigationServer3D::region_set_navigation_layers(region, 5);
			NavigationServer3D::region_set_owner_id(region, ObjectID((int64_t)7));
			NavigationServer3D::region_set_travel_cost(region, 0.66);
			NavigationServer3D::region_set_use_edge_connections(
				region, !initial_use_edge_connections);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

			CHECK_EQ(NavigationServer3D::region_get_enter_cost(region), doctest::Approx(0.55));
			CHECK_EQ(NavigationServer3D::region_get_navigation_layers(region), 5);
			CHECK_EQ(NavigationServer3D::region_get_owner_id(region), ObjectID((int64_t)7));
			CHECK_EQ(NavigationServer3D::region_get_travel_cost(region), doctest::Approx(0.66));
			CHECK_EQ(NavigationServer3D::region_get_use_edge_connections(region),
				!initial_use_edge_connections);
		}

		SUBCASE("'ProcessInfo' should report region with active map")
		{
			RID map = NavigationServer3D::map_create();
			CHECK(map.is_valid());
			NavigationServer3D::map_set_active(map, true);
			NavigationServer3D::region_set_map(region, map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_REGION_COUNT), 1);
			NavigationServer3D::region_set_map(region, RID());
			NavigationServer3D::free_rid(map);
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_EQ(NavigationServer3D::get_process_info(NavigationServer3D::INFO_REGION_COUNT), 0);
		}

		SUBCASE("Queries against empty region should return empty or invalid values")
		{
			ERR_PRINT_OFF;
			CHECK_EQ(NavigationServer3D::region_get_connections_count(region), 0);
			CHECK_EQ(NavigationServer3D::region_get_connection_pathway_end(region, 55), Vector3());
			CHECK_EQ(NavigationServer3D::region_get_connection_pathway_start(region, 55), Vector3());
			ERR_PRINT_ON;
		}

		NavigationServer3D::free_rid(region);
	}

	// This test case does not check precise values on purpose - to not be too sensitivte.
	TEST_CASE("[NavigationServer3D] Server should move agent properly")
	{

		RID map = NavigationServer3D::map_create();
		RID agent = NavigationServer3D::agent_create();

		NavigationServer3D::map_set_active(map, true);
		NavigationServer3D::agent_set_map(agent, map);
		NavigationServer3D::agent_set_avoidance_enabled(agent, true);
		NavigationServer3D::agent_set_velocity(agent, Vector3(1, 0, 1));
		CallableMock agent_avoidance_callback_mock;
		NavigationServer3D::agent_set_avoidance_callback(
			agent, callable_mp(&agent_avoidance_callback_mock, &CallableMock::function1));
		CHECK_EQ(agent_avoidance_callback_mock.function1_calls, 0);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_avoidance_callback_mock.function1_calls, 1);
		CHECK_NE(agent_avoidance_callback_mock.function1_latest_arg0, Vector3(0, 0, 0));

		NavigationServer3D::free_rid(agent);
		NavigationServer3D::free_rid(map);
	}

	// This test case does not check precise values on purpose - to not be too sensitivte.
	TEST_CASE(
		"[NavigationServer3D] Server should make agents avoid each other when avoidance enabled")
	{

		RID map = NavigationServer3D::map_create();
		RID agent_1 = NavigationServer3D::agent_create();
		RID agent_2 = NavigationServer3D::agent_create();

		NavigationServer3D::map_set_active(map, true);

		NavigationServer3D::agent_set_map(agent_1, map);
		NavigationServer3D::agent_set_avoidance_enabled(agent_1, true);
		NavigationServer3D::agent_set_position(agent_1, Vector3(0, 0, 0));
		NavigationServer3D::agent_set_radius(agent_1, 1);
		NavigationServer3D::agent_set_velocity(agent_1, Vector3(1, 0, 0));
		CallableMock agent_1_avoidance_callback_mock;
		NavigationServer3D::agent_set_avoidance_callback(
			agent_1, callable_mp(&agent_1_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer3D::agent_set_map(agent_2, map);
		NavigationServer3D::agent_set_avoidance_enabled(agent_2, true);
		NavigationServer3D::agent_set_position(agent_2, Vector3(2.5, 0, 0.5));
		NavigationServer3D::agent_set_radius(agent_2, 1);
		NavigationServer3D::agent_set_velocity(agent_2, Vector3(-1, 0, 0));
		CallableMock agent_2_avoidance_callback_mock;
		NavigationServer3D::agent_set_avoidance_callback(
			agent_2, callable_mp(&agent_2_avoidance_callback_mock, &CallableMock::function1));

		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 0);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 0);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 1);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 1);
		Vector3 agent_1_safe_velocity = agent_1_avoidance_callback_mock.function1_latest_arg0;
		Vector3 agent_2_safe_velocity = agent_2_avoidance_callback_mock.function1_latest_arg0;
		CHECK_MESSAGE(
			agent_1_safe_velocity.x > 0, "agent 1 should move a bit along desired velocity (+X)");
		CHECK_MESSAGE(
			agent_2_safe_velocity.x < 0, "agent 2 should move a bit along desired velocity (-X)");
		CHECK_MESSAGE(agent_1_safe_velocity.z < 0,
			"agent 1 should move a bit to the side so that it avoids agent 2");
		CHECK_MESSAGE(agent_2_safe_velocity.z > 0,
			"agent 2 should move a bit to the side so that it avoids agent 1");

		NavigationServer3D::free_rid(agent_2);
		NavigationServer3D::free_rid(agent_1);
		NavigationServer3D::free_rid(map);
	}

	TEST_CASE("[NavigationServer3D] Server should make agents avoid dynamic obstacles when "
			  "avoidance enabled")
	{

		RID map = NavigationServer3D::map_create();
		RID agent_1 = NavigationServer3D::agent_create();
		RID obstacle_1 = NavigationServer3D::obstacle_create();

		NavigationServer3D::map_set_active(map, true);

		NavigationServer3D::agent_set_map(agent_1, map);
		NavigationServer3D::agent_set_avoidance_enabled(agent_1, true);
		NavigationServer3D::agent_set_position(agent_1, Vector3(0, 0, 0));
		NavigationServer3D::agent_set_radius(agent_1, 1);
		NavigationServer3D::agent_set_velocity(agent_1, Vector3(1, 0, 0));
		CallableMock agent_1_avoidance_callback_mock;
		NavigationServer3D::agent_set_avoidance_callback(
			agent_1, callable_mp(&agent_1_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer3D::obstacle_set_map(obstacle_1, map);
		NavigationServer3D::obstacle_set_avoidance_enabled(obstacle_1, true);
		NavigationServer3D::obstacle_set_position(obstacle_1, Vector3(2.5, 0, 0.5));
		NavigationServer3D::obstacle_set_radius(obstacle_1, 1);

		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 0);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 1);
		Vector3 agent_1_safe_velocity = agent_1_avoidance_callback_mock.function1_latest_arg0;
		CHECK_MESSAGE(
			agent_1_safe_velocity.x > 0, "Agent 1 should move a bit along desired velocity (+X).");
		CHECK_MESSAGE(agent_1_safe_velocity.z < 0,
			"Agent 1 should move a bit to the side so that it avoids obstacle.");

		NavigationServer3D::free_rid(obstacle_1);
		NavigationServer3D::free_rid(agent_1);
		NavigationServer3D::free_rid(map);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
	}

	TEST_CASE("[NavigationServer3D] Server should make agents avoid static obstacles when "
			  "avoidance enabled")
	{

		RID map = NavigationServer3D::map_create();
		RID agent_1 = NavigationServer3D::agent_create();
		RID agent_2 = NavigationServer3D::agent_create();
		RID obstacle_1 = NavigationServer3D::obstacle_create();

		NavigationServer3D::map_set_active(map, true);

		NavigationServer3D::agent_set_map(agent_1, map);
		NavigationServer3D::agent_set_avoidance_enabled(agent_1, true);
		NavigationServer3D::agent_set_radius(agent_1, 1.6); // Have hit the obstacle already.
		NavigationServer3D::agent_set_velocity(agent_1, Vector3(1, 0, 0));
		CallableMock agent_1_avoidance_callback_mock;
		NavigationServer3D::agent_set_avoidance_callback(
			agent_1, callable_mp(&agent_1_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer3D::agent_set_map(agent_2, map);
		NavigationServer3D::agent_set_avoidance_enabled(agent_2, true);
		NavigationServer3D::agent_set_radius(agent_2, 1.4); // Haven't hit the obstacle yet.
		NavigationServer3D::agent_set_velocity(agent_2, Vector3(1, 0, 0));
		CallableMock agent_2_avoidance_callback_mock;
		NavigationServer3D::agent_set_avoidance_callback(
			agent_2, callable_mp(&agent_2_avoidance_callback_mock, &CallableMock::function1));

		NavigationServer3D::obstacle_set_map(obstacle_1, map);
		NavigationServer3D::obstacle_set_avoidance_enabled(obstacle_1, true);
		PackedVector3Array obstacle_1_vertices;

		SUBCASE("Static obstacles should work on ground level")
		{
			NavigationServer3D::agent_set_position(agent_1, Vector3(0, 0, 0));
			NavigationServer3D::agent_set_position(agent_2, Vector3(0, 0, 5));
			obstacle_1_vertices.push_back(Vector3(1.5, 0, 0.5));
			obstacle_1_vertices.push_back(Vector3(1.5, 0, 4.5));
		}

		SUBCASE("Static obstacles should work when elevated")
		{
			NavigationServer3D::agent_set_position(agent_1, Vector3(0, 5, 0));
			NavigationServer3D::agent_set_position(agent_2, Vector3(0, 5, 5));
			obstacle_1_vertices.push_back(Vector3(1.5, 0, 0.5));
			obstacle_1_vertices.push_back(Vector3(1.5, 0, 4.5));
			NavigationServer3D::obstacle_set_position(obstacle_1, Vector3(0, 5, 0));
		}

		NavigationServer3D::obstacle_set_vertices(obstacle_1, obstacle_1_vertices);

		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 0);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 0);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
		CHECK_EQ(agent_1_avoidance_callback_mock.function1_calls, 1);
		CHECK_EQ(agent_2_avoidance_callback_mock.function1_calls, 1);
		Vector3 agent_1_safe_velocity = agent_1_avoidance_callback_mock.function1_latest_arg0;
		Vector3 agent_2_safe_velocity = agent_2_avoidance_callback_mock.function1_latest_arg0;
		CHECK_MESSAGE(
			agent_1_safe_velocity.x > 0, "Agent 1 should move a bit along desired velocity (+X).");
		CHECK_MESSAGE(agent_1_safe_velocity.z < 0,
			"Agent 1 should move a bit to the side so that it avoids obstacle.");
		CHECK_MESSAGE(
			agent_2_safe_velocity.x > 0, "Agent 2 should move a bit along desired velocity (+X).");
		CHECK_MESSAGE(agent_2_safe_velocity.z == 0, "Agent 2 should not move to the side.");

		NavigationServer3D::free_rid(obstacle_1);
		NavigationServer3D::free_rid(agent_2);
		NavigationServer3D::free_rid(agent_1);
		NavigationServer3D::free_rid(map);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
	}

#ifndef DISABLE_DEPRECATED
	// This test case uses only public APIs on purpose - other test cases use simplified baking.
	// FIXME: Remove once deprecated `region_bake_navigation_mesh()` is removed.
	TEST_CASE(
		"[NavigationServer3D][SceneTree][DEPRECATED] Server should be able to bake map correctly")
	{

		// Prepare scene tree with simple mesh to serve as an input geometry.
		Node3D* node_3d = memnew(Node3D);
		SceneTree::get_root()->add_child(node_3d);
		Ref<PrimitiveMesh> plane_mesh = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
		plane_mesh->set_plane_size(Size2(10.0, 10.0));
		MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_mesh(plane_mesh);
		node_3d->add_child(mesh_instance);

		// Prepare anything necessary to bake navigation mesh.
		RID map = NavigationServer3D::map_create();
		RID region = NavigationServer3D::region_create();
		Ref<NavigationMesh> navigation_mesh = memnew(NavigationMesh);
		NavigationServer3D::map_set_use_async_iterations(map, false);
		NavigationServer3D::map_set_active(map, true);
		NavigationServer3D::region_set_use_async_iterations(region, false);
		NavigationServer3D::region_set_map(region, map);
		NavigationServer3D::region_set_navigation_mesh(region, navigation_mesh);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

		CHECK_EQ(navigation_mesh->get_polygon_count(), 0);
		CHECK_EQ(navigation_mesh->get_vertices().size(), 0);

		ERR_PRINT_OFF;
		NavigationServer3D::region_bake_navigation_mesh(navigation_mesh, node_3d);
		ERR_PRINT_ON;
		// FIXME: The above line should trigger the update (line below) under the hood.
		NavigationServer3D::region_set_navigation_mesh(region, navigation_mesh); // Force update.
		CHECK_EQ(navigation_mesh->get_polygon_count(), 2);
		CHECK_EQ(navigation_mesh->get_vertices().size(), 4);

		SUBCASE("Map should take newly baked navigation mesh into account")
		{
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_NE(
				NavigationServer3D::map_get_closest_point(map, Vector3(0, 0, 0)), Vector3(0, 0, 0));
		}

		NavigationServer3D::free_rid(region);
		NavigationServer3D::free_rid(map);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
		memdelete(mesh_instance);
		memdelete(node_3d);
	}
#endif // DISABLE_DEPRECATED

	TEST_CASE("[NavigationServer3D][SceneTree] Server should be able to parse geometry")
	{

		// Prepare scene tree with simple mesh to serve as an input geometry.
		Node3D* node_3d = memnew(Node3D);
		SceneTree::get_root()->add_child(node_3d);
		Ref<PrimitiveMesh> plane_mesh = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
		plane_mesh->set_plane_size(Size2(10.0, 10.0));
		MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_mesh(plane_mesh);
		node_3d->add_child(mesh_instance);

		Ref<NavigationMesh> navigation_mesh = memnew(NavigationMesh);
		Ref<NavigationMeshSourceGeometryData3D> source_geometry =
			memnew(NavigationMeshSourceGeometryData3D);
		CHECK_EQ(source_geometry->get_vertices().size(), 0);
		CHECK_EQ(source_geometry->get_indices().size(), 0);

		NavigationServer3D::parse_source_geometry_data(
			navigation_mesh, source_geometry, mesh_instance);
		CHECK_EQ(source_geometry->get_vertices().size(), 12);
		CHECK_EQ(source_geometry->get_indices().size(), 6);

		SUBCASE("By default, parsing should remove any data that was parsed before")
		{
			NavigationServer3D::parse_source_geometry_data(
				navigation_mesh, source_geometry, mesh_instance);
			CHECK_EQ(source_geometry->get_vertices().size(), 12);
			CHECK_EQ(source_geometry->get_indices().size(), 6);
		}

		SUBCASE("Parsed geometry should be extendable with other geometry")
		{
			source_geometry->merge(source_geometry); // Merging with itself.
			const Vector<float> vertices = source_geometry->get_vertices();
			const Vector<int> indices = source_geometry->get_indices();
			REQUIRE_EQ(vertices.size(), 24);
			REQUIRE_EQ(indices.size(), 12);
			// Check if first newly added vertex is the same as first vertex.
			CHECK_EQ(vertices[0], vertices[12]);
			CHECK_EQ(vertices[1], vertices[13]);
			CHECK_EQ(vertices[2], vertices[14]);
			// Check if first newly added index is the same as first index.
			CHECK_EQ(indices[0] + 4, indices[6]);
		}

		memdelete(mesh_instance);
		memdelete(node_3d);
	}

	// This test case uses only public APIs on purpose - other test cases use simplified baking.
	TEST_CASE("[NavigationServer3D][SceneTree] Server should be able to bake map correctly")
	{

		// Prepare scene tree with simple mesh to serve as an input geometry.
		Node3D* node_3d = memnew(Node3D);
		SceneTree::get_root()->add_child(node_3d);
		Ref<PrimitiveMesh> plane_mesh = memnew(PrimitiveMesh(PrimitiveMesh::MESH_PLANE));
		plane_mesh->set_plane_size(Size2(10.0, 10.0));
		MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
		mesh_instance->set_mesh(plane_mesh);
		node_3d->add_child(mesh_instance);

		// Prepare anything necessary to bake navigation mesh.
		RID map = NavigationServer3D::map_create();
		RID region = NavigationServer3D::region_create();
		Ref<NavigationMesh> navigation_mesh = memnew(NavigationMesh);
		NavigationServer3D::map_set_use_async_iterations(map, false);
		NavigationServer3D::map_set_active(map, true);
		NavigationServer3D::region_set_use_async_iterations(region, false);
		NavigationServer3D::region_set_map(region, map);
		NavigationServer3D::region_set_navigation_mesh(region, navigation_mesh);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

		CHECK_EQ(navigation_mesh->get_polygon_count(), 0);
		CHECK_EQ(navigation_mesh->get_vertices().size(), 0);

		Ref<NavigationMeshSourceGeometryData3D> source_geometry =
			memnew(NavigationMeshSourceGeometryData3D);
		NavigationServer3D::parse_source_geometry_data(navigation_mesh, source_geometry, node_3d);
		NavigationServer3D::bake_from_source_geometry_data(
			navigation_mesh, source_geometry, Callable());
		// FIXME: The above line should trigger the update (line below) under the hood.
		NavigationServer3D::region_set_navigation_mesh(region, navigation_mesh); // Force update.
		CHECK_EQ(navigation_mesh->get_polygon_count(), 2);
		CHECK_EQ(navigation_mesh->get_vertices().size(), 4);

		SUBCASE("Map should take newly baked navigation mesh into account")
		{
			NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
			CHECK_NE(
				NavigationServer3D::map_get_closest_point(map, Vector3(0, 0, 0)), Vector3(0, 0, 0));
		}

		NavigationServer3D::free_rid(region);
		NavigationServer3D::free_rid(map);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
		memdelete(mesh_instance);
		memdelete(node_3d);
	}

	// This test case does not check precise values on purpose - to not be too sensitivte.
	TEST_CASE("[NavigationServer3D] Server should respond to queries against valid map properly")
	{
		Ref<NavigationMesh> navigation_mesh = memnew(NavigationMesh);
		Ref<NavigationMeshSourceGeometryData3D> source_geometry =
			memnew(NavigationMeshSourceGeometryData3D);

		Ref<PrimitiveMesh> box_mesh = memnew(PrimitiveMesh(PrimitiveMesh::MESH_BOX));
		box_mesh->set_size(Vector3(10.0, 0.001, 10.0));
		source_geometry->add_mesh(box_mesh, Transform3D());
		NavigationServer3D::bake_from_source_geometry_data(
			navigation_mesh, source_geometry, Callable());
		CHECK_NE(navigation_mesh->get_polygon_count(), 0);
		CHECK_NE(navigation_mesh->get_vertices().size(), 0);

		RID map = NavigationServer3D::map_create();
		RID region = NavigationServer3D::region_create();
		NavigationServer3D::map_set_active(map, true);
		NavigationServer3D::map_set_use_async_iterations(map, false);
		NavigationServer3D::region_set_use_async_iterations(region, false);
		NavigationServer3D::region_set_map(region, map);
		NavigationServer3D::region_set_navigation_mesh(region, navigation_mesh);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.

		SUBCASE("Simple queries should return non-default values")
		{
			CHECK_NE(
				NavigationServer3D::map_get_closest_point(map, Vector3(0, 0, 0)), Vector3(0, 0, 0));
			CHECK_NE(
				NavigationServer3D::map_get_closest_point_normal(map, Vector3(0, 0, 0)), Vector3());
			CHECK(NavigationServer3D::map_get_closest_point_owner(map, Vector3(0, 0, 0)).is_valid());
			CHECK_NE(NavigationServer3D::map_get_closest_point_to_segment(
						 map, Vector3(0, 0, 0), Vector3(1, 1, 1), false),
				Vector3());
			CHECK_NE(NavigationServer3D::map_get_closest_point_to_segment(
						 map, Vector3(0, 0, 0), Vector3(1, 1, 1), true),
				Vector3());
			CHECK_NE(
				NavigationServer3D::map_get_path(map, Vector3(0, 0, 0), Vector3(10, 0, 10), true)
					.size(),
				0);
			CHECK_NE(
				NavigationServer3D::map_get_path(map, Vector3(0, 0, 0), Vector3(10, 0, 10), false)
					.size(),
				0);
		}

		SUBCASE("'map_get_closest_point_to_segment' with 'use_collision' should return default if "
				"segment doesn't intersect map")
		{
			CHECK_EQ(NavigationServer3D::map_get_closest_point_to_segment(
						 map, Vector3(1, 2, 1), Vector3(1, 1, 1), true),
				Vector3());
		}

		SUBCASE(
			"Elaborate query with 'CORRIDORFUNNEL' post-processing should yield non-empty result")
		{
			Ref<NavigationPathQueryParameters3D> query_parameters =
				memnew(NavigationPathQueryParameters3D);
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(0, 0, 0));
			query_parameters->set_target_position(Vector3(10, 0, 10));
			query_parameters->set_path_postprocessing(
				NavigationPathQueryParameters3D::PATH_POSTPROCESSING_CORRIDORFUNNEL);
			Ref<NavigationPathQueryResult3D> query_result = memnew(NavigationPathQueryResult3D);
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_NE(query_result->get_path().size(), 0);
			CHECK_NE(query_result->get_path_types().size(), 0);
			CHECK_NE(query_result->get_path_rids().size(), 0);
			CHECK_NE(query_result->get_path_owner_ids().size(), 0);
		}

		SUBCASE("Elaborate query with 'EDGECENTERED' post-processing should yield non-empty result")
		{
			Ref<NavigationPathQueryParameters3D> query_parameters =
				memnew(NavigationPathQueryParameters3D);
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(10, 0, 10));
			query_parameters->set_target_position(Vector3(0, 0, 0));
			query_parameters->set_path_postprocessing(
				NavigationPathQueryParameters3D::PATH_POSTPROCESSING_EDGECENTERED);
			Ref<NavigationPathQueryResult3D> query_result = memnew(NavigationPathQueryResult3D);
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_NE(query_result->get_path().size(), 0);
			CHECK_NE(query_result->get_path_types().size(), 0);
			CHECK_NE(query_result->get_path_rids().size(), 0);
			CHECK_NE(query_result->get_path_owner_ids().size(), 0);
		}

		SUBCASE("Elaborate query with non-matching navigation layer mask should yield empty result")
		{
			Ref<NavigationPathQueryParameters3D> query_parameters =
				memnew(NavigationPathQueryParameters3D);
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(10, 0, 10));
			query_parameters->set_target_position(Vector3(0, 0, 0));
			query_parameters->set_navigation_layers(2);
			Ref<NavigationPathQueryResult3D> query_result = memnew(NavigationPathQueryResult3D);
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
		}

		SUBCASE("Elaborate query without metadata flags should yield path only")
		{
			Ref<NavigationPathQueryParameters3D> query_parameters =
				memnew(NavigationPathQueryParameters3D);
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(10, 0, 10));
			query_parameters->set_target_position(Vector3(0, 0, 0));
			query_parameters->set_metadata_flags(0);
			Ref<NavigationPathQueryResult3D> query_result = memnew(NavigationPathQueryResult3D);
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_NE(query_result->get_path().size(), 0);
			CHECK_EQ(query_result->get_path_types().size(), 0);
			CHECK_EQ(query_result->get_path_rids().size(), 0);
			CHECK_EQ(query_result->get_path_owner_ids().size(), 0);
		}

		SUBCASE("Elaborate query with excluded region should yield empty path")
		{
			Ref<NavigationPathQueryParameters3D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(10,
 0, 10));
			query_parameters->set_target_position(Vector3(0, 0, 0));
			query_parameters->set_excluded_regions({region});
			Ref<NavigationPathQueryResult3D> query_result;
			query_result.instantiate();
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
		}

		SUBCASE("Elaborate query with included region should yield path")
		{
			Ref<NavigationPathQueryParameters3D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(10, 0, 10));
			query_parameters->set_target_position(Vector3(0, 0, 0));
			query_parameters->set_included_regions({region});
			Ref<NavigationPathQueryResult3D> query_result;
			query_result.instantiate();
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_NE(query_result->get_path().size(), 0);
		}

		SUBCASE("Elaborate query with excluded and included region should yield empty path")
		{
			Ref<NavigationPathQueryParameters3D> query_parameters;
			query_parameters.instantiate();
			query_parameters->set_map(map);
			query_parameters->set_start_position(Vector3(10, 0, 10));
			query_parameters->set_target_position(Vector3(0, 0, 0));
			query_parameters->set_excluded_regions({region});
			query_parameters->set_included_regions({region});
			Ref<NavigationPathQueryResult3D> query_result;
			query_result.instantiate();
			NavigationServer3D::query_path(query_parameters, query_result);
			CHECK_EQ(query_result->get_path().size(), 0);
		}

		NavigationServer3D::free_rid(region);
		NavigationServer3D::free_rid(map);
		NavigationServer3D::physics_process(0.0); // Give server some cycles to commit.
	}

	// FIXME: The race condition mentioned below is actually a problem and fails on CI (GH-90613).
	/*
	TEST_CASE("[NavigationServer3D] Server should be able to bake asynchronously") {
		Ref<NavigationMesh> navigation_mesh = memnew(NavigationMesh);
		Ref<NavigationMeshSourceGeometryData3D> source_geometry =
	memnew(NavigationMeshSourceGeometryData3D);

		Ref<PrimitiveMesh> box_mesh = memnew(PrimitiveMesh(PrimitiveMesh::MESH_BOX));
		box_mesh->set_size(Vector3(10.0, 0.001, 10.0));
		source_geometry->add_mesh(box_mesh, Transform3D());

		// Race condition is present below, but baking should take many orders of magnitude
		// longer than basic checks on the main thread, so it's fine.
		NavigationServer3D::bake_from_source_geometry_data_async(navigation_mesh, source_geometry,
	Callable()); CHECK(NavigationServer3D::is_baking_navigation_mesh(navigation_mesh));
		CHECK_EQ(navigation_mesh->get_polygon_count(), 0);
		CHECK_EQ(navigation_mesh->get_vertices().size(), 0);
	}
	*/

	TEST_CASE("[NavigationServer3D] Server should simplify path properly")
	{
		real_t simplify_epsilon = 0.2;
		Vector<Vector3> source_path;
		source_path.resize(7);
		source_path.write[0] = Vector3(0.0, 0.0, 0.0);
		source_path.write[1] = Vector3(0.0, 0.0, 1.0); // This point needs to go.
		source_path.write[2] = Vector3(0.0, 0.0, 2.0); // This point needs to go.
		source_path.write[3] = Vector3(0.0, 0.0, 2.0);
		source_path.write[4] = Vector3(2.0, 1.0, 3.0);
		source_path.write[5] = Vector3(2.0, 1.5, 4.0); // This point needs to go.
		source_path.write[6] = Vector3(2.0, 2.0, 5.0);
		Vector<Vector3> simplified_path =
			NavigationServer3D::simplify_path(source_path, simplify_epsilon);
		CHECK_EQ(simplified_path.size(), 4);
	}
}

} // namespace TestNavigationServer3D

#endif // MODULE_NAVIGATION_3D_ENABLED

// === Merged from tests/servers/test_text_server.cpp ===
/*  test_text_server.cpp                                                  */
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



#ifdef TOOLS_ENABLED

#include "editor/themes/builtin_fonts.gen.h"
#include "scene/resources/theme.h"

namespace TestTextServer {

TEST_SUITE("[TextServer]") {
	TEST_CASE("[TextServer] Init, font loading and shaping") {
		SUBCASE("[TextServer] Loading fonts") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_FONT_DYNAMIC)) {
					continue;
				}

				RID font = ts->create_font();
				ts->font_set_data_ptr(font, _font_Inter_Regular, _font_Inter_Regular_size);
				CHECK_FALSE_MESSAGE(font == RID(), "Loading font failed.");
				ts->free_rid(font);
			}
		}

		SUBCASE("[TextServer] Text layout: Font fallback") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_FONT_DYNAMIC) || !ts->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
					continue;
				}

				RID font1 = ts->create_font();
				ts->font_set_data_ptr(font1, _font_Inter_Regular, _font_Inter_Regular_size);
				ts->font_set_allow_system_fallback(font1, false);
				RID font2 = ts->create_font();
				ts->font_set_data_ptr(font2, _font_NotoSansThai_Regular, _font_NotoSansThai_Regular_size);
				ts->font_set_allow_system_fallback(font2, false);

				Array font = { font1, font2 };
				String test = U"คนอ้วน khon uan ראה";
				//                 6^       17^

				RID ctx = ts->create_shaped_text();
				CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
				bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

				const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
				int gl_size = ts->shaped_text_get_glyph_count(ctx);
				CHECK_FALSE_MESSAGE(gl_size == 0, "Shaping failed");
				for (int j = 0; j < gl_size; j++) {
					if (glyphs[j].start < 6) {
						CHECK_FALSE_MESSAGE(glyphs[j].font_rid != font[1], "Incorrect font selected.");
					}
					if ((glyphs[j].start > 6) && (glyphs[j].start < 16)) {
						CHECK_FALSE_MESSAGE(glyphs[j].font_rid != font[0], "Incorrect font selected.");
					}
					if (glyphs[j].start > 16) {
						CHECK_FALSE_MESSAGE(glyphs[j].font_rid != RID(), "Incorrect font selected.");
						CHECK_FALSE_MESSAGE(glyphs[j].index != test[glyphs[j].start], "Incorrect glyph index.");
					}
					CHECK_FALSE_MESSAGE((glyphs[j].start < 0 || glyphs[j].end > test.length()), "Incorrect glyph range.");
					CHECK_FALSE_MESSAGE(glyphs[j].font_size != 16, "Incorrect glyph font size.");
				}

				ts->free_rid(ctx);

				for (int j = 0; j < font.size(); j++) {
					ts->free_rid(font[j]);
				}
				font.clear();
			}
		}

		SUBCASE("[TextServer] Text layout: BiDi") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_FONT_DYNAMIC) || !ts->has_feature(TextServer::FEATURE_BIDI_LAYOUT)) {
					continue;
				}

				RID font1 = ts->create_font();
				ts->font_set_data_ptr(font1, _font_Inter_Regular, _font_Inter_Regular_size);
				RID font2 = ts->create_font();
				ts->font_set_data_ptr(font2, _font_Vazirmatn_Regular, _font_Vazirmatn_Regular_size);

				Array font = { font1, font2 };
				String test = U"Arabic (اَلْعَرَبِيَّةُ, al-ʿarabiyyah)";
				//                    7^      26^

				RID ctx = ts->create_shaped_text();
				CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
				bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

				const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
				int gl_size = ts->shaped_text_get_glyph_count(ctx);
				CHECK_FALSE_MESSAGE(gl_size == 0, "Shaping failed");
				for (int j = 0; j < gl_size; j++) {
					if (glyphs[j].count > 0) {
						if (glyphs[j].start < 7) {
							CHECK_FALSE_MESSAGE(((glyphs[j].flags & TextServer::GRAPHEME_IS_RTL) == TextServer::GRAPHEME_IS_RTL), "Incorrect direction.");
						}
						if ((glyphs[j].start > 8) && (glyphs[j].start < 23)) {
							CHECK_FALSE_MESSAGE(((glyphs[j].flags & TextServer::GRAPHEME_IS_RTL) != TextServer::GRAPHEME_IS_RTL), "Incorrect direction.");
						}
						if (glyphs[j].start > 26) {
							CHECK_FALSE_MESSAGE(((glyphs[j].flags & TextServer::GRAPHEME_IS_RTL) == TextServer::GRAPHEME_IS_RTL), "Incorrect direction.");
						}
					}
				}

				ts->free_rid(ctx);

				for (int j = 0; j < font.size(); j++) {
					ts->free_rid(font[j]);
				}
				font.clear();
			}
		}

		SUBCASE("[TextServer] Text layout: Line break and align points") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_FONT_DYNAMIC) || !ts->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
					continue;
				}

				RID font1 = ts->create_font();
				ts->font_set_data_ptr(font1, _font_Inter_Regular, _font_Inter_Regular_size);
				ts->font_set_allow_system_fallback(font1, false);
				RID font2 = ts->create_font();
				ts->font_set_data_ptr(font2, _font_NotoSansThai_Regular, _font_NotoSansThai_Regular_size);
				ts->font_set_allow_system_fallback(font2, false);
				RID font3 = ts->create_font();
				ts->font_set_data_ptr(font3, _font_Vazirmatn_Regular, _font_Vazirmatn_Regular_size);
				ts->font_set_allow_system_fallback(font3, false);

				Array font = { font1, font2, font3 };
				{
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					ts->shaped_text_add_string(ctx, U"Xtest", font, 10);
					ts->shaped_text_add_string(ctx, U"xs", font, 10);
					RID sctx = ts->shaped_text_substr(ctx, 1, 5);
					CHECK_FALSE_MESSAGE(sctx == RID(), "Creating substring text buffer failed.");
					PackedInt32Array sbrk = ts->shaped_text_get_character_breaks(sctx);
					CHECK_FALSE_MESSAGE(sbrk.size() != 5, "Invalid substring char breaks number.");
					if (sbrk.size() == 5) {
						CHECK_FALSE_MESSAGE(sbrk[0] != 2, "Invalid substring char break position.");
						CHECK_FALSE_MESSAGE(sbrk[1] != 3, "Invalid substring char break position.");
						CHECK_FALSE_MESSAGE(sbrk[2] != 4, "Invalid substring char break position.");
						CHECK_FALSE_MESSAGE(sbrk[3] != 5, "Invalid substring char break position.");
						CHECK_FALSE_MESSAGE(sbrk[4] != 6, "Invalid substring char break position.");
					}
					PackedInt32Array fbrk = ts->shaped_text_get_character_breaks(ctx);
					CHECK_FALSE_MESSAGE(fbrk.size() != 7, "Invalid char breaks number.");
					if (fbrk.size() == 7) {
						CHECK_FALSE_MESSAGE(fbrk[0] != 1, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[1] != 2, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[2] != 3, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[3] != 4, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[4] != 5, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[5] != 6, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[6] != 7, "Invalid char break position.");
					}
					PackedInt32Array rbrk = ts->string_get_character_breaks(U"Xtestxs");
					CHECK_FALSE_MESSAGE(rbrk.size() != 7, "Invalid char breaks number.");
					if (rbrk.size() == 7) {
						CHECK_FALSE_MESSAGE(rbrk[0] != 1, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[1] != 2, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[2] != 3, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[3] != 4, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[4] != 5, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[5] != 6, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[6] != 7, "Invalid char break position.");
					}

					ts->free_rid(sctx);
					ts->free_rid(ctx);
				}

				if (ts->has_feature(TextServer::FEATURE_BREAK_ITERATORS)) {
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					ts->shaped_text_add_string(ctx, U"X❤️‍🔥", font, 10);
					ts->shaped_text_add_string(ctx, U"xs", font, 10);
					RID sctx = ts->shaped_text_substr(ctx, 1, 5);
					CHECK_FALSE_MESSAGE(sctx == RID(), "Creating substring text buffer failed.");
					PackedInt32Array sbrk = ts->shaped_text_get_character_breaks(sctx);
					CHECK_FALSE_MESSAGE(sbrk.size() != 2, "Invalid substring char breaks number.");
					if (sbrk.size() == 2) {
						CHECK_FALSE_MESSAGE(sbrk[0] != 5, "Invalid substring char break position.");
						CHECK_FALSE_MESSAGE(sbrk[1] != 6, "Invalid substring char break position.");
					}
					PackedInt32Array fbrk = ts->shaped_text_get_character_breaks(ctx);
					CHECK_FALSE_MESSAGE(fbrk.size() != 4, "Invalid char breaks number.");
					if (fbrk.size() == 4) {
						CHECK_FALSE_MESSAGE(fbrk[0] != 1, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[1] != 5, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[2] != 6, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(fbrk[3] != 7, "Invalid char break position.");
					}
					PackedInt32Array rbrk = ts->string_get_character_breaks(U"X❤️‍🔥xs");
					CHECK_FALSE_MESSAGE(rbrk.size() != 4, "Invalid char breaks number.");
					if (rbrk.size() == 4) {
						CHECK_FALSE_MESSAGE(rbrk[0] != 1, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[1] != 5, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[2] != 6, "Invalid char break position.");
						CHECK_FALSE_MESSAGE(rbrk[3] != 7, "Invalid char break position.");
					}

					ts->free_rid(sctx);
					ts->free_rid(ctx);
				}

				{
					String test = U"Test test long text long text\n";
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
					ts->shaped_text_update_breaks(ctx);
					ts->shaped_text_update_justification_ops(ctx);

					const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
					int gl_size = ts->shaped_text_get_glyph_count(ctx);

					CHECK_FALSE_MESSAGE(gl_size != 30, "Invalid glyph count.");
					for (int j = 0; j < gl_size; j++) {
						bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
						bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
						bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
						bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
						bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
						if (j == 4 || j == 9 || j == 14 || j == 19 || j == 24) {
							CHECK_FALSE_MESSAGE((!soft || !space || hard || virt || elo), "Invalid glyph flags.");
						} else if (j == 29) {
							CHECK_FALSE_MESSAGE((soft || !space || !hard || virt || elo), "Invalid glyph flags.");
						} else {
							CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
						}
					}
					ts->free_rid(ctx);
				}

				{
					String test = U"الحمـد";
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
					ts->shaped_text_update_breaks(ctx);

					const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
					int gl_size = ts->shaped_text_get_glyph_count(ctx);
					CHECK_FALSE_MESSAGE(gl_size != 6, "Invalid glyph count.");
					for (int j = 0; j < gl_size; j++) {
						bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
						bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
						bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
						bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
						bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
						CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
					}
					if (ts->has_feature(TextServer::FEATURE_KASHIDA_JUSTIFICATION)) {
						ts->shaped_text_update_justification_ops(ctx);

						glyphs = ts->shaped_text_get_glyphs(ctx);
						gl_size = ts->shaped_text_get_glyph_count(ctx);

						CHECK_FALSE_MESSAGE(gl_size != 6, "Invalid glyph count.");
						for (int j = 0; j < gl_size; j++) {
							bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
							bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
							bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
							bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
							bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
							if (j == 1) {
								CHECK_FALSE_MESSAGE((soft || space || hard || virt || !elo), "Invalid glyph flags.");
							} else {
								CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
							}
						}
					}
					ts->free_rid(ctx);
				}

				{
					String test = U"الحمد";
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
					ts->shaped_text_update_breaks(ctx);

					const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
					int gl_size = ts->shaped_text_get_glyph_count(ctx);
					CHECK_FALSE_MESSAGE(gl_size != 5, "Invalid glyph count.");
					for (int j = 0; j < gl_size; j++) {
						bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
						bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
						bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
						bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
						bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
						CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
					}

					if (ts->has_feature(TextServer::FEATURE_KASHIDA_JUSTIFICATION)) {
						ts->shaped_text_update_justification_ops(ctx);

						glyphs = ts->shaped_text_get_glyphs(ctx);
						gl_size = ts->shaped_text_get_glyph_count(ctx);

						CHECK_FALSE_MESSAGE(gl_size != 6, "Invalid glyph count.");
						for (int j = 0; j < gl_size; j++) {
							bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
							bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
							bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
							bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
							bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
							if (j == 1) {
								CHECK_FALSE_MESSAGE((soft || space || hard || !virt || !elo), "Invalid glyph flags.");
							} else {
								CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
							}
						}
					}
					ts->free_rid(ctx);
				}

				{
					String test = U"الحمـد الرياضي العربي";
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
					ts->shaped_text_update_breaks(ctx);

					const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
					int gl_size = ts->shaped_text_get_glyph_count(ctx);

					CHECK_FALSE_MESSAGE(gl_size != 21, "Invalid glyph count.");
					for (int j = 0; j < gl_size; j++) {
						bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
						bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
						bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
						bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
						bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
						if (j == 6 || j == 14) {
							CHECK_FALSE_MESSAGE((!soft || !space || hard || virt || elo), "Invalid glyph flags.");
						} else {
							CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
						}
					}

					if (ts->has_feature(TextServer::FEATURE_KASHIDA_JUSTIFICATION)) {
						ts->shaped_text_update_justification_ops(ctx);

						glyphs = ts->shaped_text_get_glyphs(ctx);
						gl_size = ts->shaped_text_get_glyph_count(ctx);

						CHECK_FALSE_MESSAGE(gl_size != 23, "Invalid glyph count.");
						for (int j = 0; j < gl_size; j++) {
							bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
							bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
							bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
							bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
							bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
							if (j == 7 || j == 16) {
								CHECK_FALSE_MESSAGE((!soft || !space || hard || virt || elo), "Invalid glyph flags.");
							} else if (j == 3 || j == 9) {
								CHECK_FALSE_MESSAGE((soft || space || hard || !virt || !elo), "Invalid glyph flags.");
							} else if (j == 18) {
								CHECK_FALSE_MESSAGE((soft || space || hard || virt || !elo), "Invalid glyph flags.");
							} else {
								CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
							}
						}
					}

					ts->free_rid(ctx);
				}

				{
					String test = U"เป็น ภาษา ราชการ และ ภาษา";
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
					ts->shaped_text_update_breaks(ctx);
					ts->shaped_text_update_justification_ops(ctx);

					const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
					int gl_size = ts->shaped_text_get_glyph_count(ctx);

					CHECK_FALSE_MESSAGE(gl_size != 25, "Invalid glyph count.");
					for (int j = 0; j < gl_size; j++) {
						bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
						bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
						bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
						bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
						bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
						if (j == 4 || j == 9 || j == 16 || j == 20) {
							CHECK_FALSE_MESSAGE((!soft || !space || hard || virt || elo), "Invalid glyph flags.");
						} else {
							CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
						}
					}
					ts->free_rid(ctx);
				}

				if (ts->has_feature(TextServer::FEATURE_BREAK_ITERATORS)) { // Line breaking opportunities.
					String test = U"เป็นภาษาราชการและภาษา";
					RID ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					bool ok = ts->shaped_text_add_string(ctx, test, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
					ts->shaped_text_update_breaks(ctx);
					ts->shaped_text_update_justification_ops(ctx);

					const Glyph *glyphs = ts->shaped_text_get_glyphs(ctx);
					int gl_size = ts->shaped_text_get_glyph_count(ctx);

					CHECK_FALSE_MESSAGE(gl_size != 25, "Invalid glyph count.");
					for (int j = 0; j < gl_size; j++) {
						bool hard = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_HARD) == TextServer::GRAPHEME_IS_BREAK_HARD;
						bool soft = (glyphs[j].flags & TextServer::GRAPHEME_IS_BREAK_SOFT) == TextServer::GRAPHEME_IS_BREAK_SOFT;
						bool space = (glyphs[j].flags & TextServer::GRAPHEME_IS_SPACE) == TextServer::GRAPHEME_IS_SPACE;
						bool virt = (glyphs[j].flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL;
						bool elo = (glyphs[j].flags & TextServer::GRAPHEME_IS_ELONGATION) == TextServer::GRAPHEME_IS_ELONGATION;
						if (j == 4 || j == 9 || j == 16 || j == 20) {
							CHECK_FALSE_MESSAGE((!soft || !space || hard || !virt || elo), "Invalid glyph flags.");
						} else {
							CHECK_FALSE_MESSAGE((soft || space || hard || virt || elo), "Invalid glyph flags.");
						}
					}
					ts->free_rid(ctx);
				}

				if (ts->has_feature(TextServer::FEATURE_BREAK_ITERATORS)) { // Break line.
					struct TestCase {
						String text;
						PackedInt32Array breaks;
					};
					TestCase cases[] = {
						{ U"            เมาส์ตัวนี้", { 0, 17, 17, 23 } },
						{ U"              กู้ไฟล์", { 0, 17, 17, 21 } },
						{ U"             ไม่มีคำ", { 0, 18, 18, 20 } },
						{ U"             ไม่มีคำพูด", { 0, 18, 18, 23 } },
						{ U"            ไม่มีคำ", { 0, 17, 17, 19 } },
						{ U"         มีอุปกรณ์\nนี้", { 0, 11, 11, 19, 19, 22 } },
						{ U"الحمدا لحمدا لحمـــد", { 0, 13, 13, 20 } },
						{ U"         الحمد test", { 0, 15, 15, 19 } },
						{ U"الحمـد الرياضي العربي", { 0, 7, 7, 15, 15, 21 } },
						{ U"test \rtest", { 0, 6, 6, 10 } },
						{ U"test\r test", { 0, 5, 5, 10 } },
						{ U"test\r test \r test", { 0, 5, 5, 12, 12, 17 } },
					};
					for (size_t j = 0; j < std_size(cases); j++) {
						RID ctx = ts->create_shaped_text();
						CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
						bool ok = ts->shaped_text_add_string(ctx, cases[j].text, font, 16);
						CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

						PackedInt32Array breaks = ts->shaped_text_get_line_breaks(ctx, 90.0);
						CHECK_FALSE_MESSAGE(breaks != cases[j].breaks, "Invalid break points.");

						breaks = ts->shaped_text_get_line_breaks_adv(ctx, { 90.0 }, 0, false);
						CHECK_FALSE_MESSAGE(breaks != cases[j].breaks, "Invalid break points.");

						ts->free_rid(ctx);
					}
				}

				if (ts->has_feature(TextServer::FEATURE_BREAK_ITERATORS)) { // Break line and trim spaces.
					struct TestCase {
						String text;
						PackedInt32Array breaks;
						uint32_t flags = TextServer::BREAK_NONE;
					};
					TestCase cases[] = {
						{ U"test \rtest", { 0, 4, 6, 10 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES },
						{ U"test \rtest", { 0, 6, 6, 10 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_TRIM_START_EDGE_SPACES },
						{ U"test\r test", { 0, 4, 6, 10 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES },
						{ U"test\r test", { 0, 4, 5, 10 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_TRIM_END_EDGE_SPACES },
						{ U"test\r test \r test", { 0, 4, 6, 10, 13, 17 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES },
						{ U"test\r test \r test", { 0, 5, 6, 12, 13, 17 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_TRIM_START_EDGE_SPACES },
						{ U"test\r test \r test", { 0, 4, 5, 10, 12, 17 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND | TextServer::BREAK_TRIM_END_EDGE_SPACES },
						{ U"test\r test \r test", { 0, 5, 5, 12, 12, 17 }, TextServer::BREAK_MANDATORY | TextServer::BREAK_WORD_BOUND },
					};
					for (size_t j = 0; j < sizeof(cases) / sizeof(TestCase); j++) {
						RID ctx = ts->create_shaped_text();
						CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
						bool ok = ts->shaped_text_add_string(ctx, cases[j].text, font, 16);
						CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

						PackedInt32Array breaks = ts->shaped_text_get_line_breaks(ctx, 90.0, 0, cases[j].flags);
						CHECK_FALSE_MESSAGE(breaks != cases[j].breaks, "Invalid break points.");

						breaks = ts->shaped_text_get_line_breaks_adv(ctx, { 90.0 }, 0, false, cases[j].flags);
						CHECK_FALSE_MESSAGE(breaks != cases[j].breaks, "Invalid break points.");

						ts->free_rid(ctx);
					}
				}

				for (int j = 0; j < font.size(); j++) {
					ts->free_rid(font[j]);
				}
				font.clear();
			}
		}

		SUBCASE("[TextServer] Text layout: Line breaking") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_FONT_DYNAMIC) || !ts->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
					continue;
				}

				String test_1 = U"test test test";
				//                   5^  10^

				RID font1 = ts->create_font();
				ts->font_set_data_ptr(font1, _font_Inter_Regular, _font_Inter_Regular_size);
				RID font2 = ts->create_font();
				ts->font_set_data_ptr(font2, _font_NotoSansThai_Regular, _font_NotoSansThai_Regular_size);

				Array font = { font1, font2 };
				RID ctx = ts->create_shaped_text();
				CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
				bool ok = ts->shaped_text_add_string(ctx, test_1, font, 16);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

				PackedInt32Array brks = ts->shaped_text_get_line_breaks(ctx, 1);
				CHECK_FALSE_MESSAGE(brks.size() != 6, "Invalid line breaks number.");
				if (brks.size() == 6) {
					CHECK_FALSE_MESSAGE(brks[0] != 0, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[1] != 5, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[2] != 5, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[3] != 10, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[4] != 10, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[5] != 14, "Invalid line break position.");
				}

				brks = ts->shaped_text_get_line_breaks(ctx, 35.0, 0, TextServer::BREAK_WORD_BOUND | TextServer::BREAK_MANDATORY | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES);
				CHECK_FALSE_MESSAGE(brks.size() != 6, "Invalid line breaks number.");
				if (brks.size() == 6) {
					CHECK_FALSE_MESSAGE(brks[0] != 0, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[1] != 4, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[2] != 5, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[3] != 9, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[4] != 10, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[5] != 14, "Invalid line break position.");
				}

				ts->free_rid(ctx);

				String test_2 = U"Word Wrap";
				//                   5^

				ctx = ts->create_shaped_text();
				CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
				ok = ts->shaped_text_add_string(ctx, test_2, font, 16);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

				brks = ts->shaped_text_get_line_breaks(ctx, 43);
				CHECK_FALSE_MESSAGE(brks.size() != 4, "Invalid line breaks number.");
				if (brks.size() == 4) {
					CHECK_FALSE_MESSAGE(brks[0] != 0, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[1] != 5, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[2] != 5, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[3] != 9, "Invalid line break position.");
				}

				brks = ts->shaped_text_get_line_breaks(ctx, 43.0, 0, TextServer::BREAK_WORD_BOUND | TextServer::BREAK_MANDATORY | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES);
				CHECK_FALSE_MESSAGE(brks.size() != 4, "Invalid line breaks number.");
				if (brks.size() == 4) {
					CHECK_FALSE_MESSAGE(brks[0] != 0, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[1] != 4, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[2] != 5, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[3] != 9, "Invalid line break position.");
				}

				brks = ts->shaped_text_get_line_breaks(ctx, 43.0, 0, TextServer::BREAK_WORD_BOUND | TextServer::BREAK_ADAPTIVE | TextServer::BREAK_MANDATORY | TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES);
				CHECK_FALSE_MESSAGE(brks.size() != 4, "Invalid line breaks number.");
				if (brks.size() == 4) {
					CHECK_FALSE_MESSAGE(brks[0] != 0, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[1] != 4, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[2] != 5, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[3] != 9, "Invalid line break position.");
				}

				brks = ts->shaped_text_get_line_breaks(ctx, 43.0, 0, TextServer::BREAK_WORD_BOUND | TextServer::BREAK_ADAPTIVE | TextServer::BREAK_MANDATORY);
				CHECK_FALSE_MESSAGE(brks.size() != 6, "Invalid line breaks number.");
				if (brks.size() == 6) {
					CHECK_FALSE_MESSAGE(brks[0] != 0, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[1] != 4, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[2] != 4, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[3] != 5, "Invalid line break position.");

					CHECK_FALSE_MESSAGE(brks[4] != 5, "Invalid line break position.");
					CHECK_FALSE_MESSAGE(brks[5] != 9, "Invalid line break position.");
				}

				ts->free_rid(ctx);

				for (int j = 0; j < font.size(); j++) {
					ts->free_rid(font[j]);
				}
				font.clear();
			}
		}

		SUBCASE("[TextServer] Text layout: Justification") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_FONT_DYNAMIC) || !ts->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
					continue;
				}

				RID font1 = ts->create_font();
				ts->font_set_data_ptr(font1, _font_Inter_Regular, _font_Inter_Regular_size);
				RID font2 = ts->create_font();
				ts->font_set_data_ptr(font2, _font_Vazirmatn_Regular, _font_Vazirmatn_Regular_size);

				Array font = { font1, font2 };
				String test_1 = U"الحمد";
				String test_2 = U"الحمد test";
				String test_3 = U"test test";
				//                    7^      26^

				RID ctx;
				bool ok;
				float width_old, width;
				if (ts->has_feature(TextServer::FEATURE_KASHIDA_JUSTIFICATION)) {
					ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					ok = ts->shaped_text_add_string(ctx, test_1, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

					width_old = ts->shaped_text_get_width(ctx);
					width = ts->shaped_text_fit_to_width(ctx, 100, TextServer::JUSTIFICATION_WORD_BOUND);
					CHECK_FALSE_MESSAGE((width != width_old), "Invalid fill width.");
					width = ts->shaped_text_fit_to_width(ctx, 100, TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA);
					CHECK_FALSE_MESSAGE((width <= width_old || width > 100), "Invalid fill width.");

					ts->free_rid(ctx);

					ctx = ts->create_shaped_text();
					CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
					ok = ts->shaped_text_add_string(ctx, test_2, font, 16);
					CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

					width_old = ts->shaped_text_get_width(ctx);
					width = ts->shaped_text_fit_to_width(ctx, 100, TextServer::JUSTIFICATION_WORD_BOUND);
					CHECK_FALSE_MESSAGE((width <= width_old || width > 100), "Invalid fill width.");
					width = ts->shaped_text_fit_to_width(ctx, 100, TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA);
					CHECK_FALSE_MESSAGE((width <= width_old || width > 100), "Invalid fill width.");

					ts->free_rid(ctx);
				}

				ctx = ts->create_shaped_text();
				CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
				ok = ts->shaped_text_add_string(ctx, test_3, font, 16);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");

				width_old = ts->shaped_text_get_width(ctx);
				width = ts->shaped_text_fit_to_width(ctx, 100, TextServer::JUSTIFICATION_WORD_BOUND);
				CHECK_FALSE_MESSAGE((width <= width_old || width > 100), "Invalid fill width.");

				ts->free_rid(ctx);

				for (int j = 0; j < font.size(); j++) {
					ts->free_rid(font[j]);
				}
				font.clear();
			}
		}

		SUBCASE("[TextServer] Unicode identifiers") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				static const char32_t *data[19] = { U"-30", U"100", U"10.1", U"10,1", U"1e2", U"1e-2", U"1e2e3", U"0xAB", U"AB", U"Test1", U"1Test", U"Test*1", U"test_testeT", U"test_tes teT", U"عَلَيْكُمْ", U"عَلَيْكُمْTest", U"ӒӖӚӜ", U"_test", U"ÂÃÄÅĀĂĄÇĆĈĊ" };
				static bool isid[19] = { false, false, false, false, false, false, false, false, true, true, false, false, true, false, true, true, true, true, true };
				for (int j = 0; j < 19; j++) {
					String s = String(data[j]);
					CHECK(ts->is_valid_identifier(s) == isid[j]);
				}

				if (ts->has_feature(TextServer::FEATURE_UNICODE_IDENTIFIERS)) {
					// Test UAX 3.2 ZW(N)J usage.
					CHECK(ts->is_valid_identifier(U"\u0646\u0627\u0645\u0647\u200C\u0627\u06CC"));
					CHECK(ts->is_valid_identifier(U"\u0D26\u0D43\u0D15\u0D4D\u200C\u0D38\u0D3E\u0D15\u0D4D\u0D37\u0D3F"));
					CHECK(ts->is_valid_identifier(U"\u0DC1\u0DCA\u200D\u0DBB\u0DD3"));
				}
			}
		}

		SUBCASE("[TextServer] Unicode letters") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				struct ul_testcase {
					int fail_index = -1; // Expecting failure at given index.
					char32_t text[10]; // Using 0 as the terminator.
				};
				ul_testcase cases[14] = {
					{
							0,
							{ 0x2D, 0x33, 0x30, 0, 0, 0, 0, 0, 0, 0 }, // "-30"
					},
					{
							1,
							{ 0x61, 0x2E, 0x31, 0, 0, 0, 0, 0, 0, 0 }, // "a.1"
					},
					{
							1,
							{ 0x61, 0x2C, 0x31, 0, 0, 0, 0, 0, 0, 0 }, // "a,1"
					},
					{
							0,
							{ 0x31, 0x65, 0x2D, 0x32, 0, 0, 0, 0, 0, 0 }, // "1e-2"
					},
					{
							0,
							{ 0xAB, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // "Left-Pointing Double Angle Quotation Mark"
					},
					{
							-1,
							{ 0x41, 0x42, 0, 0, 0, 0, 0, 0, 0, 0 }, // "AB"
					},
					{
							4,
							{ 0x54, 0x65, 0x73, 0x74, 0x31, 0, 0, 0, 0, 0 }, // "Test1"
					},
					{
							2,
							{ 0x54, 0x65, 0x2A, 0x73, 0x74, 0, 0, 0, 0, 0 }, // "Te*st"
					},
					{
							4,
							{ 0x74, 0x65, 0x73, 0x74, 0x5F, 0x74, 0x65, 0x73, 0x74, 0x65 }, // "test_teste"
					},
					{
							4,
							{ 0x74, 0x65, 0x73, 0x74, 0x20, 0x74, 0x65, 0x73, 0x74, 0 }, // "test test"
					},
					{
							-1,
							{ 0x643, 0x402, 0x716, 0xB05, 0, 0, 0, 0, 0, 0 }, // "كЂܖଅ" (arabic letters),
					},
					{
							-1,
							{ 0x643, 0x402, 0x716, 0xB05, 0x54, 0x65, 0x73, 0x74, 0x30AA, 0x4E21 }, // 0-3 arabic letters, 4-7 latin letters, 8-9 CJK letters
					},
					{
							-1,
							{ 0x4D2, 0x4D6, 0x4DA, 0x4DC, 0, 0, 0, 0, 0, 0 }, // "ӒӖӚӜ" cyrillic letters
					},
					{
							-1,
							{ 0xC2, 0xC3, 0xC4, 0xC5, 0x100, 0x102, 0x104, 0xC7, 0x106, 0x108 }, // "ÂÃÄÅĀĂĄÇĆĈ" rarer latin letters
					},
				};

				for (int j = 0; j < 14; j++) {
					ul_testcase test = cases[j];
					int failed_on_index = -1;
					for (int k = 0; k < 10; k++) {
						char32_t character = test.text[k];
						if (character == 0) {
							break;
						}
						if (!ts->is_valid_letter(character)) {
							failed_on_index = k;
							break;
						}
					}

					if (test.fail_index == -1) {
						CHECK_MESSAGE(test.fail_index == failed_on_index, "In interface ", ts->get_name() + ": In test case ", j, ", the character at index ", failed_on_index, " should have been a letter.");
					} else {
						CHECK_MESSAGE(test.fail_index == failed_on_index, "In interface ", ts->get_name() + ": In test case ", j, ", expected first non-letter at index ", test.fail_index, ", but found at index ", failed_on_index);
					}
				}
			}
		}

		SUBCASE("[TextServer] Strip Diacritics") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (ts->has_feature(TextServer::FEATURE_SHAPING)) {
					CHECK(ts->strip_diacritics(U"ٱلسَّلَامُ عَلَيْكُمْ") == U"ٱلسلام عليكم");
				}

				CHECK(ts->strip_diacritics(U"pêches épinards tomates fraises") == U"peches epinards tomates fraises");
				CHECK(ts->strip_diacritics(U"ΆΈΉΊΌΎΏΪΫϓϔ") == U"ΑΕΗΙΟΥΩΙΥΥΥ");
				CHECK(ts->strip_diacritics(U"άέήίΐϊΰϋόύώ") == U"αεηιιιυυουω");
				CHECK(ts->strip_diacritics(U"ЀЁЃ ЇЌЍӢӤЙ ЎӮӰӲ ӐӒӖӚӜӞ ӦӪ Ӭ Ӵ Ӹ") == U"ЕЕГ ІКИИИИ УУУУ ААЕӘЖЗ ОӨ Э Ч Ы");
				CHECK(ts->strip_diacritics(U"ѐёѓ їќѝӣӥй ўӯӱӳ ӑӓӗӛӝӟ ӧӫ ӭ ӵ ӹ") == U"еег ікииии уууу ааеәжз оө э ч ы");
				CHECK(ts->strip_diacritics(U"ÀÁÂÃÄÅĀĂĄÇĆĈĊČĎÈÉÊËĒĔĖĘĚĜĞĠĢĤÌÍÎÏĨĪĬĮİĴĶĹĻĽÑŃŅŇŊÒÓÔÕÖØŌŎŐƠŔŖŘŚŜŞŠŢŤÙÚÛÜŨŪŬŮŰŲƯŴÝŶŹŻŽ") == U"AAAAAAAAACCCCCDEEEEEEEEEGGGGHIIIIIIIIIJKLLLNNNNŊOOOOOØOOOORRRSSSSTTUUUUUUUUUUUWYYZZZ");
				CHECK(ts->strip_diacritics(U"àáâãäåāăąçćĉċčďèéêëēĕėęěĝğġģĥìíîïĩīĭįĵķĺļľñńņňŋòóôõöøōŏőơŕŗřśŝşšţťùúûüũūŭůűųưŵýÿŷźżž") == U"aaaaaaaaacccccdeeeeeeeeegggghiiiiiiiijklllnnnnŋoooooøoooorrrssssttuuuuuuuuuuuwyyyzzz");
				CHECK(ts->strip_diacritics(U"ǍǏȈǑǪǬȌȎȪȬȮȰǓǕǗǙǛȔȖǞǠǺȀȂȦǢǼǦǴǨǸȆȐȒȘȚȞȨ Ḁ ḂḄḆ Ḉ ḊḌḎḐḒ ḔḖḘḚḜ Ḟ Ḡ ḢḤḦḨḪ ḬḮ ḰḲḴ ḶḸḺḼ ḾṀṂ ṄṆṈṊ ṌṎṐṒ ṔṖ ṘṚṜṞ ṠṢṤṦṨ ṪṬṮṰ ṲṴṶṸṺ") == U"AIIOOOOOOOOOUUUUUUUAAAAAAÆÆGGKNERRSTHE A BBB C DDDDD EEEEE F G HHHHH II KKK LLLL MMM NNNN OOOO PP RRRR SSSSS TTTT UUUUU");
				CHECK(ts->strip_diacritics(U"ǎǐȉȋǒǫǭȍȏȫȭȯȱǔǖǘǚǜȕȗǟǡǻȁȃȧǣǽǧǵǩǹȇȑȓșțȟȩ ḁ ḃḅḇ ḉ ḋḍḏḑḓ ḟ ḡ ḭḯ ḱḳḵ ḷḹḻḽ ḿṁṃ ṅṇṉṋ ṍṏṑṓ ṗṕ ṙṛṝṟ ṡṣṥṧṩ ṫṭṯṱ ṳṵṷṹṻ") == U"aiiiooooooooouuuuuuuaaaaaaææggknerrsthe a bbb c ddddd f g ii kkk llll mmm nnnn oooo pp rrrr sssss tttt uuuuu");
				CHECK(ts->strip_diacritics(U"ṼṾ ẀẂẄẆẈ ẊẌ Ẏ ẐẒẔ") == U"VV WWWWW XX Y ZZZ");
				CHECK(ts->strip_diacritics(U"ṽṿ ẁẃẅẇẉ ẋẍ ẏ ẑẓẕ ẖ ẗẘẙẛ") == U"vv wwwww xx y zzz h twys");
			}
		}

		SUBCASE("[TextServer] Word break") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
					continue;
				}

				{
					String text1 = U"linguistically similar and effectively form";
					//                           14^     22^ 26^         38^
					PackedInt32Array breaks = ts->string_get_word_breaks(text1, "en");
					CHECK(breaks.size() == 10);
					if (breaks.size() == 10) {
						CHECK(breaks[0] == 0);
						CHECK(breaks[1] == 14);
						CHECK(breaks[2] == 15);
						CHECK(breaks[3] == 22);
						CHECK(breaks[4] == 23);
						CHECK(breaks[5] == 26);
						CHECK(breaks[6] == 27);
						CHECK(breaks[7] == 38);
						CHECK(breaks[8] == 39);
						CHECK(breaks[9] == 43);
					}
				}

				if (ts->has_feature(TextServer::FEATURE_BREAK_ITERATORS)) {
					String text2 = U"เป็นภาษาราชการและภาษาประจำชาติของประเทศไทย";
					//				 เป็น ภาษา ราชการ และ ภาษา ประจำ ชาติ ของ ประเทศไทย
					//                 3^   7^    13^ 16^  20^   25^ 29^ 32^

					PackedInt32Array breaks = ts->string_get_word_breaks(text2, "th");
					CHECK(breaks.size() == 18);
					if (breaks.size() == 18) {
						CHECK(breaks[0] == 0);
						CHECK(breaks[1] == 4);
						CHECK(breaks[2] == 4);
						CHECK(breaks[3] == 8);
						CHECK(breaks[4] == 8);
						CHECK(breaks[5] == 14);
						CHECK(breaks[6] == 14);
						CHECK(breaks[7] == 17);
						CHECK(breaks[8] == 17);
						CHECK(breaks[9] == 21);
						CHECK(breaks[10] == 21);
						CHECK(breaks[11] == 26);
						CHECK(breaks[12] == 26);
						CHECK(breaks[13] == 30);
						CHECK(breaks[14] == 30);
						CHECK(breaks[15] == 33);
						CHECK(breaks[16] == 33);
						CHECK(breaks[17] == 42);
					}
				}

				if (ts->has_feature(TextServer::FEATURE_BREAK_ITERATORS)) {
					String text2 = U"U+2764 U+FE0F U+200D U+1F525 ; 13.1 # ❤️‍🔥";

					PackedInt32Array breaks = ts->string_get_character_breaks(text2, "en");
					CHECK(breaks.size() == 39);
					if (breaks.size() == 39) {
						CHECK(breaks[0] == 1);
						CHECK(breaks[1] == 2);
						CHECK(breaks[2] == 3);
						CHECK(breaks[3] == 4);
						CHECK(breaks[4] == 5);
						CHECK(breaks[5] == 6);
						CHECK(breaks[6] == 7);
						CHECK(breaks[7] == 8);
						CHECK(breaks[8] == 9);
						CHECK(breaks[9] == 10);
						CHECK(breaks[10] == 11);
						CHECK(breaks[11] == 12);
						CHECK(breaks[12] == 13);
						CHECK(breaks[13] == 14);
						CHECK(breaks[14] == 15);
						CHECK(breaks[15] == 16);
						CHECK(breaks[16] == 17);
						CHECK(breaks[17] == 18);
						CHECK(breaks[18] == 19);
						CHECK(breaks[19] == 20);
						CHECK(breaks[20] == 21);
						CHECK(breaks[21] == 22);
						CHECK(breaks[22] == 23);
						CHECK(breaks[23] == 24);
						CHECK(breaks[24] == 25);
						CHECK(breaks[25] == 26);
						CHECK(breaks[26] == 27);
						CHECK(breaks[27] == 28);
						CHECK(breaks[28] == 29);
						CHECK(breaks[29] == 30);
						CHECK(breaks[30] == 31);
						CHECK(breaks[31] == 32);
						CHECK(breaks[32] == 33);
						CHECK(breaks[33] == 34);
						CHECK(breaks[34] == 35);
						CHECK(breaks[35] == 36);
						CHECK(breaks[36] == 37);
						CHECK(breaks[37] == 38);
						CHECK(breaks[38] == 42);
					}
				}
			}
		}

		SUBCASE("[TextServer] Buffer invalidation") {
			for (int i = 0; i < TextServerManager::get_interface_count(); i++) {
				Ref<TextServer> ts = TextServerManager::get_interface(i);
				CHECK_FALSE_MESSAGE(ts.is_null(), "Invalid TS interface.");

				if (!ts->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
					continue;
				}

				RID font1 = ts->create_font();
				ts->font_set_data_ptr(font1, _font_Inter_Regular, _font_Inter_Regular_size);

				Array font = { font1 };
				RID ctx = ts->create_shaped_text();
				CHECK_FALSE_MESSAGE(ctx == RID(), "Creating text buffer failed.");
				bool ok = ts->shaped_text_add_string(ctx, "T", font, 16);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
				int gl_size = ts->shaped_text_get_glyph_count(ctx);
				CHECK_MESSAGE(gl_size == 1, "Shaping failed, invalid glyph count");

				ok = ts->shaped_text_add_object(ctx, "key", Size2(20, 20), INLINE_ALIGNMENT_CENTER, 1, 0.0);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
				gl_size = ts->shaped_text_get_glyph_count(ctx);
				CHECK_MESSAGE(gl_size == 2, "Shaping failed, invalid glyph count");

				ok = ts->shaped_text_add_string(ctx, "B", font, 16);
				CHECK_FALSE_MESSAGE(!ok, "Adding text to the buffer failed.");
				gl_size = ts->shaped_text_get_glyph_count(ctx);
				CHECK_MESSAGE(gl_size == 3, "Shaping failed, invalid glyph count");

				ts->free_rid(ctx);

				for (int j = 0; j < font.size(); j++) {
					ts->free_rid(font[j]);
				}
				font.clear();
			}
		}
	}
}

} // namespace TestTextServer

#endif // TOOLS_ENABLED

// === Merged from tests/servers/test_triangle2.cpp ===
/*  test_triangle2.cpp                                                    */
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



#include "modules/modules_enabled.gen.h" // For navigation 2D.

#ifdef MODULE_NAVIGATION_2D_ENABLED

#include "scene/main/scene_tree.h"

namespace TestTriangle2 {

TEST_SUITE("[Triangle2]") {
	TEST_CASE("[Triangle2] Test get_area") {
		const Vector2 p0(5.0, 5.0);
		const Vector2 p1(6.0, 7.0);
		const Vector2 p2(7.0, 6.0);

		CHECK_EQ(Triangle2(p0, p1, p2).get_area(), doctest::Approx(1.5));
		CHECK_EQ(Triangle2(p0, p2, p1).get_area(), doctest::Approx(1.5));

		CHECK_EQ(Triangle2(p0, p2, p2).get_area(), doctest::Approx(0.0));
		CHECK_EQ(Triangle2(p0, p1, p1).get_area(), doctest::Approx(0.0));
	}

	TEST_CASE("[Triangle2] Test get_closest_point_to") {
		const Vector2 p0(5.0, 5.0);
		const Vector2 p1(6.0, 7.0);
		const Vector2 p2(7.0, 6.0);

		const Vector2 p3(0.0, 0.0);
		const Vector2 p4(6.0, 6.5);

		const Triangle2 t(p0, p1, p2);

		CHECK(t.get_closest_point_to(p0).is_equal_approx(p0));
		CHECK(t.get_closest_point_to(p1).is_equal_approx(p1));
		CHECK(t.get_closest_point_to(p2).is_equal_approx(p2));

		CHECK(t.get_closest_point_to(p3).is_equal_approx(p0));

		CHECK(t.get_closest_point_to(p4).is_equal_approx(p4));
	}
}

} // namespace TestTriangle2

#endif // MODULE_NAVIGATION_2D_ENABLED

TEST_FORCE_LINK(test_servers)

