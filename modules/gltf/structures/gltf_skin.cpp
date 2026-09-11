/**************************************************************************/
/*  gltf_skin.cpp                                                         */
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

#include "gltf_skin.h"
#include "scene/resources/3d/skin.h"

void GLTFSkin::_bind_methods() {}

GLTFNodeIndex GLTFSkin::get_skin_root() { return skin_root; }

void GLTFSkin::set_skin_root(GLTFNodeIndex p_skin_root) { skin_root = p_skin_root; }

Vector<GLTFNodeIndex> GLTFSkin::get_joints_original() { return joints_original; }

void GLTFSkin::set_joints_original(const Vector<GLTFNodeIndex>& p_joints_original)
{
	joints_original = Vector<GLTFNodeIndex>(p_joints_original);
}

Vector<GLTFNodeIndex> GLTFSkin::get_joints() { return joints; }

void GLTFSkin::set_joints(const Vector<GLTFNodeIndex>& p_joints)
{
	joints = Vector<GLTFNodeIndex>(p_joints);
}

Vector<GLTFNodeIndex> GLTFSkin::get_non_joints() { return non_joints; }

void GLTFSkin::set_non_joints(const Vector<GLTFNodeIndex>& p_non_joints)
{
	non_joints = Vector<GLTFNodeIndex>(p_non_joints);
}

Vector<GLTFNodeIndex> GLTFSkin::get_roots() { return roots; }

void GLTFSkin::set_roots(const Vector<GLTFNodeIndex>& p_roots)
{
	roots = Vector<GLTFNodeIndex>(p_roots);
}

int GLTFSkin::get_skeleton() { return skeleton; }

void GLTFSkin::set_skeleton(int p_skeleton) { skeleton = p_skeleton; }

Ref<Skin> GLTFSkin::get_godot_skin() { return godot_skin; }

void GLTFSkin::set_godot_skin(const Ref<Skin>& p_godot_skin) { godot_skin = p_godot_skin; }


