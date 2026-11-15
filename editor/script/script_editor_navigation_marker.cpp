/**************************************************************************/
/*  script_editor_navigation_marker.cpp                                   */
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

#include "script_editor_navigation_marker.h"

#include "core/config/engine.h"
#include "core/os/memory.h"

void ScriptEditorNavigationMarker::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void ScriptEditorNavigationMarker::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void ScriptEditorNavigationMarker::init_begin()
{
	initialize();
	data->init_in_progress = true;
}

void ScriptEditorNavigationMarker::init_end()
{
	if (!data) {
		return;
	}
	data->init_in_progress = false;
}

void ScriptEditorNavigationMarker::locate_begin()
{
	initialize();
	data->locate_in_progress = true;
}

void ScriptEditorNavigationMarker::locate_end()
{
	initialize();
	data->locate_in_progress = false;
	data->locate_end_physics_frame = Engine::get_physics_frames();
	data->locate_end_process_frame = Engine::get_process_frames();
}

void ScriptEditorNavigationMarker::traverse_begin()
{
	initialize();
	data->traverse_in_progress = true;
}

void ScriptEditorNavigationMarker::traverse_end()
{
	initialize();
	data->traverse_in_progress = false;
	data->traverse_end_physics_frame = Engine::get_physics_frames();
	data->traverse_end_process_frame = Engine::get_process_frames();
}

bool ScriptEditorNavigationMarker::is_initializing()
{
	return data && data->init_in_progress;
}

bool ScriptEditorNavigationMarker::is_locating()
{
	return data && data->locate_in_progress;
}

bool ScriptEditorNavigationMarker::is_traversing()
{
	return data && data->traverse_in_progress;
}

bool ScriptEditorNavigationMarker::is_locate_just_occured()
{
	if (!data) {
		return false;
	}
	if (Engine::is_in_physics_frame()) {
		return data->locate_end_physics_frame == Engine::get_physics_frames() ||
			   data->locate_end_physics_frame == Engine::get_physics_frames() - 1;
	}
	else {
		return data->locate_end_process_frame == Engine::get_process_frames();
	}
}

bool ScriptEditorNavigationMarker::is_traverse_just_occured()
{
	if (!data) {
		return false;
	}
	if (Engine::is_in_physics_frame()) {
		return data->traverse_end_physics_frame == Engine::get_physics_frames() ||
			   data->traverse_end_physics_frame == Engine::get_physics_frames() - 1;
	}
	else {
		return data->traverse_end_process_frame == Engine::get_process_frames();
	}
}
