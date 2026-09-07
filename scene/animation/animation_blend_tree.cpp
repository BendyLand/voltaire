/**************************************************************************/
/*  animation_blend_tree.cpp                                              */
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

#include "animation_blend_tree.h"
#include "core/config/engine.h"
#include "scene/resources/animation.h"

StringName AnimationNodeAnimation::get_animation() const { return animation; }

LocalVector<StringName> (*AnimationNodeAnimation::get_editable_animation_list)() = nullptr;

void AnimationNodeAnimation::validate_node(
	const AnimationTree* p_tree, const StringName& p_path) const
{
	AnimationRootNode::validate_node(p_tree, p_path);

	const Ref<Animation>& animation_resource = p_tree->get_animation_or_null(animation);
	if (animation_resource.is_null()) {
		add_validation_error(p_tree, p_path, vformat(RTR("Animation '%s' not found."), animation));
	}
}

String AnimationNodeAnimation::get_caption() const { return "Animation"; }

void AnimationNodeAnimation::set_play_mode(PlayMode p_play_mode) { play_mode = p_play_mode; }

AnimationNodeAnimation::PlayMode AnimationNodeAnimation::get_play_mode() const { return play_mode; }

void AnimationNodeAnimation::set_advance_on_start(bool p_advance_on_start)
{
	advance_on_start = p_advance_on_start;
}

bool AnimationNodeAnimation::is_advance_on_start() const { return advance_on_start; }

bool AnimationNodeAnimation::is_using_custom_timeline() const { return use_custom_timeline; }

void AnimationNodeAnimation::set_timeline_length(double p_length) { timeline_length = p_length; }

double AnimationNodeAnimation::get_timeline_length() const { return timeline_length; }

bool AnimationNodeAnimation::is_stretching_time_scale() const { return stretch_time_scale; }

void AnimationNodeAnimation::set_start_offset(double p_offset) { start_offset = p_offset; }

double AnimationNodeAnimation::get_start_offset() const { return start_offset; }

void AnimationNodeAnimation::set_loop_mode(Animation::LoopMode p_loop_mode)
{
	loop_mode = p_loop_mode;
}

Animation::LoopMode AnimationNodeAnimation::get_loop_mode() const { return loop_mode; }

void AnimationNodeAnimation::_update_animation_cache(
	AnimationTree* p_tree, AnimationNodeInstance& p_instance) const
{
	if (p_instance.cached_animation_version == animation_version) {
		return;
	}

	const Ref<Animation>& anim = p_tree->get_animation_or_null(animation);
	if (anim.is_null()) {
		// I don't think this can even occur with validation now.
		return;
	}

	p_instance.cached_animation = anim;
	p_instance.cached_animation_version = animation_version;
}

AnimationNodeAnimation::AnimationNodeAnimation() {}

void AnimationNodeSync::set_use_sync(bool p_sync) { sync = p_sync; }

bool AnimationNodeSync::is_using_sync() const { return sync; }

AnimationNodeSync::AnimationNodeSync() {}

bool AnimationNodeOneShot::is_parameter_read_only(const StringName& p_parameter) const
{
	if (AnimationNode::is_parameter_read_only(p_parameter)) {
		return true;
	}

	if (p_parameter == active || p_parameter == internal_active) {
		return true;
	}
	return false;
}

void AnimationNodeOneShot::set_fade_in_time(double p_time) { fade_in = p_time; }

double AnimationNodeOneShot::get_fade_in_time() const { return fade_in; }

void AnimationNodeOneShot::set_fade_out_time(double p_time) { fade_out = p_time; }

double AnimationNodeOneShot::get_fade_out_time() const { return fade_out; }

void AnimationNodeOneShot::set_fade_in_curve(const Ref<Curve>& p_curve) { fade_in_curve = p_curve; }

Ref<Curve> AnimationNodeOneShot::get_fade_in_curve() const { return fade_in_curve; }

void AnimationNodeOneShot::set_fade_out_curve(const Ref<Curve>& p_curve)
{
	fade_out_curve = p_curve;
}

Ref<Curve> AnimationNodeOneShot::get_fade_out_curve() const { return fade_out_curve; }

void AnimationNodeOneShot::set_auto_restart_enabled(bool p_enabled) { auto_restart = p_enabled; }

void AnimationNodeOneShot::set_auto_restart_delay(double p_time) { auto_restart_delay = p_time; }

void AnimationNodeOneShot::set_auto_restart_random_delay(double p_time)
{
	auto_restart_random_delay = p_time;
}

bool AnimationNodeOneShot::is_auto_restart_enabled() const { return auto_restart; }

double AnimationNodeOneShot::get_auto_restart_delay() const { return auto_restart_delay; }

double AnimationNodeOneShot::get_auto_restart_random_delay() const
{
	return auto_restart_random_delay;
}

void AnimationNodeOneShot::set_mix_mode(MixMode p_mix) { mix = p_mix; }

AnimationNodeOneShot::MixMode AnimationNodeOneShot::get_mix_mode() const { return mix; }

void AnimationNodeOneShot::set_break_loop_at_end(bool p_enable) { break_loop_at_end = p_enable; }

bool AnimationNodeOneShot::is_loop_broken_at_end() const { return break_loop_at_end; }

void AnimationNodeOneShot::set_abort_on_reset(bool p_enable) { abort_on_reset = p_enable; }

bool AnimationNodeOneShot::is_aborted_on_reset() const { return abort_on_reset; }

String AnimationNodeOneShot::get_caption() const { return "OneShot"; }

bool AnimationNodeOneShot::has_filter() const { return true; }

AnimationNodeOneShot::AnimationNodeOneShot()
{
	add_input("in");
	add_input("shot");
}

String AnimationNodeAdd2::get_caption() const { return "Add2"; }

bool AnimationNodeAdd2::has_filter() const { return true; }

AnimationNodeAdd2::AnimationNodeAdd2()
{
	add_input("in");
	add_input("add");
}

String AnimationNodeAdd3::get_caption() const { return "Add3"; }

bool AnimationNodeAdd3::has_filter() const { return true; }

AnimationNodeAdd3::AnimationNodeAdd3()
{
	add_input("-add");
	add_input("in");
	add_input("+add");
}

String AnimationNodeBlend2::get_caption() const { return "Blend2"; }

bool AnimationNodeBlend2::has_filter() const { return true; }

AnimationNodeBlend2::AnimationNodeBlend2()
{
	add_input("in");
	add_input("blend");
}

String AnimationNodeBlend3::get_caption() const { return "Blend3"; }

AnimationNodeBlend3::AnimationNodeBlend3()
{
	add_input("-blend");
	add_input("in");
	add_input("+blend");
}

String AnimationNodeSub2::get_caption() const { return "Sub2"; }

bool AnimationNodeSub2::has_filter() const { return true; }

AnimationNodeSub2::AnimationNodeSub2()
{
	add_input("in");
	add_input("sub");
}

String AnimationNodeTimeScale::get_caption() const { return "TimeScale"; }

AnimationNodeTimeScale::AnimationNodeTimeScale() { add_input("in"); }

String AnimationNodeTimeSeek::get_caption() const { return "TimeSeek"; }

void AnimationNodeTimeSeek::set_explicit_elapse(bool p_enable) { explicit_elapse = p_enable; }

bool AnimationNodeTimeSeek::is_explicit_elapse() const { return explicit_elapse; }

AnimationNodeTimeSeek::AnimationNodeTimeSeek() { add_input("in"); }

bool AnimationNodeTransition::is_parameter_read_only(const StringName& p_parameter) const
{
	if (AnimationNode::is_parameter_read_only(p_parameter)) {
		return true;
	}

	if (p_parameter == current_state || p_parameter == current_index) {
		return true;
	}
	return false;
}

String AnimationNodeTransition::get_caption() const { return "Transition"; }

bool AnimationNodeTransition::add_input(const String& p_name)
{
	if (AnimationNode::add_input(p_name)) {
		input_data.push_back(InputData());
		return true;
	}
	return false;
}

void AnimationNodeTransition::remove_input(int p_index)
{
	input_data.remove_at(p_index);
	AnimationNode::remove_input(p_index);
}

void AnimationNodeTransition::set_input_as_auto_advance(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, get_input_count());
	input_data[p_input].auto_advance = p_enable;
}

bool AnimationNodeTransition::is_input_set_as_auto_advance(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, get_input_count(), false);
	return input_data[p_input].auto_advance;
}

void AnimationNodeTransition::set_input_break_loop_at_end(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, get_input_count());
	input_data[p_input].break_loop_at_end = p_enable;
}

bool AnimationNodeTransition::is_input_loop_broken_at_end(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, get_input_count(), false);
	return input_data[p_input].break_loop_at_end;
}

void AnimationNodeTransition::set_input_reset(int p_input, bool p_enable)
{
	ERR_FAIL_INDEX(p_input, get_input_count());
	input_data[p_input].reset = p_enable;
}

bool AnimationNodeTransition::is_input_reset(int p_input) const
{
	ERR_FAIL_INDEX_V(p_input, get_input_count(), true);
	return input_data[p_input].reset;
}

void AnimationNodeTransition::set_xfade_time(double p_fade) { xfade_time = p_fade; }

double AnimationNodeTransition::get_xfade_time() const { return xfade_time; }

void AnimationNodeTransition::set_xfade_curve(const Ref<Curve>& p_curve) { xfade_curve = p_curve; }

Ref<Curve> AnimationNodeTransition::get_xfade_curve() const { return xfade_curve; }

void AnimationNodeTransition::set_allow_transition_to_self(bool p_enable)
{
	allow_transition_to_self = p_enable;
}

bool AnimationNodeTransition::is_allow_transition_to_self() const
{
	return allow_transition_to_self;
}

AnimationNodeTransition::AnimationNodeTransition() {}

String AnimationNodeOutput::get_caption() const { return "Output"; }

AnimationNode::NodeTimeInfo AnimationNodeOutput::_process(ProcessState& p_process_state,
	AnimationNodeInstance& p_instance, const AnimationMixer::PlaybackInfo& p_playback_info,
	bool p_test_only)
{
	AnimationMixer::PlaybackInfo pi = p_playback_info;
	pi.weight = 1.0;
	return blend_input(p_process_state, p_instance, 0, pi, FILTER_IGNORE, true, p_test_only);
}

AnimationNodeOutput::AnimationNodeOutput() { add_input("output"); }

Ref<AnimationNode> AnimationNodeBlendTree::get_node(const StringName& p_name) const
{
	const Node* node = nodes.getptr(p_name);
	ERR_FAIL_NULL_V(node, Ref<AnimationNode>());
	return node->node;
}

void AnimationNodeBlendTree::set_node_position(const StringName& p_node, const Vector2& p_position)
{
	ERR_FAIL_COND(!nodes.has(p_node));
	nodes[p_node].position = p_position;
}

Vector2 AnimationNodeBlendTree::get_node_position(const StringName& p_node) const
{
	ERR_FAIL_COND_V(!nodes.has(p_node), Vector2());
	return nodes[p_node].position;
}

void AnimationNodeBlendTree::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	for (const KeyValue<StringName, Node>& E : nodes) {
		ChildNode cn;
		cn.name = E.key;
		cn.node = E.value.node;
		r_child_nodes->push_back(cn);
	}
}

bool AnimationNodeBlendTree::has_node(const StringName& p_name) const { return nodes.has(p_name); }

const LocalVector<StringName>* AnimationNodeBlendTree::get_node_connection_array(
	const StringName& p_name) const
{
	const Node* node = nodes.getptr(p_name);
	ERR_FAIL_NULL_V(node, nullptr);
	return &node->connections;
}

AnimationNodeBlendTree::ConnectionError AnimationNodeBlendTree::can_connect_node(
	const StringName& p_input_node, int p_input_index, const StringName& p_output_node) const
{
	if (!nodes.has(p_output_node) || p_output_node == SceneStringName(output)) {
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

	if (nodes[p_input_node].connections[p_input_index] != StringName()) {
		return CONNECTION_ERROR_CONNECTION_EXISTS;
	}

	for (const KeyValue<StringName, Node>& E : nodes) {
		for (uint32_t i = 0; i < E.value.connections.size(); i++) {
			const StringName output = E.value.connections[i];
			if (output == p_output_node) {
				return CONNECTION_ERROR_CONNECTION_EXISTS;
			}
		}
	}
	return CONNECTION_OK;
}

void AnimationNodeBlendTree::get_node_connections(LocalVector<NodeConnection>* r_connections) const
{
	for (const KeyValue<StringName, Node>& E : nodes) {
		for (uint32_t i = 0; i < E.value.connections.size(); i++) {
			const StringName output = E.value.connections[i];
			if (output != StringName()) {
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
	const Ref<AnimationNodeOutput> output = nodes[SceneStringName(output)].node;
	ERR_FAIL_COND_V(output.is_null(), NodeTimeInfo());

	AnimationMixer::PlaybackInfo pi = p_playback_info;
	pi.weight = 1.0;

	AnimationNodeInstance& output_instance =
		p_instance.get_child_instance_by_path(SceneStringName(output));
	return _blend_node(p_process_state, p_instance, output_instance, pi, FILTER_IGNORE, true,
		p_test_only, nullptr);
}

LocalVector<StringName> AnimationNodeBlendTree::get_node_list() const
{
	LocalVector<StringName> list;
	list.reserve(nodes.size());
	for (const KeyValue<StringName, Node>& E : nodes) {
		list.push_back(E.key);
	}
	list.sort_custom<StringName::AlphCompare>();
	return list;
}

void AnimationNodeBlendTree::set_graph_offset(const Vector2& p_graph_offset)
{
	graph_offset = p_graph_offset;
}

Vector2 AnimationNodeBlendTree::get_graph_offset() const { return graph_offset; }

Ref<AnimationNode> AnimationNodeBlendTree::get_child_by_name(const StringName& p_name) const
{
	return get_node(p_name);
}

void AnimationNodeBlendTree::_tree_changed() { AnimationRootNode::_tree_changed(); }

void AnimationNodeBlendTree::validate_node(
	const AnimationTree* p_tree, const StringName& p_path) const
{
	AnimationRootNode::validate_node(p_tree, p_path);

	// Validate output connection.
	{
		const LocalVector<StringName>* output_connections =
			get_node_connection_array(SceneStringName(output));
		const StringName& node_name = output_connections->operator[](0);

		if (const Node* child_node = nodes.getptr(node_name); !child_node) {
			add_validation_error(p_tree, String(p_path) + SceneStringName(output) + "/",
				RTR("Nothing connected to output."));
		}
	}

	// Rest of children.
	for (const KeyValue<StringName, Node>& E : nodes) {
		const Node& child = E.value;

		// Skip output node, already validated.
		if (E.key == SceneStringName(output)) {
			continue;
		}

		for (uint32_t input = 0; input < child.connections.size(); input++) {
			const StringName& connected_node_name = child.connections[input];
			if (const Node* connected_to = nodes.getptr(connected_node_name); !connected_to) {
				StringName path = String(p_path) + String(E.key) + "/";
				add_validation_error(p_tree, path, "Nothing connected to", input);
			}
		}
	}
}

#ifdef TOOLS_ENABLED
void AnimationNodeBlendTree::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
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
		for (const KeyValue<StringName, Node>& E : nodes) {
			r_options->push_back(String(E.key).quote());
		}
	}
	AnimationRootNode::get_argument_options(p_function, p_idx, r_options);
}
#endif

void AnimationNodeBlendTree::_initialize_node_tree()
{
	Ref<AnimationNodeOutput> output;
	output.instantiate();
	Node n;
	n.node = output;
	n.position = Vector2(300, 150);
	n.connections.resize(1);
	nodes[SceneStringName(output)] = n;
}

AnimationNodeBlendTree::AnimationNodeBlendTree() { _initialize_node_tree(); }

AnimationNodeBlendTree::~AnimationNodeBlendTree() {}


