/**************************************************************************/
/*  tween.cpp                                                             */
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

#include "tween.h"
#include "scene/animation/easing_equations.h"
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
