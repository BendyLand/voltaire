/**************************************************************************/
/*  curve_texture.cpp                                                     */
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

#include "curve_texture.h"

#include "servers/rendering/renderer.h"

CurveTexture::CurveTexture(TextureMode p_mode) :
	texture_mode(p_mode), _current_texture_mode(p_mode)
{
}

void CurveTexture::set_width(int p_width)
{
	ERR_FAIL_COND(p_width < 32 || p_width > 4096);

	if (_width == p_width) {
		return;
	}

	_width = p_width;
	_update();
}

int CurveTexture::get_width() const { return _width; }

void CurveTexture::ensure_default_setup(float p_min, float p_max)
{
	if (texture_mode == TEXTURE_MODE_XYZ) {
		if (_curve_x.is_null()) {
			Ref<Curve> curve = Ref<Curve>(memnew(Curve));
			curve->add_point(Vector2(0, 1));
			curve->add_point(Vector2(1, 1));
			curve->set_min_value(p_min);
			curve->set_max_value(p_max);
			set_curve_x(curve);
		}
		if (_curve_y.is_null()) {
			Ref<Curve> curve = Ref<Curve>(memnew(Curve));
			curve->add_point(Vector2(0, 1));
			curve->add_point(Vector2(1, 1));
			curve->set_min_value(p_min);
			curve->set_max_value(p_max);
			set_curve_y(curve);
		}
		if (_curve_z.is_null()) {
			Ref<Curve> curve = Ref<Curve>(memnew(Curve));
			curve->add_point(Vector2(0, 1));
			curve->add_point(Vector2(1, 1));
			curve->set_min_value(p_min);
			curve->set_max_value(p_max);
			set_curve_z(curve);
		}
	}
	else if (_curve.is_null()) {
		Ref<Curve> curve = Ref<Curve>(memnew(Curve));
		curve->add_point(Vector2(0, 1));
		curve->add_point(Vector2(1, 1));
		curve->set_min_value(p_min);
		curve->set_max_value(p_max);
		set_curve(curve);
	}
}

void CurveTexture::set_curve(Ref<Curve> p_curve)
{
	if (_curve != p_curve) {
		_curve = p_curve;
		_update();
	}
}

Ref<Curve> CurveTexture::get_curve() const { return _curve; }

void CurveTexture::set_curve_x(Ref<Curve> p_curve)
{
	if (_curve_x == p_curve && texture_mode == TEXTURE_MODE_XYZ) {
		return;
	}
	_curve_x = p_curve;
	texture_mode = TEXTURE_MODE_XYZ;
	_update();
}

Ref<Curve> CurveTexture::get_curve_x() const { return _curve_x; }

void CurveTexture::set_curve_y(Ref<Curve> p_curve)
{
	if (_curve_y == p_curve && texture_mode == TEXTURE_MODE_XYZ) {
		return;
	}
	_curve_y = p_curve;
	texture_mode = TEXTURE_MODE_XYZ;
	_update();
}

Ref<Curve> CurveTexture::get_curve_y() const { return _curve_y; }

void CurveTexture::set_curve_z(Ref<Curve> p_curve)
{
	if (_curve_z == p_curve && texture_mode == TEXTURE_MODE_XYZ) {
		return;
	}
	_curve_z = p_curve;
	texture_mode = TEXTURE_MODE_XYZ;
	_update();
}

Ref<Curve> CurveTexture::get_curve_z() const { return _curve_z; }

void CurveTexture::set_texture_mode(TextureMode p_mode)
{
	ERR_FAIL_COND(p_mode < TEXTURE_MODE_RGB || p_mode > TEXTURE_MODE_XYZ);
	if (texture_mode == p_mode) {
		return;
	}
	texture_mode = p_mode;
	_update();
}

CurveTexture::TextureMode CurveTexture::get_texture_mode() const { return texture_mode; }

void CurveTexture::_update()
{
	if (!Renderer::is_initialized()) {
		emit_changed();
		return;
	}

	const int channels = (texture_mode == TEXTURE_MODE_RED) ? 1 : 3;
	Vector<uint8_t> data;
	data.resize(_width * sizeof(float) * channels);

	{
		uint8_t* wd8 = data.ptrw();
		float* wd = reinterpret_cast<float*>(wd8);

		if (texture_mode == TEXTURE_MODE_XYZ) {
			if (_curve_x.is_valid()) {
				Curve& curve_x = *_curve_x;
				for (int i = 0; i < _width; ++i) {
					float t = i / static_cast<float>(_width);
					wd[i * 3 + 0] = curve_x.sample_baked(t);
				}
			}
			else {
				for (int i = 0; i < _width; ++i) {
					wd[i * 3 + 0] = 0.0f;
				}
			}

			if (_curve_y.is_valid()) {
				Curve& curve_y = *_curve_y;
				for (int i = 0; i < _width; ++i) {
					float t = i / static_cast<float>(_width);
					wd[i * 3 + 1] = curve_y.sample_baked(t);
				}
			}
			else {
				for (int i = 0; i < _width; ++i) {
					wd[i * 3 + 1] = 0.0f;
				}
			}

			if (_curve_z.is_valid()) {
				Curve& curve_z = *_curve_z;
				for (int i = 0; i < _width; ++i) {
					float t = i / static_cast<float>(_width);
					wd[i * 3 + 2] = curve_z.sample_baked(t);
				}
			}
			else {
				for (int i = 0; i < _width; ++i) {
					wd[i * 3 + 2] = 0.0f;
				}
			}
		}
		else if (_curve.is_valid()) {
			Curve& curve = *_curve;
			for (int i = 0; i < _width; ++i) {
				float t = i / static_cast<float>(_width);
				float v = curve.sample_baked(t);
				if (texture_mode == TEXTURE_MODE_RGB) {
					wd[i * 3 + 0] = v;
					wd[i * 3 + 1] = v;
					wd[i * 3 + 2] = v;
				}
				else {
					wd[i] = v;
				}
			}
		}
		else {
			for (int i = 0; i < _width * channels; ++i) {
				wd[i] = 0.0f;
			}
		}
	}

	const Image::Format new_format =
		(texture_mode == TEXTURE_MODE_RED) ? Image::FORMAT_RF : Image::FORMAT_RGBF;
	const Image::Format old_format =
		(_current_texture_mode == TEXTURE_MODE_RED) ? Image::FORMAT_RF : Image::FORMAT_RGBF;

	Ref<Image> image = memnew(Image(_width, 1, false, new_format, data));

	if (_texture.is_valid()) {
		if (old_format != new_format || _current_width != _width) {
			RID new_texture = Renderer::texture_2d_create(image);
			Renderer::texture_replace(_texture, new_texture);
		}
		else {
			Renderer::texture_2d_update(_texture, image);
		}
	}
	else {
		_texture = Renderer::texture_2d_create(image);
	}
	_current_texture_mode = texture_mode;
	_current_width = _width;

	emit_changed();
}

RID CurveTexture::get_rid() const
{
	if (!_texture.is_valid() && Renderer::is_initialized()) {
		if (_curve.is_valid() || _curve_x.is_valid() || _curve_y.is_valid() || _curve_z.is_valid()) {
			const_cast<CurveTexture*>(this)->_update();
		}
		if (!_texture.is_valid()) {
			_texture = Renderer::texture_2d_placeholder_create();
		}
	}
	return _texture;
}

CurveTexture::~CurveTexture()
{
	if (_texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(_texture);
	}
}
