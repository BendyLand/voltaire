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
#include "scene/resources/bit_map.h"
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
#include "scene/resources/bit_map.h"
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
