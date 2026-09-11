/**************************************************************************/
/*  physics_server_2d_wrap_mt.cpp                                         */
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

#include "physics_server_2d_wrap_mt.h"

void PhysicsServer2DWrapMT::_thread_exit() { exit = true; }

void PhysicsServer2DWrapMT::_thread_sync() { doing_sync.set(); }

void PhysicsServer2DWrapMT::step(real_t p_step)
{
	if (create_thread) {
		command_queue.push(physics_server_2d, &PhysicsServer2D::step, p_step);
	}
	else {
		physics_server_2d->step(p_step);
	}
}

void PhysicsServer2DWrapMT::sync()
{
	if (create_thread) {
		command_queue.push_and_sync(this, &PhysicsServer2DWrapMT::_thread_sync);
	}
	else {
		command_queue.flush_all(); // Flush all pending from other threads.
	}
	physics_server_2d->sync();
}

void PhysicsServer2DWrapMT::flush_queries() { physics_server_2d->flush_queries(); }

void PhysicsServer2DWrapMT::end_sync()
{
	physics_server_2d->end_sync();

	if (create_thread) {
		doing_sync.clear();
	}
}

PhysicsServer2DWrapMT::PhysicsServer2DWrapMT(PhysicsServer2D* p_contained, bool p_create_thread)
{
	physics_server_2d = p_contained;
	create_thread = p_create_thread;
}

PhysicsServer2DWrapMT::~PhysicsServer2DWrapMT() { memdelete(physics_server_2d); }


