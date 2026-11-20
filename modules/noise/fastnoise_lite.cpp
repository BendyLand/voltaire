/**************************************************************************/
/*  fastnoise_lite.cpp                                                    */
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

#include <cfloat> // FLT_MAX
#include "core/config/engine.h"
#include "fastnoise_lite.h"

_FastNoiseLite::FractalType FastNoiseLite::_convert_domain_warp_fractal_type_enum(
	DomainWarpFractalType p_domain_warp_fractal_type)
{
	_FastNoiseLite::FractalType type;
	switch (p_domain_warp_fractal_type) {
	case DOMAIN_WARP_FRACTAL_NONE:
		type = _FastNoiseLite::FractalType_None;
		break;
	case DOMAIN_WARP_FRACTAL_PROGRESSIVE:
		type = _FastNoiseLite::FractalType_DomainWarpProgressive;
		break;
	case DOMAIN_WARP_FRACTAL_INDEPENDENT:
		type = _FastNoiseLite::FractalType_DomainWarpIndependent;
		break;
	default:
		type = _FastNoiseLite::FractalType_None;
	}
	return type;
}

FastNoiseLite::FastNoiseLite()
{
	_noise.SetNoiseType((_FastNoiseLite::NoiseType)noise_type);
	_noise.SetSeed(seed);
	_noise.SetFrequency(frequency);

	_noise.SetFractalType((_FastNoiseLite::FractalType)fractal_type);
	_noise.SetFractalOctaves(fractal_octaves);
	_noise.SetFractalLacunarity(fractal_lacunarity);
	_noise.SetFractalGain(fractal_gain);
	_noise.SetFractalWeightedStrength(fractal_weighted_strength);
	_noise.SetFractalPingPongStrength(fractal_ping_pong_strength);

	_noise.SetCellularDistanceFunction(
		(_FastNoiseLite::CellularDistanceFunction)cellular_distance_function);
	_noise.SetCellularReturnType((_FastNoiseLite::CellularReturnType)cellular_return_type);
	_noise.SetCellularJitter(cellular_jitter);

	_domain_warp_noise.SetDomainWarpType((_FastNoiseLite::DomainWarpType)domain_warp_type);
	_domain_warp_noise.SetSeed(seed);
	_domain_warp_noise.SetDomainWarpAmp(domain_warp_amplitude);
	_domain_warp_noise.SetFrequency(domain_warp_frequency);
	_domain_warp_noise.SetFractalType(
		_convert_domain_warp_fractal_type_enum(domain_warp_fractal_type));
	_domain_warp_noise.SetFractalOctaves(domain_warp_fractal_octaves);
	_domain_warp_noise.SetFractalLacunarity(domain_warp_fractal_lacunarity);
	_domain_warp_noise.SetFractalGain(domain_warp_fractal_gain);
}

FastNoiseLite::~FastNoiseLite() {}

// General settings.

void FastNoiseLite::set_noise_type(NoiseType p_noise_type)
{
	noise_type = p_noise_type;
	_noise.SetNoiseType((_FastNoiseLite::NoiseType)p_noise_type);
	emit_changed();
}

FastNoiseLite::NoiseType FastNoiseLite::get_noise_type() const { return noise_type; }

void FastNoiseLite::set_seed(int p_seed)
{
	seed = p_seed;
	_noise.SetSeed(p_seed);
	_domain_warp_noise.SetSeed(p_seed);
	emit_changed();
}

int FastNoiseLite::get_seed() const { return seed; }

void FastNoiseLite::set_frequency(real_t p_freq)
{
	frequency = p_freq;
	_noise.SetFrequency(p_freq);
	emit_changed();
}

real_t FastNoiseLite::get_frequency() const { return frequency; }

void FastNoiseLite::set_offset(Vector3 p_offset)
{
	offset = p_offset;
	emit_changed();
}

Vector3 FastNoiseLite::get_offset() const { return offset; }

// Fractal.

void FastNoiseLite::set_fractal_type(FractalType p_type)
{
	fractal_type = p_type;
	_noise.SetFractalType((_FastNoiseLite::FractalType)p_type);
	emit_changed();
}

FastNoiseLite::FractalType FastNoiseLite::get_fractal_type() const { return fractal_type; }

void FastNoiseLite::set_fractal_octaves(int p_octaves)
{
	fractal_octaves = p_octaves;
	_noise.SetFractalOctaves(p_octaves);
	emit_changed();
}

int FastNoiseLite::get_fractal_octaves() const { return fractal_octaves; }

void FastNoiseLite::set_fractal_lacunarity(real_t p_lacunarity)
{
	fractal_lacunarity = p_lacunarity;
	_noise.SetFractalLacunarity(p_lacunarity);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_lacunarity() const { return fractal_lacunarity; }

void FastNoiseLite::set_fractal_gain(real_t p_gain)
{
	fractal_gain = p_gain;
	_noise.SetFractalGain(p_gain);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_gain() const { return fractal_gain; }

void FastNoiseLite::set_fractal_weighted_strength(real_t p_weighted_strength)
{
	fractal_weighted_strength = p_weighted_strength;
	_noise.SetFractalWeightedStrength(p_weighted_strength);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_weighted_strength() const { return fractal_weighted_strength; }

void FastNoiseLite::set_fractal_ping_pong_strength(real_t p_ping_pong_strength)
{
	fractal_ping_pong_strength = p_ping_pong_strength;
	_noise.SetFractalPingPongStrength(p_ping_pong_strength);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_ping_pong_strength() const { return fractal_ping_pong_strength; }

// Cellular.

void FastNoiseLite::set_cellular_distance_function(CellularDistanceFunction p_func)
{
	cellular_distance_function = p_func;
	_noise.SetCellularDistanceFunction((_FastNoiseLite::CellularDistanceFunction)p_func);
	emit_changed();
}

FastNoiseLite::CellularDistanceFunction FastNoiseLite::get_cellular_distance_function() const
{
	return cellular_distance_function;
}

void FastNoiseLite::set_cellular_jitter(real_t p_jitter)
{
	cellular_jitter = p_jitter;
	_noise.SetCellularJitter(p_jitter);
	emit_changed();
}

real_t FastNoiseLite::get_cellular_jitter() const { return cellular_jitter; }

void FastNoiseLite::set_cellular_return_type(CellularReturnType p_ret)
{
	cellular_return_type = p_ret;
	_noise.SetCellularReturnType((_FastNoiseLite::CellularReturnType)p_ret);
	emit_changed();
}

FastNoiseLite::CellularReturnType FastNoiseLite::get_cellular_return_type() const
{
	return cellular_return_type;
}

// Domain warp specific.

void FastNoiseLite::set_domain_warp_enabled(bool p_enabled)
{
	if (domain_warp_enabled != p_enabled) {
		domain_warp_enabled = p_enabled;
		emit_changed();
	}
}

bool FastNoiseLite::is_domain_warp_enabled() const { return domain_warp_enabled; }

void FastNoiseLite::set_domain_warp_type(DomainWarpType p_domain_warp_type)
{
	domain_warp_type = p_domain_warp_type;
	_domain_warp_noise.SetDomainWarpType((_FastNoiseLite::DomainWarpType)p_domain_warp_type);
	emit_changed();
}

FastNoiseLite::DomainWarpType FastNoiseLite::get_domain_warp_type() const
{
	return domain_warp_type;
}

void FastNoiseLite::set_domain_warp_amplitude(real_t p_amplitude)
{
	domain_warp_amplitude = p_amplitude;
	_domain_warp_noise.SetDomainWarpAmp(p_amplitude);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_amplitude() const { return domain_warp_amplitude; }

void FastNoiseLite::set_domain_warp_frequency(real_t p_frequency)
{
	domain_warp_frequency = p_frequency;
	_domain_warp_noise.SetFrequency(p_frequency);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_frequency() const { return domain_warp_frequency; }

void FastNoiseLite::set_domain_warp_fractal_type(DomainWarpFractalType p_domain_warp_fractal_type)
{
	domain_warp_fractal_type = p_domain_warp_fractal_type;

	_domain_warp_noise.SetFractalType(
		_convert_domain_warp_fractal_type_enum(p_domain_warp_fractal_type));
	emit_changed();
}

FastNoiseLite::DomainWarpFractalType FastNoiseLite::get_domain_warp_fractal_type() const
{
	return domain_warp_fractal_type;
}

void FastNoiseLite::set_domain_warp_fractal_octaves(int p_octaves)
{
	domain_warp_fractal_octaves = p_octaves;
	_domain_warp_noise.SetFractalOctaves(p_octaves);
	emit_changed();
}

int FastNoiseLite::get_domain_warp_fractal_octaves() const { return domain_warp_fractal_octaves; }

void FastNoiseLite::set_domain_warp_fractal_lacunarity(real_t p_lacunarity)
{
	domain_warp_fractal_lacunarity = p_lacunarity;
	_domain_warp_noise.SetFractalLacunarity(p_lacunarity);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_fractal_lacunarity() const
{
	return domain_warp_fractal_lacunarity;
}

void FastNoiseLite::set_domain_warp_fractal_gain(real_t p_gain)
{
	domain_warp_fractal_gain = p_gain;
	_domain_warp_noise.SetFractalGain(p_gain);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_fractal_gain() const { return domain_warp_fractal_gain; }

// Noise evaluation & image generation functions.

real_t FastNoiseLite::get_noise_1d(real_t p_x) const
{
	p_x += offset.x;
	if (domain_warp_enabled) {
		// Needed since DomainWarp expects a reference.
		real_t y_dummy = 0;
		_domain_warp_noise.DomainWarp(p_x, y_dummy);
	}
	return get_noise_2d(p_x, 0.0);
}

real_t FastNoiseLite::get_noise_2dv(Vector2 p_v) const { return get_noise_2d(p_v.x, p_v.y); }

real_t FastNoiseLite::get_noise_2d(real_t p_x, real_t p_y) const
{
	p_x += offset.x;
	p_y += offset.y;
	if (domain_warp_enabled) {
		_domain_warp_noise.DomainWarp(p_x, p_y);
	}
	return _noise.GetNoise(p_x, p_y);
}

real_t FastNoiseLite::get_noise_3dv(Vector3 p_v) const { return get_noise_3d(p_v.x, p_v.y, p_v.z); }

real_t FastNoiseLite::get_noise_3d(real_t p_x, real_t p_y, real_t p_z) const
{
	p_x += offset.x;
	p_y += offset.y;
	p_z += offset.z;
	if (domain_warp_enabled) {
		_domain_warp_noise.DomainWarp(p_x, p_y, p_z);
	}
	return _noise.GetNoise(p_x, p_y, p_z);
}

// Template specialization for faster grayscale blending.
template <> uint8_t FastNoiseLite::_alpha_blend<uint8_t>(uint8_t p_bg, uint8_t p_fg, int p_alpha) const
{
	uint16_t alpha = p_alpha + 1;
	uint16_t inv_alpha = 256 - p_alpha;

	return (uint8_t)((alpha * p_fg + inv_alpha * p_bg) >> 8);
}

Vector<Ref<Image>> FastNoiseLite::_get_seamless_image(int p_width, int p_height, int p_depth,
	bool p_invert, bool p_in_3d_space, real_t p_blend_skirt, bool p_normalize) const
{
	ERR_FAIL_COND_V(p_width <= 0 || p_height <= 0 || p_depth <= 0, Vector<Ref<Image>>());

	int skirt_width = MAX(1, p_width * p_blend_skirt);
	int skirt_height = MAX(1, p_height * p_blend_skirt);
	int skirt_depth = MAX(1, p_depth * p_blend_skirt);
	int src_width = p_width + skirt_width;
	int src_height = p_height + skirt_height;
	int src_depth = p_depth + skirt_depth;

	Vector<Ref<Image>> src =
		_get_image(src_width, src_height, src_depth, p_invert, p_in_3d_space, p_normalize);
	bool grayscale = (src[0]->get_format() == Image::FORMAT_L8);

	if (grayscale) {
		return _generate_seamless_image<uint8_t>(
			src, p_width, p_height, p_depth, p_invert, p_blend_skirt);
	}
	else {
		return _generate_seamless_image<uint32_t>(
			src, p_width, p_height, p_depth, p_invert, p_blend_skirt);
	}
}

Ref<Image> FastNoiseLite::get_seamless_image(int p_width, int p_height, bool p_invert,
	bool p_in_3d_space, real_t p_blend_skirt, bool p_normalize) const
{
	Vector<Ref<Image>> images = _get_seamless_image(
		p_width, p_height, 1, p_invert, p_in_3d_space, p_blend_skirt, p_normalize);
	if (images.is_empty()) {
		return Ref<Image>();
	}
	return images[0];
}

Vector<Ref<Image>> FastNoiseLite::_get_image(int p_width, int p_height, int p_depth, bool p_invert,
	bool p_in_3d_space, bool p_normalize) const
{
	ERR_FAIL_COND_V(p_width <= 0 || p_height <= 0 || p_depth <= 0, Vector<Ref<Image>>());

	Vector<Ref<Image>> images;
	images.resize(p_depth);

	if (p_normalize) {
		// Get all values and identify min/max values.
		LocalVector<real_t> values;
		values.resize(p_width * p_height * p_depth);

		real_t min_val = FLT_MAX;
		real_t max_val = -FLT_MAX;
		int idx = 0;
		for (int d = 0; d < p_depth; d++) {
			for (int y = 0; y < p_height; y++) {
				for (int x = 0; x < p_width; x++) {
					values[idx] = p_in_3d_space ? get_noise_3d(x, y, d) : get_noise_2d(x, y);
					if (values[idx] > max_val) {
						max_val = values[idx];
					}
					if (values[idx] < min_val) {
						min_val = values[idx];
					}
					idx++;
				}
			}
		}
		idx = 0;
		// Normalize values and write to texture.
		for (int d = 0; d < p_depth; d++) {
			Vector<uint8_t> data;
			data.resize(p_width * p_height);

			uint8_t* wd8 = data.ptrw();
			uint8_t ivalue;

			for (int y = 0; y < p_height; y++) {
				for (int x = 0; x < p_width; x++) {
					if (max_val == min_val) {
						ivalue = 0;
					}
					else {
						ivalue = static_cast<uint8_t>(
							CLAMP((values[idx] - min_val) / (max_val - min_val) * 255.f, 0, 255));
					}

					if (p_invert) {
						ivalue = 255 - ivalue;
					}

					wd8[x + y * p_width] = ivalue;
					idx++;
				}
			}
			Ref<Image> img = memnew(Image(p_width, p_height, false, Image::FORMAT_L8, data));
			images.write[d] = img;
		}
	}
	else {
		// Without normalization, the expected range of the noise function is [-1, 1].
		for (int d = 0; d < p_depth; d++) {
			Vector<uint8_t> data;
			data.resize(p_width * p_height);

			uint8_t* wd8 = data.ptrw();

			uint8_t ivalue;
			int idx = 0;
			for (int y = 0; y < p_height; y++) {
				for (int x = 0; x < p_width; x++) {
					float value = (p_in_3d_space ? get_noise_3d(x, y, d) : get_noise_2d(x, y));
					ivalue = static_cast<uint8_t>(CLAMP(value * 127.5f + 127.5f, 0.0f, 255.0f));
					wd8[idx] = p_invert ? (255 - ivalue) : ivalue;
					idx++;
				}
			}

			Ref<Image> img = memnew(Image(p_width, p_height, false, Image::FORMAT_L8, data));
			images.write[d] = img;
		}
	}

	return images;
}

Ref<Image> FastNoiseLite::get_image(
	int p_width, int p_height, bool p_invert, bool p_in_3d_space, bool p_normalize) const
{
	Vector<Ref<Image>> images =
		_get_image(p_width, p_height, 1, p_invert, p_in_3d_space, p_normalize);
	if (images.is_empty()) {
		return Ref<Image>();
	}
	return images[0];
}

void FastNoiseLite::_changed() { emit_changed(); }
