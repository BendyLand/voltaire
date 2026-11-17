/**************************************************************************/
/*  scene_tree.cpp                                                        */
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

#include "scene_tree.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "scene/animation/tween.h"
#include "scene/gui/control.h"
#include "scene/main/multiplayer_api.h"
#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/environment.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/world_2d.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#ifndef _3D_DISABLED
#include "scene/3d/node_3d.h"
#include "scene/resources/3d/world_3d.h"
#endif // _3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

void SceneTreeTimer::set_time_left(double p_time) { time_left = p_time; }

double SceneTreeTimer::get_time_left() const { return MAX(time_left, 0.0); }

void SceneTreeTimer::set_process_always(bool p_process_always)
{
	process_always = p_process_always;
}

bool SceneTreeTimer::is_process_always() { return process_always; }

void SceneTreeTimer::set_process_in_physics(bool p_process_in_physics)
{
	process_in_physics = p_process_in_physics;
}

bool SceneTreeTimer::is_process_in_physics() { return process_in_physics; }

void SceneTreeTimer::set_ignore_time_scale(bool p_ignore) { ignore_time_scale = p_ignore; }

bool SceneTreeTimer::is_ignoring_time_scale() { return ignore_time_scale; }

#ifndef _3D_DISABLED
void SceneTree::ClientPhysicsInterpolation::physics_process()
{
	for (SelfList<Node3D>* E = _node_3d_list.first(); E;) {
		Node3D* node_3d = E->self();

		SelfList<Node3D>* current = E;

		// Get the next element here BEFORE we potentially delete one.
		E = E->next();

		if (!node_3d->update_client_physics_interpolation_data()) {
			_node_3d_list.remove(current);
		}
	}
}
#endif // _3D_DISABLED

bool SceneTree::_physics_interpolation_enabled = false;
bool SceneTree::_physics_interpolation_enabled_in_project = false;

void SceneTree::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
	SceneTreeFTI::initialize();
	MainLoop::bind<SceneTree>();
}

void SceneTree::finalize()
{
	if (!data) {
		return;
	}

	finalize_tree();

	for (ProcessGroup* pg : data->process_groups) {
		if (pg) {
			memdelete(pg);
		}
	}
	data->process_groups.clear();

	SceneTreeFTI::finalize();

	memdelete(data);
	data = nullptr;

	_physics_interpolation_enabled = false;
	_physics_interpolation_enabled_in_project = false;

	if (MainLoop::destroy == &SceneTree::finalize) {
		MainLoop::reset_backend();
	}
}

void SceneTree::initialize_tree()
{
	if (!data || data->root) {
		return;
	}

	data->root = memnew(Window);
	data->root->_set_tree(true);

	data->root->set_name("root");
	data->root->set_position(Point2i());

	Size2i window_size = DisplayServer::is_initialized()
							 ? DisplayServer::window_get_size()
							 : Size2i();
	if (window_size.width <= 0 || window_size.height <= 0) {
		window_size = Size2i(1152, 648);
	}
	data->root->set_size(window_size);
}

void SceneTree::finalize_tree()
{
	if (!data) {
		return;
	}

	_flush_delete_queue();
	_flush_ugc();

	if (data->root) {
		data->root->_set_tree(false);
		data->root->_propagate_after_exit_tree();
		memdelete(data->root);
		data->root = nullptr;

		_flush_delete_queue();
	}

	// Cleanup timers.
	for (Ref<SceneTreeTimer>& timer : data->timers) {
		timer->release_connections();
	}
	data->timers.clear();

	// Cleanup tweens.
	for (Ref<Tween>& tween : data->tweens) {
		tween->clear();
	}
	data->tweens.clear();
}

SceneTreeGroup* SceneTree::add_to_group(const String& p_group, Node* p_node)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return nullptr;
	}

	HashMap<String, SceneTreeGroup>::Iterator E = data->group_map.find(p_group);
	if (!E) {
		E = data->group_map.insert(p_group, SceneTreeGroup());
	}

	ERR_FAIL_COND_V_MSG(
		E->value.nodes.has(p_node), &E->value, "Already in group: " + p_group + ".");
	E->value.nodes.push_back(p_node);
	E->value.changed = true;
	return &E->value;
}

void SceneTree::remove_from_group(const String& p_group, Node* p_node)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return;
	}

	HashMap<String, SceneTreeGroup>::Iterator E = data->group_map.find(p_group);
	ERR_FAIL_COND(!E);

	E->value.nodes.erase(p_node);
	if (E->value.nodes.is_empty()) {
		data->group_map.remove(E);
	}
}

bool SceneTree::is_accessibility_enabled()
{
	if (!DisplayServer::is_initialized() ||
		!DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER)) {
		return false;
	}

	if (!AccessibilityServer::is_initialized()) {
		return false;
	}

	AccessibilityServerEnums::AccessibilityMode accessibility_mode =
		AccessibilityServer::get_mode();
	int screen_reader_active = DisplayServer::accessibility_screen_reader_active();
	if ((accessibility_mode ==
			AccessibilityServerEnums::AccessibilityMode::ACCESSIBILITY_DISABLED) ||
		((accessibility_mode == AccessibilityServerEnums::AccessibilityMode::ACCESSIBILITY_AUTO) &&
			(screen_reader_active != 1))) {
		return false;
	}
	return true;
}

bool SceneTree::is_accessibility_supported()
{
	if (!DisplayServer::is_initialized() ||
		!DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER)) {
		return false;
	}

	if (!AccessibilityServer::is_initialized()) {
		return false;
	}

	AccessibilityServerEnums::AccessibilityMode accessibility_mode =
		AccessibilityServer::get_mode();
	if (accessibility_mode == AccessibilityServerEnums::AccessibilityMode::ACCESSIBILITY_DISABLED) {
		return false;
	}
	return true;
}

void SceneTree::_accessibility_force_update()
{
	if (data) {
		data->accessibility_force_update = true;
	}
}

void SceneTree::_update_group_order(SceneTreeGroup& g)
{
	if (!g.changed) {
		return;
	}
	if (g.nodes.is_empty()) {
		return;
	}

	Node** gr_nodes = g.nodes.ptrw();
	int gr_node_count = g.nodes.size();

	SortArray<Node*, Node::Comparator> node_sort;
	node_sort.sort(gr_nodes, gr_node_count);

	g.changed = false;
}

Window* SceneTree::get_root() { return data ? data->root : nullptr; }

void SceneTree::notify_group(const String& p_group, int p_notification)
{
	notify_group_flags(GROUP_CALL_DEFAULT, p_group, p_notification);
}

void SceneTree::set_physics_interpolation_enabled(bool p_enabled)
{
	_physics_interpolation_enabled_in_project = p_enabled;

	if (Engine::is_editor_hint()) {
		p_enabled = false;
	}

	if (p_enabled == _physics_interpolation_enabled) {
		return;
	}

	_physics_interpolation_enabled = p_enabled;
	Renderer::set_physics_interpolation_enabled(p_enabled);

	SceneTreeFTI::set_enabled(get_root(), p_enabled);

	if (data && data->root) {
		data->root->reset_physics_interpolation();
	}
}

#ifndef _3D_DISABLED
void SceneTree::client_physics_interpolation_add_node_3d(SelfList<Node3D>* p_elem)
{
	if (data) {
		data->_client_physics_interpolation._node_3d_list.add(p_elem);
	}
}

void SceneTree::client_physics_interpolation_remove_node_3d(SelfList<Node3D>* p_elem)
{
	if (data) {
		data->_client_physics_interpolation._node_3d_list.remove(p_elem);
	}
}
#endif

void SceneTree::iteration_prepare()
{
	if (_physics_interpolation_enabled) {
		flush_transform_notifications();
		SceneTreeFTI::tick_update();
		Renderer::tick();
	}
}

void SceneTree::iteration_end()
{
	if (_physics_interpolation_enabled) {
		flush_transform_notifications();

#ifndef _3D_DISABLED
		if (data) {
			data->_client_physics_interpolation.physics_process();
		}
#endif
	}
}

void SceneTree::process_tweens(double p_delta, bool p_physics)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return;
	}

	const List<Ref<Tween>>::Element* L = data->tweens.back();
	const double unscaled_delta = Engine::get_process_step();

	for (List<Ref<Tween>>::Element* E = data->tweens.front(); E;) {
		List<Ref<Tween>>::Element* N = E->next();
		Ref<Tween>& tween = E->get();

		if (!tween->can_process(data->paused) ||
			(p_physics == (tween->get_process_mode() == Tween::TWEEN_PROCESS_IDLE))) {
			if (E == L) {
				break;
			}
			E = N;
			continue;
		}

		if (!tween->step(tween->is_ignoring_time_scale() ? unscaled_delta : p_delta)) {
			tween->clear();
			data->tweens.erase(E);
		}
		if (E == L) {
			break;
		}
		E = N;
	}
}

void SceneTree::quit(int p_exit_code)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return;
	}

	OS::set_exit_code(p_exit_code);
	data->_quit = true;
}

void SceneTree::_main_window_close()
{
	if (data && data->accept_quit) {
		data->_quit = true;
	}
}

void SceneTree::_main_window_go_back()
{
	if (data && data->quit_on_go_back) {
		data->_quit = true;
	}
}

void SceneTree::_main_window_focus_in()
{
	if (Input::is_initialized()) {
		Input::ensure_touch_mouse_raised();
	}
}

void SceneTree::notification(int p_notification)
{
	Window* root_win = get_root();
	if (!root_win) {
		return;
	}

	switch (p_notification) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		root_win->propagate_notification(p_notification);
	} break;

	case NOTIFICATION_OS_MEMORY_WARNING:
	case NOTIFICATION_OS_IME_UPDATE:
	case NOTIFICATION_WM_ABOUT:
	case NOTIFICATION_CRASH:
	case NOTIFICATION_APPLICATION_RESUMED:
	case NOTIFICATION_APPLICATION_PAUSED:
	case NOTIFICATION_APPLICATION_PIP_MODE_ENTERED:
	case NOTIFICATION_APPLICATION_PIP_MODE_EXITED: {
		root_win->propagate_notification(p_notification);
	} break;

	case NOTIFICATION_APPLICATION_FOCUS_IN:
	case NOTIFICATION_APPLICATION_FOCUS_OUT: {
		if (Input::is_initialized()) {
			Input::set_application_focused(p_notification == NOTIFICATION_APPLICATION_FOCUS_IN);
			Input::release_pressed_events();
		}

		root_win->propagate_notification(p_notification);
	} break;
	}
}

bool SceneTree::is_auto_accept_quit() { return data ? data->accept_quit : true; }

void SceneTree::set_auto_accept_quit(bool p_enable)
{
	if (data) {
		data->accept_quit = p_enable;
	}
}

bool SceneTree::is_quit_on_go_back() { return data ? data->quit_on_go_back : true; }

void SceneTree::set_quit_on_go_back(bool p_enable)
{
	if (data) {
		data->quit_on_go_back = p_enable;
	}
}

#ifdef DEBUG_ENABLED
void SceneTree::set_debug_collisions_hint(bool p_enabled)
{
	if (data) {
		data->debug_collisions_hint = p_enabled;
	}
}

bool SceneTree::is_debugging_collisions_hint()
{
	return data ? data->debug_collisions_hint : false;
}

void SceneTree::set_debug_paths_hint(bool p_enabled)
{
	if (data) {
		data->debug_paths_hint = p_enabled;
	}
}

bool SceneTree::is_debugging_paths_hint() { return data ? data->debug_paths_hint : false; }

void SceneTree::set_debug_navigation_hint(bool p_enabled)
{
	if (data) {
		data->debug_navigation_hint = p_enabled;
	}
}

bool SceneTree::is_debugging_navigation_hint()
{
	return data ? data->debug_navigation_hint : false;
}
#endif

void SceneTree::set_debug_collisions_color(const Color& p_color)
{
	if (data) {
		data->debug_collisions_color = p_color;
	}
}

Color SceneTree::get_debug_collisions_color()
{
	return data ? data->debug_collisions_color : Color(0.0, 0.0, 0.0, 0.0);
}

void SceneTree::set_debug_collision_contact_color(const Color& p_color)
{
	if (data) {
		data->debug_collision_contact_color = p_color;
	}
}

Color SceneTree::get_debug_collision_contact_color()
{
	return data ? data->debug_collision_contact_color : Color();
}

void SceneTree::set_debug_paths_color(const Color& p_color)
{
	if (data) {
		data->debug_paths_color = p_color;
	}
}

Color SceneTree::get_debug_paths_color() { return data ? data->debug_paths_color : Color(); }

void SceneTree::set_debug_paths_width(float p_width)
{
	if (data) {
		data->debug_paths_width = p_width;
	}
}

float SceneTree::get_debug_paths_width() { return data ? data->debug_paths_width : 1.0f; }

Ref<Material> SceneTree::get_debug_paths_material()
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return Ref<Material>();
	}

	if (data->debug_paths_material.is_valid()) {
		return data->debug_paths_material;
	}

	Ref<StandardMaterial3D> _debug_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	_debug_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	_debug_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);

	data->debug_paths_material = _debug_material;

	return data->debug_paths_material;
}

Ref<Material> SceneTree::get_debug_collision_material()
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return Ref<Material>();
	}

	if (data->collision_material.is_valid()) {
		return data->collision_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 1);
	material->set_cull_mode(StandardMaterial3D::CULL_BACK);
	data->collision_material = material;

	return data->collision_material;
}

void SceneTree::set_pause(bool p_enabled)
{
	ERR_FAIL_COND_MSG(!Thread::is_main_thread(), "Pause can only be set from the main thread.");
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->suspended, "Pause state cannot be modified while suspended.");

	if (p_enabled == data->paused) {
		return;
	}

	data->paused = p_enabled;

#ifndef PHYSICS_3D_DISABLED
	if (PhysicsServer3D::get_singleton()) {
		PhysicsServer3D::get_singleton()->set_active(!p_enabled);
	}
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	if (PhysicsServer2D::get_singleton()) {
		PhysicsServer2D::get_singleton()->set_active(!p_enabled);
	}
#endif // PHYSICS_2D_DISABLED
	if (data->root) {
		data->root->_propagate_pause_notification(p_enabled);
	}
}

bool SceneTree::is_paused() { return data ? data->paused : false; }

void SceneTree::set_suspend(bool p_enabled)
{
	ERR_FAIL_COND_MSG(!Thread::is_main_thread(), "Suspend can only be set from the main thread.");
	if (!data || p_enabled == data->suspended) {
		return;
	}

	data->suspended = p_enabled;

	Engine::set_freeze_time_scale(p_enabled);

#ifndef PHYSICS_3D_DISABLED
	if (PhysicsServer3D::get_singleton()) {
		PhysicsServer3D::get_singleton()->set_active(!p_enabled && !data->paused);
	}
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	if (PhysicsServer2D::get_singleton()) {
		PhysicsServer2D::get_singleton()->set_active(!p_enabled && !data->paused);
	}
#endif // PHYSICS_2D_DISABLED
	if (data->root) {
		data->root->_propagate_suspend_notification(p_enabled);
	}
}

bool SceneTree::is_suspended() { return data ? data->suspended : false; }

void SceneTree::_process_groups_thread(uint32_t p_index, bool p_physics)
{
	if (!data) {
		return;
	}
	Node::current_process_thread_group = data->local_process_group_cache[p_index]->owner;
	_process_group(data->local_process_group_cache[p_index], p_physics);
	Node::current_process_thread_group = nullptr;
}

bool SceneTree::ProcessGroupSort::operator()(
	const ProcessGroup* p_left, const ProcessGroup* p_right) const
{
	int left_order = p_left->owner ? p_left->owner->data.process_thread_group_order : 0;
	int right_order = p_right->owner ? p_right->owner->data.process_thread_group_order : 0;

	if (left_order == right_order) {
		int left_threaded = p_left->owner != nullptr && p_left->owner->data.process_thread_group ==
															Node::PROCESS_THREAD_GROUP_SUB_THREAD
								? 0
								: 1;
		int right_threaded =
			p_right->owner != nullptr && p_right->owner->data.process_thread_group ==
											 Node::PROCESS_THREAD_GROUP_SUB_THREAD
				? 0
				: 1;
		return left_threaded < right_threaded;
	}
	else {
		return left_order < right_order;
	}
}

void SceneTree::_remove_process_group(Node* p_node)
{
	_THREAD_SAFE_METHOD_
	if (!data || !p_node) {
		return;
	}
	ProcessGroup* pg = (ProcessGroup*)p_node->data.process_group;
	ERR_FAIL_NULL(pg);
	ERR_FAIL_COND(pg->removed);
	pg->removed = true;
	pg->owner = nullptr;
	p_node->data.process_group = nullptr;
	data->process_groups_dirty = true;
}

void SceneTree::_add_process_group(Node* p_node)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return;
	}
	ERR_FAIL_NULL(p_node);

	ProcessGroup* pg = memnew(ProcessGroup);

	pg->owner = p_node;
	p_node->data.process_group = pg;

	data->process_groups.push_back(pg);

	data->process_groups_dirty = true;
}

void SceneTree::_remove_node_from_process_group(Node* p_node, Node* p_owner)
{
	_THREAD_SAFE_METHOD_
	if (!data || !p_node) {
		return;
	}
	ProcessGroup* pg =
		p_owner ? (ProcessGroup*)p_owner->data.process_group : &data->default_process_group;
	ERR_FAIL_NULL(pg);

	if (p_node->is_processing() || p_node->is_processing_internal()) {
		bool found = pg->nodes.erase(p_node);
		ERR_FAIL_COND(!found);
	}

	if (p_node->is_physics_processing() || p_node->is_physics_processing_internal()) {
		bool found = pg->physics_nodes.erase(p_node);
		ERR_FAIL_COND(!found);
	}
}

void SceneTree::_add_node_to_process_group(Node* p_node, Node* p_owner)
{
	_THREAD_SAFE_METHOD_
	if (!data || !p_node) {
		return;
	}
	ProcessGroup* pg =
		p_owner ? (ProcessGroup*)p_owner->data.process_group : &data->default_process_group;
	ERR_FAIL_NULL(pg);

	if (p_node->is_processing() || p_node->is_processing_internal()) {
		pg->nodes.push_back(p_node);
		pg->node_order_dirty = true;
	}

	if (p_node->is_physics_processing() || p_node->is_physics_processing_internal()) {
		pg->physics_nodes.push_back(p_node);
		pg->physics_node_order_dirty = true;
	}
}

int64_t SceneTree::get_frame() { return data ? data->current_frame : 0; }

bool SceneTree::has_group(const String& p_identifier)
{
	_THREAD_SAFE_METHOD_
	return data ? data->group_map.has(p_identifier) : false;
}

int SceneTree::get_node_count_in_group(const String& p_group)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return 0;
	}
	HashMap<String, SceneTreeGroup>::ConstIterator E = data->group_map.find(p_group);
	if (!E) {
		return 0;
	}

	return E->value.nodes.size();
}

Node* SceneTree::get_first_node_in_group(const String& p_group)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return nullptr;
	}
	HashMap<String, SceneTreeGroup>::Iterator E = data->group_map.find(p_group);
	if (!E) {
		return nullptr;
	}

	_update_group_order(E->value);

	if (E->value.nodes.is_empty()) {
		return nullptr;
	}

	return E->value.nodes[0];
}

Vector<Node*> SceneTree::get_nodes_in_group(const String& p_group)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return {};
	}
	HashMap<String, SceneTreeGroup>::Iterator E = data->group_map.find(p_group);
	if (!E) {
		return {};
	}

	_update_group_order(E->value);
	int nc = E->value.nodes.size();
	if (nc == 0) {
		return {};
	}

	return E->value.nodes;
}

int SceneTree::get_node_count() { return data ? data->nodes_in_tree_count : 0; }

void SceneTree::set_edited_scene_root(Node* p_node)
{
#ifdef TOOLS_ENABLED
	if (data) {
		data->edited_scene_root = p_node;
	}
#else
	(void)p_node;
#endif
}

Node* SceneTree::get_edited_scene_root()
{
#ifdef TOOLS_ENABLED
	return data ? data->edited_scene_root : nullptr;
#else
	return nullptr;
#endif
}

void SceneTree::set_current_scene(Node* p_scene)
{
	ERR_FAIL_COND_MSG(
		!Thread::is_main_thread(), "Changing scene can only be done from the main thread.");
	if (!data) {
		return;
	}
	ERR_FAIL_COND(p_scene && p_scene->get_parent() != data->root);
	data->current_scene = p_scene;
}

Node* SceneTree::get_current_scene() { return data ? data->current_scene : nullptr; }

Error SceneTree::change_scene_to_file(const String& p_path)
{
	ERR_FAIL_COND_V_MSG(!Thread::is_main_thread(), ERR_INVALID_PARAMETER,
		"Changing scene can only be done from the main thread.");
	Ref<PackedScene> new_scene = ResourceLoader::load(p_path);
	if (new_scene.is_null()) {
		return ERR_CANT_OPEN;
	}

	return change_scene_to_packed(new_scene.ptr());
}

Error SceneTree::change_scene_to_packed(PackedScene* rp_scene)
{
	ERR_FAIL_NULL_V(rp_scene, ERR_INVALID_PARAMETER);
	Node* new_scene = rp_scene->instantiate();
	ERR_FAIL_NULL_V(new_scene, ERR_CANT_CREATE);
	return change_scene_to_node(new_scene);
}

Error SceneTree::reload_current_scene()
{
	ERR_FAIL_COND_V_MSG(!Thread::is_main_thread(), ERR_INVALID_PARAMETER,
		"Reloading scene can only be done from the main thread.");
	ERR_FAIL_COND_V(!data || !data->current_scene, ERR_UNCONFIGURED);
	String fname = data->current_scene->get_scene_file_path();
	return change_scene_to_file(fname);
}

void SceneTree::unload_current_scene()
{
	ERR_FAIL_COND_MSG(!Thread::is_main_thread(),
		"Unloading the current scene can only be done from the main thread.");
	if (data && data->current_scene) {
		memdelete(data->current_scene);
		data->current_scene = nullptr;
	}
}

void SceneTree::add_current_scene(Node* p_current)
{
	ERR_FAIL_COND_MSG(
		!Thread::is_main_thread(), "Adding a current scene can only be done from the main thread.");
	if (!data || !data->root) {
		return;
	}
	data->current_scene = p_current;
	data->root->add_child(p_current);
}

SceneTreeTimer* SceneTree::create_timer(
	double p_delay_sec, bool p_process_always, bool p_process_in_physics, bool p_ignore_time_scale)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return nullptr;
	}
	Ref<SceneTreeTimer> stt;
	stt.instantiate();
	stt->set_process_always(p_process_always);
	stt->set_time_left(p_delay_sec);
	stt->set_process_in_physics(p_process_in_physics);
	stt->set_ignore_time_scale(p_ignore_time_scale);
	data->timers.push_back(stt);
	return stt.ptr();
}

Tween* SceneTree::create_tween()
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return nullptr;
	}
	Ref<Tween> tween;
	tween.instantiate(true);
	data->tweens.push_back(tween);
	return tween.ptr();
}

void SceneTree::remove_tween(const Ref<Tween>& p_tween)
{
	_THREAD_SAFE_METHOD_
	if (!data) {
		return;
	}
	for (List<Ref<Tween>>::Element* E = data->tweens.back(); E; E = E->prev()) {
		if (E->get() == p_tween) {
			E->erase();
			break;
		}
	}
}

MultiplayerAPI* SceneTree::get_multiplayer(const NodePath& p_for_path)
{
	ERR_FAIL_COND_V_MSG(!Thread::is_main_thread(), Ref<MultiplayerAPI>().ptr(),
		"Multiplayer can only be manipulated from the main thread.");
	if (!data) {
		return nullptr;
	}
	if (p_for_path.is_empty()) {
		return data->multiplayer.ptr();
	}

	const Vector<String> tnames = p_for_path.get_names();
	const String* nptr = tnames.ptr();
	for (const KeyValue<NodePath, Ref<MultiplayerAPI>>& E : data->custom_multiplayers) {
		const Vector<String> snames = E.key.get_names();
		if (tnames.size() < snames.size()) {
			continue;
		}
		const String* sptr = snames.ptr();
		bool valid = true;
		for (int i = 0; i < snames.size(); i++) {
			if (sptr[i] != nptr[i]) {
				valid = false;
				break;
			}
		}
		if (valid) {
			return E.value.ptr();
		}
	}

	return data->multiplayer.ptr();
}

void SceneTree::set_multiplayer(Ref<MultiplayerAPI> p_multiplayer, const NodePath& p_root_path)
{
	ERR_FAIL_COND_MSG(
		!Thread::is_main_thread(), "Multiplayer can only be manipulated from the main thread.");
	if (!data) {
		return;
	}
	if (p_root_path.is_empty()) {
		ERR_FAIL_COND(p_multiplayer.is_null());
		data->multiplayer = p_multiplayer;
	}
	else if (p_multiplayer.is_valid()) {
		data->custom_multiplayers[p_root_path] = p_multiplayer;
	}
	else {
		data->custom_multiplayers.erase(p_root_path);
	}
}

void SceneTree::set_multiplayer_poll_enabled(bool p_enabled)
{
	ERR_FAIL_COND_MSG(
		!Thread::is_main_thread(), "Multiplayer can only be manipulated from the main thread.");
	if (data) {
		data->multiplayer_poll = p_enabled;
	}
}

bool SceneTree::is_multiplayer_poll_enabled() { return data ? data->multiplayer_poll : true; }

SceneTree::IdleCallback SceneTree::idle_callbacks[SceneTree::MAX_IDLE_CALLBACKS];
int SceneTree::idle_callback_count = 0;

void SceneTree::_call_idle_callbacks()
{
	for (int i = 0; i < idle_callback_count; i++) {
		idle_callbacks[i]();
	}
}

void SceneTree::add_idle_callback(IdleCallback p_callback)
{
	ERR_FAIL_COND(idle_callback_count >= MAX_IDLE_CALLBACKS);
	idle_callbacks[idle_callback_count++] = p_callback;
}

void SceneTree::set_disable_node_threading(bool p_disable)
{
	if (data) {
		data->node_threading_disabled = p_disable;
	}
}

void SceneTree::tree_changed() {}

void SceneTreeTimer::release_connections() {}

void SceneTree::flush_transform_notifications() {}

void SceneTree::_flush_delete_queue() {}

Error SceneTree::change_scene_to_node(Node*) { return OK; }

void SceneTree::_process_group(SceneTree::ProcessGroup*, bool) {}

void SceneTree::notify_group_flags(unsigned int, String const&, int) {}

void SceneTree::_flush_ugc() {}

void SceneTree::_accessibility_notify_change(Node const*, bool) {}

bool SceneTree::physics_process(double p_time)
{
	(void)p_time;
	return false;
}

bool SceneTree::process(double p_time)
{
	(void)p_time;
	return data ? data->_quit : true;
}
