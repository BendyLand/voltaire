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
#include "scene/resources/world_2d.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"

#ifndef _3D_DISABLED
#include "scene/3d/node_3d.h"
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
#include "core/io/stream_peer.h"
#include "core/os/os.h"
#include "core/os/thread.h"
// --- Inlined from scene/main/http_request.h ---
#include "core/io/http_client.h"
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
#include "scene/main/scene_tree.h"
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
#include "timer.h"

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

#include "scene/gui/popup_menu.h"
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
#include "scene/resources/atlas_texture.h"
#include "scene/resources/font.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/style_box.h"
#include "scene/resources/world_2d.h"
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
#include "scene/resources/world_2d.h"
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
#include "scene/gui/label.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/scroll_container.h"
#include "scene/main/canvas_item.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/mesh.h"
#include "scene/resources/text_line.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_enums.h"

// 2D.
#include "scene/2d/node_2d.h"
#include "scene/resources/world_2d.h"

#ifndef _3D_DISABLED
#include "scene/3d/camera_3d.h"
#include "scene/3d/node_3d.h"
#endif // _3D_DISABLED

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef XR_DISABLED
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_interface.h"
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
#include "scene/theme/theme_db.h"
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


