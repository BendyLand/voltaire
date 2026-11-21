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
	if (PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::set_active(!p_enabled);
	}
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	if (PhysicsServer2D::is_initialized()) {
		PhysicsServer2D::set_active(!p_enabled);
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
	if (PhysicsServer3D::is_initialized()) {
		PhysicsServer3D::set_active(!p_enabled && !data->paused);
	}
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	if (PhysicsServer2D::is_initialized()) {
		PhysicsServer2D::set_active(!p_enabled && !data->paused);
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

// ==========================================================================
// Merged from scene/main/scene_tree_fti.cpp
// ==========================================================================

#ifndef _3D_DISABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/transform_interpolator.h"
#include "core/os/os.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"

#ifdef GODOT_SCENE_TREE_FTI_VERIFY
// --- scene_tree_fti_tests.h ---
#include <cstdint>

class Node3D;
class Node;
struct Transform3D;

class SceneTreeFTITests
{
	void debug_verify_failed(const Node3D* p_node, const Transform3D& p_test);

public:
	void update_dirty_nodes(Node* p_node, uint32_t p_current_half_frame,
		float p_interpolation_fraction, bool p_active,
		const Transform3D* p_parent_global_xform = nullptr, int p_depth = 0);
	void frame_update(Node* p_root, uint32_t p_half_frame, float p_interpolation_fraction);

	SceneTreeFTITests();
};
#endif

#ifdef DEV_ENABLED

// Uncomment this to enable some slow extra DEV_ENABLED
// checks to ensure there aren't more than one object added to the lists.
// #define GODOT_SCENE_TREE_FTI_EXTRA_CHECKS

// Uncomment this to regularly print the tree that is being interpolated.
// #define GODOT_SCENE_TREE_FTI_PRINT_TREE

#endif

void SceneTreeFTI::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void SceneTreeFTI::finalize()
{
	if (!data) {
		return;
	}
#ifdef GODOT_SCENE_TREE_FTI_VERIFY
	if (data->_tests) {
		memfree(data->_tests);
		data->_tests = nullptr;
	}
#endif
	memdelete(data);
	data = nullptr;
}

void SceneTreeFTI::_reset_node3d_flags(Node3D& r_node)
{
	r_node.data.fti_on_tick_xform_list = false;
	r_node.data.fti_on_tick_property_list = false;
	r_node.data.fti_on_frame_xform_list = false;
	r_node.data.fti_on_frame_property_list = false;
	r_node.data.fti_global_xform_interp_set = false;
	r_node.data.fti_frame_xform_force_update = false;
	r_node.data.fti_processed = false;
}

void SceneTreeFTI::set_enabled(Node* p_root, bool p_enabled)
{
	if (!data || data->enabled == p_enabled) {
		return;
	}
	MutexLock lock(data->mutex);

	data->tick_xform_list[0].clear();
	data->tick_xform_list[1].clear();

	data->frame_xform_list.clear();
	data->frame_xform_list_forced.clear();

	data->tick_property_list[0].clear();
	data->tick_property_list[1].clear();

	data->frame_property_list.clear();
	data->request_reset_list.clear();

	_clear_depth_lists();

	// Node3D flags must be reset.
	if (p_root) {
		_reset_flags(p_root);
	}

	data->enabled = p_enabled;
}

void SceneTreeFTI::tick_update()
{
	if (!data || !data->enabled) {
		return;
	}
	MutexLock lock(data->mutex);

	_update_request_resets();

	uint32_t curr_mirror = data->mirror;
	uint32_t prev_mirror = curr_mirror ? 0 : 1;

	LocalVector<Node3D*>& curr = data->tick_xform_list[curr_mirror];
	LocalVector<Node3D*>& prev = data->tick_xform_list[prev_mirror];

	// First detect on the previous list but not on this tick list.
	for (uint32_t n = 0; n < prev.size(); n++) {
		Node3D* s = prev[n];
		if (!s->data.fti_on_tick_xform_list) {
			// Needs a reset so jittering will stop.
			s->fti_pump_xform();

			// Optimization - detect whether we have rested at identity xform.
			s->data.fti_is_identity_xform = s->data.local_transform == Transform3D();

			// This may not get updated so set it to the same as global xform.
			// TODO: double check this is the best value.
			s->data.global_transform_interpolated = s->get_global_transform();

			// Remove from interpolation list.
			if (s->data.fti_on_frame_xform_list) {
				_node_remove_from_frame_list(*s, false);
			}

			// Ensure that the node gets at least ONE further
			// update in the resting position in the next frame update.
			if (!s->data.fti_frame_xform_force_update) {
				_node_add_to_frame_list(*s, true);
			}
		}
	}

	LocalVector<Node3D*>& curr_prop = data->tick_property_list[curr_mirror];
	LocalVector<Node3D*>& prev_prop = data->tick_property_list[prev_mirror];

	// Detect on the previous property list but not on this tick list.
	for (uint32_t n = 0; n < prev_prop.size(); n++) {
		Node3D* s = prev_prop[n];

		if (!s->data.fti_on_tick_property_list) {
			// Needs a reset so jittering will stop.
			s->fti_pump_xform();

			// Ensure the servers are up to date with the final resting value.
			s->fti_update_servers_property();

			// Remove from interpolation list.
			if (s->data.fti_on_frame_property_list) {
				s->data.fti_on_frame_property_list = false;
				data->frame_property_list.erase_unordered(s);

#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
				DEV_CHECK_ONCE(data->frame_property_list.find(s) == -1);
#endif
			}
		}
	}

	// Pump all on the property list that are NOT on the tick list.
	for (uint32_t n = 0; n < curr_prop.size(); n++) {
		Node3D* s = curr_prop[n];

		// Reset, needs to be marked each tick.
		s->data.fti_on_tick_property_list = false;
		s->fti_pump_property();
	}

	// Now pump all on the current list.
	for (uint32_t n = 0; n < curr.size(); n++) {
		Node3D* s = curr[n];

		// Reset, needs to be marked each tick.
		s->data.fti_on_tick_xform_list = false;

		// Pump.
		s->fti_pump_xform();
	}

	// Clear previous list and flip.
	prev.clear();
	prev_prop.clear();
	data->mirror = prev_mirror;
}

void SceneTreeFTI::node_3d_request_reset(Node3D* p_node)
{
	if (!data) {
		return;
	}
	DEV_CHECK_ONCE(data->enabled);
	DEV_ASSERT(p_node);

	MutexLock lock(data->mutex);

	if (!p_node->_is_physics_interpolation_reset_requested()) {
		p_node->_set_physics_interpolation_reset_requested(true);
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		DEV_CHECK_ONCE(data->request_reset_list.find(p_node) == -1);
#endif
		data->request_reset_list.push_back(p_node);
	}
}

void SceneTreeFTI::_node_3d_notify_set_property(Node3D& r_node)
{
	if (!data || !r_node.is_physics_interpolated()) {
		return;
	}

	DEV_CHECK_ONCE(data->enabled);

	// Note that a Node3D can be on BOTH the transform list and the property list.
	if (!r_node.data.fti_on_tick_property_list) {
		r_node.data.fti_on_tick_property_list = true;

		// Should only appear once in the property list.
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		DEV_CHECK_ONCE(data->tick_property_list[data->mirror].find(&r_node) == -1);
#endif
		data->tick_property_list[data->mirror].push_back(&r_node);
	}

	if (!r_node.data.fti_on_frame_property_list) {
		r_node.data.fti_on_frame_property_list = true;

		// Should only appear once in the property frame list.
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		DEV_CHECK_ONCE(data->frame_property_list.find(&r_node) == -1);
#endif
		data->frame_property_list.push_back(&r_node);
	}
}

void SceneTreeFTI::_clear_depth_lists()
{
	if (!data) {
		return;
	}
	for (uint32_t d = 0; d < Data::scene_tree_depth_limit; d++) {
		data->dirty_node_depth_lists[d].clear();
	}
}

void SceneTreeFTI::_node_add_to_frame_list(Node3D& r_node, bool p_forced)
{
	if (!data) {
		return;
	}
	if (p_forced) {
		DEV_ASSERT(!r_node.data.fti_frame_xform_force_update);
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		int64_t found = data->frame_xform_list_forced.find(&r_node);
		if (found != -1) {
			ERR_FAIL_COND(found != -1);
		}
#endif
		data->frame_xform_list_forced.push_back(&r_node);
		r_node.data.fti_frame_xform_force_update = true;
	}
	else {
		DEV_ASSERT(!r_node.data.fti_on_frame_xform_list);
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		int64_t found = data->frame_xform_list.find(&r_node);
		if (found != -1) {
			ERR_FAIL_COND(found != -1);
		}
#endif
		data->frame_xform_list.push_back(&r_node);
		r_node.data.fti_on_frame_xform_list = true;
	}
}

void SceneTreeFTI::_node_remove_from_frame_list(Node3D& r_node, bool p_forced)
{
	if (!data) {
		return;
	}
	if (p_forced) {
		DEV_ASSERT(r_node.data.fti_frame_xform_force_update);
		data->frame_xform_list_forced.erase_unordered(&r_node);
		r_node.data.fti_frame_xform_force_update = false;
	}
	else {
		DEV_ASSERT(r_node.data.fti_on_frame_xform_list);
		data->frame_xform_list.erase_unordered(&r_node);
		r_node.data.fti_on_frame_xform_list = false;
	}
}

void SceneTreeFTI::_node_3d_notify_set_xform(Node3D& r_node)
{
	if (!data) {
		return;
	}
	DEV_CHECK_ONCE(data->enabled);

	if (!r_node.is_physics_interpolated()) {
		// Force an update of non-interpolated to servers
		// on the next traversal.
		if (!r_node.data.fti_frame_xform_force_update) {
			_node_add_to_frame_list(r_node, true);
		}

		// ToDo: Double check this is a win,
		// non-interpolated nodes we always check for identity,
		// *just in case*.
		r_node.data.fti_is_identity_xform = r_node.get_transform() == Transform3D();
		return;
	}

	r_node.data.fti_is_identity_xform = false;

	if (!r_node.data.fti_on_tick_xform_list) {
		r_node.data.fti_on_tick_xform_list = true;

		// Should only appear once in the xform list.
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		DEV_CHECK_ONCE(data->tick_xform_list[data->mirror].find(&r_node) == -1);
#endif
		data->tick_xform_list[data->mirror].push_back(&r_node);
	}

	if (!r_node.data.fti_on_frame_xform_list) {
		_node_add_to_frame_list(r_node, false);
	}

	// If we are in the second half of a frame, always add to the force update list,
	// because we ignore the tick update list during the second update.
	if (data->in_frame) {
		if (!r_node.data.fti_frame_xform_force_update) {
			_node_add_to_frame_list(r_node, true);
		}
	}
}

void SceneTreeFTI::node_3d_notify_delete(Node3D* p_node)
{
	if (!data || !data->enabled) {
		return;
	}

	ERR_FAIL_NULL(p_node);

	MutexLock lock(data->mutex);

	// Remove from frame lists.
	if (p_node->data.fti_on_frame_xform_list) {
		_node_remove_from_frame_list(*p_node, false);
	}
	if (p_node->data.fti_frame_xform_force_update) {
		_node_remove_from_frame_list(*p_node, true);
	}

	// Ensure this is kept in sync with the lists, in case a node
	// is removed and re-added to the scene tree multiple times
	// on the same frame / tick.
	p_node->_set_physics_interpolation_reset_requested(false);

	// Keep flags consistent for the same as a new node,
	// because this node may re-enter the scene tree.
	_reset_node3d_flags(*p_node);

	// This can potentially be optimized for large scenes with large churn,
	// as it will be doing a linear search through the lists.
	data->tick_xform_list[0].erase_unordered(p_node);
	data->tick_xform_list[1].erase_unordered(p_node);

	data->tick_property_list[0].erase_unordered(p_node);
	data->tick_property_list[1].erase_unordered(p_node);

	data->frame_property_list.erase_unordered(p_node);
	data->request_reset_list.erase_unordered(p_node);

#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
	// There should only be one occurrence on the lists.
	// Check this in DEV_ENABLED builds.
	DEV_CHECK_ONCE(data->tick_xform_list[0].find(p_node) == -1);
	DEV_CHECK_ONCE(data->tick_xform_list[1].find(p_node) == -1);

	DEV_CHECK_ONCE(data->tick_property_list[0].find(p_node) == -1);
	DEV_CHECK_ONCE(data->tick_property_list[1].find(p_node) == -1);

	DEV_CHECK_ONCE(data->frame_property_list.find(p_node) == -1);
	DEV_CHECK_ONCE(data->request_reset_list.find(p_node) == -1);

	DEV_CHECK_ONCE(data->frame_xform_list.find(p_node) == -1);
	DEV_CHECK_ONCE(data->frame_xform_list_forced.find(p_node) == -1);
#endif
}

void SceneTreeFTI::frame_update(Node* p_root, bool p_frame_start)
{
	(void)p_root;
	(void)p_frame_start;
}

void SceneTreeFTI::_create_depth_lists() {}

void SceneTreeFTI::_update_request_resets() {}

void SceneTreeFTI::_reset_flags(Node*) {}

#endif // ndef _3D_DISABLED

// ==========================================================================
// Merged from scene/main/scene_tree_fti_tests.cpp
// ==========================================================================

#ifndef _3D_DISABLED

#ifdef GODOT_SCENE_TREE_FTI_VERIFY
#include "scene/3d/node_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/main/scene_tree.h"

void SceneTreeFTITests::debug_verify_failed(const Node3D* p_node_3d, const Transform3D& p_test)
{
	print_line("VERIFY FAILED\n");
	print_line("test xform : " + String(p_test));

	bool first = true;

	while (p_node_3d) {
		int32_t depth = MAX(p_node_3d->_get_scene_tree_depth(), 0);
		String tabs;
		for (int32_t n = 0; n < depth; n++) {
			tabs += "\t";
		}

		bool interp_equal = p_node_3d->_get_cached_global_transform_interpolated() == p_test;
		bool glob_equal = p_node_3d->get_global_transform() == p_test;

		String sz = tabs + p_node_3d->get_description() + "\n";

		if (first) {
			sz += tabs + "... " + String(p_test) + "\n";
		}

		sz += tabs + (p_node_3d->data.fti_global_xform_interp_set ? "[I] " : "[i] ") +
			  String(p_node_3d->_get_cached_global_transform_interpolated()) +
			  (interp_equal ? " ***" : "") + "\n";
		sz += tabs + "[g] " + String(p_node_3d->get_global_transform()) +
			  (glob_equal ? " ***" : "");

		print_line(sz);

		p_node_3d = p_node_3d->get_parent_node_3d();
		first = false;
	}
}

void SceneTreeFTITests::update_dirty_nodes(Node* p_node, uint32_t p_current_half_frame,
	float p_interpolation_fraction, bool p_active, const Transform3D* p_parent_global_xform,
	int p_depth)
{
	if (!SceneTreeFTI::data) {
		return;
	}
	SceneTreeFTI::Data& data = *SceneTreeFTI::data;

	bool should_verify =
		(data.traversal_mode == SceneTreeFTI::TM_DEBUG) && data.use_optimized_traversal_method;
	bool set_state = !should_verify;

	Node3D* s = dynamic_cast<Node3D*>(p_node);

	if (s && !s->is_visible()) {
		return;
	}

	if (!s) {
		for (int n = 0; n < p_node->get_child_count(); n++) {
			update_dirty_nodes(p_node->get_child(n), p_current_half_frame, p_interpolation_fraction,
				p_active, nullptr, p_depth + 1);
		}
		return;
	}

	if (s->_test_dirty_bits(Node3D::DIRTY_GLOBAL_TRANSFORM)) {
		_ALLOW_DISCARD_ s->get_global_transform();
	}

	if (!p_active) {
		if (data.frame_start) {
			if (s->data.fti_on_frame_xform_list || s->data.fti_frame_xform_force_update) {
				p_active = true;
			}
		}
		else {
			if (s->_test_dirty_bits(Node3D::DIRTY_GLOBAL_INTERPOLATED_TRANSFORM)) {
				p_active = true;
			}
		}
	}

	if (data.frame_start) {
		s->data.fti_global_xform_interp_set = p_active;
	}

	if (p_active) {
		Transform3D local_interp;
		if (s->is_physics_interpolated()) {
			if (s->data.fti_on_tick_xform_list) {
				TransformInterpolator::interpolate_transform_3d(s->data.local_transform_prev,
					s->get_transform(), local_interp, p_interpolation_fraction);
			}
			else {
				local_interp = s->get_transform();
			}
		}
		else {
			local_interp = s->get_transform();
		}

		if (!s->is_set_as_top_level()) {
			if (p_parent_global_xform) {
				if (should_verify) {
					Transform3D test = (*p_parent_global_xform) * local_interp;
					if (s->data.disable_scale) {
						test.basis.orthonormalize();
					}
					if (s->data.global_transform_interpolated != test) {
						debug_verify_failed(s, test);
						DEV_ASSERT(s->data.global_transform_interpolated == test);
					}
				}
				else {
					s->data.global_transform_interpolated =
						s->data.fti_is_identity_xform ? (*p_parent_global_xform)
													  : (*p_parent_global_xform) * local_interp;
				}
			}
			else {
				const Node3D* parent = s->get_parent_node_3d();

				if (parent) {
					const Transform3D& parent_glob = parent->data.fti_global_xform_interp_set
														 ? parent->data.global_transform_interpolated
														 : parent->get_global_transform();

					if (should_verify) {
						Transform3D test = parent_glob * local_interp;
						if (s->data.disable_scale) {
							test.basis.orthonormalize();
						}
						if (s->data.global_transform_interpolated != test) {
							debug_verify_failed(s, test);
							DEV_ASSERT(s->data.global_transform_interpolated == test);
						}
					}
					else {
						s->data.global_transform_interpolated = s->data.fti_is_identity_xform
																	? parent_glob
																	: parent_glob * local_interp;
					}
				}
				else {
					if (set_state) {
						s->data.global_transform_interpolated = local_interp;
					}
				}
			}
		}
		else {
			if (set_state) {
				s->data.global_transform_interpolated = local_interp;
			}
		}

		if (set_state) {
			if (s->data.disable_scale) {
				s->data.global_transform_interpolated.basis.orthonormalize();
			}

			s->fti_update_servers_xform();

			s->data.fti_frame_xform_force_update = false;
		}

		s->data.fti_processed = true;
	} // if active.

	if (set_state) {
		s->_clear_dirty_bits(Node3D::DIRTY_GLOBAL_INTERPOLATED_TRANSFORM);
	}

	for (int n = 0; n < p_node->get_child_count(); n++) {
		update_dirty_nodes(p_node->get_child(n), p_current_half_frame, p_interpolation_fraction,
			p_active,
			s->data.fti_global_xform_interp_set ? &s->data.global_transform_interpolated
												: &s->data.global_transform,
			p_depth + 1);
	}
}

void SceneTreeFTITests::frame_update(
	Node* p_root, uint32_t p_half_frame, float p_interpolation_fraction)
{
	if (!SceneTreeFTI::data) {
		return;
	}
	SceneTreeFTI::Data& data = *SceneTreeFTI::data;

	// For testing, use both methods.
	// FIRST the entire tree, writing out state.
	{
		data.use_optimized_traversal_method = false;
		update_dirty_nodes(p_root, p_half_frame, p_interpolation_fraction, false);
	}

	// SECOND the optimized depth lists only,
	// no writing of state, and verifying results.
	{
		data.use_optimized_traversal_method = true;

		SceneTreeFTI::_create_depth_lists();

		for (uint32_t d = 0; d < SceneTreeFTI::Data::scene_tree_depth_limit; d++) {
			const LocalVector<Node3D*>& list = data.dirty_node_depth_lists[d];

			for (uint32_t n = 0; n < list.size(); n++) {
				Node3D* s = list[n];

				if (s->data.fti_processed) {
					continue;
				}

				if (s->has_render_instance()) {
					if (!s->_is_vi_visible()) {
						continue;
					}
				}
				else if (!s->is_visible_in_tree()) {
					continue;
				}

				update_dirty_nodes(s, p_half_frame, p_interpolation_fraction, true);
			}
		}

		SceneTreeFTI::_clear_depth_lists();
	}
}

SceneTreeFTITests::SceneTreeFTITests()
{
	print_line("SceneTreeFTI : GODOT_SCENE_TREE_FTI_VERIFY defined");
}

#endif // def GODOT_SCENE_TREE_FTI_VERIFY

#endif // ndef _3D_DISABLED

// ==========================================================================
// Merged from scene/main/multiplayer_api.cpp
// ==========================================================================

#include "multiplayer_api.h"

#include "core/io/marshalls.h"
#include "modules/modules_enabled.gen.h"

String MultiplayerAPI::default_interface;

void MultiplayerAPI::set_default_interface(const String& p_interface)
{
	default_interface = String(p_interface);
}

String MultiplayerAPI::get_default_interface() { return default_interface; }

Ref<MultiplayerAPI> MultiplayerAPI::create_default_interface()
{
	return Ref<MultiplayerAPI>(memnew(MultiplayerAPI));
}

#ifndef MODULE_MULTIPLAYER_ENABLED

MultiplayerAPI::MultiplayerAPI() {}

MultiplayerAPI::~MultiplayerAPI() {}

int MultiplayerAPI::get_unique_id() { return 0; }

Vector<int> MultiplayerAPI::get_peer_ids() { return Vector<int>(); }

int MultiplayerAPI::get_remote_sender_id() const { return 0; }

Error MultiplayerAPI::poll() { return OK; }

void MultiplayerAPI::set_multiplayer_peer(const Ref<MultiplayerPeer>& p_peer) {}

Ref<MultiplayerPeer> MultiplayerAPI::get_multiplayer_peer() { return Ref<MultiplayerPeer>(); }

void MultiplayerAPI::clear() {}

void MultiplayerAPI::set_root_path(const NodePath& p_path) {}

NodePath MultiplayerAPI::get_root_path() const { return NodePath(); }

void MultiplayerAPI::disconnect_peer(int p_id) {}

Error MultiplayerAPI::send_auth(int p_to, Vector<uint8_t> p_bytes) { return ERR_UNCONFIGURED; }

Error MultiplayerAPI::complete_auth(int p_peer) { return ERR_UNCONFIGURED; }

void MultiplayerAPI::set_auth_timeout(double p_timeout) {}

double MultiplayerAPI::get_auth_timeout() const { return 0.0; }

Vector<int> MultiplayerAPI::get_authenticating_peer_ids() { return Vector<int>(); }

Error MultiplayerAPI::send_command(int p_to, const uint8_t* p_packet, int p_packet_len)
{
	return ERR_UNCONFIGURED;
}

Error MultiplayerAPI::send_bytes(
	Vector<uint8_t> p_data, int p_to, MultiplayerPeer::TransferMode p_mode, int p_channel)
{
	return ERR_UNCONFIGURED;
}

HashSet<int> MultiplayerAPI::get_connected_peers() const { return HashSet<int>(); }

void MultiplayerAPI::set_remote_sender_override(int p_id) {}

void MultiplayerAPI::set_refuse_new_connections(bool p_refuse) {}

bool MultiplayerAPI::is_refusing_new_connections() const { return false; }

void MultiplayerAPI::set_allow_object_decoding(bool p_enable) {}

bool MultiplayerAPI::is_object_decoding_allowed() const { return false; }

void MultiplayerAPI::set_server_relay_enabled(bool p_enabled) {}

bool MultiplayerAPI::is_server_relay_enabled() const { return false; }

void MultiplayerAPI::set_max_sync_packet_size(int p_size) {}

int MultiplayerAPI::get_max_sync_packet_size() const { return 0; }

void MultiplayerAPI::set_max_delta_packet_size(int p_size) {}

int MultiplayerAPI::get_max_delta_packet_size() const { return 0; }

#endif // MODULE_MULTIPLAYER_ENABLED

// ==========================================================================
// Merged from scene/main/multiplayer_peer.cpp
// ==========================================================================

#include "core/os/os.h"
#include "multiplayer_peer.h"

uint32_t MultiplayerPeer::generate_unique_id() const
{
	uint32_t hash = 0;

	while (hash == 0 || hash == 1) {
		hash = hash_murmur3_one_32((uint32_t)OS::get_ticks_usec());
		hash = hash_murmur3_one_32((uint32_t)OS::get_unix_time(), hash);
		hash =
			hash_murmur3_one_32((uint32_t)OS::get_user_data_dir().hash64(), hash);
		hash = hash_murmur3_one_32((uint32_t)((uint64_t)this), hash);  // Rely on ASLR heap
		hash = hash_murmur3_one_32((uint32_t)((uint64_t)&hash), hash); // Rely on ASLR stack

		hash = hash_fmix32(hash);
		hash =
			hash &
			0x7FFFFFFF; // Make it compatible with unsigned, since negative ID is used for exclusion
	}

	return hash;
}

void MultiplayerPeer::set_transfer_channel(int p_channel) { transfer_channel = p_channel; }

int MultiplayerPeer::get_transfer_channel() const { return transfer_channel; }

void MultiplayerPeer::set_transfer_mode(TransferMode p_mode) { transfer_mode = p_mode; }

MultiplayerPeer::TransferMode MultiplayerPeer::get_transfer_mode() const { return transfer_mode; }

void MultiplayerPeer::set_refuse_new_connections(bool p_enable) { refuse_connections = p_enable; }

bool MultiplayerPeer::is_refusing_new_connections() const { return refuse_connections; }

bool MultiplayerPeer::is_server_relay_supported() const { return false; }


/*************/

Error MultiplayerPeerExtension::get_packet(const uint8_t** r_buffer, int& r_buffer_size)
{
	if (script_buffer.is_empty()) {
		return Error::ERR_UNAVAILABLE;
	}

	*r_buffer = script_buffer.ptr();
	r_buffer_size = script_buffer.size();

	return Error::OK;
}

Error MultiplayerPeerExtension::put_packet(const uint8_t* p_buffer, int p_buffer_size)
{
	ERR_FAIL_COND_V(p_buffer_size < 0, ERR_INVALID_PARAMETER);
	PackedByteArray a;
	a.resize(p_buffer_size);
	memcpy(a.ptrw(), p_buffer, p_buffer_size);

	WARN_PRINT_ONCE("MultiplayerPeerExtension::_put_packet_native is unimplemented!");
	return FAILED;
}

void MultiplayerPeerExtension::set_refuse_new_connections(bool p_enable)
{
	MultiplayerPeer::set_refuse_new_connections(p_enable);
}

bool MultiplayerPeerExtension::is_refusing_new_connections() const
{
	return MultiplayerPeer::is_refusing_new_connections();
}

bool MultiplayerPeerExtension::is_server_relay_supported() const
{
	return MultiplayerPeer::is_server_relay_supported();
}

// ==========================================================================
// Merged from scene/main/http_request.cpp
// ==========================================================================

#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/stream_peer_gzip.h"
#include "core/os/os.h"
#include "core/os/thread.h"
// --- Inlined from scene/main/http_request.h ---
#include "core/io/http_client.h"
#include "core/io/stream_peer_gzip.h"
#include "core/templates/safe_refcount.h"
#include "scene/main/node.h"

class Thread;
class Timer;
class StreamPeerGZIP;

class HTTPRequest : public Node
{
public:
	enum Result
	{
		RESULT_SUCCESS,
		RESULT_CHUNKED_BODY_SIZE_MISMATCH,
		RESULT_CANT_CONNECT,
		RESULT_CANT_RESOLVE,
		RESULT_CONNECTION_ERROR,
		RESULT_TLS_HANDSHAKE_ERROR,
		RESULT_NO_RESPONSE,
		RESULT_BODY_SIZE_LIMIT_EXCEEDED,
		RESULT_BODY_DECOMPRESS_FAILED,
		RESULT_REQUEST_FAILED,
		RESULT_DOWNLOAD_FILE_CANT_OPEN,
		RESULT_DOWNLOAD_FILE_WRITE_ERROR,
		RESULT_REDIRECT_LIMIT_REACHED,
		RESULT_TIMEOUT

	};

private:
	bool requesting = false;
	bool download_complete = false;

	String request_string;
	String url;
	int port = 80;
	Vector<String> headers;
	bool use_tls = false;
	Ref<TLSOptions> tls_options;
	HTTPClient::Method method;
	Vector<uint8_t> request_data;

	bool request_sent = false;
	Ref<HTTPClient> client;
	PackedByteArray body;
	SafeFlag use_threads;
	bool accept_gzip = true;

	bool got_response = false;
	int response_code = 0;
	Vector<String> response_headers;

	String download_to_file;
	bool keep_partial_download = false;
	bool append_to_download_file = false;

	Ref<StreamPeerGZIP> decompressor;
	Ref<FileAccess> file;

	int body_len = -1;
	SafeNumeric<int> downloaded;
	SafeNumeric<int> final_body_size;
	int body_size_limit = -1;

	int redirections = 0;

	bool _update_connection();

	int max_redirects = 8;

	double timeout = 0;

	void _redirect_request(const String& p_new_url);

	bool _is_content_header(const String& p_header) const;
	bool _is_method_safe() const;
	Error _get_redirect_headers(Vector<String>* r_headers);
	bool _is_automatic_redirect() const;

	bool _handle_response(bool* ret_value);

	Error _parse_url(const String& p_url);
	Error _request();

	bool has_header(const PackedStringArray& p_headers, const String& p_header_name);
	String get_header_value(const PackedStringArray& p_headers, const String& header_name);

	SafeFlag thread_done;
	SafeFlag thread_request_quit;

	Thread thread;

	void _defer_done(int p_status, int p_code, const PackedStringArray& p_headers,
		const PackedByteArray& p_data);
	void _request_done(int p_status, int p_code, const PackedStringArray& p_headers,
		const PackedByteArray& p_data);
	static void _thread_func(void* p_userdata);

protected:
	void _notification(int p_what);

public:
	void cancel_request();
	HTTPClient::Status get_http_client_status() const;

	void set_use_threads(bool p_use);
	bool is_using_threads() const;

	void set_accept_gzip(bool p_gzip);
	bool is_accepting_gzip() const;

	void set_download_file(const String& p_file);
	String get_download_file() const;

	void set_keep_partial_download(bool p_keep);
	bool is_keeping_partial_download() const;

	void set_append_to_download_file(bool p_append);
	bool is_appending_to_download_file() const;

	void set_download_chunk_size(int p_chunk_size);
	int get_download_chunk_size() const;

	void set_body_size_limit(int p_bytes);
	int get_body_size_limit() const;

	void set_max_redirects(int p_max);
	int get_max_redirects() const;

	Timer* timer = nullptr;

	void set_timeout(double p_timeout);
	double get_timeout();

	void _timeout();

	int get_downloaded_bytes() const;
	int get_body_size() const;

	void set_http_proxy(const String& p_host, int p_port);
	void set_https_proxy(const String& p_host, int p_port);

	void set_tls_options(const Ref<TLSOptions>& p_options);

	HTTPRequest();
};



// --- End scene/main/http_request.h ---
#include "scene/main/timer.h"

Error HTTPRequest::_request()
{
	return client->connect_to_host(url, port, use_tls ? tls_options : nullptr);
}

Error HTTPRequest::_parse_url(const String& p_url)
{
	use_tls = false;
	request_string = "";
	port = 80;
	request_sent = false;
	got_response = false;
	body_len = -1;
	body.clear();
	downloaded.set(0);
	final_body_size.set(0);
	redirections = 0;

	String scheme;
	String fragment;
	Error err = p_url.parse_url(scheme, url, port, request_string, fragment);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Error parsing URL: '%s'.", p_url));

	if (scheme == "https://") {
		use_tls = true;
	}
	else if (scheme != "http://") {
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER, vformat("Invalid URL scheme: '%s'.", scheme));
	}

	if (port == 0) {
		port = use_tls ? 443 : 80;
	}
	if (request_string.is_empty()) {
		request_string = "/";
	}
	return OK;
}

bool HTTPRequest::has_header(const PackedStringArray& p_headers, const String& p_header_name)
{
	bool exists = false;

	String lower_case_header_name = p_header_name.to_lower();
	for (int i = 0; i < p_headers.size() && !exists; i++) {
		String sanitized = p_headers[i].strip_edges().to_lower();
		if (sanitized.begins_with(lower_case_header_name)) {
			exists = true;
		}
	}

	return exists;
}

String HTTPRequest::get_header_value(
	const PackedStringArray& p_headers, const String& p_header_name)
{
	String value = "";

	String lowwer_case_header_name = p_header_name.to_lower();
	for (int i = 0; i < p_headers.size(); i++) {
		if (p_headers[i].find_char(':') > 0) {
			Vector<String> parts = p_headers[i].split(":", false, 1);
			if (parts.size() > 1 && parts[0].strip_edges().to_lower() == lowwer_case_header_name) {
				value = parts[1].strip_edges();
				break;
			}
		}
	}

	return value;
}

void HTTPRequest::_thread_func(void* p_userdata)
{
	HTTPRequest* hr = static_cast<HTTPRequest*>(p_userdata);

	Error err = hr->_request();

	if (err != OK) {
		hr->_defer_done(RESULT_CANT_CONNECT, 0, PackedStringArray(), PackedByteArray());
	}
	else {
		while (!hr->thread_request_quit.is_set()) {
			bool exit = hr->_update_connection();
			if (exit) {
				break;
			}
			OS::delay_usec(1);
		}
	}

	hr->thread_done.set();
}

void HTTPRequest::cancel_request()
{
	timer->stop();

	if (!requesting) {
		return;
	}

	if (!use_threads.is_set()) {
		set_process_internal(false);
	}
	else {
		thread_request_quit.set();
		if (thread.is_started()) {
			thread.wait_to_finish();
		}
	}

	if (file.is_valid()) {
		file.unref();
		if (!download_complete && !keep_partial_download) {
			DirAccess::remove_absolute(download_to_file);
		}
	}

	decompressor.unref();
	client->close();
	body.clear();
	got_response = false;
	response_code = -1;
	request_sent = false;
	download_complete = false;
	append_to_download_file = false;
	requesting = false;
}

bool HTTPRequest::_is_content_header(const String& p_header) const
{
	return (p_header.begins_with("content-type:") || p_header.begins_with("content-length:") ||
			p_header.begins_with("content-location:") ||
			p_header.begins_with("content-encoding:") ||
			p_header.begins_with("transfer-encoding:") || p_header.begins_with("connection:") ||
			p_header.begins_with("authorization:"));
}

bool HTTPRequest::_is_method_safe() const
{
	return (method == HTTPClient::METHOD_GET || method == HTTPClient::METHOD_HEAD ||
			method == HTTPClient::METHOD_OPTIONS || method == HTTPClient::METHOD_TRACE);
}

Error HTTPRequest::_get_redirect_headers(Vector<String>* r_headers)
{
	for (const String& E : headers) {
		const String h = E.to_lower();
		// We strip content headers when changing a redirect to GET.
		if (!_is_content_header(h)) {
			r_headers->push_back(E);
		}
	}
	return OK;
}

bool HTTPRequest::_is_automatic_redirect() const
{
	if (unlikely(method == HTTPClient::METHOD_CONNECT)) {
		// Never automatically redirect CONNECT requests.
		return false;
	}
	else if (response_code == 301 || response_code == 302 || response_code == 303 ||
			   response_code == 305) {
		// We change unsafe methods to GET for 301, 302, and 303, so these are always redirected.
		// 305 is deprecated and treated as equivalent to 302.
		return true;
	}
	else if ((response_code == 307 || response_code == 308) && _is_method_safe()) {
		// We only automatically redirect for safe methods on method-preserving status codes.
		return true;
	}
	else {
		return false;
	}
}

bool HTTPRequest::_handle_response(bool* ret_value)
{
	if (!client->has_response()) {
		_defer_done(RESULT_NO_RESPONSE, 0, PackedStringArray(), PackedByteArray());
		*ret_value = true;
		return true;
	}

	got_response = true;
	response_code = client->get_response_code();
	List<String> rheaders;
	client->get_response_headers(&rheaders);
	response_headers.clear();
	downloaded.set(0);
	final_body_size.set(0);
	decompressor.unref();

	for (const String& E : rheaders) {
		response_headers.push_back(E);
	}

	if (_is_automatic_redirect()) {
		// Follow redirect.

		if (max_redirects >= 0 && redirections >= max_redirects) {
			_defer_done(
				RESULT_REDIRECT_LIMIT_REACHED, response_code, response_headers, PackedByteArray());
			*ret_value = true;
			return true;
		}

		String new_request;

		for (const String& E : rheaders) {
			if (E.to_lower().begins_with("location: ")) {
				new_request = E.substr(9).strip_edges();
			}
		}

		if (!new_request.is_empty()) {
			// Process redirect.
			client->close();
			int new_redirs = redirections + 1; // Because _request() will clear it.
			Error err;
			if (new_request.begins_with("http")) {
				// New url, new request.
				_parse_url(new_request);
			}
			else {
				request_string = new_request;
			}

			err = _request();
			if (err == OK) {
				request_sent = false;
				got_response = false;
				body_len = -1;
				body.clear();
				downloaded.set(0);
				final_body_size.set(0);
				redirections = new_redirs;
				*ret_value = false;
				if (!_is_method_safe()) {
					// 301, 302, and 303 are changed to GET for unsafe methods.
					// See: https://www.rfc-editor.org/rfc/rfc9110#section-15.4-3.1
					method = HTTPClient::METHOD_GET;
					// Content headers should be dropped if changing method.
					// See: https://www.rfc-editor.org/rfc/rfc9110#section-15.4-6.2.1
					Vector<String> req_headers;
					_get_redirect_headers(&req_headers);
					headers = req_headers;
				}
				return true;
			}
		}
	}

	// Check if we need to start streaming decompression.
	String content_encoding;
	if (accept_gzip) {
		content_encoding = get_header_value(response_headers, "Content-Encoding").to_lower();
	}
	if (content_encoding == "gzip") {
		decompressor.instantiate();
		decompressor->start_decompression(false, get_download_chunk_size());
	}
	else if (content_encoding == "deflate") {
		decompressor.instantiate();
		decompressor->start_decompression(true, get_download_chunk_size());
	}

	return false;
}

bool HTTPRequest::_update_connection()
{
	switch (client->get_status()) {
	case HTTPClient::STATUS_DISCONNECTED: {
		_defer_done(RESULT_CANT_CONNECT, 0, PackedStringArray(), PackedByteArray());
		return true; // End it, since it's disconnected.
	} break;
	case HTTPClient::STATUS_RESOLVING: {
		client->poll();
		// Must wait.
		return false;
	} break;
	case HTTPClient::STATUS_CANT_RESOLVE: {
		_defer_done(RESULT_CANT_RESOLVE, 0, PackedStringArray(), PackedByteArray());
		return true;

	} break;
	case HTTPClient::STATUS_CONNECTING: {
		client->poll();
		// Must wait.
		return false;
	} break; // Connecting to IP.
	case HTTPClient::STATUS_CANT_CONNECT: {
		_defer_done(RESULT_CANT_CONNECT, 0, PackedStringArray(), PackedByteArray());
		return true;

	} break;
	case HTTPClient::STATUS_CONNECTED: {
		if (request_sent) {
			if (!got_response) {
				// No body.

				bool ret_value;

				if (_handle_response(&ret_value)) {
					return ret_value;
				}

				_defer_done(RESULT_SUCCESS, response_code, response_headers, PackedByteArray());
				return true;
			}
			if (body_len < 0) {
				// Chunked transfer is done.
				download_complete = true;
				_defer_done(RESULT_SUCCESS, response_code, response_headers, body);
				return true;
			}

			_defer_done(RESULT_CHUNKED_BODY_SIZE_MISMATCH, response_code, response_headers,
				PackedByteArray());
			return true;
			// Request might have been done.
		}
		else {
			// Did not request yet, do request.

			int size = request_data.size();
			Error err = client->request(
				method, request_string, headers, size > 0 ? request_data.ptr() : nullptr, size);
			if (err != OK) {
				_defer_done(RESULT_CONNECTION_ERROR, 0, PackedStringArray(), PackedByteArray());
				return true;
			}

			request_sent = true;
			return false;
		}
	} break; // Connected: break requests only accepted here.
	case HTTPClient::STATUS_REQUESTING: {
		// Must wait, still requesting.
		client->poll();
		return false;

	} break; // Request in progress.
	case HTTPClient::STATUS_BODY: {
		if (!got_response) {
			bool ret_value;

			if (_handle_response(&ret_value)) {
				return ret_value;
			}

			if (!client->is_response_chunked() && client->get_response_body_length() == 0) {
				_defer_done(RESULT_SUCCESS, response_code, response_headers, PackedByteArray());
				return true;
			}

			// No body len (-1) if chunked or no content-length header was provided.
			// Change your webserver configuration if you want body len.
			body_len = client->get_response_body_length();

			if (body_size_limit >= 0 && body_len > body_size_limit) {
				_defer_done(RESULT_BODY_SIZE_LIMIT_EXCEEDED, response_code, response_headers,
					PackedByteArray());
				return true;
			}

			if (!download_to_file.is_empty()) {
				if (append_to_download_file &&
					response_code == HTTPClient::RESPONSE_PARTIAL_CONTENT) {
					file = FileAccess::open(download_to_file, FileAccess::READ_WRITE);
					if (file.is_valid()) {
						file->seek_end();
					}
				}
				else {
					file = FileAccess::open(download_to_file, FileAccess::WRITE);
				}
				if (file.is_null()) {
					_defer_done(RESULT_DOWNLOAD_FILE_CANT_OPEN, response_code, response_headers,
						PackedByteArray());
					return true;
				}
			}
		}

		client->poll();
		if (client->get_status() != HTTPClient::STATUS_BODY) {
			return false;
		}

		PackedByteArray chunk;
		if (decompressor.is_null()) {
			// Chunk can be read directly.
			chunk = client->read_response_body_chunk();
			downloaded.add(chunk.size());
		}
		else {
			// Chunk is the result of decompression.
			PackedByteArray compressed = client->read_response_body_chunk();
			downloaded.add(compressed.size());

			int pos = 0;
			int left = compressed.size();
			while (left) {
				int w = 0;
				Error err = decompressor->put_partial_data(compressed.ptr() + pos, left, w);
				if (err == OK) {
					PackedByteArray dc;
					dc.resize(decompressor->get_available_bytes());
					err = decompressor->get_data(dc.ptrw(), dc.size());
					chunk.append_array(dc);
				}
				if (err != OK) {
					_defer_done(RESULT_BODY_DECOMPRESS_FAILED, response_code, response_headers,
						PackedByteArray());
					return true;
				}
				// We need this check here because a "zip bomb" could result in a chunk of few kilos
				// decompressing into gigabytes of data.
				if (body_size_limit >= 0 &&
					final_body_size.get() + chunk.size() > body_size_limit) {
					_defer_done(RESULT_BODY_SIZE_LIMIT_EXCEEDED, response_code, response_headers,
						PackedByteArray());
					return true;
				}
				pos += w;
				left -= w;
			}
		}
		final_body_size.add(chunk.size());

		if (body_size_limit >= 0 && final_body_size.get() > body_size_limit) {
			_defer_done(RESULT_BODY_SIZE_LIMIT_EXCEEDED, response_code, response_headers,
				PackedByteArray());
			return true;
		}

		if (chunk.size()) {
			if (file.is_valid()) {
				const uint8_t* r = chunk.ptr();
				file->store_buffer(r, chunk.size());
				if (file->get_error() != OK) {
					_defer_done(RESULT_DOWNLOAD_FILE_WRITE_ERROR, response_code, response_headers,
						PackedByteArray());
					return true;
				}
			}
			else {
				body.append_array(chunk);
			}
		}

		if (body_len >= 0) {
			if (downloaded.get() == body_len) {
				download_complete = true;
				_defer_done(RESULT_SUCCESS, response_code, response_headers, body);
				return true;
			}
		}
		else if (client->get_status() == HTTPClient::STATUS_DISCONNECTED) {
			// We read till EOF, with no errors. Request is done.
			download_complete = true;
			_defer_done(RESULT_SUCCESS, response_code, response_headers, body);
			return true;
		}

		return false;

	} break; // Request resulted in body: break which must be read.
	case HTTPClient::STATUS_CONNECTION_ERROR: {
		_defer_done(RESULT_CONNECTION_ERROR, 0, PackedStringArray(), PackedByteArray());
		return true;
	} break;
	case HTTPClient::STATUS_TLS_HANDSHAKE_ERROR: {
		_defer_done(RESULT_TLS_HANDSHAKE_ERROR, 0, PackedStringArray(), PackedByteArray());
		return true;
	} break;
	}

	ERR_FAIL_V(false);
}

void HTTPRequest::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_INTERNAL_PROCESS: {
		if (use_threads.is_set()) {
			return;
		}
		bool done = _update_connection();
		if (done) {
			set_process_internal(false);
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (requesting) {
			cancel_request();
		}
	} break;
	}
}

void HTTPRequest::set_use_threads(bool p_use)
{
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);
#ifdef THREADS_ENABLED
	use_threads.set_to(p_use);
#endif
}

bool HTTPRequest::is_using_threads() const { return use_threads.is_set(); }

void HTTPRequest::set_accept_gzip(bool p_gzip) { accept_gzip = p_gzip; }

bool HTTPRequest::is_accepting_gzip() const { return accept_gzip; }

void HTTPRequest::set_body_size_limit(int p_bytes)
{
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);

	body_size_limit = p_bytes;
}

int HTTPRequest::get_body_size_limit() const { return body_size_limit; }

void HTTPRequest::set_download_file(const String& p_file)
{
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);

	download_to_file = p_file;
}

String HTTPRequest::get_download_file() const { return download_to_file; }

void HTTPRequest::set_keep_partial_download(bool p_keep) { keep_partial_download = p_keep; }

bool HTTPRequest::is_keeping_partial_download() const { return keep_partial_download; }

void HTTPRequest::set_append_to_download_file(bool p_append) { append_to_download_file = p_append; }

bool HTTPRequest::is_appending_to_download_file() const { return append_to_download_file; }

void HTTPRequest::set_download_chunk_size(int p_chunk_size)
{
	ERR_FAIL_COND(get_http_client_status() != HTTPClient::STATUS_DISCONNECTED);

	client->set_read_chunk_size(p_chunk_size);
}

int HTTPRequest::get_download_chunk_size() const { return client->get_read_chunk_size(); }

HTTPClient::Status HTTPRequest::get_http_client_status() const { return client->get_status(); }

void HTTPRequest::set_max_redirects(int p_max) { max_redirects = p_max; }

int HTTPRequest::get_max_redirects() const { return max_redirects; }

int HTTPRequest::get_downloaded_bytes() const { return downloaded.get(); }

int HTTPRequest::get_body_size() const { return body_len; }

void HTTPRequest::set_http_proxy(const String& p_host, int p_port)
{
	client->set_http_proxy(p_host, p_port);
}

void HTTPRequest::set_https_proxy(const String& p_host, int p_port)
{
	client->set_https_proxy(p_host, p_port);
}

void HTTPRequest::set_timeout(double p_timeout)
{
	ERR_FAIL_COND(p_timeout < 0);
	timeout = p_timeout;
}

double HTTPRequest::get_timeout() { return timeout; }

void HTTPRequest::_timeout()
{
	cancel_request();
	_defer_done(RESULT_TIMEOUT, 0, PackedStringArray(), PackedByteArray());
}

void HTTPRequest::set_tls_options(const Ref<TLSOptions>& p_options)
{
	ERR_FAIL_COND(p_options.is_null() || p_options->is_server());
	tls_options = p_options;
}

void HTTPRequest::_defer_done(int p_status, int p_code, const PackedStringArray& p_headers,
	const PackedByteArray& p_data) {}
