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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "scene/animation/animation_mixer.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/main/node.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/environment.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/packed_scene.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#ifndef _3D_DISABLED
#include "scene/3d/node_3d.h"
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
#include "core/math/geometry_3d.h"
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

#include "scene/main/scene_tree.h"

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
#include "scene/main/scene_tree.h"

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
#include "core/io/stream_peer.h"
#include "core/os/os.h"
#include "core/os/thread.h"
// --- Inlined from scene/main/http_request.h ---
#include "core/io/stream_peer.h"
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
#include "scene/main/scene_tree.h"

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

// ============================================================================
// Merged from scene/main/node.cpp
// ============================================================================

#include "core/templates/vector.h"
#include "core/types.h"
#include "node.h"
#include "scene/resources/environment.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/string/print_string.h"
#include "scene/animation/animation_mixer.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/accessibility_server.h"

#ifdef DEBUG_ENABLED
#endif

#ifdef DEBUG_ENABLED
SafeNumeric<uint64_t> Node::total_node_count{0};
#endif

thread_local Node* Node::current_process_thread_group = nullptr;

void Node::_propagate_physics_interpolation_reset_requested(bool p_requested)
{
	if (is_physics_interpolated()) {
		data.physics_interpolation_reset_requested = p_requested;
	}

	data.blocked++;
	for (KeyValue<String, Node*>& K : data.children) {
		K.value->_propagate_physics_interpolation_reset_requested(p_requested);
	}
	data.blocked--;
}

void Node::move_child(Node* rp_child, int p_index)
{
	ERR_FAIL_COND_MSG(data.inside_tree && !Thread::is_main_thread(),
		"Moving child node positions inside the SceneTree is only allowed from the main thread. "
		"Use call_deferred(\"move_child\",child,index).");

	_update_children_cache();
	// We need to check whether node is internal and move it only in the relevant node range.
	if (rp_child->data.internal_mode == INTERNAL_MODE_FRONT) {
		if (p_index < 0) {
			p_index += data.internal_children_front_count_cache;
		}
		ERR_FAIL_INDEX_MSG(p_index, data.internal_children_front_count_cache,
			vformat("Invalid new child index: %d. Child is internal.", p_index));
		_move_child(rp_child, p_index);
	}
	else if (rp_child->data.internal_mode == INTERNAL_MODE_BACK) {
		if (p_index < 0) {
			p_index += data.internal_children_back_count_cache;
		}
		ERR_FAIL_INDEX_MSG(p_index, data.internal_children_back_count_cache,
			vformat("Invalid new child index: %d. Child is internal.", p_index));
		_move_child(rp_child,
			(int)data.children_cache.size() - data.internal_children_back_count_cache + p_index);
	}
	else {
		if (p_index < 0) {
			p_index += get_child_count(false);
		}
		ERR_FAIL_INDEX_MSG(p_index,
			(int)data.children_cache.size() + 1 - data.internal_children_front_count_cache -
				data.internal_children_back_count_cache,
			vformat("Invalid new child index: %d.", p_index));
		_move_child(rp_child, p_index + data.internal_children_front_count_cache);
	}
}

void Node::_propagate_groups_dirty()
{
	for (const KeyValue<String, GroupData>& E : data.grouped) {
		if (E.value.group) {
			E.value.group->changed = true;
		}
	}

	for (KeyValue<String, Node*>& K : data.children) {
		K.value->_propagate_groups_dirty();
	}
}

void Node::add_child_notify(Node* p_child)
{
	// to be used when not wanted
}

void Node::remove_child_notify(Node* p_child)
{
	// to be used when not wanted
}

void Node::move_child_notify(Node* p_child)
{
	// to be used when not wanted
}

void Node::owner_changed_notify() {}

void Node::_physics_interpolated_changed() {}

void Node::set_physics_process(bool p_process)
{
	ERR_THREAD_GUARD
	if (data.physics_process == p_process) {
		return;
	}

	if (!is_inside_tree()) {
		data.physics_process = p_process;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.physics_process = p_process;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

bool Node::is_physics_processing() const { return data.physics_process; }

void Node::set_physics_process_internal(bool p_process_internal)
{
	ERR_THREAD_GUARD
	if (data.physics_process_internal == p_process_internal) {
		return;
	}

	if (!is_inside_tree()) {
		data.physics_process_internal = p_process_internal;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.physics_process_internal = p_process_internal;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

bool Node::is_physics_processing_internal() const { return data.physics_process_internal; }

Node::ProcessMode Node::get_process_mode() const { return data.process_mode; }

void Node::set_multiplayer_authority(int p_peer_id, bool p_recursive)
{
	ERR_THREAD_GUARD
	data.multiplayer_authority = p_peer_id;

	if (p_recursive) {
		for (KeyValue<String, Node*>& K : data.children) {
			K.value->set_multiplayer_authority(p_peer_id, true);
		}
	}
}

int Node::get_multiplayer_authority() const { return data.multiplayer_authority; }

bool Node::is_multiplayer_authority() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), false);

	Ref<MultiplayerAPI> api = get_multiplayer();
	return api.is_valid() && (api->get_unique_id() == data.multiplayer_authority);
}

Ref<MultiplayerAPI> Node::get_multiplayer() const
{
	if (!is_inside_tree()) {
		return Ref<MultiplayerAPI>();
	}
	return SceneTree::get_multiplayer(get_path());
}

bool Node::can_process_notification(int p_what) const
{
	switch (p_what) {
	case NOTIFICATION_PHYSICS_PROCESS:
		return data.physics_process;
	case NOTIFICATION_PROCESS:
		return data.process;
	case NOTIFICATION_INTERNAL_PROCESS:
		return data.process_internal;
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS:
		return data.physics_process_internal;
	}

	return true;
}

bool Node::can_process() const
{
	return is_inside_tree() && !SceneTree::is_suspended() && _can_process(SceneTree::is_paused());
}

bool Node::_can_process(bool p_paused) const
{
	ProcessMode process_mode;

	if (data.process_mode == PROCESS_MODE_INHERIT) {
		if (!data.process_owner) {
			process_mode = PROCESS_MODE_PAUSABLE;
		}
		else {
			process_mode = data.process_owner->data.process_mode;
		}
	}
	else {
		process_mode = data.process_mode;
	}

	// The owner can't be set to inherit, must be a bug.
	ERR_FAIL_COND_V(process_mode == PROCESS_MODE_INHERIT, false);

	if (process_mode == PROCESS_MODE_DISABLED) {
		return false;
	}
	else if (process_mode == PROCESS_MODE_ALWAYS) {
		return true;
	}

	if (p_paused) {
		return process_mode == PROCESS_MODE_WHEN_PAUSED;
	}
	else {
		return process_mode == PROCESS_MODE_PAUSABLE;
	}
}

void Node::set_physics_interpolation_mode(PhysicsInterpolationMode p_mode)
{
	ERR_THREAD_GUARD
	if (data.physics_interpolation_mode == p_mode) {
		return;
	}
	data.physics_interpolation_mode = p_mode;
	if (p_mode == PHYSICS_INTERPOLATION_MODE_OFF) {
		data.physics_interpolated = false;
	} else if (p_mode == PHYSICS_INTERPOLATION_MODE_ON) {
		data.physics_interpolated = true;
	}
	_physics_interpolated_changed();
}

bool Node::is_physics_interpolated_and_enabled() const
{
	return SceneTree::is_fti_enabled() && is_physics_interpolated();
}

void Node::reset_physics_interpolation()
{
	if (SceneTree::is_fti_enabled() && is_inside_tree()) {
		propagate_notification(NOTIFICATION_RESET_PHYSICS_INTERPOLATION);

		// If `reset_physics_interpolation()` is called explicitly by the user
		// (e.g. from scripts) then we prevent deferred auto-resets taking place.
		// The user is trusted to call reset in the right order, and auto-reset
		// will interfere with their control of prev / curr, so should be turned off.
		_propagate_physics_interpolation_reset_requested(false);
	}
}

bool Node::_is_enabled() const
{
	ProcessMode process_mode;

	if (data.process_mode == PROCESS_MODE_INHERIT) {
		if (!data.process_owner) {
			process_mode = PROCESS_MODE_PAUSABLE;
		}
		else {
			process_mode = data.process_owner->data.process_mode;
		}
	}
	else {
		process_mode = data.process_mode;
	}

	return (process_mode != PROCESS_MODE_DISABLED);
}

bool Node::is_enabled() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), false);
	return _is_enabled();
}

double Node::get_physics_process_delta_time() const
{
	if (data.inside_tree) {
		return SceneTree::get_physics_process_time();
	}
	else {
		return 0;
	}
}

double Node::get_process_delta_time() const
{
	if (data.inside_tree) {
		return SceneTree::get_process_time();
	}
	else {
		return 0;
	}
}

void Node::set_process(bool p_process)
{
	ERR_THREAD_GUARD
	if (data.process == p_process) {
		return;
	}

	if (!is_inside_tree()) {
		data.process = p_process;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.process = p_process;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

bool Node::is_processing() const { return data.process; }

void Node::set_process_internal(bool p_process_internal)
{
	ERR_THREAD_GUARD
	if (data.process_internal == p_process_internal) {
		return;
	}

	if (!is_inside_tree()) {
		data.process_internal = p_process_internal;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.process_internal = p_process_internal;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

void Node::_add_process_group() { SceneTree::_add_process_group(this); }

void Node::_remove_process_group() { SceneTree::_remove_process_group(this); }

void Node::_remove_from_process_thread_group()
{
	SceneTree::_remove_node_from_process_group(this, data.process_thread_group_owner);
}

void Node::_add_to_process_thread_group()
{
	SceneTree::_add_node_to_process_group(this, data.process_thread_group_owner);
}

void Node::_remove_tree_from_process_thread_group()
{
	if (!is_inside_tree()) {
		return; // May not be initialized yet.
	}

	for (KeyValue<String, Node*>& K : data.children) {
		if (K.value->data.process_thread_group != PROCESS_THREAD_GROUP_INHERIT) {
			continue;
		}

		K.value->_remove_tree_from_process_thread_group();
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}
}

void Node::_add_tree_to_process_thread_group(Node* p_owner)
{
	data.process_thread_group_owner = p_owner;
	if (p_owner != nullptr) {
		data.process_group = p_owner->data.process_group;
	}
	else {
		data.process_group = SceneTree::data ? &SceneTree::data->default_process_group : nullptr;
	}

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}

	for (KeyValue<String, Node*>& K : data.children) {
		if (K.value->data.process_thread_group != PROCESS_THREAD_GROUP_INHERIT) {
			continue;
		}

		K.value->_add_tree_to_process_thread_group(p_owner);
	}
}

bool Node::is_processing_internal() const { return data.process_internal; }

void Node::set_process_thread_group_order(int p_order)
{
	ERR_THREAD_GUARD
	if (data.process_thread_group_order == p_order) {
		return;
	}

	data.process_thread_group_order = p_order;

	// Not yet in the tree (or not a group owner, in whose case this is pointless but harmless);
	// trivial update.
	if (!is_inside_tree() || data.process_thread_group_owner != this) {
		return;
	}

	if (SceneTree::data) {
		SceneTree::data->process_groups_dirty = true;
	}
}

int Node::get_process_thread_group_order() const { return data.process_thread_group_order; }

void Node::set_process_priority(int p_priority)
{
	ERR_THREAD_GUARD
	if (data.process_priority == p_priority) {
		return;
	}
	if (!is_inside_tree()) {
		// Not yet in the tree; trivial update.
		data.process_priority = p_priority;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.process_priority = p_priority;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

int Node::get_process_priority() const { return data.process_priority; }

void Node::set_physics_process_priority(int p_priority)
{
	ERR_THREAD_GUARD
	if (data.physics_process_priority == p_priority) {
		return;
	}
	if (!is_inside_tree()) {
		// Not yet in the tree; trivial update.
		data.physics_process_priority = p_priority;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.physics_process_priority = p_priority;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

int Node::get_physics_process_priority() const { return data.physics_process_priority; }

Node::ProcessThreadGroup Node::get_process_thread_group() const
{
	return data.process_thread_group;
}

void Node::set_process_thread_messages(uint32_t p_flags)
{
	ERR_THREAD_GUARD
	if (data.process_thread_messages == p_flags) {
		return;
	}

	data.process_thread_messages = p_flags;
}

uint32_t Node::get_process_thread_messages() const { return data.process_thread_messages; }

bool Node::is_processing_input() const { return data.input; }

bool Node::is_processing_shortcut_input() const { return data.shortcut_input; }

bool Node::is_processing_unhandled_input() const { return data.unhandled_input; }

bool Node::is_processing_unhandled_key_input() const { return data.unhandled_key_input; }

void Node::set_auto_translate_mode(AutoTranslateMode p_mode)
{
	ERR_THREAD_GUARD
	if (data.auto_translate_mode == p_mode) {
		return;
	}

	if (p_mode == AUTO_TRANSLATE_MODE_INHERIT && data.inside_tree && !data.parent) {
		ERR_FAIL_MSG("The root node can't be set to Inherit auto translate mode.");
	}

	data.auto_translate_mode = p_mode;
	data.is_auto_translating = p_mode != AUTO_TRANSLATE_MODE_DISABLED;
	data.is_auto_translate_dirty = true;

	propagate_notification(NOTIFICATION_TRANSLATION_CHANGED);
}

Node::AutoTranslateMode Node::get_auto_translate_mode() const { return data.auto_translate_mode; }

bool Node::can_auto_translate() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (!data.is_auto_translate_dirty || data.auto_translate_mode != AUTO_TRANSLATE_MODE_INHERIT) {
		return data.is_auto_translating;
	}

	data.is_auto_translate_dirty = false;

	Node* parent = data.parent;
	while (parent) {
		if (parent->data.auto_translate_mode == AUTO_TRANSLATE_MODE_INHERIT) {
			parent = parent->data.parent;
			continue;
		}

		data.is_auto_translating = parent->data.auto_translate_mode == AUTO_TRANSLATE_MODE_ALWAYS;
		break;
	}

	return data.is_auto_translating;
}

void Node::set_translation_domain_inherited()
{
	ERR_THREAD_GUARD

	if (data.is_translation_domain_inherited) {
		return;
	}
	data.is_translation_domain_inherited = true;
	data.is_translation_domain_dirty = true;
	_propagate_translation_domain_dirty();
}

String Node::get_name() const { return data.name; }

void Node::_set_name_nocheck(const String& p_name) { data.name = p_name; }

static SafeRefCount node_hrcr_count;

void Node::queue_free() {}

void Node::init_node_hrcr() { node_hrcr_count.init(1); }

#ifdef TOOLS_ENABLED
String Node::validate_child_name(Node* p_child)
{
	String name = p_child->data.name;
	_generate_serial_child_name(p_child, name);
	return name;
}

String Node::prevalidate_child_name(Node* p_child, String p_name)
{
	_generate_serial_child_name(p_child, p_name);
	return p_name;
}
#endif

// Return s + 1 as if it were an integer
String increase_numeric_string(const String& s)
{
	String res = s;
	bool carry = res.length() > 0;

	for (int i = res.length() - 1; i >= 0; i--) {
		if (!carry) {
			break;
		}
		char32_t n = s[i];
		if (n == '9') { // keep carry as true: 9 + 1
			res[i] = '0';
		}
		else {
			res[i] = s[i] + 1;
			carry = false;
		}
	}

	if (carry) {
		res = "1" + res;
	}

	return res;
}

Node::InternalMode Node::get_internal_mode() const { return data.internal_mode; }

void Node::add_child(Node* rp_child, bool p_force_readable_name, InternalMode p_internal)
{
	ERR_FAIL_COND_MSG(data.inside_tree && !Thread::is_main_thread(),
		"Adding children to a node inside the SceneTree is only allowed from the main thread. Use "
		"call_deferred(\"add_child\",node).");

	ERR_THREAD_GUARD
	ERR_FAIL_COND_MSG(rp_child->data.parent,
		vformat("Can't add child '%s' to '%s', already has a parent '%s'.", rp_child->get_name(),
			get_name(), rp_child->data.parent->get_name())); // Fail if node has a parent
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(rp_child->is_ancestor_of(this),
		vformat("Can't add child '%s' to '%s' as it would result in a cyclic dependency since '%s' "
				"is already a parent of '%s'.",
			rp_child->get_name(), get_name(), rp_child->get_name(), get_name()));
#endif
	ERR_FAIL_COND_MSG(data.blocked > 0,
		"Parent node is busy setting up children, `add_child()` failed. Consider "
		"using `add_child.call_deferred(child)` instead.");

	_validate_child_name(rp_child, p_force_readable_name);

#ifdef DEBUG_ENABLED
	if (rp_child->data.owner && !rp_child->data.owner->is_ancestor_of(rp_child)) {
		// Owner of rp_child should be ancestor of rp_child.
		WARN_PRINT(vformat("Adding '%s' as child to '%s' will make owner '%s' inconsistent. "
						   "Consider unsetting the owner beforehand.",
			rp_child->get_name(), get_name(), rp_child->data.owner->get_name()));
	}
#endif // DEBUG_ENABLED

	_add_child_nocheck(rp_child, rp_child->data.name, p_internal);
}

void Node::remove_child(Node* rp_child) {}

void Node::add_sibling(Node* rp_sibling, bool p_force_readable_name)
{
	ERR_FAIL_COND_MSG(data.inside_tree && !Thread::is_main_thread(),
		"Adding a sibling to a node inside the SceneTree is only allowed from the main thread. Use "
		"call_deferred(\"add_sibling\",node).");
	data.parent->add_child(rp_sibling, p_force_readable_name, data.internal_mode);
	data.parent->_update_children_cache();
	data.parent->_move_child(rp_sibling, get_index() + 1);
}

void Node::_update_children_cache_impl() const
{
	// Assign children
	data.children_cache.resize(data.children.size());
	int idx = 0;
	for (const KeyValue<String, Node*>& K : data.children) {
		data.children_cache[idx] = K.value;
		idx++;
	}
	// Sort them
	data.children_cache.sort_custom<ComparatorByIndex>();
	// Update indices
	data.external_children_count_cache = 0;
	data.internal_children_back_count_cache = 0;
	data.internal_children_front_count_cache = 0;

	for (uint32_t i = 0; i < data.children_cache.size(); i++) {
		switch (data.children_cache[i]->data.internal_mode) {
		case INTERNAL_MODE_DISABLED: {
			data.children_cache[i]->data.index = data.external_children_count_cache++;
		} break;
		case INTERNAL_MODE_FRONT: {
			data.children_cache[i]->data.index = data.internal_children_front_count_cache++;
		} break;
		case INTERNAL_MODE_BACK: {
			data.children_cache[i]->data.index = data.internal_children_back_count_cache++;
		} break;
		}
	}
	data.children_cache_dirty = false;
}

template <bool p_include_internal> Iterable<Node::ChildrenIterator> Node::iterate_children() const
{
	// The thread guard is omitted for performance reasons.
	// ERR_THREAD_GUARD_V(Iterable<ChildrenIterator>(nullptr, nullptr));

	_update_children_cache();
	const uint32_t size = data.children_cache.size();
	// Might be null, but then size and internal counts are also 0.
	Node** ptr = data.children_cache.ptr();

	if constexpr (p_include_internal) {
		return Iterable(ChildrenIterator(ptr), ChildrenIterator(ptr + size));
	}
	else {
		return Iterable(ChildrenIterator(ptr + data.internal_children_front_count_cache),
			ChildrenIterator(ptr + size - data.internal_children_back_count_cache));
	}
}

template Iterable<Node::ChildrenIterator> Node::iterate_children<true>() const;
template Iterable<Node::ChildrenIterator> Node::iterate_children<false>() const;

int Node::get_child_count(bool p_include_internal) const
{
	ERR_THREAD_GUARD_V(0);
	if (p_include_internal) {
		return data.children.size();
	}

	_update_children_cache();
	return data.children_cache.size() - data.internal_children_front_count_cache -
		   data.internal_children_back_count_cache;
}

Node* Node::get_child(int p_index, bool p_include_internal) const
{
	ERR_THREAD_GUARD_V(nullptr);
	_update_children_cache();

	if (p_include_internal) {
		if (p_index < 0) {
			p_index += data.children_cache.size();
		}
		ERR_FAIL_INDEX_V(p_index, (int)data.children_cache.size(), nullptr);
		return data.children_cache[p_index];
	}
	else {
		if (p_index < 0) {
			p_index += (int)data.children_cache.size() - data.internal_children_front_count_cache -
					   data.internal_children_back_count_cache;
		}
		ERR_FAIL_INDEX_V(p_index,
			(int)data.children_cache.size() - data.internal_children_front_count_cache -
				data.internal_children_back_count_cache,
			nullptr);
		p_index += data.internal_children_front_count_cache;
		return data.children_cache[p_index];
	}
}

Vector<Node*> Node::get_children(bool p_include_internal) const
{
	ERR_THREAD_GUARD_V(Vector<Node*>());
	_update_children_cache();

	Vector<Node*> children;

	if (p_include_internal) {
		children.resize(data.children_cache.size());
		Vector<Node*>::Iterator itr = children.begin();
		for (Node* child : data.children_cache) {
			*itr = child;
			++itr;
		}
	}
	else {
		const int size = data.children_cache.size() - data.internal_children_back_count_cache;
		children.resize(size - data.internal_children_front_count_cache);

		Vector<Node*>::Iterator itr = children.begin();
		for (int i = data.internal_children_front_count_cache; i < size; i++) {
			*itr = data.children_cache[i];
			++itr;
		}
	}

	return children;
}

Node* Node::_get_child_by_name(const String& p_name) const
{
	const Node* const* node = data.children.getptr(p_name);
	if (node) {
		return const_cast<Node*>(*node);
	}
	else {
		return nullptr;
	}
}

Node* Node::get_node_or_null(const NodePath& p_path) const
{
	ERR_THREAD_GUARD_V(nullptr);
	if (p_path.is_empty()) {
		return nullptr;
	}

	ERR_FAIL_COND_V_MSG(!data.inside_tree && p_path.is_absolute(), nullptr,
		"Can't use get_node() with absolute paths from outside the active scene tree.");

	Node* current = nullptr;
	Node* root = nullptr;

	if (!p_path.is_absolute()) {
		current = const_cast<Node*>(this); // start from this
	}
	else {
		root = const_cast<Node*>(this);
		while (root->data.parent) {
			root = root->data.parent; // start from root
		}
	}

	for (int i = 0; i < p_path.get_name_count(); i++) {
		String name = p_path.get_name(i);
		Node* next = nullptr;

		if (name == SNAME(".")) {
			next = current;

		}
		else if (name == SNAME("..")) {
			if (current == nullptr || !current->data.parent) {
				return nullptr;
			}

			next = current->data.parent;
		}
		else if (current == nullptr) {
			if (name == root->get_name()) {
				next = root;
			}

		}
		else if (name.is_node_unique_name()) {
			Node** unique = current->data.owned_unique_nodes.getptr(name);
			if (!unique && current->data.owner) {
				unique = current->data.owner->data.owned_unique_nodes.getptr(name);
			}
			if (!unique) {
				return nullptr;
			}
			next = *unique;
		}
		else {
			next = nullptr;
			const Node* const* node = current->data.children.getptr(name);
			if (node) {
				next = const_cast<Node*>(*node);
			}
			else {
				return nullptr;
			}
		}
		current = next;
	}

	return current;
}

Node* Node::get_node(const NodePath& p_path) const
{
	Node* node = get_node_or_null(p_path);

	if (unlikely(!node)) {
		const String desc = "";
		if (p_path.is_absolute()) {
			ERR_FAIL_V_MSG(
				nullptr, vformat(R"(Node not found: "%s" (absolute path attempted from "%s").)",
							 p_path, desc));
		}
		else {
			ERR_FAIL_V_MSG(
				nullptr, vformat(R"(Node not found: "%s" (relative to "%s").)", p_path, desc));
		}
	}

	return node;
}

bool Node::has_node(const NodePath& p_path) const { return get_node_or_null(p_path) != nullptr; }

// Finds the first child node (in tree order) whose name matches the given pattern.
// Can be recursive or not, and limited to owned nodes.
Node* Node::find_child(const String& p_pattern, bool p_recursive, bool p_owned) const
{
	ERR_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(p_pattern.is_empty(), nullptr);
	_update_children_cache();
	Node* const* cptr = data.children_cache.ptr();
	int ccount = data.children_cache.size();
	for (int i = 0; i < ccount; i++) {
		if (p_owned && !cptr[i]->data.owner) {
			continue;
		}
		if (cptr[i]->data.name.match(p_pattern)) {
			return cptr[i];
		}

		if (!p_recursive) {
			continue;
		}

		Node* ret = cptr[i]->find_child(p_pattern, true, p_owned);
		if (ret) {
			return ret;
		}
	}
	return nullptr;
}

void Node::reparent(Node* rp_parent, bool p_keep_global_transform)
{
	ERR_THREAD_GUARD
	ERR_FAIL_NULL_MSG(data.parent, "Node needs a parent to be reparented.");
	ERR_FAIL_COND_MSG(
		rp_parent == this, vformat("Can't reparent '%s' to itself.", rp_parent->get_name()));

	if (rp_parent == data.parent) {
		return;
	}

	bool preserve_owner =
		data.owner && (data.owner == rp_parent || data.owner->is_ancestor_of(rp_parent));
	Node* owner_temp = data.owner;
	LocalVector<Node*> common_parents;

	// If the new parent is related to the owner, find all children of the reparented node who have
	// the same owner so that we can reassign them.
	if (preserve_owner) {
		LocalVector<Node*> to_visit;

		to_visit.push_back(this);
		common_parents.push_back(this);

		while (to_visit.size() > 0) {
			Node* check = to_visit[to_visit.size() - 1];
			to_visit.resize(to_visit.size() - 1);

			for (int i = 0; i < check->get_child_count(false); i++) {
				Node* child = check->get_child(i, false);
				to_visit.push_back(child);
				if (child->data.owner == owner_temp) {
					common_parents.push_back(child);
				}
			}
		}
	}

	data.parent->remove_child(this);
	rp_parent->add_child(this);
	// Reassign the old owner to those found nodes.
	if (preserve_owner) {
		for (Node* E : common_parents) {
			E->set_owner(owner_temp);
		}
	}
}

Node* Node::get_parent() const { return data.parent; }

Node* Node::find_parent(const String& p_pattern) const
{
	ERR_THREAD_GUARD_V(nullptr);
	Node* p = data.parent;
	while (p) {
		if (p->data.name.match(p_pattern)) {
			return p;
		}
		p = p->data.parent;
	}

	return nullptr;
}

void Node::set_unique_scene_id(int32_t p_unique_id) { data.unique_scene_id = p_unique_id; }

int32_t Node::get_unique_scene_id() const { return data.unique_scene_id; }

Window* Node::get_window() const
{
	ERR_THREAD_GUARD_V(nullptr);
	Viewport* vp = get_viewport();
	if (vp) {
		return vp->get_base_window();
	}
	return nullptr;
}

Window* Node::get_non_popup_window() const
{
	Window* w = get_window();
	while (w && w->is_popup()) {
		w = w->get_parent_visible_window();
	}
	return w;
}

Window* Node::get_last_exclusive_window() const
{
	Window* w = get_window();
	while (w && w->get_exclusive_child()) {
		w = w->get_exclusive_child();
	}

	return w;
}

bool Node::is_ancestor_of(const Node* rp_node) const
{
	const Node* n = rp_node;

	if (is_inside_tree() && rp_node->data.inside_tree) {
		const int depth = data.depth;
		while (n->data.depth > depth) {
			n = n->data.parent;
		}
		return n == this;
	}

	n = rp_node->data.parent;
	while (n) {
		if (n == this) {
			return true;
		}
		n = n->data.parent;
	}

	return false;
}

bool Node::is_greater_than(const Node* rp_node) const
{
	// parent->get_child(1) > parent->get_child(0) > parent

	ERR_FAIL_COND_V(!data.inside_tree, false);
	ERR_FAIL_COND_V(data.depth < 0, false);

	_update_children_cache();

	bool this_is_deeper = this->data.depth > rp_node->data.depth;

	const Node* deep = this;
	const Node* shallow = rp_node;
	if (!this_is_deeper) {
		deep = rp_node;
		shallow = this;
	}

	while (deep->data.depth > shallow->data.depth) {
		deep = deep->data.parent;
	}

	if (deep == shallow) { // Shallow is ancestor of deep.
		return this_is_deeper;
	}

	while (deep->data.parent != shallow->data.parent) {
		deep = deep->data.parent;
		shallow = shallow->data.parent;
	}

	return (deep->get_index() > shallow->get_index()) == this_is_deeper;
}

void Node::get_owned_by(Node* p_by, List<Node*>* p_owned)
{
	if (data.owner == p_by) {
		p_owned->push_back(this);
	}

	for (KeyValue<String, Node*>& K : data.children) {
		K.value->get_owned_by(p_by, p_owned);
	}
}

void Node::_set_owner_nocheck(Node* p_owner)
{
	if (data.owner == p_owner) {
		return;
	}

	ERR_FAIL_COND(data.owner);
	data.owner = p_owner;
	data.owner->data.owned.push_back(this);
	data.OW = data.owner->data.owned.back();

	owner_changed_notify();
}

void Node::_release_unique_name_in_owner()
{
	ERR_FAIL_NULL(data.owner); // Safety check.
	String key = String(UNIQUE_NODE_PREFIX + data.name);
	Node** which = data.owner->data.owned_unique_nodes.getptr(key);
	if (which == nullptr || *which != this) {
		return; // Ignore.
	}
	data.owner->data.owned_unique_nodes.erase(key);
}

void Node::_acquire_unique_name_in_owner()
{
	ERR_FAIL_NULL(data.owner); // Safety check.
	String key = String(UNIQUE_NODE_PREFIX + data.name);
	Node** which = data.owner->data.owned_unique_nodes.getptr(key);
	if (which != nullptr && *which != this) {
		String which_path =
			String(is_inside_tree() ? (*which)->get_path() : data.owner->get_path_to(*which));
		WARN_PRINT(
			vformat("Setting node name '%s' to be unique within scene for '%s', but it's already "
					"claimed by '%s'.\n'%s' is no longer set as having a unique name.",
				get_name(), is_inside_tree() ? get_path() : data.owner->get_path_to(this),
				which_path, which_path));
		data.unique_name_in_owner = false;
		return;
	}
	data.owner->data.owned_unique_nodes[key] = this;
}

bool Node::is_unique_name_in_owner() const { return data.unique_name_in_owner; }

void Node::set_owner(Node* p_owner)
{
	ERR_MAIN_THREAD_GUARD
	if (data.owner) {
		_clean_up_owner();
	}

	ERR_FAIL_COND(p_owner == this);

	if (!p_owner) {
		return;
	}

	bool owner_valid = p_owner->is_ancestor_of(this);

	ERR_FAIL_COND_MSG(!owner_valid, "Invalid owner. Owner must be an ancestor in the tree.");

	_set_owner_nocheck(p_owner);

	if (data.unique_name_in_owner) {
		_acquire_unique_name_in_owner();
	}

	_emit_editor_state_changed();
}

Node* Node::get_owner() const { return data.owner; }

void Node::_clean_up_owner()
{
	ERR_FAIL_NULL(data.owner); // Safety check.

	if (data.unique_name_in_owner) {
		_release_unique_name_in_owner();
	}
	data.owner->data.owned.erase(data.OW);
	data.owner = nullptr;
	data.OW = nullptr;
}

Node* Node::find_common_parent_with(const Node* p_node) const
{
	if (this == p_node) {
		return const_cast<Node*>(p_node);
	}

	HashSet<const Node*> visited;

	const Node* n = this;

	while (n) {
		visited.insert(n);
		n = n->data.parent;
	}

	const Node* common_parent = p_node;

	while (common_parent) {
		if (visited.has(common_parent)) {
			break;
		}
		common_parent = common_parent->data.parent;
	}

	if (!common_parent) {
		return nullptr;
	}

	return const_cast<Node*>(common_parent);
}

NodePath Node::get_path_to(const Node* rp_node, bool p_use_unique_path) const
{
	if (this == rp_node) {
		return NodePath(".");
	}

	HashSet<const Node*> visited;

	const Node* n = this;

	while (n) {
		visited.insert(n);
		n = n->data.parent;
	}

	const Node* common_parent = rp_node;

	while (common_parent) {
		if (visited.has(common_parent)) {
			break;
		}
		common_parent = common_parent->data.parent;
	}

	ERR_FAIL_NULL_V_MSG(common_parent, NodePath(),
		vformat("No path can be resolved between the nodes and as they share no common ancestor."));

	visited.clear();

	Vector<String> path;
	String up = String("..");

	if (p_use_unique_path) {
		n = rp_node;

		bool is_detected = false;
		while (n != common_parent) {
			if (n->is_unique_name_in_owner() && n->get_owner() == get_owner()) {
				path.push_back(UNIQUE_NODE_PREFIX + String(n->get_name()));
				is_detected = true;
				break;
			}
			path.push_back(n->get_name());
			n = n->data.parent;
		}

		if (!is_detected) {
			n = this;

			String detected_name;
			int up_count = 0;
			while (n != common_parent) {
				if (n->is_unique_name_in_owner() && n->get_owner() == get_owner()) {
					detected_name = n->get_name();
					up_count = 0;
				}
				up_count++;
				n = n->data.parent;
			}

			for (int i = 0; i < up_count; i++) {
				path.push_back(up);
			}

			if (!detected_name.is_empty()) {
				path.push_back(UNIQUE_NODE_PREFIX + detected_name);
			}
		}
	}
	else {
		n = rp_node;

		while (n != common_parent) {
			path.push_back(n->get_name());
			n = n->data.parent;
		}

		n = this;

		while (n != common_parent) {
			path.push_back(up);
			n = n->data.parent;
		}
	}

	path.reverse();

	return NodePath(path, false);
}

NodePath Node::get_path() const
{
	ERR_FAIL_COND_V_MSG(
		!is_inside_tree(), NodePath(), "Cannot get path of node as it is not in a scene tree.");

	if (data.path_cache) {
		return *data.path_cache;
	}

	const Node* n = this;

	Vector<String> path;
	path.resize(data.depth);

	String* ptrw = path.ptrw();
	while (n) {
		ptrw[n->data.depth - 1] = n->get_name();
		n = n->data.parent;
	}

	data.path_cache = memnew(NodePath(path, true));

	return *data.path_cache;
}

bool Node::is_in_group(const String& p_identifier) const
{
	ERR_THREAD_GUARD_V(false);
	return data.grouped.has(p_identifier);
}

void Node::add_to_group(const String& p_identifier, bool p_persistent)
{
	ERR_THREAD_GUARD
	ERR_FAIL_COND_MSG(p_identifier.is_empty(),
		vformat("Cannot add node '%s' to a group with an empty name.", get_name()));

	if (data.grouped.has(p_identifier)) {
		return;
	}

	GroupData gd;

	if (data.inside_tree) {
		gd.group = SceneTree::add_to_group(p_identifier, this);
	}
	else {
		gd.group = nullptr;
	}

	gd.persistent = p_persistent;

	data.grouped[p_identifier] = gd;
	if (p_persistent) {
		_emit_editor_state_changed();
	}
}

void Node::remove_from_group(const String& p_identifier)
{
	ERR_THREAD_GUARD
	HashMap<String, GroupData>::Iterator E = data.grouped.find(p_identifier);

	if (!E) {
		return;
	}

#ifdef TOOLS_ENABLED
	bool persistent = E->value.persistent;
#endif

	if (data.inside_tree) {
		SceneTree::remove_from_group(E->key, this);
	}

	data.grouped.remove(E);

#ifdef TOOLS_ENABLED
	if (persistent) {
		_emit_editor_state_changed();
	}
#endif
}

void Node::get_groups(List<GroupInfo>* p_groups) const
{
	ERR_THREAD_GUARD
	for (const KeyValue<String, GroupData>& E : data.grouped) {
		GroupInfo gi;
		gi.name = E.key;
		gi.persistent = E.value.persistent;
		p_groups->push_back(gi);
	}
}

int Node::get_persistent_group_count() const
{
	ERR_THREAD_GUARD_V(0);
	int count = 0;

	for (const KeyValue<String, GroupData>& E : data.grouped) {
		if (E.value.persistent) {
			count += 1;
		}
	}

	return count;
}

String Node::_get_tree_string_pretty(const String& p_prefix, bool p_last)
{
	String new_prefix = p_last ? String::utf8(" ┖╴") : String::utf8(" ┠╴");
	_update_children_cache();
	String return_tree = p_prefix + new_prefix + String(get_name()) + "\n";
	for (uint32_t i = 0; i < data.children_cache.size(); i++) {
		new_prefix = p_last ? String::utf8("   ") : String::utf8(" ┃ ");
		return_tree += data.children_cache[i]->_get_tree_string_pretty(
			p_prefix + new_prefix, i == data.children_cache.size() - 1);
	}
	return return_tree;
}

String Node::get_tree_string_pretty() { return _get_tree_string_pretty("", true); }

String Node::_get_tree_string(const Node* p_node)
{
	_update_children_cache();
	String return_tree = String(p_node->get_path_to(this)) + "\n";
	for (uint32_t i = 0; i < data.children_cache.size(); i++) {
		return_tree += data.children_cache[i]->_get_tree_string(p_node);
	}
	return return_tree;
}

String Node::get_tree_string() { return _get_tree_string(this); }

void Node::_propagate_replace_owner(Node* p_owner, Node* p_by_owner)
{
	if (get_owner() == p_owner) {
		set_owner(p_by_owner);
	}

	data.blocked++;
	for (KeyValue<String, Node*>& K : data.children) {
		K.value->_propagate_replace_owner(p_owner, p_by_owner);
	}
	data.blocked--;
}

Tween* Node::create_tween()
{
	ERR_THREAD_GUARD_V(Ref<Tween>().ptr());

	ERR_FAIL_COND_V_MSG(!SceneTree::is_initialized(), Ref<Tween>().ptr(),
		"No available SceneTree to create the Tween.");

	Ref<Tween> tween = SceneTree::create_tween();
	tween->bind_node(this);
	return tween.ptr();
}

void Node::set_scene_file_path(const String& p_scene_file_path)
{
	ERR_THREAD_GUARD
	data.scene_file_path = p_scene_file_path;
	_emit_editor_state_changed();
}

String Node::get_scene_file_path() const { return data.scene_file_path; }

String Node::get_editor_description() const { return data.editor_description; }

void Node::set_editable_instance(Node* rp_node, bool p_editable)
{
	ERR_THREAD_GUARD
	ERR_FAIL_COND(!is_ancestor_of(rp_node));
	if (!p_editable) {
		rp_node->data.editable_instance = false;
		// Avoid this flag being needlessly saved;
		// also give more visual feedback if editable children are re-enabled
		set_display_folded(false);
	}
	else {
		rp_node->data.editable_instance = true;
	}

	rp_node->_emit_editor_state_changed();
}

bool Node::is_editable_instance(const Node* p_node) const
{
	if (!p_node) {
		return false; // Easier, null is never editable. :)
	}
	ERR_FAIL_COND_V(!is_ancestor_of(p_node), false);
	return p_node->data.editable_instance;
}

Node* Node::get_deepest_editable_node(Node* p_start_node) const
{
	ERR_THREAD_GUARD_V(nullptr);
	ERR_FAIL_NULL_V(p_start_node, nullptr);
	ERR_FAIL_COND_V(!is_ancestor_of(p_start_node), p_start_node);

	const Node* iterated_item = p_start_node;
	Node* node = p_start_node;

	while (iterated_item->get_owner() && iterated_item->get_owner() != this) {
		if (!is_editable_instance(iterated_item->get_owner())) {
			node = iterated_item->get_owner();
		}

		iterated_item = iterated_item->get_owner();
	}

	return node;
}

#ifdef TOOLS_ENABLED

String Node::get_property_store_alias(const String& p_property) const { return p_property; }

bool Node::is_part_of_edited_scene() const
{
	Node* edited_root = SceneTree::get_edited_scene_root();
	return Engine::is_editor_hint() && is_inside_tree() && edited_root &&
		   edited_root->get_parent() && // Defend against edge cases when creating new scenes and
										// they are not fully added to the tree yet.
		   edited_root->get_parent()->is_ancestor_of(this);
}
#endif

void Node::set_scene_instance_state(const Ref<SceneState>& p_state)
{
	ERR_THREAD_GUARD
	data.instance_state = p_state;
}

Ref<SceneState> Node::get_scene_instance_state() const { return data.instance_state; }

void Node::set_scene_inherited_state(const Ref<SceneState>& p_state)
{
	ERR_THREAD_GUARD
	data.inherited_state = p_state;
	_emit_editor_state_changed();
}

Ref<SceneState> Node::get_scene_inherited_state() const { return data.inherited_state; }

void Node::set_scene_instance_load_placeholder(bool p_enable) { data.use_placeholder = p_enable; }

bool Node::get_scene_instance_load_placeholder() const { return data.use_placeholder; }

Node* Node::duplicate(int p_flags) const
{
	ERR_THREAD_GUARD_V(nullptr);
	Node* dupe = _duplicate(p_flags);

	ERR_FAIL_NULL_V_MSG(dupe, nullptr, "Failed to duplicate node.");

	if (p_flags & DUPLICATE_SCRIPTS) {
		_duplicate_scripts(this, dupe);
	}

	_duplicate_properties(this, this, dupe, p_flags);

	if (p_flags & DUPLICATE_SIGNALS) {
		_duplicate_signals(this, dupe);
	}

	return dupe;
}

#ifdef TOOLS_ENABLED
Node* Node::duplicate_from_editor(HashMap<const Node*, Node*>& r_duplimap) const
{
	HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>> tmp;
	return duplicate_from_editor(r_duplimap, nullptr, tmp);
}

Node* Node::duplicate_from_editor(HashMap<const Node*, Node*>& r_duplimap, Node* p_scene_root,
	HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& p_resource_remap) const
{
	int flags = DUPLICATE_SIGNALS | DUPLICATE_GROUPS | DUPLICATE_SCRIPTS |
				DUPLICATE_USE_INSTANTIATION | DUPLICATE_FROM_EDITOR;
	Node* dupe = _duplicate(flags, &r_duplimap);

	ERR_FAIL_NULL_V_MSG(dupe, nullptr, "Failed to duplicate node.");

	if (flags & DUPLICATE_SCRIPTS) {
		_duplicate_scripts(this, dupe);
	}

	_duplicate_properties(this, this, dupe, flags);

	// This is used by SceneTreeDock's paste functionality. When pasting to foreign scene, resources
	// are duplicated.
	if (p_scene_root) {
		remap_node_resources(dupe, p_scene_root, p_resource_remap);
	}

	// Duplication of signals must happen after all the node descendants have been copied,
	// because re-targeting of connections from some descendant to another is not possible
	// if the emitter node comes later in tree order than the receiver
	_duplicate_signals(this, dupe);

	return dupe;
}

#endif

static void find_owned_by(Node* p_by, Node* p_node, List<Node*>* p_owned)
{
	if (p_node->get_owner() == p_by) {
		p_owned->push_back(p_node);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		find_owned_by(p_by, p_node->get_child(i), p_owned);
	}
}

bool Node::has_node_and_resource(const NodePath& p_path) const
{
	ERR_THREAD_GUARD_V(false);
	if (!has_node(p_path)) {
		return false;
	}
	Ref<Resource> res;
	Vector<String> leftover_path;
	Node* node = get_node_and_resource(p_path, res, leftover_path, false);

	return node;
}

void Node::_set_tree(bool p_in_tree)
{
	bool tree_changed_a = false;
	bool tree_changed_b = false;

	if (data.inside_tree) {
		_propagate_exit_tree();
		tree_changed_a = true;
	}

	if (p_in_tree) {
		_propagate_enter_tree();
		if (!data.parent || data.parent->data.ready_notified) { // No parent (root) or parent ready
			_propagate_ready(); // reverse_notification(NOTIFICATION_READY);
		}
		tree_changed_b = true;
	}

	if (tree_changed_a && SceneTree::is_initialized()) {
		SceneTree::tree_changed();
	}
	if (tree_changed_b && SceneTree::is_initialized()) {
		SceneTree::tree_changed();
	}
}

#ifdef TOOLS_ENABLED
static void _add_nodes_to_options(const Node* p_base, const Node* p_node, List<String>* r_options)
{
	if (p_node != p_base && !p_node->get_owner()) {
		return;
	}
	if (p_node->is_unique_name_in_owner() && p_node->get_owner() == p_base) {
		String n = "%" + p_node->get_name();
		r_options->push_back(n.quote());
	}
	String n = String(p_base->get_path_to(p_node));
	r_options->push_back(n.quote());
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_add_nodes_to_options(p_base, p_node->get_child(i), r_options);
	}
}

#endif

PackedStringArray Node::get_configuration_warnings() const
{
	ERR_THREAD_GUARD_V(PackedStringArray());
	PackedStringArray ret;
	return ret;
}

void Node::update_configuration_warnings() {}

void Node::set_display_folded(bool p_folded)
{
	ERR_THREAD_GUARD
	data.display_folded = p_folded;
}

bool Node::is_displayed_folded() const { return data.display_folded; }

bool Node::is_ready() const { return !data.ready_first; }

void Node::request_ready()
{
	ERR_THREAD_GUARD
	data.ready_first = true;
}

void Node::_call_input(const InputEvent& p_event)
{
	if (!is_inside_tree() || !get_viewport() || get_viewport()->is_input_handled()) {
		return;
	}
	input(p_event);
}

void Node::_call_unhandled_input(const InputEvent& p_event)
{
	if (!is_inside_tree() || !get_viewport() || get_viewport()->is_input_handled()) {
		return;
	}
	unhandled_input(p_event);
}

String Node::_to_string() { return String(); }

void Node::_call_unhandled_key_input(const InputEvent& p_event)
{
	if (!is_inside_tree() || !get_viewport() || get_viewport()->is_input_handled()) {
		return;
	}
	unhandled_key_input(p_event);
}

void Node::input(const InputEvent& p_event) {}

void Node::unhandled_input(const InputEvent& p_event) {}

void Node::unhandled_key_input(const InputEvent& p_key_event) {}

RID Node::get_focused_accessibility_element() const { return get_accessibility_element(); }

Transform2D Node::get_accessibility_transform() const
{
	if (is_inside_tree() && data.parent) {
		return data.parent->get_accessibility_transform();
	}
	else {
		return Transform2D();
	}
}

void Node::queue_accessibility_update()
{
	if (is_inside_tree() && !is_part_of_edited_scene()) {
		SceneTree::_accessibility_notify_change(this);
	}
}

RID Node::get_accessibility_element() const
{
	if (is_part_of_edited_scene()) {
		return RID();
	}
	if (unlikely(data.accessibility_element.is_null())) {
		Window* w = get_non_popup_window();
		if (w && w->get_window_id() != DisplayServerEnums::INVALID_WINDOW_ID &&
			get_window()->is_visible()) {
			data.accessibility_element = AccessibilityServer::create_element(
				w->get_window_id(), AccessibilityServerEnums::ROLE_CONTAINER);
		}
	}
	return data.accessibility_element;
}

Node::~Node()
{
	data.grouped.clear();
	data.owned.clear();
	data.children.clear();
	data.children_cache.clear();

	ERR_FAIL_COND(data.parent);
	ERR_FAIL_COND(data.children_cache.size());

#ifdef DEBUG_ENABLED
	total_node_count.decrement();
#endif
}

Node::Node()
{
	// Initialize scalar and pointer fields
	data.name = String();
	data.parent = nullptr;
	data.owner = nullptr;
	data.inside_tree = false;
	data.viewport = nullptr;
	data.depth = -1;
	data.index = -1;
	data.blocked = 0;
	data.internal_mode = INTERNAL_MODE_DISABLED;
	data.unique_name_in_owner = false;
	data.instance_state = Ref<SceneState>();
	data.inherited_state = Ref<SceneState>();

	data.children_cache_dirty = true;
	data.internal_children_front_count_cache = 0;
	data.internal_children_back_count_cache = 0;
	data.external_children_count_cache = 0;

	data.process_owner = nullptr;
	data.process_group = nullptr;
	data.process_thread_group_order = 0;
	data.process_thread_messages = 0;
	data.process_thread_group = PROCESS_THREAD_GROUP_INHERIT;
	data.multiplayer_authority = 1;

#ifdef DEBUG_ENABLED
	total_node_count.increment();
#endif
}

String Node::get_description(bool p_show_not_in_tree) const { return String(); }

void Node::set_name(const String& p_name) { data.name = p_name; }

void Node::_emit_editor_state_changed() {}

void Node::_move_child(Node* p_child, int p_index, bool p_ignore_end) {}

void Node::set_process_shortcut_input(bool) {}

void Node::set_process_input(bool) {}

void Node::set_process_mode(ProcessMode p_mode) { data.process_mode = p_mode; }

void Node::get_argument_options(String const&, int, List<String, DefaultAllocator>*) const {}

void Node::notify_deferred_thread_group(int) {}

void Node::_generate_serial_child_name(Node const*, String&) const {}

void Node::_duplicate_signals(Node const*, Node*) const {}

void Node::_duplicate_scripts(Node const*, Node*) const {}

void Node::_duplicate_properties(Node const*, Node const*, Node*, int) const {}

Node* Node::_duplicate(int,
	HashMap<Node const*, Node*, HashMapHasherDefault, HashMapComparatorDefault<Node const*, void>,
		DefaultTypedAllocator<HashMapElement<Node const*, Node*>>>*) const
{
	return nullptr;
}

void Node::set_process_unhandled_key_input(bool) {}

void Node::set_process_unhandled_input(bool) {}

void Node::remap_node_resources(Node* p_node, Node* p_scene_root,
	HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& p_resource_remap) const
{
}

void Node::_propagate_translation_domain_dirty() {}

void Node::_propagate_suspend_notification(bool) {}

void Node::_propagate_pause_notification(bool) {}

void Node::_propagate_enter_tree()
{
	if (data.parent) {
		data.depth = data.parent->data.depth + 1;
	}
	else {
		data.depth = 1;
	}

	data.inside_tree = true;

	// 1. Notify this node that it entered the tree (attaches CanvasItem RID & Control
	// theme/viewport)
	notification(NOTIFICATION_ENTER_TREE);

	// 2. Propagate down to all children
	for (KeyValue<String, Node*>& K : data.children) {
		if (K.value && !K.value->is_inside_tree()) {
			K.value->_propagate_enter_tree();
		}
	}
}

void Node::_propagate_ready()
{
    _update_children_cache();
    for (uint32_t i = 0; i < data.children_cache.size(); i++) {
        Node* c = data.children_cache[i];
        if (c) {
            c->_propagate_ready();
        }
    }

    notification(NOTIFICATION_POST_ENTER_TREE);

    if (data.ready_first) {
        data.ready_first = false;
        notification(NOTIFICATION_READY);
    }
    data.ready_notified = true;
}

void Node::_propagate_exit_tree()
{
    _update_children_cache();
    for (int i = (int)data.children_cache.size() - 1; i >= 0; i--) {
        Node* c = data.children_cache[i];
        if (c && c->is_inside_tree()) {
            c->_propagate_exit_tree();
        }
    }

    notification(NOTIFICATION_EXIT_TREE, true);

    data.inside_tree = false;
    data.ready_notified = false;
    data.depth = -1;
}

void Node::_propagate_after_exit_tree()
{
    _update_children_cache();
    for (uint32_t i = 0; i < data.children_cache.size(); i++) {
        Node* c = data.children_cache[i];
        if (c) {
            c->_propagate_after_exit_tree();
        }
    }
    notification(NOTIFICATION_UNPARENTED);
}

void Node::propagate_notification(int p_notification)
{
    notification(p_notification);
    _update_children_cache();
    for (uint32_t i = 0; i < data.children_cache.size(); i++) {
        if (data.children_cache[i]) {
            data.children_cache[i]->propagate_notification(p_notification);
        }
    }
}

Node* Node::get_node_and_resource(NodePath const&, Ref<Resource>&, Vector<String>&, bool) const
{
	return nullptr;
}

void Node::set_translation_domain(const String& p_domain) {}

void Node::remove_meta(const String& p_name) {}

bool Node::has_signal(const String& p_name) const { return false; }

bool Node::has_meta(const String& p_name) const { return false; }

bool Node::has_connections(const String& p_signal) const { return false; }

String Node::get_translation_domain() const { return String(); }

int Node::get_persistent_signal_connection_count() const { return 0; }

void Node::get_meta_list(List<String>* p_list) const {}

void Node::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_PROCESS: {
		// Virtual process hook if used
	} break;
	case NOTIFICATION_ENTER_TREE: {
		ERR_FAIL_COND(!data.inside_tree);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		ERR_FAIL_COND(!data.inside_tree);
	} break;
	case NOTIFICATION_READY: {
		// Ready hook
	} break;
	}
}

void Node::_validate_child_name(Node* p_child, bool p_force_human_readable)
{
	String name = p_child->data.name;
	if (name.is_empty() || data.children.has(name)) {
		// Generate a guaranteed unique key so HashMap doesn't overwrite unnamed UI children
		static uint64_t auto_id = 0;
		name = vformat("@Node@%d", ++auto_id);
		p_child->data.name = name;
	}
}

void Node::_add_child_nocheck(Node* p_child, const String& p_name, InternalMode p_internal)
{
	p_child->data.name = p_name;
	_validate_child_name(p_child, false);

	p_child->data.parent = this;
	p_child->data.internal_mode = p_internal;

	data.children.insert(p_child->data.name, p_child);
	data.children_cache_dirty = true;

	// If parent is already in the tree, bring the new child (and its subtree) into the tree!
	if (data.inside_tree) {
		p_child->_set_tree(true);
	}

	notification(NOTIFICATION_CHILD_ORDER_CHANGED);
}

// ==========================================================================
// Merged from scene/main/instance_placeholder.cpp
// ==========================================================================

#include "core/io/resource_loader.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/packed_scene.h"

void InstancePlaceholder::set_instance_path(const String& p_name) { path = p_name; }

String InstancePlaceholder::get_instance_path() const { return path; }

Node* InstancePlaceholder::try_get_node(
	InstancePlaceholder* p_placeholder, Node* p_instance, const NodePath& p_path)
{
	// First try to resolve internally,
	// if that fails try resolving externally.
	Node* node = p_instance->get_node_or_null(p_path);
	if (node == nullptr) {
		node = p_placeholder->get_node_or_null(p_path);
	}

	return node;
}

InstancePlaceholder::InstancePlaceholder() {}

// ==========================================================================
// Merged from scene/main/missing_node.cpp
// ==========================================================================

#include "scene/main/scene_tree.h"

void MissingNode::set_original_class(const String& p_class) { original_class = p_class; }

String MissingNode::get_original_class() const { return original_class; }

void MissingNode::set_original_scene(const String& p_scene) { original_scene = p_scene; }

String MissingNode::get_original_scene() const { return original_scene; }

void MissingNode::set_recording_properties(bool p_enable) { recording_properties = p_enable; }

bool MissingNode::is_recording_properties() const { return recording_properties; }

void MissingNode::set_recording_signals(bool p_enable) { recording_signals = p_enable; }

bool MissingNode::is_recording_signals() const { return recording_signals; }

PackedStringArray MissingNode::get_configuration_warnings() const
{
	// The mere existence of this node is warning.
	PackedStringArray warnings = Node::get_configuration_warnings();
	if (!original_scene.is_empty()) {
		warnings.push_back(vformat(RTR("This node was an instance of scene '%s', which was no "
									   "longer available when this scene was loaded."),
			original_scene));
		warnings.push_back(
			vformat(RTR("Saving current scene will discard instance and all its properties, "
						"including editable children edits (if existing).")));
	}
	else if (!original_class.is_empty()) {
		warnings.push_back(vformat(RTR("This node was saved as class type '%s', which was no "
									   "longer available when this scene was loaded."),
			original_class));
		warnings.push_back(
			RTR("Data from the original node is kept as a placeholder until this type of node is "
				"available again. It can hence be safely re-saved without risk of data loss."));
	}
	else {
		warnings.push_back(
			RTR("Unrecognized missing node. Check scene dependency errors for details."));
	}
	return warnings;
}

MissingNode::MissingNode() {}

// ==========================================================================
// Merged from scene/main/resource_preloader.cpp
// ==========================================================================

#include "core/templates/rb_set.h"
// --- Inlined from scene/main/resource_preloader.h ---
#include "scene/main/node.h"

class ResourcePreloader : public Node
{
	HashMap<String, Ref<Resource>> resources;

	Vector<String> _get_resource_list() const;

protected:

public:
	void add_resource(const String& p_name, const Ref<Resource>& p_resource);
	void remove_resource(const String& p_name);
	void rename_resource(const String& p_from_name, const String& p_to_name);
	bool has_resource(const String& p_name) const;
	Ref<Resource> get_resource(const String& p_name) const;

	void get_resource_list(List<String>* p_list);

	ResourcePreloader();
};



// --- End scene/main/resource_preloader.h ---

void ResourcePreloader::rename_resource(const String& p_from_name, const String& p_to_name)
{
	ERR_FAIL_COND(!resources.has(p_from_name));

	Ref<Resource> res = resources[p_from_name];

	resources.erase(p_from_name);
	add_resource(p_to_name, res);
}

bool ResourcePreloader::has_resource(const String& p_name) const
{
	return resources.has(p_name);
}

Ref<Resource> ResourcePreloader::get_resource(const String& p_name) const
{
	ERR_FAIL_COND_V(!resources.has(p_name), Ref<Resource>());
	return resources[p_name];
}

Vector<String> ResourcePreloader::_get_resource_list() const
{
	Vector<String> res;
	res.resize(resources.size());
	int i = 0;
	for (const KeyValue<String, Ref<Resource>>& E : resources) {
		res.set(i, E.key);
		i++;
	}

	return res;
}

void ResourcePreloader::get_resource_list(List<String>* p_list)
{
	for (const KeyValue<String, Ref<Resource>>& E : resources) {
		p_list->push_back(E.key);
	}
}


ResourcePreloader::ResourcePreloader() {}



void ResourcePreloader::add_resource(String const&, Ref<Resource> const&) {}

// ==========================================================================
// Merged from scene/main/shader_globals_override.cpp
// ==========================================================================

#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"
// --- Inlined from scene/main/shader_globals_override.h ---
#include "scene/main/node.h"

class ShaderGlobalsOverride : public Node {

	String *_remap(const String &p_name) const;

	bool active = false;
	mutable HashMap<String, String> param_remaps;

	void _activate();

protected:
	void _notification(int p_what);

public:
	PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/main/shader_globals_override.h ---

String* ShaderGlobalsOverride::_remap(const String& p_name) const
{
	String* r = param_remaps.getptr(p_name);
	if (!r) {
		// not cached, do caching
		String p = p_name;
		if (p.begins_with("params/")) {
			String q = p.replace_first("params/", "");
			param_remaps[p] = q;
			r = param_remaps.getptr(p);
		}
	}

	return r;
}

PackedStringArray ShaderGlobalsOverride::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (!active) {
		warnings.push_back(RTR("ShaderGlobalsOverride is not active because another node of the "
							   "same type is in the scene."));
	}

	return warnings;
}

// ==========================================================================
// Merged from scene/main/timer.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"

void Timer::set_wait_time(double p_time)
{
	ERR_FAIL_COND_MSG(p_time <= 0, "Time should be greater than zero.");
	wait_time = p_time;
	update_configuration_warnings();
}

double Timer::get_wait_time() const { return wait_time; }

void Timer::set_one_shot(bool p_one_shot) { one_shot = p_one_shot; }

bool Timer::is_one_shot() const { return one_shot; }

void Timer::set_autostart(bool p_start) { autostart = p_start; }

bool Timer::has_autostart() const { return autostart; }

void Timer::stop()
{
	time_left = -1;
	_set_process(false);
	autostart = false;
}

void Timer::set_paused(bool p_paused)
{
	if (paused == p_paused) {
		return;
	}

	paused = p_paused;
	_set_process(processing);
}

bool Timer::is_paused() const { return paused; }

void Timer::set_ignore_time_scale(bool p_ignore) { ignore_time_scale = p_ignore; }

bool Timer::is_ignoring_time_scale() { return ignore_time_scale; }

bool Timer::is_stopped() const { return get_time_left() <= 0; }

double Timer::get_time_left() const { return time_left > 0 ? time_left : 0; }

void Timer::set_timer_process_callback(TimerProcessCallback p_callback)
{
	if (timer_process_callback == p_callback) {
		return;
	}

	switch (timer_process_callback) {
	case TIMER_PROCESS_PHYSICS:
		if (is_physics_processing_internal()) {

		set_physics_process_internal(false);
			set_process_internal(true);
		}
		break;
	case TIMER_PROCESS_IDLE:
		if (is_processing_internal()) {
			set_process_internal(false);
			set_physics_process_internal(true);
		}
		break;
	}
	timer_process_callback = p_callback;
}

Timer::TimerProcessCallback Timer::get_timer_process_callback() const
{
	return timer_process_callback;
}

void Timer::_set_process(bool p_process, bool p_force)
{
	switch (timer_process_callback) {
	case TIMER_PROCESS_PHYSICS:
		set_physics_process_internal(p_process && !paused);
		break;
	case TIMER_PROCESS_IDLE:
		set_process_internal(p_process && !paused);
		break;
	}
	processing = p_process;
}

PackedStringArray Timer::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (wait_time < 0.05 - CMP_EPSILON) {
		warnings.push_back(
			RTR("Very low timer wait times (< 0.05 seconds) may behave in significantly different "
				"ways depending on the rendered or physics frame rate.\nConsider using a script's "
				"process loop instead of relying on a Timer for very low wait times."));
	}

	return warnings;
}

// ==========================================================================
// Merged from scene/main/status_indicator.cpp
// ==========================================================================

#include "scene/gui/dialogs.h"
#include "servers/display/display_server.h"
// --- Inlined from scene/main/status_indicator.h ---
#include "scene/main/node.h"
#include "scene/resources/texture.h"
#include "servers/display/display_server_enums.h"

class StatusIndicator : public Node {

	Ref<Texture2D> icon;
	String tooltip;
	bool visible = true;
	DisplayServerEnums::IndicatorID iid = DisplayServerEnums::INVALID_INDICATOR_ID;
	NodePath menu;

protected:
	void _notification(int p_what);

	void _callback(MouseButton p_index, const Point2i &p_pos);

public:
	void set_icon(const Ref<Texture2D> &p_icon);
	Ref<Texture2D> get_icon() const;

	void set_tooltip(const String &p_tooltip);
	String get_tooltip() const;

	void set_menu(const NodePath &p_menu);
	NodePath get_menu() const;

	void set_visible(bool p_visible);
	bool is_visible() const;

	Rect2 get_rect() const;
};

// --- End scene/main/status_indicator.h ---

void StatusIndicator::set_icon(const Ref<Texture2D>& p_icon)
{
	ERR_MAIN_THREAD_GUARD;
	icon = p_icon;
	if (iid != DisplayServerEnums::INVALID_INDICATOR_ID) {
		DisplayServer::status_indicator_set_icon(iid, icon);
	}
}

Ref<Texture2D> StatusIndicator::get_icon() const { return icon; }

void StatusIndicator::set_tooltip(const String& p_tooltip)
{
	ERR_MAIN_THREAD_GUARD;
	tooltip = p_tooltip;
	if (iid != DisplayServerEnums::INVALID_INDICATOR_ID) {
		DisplayServer::status_indicator_set_tooltip(iid, tooltip);
	}
}

String StatusIndicator::get_tooltip() const { return tooltip; }

NodePath StatusIndicator::get_menu() const { return menu; }

bool StatusIndicator::is_visible() const { return visible; }

Rect2 StatusIndicator::get_rect() const
{
	if (iid == DisplayServerEnums::INVALID_INDICATOR_ID) {
		return Rect2();
	}
	return DisplayServer::status_indicator_get_rect(iid);
}

// ============================================================================
// Merged from scene/main/canvas_item.cpp
// ============================================================================

#include "canvas_item.h"
#include "scene/2d/node_2d.h"
#include "scene/main/canvas_item.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/theme.h"
#include "scene/main/viewport.h"
#include "servers/display/accessibility_server.h"
#include "servers/rendering/renderer.h"

#define ERR_DRAW_GUARD                                                                             \
	ERR_FAIL_COND_MSG(!drawing,                                                                    \
		"Drawing is only allowed inside this node's `_draw()`, functions connected to "            \
		"its `draw` signal, or when it receives NOTIFICATION_DRAW.")

#ifdef DEBUG_ENABLED
bool CanvasItem::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	if (_edit_use_rect()) {
		return _edit_get_rect().has_point(p_point);
	}
	else {
		return p_point.length() < p_tolerance;
	}
}
#endif // DEBUG_ENABLED

#ifdef TOOLS_ENABLED
Transform2D CanvasItem::_edit_get_transform() const
{
	return Transform2D(_edit_get_rotation(), _edit_get_position() + _edit_get_pivot());
}
#endif // TOOLS_ENABLED

bool CanvasItem::is_visible_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return visible && parent_visible_in_tree;
}

void CanvasItem::_propagate_visibility_changed(bool p_parent_visible_in_tree)
{
	parent_visible_in_tree = p_parent_visible_in_tree;
	if (!visible) {
		return;
	}

	_handle_visibility_change(p_parent_visible_in_tree);
}

void CanvasItem::set_visible(bool p_visible) { visible = p_visible; }

void CanvasItem::show()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(true);
}

void CanvasItem::hide()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(false);
}

bool CanvasItem::is_visible() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return visible;
}

CanvasItem* CanvasItem::current_item_drawn = nullptr;

CanvasItem* CanvasItem::get_current_item_drawn() { return current_item_drawn; }

Transform2D CanvasItem::get_global_transform_with_canvas() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (canvas_layer) {
		return canvas_layer->get_final_transform() * get_global_transform();
	}
	else if (is_inside_tree()) {
		return get_viewport()->get_canvas_transform() * get_global_transform();
	}
	else {
		return get_global_transform();
	}
}

Transform2D CanvasItem::get_screen_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	ERR_FAIL_COND_V(!is_inside_tree(), Transform2D());
	return get_viewport()->get_popup_base_transform() * get_global_transform_with_canvas();
}

CanvasItem* CanvasItem::get_parent_item() const
{
	CanvasItem ci = CanvasItem();
	return &ci;
}

Transform2D CanvasItem::get_global_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());

	if (_is_global_invalid()) {
		// This code can enter multiple times from threads if dirty, this is expected.
		const CanvasItem* pi = get_parent_item();
		Transform2D new_global;
		if (pi) {
			new_global = pi->get_global_transform() * get_transform();
		}
		else {
			new_global = get_transform();
		}

		global_transform = new_global;
		_set_global_invalid(false);
	}

	return global_transform;
}

// Same as get_global_transform() but no reset for `global_invalid`.
Transform2D CanvasItem::get_global_transform_const() const
{
	if (_is_global_invalid()) {
		const CanvasItem* pi = get_parent_item();
		if (pi) {
			global_transform = pi->get_global_transform_const() * get_transform();
		}
		else {
			global_transform = get_transform();
		}
	}

	return global_transform;
}

void CanvasItem::_set_global_invalid(bool p_invalid) const
{
	if (is_group_processing()) {
		if (p_invalid) {
			global_invalid.mt.set();
		}
		else {
			global_invalid.mt.clear();
		}
	}
	else {
		global_invalid.st = p_invalid;
	}
}

void CanvasItem::_top_level_raise_self()
{
	if (!is_inside_tree()) {
		return;
	}

	if (canvas_layer) {
		Renderer::canvas_item_set_draw_index(canvas_item, canvas_layer->get_sort_index());
	}
	else {
		Renderer::canvas_item_set_draw_index(
			canvas_item, get_viewport()->gui_get_canvas_sort_index());
	}
}

bool CanvasItem::_is_oversampling_with_scale() const
{
	if (oversampling_with_scale == OVERSAMPLING_WITH_SCALE_PARENT_NODE) {
		CanvasItem* ci = get_parent_item();
		if (ci) {
			return ci->_is_oversampling_with_scale();
		}
	}
	return oversampling_with_scale == OVERSAMPLING_WITH_SCALE_ENABLED;
}

CanvasItem::OversamplingWithScale CanvasItem::get_oversampling_with_scale() const
{
	return oversampling_with_scale;
}

void CanvasItem::set_oversampling_with_scale(CanvasItem::OversamplingWithScale p_mode)
{
	if (oversampling_with_scale == p_mode) {
		return;
	}
	oversampling_with_scale = p_mode;
	_update_oversampling(true);
}

void CanvasItem::_window_visibility_changed()
{
	_propagate_visibility_changed(window->is_visible());
}

void CanvasItem::move_to_front()
{
	ERR_MAIN_THREAD_GUARD;
	if (!get_parent()) {
		return;
	}
	get_parent()->move_child(this, -1);
}

void CanvasItem::set_modulate(const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	if (modulate == p_modulate) {
		return;
	}

	modulate = p_modulate;
	Renderer::canvas_item_set_modulate(canvas_item, modulate);
}

Color CanvasItem::get_modulate() const
{
	ERR_READ_THREAD_GUARD_V(Color());
	return modulate;
}

Color CanvasItem::get_modulate_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(Color());
	Color final_modulate = modulate;
	CanvasItem* parent_item = get_parent_item();
	while (parent_item) {
		final_modulate *= parent_item->get_modulate();
		parent_item = parent_item->get_parent_item();
	}
	return final_modulate;
}

bool CanvasItem::is_set_as_top_level() const { return top_level; }

void CanvasItem::set_self_modulate(const Color& p_self_modulate)
{
	ERR_THREAD_GUARD;
	if (self_modulate == p_self_modulate) {
		return;
	}

	self_modulate = p_self_modulate;
	Renderer::canvas_item_set_self_modulate(canvas_item, self_modulate);
}

Color CanvasItem::get_self_modulate() const
{
	ERR_READ_THREAD_GUARD_V(Color());
	return self_modulate;
}

void CanvasItem::set_light_mask(int p_light_mask)
{
	ERR_THREAD_GUARD;
	if (light_mask == p_light_mask) {
		return;
	}

	light_mask = p_light_mask;
	Renderer::canvas_item_set_light_mask(canvas_item, p_light_mask);
}

int CanvasItem::get_light_mask() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return light_mask;
}

void CanvasItem::set_z_as_relative(bool p_enabled)
{
	ERR_THREAD_GUARD;
	if (z_relative == p_enabled) {
		return;
	}
	z_relative = p_enabled;
	Renderer::canvas_item_set_z_as_relative_to_parent(canvas_item, p_enabled);
}

bool CanvasItem::is_z_relative() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return z_relative;
}

int CanvasItem::get_z_index() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return z_index;
}

int CanvasItem::get_effective_z_index() const
{
	ERR_READ_THREAD_GUARD_V(0);
	int effective_z_index = z_index;
	if (is_z_relative()) {
		CanvasItem* p = get_parent_item();
		if (p) {
			effective_z_index += p->get_effective_z_index();
		}
	}
	return effective_z_index;
}

void CanvasItem::set_y_sort_enabled(bool p_enabled)
{
	ERR_THREAD_GUARD;
	y_sort_enabled = p_enabled;
	Renderer::canvas_item_set_sort_children_by_y(canvas_item, y_sort_enabled);
}

bool CanvasItem::is_y_sort_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return y_sort_enabled;
}

void CanvasItem::draw_dashed_line(const Point2& p_from, const Point2& p_to, const Color& p_color,
	real_t p_width, real_t p_dash, bool p_aligned, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	ERR_FAIL_COND(p_dash <= 0.0);

	float length = (p_to - p_from).length();
	Vector2 step = p_dash * (p_to - p_from).normalized();

	if (length < p_dash || step == Vector2()) {
		Renderer::canvas_item_add_line(canvas_item, p_from, p_to, p_color, p_width, p_antialiased);
		return;
	}

	int steps = (p_aligned) ? Math::ceil(length / p_dash) : Math::floor(length / p_dash);
	if (steps % 2 == 0) {
		steps--;
	}

	Point2 off = p_from;
	if (p_aligned) {
		off += (p_to - p_from).normalized() * (length - steps * p_dash) / 2.0;
	}

	Vector<Vector2> points;
	points.resize(steps + 1);
	for (int i = 0; i < steps; i += 2) {
		points.write[i] = (i == 0) ? p_from : off;
		points.write[i + 1] = (p_aligned && i == steps - 1) ? p_to : (off + step);
		off += step * 2;
	}

	Vector<Color> colors = {p_color};

	Renderer::canvas_item_add_multiline(canvas_item, points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_line(const Point2& p_from, const Point2& p_to, const Color& p_color,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_line(canvas_item, p_from, p_to, p_color, p_width, p_antialiased);
}

void CanvasItem::draw_polyline(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Vector<Color> colors = {p_color};
	Renderer::canvas_item_add_polyline(canvas_item, p_points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_polyline_colors(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_polyline(canvas_item, p_points, p_colors, p_width, p_antialiased);
}

void CanvasItem::draw_ellipse_arc(const Vector2& p_center, real_t p_major, real_t p_minor,
	real_t p_start_angle, real_t p_end_angle, int p_point_count, const Color& p_color,
	real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Vector<Point2> points;
	points.resize(p_point_count);
	Point2* points_ptr = points.ptrw();

	// Clamp angle difference to full circle so arc won't overlap itself.
	const real_t delta_angle = CLAMP(p_end_angle - p_start_angle, -Math::TAU, Math::TAU);
	for (int i = 0; i < p_point_count; i++) {
		real_t theta = (i / (p_point_count - 1.0f)) * delta_angle + p_start_angle;
		points_ptr[i] = p_center + Vector2(p_major * Math::cos(theta), p_minor * Math::sin(theta));
	}

	draw_polyline(points, p_color, p_width, p_antialiased);
}

void CanvasItem::draw_arc(const Vector2& p_center, real_t p_radius, real_t p_start_angle,
	real_t p_end_angle, int p_point_count, const Color& p_color, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	draw_ellipse_arc(p_center, p_radius, p_radius, p_start_angle, p_end_angle, p_point_count,
		p_color, p_width, p_antialiased);
}

void CanvasItem::draw_multiline(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Vector<Color> colors = {p_color};
	Renderer::canvas_item_add_multiline(canvas_item, p_points, colors, p_width, p_antialiased);
}

void CanvasItem::draw_multiline_colors(const Vector<Point2>& p_points,
	const Vector<Color>& p_colors, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_multiline(canvas_item, p_points, p_colors, p_width, p_antialiased);
}

void CanvasItem::draw_rect(
	const Rect2& p_rect, const Color& p_color, bool p_filled, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Rect2 rect = p_rect.abs();

	if (p_filled) {
		if (p_width != -1.0) {
			WARN_PRINT(
				"The draw_rect() \"width\" argument has no effect when \"filled\" is \"true\".");
		}

		Renderer::canvas_item_add_rect(canvas_item, rect, p_color, p_antialiased);
	}
	else if (p_width >= rect.size.width || p_width >= rect.size.height) {
		Renderer::canvas_item_add_rect(
			canvas_item, rect.grow(0.5f * p_width), p_color, p_antialiased);
	}
	else {
		Vector<Vector2> points;
		points.resize(5);
		points.write[0] = rect.position;
		points.write[1] = rect.position + Vector2(rect.size.x, 0);
		points.write[2] = rect.position + rect.size;
		points.write[3] = rect.position + Vector2(0, rect.size.y);
		points.write[4] = rect.position;

		Vector<Color> colors = {p_color};

		Renderer::canvas_item_add_polyline(canvas_item, points, colors, p_width, p_antialiased);
	}
}

void CanvasItem::draw_ellipse(const Point2& p_pos, real_t p_major, real_t p_minor,
	const Color& p_color, bool p_filled, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	if (p_filled) {
		if (p_width != -1.0) {
			WARN_PRINT("The \"width\" argument has no effect when \"filled\" is \"true\".");
		}

		Renderer::canvas_item_add_ellipse(
			canvas_item, p_pos, p_major, p_minor, p_color, p_antialiased);
	}
	else if (p_width >= 2.0 * MAX(p_major, p_minor)) {
		Renderer::canvas_item_add_ellipse(canvas_item, p_pos, p_major + 0.5 * p_width,
			p_minor + 0.5 * p_width, p_color, p_antialiased);
	}
	else {
		// Tessellation count is hardcoded. Keep in sync with the same variable in
		// `RendererCanvasCull::canvas_item_add_circle()`.
		const int circle_segments = 64;

		Vector<Vector2> points;
		points.resize(circle_segments + 1);

		Vector2* points_ptr = points.ptrw();
		const real_t circle_point_step = Math::TAU / circle_segments;

		for (int i = 0; i < circle_segments; i++) {
			float angle = i * circle_point_step;
			points_ptr[i].x = Math::cos(angle) * p_major;
			points_ptr[i].y = Math::sin(angle) * p_minor;
			points_ptr[i] += p_pos;
		}
		points_ptr[circle_segments] = points_ptr[0];

		Vector<Color> colors = {p_color};

		Renderer::canvas_item_add_polyline(canvas_item, points, colors, p_width, p_antialiased);
	}
}

void CanvasItem::draw_circle(const Point2& p_pos, real_t p_radius, const Color& p_color,
	bool p_filled, real_t p_width, bool p_antialiased)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	draw_ellipse(p_pos, p_radius, p_radius, p_color, p_filled, p_width, p_antialiased);
}

void CanvasItem::draw_texture(Texture2D* rp_texture, const Point2& p_pos, const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_texture->draw(canvas_item, p_pos, p_modulate, false);
}

void CanvasItem::draw_texture_rect(Texture2D* rp_texture, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_texture->draw_rect(canvas_item, p_rect, p_tile, p_modulate, p_transpose);
}

void CanvasItem::draw_texture_rect_region(Texture2D* rp_texture, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_texture->draw_rect_region(
		canvas_item, p_rect, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

void CanvasItem::draw_msdf_texture_rect_region(Texture2D* rp_texture, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, double p_outline, double p_pixel_range,
	double p_scale)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	Renderer::canvas_item_add_msdf_texture_rect_region(canvas_item, p_rect, rp_texture->get_rid(),
		p_src_rect, p_modulate, p_outline, p_pixel_range, p_scale);
}

void CanvasItem::draw_lcd_texture_rect_region(
	Texture2D* rp_texture, const Rect2& p_rect, const Rect2& p_src_rect, const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	Renderer::canvas_item_add_lcd_texture_rect_region(
		canvas_item, p_rect, rp_texture->get_rid(), p_src_rect, p_modulate);
}

void CanvasItem::draw_style_box(StyleBox* rp_style_box, const Rect2& p_rect)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_style_box->draw(canvas_item, p_rect);
}

void CanvasItem::draw_primitive(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	RID rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
	Renderer::canvas_item_add_primitive(canvas_item, p_points, p_colors, p_uvs, rid);
}

void CanvasItem::draw_set_transform(const Point2& p_offset, real_t p_rot, const Size2& p_scale)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Transform2D xform(p_rot, p_scale, 0.0, p_offset);
	Renderer::canvas_item_add_set_transform(canvas_item, xform);
}

void CanvasItem::draw_set_transform_matrix(const Transform2D& p_matrix)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_set_transform(canvas_item, p_matrix);
}

void CanvasItem::draw_animation_slice(
	double p_animation_length, double p_slice_begin, double p_slice_end, double p_offset)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_animation_slice(
		canvas_item, p_animation_length, p_slice_begin, p_slice_end, p_offset);
}

void CanvasItem::draw_end_animation()
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	Renderer::canvas_item_add_animation_slice(canvas_item, 1, 0, 2, 0);
}

void CanvasItem::draw_polygon(const Vector<Point2>& p_points, const Vector<Color>& p_colors,
	const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture)
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	const Ref<AtlasTexture> atlas = p_texture;
	if (atlas.is_valid() && !atlas->is_animated() && atlas->get_atlas().is_valid()) {
		const Ref<Texture2D>& texture = atlas->get_atlas();
		const Vector2 atlas_size = texture->get_size();

		const Vector2 remap_min = atlas->get_region().position / atlas_size;
		const Vector2 remap_max = atlas->get_region().get_end() / atlas_size;

		PackedVector2Array uvs = p_uvs;
		for (Vector2& p : uvs) {
			p.x = Math::remap(p.x, 0, 1, remap_min.x, remap_max.x);
			p.y = Math::remap(p.y, 0, 1, remap_min.y, remap_max.y);
		}
		Renderer::canvas_item_add_polygon(canvas_item, p_points, p_colors, uvs, texture->get_rid());
	}
	else {
		RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
		Renderer::canvas_item_add_polygon(canvas_item, p_points, p_colors, p_uvs, texture_rid);
	}
}

void CanvasItem::draw_colored_polygon(const Vector<Point2>& p_points, const Color& p_color,
	const Vector<Point2>& p_uvs, Ref<Texture2D> p_texture)
{
	draw_polygon(p_points, {p_color}, p_uvs, p_texture);
}

void CanvasItem::draw_mesh(Mesh* rp_mesh, const Ref<Texture2D>& p_texture,
	const Transform2D& p_transform, const Color& p_modulate)
{
	ERR_THREAD_GUARD;
	RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();

	Renderer::canvas_item_add_mesh(
		canvas_item, rp_mesh->get_rid(), p_transform, p_modulate, texture_rid);
}

void CanvasItem::draw_multimesh(MultiMesh* rp_multimesh, const Ref<Texture2D>& p_texture)
{
	ERR_THREAD_GUARD;
	RID texture_rid = p_texture.is_valid() ? p_texture->get_rid() : RID();
	Renderer::canvas_item_add_multimesh(canvas_item, rp_multimesh->get_rid(), texture_rid);
}

void CanvasItem::draw_string(Font* rp_font, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, const Color& p_modulate,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_font->draw_string(canvas_item, p_pos, p_text, p_alignment, p_width, p_font_size, p_modulate,
		p_jst_flags, p_direction, p_orientation, p_oversampling);
}

void CanvasItem::draw_multiline_string(Font* rp_font, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_max_lines,
	const Color& p_modulate, uint32_t p_brk_flags, uint32_t p_jst_flags,
	TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_font->draw_multiline_string(canvas_item, p_pos, p_text, p_alignment, p_width, p_font_size,
		p_max_lines, p_modulate, p_brk_flags, p_jst_flags, p_direction, p_orientation,
		p_oversampling);
}

void CanvasItem::draw_string_outline(Font* rp_font, const Point2& p_pos, const String& p_text,
	HorizontalAlignment p_alignment, float p_width, int p_font_size, int p_size,
	const Color& p_modulate, uint32_t p_jst_flags, TextServer::Direction p_direction,
	TextServer::Orientation p_orientation, float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;

	rp_font->draw_string_outline(canvas_item, p_pos, p_text, p_alignment, p_width, p_font_size,
		p_size, p_modulate, p_jst_flags, p_direction, p_orientation, p_oversampling);
}

void CanvasItem::draw_multiline_string_outline(Font* rp_font, const Point2& p_pos,
	const String& p_text, HorizontalAlignment p_alignment, float p_width, int p_font_size,
	int p_max_lines, int p_size, const Color& p_modulate, uint32_t p_brk_flags,
	uint32_t p_jst_flags, TextServer::Direction p_direction, TextServer::Orientation p_orientation,
	float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	rp_font->draw_multiline_string_outline(canvas_item, p_pos, p_text, p_alignment, p_width,
		p_font_size, p_max_lines, p_size, p_modulate, p_brk_flags, p_jst_flags, p_direction,
		p_orientation, p_oversampling);
}

void CanvasItem::draw_char(Font* rp_font, const Point2& p_pos, const String& p_char,
	int p_font_size, const Color& p_modulate, float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	ERR_FAIL_COND(p_char.length() != 1);

	rp_font->draw_char(canvas_item, p_pos, p_char[0], p_font_size, p_modulate, p_oversampling);
}

void CanvasItem::draw_char_outline(Font* rp_font, const Point2& p_pos, const String& p_char,
	int p_font_size, int p_size, const Color& p_modulate, float p_oversampling) const
{
	ERR_THREAD_GUARD;
	ERR_DRAW_GUARD;
	ERR_FAIL_COND(p_char.length() != 1);

	rp_font->draw_char_outline(
		canvas_item, p_pos, p_char[0], p_font_size, p_size, p_modulate, p_oversampling);
}

void CanvasItem::_notify_transform_deferred()
{
	if (is_inside_tree() && notify_transform && !xform_change.in_list() && SceneTree::data) {
		SceneTree::data->xform_change_list.add(&xform_change);
	}
}

void CanvasItem::_physics_interpolated_changed()
{
	Renderer::canvas_item_set_interpolated(canvas_item, is_physics_interpolated());
}

void CanvasItem::set_canvas_item_use_identity_transform(bool p_enable)
{
	// Prevent sending item transforms to Renderer when using global coords.
	_set_use_identity_transform(p_enable);

	// Let Renderer know not to concatenate the parent transform during the render.
	Renderer::canvas_item_set_use_identity_transform(get_canvas_item(), p_enable);

	if (is_inside_tree()) {
		if (p_enable) {
			// Make sure item is using identity transform in server.
			Renderer::canvas_item_set_transform(get_canvas_item(), Transform2D());
		}
		else {
			// Make sure item transform is up to date in server if switching identity transform off.
			Renderer::canvas_item_set_transform(get_canvas_item(), get_transform());
		}
	}
}

Rect2 CanvasItem::get_viewport_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());
	return get_viewport()->get_visible_rect();
}

RID CanvasItem::get_canvas() const
{
	ERR_READ_THREAD_GUARD_V(RID());
	ERR_FAIL_COND_V(!is_inside_tree(), RID());

	if (canvas_layer) {
		return canvas_layer->get_canvas();
	}
	else {
		return get_viewport()->find_world_2d()->get_canvas();
	}
}

Ref<World2D> CanvasItem::get_world_2d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World2D>());
	ERR_FAIL_COND_V(!is_inside_tree(), Ref<World2D>());

	CanvasItem* tl = get_top_level();

	if (tl->get_viewport()) {
		return tl->get_viewport()->find_world_2d();
	}
	else {
		return Ref<World2D>();
	}
}

RID CanvasItem::get_viewport_rid() const
{
	ERR_READ_THREAD_GUARD_V(RID());
	ERR_FAIL_COND_V(!is_inside_tree(), RID());
	return get_viewport()->get_viewport_rid();
}

void CanvasItem::set_block_transform_notify(bool p_enable)
{
	ERR_THREAD_GUARD;
	block_transform_notify = p_enable;
}

bool CanvasItem::is_block_transform_notify_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return block_transform_notify;
}

void CanvasItem::set_draw_behind_parent(bool p_enable)
{
	ERR_THREAD_GUARD;
	if (behind == p_enable) {
		return;
	}
	behind = p_enable;
	Renderer::canvas_item_set_draw_behind_parent(canvas_item, behind);
}

bool CanvasItem::is_draw_behind_parent_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return behind;
}

void CanvasItem::set_use_parent_material(bool p_use_parent_material)
{
	ERR_THREAD_GUARD;
	use_parent_material = p_use_parent_material;
	Renderer::canvas_item_set_use_parent_material(canvas_item, p_use_parent_material);
}

bool CanvasItem::get_use_parent_material() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_parent_material;
}

void CanvasItem::set_material(const Ref<Material>& p_material) {}

void CanvasItem::_update_self_texture_repeat(RSE::CanvasItemTextureRepeat p_texture_repeat) {}

void CanvasItem::_update_self_texture_filter(RSE::CanvasItemTextureFilter p_texture_filter) {}

void CanvasItem::set_z_index(int p_z) {}

Ref<Material> CanvasItem::get_material() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Material>());
	return material;
}

Vector2 CanvasItem::make_canvas_position_local(const Vector2& screen_point) const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	ERR_FAIL_COND_V(!is_inside_tree(), screen_point);

	Transform2D local_matrix = (get_canvas_transform() * get_global_transform()).affine_inverse();

	return local_matrix.xform(screen_point);
}

InputEvent CanvasItem::make_input_local(const InputEvent& p_event) const
{
	ERR_FAIL_COND_V(!is_inside_tree(), p_event);
	return p_event.xformed_by((get_canvas_transform() * get_global_transform()).affine_inverse());
}

Vector2 CanvasItem::get_global_mouse_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	ERR_FAIL_NULL_V(get_viewport(), Vector2());
	return get_canvas_transform().affine_inverse().xform(get_viewport()->get_mouse_position());
}

Vector2 CanvasItem::get_local_mouse_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	ERR_FAIL_NULL_V(get_viewport(), Vector2());

	return get_global_transform().affine_inverse().xform(get_global_mouse_position());
}

Transform2D CanvasItem::get_viewport_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	ERR_FAIL_COND_V(!is_inside_tree(), Transform2D());

	if (canvas_layer) {
		return get_viewport()->get_final_transform() * canvas_layer->get_final_transform();
	}
	else {
		return get_viewport()->get_final_transform() * get_viewport()->get_canvas_transform();
	}
}

void CanvasItem::set_notify_local_transform(bool p_enable)
{
	ERR_THREAD_GUARD;
	notify_local_transform = p_enable;
}

bool CanvasItem::is_local_transform_notification_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return notify_local_transform;
}

void CanvasItem::set_notify_transform(bool p_enable)
{
	ERR_THREAD_GUARD;
	if (notify_transform == p_enable) {
		return;
	}

	notify_transform = p_enable;

	if (notify_transform && is_inside_tree()) {
		// This ensures that invalid globals get resolved, so notifications can be received.
		_ALLOW_DISCARD_ get_global_transform();
	}
}

bool CanvasItem::is_transform_notification_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return notify_transform;
}

int CanvasItem::get_canvas_layer() const
{
	ERR_READ_THREAD_GUARD_V(0);
	if (canvas_layer) {
		return canvas_layer->get_layer();
	}
	else {
		return 0;
	}
}

CanvasLayer* CanvasItem::get_canvas_layer_node() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return canvas_layer;
}

void CanvasItem::set_visibility_layer(uint32_t p_visibility_layer)
{
	ERR_THREAD_GUARD;
	visibility_layer = p_visibility_layer;
	Renderer::canvas_item_set_visibility_layer(canvas_item, p_visibility_layer);
}

uint32_t CanvasItem::get_visibility_layer() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return visibility_layer;
}

void CanvasItem::set_visibility_layer_bit(uint32_t p_visibility_layer, bool p_enable)
{
	ERR_THREAD_GUARD;
	ERR_FAIL_UNSIGNED_INDEX(p_visibility_layer, 32);
	if (p_enable) {
		set_visibility_layer(visibility_layer | (1 << p_visibility_layer));
	}
	else {
		set_visibility_layer(visibility_layer & (~(1 << p_visibility_layer)));
	}
}

bool CanvasItem::get_visibility_layer_bit(uint32_t p_visibility_layer) const
{
	ERR_READ_THREAD_GUARD_V(false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_visibility_layer, 32, false);
	return (visibility_layer & (1 << p_visibility_layer));
}

void CanvasItem::_refresh_texture_filter_cache() const
{
	if (!is_inside_tree()) {
		return;
	}

	if (texture_filter == TEXTURE_FILTER_PARENT_NODE) {
		CanvasItem* parent_item = get_parent_item();
		if (parent_item) {
			texture_filter_cache = parent_item->texture_filter_cache;
		}
		else {
			texture_filter_cache = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
		}
	}
	else {
		texture_filter_cache = RSE::CanvasItemTextureFilter(texture_filter);
	}
}

CanvasItem::TextureFilter CanvasItem::get_texture_filter() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_FILTER_NEAREST);
	return texture_filter;
}

void CanvasItem::_refresh_texture_repeat_cache() const
{
	if (!is_inside_tree()) {
		return;
	}

	if (texture_repeat == TEXTURE_REPEAT_PARENT_NODE) {
		CanvasItem* parent_item = get_parent_item();
		if (parent_item) {
			texture_repeat_cache = parent_item->texture_repeat_cache;
		}
		else {
			texture_repeat_cache = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
		}
	}
	else {
		texture_repeat_cache = RSE::CanvasItemTextureRepeat(texture_repeat);
	}
}

CanvasItem::ClipChildrenMode CanvasItem::get_clip_children_mode() const
{
	ERR_READ_THREAD_GUARD_V(CLIP_CHILDREN_DISABLED);
	return clip_children_mode;
}

CanvasItem::TextureRepeat CanvasItem::get_texture_repeat() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_REPEAT_DISABLED);
	return texture_repeat;
}

CanvasItem::TextureFilter CanvasItem::get_texture_filter_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_FILTER_NEAREST);
	_refresh_texture_filter_cache();
	return (TextureFilter)texture_filter_cache;
}

CanvasItem::TextureRepeat CanvasItem::get_texture_repeat_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(TEXTURE_REPEAT_DISABLED);
	_refresh_texture_repeat_cache();
	return (TextureRepeat)texture_repeat_cache;
}

PackedStringArray CanvasItem::get_configuration_warnings() const { return PackedStringArray(); }

CanvasItem::CanvasItem() { canvas_item = Renderer::canvas_item_create(); }

CanvasItem::~CanvasItem()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(canvas_item);
}

void CanvasItem::queue_redraw()
{
    if (!is_inside_tree() || !canvas_item.is_valid()) {
        return;
    }
    Renderer::canvas_item_clear(canvas_item);
    if (is_visible_in_tree()) {
        drawing = true;
        notification(NOTIFICATION_DRAW);
        if (draw_callback) {
            draw_callback(this);
        }
        drawing = false;
    }
}

void CanvasItem::_top_level_changed() {}

void CanvasItem::_top_level_changed_on_parent() {}

void CanvasItem::item_rect_changed(bool p_size_changed) {}

Transform2D CanvasItem::get_canvas_transform() const { return Transform2D(); }

void CanvasItem::set_texture_filter(CanvasItem::TextureFilter) {}

void CanvasItem::_handle_visibility_change(bool) {}

void CanvasItem::_update_oversampling(bool) {}

void CanvasItem::set_texture_repeat(CanvasItem::TextureRepeat) {}

void CanvasItem::set_as_top_level(bool) {}

CanvasItem* CanvasItem::get_top_level() const
{
	CanvasItem ci = CanvasItem();
	return &ci;
}

void CanvasItem::_edit_set_scale(const Size2& p_scale) {}

Size2 CanvasItem::_edit_get_scale() const { return Size2(); }

Point2 CanvasItem::_edit_get_position() const { return Point2(); }

void CanvasItem::_edit_set_position(const Point2& p_position) {}

Transform2D CanvasItem::get_transform() const { return Transform2D(); }

void CanvasItem::_notification(int p_what) {}

// --- canvas_item.compat.inc ---

#ifndef DISABLE_DEPRECATED

#include "canvas_item.h"
#include "scene/resources/font.h"

void CanvasItem::_draw_char_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
	const String& p_char, int p_font_size, const Color& p_modulate) const
{
	draw_char(p_font.ptr(), p_pos, p_char, p_font_size, p_modulate, 0.0);
}

void CanvasItem::_draw_char_outline_bind_compat_104872(const Ref<Font>& p_font, const Point2& p_pos,
	const String& p_char, int p_font_size, int p_size, const Color& p_modulate) const
{
	draw_char_outline(p_font.ptr(), p_pos, p_char, p_font_size, p_size, p_modulate, 0.0);
}

void CanvasItem::_draw_circle_bind_compat_84472(
	const Point2& p_pos, real_t p_radius, const Color& p_color)
{
	draw_circle(p_pos, p_radius, p_color, true, -1.0, false);
}

void CanvasItem::_draw_rect_bind_compat_84523(
	const Rect2& p_rect, const Color& p_color, bool p_filled, real_t p_width)
{
	draw_rect(p_rect, p_color, p_filled, p_width, false);
}

void CanvasItem::_draw_dashed_line_bind_compat_84523(const Point2& p_from, const Point2& p_to,
	const Color& p_color, real_t p_width, real_t p_dash, bool p_aligned)
{
	draw_dashed_line(p_from, p_to, p_color, p_width, p_dash, p_aligned, false);
}

void CanvasItem::_draw_multiline_bind_compat_84523(
	const Vector<Point2>& p_points, const Color& p_color, real_t p_width)
{
	draw_multiline(p_points, p_color, p_width, false);
}

void CanvasItem::_draw_multiline_colors_bind_compat_84523(
	const Vector<Point2>& p_points, const Vector<Color>& p_colors, real_t p_width)
{
	draw_multiline_colors(p_points, p_colors, p_width, false);
}

void CanvasItem::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

#undef ERR_DRAW_GUARD

// ==========================================================================
// Merged from scene/main/canvas_layer.cpp
// ==========================================================================

#include "scene/main/canvas_item.h"
#include "scene/2d/line_2d.h"
#include "scene/main/canvas_item.h"
#include "scene/main/viewport.h"
#include "servers/rendering/renderer.h"

void CanvasLayer::_notification(int p_what)
{
	if (!parallax_enabled) {
		return;
	}

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		parallax.group_name = "__cameras_" + itos(get_viewport().get_id());
		add_to_group(parallax.group_name);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		remove_from_group(parallax.group_name);
	} break;
	}
}

void CanvasLayer::set_layer(int p_xform)
{
	layer = p_xform;
	if (viewport.is_valid()) {
		Renderer::viewport_set_canvas_stacking(
			viewport, canvas, layer, get_index());
		vp->gui_set_root_order_dirty();
	}
}

int CanvasLayer::get_layer() const { return layer; }

void CanvasLayer::show() { set_visible(true); }

void CanvasLayer::hide() { set_visible(false); }

bool CanvasLayer::is_visible() const { return visible; }

void CanvasLayer::set_transform(const Transform2D& p_xform)
{
	transform = p_xform;
	locrotscale_dirty = true;
	if (viewport.is_valid()) {
		Renderer::viewport_set_canvas_transform(
			viewport, canvas, transform);
	}
}

Transform2D CanvasLayer::get_transform() const { return transform; }

Transform2D CanvasLayer::get_final_transform() const
{
	if (is_following_viewport()) {
		Transform2D follow;
		follow.scale(Vector2(get_follow_viewport_scale(), get_follow_viewport_scale()));
		if (vp) {
			follow = vp->get_canvas_transform() * follow;
		}
		return follow * transform;
	}
	return transform;
}

void CanvasLayer::_update_xform()
{
	transform.set_rotation_and_scale(rot, scale);
	transform.set_origin(ofs);
	if (viewport.is_valid()) {
		Renderer::viewport_set_canvas_transform(
			viewport, canvas, transform);
	}
}

void CanvasLayer::_update_locrotscale() const
{
	ofs = transform.columns[2];
	rot = transform.get_rotation();
	scale = transform.get_scale();
	locrotscale_dirty = false;
}

void CanvasLayer::set_offset(const Vector2& p_offset)
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	ofs = p_offset;
	_update_xform();
}

Vector2 CanvasLayer::get_offset() const
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	return ofs;
}

void CanvasLayer::set_rotation(real_t p_radians)
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	rot = p_radians;
	_update_xform();
}

real_t CanvasLayer::get_rotation() const
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	return rot;
}

void CanvasLayer::set_scale(const Vector2& p_scale)
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	scale = p_scale;
	_update_xform();
}

Vector2 CanvasLayer::get_scale() const
{
	if (locrotscale_dirty) {
		_update_locrotscale();
	}

	return scale;
}

void CanvasLayer::update_draw_order()
{
	if (is_inside_tree()) {
		Renderer::viewport_set_canvas_stacking(
			viewport, canvas, layer, get_index());
	}
}

Size2 CanvasLayer::get_viewport_size() const
{
	if (!is_inside_tree()) {
		return Size2(1, 1);
	}

	ERR_FAIL_NULL_V_MSG(vp, Size2(1, 1), "Viewport is not initialized.");

	Rect2 r = vp->get_visible_rect();
	return r.size;
}

RID CanvasLayer::get_viewport() const { return viewport; }

Node* CanvasLayer::get_custom_viewport() const { return custom_viewport; }

void CanvasLayer::reset_sort_index() { sort_index = 0; }

int CanvasLayer::get_sort_index() { return sort_index++; }

RID CanvasLayer::get_canvas() const { return canvas; }

void CanvasLayer::set_follow_viewport(bool p_enable)
{
	if (follow_viewport == p_enable) {
		return;
	}

	follow_viewport = p_enable;
	_update_follow_viewport();
}

bool CanvasLayer::is_following_viewport() const { return follow_viewport; }

void CanvasLayer::set_follow_viewport_scale(float p_ratio)
{
	follow_viewport_scale = p_ratio;
	_update_follow_viewport();
}

float CanvasLayer::get_follow_viewport_scale() const { return follow_viewport_scale; }

void CanvasLayer::_update_follow_viewport(bool p_force_exit)
{
	if (!is_inside_tree()) {
		return;
	}
	if (p_force_exit || !follow_viewport) {
		Renderer::canvas_set_parent(canvas, RID(), 1.0);
	}
	else {
		Renderer::canvas_set_parent(
			canvas, vp->get_world_2d()->get_canvas(), follow_viewport_scale);
	}
}

void CanvasLayer::set_parallax_enabled(bool p_enabled)
{
	if (parallax_enabled == p_enabled) {
		return;
	}

	if (parallax_enabled && is_inside_tree() && !parallax.group_name.is_empty()) {
		remove_from_group(parallax.group_name);
	}

	parallax_enabled = p_enabled;

	if (parallax_enabled && is_inside_tree()) {
		parallax.group_name = "__cameras_" + itos(get_viewport().get_id());
		add_to_group(parallax.group_name);
		_update_scroll();
	}
}

void CanvasLayer::_camera_moved(const Transform2D& p_transform, const Point2& p_screen_offset,
	const Point2& p_adj_screen_offset)
{
	if (!parallax_enabled) {
		return;
	}

	parallax.screen_offset = p_screen_offset;

	set_scroll_scale(p_transform.get_scale().dot(Vector2(0.5, 0.5)));
	set_scroll_offset(p_transform.get_origin());
}

void CanvasLayer::set_scroll_scale(real_t p_scale) { parallax.scale = p_scale; }

real_t CanvasLayer::get_scroll_scale() const { return parallax.scale; }

void CanvasLayer::set_scroll_offset(const Point2& p_ofs)
{
	parallax.offset = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_scroll_offset() const { return parallax.offset; }

void CanvasLayer::set_scroll_base_offset(const Point2& p_ofs)
{
	parallax.base_offset = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_scroll_base_offset() const { return parallax.base_offset; }

void CanvasLayer::set_scroll_base_scale(const Point2& p_ofs)
{
	parallax.base_scale = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_scroll_base_scale() const { return parallax.base_scale; }

void CanvasLayer::set_limit_begin(const Point2& p_ofs)
{
	parallax.limit_begin = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_limit_begin() const { return parallax.limit_begin; }

void CanvasLayer::set_limit_end(const Point2& p_ofs)
{
	parallax.limit_end = p_ofs;
	_update_scroll();
}

Point2 CanvasLayer::get_limit_end() const { return parallax.limit_end; }

void CanvasLayer::set_ignore_camera_zoom(bool p_ignore) { parallax.ignore_camera_zoom = p_ignore; }

bool CanvasLayer::is_ignore_camera_zoom() const { return parallax.ignore_camera_zoom; }

Vector2 CanvasLayer::get_final_offset() const { return parallax.final_offset; }

void CanvasLayer::_update_scroll()
{
	if (!parallax_enabled || !is_inside_tree()) {
		return;
	}

	Vector2 scroll_ofs = parallax.base_offset + parallax.offset * parallax.base_scale;

	Size2 vps = get_viewport_size();
	scroll_ofs = -scroll_ofs;
	if (parallax.limit_begin.x < parallax.limit_end.x) {
		if (scroll_ofs.x < parallax.limit_begin.x) {
			scroll_ofs.x = parallax.limit_begin.x;
		}
		else if (scroll_ofs.x + vps.x > parallax.limit_end.x) {
			scroll_ofs.x = parallax.limit_end.x - vps.x;
		}
	}

	if (parallax.limit_begin.y < parallax.limit_end.y) {
		if (scroll_ofs.y < parallax.limit_begin.y) {
			scroll_ofs.y = parallax.limit_begin.y;
		}
		else if (scroll_ofs.y + vps.y > parallax.limit_end.y) {
			scroll_ofs.y = parallax.limit_end.y - vps.y;
		}
	}
	scroll_ofs = -scroll_ofs;

	parallax.final_offset = scroll_ofs;

	real_t scroll_scale = parallax.scale;
	for (int i = 0; i < get_child_count(); i++) {
		ParallaxLayer* l = dynamic_cast<ParallaxLayer*>(get_child(i));
		if (!l) {
			continue;
		}

		if (parallax.ignore_camera_zoom) {
			l->set_base_offset_and_scale(
				(scroll_ofs + parallax.screen_offset * (scroll_scale - 1.0)) / scroll_scale, 1.0);
		}
		else {
			l->set_base_offset_and_scale(scroll_ofs, scroll_scale);
		}
	}
}

CanvasLayer::CanvasLayer(bool p_parallax)
{
	canvas = Renderer::canvas_create();
	if (p_parallax) {
		parallax_enabled = true;
		set_layer(-100);
	}
}

CanvasLayer::~CanvasLayer()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(canvas);
}

void CanvasLayer::set_visible(bool) {}

void CanvasLayer::set_custom_viewport(Node*) {}

// ============================================================================
// Merged from scene/main/viewport.cpp
// ============================================================================

#include <execinfo.h>
#include <unistd.h>
#include "core/string/print_string.h"
#include "viewport.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/templates/pair.h"
#include "core/templates/sort_array.h"
#include "scene/gui/control.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/scroll_container.h"
#include "scene/main/canvas_item.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"
#include "scene/resources/font.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"

// 2D.
#include "scene/2d/node_2d.h"
#include "scene/main/viewport.h"

#ifndef _3D_DISABLED
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/node_3d.h"
#endif // _3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef XR_DISABLED
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

void ViewportTexture::reset_local_to_scene()
{
	vp_changed = true;

	if (vp) {
		vp->viewport_textures.erase(this);
		vp = nullptr;
	}

	if (proxy.is_valid() && proxy_ph.is_null()) {
		proxy_ph = Renderer::texture_2d_placeholder_create();
		Renderer::texture_proxy_update(proxy, proxy_ph);
	}
}

void ViewportTexture::set_viewport_path_in_scene(const NodePath& p_path)
{
	if (path == p_path) {
		return;
	}

	path = p_path;

	reset_local_to_scene();

	if (get_local_scene() && !path.is_empty()) {
		setup_local_to_scene();
	}
	else {
		if (path.is_empty()) {
			vp_changed = false;
		}
		emit_changed();
	}
}

NodePath ViewportTexture::get_viewport_path_in_scene() const { return path; }

int ViewportTexture::get_width() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return 0;
	}
	if (vp->is_sub_viewport()) {
		return vp->size.width;
	}
	return vp->size.width * vp->get_stretch_transform().get_scale().width;
}

int ViewportTexture::get_height() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return 0;
	}
	if (vp->is_sub_viewport()) {
		return vp->size.height;
	}
	return vp->size.height * vp->get_stretch_transform().get_scale().height;
}

Size2 ViewportTexture::get_size() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return Size2();
	}
	if (vp->is_sub_viewport()) {
		return vp->size;
	}
	Size2 scale = vp->get_stretch_transform().get_scale();
	return Size2(vp->size.width * scale.width, vp->size.height * scale.height).ceil();
}

RID ViewportTexture::get_rid() const
{
	if (proxy.is_null()) {
		proxy_ph = Renderer::texture_2d_placeholder_create();
		proxy = Renderer::texture_proxy_create(proxy_ph);
	}
	return proxy;
}

bool ViewportTexture::has_alpha() const { return false; }

Ref<Image> ViewportTexture::get_image() const
{
	if (!vp) {
		_err_print_viewport_not_set();
		return Ref<Image>();
	}
	return Renderer::texture_2d_get(vp->texture_rid);
}

void ViewportTexture::_err_print_viewport_not_set() const
{
	if (!vp_pending && !vp_changed) {
		ERR_PRINT("Viewport Texture must be set to use it.");
	}
}

ViewportTexture::ViewportTexture() { set_local_to_scene(true); }

ViewportTexture::~ViewportTexture()
{
	if (vp) {
		vp->viewport_textures.erase(this);
	}

	ERR_FAIL_NULL(Renderer::data);

	if (proxy_ph.is_valid()) {
		Renderer::free_rid(proxy_ph);
	}
	if (proxy.is_valid()) {
		Renderer::free_rid(proxy);
	}
}

void Viewport::_sub_window_update_order()
{
	if (gui.sub_windows.size() < 2) {
		return;
	}

	// Reorder 'always on top' windows.
	int last_index = gui.sub_windows.size() - 1;
	for (int index = last_index, insert_index = last_index; index >= 0; index--) {
		SubWindow sw = gui.sub_windows[index];
		Window* parent_window = sw.window->get_parent_visible_window();
		bool parent_is_always_on_top =
			(parent_window != nullptr) && parent_window->get_flag(Window::FLAG_ALWAYS_ON_TOP);
		if (sw.window->get_flag(Window::FLAG_ALWAYS_ON_TOP) ||
			(parent_is_always_on_top && sw.window->is_exclusive())) {
			if (index != insert_index) {
				gui.sub_windows.remove_at(index);
				gui.sub_windows.insert(insert_index, sw);
			}
			insert_index--;
		}
	}

	// Reorder exclusive children.
	for (int parent_index = 0; parent_index < gui.sub_windows.size(); parent_index++) {
		Window* exclusive_child = gui.sub_windows[parent_index].window->get_exclusive_child();
		if (exclusive_child != nullptr && exclusive_child->is_visible()) {
			int child_index = _sub_window_find(exclusive_child);
			if (child_index < parent_index) {
				SubWindow sw = gui.sub_windows[child_index];
				gui.sub_windows.remove_at(child_index);
				gui.sub_windows.insert(parent_index, sw);
			}
		}
	}

	for (int i = 0; i < gui.sub_windows.size(); i++) {
		Renderer::canvas_item_set_draw_index(gui.sub_windows[i].canvas_item, i);
	}
}

void Viewport::_sub_window_register(Window* p_window)
{
	ERR_FAIL_COND(!is_inside_tree());
	for (int i = 0; i < gui.sub_windows.size(); i++) {
		ERR_FAIL_COND(gui.sub_windows[i].window == p_window);
	}

	if (gui.sub_windows.is_empty()) {
		subwindow_canvas = Renderer::canvas_create();
		Renderer::viewport_attach_canvas(viewport, subwindow_canvas);
		Renderer::viewport_set_canvas_stacking(viewport, subwindow_canvas, SUBWINDOW_CANVAS_LAYER, 0);
	}
	SubWindow sw;
	sw.canvas_item = Renderer::canvas_item_create();
	Renderer::canvas_item_set_parent(sw.canvas_item, subwindow_canvas);
	sw.window = p_window;
	gui.sub_windows.push_back(sw);

	if (gui.subwindow_drag == SUB_WINDOW_DRAG_DISABLED) {
		if (p_window->get_flag(Window::FLAG_NO_FOCUS)) {
			_sub_window_update_order();
		}
		else {
			_sub_window_grab_focus(p_window);
		}
	}
	else {
		int index = _sub_window_find(gui.currently_dragged_subwindow);
		sw = gui.sub_windows[index];
		gui.sub_windows.remove_at(index);
		gui.sub_windows.push_back(sw);
		_sub_window_update_order();
	}

	Renderer::viewport_set_parent_viewport(p_window->viewport, viewport);
}

void Viewport::_sub_window_update(Window* p_window)
{
	int index = _sub_window_find(p_window);

	// _sub_window_update is sometimes called deferred, and the window may have been closed since
	// then. For example, when the user resizes the game window. In that case, _sub_window_find will
	// not find it, which is expected.
	if (index == -1) {
		return;
	}

	SubWindow& sw = gui.sub_windows.write[index];
	sw.pending_window_update = false;

	Renderer::canvas_item_clear(sw.canvas_item);
	const Rect2i r = Rect2i(p_window->get_position(), p_window->get_size());

	if (!p_window->get_flag(Window::FLAG_BORDERLESS)) {
		TextServer::set_current_drawn_item_oversampling(get_oversampling());

		Ref<StyleBox> panel = gui.subwindow_focused == p_window
								  ? p_window->theme_cache.embedded_border
								  : p_window->theme_cache.embedded_unfocused_border;
		panel->draw(sw.canvas_item, r);

		// Draw the title bar text.
		Ref<Font> title_font = p_window->theme_cache.title_font;
		int font_size = p_window->theme_cache.title_font_size;
		Color title_color = p_window->theme_cache.title_color;
		int title_height = p_window->theme_cache.title_height;
		int close_h_ofs = p_window->theme_cache.close_h_offset;
		int close_v_ofs = p_window->theme_cache.close_v_offset;

		const real_t title_space = r.size.width - panel->get_minimum_size().x - close_h_ofs;
		if (title_space > 0) {
			TextLine title_text = TextLine(p_window->get_displayed_title(), title_font, font_size);
			title_text.set_width(title_space);
			title_text.set_direction(
				p_window->is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR);
			int x = (r.size.width - title_text.get_size().x) / 2;
			int y = (-title_height - title_text.get_size().y) / 2;

			Color font_outline_color = p_window->theme_cache.title_outline_modulate;
			int outline_size = p_window->theme_cache.title_outline_size;
			if (outline_size > 0 && font_outline_color.a > 0) {
				title_text.draw_outline(
					sw.canvas_item, r.position + Point2(x, y), outline_size, font_outline_color);
			}
			title_text.draw(sw.canvas_item, r.position + Point2(x, y), title_color);
		}

		bool pressed = gui.subwindow_focused == sw.window &&
					   gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE &&
					   gui.subwindow_drag_close_inside;
		Ref<Texture2D> close_icon =
			pressed ? p_window->theme_cache.close_pressed : p_window->theme_cache.close;
		close_icon->draw(
			sw.canvas_item, r.position + Vector2(r.size.width - close_h_ofs, -close_v_ofs));

		TextServer::set_current_drawn_item_oversampling(0.0);
	}

	const Transform2D xform = sw.window->window_transform * sw.window->stretch_transform;
	Rect2 vr = xform.xform(sw.window->get_visible_rect());
	vr.position += p_window->get_position();
	if (vr != r) {
		Renderer::canvas_item_add_rect(sw.canvas_item, r, Color());
	}
	Renderer::canvas_item_add_texture_rect(sw.canvas_item, vr, sw.window->get_texture()->get_rid());
}

int Viewport::_sub_window_find(Window* p_window) const
{
	for (int i = 0; i < gui.sub_windows.size(); i++) {
		if (gui.sub_windows[i].window == p_window) {
			return i;
		}
	}

	return -1;
}

void Viewport::_update_viewport_path()
{
	if (!is_inside_tree()) {
		return;
	}

	for (ViewportTexture* E : viewport_textures) {
		Node* loc_scene = E->get_local_scene();
		if (loc_scene && loc_scene->is_inside_tree()) {
			E->path = loc_scene->get_path_to(this);
		}
	}
}

bool Viewport::_can_hide_focus_state()
{
	return Engine::is_editor_hint() ||
		   GLOBAL_GET_CACHED(int, "gui/common/show_focus_state_on_pointer_event") < 2;
}

RID Viewport::get_viewport_rid() const
{
	ERR_READ_THREAD_GUARD_V(RID());
	return viewport;
}

void Viewport::set_use_oversampling(bool p_oversampling)
{
	ERR_MAIN_THREAD_GUARD;
	if (use_font_oversampling == p_oversampling) {
		return;
	}
	use_font_oversampling = p_oversampling;
	_set_size(_get_size(), _get_view_count(), _get_size_2d_override(), _is_size_allocated());
}

bool Viewport::is_using_oversampling() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_font_oversampling;
}

void Viewport::set_oversampling_override(float p_oversampling)
{
	ERR_MAIN_THREAD_GUARD;
	if (font_oversampling_override == p_oversampling) {
		return;
	}
	font_oversampling_override = p_oversampling;
	_set_size(_get_size(), _get_view_count(), _get_size_2d_override(), _is_size_allocated());
}

float Viewport::get_oversampling_override() const
{
	ERR_READ_THREAD_GUARD_V(0.0);
	return font_oversampling_override;
}

void Viewport::_check_xr_size()
{
#ifndef XR_DISABLED
	// If our viewport has the use_xr flag set, our size and layout is managed by the XRServer.
	if (use_xr && XRServer::is_initialized()) {
		Ref<XRInterface> xr_interface = XRServer::get_primary_interface();
		if (xr_interface.is_valid() && xr_interface->is_initialized()) {
			Size2 xr_size = xr_interface->get_render_target_size();
			int xr_view_count = xr_interface->get_view_count();

			_set_size((Size2i)xr_size, xr_view_count, Size2i(0, 0), true);
		}
		else {
			// Set to default and prevent rendering for now (unless in editor, so we get a preview).
			bool is_editor = Engine::is_editor_hint();
			_set_size(is_editor ? Size2i(512, 512) : Size2i(0, 0), is_editor ? 1 : 0, Size2i(0, 0),
				false);
		}
	}
#endif // XR_DISABLED
}

Size2i Viewport::_get_size() const { return size; }

Size2 Viewport::_get_size_2d_override() const { return size_2d_override; }

int Viewport::_get_view_count() const { return view_count; }

bool Viewport::_is_size_allocated() const { return size_allocated; }

Rect2 Viewport::get_visible_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2());
	Rect2 r;

	if (size == Size2()) {
		r = Rect2(Point2(), DisplayServer::window_get_size());
	}
	else {
		r = Rect2(Point2(), size);
	}

	if (size_2d_override != Size2()) {
		r.size = size_2d_override;
	}
	return r;
}

void Viewport::set_canvas_transform(const Transform2D& p_transform)
{
	ERR_MAIN_THREAD_GUARD;
	canvas_transform = p_transform;

	Renderer::viewport_set_canvas_transform(
		viewport, find_world_2d()->get_canvas(), canvas_transform);
}

Transform2D Viewport::get_canvas_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return canvas_transform;
}

void Viewport::_update_global_transform()
{
	Transform2D sxform = stretch_transform * global_canvas_transform;

	Renderer::viewport_set_global_canvas_transform(viewport, sxform);
}

void Viewport::set_global_canvas_transform(const Transform2D& p_transform)
{
	ERR_MAIN_THREAD_GUARD;
	global_canvas_transform = p_transform;

	_update_global_transform();
}

Transform2D Viewport::get_global_canvas_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return global_canvas_transform;
}

void Viewport::_canvas_layer_add(CanvasLayer* p_canvas_layer)
{
	canvas_layers.insert(p_canvas_layer);
}

void Viewport::_canvas_layer_remove(CanvasLayer* p_canvas_layer)
{
	canvas_layers.erase(p_canvas_layer);
}

void Viewport::set_transparent_background(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	transparent_bg = p_enable;
	Renderer::viewport_set_transparent_background(viewport, p_enable);
}

bool Viewport::has_transparent_background() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return transparent_bg;
}

void Viewport::set_use_hdr_2d(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	use_hdr_2d = p_enable;
	Renderer::viewport_set_use_hdr_2d(viewport, p_enable);
}

bool Viewport::is_using_hdr_2d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_hdr_2d;
}

void Viewport::set_world_2d(const Ref<World2D>& p_world_2d)
{
	ERR_MAIN_THREAD_GUARD;
	if (world_2d == p_world_2d) {
		return;
	}

	if (is_inside_tree()) {
		Renderer::viewport_remove_canvas(viewport, current_canvas);
	}

	if (world_2d.is_valid()) {
		world_2d->remove_viewport(this);
	}

	if (p_world_2d.is_valid()) {
		bool do_propagate = world_2d.is_valid() && is_inside_tree();
		world_2d = p_world_2d;
		if (do_propagate) {
			_propagate_world_2d_changed(this);
		}
	}
	else {
		WARN_PRINT("Invalid world_2d");
		world_2d.instantiate();
	}

	world_2d->register_viewport(this);
	_update_audio_listener_2d();

	if (is_inside_tree()) {
		current_canvas = find_world_2d()->get_canvas();
		Renderer::viewport_attach_canvas(viewport, current_canvas);
	}
}

Ref<World2D> Viewport::find_world_2d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World2D>());
	if (world_2d.is_valid()) {
		return world_2d;
	}
	else if (parent) {
		return parent->find_world_2d();
	}
	else {
		return Ref<World2D>();
	}
}

Ref<World2D> Viewport::get_world_2d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World2D>());
	return world_2d;
}

Transform2D Viewport::get_stretch_transform() const { return stretch_transform; }

Transform2D Viewport::get_final_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return stretch_transform * global_canvas_transform;
}

Ref<ViewportTexture> Viewport::get_texture() const
{
	ERR_READ_THREAD_GUARD_V(Ref<ViewportTexture>());
	return default_texture;
}

void Viewport::set_positional_shadow_atlas_size(int p_size)
{
	ERR_MAIN_THREAD_GUARD;
	positional_shadow_atlas_size = p_size;
	Renderer::viewport_set_positional_shadow_atlas_size(
		viewport, p_size, positional_shadow_atlas_16_bits);
}

int Viewport::get_positional_shadow_atlas_size() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return positional_shadow_atlas_size;
}

void Viewport::set_positional_shadow_atlas_16_bits(bool p_16_bits)
{
	ERR_MAIN_THREAD_GUARD;
	if (positional_shadow_atlas_16_bits == p_16_bits) {
		return;
	}

	positional_shadow_atlas_16_bits = p_16_bits;
	Renderer::viewport_set_positional_shadow_atlas_size(
		viewport, positional_shadow_atlas_size, positional_shadow_atlas_16_bits);
}

bool Viewport::get_positional_shadow_atlas_16_bits() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return positional_shadow_atlas_16_bits;
}

void Viewport::set_positional_shadow_atlas_quadrant_subdiv(
	int p_quadrant, PositionalShadowAtlasQuadrantSubdiv p_subdiv)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_quadrant, 4);
	ERR_FAIL_INDEX(p_subdiv, SHADOW_ATLAS_QUADRANT_SUBDIV_MAX);

	if (positional_shadow_atlas_quadrant_subdiv[p_quadrant] == p_subdiv) {
		return;
	}

	positional_shadow_atlas_quadrant_subdiv[p_quadrant] = p_subdiv;
	static const int subdiv[SHADOW_ATLAS_QUADRANT_SUBDIV_MAX] = {0, 1, 4, 16, 64, 256, 1024};

	Renderer::viewport_set_positional_shadow_atlas_quadrant_subdivision(
		viewport, p_quadrant, subdiv[p_subdiv]);
}

Viewport::PositionalShadowAtlasQuadrantSubdiv Viewport::get_positional_shadow_atlas_quadrant_subdiv(
	int p_quadrant) const
{
	ERR_READ_THREAD_GUARD_V(SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	ERR_FAIL_INDEX_V(p_quadrant, 4, SHADOW_ATLAS_QUADRANT_SUBDIV_DISABLED);
	return positional_shadow_atlas_quadrant_subdiv[p_quadrant];
}

Vector2 Viewport::get_mouse_position() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	if (get_section_root_viewport() != SceneTree::get_root()) {
		// Rely on the most recent mouse coordinate from an InputEventMouse in push_input.
		// In this case get_screen_transform is not applicable, because it is ambiguous.
		return gui.last_mouse_pos;
	}
	else if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_MOUSE)) {
		Transform2D xform = get_screen_transform_internal(true);
		if (xform.determinant() == 0) {
			// Screen transform can be non-invertible when the Window is minimized.
			return Vector2();
		}
		return xform.affine_inverse().xform(DisplayServer::mouse_get_position());
	}
	else {
		// Fallback to Input for getting mouse position in case of emulated mouse.
		return get_screen_transform_internal().affine_inverse().xform(
			Input::get_mouse_position());
	}
}

void Viewport::warp_mouse(const Vector2& p_position)
{
	ERR_MAIN_THREAD_GUARD;
	Transform2D xform = get_screen_transform_internal();
	Vector2 gpos = xform.xform(p_position);
	Input::warp_mouse(gpos);
}

Point2 Viewport::wrap_mouse_in_rect(const Vector2& p_relative, const Rect2& p_rect)
{
	// Move the mouse cursor from its current position to a location bounded by `p_rect`
	// in accordance with a heuristic that takes the traveled distance `p_relative` of the mouse
	// into account.

	// All parameters are in viewport coordinates.
	// p_relative denotes the distance to the previous mouse position.
	// p_rect denotes the area, in which the mouse should be confined in.

	// The relative distance reported for the next event after a warp is in the boundaries of the
	// size of the rect on that axis, but it may be greater, in which case there's no problem as
	// fmod() will warp it, but if the pointer has moved in the opposite direction between the
	// pointer relocation and the subsequent event, the reported relative distance will be less
	// than the size of the rect and thus fmod() will be disabled for handling the situation.
	// And due to this mouse warping mechanism being stateless, we need to apply some heuristics
	// to detect the warp: if the relative distance is greater than the half of the size of the
	// relevant rect (checked per each axis), it will be considered as the consequence of a former
	// pointer warp.

	const Point2 rel_sign(p_relative.x >= 0.0f ? 1 : -1, p_relative.y >= 0.0 ? 1 : -1);
	const Size2 warp_margin = p_rect.size * 0.5f;
	const Point2 rel_warped(Math::fmod(p_relative.x + rel_sign.x * warp_margin.x, p_rect.size.x) -
								rel_sign.x * warp_margin.x,
		Math::fmod(p_relative.y + rel_sign.y * warp_margin.y, p_rect.size.y) -
			rel_sign.y * warp_margin.y);

	const Point2 pos_local = get_mouse_position() - p_rect.position;
	const Point2 pos_warped(
		Math::fposmod(pos_local.x, p_rect.size.x), Math::fposmod(pos_local.y, p_rect.size.y));
	if (pos_warped != pos_local) {
		warp_mouse(pos_warped + p_rect.position);
	}

	return rel_warped;
}

void Viewport::_gui_cancel_tooltip()
{
	gui.tooltip_control = nullptr;
	gui.tooltip_text = "";

	if (gui.tooltip_timer.is_valid()) {
		gui.tooltip_timer->release_connections();
		gui.tooltip_timer = Ref<SceneTreeTimer>();
	}
	if (gui.tooltip_popup) {
		gui.tooltip_popup->queue_free();
	}
}

void Viewport::cancel_tooltip() { _gui_cancel_tooltip(); }

void Viewport::show_tooltip(Control* p_control)
{
	if (!p_control) {
		return;
	}

	if (gui.tooltip_timer.is_valid()) {
		gui.tooltip_timer->release_connections();
		gui.tooltip_timer = Ref<SceneTreeTimer>();
	}
	gui.tooltip_control = p_control;
	_gui_show_tooltip_at(p_control->get_size() / 2);
}

void Viewport::_gui_show_tooltip() { _gui_show_tooltip_at(gui.last_mouse_pos); }

// `gui_find_control` doesn't take embedded windows into account. So the caller of this function
// needs to make sure, that there is no embedded window at the specified position.
Control* Viewport::gui_find_control(const Point2& p_global)
{
	ERR_MAIN_THREAD_GUARD_V(nullptr);

	_gui_sort_roots();

	for (List<Control*>::Element* E = gui.roots.back(); E; E = E->prev()) {
		Control* sw = E->get();
		if (!sw->is_visible_in_tree()) {
			continue;
		}

		Transform2D xform;
		CanvasItem* pci = sw->get_parent_item();
		if (pci) {
			xform = pci->get_global_transform_with_canvas();
		}
		else {
			xform = sw->get_canvas_transform();
		}

		Control* ret = _gui_find_control_at_pos(sw, p_global, xform);
		if (ret) {
			return ret;
		}
	}

	return nullptr;
}

void Viewport::_perform_drop(Control* p_control)
{
	gui_perform_drop_at(p_control ? p_control->get_local_mouse_position() : Vector2(), p_control);
}

List<Control*>::Element* Viewport::_gui_add_root_control(Control* p_control)
{
	gui.roots_order_dirty = true;
	return gui.roots.push_back(p_control);
}

void Viewport::gui_set_root_order_dirty()
{
	ERR_MAIN_THREAD_GUARD;
	gui.roots_order_dirty = true;
}

void Viewport::_gui_force_drag_start()
{
	Viewport* section_root = get_section_root_viewport();
	section_root->gui.global_dragging = true;
}

void Viewport::_gui_force_drag_cancel()
{
	Viewport* section_root = get_section_root_viewport();
	section_root->gui.global_dragging = false;
}

void Viewport::_gui_remove_root_control(List<Control*>::Element* RI) { gui.roots.erase(RI); }

void Viewport::_gui_unfocus_control(Control* p_control)
{
	if (gui.key_focus == p_control) {
		gui.key_focus->release_focus();
	}
}

void Viewport::canvas_item_top_level_changed() { _gui_update_mouse_over(); }

void Viewport::_gui_remove_focus_for_window(Node* p_window)
{
	if (get_base_window() == p_window) {
		gui_release_focus();
	}
}

bool Viewport::_gui_control_has_focus(const Control* p_control, bool p_ignore_hidden_focus)
{
	return (!p_ignore_hidden_focus || !gui.hide_focus) && gui.key_focus == p_control;
}

void Viewport::_gui_accept_event()
{
	if (is_inside_tree()) {
		set_input_as_handled();
	}
}

void Viewport::push_text_input(const String& p_text) { _push_text_input(p_text, false); }

Viewport::SubWindowResize Viewport::_sub_window_get_resize_margin(
	Window* p_subwindow, const Point2& p_point)
{
	if (p_subwindow->get_flag(Window::FLAG_BORDERLESS) ||
		p_subwindow->get_flag(Window::FLAG_RESIZE_DISABLED)) {
		return SUB_WINDOW_RESIZE_DISABLED;
	}

	Rect2i r = Rect2i(p_subwindow->get_position(), p_subwindow->get_size());

	int title_height = p_subwindow->theme_cache.title_height;

	r.position.y -= title_height;
	r.size.y += title_height;

	if (r.has_point(p_point)) {
		return SUB_WINDOW_RESIZE_DISABLED; // It's inside, so no resize.
	}

	int dist_x =
		p_point.x < r.position.x
			? (p_point.x - r.position.x)
			: (p_point.x > (r.position.x + r.size.x) ? (p_point.x - (r.position.x + r.size.x)) : 0);
	int dist_y =
		p_point.y < r.position.y
			? (p_point.y - r.position.y)
			: (p_point.y > (r.position.y + r.size.y) ? (p_point.y - (r.position.y + r.size.y)) : 0);

	int limit = p_subwindow->theme_cache.resize_margin;

	if (Math::abs(dist_x) > limit) {
		return SUB_WINDOW_RESIZE_DISABLED;
	}

	if (Math::abs(dist_y) > limit) {
		return SUB_WINDOW_RESIZE_DISABLED;
	}

	if (dist_x < 0 && dist_y < 0) {
		return SUB_WINDOW_RESIZE_TOP_LEFT;
	}

	if (dist_x == 0 && dist_y < 0) {
		return SUB_WINDOW_RESIZE_TOP;
	}

	if (dist_x > 0 && dist_y < 0) {
		return SUB_WINDOW_RESIZE_TOP_RIGHT;
	}

	if (dist_x < 0 && dist_y == 0) {
		return SUB_WINDOW_RESIZE_LEFT;
	}

	if (dist_x > 0 && dist_y == 0) {
		return SUB_WINDOW_RESIZE_RIGHT;
	}

	if (dist_x < 0 && dist_y > 0) {
		return SUB_WINDOW_RESIZE_BOTTOM_LEFT;
	}

	if (dist_x == 0 && dist_y > 0) {
		return SUB_WINDOW_RESIZE_BOTTOM;
	}

	if (dist_x > 0 && dist_y > 0) {
		return SUB_WINDOW_RESIZE_BOTTOM_RIGHT;
	}

	return SUB_WINDOW_RESIZE_DISABLED;
}

bool Viewport::_sub_windows_forward_input(const InputEvent& p_event)
{
	if (gui.subwindow_drag != SUB_WINDOW_DRAG_DISABLED) {
		ERR_FAIL_NULL_V(gui.currently_dragged_subwindow, false);

		if (p_event.type == InputEventType::MOUSE_BUTTON && !p_event.is_pressed() &&
			p_event.mouse_button.button_index == MouseButton::LEFT) {
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE) {
				if (gui.subwindow_drag_close_rect.has_point(p_event.mouse_button.position)) {
					// Close window.
					gui.currently_dragged_subwindow->_event_callback(
						DisplayServerEnums::WINDOW_EVENT_CLOSE_REQUEST);
				}
			}
			gui.subwindow_drag = SUB_WINDOW_DRAG_DISABLED;
			if (gui.currently_dragged_subwindow != nullptr) { // May have been erased.
				_sub_window_update(gui.currently_dragged_subwindow);
				gui.currently_dragged_subwindow = nullptr;
			}
		}

		if (p_event.type == InputEventType::MOUSE_MOTION) {
			const Vector2 mm_pos = p_event.mouse_motion.position;
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_MOVE) {
				Vector2 diff = mm_pos - gui.subwindow_drag_from;
				Rect2i new_rect(
					gui.subwindow_drag_pos + diff, gui.currently_dragged_subwindow->get_size());

				if (gui.currently_dragged_subwindow->is_clamped_to_embedder()) {
					new_rect = gui.currently_dragged_subwindow->fit_rect_in_parent(
						new_rect, get_visible_rect());
				}

				gui.currently_dragged_subwindow->_rect_changed_callback(new_rect);

				if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_CURSOR_SHAPE)) {
					DisplayServer::cursor_set_shape(DisplayServerEnums::CURSOR_MOVE);
				}
			}
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE) {
				gui.subwindow_drag_close_inside = gui.subwindow_drag_close_rect.has_point(mm_pos);
			}
			if (gui.subwindow_drag == SUB_WINDOW_DRAG_RESIZE) {
				Vector2i diff = mm_pos - gui.subwindow_drag_from;
				Size2i min_size = gui.currently_dragged_subwindow->get_min_size();
				Size2i min_size_clamped =
					gui.currently_dragged_subwindow->get_clamped_minimum_size();

				min_size_clamped = min_size_clamped.maxi(1);

				Rect2i r = gui.subwindow_resize_from_rect;

				Size2i limit = r.size - min_size_clamped;

				switch (gui.subwindow_resize_mode) {
				case SUB_WINDOW_RESIZE_TOP_LEFT: {
					diff.x = MIN(diff.x, limit.x);
					diff.y = MIN(diff.y, limit.y);
					r.position += diff;
					r.size -= diff;
				} break;
				case SUB_WINDOW_RESIZE_TOP: {
					diff.x = 0;
					diff.y = MIN(diff.y, limit.y);
					r.position += diff;
					r.size -= diff;
				} break;
				case SUB_WINDOW_RESIZE_TOP_RIGHT: {
					diff.x = MAX(diff.x, -limit.x);
					diff.y = MIN(diff.y, limit.y);
					r.position.y += diff.y;
					r.size.y -= diff.y;
					r.size.x += diff.x;
				} break;
				case SUB_WINDOW_RESIZE_LEFT: {
					diff.x = MIN(diff.x, limit.x);
					diff.y = 0;
					r.position += diff;
					r.size -= diff;

				} break;
				case SUB_WINDOW_RESIZE_RIGHT: {
					diff.x = MAX(diff.x, -limit.x);
					r.size.x += diff.x;
				} break;
				case SUB_WINDOW_RESIZE_BOTTOM_LEFT: {
					diff.x = MIN(diff.x, limit.x);
					diff.y = MAX(diff.y, -limit.y);
					r.position.x += diff.x;
					r.size.x -= diff.x;
					r.size.y += diff.y;

				} break;
				case SUB_WINDOW_RESIZE_BOTTOM: {
					diff.y = MAX(diff.y, -limit.y);
					r.size.y += diff.y;
				} break;
				case SUB_WINDOW_RESIZE_BOTTOM_RIGHT: {
					diff.x = MAX(diff.x, -limit.x);
					diff.y = MAX(diff.y, -limit.y);
					r.size += diff;

				} break;
				default: {
				}
				}

				Size2i max_size = gui.currently_dragged_subwindow->get_max_size();
				if ((max_size.x > 0 || max_size.y > 0) &&
					(max_size.x >= min_size.x && max_size.y >= min_size.y)) {
					max_size = max_size.maxi(1);

					if (r.size.x > max_size.x) {
						r.size.x = max_size.x;
					}
					if (r.size.y > max_size.y) {
						r.size.y = max_size.y;
					}
				}

				gui.currently_dragged_subwindow->_rect_changed_callback(r);
			}

			if (gui.currently_dragged_subwindow) { // May have been erased.
				_sub_window_update(gui.currently_dragged_subwindow);
			}
		}

		return true; // Handled.
	}
	// If the event is a mouse button, we need to check whether another window was clicked.
	if (p_event.type == InputEventType::MOUSE_BUTTON && p_event.is_pressed() &&
		p_event.mouse_button.button_index == MouseButton::LEFT) {
		const Vector2 mb_pos = p_event.mouse_button.position;
		Window* click_on_window = nullptr;
		for (int i = gui.sub_windows.size() - 1; i >= 0; i--) {
			SubWindow sw = gui.sub_windows.write[i];

			// Clicked inside window?

			Rect2i r = Rect2i(sw.window->get_position(), sw.window->get_size());

			if (!sw.window->get_flag(Window::FLAG_BORDERLESS)) {
				// Check top bar.
				int title_height = sw.window->theme_cache.title_height;
				Rect2i title_bar = r;
				title_bar.position.y -= title_height;
				title_bar.size.y = title_height;

				if (title_bar.size.y > 0 && title_bar.has_point(mb_pos)) {
					click_on_window = sw.window;

					int close_h_ofs = sw.window->theme_cache.close_h_offset;
					int close_v_ofs = sw.window->theme_cache.close_v_offset;
					bool pressed = gui.subwindow_focused == sw.window &&
								   gui.subwindow_drag == SUB_WINDOW_DRAG_CLOSE &&
								   gui.subwindow_drag_close_inside;
					Ref<Texture2D> close_icon = pressed ? sw.window->theme_cache.close_pressed
														: sw.window->theme_cache.close;

					Rect2 close_rect;
					close_rect.position =
						Vector2(r.position.x + r.size.x - close_h_ofs, r.position.y - close_v_ofs);
					close_rect.size = close_icon->get_size();

					if (gui.subwindow_focused != sw.window) {
						// Refocus.
						_sub_window_grab_focus(sw.window);
					}

					if (close_rect.has_point(mb_pos)) {
						gui.subwindow_drag = SUB_WINDOW_DRAG_CLOSE;
						gui.subwindow_drag_close_inside = true; // Starts inside.
						gui.subwindow_drag_close_rect = close_rect;
					}
					else {
						gui.subwindow_drag = SUB_WINDOW_DRAG_MOVE;
					}

					gui.subwindow_drag_from = mb_pos;
					gui.subwindow_drag_pos = sw.window->get_position();

					_sub_window_update(sw.window);
				}
				else {
					gui.subwindow_resize_mode =
						_sub_window_get_resize_margin(sw.window, mb_pos);
					if (gui.subwindow_resize_mode != SUB_WINDOW_RESIZE_DISABLED) {
						if (gui.subwindow_focused != sw.window) {
							// Refocus.
							_sub_window_grab_focus(sw.window);
						}

						gui.subwindow_resize_from_rect = r;
						gui.subwindow_drag_from = mb_pos;
						gui.subwindow_drag = SUB_WINDOW_DRAG_RESIZE;
						click_on_window = sw.window;
					}
				}
			}
			if (!click_on_window && r.has_point(mb_pos)) {
				// Clicked, see if it needs to fetch focus.
				if (gui.subwindow_focused != sw.window) {
					// Refocus.
					_sub_window_grab_focus(sw.window);
				}

				click_on_window = sw.window;
			}

			if (click_on_window) {
				break;
			}
		}

		gui.currently_dragged_subwindow = click_on_window;

		if (!click_on_window && gui.subwindow_focused) {
			// No window found and clicked, remove focus.
			_sub_window_grab_focus(nullptr);
		}
	}

	if (gui.subwindow_focused && p_event.type == InputEventType::MOUSE_MOTION) {
		SubWindowResize resize = _sub_window_get_resize_margin(
			gui.subwindow_focused, p_event.mouse_motion.position);
		if (resize != SUB_WINDOW_RESIZE_DISABLED) {
			DisplayServerEnums::CursorShape shapes[SUB_WINDOW_RESIZE_MAX] = {
				DisplayServerEnums::CURSOR_ARROW, DisplayServerEnums::CURSOR_FDIAGSIZE,
				DisplayServerEnums::CURSOR_VSIZE, DisplayServerEnums::CURSOR_BDIAGSIZE,
				DisplayServerEnums::CURSOR_HSIZE, DisplayServerEnums::CURSOR_HSIZE,
				DisplayServerEnums::CURSOR_BDIAGSIZE, DisplayServerEnums::CURSOR_VSIZE,
				DisplayServerEnums::CURSOR_FDIAGSIZE};

			if (DisplayServer::has_feature(DisplayServerEnums::FEATURE_CURSOR_SHAPE)) {
				DisplayServer::cursor_set_shape(shapes[resize]);
			}

			return true; // Reserved for showing the resize cursor.
		}
	}

	if (gui.subwindow_drag != SUB_WINDOW_DRAG_DISABLED) {
		return true; // Dragging, don't pass the event.
	}

	if (!gui.subwindow_focused) {
		// No window focus, check for unfocusable windows under the cursor.
		if (p_event.is_mouse()) {
			const Vector2 me_pos = p_event.get_mouse_position();
			for (int i = gui.sub_windows.size() - 1; i >= 0; i--) {
				const SubWindow& sw = gui.sub_windows[i];
				if (!sw.window->get_flag(Window::FLAG_NO_FOCUS) ||
					sw.window->get_flag(Window::FLAG_MOUSE_PASSTHROUGH)) {
					continue;
				}
				Rect2i r = Rect2i(sw.window->get_position(), sw.window->get_size());
				if (r.has_point(me_pos)) {
					Transform2D window_ofs;
					window_ofs.set_origin(-sw.window->get_position());
					InputEvent ev = p_event.xformed_by(window_ofs);
					sw.window->_window_input(ev);
					return true;
				}
			}
		}
		return false;
	}

	Transform2D window_ofs;
	window_ofs.set_origin(-gui.subwindow_focused->get_position());

	InputEvent ev = p_event.xformed_by(window_ofs);

	gui.subwindow_focused->_window_input(ev);

	return true;
}

void Viewport::_window_start_drag(Window* p_window)
{
	int index = _sub_window_find(p_window);
	ERR_FAIL_COND(index == -1);

	SubWindow sw = gui.sub_windows.write[index];

	if (gui.subwindow_focused != sw.window) {
		// Refocus.
		_sub_window_grab_focus(sw.window);
	}

	gui.subwindow_drag = SUB_WINDOW_DRAG_MOVE;
	gui.subwindow_drag_from = get_mouse_position();
	gui.subwindow_drag_pos = sw.window->get_position();
	gui.currently_dragged_subwindow = sw.window;

	_sub_window_update(sw.window);
}

void Viewport::_window_start_resize(SubWindowResize p_edge, Window* p_window)
{
	int index = _sub_window_find(p_window);
	ERR_FAIL_COND(index == -1);

	SubWindow sw = gui.sub_windows.write[index];
	Rect2i r = Rect2i(sw.window->get_position(), sw.window->get_size());

	if (gui.subwindow_focused != sw.window) {
		// Refocus.
		_sub_window_grab_focus(sw.window);
	}

	gui.subwindow_drag = SUB_WINDOW_DRAG_RESIZE;
	gui.subwindow_resize_mode = p_edge;
	gui.subwindow_resize_from_rect = r;
	gui.subwindow_drag_from = get_mouse_position();
	gui.subwindow_drag_pos = sw.window->get_position();
	gui.currently_dragged_subwindow = sw.window;

	_sub_window_update(sw.window);
}

void Viewport::_update_mouse_over(const InputEvent& p_mm)
{
	if (!p_mm.is_mouse() || is_attached_in_viewport()) {
		return;
	}

	const Vector2 mm_pos = p_mm.get_mouse_position();
	if (SceneTree::get_root()->is_embedding_subwindows() || is_sub_viewport()) {
		_update_mouse_over(mm_pos);
	}
	else {
		Window* receiving_window = SceneTree::get_root()->gui.windowmanager_window_over;
		if (!receiving_window) {
			return;
		}
		if (receiving_window->get_window_id() != p_mm.window_id) {
			Vector2 pos = DisplayServer::mouse_get_position() - receiving_window->get_position();
			pos = receiving_window->get_final_transform().affine_inverse().xform(pos);
			receiving_window->_update_mouse_over(pos);
		}
		else {
			receiving_window->_update_mouse_over(mm_pos);
		}
	}
}

#ifndef DISABLE_DEPRECATED
void Viewport::push_unhandled_input(const InputEvent& p_event, bool p_local_coords)
{
	ERR_MAIN_THREAD_GUARD;
	WARN_DEPRECATED_MSG(
		R"*(The "push_unhandled_input()" method is deprecated, use "push_input()" instead.)*");
	ERR_FAIL_COND(!is_inside_tree());

	local_input_handled = false;

	if (disable_input || disable_input_override || !_can_consume_input_events()) {
		return;
	}

	if (Engine::is_editor_hint() && SceneTree::get_edited_scene_root() &&
		SceneTree::get_edited_scene_root()->is_ancestor_of(this)) {
		return;
	}

	InputEvent ev = p_local_coords ? p_event : _make_input_local(p_event);
	_push_unhandled_input_internal(ev);
}
#endif // DISABLE_DEPRECATED

void Viewport::notify_mouse_exited()
{
	if (!gui.mouse_in_viewport) {
		WARN_PRINT_ED("The Viewport was previously notified that the mouse has left its area. "
					  "There is no need to notify it at this time.");
		return;
	}
	_mouse_leave_viewport();
}

#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
void Viewport::set_physics_object_picking(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	physics_object_picking = p_enable;
	if (physics_object_picking) {
		add_to_group("_picking_viewports");
	}
	else {
		physics_picking_events.clear();
		if (is_in_group("_picking_viewports")) {
			remove_from_group("_picking_viewports");
		}
	}
}

bool Viewport::get_physics_object_picking()
{
	ERR_READ_THREAD_GUARD_V(false);
	return physics_object_picking;
}

void Viewport::set_physics_object_picking_sort(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	physics_object_picking_sort = p_enable;
}

bool Viewport::get_physics_object_picking_sort()
{
	ERR_READ_THREAD_GUARD_V(false);
	return physics_object_picking_sort;
}

void Viewport::set_physics_object_picking_first_only(bool p_enable)
{
	physics_object_picking_first_only = p_enable;
}

bool Viewport::get_physics_object_picking_first_only() { return physics_object_picking_first_only; }
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)

Vector2 Viewport::get_camera_coords(const Vector2& p_viewport_coords) const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	Transform2D xf = stretch_transform * global_canvas_transform;
	return xf.xform(p_viewport_coords);
}

Vector2 Viewport::get_camera_rect_size() const
{
	ERR_READ_THREAD_GUARD_V(Vector2());
	return size;
}

void Viewport::set_disable_input(bool p_disable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_disable == disable_input) {
		return;
	}
	if (p_disable && !disable_input_override) {
		_drop_mouse_focus();
		_mouse_leave_viewport();
		_gui_cancel_tooltip();
	}
	disable_input = p_disable;
}

bool Viewport::is_input_disabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return disable_input;
}

void Viewport::set_disable_input_override(bool p_disable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_disable == disable_input_override) {
		return;
	}
	if (p_disable && !disable_input) {
		_drop_mouse_focus();
		_mouse_leave_viewport();
		_gui_cancel_tooltip();
	}
	disable_input_override = p_disable;
}

String Viewport::gui_get_drag_description() const
{
	ERR_READ_THREAD_GUARD_V(String());
	if (get_section_root_viewport()->gui.drag_description.is_empty()) {
		return RTR("Drag-and-drop data");
	}
	else {
		return get_section_root_viewport()->gui.drag_description;
	}
}

void Viewport::gui_set_drag_description(const String& p_description)
{
	gui.drag_description = p_description;
}

PackedStringArray Viewport::get_configuration_warnings() const
{
	ERR_MAIN_THREAD_GUARD_V(PackedStringArray());
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (size.x <= 1 || size.y <= 1) {
		warnings.push_back(RTR("The Viewport size must be greater than or equal to 2 pixels on "
							   "both dimensions to render anything."));
	}
	return warnings;
}

void Viewport::gui_reset_canvas_sort_index()
{
	ERR_MAIN_THREAD_GUARD;
	gui.canvas_sort_index = 0;
}

int Viewport::gui_get_canvas_sort_index()
{
	ERR_MAIN_THREAD_GUARD_V(0);
	return gui.canvas_sort_index++;
}

Control* Viewport::gui_get_focus_owner() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return gui.key_focus;
}

void Viewport::set_msaa_2d(MSAA p_msaa)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_msaa, MSAA_MAX);
	if (msaa_2d == p_msaa) {
		return;
	}
	msaa_2d = p_msaa;
	Renderer::viewport_set_msaa_2d(viewport, RSE::ViewportMSAA(p_msaa));
}

Viewport::MSAA Viewport::get_msaa_2d() const
{
	ERR_READ_THREAD_GUARD_V(MSAA_DISABLED);
	return msaa_2d;
}

void Viewport::set_msaa_3d(MSAA p_msaa)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_msaa, MSAA_MAX);
	if (msaa_3d == p_msaa) {
		return;
	}
	msaa_3d = p_msaa;
	Renderer::viewport_set_msaa_3d(viewport, RSE::ViewportMSAA(p_msaa));
}

Viewport::MSAA Viewport::get_msaa_3d() const
{
	ERR_READ_THREAD_GUARD_V(MSAA_DISABLED);
	return msaa_3d;
}

void Viewport::set_screen_space_aa(ScreenSpaceAA p_screen_space_aa)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_screen_space_aa, SCREEN_SPACE_AA_MAX);
	if (screen_space_aa == p_screen_space_aa) {
		return;
	}
	screen_space_aa = p_screen_space_aa;
	Renderer::viewport_set_screen_space_aa(viewport, RSE::ViewportScreenSpaceAA(p_screen_space_aa));
}

Viewport::ScreenSpaceAA Viewport::get_screen_space_aa() const
{
	ERR_READ_THREAD_GUARD_V(SCREEN_SPACE_AA_DISABLED);
	return screen_space_aa;
}

void Viewport::set_use_taa(bool p_use_taa)
{
	ERR_MAIN_THREAD_GUARD;
	if (use_taa == p_use_taa) {
		return;
	}
	use_taa = p_use_taa;
	Renderer::viewport_set_use_taa(viewport, p_use_taa);
}

bool Viewport::is_using_taa() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_taa;
}

void Viewport::set_use_debanding(bool p_use_debanding)
{
	ERR_MAIN_THREAD_GUARD;
	if (use_debanding == p_use_debanding) {
		return;
	}
	use_debanding = p_use_debanding;
	Renderer::viewport_set_use_debanding(viewport, p_use_debanding);
}

bool Viewport::is_using_debanding() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_debanding;
}

void Viewport::set_mesh_lod_threshold(float p_pixels)
{
	ERR_MAIN_THREAD_GUARD;
	mesh_lod_threshold = p_pixels;
	Renderer::viewport_set_mesh_lod_threshold(viewport, mesh_lod_threshold);
}

float Viewport::get_mesh_lod_threshold() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return mesh_lod_threshold;
}

bool Viewport::is_using_occlusion_culling() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_occlusion_culling;
}

void Viewport::set_debug_draw(DebugDraw p_debug_draw)
{
	ERR_MAIN_THREAD_GUARD;
	debug_draw = p_debug_draw;
	Renderer::viewport_set_debug_draw(viewport, RSE::ViewportDebugDraw(p_debug_draw));
}

Viewport::DebugDraw Viewport::get_debug_draw() const
{
	ERR_READ_THREAD_GUARD_V(DEBUG_DRAW_DISABLED);
	return debug_draw;
}

int Viewport::get_render_info(RenderInfoType p_type, RenderInfo p_info)
{
	ERR_READ_THREAD_GUARD_V(0);
	return Renderer::viewport_get_render_info(
		viewport, RSE::ViewportRenderInfoType(p_type), RSE::ViewportRenderInfo(p_info));
}

void Viewport::set_snap_controls_to_pixels(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	snap_controls_to_pixels = p_enable;
}

bool Viewport::is_snap_controls_to_pixels_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return snap_controls_to_pixels;
}

void Viewport::set_snap_2d_transforms_to_pixel(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	snap_2d_transforms_to_pixel = p_enable;
	Renderer::viewport_set_snap_2d_transforms_to_pixel(viewport, snap_2d_transforms_to_pixel);
}

bool Viewport::is_snap_2d_transforms_to_pixel_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return snap_2d_transforms_to_pixel;
}

void Viewport::set_snap_2d_vertices_to_pixel(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	snap_2d_vertices_to_pixel = p_enable;
	Renderer::viewport_set_snap_2d_vertices_to_pixel(viewport, snap_2d_vertices_to_pixel);
}

bool Viewport::is_snap_2d_vertices_to_pixel_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return snap_2d_vertices_to_pixel;
}

bool Viewport::gui_is_dragging() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return get_section_root_viewport()->gui.global_dragging;
}

bool Viewport::gui_is_drag_successful() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return gui.drag_successful;
}

void Viewport::gui_cancel_drag()
{
	ERR_MAIN_THREAD_GUARD;
	if (gui_is_dragging()) {
		_perform_drop();
	}
}

void Viewport::set_handle_input_locally(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	handle_input_locally = p_enable;
}

bool Viewport::is_handling_input_locally() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return handle_input_locally;
}

void Viewport::set_propagate_shortcuts_to_parent(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	propagate_shortcuts_to_parent = p_enable;
}

bool Viewport::gui_shortcut_use_focus_owner() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return shortcut_use_focus_owner;
}

void Viewport::set_default_canvas_item_texture_filter(DefaultCanvasItemTextureFilter p_filter)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_filter, DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_MAX);

	if (default_canvas_item_texture_filter == p_filter) {
		return;
	}
	default_canvas_item_texture_filter = p_filter;
	_update_texture_filter_changed(true);
}

Viewport::DefaultCanvasItemTextureFilter Viewport::get_default_canvas_item_texture_filter() const
{
	ERR_READ_THREAD_GUARD_V(DEFAULT_CANVAS_ITEM_TEXTURE_FILTER_NEAREST);
	return default_canvas_item_texture_filter;
}

RenderingServerEnums::CanvasItemTextureFilter Viewport::get_texture_filter_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR);
	_refresh_texture_filter_cache();
	return default_canvas_item_texture_filter_cache;
}

void Viewport::set_default_canvas_item_texture_repeat(DefaultCanvasItemTextureRepeat p_repeat)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_repeat, DEFAULT_CANVAS_ITEM_TEXTURE_REPEAT_MAX);

	if (default_canvas_item_texture_repeat == p_repeat) {
		return;
	}

	default_canvas_item_texture_repeat = p_repeat;
	_update_texture_repeat_changed(true);
}

Viewport::DefaultCanvasItemTextureRepeat Viewport::get_default_canvas_item_texture_repeat() const
{
	ERR_READ_THREAD_GUARD_V(DEFAULT_CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
	return default_canvas_item_texture_repeat;
}

RenderingServerEnums::CanvasItemTextureRepeat Viewport::get_texture_repeat_in_tree() const
{
	ERR_READ_THREAD_GUARD_V(RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
	_refresh_texture_repeat_cache();
	return default_canvas_item_texture_repeat_cache;
}

Viewport::VRSMode Viewport::get_vrs_mode() const
{
	ERR_READ_THREAD_GUARD_V(VRS_DISABLED);
	return vrs_mode;
}

void Viewport::set_vrs_update_mode(VRSUpdateMode p_vrs_update_mode)
{
	ERR_MAIN_THREAD_GUARD;

	vrs_update_mode = p_vrs_update_mode;
	switch (p_vrs_update_mode) {
	case VRS_UPDATE_ONCE: {
		Renderer::viewport_set_vrs_update_mode(viewport, RSE::VIEWPORT_VRS_UPDATE_ONCE);
	} break;
	case VRS_UPDATE_ALWAYS: {
		Renderer::viewport_set_vrs_update_mode(viewport, RSE::VIEWPORT_VRS_UPDATE_ALWAYS);
	} break;
	default: {
		Renderer::viewport_set_vrs_update_mode(viewport, RSE::VIEWPORT_VRS_UPDATE_DISABLED);
	} break;
	}
}

Viewport::VRSUpdateMode Viewport::get_vrs_update_mode() const
{
	ERR_READ_THREAD_GUARD_V(VRS_UPDATE_DISABLED);
	return vrs_update_mode;
}

void Viewport::set_vrs_texture(Ref<Texture2D> p_texture)
{
	ERR_MAIN_THREAD_GUARD;
	vrs_texture = p_texture;

	// TODO need to add something here in case the RID changes
	RID tex = p_texture.is_valid() ? p_texture->get_rid() : RID();
	Renderer::viewport_set_vrs_texture(viewport, tex);
}

Ref<Texture2D> Viewport::get_vrs_texture() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Texture2D>());
	return vrs_texture;
}

DisplayServerEnums::WindowID Viewport::get_window_id() const
{
	ERR_READ_THREAD_GUARD_V(DisplayServerEnums::INVALID_WINDOW_ID);
	return DisplayServerEnums::MAIN_WINDOW_ID;
}

Viewport* Viewport::get_parent_viewport() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!is_inside_tree(), nullptr);
	if (!get_parent()) {
		return nullptr; // root viewport
	}

	return get_parent()->get_viewport();
}

void Viewport::set_embedding_subwindows(bool p_embed)
{
	ERR_THREAD_GUARD;
	if (gui.embed_subwindows_hint == p_embed) {
		return;
	}

	bool allow_change = true;

	if (!is_inside_tree()) {
		// Change can happen since no child window is displayed.
	}
	else if (gui.embed_subwindows_hint) {
		if (!gui.sub_windows.is_empty()) {
			// Prevent change when this viewport has embedded windows.
			allow_change = false;
		}
	}
	else {
		Viewport* vp = this;
		while (true) {
			if (!vp->get_parent()) {
				// Root window reached.
				break;
			}
			vp = vp->get_parent()->get_viewport();
			if (vp->is_embedding_subwindows()) {
				for (int i = 0; i < vp->gui.sub_windows.size(); i++) {
					if (is_ancestor_of(vp->gui.sub_windows[i].window)) {
						// Prevent change when this viewport has child windows that are displayed in
						// an ancestor viewport.
						allow_change = false;
						break;
					}
				}
			}
		}

		if (allow_change) {
			Vector<DisplayServerEnums::WindowID> wl =
				DisplayServer::get_window_list();
			for (const DisplayServerEnums::WindowID& window_id : wl) {
				const Window* w = Window::get_from_id(window_id);
				if (w && is_ancestor_of(w)) {
					// Prevent change when this viewport has child windows that are displayed as
					// native windows.
					allow_change = false;
					break;
				}
			}
		}
	}

	if (allow_change) {
		gui.embed_subwindows_hint = p_embed;
	}
	else {
		WARN_PRINT("Can't change \"gui_embed_subwindows\" while a child window is displayed. "
				   "Consider hiding all child windows before changing this value.");
	}
}

bool Viewport::is_embedding_subwindows() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return gui.embed_subwindows_hint;
}

void Viewport::set_drag_threshold(int p_threshold)
{
	ERR_MAIN_THREAD_GUARD
	gui.drag_threshold = p_threshold;
}

int Viewport::get_drag_threshold() const
{
	ERR_READ_THREAD_GUARD_V(10);
	return gui.drag_threshold;
}

void Viewport::subwindow_set_popup_safe_rect(Window* p_window, const Rect2i& p_rect)
{
	int index = _sub_window_find(p_window);
	ERR_FAIL_COND(index == -1);

	gui.sub_windows.write[index].parent_safe_rect = p_rect;
}

Rect2i Viewport::subwindow_get_popup_safe_rect(Window* p_window) const
{
	int index = _sub_window_find(p_window);
	// FIXME: Re-enable ERR_FAIL_COND after rewriting embedded window popup closing.
	// Currently it is expected, that index == -1 can happen.
	if (index == -1) {
		return Rect2i();
	}
	// ERR_FAIL_COND_V(index == -1, Rect2i());

	return gui.sub_windows[index].parent_safe_rect;
}

void Viewport::set_sdf_oversize(SDFOversize p_sdf_oversize)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_sdf_oversize, SDF_OVERSIZE_MAX);
	sdf_oversize = p_sdf_oversize;
	Renderer::viewport_set_sdf_oversize_and_scale(
		viewport, RSE::ViewportSDFOversize(sdf_oversize), RSE::ViewportSDFScale(sdf_scale));
}

Viewport::SDFOversize Viewport::get_sdf_oversize() const
{
	ERR_READ_THREAD_GUARD_V(SDF_OVERSIZE_100_PERCENT);
	return sdf_oversize;
}

void Viewport::set_sdf_scale(SDFScale p_sdf_scale)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_sdf_scale, SDF_SCALE_MAX);
	sdf_scale = p_sdf_scale;
	Renderer::viewport_set_sdf_oversize_and_scale(
		viewport, RSE::ViewportSDFOversize(sdf_oversize), RSE::ViewportSDFScale(sdf_scale));
}

Viewport::SDFScale Viewport::get_sdf_scale() const
{
	ERR_READ_THREAD_GUARD_V(SDF_SCALE_100_PERCENT);
	return sdf_scale;
}

Transform2D Viewport::get_screen_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return get_screen_transform_internal();
}

Transform2D Viewport::get_screen_transform_internal(bool p_absolute_position) const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return get_final_transform();
}

void Viewport::update_mouse_cursor_state()
{
	// Updates need to happen in Window, because SubViewportContainers might be hidden behind other
	// Controls.
	Window* base_window = get_base_window();
	if (base_window) {
		base_window->update_mouse_cursor_state();
	}
}

template <> Camera3D* Viewport::CameraOverride<Camera3D>::get_overridden_camera() const {}

template <> Camera2D* Viewport::CameraOverride<Camera2D>::get_overridden_camera() const {}

template <> void Viewport::CameraOverride<Camera3D>::enable(Viewport*, Camera3D const*) {}

template <> void Viewport::CameraOverride<Camera3D>::disable(Camera3D*) {}

template <> void Viewport::CameraOverride<Camera2D>::set_overridden_camera(Camera2D const*) {}

template <> void Viewport::CameraOverride<Camera2D>::enable(Viewport*, Camera2D const*) {}

template <> void Viewport::CameraOverride<Camera2D>::disable(Camera2D*) {}

void Viewport::set_canvas_cull_mask(uint32_t p_canvas_cull_mask)
{
	ERR_MAIN_THREAD_GUARD;
	canvas_cull_mask = p_canvas_cull_mask;
	Renderer::viewport_set_canvas_cull_mask(viewport, canvas_cull_mask);
}

uint32_t Viewport::get_canvas_cull_mask() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return canvas_cull_mask;
}

void Viewport::set_canvas_cull_mask_bit(uint32_t p_layer, bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_UNSIGNED_INDEX(p_layer, 32);
	if (p_enable) {
		set_canvas_cull_mask(canvas_cull_mask | (1 << p_layer));
	}
	else {
		set_canvas_cull_mask(canvas_cull_mask & (~(1 << p_layer)));
	}
}

bool Viewport::get_canvas_cull_mask_bit(uint32_t p_layer) const
{
	ERR_READ_THREAD_GUARD_V(false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_layer, 32, false);
	return (canvas_cull_mask & (1 << p_layer));
}

void Viewport::_update_audio_listener_2d()
{
	if (AudioServer::is_initialized()) {
		AudioServer::notify_listener_changed();
	}
}

void Viewport::_audio_listener_2d_set(AudioListener2D* p_audio_listener)
{
	if (audio_listener_2d == p_audio_listener) {
		return;
	}
	else if (audio_listener_2d) {
		audio_listener_2d->clear_current();
	}
	audio_listener_2d = p_audio_listener;
}

void Viewport::_audio_listener_2d_remove(AudioListener2D* p_audio_listener)
{
	if (audio_listener_2d == p_audio_listener) {
		audio_listener_2d = nullptr;
	}
}

void Viewport::_camera_2d_set(Camera2D* p_camera_2d)
{
#if DEBUG_ENABLED
	if (is_camera_2d_override_enabled()) {
		camera_2d_override.set_overridden_camera(p_camera_2d);
		return;
	}
#endif // DEBUG_ENABLED

	camera_2d = p_camera_2d;
}

AudioListener2D* Viewport::get_audio_listener_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return audio_listener_2d;
}

void Viewport::set_as_audio_listener_2d(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_enable == is_audio_listener_2d_enabled) {
		return;
	}

	is_audio_listener_2d_enabled = p_enable;
	_update_audio_listener_2d();
}

bool Viewport::is_audio_listener_2d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return is_audio_listener_2d_enabled;
}

Camera2D* Viewport::get_camera_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return camera_2d;
}

#if DEBUG_ENABLED
void Viewport::enable_camera_2d_override(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;

	if (p_enable) {
		camera_2d_override.enable(this, camera_2d);
	}
	else {
		camera_2d_override.disable(camera_2d);
	}
}

bool Viewport::is_camera_2d_override_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return camera_2d_override.is_enabled();
}

Camera2D* Viewport::get_overridden_camera_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_2d_override.is_enabled(), nullptr);
	return camera_2d_override.get_overridden_camera();
}

Camera2D* Viewport::get_override_camera_2d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_2d_override.is_enabled(), nullptr);
	return camera_2d_override.is_enabled() ? get_camera_2d() : nullptr;
}
#endif // DEBUG_ENABLED

#ifndef _3D_DISABLED
AudioListener3D* Viewport::get_audio_listener_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return audio_listener_3d;
}

void Viewport::set_as_audio_listener_3d(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_enable == is_audio_listener_3d_enabled) {
		return;
	}

	is_audio_listener_3d_enabled = p_enable;
	_update_audio_listener_3d();
}

bool Viewport::is_audio_listener_3d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return is_audio_listener_3d_enabled;
}

void Viewport::_update_audio_listener_3d()
{
	if (AudioServer::is_initialized()) {
		AudioServer::notify_listener_changed();
	}
}

void Viewport::_listener_transform_3d_changed_notify() {}

void Viewport::_audio_listener_3d_set(AudioListener3D* p_listener)
{
	if (audio_listener_3d == p_listener) {
		return;
	}

	audio_listener_3d = p_listener;

	_update_audio_listener_3d();
	_listener_transform_3d_changed_notify();
}

bool Viewport::_audio_listener_3d_add(AudioListener3D* p_listener)
{
	audio_listener_3d_set.insert(p_listener);
	return audio_listener_3d_set.size() == 1;
}

void Viewport::_audio_listener_3d_remove(AudioListener3D* p_listener)
{
	audio_listener_3d_set.erase(p_listener);
	if (audio_listener_3d == p_listener) {
		audio_listener_3d = nullptr;
	}
}

void Viewport::_audio_listener_3d_make_next_current(AudioListener3D* p_exclude)
{
	if (audio_listener_3d_set.size() > 0) {
		for (AudioListener3D* E : audio_listener_3d_set) {
			if (p_exclude == E) {
				continue;
			}
			if (!E->is_inside_tree()) {
				continue;
			}
			if (audio_listener_3d != nullptr) {
				return;
			}

			E->make_current();
		}
	}
	else {
		// Attempt to reset listener to the camera position.
		if (camera_3d != nullptr) {
			_update_audio_listener_3d();
			_camera_3d_transform_changed_notify();
		}
	}
}

Camera3D* Viewport::get_camera_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return camera_3d;
}

void Viewport::_camera_3d_transform_changed_notify() {}

bool Viewport::_camera_3d_add(Camera3D* p_camera)
{
	camera_3d_set.insert(p_camera);
	return camera_3d_set.size() == 1;
}

void Viewport::_camera_3d_remove(Camera3D* p_camera)
{
	camera_3d_set.erase(p_camera);
	if (camera_3d == p_camera) {
		_camera_3d_set(nullptr);
	}
}

void Viewport::_camera_3d_make_next_current(Camera3D* p_exclude)
{
	for (Camera3D* E : camera_3d_set) {
		if (p_exclude == E) {
			continue;
		}
		if (!E->is_inside_tree()) {
			continue;
		}
		if (camera_3d != nullptr) {
			return;
		}

		E->make_current();
	}
}

#if DEBUG_ENABLED
void Viewport::enable_camera_3d_override(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;

	if (p_enable) {
		camera_3d_override.enable(this, camera_3d);
	}
	else {
		camera_3d_override.disable(camera_3d);
	}
}

bool Viewport::is_camera_3d_override_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return camera_3d_override.is_enabled();
}

Camera3D* Viewport::get_overridden_camera_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_3d_override.is_enabled(), nullptr);
	return camera_3d_override.get_overridden_camera();
}

Camera3D* Viewport::get_override_camera_3d() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(!camera_3d_override.is_enabled(), nullptr);
	return get_camera_3d();
}
#endif // DEBUG_ENABLED

void Viewport::set_disable_3d(bool p_disable)
{
	ERR_MAIN_THREAD_GUARD;
	disable_3d = p_disable;
	Renderer::viewport_set_disable_3d(viewport, disable_3d);
}

bool Viewport::is_3d_disabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return disable_3d;
}

Ref<World3D> Viewport::get_world_3d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World3D>());
	return world_3d;
}

Ref<World3D> Viewport::find_world_3d() const
{
	ERR_READ_THREAD_GUARD_V(Ref<World3D>());
	if (own_world_3d.is_valid()) {
		return own_world_3d;
	}
	else if (world_3d.is_valid()) {
		return world_3d;
	}
	else if (parent) {
		return parent->find_world_3d();
	}
	else {
		return Ref<World3D>();
	}
}

void Viewport::_own_world_3d_changed()
{
	ERR_FAIL_COND(world_3d.is_null());
	ERR_FAIL_COND(own_world_3d.is_null());

	if (is_inside_tree()) {
		_propagate_exit_world_3d(this);
	}

	if (is_inside_tree()) {
		_propagate_enter_world_3d(this);
	}

	if (is_inside_tree()) {
		Renderer::viewport_set_scenario(viewport, find_world_3d()->get_scenario());
	}

	_update_audio_listener_3d();
}

bool Viewport::is_using_own_world_3d() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return own_world_3d.is_valid();
}

#ifndef XR_DISABLED

bool Viewport::is_using_xr() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return use_xr;
}
#endif // XR_DISABLED

void Viewport::set_scaling_3d_mode(Scaling3DMode p_scaling_3d_mode)
{
	ERR_MAIN_THREAD_GUARD;
	if (scaling_3d_mode == p_scaling_3d_mode) {
		return;
	}

	scaling_3d_mode = p_scaling_3d_mode;
	Renderer::viewport_set_scaling_3d_mode(viewport, (RSE::ViewportScaling3DMode)(int)p_scaling_3d_mode);
}

Viewport::Scaling3DMode Viewport::get_scaling_3d_mode() const
{
	ERR_READ_THREAD_GUARD_V(SCALING_3D_MODE_BILINEAR);
	return scaling_3d_mode;
}

void Viewport::set_scaling_3d_scale(float p_scaling_3d_scale)
{
	ERR_MAIN_THREAD_GUARD;
	// Clamp to reasonable values that are actually useful.
	// Values above 2.0 don't serve a practical purpose since the viewport
	// isn't displayed with mipmaps.
	scaling_3d_scale = CLAMP(p_scaling_3d_scale, 0.1, 2.0);

	Renderer::viewport_set_scaling_3d_scale(viewport, scaling_3d_scale);
}

float Viewport::get_scaling_3d_scale() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return scaling_3d_scale;
}

void Viewport::set_fsr_sharpness(float p_fsr_sharpness)
{
	ERR_MAIN_THREAD_GUARD;
	if (fsr_sharpness == p_fsr_sharpness) {
		return;
	}

	if (p_fsr_sharpness < 0.0f) {
		p_fsr_sharpness = 0.0f;
	}

	fsr_sharpness = p_fsr_sharpness;
	Renderer::viewport_set_fsr_sharpness(viewport, p_fsr_sharpness);
}

float Viewport::get_fsr_sharpness() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return fsr_sharpness;
}

void Viewport::set_texture_mipmap_bias(float p_texture_mipmap_bias)
{
	ERR_MAIN_THREAD_GUARD;
	if (texture_mipmap_bias == p_texture_mipmap_bias) {
		return;
	}

	texture_mipmap_bias = p_texture_mipmap_bias;
	Renderer::viewport_set_texture_mipmap_bias(viewport, p_texture_mipmap_bias);
}

float Viewport::get_texture_mipmap_bias() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return texture_mipmap_bias;
}

void Viewport::set_anisotropic_filtering_level(AnisotropicFiltering p_anisotropic_filtering_level)
{
	ERR_MAIN_THREAD_GUARD;
	if (anisotropic_filtering_level == p_anisotropic_filtering_level) {
		return;
	}

	anisotropic_filtering_level = p_anisotropic_filtering_level;
	Renderer::viewport_set_anisotropic_filtering_level(
		viewport, (RSE::ViewportAnisotropicFiltering)(int)p_anisotropic_filtering_level);
}

Viewport::AnisotropicFiltering Viewport::get_anisotropic_filtering_level() const
{
	ERR_READ_THREAD_GUARD_V(ANISOTROPY_DISABLED);
	return anisotropic_filtering_level;
}

#endif // _3D_DISABLED

Viewport::~Viewport()
{
	// Erase itself from viewport textures.
	for (ViewportTexture* E : viewport_textures) {
		E->vp = nullptr;
	}
	if (world_2d.is_valid()) {
		world_2d->remove_viewport(this);
	}
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(viewport);
}

void SubViewport::set_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	_internal_set_size(p_size, _get_view_count());
}

void SubViewport::set_size_force(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	// Use only for setting the size from the parent SubViewportContainer with enabled stretch mode.
	// Don't expose function to scripting.
	_internal_set_size(p_size, _get_view_count(), true);
}

Size2i SubViewport::get_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return _get_size();
}

void SubViewport::set_view_count(const int p_view_count)
{
	ERR_MAIN_THREAD_GUARD;

	// Q: add `, _get_size_2d_override(), true` ?
	_internal_set_size(_get_size(), p_view_count);
}

int SubViewport::get_view_count() const
{
	ERR_READ_THREAD_GUARD_V(1);

	return _get_view_count();
}

void SubViewport::set_size_2d_override(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	_set_size(_get_size(), _get_view_count(), p_size, true);
}

Size2i SubViewport::get_size_2d_override() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	// Rounding will cause offset issues with the
	// exact positioning of subwindows, but changing the
	// type of size_2d_override would break compatibility.
	return Size2i((_get_size_2d_override() + Size2(0.5, 0.5)).floor());
}

void SubViewport::set_size_2d_override_stretch(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	if (p_enable == size_2d_override_stretch) {
		return;
	}

	size_2d_override_stretch = p_enable;
	_set_size(_get_size(), _get_view_count(), _get_size_2d_override(), true);
}

bool SubViewport::is_size_2d_override_stretch_enabled() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return size_2d_override_stretch;
}

void SubViewport::set_update_mode(UpdateMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	update_mode = p_mode;
	Renderer::viewport_set_update_mode(get_viewport_rid(), RSE::ViewportUpdateMode(p_mode));
}

SubViewport::UpdateMode SubViewport::get_update_mode() const
{
	ERR_READ_THREAD_GUARD_V(UPDATE_DISABLED);
	return update_mode;
}

void SubViewport::set_clear_mode(ClearMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	clear_mode = p_mode;
	Renderer::viewport_set_clear_mode(get_viewport_rid(), RSE::ViewportClearMode(p_mode));
}

SubViewport::ClearMode SubViewport::get_clear_mode() const
{
	ERR_READ_THREAD_GUARD_V(CLEAR_MODE_ALWAYS);
	return clear_mode;
}

DisplayServerEnums::WindowID SubViewport::get_window_id() const
{
	ERR_READ_THREAD_GUARD_V(DisplayServerEnums::INVALID_WINDOW_ID);
	return DisplayServerEnums::INVALID_WINDOW_ID;
}

SubViewport::SubViewport()
{
	Renderer::viewport_set_size(
		get_viewport_rid(), get_size().width, get_size().height, get_view_count());
}

#if DEBUG_ENABLED
template <class T> bool Viewport::CameraOverride<T>::is_enabled() const { return enabled; }

// Explicit template instantiation to allow template definitions inside cpp file
// and prevent instantiation using other than the desired camera types.
template class Viewport::CameraOverride<Camera2D>;
#ifndef _3D_DISABLED
template class Viewport::CameraOverride<Camera3D>;
#endif // _3D_DISABLED
#endif // DEBUG_ENABLED

void Viewport::_sub_window_grab_focus(Window* p_window) {}

bool Viewport::_set_size(
	const Size2i& p_size, const int p_view_count, const Size2& p_size_2d_override, bool p_allocated)
{
	size = p_size;
	Renderer::viewport_set_size(viewport, p_size.width, p_size.height, p_view_count);
	return true;
}

void Viewport::set_input_as_handled() {}

bool Viewport::is_input_handled() const { return false; }

void Viewport::_camera_3d_set(Camera3D* p_camera) {}

bool Viewport::is_visible_subviewport() const { return false; }

void Viewport::gui_perform_drop_at(Vector2 const&, Control*) {}

Window* Viewport::get_base_window() {}

void SubViewport::_internal_set_size(Vector2i const&, int, bool) {}

Viewport::Viewport()
{
	viewport = Renderer::viewport_create();
	world_2d.instantiate();
	world_2d->register_viewport(this);
	current_canvas = world_2d->get_canvas();
	Renderer::viewport_attach_canvas(viewport, current_canvas);
}

void Viewport::_push_text_input(String const&, bool) {}

void Viewport::_gui_update_mouse_over() {}

void Viewport::_gui_show_tooltip_at(Vector2i const&) {}

void Viewport::gui_release_focus() {}

void Viewport::_drop_mouse_focus() {}

void Viewport::_update_texture_repeat_changed(bool) {}

void Viewport::_update_texture_filter_changed(bool) {}

void Viewport::_refresh_texture_repeat_cache() const {}

void Viewport::_refresh_texture_filter_cache() const {}

void Viewport::_push_unhandled_input_internal(const InputEvent&) {}

void Viewport::push_input(const InputEvent&, bool) {}

void Viewport::_propagate_world_2d_changed(Node*) {}

void Viewport::_propagate_exit_world_3d(Node*) {}

void Viewport::_propagate_enter_world_3d(Node*) {}

void Viewport::_mouse_leave_viewport() {}

void Viewport::_update_mouse_over(Vector2 p_pos) {}

InputEvent Viewport::_make_input_local(const InputEvent& p_ev)
{
	return p_ev;
}

void Viewport::_gui_sort_roots() {}

void Viewport::_gui_set_drag_preview(Control*, Control*) {}

void Viewport::_gui_grab_click_focus(Control*) {}

Control* Viewport::_gui_find_control_at_pos(CanvasItem*, Vector2 const&, Transform2D const&)
{
	Control c = Control();
	return &c;
}

void Viewport::_gui_control_grab_focus(Control*, bool) {}

void ViewportTexture::setup_local_to_scene() {}

Transform2D SubViewport::get_screen_transform_internal(bool p_include_canvas_transform) const
{
	return Transform2D();
}

Transform2D SubViewport::get_popup_base_transform() const { return Transform2D(); }

Viewport* SubViewport::get_section_root_viewport() const { return nullptr; }

bool SubViewport::is_attached_in_viewport() const { return false; }

// --- viewport.compat.inc ---

#ifndef DISABLE_DEPRECATED

#include "viewport.h"

#ifndef XR_DISABLED
bool Viewport::_is_using_xr_115799() { return is_using_xr(); }
#endif

void Viewport::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED



// ============================================================================
// Merged from scene/main/window.cpp
// ============================================================================

#include "window.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/os.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/theme.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"

// Editor integration.

int Window::root_layout_direction = 0;

void Window::set_root_layout_direction(int p_root_dir) { root_layout_direction = p_root_dir; }

// Dynamic properties.

Window* Window::focused_window = nullptr;

void Window::set_title(const String& p_title) {}

String Window::get_title() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return title;
}

String Window::get_displayed_title() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return displayed_title;
}

Window::WindowInitialPosition Window::get_initial_position() const
{
	ERR_READ_THREAD_GUARD_V(WINDOW_INITIAL_POSITION_ABSOLUTE);
	return initial_position;
}

void Window::set_current_screen(int p_screen)
{
	ERR_MAIN_THREAD_GUARD;

	current_screen = p_screen;
	if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}
	DisplayServer::window_set_current_screen(p_screen, window_id);
}

int Window::get_current_screen() const
{
	ERR_READ_THREAD_GUARD_V(0);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		current_screen = DisplayServer::window_get_current_screen(window_id);
	}
	return current_screen;
}

void Window::set_position(const Point2i& p_position)
{
	ERR_MAIN_THREAD_GUARD;

	position = p_position;

	if (embedder) {
		embedder->_sub_window_update(this);

	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_position(p_position, window_id);
	}
}

Point2i Window::get_position() const
{
	ERR_READ_THREAD_GUARD_V(Point2i());

	return position;
}

void Window::move_to_center()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());

	Rect2 parent_rect;

	if (is_embedded()) {
		parent_rect = get_embedder()->get_visible_rect();
	}
	else {
		int parent_screen =
			DisplayServer::window_get_current_screen(get_window_id());
		parent_rect.position = DisplayServer::screen_get_position(parent_screen);
		parent_rect.size = DisplayServer::screen_get_size(parent_screen);
	}

	if (parent_rect != Rect2()) {
		set_position(parent_rect.position + (parent_rect.size - get_size()) / 2);
	}
}

void Window::set_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif

	size = p_size;
	_update_window_size();
	_settings_changed();
}

Size2i Window::get_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return size;
}

void Window::reset_size()
{
	ERR_MAIN_THREAD_GUARD;
	set_size(Size2i());
}

Point2i Window::get_position_with_decorations() const
{
	ERR_READ_THREAD_GUARD_V(Point2i());
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_get_position_with_decorations(window_id);
	}
	if (visible && is_embedded() && !get_flag(Window::FLAG_BORDERLESS)) {
		Size2 border_offset;
		if (theme_cache.embedded_border.is_valid()) {
			border_offset = theme_cache.embedded_border->get_offset();
		}
		if (theme_cache.embedded_unfocused_border.is_valid()) {
			border_offset = border_offset.max(theme_cache.embedded_unfocused_border->get_offset());
		}
		return position - border_offset;
	}
	return position;
}

Size2i Window::get_size_with_decorations() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_get_size_with_decorations(window_id);
	}
	if (visible && is_embedded() && !get_flag(Window::FLAG_BORDERLESS)) {
		Size2 border_size;
		if (theme_cache.embedded_border.is_valid()) {
			border_size = theme_cache.embedded_border->get_minimum_size();
		}
		if (theme_cache.embedded_unfocused_border.is_valid()) {
			border_size =
				border_size.max(theme_cache.embedded_unfocused_border->get_minimum_size());
		}
		return size + border_size;
	}
	return size;
}

Size2i Window::_clamp_limit_size(const Size2i& p_limit_size)
{
	// Force window limits to respect size limitations of rendering server.
	Size2i max_window_size = Renderer::get_maximum_viewport_size();
	if (max_window_size != Size2i()) {
		return p_limit_size.clamp(Vector2i(), max_window_size);
	}
	else {
		return p_limit_size.maxi(0);
	}
}

void Window::_validate_limit_size()
{
	// When max_size is invalid, max_size_used falls back to respect size limitations of rendering
	// server.
	bool max_size_valid =
		(max_size.x > 0 || max_size.y > 0) && max_size.x >= min_size.x && max_size.y >= min_size.y;
	max_size_used = max_size_valid ? max_size : Renderer::get_maximum_viewport_size();
}

void Window::set_max_size(const Size2i& p_max_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif
	Size2i max_size_clamped = _clamp_limit_size(p_max_size);
	if (max_size == max_size_clamped) {
		return;
	}
	max_size = max_size_clamped;

	_validate_limit_size();
	_update_window_size();
}

Size2i Window::get_max_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return max_size;
}

void Window::set_min_size(const Size2i& p_min_size)
{
	ERR_MAIN_THREAD_GUARD;
#if defined(ANDROID_ENABLED)
	if (!get_parent() && is_inside_tree()) {
		// Can't set root window size on Android.
		return;
	}
#endif
	Size2i min_size_clamped = _clamp_limit_size(p_min_size);
	if (min_size == min_size_clamped) {
		return;
	}
	min_size = min_size_clamped;

	_validate_limit_size();
	_update_window_size();
}

Size2i Window::get_min_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return min_size;
}

void Window::set_mode(Mode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	mode = p_mode;

	if (embedder) {
		embedder->_sub_window_update(this);

	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_mode(
			DisplayServerEnums::WindowMode(p_mode), window_id);
	}
}

Window::Mode Window::get_mode() const
{
	ERR_READ_THREAD_GUARD_V(MODE_WINDOWED);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		mode = (Mode)DisplayServer::window_get_mode(window_id);
	}
	return mode;
}

void Window::set_fullscreen_shortcut_enabled(bool p_enabled)
{
	fullscreen_shortcut_enabled = p_enabled;
}

bool Window::is_fullscreen_shortcut_enabled() const { return fullscreen_shortcut_enabled; }

void Window::set_flag(Flags p_flag, bool p_enabled)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_flag, FLAG_MAX);
	flags[p_flag] = p_enabled;

	if (p_flag == FLAG_TRANSPARENT) {
		set_transparent_background(p_enabled);
	}

	if (embedder) {
		embedder->_sub_window_update(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (!is_in_edited_scene_root()) {
			DisplayServer::window_set_flag(
				DisplayServerEnums::WindowFlags(p_flag), p_enabled, window_id);
		}
	}
}

bool Window::get_flag(Flags p_flag) const
{
	ERR_READ_THREAD_GUARD_V(false);
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (!is_in_edited_scene_root()) {
			flags[p_flag] = DisplayServer::window_get_flag(
				DisplayServerEnums::WindowFlags(p_flag), window_id);
		}
	}
	return flags[p_flag];
}

bool Window::is_popup() const
{
	return (get_flag(Window::FLAG_POPUP) && get_flag(Window::FLAG_NO_FOCUS)) ||
		   get_flag(Window::FLAG_MOUSE_PASSTHROUGH);
}

void Window::set_hdr_output_requested(bool p_requested)
{
	ERR_MAIN_THREAD_GUARD;

	hdr_output_requested = p_requested;

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_request_hdr_output(hdr_output_requested, window_id);
	}

	_update_viewport_for_hdr_output();
}

bool Window::is_hdr_output_requested() const
{
	ERR_READ_THREAD_GUARD_V(false);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		hdr_output_requested =
			DisplayServer::window_is_hdr_output_requested(window_id);
	}

	return hdr_output_requested;
}

float Window::get_output_max_linear_value() const
{
	ERR_READ_THREAD_GUARD_V(1.0f);

	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_get_output_max_linear_value(window_id);
	}

	return 1.0f;
}

bool Window::is_maximize_allowed() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_is_maximize_allowed(window_id);
	}
	return true;
}

void Window::request_attention()
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_request_attention(window_id);
	}
}

void Window::set_taskbar_progress_value(float p_value)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_taskbar_progress_value(p_value, window_id);
	}
}

void Window::set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_taskbar_progress_state(p_state);
	}
}

#ifndef DISABLE_DEPRECATED
void Window::move_to_foreground()
{
	WARN_DEPRECATED_MSG(
		R"*(The "move_to_foreground()" method is deprecated, use "grab_focus()" instead.)*");
	grab_focus();
}
#endif // DISABLE_DEPRECATED

bool Window::can_draw() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (!is_inside_tree()) {
		return false;
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_can_draw(window_id);
	}

	return visible;
}

void Window::set_ime_active(bool p_active)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_ime_active(p_active, window_id);
	}
}

void Window::set_ime_position(const Point2i& p_pos)
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_ime_position(p_pos, window_id);
	}
}

bool Window::is_embedded() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return get_embedder() != nullptr;
}

bool Window::is_in_edited_scene_root() const
{
	ERR_READ_THREAD_GUARD_V(false);
#ifdef TOOLS_ENABLED
	return is_part_of_edited_scene();
#else
	return false;
#endif
}

void Window::_update_from_window()
{
	ERR_FAIL_COND(window_id == DisplayServerEnums::INVALID_WINDOW_ID);
	mode = (Mode)DisplayServer::window_get_mode(window_id);
	for (int i = 0; i < FLAG_MAX; i++) {
		flags[i] = DisplayServer::window_get_flag(
			DisplayServerEnums::WindowFlags(i), window_id);
	}
}

void Window::_clear_window()
{
	ERR_FAIL_COND(window_id == DisplayServerEnums::INVALID_WINDOW_ID);

	bool had_focus = has_focus();

	if (DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		float win_scale = DisplayServer::window_get_scale(window_id);

		Size2i adjusted_size = Size2i(size.width / win_scale, size.height / win_scale);
		Size2i adjusted_pos = Size2i(position.x / win_scale, position.y / win_scale);

		_rect_changed_callback(Rect2i(adjusted_pos, adjusted_size));
	}

	if (transient_parent && transient_parent->window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_set_transient(
			window_id, DisplayServerEnums::INVALID_WINDOW_ID);
	}

	for (const Window* E : transient_children) {
		if (E->window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
			DisplayServer::window_set_transient(
				E->window_id, DisplayServerEnums::INVALID_WINDOW_ID);
		}
	}

	_update_from_window();

	DisplayServer::delete_sub_window(window_id);
	window_id = DisplayServerEnums::INVALID_WINDOW_ID;

	// If closing window was focused and has a parent, return focus.
	if (had_focus && transient_parent) {
		transient_parent->grab_focus();
	}

	_update_viewport_size();
	Renderer::viewport_set_update_mode(
		get_viewport_rid(), RSE::VIEWPORT_UPDATE_DISABLED);

	if (transient && transient_to_focused) {
		_clear_transient();
	}
}

void Window::_rect_changed_callback(const Rect2i& p_callback)
{
	// we must always accept this as the truth
	if (size == p_callback.size && position == p_callback.position) {
		return;
	}

	if (position != p_callback.position) {
		position = p_callback.position;
		_propagate_window_notification(this, NOTIFICATION_WM_POSITION_CHANGED);
	}

	if (size != p_callback.size) {
		size = p_callback.size;
		_update_viewport_size();
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID &&
		!DisplayServer::has_feature(
			DisplayServerEnums::FEATURE_SELF_FITTING_WINDOWS)) {
		Vector2 sz_out =
			DisplayServer::window_get_size_with_decorations(window_id);
		Vector2 pos_out =
			DisplayServer::window_get_position_with_decorations(window_id);
		Vector2 sz_in = DisplayServer::window_get_size(window_id);
		Vector2 pos_in = DisplayServer::window_get_position(window_id);
		AccessibilityServer::set_window_rect(
			window_id, Rect2(pos_out, sz_out), Rect2(pos_in, sz_in));
	}
	queue_accessibility_update();
}

void Window::update_mouse_cursor_state()
{
	ERR_MAIN_THREAD_GUARD;
	// Update states based on mouse cursor position.
	// This includes updated mouse_enter or mouse_exit signals or the current mouse cursor shape.
	// These details are set in Viewport::_gui_input_event. To instantly
	// see the changes in the viewport, we need to trigger a mouse motion event.
	// This function should be called whenever scene tree changes affect the mouse cursor.
	InputEvent mm = InputEvent::make_mouse_motion();
	Vector2 pos = get_mouse_position();
	Transform2D xform = get_global_canvas_transform().affine_inverse();
	mm.mouse_motion.position = pos;
	mm.mouse_motion.global_position = xform.xform(pos);
	mm.device = InputEvent::DEVICE_ID_INTERNAL;
	push_input(mm, true);
}

void Window::show()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(true);
}

void Window::hide()
{
	ERR_MAIN_THREAD_GUARD;
	set_visible(false);
}

void Window::_accessibility_activate()
{
	_accessibility_notify_enter(this);
	if (!get_embedder()) {
		AccessibilityServer::window_activation_completed(get_window_id());
	}
}

void Window::_accessibility_deactivate()
{
	_accessibility_notify_exit(this);
	if (!get_embedder()) {
		AccessibilityServer::window_deactivation_completed(get_window_id());
	}
}

void Window::_clear_transient()
{
	if (transient_parent) {
		if (transient_parent->window_id != DisplayServerEnums::INVALID_WINDOW_ID &&
			window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
			DisplayServer::window_set_transient(
				window_id, DisplayServerEnums::INVALID_WINDOW_ID);
		}
		transient_parent->transient_children.erase(this);
		if (transient_parent->exclusive_child == this) {
			transient_parent->exclusive_child = nullptr;
		}
		transient_parent = nullptr;
	}
}

void Window::_set_transient_exclusive_child(bool p_clear_invalid)
{
	if (exclusive && visible && is_inside_tree()) {
		if (!is_in_edited_scene_root()) {
			// Transient parent has another exclusive child.
			if (transient_parent->exclusive_child && transient_parent->exclusive_child != this) {
				ERR_PRINT(vformat("Attempting to make child window exclusive, but the parent "
								  "window already has another exclusive child. This window, "
								  "parent window, and current exclusive child window"));
			}
			transient_parent->exclusive_child = this;
		}
	}
	else if (p_clear_invalid) {
		if (transient_parent->exclusive_child == this) {
			transient_parent->exclusive_child = nullptr;
		}
	}
}

void Window::set_transient(bool p_transient)
{
	ERR_MAIN_THREAD_GUARD;
	if (transient == p_transient) {
		return;
	}

	transient = p_transient;

	if (!is_inside_tree()) {
		return;
	}

	if (transient) {
		if (!transient_to_focused) {
			_make_transient();
		}
	}
	else {
		_clear_transient();
	}
}

bool Window::is_transient() const { return transient; }

void Window::set_transient_to_focused(bool p_transient_to_focused)
{
	ERR_MAIN_THREAD_GUARD;
	if (transient_to_focused == p_transient_to_focused) {
		return;
	}

	transient_to_focused = p_transient_to_focused;
}

bool Window::is_transient_to_focused() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return transient_to_focused;
}

void Window::set_exclusive(bool p_exclusive)
{
	ERR_MAIN_THREAD_GUARD;
	if (exclusive == p_exclusive) {
		return;
	}

	exclusive = p_exclusive;

	if (!embedder && window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		if (is_in_edited_scene_root()) {
			DisplayServer::window_set_exclusive(window_id, false);
		}
		else {
			DisplayServer::window_set_exclusive(window_id, exclusive);
		}
	}

	if (transient_parent) {
		_set_transient_exclusive_child(true);
	}
}

bool Window::is_exclusive() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return exclusive;
}

bool Window::is_visible() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return visible;
}

Size2i Window::_clamp_window_size(const Size2i& p_size)
{
	Size2i window_size_clamped = p_size;
	Size2 minsize = get_clamped_minimum_size();
	window_size_clamped = window_size_clamped.max(minsize);

	if (max_size_used != Size2i()) {
		window_size_clamped = window_size_clamped.min(max_size_used);
	}

	return window_size_clamped;
}

void Window::_update_window_size()
{
	Size2i size_limit = get_clamped_minimum_size();
	if (!embedder && window_id != DisplayServerEnums::INVALID_WINDOW_ID && keep_title_visible) {
		Size2i title_size =
			DisplayServer::window_get_title_size(displayed_title, window_id);
		size_limit = size_limit.max(title_size);
	}

	size = size.max(size_limit);

	bool reset_min_first = false;

	if (max_size_used != Size2i()) {
		// Force window size to respect size limitations of max_size_used.
		size = size.min(max_size_used);

		if (size_limit.x > max_size_used.x) {
			size_limit.x = max_size_used.x;
			reset_min_first = true;
		}
		if (size_limit.y > max_size_used.y) {
			size_limit.y = max_size_used.y;
			reset_min_first = true;
		}
	}

	if (embedder) {
		size = size.maxi(1);

		embedder->_sub_window_update(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		// When main window embedded in the editor, we can't resize the main window.
		if (window_id != DisplayServerEnums::MAIN_WINDOW_ID ||
			!Engine::is_embedded_in_editor()) {
			if (reset_min_first && wrap_controls) {
				// Avoid an error if setting max_size to a value between min_size and the previous
				// size_limit.
				DisplayServer::window_set_min_size(Size2i(), window_id);
			}

			DisplayServer::window_set_max_size(max_size_used, window_id);
			DisplayServer::window_set_min_size(size_limit, window_id);
			DisplayServer::window_set_size(size, window_id);
		}
		else if (Engine::is_embedded_in_editor()) {
			size = DisplayServer::window_get_size(window_id); // Reset size.
		}
	}

	// update the viewport
	_update_viewport_size();
}

void Window::set_force_native(bool p_force_native)
{
	if (force_native == p_force_native) {
		return;
	}
	if (is_visible() && !is_in_edited_scene_root()) {
		ERR_FAIL_MSG("Can't change \"force_native\" while a window is displayed. Consider hiding "
					 "window before changing this value.");
	}
	if (window_id == DisplayServerEnums::MAIN_WINDOW_ID) {
		return;
	}
	force_native = p_force_native;
	if (!is_in_edited_scene_root() && is_inside_tree() &&
		SceneTree::get_root()->is_embedding_subwindows()) {
		set_embedding_subwindows(force_native);
	}
}

bool Window::get_force_native() const { return force_native; }

Viewport* Window::get_embedder() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	if (force_native &&
		DisplayServer::has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS) &&
		!is_in_edited_scene_root()) {
		return nullptr;
	}

	Viewport* vp = get_parent_viewport();

	while (vp) {
		if (vp->is_embedding_subwindows()) {
			return vp;
		}

		if (vp->get_parent()) {
			vp = vp->get_parent()->get_viewport();
		}
		else {
			vp = nullptr;
		}
	}
	return nullptr;
}

RID Window::get_accessibility_element() const
{
	if (!visible || is_part_of_edited_scene()) {
		return RID();
	}
	if (get_embedder() || is_popup()) {
		return Node::get_accessibility_element();
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return AccessibilityServer::get_window_root(window_id);
	}
	else {
		return RID();
	}
}

RID Window::get_focused_accessibility_element() const
{
	if (window_id == DisplayServerEnums::MAIN_WINDOW_ID) {
		if (get_child_count() > 0) {
			return get_child(0)->get_focused_accessibility_element(); // Try scene tree root node.
		}
	}
	return Node::get_focused_accessibility_element();
}

String Window::_get_accessibility_name() const
{
	if (accessibility_name.is_empty()) {
		return displayed_title;
	}
	else {
		return accessibility_name;
	}
}

Transform2D Window::get_accessibility_transform() const
{
	if (is_inside_tree() && get_parent()) {
		Transform2D parent_tr = get_parent()->get_accessibility_transform();
		Transform2D window_tr;
		if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
			window_tr.set_origin(position);
		}
		else {
			Window* np = get_non_popup_window();
			if (np) {
				window_tr.set_origin(get_position() - np->get_position());
			}
		}
		window_tr.set_scale(Vector2(1.f, 1.f) * get_content_scale_factor());
		return parent_tr.affine_inverse() * window_tr;
	}
	else {
		return Transform2D();
	}
}

void Window::set_content_scale_size(const Size2i& p_size)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(p_size.x < 0);
	ERR_FAIL_COND(p_size.y < 0);
	content_scale_size = p_size;
	_update_viewport_size();
}

Size2i Window::get_content_scale_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2i());
	return content_scale_size;
}

void Window::set_content_scale_mode(ContentScaleMode p_mode)
{
	ERR_MAIN_THREAD_GUARD;
	content_scale_mode = p_mode;
	_update_viewport_size();
}

Window::ContentScaleMode Window::get_content_scale_mode() const
{
	ERR_READ_THREAD_GUARD_V(CONTENT_SCALE_MODE_DISABLED);
	return content_scale_mode;
}

void Window::set_content_scale_aspect(ContentScaleAspect p_aspect)
{
	ERR_MAIN_THREAD_GUARD;

	content_scale_aspect = p_aspect;
	_update_viewport_size();
}

Window::ContentScaleAspect Window::get_content_scale_aspect() const
{
	ERR_READ_THREAD_GUARD_V(CONTENT_SCALE_ASPECT_IGNORE);
	return content_scale_aspect;
}

void Window::set_content_scale_stretch(ContentScaleStretch p_stretch)
{
	content_scale_stretch = p_stretch;
	_update_viewport_size();
}

Window::ContentScaleStretch Window::get_content_scale_stretch() const
{
	return content_scale_stretch;
}

void Window::set_keep_title_visible(bool p_title_visible)
{
	if (keep_title_visible == p_title_visible) {
		return;
	}
	keep_title_visible = p_title_visible;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		_update_window_size();
	}
}

bool Window::get_keep_title_visible() const { return keep_title_visible; }

void Window::set_content_scale_factor(real_t p_factor)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(p_factor <= 0);
	content_scale_factor = p_factor;
	_update_viewport_size();
}

real_t Window::get_content_scale_factor() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return content_scale_factor;
}

void Window::set_nonclient_area(const Rect2i& p_rect)
{
	ERR_MAIN_THREAD_GUARD;
	nonclient_area = p_rect;
}

Rect2i Window::get_nonclient_area() const { return nonclient_area; }

DisplayServerEnums::WindowID Window::get_window_id() const
{
	ERR_READ_THREAD_GUARD_V(DisplayServerEnums::INVALID_WINDOW_ID);
	if (get_embedder()) {
#ifdef TOOLS_ENABLED
		if (is_part_of_edited_scene()) {
			return DisplayServerEnums::MAIN_WINDOW_ID;
		}
#endif
		return parent->get_window_id();
	}
	return window_id;
}

void Window::set_mouse_passthrough_polygon(const Vector<Vector2>& p_region)
{
	ERR_MAIN_THREAD_GUARD;
	mpath = p_region;
	if (window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}
	DisplayServer::window_set_mouse_passthrough(mpath, window_id);
}

Vector<Vector2> Window::get_mouse_passthrough_polygon() const { return mpath; }

void Window::set_wrap_controls(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	wrap_controls = p_enable;

	if (!is_inside_tree()) {
		return;
	}

	if (updating_child_controls) {
		_update_child_controls();
	}
	else {
		_update_window_size();
	}
}

void Window::child_controls_changed() {}

bool Window::is_wrapping_controls() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return wrap_controls;
}

void Window::_update_viewport_for_hdr_output()
{
	// If HDR output is enabled, we need to enable HDR 2D rendering as well.
	// This is required to get the correct dynamic range for the final output.
	// We only need to do this if the viewport is not already set up for HDR 2D rendering.

	if (!is_using_hdr_2d()) {
		Renderer::viewport_set_use_hdr_2d(viewport, hdr_output_requested);
	}
}

void Window::_update_child_controls()
{
	if (!updating_child_controls) {
		return;
	}

	_update_window_size();

	updating_child_controls = false;
}

bool Window::_can_consume_input_events() const { return exclusive_child == nullptr; }

void Window::_window_input_text(const String& p_text, bool p_emit_signal)
{
	_push_text_input(p_text, p_emit_signal);
}

Viewport* Window::get_parent_viewport() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	if (get_parent()) {
		return get_parent()->get_viewport();
	}
	else {
		return nullptr;
	}
}

Window* Window::get_non_popup_window() const
{
	Window* w = const_cast<Window*>(this);
	while (w && w->is_popup()) {
		w = w->get_parent_visible_window();
	}
	return w;
}

void Window::popup(const Rect2i& p_screen_rect) {}

void Window::popup_on_parent(const Rect2i& p_parent_rect)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!is_inside_tree());
	ERR_FAIL_COND_MSG(
		window_id == DisplayServerEnums::MAIN_WINDOW_ID, "Can't popup the main window.");

	if (!is_embedded()) {
		Window* window = get_parent_visible_window();

		if (!window) {
			popup(p_parent_rect);
		}
		else {
			popup(Rect2i(window->get_position() + p_parent_rect.position, p_parent_rect.size));
		}
	}
	else {
		popup(p_parent_rect);
	}
}

bool Window::_try_parent_dialog(Node* p_from_node)
{
	ERR_FAIL_NULL_V(p_from_node, false);
	ERR_FAIL_COND_V_MSG(is_inside_tree(), false,
		"Attempting to parent and popup a dialog that already has a parent.");

	Window* w = p_from_node->get_last_exclusive_window();
	if (w && w != this) {
		w->add_child(this);
		return true;
	}
	return false;
}

void Window::popup_centered(const Size2i& p_minsize) {}

void Window::popup_centered_ratio(float p_ratio) {}

void Window::popup_centered_clamped(const Size2i& p_size, float p_fallback_ratio) {}

void Window::popup_exclusive(Node* p_from_node, const Rect2i& p_screen_rect)
{
	if (_try_parent_dialog(p_from_node)) {
		popup(p_screen_rect);
	}
}

void Window::popup_exclusive_on_parent(Node* p_from_node, const Rect2i& p_parent_rect)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_on_parent(p_parent_rect);
	}
}

void Window::popup_exclusive_centered_ratio(Node* p_from_node, float p_ratio)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_centered_ratio(p_ratio);
	}
}

void Window::popup_exclusive_centered_clamped(
	Node* p_from_node, const Size2i& p_size, float p_fallback_ratio)
{
	if (_try_parent_dialog(p_from_node)) {
		popup_centered_clamped(p_size, p_fallback_ratio);
	}
}

Rect2i Window::fit_rect_in_parent(Rect2i p_rect, const Rect2i& p_parent_rect) const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	Size2i limit = p_parent_rect.size;
	if (p_rect.position.x + p_rect.size.x > limit.x) {
		p_rect.position.x = limit.x - p_rect.size.x;
	}
	if (p_rect.position.y + p_rect.size.y > limit.y) {
		p_rect.position.y = limit.y - p_rect.size.y;
	}

	if (p_rect.position.x < 0) {
		p_rect.position.x = 0;
	}

	int title_height = get_flag(Window::FLAG_BORDERLESS) ? 0 : theme_cache.title_height;

	if (p_rect.position.y < title_height) {
		p_rect.position.y = title_height;
	}

	return p_rect;
}

Size2 Window::get_contents_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	return _get_contents_minimum_size();
}

Size2 Window::get_clamped_minimum_size() const
{
	ERR_READ_THREAD_GUARD_V(Size2());
	if (!wrap_controls) {
		return min_size;
	}

	return min_size.max(get_contents_minimum_size() * get_content_scale_factor());
}

void Window::grab_focus()
{
	ERR_MAIN_THREAD_GUARD;
	if (embedder) {
		embedder->_sub_window_grab_focus(this);
	}
	else if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_move_to_foreground(window_id);
	}
}

bool Window::has_focus() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_is_focused(window_id);
	}
	return focused;
}

bool Window::has_focus_or_active_popup() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		return DisplayServer::window_is_focused(window_id) ||
			   (DisplayServer::window_get_active_popup() == window_id);
	}
	return focused;
}

void Window::start_drag()
{
	ERR_MAIN_THREAD_GUARD;
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_start_drag(window_id);
	}
	else if (embedder) {
		embedder->_window_start_drag(this);
	}
}

void Window::start_resize(DisplayServerEnums::WindowResizeEdge p_edge)
{
	ERR_MAIN_THREAD_GUARD;
	if (get_flag(FLAG_RESIZE_DISABLED)) {
		return;
	}
	if (window_id != DisplayServerEnums::INVALID_WINDOW_ID) {
		DisplayServer::window_start_resize(p_edge, window_id);
	}
	else if (embedder) {
		switch (p_edge) {
		case DisplayServerEnums::WINDOW_EDGE_TOP_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_TOP_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_TOP_RIGHT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_RIGHT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_LEFT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM_LEFT, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM, this);
		} break;
		case DisplayServerEnums::WINDOW_EDGE_BOTTOM_RIGHT: {
			embedder->_window_start_resize(Viewport::SUB_WINDOW_RESIZE_BOTTOM_RIGHT, this);
		} break;
		default:
			break;
		}
	}
}

Rect2i Window::get_usable_parent_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2());
	Rect2i parent_rect;
	if (is_embedded()) {
		parent_rect = get_embedder()->get_visible_rect();
	}
	else {
		const Window* w = is_visible() ? this : get_parent_visible_window();
		// find a parent that can contain us
		ERR_FAIL_NULL_V(w, Rect2());

		parent_rect = DisplayServer::screen_get_usable_rect(
			DisplayServer::window_get_current_screen(w->get_window_id()));
	}
	return parent_rect;
}

void Window::set_accessibility_description(const String& p_description)
{
	ERR_MAIN_THREAD_GUARD;
	if (accessibility_description != p_description) {
		accessibility_description = p_description;
		queue_accessibility_update();
	}
}

void Window::accessibility_announcement(const String& p_announcement)
{
	ERR_MAIN_THREAD_GUARD;
	announcement = p_announcement;
	queue_accessibility_update();
}

void Window::add_child_notify(Node* p_child)
{
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

void Window::remove_child_notify(Node* p_child)
{
	if (is_inside_tree() && wrap_controls) {
		child_controls_changed();
	}
}

// Theming.

void Window::set_theme_owner_node(Node* p_node)
{
	ERR_MAIN_THREAD_GUARD;
	theme_owner->set_owner_node(p_node);
}

Node* Window::get_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(nullptr);
	return theme_owner->get_owner_node();
}

bool Window::has_theme_owner_node() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return theme_owner->has_owner_node();
}

void Window::set_theme_context(ThemeContext* p_context, bool p_propagate)
{
	ERR_MAIN_THREAD_GUARD;
	theme_owner->set_owner_context(p_context, p_propagate);
}

Ref<Theme> Window::get_theme() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Theme>());
	return theme;
}

void Window::_theme_changed()
{
	if (is_inside_tree()) {
		theme_owner->propagate_theme_changed(this, this, true, false);
	}
}

void Window::_invalidate_theme_cache()
{
	theme_icon_cache.clear();
	theme_style_cache.clear();
	theme_font_cache.clear();
	theme_font_size_cache.clear();
	theme_color_cache.clear();
	theme_constant_cache.clear();
}

void Window::_update_embedded_window()
{
	if (!updating_embedded_window) {
		return;
	}

	if (embedder) {
		embedder->_sub_window_update(this);
	};

	updating_embedded_window = false;
}

String Window::get_theme_type_variation() const
{
	ERR_READ_THREAD_GUARD_V(String());
	return theme_type_variation;
}

/// Theme property lookup.

#ifdef TOOLS_ENABLED
Ref<Texture2D> Window::get_editor_theme_icon(const String& p_name) const
{
	return get_theme_icon(p_name, SNAME("EditorIcons"));
}
#endif

void Window::add_theme_font_size_override(const String& p_name, int p_font_size)
{
	ERR_MAIN_THREAD_GUARD;
	theme_font_size_override[p_name] = p_font_size;
	_notify_theme_override_changed();
}

void Window::add_theme_color_override(const String& p_name, const Color& p_color)
{
	ERR_MAIN_THREAD_GUARD;
	theme_color_override[p_name] = p_color;
	_notify_theme_override_changed();
}

void Window::add_theme_constant_override(const String& p_name, int p_constant)
{
	ERR_MAIN_THREAD_GUARD;
	theme_constant_override[p_name] = p_constant;
	_notify_theme_override_changed();
}

void Window::remove_theme_font_size_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_font_size_override.erase(p_name);
	_notify_theme_override_changed();
}

void Window::remove_theme_color_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_color_override.erase(p_name);
	_notify_theme_override_changed();
}

Ref<StyleBox> Window::get_theme_stylebox(const String& p_name, const String& p_theme_type) const
{
	Ref<StyleBox> sb = memnew(StyleBox);
	return sb;
}

void Window::remove_theme_constant_override(const String& p_name)
{
	ERR_MAIN_THREAD_GUARD;
	theme_constant_override.erase(p_name);
	_notify_theme_override_changed();
}

bool Window::has_theme_icon_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Texture2D>* tex = theme_icon_override.getptr(p_name);
	return tex != nullptr;
}

bool Window::has_theme_stylebox_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<StyleBox>* style = theme_style_override.getptr(p_name);
	return style != nullptr;
}

bool Window::has_theme_font_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Ref<Font>* font = theme_font_override.getptr(p_name);
	return font != nullptr;
}

bool Window::has_theme_font_size_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* font_size = theme_font_size_override.getptr(p_name);
	return font_size != nullptr;
}

bool Window::has_theme_color_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const Color* color = theme_color_override.getptr(p_name);
	return color != nullptr;
}

bool Window::has_theme_constant_override(const String& p_name) const
{
	ERR_READ_THREAD_GUARD_V(false);
	const int* constant = theme_constant_override.getptr(p_name);
	return constant != nullptr;
}

/// Default theme properties.

float Window::get_theme_default_base_scale() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return theme_owner->get_theme_default_base_scale();
}

Ref<Font> Window::get_theme_default_font() const
{
	ERR_READ_THREAD_GUARD_V(Ref<Font>());
	return theme_owner->get_theme_default_font();
}

int Window::get_theme_default_font_size() const
{
	ERR_READ_THREAD_GUARD_V(0);
	return theme_owner->get_theme_default_font_size();
}

/// Bulk actions.

void Window::begin_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	bulk_theme_override = true;
}

void Window::end_bulk_theme_override()
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_COND(!bulk_theme_override);

	bulk_theme_override = false;
	_notify_theme_override_changed();
}

//

Rect2i Window::get_parent_rect() const
{
	ERR_READ_THREAD_GUARD_V(Rect2i());
	ERR_FAIL_COND_V(!is_inside_tree(), Rect2i());
	if (is_embedded()) {
		// viewport
		Node* n = get_parent();
		ERR_FAIL_NULL_V(n, Rect2i());
		Viewport* p = n->get_viewport();
		ERR_FAIL_NULL_V(p, Rect2i());

		return p->get_visible_rect();
	}
	else {
		int x = get_position().x;
		int closest_dist = 0x7FFFFFFF;
		Rect2i closest_rect;
		for (int i = 0; i < DisplayServer::get_screen_count(); i++) {
			Rect2i s(DisplayServer::screen_get_position(i),
				DisplayServer::screen_get_size(i));
			int d;
			if (x >= s.position.x && x < s.size.x) {
				// contained
				closest_rect = s;
				break;
			}
			else if (x < s.position.x) {
				d = s.position.x - x;
			}
			else {
				d = x - (s.position.x + s.size.x);
			}

			if (d < closest_dist) {
				closest_dist = d;
				closest_rect = s;
			}
		}
		return closest_rect;
	}
}

void Window::set_clamp_to_embedder(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	clamp_to_embedder = p_enable;
}

bool Window::is_clamped_to_embedder() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return clamp_to_embedder;
}

void Window::set_unparent_when_invisible(bool p_unparent) { unparent_when_invisible = p_unparent; }

void Window::set_layout_direction(Window::LayoutDirection p_direction)
{
	ERR_MAIN_THREAD_GUARD;
	ERR_FAIL_INDEX(p_direction, LAYOUT_DIRECTION_MAX);

	layout_dir = p_direction;
	propagate_notification(Control::NOTIFICATION_LAYOUT_DIRECTION_CHANGED);
}

Window::LayoutDirection Window::get_layout_direction() const
{
	ERR_READ_THREAD_GUARD_V(LAYOUT_DIRECTION_INHERITED);
	return layout_dir;
}

#ifndef DISABLE_DEPRECATED

void Window::set_use_font_oversampling(bool p_oversampling)
{
	Viewport::set_use_oversampling(p_oversampling);
}

bool Window::is_using_font_oversampling() const { return Viewport::is_using_oversampling(); }

void Window::set_auto_translate(bool p_enable)
{
	ERR_MAIN_THREAD_GUARD;
	set_auto_translate_mode(p_enable ? AUTO_TRANSLATE_MODE_ALWAYS : AUTO_TRANSLATE_MODE_DISABLED);
}

bool Window::is_auto_translating() const
{
	ERR_READ_THREAD_GUARD_V(false);
	return can_auto_translate();
}
#endif

Transform2D Window::get_final_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	return window_transform * stretch_transform * global_canvas_transform;
}

Transform2D Window::get_screen_transform_internal(bool p_absolute_position) const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	Transform2D embedder_transform;
	if (get_embedder()) {
		embedder_transform.translate_local(get_position());
		embedder_transform =
			get_embedder()->get_screen_transform_internal(p_absolute_position) * embedder_transform;
	}
	else if (p_absolute_position) {
		embedder_transform.translate_local(get_position());
	}
	return embedder_transform * get_final_transform();
}

Transform2D Window::get_popup_base_transform_native() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (!DisplayServer::has_feature(DisplayServerEnums::FEATURE_SUBWINDOWS)) {
		return Transform2D();
	}
	Transform2D popup_base_transform;
	popup_base_transform.set_origin(get_position());
	popup_base_transform *= get_final_transform();
	if (get_embedder()) {
		return get_embedder()->get_popup_base_transform_native() * popup_base_transform;
	}
	return popup_base_transform;
}

Transform2D Window::get_popup_base_transform() const
{
	ERR_READ_THREAD_GUARD_V(Transform2D());
	if (is_embedding_subwindows()) {
		return Transform2D();
	}
	Transform2D popup_base_transform;
	popup_base_transform.set_origin(get_position());
	popup_base_transform *= get_final_transform();
	if (get_embedder()) {
		return get_embedder()->get_popup_base_transform() * popup_base_transform;
	}
	return popup_base_transform;
}

Viewport* Window::get_section_root_viewport() const
{
	if (get_embedder()) {
		return get_embedder()->get_section_root_viewport();
	}
	if (is_inside_tree()) {
		// Native window.
		return SceneTree::get_root();
	}
	Window* vp = const_cast<Window*>(this);
	return vp;
}

bool Window::is_attached_in_viewport() const { return get_embedder(); }

void Window::_update_mouse_over(Vector2 p_pos) {}

void Window::_mouse_leave_viewport()
{
	Viewport::_mouse_leave_viewport();
	if (is_embedded()) {
		mouse_in_window = false;
		_propagate_window_notification(this, NOTIFICATION_WM_MOUSE_EXIT);
	}
}

Window::Window()
{
	if (Renderer::data) {
		max_size = Renderer::get_maximum_viewport_size();
		max_size_used = max_size; // Update max_size_used.
	}

	theme_owner = memnew(ThemeOwner(this));
	Renderer::viewport_set_update_mode(
		get_viewport_rid(), RSE::VIEWPORT_UPDATE_DISABLED);
}

void Window::_update_viewport_size() {}

Color Window::get_theme_color(const String& p_name, const String& p_theme_type) const
{
	return Color();
}

Window* Window::get_parent_visible_window() const {}

void Window::_notify_theme_override_changed() {}

bool Window::is_layout_rtl() const {}


void Window::set_visible(bool) {}

Ref<Font> Window::get_theme_font(String const&, String const&) const {}

Window* Window::get_from_id(int) {}

void Window::_window_input(const InputEvent&) {}

void Window::_propagate_window_notification(Node*, int) {}

void Window::_popup_base(Rect2i const&) {}

int Window::get_theme_font_size(String const&, String const&) const {}

void Window::_update_theme_item_cache() {}

void Window::_settings_changed() {}

void Window::_make_transient() {}

Ref<Texture2D> Window::get_theme_icon(String const&, String const&) const {}

void Window::_event_callback(DisplayServerEnums::WindowEvent) {}

void Window::add_theme_font_override(String const&, Ref<Font> const&) {}

void Window::_accessibility_notify_exit(Node*) {}

void Window::_accessibility_notify_enter(Node*) {}

Size2 Window::_get_contents_minimum_size() const { return Size2(); }


// --- Merged from scene/register_scene_types.cpp ---
#include "main/main.h"

#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/shader.h"
#include "scene/main/node.h"

static Ref<ResourceFormatSaverText> resource_saver_text;
static Ref<ResourceFormatLoaderText> resource_loader_text;

static Ref<ResourceFormatLoaderCompressedTexture> resource_loader_compressed_texture;

static Ref<ResourceFormatSaverShader> resource_saver_shader;
static Ref<ResourceFormatLoaderShader> resource_loader_shader;

void unregister_scene_types() { SceneStrings::finalize(); }

void register_scene_types() { SceneStrings::initialize(); }

void register_scene_singletons() {}

// ==========================================================================
// Merged from scene/property_list_helper.cpp
// ==========================================================================

#include "scene/main/node.h"

HashMap<String, Vector<PropertyListHelper*>> PropertyListHelper::base_helpers; // static

void PropertyListHelper::clear_base_helpers()
{ // static
	for (KeyValue<String, Vector<PropertyListHelper*>>& E : base_helpers) {
		for (PropertyListHelper* helper : E.value) {
			helper->clear();
		}
	}
	base_helpers.clear();
}

void PropertyListHelper::register_base_helper(
	const String& p_class_name, PropertyListHelper* p_helper)
{ // static
	base_helpers[p_class_name].push_back(p_helper);
}

Vector<PropertyListHelper*> PropertyListHelper::get_helpers_for_class(
	const String& p_class_name)
{
	if (base_helpers.has(p_class_name)) {
		return base_helpers[p_class_name];
	}
	else {
		return Vector<PropertyListHelper*>();
	}
}

void PropertyListHelper::set_prefix(const String& p_prefix) { prefix = p_prefix; }

#ifdef TOOLS_ENABLED

#endif

bool PropertyListHelper::property_can_revert(const String& p_property) const
{
	return is_property_valid(p_property);
}

bool PropertyListHelper::is_property_valid(const String& p_property, int* r_index) const
{
	return false;
}

void PropertyListHelper::clear() {}

// ==========================================================================
// Merged from scene/property_utils.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "core/io/resource_loader.h"
#include "core/templates/local_vector.h"
#include "scene/main/node.h"
#include "scene/resources/packed_scene.h"

// Like SceneState::PackState, but using a raw pointer to avoid the cost of
// updating the reference count during the internal work of the functions below
namespace
{
struct _FastPackState
{
	SceneState* state = nullptr;
	int node = -1;
};
} // namespace

static bool _collect_inheritance_chain(const Ref<SceneState>& p_state, const NodePath& p_path,
	LocalVector<_FastPackState>& r_states_stack)
{
	bool found = false;

	LocalVector<_FastPackState> inheritance_states;

	Ref<SceneState> state = p_state;
	while (state.is_valid()) {
		int node = state->find_node_by_path(p_path);
		if (node >= 0) {
			// This one has state for this node
			inheritance_states.push_back({state.ptr(), node});
			found = true;
		}
		state = state->get_base_scene_state();
	}

	if (inheritance_states.size() > 0) {
		for (int i = inheritance_states.size() - 1; i >= 0; --i) {
			r_states_stack.push_back(inheritance_states[i]);
		}
	}

	return found;
}

// ============================================================================
// Merged from scene/audio/audio_stream_player.cpp
// ============================================================================

#include "scene/animation/animation_mixer.h"
#include "servers/audio/audio_stream.h"
#include "servers/display/accessibility_server.h"

void AudioStreamPlayer::set_stream(Ref<AudioStream> p_stream) { internal->set_stream(p_stream); }

Ref<AudioStream> AudioStreamPlayer::get_stream() const { return internal->stream; }

void AudioStreamPlayer::set_volume_db(float p_volume)
{
	ERR_FAIL_COND_MSG(Math::is_nan(p_volume), "Volume can't be set to NaN.");
	internal->volume_db = p_volume;

	Vector<AudioFrame> volume_vector = _get_volume_vector();
	for (Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
		AudioServer::set_playback_all_bus_volumes_linear(playback, volume_vector);
	}
}

float AudioStreamPlayer::get_volume_db() const { return internal->volume_db; }

void AudioStreamPlayer::set_volume_linear(float p_volume)
{
	set_volume_db(Math::linear_to_db(p_volume));
}

float AudioStreamPlayer::get_volume_linear() const { return Math::db_to_linear(get_volume_db()); }

void AudioStreamPlayer::set_pitch_scale(float p_pitch_scale)
{
	internal->set_pitch_scale(p_pitch_scale);
}

float AudioStreamPlayer::get_pitch_scale() const { return internal->pitch_scale; }

void AudioStreamPlayer::set_max_polyphony(int p_max_polyphony)
{
	internal->set_max_polyphony(p_max_polyphony);
}

int AudioStreamPlayer::get_max_polyphony() const { return internal->max_polyphony; }

void AudioStreamPlayer::play(float p_from_pos)
{
	Ref<AudioStreamPlayback> stream_playback = internal->play_basic();
	if (stream_playback.is_null()) {
		return;
	}
	AudioServer::start_playback_stream(
		stream_playback, internal->bus, _get_volume_vector(), p_from_pos, internal->pitch_scale);
	internal->ensure_playback_limit();

	// Sample handling.
	if (stream_playback->get_is_sample() && stream_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = stream_playback->get_sample_playback();
		sample_playback->offset = p_from_pos;
		sample_playback->volume_vector = _get_volume_vector();
		sample_playback->bus = get_bus();

		AudioServer::start_sample_playback(sample_playback);
	}
}

void AudioStreamPlayer::seek(float p_seconds) { internal->seek(p_seconds); }

void AudioStreamPlayer::stop() { internal->stop_basic(); }

bool AudioStreamPlayer::is_playing() const { return internal->is_playing(); }

float AudioStreamPlayer::get_playback_position() { return internal->get_playback_position(); }

void AudioStreamPlayer::set_bus(const String& p_bus)
{
	internal->bus = p_bus;
	for (const Ref<AudioStreamPlayback>& playback : internal->stream_playbacks) {
		AudioServer::set_playback_bus_exclusive(
			playback, p_bus, _get_volume_vector());
	}
}

String AudioStreamPlayer::get_bus() const { return internal->get_bus(); }

void AudioStreamPlayer::set_autoplay(bool p_enable) { internal->autoplay = p_enable; }

bool AudioStreamPlayer::is_autoplay_enabled() const { return internal->autoplay; }

void AudioStreamPlayer::set_mix_target(MixTarget p_target) { mix_target = p_target; }

AudioStreamPlayer::MixTarget AudioStreamPlayer::get_mix_target() const { return mix_target; }

void AudioStreamPlayer::_set_playing(bool p_enable) { internal->set_playing(p_enable); }

void AudioStreamPlayer::set_stream_paused(bool p_pause) { internal->set_stream_paused(p_pause); }

bool AudioStreamPlayer::get_stream_paused() const { return internal->get_stream_paused(); }

Vector<AudioFrame> AudioStreamPlayer::_get_volume_vector()
{
	Vector<AudioFrame> volume_vector;
	// We need at most four stereo pairs (for 7.1 systems).
	volume_vector.resize(4);

	// Initialize the volume vector to zero.
	for (AudioFrame& channel_volume_db : volume_vector) {
		channel_volume_db = AudioFrame(0, 0);
	}

	float volume_linear = Math::db_to_linear(internal->volume_db);

	// Set the volume vector up according to the speaker mode and mix target.
	// TODO do we need to scale the volume down when we output to more channels?
	if (AudioServer::get_speaker_mode() == AudioServer::SPEAKER_MODE_STEREO) {
		volume_vector.write[0] = AudioFrame(volume_linear, volume_linear);
	}
	else {
		switch (mix_target) {
		case MIX_TARGET_STEREO: {
			volume_vector.write[0] = AudioFrame(volume_linear, volume_linear);
		} break;
		case MIX_TARGET_SURROUND: {
			// TODO Make sure this is right.
			volume_vector.write[0] = AudioFrame(volume_linear, volume_linear);
			volume_vector.write[1] = AudioFrame(volume_linear, /* LFE= */ 1.0f);
			volume_vector.write[2] = AudioFrame(volume_linear, volume_linear);
			volume_vector.write[3] = AudioFrame(volume_linear, volume_linear);
		} break;
		case MIX_TARGET_CENTER: {
			// TODO Make sure this is right.
			volume_vector.write[1] = AudioFrame(volume_linear, /* LFE= */ 1.0f);
		} break;
		}
	}
	return volume_vector;
}

bool AudioStreamPlayer::has_stream_playback() { return internal->has_stream_playback(); }

Ref<AudioStreamPlayback> AudioStreamPlayer::get_stream_playback()
{
	return internal->get_stream_playback();
}

AudioServer::PlaybackType AudioStreamPlayer::get_playback_type() const
{
	return internal->get_playback_type();
}

void AudioStreamPlayer::set_playback_type(AudioServer::PlaybackType p_playback_type)
{
	internal->set_playback_type(p_playback_type);
}

AudioStreamPlayer::~AudioStreamPlayer() { memdelete(internal); }

void AudioStreamPlayerInternal::set_stream(Ref<AudioStream>) {}

void AudioStreamPlayerInternal::set_playing(bool) {}

void AudioStreamPlayerInternal::seek(float) {}

Ref<AudioStreamPlayback> AudioStreamPlayerInternal::play_basic()
{
	Ref<AudioStreamPlayback> asp = memnew(AudioStreamPlayback);
	return asp;
}

Error AudioServer::set_input_device_active(bool) { return OK; }

int AudioDriver::_get_configured_mix_rate() { return 0; }

void AudioStreamPlayerInternal::process() {}

void AudioStreamPlayerInternal::notification(int) {}

// ==========================================================================
// Merged from scene/audio/audio_stream_player_internal.cpp
// ==========================================================================

#include "core/config/engine.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "servers/audio/audio_stream.h"

void AudioStreamPlayerInternal::_set_process(bool p_enabled)
{
	if (physical) {
		node->set_physics_process_internal(p_enabled);
	}
	else {
		node->set_process_internal(p_enabled);
	}
}

void AudioStreamPlayerInternal::ensure_playback_limit()
{
	while (stream_playbacks.size() > max_polyphony) {
		AudioServer::stop_playback_stream(stream_playbacks[0]);
		stream_playbacks.remove_at(0);
	}
}

void AudioStreamPlayerInternal::set_stream_paused(bool p_pause)
{
	// TODO this does not have perfect recall, fix that maybe? If there are zero playbacks
	// registered with the AudioServer, this bool isn't persisted.
	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::set_playback_paused(playback, p_pause);
		if (_is_sample() && playback->get_sample_playback().is_valid()) {
			AudioServer::set_sample_playback_pause(
				playback->get_sample_playback(), p_pause);
		}
	}
}

bool AudioStreamPlayerInternal::get_stream_paused() const
{
	// There's currently no way to pause some playback streams but not others. Check the first and
	// don't bother looking at the rest.
	if (!stream_playbacks.is_empty()) {
		return AudioServer::is_playback_paused(stream_playbacks[0]);
	}
	return false;
}

void AudioStreamPlayerInternal::stop_basic()
{
	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::stop_playback_stream(playback);
	}
	stream_playbacks.clear();

	active.clear();
	_set_process(false);
}

bool AudioStreamPlayerInternal::is_playing() const
{
	for (const Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		if (AudioServer::is_playback_active(playback)) {
			return true;
		}
	}
	return false;
}

float AudioStreamPlayerInternal::get_playback_position()
{
	// Return the playback position of the most recently started playback stream.
	if (!stream_playbacks.is_empty()) {
		return AudioServer::get_playback_position(
			stream_playbacks[stream_playbacks.size() - 1]);
	}
	return 0;
}

bool AudioStreamPlayerInternal::is_active() const { return active.is_set(); }

void AudioStreamPlayerInternal::set_pitch_scale(float p_pitch_scale)
{
	ERR_FAIL_COND(p_pitch_scale <= 0.0);
	pitch_scale = p_pitch_scale;

	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::set_playback_pitch_scale(playback, pitch_scale);
	}
}

void AudioStreamPlayerInternal::set_max_polyphony(int p_max_polyphony)
{
	if (p_max_polyphony > 0) {
		max_polyphony = p_max_polyphony;
	}
}

bool AudioStreamPlayerInternal::has_stream_playback() { return !stream_playbacks.is_empty(); }

Ref<AudioStreamPlayback> AudioStreamPlayerInternal::get_stream_playback()
{
	ERR_FAIL_COND_V_MSG(stream_playbacks.is_empty(), Ref<AudioStreamPlayback>(),
		"Player is inactive. Call play() before requesting get_stream_playback().");
	return stream_playbacks[stream_playbacks.size() - 1];
}

void AudioStreamPlayerInternal::set_playback_type(AudioServer::PlaybackType p_playback_type)
{
	playback_type = p_playback_type;
}

AudioServer::PlaybackType AudioStreamPlayerInternal::get_playback_type() const
{
	return playback_type;
}

String AudioStreamPlayerInternal::get_bus() const
{
	const String bus_name = bus;
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == bus_name) {
			return bus;
		}
	}
	return SceneString(Master);
}

// ============================================================================
// Merged from scene/theme/theme_db.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "scene/gui/control.h"
#include "scene/main/node.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"
#include "scene/resources/theme.h"
#include "scene/resources/texture.h"
// --- Inlined from scene/theme/default_theme.h ---
#include "scene/resources/theme.h"

void fill_default_theme(Ref<Theme> &theme, const Ref<Font> &default_font, const Ref<Font> &bold_font, const Ref<Font> &bold_italics_font, const Ref<Font> &italics_font, Ref<Texture2D> &default_icon, Ref<StyleBox> &default_style, float p_scale);
void make_default_theme(float p_scale, Ref<Font> p_font, TextServer::SubpixelPositioning p_font_subpixel = TextServer::SUBPIXEL_POSITIONING_AUTO, TextServer::Hinting p_font_hinting = TextServer::HINTING_LIGHT, TextServer::FontAntialiasing p_font_antialiased = TextServer::FONT_ANTIALIASING_GRAY, bool p_font_msdf = false, bool p_font_generate_mipmaps = false);

// --- End scene/theme/default_theme.h ---
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#include "scene/resources/theme.h"

void ThemeDB::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void ThemeDB::finalize()
{
	if (!data) {
		return;
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->project_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
	memdelete(data);
	data = nullptr;
}

void ThemeDB::initialize_theme()
{
	if (!data) {
		return;
	}
	if (Renderer::data) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::initialize_theme_noproject()
{
	if (!data) {
		return;
	}
	if (Renderer::data) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::finalize_theme()
{
	if (!data) {
		return;
	}
	if (!Renderer::data) {
		WARN_PRINT("Finalizing theme when there is no Renderer is an error; check the order "
				   "of operations.");
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
}

// Global Theme resources.

void ThemeDB::set_default_theme(const Ref<Theme>& p_default)
{
	if (data) {
		data->default_theme = p_default;
	}
}

Ref<Theme> ThemeDB::get_default_theme() { return data ? data->default_theme : Ref<Theme>(); }

void ThemeDB::set_project_theme(const Ref<Theme>& p_project_default)
{
	if (data) {
		data->project_theme = p_project_default;
	}
}

Ref<Theme> ThemeDB::get_project_theme() { return data ? data->project_theme : Ref<Theme>(); }

void ThemeDB::set_fallback_base_scale(float p_base_scale)
{
	if (data) {
		data->fallback_base_scale = p_base_scale;
	}
}

float ThemeDB::get_fallback_base_scale() { return data ? data->fallback_base_scale : 1.0f; }

void ThemeDB::set_fallback_font(const Ref<Font>& p_font)
{
	if (data) {
		data->fallback_font = p_font;
	}
}

Ref<Font> ThemeDB::get_fallback_font() { return data ? data->fallback_font : Ref<Font>(); }

void ThemeDB::set_fallback_font_size(int p_font_size)
{
	if (data) {
		data->fallback_font_size = p_font_size;
	}
}

int ThemeDB::get_fallback_font_size() { return data ? data->fallback_font_size : 16; }

void ThemeDB::set_fallback_icon(const Ref<Texture2D>& p_icon)
{
	if (data) {
		data->fallback_icon = p_icon;
	}
}

Ref<Texture2D> ThemeDB::get_fallback_icon()
{
	return data ? data->fallback_icon : Ref<Texture2D>();
}

void ThemeDB::set_fallback_stylebox(const Ref<StyleBox>& p_stylebox)
{
	if (data) {
		data->fallback_stylebox = p_stylebox;
	}
}

Ref<StyleBox> ThemeDB::get_fallback_stylebox()
{
	return data ? data->fallback_stylebox : Ref<StyleBox>();
}

void ThemeDB::get_native_type_dependencies(const String& p_base_type, Vector<String>& r_result)
{
	(void)r_result;
	if (p_base_type == String()) {
		return;
	}
}

void ThemeDB::_init_default_theme_context()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
	}
	data->default_theme_context = memnew(ThemeContext);
	Vector<Ref<Theme>> themes;

#ifdef TOOLS_ENABLED
	if (!Engine::is_editor_hint()) {
		themes.push_back(data->project_theme);
	}
#else
	themes.push_back(data->project_theme);
#endif
	themes.push_back(data->default_theme);
	data->default_theme_context->set_themes(themes);
}

void ThemeDB::_finalize_theme_contexts()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
		data->default_theme_context = nullptr;
	}
	while (data->theme_contexts.size()) {
		HashMap<Node*, ThemeContext*>::Iterator E = data->theme_contexts.begin();
		memdelete(E->value);
		data->theme_contexts.remove(E);
	}
}

ThemeContext* ThemeDB::create_theme_context(Node* p_node, Vector<Ref<Theme>>& p_themes)
{
	(void)p_node;
	(void)p_themes;
	return nullptr;
}

void ThemeDB::destroy_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return;
	}
	memdelete(data->theme_contexts[p_node]);
	data->theme_contexts.erase(p_node);
}

ThemeContext* ThemeDB::get_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return nullptr;
	}
	return data->theme_contexts[p_node];
}

ThemeContext* ThemeDB::get_default_theme_context()
{
	return data ? data->default_theme_context : nullptr;
}

ThemeContext* ThemeDB::get_nearest_theme_context(Node* p_for_node)
{
	if (!data || !p_for_node) {
		return nullptr;
	}
	ERR_FAIL_COND_V(!p_for_node->is_inside_tree(), nullptr);
	Node* parent_node = p_for_node->get_parent();
	while (parent_node) {
		if (data->theme_contexts.has(parent_node)) {
			return data->theme_contexts[parent_node];
		}
		parent_node = parent_node->get_parent();
	}
	return nullptr;
}

// Theme item binding.

void ThemeDB::bind_class_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::bind_class_external_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, const String& p_type_name,
	ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.type_name = p_type_name;
	bind.external = true;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::update_class_instance_items(Node* p_instance) { (void)p_instance; }

void ThemeDB::get_class_items(const String& p_class_name, List<ThemeItemBind>* r_list,
	bool p_include_inherited, Theme::DataType p_filter_type)
{
	if (!data) {
		return;
	}
	List<String> class_hierarchy;
	String class_name = p_class_name;
	while (class_name != String()) {
		class_hierarchy.push_front(class_name);
	}
	HashSet<String> inherited_props;
	for (const String& theme_type : class_hierarchy) {
		HashMap<String, List<ThemeItemBind>>::Iterator E =
			data->theme_item_binds_list.find(theme_type);
		if (E) {
			for (const ThemeItemBind& F : E->value) {
				if (p_filter_type != Theme::DATA_TYPE_MAX && F.data_type != p_filter_type) {
					continue;
				}
				if (inherited_props.has(F.item_name)) {
					continue;
				}
				if (F.external || F.class_name != p_class_name) {
					inherited_props.insert(F.item_name);
					if (!p_include_inherited) {
						continue;
					}
				}
				r_list->push_back(F);
			}
		}
	}
}

void ThemeDB::_sort_theme_items()
{
	if (!data) {
		return;
	}
	for (KeyValue<String, List<ThemeDB::ThemeItemBind>>& E : data->theme_item_binds_list) {
		E.value.sort_custom<ThemeItemBind::SortByType>();
	}
}

const Vector<Ref<Theme>> ThemeContext::get_themes() const { return themes; }

Ref<Theme> ThemeContext::get_fallback_theme() const
{
	if (themes.is_empty()) {
		return ThemeDB::get_default_theme();
	}
	return themes[themes.size() - 1];
}

// --- theme_owner.cpp ---

#include "scene/gui/control.h"
#include "scene/main/window.h"
#include "scene/resources/theme.h"

Node* ThemeOwner::_get_next_owner_node(Node* p_from_node) const
{
	Node n = Node();
	return &n;
}

Ref<Theme> ThemeOwner::_get_owner_node_theme(Node* p_owner_node) const
{
	Ref<Theme> t = memnew(Theme);
	return t;
}

ThemeContext* ThemeOwner::_get_active_owner_context() const
{
	if (owner_context) {
		return owner_context;
	}

	return ThemeDB::get_default_theme_context();
}

bool ThemeOwner::has_theme_item_in_types(
	Theme::DataType p_data_type, const String& p_name, const Vector<String>& p_theme_types)
{
	ERR_FAIL_COND_V_MSG(
		p_theme_types.is_empty(), false, "At least one theme type must be specified.");

	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered.
	Node* current_owner = owner_node;

	while (current_owner) {
		// For each theme resource check the theme types provided and see if p_name exists with any
		// of them.
		for (const String& E : p_theme_types) {
			Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

			if (owner_theme.is_valid() && owner_theme->has_theme_item(p_data_type, p_name, E)) {
				return true;
			}
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			for (const String& E : p_theme_types) {
				if (theme->has_theme_item(p_data_type, p_name, E)) {
					return true;
				}
			}
		}
	}

	// Finally, if no match exists, return false.
	return false;
}

float ThemeOwner::get_theme_default_base_scale()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_base_scale()) {
			return owner_theme->get_default_base_scale();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_base_scale()) {
				return theme->get_default_base_scale();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_base_scale();
}

Ref<Font> ThemeOwner::get_theme_default_font()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font()) {
			return owner_theme->get_default_font();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font()) {
				return theme->get_default_font();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_font();
}

int ThemeOwner::get_theme_default_font_size()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font_size()) {
			return owner_theme->get_default_font_size();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font_size()) {
				return theme->get_default_font_size();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_font_size();
}



void ThemeOwner::set_owner_node(Node*) {}

void ThemeOwner::set_owner_context(ThemeContext*, bool) {}

void ThemeOwner::propagate_theme_changed(Node*, Node*, bool, bool) {}

void ThemeContext::set_themes(Vector<Ref<Theme>>&) {}

// --- default_theme.cpp ---

#include "core/io/image.h"

#include "scene/resources/font.h"
#include "scene/resources/gradient.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/theme.h"
#include "scene/main/node.h"
#include "scene/theme/default_theme_icons.gen.h"
#include "scene/resources/theme.h"
#include "servers/text/text_server.h"

#ifdef BROTLI_ENABLED
#include "scene/theme/default_theme_icons.gen.h"
#endif

static const int default_font_size = 16;

static float scale = 1.0;

static const int default_margin = 4;
static const int default_corner_radius = 3;

static Ref<StyleBox> make_flat_stylebox(Color p_color, float p_margin_left = default_margin,
	float p_margin_top = default_margin, float p_margin_right = default_margin,
	float p_margin_bottom = default_margin, int p_corner_radius = default_corner_radius,
	bool p_draw_center = true, int p_border_width = 0)
{
	Ref<StyleBox> style(memnew(StyleBox(StyleBox::STYLE_FLAT)));
	style->set_bg_color(p_color);
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));

	style->set_corner_radius_all(Math::round(p_corner_radius * scale));
	style->set_anti_aliased(true);
	// Adjust level of detail based on the corners' effective sizes.
	style->set_corner_detail(MIN(Math::ceil(1.5 * p_corner_radius), 6) * scale);

	style->set_draw_center(p_draw_center);
	style->set_border_width_all(Math::round(p_border_width * scale));

	return style;
}

static Ref<StyleBox> sb_expand(
	Ref<StyleBox> p_sbox, float p_left, float p_top, float p_right, float p_bottom)
{
	p_sbox->set_expand_margin(SIDE_LEFT, Math::round(p_left * scale));
	p_sbox->set_expand_margin(SIDE_TOP, Math::round(p_top * scale));
	p_sbox->set_expand_margin(SIDE_RIGHT, Math::round(p_right * scale));
	p_sbox->set_expand_margin(SIDE_BOTTOM, Math::round(p_bottom * scale));
	return p_sbox;
}

static Ref<StyleBox> make_empty_stylebox(float p_margin_left = -1, float p_margin_top = -1,
	float p_margin_right = -1, float p_margin_bottom = -1)
{
	Ref<StyleBox> style(memnew(StyleBox(StyleBox::STYLE_EMPTY)));
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));
	return style;
}

void make_default_theme(float p_scale, Ref<Font> p_font,
	TextServer::SubpixelPositioning p_font_subpixel, TextServer::Hinting p_font_hinting,
	TextServer::FontAntialiasing p_font_antialiasing, bool p_font_msdf,
	bool p_font_generate_mipmaps)
{
	Ref<Theme> t;
	t.instantiate();
	Ref<StyleBox> default_style;
	Ref<Texture2D> default_icon;
	Ref<Font> default_font;
	Ref<FontVariation> bold_font;
	Ref<FontVariation> bold_italics_font;
	Ref<FontVariation> italics_font;
	float default_scale = CLAMP(p_scale, 0.5, 8.0);
	if (p_font.is_valid()) {
		// Use the custom font defined in the Project Settings.
		default_font = p_font;
	}
	else {
		// Use the default DynamicFont (separate from the editor font).
		// The default DynamicFont is chosen to have a small file size since it's
		// embedded in both editor and export template binaries.
		Ref<FontFile> dynamic_font;
		dynamic_font.instantiate();
#ifdef BROTLI_ENABLED
		dynamic_font->set_data_ptr(_font_OpenSans_SemiBold, _font_OpenSans_SemiBold_size);
		dynamic_font->set_subpixel_positioning(p_font_subpixel);
		dynamic_font->set_hinting(p_font_hinting);
		dynamic_font->set_antialiasing(p_font_antialiasing);
		dynamic_font->set_multichannel_signed_distance_field(p_font_msdf);
		dynamic_font->set_generate_mipmaps(p_font_generate_mipmaps);
#endif
		default_font = dynamic_font;
	}
	if (default_font.is_valid()) {
		bold_font.instantiate();
		bold_font->set_base_font(default_font);
		bold_font->set_variation_embolden(1.2);
		bold_italics_font.instantiate();
		bold_italics_font->set_base_font(default_font);
		bold_italics_font->set_variation_embolden(1.2);
		bold_italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
		italics_font.instantiate();
		italics_font->set_base_font(default_font);
		italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
	}
	fill_default_theme(t, default_font, bold_font, bold_italics_font, italics_font, default_icon,
		default_style, default_scale);
	ThemeDB::set_default_theme(t);
	ThemeDB::set_fallback_base_scale(default_scale);
	ThemeDB::set_fallback_icon(default_icon);
	ThemeDB::set_fallback_stylebox(default_style);
	ThemeDB::set_fallback_font(default_font);
	ThemeDB::set_fallback_font_size(default_font_size * default_scale);
}

void fill_default_theme(Ref<Theme>& r_theme, const Ref<Font>& p_font, const Ref<Font>& p_bold_font,
	const Ref<Font>& p_bold_italics_font, const Ref<Font>& p_italics_font, Ref<Texture2D>& r_icons,
	Ref<StyleBox>& r_style, float p_scale)
{
}


// --- Merged from scene/animation/animation_tree.cpp ---
// --- animation_tree.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "scene/animation/animation_mixer.h"

void AnimationTree::_set_process_callback_bind_compat_80813(
	AnimationTree::AnimationProcessCallback p_mode)
{
	set_callback_mode_process(
		static_cast<AnimationMixer::AnimationCallbackModeProcess>(static_cast<int>(p_mode)));
}

AnimationTree::AnimationProcessCallback
AnimationTree::_get_process_callback_bind_compat_80813() const
{
	return static_cast<AnimationProcessCallback>(static_cast<int>(get_callback_mode_process()));
}

Ref<AnimationNode> AnimationTree::_get_tree_root_bind_compat_80813() const
{
	return get_root_animation_node();
}

void AnimationTree::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "scene/animation/animation_mixer.h"

thread_local AnimationNode::ProcessState* AnimationNode::tls_process_state = nullptr;
thread_local AnimationNodeInstance* AnimationNode::current_instance = nullptr;

bool AnimationNode::is_parameter_read_only(const String& p_parameter) const
{
	if (p_parameter == current_length || p_parameter == current_position ||
		p_parameter == current_delta) {
		return true;
	}

	return false;
}

void AnimationNode::blend_animation(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const String& p_animation,
	AnimationMixer::PlaybackInfo& p_playback_info)
{
	p_playback_info.track_weights = &p_instance.track_weights;
	p_process_state.tree->make_animation_instance(p_animation, p_playback_info);
}

void AnimationNode::add_validation_error(const AnimationTree* p_tree, const String& p_path,
	const String& p_error, int p_input_index) const
{
	p_tree->_add_validation_error(p_path, p_error, p_input_index);
}

void AnimationNode::make_invalid(
	ProcessState& p_process_state, AnimationNodeInstance& p_instance, const String& p_reason)
{
	p_process_state.valid = false;

	// TODO:
	// Currently, if the AnimationTree stops due to an error as valid state at runtime, there is no
	// way for the user to be aware of the error. I assume we should output an error when there is a
	// change in p_process_state.valid between frames. Caching invalid_instance and checking the
	// difference every frame is accurate, but it is somewhat costly and therefore likely not
	// preferred. If we were to implement it, we should add an option in the project settings'
	// animation tab, like `enum PrintErrorMode: {NONE, LOOSE, STRICT}`.
	InvalidInstance& invalid_instance = p_process_state.invalid_instances[p_instance.path];
	invalid_instance.errors.push_back(p_reason);
}

AnimationNode::NodeTimeInfo AnimationNode::blend_input(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, int p_input,
	const AnimationMixer::PlaybackInfo& p_playback_info, FilterAction p_filter, bool p_sync,
	bool p_test_only)
{
	ERR_FAIL_INDEX_V(p_input, (int64_t)inputs.size(), NodeTimeInfo());

	AnimationNodeInstance* node_instance = nullptr;
	if (likely(p_instance.connection_instances.size() > 0)) {
		node_instance = p_instance.connection_instances[p_input];
	}
	if (unlikely(!node_instance)) {
		if (!p_test_only && p_instance.is_blended()) {
			make_invalid(p_process_state, p_instance,
				vformat(RTR("Nothing connected to input %d."), p_input));
		}
		return NodeTimeInfo();
	}

	real_t activity = 0.0;

	NodeTimeInfo nti = _blend_node(p_process_state, p_instance, *node_instance, p_playback_info,
		p_filter, p_sync, p_test_only, &activity);

#ifdef ENABLE_ACTIVITY_TRACKING
	LocalVector<AnimationNodeInstance::Activity>& input_activity = p_instance.input_activity;
	input_activity[p_input].last_pass = p_process_state.last_pass;
	input_activity[p_input].activity = activity;
#endif

	return nti;
}

AnimationNode::NodeTimeInfo AnimationNode::blend_node(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, AnimationNodeInstance* p_other,
	const AnimationMixer::PlaybackInfo& p_playback_info, FilterAction p_filter, bool p_sync,
	bool p_test_only)
{
	ERR_FAIL_NULL_V(p_other, NodeTimeInfo());
	return _blend_node(p_process_state, p_instance, *p_other, p_playback_info, p_filter, p_sync,
		p_test_only, nullptr);
}

AnimationNode::NodeTimeInfo AnimationNode::_blend_node(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, AnimationNodeInstance& p_other,
	AnimationMixer::PlaybackInfo p_playback_info, FilterAction p_filter, bool p_sync,
	bool p_test_only, real_t* r_activity)
{
	int blend_count = p_instance.track_weights.size();

	if ((int64_t)p_other.track_weights.size() != blend_count) {
		p_other.track_weights.resize(blend_count);
	}

	real_t* blendw = p_other.track_weights.ptr();
	const real_t* blendr = p_instance.track_weights.ptr();

	bool any_valid = false;

	if (has_filter() && is_filter_enabled() && p_filter != FILTER_IGNORE) {
		_update_filter_cache(p_process_state, p_instance);
		// All to zero by default.
		memset(blendw, 0, sizeof(real_t) * blend_count);

		for (const int idx : p_instance.filtered_track_indices_cache) {
			blendw[idx] = 1.0; // Filtered goes to one.
		}

		switch (p_filter) {
		case FILTER_IGNORE:
			break; // Will not happen anyway.
		case FILTER_PASS: {
			// Values filtered pass, the rest don't.
			for (int i = 0; i < blend_count; i++) {
				if (blendw[i] == 0) { // Not filtered, does not pass.
					continue;
				}

				blendw[i] = blendr[i] * p_playback_info.weight;
				if (!Math::is_zero_approx(blendw[i])) {
					any_valid = true;
				}
			}

		} break;
		case FILTER_STOP: {
			// Values filtered don't pass, the rest are blended.

			for (int i = 0; i < blend_count; i++) {
				if (blendw[i] > 0) { // Filtered, does not pass.
					continue;
				}

				blendw[i] = blendr[i] * p_playback_info.weight;
				if (!Math::is_zero_approx(blendw[i])) {
					any_valid = true;
				}
			}

		} break;
		case FILTER_BLEND: {
			// Filtered values are blended, the rest are passed without blending.

			for (int i = 0; i < blend_count; i++) {
				if (blendw[i] == 1.0) {
					blendw[i] = blendr[i] * p_playback_info.weight; // Filtered, blend.
				}
				else {
					blendw[i] = blendr[i]; // Not filtered, do not blend.
				}

				if (!Math::is_zero_approx(blendw[i])) {
					any_valid = true;
				}
			}

		} break;
		}
	}
	else {
		for (int i = 0; i < blend_count; i++) {
			// Regular blend.
			blendw[i] = blendr[i] * p_playback_info.weight;
			if (!Math::is_zero_approx(blendw[i])) {
				any_valid = true;
			}
		}
	}

	if (r_activity) {
		*r_activity = 0;
#ifdef ENABLE_ACTIVITY_TRACKING
		for (int i = 0; i < blend_count; i++) {
			*r_activity = MAX(*r_activity, Math::abs(blendw[i]));
		}
#endif
	}

	// This process, which depends on p_sync is needed to process sync correctly in the case of
	// that a synced AnimationNode exists under an un-synced AnimationNode.
	if (!p_playback_info.seeked && !p_sync && !any_valid) {
		p_playback_info.delta = 0.0;
	}
	p_other.blended = any_valid;
	return p_other.resource->_pre_process(p_process_state, p_other, p_playback_info, p_test_only);
}

void AnimationNode::get_child_nodes(LocalVector<ChildNode>* r_child_nodes) {}

String AnimationNode::get_caption() const
{
	if (endpoint_type == ENDPOINT_START) {
		return "Start";
	}
	if (endpoint_type == ENDPOINT_END) {
		return "End";
	}
	return "Node";
}

void AnimationNode::set_use_sync(bool p_sync) { sync = p_sync; }

bool AnimationNode::is_using_sync() const { return sync; }

void AnimationNode::remove_input(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int64_t)inputs.size());
	inputs.remove_at(p_index);
	emit_changed();
}

bool AnimationNode::set_input_name(int p_input, const String& p_name)
{
	ERR_FAIL_INDEX_V(p_input, (int64_t)inputs.size(), false);
	ERR_FAIL_COND_V(p_name.contains_char('.') || p_name.contains_char('/'), false);
	inputs[p_input].name = p_name;
	emit_changed();
	return true;
}

String AnimationNode::get_input_name(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int64_t)inputs.size(), String());
	return inputs[p_input].name;
}

int AnimationNode::get_input_count() const { return inputs.size(); }

int AnimationNode::find_input(const String& p_name) const
{
	int idx = -1;
	for (int i = 0; i < (int64_t)inputs.size(); i++) {
		if (inputs[i].name == p_name) {
			idx = i;
			break;
		}
	}
	return idx;
}

AnimationNode::NodeTimeInfo AnimationNode::_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	double r_ret = 0.0;
	NodeTimeInfo nti;
	nti.delta = r_ret;
	return nti;
}

void AnimationNode::set_filter_path(const NodePath& p_path, bool p_enable)
{
	if (p_enable) {
		(void)p_path.hash(); // Make sure the cache is valid.
		filter.insert(p_path);
	}
	else {
		filter.erase(p_path);
	}
	_mark_filters_dirty();
}

void AnimationNode::set_filter_enabled(bool p_enable)
{
	filter_enabled = p_enable;
	_mark_filters_dirty();
}

bool AnimationNode::is_filter_enabled() const { return filter_enabled; }

void AnimationNode::_mark_filters_dirty()
{
	filters_version++;
	if (unlikely(filters_version == 0)) {
		filters_version = 1;
	}
}

void AnimationNode::set_deletable(bool p_closable) { closable = p_closable; }

bool AnimationNode::is_deletable() const { return closable; }

bool AnimationNode::is_process_testing() const
{
	ERR_FAIL_NULL_V(tls_process_state, false);
	return tls_process_state->is_testing;
}

bool AnimationNode::is_path_filtered(const NodePath& p_path) const { return filter.has(p_path); }

void AnimationNode::_update_filter_cache(
	const ProcessState& p_process_state, const AnimationNodeInstance& p_instance)
{
	bool filters_dirty = p_instance.filters_version != filters_version;
	if (!p_process_state.track_map_updated && !filters_dirty) {
		return; // Cache is valid.
	}

	p_instance.filtered_track_indices_cache.clear();
	if (p_instance.filtered_track_indices_cache.size() < filter.size()) {
		p_instance.filtered_track_indices_cache.reserve(filter.size());
	}

	for (const NodePath& path : filter) {
		if (const int* p = p_process_state.track_map->getptr(path)) {
			p_instance.filtered_track_indices_cache.push_back(*p);
		}
	}
	p_instance.filters_version = filters_version;
}

Ref<AnimationNode> AnimationNode::find_node_by_path(const String& p_name) const
{
	Vector<String> split = p_name.split("/");
	Ref<AnimationNode> ret = const_cast<AnimationNode*>(this);
	for (int i = 0; i < split.size(); i++) {
		ret = ret->get_child_by_name(split[i]);
		if (ret.is_null()) {
			break;
		}
	}
	return ret;
}

void AnimationNode::blend_animation_ex(const String& p_animation, double p_time, double p_delta,
	bool p_seeked, bool p_is_external_seeking, real_t p_blend, Animation::LoopedFlag p_looped_flag)
{
	ERR_FAIL_NULL(tls_process_state);
	ERR_FAIL_NULL(current_instance);

	AnimationMixer::PlaybackInfo info;
	info.time = p_time;
	info.delta = p_delta;
	info.seeked = p_seeked;
	info.is_external_seeking = p_is_external_seeking;
	info.weight = p_blend;
	info.looped_flag = p_looped_flag;

	blend_animation(*tls_process_state, *current_instance, p_animation, info);
}

double AnimationNode::blend_node_ex(const String& p_sub_path, const Ref<AnimationNode>& p_node,
	double p_time, bool p_seek, bool p_is_external_seeking, real_t p_blend, FilterAction p_filter,
	bool p_sync, bool p_test_only)
{
	ERR_FAIL_NULL_V(tls_process_state, 0.0);
	ERR_FAIL_NULL_V(current_instance, 0.0);

	AnimationNodeInstance* other_instance =
		current_instance->get_child_instance_by_path_or_null(p_sub_path);
	ERR_FAIL_NULL_V_MSG(other_instance, 0.0,
		vformat("The sub-path '%s' does not exist under the current node instance.",
			String(p_sub_path)));

	AnimationMixer::PlaybackInfo info;
	info.time = p_time;
	info.seeked = p_seek;
	info.is_external_seeking = p_is_external_seeking;
	info.weight = p_blend;

	NodeTimeInfo nti = blend_node(
		*tls_process_state, *current_instance, other_instance, info, p_filter, p_sync, p_test_only);
	return nti.length - nti.position;
}

double AnimationNode::blend_input_ex(int p_input, double p_time, bool p_seek,
	bool p_is_external_seeking, real_t p_blend, FilterAction p_filter, bool p_sync,
	bool p_test_only)
{
	ERR_FAIL_NULL_V(tls_process_state, 0.0);
	ERR_FAIL_NULL_V(current_instance, 0.0);

	AnimationMixer::PlaybackInfo info;
	info.time = p_time;
	info.seeked = p_seek;
	info.is_external_seeking = p_is_external_seeking;
	info.weight = p_blend;

	NodeTimeInfo nti = blend_input(
		*tls_process_state, *current_instance, p_input, info, p_filter, p_sync, p_test_only);
	return nti.length - nti.position;
}

#ifdef TOOLS_ENABLED

#endif

AnimationNode::AnimationNode() {}

void AnimationTree::set_root_animation_node(const Ref<AnimationNode>& p_animation_node)
{
	root_animation_node = p_animation_node;
	validation_dirty = true;
	properties_dirty = true;
}

Ref<AnimationNode> AnimationTree::get_root_animation_node() const
{
	return root_animation_node;
}

bool AnimationTree::_blend_pre_process(
	double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	_update_properties(); // If properties need updating, update them.

	if (root_animation_node.is_null()) {
		process_state = AnimationNode::ProcessState();
		return false; // Abort after _update_properties() and init process_state.
	}

	if (validation_dirty) {
		_update_connections();
		validation_dirty = false;
	}

	AnimationNodeInstance& instance =
		get_node_instance_by_path(SNAME(Animation::PARAMETERS_BASE_PATH.ascii().get_data()));

	{ // Setup.
		process_pass++;
		if (unlikely(process_pass == 0)) {
			process_pass = 1;
		}

		// Init process state.
		process_state = AnimationNode::ProcessState();
		process_state.tree = this;
		process_state.valid = true;
		process_state.invalid_instances.clear();
		process_state.last_pass = process_pass;
		process_state.track_map = &p_track_map;
		process_state.track_map_updated = track_map_version != last_track_map_version;
		process_state.is_testing = false;

		last_track_map_version = track_map_version;

		// Init node state for root AnimationNode.
		instance.track_weights.resize(p_track_count);
		real_t* src_blendsw = instance.track_weights.ptr();
		for (int i = 0; i < p_track_count; i++) {
			src_blendsw[i] = 1.0; // By default all go to 1 for the root input.
		}
		instance.blended = true;
		instance.path = SNAME(Animation::PARAMETERS_BASE_PATH.ascii().get_data());
	}

	// Process.
	{
		PlaybackInfo pi;
		pi.delta = p_delta;

		if (started) {
			started = false;
			// If started, seek.
			pi.seeked = true;
		}
		else {
			pi.seeked = false;
		}

		AnimationNode::tls_process_state = &process_state;
		root_animation_node->_pre_process(process_state, instance, pi, false);
		AnimationNode::tls_process_state = nullptr;
	}

	if (!process_state.valid) {
		return false; // State is not valid, abort process.
	}

	return true;
}

void AnimationTree::_set_active(bool p_active)
{
	_set_process(p_active);
	started = p_active;
}

void AnimationTree::set_advance_expression_base_node(const NodePath& p_path)
{
	advance_expression_base_node = p_path;
}

NodePath AnimationTree::get_advance_expression_base_node() const
{
	return advance_expression_base_node;
}

bool AnimationTree::is_state_invalid() const { return !process_state.valid; }

const AHashMap<String, AnimationNode::InvalidInstance>&
AnimationTree::get_invalid_instances() const
{
	return process_state.invalid_instances;
}

PackedStringArray AnimationTree::get_configuration_warnings() const
{
	PackedStringArray warnings = AnimationMixer::get_configuration_warnings();
	if (root_animation_node.is_null()) {
		warnings.push_back(RTR("No root AnimationNode for the graph is set."));
	}
	return warnings;
}

void AnimationTree::_add_validation_error(
	const String& p_path, const String& p_error, int p_input_index) const
{
	AnimationNode::InvalidInstance& invalid_instance = process_state.invalid_instances[p_path];

	if (p_input_index == -1) {
		if (invalid_instance.errors.find(p_error) == -1) {
			invalid_instance.errors.push_back(p_error);
		}
	}
	else {
		for (const AnimationNode::InvalidInstance::InputError& E : invalid_instance.input_errors) {
			if (E.index == p_input_index) {
				return;
			}
		}
		invalid_instance.input_errors.push_back({p_input_index, p_error});
	}
}

void AnimationTree::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_setup_animation_player();
		if (active) {
			_set_process(true);
		}
	} break;
	}
}

NodePath AnimationTree::get_animation_player() const { return animation_player; }

#ifdef ENABLE_ACTIVITY_TRACKING
real_t AnimationTree::get_connection_activity(const String& p_path, int p_connection) const
{
	const AnimationNodeInstance* a = get_node_instance_by_path_or_null(p_path);
	if (!a) {
		return 0;
	}

	const LocalVector<AnimationNodeInstance::Activity>& activity = a->input_activity;
	if (p_connection < 0 || p_connection >= (int64_t)activity.size() ||
		activity[p_connection].last_pass != process_pass) {
		return 0;
	}

	return activity[p_connection].activity;
}
#endif

#ifdef TOOLS_ENABLED
String AnimationTree::get_editor_error_message() const
{
	if (!is_active()) {
		return TTR("The AnimationTree is inactive.\nActivate it in the inspector to enable "
				   "playback; check node warnings if activation fails.");
	}
	else if (!is_enabled()) {
		return TTR("The AnimationTree node (or one of its ancestors) has its process mode set to "
				   "Disabled.\nChange the process mode in the inspector to allow playback.");
	}

	return "";
}
#endif

AnimationTree::AnimationTree()
{
	deterministic = true;
	callback_mode_discrete = ANIMATION_CALLBACK_MODE_DISCRETE_FORCE_CONTINUOUS;
}

AnimationTree::~AnimationTree() {}

bool AnimationNode::has_filter() const { return false; }

Ref<AnimationNode> AnimationNode::get_child_by_name(const String& p_name) const
{
	return Ref<AnimationNode>();
}

void AnimationNode::_tree_changed() {}

void AnimationNode::_add_node(const Ref<AnimationNode>& p_node) {}

void AnimationNode::_remove_node(const Ref<AnimationNode>& p_node) {}

bool AnimationNode::add_input(const String& p_name) { return true; }

uint32_t AnimationTree::_get_libraries_property_usage() const { return 0; }

void AnimationTree::_update_properties() const {}

void AnimationTree::_update_connections() {}

void AnimationTree::_setup_animation_player() {}

// --- AnimationNode base methods (moved from animation_node_extension.cpp) ---

AnimationNode::NodeTimeInfo AnimationNode::_pre_process(
	AnimationNode::ProcessState&, AnimationNodeInstance&, AnimationMixer::PlaybackInfo const&, bool)
{
	return AnimationNode::NodeTimeInfo();
}

void AnimationNode::get_argument_options(
	String const&, int, List<String, DefaultAllocator>*) const
{
}

// ==========================================================================
// Merged from scene/animation/animation_blend_space.cpp
// ==========================================================================
// --- Inlined from scene/animation/animation_blend_space.h ---
#include "scene/animation/animation_mixer.h"

class AnimationNodeBlendSpace final : public AnimationNode
{
	const String ERR_NO_BLEND_POINT = "No blend points exist, so blending cannot take place.";
	const String ERR_NO_TRIANGLE = "No triangles exist, so blending cannot take place.";
	const String ERR_INVALID_POINT =
		"Cyclic sync modes require that all blend points in BlendSpace use non-nested Animation "
		"nodes with a finite, immutable length.";

public:
	enum Dimension
	{
		DIMENSION_1D,
		DIMENSION_2D,
	};

	enum BlendMode
	{
		BLEND_MODE_INTERPOLATED,
		BLEND_MODE_DISCRETE,
		BLEND_MODE_DISCRETE_CARRY,
	};

	enum SyncMode
	{
		SYNC_MODE_NONE,			   // Inactive animations are frozen (not advanced).
		SYNC_MODE_INDEPENDENT,	   // Inactive animations advance with weight=0 (previous "sync"
								   // behavior).
		SYNC_MODE_CYCLIC_MUTABLE,  // Time-scaled with blend-weight-dependent cycle length
								   // (self-normalizing).
		SYNC_MODE_CYCLIC_CONSTANT, // Time-scaled to complete one cycle in cyclic_length seconds.
	};

private:
	enum
	{
		MAX_BLEND_POINTS = 64
	};

	struct BlendPoint
	{
		String name;
		Ref<AnimationNode> node;
		Vector2 position;

		void reset()
		{
			name = String();
			node = Ref<AnimationNode>();
			position = Vector2();
		}
	};

	struct BlendTriangle
	{
		int points[3] = {};
	};

	Dimension dimension = DIMENSION_1D;
	BlendPoint blend_points[MAX_BLEND_POINTS];
	int blend_points_used = 0;

	Vector<BlendTriangle> triangles;

	String blend_position = "blend_position";
	String closest = "closest";
	Vector2 max_space = Vector2(1, 1);
	Vector2 min_space = Vector2(-1, -1);
	Vector2 snap = Vector2(0.1f, 0.1f);
	String value_label = "value";
	String x_label = "x";
	String y_label = "y";
	BlendMode blend_mode = BLEND_MODE_INTERPOLATED;

	bool auto_triangles = true;
	bool triangles_dirty = false;

	SyncMode sync_mode = SYNC_MODE_NONE;
	double cyclic_length = 0.0;
	double inverted_cycle_length = 0.0; // Cached 1/cyclic_length.
	LocalVector<double> cached_lengths;
	bool lengths_dirty = true;
	bool is_contain_invalid_point = false;

	void _set_triangles(const Vector<int>& p_triangles);
	Vector<int> _get_triangles() const;

#ifndef DISABLE_DEPRECATED
	void _retry_set_triangles(const Vector<int>& p_triangles);
	void _add_blend_point_bind_compat_110369(
		const Ref<AnimationNode>& p_node, float p_position, int p_at_index = -1);
	void _add_blend_point_bind_compat_110369(
		const Ref<AnimationNode>& p_node, const Vector2& p_position, int p_at_index = -1);
	static void _bind_compatibility_methods();
#endif

	void _blend_triangle(
		const Vector2& p_pos, const LocalVector<Vector2>& p_points, LocalVector<float>& r_weights);
	void _update_triangles();
	void _queue_auto_triangles();
	void _check_can_sync();

protected:
	virtual void _tree_changed() override;

public:
	Dimension get_dimension() const { return dimension; }

	virtual void validate_node(
		const AnimationTree* p_tree, const String& p_path) const override;
	virtual void get_child_nodes(LocalVector<ChildNode>* r_child_nodes) override;

	void add_blend_point(const Ref<AnimationNode>& p_node, float p_position,
		int p_at_index = -1, const String& p_name = String());
	void add_blend_point(const Ref<AnimationNode>& p_node, const Vector2& p_position,
		int p_at_index = -1, const String& p_name = String());

	void set_blend_point_position(int p_point, float p_position);
	void set_blend_point_position(int p_point, const Vector2& p_position);
	Vector2 get_blend_point_position(int p_point) const;
	float get_blend_point_position_1d(int p_point) const;

	void set_blend_point_node(int p_point, const Ref<AnimationNode>& p_node);
	Ref<AnimationNode> get_blend_point_node(int p_point) const;
	void set_blend_point_name(int p_point, const String& p_name);
	const String& get_blend_point_name(int p_point) const;
	int find_blend_point_by_name(const String& p_name) const;
	void remove_blend_point(int p_point);
	int get_blend_point_count() const;
	void reorder_blend_point(int p_from_index, int p_to_index);

	// 2D triangle methods
	bool has_triangle(int p_x, int p_y, int p_z) const;
	void add_triangle(int p_x, int p_y, int p_z, int p_at_index = -1);
	int get_triangle_point(int p_triangle, int p_point);
	void remove_triangle(int p_triangle);
	int get_triangle_count() const;
	Vector2 get_closest_point(const Vector2& p_point);
	void set_auto_triangles(bool p_enable);
	bool get_auto_triangles() const;

	// Space & snap (1D and 2D)
	void set_min_space(float p_min);
	void set_min_space(const Vector2& p_min);
	Vector2 get_min_space() const;
	float get_min_space_1d() const;

	void set_max_space(float p_max);
	void set_max_space(const Vector2& p_max);
	Vector2 get_max_space() const;
	float get_max_space_1d() const;

	void set_snap(float p_snap);
	void set_snap(const Vector2& p_snap);
	Vector2 get_snap() const;
	float get_snap_1d() const;

	// Labels
	void set_value_label(const String& p_label);
	String get_value_label() const;
	void set_x_label(const String& p_label);
	String get_x_label() const;
	void set_y_label(const String& p_label);
	String get_y_label() const;

	// Blend & sync modes
	void set_blend_mode(BlendMode p_blend_mode);
	BlendMode get_blend_mode() const;

	void set_cyclic_length(double p_length);
	double get_cyclic_length() const;

#ifndef DISABLE_DEPRECATED
	virtual void set_use_sync(bool p_sync) override;
	virtual bool is_using_sync() const override;
#endif // DISABLE_DEPRECATED

	void set_sync_mode(SyncMode p_sync_mode);
	SyncMode get_sync_mode() const;

	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false) override;
	virtual String get_caption() const override;
	virtual Ref<AnimationNode> get_child_by_name(const String& p_name) const override;

	explicit AnimationNodeBlendSpace(Dimension p_dimension = DIMENSION_1D) : dimension(p_dimension)
	{
		root_node = true;
	}
};

// --- End scene/animation/animation_blend_space.h ---
#include "core/math/geometry_2d.h"
#include "scene/animation/animation_mixer.h"

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::_add_blend_point_bind_compat_110369(
	const Ref<AnimationNode>& p_node, float p_position, int p_at_index)
{
	int n = p_at_index == -1 ? blend_points_used : p_at_index;
	while (find_blend_point_by_name(itos(n)) != -1) {
		n++;
	}
	add_blend_point(p_node, p_position, p_at_index, String(itos(n)));
}

void AnimationNodeBlendSpace::_add_blend_point_bind_compat_110369(
	const Ref<AnimationNode>& p_node, const Vector2& p_position, int p_at_index)
{
	int n = p_at_index == -1 ? blend_points_used : p_at_index;
	while (find_blend_point_by_name(itos(n)) != -1) {
		n++;
	}
	add_blend_point(p_node, p_position, p_at_index, String(itos(n)));
}

void AnimationNodeBlendSpace::_bind_compatibility_methods() {}
#endif // DISABLE_DEPRECATED

Ref<AnimationNode> AnimationNodeBlendSpace::get_child_by_name(const String& p_name) const
{
	int point_index = find_blend_point_by_name(p_name);
	if (point_index != -1) {
		return get_blend_point_node(point_index);
	}
	return Ref<AnimationNode>();
}

void AnimationNodeBlendSpace::_tree_changed()
{
	AnimationNode::_tree_changed();
	_check_can_sync();
}

void AnimationNodeBlendSpace::validate_node(
	const AnimationTree* p_tree, const String& p_path) const
{
	AnimationNode::validate_node(p_tree, p_path);

	if (dimension == DIMENSION_1D) {
		if (get_blend_point_count() == 0) {
			add_validation_error(p_tree, p_path, RTR(ERR_NO_BLEND_POINT));
		}
	}
	else {
		const_cast<AnimationNodeBlendSpace*>(this)->_update_triangles();
		if (get_triangle_count() == 0) {
			add_validation_error(p_tree, p_path, RTR(ERR_NO_TRIANGLE));
		}
	}

	const_cast<AnimationNodeBlendSpace*>(this)->_check_can_sync();
	if (is_contain_invalid_point) {
		add_validation_error(p_tree, p_path, RTR(ERR_INVALID_POINT));
	}
}

void AnimationNodeBlendSpace::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	for (int i = 0; i < blend_points_used; i++) {
		ChildNode cn;
		cn.name = blend_points[i].name;
		cn.node = blend_points[i].node;
		r_child_nodes->push_back(cn);
	}
}

void AnimationNodeBlendSpace::add_blend_point(const Ref<AnimationNode>& p_node,
	float p_position, int p_at_index, const String& p_name)
{
	add_blend_point(p_node, Vector2(p_position, 0.0f), p_at_index, p_name);
}

void AnimationNodeBlendSpace::add_blend_point(const Ref<AnimationNode>& p_node,
	const Vector2& p_position, int p_at_index, const String& p_name)
{
	ERR_FAIL_COND(blend_points_used >= MAX_BLEND_POINTS);
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(p_at_index < -1 || p_at_index > blend_points_used);
#ifndef DISABLE_DEPRECATED
	if (p_name == String()) {
		_add_blend_point_bind_compat_110369(p_node, p_position, p_at_index);
		WARN_PRINT_ED("AnimationNodeBlendSpace::add_blend_point: No name provided, using safe "
					  "index as reference. In the future, empty names will be deprecated, so "
					  "explicitly passing a name is recommended.");
		return;
	}
#else
	ERR_FAIL_COND(p_name == String());
#endif

	if (p_at_index == -1 || p_at_index == blend_points_used) {
		p_at_index = blend_points_used;
	}
	else {
		for (int i = blend_points_used; i > p_at_index; i--) {
			blend_points[i] = blend_points[i - 1];
		}
		if (dimension == DIMENSION_2D) {
			for (int i = 0; i < triangles.size(); i++) {
				for (int j = 0; j < 3; j++) {
					if (triangles[i].points[j] >= p_at_index) {
						triangles.write[i].points[j]++;
					}
				}
			}
		}
	}

	blend_points[p_at_index].node = p_node;
	blend_points[p_at_index].position = p_position;
	blend_points[p_at_index].name = p_name;

	_add_node(blend_points[p_at_index].node);
	blend_points_used++;

	lengths_dirty = true;
	if (dimension == DIMENSION_2D) {
		_queue_auto_triangles();
	}

	_tree_changed();
}

void AnimationNodeBlendSpace::set_blend_point_position(int p_point, float p_position)
{
	set_blend_point_position(p_point, Vector2(p_position, 0.0f));
}

void AnimationNodeBlendSpace::set_blend_point_position(int p_point, const Vector2& p_position)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	blend_points[p_point].position = p_position;
	if (dimension == DIMENSION_2D) {
		_queue_auto_triangles();
	}
}

Vector2 AnimationNodeBlendSpace::get_blend_point_position(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Vector2());
	return blend_points[p_point].position;
}

float AnimationNodeBlendSpace::get_blend_point_position_1d(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, 0.0f);
	return blend_points[p_point].position.x;
}

void AnimationNodeBlendSpace::set_blend_point_node(
	int p_point, const Ref<AnimationNode>& p_node)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	ERR_FAIL_COND(p_node.is_null());

	if (blend_points[p_point].node.is_valid()) {
		_remove_node(blend_points[p_point].node);
	}
	blend_points[p_point].node = p_node;
	_add_node(blend_points[p_point].node);

	lengths_dirty = true;
	_tree_changed();
}

Ref<AnimationNode> AnimationNodeBlendSpace::get_blend_point_node(int p_point) const
{
	ERR_FAIL_INDEX_V(p_point, MAX_BLEND_POINTS, Ref<AnimationNode>());
	return blend_points[p_point].node;
}

void AnimationNodeBlendSpace::set_blend_point_name(int p_point, const String& p_name)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	ERR_FAIL_COND(p_name.is_empty());
	blend_points[p_point].name = p_name;
}

const String& AnimationNodeBlendSpace::get_blend_point_name(int p_point) const
{
	const static String empty = String();
	ERR_FAIL_INDEX_V(p_point, blend_points_used, empty);
	return blend_points[p_point].name;
}

int AnimationNodeBlendSpace::find_blend_point_by_name(const String& p_name) const
{
	for (int i = 0; i < blend_points_used; i++) {
		if (blend_points[i].name == p_name) {
			return i;
		}
	}
	return -1;
}

void AnimationNodeBlendSpace::remove_blend_point(int p_point)
{
	ERR_FAIL_INDEX(p_point, blend_points_used);
	if (blend_points[p_point].node.is_valid()) {
		_remove_node(blend_points[p_point].node);
	}
	for (int i = p_point; i < blend_points_used - 1; i++) {
		blend_points[i] = blend_points[i + 1];
	}
	blend_points[blend_points_used - 1].reset();
	blend_points_used--;

	if (dimension == DIMENSION_2D) {
		for (int i = triangles.size() - 1; i >= 0; i--) {
			if (triangles[i].points[0] == p_point || triangles[i].points[1] == p_point ||
				triangles[i].points[2] == p_point) {
				triangles.remove_at(i);
			}
			else {
				for (int j = 0; j < 3; j++) {
					if (triangles[i].points[j] > p_point) {
						triangles.write[i].points[j]--;
					}
				}
			}
		}
		_queue_auto_triangles();
	}

	lengths_dirty = true;
	_tree_changed();
}

int AnimationNodeBlendSpace::get_blend_point_count() const { return blend_points_used; }

void AnimationNodeBlendSpace::reorder_blend_point(int p_from_index, int p_to_index)
{
	ERR_FAIL_INDEX(p_from_index, blend_points_used);
	ERR_FAIL_INDEX(p_to_index, blend_points_used);
	if (p_from_index == p_to_index) {
		return;
	}
	SWAP(blend_points[p_from_index], blend_points[p_to_index]);
	if (dimension == DIMENSION_2D) {
		for (int i = 0; i < triangles.size(); i++) {
			for (int j = 0; j < 3; j++) {
				if (triangles[i].points[j] == p_from_index) {
					triangles.write[i].points[j] = p_to_index;
				}
				else if (triangles[i].points[j] == p_to_index) {
					triangles.write[i].points[j] = p_from_index;
				}
			}
		}
	}
	lengths_dirty = true;
	_tree_changed();
}

bool AnimationNodeBlendSpace::has_triangle(int p_x, int p_y, int p_z) const
{
	ERR_FAIL_INDEX_V(p_x, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_y, blend_points_used, false);
	ERR_FAIL_INDEX_V(p_z, blend_points_used, false);

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		if (all_equal) {
			return true;
		}
	}

	return false;
}

void AnimationNodeBlendSpace::add_triangle(int p_x, int p_y, int p_z, int p_at_index)
{
	ERR_FAIL_INDEX(p_x, blend_points_used);
	ERR_FAIL_INDEX(p_y, blend_points_used);
	ERR_FAIL_INDEX(p_z, blend_points_used);

	_update_triangles();

	BlendTriangle t;
	t.points[0] = p_x;
	t.points[1] = p_y;
	t.points[2] = p_z;

	SortArray<int> sort;
	sort.sort(t.points, 3);

	for (int i = 0; i < triangles.size(); i++) {
		bool all_equal = true;
		for (int j = 0; j < 3; j++) {
			if (triangles[i].points[j] != t.points[j]) {
				all_equal = false;
				break;
			}
		}
		ERR_FAIL_COND(all_equal);
	}

	if (p_at_index == -1 || p_at_index == triangles.size()) {
		triangles.push_back(t);
	}
	else {
		triangles.insert(p_at_index, t);
	}
}

int AnimationNodeBlendSpace::get_triangle_point(int p_triangle, int p_point)
{
	_update_triangles();

	ERR_FAIL_INDEX_V(p_point, 3, -1);
	ERR_FAIL_INDEX_V(p_triangle, triangles.size(), -1);
	return triangles[p_triangle].points[p_point];
}

void AnimationNodeBlendSpace::remove_triangle(int p_triangle)
{
	ERR_FAIL_INDEX(p_triangle, triangles.size());
	triangles.remove_at(p_triangle);
}

int AnimationNodeBlendSpace::get_triangle_count() const { return triangles.size(); }

void AnimationNodeBlendSpace::set_min_space(float p_min)
{
	min_space.x = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
}

void AnimationNodeBlendSpace::set_min_space(const Vector2& p_min)
{
	min_space = p_min;
	if (min_space.x >= max_space.x) {
		min_space.x = max_space.x - 1;
	}
	if (min_space.y >= max_space.y) {
		min_space.y = max_space.y - 1;
	}
}

Vector2 AnimationNodeBlendSpace::get_min_space() const { return min_space; }

float AnimationNodeBlendSpace::get_min_space_1d() const { return min_space.x; }

void AnimationNodeBlendSpace::set_max_space(float p_max)
{
	max_space.x = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
}

void AnimationNodeBlendSpace::set_max_space(const Vector2& p_max)
{
	max_space = p_max;
	if (max_space.x <= min_space.x) {
		max_space.x = min_space.x + 1;
	}
	if (max_space.y <= min_space.y) {
		max_space.y = min_space.y + 1;
	}
}

Vector2 AnimationNodeBlendSpace::get_max_space() const { return max_space; }

float AnimationNodeBlendSpace::get_max_space_1d() const { return max_space.x; }

void AnimationNodeBlendSpace::set_snap(float p_snap) { snap.x = p_snap; }

void AnimationNodeBlendSpace::set_snap(const Vector2& p_snap) { snap = p_snap; }

Vector2 AnimationNodeBlendSpace::get_snap() const { return snap; }

float AnimationNodeBlendSpace::get_snap_1d() const { return snap.x; }

void AnimationNodeBlendSpace::set_value_label(const String& p_label) { value_label = p_label; }

String AnimationNodeBlendSpace::get_value_label() const { return value_label; }

void AnimationNodeBlendSpace::set_x_label(const String& p_label) { x_label = p_label; }

String AnimationNodeBlendSpace::get_x_label() const { return x_label; }

void AnimationNodeBlendSpace::set_y_label(const String& p_label) { y_label = p_label; }

String AnimationNodeBlendSpace::get_y_label() const { return y_label; }

void AnimationNodeBlendSpace::_set_triangles(const Vector<int>& p_triangles)
{
	triangles.clear();
	if (p_triangles.size() % 3 != 0) {
		return;
	}
	for (int i = 0; i < p_triangles.size(); i += 3) {
		add_triangle(p_triangles[i + 0], p_triangles[i + 1], p_triangles[i + 2]);
	}
}

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::_retry_set_triangles(const Vector<int>& p_triangles)
{
	for (int i = 0; i < p_triangles.size(); i += 3) {
		int x = p_triangles[i + 0];
		int y = p_triangles[i + 1];
		int z = p_triangles[i + 2];
		if (x >= blend_points_used || y >= blend_points_used || z >= blend_points_used ||
			has_triangle(x, y, z)) {
			continue;
		}
		add_triangle(x, y, z);
	}
}
#endif // DISABLE_DEPRECATED

Vector<int> AnimationNodeBlendSpace::_get_triangles() const
{
	Vector<int> t;
	if (auto_triangles && triangles_dirty) {
		return t;
	}

	t.resize(triangles.size() * 3);
	for (int i = 0; i < triangles.size(); i++) {
		t.write[i * 3 + 0] = triangles[i].points[0];
		t.write[i * 3 + 1] = triangles[i].points[1];
		t.write[i * 3 + 2] = triangles[i].points[2];
	}
	return t;
}

Vector2 AnimationNodeBlendSpace::get_closest_point(const Vector2& p_point)
{
	_update_triangles();

	if (triangles.is_empty()) {
		return Vector2();
	}

	Vector2 best_point;
	bool first = true;

	for (int i = 0; i < triangles.size(); i++) {
		Vector2 points[3];
		for (int j = 0; j < 3; j++) {
			points[j] = get_blend_point_position(get_triangle_point(i, j));
		}

		if (Geometry2D::is_point_in_triangle(p_point, points[0], points[1], points[2])) {
			return p_point;
		}

		for (int j = 0; j < 3; j++) {
			const Vector2 segment_a = points[j];
			const Vector2 segment_b = points[(j + 1) % 3];
			Vector2 closest_point =
				Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
			if (first || closest_point.distance_to(p_point) < best_point.distance_to(p_point)) {
				best_point = closest_point;
				first = false;
			}
		}
	}

	return best_point;
}

void AnimationNodeBlendSpace::_blend_triangle(
	const Vector2& p_pos, const LocalVector<Vector2>& p_points, LocalVector<float>& r_weights)
{
	if (p_pos.is_equal_approx(p_points[0])) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.is_equal_approx(p_points[1])) {
		r_weights[0] = 0;
		r_weights[1] = 1;
		r_weights[2] = 0;
		return;
	}
	if (p_pos.is_equal_approx(p_points[2])) {
		r_weights[0] = 0;
		r_weights[1] = 0;
		r_weights[2] = 1;
		return;
	}

	Vector2 v0 = p_points[1] - p_points[0];
	Vector2 v1 = p_points[2] - p_points[0];
	Vector2 v2 = p_pos - p_points[0];

	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float d20 = v2.dot(v0);
	float d21 = v2.dot(v1);
	float denom = (d00 * d11 - d01 * d01);
	if (denom == 0) {
		r_weights[0] = 1;
		r_weights[1] = 0;
		r_weights[2] = 0;
		return;
	}
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	r_weights[0] = u;
	r_weights[1] = v;
	r_weights[2] = w;
}

void AnimationNodeBlendSpace::_check_can_sync()
{
	is_contain_invalid_point = false;
	if (sync_mode < SYNC_MODE_CYCLIC_MUTABLE) {
		return;
	}
	for (int i = 0; i < blend_points_used; i++) {
		Ref<AnimationNodeBlendNode> na = blend_points[i].node;
		if (na.is_null() || na->get_node_type() != AnimationNodeBlendNode::NODE_ANIMATION) {
			is_contain_invalid_point = true;
			break;
		}
	}
}

String AnimationNodeBlendSpace::get_caption() const
{
	return dimension == DIMENSION_2D ? "BlendSpace2D" : "BlendSpace1D";
}

void AnimationNodeBlendSpace::set_auto_triangles(bool p_enable)
{
	if (auto_triangles == p_enable) {
		return;
	}

	auto_triangles = p_enable;
	_queue_auto_triangles();
}

bool AnimationNodeBlendSpace::get_auto_triangles() const { return auto_triangles; }

void AnimationNodeBlendSpace::set_blend_mode(BlendMode p_blend_mode)
{
	blend_mode = p_blend_mode;
}

AnimationNodeBlendSpace::BlendMode AnimationNodeBlendSpace::get_blend_mode() const
{
	return blend_mode;
}

#ifndef DISABLE_DEPRECATED
void AnimationNodeBlendSpace::set_use_sync(bool p_sync)
{
	sync_mode = p_sync ? SYNC_MODE_INDEPENDENT : SYNC_MODE_NONE;
}

bool AnimationNodeBlendSpace::is_using_sync() const { return sync_mode != SYNC_MODE_NONE; }
#endif // DISABLE_DEPRECATED

void AnimationNodeBlendSpace::set_sync_mode(SyncMode p_sync_mode)
{
	sync_mode = p_sync_mode;
	_check_can_sync();
}

AnimationNodeBlendSpace::SyncMode AnimationNodeBlendSpace::get_sync_mode() const
{
	return sync_mode;
}

void AnimationNodeBlendSpace::set_cyclic_length(double p_length)
{
	cyclic_length = p_length;
	inverted_cycle_length = (p_length > CMP_EPSILON) ? (1.0 / p_length) : 0.0;
}

double AnimationNodeBlendSpace::get_cyclic_length() const { return cyclic_length; }

AnimationNode::NodeTimeInfo AnimationNodeBlendSpace::_process(
	AnimationNode::ProcessState& r_process_state, AnimationNodeInstance& p_instance,
	const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only)
{
	return AnimationNode::NodeTimeInfo();
}

void AnimationNodeBlendSpace::_update_triangles() {}

void AnimationNodeBlendSpace::_queue_auto_triangles() {}

// ==========================================================================
// Merged from scene/animation/animation_blend_tree.cpp
// ==========================================================================

#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "scene/resources/animation.h"

LocalVector<String> (*AnimationNodeBlendNode::get_editable_animation_list)() = nullptr;

void AnimationNodeBlendNode::validate_node(
	const AnimationTree* p_tree, const String& p_path) const
{
	AnimationNode::validate_node(p_tree, p_path);

	if (node_type == NODE_ANIMATION) {
		const Ref<Animation>& animation_resource = p_tree->get_animation_or_null(animation);
		if (animation_resource.is_null()) {
			add_validation_error(
				p_tree, p_path, vformat(RTR("Animation '%s' not found."), animation));
		}
	}
}

bool AnimationNodeBlendNode::is_parameter_read_only(const String& p_parameter) const
{
	if (AnimationNode::is_parameter_read_only(p_parameter)) {
		return true;
	}

	if (node_type == NODE_ONESHOT) {
		return p_parameter == active || p_parameter == internal_active;
	}
	if (node_type == NODE_TRANSITION) {
		return p_parameter == current_state || p_parameter == current_index;
	}
	return false;
}

String AnimationNodeBlendNode::get_caption() const
{
	switch (node_type) {
	case NODE_ANIMATION:
		return "Animation";
	case NODE_ONESHOT:
		return "OneShot";
	case NODE_ADD:
		return three_way ? "Add3" : "Add2";
	case NODE_BLEND:
		return three_way ? "Blend3" : "Blend2";
	case NODE_SUB:
		return "Sub2";
	case NODE_TIME_SCALE:
		return "TimeScale";
	case NODE_TIME_SEEK:
		return "TimeSeek";
	case NODE_TRANSITION:
		return "Transition";
	case NODE_OUTPUT:
		return "Output";
	}
	return "Node";
}

bool AnimationNodeBlendNode::has_filter() const
{
	switch (node_type) {
	case NODE_ONESHOT:
	case NODE_ADD:
	case NODE_SUB:
		return true;
	case NODE_BLEND:
		return !three_way;
	case NODE_ANIMATION:
	case NODE_TIME_SCALE:
	case NODE_TIME_SEEK:
	case NODE_TRANSITION:
	case NODE_OUTPUT:
		return false;
	}
	return false;
}

AnimationNode::NodeTimeInfo AnimationNodeBlendNode::_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	if (node_type == NODE_OUTPUT) {
		AnimationMixer::PlaybackInfo pi = p_playback_info;
		pi.weight = 1.0;
		return blend_input(p_process_state, p_instance, 0, pi, FILTER_IGNORE, true, p_test_only);
	}
	return AnimationNode::NodeTimeInfo();
}

void AnimationNodeBlendNode::set_animation(const String& p_name)
{
	animation = p_name;
	animation_version++;
	if (unlikely(animation_version == 0)) {
		animation_version = 1;
	}
	emit_changed();
}

String AnimationNodeBlendNode::get_animation() const { return animation; }

void AnimationNodeBlendNode::set_play_mode(PlayMode p_play_mode) { play_mode = p_play_mode; }

AnimationNodeBlendNode::PlayMode AnimationNodeBlendNode::get_play_mode() const
{
	return play_mode;
}

void AnimationNodeBlendNode::set_backward(
	AnimationNodeInstance& p_instance, ProcessState& p_process_state, bool p_backward)
{
}

bool AnimationNodeBlendNode::is_backward(
	AnimationNodeInstance& p_instance, ProcessState& p_process_state) const
{
	return false;
}

void AnimationNodeBlendNode::set_advance_on_start(bool p_advance_on_start)
{
	advance_on_start = p_advance_on_start;
}

bool AnimationNodeBlendNode::is_advance_on_start() const { return advance_on_start; }

void AnimationNodeBlendNode::set_use_custom_timeline(bool p_use_custom_timeline)
{
	use_custom_timeline = p_use_custom_timeline;
}

bool AnimationNodeBlendNode::is_using_custom_timeline() const { return use_custom_timeline; }

void AnimationNodeBlendNode::set_timeline_length(double p_length) { timeline_length = p_length; }

double AnimationNodeBlendNode::get_timeline_length() const { return timeline_length; }

void AnimationNodeBlendNode::set_stretch_time_scale(bool p_stretch_time_scale)
{
	stretch_time_scale = p_stretch_time_scale;
}

bool AnimationNodeBlendNode::is_stretching_time_scale() const { return stretch_time_scale; }

void AnimationNodeBlendNode::set_start_offset(double p_offset) { start_offset = p_offset; }

double AnimationNodeBlendNode::get_start_offset() const { return start_offset; }

void AnimationNodeBlendNode::set_loop_mode(Animation::LoopMode p_loop_mode)
{
	loop_mode = p_loop_mode;
}

Animation::LoopMode AnimationNodeBlendNode::get_loop_mode() const { return loop_mode; }

void AnimationNodeBlendNode::_update_animation_cache(
	AnimationTree* p_tree, AnimationNodeInstance& p_instance) const
{
	if (p_instance.cached_animation_version == animation_version) {
		return;
	}

	const Ref<Animation>& anim = p_tree->get_animation_or_null(animation);
	if (anim.is_null()) {
		return;
	}

	p_instance.cached_animation = anim;
	p_instance.cached_animation_version = animation_version;
}

void AnimationNodeBlendNode::set_fade_in_time(double p_time) { fade_in = p_time; }

double AnimationNodeBlendNode::get_fade_in_time() const { return fade_in; }

void AnimationNodeBlendNode::set_fade_out_time(double p_time) { fade_out = p_time; }

double AnimationNodeBlendNode::get_fade_out_time() const { return fade_out; }

void AnimationNodeBlendNode::set_fade_in_curve(const Ref<Curve>& p_curve)
{
	fade_in_curve = p_curve;
}

Ref<Curve> AnimationNodeBlendNode::get_fade_in_curve() const { return fade_in_curve; }

void AnimationNodeBlendNode::set_fade_out_curve(const Ref<Curve>& p_curve)
{
	fade_out_curve = p_curve;
}

Ref<Curve> AnimationNodeBlendNode::get_fade_out_curve() const { return fade_out_curve; }

void AnimationNodeBlendNode::set_auto_restart_enabled(bool p_enabled) { auto_restart = p_enabled; }

void AnimationNodeBlendNode::set_auto_restart_delay(double p_time) { auto_restart_delay = p_time; }

void AnimationNodeBlendNode::set_auto_restart_random_delay(double p_time)
{
	auto_restart_random_delay = p_time;
}

bool AnimationNodeBlendNode::is_auto_restart_enabled() const { return auto_restart; }

double AnimationNodeBlendNode::get_auto_restart_delay() const { return auto_restart_delay; }

double AnimationNodeBlendNode::get_auto_restart_random_delay() const
{
	return auto_restart_random_delay;
}

void AnimationNodeBlendNode::set_mix_mode(MixMode p_mix) { mix = p_mix; }

AnimationNodeBlendNode::MixMode AnimationNodeBlendNode::get_mix_mode() const { return mix; }

void AnimationNodeBlendNode::set_break_loop_at_end(bool p_enable) { break_loop_at_end = p_enable; }

bool AnimationNodeBlendNode::is_loop_broken_at_end() const { return break_loop_at_end; }

void AnimationNodeBlendNode::set_abort_on_reset(bool p_enable) { abort_on_reset = p_enable; }

bool AnimationNodeBlendNode::is_aborted_on_reset() const { return abort_on_reset; }

void AnimationNodeBlendNode::set_explicit_elapse(bool p_enable) { explicit_elapse = p_enable; }

bool AnimationNodeBlendNode::is_explicit_elapse() const { return explicit_elapse; }

void AnimationNodeBlendNode::set_input_count(int p_inputs)
{
	while (get_input_count() < p_inputs) {
		add_input("state_" + itos(get_input_count()));
	}
	while (get_input_count() > p_inputs) {
		remove_input(get_input_count() - 1);
	}
}

bool AnimationNodeBlendNode::add_input(const String& p_name)
{
	if (AnimationNode::add_input(p_name)) {
		if (node_type == NODE_TRANSITION) {
			input_data.push_back(InputData());
		}
		return true;
	}
	return false;
}

void AnimationNodeBlendNode::remove_input(int p_index)
{
	if (node_type == NODE_TRANSITION) {
		input_data.remove_at(p_index);
	}
	AnimationNode::remove_input(p_index);
}

bool AnimationNodeBlendNode::set_input_name(int p_input, const String& p_name)
{
	if (node_type == NODE_TRANSITION) {
		return false;
	}
	return AnimationNode::set_input_name(p_input, p_name);
}

void AnimationNodeBlendNode::set_input_as_auto_advance(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, (int)input_data.size());
	input_data[p_input].auto_advance = p_enable;
}

bool AnimationNodeBlendNode::is_input_set_as_auto_advance(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int)input_data.size(), false);
	return input_data[p_input].auto_advance;
}

void AnimationNodeBlendNode::set_input_break_loop_at_end(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, (int)input_data.size());
	input_data[p_input].break_loop_at_end = p_enable;
}

bool AnimationNodeBlendNode::is_input_loop_broken_at_end(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int)input_data.size(), false);
	return input_data[p_input].break_loop_at_end;
}

void AnimationNodeBlendNode::set_input_reset(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, (int)input_data.size());
	input_data[p_input].reset = p_enable;
}

bool AnimationNodeBlendNode::is_input_reset(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, (int)input_data.size(), true);
	return input_data[p_input].reset;
}

void AnimationNodeBlendNode::set_xfade_time(double p_fade) { xfade_time = p_fade; }

double AnimationNodeBlendNode::get_xfade_time() const { return xfade_time; }

void AnimationNodeBlendNode::set_xfade_curve(const Ref<Curve>& p_curve) { xfade_curve = p_curve; }

Ref<Curve> AnimationNodeBlendNode::get_xfade_curve() const { return xfade_curve; }

void AnimationNodeBlendNode::set_allow_transition_to_self(bool p_enable)
{
	allow_transition_to_self = p_enable;
}

bool AnimationNodeBlendNode::is_allow_transition_to_self() const
{
	return allow_transition_to_self;
}

AnimationNodeBlendNode::AnimationNodeBlendNode(NodeType p_type, bool p_three_way)
	: node_type(p_type), three_way(p_three_way)
{
	switch (node_type) {
	case NODE_ANIMATION: {
		root_node = true;
	} break;
	case NODE_ONESHOT: {
		add_input("in");
		add_input("shot");
	} break;
	case NODE_ADD: {
		if (three_way) {
			add_input("-add");
			add_input("in");
			add_input("+add");
		}
		else {
			add_input("in");
			add_input("add");
		}
	} break;
	case NODE_BLEND: {
		if (three_way) {
			add_input("-blend");
			add_input("in");
			add_input("+blend");
		}
		else {
			add_input("in");
			add_input("blend");
		}
	} break;
	case NODE_SUB: {
		add_input("in");
		add_input("sub");
	} break;
	case NODE_TIME_SCALE:
	case NODE_TIME_SEEK: {
		add_input("in");
	} break;
	case NODE_TRANSITION:
		break;
	case NODE_OUTPUT: {
		add_input("output");
	} break;
	}
}

Ref<AnimationNode> AnimationNodeBlendTree::get_node(const String& p_name) const
{
	const Node* node = nodes.getptr(p_name);
	ERR_FAIL_NULL_V(node, Ref<AnimationNode>());
	return node->node;
}

void AnimationNodeBlendTree::set_node_position(const String& p_node, const Vector2& p_position)
{
	ERR_FAIL_COND(!nodes.has(p_node));
	nodes[p_node].position = p_position;
}

Vector2 AnimationNodeBlendTree::get_node_position(const String& p_node) const
{
	ERR_FAIL_COND_V(!nodes.has(p_node), Vector2());
	return nodes[p_node].position;
}

void AnimationNodeBlendTree::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	for (const KeyValue<String, Node>& E : nodes) {
		ChildNode cn;
		cn.name = E.key;
		cn.node = E.value.node;
		r_child_nodes->push_back(cn);
	}
}

bool AnimationNodeBlendTree::has_node(const String& p_name) const { return nodes.has(p_name); }

const LocalVector<String>* AnimationNodeBlendTree::get_node_connection_array(
	const String& p_name) const
{
	const Node* node = nodes.getptr(p_name);
	ERR_FAIL_NULL_V(node, nullptr);
	return &node->connections;
}

AnimationNodeBlendTree::ConnectionError AnimationNodeBlendTree::can_connect_node(
	const String& p_input_node, int p_input_index, const String& p_output_node) const
{
	if (!nodes.has(p_output_node) || p_output_node == SceneString(output)) {
		return CONNECTION_ERROR_NO_OUTPUT;
	}

	if (!nodes.has(p_input_node)) {
		return CONNECTION_ERROR_NO_INPUT;
	}

	if (p_input_node == p_output_node) {
		return CONNECTION_ERROR_SAME_NODE;
	}

	Ref<AnimationNode> input = nodes[p_input_node].node;

	if (p_input_index < 0 || p_input_index >= (int)nodes[p_input_node].connections.size()) {
		return CONNECTION_ERROR_NO_INPUT_INDEX;
	}

	if (nodes[p_input_node].connections[p_input_index] != String()) {
		return CONNECTION_ERROR_CONNECTION_EXISTS;
	}

	for (const KeyValue<String, Node>& E : nodes) {
		for (uint32_t i = 0; i < E.value.connections.size(); i++) {
			const String output = E.value.connections[i];
			if (output == p_output_node) {
				return CONNECTION_ERROR_CONNECTION_EXISTS;
			}
		}
	}
	return CONNECTION_OK;
}

void AnimationNodeBlendTree::get_node_connections(LocalVector<NodeConnection>* r_connections) const
{
	for (const KeyValue<String, Node>& E : nodes) {
		for (uint32_t i = 0; i < E.value.connections.size(); i++) {
			const String output = E.value.connections[i];
			if (output != String()) {
				NodeConnection nc;
				nc.input_node = E.key;
				nc.input_index = i;
				nc.output_node = output;
				r_connections->push_back(nc);
			}
		}
	}
}

String AnimationNodeBlendTree::get_caption() const { return "BlendTree"; }

AnimationNode::NodeTimeInfo AnimationNodeBlendTree::_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	const Ref<AnimationNodeBlendNode> output = nodes[SceneString(output)].node;
	ERR_FAIL_COND_V(
		output.is_null() || output->get_node_type() != AnimationNodeBlendNode::NODE_OUTPUT,
		NodeTimeInfo());

	AnimationMixer::PlaybackInfo pi = p_playback_info;
	pi.weight = 1.0;

	AnimationNodeInstance& output_instance =
		p_instance.get_child_instance_by_path(SceneString(output));
	return _blend_node(p_process_state, p_instance, output_instance, pi, FILTER_IGNORE, true,
		p_test_only, nullptr);
}

LocalVector<String> AnimationNodeBlendTree::get_node_list() const
{
	LocalVector<String> list;
	list.reserve(nodes.size());
	for (const KeyValue<String, Node>& E : nodes) {
		list.push_back(E.key);
	}
	list.sort();
	return list;
}

void AnimationNodeBlendTree::set_graph_offset(const Vector2& p_graph_offset)
{
	graph_offset = p_graph_offset;
}

Vector2 AnimationNodeBlendTree::get_graph_offset() const { return graph_offset; }

Ref<AnimationNode> AnimationNodeBlendTree::get_child_by_name(const String& p_name) const
{
	return get_node(p_name);
}

void AnimationNodeBlendTree::_tree_changed() { AnimationNode::_tree_changed(); }

void AnimationNodeBlendTree::validate_node(
	const AnimationTree* p_tree, const String& p_path) const
{
	AnimationNode::validate_node(p_tree, p_path);

	// Validate output connection.
	{
		const LocalVector<String>* output_connections =
			get_node_connection_array(SceneString(output));
		const String& node_name = output_connections->operator[](0);

		if (const Node* child_node = nodes.getptr(node_name); !child_node) {
			add_validation_error(p_tree, String(p_path) + SceneString(output) + "/",
				RTR("Nothing connected to output."));
		}
	}

	// Rest of children.
	for (const KeyValue<String, Node>& E : nodes) {
		const Node& child = E.value;

		// Skip output node, already validated.
		if (E.key == SceneString(output)) {
			continue;
		}

		for (uint32_t input = 0; input < child.connections.size(); input++) {
			const String& connected_node_name = child.connections[input];
			if (const Node* connected_to = nodes.getptr(connected_node_name); !connected_to) {
				String path = String(p_path) + String(E.key) + "/";
				add_validation_error(p_tree, path, "Nothing connected to", input);
			}
		}
	}
}

#ifdef TOOLS_ENABLED
void AnimationNodeBlendTree::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	bool add_node_options = false;
	if (p_idx == 0) {
		add_node_options =
			(pf == "get_node" || pf == "has_node" || pf == "rename_node" || pf == "remove_node" ||
				pf == "set_node_position" || pf == "get_node_position" || pf == "connect_node" ||
				pf == "disconnect_node");
	}
	else if (p_idx == 2) {
		add_node_options = (pf == "connect_node" || pf == "disconnect_node");
	}
	if (add_node_options) {
		for (const KeyValue<String, Node>& E : nodes) {
			r_options->push_back(String(E.key).quote());
		}
	}
	AnimationNode::get_argument_options(p_function, p_idx, r_options);
}
#endif

void AnimationNodeBlendTree::_initialize_node_tree()
{
	Ref<AnimationNodeBlendNode> output;
	output.instantiate(AnimationNodeBlendNode::NODE_OUTPUT);
	Node n;
	n.node = output;
	n.position = Vector2(300, 150);
	n.connections.resize(1);
	nodes[SceneString(output)] = std::move(n);
}

AnimationNodeBlendTree::AnimationNodeBlendTree()
{
	root_node = true;
	_initialize_node_tree();
}

void AnimationNodeBlendTree::reset_state() {}

// ==========================================================================
// Merged from scene/animation/animation_node_state_machine.cpp
// ==========================================================================
// --- Inlined from scene/animation/animation_node_state_machine.h ---
#include "core/math/expression.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/curve.h"

class AnimationNodeStateMachineTransition : public Resource
{
public:
	enum SwitchMode
	{
		SWITCH_MODE_IMMEDIATE,
		SWITCH_MODE_SYNC,
		SWITCH_MODE_AT_END,
	};

	enum AdvanceMode
	{
		ADVANCE_MODE_DISABLED,
		ADVANCE_MODE_ENABLED,
		ADVANCE_MODE_AUTO,
	};

private:
	SwitchMode switch_mode = SWITCH_MODE_IMMEDIATE;
	AdvanceMode advance_mode = ADVANCE_MODE_ENABLED;
	String advance_condition;
	String advance_condition_name;
	float xfade_time = 0.0;
	Ref<Curve> xfade_curve;
	bool break_loop_at_end = false;
	bool reset = true;
	int priority = 1;
	String advance_expression;

	friend class AnimationNodeStateMachinePlayback;
	Ref<Expression> expression;

protected:

public:
	void set_switch_mode(SwitchMode p_mode);
	SwitchMode get_switch_mode() const;

	void set_advance_mode(AdvanceMode p_mode);
	AdvanceMode get_advance_mode() const;

	void set_advance_condition(const String& p_condition);
	String get_advance_condition() const;

	String get_advance_condition_name() const;

	void set_advance_expression(const String& p_expression);
	String get_advance_expression() const;

	void set_xfade_time(float p_xfade);
	float get_xfade_time() const;

	void set_break_loop_at_end(bool p_enable);
	bool is_loop_broken_at_end() const;

	void set_reset(bool p_reset);
	bool is_reset() const;

	void set_xfade_curve(const Ref<Curve>& p_curve);
	Ref<Curve> get_xfade_curve() const;

	void set_priority(int p_priority);
	int get_priority() const;

	AnimationNodeStateMachineTransition();
};

class AnimationNodeStateMachinePlayback;

class AnimationNodeStateMachine final : public AnimationNode
{
public:
	enum StateMachineType
	{
		STATE_MACHINE_TYPE_ROOT,
		STATE_MACHINE_TYPE_NESTED,
		STATE_MACHINE_TYPE_GROUPED,
	};

private:
	friend class AnimationNodeStateMachinePlayback;

	StateMachineType state_machine_type = STATE_MACHINE_TYPE_ROOT;

	struct State
	{
		Ref<AnimationNode> node;
		Vector2 position;
	};

	HashMap<String, State> states;
	bool allow_transition_to_self = false;
	bool reset_ends = false;

	struct Transition
	{
		String from;
		String to;
		Ref<AnimationNodeStateMachineTransition> transition;
	};

	Vector<Transition> transitions;

	String playback = "playback";
	bool updating_transitions = false;

	Vector2 graph_offset;

	void _remove_transition(const Ref<AnimationNodeStateMachineTransition> p_transition);
	void _rename_transitions(const String& p_name, const String& p_new_name);
	bool _can_connect(const String& p_name);

protected:
	bool _check_advance_condition(const Ref<AnimationNodeStateMachine> p_state_machine,
		const Ref<AnimationNodeStateMachineTransition> p_transition) const;

	virtual void _tree_changed() override;

	virtual void reset_state() override;

public:
	virtual void validate_node(
		const AnimationTree* p_tree, const String& p_path) const override;

	virtual bool is_parameter_read_only(const String& p_parameter) const override;

	void add_node(
		const String& p_name, Ref<AnimationNode> p_node, const Vector2& p_position = Vector2());
	void replace_node(const String& p_name, Ref<AnimationNode> p_node);
	Ref<AnimationNode> get_node(const String& p_name) const;
	void remove_node(const String& p_name);
	void rename_node(const String& p_name, const String& p_new_name);
	bool has_node(const String& p_name) const;
	String get_node_name(const Ref<AnimationNode>& p_node) const;
	LocalVector<String> get_node_list() const;

	void set_node_position(const String& p_name, const Vector2& p_position);
	Vector2 get_node_position(const String& p_name) const;

	virtual void get_child_nodes(LocalVector<ChildNode>* r_child_nodes) override;

	bool has_transition(const String& p_from, const String& p_to) const;
	bool has_transition_from(const String& p_from) const;
	bool has_transition_to(const String& p_to) const;
	int find_transition(const String& p_from, const String& p_to) const;
	Vector<int> find_transition_from(const String& p_from) const;
	Vector<int> find_transition_to(const String& p_to) const;
	void add_transition(const String& p_from, const String& p_to,
		const Ref<AnimationNodeStateMachineTransition>& p_transition);
	Ref<AnimationNodeStateMachineTransition> get_transition(int p_transition) const;
	String get_transition_from(int p_transition) const;
	String get_transition_to(int p_transition) const;
	int get_transition_count() const;
	bool is_transition_across_group(int p_transition) const;
	void remove_transition_by_index(const int p_transition);
	void remove_transition(const String& p_from, const String& p_to);

	void set_state_machine_type(StateMachineType p_state_machine_type);
	StateMachineType get_state_machine_type() const;

	void set_allow_transition_to_self(bool p_enable);
	bool is_allow_transition_to_self() const;

	void set_reset_ends(bool p_enable);
	bool are_ends_reset() const;

	bool can_edit_node(const String& p_name) const;

	void set_graph_offset(const Vector2& p_offset);
	Vector2 get_graph_offset() const;

	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false) override;
	virtual String get_caption() const override;

	virtual Ref<AnimationNode> get_child_by_name(const String& p_name) const override;

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const override;
#endif

	Vector<String> get_nodes_with_transitions_from(const String& p_node) const;
	Vector<String> get_nodes_with_transitions_to(const String& p_node) const;

	AnimationNodeStateMachine();
};

class AnimationNodeStateMachinePlayback : public Resource
{
	friend class AnimationNodeStateMachine;

	struct AStarCost
	{
		float distance = 0.0;
		String prev;
	};

	struct TransitionInfo
	{
		String from;
		String to;
		String next;
	};

	struct NextInfo
	{
		String node;
		double xfade;
		Ref<Curve> curve;
		AnimationNodeStateMachineTransition::SwitchMode switch_mode;
		bool is_reset;
		bool break_loop_at_end;
	};

	struct ChildStateMachineInfo
	{
		Ref<AnimationNodeStateMachinePlayback> playback;
		Vector<String> path;
		bool is_reset = false;
	};

	Ref<AnimationNodeStateMachineTransition> default_transition;
	String base_path;

	AnimationNode::NodeTimeInfo current_nti;
	String current = SceneString(Start);
	Ref<Curve> current_curve;

	Ref<AnimationNodeStateMachineTransition> group_start_transition;
	Ref<AnimationNodeStateMachineTransition> group_end_transition;

	AnimationNode::NodeTimeInfo fadeing_from_nti;
	String fading_from;
	float fading_time = 0.0;
	float fading_pos = 0.0;

	Vector<String> path;
	bool playing = false;

	String start_request;
	String travel_request;
	bool reset_request = false;
	bool reset_request_on_teleport = false;
	bool _reset_request_for_fading_from = false;
	bool next_request = false;
	bool stop_request = false;
	bool teleport_request = false;

	bool is_grouped = false;

	void _clear_fading(AnimationNode::ProcessState& p_process_state,
		AnimationNodeStateMachine* p_state_machine, const String& p_state);
	void _signal_state_change(
		AnimationTree* p_animation_tree, const String& p_state, bool p_started);
	void _travel_main(const String& p_state, bool p_reset_on_teleport = true);
	void _start_main(const String& p_state, bool p_reset = true);
	void _next_main();
	void _stop_main();

	bool _make_travel_path(AnimationNode::ProcessState& p_process_state, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, bool p_is_allow_transition_to_self,
		Vector<String>& r_path, bool p_test_only);
	String _validate_path(AnimationNodeStateMachine* p_state_machine, const String& p_path);
	bool _travel(AnimationNode::ProcessState& p_process_state, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, bool p_is_allow_transition_to_self,
		bool p_test_only);
	void _start(
		AnimationNode::ProcessState& p_process_state, AnimationNodeStateMachine* p_state_machine);

	void _clear_path_children(AnimationNode::ProcessState& p_process_state, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, bool p_test_only);
	void _start_children(AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine,
		const String& p_path, bool p_test_only);

	AnimationNode::NodeTimeInfo process(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationNodeStateMachine* p_state_machine,
		const AnimationMixer::PlaybackInfo p_playback_info, bool p_test_only);
	AnimationNode::NodeTimeInfo _process(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationNodeStateMachine* p_state_machine,
		const AnimationMixer::PlaybackInfo p_playback_info, bool p_test_only);

	bool _transition_to_next_recursive(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine, double p_delta, bool p_test_only);
	NextInfo _find_next(AnimationNode::ProcessState& p_process_state,
		AnimationNodeInstance& p_instance, AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine) const;
	Ref<AnimationNodeStateMachineTransition> _check_group_transition(AnimationTree* p_tree,
		AnimationNodeStateMachine* p_state_machine,
		const AnimationNodeStateMachine::Transition& p_transition,
		Ref<AnimationNodeStateMachine>& r_state_machine, bool& r_bypass) const;

	void _set_current(AnimationNode::ProcessState& p_process_state,
		AnimationNodeStateMachine* p_state_machine, const String& p_state);
	void _set_grouped(bool p_is_grouped);
	void _set_base_path(const String& p_base_path);
	Ref<AnimationNodeStateMachinePlayback> _get_parent_playback(AnimationTree* p_tree) const;
	Ref<AnimationNodeStateMachine> _get_parent_state_machine(AnimationTree* p_tree) const;
	Ref<AnimationNodeStateMachineTransition> _get_group_start_transition() const;
	Ref<AnimationNodeStateMachineTransition> _get_group_end_transition() const;

protected:

public:
	void travel(const String& p_state, bool p_reset_on_teleport = true);
	void start(const String& p_state, bool p_reset = true);
	void next();
	void stop();
	bool is_playing() const;
	bool is_end() const;
	String get_current_node() const;
	String get_fading_from_node() const;
	Vector<String> get_travel_path() const;
	float get_current_play_pos() const;
	float get_current_length() const;

	float get_fading_from_play_pos() const;
	float get_fading_from_length() const;

	float get_fading_time() const;
	float get_fading_pos() const;

	void clear_path();
	void push_path(const String& p_state);

	AnimationNodeStateMachinePlayback();
};



// --- End scene/animation/animation_node_state_machine.h ---
#include "core/config/engine.h"

/////////////////////////////////////////////////

void AnimationNodeStateMachineTransition::set_switch_mode(SwitchMode p_mode)
{
	switch_mode = p_mode;
}

AnimationNodeStateMachineTransition::SwitchMode
AnimationNodeStateMachineTransition::get_switch_mode() const
{
	return switch_mode;
}

void AnimationNodeStateMachineTransition::set_advance_mode(AdvanceMode p_mode)
{
	advance_mode = p_mode;
}

AnimationNodeStateMachineTransition::AdvanceMode
AnimationNodeStateMachineTransition::get_advance_mode() const
{
	return advance_mode;
}

String AnimationNodeStateMachineTransition::get_advance_condition() const
{
	return advance_condition;
}

String AnimationNodeStateMachineTransition::get_advance_condition_name() const
{
	return advance_condition_name;
}

void AnimationNodeStateMachineTransition::set_advance_expression(const String& p_expression)
{
	advance_expression = p_expression;

	String advance_expression_stripped = advance_expression.strip_edges();
	if (advance_expression_stripped == String()) {
		expression.unref();
		return;
	}

	if (expression.is_null()) {
		expression.instantiate();
	}

	expression->parse(advance_expression_stripped);
}

String AnimationNodeStateMachineTransition::get_advance_expression() const
{
	return advance_expression;
}

void AnimationNodeStateMachineTransition::set_xfade_time(float p_xfade)
{
	ERR_FAIL_COND(p_xfade < 0);
	xfade_time = p_xfade;
	emit_changed();
}

float AnimationNodeStateMachineTransition::get_xfade_time() const { return xfade_time; }

void AnimationNodeStateMachineTransition::set_xfade_curve(const Ref<Curve>& p_curve)
{
	xfade_curve = p_curve;
	emit_changed();
}

Ref<Curve> AnimationNodeStateMachineTransition::get_xfade_curve() const { return xfade_curve; }

void AnimationNodeStateMachineTransition::set_break_loop_at_end(bool p_enable)
{
	break_loop_at_end = p_enable;
	emit_changed();
}

bool AnimationNodeStateMachineTransition::is_loop_broken_at_end() const
{
	return break_loop_at_end;
}

void AnimationNodeStateMachineTransition::set_reset(bool p_reset)
{
	reset = p_reset;
	emit_changed();
}

bool AnimationNodeStateMachineTransition::is_reset() const { return reset; }

void AnimationNodeStateMachineTransition::set_priority(int p_priority)
{
	priority = p_priority;
	emit_changed();
}

int AnimationNodeStateMachineTransition::get_priority() const { return priority; }

AnimationNodeStateMachineTransition::AnimationNodeStateMachineTransition() {}

////////////////////////////////////////////////////////

void AnimationNodeStateMachinePlayback::_set_current(AnimationNode::ProcessState& p_process_state,
	AnimationNodeStateMachine* p_state_machine, const String& p_state)
{
	current = p_state;
	if (current == String()) {
		group_start_transition = Ref<AnimationNodeStateMachineTransition>();
		group_end_transition = Ref<AnimationNodeStateMachineTransition>();
		return;
	}

	AnimationTree* tree = p_process_state.tree;
	Ref<AnimationNodeStateMachine> anodesm = p_state_machine->find_node_by_path(current);
	if (anodesm.is_null()) {
		group_start_transition = Ref<AnimationNodeStateMachineTransition>();
		group_end_transition = Ref<AnimationNodeStateMachineTransition>();
		_signal_state_change(tree, current, true);
		return;
	}

	Vector<int> indices = p_state_machine->find_transition_to(current);
	int group_start_size = indices.size();
	if (group_start_size) {
		group_start_transition = p_state_machine->get_transition(indices[0]);
	}
	else {
		group_start_transition = Ref<AnimationNodeStateMachineTransition>();
	}

	indices = p_state_machine->find_transition_from(current);
	int group_end_size = indices.size();
	if (group_end_size) {
		group_end_transition = p_state_machine->get_transition(indices[0]);
	}
	else {
		group_end_transition = Ref<AnimationNodeStateMachineTransition>();
	}

	// Validation.
	if (anodesm->get_state_machine_type() ==
		AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		indices = anodesm->find_transition_from(SceneString(Start));
		int anodesm_start_size = indices.size();
		indices = anodesm->find_transition_to(SceneString(End));
		int anodesm_end_size = indices.size();
		if (group_start_size > 1) {
			WARN_PRINT_ED("There are two or more transitions to the Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + ", which may result in unintended transitions.");
		}
		if (group_end_size > 1) {
			WARN_PRINT_ED("There are two or more transitions from the Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + ", which may result in unintended transitions.");
		}
		if (anodesm_start_size > 1) {
			WARN_PRINT_ED("There are two or more transitions from the Start of Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + current + ", which may result in unintended transitions.");
		}
		if (anodesm_end_size > 1) {
			WARN_PRINT_ED("There are two or more transitions to the End of Grouped "
						  "AnimationNodeStateMachine in AnimationNodeStateMachine: " +
						  base_path + current + ", which may result in unintended transitions.");
		}
		if (anodesm_start_size != group_start_size) {
			ERR_PRINT_ED("There is a mismatch in the number of start transitions in and out of the "
						 "Grouped AnimationNodeStateMachine on AnimationNodeStateMachine: " +
						 base_path + current + ".");
		}
		if (anodesm_end_size != group_end_size) {
			ERR_PRINT_ED("There is a mismatch in the number of end transitions in and out of the "
						 "Grouped AnimationNodeStateMachine on AnimationNodeStateMachine: " +
						 base_path + current + ".");
		}
	}
	else {
		_signal_state_change(tree, current, true);
	}
}

void AnimationNodeStateMachinePlayback::_set_grouped(bool p_is_grouped)
{
	is_grouped = p_is_grouped;
}

void AnimationNodeStateMachinePlayback::travel(const String& p_state, bool p_reset_on_teleport)
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	ERR_FAIL_COND_EDMSG(String(p_state).contains("/Start") || String(p_state).contains("/End"),
		"Grouped AnimationNodeStateMachinePlayback doesn't allow to play Start/End directly. "
		"Instead, play the prev or next state of group in the parent AnimationNodeStateMachine.");
	_travel_main(p_state, p_reset_on_teleport);
}

void AnimationNodeStateMachinePlayback::start(const String& p_state, bool p_reset)
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	ERR_FAIL_COND_EDMSG(String(p_state).contains("/Start") || String(p_state).contains("/End"),
		"Grouped AnimationNodeStateMachinePlayback doesn't allow to play Start/End directly. "
		"Instead, play the prev or next state of group in the parent AnimationNodeStateMachine.");
	_start_main(p_state, p_reset);
}

void AnimationNodeStateMachinePlayback::next()
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	_next_main();
}

void AnimationNodeStateMachinePlayback::stop()
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	_stop_main();
}

void AnimationNodeStateMachinePlayback::_travel_main(
	const String& p_state, bool p_reset_on_teleport)
{
	travel_request = p_state;
	reset_request_on_teleport = p_reset_on_teleport;
	stop_request = false;
}

void AnimationNodeStateMachinePlayback::_start_main(const String& p_state, bool p_reset)
{
	travel_request = String();
	path.clear();
	reset_request = p_reset;
	start_request = p_state;
	stop_request = false;
}

void AnimationNodeStateMachinePlayback::_next_main() { next_request = true; }

void AnimationNodeStateMachinePlayback::_stop_main() { stop_request = true; }

bool AnimationNodeStateMachinePlayback::is_playing() const { return playing; }

bool AnimationNodeStateMachinePlayback::is_end() const
{
	return current == SceneString(End) && fading_from == String();
}

String AnimationNodeStateMachinePlayback::get_current_node() const { return current; }

String AnimationNodeStateMachinePlayback::get_fading_from_node() const { return fading_from; }

Vector<String> AnimationNodeStateMachinePlayback::get_travel_path() const { return path; }

float AnimationNodeStateMachinePlayback::get_current_play_pos() const
{
	return current_nti.position;
}

float AnimationNodeStateMachinePlayback::get_current_length() const { return current_nti.length; }

float AnimationNodeStateMachinePlayback::get_fading_from_play_pos() const
{
	return fadeing_from_nti.position;
}

float AnimationNodeStateMachinePlayback::get_fading_from_length() const
{
	return fadeing_from_nti.length;
}

float AnimationNodeStateMachinePlayback::get_fading_time() const { return fading_time; }

float AnimationNodeStateMachinePlayback::get_fading_pos() const { return fading_pos; }

bool _is_grouped_state_machine(const Ref<AnimationNodeStateMachine> p_node)
{
	return p_node.is_valid() && p_node->get_state_machine_type() ==
									AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED;
}

void AnimationNodeStateMachinePlayback::_clear_fading(AnimationNode::ProcessState& p_process_state,
	AnimationNodeStateMachine* p_state_machine, const String& p_state)
{
	if (!p_state.is_empty() && !_is_grouped_state_machine(p_state_machine->get_node(p_state))) {
		_signal_state_change(p_process_state.tree, p_state, false);
	}
	fading_from = String();
	fadeing_from_nti = AnimationNode::NodeTimeInfo();
}

void AnimationNodeStateMachinePlayback::_start(
	AnimationNode::ProcessState& p_process_state, AnimationNodeStateMachine* p_state_machine)
{
	playing = true;
	_set_current(p_process_state, p_state_machine,
		start_request != String() ? start_request : SceneString(Start));
	teleport_request = true;
	stop_request = false;
	start_request = String();
}

bool AnimationNodeStateMachinePlayback::_travel(AnimationNode::ProcessState& p_process_state,
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine,
	bool p_is_allow_transition_to_self, bool p_test_only)
{
	return _make_travel_path(
		p_process_state, p_tree, p_state_machine, p_is_allow_transition_to_self, path, p_test_only);
}

String AnimationNodeStateMachinePlayback::_validate_path(
	AnimationNodeStateMachine* p_state_machine, const String& p_path)
{
	if (p_state_machine->get_state_machine_type() ==
		AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		return p_path; // Grouped state machine doesn't allow validate-able request.
	}
	String target = p_path;
	Ref<AnimationNodeStateMachine> anodesm = p_state_machine->find_node_by_path(target);
	while (anodesm.is_valid() && anodesm->get_state_machine_type() ==
									 AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		Vector<int> indices = anodesm->find_transition_from(SceneString(Start));
		if (indices.size()) {
			target =
				target + "/" + anodesm->get_transition_to(indices[0]); // Find next state of Start.
		}
		else {
			break; // There is no transition in Start state of grouped state machine.
		}
		anodesm = p_state_machine->find_node_by_path(target);
	}
	return target;
}

AnimationNode::NodeTimeInfo AnimationNodeStateMachinePlayback::process(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationNodeStateMachine* p_state_machine, const AnimationMixer::PlaybackInfo p_playback_info,
	bool p_test_only)
{
	AnimationNode::NodeTimeInfo nti =
		_process(p_process_state, p_instance, p_state_machine, p_playback_info, p_test_only);
	start_request = String();
	next_request = false;
	stop_request = false;
	reset_request_on_teleport = false;
	return nti;
}

AnimationNode::NodeTimeInfo AnimationNodeStateMachinePlayback::_process(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationNodeStateMachine* p_state_machine, const AnimationMixer::PlaybackInfo p_playback_info,
	bool p_test_only)
{
	AnimationTree* tree = p_process_state.tree;

	double p_time = p_playback_info.time;
	double p_delta = p_playback_info.delta;
	bool p_seek = p_playback_info.seeked;
	bool p_is_external_seeking = p_playback_info.is_external_seeking;

	// Check seek to 0 (means reset) by parent AnimationNode.
	if (Math::is_zero_approx(p_time) && p_seek && !p_is_external_seeking) {
		if (p_state_machine->state_machine_type !=
				AnimationNodeStateMachine::STATE_MACHINE_TYPE_NESTED ||
			is_end() || !playing) {
			// Restart state machine.
			if (p_state_machine->get_state_machine_type() !=
				AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
				path.clear();
				_clear_path_children(p_process_state, tree, p_state_machine, p_test_only);
			}
			_start(p_process_state, p_state_machine);
			reset_request = true;
		}
		else {
			// Reset current state.
			reset_request = true;
			teleport_request = true;
		}
	}

	if (stop_request) {
		start_request = String();
		travel_request = String();
		path.clear();
		playing = false;
		return AnimationNode::NodeTimeInfo();
	}

	if (!playing && start_request != String() && travel_request != String()) {
		return AnimationNode::NodeTimeInfo();
	}

	// Process start/travel request.
	if (start_request != String() || travel_request != String()) {
		if (p_state_machine->get_state_machine_type() !=
			AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
			_clear_path_children(p_process_state, tree, p_state_machine, p_test_only);
		}
	}

	if (start_request != String()) {
		path.clear();
		String start_target = _validate_path(p_state_machine, start_request);
		Vector<String> start_path = String(start_target).split("/");
		start_request = start_path[0];
		if (start_path.size()) {
			_start_children(tree, p_state_machine, start_target, p_test_only);
		}
		// Teleport to start.
		if (p_state_machine->states.has(start_request)) {
			_start(p_process_state, p_state_machine);
		}
		else {
			String node = start_request;
			ERR_FAIL_V_MSG(AnimationNode::NodeTimeInfo(), "No such node: '" + node + "'");
		}
	}

	if (travel_request != String()) {
		// Fix path.
		String travel_target = _validate_path(p_state_machine, travel_request);
		Vector<String> travel_path = travel_target.split("/");
		travel_request = travel_path[0];
		String temp_travel_request = travel_request; // For the case that can't travel.
		// Process children.
		Vector<String> new_path;
		bool can_travel = _make_travel_path(p_process_state, tree, p_state_machine,
			travel_path.size() <= 1 ? p_state_machine->is_allow_transition_to_self() : false,
			new_path, p_test_only);
		if (travel_path.size()) {
			_start_children(tree, p_state_machine, travel_target, p_test_only);
		}
		// Process to travel.
		if (can_travel) {
			path = new_path;
		}
		else {
			// Can't travel, then teleport.
			if (p_state_machine->states.has(temp_travel_request)) {
				path.clear();
				if (current != temp_travel_request || reset_request_on_teleport) {
					_set_current(p_process_state, p_state_machine, temp_travel_request);
					reset_request = reset_request_on_teleport;
					teleport_request = true;
				}
			}
			else {
				ERR_FAIL_V_MSG(
					AnimationNode::NodeTimeInfo(), "No such node: '" + temp_travel_request + "'");
			}
		}
	}

	AnimationMixer::PlaybackInfo pi = p_playback_info;

	if (teleport_request) {
		teleport_request = false;
		// Clear fading on teleport.
		fading_from = String();
		fadeing_from_nti = AnimationNode::NodeTimeInfo();
		fading_pos = 0;
		// Init current length.
		pi.time = 0;
		pi.seeked = true;
		pi.is_external_seeking = false;
		pi.weight = 0;
		AnimationNodeInstance* other_instance =
			p_instance.get_child_instance_by_path_or_null(current);
		current_nti = p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
			AnimationNode::FILTER_IGNORE, true, true);
		// Don't process first node if not necessary, instead process next node.
		_transition_to_next_recursive(
			p_process_state, p_instance, tree, p_state_machine, p_delta, p_test_only);
	}

	// Check current node existence.
	if (!p_state_machine->states.has(current)) {
		playing = false; // Current does not exist.
		_set_current(p_process_state, p_state_machine, String());
		return AnimationNode::NodeTimeInfo();
	}

	// Special case for grouped state machine Start/End to make priority with parent blend (means
	// don't treat Start and End states as RESET animations).
	bool is_start_of_group = false;
	bool is_end_of_group = false;
	if (!p_state_machine->are_ends_reset() ||
		p_state_machine->get_state_machine_type() ==
			AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		is_start_of_group = fading_from == SceneString(Start);
		is_end_of_group = current == SceneString(End);
	}

	// Calc blend amount by cross-fade.
	float fade_blend = 1.0;
	if (fading_time && fading_from != String()) {
		if (!p_state_machine->states.has(fading_from)) {
			fading_from = String();
		}
		else {
			if (!p_seek) {
				fading_pos += Math::abs(p_delta);
			}
			fade_blend = MIN(1.0, fading_pos / fading_time);
		}
	}
	if (current_curve.is_valid()) {
		fade_blend = current_curve->sample(fade_blend);
	}
	fade_blend = Math::is_zero_approx(fade_blend) ? CMP_EPSILON : fade_blend;
	if (is_start_of_group) {
		fade_blend = 1.0;
	}
	else if (is_end_of_group) {
		fade_blend = 0.0;
	}

	// Main process.
	pi = p_playback_info;
	pi.weight = fade_blend;
	if (reset_request) {
		reset_request = false;
		pi.time = 0;
		pi.seeked = true;
	}
	AnimationNodeInstance* other_instance = p_instance.get_child_instance_by_path_or_null(current);
	current_nti = p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
		AnimationNode::FILTER_IGNORE, true,
		p_test_only); // Blend values must be more than CMP_EPSILON to process discrete keys in
					  // edge.

	// Cross-fade process.
	if (fading_from != String()) {
		double fade_blend_inv = 1.0 - fade_blend;
		fade_blend_inv = Math::is_zero_approx(fade_blend_inv) ? CMP_EPSILON : fade_blend_inv;
		if (is_start_of_group) {
			fade_blend_inv = 0.0;
		}
		else if (is_end_of_group) {
			fade_blend_inv = 1.0;
		}

		pi = p_playback_info;
		pi.weight = fade_blend_inv;
		if (_reset_request_for_fading_from) {
			_reset_request_for_fading_from = false;
			pi.time = 0;
			pi.seeked = true;
		}
		AnimationNodeInstance* fading_from_instance =
			p_instance.get_child_instance_by_path_or_null(fading_from);
		fadeing_from_nti = p_state_machine->blend_node(p_process_state, p_instance,
			fading_from_instance, pi, AnimationNode::FILTER_IGNORE, true,
			p_test_only); // Blend values must be more than CMP_EPSILON to process discrete keys in
						  // edge.

		if (Animation::is_greater_or_equal_approx(fading_pos, fading_time)) {
			// Finish fading.
			_clear_fading(p_process_state, p_state_machine, fading_from);
		}
	}

	// Find next and see when to transition.
	bool will_end = _transition_to_next_recursive(
						p_process_state, p_instance, tree, p_state_machine, p_delta, p_test_only) ||
					current == SceneString(End);

	// Predict remaining time.
	if (will_end || ((p_state_machine->get_state_machine_type() ==
						 AnimationNodeStateMachine::STATE_MACHINE_TYPE_NESTED) &&
						!p_state_machine->has_transition_from(current))) {
		// There is no next transition.
		if (fading_from != String()) {
			return Animation::is_greater_approx(
					   current_nti.get_remain(), fadeing_from_nti.get_remain())
					   ? current_nti
					   : fadeing_from_nti;
		}
		return current_nti;
	}

	if (!is_end()) {
		current_nti.is_infinity = true;
	}

	return current_nti;
}

bool AnimationNodeStateMachinePlayback::_transition_to_next_recursive(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine, double p_delta,
	bool p_test_only)
{
	_reset_request_for_fading_from = false;

	AnimationMixer::PlaybackInfo pi;
	pi.delta = p_delta;
	NextInfo next;
	Vector<String> transition_path;
	transition_path.push_back(current);
	while (true) {
		next = _find_next(p_process_state, p_instance, p_tree, p_state_machine);

		if (transition_path.has(next.node)) {
			WARN_PRINT_ONCE_ED("AnimationNodeStateMachinePlayback: " + base_path +
							   "playback has detected one or more looped transitions in a single "
							   "frame and aborted to prevent an infinite loop. You may need to "
							   "check the transition settings.");
			break; // Maybe infinity loop, do nothing more.
		}

		transition_path.push_back(next.node);

		// Setting for fading.
		if (next.xfade) {
			// Time to fade.
			fading_from = current;
			fading_time = next.xfade;
			fading_pos = 0;
		}
		else {
			if (reset_request) {
				// There is no possibility of processing doubly. Now we can apply reset actually in
				// here.
				pi.time = 0;
				pi.seeked = true;
				pi.is_external_seeking = false;
				pi.weight = 0;
				AnimationNodeInstance* other_instance =
					p_instance.get_child_instance_by_path_or_null(current);
				p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
					AnimationNode::FILTER_IGNORE, true, p_test_only);
			}
			_clear_fading(p_process_state, p_state_machine, current);
			fading_time = 0;
			fading_pos = 0;
		}

		// If it came from path, remove path.
		if (path.size()) {
			path.remove_at(0);
		}

		// Update current status.
		_set_current(p_process_state, p_state_machine, next.node);
		current_curve = next.curve;

		if (current == SceneString(End)) {
			break;
		}

		_reset_request_for_fading_from =
			reset_request; // To avoid processing doubly, it must be reset in the fading process
						   // within _process().
		reset_request = next.is_reset;

		fadeing_from_nti = current_nti;

		AnimationNodeInstance* other_instance =
			p_instance.get_child_instance_by_path_or_null(current);
		if (next.switch_mode == AnimationNodeStateMachineTransition::SWITCH_MODE_SYNC) {
			pi.time = current_nti.position;
			pi.seeked = true;
			pi.is_external_seeking = false;
			pi.weight = 0;
			p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
				AnimationNode::FILTER_IGNORE, true, p_test_only);
		}

		// Just get length to find next recursive.
		pi.time = 0;
		pi.is_external_seeking = false;
		pi.weight = 0;
		pi.seeked = next.is_reset;
		current_nti = p_state_machine->blend_node(p_process_state, p_instance, other_instance, pi,
			AnimationNode::FILTER_IGNORE, true,
			true); // Just retrieve remain length, don't process.

		// Fading must be processed.
		if (fading_time) {
			break;
		}
	}

	return next.node == SceneString(End);
}

Ref<AnimationNodeStateMachineTransition> AnimationNodeStateMachinePlayback::_check_group_transition(
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine,
	const AnimationNodeStateMachine::Transition& p_transition,
	Ref<AnimationNodeStateMachine>& r_state_machine, bool& r_bypass) const
{
	Ref<AnimationNodeStateMachineTransition> temp_transition;
	Ref<AnimationNodeStateMachinePlayback> parent_playback;
	if (r_state_machine->get_state_machine_type() ==
		AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		if (p_transition.from == SceneString(Start)) {
			parent_playback = _get_parent_playback(p_tree);
			if (parent_playback.is_valid()) {
				r_bypass = true;
				temp_transition = parent_playback->_get_group_start_transition();
			}
		}
		else if (p_transition.to == SceneString(End)) {
			parent_playback = _get_parent_playback(p_tree);
			if (parent_playback.is_valid()) {
				temp_transition = parent_playback->_get_group_end_transition();
			}
		}
		if (temp_transition.is_valid()) {
			r_state_machine = _get_parent_state_machine(p_tree);
			return temp_transition;
		}
	}
	return p_transition.transition;
}

AnimationNodeStateMachinePlayback::NextInfo AnimationNodeStateMachinePlayback::_find_next(
	AnimationNode::ProcessState& p_process_state, AnimationNodeInstance& p_instance,
	AnimationTree* p_tree, AnimationNodeStateMachine* p_state_machine) const
{
	NextInfo next;
	if (path.size()) {
		for (int i = 0; i < p_state_machine->transitions.size(); i++) {
			Ref<AnimationNodeStateMachine> anodesm = p_state_machine;
			bool bypass = false;
			Ref<AnimationNodeStateMachineTransition> ref_transition = _check_group_transition(
				p_tree, p_state_machine, p_state_machine->transitions[i], anodesm, bypass);
			if (ref_transition->get_advance_mode() ==
				AnimationNodeStateMachineTransition::ADVANCE_MODE_DISABLED) {
				continue;
			}
			if (p_state_machine->transitions[i].from == current &&
				p_state_machine->transitions[i].to == path[0]) {
				next.node = path[0];
				next.xfade = ref_transition->get_xfade_time();
				next.curve = ref_transition->get_xfade_curve();
				next.switch_mode = ref_transition->get_switch_mode();
				next.is_reset = ref_transition->is_reset();
				next.break_loop_at_end = ref_transition->is_loop_broken_at_end();
			}
		}
	}
	else {
		int auto_advance_to = -1;
		float priority_best = 1e20;
		for (int i = 0; i < p_state_machine->transitions.size(); i++) {
			Ref<AnimationNodeStateMachine> anodesm = p_state_machine;
			bool bypass = false;
			Ref<AnimationNodeStateMachineTransition> ref_transition = _check_group_transition(
				p_tree, p_state_machine, p_state_machine->transitions[i], anodesm, bypass);
			if (ref_transition->get_advance_mode() ==
				AnimationNodeStateMachineTransition::ADVANCE_MODE_DISABLED) {
				continue;
			}
			if (p_state_machine->transitions[i].from == current) {
				if (ref_transition->get_priority() <= priority_best) {
					priority_best = ref_transition->get_priority();
					auto_advance_to = i;
				}
			}
		}

		if (auto_advance_to != -1) {
			next.node = p_state_machine->transitions[auto_advance_to].to;
			Ref<AnimationNodeStateMachine> anodesm = p_state_machine;
			bool bypass = false;
			Ref<AnimationNodeStateMachineTransition> ref_transition =
				_check_group_transition(p_tree, p_state_machine,
					p_state_machine->transitions[auto_advance_to], anodesm, bypass);
			next.xfade = ref_transition->get_xfade_time();
			next.curve = ref_transition->get_xfade_curve();
			next.switch_mode = ref_transition->get_switch_mode();
			next.is_reset = ref_transition->is_reset();
			next.break_loop_at_end = ref_transition->is_loop_broken_at_end();
		}
	}

	return next;
}

void AnimationNodeStateMachinePlayback::clear_path() { path.clear(); }

void AnimationNodeStateMachinePlayback::push_path(const String& p_state)
{
	path.push_back(p_state);
}

void AnimationNodeStateMachinePlayback::_set_base_path(const String& p_base_path)
{
	base_path = p_base_path;
}

Ref<AnimationNodeStateMachine> AnimationNodeStateMachinePlayback::_get_parent_state_machine(
	AnimationTree* p_tree) const
{
	if (base_path.is_empty()) {
		return Ref<AnimationNodeStateMachine>();
	}
	Vector<String> split = base_path.split("/");
	ERR_FAIL_COND_V_MSG(split.size() < 3, Ref<AnimationNodeStateMachine>(), "Path is too short.");
	split = split.slice(1, split.size() - 2);
	Ref<AnimationNode> root = p_tree->get_root_animation_node();
	ERR_FAIL_COND_V_MSG(root.is_null(), Ref<AnimationNodeStateMachine>(),
		"There is no root AnimationNode in AnimationTree: " + String(p_tree->get_name()));
	String anodesm_path = String("/").join(split);
	Ref<AnimationNodeStateMachine> anodesm =
		!anodesm_path.size() ? root : root->find_node_by_path(anodesm_path);
	ERR_FAIL_COND_V_MSG(anodesm.is_null(), Ref<AnimationNodeStateMachine>(),
		"Can't get state machine with path: " + anodesm_path);
	return anodesm;
}

Ref<AnimationNodeStateMachineTransition>
AnimationNodeStateMachinePlayback::_get_group_start_transition() const
{
	ERR_FAIL_COND_V_MSG(group_start_transition.is_null(),
		Ref<AnimationNodeStateMachineTransition>(), "Group start transition is null.");
	return group_start_transition;
}

Ref<AnimationNodeStateMachineTransition>
AnimationNodeStateMachinePlayback::_get_group_end_transition() const
{
	ERR_FAIL_COND_V_MSG(group_end_transition.is_null(), Ref<AnimationNodeStateMachineTransition>(),
		"Group end transition is null.");
	return group_end_transition;
}

AnimationNodeStateMachinePlayback::AnimationNodeStateMachinePlayback()
{
	set_local_to_scene(true); // Only one per instantiated scene.
	default_transition.instantiate();
	default_transition->set_xfade_time(0);
	default_transition->set_reset(true);
	default_transition->set_advance_mode(AnimationNodeStateMachineTransition::ADVANCE_MODE_AUTO);
	default_transition->set_switch_mode(AnimationNodeStateMachineTransition::SWITCH_MODE_IMMEDIATE);
}

///////////////////////////////////////////////////////

bool AnimationNodeStateMachine::is_parameter_read_only(const String& p_parameter) const
{
	if (AnimationNode::is_parameter_read_only(p_parameter)) {
		return true;
	}

	if (p_parameter == playback) {
		return true;
	}
	return false;
}

void AnimationNodeStateMachine::add_node(
	const String& p_name, Ref<AnimationNode> p_node, const Vector2& p_position)
{
	ERR_FAIL_COND(states.has(p_name));
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(String(p_name).contains_char('/'));

	State state_new;
	state_new.node = p_node;
	state_new.position = p_position;

	states[p_name] = state_new;

	_add_node(p_node);
	emit_changed();
	_tree_changed();
}

void AnimationNodeStateMachine::replace_node(const String& p_name, Ref<AnimationNode> p_node)
{
	ERR_FAIL_COND(states.has(p_name) == false);
	ERR_FAIL_COND(p_node.is_null());
	ERR_FAIL_COND(String(p_name).contains_char('/'));

	{
		Ref<AnimationNode> node = states[p_name].node;
		if (node.is_valid()) {
			_remove_node(node);
		}
	}

	states[p_name].node = p_node;

	_add_node(p_node);
	emit_changed();
	_tree_changed();
}

AnimationNodeStateMachine::StateMachineType
AnimationNodeStateMachine::get_state_machine_type() const
{
	return state_machine_type;
}

void AnimationNodeStateMachine::set_allow_transition_to_self(bool p_enable)
{
	allow_transition_to_self = p_enable;
}

bool AnimationNodeStateMachine::is_allow_transition_to_self() const
{
	return allow_transition_to_self;
}

void AnimationNodeStateMachine::set_reset_ends(bool p_enable) { reset_ends = p_enable; }

bool AnimationNodeStateMachine::are_ends_reset() const { return reset_ends; }

Ref<AnimationNode> AnimationNodeStateMachine::get_node(const String& p_name) const
{
	ERR_FAIL_COND_V_EDMSG(
		!states.has(p_name), Ref<AnimationNode>(), String(p_name) + " is not found current state.");

	return states[p_name].node;
}

String AnimationNodeStateMachine::get_node_name(const Ref<AnimationNode>& p_node) const
{
	for (const KeyValue<String, State>& E : states) {
		if (E.value.node == p_node) {
			return E.key;
		}
	}

	ERR_FAIL_V(String());
}

void AnimationNodeStateMachine::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	Vector<String> nodes;

	for (const KeyValue<String, State>& E : states) {
		nodes.push_back(E.key);
	}

	nodes.sort();

	for (int i = 0; i < nodes.size(); i++) {
		ChildNode cn;
		cn.name = nodes[i];
		cn.node = states[cn.name].node;
		r_child_nodes->push_back(cn);
	}
}

bool AnimationNodeStateMachine::has_node(const String& p_name) const
{
	return states.has(p_name);
}

void AnimationNodeStateMachine::_rename_transitions(
	const String& p_name, const String& p_new_name)
{
	if (updating_transitions) {
		return;
	}

	updating_transitions = true;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_name) {
			transitions.write[i].from = p_new_name;
		}
		if (transitions[i].to == p_name) {
			transitions.write[i].to = p_new_name;
		}
	}
	updating_transitions = false;
}

LocalVector<String> AnimationNodeStateMachine::get_node_list() const
{
	LocalVector<String> nodes;
	nodes.reserve(states.size());
	for (const KeyValue<String, State>& E : states) {
		nodes.push_back(E.key);
	}
	nodes.sort();
	return nodes;
}

bool AnimationNodeStateMachine::has_transition(
	const String& p_from, const String& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			return true;
		}
	}
	return false;
}

bool AnimationNodeStateMachine::has_transition_from(const String& p_from) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from) {
			return true;
		}
	}
	return false;
}

bool AnimationNodeStateMachine::has_transition_to(const String& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].to == p_to) {
			return true;
		}
	}
	return false;
}

int AnimationNodeStateMachine::find_transition(
	const String& p_from, const String& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			return i;
		}
	}
	return -1;
}

Vector<int> AnimationNodeStateMachine::find_transition_from(const String& p_from) const
{
	Vector<int> ret;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from) {
			ret.push_back(i);
		}
	}
	return ret;
}

Vector<int> AnimationNodeStateMachine::find_transition_to(const String& p_to) const
{
	Vector<int> ret;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].to == p_to) {
			ret.push_back(i);
		}
	}
	return ret;
}

bool AnimationNodeStateMachine::_can_connect(const String& p_name)
{
	if (states.has(p_name)) {
		return true;
	}

	String node_name = p_name;
	if (node_name.get_slice_count("/") < 2) {
		return false;
	}

	return false;
}

Ref<AnimationNodeStateMachineTransition> AnimationNodeStateMachine::get_transition(
	int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), Ref<AnimationNodeStateMachineTransition>());
	return transitions[p_transition].transition;
}

String AnimationNodeStateMachine::get_transition_from(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), String());
	return transitions[p_transition].from;
}

String AnimationNodeStateMachine::get_transition_to(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), String());
	return transitions[p_transition].to;
}

bool AnimationNodeStateMachine::is_transition_across_group(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), false);
	if (get_state_machine_type() == AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		if (transitions[p_transition].from == SceneString(Start) ||
			transitions[p_transition].to == SceneString(End)) {
			return true;
		}
	}
	return false;
}

int AnimationNodeStateMachine::get_transition_count() const { return transitions.size(); }

void AnimationNodeStateMachine::remove_transition(const String& p_from, const String& p_to)
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			remove_transition_by_index(i);
			return;
		}
	}
}

void AnimationNodeStateMachine::_remove_transition(
	const Ref<AnimationNodeStateMachineTransition> p_transition)
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].transition == p_transition) {
			remove_transition_by_index(i);
			return;
		}
	}
}

void AnimationNodeStateMachine::set_graph_offset(const Vector2& p_offset)
{
	graph_offset = p_offset;
}

Vector2 AnimationNodeStateMachine::get_graph_offset() const { return graph_offset; }

String AnimationNodeStateMachine::get_caption() const { return "StateMachine"; }

Ref<AnimationNode> AnimationNodeStateMachine::get_child_by_name(const String& p_name) const
{
	return get_node(p_name);
}

void AnimationNodeStateMachine::reset_state()
{
	states.clear();
	transitions.clear();
	playback = "playback";
	graph_offset = Vector2();

	Ref<AnimationNode> s;
	s.instantiate();
	s->set_endpoint_type(AnimationNode::ENDPOINT_START);
	State start;
	start.node = s;
	start.position = Vector2(200, 100);
	states[SceneString(Start)] = start;

	Ref<AnimationNode> e;
	e.instantiate();
	e->set_endpoint_type(AnimationNode::ENDPOINT_END);
	State end;
	end.node = e;
	end.position = Vector2(900, 100);
	states[SceneString(End)] = end;

	emit_changed();
	_tree_changed();
}

void AnimationNodeStateMachine::set_node_position(
	const String& p_name, const Vector2& p_position)
{
	ERR_FAIL_COND(!states.has(p_name));
	states[p_name].position = p_position;
}

Vector2 AnimationNodeStateMachine::get_node_position(const String& p_name) const
{
	ERR_FAIL_COND_V(!states.has(p_name), Vector2());
	return states[p_name].position;
}

void AnimationNodeStateMachine::_tree_changed()
{
	emit_changed();
	AnimationNode::_tree_changed();
}

#ifdef TOOLS_ENABLED
void AnimationNodeStateMachine::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	bool add_state_options = false;
	if (p_idx == 0) {
		add_state_options =
			(pf == "get_node" || pf == "has_node" || pf == "rename_node" || pf == "remove_node" ||
				pf == "replace_node" || pf == "set_node_position" || pf == "get_node_position");
	}
	else if (p_idx <= 1) {
		add_state_options =
			(pf == "has_transition" || pf == "add_transition" || pf == "remove_transition");
	}
	if (add_state_options) {
		for (const KeyValue<String, State>& E : states) {
			r_options->push_back(String(E.key).quote());
		}
	}
	AnimationNode::get_argument_options(p_function, p_idx, r_options);
}
#endif

Vector<String> AnimationNodeStateMachine::get_nodes_with_transitions_from(
	const String& p_node) const
{
	Vector<String> result;
	for (const Transition& transition : transitions) {
		if (transition.from == p_node) {
			result.push_back(transition.to);
		}
	}
	return result;
}

Vector<String> AnimationNodeStateMachine::get_nodes_with_transitions_to(
	const String& p_node) const
{
	Vector<String> result;
	for (const Transition& transition : transitions) {
		if (transition.to == p_node) {
			result.push_back(transition.from);
		}
	}
	return result;
}

AnimationNodeStateMachine::AnimationNodeStateMachine()
{
	root_node = true;

	Ref<AnimationNode> s;
	s.instantiate();
	s->set_endpoint_type(AnimationNode::ENDPOINT_START);
	State start;
	start.node = s;
	start.position = Vector2(200, 100);
	states[SceneString(Start)] = start;

	Ref<AnimationNode> e;
	e.instantiate();
	e->set_endpoint_type(AnimationNode::ENDPOINT_END);
	State end;
	end.node = e;
	end.position = Vector2(900, 100);
	states[SceneString(End)] = end;
}

void AnimationNodeStateMachinePlayback::_signal_state_change(
	AnimationTree*, String const&, bool)
{
}

void AnimationNodeStateMachine::remove_transition_by_index(int) {}

void AnimationNodeStateMachinePlayback::_start_children(
	AnimationTree*, AnimationNodeStateMachine*, String const&, bool)
{
}

bool AnimationNodeStateMachinePlayback::_make_travel_path(AnimationNode::ProcessState&,
	AnimationTree*, AnimationNodeStateMachine*, bool, Vector<String>&, bool)
{
	return true;
}

Ref<AnimationNodeStateMachinePlayback> AnimationNodeStateMachinePlayback::_get_parent_playback(
	AnimationTree*) const
{
	Ref<AnimationNodeStateMachinePlayback> ansmp = memnew(AnimationNodeStateMachinePlayback);
	return ansmp;
}

void AnimationNodeStateMachinePlayback::_clear_path_children(
	AnimationNode::ProcessState&, AnimationTree*, AnimationNodeStateMachine*, bool)
{
}

bool AnimationNodeStateMachine::can_edit_node(String const&) const { return true; }

void AnimationNodeStateMachine::validate_node(
	const AnimationTree* p_tree, const String& p_node) const
{
}

AnimationNode::NodeTimeInfo AnimationNodeStateMachine::_process(
	AnimationNode::ProcessState& r_process_state, AnimationNodeInstance& p_instance,
	const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only)
{
	return AnimationNode::NodeTimeInfo();
}

// ============================================================================
// Merged from scene/animation/animation_mixer.cpp
// ============================================================================

#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/string/ustring.h"
#include "scene/2d/node_2d.h"
#include "scene/animation/animation_mixer.h"
#include "scene/resources/animation.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"

#ifndef _3D_DISABLED
#include "scene/3d/visual_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#endif // _3D_DISABLED

void AnimationMixer::_animation_added(const String& p_name, const String& p_library)
{
	_animation_set_cache_update();
}

void AnimationMixer::_animation_removed(const String& p_name, const String& p_library)
{
	const String name =
		p_library == String() ? p_name : String(String(p_library) + "/" + String(p_name));

	if (!animation_set.has(name)) {
		return; // No need to update because not the one from the library being used.
	}

	_animation_set_cache_update();

	_remove_animation(name);
}

void AnimationMixer::_animation_changed(const String& p_name) { _clear_caches(); }

void AnimationMixer::get_animation_library_list(LocalVector<String>* p_libraries) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		p_libraries->push_back(lib.name);
	}
}

Ref<AnimationLibrary> AnimationMixer::get_animation_library(const String& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return lib.library;
		}
	}
	ERR_FAIL_V(Ref<AnimationLibrary>());
}

bool AnimationMixer::has_animation_library(const String& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return true;
		}
	}

	return false;
}

const String& AnimationMixer::find_animation_library(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.value.animation_library;
		}
	}
	const static String empty = String();
	return empty;
}

LocalVector<String> AnimationMixer::get_sorted_animation_list() const
{
	LocalVector<String> animations;
	get_animation_list(&animations);
	animations.sort();
	return animations;
}

void AnimationMixer::get_animation_list(LocalVector<String>* p_animations) const
{
	p_animations->reserve(p_animations->size() + animation_set.size());
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		p_animations->push_back(E.key);
	}
}

const Ref<Animation>& AnimationMixer::get_animation(const String& p_name) const
{
	const Ref<Animation>& animation = get_animation_or_null(p_name);
	ERR_FAIL_COND_V_MSG(
		animation.is_null(), animation, vformat("Animation not found: \"%s\".", p_name));
	return animation;
}

const Ref<Animation>& AnimationMixer::get_animation_or_null(const String& p_name) const
{
	const AnimationData* ad = animation_set.getptr(p_name);
	if (!ad) {
		const static Ref<Animation> empty = Ref<Animation>();
		return empty;
	}
	return ad->animation;
}

bool AnimationMixer::has_animation(const String& p_name) const
{
	return animation_set.has(p_name);
}

String AnimationMixer::find_animation(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.key;
		}
	}
	return String();
}

void AnimationMixer::_set_process(bool p_process, bool p_force)
{
	if (processing == p_process && !p_force) {
		return;
	}

	switch (callback_mode_process) {
	case ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS:
#ifdef TOOLS_ENABLED
		set_physics_process_internal(p_process && active && !editing);
#else
		set_physics_process_internal(p_process && active);
#endif // TOOLS_ENABLED
		break;
	case ANIMATION_CALLBACK_MODE_PROCESS_IDLE:
#ifdef TOOLS_ENABLED
		set_process_internal(p_process && active && !editing);
#else
		set_process_internal(p_process && active);
#endif // TOOLS_ENABLED
		break;
	case ANIMATION_CALLBACK_MODE_PROCESS_MANUAL:
		break;
	}

	processing = p_process;
}

void AnimationMixer::set_active(bool p_active)
{
	if (active == p_active) {
		return;
	}

	active = p_active;
	_set_active(active);
	_set_process(processing, true);

	if (!active && is_inside_tree()) {
		_clear_caches();
	}
}

bool AnimationMixer::is_active() const { return active; }

void AnimationMixer::set_root_node(const NodePath& p_path)
{
	root_node = p_path;
	_clear_caches();
}

NodePath AnimationMixer::get_root_node() const { return root_node; }

void AnimationMixer::set_deterministic(bool p_deterministic)
{
	deterministic = p_deterministic;
	_clear_caches();
}

bool AnimationMixer::is_deterministic() const { return deterministic; }

void AnimationMixer::set_callback_mode_process(AnimationCallbackModeProcess p_mode)
{
	if (callback_mode_process == p_mode) {
		return;
	}

	bool was_active = is_active();
	if (was_active) {
		set_active(false);
	}

	callback_mode_process = p_mode;

	if (was_active) {
		set_active(true);
	}
}

AnimationMixer::AnimationCallbackModeProcess AnimationMixer::get_callback_mode_process() const
{
	return callback_mode_process;
}

AnimationMixer::AnimationCallbackModeMethod AnimationMixer::get_callback_mode_method() const
{
	return callback_mode_method;
}

AnimationMixer::AnimationCallbackModeDiscrete AnimationMixer::get_callback_mode_discrete() const
{
	return callback_mode_discrete;
}

void AnimationMixer::set_audio_max_polyphony(int p_audio_max_polyphony)
{
	ERR_FAIL_COND(p_audio_max_polyphony < 0 || p_audio_max_polyphony > 128);
	audio_max_polyphony = p_audio_max_polyphony;
}

int AnimationMixer::get_audio_max_polyphony() const { return audio_max_polyphony; }

#ifdef TOOLS_ENABLED

bool AnimationMixer::is_editing() const { return editing; }

void AnimationMixer::set_dummy(bool p_dummy) { dummy = p_dummy; }

bool AnimationMixer::is_dummy() const { return dummy; }
#endif // TOOLS_ENABLED

void AnimationMixer::_clear_caches(bool p_clear_track_cache) {}

void AnimationMixer::_init_root_motion_cache()
{
	root_motion_cache.loc = Vector3(0, 0, 0);
	root_motion_cache.rot = Quaternion(0, 0, 0, 1);
	root_motion_cache.scale = Vector3(1, 1, 1);
	root_motion_position = Vector3(0, 0, 0);
	root_motion_rotation = Quaternion(0, 0, 0, 1);
	root_motion_scale = Vector3(0, 0, 0);
	root_motion_position_accumulator = Vector3(0, 0, 0);
	root_motion_rotation_accumulator = Quaternion(0, 0, 0, 1);
	root_motion_scale_accumulator = Vector3(1, 1, 1);
}

void AnimationMixer::_create_track_num_to_track_cache_for_animation(
	const Ref<Animation>& p_animation)
{
	if (animation_track_num_to_track_cache.has(p_animation)) {
		// In AnimationMixer::_update_caches, it retrieves all animations via
		// AnimationMixer::get_animation_list Since multiple AnimationLibraries can share the same
		// Animation, it is possible that the cache is already created.
		return;
	}
	LocalVector<TrackCache*>& track_num_to_track_cache =
		animation_track_num_to_track_cache.insert_new(p_animation, LocalVector<TrackCache*>())
			->value;
	const LocalVector<Animation::Track*>& tracks = p_animation->get_tracks();

	track_num_to_track_cache.resize(tracks.size());
	for (uint32_t i = 0; i < tracks.size(); i++) {
		TrackCache** track_ptr = track_cache.getptr(tracks[i]->get_unique_id());
		if (track_ptr == nullptr) {
			track_num_to_track_cache[i] = nullptr;
		}
		else {
			track_num_to_track_cache[i] = *track_ptr;
		}
	}
}

bool AnimationMixer::_blend_pre_process(
	double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	return true;
}

void AnimationMixer::_blend_capture(double p_delta) { blend_capture(p_delta); }

void AnimationMixer::blend_capture(double p_delta)
{
	if (capture_cache.animation.is_null()) {
		return;
	}

	capture_cache.remain -= p_delta * capture_cache.step;
	if (Animation::is_less_or_equal_approx(capture_cache.remain, 0)) {
		if (capture_cache.animation.is_valid()) {
			animation_track_num_to_track_cache.erase(capture_cache.animation);
		}
		capture_cache.clear();
		return;
	}

	real_t weight = Tween::run_equation(
		capture_cache.trans_type, capture_cache.ease_type, capture_cache.remain, 0.0, 1.0, 1.0);

	// Blend with other animations.
	real_t inv = 1.0 - weight;
	for (AnimationInstance& ai : animation_instances) {
		ai.playback_info.weight *= inv;
	}

	// Build capture animation instance.
	PlaybackInfo pi;
	pi.weight = weight;

	AnimationInstance ai;
	ai.animation = capture_cache.animation;
	ai.playback_info = pi;

	animation_instances.push_back(ai);
}

void AnimationMixer::_blend_calc_total_weight()
{
	for (const AnimationInstance& ai : animation_instances) {
		const Ref<Animation>& a = ai.animation;
		real_t weight = ai.playback_info.weight;
		if (Math::is_zero_approx(weight)) {
			continue;
		}
		Span<real_t> track_weights = ai.playback_info.track_weights != nullptr
										 ? *ai.playback_info.track_weights
										 : Span<real_t>();

		LocalVector<TrackCache*>* t_cache = animation_track_num_to_track_cache.getptr(a);
		ERR_CONTINUE_EDMSG(!t_cache, "No animation in cache.");
		LocalVector<TrackCache*>& track_num_to_track_cache = *t_cache;

		uint64_t pass_id = ++animation_instance_weight_pass_counter;
		// Handle wrap (slower but rare).
		if (unlikely(pass_id == 0)) {
			for (KeyValue<Animation::TrackCacheID, TrackCache*>& kv : track_cache) {
				if (kv.value) {
					kv.value->animation_instance_weight_applied_at = 0;
				}
			}
			animation_instance_weight_pass_counter = 1;
			pass_id = 1;
		}

		const LocalVector<Animation::Track*>& tracks = a->get_tracks();
		Animation::Track* const* tracks_ptr = tracks.ptr();
		int count = tracks.size();
		for (int i = 0; i < count; i++) {
			Animation::Track* animation_track = tracks_ptr[i];
			if (!animation_track->enabled) {
				continue;
			}
			TrackCache* track = track_num_to_track_cache[i];
			if (track == nullptr) {
				// No path, but avoid error spamming.
				continue;
			}

			// In some cases (e.g. transform TrackCache),
			// multiple Animation::Tracks (e.g. TYPE_POSITION_3D, TYPE_ROTATION_3D and
			// TYPE_SCALE_3D) can point to the same TrackCache instance. So we need to make sure
			// that the weight is added only once per AnimationInstance.
			if (track->animation_instance_weight_applied_at == pass_id) {
				continue;
			}

			int blend_idx = track->blend_idx;
			ERR_CONTINUE(blend_idx < 0 || blend_idx >= track_count);
			real_t blend;
			if (!track_weights.is_empty() && blend_idx < static_cast<int>(track_weights.size())) {
				blend = track_weights[blend_idx] * weight;
			}
			else {
				blend = weight;
			}
			track->total_weight += blend;
			track->animation_instance_weight_applied_at = pass_id;
		}
	}
}

void AnimationMixer::make_animation_instance(
	const String& p_name, const PlaybackInfo& p_playback_info)
{
	const Ref<Animation>& animation = get_animation_or_null(p_name);
	ERR_FAIL_COND(animation.is_null());

	AnimationInstance ai;
	ai.animation = animation;
	ai.playback_info = p_playback_info;

	animation_instances.push_back(std::move(ai));
}

void AnimationMixer::clear_animation_instances() { animation_instances.clear(); }

void AnimationMixer::advance(double p_time) { _process_animation(p_time); }

void AnimationMixer::clear_caches() { _clear_caches(); }

NodePath AnimationMixer::get_root_motion_track() const { return root_motion_track; }

void AnimationMixer::set_root_motion_local(bool p_enabled) { root_motion_local = p_enabled; }

bool AnimationMixer::is_root_motion_local() const { return root_motion_local; }

Vector3 AnimationMixer::get_root_motion_position() const { return root_motion_position; }

Quaternion AnimationMixer::get_root_motion_rotation() const { return root_motion_rotation; }

Vector3 AnimationMixer::get_root_motion_scale() const { return root_motion_scale; }

Vector3 AnimationMixer::get_root_motion_position_accumulator() const
{
	return root_motion_position_accumulator;
}

Quaternion AnimationMixer::get_root_motion_rotation_accumulator() const
{
	return root_motion_rotation_accumulator;
}

Vector3 AnimationMixer::get_root_motion_scale_accumulator() const
{
	return root_motion_scale_accumulator;
}

void AnimationMixer::set_reset_on_save_enabled(bool p_enabled) { reset_on_save = p_enabled; }

bool AnimationMixer::is_reset_on_save_enabled() const { return reset_on_save; }

bool AnimationMixer::can_apply_reset() const { return has_animation(SceneString(RESET)); }

void AnimationMixer::_node_removed(Node* p_node) { _clear_caches(); }

void AnimationMixer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (!processing) {
			set_physics_process_internal(false);
			set_process_internal(false);
		}
		_clear_caches();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (active && callback_mode_process == ANIMATION_CALLBACK_MODE_PROCESS_IDLE) {
			_process_animation(get_process_delta_time());
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (active && callback_mode_process == ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS) {
			_process_animation(get_physics_process_delta_time());
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_clear_caches();
	} break;
	}
}

#ifdef TOOLS_ENABLED
void AnimationMixer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0) {
		if (pf == "get_animation" || pf == "has_animation") {
			for (const String& name : get_sorted_animation_list()) {
				r_options->push_back(String(name).quote());
			}
		}
		else if (pf == "get_animation_library" || pf == "has_animation_library" ||
				   pf == "remove_animation_library" || pf == "rename_animation_library") {
			LocalVector<String> al;
			get_animation_library_list(&al);
			for (const String& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	Node::get_argument_options(p_function, p_idx, r_options);
}
#endif

AnimationMixer::AnimationMixer() { root_node = NodePath(".."); }

AnimationMixer::~AnimationMixer() {}

void AnimatedValuesBackup::set_data(
	const AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher>& p_data)
{
	clear_data();

	for (const KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& E : p_data) {
		AnimationMixer::TrackCache* track = get_cache_copy(E.value);
		if (!track) {
			continue; // Some types of tracks do not get a copy and must be ignored.
		}

		data.insert(E.key, track);
	}
}

AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher>
AnimatedValuesBackup::get_data() const
{
	AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher> ret;
	for (const KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& E : data) {
		AnimationMixer::TrackCache* track = get_cache_copy(E.value);
		ERR_CONTINUE(
			!track); // Backup shouldn't contain tracks that cannot be copied, this is a mistake.

		ret.insert(E.key, track);
	}
	return ret;
}

void AnimatedValuesBackup::clear_data()
{
	for (KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& K : data) {
		memdelete(K.value);
	}
	data.clear();
}

AnimationMixer::TrackCache* AnimatedValuesBackup::get_cache_copy(
	AnimationMixer::TrackCache* p_cache) const
{
	switch (p_cache->type) {
	case Animation::TYPE_BEZIER:
	case Animation::TYPE_VALUE:
	case Animation::TYPE_POSITION_3D:
	case Animation::TYPE_ROTATION_3D:
	case Animation::TYPE_SCALE_3D:
	case Animation::TYPE_BLEND_SHAPE:
	case Animation::TYPE_AUDIO: {
		return memnew(AnimationMixer::TrackCache(*p_cache));
	}

	case Animation::TYPE_METHOD:
	case Animation::TYPE_ANIMATION: {
		// Nothing to do here.
	} break;
	}
	return nullptr;
}

void AnimationMixer::_animation_set_cache_update() {}

void AnimationMixer::_remove_animation(const String& p_name) {}

void AnimationMixer::_process_animation(double p_delta, bool p_update_only) {}

void AnimationMixer::_animation_renamed(
	const String& p_name, const String& p_to_name, const String& p_library)
{
}

void AnimationMixer::_set_active(bool p_active) {}

uint32_t AnimationMixer::_get_libraries_property_usage() const { return 0; }

void AnimationMixer::_blend_post_process() {}

// ==========================================================================
// Merged from scene/animation/animation_player.cpp
// ==========================================================================

// --- animation_player.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "scene/animation/animation_mixer.h"

void AnimationPlayer::_set_process_callback_bind_compat_80813(
	AnimationPlayer::AnimationProcessCallback p_mode)
{
	set_callback_mode_process(
		static_cast<AnimationMixer::AnimationCallbackModeProcess>(static_cast<int>(p_mode)));
}

AnimationPlayer::AnimationProcessCallback
AnimationPlayer::_get_process_callback_bind_compat_80813() const
{
	return static_cast<AnimationProcessCallback>(static_cast<int>(get_callback_mode_process()));
}

AnimationPlayer::AnimationMethodCallMode
AnimationPlayer::_get_method_call_mode_bind_compat_80813() const
{
	return static_cast<AnimationMethodCallMode>(static_cast<int>(get_callback_mode_method()));
}

void AnimationPlayer::_set_root_bind_compat_80813(const NodePath& p_root) { set_root_node(p_root); }

NodePath AnimationPlayer::_get_root_bind_compat_80813() const { return get_root_node(); }

void AnimationPlayer::_seek_bind_compat_80813(double p_time, bool p_update)
{
	seek(p_time, p_update, false);
}

String AnimationPlayer::_get_current_animation_bind_compat_110767() const
{
	return get_current_animation();
}

void AnimationPlayer::_set_current_animation_bind_compat_110767(const String& p_animation)
{
	set_current_animation(p_animation);
}

String AnimationPlayer::_get_assigned_animation_bind_compat_110767() const
{
	return get_assigned_animation();
}

void AnimationPlayer::_set_assigned_animation_bind_compat_110767(const String& p_animation)
{
	set_assigned_animation(p_animation);
}

String AnimationPlayer::_get_autoplay_bind_compat_110767() const { return get_autoplay(); }

void AnimationPlayer::_set_autoplay_bind_compat_110767(const String& p_name)
{
	set_autoplay(p_name);
}

void AnimationPlayer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "scene/animation/animation_mixer.h"
#include "core/config/engine.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"

void AnimationPlayer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		if (!Engine::is_editor_hint() && animation_set.has(autoplay)) {
			set_active(active);
			play(autoplay);
			_check_immediately_after_start();
		}
	} break;
	}
}

void AnimationPlayer::_process_playback_data(PlaybackData& cd, double p_delta, float p_blend,
	bool p_seeked, bool p_internal_seeked, bool p_started, bool p_is_current)
{
	double speed = speed_scale * cd.speed_scale;
	bool backwards = std::signbit(speed); // Negative zero means playing backwards too.
	double delta = p_started ? 0 : p_delta * speed;
	double next_pos = cd.pos + delta;

	double start = cd.get_start_time();
	double end = cd.get_end_time();
	AnimationData* p_from = &animation_set[cd.animation_name];

	Animation::LoopedFlag looped_flag = Animation::LOOPED_FLAG_NONE;

	switch (p_from->animation->get_loop_mode()) {
	case Animation::LOOP_NONE: {
		if (Animation::is_less_approx(next_pos, start)) {
			next_pos = start;
		}
		else if (Animation::is_greater_approx(next_pos, end)) {
			next_pos = end;
		}
		delta = next_pos - cd.pos; // Fix delta (after determination of backwards because negative
								   // zero is lost here).
	} break;

	case Animation::LOOP_LINEAR: {
		if (Animation::is_less_approx(next_pos, start) &&
			Animation::is_greater_or_equal_approx(cd.pos, start)) {
			looped_flag = Animation::LOOPED_FLAG_START;
		}
		if (Animation::is_greater_approx(next_pos, end) &&
			Animation::is_less_or_equal_approx(cd.pos, end)) {
			looped_flag = Animation::LOOPED_FLAG_END;
		}
		next_pos = Math::fposmod(next_pos - start, end - start) + start;
	} break;

	case Animation::LOOP_PINGPONG: {
		if (Animation::is_less_approx(next_pos, start) &&
			Animation::is_greater_or_equal_approx(cd.pos, start)) {
			cd.speed_scale *= -1.0;
			looped_flag = Animation::LOOPED_FLAG_START;
		}
		if (Animation::is_greater_approx(next_pos, end) &&
			Animation::is_less_or_equal_approx(cd.pos, end)) {
			cd.speed_scale *= -1.0;
			looped_flag = Animation::LOOPED_FLAG_END;
		}
		next_pos = Math::pingpong(next_pos - start, end - start) + start;
	} break;

	default:
		break;
	}

	double prev_pos = cd.pos; // The animation may be changed during process, so it is safer that
							  // the state is changed before process.

	// End detection.
	if (p_is_current) {
		if (p_from->animation->get_loop_mode() == Animation::LOOP_NONE) {
			if (!backwards && Animation::is_less_or_equal_approx(prev_pos, end) &&
				Math::is_equal_approx(next_pos, end)) {
				// Playback finished.
				next_pos = end; // Snap to the edge.
				end_reached = true;
				end_notify = Animation::is_less_approx(
					prev_pos, end); // Notify only if not already at the end.
				p_blend = 1.0;
			}
			if (backwards && Animation::is_greater_or_equal_approx(prev_pos, start) &&
				Math::is_equal_approx(next_pos, start)) {
				// Playback finished.
				next_pos = start; // Snap to the edge.
				end_reached = true;
				end_notify = Animation::is_greater_approx(
					prev_pos, start); // Notify only if not already at the beginning.
				p_blend = 1.0;
			}
		}
	}

	cd.pos = next_pos;

	PlaybackInfo pi;
	if (p_started) {
		pi.time = prev_pos;
		pi.delta = 0;
		pi.start = start;
		pi.end = end;
		pi.seeked = true;
	}
	else {
		pi.time = next_pos;
		pi.delta = delta;
		pi.start = start;
		pi.end = end;
		pi.seeked = p_seeked;
	}
	if (Math::is_zero_approx(pi.delta) && backwards) {
		pi.delta = -0.0; // Sign is needed to handle converted Continuous track from Discrete track
						 // correctly.
	}
	// Immediately after playback, discrete keys should be retrieved with EXACT mode since behind
	// keys must be ignored at that time.
	pi.is_external_seeking = !p_internal_seeked && !p_started;
	pi.looped_flag = looped_flag;
	pi.weight = p_blend;
	make_animation_instance(cd.animation_name, pi);
}

float AnimationPlayer::get_current_blend_amount()
{
	Playback& c = playback;
	float blend = 1.0;
	for (const Blend& b : c.blend) {
		blend = blend - b.blend_left;
	}
	return MAX(0, blend);
}

void AnimationPlayer::_blend_playback_data(double p_delta, bool p_started)
{
	Playback& c = playback;

	bool seeked = c.seeked; // The animation may be changed during process, so it is safer that the
							// state is changed before process.
	bool internal_seeked = c.internal_seeked;

	if (!Math::is_zero_approx(p_delta)) {
		c.seeked = false;
		c.internal_seeked = false;
	}

	// Second, process current animation to check if the animation end reached.
	_process_playback_data(
		c.current, p_delta, get_current_blend_amount(), seeked, internal_seeked, p_started, true);

	// Finally, if not end the animation, do blending.
	if (end_reached) {
		playback.blend.clear();
		if (end_notify) {
			finished_anim = playback.assigned;
		}
		return;
	}
	LocalVector<int> to_erase;
	for (uint32_t i = 0; i < c.blend.size(); i++) {
		Blend& b = c.blend[i];
		b.blend_left = MAX(0, b.blend_left - Math::abs(speed_scale * p_delta) / b.blend_time);
		if (Animation::is_less_or_equal_approx(b.blend_left, 0)) {
			to_erase.push_back(i);
			b.blend_left = CMP_EPSILON; // May want to play last frame.
		}
		// Note: There may be issues if an animation event triggers an animation change while this
		// blend is active, so it is best to use "deferred" calls instead of "immediate" for
		// animation events that can trigger new animations.
		_process_playback_data(b.data, p_delta, b.blend_left, false, false, false);
	}
	for (int i = to_erase.size() - 1; i >= 0; i--) {
		c.blend.remove_at(to_erase[i]);
	}
}

void AnimationPlayer::_blend_capture(double p_delta)
{
	blend_capture(p_delta * Math::abs(speed_scale));
}

void AnimationPlayer::queue(const String& p_name)
{
	if (!is_playing()) {
		play(p_name);
	}
	else {
		playback_queue.push_back(p_name);
	}
}

void AnimationPlayer::clear_queue() { playback_queue.clear(); }

void AnimationPlayer::play_backwards(const String& p_name, double p_custom_blend)
{
	play(p_name, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_with_markers_backwards(const String& p_name,
	const String& p_start_marker, const String& p_end_marker, double p_custom_blend)
{
	play_section_with_markers(p_name, p_start_marker, p_end_marker, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_backwards(
	const String& p_name, double p_start_time, double p_end_time, double p_custom_blend)
{
	play_section(p_name, p_start_time, p_end_time, p_custom_blend, -1, true);
}

void AnimationPlayer::play(
	const String& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
{
	if (auto_capture) {
		play_with_capture(p_name, auto_capture_duration, p_custom_blend, p_custom_scale, p_from_end,
			auto_capture_transition_type, auto_capture_ease_type);
	}
	else {
		_play(p_name, p_custom_blend, p_custom_scale, p_from_end);
	}
}

void AnimationPlayer::_play(
	const String& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
{
	play_section_with_markers(
		p_name, String(), String(), p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_section_with_markers(const String& p_name,
	const String& p_start_marker, const String& p_end_marker, double p_custom_blend,
	float p_custom_scale, bool p_from_end)
{
	String name = p_name;

	if (name == String()) {
		name = playback.assigned;
	}

	AnimationData* ad = animation_set.getptr(name);
	ERR_FAIL_NULL_MSG(ad, vformat("Animation not found: %s.", name));

	const Ref<Animation>& animation = ad->animation;

	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && !p_start_marker.is_empty(),
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(!p_start_marker.is_empty() && !animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker, name));
	ERR_FAIL_COND_MSG(!p_end_marker.is_empty() && !animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker, name));

	double start_time = !p_start_marker.is_empty() ? animation->get_marker_time(p_start_marker) : -1;
	double end_time = !p_end_marker.is_empty() ? animation->get_marker_time(p_end_marker) : -1;

	ERR_FAIL_COND_MSG(
		!p_start_marker.is_empty() && !p_end_marker.is_empty() && Animation::is_greater_approx(start_time, end_time),
		vformat("End marker %s is placed earlier than start marker %s in animation: %s.",
			p_end_marker, p_start_marker, name));

	if (!p_start_marker.is_empty() && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time start marker: %s is invalid in the section, so the "
							  "start of the animation: %s is used instead.",
			p_start_marker, playback.current.animation_name));
	}
	if (!p_end_marker.is_empty() && Animation::is_less_approx(end_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time end marker: %s is invalid in the section, so the end "
							  "of the animation: %s is used instead.",
			p_end_marker, playback.current.animation_name));
	}

	play_section(name, start_time, end_time, p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_with_capture(const String& p_name, double p_duration,
	double p_custom_blend, float p_custom_scale, bool p_from_end,
	Tween::TransitionType p_trans_type, Tween::EaseType p_ease_type)
{
	_capture(p_name, p_from_end, p_duration, p_trans_type, p_ease_type);
	_play(p_name, p_custom_blend, p_custom_scale, p_from_end);
}

bool AnimationPlayer::is_playing() const { return playing; }

String AnimationPlayer::get_current_animation() const
{
	return (is_playing() ? playback.assigned : String());
}

String AnimationPlayer::get_assigned_animation() const { return playback.assigned; }

void AnimationPlayer::pause() { _stop_internal(false, false); }

void AnimationPlayer::stop(bool p_keep_state) { _stop_internal(true, p_keep_state); }

void AnimationPlayer::set_speed_scale(float p_speed) { speed_scale = p_speed; }

float AnimationPlayer::get_speed_scale() const { return speed_scale; }

float AnimationPlayer::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * playback.current.speed_scale;
}

void AnimationPlayer::seek_internal(
	double p_time, bool p_update, bool p_update_only, bool p_is_internal_seek)
{
	if (!active) {
		return;
	}

	bool is_backward = Animation::is_less_approx(p_time, playback.current.pos);

	_check_immediately_after_start();

	playback.current.pos = p_time;
	if (!playback.current.is_enabled) {
		if (!playback.assigned.is_empty()) {
			AnimationData* ad = animation_set.getptr(playback.assigned);
			ERR_FAIL_NULL_MSG(ad, vformat("Animation not found: %s.", playback.assigned));
			playback.current.is_enabled = true;
			playback.current.animation_name = playback.assigned;
			playback.current.animation_length = ad->animation->get_length();
		}
		if (!playback.current.is_enabled) {
			return; // There is no animation.
		}
	}

	double start = playback.current.get_start_time();
	double end = playback.current.get_end_time();

	// Clamp the seek position.
	p_time = CLAMP(p_time, start, end);

	playback.seeked = true;
	playback.internal_seeked = p_is_internal_seek;

	if (p_update) {
		_process_animation(is_backward ? -0.0 : 0.0, p_update_only);
		playback.seeked =
			false; // If animation was proceeded here, no more seek in internal process.
	}
}

void AnimationPlayer::seek(double p_time, bool p_update, bool p_update_only)
{
	seek_internal(p_time, p_update, p_update_only);
}

void AnimationPlayer::advance(double p_time)
{
	_check_immediately_after_start();
	AnimationMixer::advance(p_time);
}

void AnimationPlayer::_check_immediately_after_start()
{
	if (playback.started) {
		_process_animation(
			0); // Force process current key for Discrete/Method/Audio/AnimationPlayback. Then,
				// started flag is cleared.
	}
}

bool AnimationPlayer::is_valid() const { return playback.current.is_enabled; }

double AnimationPlayer::get_current_animation_position() const
{
	ERR_FAIL_COND_V_MSG(
		!playback.current.is_enabled, 0, "AnimationPlayer has no current animation.");
	return playback.current.pos;
}

double AnimationPlayer::get_current_animation_length() const
{
	ERR_FAIL_COND_V_MSG(
		!playback.current.is_enabled, 0, "AnimationPlayer has no current animation.");
	return playback.current.animation_length;
}

void AnimationPlayer::set_section_with_markers(
	const String& p_start_marker, const String& p_end_marker)
{
	ERR_FAIL_COND_MSG(!playback.current.is_enabled, "AnimationPlayer has no current animation.");
	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && !p_start_marker.is_empty(),
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(
		!p_start_marker.is_empty() &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker,
			playback.current.animation_name));
	ERR_FAIL_COND_MSG(
		!p_end_marker.is_empty() &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker,
			playback.current.animation_name));
	double start_time =
		!p_start_marker.is_empty() ? animation_set[playback.current.animation_name].animation->get_marker_time(
							 p_start_marker)
					   : -1;
	double end_time =
		!p_end_marker.is_empty() ? animation_set[playback.current.animation_name].animation->get_marker_time(
						   p_end_marker)
					 : -1;
	if (!p_start_marker.is_empty() && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ONCE_ED(vformat("Marker %s time must be positive in animation: %s.",
			p_start_marker, playback.current.animation_name));
	}
	if (!p_end_marker.is_empty() && Animation::is_less_approx(end_time, 0)) {
		WARN_PRINT_ONCE_ED(vformat("Marker %s time must be positive in animation: %s.",
			p_end_marker, playback.current.animation_name));
	}
	set_section(start_time, end_time);
}

void AnimationPlayer::set_section(double p_start_time, double p_end_time)
{
	ERR_FAIL_COND_MSG(!playback.current.is_enabled, "AnimationPlayer has no current animation.");
	ERR_FAIL_COND_MSG(Animation::is_greater_or_equal_approx(p_start_time, 0) &&
						  Animation::is_greater_or_equal_approx(p_end_time, 0) &&
						  Animation::is_greater_or_equal_approx(p_start_time, p_end_time),
		vformat("Start time %f is greater than end time %f.", p_start_time, p_end_time));
	playback.current.start_time = p_start_time;
	playback.current.end_time = p_end_time;
	playback.current.pos = CLAMP(
		playback.current.pos, playback.current.get_start_time(), playback.current.get_end_time());
}

void AnimationPlayer::reset_section()
{
	playback.current.start_time = -1;
	playback.current.end_time = -1;
}

double AnimationPlayer::get_section_start_time() const
{
	ERR_FAIL_COND_V_MSG(!playback.current.is_enabled, playback.current.start_time,
		"AnimationPlayer has no current animation.");
	return playback.current.get_start_time();
}

double AnimationPlayer::get_section_end_time() const
{
	ERR_FAIL_COND_V_MSG(!playback.current.is_enabled, playback.current.end_time,
		"AnimationPlayer has no current animation.");
	return playback.current.get_end_time();
}

bool AnimationPlayer::has_section() const
{
	return Animation::is_greater_or_equal_approx(playback.current.start_time, 0) ||
		   Animation::is_greater_or_equal_approx(playback.current.end_time, 0);
}

void AnimationPlayer::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

String AnimationPlayer::get_autoplay() const { return autoplay; }

void AnimationPlayer::set_movie_quit_on_finish_enabled(bool p_enabled)
{
	movie_quit_on_finish = p_enabled;
}

bool AnimationPlayer::is_movie_quit_on_finish_enabled() const { return movie_quit_on_finish; }

void AnimationPlayer::set_clear_cache_on_stop_enabled(bool p_enabled)
{
	clear_cache_on_stop = p_enabled;
}

bool AnimationPlayer::is_clear_cache_on_stop_enabled() const { return clear_cache_on_stop; }

void AnimationPlayer::_animation_changed(const String& p_name)
{
	AnimationMixer::_animation_changed(p_name);
	if (playback.current.is_enabled && playback.current.animation_name == p_name &&
		animation_set.has(p_name)) {
		playback.current.animation_length = animation_set[p_name].animation->get_length();
	}
}

void AnimationPlayer::animation_set_next(const String& p_animation, const String& p_next)
{
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation), vformat("Animation not found: %s.", p_animation));
	animation_next_set[p_animation] = p_next;
}

String AnimationPlayer::animation_get_next(const String& p_animation) const
{
	const String* next = animation_next_set.getptr(p_animation);
	if (!next) {
		return String();
	}
	return *next;
}

void AnimationPlayer::set_default_blend_time(double p_default) { default_blend_time = p_default; }

double AnimationPlayer::get_default_blend_time() const { return default_blend_time; }

void AnimationPlayer::set_blend_time(
	const String& p_animation1, const String& p_animation2, double p_time)
{
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation1), vformat("Animation not found: %s.", p_animation1));
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation2), vformat("Animation not found: %s.", p_animation2));
	ERR_FAIL_COND_MSG(p_time < 0, "Blend time cannot be smaller than 0.");

	BlendKey bk;
	bk.from = p_animation1;
	bk.to = p_animation2;
	if (Math::is_zero_approx(p_time)) {
		blend_times.erase(bk);
	}
	else {
		blend_times[bk] = p_time;
	}
}

double AnimationPlayer::get_blend_time(
	const String& p_animation1, const String& p_animation2) const
{
	BlendKey bk;
	bk.from = p_animation1;
	bk.to = p_animation2;

	if (const double* blend_time = blend_times.getptr(bk)) {
		return *blend_time;
	}
	else {
		return 0;
	}
}

bool AnimationPlayer::is_auto_capture() const { return auto_capture; }

void AnimationPlayer::set_auto_capture_duration(double p_auto_capture_duration)
{
	auto_capture_duration = p_auto_capture_duration;
}

double AnimationPlayer::get_auto_capture_duration() const { return auto_capture_duration; }

void AnimationPlayer::set_auto_capture_transition_type(
	Tween::TransitionType p_auto_capture_transition_type)
{
	auto_capture_transition_type = p_auto_capture_transition_type;
}

Tween::TransitionType AnimationPlayer::get_auto_capture_transition_type() const
{
	return auto_capture_transition_type;
}

void AnimationPlayer::set_auto_capture_ease_type(Tween::EaseType p_auto_capture_ease_type)
{
	auto_capture_ease_type = p_auto_capture_ease_type;
}

Tween::EaseType AnimationPlayer::get_auto_capture_ease_type() const
{
	return auto_capture_ease_type;
}

#ifdef TOOLS_ENABLED
void AnimationPlayer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 &&
		(pf == "play" || pf == "play_backwards" || pf == "has_animation" || pf == "queue")) {
		for (const String& name : get_sorted_animation_list()) {
			r_options->push_back(String(name).quote());
		}
	}
	AnimationMixer::get_argument_options(p_function, p_idx, r_options);
}
#endif

void AnimationPlayer::_animation_removed(const String& p_name, const String& p_library)
{
	AnimationMixer::_animation_removed(p_name, p_library);

	const String& name =
		p_library == String() ? p_name : String(String(p_library) + "/" + String(p_name));

	if (!animation_set.has(name)) {
		return; // No need to update because not the one from the library being used.
	}

	_animation_set_cache_update();

	// Erase blends if needed
	LocalVector<BlendKey> to_erase;
	for (const KeyValue<BlendKey, double>& E : blend_times) {
		const BlendKey& bk = E.key;
		if (bk.from == name || bk.to == name) {
			to_erase.push_back(bk);
		}
	}

	for (const BlendKey& bk : to_erase) {
		blend_times.erase(bk);
	}
}

void AnimationPlayer::_rename_animation(const String& p_from_name, const String& p_to_name)
{
	// Rename autoplay or blends if needed.
	LocalVector<BlendKey> to_erase;
	HashMap<BlendKey, double, BlendKey> to_insert;
	for (const KeyValue<BlendKey, double>& E : blend_times) {
		BlendKey bk = E.key;
		BlendKey new_bk = bk;
		bool erase = false;
		if (bk.from == p_from_name) {
			new_bk.from = p_to_name;
			erase = true;
		}
		if (bk.to == p_from_name) {
			new_bk.to = p_to_name;
			erase = true;
		}

		if (erase) {
			to_erase.push_back(bk);
			to_insert[new_bk] = E.value;
		}
	}

	for (const BlendKey& bk : to_erase) {
		blend_times.erase(bk);
	}

	while (to_insert.size()) {
		blend_times[to_insert.begin()->key] = to_insert.begin()->value;
		to_insert.remove(to_insert.begin());
	}

	if (autoplay == p_from_name) {
		autoplay = p_to_name;
	}
}

AnimationPlayer::AnimationPlayer() {}

AnimationPlayer::~AnimationPlayer() {}

void AnimationPlayer::_capture(
	String const&, bool, double, Tween::TransitionType, Tween::EaseType)
{
}

void AnimationPlayer::_stop_internal(bool, bool) {}

void AnimationPlayer::play_section(String const&, double, double, double, float, bool) {}

void AnimationPlayer::set_current_animation(String const&) {}

void AnimationPlayer::set_assigned_animation(String const&) {}

bool AnimationPlayer::_blend_pre_process(
		double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	return false;
}

void AnimationPlayer::_blend_post_process() {}

// ==========================================================================
// Merged from scene/animation/root_motion_view.cpp
// ==========================================================================

#ifndef _3D_DISABLED

#include "core/config/engine.h"
// --- Inlined from scene/animation/root_motion_view.h ---
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/mesh.h"

class RootMotionView : public Node3D {
public:
	RenderInstance3D render_instance;

	Ref<ImmediateMesh> immediate;
	NodePath path;
	real_t cell_size = 1.0;
	real_t radius = 10.0;
	bool use_in_game = false;
	Color color = Color(0.5, 0.5, 1.0);
	bool first = true;
	bool zero_y = true;

	Ref<Material> immediate_material;

	Transform3D accumulated;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID &p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_animation_mixer(const NodePath &p_path);
	NodePath get_animation_mixer() const;

	void set_color(const Color &p_color);
	Color get_color() const;

	void set_cell_size(float p_size);
	float get_cell_size() const;

	void set_radius(float p_radius);
	float get_radius() const;

	void set_zero_y(bool p_zero_y);
	bool get_zero_y() const;

	AABB get_aabb() const;

	RootMotionView();
	~RootMotionView();
};

// --- End scene/animation/root_motion_view.h ---
#include "scene/animation/animation_mixer.h"
#include "scene/resources/material.h"

void RootMotionView::set_animation_mixer(const NodePath& p_path)
{
	path = p_path;
	first = true;
}

NodePath RootMotionView::get_animation_mixer() const { return path; }

void RootMotionView::set_color(const Color& p_color)
{
	color = p_color;
	first = true;
}

Color RootMotionView::get_color() const { return color; }

void RootMotionView::set_cell_size(float p_size)
{
	cell_size = p_size;
	first = true;
}

float RootMotionView::get_cell_size() const { return cell_size; }

void RootMotionView::set_radius(float p_radius)
{
	radius = p_radius;
	first = true;
}

float RootMotionView::get_radius() const { return radius; }

void RootMotionView::set_zero_y(bool p_zero_y) { zero_y = p_zero_y; }

bool RootMotionView::get_zero_y() const { return zero_y; }

AABB RootMotionView::get_aabb() const
{
	return AABB(Vector3(-radius, 0, -radius), Vector3(radius * 2, 0.001, radius * 2));
}


void RootMotionView::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void RootMotionView::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

RootMotionView::RootMotionView()
{
	if (Engine::is_editor_hint()) {
		set_process_internal(true);
	}
	immediate.instantiate();
	set_base(immediate->get_rid());
}

RootMotionView::~RootMotionView() { set_base(RID()); }

#endif // _3D_DISABLED

// ============================================================================
// Merged from scene/animation/tween.cpp
// ============================================================================

#include "scene/animation/animation_mixer.h"
// --- Inlined from scene/animation/easing_equations.h ---
#include "core/math/math_funcs.h"

/*
 * Derived from Robert Penner's easing equations: http://robertpenner.com/easing/
 *
 * Copyright (c) 2001 Robert Penner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

namespace Linear {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * t / d + b;
}
}; // namespace Linear

namespace Sine {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return -c * Math::cos(t / d * (Math::PI / 2)) + c + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::sin(t / d * (Math::PI / 2)) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	return -c / 2 * (Math::cos(Math::PI * t / d) - 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Sine

namespace Quint {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 5) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return c * (Math::pow(t / d - 1, 5) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 5) + b;
	}
	return c / 2 * (Math::pow(t - 2, 5) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quint

namespace Quart {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 4) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return -c * (Math::pow(t / d - 1, 4) - 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 4) + b;
	}
	return -c / 2 * (Math::pow(t - 2, 4) - 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quart

namespace Quad {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 2) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return -c * t * (t - 2) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 2) + b;
	}
	return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quad

namespace Expo {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}
	return c * Math::pow(2, 10 * (t / d - 1)) + b - c * 0.001;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	if (t == d) {
		return b + c;
	}
	return c * 1.001 * (-Math::pow(2, -10 * t / d) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	if (t == d) {
		return b + c;
	}

	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(2, 10 * (t - 1)) + b - c * 0.0005;
	}
	return c / 2 * 1.0005 * (-Math::pow(2, -10 * (t - 1)) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Expo

namespace Elastic {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	t /= d;
	if (t == 1) {
		return b + c;
	}

	t -= 1;
	float p = d * 0.3f;
	float a = c * Math::pow(2, 10 * t);
	float s = p / 4;

	return -(a * Math::sin((t * d - s) * (2 * Math::PI) / p)) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	t /= d;
	if (t == 1) {
		return b + c;
	}

	float p = d * 0.3f;
	float s = p / 4;

	return (c * Math::pow(2, -10 * t) * Math::sin((t * d - s) * (2 * Math::PI) / p) + c + b);
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	if ((t /= d / 2) == 2) {
		return b + c;
	}

	float p = d * (0.3f * 1.5f);
	float a = c;
	float s = p / 4;

	if (t < 1) {
		t -= 1;
		a *= Math::pow(2, 10 * t);
		return -0.5f * (a * Math::sin((t * d - s) * (2 * Math::PI) / p)) + b;
	}

	t -= 1;
	a *= Math::pow(2, -10 * t);
	return a * Math::sin((t * d - s) * (2 * Math::PI) / p) * 0.5f + c + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Elastic

namespace Cubic {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return c * t * t * t + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d - 1;
	return c * (t * t * t + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t /= d / 2;
	if (t < 1) {
		return c / 2 * t * t * t + b;
	}

	t -= 2;
	return c / 2 * (t * t * t + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Cubic

namespace Circ {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return -c * (Math::sqrt(1 - t * t) - 1) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d - 1;
	return c * Math::sqrt(1 - t * t) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t /= d / 2;
	if (t < 1) {
		return -c / 2 * (Math::sqrt(1 - t * t) - 1) + b;
	}

	t -= 2;
	return c / 2 * (Math::sqrt(1 - t * t) + 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Circ

namespace Bounce {
inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;

	if (t < (1 / 2.75f)) {
		return c * (7.5625f * t * t) + b;
	}

	if (t < (2 / 2.75f)) {
		t -= 1.5f / 2.75f;
		return c * (7.5625f * t * t + 0.75f) + b;
	}

	if (t < (2.5 / 2.75)) {
		t -= 2.25f / 2.75f;
		return c * (7.5625f * t * t + 0.9375f) + b;
	}

	t -= 2.625f / 2.75f;
	return c * (7.5625f * t * t + 0.984375f) + b;
}

inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c - out(d - t, 0, c, d) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return in(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return out(t * 2 - d, b + h, h, d);
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Bounce

namespace Back {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f;
	t /= d;

	return c * t * t * ((s + 1) * t - s) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f;
	t = t / d - 1;

	return c * (t * t * ((s + 1) * t + s) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f * 1.525f;
	t /= d / 2;

	if (t < 1) {
		return c / 2 * (t * t * ((s + 1) * t - s)) + b;
	}

	t -= 2;
	return c / 2 * (t * t * ((s + 1) * t + s) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Back

namespace Spring {
inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	real_t s = 1.0 - t;
	t = (Math::sin(t * Math::PI * (0.2 + 2.5 * t * t * t)) * Math::pow(s, (real_t)2.2) + t) * (1.0 + (1.2 * s));
	return c * t + b;
}

inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c - out(d - t, 0, c, d) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return in(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return out(t * 2 - d, b + h, h, d);
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Spring

// --- End scene/animation/easing_equations.h ---
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/animation.h"

#define CHECK_VALID()                                                                              \
	ERR_FAIL_COND_V_MSG(                                                                           \
		!valid, nullptr, "Tween invalid. Either finished or created outside scene tree.");         \
	ERR_FAIL_COND_V_MSG(                                                                           \
		started, nullptr, "Can't append to a Tween that has started. Use stop() first.");

Tween::interpolater Tween::interpolaters[Tween::TRANS_MAX][Tween::EASE_MAX] = {
	{&Linear::in, &Linear::in, &Linear::in, &Linear::in}, // Linear is the same for each easing.
	{&Sine::in, &Sine::out, &Sine::in_out, &Sine::out_in},
	{&Quint::in, &Quint::out, &Quint::in_out, &Quint::out_in},
	{&Quart::in, &Quart::out, &Quart::in_out, &Quart::out_in},
	{&Quad::in, &Quad::out, &Quad::in_out, &Quad::out_in},
	{&Expo::in, &Expo::out, &Expo::in_out, &Expo::out_in},
	{&Elastic::in, &Elastic::out, &Elastic::in_out, &Elastic::out_in},
	{&Cubic::in, &Cubic::out, &Cubic::in_out, &Cubic::out_in},
	{&Circ::in, &Circ::out, &Circ::in_out, &Circ::out_in},
	{&Bounce::in, &Bounce::out, &Bounce::in_out, &Bounce::out_in},
	{&Back::in, &Back::out, &Back::in_out, &Back::out_in},
	{&Spring::in, &Spring::out, &Spring::in_out, &Spring::out_in},
};

void Tween::_start_tweeners()
{
	if (tweeners.is_empty()) {
		dead = true;
		ERR_FAIL_MSG("Tween without commands, aborting.");
	}

	for (Ref<Tweener>& tweener : tweeners[current_step]) {
		tweener->start();
	}
}

void Tween::_stop_internal(bool p_reset)
{
	running = false;
	if (p_reset) {
		started = false;
		dead = false;
		total_time = 0;
	}
}

Ref<Tweener> Tween::tween_interval(double p_time)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_INTERVAL, p_time);
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_subtween(Tween* rp_subtween)
{
	CHECK_VALID();
	ERR_FAIL_NULL_V(rp_subtween, nullptr);

	Ref<Tweener> tweener;
	tweener.instantiate(Ref<Tween>(rp_subtween));

	// Remove the tween from its parent tree, if it has one.
	// If the user created this tween without a parent tree attached,
	// then this step isn't necessary.
	if (tweener->subtween->is_in_tree) {
		SceneTree::remove_tween(tweener->subtween);
		tweener->subtween->is_in_tree = false;
	}
	subtweens.push_back(rp_subtween);
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_callback(void (*p_callback)())
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_CALLBACK);
	tweener->callback = p_callback;
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_property(const Vector<String>& p_property, double p_duration)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_PROPERTY, p_duration);
	tweener->property = p_property;
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_method(double p_duration)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_METHOD, p_duration);
	append(tweener);
	return tweener;
}

void Tween::append(Ref<Tweener> p_tweener)
{
	p_tweener->set_tween(this);

	if (parallel_enabled) {
		current_step = MAX(current_step, 0);
	}
	else {
		current_step++;
	}
	parallel_enabled = default_parallel;

	tweeners.resize(current_step + 1);
	tweeners[current_step].push_back(p_tweener);
}

void Tween::stop() { _stop_internal(true); }

void Tween::pause() { _stop_internal(false); }

void Tween::play()
{
	ERR_FAIL_COND_MSG(!valid, "Tween invalid. Either finished or created outside scene tree.");
	ERR_FAIL_COND_MSG(dead, "Can't play finished Tween, use stop() first to reset its state.");
	running = true;
}

void Tween::kill()
{
	running = false; // For the sake of is_running().
	valid = false;
	dead = true;

	// Kill all subtweens of this tween.
	for (Ref<Tween>& st : subtweens) {
		st->kill();
	}
}

bool Tween::has_tweeners() const { return !tweeners.is_empty(); }

bool Tween::is_running() { return running; }

bool Tween::is_valid() { return valid; }

void Tween::clear()
{
	valid = false;
	tweeners.clear();
}

Tween* Tween::set_process_mode(TweenProcessMode p_mode)
{
	process_mode = p_mode;
	return this;
}

Tween::TweenProcessMode Tween::get_process_mode() const { return process_mode; }

Tween* Tween::set_pause_mode(TweenPauseMode p_mode)
{
	pause_mode = p_mode;
	return this;
}

Tween::TweenPauseMode Tween::get_pause_mode() const { return pause_mode; }

Tween* Tween::set_ignore_time_scale(bool p_ignore)
{
	ignore_time_scale = p_ignore;
	return this;
}

bool Tween::is_ignoring_time_scale() const { return ignore_time_scale; }

Tween* Tween::set_parallel(bool p_parallel)
{
	default_parallel = p_parallel;
	parallel_enabled = p_parallel;
	return this;
}

Tween* Tween::set_loops(int p_loops)
{
	loops = p_loops;
	return this;
}

int Tween::get_loops_left() const
{
	if (loops <= 0) {
		return -1; // Infinite loop.
	}
	else {
		return loops - loops_done;
	}
}

Tween* Tween::set_speed_scale(float p_speed)
{
	speed_scale = p_speed;
	return this;
}

Tween* Tween::set_trans(TransitionType p_trans)
{
	default_transition = p_trans;
	return this;
}

Tween::TransitionType Tween::get_trans() const { return default_transition; }

Tween* Tween::set_ease(EaseType p_ease)
{
	default_ease = p_ease;
	return this;
}

Tween::EaseType Tween::get_ease() const { return default_ease; }

Tween* Tween::parallel()
{
	parallel_enabled = true;
	return this;
}

Tween* Tween::chain()
{
	parallel_enabled = false;
	return this;
}

bool Tween::custom_step(double p_delta)
{
	ERR_FAIL_COND_V_MSG(in_step, true, "Can't call custom_step() during another Tween step.");

	bool r = running;
	running = true;
	bool ret = step(p_delta);
	running = running && r; // Running might turn false when Tween finished.
	return ret;
}

bool Tween::step(double p_delta) { return true; }

bool Tween::can_process(bool p_tree_paused) const
{
	if (is_bound && pause_mode == TWEEN_PAUSE_BOUND) {
		Node* node = get_bound_node();
		if (node) {
			return node->is_inside_tree() && node->can_process();
		}
	}

	return !p_tree_paused || pause_mode == TWEEN_PAUSE_PROCESS;
}

double Tween::get_total_time() const { return total_time; }

real_t Tween::run_equation(TransitionType p_trans_type, EaseType p_ease_type, real_t p_time,
	real_t p_initial, real_t p_delta, real_t p_duration)
{
	if (p_duration == 0) {
		// Special case to avoid dividing by 0 in equations.
		return p_initial + p_delta;
	}

	interpolater func = interpolaters[p_trans_type][p_ease_type];
	return func(p_time, p_initial, p_delta, p_duration);
}

Tween* Tween::bind_node(const Node* rp_node)
{
	ERR_FAIL_NULL_V(rp_node, this);
	bound_node = const_cast<Node*>(rp_node);
	is_bound = true;
	return this;
}

Node* Tween::get_bound_node() const { return bound_node; }

String Tween::_to_string() { return String(); }

Tween::Tween() { ERR_FAIL_MSG("Tween can't be created directly. Use create_tween() method."); }

Tween::Tween(bool p_in_tree)
{
	is_in_tree = p_in_tree;
	valid = true;
}

Tween::~Tween() = default;

Ref<Tweener> Tweener::from(double p_value)
{
	initial_val = p_value;
	do_continue = false;
	return this;
}

Ref<Tweener> Tweener::from_current()
{
	do_continue = false;
	return this;
}

Ref<Tweener> Tweener::as_relative()
{
	relative = true;
	return this;
}

Ref<Tweener> Tweener::set_trans(Tween::TransitionType p_trans)
{
	trans_type = p_trans;
	return this;
}

Ref<Tweener> Tweener::set_ease(Tween::EaseType p_ease)
{
	ease_type = p_ease;
	return this;
}

Ref<Tweener> Tweener::set_custom_interpolator(real_t (*p_interpolator)(real_t))
{
	custom_interpolator = p_interpolator;
	return this;
}

Ref<Tweener> Tweener::set_delay(double p_delay)
{
	delay = p_delay;
	return this;
}

Ref<Tweener> Tweener::set_timeout(double p_timeout)
{
	timeout = p_timeout;
	return this;
}

void Tweener::set_tween(const Ref<Tween>& p_tween)
{
	tween = p_tween.ptr();
	if (type == TWEENER_PROPERTY || type == TWEENER_METHOD) {
		if (trans_type == Tween::TRANS_MAX && p_tween.is_valid()) {
			trans_type = p_tween->get_trans();
		}
		if (ease_type == Tween::EASE_MAX && p_tween.is_valid()) {
			ease_type = p_tween->get_ease();
		}
	}
}

Ref<Tween> Tweener::_get_tween() { return Ref<Tween>(tween); }

void Tweener::_finish() { finished = true; }

void Tweener::start()
{
	elapsed_time = 0;
	finished = false;

	switch (type) {
	case TWEENER_SUBTWEEN: {
		if (subtween.is_null()) {
			_finish();
			return;
		}
		// Reset the subtween.
		subtween->stop();

		// It's possible that a subtween could be killed before it is started;
		// if so, we just want to skip it entirely.
		if (subtween->is_valid()) {
			subtween->play();
		}
		else {
			_finish();
		}
	} break;
	case TWEENER_AWAIT: {
		received = false;
	} break;
	case TWEENER_INTERVAL:
	case TWEENER_CALLBACK:
	case TWEENER_PROPERTY:
	case TWEENER_METHOD:
		break;
	}
}

bool Tweener::step(double& r_delta)
{
	switch (type) {
	case TWEENER_INTERVAL: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;

		if (elapsed_time < duration) {
			r_delta = 0;
			return true;
		}
		else {
			r_delta = elapsed_time - duration;
			_finish();
			return false;
		}
	}
	case TWEENER_SUBTWEEN: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;

		if (elapsed_time < delay) {
			r_delta = 0;
			return true;
		}

		if (subtween.is_null() || !subtween->step(r_delta)) {
			r_delta = elapsed_time - delay - (subtween.is_valid() ? subtween->get_total_time() : 0.0);
			_finish();
			return false;
		}

		r_delta = 0;
		return true;
	}
	case TWEENER_CALLBACK: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;
		if (elapsed_time < delay) {
			r_delta = 0;
			return true;
		}

		if (callback) {
			callback();
		}
		r_delta = elapsed_time - delay;
		_finish();
		return false;
	}
	case TWEENER_PROPERTY:
	case TWEENER_METHOD:
	case TWEENER_AWAIT: {
		return true;
	}
	}
	return false;
}

Tweener::Tweener()
{
	ERR_FAIL_MSG("Tweener can't be created directly. Use the tween_*() methods in Tween.");
}

Tweener::Tweener(Type p_type) : type(p_type) {}

Tweener::Tweener(Type p_type, double p_duration) : type(p_type), duration(p_duration) {}

Tweener::Tweener(const Ref<Tween>& p_subtween) : subtween(p_subtween), type(TWEENER_SUBTWEEN) {}
