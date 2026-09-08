/**************************************************************************/
/*  scene_tree_fti.cpp                                                    */
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

#ifndef _3D_DISABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/transform_interpolator.h"
#include "core/os/os.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene_tree_fti.h"

#ifdef GODOT_SCENE_TREE_FTI_VERIFY
#include "scene/main/scene_tree_fti_tests.h"
#endif

#ifdef DEV_ENABLED

// Uncomment this to enable some slow extra DEV_ENABLED
// checks to ensure there aren't more than one object added to the lists.
// #define GODOT_SCENE_TREE_FTI_EXTRA_CHECKS

// Uncomment this to regularly print the tree that is being interpolated.
// #define GODOT_SCENE_TREE_FTI_PRINT_TREE

#endif

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
	if (data.enabled == p_enabled) {
		return;
	}
	MutexLock lock(data.mutex);

	data.tick_xform_list[0].clear();
	data.tick_xform_list[1].clear();

	data.frame_xform_list.clear();
	data.frame_xform_list_forced.clear();

	data.tick_property_list[0].clear();
	data.tick_property_list[1].clear();

	data.frame_property_list.clear();
	data.request_reset_list.clear();

	_clear_depth_lists();

	// Node3D flags must be reset.
	if (p_root) {
		_reset_flags(p_root);
	}

	data.enabled = p_enabled;
}

void SceneTreeFTI::tick_update()
{
	if (!data.enabled) {
		return;
	}
	MutexLock lock(data.mutex);

	_update_request_resets();

	uint32_t curr_mirror = data.mirror;
	uint32_t prev_mirror = curr_mirror ? 0 : 1;

	LocalVector<Node3D*>& curr = data.tick_xform_list[curr_mirror];
	LocalVector<Node3D*>& prev = data.tick_xform_list[prev_mirror];

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

	LocalVector<Node3D*>& curr_prop = data.tick_property_list[curr_mirror];
	LocalVector<Node3D*>& prev_prop = data.tick_property_list[prev_mirror];

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
				data.frame_property_list.erase_unordered(s);

#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
				DEV_CHECK_ONCE(data.frame_property_list.find(s) == -1);
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
	data.mirror = prev_mirror;
}

void SceneTreeFTI::node_3d_request_reset(Node3D* p_node)
{
	DEV_CHECK_ONCE(data.enabled);
	DEV_ASSERT(p_node);

	MutexLock lock(data.mutex);

	if (!p_node->_is_physics_interpolation_reset_requested()) {
		p_node->_set_physics_interpolation_reset_requested(true);
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		DEV_CHECK_ONCE(data.request_reset_list.find(p_node) == -1);
#endif
		data.request_reset_list.push_back(p_node);
	}
}

void SceneTreeFTI::_node_3d_notify_set_property(Node3D& r_node)
{
	if (!r_node.is_physics_interpolated()) {
		return;
	}

	DEV_CHECK_ONCE(data.enabled);

	// Note that a Node3D can be on BOTH the transform list and the property list.
	if (!r_node.data.fti_on_tick_property_list) {
		r_node.data.fti_on_tick_property_list = true;

		// Should only appear once in the property list.
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		DEV_CHECK_ONCE(data.tick_property_list[data.mirror].find(&r_node) == -1);
#endif
		data.tick_property_list[data.mirror].push_back(&r_node);
	}

	if (!r_node.data.fti_on_frame_property_list) {
		r_node.data.fti_on_frame_property_list = true;

		// Should only appear once in the property frame list.
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		DEV_CHECK_ONCE(data.frame_property_list.find(&r_node) == -1);
#endif
		data.frame_property_list.push_back(&r_node);
	}
}

void SceneTreeFTI::_clear_depth_lists()
{
	for (uint32_t d = 0; d < data.scene_tree_depth_limit; d++) {
		data.dirty_node_depth_lists[d].clear();
	}
}

void SceneTreeFTI::_node_add_to_frame_list(Node3D& r_node, bool p_forced)
{
	if (p_forced) {
		DEV_ASSERT(!r_node.data.fti_frame_xform_force_update);
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		int64_t found = data.frame_xform_list_forced.find(&r_node);
		if (found != -1) {
			ERR_FAIL_COND(found != -1);
		}
#endif
		data.frame_xform_list_forced.push_back(&r_node);
		r_node.data.fti_frame_xform_force_update = true;
	}
	else {
		DEV_ASSERT(!r_node.data.fti_on_frame_xform_list);
#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
		int64_t found = data.frame_xform_list.find(&r_node);
		if (found != -1) {
			ERR_FAIL_COND(found != -1);
		}
#endif
		data.frame_xform_list.push_back(&r_node);
		r_node.data.fti_on_frame_xform_list = true;
	}
}

void SceneTreeFTI::_node_remove_from_frame_list(Node3D& r_node, bool p_forced)
{
	if (p_forced) {
		DEV_ASSERT(r_node.data.fti_frame_xform_force_update);
		data.frame_xform_list_forced.erase_unordered(&r_node);
		r_node.data.fti_frame_xform_force_update = false;
	}
	else {
		DEV_ASSERT(r_node.data.fti_on_frame_xform_list);
		data.frame_xform_list.erase_unordered(&r_node);
		r_node.data.fti_on_frame_xform_list = false;
	}
}

void SceneTreeFTI::_node_3d_notify_set_xform(Node3D& r_node)
{
	DEV_CHECK_ONCE(data.enabled);

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
		DEV_CHECK_ONCE(data.tick_xform_list[data.mirror].find(&r_node) == -1);
#endif
		data.tick_xform_list[data.mirror].push_back(&r_node);

		// The following flag could have been previously set
		// (for removal from the tick list).
		// We no longer need this guarantee,
		// however there is probably no downside to leaving it set
		// as it will be cleared on the next frame anyway.
		// This line is left for reference.
		// r_node.data.fti_frame_xform_force_update = false;
	}

	if (!r_node.data.fti_on_frame_xform_list) {
		_node_add_to_frame_list(r_node, false);
	}

	// If we are in the second half of a frame, always add to the force update list,
	// because we ignore the tick update list during the second update.
	if (data.in_frame) {
		if (!r_node.data.fti_frame_xform_force_update) {
			_node_add_to_frame_list(r_node, true);
		}
	}
}

void SceneTreeFTI::node_3d_notify_delete(Node3D* p_node)
{
	if (!data.enabled) {
		return;
	}

	ERR_FAIL_NULL(p_node);

	MutexLock lock(data.mutex);

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
	data.tick_xform_list[0].erase_unordered(p_node);
	data.tick_xform_list[1].erase_unordered(p_node);

	data.tick_property_list[0].erase_unordered(p_node);
	data.tick_property_list[1].erase_unordered(p_node);

	data.frame_property_list.erase_unordered(p_node);
	data.request_reset_list.erase_unordered(p_node);

#ifdef GODOT_SCENE_TREE_FTI_EXTRA_CHECKS
	// There should only be one occurrence on the lists.
	// Check this in DEV_ENABLED builds.
	DEV_CHECK_ONCE(data.tick_xform_list[0].find(p_node) == -1);
	DEV_CHECK_ONCE(data.tick_xform_list[1].find(p_node) == -1);

	DEV_CHECK_ONCE(data.tick_property_list[0].find(p_node) == -1);
	DEV_CHECK_ONCE(data.tick_property_list[1].find(p_node) == -1);

	DEV_CHECK_ONCE(data.frame_property_list.find(p_node) == -1);
	DEV_CHECK_ONCE(data.request_reset_list.find(p_node) == -1);

	DEV_CHECK_ONCE(data.frame_xform_list.find(p_node) == -1);
	DEV_CHECK_ONCE(data.frame_xform_list_forced.find(p_node) == -1);
#endif
}

SceneTreeFTI::~SceneTreeFTI()
{
#ifdef GODOT_SCENE_TREE_FTI_VERIFY
	if (_tests) {
		memfree(_tests);
		_tests = nullptr;
	}
#endif
}

#endif // ndef _3D_DISABLED


