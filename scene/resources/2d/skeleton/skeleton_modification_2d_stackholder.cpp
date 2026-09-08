/**************************************************************************/
/*  skeleton_modification_2d_stackholder.cpp                              */
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

#include "core/config/engine.h"
#include "scene/2d/skeleton_2d.h"
#include "skeleton_modification_2d_stackholder.h"

void SkeletonModification2DStackHolder::_execute(float p_delta)
{
	ERR_FAIL_COND_MSG(!stack || !is_setup || stack->skeleton == nullptr,
		"Modification is not setup and therefore cannot execute!");

	if (held_modification_stack.is_valid()) {
		held_modification_stack->execute(p_delta, execution_mode);
	}
}

void SkeletonModification2DStackHolder::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;

	if (stack != nullptr) {
		is_setup = true;

		if (held_modification_stack.is_valid()) {
			held_modification_stack->set_skeleton(stack->get_skeleton());
			held_modification_stack->setup();
		}
	}
}

void SkeletonModification2DStackHolder::_draw_editor_gizmo()
{
	if (stack) {
		if (held_modification_stack.is_valid()) {
			held_modification_stack->draw_editor_gizmos();
		}
	}
}

void SkeletonModification2DStackHolder::set_held_modification_stack(
	Ref<SkeletonModificationStack2D> p_held_stack)
{
	held_modification_stack = p_held_stack;

	if (is_setup && held_modification_stack.is_valid()) {
		held_modification_stack->set_skeleton(stack->get_skeleton());
		held_modification_stack->setup();
	}
}

Ref<SkeletonModificationStack2D>
SkeletonModification2DStackHolder::get_held_modification_stack() const
{
	return held_modification_stack;
}

SkeletonModification2DStackHolder::SkeletonModification2DStackHolder()
{
	stack = nullptr;
	is_setup = false;
	enabled = true;
}

SkeletonModification2DStackHolder::~SkeletonModification2DStackHolder() {}


