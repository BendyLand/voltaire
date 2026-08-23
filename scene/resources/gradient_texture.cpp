/**************************************************************************/
/*  gradient_texture.cpp                                                  */
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

#include "core/math/geometry_2d.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "gradient_texture.h"
#include "servers/rendering/rendering_server.h"

GradientTexture1D::GradientTexture1D() { _queue_update(); }

GradientTexture1D::~GradientTexture1D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RS::get_singleton()->free_rid(texture);
	}
}

void GradientTexture1D::_bind_methods() {}

void GradientTexture1D::set_gradient(Ref<Gradient> p_gradient)
{
	if (p_gradient == gradient) {
		return;
	}
	if (gradient.is_valid()) {
		gradient->disconnect_changed(callable_mp(this, &GradientTexture1D::_queue_update));
	}
	gradient = p_gradient;
	if (gradient.is_valid()) {
		gradient->connect_changed(callable_mp(this, &GradientTexture1D::_queue_update));
	}
	_queue_update();
	emit_changed();
}

Ref<Gradient> GradientTexture1D::get_gradient() const { return gradient; }

void GradientTexture1D::_queue_update()
{
	if (update_pending) {
		return;
	}
	update_pending = true;
	callable_mp(this, &GradientTexture1D::update_now).call_deferred();
}

void GradientTexture1D::_update() const
{
	update_pending = false;

	if (gradient.is_null()) {
		return;
	}

	if (use_hdr) {
		// High dynamic range.
		Ref<Image> image = memnew(Image(width, 1, false, Image::FORMAT_RGBAF));
		Gradient& g = **gradient;
		// `create()` isn't available for non-uint8_t data, so fill in the data manually.
		for (int i = 0; i < width; i++) {
			float ofs = float(i) / (width - 1);
			image->set_pixel(i, 0, g.get_color_at_offset(ofs));
		}

		if (texture.is_valid()) {
			RID new_texture = RS::get_singleton()->texture_2d_create(image);
			RS::get_singleton()->texture_replace(texture, new_texture);
		}
		else {
			texture = RS::get_singleton()->texture_2d_create(image);
		}
	}
	else {
		// Low dynamic range. "Overbright" colors will be clamped.
		Vector<uint8_t> data;
		data.resize(width * 4);
		{
			uint8_t* wd8 = data.ptrw();
			Gradient& g = **gradient;

			for (int i = 0; i < width; i++) {
				float ofs = float(i) / (width - 1);
				Color color = g.get_color_at_offset(ofs);

				wd8[i * 4 + 0] = uint8_t(color.get_r8());
				wd8[i * 4 + 1] = uint8_t(color.get_g8());
				wd8[i * 4 + 2] = uint8_t(color.get_b8());
				wd8[i * 4 + 3] = uint8_t(color.get_a8());
			}
		}

		Ref<Image> image = memnew(Image(width, 1, false, Image::FORMAT_RGBA8, data));

		if (texture.is_valid()) {
			RID new_texture = RS::get_singleton()->texture_2d_create(image);
			RS::get_singleton()->texture_replace(texture, new_texture);
		}
		else {
			texture = RS::get_singleton()->texture_2d_create(image);
		}
	}
	RS::get_singleton()->texture_set_path(texture, get_path());
}

void GradientTexture1D::set_width(int p_width)
{
	ERR_FAIL_COND_MSG(
		p_width <= 0 || p_width > 16384, "Texture dimensions have to be within 1 to 16384 range.");
	width = p_width;
	_queue_update();
	emit_changed();
}

int GradientTexture1D::get_width() const { return width; }

void GradientTexture1D::set_use_hdr(bool p_enabled)
{
	if (p_enabled == use_hdr) {
		return;
	}

	use_hdr = p_enabled;
	_queue_update();
	emit_changed();
}

bool GradientTexture1D::is_using_hdr() const { return use_hdr; }

RID GradientTexture1D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = RS::get_singleton()->texture_2d_placeholder_create();
	}
	return texture;
}

Ref<Image> GradientTexture1D::get_image() const
{
	update_now();
	if (!texture.is_valid()) {
		return Ref<Image>();
	}
	return RenderingServer::get_singleton()->texture_2d_get(texture);
}

void GradientTexture1D::update_now() const
{
	if (update_pending) {
		_update();
	}
}

//////////////////

GradientTexture2D::GradientTexture2D() { _queue_update(); }

GradientTexture2D::~GradientTexture2D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RS::get_singleton()->free_rid(texture);
	}
}

void GradientTexture2D::set_gradient(Ref<Gradient> p_gradient)
{
	if (gradient == p_gradient) {
		return;
	}
	if (gradient.is_valid()) {
		gradient->disconnect_changed(callable_mp(this, &GradientTexture2D::_queue_update));
	}
	gradient = p_gradient;
	if (gradient.is_valid()) {
		gradient->connect_changed(callable_mp(this, &GradientTexture2D::_queue_update));
	}
	_queue_update();
	emit_changed();
}

Ref<Gradient> GradientTexture2D::get_gradient() const { return gradient; }

void GradientTexture2D::_queue_update()
{
	if (update_pending) {
		return;
	}
	update_pending = true;
	callable_mp(this, &GradientTexture2D::update_now).call_deferred();
}

void GradientTexture2D::_update() const
{
	update_pending = false;

	if (gradient.is_null()) {
		return;
	}
	Ref<Image> image;
	image.instantiate();

	if (gradient->get_point_count() <= 1) { // No need to interpolate.
		image->initialize_data(
			width, height, false, (use_hdr) ? Image::FORMAT_RGBAF : Image::FORMAT_RGBA8);
		image->fill(
			(gradient->get_point_count() == 1) ? gradient->get_color(0) : Color(0, 0, 0, 1));
	}
	else {
		if (use_hdr) {
			image->initialize_data(width, height, false, Image::FORMAT_RGBAF);
			Gradient& g = **gradient;
			// `create()` isn't available for non-uint8_t data, so fill in the data manually.
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
					float ofs = _get_gradient_offset_at(x, y);
					image->set_pixel(x, y, g.get_color_at_offset(ofs));
				}
			}
		}
		else {
			Vector<uint8_t> data;
			data.resize(width * height * 4);
			{
				uint8_t* wd8 = data.ptrw();
				Gradient& g = **gradient;
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
						float ofs = _get_gradient_offset_at(x, y);
						const Color& c = g.get_color_at_offset(ofs);

						wd8[(x + (y * width)) * 4 + 0] = uint8_t(c.get_r8());
						wd8[(x + (y * width)) * 4 + 1] = uint8_t(c.get_g8());
						wd8[(x + (y * width)) * 4 + 2] = uint8_t(c.get_b8());
						wd8[(x + (y * width)) * 4 + 3] = uint8_t(c.get_a8());
					}
				}
			}
			image->set_data(width, height, false, Image::FORMAT_RGBA8, data);
		}
	}

	if (texture.is_valid()) {
		RID new_texture = RS::get_singleton()->texture_2d_create(image);
		RS::get_singleton()->texture_replace(texture, new_texture);
	}
	else {
		texture = RS::get_singleton()->texture_2d_create(image);
	}
	RS::get_singleton()->texture_set_path(texture, get_path());
}

float GradientTexture2D::_get_gradient_offset_at(int x, int y) const
{
	if (fill_to == fill_from) {
		return 0;
	}
	float ofs = 0;
	Vector2 pos;
	if (width > 1) {
		pos.x = static_cast<float>(x) / (width - 1);
	}
	if (height > 1) {
		pos.y = static_cast<float>(y) / (height - 1);
	}
	if (fill == Fill::FILL_LINEAR) {
		const Vector2 closest =
			Geometry2D::get_closest_point_to_segment_uncapped(pos, fill_from, fill_to);
		ofs = (closest - fill_from).length() / (fill_to - fill_from).length();
		if ((closest - fill_from).dot(fill_to - fill_from) < 0) {
			ofs *= -1;
		}
	}
	else if (fill == Fill::FILL_RADIAL) {
		ofs = (pos - fill_from).length() / (fill_to - fill_from).length();
	}
	else if (fill == Fill::FILL_SQUARE) {
		ofs = MAX(Math::abs(pos.x - fill_from.x), Math::abs(pos.y - fill_from.y)) /
			  MAX(Math::abs(fill_to.x - fill_from.x), Math::abs(fill_to.y - fill_from.y));
	}
	else if (fill == Fill::FILL_CONIC) {
		float rel_angle = (fill_to - fill_from).angle_to(pos - fill_from);
		ofs = Math::fposmod((double)rel_angle, Math::TAU) / Math::TAU;
	}
	if (repeat == Repeat::REPEAT_NONE) {
		ofs = CLAMP(ofs, 0.0, 1.0);
	}
	else if (repeat == Repeat::REPEAT) {
		ofs = Math::fmod(ofs, 1.0f);
		if (ofs < 0) {
			ofs = 1 + ofs;
		}
	}
	else if (repeat == Repeat::REPEAT_MIRROR) {
		ofs = Math::abs(ofs);
		ofs = Math::fmod(ofs, 2.0f);
		if (ofs > 1.0) {
			ofs = 2.0 - ofs;
		}
	}
	return ofs;
}

void GradientTexture2D::set_width(int p_width)
{
	ERR_FAIL_COND_MSG(
		p_width <= 0 || p_width > 16384, "Texture dimensions have to be within 1 to 16384 range.");
	width = p_width;
	_queue_update();
	emit_changed();
}

int GradientTexture2D::get_width() const { return width; }

void GradientTexture2D::set_height(int p_height)
{
	ERR_FAIL_COND_MSG(p_height <= 0 || p_height > 16384,
		"Texture dimensions have to be within 1 to 16384 range.");
	height = p_height;
	_queue_update();
	emit_changed();
}

int GradientTexture2D::get_height() const { return height; }

void GradientTexture2D::set_use_hdr(bool p_enabled)
{
	if (p_enabled == use_hdr) {
		return;
	}

	use_hdr = p_enabled;
	_queue_update();
	emit_changed();
}

bool GradientTexture2D::is_using_hdr() const { return use_hdr; }

void GradientTexture2D::set_fill_from(Vector2 p_fill_from)
{
	fill_from = p_fill_from;
	_queue_update();
	emit_changed();
}

Vector2 GradientTexture2D::get_fill_from() const { return fill_from; }

void GradientTexture2D::set_fill_to(Vector2 p_fill_to)
{
	fill_to = p_fill_to;
	_queue_update();
	emit_changed();
}

Vector2 GradientTexture2D::get_fill_to() const { return fill_to; }

void GradientTexture2D::set_fill(Fill p_fill)
{
	fill = p_fill;
	_queue_update();
	emit_changed();
}

GradientTexture2D::Fill GradientTexture2D::get_fill() const { return fill; }

void GradientTexture2D::set_repeat(Repeat p_repeat)
{
	repeat = p_repeat;
	_queue_update();
	emit_changed();
}

GradientTexture2D::Repeat GradientTexture2D::get_repeat() const { return repeat; }

RID GradientTexture2D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = RS::get_singleton()->texture_2d_placeholder_create();
	}
	return texture;
}

Ref<Image> GradientTexture2D::get_image() const
{
	update_now();
	if (!texture.is_valid()) {
		return Ref<Image>();
	}
	return RenderingServer::get_singleton()->texture_2d_get(texture);
}

void GradientTexture2D::update_now() const
{
	if (update_pending) {
		_update();
	}
}

void GradientTexture2D::_bind_methods() {}


