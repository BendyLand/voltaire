/**************************************************************************/
/*  compressed_texture_resource_format.cpp                                */
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

#include "compressed_texture_resource_format.h"

#include "scene/resources/compressed_texture.h"

Ref<Resource> ResourceFormatLoaderCompressedTexture::load(const String& p_path,
	const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress,
	CacheMode p_cache_mode)
{
	if (p_path.has_extension("ctex")) {
		Ref<CompressedTexture2D> st;
		st.instantiate();
		Error err = st->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return st;
	}

	if (p_path.has_extension("ctexarray") || p_path.has_extension("ccube") ||
		p_path.has_extension("ccubearray")) {
		Ref<CompressedTextureLayered> ct;
		if (p_path.has_extension("ctexarray")) {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_2D_ARRAY));
		}
		else if (p_path.has_extension("ccube")) {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP));
		}
		else {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP_ARRAY));
		}
		Error err = ct->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return ct;
	}

	if (p_path.has_extension("ctex3d")) {
		Ref<CompressedTexture3D> st;
		st.instantiate();
		Error err = st->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return st;
	}

	if (r_error) {
		*r_error = ERR_FILE_UNRECOGNIZED;
	}
	return Ref<Resource>();
}

void ResourceFormatLoaderCompressedTexture::get_recognized_extensions(
	List<String>* p_extensions) const
{
	p_extensions->push_back("ctex");
	p_extensions->push_back("ctexarray");
	p_extensions->push_back("ccube");
	p_extensions->push_back("ccubearray");
	p_extensions->push_back("ctex3d");
}

bool ResourceFormatLoaderCompressedTexture::handles_type(const String& p_type) const
{
	return p_type == "CompressedTexture2D" || p_type == "CompressedTextureLayered" ||
		   p_type == "CompressedTexture3D";
}

String ResourceFormatLoaderCompressedTexture::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("ctex")) {
		return "CompressedTexture2D";
	}
	if (p_path.has_extension("ctexarray") || p_path.has_extension("ccube") ||
		p_path.has_extension("ccubearray")) {
		return "CompressedTextureLayered";
	}
	if (p_path.has_extension("ctex3d")) {
		return "CompressedTexture3D";
	}
	return "";
}
