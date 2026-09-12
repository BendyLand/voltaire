/**************************************************************************/
/*  animation_node_state_machine.cpp                                      */
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

#include "animation_node_state_machine.h"
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

StringName AnimationNodeStateMachineTransition::get_advance_condition() const
{
	return advance_condition;
}

StringName AnimationNodeStateMachineTransition::get_advance_condition_name() const
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
	AnimationNodeStateMachine* p_state_machine, const StringName& p_state)
{
	current = p_state;
	if (current == StringName()) {
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
		indices = anodesm->find_transition_from(SceneStringName(Start));
		int anodesm_start_size = indices.size();
		indices = anodesm->find_transition_to(SceneStringName(End));
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

void AnimationNodeStateMachinePlayback::travel(const StringName& p_state, bool p_reset_on_teleport)
{
	ERR_FAIL_COND_EDMSG(is_grouped, "Grouped AnimationNodeStateMachinePlayback must be handled by "
									"parent AnimationNodeStateMachinePlayback. You need to "
									"retrieve the parent Root/Nested AnimationNodeStateMachine.");
	ERR_FAIL_COND_EDMSG(String(p_state).contains("/Start") || String(p_state).contains("/End"),
		"Grouped AnimationNodeStateMachinePlayback doesn't allow to play Start/End directly. "
		"Instead, play the prev or next state of group in the parent AnimationNodeStateMachine.");
	_travel_main(p_state, p_reset_on_teleport);
}

void AnimationNodeStateMachinePlayback::start(const StringName& p_state, bool p_reset)
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
	const StringName& p_state, bool p_reset_on_teleport)
{
	travel_request = p_state;
	reset_request_on_teleport = p_reset_on_teleport;
	stop_request = false;
}

void AnimationNodeStateMachinePlayback::_start_main(const StringName& p_state, bool p_reset)
{
	travel_request = StringName();
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
	return current == SceneStringName(End) && fading_from == StringName();
}

StringName AnimationNodeStateMachinePlayback::get_current_node() const { return current; }

StringName AnimationNodeStateMachinePlayback::get_fading_from_node() const { return fading_from; }

Vector<StringName> AnimationNodeStateMachinePlayback::get_travel_path() const { return path; }

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
	AnimationNodeStateMachine* p_state_machine, const StringName& p_state)
{
	if (!p_state.is_empty() && !_is_grouped_state_machine(p_state_machine->get_node(p_state))) {
		_signal_state_change(p_process_state.tree, p_state, false);
	}
	fading_from = StringName();
	fadeing_from_nti = AnimationNode::NodeTimeInfo();
}

void AnimationNodeStateMachinePlayback::_start(
	AnimationNode::ProcessState& p_process_state, AnimationNodeStateMachine* p_state_machine)
{
	playing = true;
	_set_current(p_process_state, p_state_machine,
		start_request != StringName() ? start_request : SceneStringName(Start));
	teleport_request = true;
	stop_request = false;
	start_request = StringName();
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
		Vector<int> indices = anodesm->find_transition_from(SceneStringName(Start));
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
	start_request = StringName();
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
		start_request = StringName();
		travel_request = StringName();
		path.clear();
		playing = false;
		return AnimationNode::NodeTimeInfo();
	}

	if (!playing && start_request != StringName() && travel_request != StringName()) {
		return AnimationNode::NodeTimeInfo();
	}

	// Process start/travel request.
	if (start_request != StringName() || travel_request != StringName()) {
		if (p_state_machine->get_state_machine_type() !=
			AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
			_clear_path_children(p_process_state, tree, p_state_machine, p_test_only);
		}
	}

	if (start_request != StringName()) {
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
			StringName node = start_request;
			ERR_FAIL_V_MSG(AnimationNode::NodeTimeInfo(), "No such node: '" + node + "'");
		}
	}

	if (travel_request != StringName()) {
		// Fix path.
		String travel_target = _validate_path(p_state_machine, travel_request);
		Vector<String> travel_path = travel_target.split("/");
		travel_request = travel_path[0];
		StringName temp_travel_request = travel_request; // For the case that can't travel.
		// Process children.
		Vector<StringName> new_path;
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
		fading_from = StringName();
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
		_set_current(p_process_state, p_state_machine, StringName());
		return AnimationNode::NodeTimeInfo();
	}

	// Special case for grouped state machine Start/End to make priority with parent blend (means
	// don't treat Start and End states as RESET animations).
	bool is_start_of_group = false;
	bool is_end_of_group = false;
	if (!p_state_machine->are_ends_reset() ||
		p_state_machine->get_state_machine_type() ==
			AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		is_start_of_group = fading_from == SceneStringName(Start);
		is_end_of_group = current == SceneStringName(End);
	}

	// Calc blend amount by cross-fade.
	float fade_blend = 1.0;
	if (fading_time && fading_from != StringName()) {
		if (!p_state_machine->states.has(fading_from)) {
			fading_from = StringName();
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
	if (fading_from != StringName()) {
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
					current == SceneStringName(End);

	// Predict remaining time.
	if (will_end || ((p_state_machine->get_state_machine_type() ==
						 AnimationNodeStateMachine::STATE_MACHINE_TYPE_NESTED) &&
						!p_state_machine->has_transition_from(current))) {
		// There is no next transition.
		if (fading_from != StringName()) {
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
	Vector<StringName> transition_path;
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

		if (current == SceneStringName(End)) {
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

	return next.node == SceneStringName(End);
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
		if (p_transition.from == SceneStringName(Start)) {
			parent_playback = _get_parent_playback(p_tree);
			if (parent_playback.is_valid()) {
				r_bypass = true;
				temp_transition = parent_playback->_get_group_start_transition();
			}
		}
		else if (p_transition.to == SceneStringName(End)) {
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

void AnimationNodeStateMachinePlayback::push_path(const StringName& p_state)
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

bool AnimationNodeStateMachine::is_parameter_read_only(const StringName& p_parameter) const
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
	const StringName& p_name, Ref<AnimationNode> p_node, const Vector2& p_position)
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

void AnimationNodeStateMachine::replace_node(const StringName& p_name, Ref<AnimationNode> p_node)
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

Ref<AnimationNode> AnimationNodeStateMachine::get_node(const StringName& p_name) const
{
	ERR_FAIL_COND_V_EDMSG(
		!states.has(p_name), Ref<AnimationNode>(), String(p_name) + " is not found current state.");

	return states[p_name].node;
}

StringName AnimationNodeStateMachine::get_node_name(const Ref<AnimationNode>& p_node) const
{
	for (const KeyValue<StringName, State>& E : states) {
		if (E.value.node == p_node) {
			return E.key;
		}
	}

	ERR_FAIL_V(StringName());
}

void AnimationNodeStateMachine::get_child_nodes(LocalVector<ChildNode>* r_child_nodes)
{
	Vector<StringName> nodes;

	for (const KeyValue<StringName, State>& E : states) {
		nodes.push_back(E.key);
	}

	nodes.sort_custom<StringName::AlphCompare>();

	for (int i = 0; i < nodes.size(); i++) {
		ChildNode cn;
		cn.name = nodes[i];
		cn.node = states[cn.name].node;
		r_child_nodes->push_back(cn);
	}
}

bool AnimationNodeStateMachine::has_node(const StringName& p_name) const
{
	return states.has(p_name);
}

void AnimationNodeStateMachine::_rename_transitions(
	const StringName& p_name, const StringName& p_new_name)
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

LocalVector<StringName> AnimationNodeStateMachine::get_node_list() const
{
	LocalVector<StringName> nodes;
	nodes.reserve(states.size());
	for (const KeyValue<StringName, State>& E : states) {
		nodes.push_back(E.key);
	}
	nodes.sort_custom<StringName::AlphCompare>();
	return nodes;
}

bool AnimationNodeStateMachine::has_transition(
	const StringName& p_from, const StringName& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			return true;
		}
	}
	return false;
}

bool AnimationNodeStateMachine::has_transition_from(const StringName& p_from) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from) {
			return true;
		}
	}
	return false;
}

bool AnimationNodeStateMachine::has_transition_to(const StringName& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].to == p_to) {
			return true;
		}
	}
	return false;
}

int AnimationNodeStateMachine::find_transition(
	const StringName& p_from, const StringName& p_to) const
{
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from && transitions[i].to == p_to) {
			return i;
		}
	}
	return -1;
}

Vector<int> AnimationNodeStateMachine::find_transition_from(const StringName& p_from) const
{
	Vector<int> ret;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].from == p_from) {
			ret.push_back(i);
		}
	}
	return ret;
}

Vector<int> AnimationNodeStateMachine::find_transition_to(const StringName& p_to) const
{
	Vector<int> ret;
	for (int i = 0; i < transitions.size(); i++) {
		if (transitions[i].to == p_to) {
			ret.push_back(i);
		}
	}
	return ret;
}

bool AnimationNodeStateMachine::_can_connect(const StringName& p_name)
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

StringName AnimationNodeStateMachine::get_transition_from(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), StringName());
	return transitions[p_transition].from;
}

StringName AnimationNodeStateMachine::get_transition_to(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), StringName());
	return transitions[p_transition].to;
}

bool AnimationNodeStateMachine::is_transition_across_group(int p_transition) const
{
	ERR_FAIL_INDEX_V(p_transition, transitions.size(), false);
	if (get_state_machine_type() == AnimationNodeStateMachine::STATE_MACHINE_TYPE_GROUPED) {
		if (transitions[p_transition].from == SceneStringName(Start) ||
			transitions[p_transition].to == SceneStringName(End)) {
			return true;
		}
	}
	return false;
}

int AnimationNodeStateMachine::get_transition_count() const { return transitions.size(); }

void AnimationNodeStateMachine::remove_transition(const StringName& p_from, const StringName& p_to)
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

Ref<AnimationNode> AnimationNodeStateMachine::get_child_by_name(const StringName& p_name) const
{
	return get_node(p_name);
}

void AnimationNodeStateMachine::reset_state()
{
	states.clear();
	transitions.clear();
	playback = "playback";
	graph_offset = Vector2();

	Ref<AnimationNodeStartState> s;
	s.instantiate();
	State start;
	start.node = s;
	start.position = Vector2(200, 100);
	states[SceneStringName(Start)] = start;

	Ref<AnimationNodeEndState> e;
	e.instantiate();
	State end;
	end.node = e;
	end.position = Vector2(900, 100);
	states[SceneStringName(End)] = end;

	emit_changed();
	_tree_changed();
}

void AnimationNodeStateMachine::set_node_position(
	const StringName& p_name, const Vector2& p_position)
{
	ERR_FAIL_COND(!states.has(p_name));
	states[p_name].position = p_position;
}

Vector2 AnimationNodeStateMachine::get_node_position(const StringName& p_name) const
{
	ERR_FAIL_COND_V(!states.has(p_name), Vector2());
	return states[p_name].position;
}

void AnimationNodeStateMachine::_tree_changed()
{
	emit_changed();
	AnimationRootNode::_tree_changed();
}

#ifdef TOOLS_ENABLED
void AnimationNodeStateMachine::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
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
		for (const KeyValue<StringName, State>& E : states) {
			r_options->push_back(String(E.key).quote());
		}
	}
	AnimationRootNode::get_argument_options(p_function, p_idx, r_options);
}
#endif

Vector<StringName> AnimationNodeStateMachine::get_nodes_with_transitions_from(
	const StringName& p_node) const
{
	Vector<StringName> result;
	for (const Transition& transition : transitions) {
		if (transition.from == p_node) {
			result.push_back(transition.to);
		}
	}
	return result;
}

Vector<StringName> AnimationNodeStateMachine::get_nodes_with_transitions_to(
	const StringName& p_node) const
{
	Vector<StringName> result;
	for (const Transition& transition : transitions) {
		if (transition.to == p_node) {
			result.push_back(transition.from);
		}
	}
	return result;
}

AnimationNodeStateMachine::AnimationNodeStateMachine()
{
	Ref<AnimationNodeStartState> s;
	s.instantiate();
	State start;
	start.node = s;
	start.position = Vector2(200, 100);
	states[SceneStringName(Start)] = start;

	Ref<AnimationNodeEndState> e;
	e.instantiate();
	State end;
	end.node = e;
	end.position = Vector2(900, 100);
	states[SceneStringName(End)] = end;
}


