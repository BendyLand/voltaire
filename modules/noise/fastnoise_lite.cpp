#include "modules/modules_enabled.gen.h"

#if defined(MODULE_NOISE_ENABLED)
#include <cfloat> // FLT_MAX
#include "modules/register_module_types.h"
#include "servers/rendering/renderer.h"
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

NoiseTexture2D::NoiseTexture2D()
{
	noise = Ref<FastNoiseLite>();

	_queue_update();
}

void NoiseTexture2D::_set_texture_image(const Ref<Image>& p_image)
{
	image = p_image;
	if (image.is_valid()) {
		if (texture.is_valid()) {
			RID new_texture = Renderer::texture_2d_create(p_image);
			Renderer::texture_replace(texture, new_texture);
		}
		else {
			texture = Renderer::texture_2d_create(p_image);
		}
		Renderer::texture_set_path(texture, get_path());
	}
	emit_changed();
}

Ref<Image> NoiseTexture2D::_generate_texture()
{
	// Prevent memdelete due to unref() on other thread.
	Ref<FastNoiseLite> ref_noise = noise;

	if (ref_noise.is_null()) {
		return Ref<Image>();
	}

	Ref<Image> new_image;

	if (seamless) {
		new_image = ref_noise->get_seamless_image(
			size.x, size.y, invert, in_3d_space, seamless_blend_skirt, normalize);
	}
	else {
		new_image = ref_noise->get_image(size.x, size.y, invert, in_3d_space, normalize);
	}
	if (color_ramp.is_valid()) {
		new_image = _modulate_with_gradient(new_image, color_ramp);
	}
	if (as_normal_map) {
		new_image->bump_map_to_normal_map(bump_strength);
	}
	if (generate_mipmaps) {
		new_image->generate_mipmaps();
	}

	return new_image;
}

Ref<Image> NoiseTexture2D::_modulate_with_gradient(Ref<Image> p_image, Ref<Gradient> p_gradient)
{
	int width = p_image->get_width();
	int height = p_image->get_height();

	Ref<Image> new_image = Image::create_empty(width, height, false, Image::FORMAT_RGBA8);

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			Color pixel_color = p_image->get_pixel(col, row);
			Color ramp_color = p_gradient->get_color_at_offset(pixel_color.get_luminance());
			new_image->set_pixel(col, row, ramp_color);
		}
	}

	return new_image;
}

void NoiseTexture2D::set_noise(Ref<FastNoiseLite> p_noise)
{
	if (noise == p_noise) {
		return;
	}
	noise = p_noise;
	_queue_update();
}

Ref<FastNoiseLite> NoiseTexture2D::get_noise() { return noise; }

void NoiseTexture2D::set_width(int p_width)
{
	ERR_FAIL_COND(p_width <= 0);
	if (p_width == size.x) {
		return;
	}
	size.x = p_width;
	_queue_update();
}

void NoiseTexture2D::set_height(int p_height)
{
	ERR_FAIL_COND(p_height <= 0);
	if (p_height == size.y) {
		return;
	}
	size.y = p_height;
	_queue_update();
}

void NoiseTexture2D::set_invert(bool p_invert)
{
	if (p_invert == invert) {
		return;
	}
	invert = p_invert;
	_queue_update();
}

bool NoiseTexture2D::get_invert() const { return invert; }

void NoiseTexture2D::set_in_3d_space(bool p_enable)
{
	if (p_enable == in_3d_space) {
		return;
	}
	in_3d_space = p_enable;
	_queue_update();
}

bool NoiseTexture2D::is_in_3d_space() const { return in_3d_space; }

void NoiseTexture2D::set_generate_mipmaps(bool p_enable)
{
	if (p_enable == generate_mipmaps) {
		return;
	}
	generate_mipmaps = p_enable;
	_queue_update();
}

bool NoiseTexture2D::is_generating_mipmaps() const { return generate_mipmaps; }

void NoiseTexture2D::set_seamless(bool p_seamless)
{
	if (p_seamless == seamless) {
		return;
	}
	seamless = p_seamless;
	_queue_update();
}

bool NoiseTexture2D::get_seamless() { return seamless; }

void NoiseTexture2D::set_seamless_blend_skirt(real_t p_blend_skirt)
{
	ERR_FAIL_COND(p_blend_skirt < 0 || p_blend_skirt > 1);

	if (p_blend_skirt == seamless_blend_skirt) {
		return;
	}
	seamless_blend_skirt = p_blend_skirt;
	_queue_update();
}

real_t NoiseTexture2D::get_seamless_blend_skirt() { return seamless_blend_skirt; }

void NoiseTexture2D::set_as_normal_map(bool p_as_normal_map)
{
	if (p_as_normal_map == as_normal_map) {
		return;
	}
	as_normal_map = p_as_normal_map;
	_queue_update();
}

bool NoiseTexture2D::is_normal_map() { return as_normal_map; }

void NoiseTexture2D::set_bump_strength(float p_bump_strength)
{
	if (p_bump_strength == bump_strength) {
		return;
	}
	bump_strength = p_bump_strength;
	if (as_normal_map) {
		_queue_update();
	}
}

float NoiseTexture2D::get_bump_strength() { return bump_strength; }

void NoiseTexture2D::set_normalize(bool p_normalize)
{
	if (normalize == p_normalize) {
		return;
	}
	normalize = p_normalize;
	_queue_update();
}

bool NoiseTexture2D::is_normalized() const { return normalize; }

void NoiseTexture2D::set_color_ramp(const Ref<Gradient>& p_gradient)
{
	if (color_ramp == p_gradient) {
		return;
	}
	color_ramp = p_gradient;
	_queue_update();
}

Ref<Gradient> NoiseTexture2D::get_color_ramp() const { return color_ramp; }

int NoiseTexture2D::get_width() const { return size.x; }

int NoiseTexture2D::get_height() const { return size.y; }

RID NoiseTexture2D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_2d_placeholder_create();
	}

	return texture;
}

Ref<Image> NoiseTexture2D::get_image() const { return image; }

void NoiseTexture2D::_queue_update() {}

NoiseTexture3D::NoiseTexture3D()
{
	noise = Ref<FastNoiseLite>();

	_queue_update();
}

Ref<Image> NoiseTexture3D::_modulate_with_gradient(Ref<Image> p_image, Ref<Gradient> p_gradient)
{
	int w = p_image->get_width();
	int h = p_image->get_height();

	Ref<Image> new_image = Image::create_empty(w, h, false, Image::FORMAT_RGBA8);

	for (int row = 0; row < h; row++) {
		for (int col = 0; col < w; col++) {
			Color pixel_color = p_image->get_pixel(col, row);
			Color ramp_color = p_gradient->get_color_at_offset(pixel_color.get_luminance());
			new_image->set_pixel(col, row, ramp_color);
		}
	}

	return new_image;
}

void NoiseTexture3D::set_noise(Ref<FastNoiseLite> p_noise)
{
	if (noise == p_noise) {
		return;
	}
	noise = p_noise;
	_queue_update();
}

Ref<FastNoiseLite> NoiseTexture3D::get_noise() { return noise; }

void NoiseTexture3D::set_width(int p_width)
{
	ERR_FAIL_COND(p_width <= 0);
	if (p_width == width) {
		return;
	}
	width = p_width;
	_queue_update();
}

void NoiseTexture3D::set_height(int p_height)
{
	ERR_FAIL_COND(p_height <= 0);
	if (p_height == height) {
		return;
	}
	height = p_height;
	_queue_update();
}

void NoiseTexture3D::set_depth(int p_depth)
{
	ERR_FAIL_COND(p_depth <= 0);
	if (p_depth == depth) {
		return;
	}
	depth = p_depth;
	_queue_update();
}

void NoiseTexture3D::set_invert(bool p_invert)
{
	if (p_invert == invert) {
		return;
	}
	invert = p_invert;
	_queue_update();
}

bool NoiseTexture3D::get_invert() const { return invert; }

void NoiseTexture3D::set_seamless(bool p_seamless)
{
	if (p_seamless == seamless) {
		return;
	}
	seamless = p_seamless;
	_queue_update();
}

bool NoiseTexture3D::get_seamless() { return seamless; }

void NoiseTexture3D::set_seamless_blend_skirt(real_t p_blend_skirt)
{
	ERR_FAIL_COND(p_blend_skirt < 0.05 || p_blend_skirt > 1);

	if (p_blend_skirt == seamless_blend_skirt) {
		return;
	}
	seamless_blend_skirt = p_blend_skirt;
	_queue_update();
}

real_t NoiseTexture3D::get_seamless_blend_skirt() { return seamless_blend_skirt; }

void NoiseTexture3D::set_normalize(bool p_normalize)
{
	if (normalize == p_normalize) {
		return;
	}
	normalize = p_normalize;
	_queue_update();
}

bool NoiseTexture3D::is_normalized() const { return normalize; }

void NoiseTexture3D::set_color_ramp(const Ref<Gradient>& p_gradient)
{
	if (color_ramp == p_gradient) {
		return;
	}
	color_ramp = p_gradient;
	_queue_update();
}

Ref<Gradient> NoiseTexture3D::get_color_ramp() const { return color_ramp; }

int NoiseTexture3D::get_width() const { return width; }

int NoiseTexture3D::get_height() const { return height; }

int NoiseTexture3D::get_depth() const { return depth; }

bool NoiseTexture3D::has_mipmaps() const { return false; }

RID NoiseTexture3D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_3d_placeholder_create();
	}

	return texture;
}

Vector<Ref<Image>> NoiseTexture3D::get_data() const
{
	ERR_FAIL_COND_V(!texture.is_valid(), Vector<Ref<Image>>());
	return Renderer::texture_3d_get(texture);
}

Image::Format NoiseTexture3D::get_format() const { return format; }

void NoiseTexture3D::_queue_update() {}

void initialize_noise_module(ModuleInitializationLevel p_level) {}

void uninitialize_noise_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_NOISE_ENABLED)


// --- Merged from modules/regex/regex.cpp ---
#if defined(MODULE_REGEX_ENABLED)
#include "core/os/memory.h"
#include "modules/register_module_types.h"
#include "modules/regex/regex.h"

extern "C"
{
#include <pcre2.h>
}

static void* _regex_malloc(PCRE2_SIZE size, void* user) { return memalloc(size); }

static void _regex_free(void* ptr, void* user)
{
	if (ptr) {
		memfree(ptr);
	}
}

String RegExMatch::get_subject() const { return subject; }

int RegExMatch::get_group_count() const
{
	if (data.is_empty()) {
		return 0;
	}
	return data.size() - 1;
}

PackedStringArray RegExMatch::get_strings() const
{
	PackedStringArray result;

	int size = data.size();

	for (int i = 0; i < size; i++) {
		int start = data[i].start;

		if (start == -1) {
			result.append(String());
			continue;
		}

		int length = data[i].end - start;

		result.append(subject.substr(start, length));
	}

	return result;
}

void RegEx::_pattern_info(uint32_t what, void* where) const
{
	pcre2_pattern_info_32((pcre2_code_32*)code, what, where);
}

Ref<RegEx> RegEx::create_from_string(const String& p_pattern, bool p_show_error)
{
	Ref<RegEx> ret;
	ret.instantiate();
	ret->compile(p_pattern, p_show_error);
	return ret;
}

void RegEx::clear()
{
	if (code) {
		pcre2_code_free_32((pcre2_code_32*)code);
		code = nullptr;
	}
}

Error RegEx::compile(const String& p_pattern, bool p_show_error)
{
	pattern = p_pattern;
	clear();

	int err;
	PCRE2_SIZE offset;
	uint32_t flags = PCRE2_DUPNAMES;

	pcre2_general_context_32* gctx = (pcre2_general_context_32*)general_ctx;
	pcre2_compile_context_32* cctx = pcre2_compile_context_create_32(gctx);
	PCRE2_SPTR32 p = (PCRE2_SPTR32)pattern.get_data();

	code = pcre2_compile_32(p, pattern.length(), flags, &err, &offset, cctx);

	pcre2_compile_context_free_32(cctx);

	if (!code) {
		if (p_show_error) {
			PCRE2_UCHAR32 buf[256];
			pcre2_get_error_message_32(err, buf, 256);
			String message = String::num_int64(offset) + ": " + String((const char32_t*)buf);
			ERR_PRINT(message);
		}
		return FAILED;
	}
	return OK;
}

Ref<RegExMatch> RegEx::search(const String& p_subject, int p_offset, int p_end) const
{
	ERR_FAIL_COND_V(!is_valid(), nullptr);
	ERR_FAIL_COND_V_MSG(p_offset < 0, nullptr, "RegEx search offset must be >= 0");

	Ref<RegExMatch> result = memnew(RegExMatch);

	int length = p_subject.length();
	if (p_end >= 0 && p_end < length) {
		length = p_end;
	}

	pcre2_code_32* c = (pcre2_code_32*)code;
	pcre2_general_context_32* gctx = (pcre2_general_context_32*)general_ctx;
	pcre2_match_context_32* mctx = pcre2_match_context_create_32(gctx);
	PCRE2_SPTR32 s = (PCRE2_SPTR32)p_subject.get_data();

	pcre2_match_data_32* match = pcre2_match_data_create_from_pattern_32(c, gctx);

	int res = pcre2_match_32(c, s, length, p_offset, 0, match, mctx);

	if (res < 0) {
		pcre2_match_data_free_32(match);
		pcre2_match_context_free_32(mctx);

		return nullptr;
	}

	uint32_t size = pcre2_get_ovector_count_32(match);
	PCRE2_SIZE* ovector = pcre2_get_ovector_pointer_32(match);

	result->data.resize(size);

	for (uint32_t i = 0; i < size; i++) {
		result->data.write[i].start = ovector[i * 2];
		result->data.write[i].end = ovector[i * 2 + 1];
	}

	pcre2_match_data_free_32(match);
	pcre2_match_context_free_32(mctx);

	result->subject = p_subject;

	uint32_t count;
	const char32_t* table;
	uint32_t entry_size;

	_pattern_info(PCRE2_INFO_NAMECOUNT, &count);
	_pattern_info(PCRE2_INFO_NAMETABLE, &table);
	_pattern_info(PCRE2_INFO_NAMEENTRYSIZE, &entry_size);

	for (uint32_t i = 0; i < count; i++) {
		char32_t id = table[i * entry_size];
		if (result->data[id].start == -1) {
			continue;
		}
		String name = &table[i * entry_size + 1];
		if (result->names.has(name)) {
			continue;
		}

		result->names.insert(name, id);
	}

	return result;
}

int RegEx::_sub(const String& p_subject, const String& p_replacement, int p_offset, int p_end,
	uint32_t p_flags, String& r_output) const
{
	// `safety_zone` is the number of chars we allocate in addition to the number of chars expected
	// in order to guard against the PCRE API writing one additional `\0` at the end. PCRE's API
	// docs are unclear on whether PCRE understands outlength in `pcre2_substitute(`) as counting an
	// implicit additional terminating char or not. Always allocating one char more than telling
	// PCRE has us on the safe side.
	const int safety_zone = 1;

	PCRE2_SIZE olength =
		p_subject.length() + 1; // Space for output string and one terminating `\0` character.
	Vector<char32_t> output;
	output.resize(olength + safety_zone);

	PCRE2_SIZE length = p_subject.length();
	if (p_end >= 0 && (uint32_t)p_end < length) {
		length = p_end;
	}

	pcre2_code_32* c = (pcre2_code_32*)code;
	pcre2_general_context_32* gctx = (pcre2_general_context_32*)general_ctx;
	pcre2_match_context_32* mctx = pcre2_match_context_create_32(gctx);
	PCRE2_SPTR32 s = (PCRE2_SPTR32)p_subject.get_data();
	PCRE2_SPTR32 r = (PCRE2_SPTR32)p_replacement.get_data();
	PCRE2_UCHAR32* o = (PCRE2_UCHAR32*)output.ptrw();

	pcre2_match_data_32* match = pcre2_match_data_create_from_pattern_32(c, gctx);

	int res = pcre2_substitute_32(
		c, s, length, p_offset, p_flags, match, mctx, r, p_replacement.length(), o, &olength);

	if (res == PCRE2_ERROR_NOMEMORY) {
		output.resize(olength + safety_zone);
		o = (PCRE2_UCHAR32*)output.ptrw();
		res = pcre2_substitute_32(
			c, s, length, p_offset, p_flags, match, mctx, r, p_replacement.length(), o, &olength);
	}

	pcre2_match_data_free_32(match);
	pcre2_match_context_free_32(mctx);

	if (res >= 0) {
		r_output = String::utf32(Span(output.ptr(), olength)) + p_subject.substr(length);
	}

	return res;
}

String RegEx::sub(
	const String& p_subject, const String& p_replacement, bool p_all, int p_offset, int p_end) const
{
	ERR_FAIL_COND_V(!is_valid(), String());
	ERR_FAIL_COND_V_MSG(p_offset < 0, String(), "RegEx sub offset must be >= 0");

	uint32_t flags = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_UNSET_EMPTY;
	if (p_all) {
		flags |= PCRE2_SUBSTITUTE_GLOBAL;
	}

	String output;
	const int res = _sub(p_subject, p_replacement,
p_offset, p_end, flags, output);

	if (res < 0) {
		PCRE2_UCHAR32 buf[256];
		pcre2_get_error_message_32(res, buf, 256);
		String message = "PCRE2 Error: " + String((const char32_t*)buf);
		ERR_PRINT(message);

		if (res == PCRE2_ERROR_NOSUBSTRING) {
			flags |= PCRE2_SUBSTITUTE_UNKNOWN_UNSET;
			_sub(p_subject, p_replacement, p_offset, p_end, flags, output);
		}
	}

	return output;
}

bool RegEx::is_valid() const { return (code != nullptr); }

String RegEx::get_pattern() const { return pattern; }

int RegEx::get_group_count() const
{
	ERR_FAIL_COND_V(!is_valid(), 0);

	uint32_t count;

	_pattern_info(PCRE2_INFO_CAPTURECOUNT, &count);

	return count;
}

PackedStringArray RegEx::get_names() const
{
	PackedStringArray result;

	ERR_FAIL_COND_V(!is_valid(), result);

	uint32_t count;
	const char32_t* table;
	uint32_t entry_size;

	_pattern_info(PCRE2_INFO_NAMECOUNT, &count);
	_pattern_info(PCRE2_INFO_NAMETABLE, &table);
	_pattern_info(PCRE2_INFO_NAMEENTRYSIZE, &entry_size);

	for (uint32_t i = 0; i < count; i++) {
		String name = &table[i * entry_size + 1];
		if (!result.has(name)) {
			result.append(name);
		}
	}

	return result;
}

RegEx::RegEx()
{
	general_ctx = pcre2_general_context_create_32(&_regex_malloc, &_regex_free, nullptr);
}

RegEx::RegEx(const String& p_pattern)
{
	general_ctx = pcre2_general_context_create_32(&_regex_malloc, &_regex_free, nullptr);
	compile(p_pattern);
}

RegEx::~RegEx()
{
	if (code) {
		pcre2_code_free_32((pcre2_code_32*)code);
	}
	pcre2_general_context_free_32((pcre2_general_context_32*)general_ctx);
}

#ifndef DISABLE_DEPRECATED


Ref<RegEx> RegEx::_create_from_string_bind_compat_95212(const String& p_pattern)
{
	return create_from_string(p_pattern, true);
}

Error RegEx::_compile_bind_compat_95212(const String& p_pattern)
{
	return compile(p_pattern, true);
}

void RegEx::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

void initialize_regex_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_regex_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_REGEX_ENABLED)


// --- Merged from modules/zip/zip_reader.cpp ---
#if defined(MODULE_ZIP_ENABLED)
#include "modules/zip/zip_reader.h"
#include "modules/register_module_types.h"

#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/os/time.h"

// ZIPPacker

Error ZIPPacker::open(const String& p_path, ZipAppend p_append)
{
	if (fa.is_valid()) {
		close();
	}

	zlib_filefunc_def io = zipio_create_io(&fa);
	zf = zipOpen2(p_path.utf8().get_data(), p_append, nullptr, &io);
	return zf != nullptr ? OK : FAILED;
}

Error ZIPPacker::close()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker cannot be closed because it is not open.");

	Error err = zipClose(zf, nullptr) == ZIP_OK ? OK : FAILED;
	if (err == OK) {
		DEV_ASSERT(fa.is_null());
		directories.clear();
		zf = nullptr;
	}

	return err;
}

void ZIPPacker::set_compression_level(int p_compression_level)
{
	ERR_FAIL_COND_MSG(
		p_compression_level < Z_DEFAULT_COMPRESSION || p_compression_level > Z_BEST_COMPRESSION,
		"Invalid compression level.");
	compression_level = p_compression_level;
}

int ZIPPacker::get_compression_level() const { return compression_level; }

Error ZIPPacker::write_file(const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker must be opened before use.");

	return zipWriteInFileInZip(zf, p_data.ptr(), p_data.size()) == ZIP_OK ? OK : FAILED;
}

Error ZIPPacker::close_file()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker must be opened before use.");

	return zipCloseFileInZip(zf) == ZIP_OK ? OK : FAILED;
}

ZIPPacker::ZIPPacker() {}

ZIPPacker::~ZIPPacker()
{
	if (fa.is_valid()) {
		close();
	}
}

Error ZIPPacker::start_file(
	const String& p_path, uint32_t p_permissions, uint64_t p_modified_time)
{
	return OK;
}

// ZIPReader

Error ZIPReader::open(const String& p_path)
{
	if (fa.is_valid()) {
		close();
	}

	zlib_filefunc_def io = zipio_create_io(&fa);
	uzf = unzOpen2(p_path.utf8().get_data(), &io);
	return uzf != nullptr ? OK : FAILED;
}

Error ZIPReader::close()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPReader cannot be closed because it is not open.");

	Error err = unzClose(uzf) == UNZ_OK ? OK : FAILED;
	if (err == OK) {
		DEV_ASSERT(fa.is_null());
		uzf = nullptr;
	}

	return err;
}

PackedStringArray ZIPReader::get_files()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedStringArray(), "ZIPReader must be opened before use.");

	unz_global_info gi;
	int err = unzGetGlobalInfo(uzf, &gi);
	ERR_FAIL_COND_V(err != UNZ_OK, PackedStringArray());
	if (gi.number_entry == 0) {
		return PackedStringArray();
	}

	err = unzGoToFirstFile(uzf);
	ERR_FAIL_COND_V(err != UNZ_OK, PackedStringArray());

	List<String> s;
	do {
		unz_file_info64 file_info;
		String filepath;

		err = godot_unzip_get_current_file_info(uzf, file_info, filepath);
		if (err == UNZ_OK) {
			s.push_back(filepath);
		}
	} while (unzGoToNextFile(uzf) == UNZ_OK);

	PackedStringArray arr;
	arr.resize(s.size());
	int idx = 0;
	for (const List<String>::Element* E = s.front(); E; E = E->next()) {
		arr.set(idx++, E->get());
	}
	return arr;
}

PackedByteArray ZIPReader::read_file(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "ZIPReader must be opened before use.");

	int err = UNZ_OK;

	// Locate and open the file.
	err = godot_unzip_locate_file(uzf, p_path, p_case_sensitive);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "File does not exist in zip archive: " + p_path);
	err = unzOpenCurrentFile(uzf);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "Could not open file within zip archive.");

	// Read the file info.
	unz_file_info info;
	err = unzGetCurrentFileInfo(uzf, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "Unable to read file information from zip archive.");
	ERR_FAIL_COND_V_MSG(info.uncompressed_size > INT_MAX, PackedByteArray(),
		"File contents too large to read from zip archive (>2 GB).");

	// Read the file data.
	PackedByteArray data;
	data.resize(info.uncompressed_size);
	uint8_t* buffer = data.ptrw();
	int to_read = data.size();
	while (to_read > 0) {
		int bytes_read = unzReadCurrentFile(uzf, buffer, to_read);
		ERR_FAIL_COND_V_MSG(
			bytes_read < 0, PackedByteArray(), "IO/zlib error reading file from zip archive.");
		ERR_FAIL_COND_V_MSG(bytes_read == UNZ_EOF && to_read != 0, PackedByteArray(),
			"Incomplete file read from zip archive.");
		DEV_ASSERT(bytes_read <= to_read);
		buffer += bytes_read;
		to_read -= bytes_read;
	}

	// Verify the data and return.
	err = unzCloseCurrentFile(uzf);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "CRC error reading file from zip archive.");
	return data;
}

bool ZIPReader::file_exists(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "ZIPReader must be opened before use.");

	int cs = p_case_sensitive ? 1 : 2;
	if (unzLocateFile(uzf, p_path.utf8().get_data(), cs) != UNZ_OK) {
		return false;
	}
	if (unzOpenCurrentFile(uzf) != UNZ_OK) {
		return false;
	}

	unzCloseCurrentFile(uzf);
	return true;
}

int ZIPReader::get_compression_level(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), -1, "ZIPReader must be opened before use.");

	int cs = p_case_sensitive ? 1 : 2;
	if (unzLocateFile(uzf, p_path.utf8().get_data(), cs) != UNZ_OK) {
		return -1;
	}

	int method;
	int level;
	if (unzOpenCurrentFile2(uzf, &method, &level, 1) != UNZ_OK) {
		return -1;
	}

	unzCloseCurrentFile(uzf);

	return level;
}

ZIPReader::ZIPReader() {}

ZIPReader::~ZIPReader()
{
	if (fa.is_valid()) {
		close();
	}
}

// Module registration

void initialize_zip_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_zip_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_ZIP_ENABLED)
