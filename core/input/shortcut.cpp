/**************************************************************************/
/*  shortcut.cpp                                                          */
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

#include "shortcut.h"

void Shortcut::set_events(const Vector<InputEvent>& p_events)
{
	events = p_events;
	emit_changed();
}

const Vector<InputEvent>& Shortcut::get_events() const { return events; }

void Shortcut::set_events_list(const List<InputEvent>* p_events)
{
	events.clear();
	if (!p_events) {
		return;
	}
	for (const InputEvent& ie : *p_events) {
		events.push_back(ie);
	}
}

bool Shortcut::matches_event(const InputEvent& p_event) const
{
	if (p_event.type == InputEventType::SHORTCUT && p_event.shortcut.shortcut == this) {
		return true;
	}

	for (int i = 0; i < events.size(); i++) {
		const InputEvent& ie = events[i];
		if (ie.is_valid() && ie.is_match(p_event)) {
			return true;
		}
	}

	return false;
}

String Shortcut::get_as_text() const
{
	for (int i = 0; i < events.size(); i++) {
		const InputEvent& ie = events[i];
		if (ie.is_valid()) {
			return ie.as_text();
		}
	}

	return "None";
}

Ref<Shortcut> Shortcut::make_from_action(const String& p_action)
{
	InputEvent event = InputEvent::make_action(p_action);

	Ref<Shortcut> shortcut;
	shortcut.instantiate();
	shortcut->set_events({event});
	return shortcut;
}

bool Shortcut::has_valid_event() const
{
	for (int i = 0; i < events.size(); i++) {
		if (events[i].is_valid()) {
			return true;
		}
	}

	return false;
}

bool Shortcut::is_event_array_equal(
	const Vector<InputEvent>& p_event_array1, const Vector<InputEvent>& p_event_array2)
{
	if (p_event_array1.size() != p_event_array2.size()) {
		return false;
	}
	for (int i = 0; i < p_event_array1.size(); i++) {
		if (!p_event_array1[i].is_match(p_event_array2[i], true)) {
			return false;
		}
	}
	return true;
}
