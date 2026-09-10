/**************************************************************************/
/*  texture_rect.cpp                                                      */
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

#include "scene/resources/atlas_texture.h"
#include "servers/rendering/rendering_server.h"
#include "texture_rect.h"

Size2 TextureRect::get_minimum_size() const
{
	if (texture.is_valid()) {
		switch (expand_mode) {
		case EXPAND_KEEP_SIZE: {
			return texture->get_size();
		} break;
		case EXPAND_IGNORE_SIZE: {
			return Size2();
		} break;
		case EXPAND_FIT_WIDTH: {
			return Size2(get_size().y, 0);
		} break;
		case EXPAND_FIT_WIDTH_PROPORTIONAL: {
			real_t ratio = real_t(texture->get_width()) / texture->get_height();
			return Size2(get_size().y * ratio, 0);
		} break;
		case EXPAND_FIT_HEIGHT: {
			return Size2(0, get_size().x);
		} break;
		case EXPAND_FIT_HEIGHT_PROPORTIONAL: {
			real_t ratio = real_t(texture->get_height()) / texture->get_width();
			return Size2(0, get_size().x * ratio);
		} break;
		}
	}
	return Size2();
}

PackedStringArray TextureRect::get_configuration_warnings() const
{
	PackedStringArray warnings = Control::get_configuration_warnings();

	if (stretch_mode == STRETCH_TILE) {
		Ref<AtlasTexture> at = texture;
		while (at.is_valid()) {
			if (at->get_margin() != Rect2()) {
				warnings.push_back(vformat(RTR("STRETCH_TILE mode is not supported for an "
											   "AtlasTexture with non-zero margin.")));
				break;
			}
			at = at->get_atlas();
		}
	}

	return warnings;
}

Ref<Texture2D> TextureRect::get_texture() const { return texture; }

TextureRect::ExpandMode TextureRect::get_expand_mode() const { return expand_mode; }

TextureRect::StretchMode TextureRect::get_stretch_mode() const { return stretch_mode; }

bool TextureRect::is_flipped_h() const { return hflip; }

bool TextureRect::is_flipped_v() const { return vflip; }

TextureRect::TextureRect() { set_mouse_filter(MOUSE_FILTER_PASS); }

TextureRect::~TextureRect() {}


