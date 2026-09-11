/**************************************************************************/
/*  shortcut.cpp                                                          */
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

#include "shortcut.h"

void Shortcut::set_events_list(const List<Ref<InputEvent>>* p_events)
{
	events.clear();

	for (const Ref<InputEvent>& ie : *p_events) {
		events.push_back(ie);
	}
}

bool Shortcut::matches_event(const Ref<InputEvent>& p_event) const
{
	Ref<InputEventShortcut> ies = p_event;
	if (ies.is_valid()) {
		if (ies->get_shortcut().ptr() == this) {
			return true;
		}
	}

	for (int i = 0; i < events.size(); i++) {
		Ref<InputEvent> ie = events[i];
		bool valid = ie.is_valid() && ie->is_match(p_event);

		// Stop on first valid event - don't need to check further.
		if (valid) {
			return true;
		}
	}

	return false;
}

String Shortcut::get_as_text() const
{
	for (int i = 0; i < events.size(); i++) {
		Ref<InputEvent> ie = events[i];
		// Return first shortcut which is valid
		if (ie.is_valid()) {
			return ie->as_text();
		}
	}

	return "None";
}

Ref<Shortcut> Shortcut::make_from_action(const StringName& p_action)
{
	Ref<InputEventAction> event;
	event.instantiate();
	event->set_action(p_action);

	Ref<Shortcut> shortcut;
	shortcut.instantiate();
	shortcut->set_events({event});
	return shortcut;
}

bool Shortcut::has_valid_event() const
{
	// Tests if there is ANY input event which is valid.
	for (int i = 0; i < events.size(); i++) {
		Ref<InputEvent> ie = events[i];
		if (ie.is_valid()) {
			return true;
		}
	}

	return false;
}

void Shortcut::_bind_methods() {}


