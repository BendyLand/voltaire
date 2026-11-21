/**************************************************************************/
/*  tween.h                                                               */
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

#pragma once

#include "core/types.h"

class Tweener;
class Node;
class SceneTree;

class Tween final : public RefCounted
{
	friend class Tweener;

public:
	enum TweenProcessMode
	{
		TWEEN_PROCESS_PHYSICS,
		TWEEN_PROCESS_IDLE,
	};

	enum TweenPauseMode
	{
		TWEEN_PAUSE_BOUND,
		TWEEN_PAUSE_STOP,
		TWEEN_PAUSE_PROCESS,
	};

	enum TransitionType
	{
		TRANS_LINEAR,
		TRANS_SINE,
		TRANS_QUINT,
		TRANS_QUART,
		TRANS_QUAD,
		TRANS_EXPO,
		TRANS_ELASTIC,
		TRANS_CUBIC,
		TRANS_CIRC,
		TRANS_BOUNCE,
		TRANS_BACK,
		TRANS_SPRING,
		TRANS_MAX
	};

	enum EaseType
	{
		EASE_IN,
		EASE_OUT,
		EASE_IN_OUT,
		EASE_OUT_IN,
		EASE_MAX
	};

private:
	TweenProcessMode process_mode = TweenProcessMode::TWEEN_PROCESS_IDLE;
	TweenPauseMode pause_mode = TweenPauseMode::TWEEN_PAUSE_BOUND;
	TransitionType default_transition = TransitionType::TRANS_LINEAR;
	EaseType default_ease = EaseType::EASE_IN_OUT;

	bool is_in_tree = false;
	LocalVector<List<Ref<Tweener>>> tweeners;
	LocalVector<Ref<Tween>> subtweens;
	double total_time = 0;
	int current_step = -1;
	int loops = 1;
	int loops_done = 0;
	float speed_scale = 1;
	bool ignore_time_scale = false;

	Node* bound_node = nullptr;
	bool is_bound = false;
	bool started = false;
	bool running = true;
	bool in_step = false;
	bool dead = false;
	bool valid = false;
	bool default_parallel = false;
	bool parallel_enabled = false;
#ifdef DEBUG_ENABLED
	bool is_infinite = false;
#endif

	typedef real_t (*interpolater)(real_t t, real_t b, real_t c, real_t d);
	static interpolater interpolaters[TRANS_MAX][EASE_MAX];

	void _start_tweeners();
	void _stop_internal(bool p_reset);

protected:
	String _to_string();

public:
	Ref<Tweener> tween_interval(double p_time);
	Ref<Tweener> tween_subtween(Tween* rp_subtween);
	Ref<Tweener> tween_callback(void (*p_callback)() = nullptr);
	Ref<Tweener> tween_property(const Vector<String>& p_property, double p_duration);
	Ref<Tweener> tween_method(double p_duration);
	void append(Ref<Tweener> p_tweener);

	bool custom_step(double p_delta);
	void stop();
	void pause();
	void play();
	void kill();

	bool has_tweeners() const;
	bool is_running();
	bool is_valid();
	void clear();

	Tween* bind_node(const Node* rp_node);
	Tween* set_process_mode(TweenProcessMode p_mode);
	TweenProcessMode get_process_mode() const;
	Tween* set_pause_mode(TweenPauseMode p_mode);
	TweenPauseMode get_pause_mode() const;
	Tween* set_ignore_time_scale(bool p_ignore = true);
	bool is_ignoring_time_scale() const;

	Tween* set_parallel(bool p_parallel);
	Tween* set_loops(int p_loops);
	int get_loops_left() const;
	Tween* set_speed_scale(float p_speed);
	Tween* set_trans(TransitionType p_trans);
	TransitionType get_trans() const;
	Tween* set_ease(EaseType p_ease);
	EaseType get_ease() const;

	Tween* parallel();
	Tween* chain();

	static real_t run_equation(
		TransitionType p_trans_type, EaseType p_ease_type, real_t t, real_t b, real_t c, real_t d);

	bool step(double p_delta);
	bool can_process(bool p_tree_paused) const;
	Node* get_bound_node() const;
	double get_total_time() const;

	Tween();
	explicit Tween(bool p_in_tree);
	~Tween();
};

class Tweener final : public RefCounted
{
	friend class Tween;

public:
	enum Type
	{
		TWEENER_INTERVAL,
		TWEENER_CALLBACK,
		TWEENER_SUBTWEEN,
		TWEENER_PROPERTY,
		TWEENER_METHOD,
		TWEENER_AWAIT,
	};

	Type get_type() const { return type; }

	Ref<Tweener> from(double p_value);
	Ref<Tweener> from_current();
	Ref<Tweener> as_relative();
	Ref<Tweener> set_trans(Tween::TransitionType p_trans);
	Ref<Tweener> set_ease(Tween::EaseType p_ease);
	Ref<Tweener> set_custom_interpolator(real_t (*p_interpolator)(real_t));
	Ref<Tweener> set_delay(double p_delay);
	Ref<Tweener> set_timeout(double p_timeout);

	void set_tween(const Ref<Tween>& p_tween);
	void start();
	bool step(double& r_delta);

	Tweener();
	explicit Tweener(Type p_type);
	Tweener(Type p_type, double p_duration);
	explicit Tweener(const Ref<Tween>& p_subtween);
	~Tweener() = default;

	Ref<Tween> subtween;

private:
	Ref<Tween> _get_tween();
	void _finish();

	Type type = TWEENER_INTERVAL;
	Tween* tween = nullptr;
	Ref<RefCounted> ref_copy; // Makes sure that RefCounted objects are not freed too early.
	Vector<String> property;
	real_t (*custom_interpolator)(real_t) = nullptr;
	void (*callback)() = nullptr;

	double elapsed_time = 0.0;
	double duration = 0.0;
	double delay = 0.0;
	double timeout = -1.0;
	double initial_val = 0.0;
	Tween::TransitionType trans_type = Tween::TRANS_MAX;
	Tween::EaseType ease_type = Tween::EASE_MAX;

	bool finished = false;
	bool do_continue = true;
	bool do_continue_delayed = false;
	bool relative = false;
	bool received = false;
};
