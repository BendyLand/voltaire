/**************************************************************************/
/*  animation_blend_tree.cpp                                              */
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

#include "animation_blend_tree.h"
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
