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

#include "test_main.h"

#include "core/config/engine.h"
#include "core/input/input.h"
#include "core/io/dir_access.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/theme.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "tests/display_server_mock.h"
#include "tests/force_link.gen.h"
#include "tests/signal_watcher.h"
#include "tests/test_macros.h"
#include "tests/test_utils.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED
#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED
#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

#include "modules/modules_tests.gen.h" // IWYU pragma: keep // TODO: Migrate module tests to compilation files.

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
