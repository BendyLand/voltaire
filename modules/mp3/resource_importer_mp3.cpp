/**************************************************************************/
/*  resource_importer_mp3.cpp                                             */
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

#include "audio_stream_mp3.h"
#include "core/io/resource_saver.h"
#include "resource_importer_mp3.h"

#ifdef TOOLS_ENABLED
#include "editor/import/audio_stream_import_settings.h"
#endif

String ResourceImporterMP3::get_importer_name() const { return "mp3"; }

String ResourceImporterMP3::get_visible_name() const { return "MP3"; }

void ResourceImporterMP3::get_recognized_extensions(List<String>* p_extensions) const
{
#ifndef DR_MP3_ONLY_MP3
	p_extensions->push_back("mp1");
	p_extensions->push_back("mp2");
#endif
	p_extensions->push_back("mp3");
}

String ResourceImporterMP3::get_save_extension() const { return "mp3str"; }

String ResourceImporterMP3::get_resource_type() const { return "AudioStreamMP3"; }

int ResourceImporterMP3::get_preset_count() const { return 0; }

String ResourceImporterMP3::get_preset_name(int p_idx) const { return String(); }

#ifdef TOOLS_ENABLED
bool ResourceImporterMP3::has_advanced_options() const { return true; }

void ResourceImporterMP3::show_advanced_options(const String& p_path)
{
	Ref<AudioStreamMP3> mp3_stream = AudioStreamMP3::load_from_file(p_path);
	if (mp3_stream.is_valid()) {
		AudioStreamImportSettingsDialog::get_singleton()->edit(p_path, "mp3", mp3_stream);
	}
}
#endif


