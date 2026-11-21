/**************************************************************************/
/*  image_loader_webp.h                                                   */
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

#pragma once

#include "core/io/image_loader.h"

class ImageLoaderWebP final : public ImageFormatLoader {
public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	ImageLoaderWebP();
};

namespace WebPCommon {
// Given an image, pack this data into a WebP file.
Vector<uint8_t> _webp_lossy_pack(const Ref<Image> &p_image, float p_quality);
Vector<uint8_t> _webp_lossless_pack(const Ref<Image> &p_image);
// Helper function for those above.
Vector<uint8_t> _webp_packer(const Ref<Image> &p_image, float p_quality, bool p_lossless);
// Given a WebP file, unpack it into an image.
Ref<Image> _webp_unpack(const Vector<uint8_t> &p_buffer);
Error webp_load_image_from_buffer(Image *p_image, const uint8_t *p_buffer, int p_buffer_len);
} //namespace WebPCommon

#include "core/io/resource_saver.h"

class ResourceSaverWebP final : public ResourceFormatSaver
{
public:
	static Error save_image(const String& p_path, const Ref<Image>& p_img,
		const bool p_lossy = false, const float p_quality = 0.75f);
	static Vector<uint8_t> save_image_to_buffer(
		const Ref<Image>& p_img, const bool p_lossy = false, const float p_quality = 0.75f);

	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceSaverWebP();
};
