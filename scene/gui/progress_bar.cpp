/**************************************************************************/
/*  progress_bar.cpp                                                      */
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
#include "core/string/translation_server.h"
#include "progress_bar.h"
#include "scene/resources/text_line.h"
#include "scene/theme/theme_db.h"
#include "servers/display/accessibility_server.h"

Size2 ProgressBar::get_minimum_size() const
{
	Size2 minimum_size = theme_cache.background_style->get_minimum_size();
	minimum_size = minimum_size.max(theme_cache.fill_style->get_minimum_size());
	if (show_percentage) {
		String txt = "100%";
		TextLine tl = TextLine(txt, theme_cache.font, theme_cache.font_size);
		minimum_size.height = MAX(minimum_size.height,
			theme_cache.background_style->get_minimum_size().height + tl.get_size().y);
	}
	else { // this is needed, else the progressbar will collapse
		minimum_size = minimum_size.maxf(1);
	}
	return minimum_size;
}

int ProgressBar::get_fill_mode() { return mode; }

bool ProgressBar::is_percentage_shown() const { return show_percentage; }

bool ProgressBar::is_indeterminate() const { return indeterminate; }

bool ProgressBar::is_editor_preview_indeterminate_enabled() const
{
	return editor_preview_indeterminate;
}

ProgressBar::ProgressBar()
{
	set_v_size_flags(0);
	set_step(0.01);
}


