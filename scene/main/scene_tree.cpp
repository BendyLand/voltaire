/**************************************************************************/
/*  scene_tree.cpp                                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

STATIC_ASSERT_INCOMPLETE_TYPE(class, RenderingServer);

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
#include "servers/rendering/rendering_server.h"

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
// This should be called once per physics tick, to make sure the transform previous and current
// is kept up to date on the few Node3Ds that are using client side physics interpolation.
void SceneTree::ClientPhysicsInterpolation::physics_process()
{
	for (SelfList<Node3D>* E = _node_3d_list.first(); E;) {
		Node3D* node_3d = E->self();

		SelfList<Node3D>* current = E;

		// Get the next element here BEFORE we potentially delete one.
		E = E->next();

		// This will return false if the Node3D has timed out ..
		// i.e. if get_global_transform_interpolated() has not been called
		// for a few seconds, we can delete from the list to keep processing
		// to a minimum.
		if (!node_3d->update_client_physics_interpolation_data()) {
			_node_3d_list.remove(current);
		}
	}
}
#endif // _3D_DISABLED

bool SceneTree::_physics_interpolation_enabled = false;
bool SceneTree::_physics_interpolation_enabled_in_project = false;

SceneTreeGroup* SceneTree::add_to_group(const StringName& p_group, Node* p_node)
{
	_THREAD_SAFE_METHOD_

	HashMap<StringName, SceneTreeGroup>::Iterator E = group_map.find(p_group);
	if (!E) {
		E = group_map.insert(p_group, SceneTreeGroup());
	}

	ERR_FAIL_COND_V_MSG(
		E->value.nodes.has(p_node), &E->value, "Already in group: " + p_group + ".");
	E->value.nodes.push_back(p_node);
	E->value.changed = true;
	return &E->value;
}

void SceneTree::remove_from_group(const StringName& p_group, Node* p_node)
{
	_THREAD_SAFE_METHOD_

	HashMap<StringName, SceneTreeGroup>::Iterator E = group_map.find(p_group);
	ERR_FAIL_COND(!E);

	E->value.nodes.erase(p_node);
	if (E->value.nodes.is_empty()) {
		group_map.remove(E);
	}
}

bool SceneTree::is_accessibility_enabled() const
{
	if (!DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER)) {
		return false;
	}

	AccessibilityServerEnums::AccessibilityMode accessibility_mode =
		AccessibilityServer::get_singleton()->get_mode();
	int screen_reader_active = DisplayServer::get_singleton()->accessibility_screen_reader_active();
	if ((accessibility_mode ==
			AccessibilityServerEnums::AccessibilityMode::ACCESSIBILITY_DISABLED) ||
		((accessibility_mode == AccessibilityServerEnums::AccessibilityMode::ACCESSIBILITY_AUTO) &&
			(screen_reader_active != 1))) {
		return false;
	}
	return true;
}

bool SceneTree::is_accessibility_supported() const
{
	if (!DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_ACCESSIBILITY_SCREEN_READER)) {
		return false;
	}

	AccessibilityServerEnums::AccessibilityMode accessibility_mode =
		AccessibilityServer::get_singleton()->get_mode();
	if (accessibility_mode == AccessibilityServerEnums::AccessibilityMode::ACCESSIBILITY_DISABLED) {
		return false;
	}
	return true;
}

void SceneTree::_accessibility_force_update() { accessibility_force_update = true; }

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

Window* SceneTree::get_root() const { return root; }

void SceneTree::notify_group(const StringName& p_group, int p_notification)
{
	notify_group_flags(GROUP_CALL_DEFAULT, p_group, p_notification);
}

void SceneTree::set_physics_interpolation_enabled(bool p_enabled)
{
	// This version is for use in editor.
	_physics_interpolation_enabled_in_project = p_enabled;

	// We never want interpolation in the editor.
	if (Engine::get_singleton()->is_editor_hint()) {
		p_enabled = false;
	}

	if (p_enabled == _physics_interpolation_enabled) {
		return;
	}

	_physics_interpolation_enabled = p_enabled;
	RenderingServer::get_singleton()->set_physics_interpolation_enabled(p_enabled);

	get_scene_tree_fti().set_enabled(get_root(), p_enabled);

	// Perform an auto reset on the root node for convenience for the user.
	if (root) {
		root->reset_physics_interpolation();
	}
}

#ifndef _3D_DISABLED
void SceneTree::client_physics_interpolation_add_node_3d(SelfList<Node3D>* p_elem)
{
	// This ensures that _update_physics_interpolation_data() will be called at least once every
	// physics tick, to ensure the previous and current transforms are kept up to date.
	_client_physics_interpolation._node_3d_list.add(p_elem);
}

void SceneTree::client_physics_interpolation_remove_node_3d(SelfList<Node3D>* p_elem)
{
	_client_physics_interpolation._node_3d_list.remove(p_elem);
}
#endif

void SceneTree::iteration_prepare()
{
	if (_physics_interpolation_enabled) {
		// Make sure any pending transforms from the last tick / frame
		// are flushed before pumping the interpolation prev and currents.
		flush_transform_notifications();
		get_scene_tree_fti().tick_update();
		RenderingServer::get_singleton()->tick();
	}
}

void SceneTree::iteration_end()
{
	// When physics interpolation is active, we want all pending transforms
	// to be flushed to the RenderingServer before finishing a physics tick.
	if (_physics_interpolation_enabled) {
		flush_transform_notifications();

#ifndef _3D_DISABLED
		// Any objects performing client physics interpolation
		// should be given an opportunity to keep their previous transforms
		// up to date.
		_client_physics_interpolation.physics_process();
#endif
	}
}

void SceneTree::process_tweens(double p_delta, bool p_physics)
{
	_THREAD_SAFE_METHOD_
	// This methods works similarly to how SceneTreeTimers are handled.
	const List<Ref<Tween>>::Element* L = tweens.back();
	const double unscaled_delta = Engine::get_singleton()->get_process_step();

	for (List<Ref<Tween>>::Element* E = tweens.front(); E;) {
		List<Ref<Tween>>::Element* N = E->next();
		Ref<Tween>& tween = E->get();

		// Don't process if paused or process mode doesn't match.
		if (!tween->can_process(paused) ||
			(p_physics == (tween->get_process_mode() == Tween::TWEEN_PROCESS_IDLE))) {
			if (E == L) {
				break;
			}
			E = N;
			continue;
		}

		if (!tween->step(tween->is_ignoring_time_scale() ? unscaled_delta : p_delta)) {
			tween->clear();
			tweens.erase(E);
		}
		if (E == L) {
			break;
		}
		E = N;
	}
}

void SceneTree::finalize()
{
	_flush_delete_queue();

	_flush_ugc();

	if (root) {
		root->_set_tree(nullptr);
		root->_propagate_after_exit_tree();
		memdelete(root); // delete root
		root = nullptr;

		// In case deletion of some objects was queued when destructing the `root`.
		// E.g. if `queue_free()` was called for some node outside the tree when handling
		// NOTIFICATION_PREDELETE for some node in the tree.
		_flush_delete_queue();
	}

	MainLoop::finalize();

	// Cleanup timers.
	for (Ref<SceneTreeTimer>& timer : timers) {
		timer->release_connections();
	}
	timers.clear();

	// Cleanup tweens.
	for (Ref<Tween>& tween : tweens) {
		tween->clear();
	}
	tweens.clear();
}

void SceneTree::quit(int p_exit_code)
{
	_THREAD_SAFE_METHOD_

	OS::get_singleton()->set_exit_code(p_exit_code);
	_quit = true;
}

void SceneTree::_main_window_close()
{
	if (accept_quit) {
		_quit = true;
	}
}

void SceneTree::_main_window_go_back()
{
	if (quit_on_go_back) {
		_quit = true;
	}
}

void SceneTree::_main_window_focus_in()
{
	Input* id = Input::get_singleton();
	if (id) {
		id->ensure_touch_mouse_raised();
	}
}

void SceneTree::_notification(int p_notification)
{
	if (!get_root()) {
		return;
	}

	switch (p_notification) {
	case NOTIFICATION_TRANSLATION_CHANGED: {
		get_root()->propagate_notification(p_notification);
	} break;

	case NOTIFICATION_OS_MEMORY_WARNING:
	case NOTIFICATION_OS_IME_UPDATE:
	case NOTIFICATION_WM_ABOUT:
	case NOTIFICATION_CRASH:
	case NOTIFICATION_APPLICATION_RESUMED:
	case NOTIFICATION_APPLICATION_PAUSED:
	case NOTIFICATION_APPLICATION_PIP_MODE_ENTERED:
	case NOTIFICATION_APPLICATION_PIP_MODE_EXITED: {
		// Pass these to nodes, since they are mirrored.
		get_root()->propagate_notification(p_notification);
	} break;

	case NOTIFICATION_APPLICATION_FOCUS_IN:
	case NOTIFICATION_APPLICATION_FOCUS_OUT: {
		if (Input::get_singleton()) {
			Input::get_singleton()->application_focused =
				p_notification == NOTIFICATION_APPLICATION_FOCUS_IN;

			// `release_pressed_events()` already preserves joypad state when the
			// unfocused joypad setting is disabled, but keyboard state still needs
			// to be released after focus loss.
			Input::get_singleton()->release_pressed_events();
		}

		// Pass these to nodes, since they are mirrored.
		get_root()->propagate_notification(p_notification);
	} break;
	}
}

bool SceneTree::is_auto_accept_quit() const { return accept_quit; }

void SceneTree::set_auto_accept_quit(bool p_enable) { accept_quit = p_enable; }

bool SceneTree::is_quit_on_go_back() const { return quit_on_go_back; }

void SceneTree::set_quit_on_go_back(bool p_enable) { quit_on_go_back = p_enable; }

#ifdef DEBUG_ENABLED
void SceneTree::set_debug_collisions_hint(bool p_enabled) { debug_collisions_hint = p_enabled; }

bool SceneTree::is_debugging_collisions_hint() const { return debug_collisions_hint; }

void SceneTree::set_debug_paths_hint(bool p_enabled) { debug_paths_hint = p_enabled; }

bool SceneTree::is_debugging_paths_hint() const { return debug_paths_hint; }

void SceneTree::set_debug_navigation_hint(bool p_enabled) { debug_navigation_hint = p_enabled; }

bool SceneTree::is_debugging_navigation_hint() const { return debug_navigation_hint; }
#endif

void SceneTree::set_debug_collisions_color(const Color& p_color)
{
	debug_collisions_color = p_color;
}

Color SceneTree::get_debug_collisions_color() const { return debug_collisions_color; }

void SceneTree::set_debug_collision_contact_color(const Color& p_color)
{
	debug_collision_contact_color = p_color;
}

Color SceneTree::get_debug_collision_contact_color() const { return debug_collision_contact_color; }

void SceneTree::set_debug_paths_color(const Color& p_color) { debug_paths_color = p_color; }

Color SceneTree::get_debug_paths_color() const { return debug_paths_color; }

void SceneTree::set_debug_paths_width(float p_width) { debug_paths_width = p_width; }

float SceneTree::get_debug_paths_width() const { return debug_paths_width; }

Ref<Material> SceneTree::get_debug_paths_material()
{
	_THREAD_SAFE_METHOD_

	if (debug_paths_material.is_valid()) {
		return debug_paths_material;
	}

	Ref<StandardMaterial3D> _debug_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	_debug_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	_debug_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	_debug_material->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	_debug_material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	_debug_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	_debug_material->set_albedo(get_debug_paths_color());

	debug_paths_material = _debug_material;

	return debug_paths_material;
}

Ref<Material> SceneTree::get_debug_collision_material()
{
	_THREAD_SAFE_METHOD_

	if (collision_material.is_valid()) {
		return collision_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 1);
	material->set_cull_mode(StandardMaterial3D::CULL_BACK);
	material->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
	material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

	collision_material = material;

	return collision_material;
}

void SceneTree::set_pause(bool p_enabled)
{
	ERR_FAIL_COND_MSG(!Thread::is_main_thread(), "Pause can only be set from the main thread.");
	ERR_FAIL_COND_MSG(suspended, "Pause state cannot be modified while suspended.");

	if (p_enabled == paused) {
		return;
	}

	paused = p_enabled;

#ifndef PHYSICS_3D_DISABLED
	PhysicsServer3D::get_singleton()->set_active(!p_enabled);
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	PhysicsServer2D::get_singleton()->set_active(!p_enabled);
#endif // PHYSICS_2D_DISABLED
	if (get_root()) {
		get_root()->_propagate_pause_notification(p_enabled);
	}
}

bool SceneTree::is_paused() const { return paused; }

void SceneTree::set_suspend(bool p_enabled)
{
	ERR_FAIL_COND_MSG(!Thread::is_main_thread(), "Suspend can only be set from the main thread.");

	if (p_enabled == suspended) {
		return;
	}

	suspended = p_enabled;

	Engine::get_singleton()->set_freeze_time_scale(p_enabled);

#ifndef PHYSICS_3D_DISABLED
	PhysicsServer3D::get_singleton()->set_active(!p_enabled && !paused);
#endif // PHYSICS_3D_DISABLED
#ifndef PHYSICS_2D_DISABLED
	PhysicsServer2D::get_singleton()->set_active(!p_enabled && !paused);
#endif // PHYSICS_2D_DISABLED
	if (get_root()) {
		get_root()->_propagate_suspend_notification(p_enabled);
	}
}

bool SceneTree::is_suspended() const { return suspended; }

void SceneTree::_process_groups_thread(uint32_t p_index, bool p_physics)
{
	Node::current_process_thread_group = local_process_group_cache[p_index]->owner;
	_process_group(local_process_group_cache[p_index], p_physics);
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
	ProcessGroup* pg = (ProcessGroup*)p_node->data.process_group;
	ERR_FAIL_NULL(pg);
	ERR_FAIL_COND(pg->removed);
	pg->removed = true;
	pg->owner = nullptr;
	p_node->data.process_group = nullptr;
	process_groups_dirty = true;
}

void SceneTree::_add_process_group(Node* p_node)
{
	_THREAD_SAFE_METHOD_
	ERR_FAIL_NULL(p_node);

	ProcessGroup* pg = memnew(ProcessGroup);

	pg->owner = p_node;
	p_node->data.process_group = pg;

	process_groups.push_back(pg);

	process_groups_dirty = true;
}

void SceneTree::_remove_node_from_process_group(Node* p_node, Node* p_owner)
{
	_THREAD_SAFE_METHOD_
	ProcessGroup* pg =
		p_owner ? (ProcessGroup*)p_owner->data.process_group : &default_process_group;

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
	ProcessGroup* pg =
		p_owner ? (ProcessGroup*)p_owner->data.process_group : &default_process_group;

	if (p_node->is_processing() || p_node->is_processing_internal()) {
		pg->nodes.push_back(p_node);
		pg->node_order_dirty = true;
	}

	if (p_node->is_physics_processing() || p_node->is_physics_processing_internal()) {
		pg->physics_nodes.push_back(p_node);
		pg->physics_node_order_dirty = true;
	}
}

int64_t SceneTree::get_frame() const { return current_frame; }

bool SceneTree::has_group(const StringName& p_identifier) const
{
	_THREAD_SAFE_METHOD_
	return group_map.has(p_identifier);
}

int SceneTree::get_node_count_in_group(const StringName& p_group) const
{
	_THREAD_SAFE_METHOD_
	HashMap<StringName, SceneTreeGroup>::ConstIterator E = group_map.find(p_group);
	if (!E) {
		return 0;
	}

	return E->value.nodes.size();
}

Node* SceneTree::get_first_node_in_group(const StringName& p_group)
{
	_THREAD_SAFE_METHOD_
	HashMap<StringName, SceneTreeGroup>::Iterator E = group_map.find(p_group);
	if (!E) {
		return nullptr; // No group.
	}

	_update_group_order(E->value); // Update order just in case.

	if (E->value.nodes.is_empty()) {
		return nullptr;
	}

	return E->value.nodes[0];
}

Vector<Node*> SceneTree::get_nodes_in_group(const StringName& p_group)
{
	_THREAD_SAFE_METHOD_
	HashMap<StringName, SceneTreeGroup>::Iterator E = group_map.find(p_group);
	if (!E) {
		return {};
	}

	_update_group_order(E->value); // update order just in case
	int nc = E->value.nodes.size();
	if (nc == 0) {
		return {};
	}

	return E->value.nodes;
}

int SceneTree::get_node_count() const { return nodes_in_tree_count; }

void SceneTree::set_edited_scene_root(Node* p_node)
{
#ifdef TOOLS_ENABLED
	edited_scene_root = p_node;
#endif
}

Node* SceneTree::get_edited_scene_root() const
{
#ifdef TOOLS_ENABLED
	return edited_scene_root;
#else
	return nullptr;
#endif
}

void SceneTree::set_current_scene(Node* p_scene)
{
	ERR_FAIL_COND_MSG(
		!Thread::is_main_thread(), "Changing scene can only be done from the main thread.");
	ERR_FAIL_COND(p_scene && p_scene->get_parent() != root);
	current_scene = p_scene;
}

Node* SceneTree::get_current_scene() const { return current_scene; }

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
	Node* new_scene = rp_scene->instantiate();
	ERR_FAIL_NULL_V(new_scene, ERR_CANT_CREATE);
	return change_scene_to_node(new_scene);
}

Error SceneTree::reload_current_scene()
{
	ERR_FAIL_COND_V_MSG(!Thread::is_main_thread(), ERR_INVALID_PARAMETER,
		"Reloading scene can only be done from the main thread.");
	ERR_FAIL_NULL_V(current_scene, ERR_UNCONFIGURED);
	String fname = current_scene->get_scene_file_path();
	return change_scene_to_file(fname);
}

void SceneTree::unload_current_scene()
{
	ERR_FAIL_COND_MSG(!Thread::is_main_thread(),
		"Unloading the current scene can only be done from the main thread.");
	if (current_scene) {
		memdelete(current_scene);
		current_scene = nullptr;
	}
}

void SceneTree::add_current_scene(Node* p_current)
{
	ERR_FAIL_COND_MSG(
		!Thread::is_main_thread(), "Adding a current scene can only be done from the main thread.");
	current_scene = p_current;
	root->add_child(p_current);
}

SceneTreeTimer* SceneTree::create_timer(
	double p_delay_sec, bool p_process_always, bool p_process_in_physics, bool p_ignore_time_scale)
{
	_THREAD_SAFE_METHOD_
	Ref<SceneTreeTimer> stt;
	stt.instantiate();
	stt->set_process_always(p_process_always);
	stt->set_time_left(p_delay_sec);
	stt->set_process_in_physics(p_process_in_physics);
	stt->set_ignore_time_scale(p_ignore_time_scale);
	timers.push_back(stt);
	return stt.ptr();
}

Tween* SceneTree::create_tween()
{
	_THREAD_SAFE_METHOD_
	Ref<Tween> tween;
	tween.instantiate(this);
	tweens.push_back(tween);
	return tween.ptr();
}

void SceneTree::remove_tween(const Ref<Tween>& p_tween)
{
	_THREAD_SAFE_METHOD_
	for (List<Ref<Tween>>::Element* E = tweens.back(); E; E = E->prev()) {
		if (E->get() == p_tween) {
			E->erase();
			break;
		}
	}
}

MultiplayerAPI* SceneTree::get_multiplayer(const NodePath& p_for_path) const
{
	ERR_FAIL_COND_V_MSG(!Thread::is_main_thread(), Ref<MultiplayerAPI>().ptr(),
		"Multiplayer can only be manipulated from the main thread.");
	if (p_for_path.is_empty()) {
		return multiplayer.ptr();
	}

	const Vector<StringName> tnames = p_for_path.get_names();
	const StringName* nptr = tnames.ptr();
	for (const KeyValue<NodePath, Ref<MultiplayerAPI>>& E : custom_multiplayers) {
		const Vector<StringName> snames = E.key.get_names();
		if (tnames.size() < snames.size()) {
			continue;
		}
		const StringName* sptr = snames.ptr();
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

	return multiplayer.ptr();
}

void SceneTree::set_multiplayer_poll_enabled(bool p_enabled)
{
	ERR_FAIL_COND_MSG(
		!Thread::is_main_thread(), "Multiplayer can only be manipulated from the main thread.");
	multiplayer_poll = p_enabled;
}

bool SceneTree::is_multiplayer_poll_enabled() const { return multiplayer_poll; }

SceneTree* SceneTree::singleton = nullptr;

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

#ifdef TOOLS_ENABLED

#endif

void SceneTree::set_disable_node_threading(bool p_disable) { node_threading_disabled = p_disable; }


