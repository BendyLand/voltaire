/**************************************************************************/
/*  resource_importer_ogg_vorbis.cpp                                      */
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

#include "core/io/resource_saver.h"
#include "resource_importer_ogg_vorbis.h"

#ifdef TOOLS_ENABLED
#include "editor/import/audio_stream_import_settings.h"
#endif

#include <ogg/ogg.h>
#include <vorbis/codec.h>

String ResourceImporterOggVorbis::get_importer_name() const { return "oggvorbisstr"; }

String ResourceImporterOggVorbis::get_visible_name() const { return "Ogg Vorbis"; }

void ResourceImporterOggVorbis::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("ogg");
}

String ResourceImporterOggVorbis::get_save_extension() const { return "oggvorbisstr"; }

String ResourceImporterOggVorbis::get_resource_type() const { return "AudioStreamOggVorbis"; }

#ifdef TOOLS_ENABLED
bool ResourceImporterOggVorbis::has_advanced_options() const { return true; }

void ResourceImporterOggVorbis::show_advanced_options(const String& p_path)
{
	Ref<AudioStreamOggVorbis> ogg_stream = AudioStreamOggVorbis::load_from_file(p_path);
	if (ogg_stream.is_valid()) {
		AudioStreamImportSettingsDialog::get_singleton()->edit(p_path, "oggvorbisstr", ogg_stream);
	}
}
#endif

#ifndef DISABLE_DEPRECATED
Ref<AudioStreamOggVorbis> ResourceImporterOggVorbis::load_from_buffer(
	const Vector<uint8_t>& p_stream_data)
{
	return AudioStreamOggVorbis::load_from_buffer(p_stream_data);
}

Ref<AudioStreamOggVorbis> ResourceImporterOggVorbis::load_from_file(const String& p_path)
{
	return AudioStreamOggVorbis::load_from_file(p_path);
}
#endif

void ResourceImporterOggVorbis::_bind_methods() {}

ResourceImporterOggVorbis::ResourceImporterOggVorbis() {}


