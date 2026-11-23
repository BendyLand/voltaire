/**************************************************************************/
/*  image_texture.h                                                       */
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

#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
// --- Folded from scene/resources/bit_map.h ---
#include "core/io/image.h"
#include "core/io/resource.h"

template <typename T> class TypedArray;

class BitMap : public Resource
{
	Vector<uint8_t> bitmask;
	int width = 0;
	int height = 0;

	Vector<Vector<Vector2>> _march_square(const Rect2i& p_rect, const Point2i& p_start) const;

	TypedArray<PackedVector2Array> _opaque_to_polygons_bind(
		const Rect2i& p_rect, float p_epsilon) const;

public:
	void create(const Size2i& p_size);

	void set_bitv(const Point2i& p_pos, bool p_value);
	void set_bit(int p_x, int p_y, bool p_value);
	void set_bit_rect(const Rect2i& p_rect, bool p_value);
	bool get_bitv(const Point2i& p_pos) const;
	bool get_bit(int p_x, int p_y) const;

	int get_true_bit_count() const;

	Size2i get_size() const;
	void resize(const Size2i& p_new_size);

	void grow_mask(int p_pixels, const Rect2i& p_rect);
	void shrink_mask(int p_pixels, const Rect2i& p_rect);

	void blit(const Vector2i& p_pos, const Ref<BitMap>& p_bitmap);
	Ref<Image> convert_to_image() const;

	Vector<Vector<Vector2>> clip_opaque_to_polygons(
		const Rect2i& p_rect, float p_epsilon = 2.0) const;
};
// --- End scene/resources/bit_map.h ---
#include "scene/resources/texture.h"

class BitMap;

class ImageTexture final : public Texture2D
{
	struct SVGData
	{
		String source;
		float base_scale = 1.0f;
		float saturation = 1.0f;
		bool fix_alpha_border = false;
		bool premult_alpha = false;
		mutable RID base_texture;
		mutable HashMap<double, RID> texture_cache;
		mutable HashMap<Color, Color> cmap;
		mutable Size2 base_size;
		mutable Size2 size;
	};

	struct ScalingLevel
	{
		HashSet<ImageTexture*> textures;
		int32_t refcount = 1;
	};

	static Mutex svg_mutex;
	static HashMap<double, ScalingLevel> scaling_levels;

	mutable RID texture;
	Image::Format format = Image::FORMAT_L8;
	bool mipmaps = false;
	int w = 0;
	int h = 0;
	Size2 size_override;
	mutable Ref<BitMap> alpha_cache;
	bool image_stored = false;
	SVGData* svg_data = nullptr;

	SVGData* _ensure_svg_data();
	void _svg_remove_scale(double p_scale);
	RID _svg_ensure_scale(double p_scale) const;
	RID _svg_load_at_scale(double p_scale, bool p_set_size) const;
	void _svg_update_texture();
	void _svg_clear();
	void _svg_destroy();

protected:
	virtual void reload_from_file() override;

public:
	void set_image(const Ref<Image>& p_image);
	static Ref<ImageTexture> create_from_image(const Ref<Image>& p_image);
	static Ref<ImageTexture> create_from_string(const String& p_source, float p_scale = 1.0f,
		float p_saturation = 1.0f, const HashMap<Color, Color>& p_color_map = HashMap<Color, Color>());

	// SVG / DPI-aware scalable source mode (folded from DPITexture).
	bool is_svg() const { return svg_data != nullptr; }

	void set_source(const String& p_source);
	String get_source() const;

	void set_fix_alpha_border(bool p_enabled);
	bool get_fix_alpha_border() const;

	void set_premult_alpha(bool p_enabled);
	bool get_premult_alpha() const;

	void set_base_scale(float p_scale);
	float get_base_scale() const;

	void set_saturation(float p_saturation);
	float get_saturation() const;

	void set_color_map(const HashMap<Color, Color>& p_color_map);
	HashMap<Color, Color> get_color_map() const;

	static void reference_scaling_level(double p_scale);
	static void unreference_scaling_level(double p_scale);

	virtual Image::Format get_format() const override;

	void update(const Ref<Image>& p_image);
	Ref<Image> get_image() const override;

	int get_width() const override;
	int get_height() const override;

	virtual RID get_rid() const override;
	virtual RID get_scaled_rid() const override;

	bool has_alpha() const override;
	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const override;

	void draw_msdf_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), int p_outline_size = 0, float p_px_range = 1.0,
		float p_scale = 1.0) const;
	void draw_lcd_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1)) const;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	void set_size_override(const Size2i& p_size);

	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	ImageTexture() = default;
	~ImageTexture();
};

class ImageTextureLayered final : public TextureLayered
{
	LayeredType layered_type = LAYERED_TYPE_2D_ARRAY;

	mutable RID texture;
	Image::Format format = Image::FORMAT_L8;

	int width = 0;
	int height = 0;
	int layers = 0;
	bool mipmaps = false;

public:
	void set_layered_type(LayeredType p_type);

	virtual Image::Format get_format() const override;
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual int get_layers() const override;
	virtual bool has_mipmaps() const override;
	virtual LayeredType get_layered_type() const override;

	Error create_from_images(Vector<Ref<Image>> p_images);
	void update_layer(const Ref<Image>& p_image, int p_layer);
	virtual Ref<Image> get_layer_data(int p_layer) const override;

	virtual RID get_rid() const override;
	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	ImageTextureLayered(LayeredType p_layered_type = LAYERED_TYPE_2D_ARRAY);
	~ImageTextureLayered();
};

class ImageTexture3D final : public Texture3D
{
	mutable RID texture;

	Image::Format format = Image::FORMAT_L8;
	int width = 1;
	int height = 1;
	int depth = 1;
	bool mipmaps = false;
	bool images_stored = false;

public:
	virtual Image::Format get_format() const override;
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual int get_depth() const override;
	virtual bool has_mipmaps() const override;

	Error create(Image::Format p_format, int p_width, int p_height, int p_depth, bool p_mipmaps,
		const Vector<Ref<Image>>& p_data);
	void update(const Vector<Ref<Image>>& p_data);
	virtual Vector<Ref<Image>> get_data() const override;

	virtual RID get_rid() const override;
	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	ImageTexture3D();
	~ImageTexture3D();
};

// --- Folded from scene/resources/atlas_texture.h ---
#include "core/os/mutex.h"
#include "core/templates/local_vector.h"
#include "scene/resources/texture.h"

class AtlasTexture final : public Texture2D
{
public:
	enum
	{
		MAX_FRAMES = 256
	};

private:
	struct Frame
	{
		Ref<Texture2D> texture;
		float duration = 1.0f;
	};

	struct AnimData
	{
		RWLock rw_lock;
		RID proxy_ph;
		RID proxy;
		LocalVector<Frame> frames;
		int frame_count = 1;
		int current_frame = 0;
		bool pause = false;
		bool one_shot = false;
		float speed_scale = 1.0f;
		float time = 0.0f;
		uint64_t prev_ticks = 0;
	};

	Ref<Texture2D> atlas;
	Rect2 region;		  // Only for property value.
	Rect2 rounded_region; // Region with rounded size (image size is always integer).
	Rect2 margin;
	bool filter_clip = false;

	mutable AnimData* anim = nullptr;

	Rect2 _get_region_rect() const;
	AnimData* _ensure_anim();
	void _update_proxy() const;

public:
	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual RID get_rid() const override;

	virtual bool has_alpha() const override;

	void set_atlas(const Ref<Texture2D>& p_atlas);
	Ref<Texture2D> get_atlas() const;

	void set_region(const Rect2& p_region);
	Rect2 get_region() const;

	void set_margin(const Rect2& p_margin);
	Rect2 get_margin() const;

	void set_filter_clip(const bool p_enable);
	bool has_filter_clip() const;

	// Animated frame sequence mode (folded from AnimatedTexture).
	bool is_animated() const { return anim != nullptr; }

	void set_frames(int p_frames);
	int get_frames() const;

	void set_current_frame(int p_frame);
	int get_current_frame() const;

	void set_pause(bool p_pause);
	bool get_pause() const;

	void set_one_shot(bool p_one_shot);
	bool get_one_shot() const;

	void set_frame_texture(int p_frame, const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_frame_texture(int p_frame) const;

	void set_frame_duration(int p_frame, float p_duration);
	float get_frame_duration(int p_frame) const;

	void set_speed_scale(float p_scale);
	float get_speed_scale() const;

	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const override;
	virtual bool get_rect_region(const Rect2& p_rect, const Rect2& p_src_rect, Rect2& r_rect,
		Rect2& r_src_rect) const override;

	bool is_pixel_opaque(int p_x, int p_y) const override;

	virtual Ref<Image> get_image() const override;

	AtlasTexture() = default;
	~AtlasTexture();
};

// --- Folded from scene/resources/compressed_texture.h ---

#include "scene/resources/texture.h"
#include "servers/rendering/rendering_server_enums.h"

class BitMap;

class CompressedTexture2D final : public Texture2D
{
public:
	enum DataFormat
	{
		DATA_FORMAT_IMAGE,
		DATA_FORMAT_PNG,
		DATA_FORMAT_WEBP,
		DATA_FORMAT_BASIS_UNIVERSAL,
	};

	enum PortableDataFormat
	{
		PORTABLE_DATA_FORMAT_UNDEFINED,
		PORTABLE_DATA_FORMAT_IMAGE,
		PORTABLE_DATA_FORMAT_PNG,
		PORTABLE_DATA_FORMAT_WEBP,
		PORTABLE_DATA_FORMAT_BASIS_UNIVERSAL,
	};

	enum CompressionMode
	{
		COMPRESSION_MODE_LOSSLESS,
		COMPRESSION_MODE_LOSSY,
		COMPRESSION_MODE_BASIS_UNIVERSAL,
		COMPRESSION_MODE_S3TC,
		COMPRESSION_MODE_ETC2,
		COMPRESSION_MODE_BPTC,
		COMPRESSION_MODE_ASTC,
	};

	enum
	{
		FORMAT_VERSION = 1
	};

	enum FormatBits
	{
		FORMAT_BIT_STREAM = 1 << 22,
		FORMAT_BIT_HAS_MIPMAPS = 1 << 23,
		FORMAT_BIT_DETECT_3D = 1 << 24,
		// FORMAT_BIT_DETECT_SRGB = 1 << 25,
		FORMAT_BIT_DETECT_NORMAL = 1 << 26,
		FORMAT_BIT_DETECT_ROUGNESS = 1 << 27,
	};

private:
	String path_to_file;
	mutable RID texture;
	Image::Format format = Image::FORMAT_L8;
	int w = 0;
	int h = 0;
	mutable Ref<BitMap> alpha_cache;

	// Portable inline compressed buffer fields (folded from PortableCompressedTexture2D).
	bool is_portable = false;
	CompressionMode compression_mode = COMPRESSION_MODE_LOSSLESS;
	static bool keep_all_compressed_buffers;
	bool keep_compressed_buffer = false;
	Vector<uint8_t> compressed_buffer;
	Size2 size_override;
	bool mipmaps = false;
	bool image_stored = false;
	Image::BasisUniversalPackerParams basisu_params;

	Error _load_data(const String& p_path, int& r_width, int& r_height, Ref<Image>& image,
		bool& r_request_3d, bool& r_request_normal, bool& r_request_roughness, int& mipmap_limit,
		int p_size_limit = 0);
	virtual void reload_from_file() override;

	static void _requested_3d(void* p_ud);
	static void _requested_roughness(void* p_ud, const String& p_normal_path,
		RSE::TextureDetectRoughnessChannel p_roughness_channel);
	static void _requested_normal(void* p_ud);

	Vector<uint8_t> _get_data() const;
	void _set_data(const Vector<uint8_t>& p_data);

public:
	static Ref<Image> load_image_from_file(Ref<FileAccess> p_file, int p_size_limit);

	typedef void (*TextureFormatRequestCallback)(const Ref<CompressedTexture2D>&);
	typedef void (*TextureFormatRoughnessRequestCallback)(const Ref<CompressedTexture2D>&,
		const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_roughness_channel);

	static TextureFormatRequestCallback request_3d_callback;
	static TextureFormatRoughnessRequestCallback request_roughness_callback;
	static TextureFormatRequestCallback request_normal_callback;

	virtual Image::Format get_format() const override;
	Error load(const String& p_path);
	String get_load_path() const;

	// Portable compressed mode API (folded from PortableCompressedTexture2D).
	bool is_portable_texture() const { return is_portable; }
	CompressionMode get_compression_mode() const;
	void create_from_image(const Ref<Image>& p_image, CompressionMode p_compression_mode,
		bool p_normal_map = false, float p_lossy_quality = 0.8f);
	void update(const Ref<Image>& p_image);

	void set_size_override(const Size2& p_size);
	Size2 get_size_override() const;

	void set_keep_compressed_buffer(bool p_keep);
	bool is_keeping_compressed_buffer() const;

	void set_basisu_compressor_params(int p_uastc_level, float p_rdo_quality_loss);

	static void set_keep_all_compressed_buffers(bool p_keep);
	static bool is_keeping_all_compressed_buffers();

	int get_width() const override;
	int get_height() const override;
	virtual RID get_rid() const override;

	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	virtual void draw(RID p_canvas_item, const Point2& p_pos,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile = false,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false) const override;
	virtual void draw_rect_region(RID p_canvas_item, const Rect2& p_rect, const Rect2& p_src_rect,
		const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = true) const override;

	virtual bool is_pixel_opaque(int p_x, int p_y) const override;
	virtual bool has_alpha() const override;

	virtual Ref<Image> get_image() const override;

	CompressedTexture2D() = default;
	~CompressedTexture2D();
};

class CompressedTextureLayered final : public TextureLayered
{
public:
	enum DataFormat
	{
		DATA_FORMAT_IMAGE,
		DATA_FORMAT_PNG,
		DATA_FORMAT_WEBP,
		DATA_FORMAT_BASIS_UNIVERSAL,
	};

	enum
	{
		FORMAT_VERSION = 1
	};

	enum FormatBits
	{
		FORMAT_BIT_STREAM = 1 << 22,
		FORMAT_BIT_HAS_MIPMAPS = 1 << 23,
	};

private:
	Error _load_data(
		const String& p_path, Vector<Ref<Image>>& images, int& mipmap_limit, int p_size_limit = 0);
	String path_to_file;
	mutable RID texture;
	Image::Format format = Image::FORMAT_L8;
	int w = 0;
	int h = 0;
	int layers = 0;
	bool mipmaps = false;
	LayeredType layered_type = LayeredType::LAYERED_TYPE_2D_ARRAY;

	virtual void reload_from_file() override;

public:
	Image::Format get_format() const override;
	Error load(const String& p_path);
	String get_load_path() const;
	virtual LayeredType get_layered_type() const override;

	int get_width() const override;
	int get_height() const override;
	int get_layers() const override;
	virtual bool has_mipmaps() const override;
	virtual RID get_rid() const override;

	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	virtual Ref<Image> get_layer_data(int p_layer) const override;

	CompressedTextureLayered(LayeredType p_layered_type = LAYERED_TYPE_2D_ARRAY);
	~CompressedTextureLayered();
};

class CompressedTexture3D final : public Texture3D
{
public:
	enum DataFormat
	{
		DATA_FORMAT_IMAGE,
		DATA_FORMAT_PNG,
		DATA_FORMAT_WEBP,
		DATA_FORMAT_BASIS_UNIVERSAL,
	};

	enum
	{
		FORMAT_VERSION = 1
	};

	enum FormatBits
	{
		FORMAT_BIT_STREAM = 1 << 22,
		FORMAT_BIT_HAS_MIPMAPS = 1 << 23,
	};

private:
	Error _load_data(const String& p_path, Vector<Ref<Image>>& r_data, Image::Format& r_format,
		int& r_width, int& r_height, int& r_depth, bool& r_mipmaps);
	String path_to_file;
	mutable RID texture;
	Image::Format format = Image::FORMAT_L8;
	int w = 0;
	int h = 0;
	int d = 0;
	bool mipmaps = false;

	virtual void reload_from_file() override;

public:
	Image::Format get_format() const override;
	Error load(const String& p_path);
	String get_load_path() const;

	int get_width() const override;
	int get_height() const override;
	int get_depth() const override;
	virtual bool has_mipmaps() const override;
	virtual RID get_rid() const override;

	virtual void set_path(const String& p_path, bool p_take_over = false) override;

	virtual Vector<Ref<Image>> get_data() const override;

	~CompressedTexture3D();
};

// --- Folded from scene/resources/compressed_texture_resource_format.h ---
#include "core/io/resource_loader.h"

class ResourceFormatLoaderCompressedTexture final : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
};

// --- End scene/resources/compressed_texture_resource_format.h ---

// --- Folded from modules/dds/texture_loader_dds.h ---
#include "core/io/image.h"
#include "core/io/resource_loader.h"

class ResourceFormatDDS final : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;

	ResourceFormatDDS();

	virtual ~ResourceFormatDDS() {}
};

Error save_dds(const String &p_path, const Ref<Image> &p_img);
Vector<uint8_t> save_dds_buffer(const Ref<Image> &p_img);
// --- End modules/dds/texture_loader_dds.h ---

// --- Folded from modules/noise/fastnoise_lite.h ---
#include <thirdparty/misc/FastNoiseLite.h>
#include "core/io/image.h"

typedef fastnoiselite::FastNoiseLite _FastNoiseLite;

class FastNoiseLite final : public Resource
{
public:
	enum NoiseType
	{
		TYPE_SIMPLEX = _FastNoiseLite::NoiseType_OpenSimplex2,
		TYPE_SIMPLEX_SMOOTH = _FastNoiseLite::NoiseType_OpenSimplex2S,
		TYPE_CELLULAR = _FastNoiseLite::NoiseType_Cellular,
		TYPE_PERLIN = _FastNoiseLite::NoiseType_Perlin,
		TYPE_VALUE_CUBIC = _FastNoiseLite::NoiseType_ValueCubic,
		TYPE_VALUE = _FastNoiseLite::NoiseType_Value,
	};

	enum FractalType
	{
		FRACTAL_NONE = _FastNoiseLite::FractalType_None,
		FRACTAL_FBM = _FastNoiseLite::FractalType_FBm,
		FRACTAL_RIDGED = _FastNoiseLite::FractalType_Ridged,
		FRACTAL_PING_PONG = _FastNoiseLite::FractalType_PingPong,
	};

	enum CellularDistanceFunction
	{
		DISTANCE_EUCLIDEAN = _FastNoiseLite::CellularDistanceFunction_Euclidean,
		DISTANCE_EUCLIDEAN_SQUARED = _FastNoiseLite::CellularDistanceFunction_EuclideanSq,
		DISTANCE_MANHATTAN = _FastNoiseLite::CellularDistanceFunction_Manhattan,
		DISTANCE_HYBRID = _FastNoiseLite::CellularDistanceFunction_Hybrid
	};

	enum CellularReturnType
	{
		RETURN_CELL_VALUE = _FastNoiseLite::CellularReturnType_CellValue,
		RETURN_DISTANCE = _FastNoiseLite::CellularReturnType_Distance,
		RETURN_DISTANCE2 = _FastNoiseLite::CellularReturnType_Distance2,
		RETURN_DISTANCE2_ADD = _FastNoiseLite::CellularReturnType_Distance2Add,
		RETURN_DISTANCE2_SUB = _FastNoiseLite::CellularReturnType_Distance2Sub,
		RETURN_DISTANCE2_MUL = _FastNoiseLite::CellularReturnType_Distance2Mul,
		RETURN_DISTANCE2_DIV = _FastNoiseLite::CellularReturnType_Distance2Div
	};

	enum DomainWarpType
	{
		DOMAIN_WARP_SIMPLEX = _FastNoiseLite::DomainWarpType_OpenSimplex2,
		DOMAIN_WARP_SIMPLEX_REDUCED = _FastNoiseLite::DomainWarpType_OpenSimplex2Reduced,
		DOMAIN_WARP_BASIC_GRID = _FastNoiseLite::DomainWarpType_BasicGrid
	};

	enum DomainWarpFractalType
	{
		DOMAIN_WARP_FRACTAL_NONE,
		DOMAIN_WARP_FRACTAL_PROGRESSIVE,
		DOMAIN_WARP_FRACTAL_INDEPENDENT
	};

private:
	// Helper struct for get_seamless_image().
	template <typename T> struct img_buff
	{
		T* img = nullptr;
		int width; // Array dimensions & default modulo for image.
		int height;
		int offset_x; // Offset index location on image (wrapped by specified modulo).
		int offset_y;
		int alt_width; // Alternate module for image.
		int alt_height;

		enum ALT_MODULO
		{
			DEFAULT = 0,
			ALT_X,
			ALT_Y,
			ALT_XY
		};

		// Multi-dimensional array indexer (e.g. img[x][y]) that supports multiple modulos.
		T& operator()(int x, int y, ALT_MODULO mode = DEFAULT)
		{
			switch (mode) {
			case ALT_XY:
				return img[(x + offset_x) % alt_width + ((y + offset_y) % alt_height) * width];
			case ALT_X:
				return img[(x + offset_x) % alt_width + ((y + offset_y) % height) * width];
			case ALT_Y:
				return img[(x + offset_x) % width + ((y + offset_y) % alt_height) * width];
			default:
				return img[(x + offset_x) % width + ((y + offset_y) % height) * width];
			}
		}
	};

	union l2c
	{
		uint32_t l;
		uint8_t c[4];

		struct
		{
			uint8_t r;
			uint8_t g;
			uint8_t b;
			uint8_t a;
		};
	};

	template <typename T> T _alpha_blend(T p_bg, T p_fg, int p_alpha) const
	{
		l2c fg, bg, out;

		fg.l = p_fg;
		bg.l = p_bg;

		uint16_t alpha;
		uint16_t inv_alpha;

		// If no alpha argument specified, use the alpha channel in the color
		if (p_alpha == -1) {
			alpha = fg.c[3] + 1;
			inv_alpha = 256 - fg.c[3];
		}
		else {
			alpha = p_alpha + 1;
			inv_alpha = 256 - p_alpha;
		}

		out.c[0] = (uint8_t)((alpha * fg.c[0] + inv_alpha * bg.c[0]) >> 8);
		out.c[1] = (uint8_t)((alpha * fg.c[1] + inv_alpha * bg.c[1]) >> 8);
		out.c[2] = (uint8_t)((alpha * fg.c[2] + inv_alpha * bg.c[2]) >> 8);
		out.c[3] = 0xFF;

		return out.l;
	}

	template <typename T>
	Vector<Ref<Image>> _generate_seamless_image(Vector<Ref<Image>> p_src, int p_width, int p_height,
		int p_depth, bool p_invert, real_t p_blend_skirt) const
	{
		ERR_FAIL_COND_V(p_blend_skirt < 0, Vector<Ref<Image>>());

		int skirt_width = MAX(1, p_width * p_blend_skirt);
		int skirt_height = MAX(1, p_height * p_blend_skirt);
		int src_width = p_width + skirt_width;
		int src_height = p_height + skirt_height;
		int half_width = p_width * 0.5;
		int half_height = p_height * 0.5;
		int skirt_edge_x = half_width + skirt_width;
		int skirt_edge_y = half_height + skirt_height;

		Image::Format format = p_src[0]->get_format();
		int pixel_size = Image::get_format_pixel_size(format);

		Vector<Ref<Image>> images;
		images.resize(p_src.size());

		// First blend across x and y for all slices.
		for (int d = 0; d < images.size(); d++) {
			Vector<uint8_t> dest;
			dest.resize(p_width * p_height * pixel_size);

			img_buff<T> rd_src = {(T*)p_src[d]->get_data().ptr(), src_width, src_height, half_width,
				half_height, p_width, p_height};

			// `wr` is setup for straight x/y coordinate array access.
			img_buff<T> wr = {(T*)dest.ptrw(), p_width, p_height, 0, 0, 0, 0};
			// `rd_dest` is a readable pointer to `wr`, i.e. what has already been written to the
			// output buffer.
			img_buff<T> rd_dest = {(T*)dest.ptr(), p_width, p_height, 0, 0, 0, 0};

			// Swap the quadrants to make edges seamless.
			for (int y = 0; y < p_height; y++) {
				for (int x = 0; x < p_width; x++) {
					wr(x, y) = rd_src(x, y, img_buff<T>::ALT_XY);
				}
			}

			// Blend the vertical skirt over the middle seam.
			for (int x = half_width; x < skirt_edge_x; x++) {
				int alpha = 255 * (1 - Math::smoothstep(
										   0.1f, 0.9f, float(x - half_width) / float(skirt_width)));
				for (int y = 0; y < p_height; y++) {
					// Skip the center square
					if (y == half_height) {
						y = skirt_edge_y - 1;
					}
					else {
						wr(x, y) =
							_alpha_blend<T>(rd_dest(x, y), rd_src(x, y, img_buff<T>::ALT_Y), alpha);
					}
				}
			}

			// Blend the horizontal skirt over the middle seam.
			for (int y = half_height; y < skirt_edge_y; y++) {
				int alpha = 255 * (1 - Math::smoothstep(0.1f, 0.9f,
										   float(y - half_height) / float(skirt_height)));
				for (int x = 0; x < p_width; x++) {
					// Skip the center square
					if (x == half_width) {
						x = skirt_edge_x - 1;
					}
					else {
						wr(x, y) =
							_alpha_blend<T>(rd_dest(x, y), rd_src(x, y, img_buff<T>::ALT_X), alpha);
					}
				}
			}

			// Fill in the center square.
			for (int y = half_height; y < skirt_edge_y; y++) {
				for (int x = half_width; x < skirt_edge_x; x++) {
					int xpos = 255 * (1 - Math::smoothstep(0.1f, 0.9f,
											  float(x - half_width) / float(skirt_width)));
					int ypos = 255 * (1 - Math::smoothstep(0.1f, 0.9f,
											  float(y - half_height) / float(skirt_height)));

					T top_blend = _alpha_blend<T>(
						rd_src(x, y, img_buff<T>::ALT_X), rd_src(x, y, img_buff<T>::DEFAULT), xpos);
					T bottom_blend = _alpha_blend<T>(
						rd_src(x, y, img_buff<T>::ALT_XY), rd_src(x, y, img_buff<T>::ALT_Y), xpos);
					wr(x, y) = _alpha_blend<T>(bottom_blend, top_blend, ypos);
				}
			}
			Ref<Image> image = memnew(Image(p_width, p_height, false, format, dest));
			p_src.write[d].unref();
			images.write[d] = image;
		}

		// Now blend across z.
		if (p_depth > 1) {
			int skirt_depth = MAX(1, p_depth * p_blend_skirt);
			int half_depth = p_depth * 0.5;
			int skirt_edge_z = half_depth + skirt_depth;

			// Swap halves on depth.
			for (int i = 0; i < half_depth; i++) {
				Ref<Image> img = images[i];
				images.write[i] = images[i + half_depth];
				images.write[i + half_depth] = img;
			}

			Vector<Ref<Image>> new_images = images;
			new_images.resize(p_depth);

			// Scale seamless generation to third dimension.
			for (int z = half_depth; z < skirt_edge_z; z++) {
				int alpha = 255 * (1 - Math::smoothstep(
										   0.1f, 0.9f, float(z - half_depth) / float(skirt_depth)));

				Vector<uint8_t> img = images[z % p_depth]->get_data();
				Vector<uint8_t> skirt = images[(z - half_depth) + p_depth]->get_data();

				Vector<uint8_t> dest;
				dest.resize(images[0]->get_width() * images[0]->get_height() *
							Image::get_format_pixel_size(images[0]->get_format()));

				for (int i = 0; i < img.size(); i++) {
					uint8_t fg = skirt[i];
					uint8_t bg = img[i];

					uint16_t a = alpha + 1;
					uint16_t inv_a = 256 - alpha;

					dest.write[i] = (uint8_t)((a * fg + inv_a * bg) >> 8);
				}

				Ref<Image> new_image = memnew(Image(images[0]->get_width(), images[0]->get_height(),
					false, images[0]->get_format(), dest));
				new_images.write[z % p_depth] = new_image;
			}
			return new_images;
		}
		return images;
	}

	_FastNoiseLite _noise;
	_FastNoiseLite _domain_warp_noise;

	Vector3 offset;
	NoiseType noise_type = TYPE_SIMPLEX_SMOOTH;

	int seed = 0;
	real_t frequency = 0.01;

	// Fractal specific.
	FractalType fractal_type = FRACTAL_FBM;
	int fractal_octaves = 5;
	real_t fractal_lacunarity = 2;
	real_t fractal_gain = 0.5;
	real_t fractal_weighted_strength = 0;
	real_t fractal_ping_pong_strength = 2;

	// Cellular specific.
	CellularDistanceFunction cellular_distance_function = DISTANCE_EUCLIDEAN;
	CellularReturnType cellular_return_type = RETURN_DISTANCE;
	real_t cellular_jitter = 1.0;

	// Domain warp specific.
	bool domain_warp_enabled = false;
	DomainWarpType domain_warp_type = DOMAIN_WARP_SIMPLEX;
	real_t domain_warp_amplitude = 30.0;
	real_t domain_warp_frequency = 0.05;
	DomainWarpFractalType domain_warp_fractal_type = DOMAIN_WARP_FRACTAL_PROGRESSIVE;
	int domain_warp_fractal_octaves = 5;
	real_t domain_warp_fractal_lacunarity = 6;
	real_t domain_warp_fractal_gain = 0.5;

	_FastNoiseLite::FractalType _convert_domain_warp_fractal_type_enum(
		DomainWarpFractalType p_domain_warp_fractal_type);

public:
	FastNoiseLite();
	~FastNoiseLite();

	// General noise settings.

	void set_noise_type(NoiseType p_noise_type);
	NoiseType get_noise_type() const;

	void set_seed(int p_seed);
	int get_seed() const;

	void set_frequency(real_t p_freq);
	real_t get_frequency() const;

	void set_offset(Vector3 p_offset);
	Vector3 get_offset() const;

	// Fractal specific.

	void set_fractal_type(FractalType p_type);
	FractalType get_fractal_type() const;

	void set_fractal_octaves(int p_octaves);
	int get_fractal_octaves() const;

	void set_fractal_lacunarity(real_t p_lacunarity);
	real_t get_fractal_lacunarity() const;

	void set_fractal_gain(real_t p_gain);
	real_t get_fractal_gain() const;

	void set_fractal_weighted_strength(real_t p_weighted_strength);
	real_t get_fractal_weighted_strength() const;

	void set_fractal_ping_pong_strength(real_t p_ping_pong_strength);
	real_t get_fractal_ping_pong_strength() const;

	// Cellular specific.

	void set_cellular_distance_function(CellularDistanceFunction p_func);
	CellularDistanceFunction get_cellular_distance_function() const;

	void set_cellular_return_type(CellularReturnType p_ret);
	CellularReturnType get_cellular_return_type() const;

	void set_cellular_jitter(real_t p_jitter);
	real_t get_cellular_jitter() const;

	// Domain warp specific.

	void set_domain_warp_enabled(bool p_enabled);
	bool is_domain_warp_enabled() const;

	void set_domain_warp_type(DomainWarpType p_domain_warp_type);
	DomainWarpType get_domain_warp_type() const;

	void set_domain_warp_amplitude(real_t p_amplitude);
	real_t get_domain_warp_amplitude() const;

	void set_domain_warp_frequency(real_t p_frequency);
	real_t get_domain_warp_frequency() const;

	void set_domain_warp_fractal_type(DomainWarpFractalType p_domain_warp_fractal_type);
	DomainWarpFractalType get_domain_warp_fractal_type() const;

	void set_domain_warp_fractal_octaves(int p_octaves);
	int get_domain_warp_fractal_octaves() const;

	void set_domain_warp_fractal_lacunarity(real_t p_lacunarity);
	real_t get_domain_warp_fractal_lacunarity() const;

	void set_domain_warp_fractal_gain(real_t p_gain);
	real_t get_domain_warp_fractal_gain() const;

	// Noise evaluation & image generation methods.
	real_t get_noise_1d(real_t p_x) const;

	real_t get_noise_2dv(Vector2 p_v) const;
	real_t get_noise_2d(real_t p_x, real_t p_y) const;

	real_t get_noise_3dv(Vector3 p_v) const;
	real_t get_noise_3d(real_t p_x, real_t p_y, real_t p_z) const;

	Vector<Ref<Image>> _get_image(int p_width, int p_height, int p_depth, bool p_invert = false,
		bool p_in_3d_space = false, bool p_normalize = true) const;
	Ref<Image> get_image(int p_width, int p_height, bool p_invert = false,
		bool p_in_3d_space = false, bool p_normalize = true) const;

	Vector<Ref<Image>> _get_seamless_image(int p_width, int p_height, int p_depth,
		bool p_invert = false, bool p_in_3d_space = false, real_t p_blend_skirt = 0.1,
		bool p_normalize = true) const;
	Ref<Image> get_seamless_image(int p_width, int p_height, bool p_invert = false,
		bool p_in_3d_space = false, real_t p_blend_skirt = 0.1, bool p_normalize = true) const;

	void _changed();
};

template <> uint8_t FastNoiseLite::_alpha_blend<uint8_t>(uint8_t p_bg, uint8_t p_fg, int p_alpha) const;

#include "scene/resources/material.h"
#include "scene/resources/texture.h"

class NoiseTexture2D final : public Texture2D
{
private:
	Ref<Image> image;

	bool first_time = true;
	bool update_queued = false;
	bool regen_queued = false;

	mutable RID texture;
	uint32_t flags = 0;

	Size2i size = Size2i(512, 512);
	bool invert = false;
	bool in_3d_space = false;
	bool generate_mipmaps = true;
	bool seamless = false;
	real_t seamless_blend_skirt = 0.1;
	bool as_normal_map = false;
	float bump_strength = 8.0;
	bool normalize = true;

	Ref<Gradient> color_ramp;
	Ref<FastNoiseLite> noise;

	void _thread_done(const Ref<Image>& p_image);
	void _thread_function();

	void _queue_update();
	Ref<Image> _generate_texture();
	void _update_texture();
	void _set_texture_image(const Ref<Image>& p_image);

	Ref<Image> _modulate_with_gradient(Ref<Image> p_image, Ref<Gradient> p_gradient);

public:
	void set_noise(Ref<FastNoiseLite> p_noise);
	Ref<FastNoiseLite> get_noise();

	void set_width(int p_width);
	void set_height(int p_height);

	void set_invert(bool p_invert);
	bool get_invert() const;

	void set_in_3d_space(bool p_enable);
	bool is_in_3d_space() const;

	void set_generate_mipmaps(bool p_enable);
	bool is_generating_mipmaps() const;

	void set_seamless(bool p_seamless);
	bool get_seamless();

	void set_seamless_blend_skirt(real_t p_blend_skirt);
	real_t get_seamless_blend_skirt();

	void set_as_normal_map(bool p_as_normal_map);
	bool is_normal_map();

	void set_bump_strength(float p_bump_strength);
	float get_bump_strength();

	void set_normalize(bool p_normalize);
	bool is_normalized() const;

	void set_color_ramp(const Ref<Gradient>& p_gradient);
	Ref<Gradient> get_color_ramp() const;

	int get_width() const override;
	int get_height() const override;

	virtual RID get_rid() const override;

	virtual bool has_alpha() const override { return false; }

	virtual Ref<Image> get_image() const override;

	NoiseTexture2D();
	virtual ~NoiseTexture2D() = default;
};

class NoiseTexture3D final : public Texture3D
{
private:
	bool first_time = true;
	bool update_queued = false;
	bool regen_queued = false;

	mutable RID texture;
	uint32_t flags = 0;

	int width = 64;
	int height = 64;
	int depth = 64;
	bool invert = false;
	bool seamless = false;
	real_t seamless_blend_skirt = 0.1;
	bool normalize = true;

	Ref<Gradient> color_ramp;
	Ref<FastNoiseLite> noise;

	Image::Format format = Image::FORMAT_L8;

	void _thread_function();

	void _queue_update();
	void _update_texture();

	Ref<Image> _modulate_with_gradient(Ref<Image> p_image, Ref<Gradient> p_gradient);

public:
	void set_noise(Ref<FastNoiseLite> p_noise);
	Ref<FastNoiseLite> get_noise();

	void set_width(int p_width);
	void set_height(int p_height);
	void set_depth(int p_depth);

	void set_invert(bool p_invert);
	bool get_invert() const;

	void set_seamless(bool p_seamless);
	bool get_seamless();

	void set_seamless_blend_skirt(real_t p_blend_skirt);
	real_t get_seamless_blend_skirt();

	void set_normalize(bool p_normalize);
	bool is_normalized() const;

	void set_color_ramp(const Ref<Gradient>& p_gradient);
	Ref<Gradient> get_color_ramp() const;

	virtual int get_width() const override;
	virtual int get_height() const override;
	virtual int get_depth() const override;

	virtual bool has_mipmaps() const override;

	virtual RID get_rid() const override;

	virtual Vector<Ref<Image>> get_data() const override;
	virtual Image::Format get_format() const override;

	NoiseTexture3D();
	virtual ~NoiseTexture3D() = default;
};
// --- End modules/noise/fastnoise_lite.h ---
