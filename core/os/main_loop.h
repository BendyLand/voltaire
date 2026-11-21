/**************************************************************************/
/*  main_loop.h                                                           */
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

#include "core/typedefs.h"

class MainLoop final
{
private:
	static inline bool _active = false;

	static void _default_initialize() {}
	static void _default_iteration_prepare() {}
	static bool _default_physics_process(double p_time)
	{
		(void)p_time;
		return false;
	}
	static void _default_iteration_end() {}
	static bool _default_process(double p_time)
	{
		(void)p_time;
		return false;
	}
	static void _default_finalize() {}
	static void _default_notification(int p_notification) { (void)p_notification; }
	static void _default_destroy() {}

public:
	MainLoop() = delete;

	enum
	{
		// make sure these are replicated in Node
		NOTIFICATION_OS_MEMORY_WARNING = 2009,
		NOTIFICATION_TRANSLATION_CHANGED = 2010,
		NOTIFICATION_WM_ABOUT = 2011,
		NOTIFICATION_CRASH = 2012,
		NOTIFICATION_OS_IME_UPDATE = 2013,
		NOTIFICATION_APPLICATION_RESUMED = 2014,
		NOTIFICATION_APPLICATION_PAUSED = 2015,
		NOTIFICATION_APPLICATION_FOCUS_IN = 2016,
		NOTIFICATION_APPLICATION_FOCUS_OUT = 2017,
		NOTIFICATION_TEXT_SERVER_CHANGED = 2018,
		NOTIFICATION_APPLICATION_PIP_MODE_ENTERED = 2019,
		NOTIFICATION_APPLICATION_PIP_MODE_EXITED = 2020,
	};

	inline static void (*initialize)() = &_default_initialize;
	inline static void (*iteration_prepare)() = &_default_iteration_prepare;
	inline static bool (*physics_process)(double p_time) = &_default_physics_process;
	inline static void (*iteration_end)() = &_default_iteration_end;
	inline static bool (*process)(double p_time) = &_default_process;
	inline static void (*finalize)() = &_default_finalize;
	inline static void (*notification)(int p_notification) = &_default_notification;
	inline static void (*destroy)() = &_default_destroy;

	template <typename Impl> static void bind()
	{
		initialize = &Impl::initialize_tree;
		iteration_prepare = &Impl::iteration_prepare;
		physics_process = &Impl::physics_process;
		iteration_end = &Impl::iteration_end;
		process = &Impl::process;
		finalize = &Impl::finalize_tree;
		notification = &Impl::notification;
		destroy = &Impl::finalize;
		_active = true;
	}

	static void reset_backend();
	static void clear();
	_FORCE_INLINE_ static bool is_active() { return _active; }
};
