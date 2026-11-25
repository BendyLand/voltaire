/**************************************************************************/
/*  test_main.cpp                                                         */
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

#include "servers/display/display_server.h"

#include "core/os/os.h"
#include "core/io/net_socket.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/theme.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "tests/test_macros.h"

#ifdef TOOLS_ENABLED
#include "scene/main/scene_tree.h"
#endif // TOOLS_ENABLED

#ifndef NAVIGATION_2D_DISABLED
#include "scene/main/scene_tree.h"
#endif // NAVIGATION_2D_DISABLED
#ifndef NAVIGATION_3D_DISABLED
#include "scene/main/scene_tree.h"
#endif // NAVIGATION_3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
#include "scene/main/scene_tree.h"
#endif // PHYSICS_2D_DISABLED
#ifndef PHYSICS_3D_DISABLED
#include "scene/main/scene_tree.h"
#endif // PHYSICS_3D_DISABLED


namespace ForceLink {
	void force_link_tests();
	void force_link_test_core();
	void force_link_test_file_access();
	void force_link_test_math();
	void force_link_test_templates();
	void force_link_test_servers();
	void force_link_test_scene_gui();
	void force_link_test_scene_resources();
	void force_link_test_scene_nodes();
	void force_link_test_modules();
} // namespace ForceLink

void ForceLink::force_link_tests() {
	force_link_test_core();
	force_link_test_file_access();
	force_link_test_math();
	force_link_test_templates();
	force_link_test_servers();
	force_link_test_scene_gui();
	force_link_test_scene_resources();
	force_link_test_scene_nodes();
	force_link_test_modules();
}

int test_main(int argc, char *argv[]) {
	ForceLink::force_link_tests();

	bool run_tests = true;

	// Convert arguments to Godot's command-line.
	List<String> args;

	for (int i = 0; i < argc; i++) {
		args.push_back(String::utf8(argv[i]));
	}
	OS::set_cmdline("", args, List<String>());
	DisplayServerMock::register_mock_driver();


	{
		const String test_path = TestUtils::get_temp_path("");
		Ref<DirAccess> da = DirAccess::open(test_path); // get_temp_path() automatically creates the folder.
		ERR_FAIL_COND_V(da.is_null(), 0);
		ERR_FAIL_COND_V_MSG(da->erase_contents_recursive() != OK, 0, "Failed to delete files");
	}

	// Run custom test tools.
	if (test_commands) {
		for (const KeyValue<String, TestFunc> &E : (*test_commands)) {
			if (args.find(E.key)) {
				const TestFunc &test_func = E.value;
				test_func();
				run_tests = false;
				break;
			}
		}
		if (!run_tests) {
			delete test_commands;
			return 0;
		}
	}
	// Doctest runner.
	doctest::Context test_context;
	LocalVector<String> test_args;

	// Clean arguments of "--test" from the args.
	for (int x = 0; x < argc; x++) {
		String arg = String(argv[x]);
		if (arg != "--test") {
			test_args.push_back(arg);
		}
	}

	if (test_args.size() > 0) {
		// Convert Godot command line arguments back to standard arguments.
		char **doctest_args = new char *[test_args.size()];
		for (uint32_t x = 0; x < test_args.size(); x++) {
			// Operation to convert Godot string to non wchar string.
			CharString cs = test_args[x].utf8();
			const char *str = cs.get_data();
			// Allocate the string copy.
			doctest_args[x] = new char[strlen(str) + 1];
			// Copy this into memory.
			memcpy(doctest_args[x], str, strlen(str) + 1);
		}

		test_context.applyCommandLine(test_args.size(), doctest_args);

		for (uint32_t x = 0; x < test_args.size(); x++) {
			delete[] doctest_args[x];
		}
		delete[] doctest_args;
	}

	return test_context.run();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct GodotTestCaseListener : public doctest::IReporter {
	GodotTestCaseListener(const doctest::ContextOptions &p_in) {}

	void test_case_start(const doctest::TestCaseData &p_in) override {
		reinitialize();

		String name = String(p_in.m_name);
		String suite_name = String(p_in.m_test_suite);

		if (name.contains("[SceneTree]") || name.contains("[Editor]")) {
			Input::initialize();
			Input::set_use_accumulated_input(false);

			Error err = OK;
			OS::set_has_server_feature_callback(nullptr);

			for (int i = 0; i < AccessibilityServer::get_create_function_count(); i++) {
				if (String("dummy") == AccessibilityServer::get_create_function_name(i)) {
					AccessibilityServer::create(i, err);
					break;
				}
			}

			for (int i = 0; i < DisplayServer::get_create_function_count(); i++) {
				if (String("mock") == DisplayServer::get_create_function_name(i)) {
					DisplayServer::create(i, "", DisplayServerEnums::WindowMode::WINDOW_MODE_MINIMIZED, DisplayServerEnums::VSyncMode::VSYNC_ENABLED, 0, nullptr, Vector2i(0, 0), DisplayServerEnums::SCREEN_PRIMARY, DisplayServerEnums::CONTEXT_EDITOR, 0, err);
					break;
				}
			}

			// ThemeDB requires Renderer to initialize the default theme.
			// So we have to do this for each test case. Also make sure there is
			// no residual theme from something else.
			ThemeDB::initialize();
			ThemeDB::finalize_theme();
			ThemeDB::initialize_theme();

#ifndef PHYSICS_3D_DISABLED
			PhysicsServer3D::initialize();
#endif // PHYSICS_3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
			PhysicsServer2D::initialize();
#endif // PHYSICS_2D_DISABLED

			ERR_PRINT_OFF;
#ifndef NAVIGATION_3D_DISABLED
			NavigationServer3D::initialize();
#endif // NAVIGATION_3D_DISABLED
#ifndef NAVIGATION_2D_DISABLED
			NavigationServer2D::initialize();
#endif // NAVIGATION_2D_DISABLED
			ERR_PRINT_ON;

			InputMap::initialize();
			InputMap::load_default();

			SceneTree::initialize();
			SceneTree::initialize_tree();
			if (!DisplayServer::has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS)) {
				SceneTree::get_root()->set_embedding_subwindows(true);
			}

#ifdef TOOLS_ENABLED
			if (name.contains("[Editor]")) {
				Engine::set_editor_hint(true);
				EditorPaths::initialize();
				EditorSettings::initialize();
			}
#endif // TOOLS_ENABLED

			return;
		}

		if (name.contains("[Audio]")) {
			// The last driver index should always be the dummy driver.
			int dummy_idx = AudioDriverManager::get_driver_count() - 1;
			AudioDriverManager::initialize(dummy_idx);
			AudioServer::initialize();
			AudioServer::init();
			return;
		}

#ifndef NAVIGATION_3D_DISABLED
		if (suite_name.contains("[Navigation3D]") && navigation_server_3d == nullptr) {
			ERR_PRINT_OFF;
			NavigationServer3D::initialize();
			ERR_PRINT_ON;
			return;
		}
#endif // NAVIGATION_3D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
		if (suite_name.contains("[Navigation2D]") && navigation_server_2d == nullptr) {
			ERR_PRINT_OFF;
			NavigationServer2D::initialize();
			ERR_PRINT_ON;
			return;
		}
#endif // NAVIGATION_2D_DISABLED
	}

	void test_case_end(const doctest::CurrentTestCaseStats &) override {
#ifdef TOOLS_ENABLED
		if (EditorSettings::is_initialized()) {
			EditorSettings::finalize();

			// Instantiating the EditorSettings singleton sets the locale to the editor's language.
			TranslationServer::set_locale("en");
		}
		if (EditorPaths::is_initialized()) {
			EditorPaths::finalize();
		}
#endif // TOOLS_ENABLED

		Engine::set_editor_hint(false);

		if (SceneTree::is_initialized()) {
			SceneTree::finalize_tree();
		}

		SceneTree::finalize();

#ifndef NAVIGATION_3D_DISABLED
		NavigationServer3D::finalize();
#endif // NAVIGATION_3D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
		NavigationServer2D::finalize();
#endif // NAVIGATION_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
		if (PhysicsServer3D::is_initialized()) {
			PhysicsServer3D::finish();
			PhysicsServer3D::finalize();
		}
#endif // PHYSICS_3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
		if (PhysicsServer2D::is_initialized()) {
			PhysicsServer2D::finish();
			PhysicsServer2D::finalize();
		}
#endif // PHYSICS_2D_DISABLED

		Input::finalize();

		if (Renderer::data) {
			ThemeDB::finalize_theme();
		}
		ThemeDB::finalize();

		DisplayServer::finalize();
		AccessibilityServer::finalize();
		InputMap::finalize();

		if (AudioServer::is_initialized()) {
			AudioServer::finish();
			AudioServer::finalize();
		}
	}

	void test_run_start() override {
		SignalWatcher::initialize();
	}

	void test_run_end(const doctest::TestRunStats &) override {
		SignalWatcher::finalize();
	}

	void test_case_reenter(const doctest::TestCaseData &) override {
		reinitialize();
	}

	void subcase_start(const doctest::SubcaseSignature &) override {
		reinitialize();
	}

	void report_query(const doctest::QueryData &) override {}
	void test_case_exception(const doctest::TestCaseException &) override {}
	void subcase_end() override {}

	void log_assert(const doctest::AssertData &in) override {}
	void log_message(const doctest::MessageData &) override {}
	void test_case_skipped(const doctest::TestCaseData &) override {}

private:
	void reinitialize() {
		Math::seed(0x60d07);
		SignalWatcher::_clear_signals();
	}
};

REGISTER_LISTENER("GodotTestCaseListener", 1, GodotTestCaseListener);

// === Merged from tests/display_server_mock.cpp ===
/*  display_server_mock.cpp                                               */
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
#include "servers/rendering/renderer.h"

Vector<String> DisplayServerMock::get_rendering_drivers_func() {
	Vector<String> drivers;
	drivers.push_back("dummy");
	return drivers;
}

void DisplayServerMock::_set_mouse_position(const Point2i &p_position) {
	if (mouse_position == p_position) {
		return;
	}
	mouse_position = p_position;
	_set_window_over(Rect2i(Point2i(0, 0), window_get_size()).has_point(p_position));
}
void DisplayServerMock::_set_window_over(bool p_over) {
	if (p_over == window_over) {
		return;
	}
	window_over = p_over;
	_send_window_event(p_over ? DisplayServerEnums::WINDOW_EVENT_MOUSE_ENTER : DisplayServerEnums::WINDOW_EVENT_MOUSE_EXIT);
}
void DisplayServerMock::_send_window_event(DisplayServerEnums::WindowEvent p_event) {
	if (event_callback.is_valid()) {
		Variant event = int(p_event);
		event_callback.call(event);
	}
}

bool DisplayServerMock::has_feature(DisplayServerEnums::Feature p_feature) const {
	switch (p_feature) {
		case DisplayServerEnums::FEATURE_MOUSE:
		case DisplayServerEnums::FEATURE_CURSOR_SHAPE:
		case DisplayServerEnums::FEATURE_CLIPBOARD:
		case DisplayServerEnums::FEATURE_CLIPBOARD_PRIMARY:
			return true;
		default: {
		}
	}
	return false;
}

DisplayServerDriver *DisplayServerMock::create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error) {
	r_error = OK;
	RasterizerDummy::make_current();
	return memnew(DisplayServerMock());
}

void DisplayServerMock::simulate_event(Ref<InputEvent> p_event) {
	Ref<InputEvent> event = p_event;
	Ref<InputEventMouse> me = p_event;
	if (me.is_valid()) {
		Ref<InputEventMouseMotion> mm = p_event;
		if (mm.is_valid()) {
			mm->set_relative(mm->get_position() - mouse_position);
			event = mm;
		}
		_set_mouse_position(me->get_position());
	}
	Input::parse_input_event(event);
}

// === Merged from tests/signal_watcher.cpp ===
/*  signal_watcher.cpp                                                    */
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

void SignalWatcher::_add_signal_entry(const Array &p_args, const String &p_name) {
	ERR_FAIL_NULL(data);
	if (!data->_signals.has(p_name)) {
		data->_signals[p_name] = Array();
	}
	data->_signals[p_name].push_back(p_args);
}

void SignalWatcher::_signal_callback_zero(const String &p_name) {
	Array args;
	_add_signal_entry(args, p_name);
}

void SignalWatcher::_signal_callback_one(Variant p_arg1, const String &p_name) {
	Array args = { p_arg1 };
	_add_signal_entry(args, p_name);
}

void SignalWatcher::_signal_callback_two(Variant p_arg1, Variant p_arg2, const String &p_name) {
	Array args = { p_arg1, p_arg2 };
	_add_signal_entry(args, p_name);
}

void SignalWatcher::_signal_callback_three(Variant p_arg1, Variant p_arg2, Variant p_arg3, const String &p_name) {
	Array args = { p_arg1, p_arg2, p_arg3 };
	_add_signal_entry(args, p_name);
}

void SignalWatcher::watch_signal(Object *p_object, const String &p_signal) {
	MethodInfo method_info;
	ClassDB::get_signal(p_object->get_class(), p_signal, &method_info);
	switch (method_info.arguments.size()) {
		case 0: {
			p_object->connect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_zero).bind(p_signal));
		} break;
		case 1: {
			p_object->connect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_one).bind(p_signal));
		} break;
		case 2: {
			p_object->connect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_two).bind(p_signal));
		} break;
		case 3: {
			p_object->connect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_three).bind(p_signal));
		} break;
		default: {
			MESSAGE("Signal ", p_signal, " arg count not supported.");
		} break;
	}
}

void SignalWatcher::unwatch_signal(Object *p_object, const String &p_signal) {
	MethodInfo method_info;
	ClassDB::get_signal(p_object->get_class(), p_signal, &method_info);
	switch (method_info.arguments.size()) {
		case 0: {
			p_object->disconnect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_zero));
		} break;
		case 1: {
			p_object->disconnect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_one));
		} break;
		case 2: {
			p_object->disconnect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_two));
		} break;
		case 3: {
			p_object->disconnect(p_signal, callable_mp_static(&SignalWatcher::_signal_callback_three));
		} break;
		default: {
			MESSAGE("Signal ", p_signal, " arg count not supported.");
		} break;
	}
}

bool SignalWatcher::check(const String &p_name, const Array &p_args) {
	ERR_FAIL_NULL_V(data, false);
	if (!data->_signals.has(p_name)) {
		MESSAGE("Signal ", p_name, " not emitted");
		return false;
	}

	if (p_args.size() != data->_signals[p_name].size()) {
		MESSAGE("Signal has " << data->_signals[p_name] << " expected " << p_args);
		discard_signal(p_name);
		return false;
	}

	bool match = true;
	for (int i = 0; i < p_args.size(); i++) {
		if (((Array)p_args[i]).size() != ((Array)data->_signals[p_name][i]).size()) {
			MESSAGE("Signal has " << data->_signals[p_name][i] << " expected " << p_args[i]);
			match = false;
			continue;
		}

		for (int j = 0; j < ((Array)p_args[i]).size(); j++) {
			if (((Array)p_args[i])[j] != ((Array)data->_signals[p_name][i])[j]) {
				MESSAGE("Signal has " << data->_signals[p_name][i] << " expected " << p_args[i]);
				match = false;
				break;
			}
		}
	}

	discard_signal(p_name);
	return match;
}

bool SignalWatcher::check_false(const String &p_name) {
	ERR_FAIL_NULL_V(data, false);
	bool has = data->_signals.has(p_name);
	if (has) {
		MESSAGE("Signal has " << data->_signals[p_name] << " expected none.");
	}
	discard_signal(p_name);
	return !has;
}

void SignalWatcher::discard_signal(const String &p_name) {
	if (data && data->_signals.has(p_name)) {
		data->_signals.erase(p_name);
	}
}

void SignalWatcher::_clear_signals() {
	if (data) {
		data->_signals.clear();
	}
}

void SignalWatcher::initialize() {
	ERR_FAIL_COND_MSG(data, "SignalWatcher is already initialized.");
	data = memnew(Data);
}

void SignalWatcher::finalize() {
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

// === Merged from tests/test_macros.cpp ===
/*  test_macros.cpp                                                       */
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

#define DOCTEST_CONFIG_IMPLEMENT
#include "tests/test_macros.h"

HashMap<String, TestFunc> *test_commands = nullptr;

int register_test_command(String p_command, TestFunc p_function) {
	if (!test_commands) {
		test_commands = new HashMap<String, TestFunc>;
	}
	test_commands->insert(p_command, p_function);
	return 0;
}

// === Merged from tests/test_utils.cpp ===
/*  test_utils.cpp                                                        */
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
#include "core/io/net_socket.h"
#include "core/os/os.h"

String TestUtils::get_data_path(const String &p_file) {
	String data_path = "../tests/data";
	return get_executable_dir().path_join(data_path.path_join(p_file));
}

String TestUtils::get_executable_dir() {
	return OS::get_executable_path().get_base_dir();
}

String TestUtils::get_temp_path(const String &p_suffix) {
	const String temp_base = OS::get_cache_path().path_join("godot_test");
	DirAccess::make_dir_absolute(temp_base); // Ensure the directory exists.
	return temp_base.path_join(p_suffix);
}

String &TestProjectSettingsInternalsAccessor::resource_path() {
	return ProjectSettings::resource_path;
}
