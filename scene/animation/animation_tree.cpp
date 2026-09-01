/**************************************************************************/
/*  animation_tree.cpp                                                    */
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

#include "animation_tree.compat.inc"
#include "animation_tree.h"
#include "core/config/engine.h"
#include "scene/animation/animation_blend_tree.h"
#include "scene/animation/animation_player.h"

thread_local AnimationNode::ProcessState* AnimationNode::tls_process_state = nullptr;
thread_local AnimationNodeInstance* AnimationNode::current_instance = nullptr;

bool AnimationNode::is_parameter_read_only(const StringName& p_parameter) const
{
	if (p_parameter == current_length || p_parameter == current_position ||
		p_parameter == current_delta) {
		return true;
	}

	return false;
}

void AnimationNode::blend_animation(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const StringName& p_animation,
	AnimationMixer::PlaybackInfo& p_playback_info)
{
	p_playback_info.track_weights = &p_instance.track_weights;
	p_process_state.tree->make_animation_instance(p_animation, p_playback_info);
}

AnimationNode::NodeTimeInfo AnimationNode::_pre_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	ERR_FAIL_NULL_V(tls_process_state, NodeTimeInfo()); // Should not ever happen.
	ERR_FAIL_COND_V_MSG(tls_process_state != &p_process_state, NodeTimeInfo(),
		"AnimationNodes can only be processed from within their own AnimationTree.");

	AnimationNodeInstance* prev_instance = current_instance;

	current_instance = &p_instance;
	NodeTimeInfo nti = process(p_process_state, p_instance, p_playback_info, p_test_only);
	current_instance = prev_instance;

	return nti;
}

void AnimationNode::add_validation_error(const AnimationTree* p_tree, const StringName& p_path,
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
	// that a synced AnimationNodeSync exists under the un-synced AnimationNodeSync.
	if (!p_playback_info.seeked && !p_sync && !any_valid) {
		p_playback_info.delta = 0.0;
	}
	p_other.blended = any_valid;
	return p_other.resource->_pre_process(p_process_state, p_other, p_playback_info, p_test_only);
}

String AnimationNode::get_caption() const
{
	String ret = "Node";
	return ret;
}

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

void AnimationNode::blend_animation_ex(const StringName& p_animation, double p_time, double p_delta,
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

double AnimationNode::blend_node_ex(const StringName& p_sub_path, const Ref<AnimationNode>& p_node,
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

Ref<AnimationRootNode> AnimationTree::get_root_animation_node() const
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
		root_animation_node->_pre_process(process_state, instance,
 pi, false);
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

const AHashMap<StringName, AnimationNode::InvalidInstance>&
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
	const StringName& p_path, const String& p_error, int p_input_index) const
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
real_t AnimationTree::get_connection_activity(const StringName& p_path, int p_connection) const
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

Ref<AnimationNode> AnimationNode::get_child_by_name(const StringName& p_name) const
{
	return Ref<AnimationNode>();
}


