/**************************************************************************/
/*  resource_loader.h                                                     */
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

#include "core/types.h"
// --- Folded from core/io/resource_loader_constants.h ---
namespace ResourceLoaderConstants
{

enum CacheMode
{
	CACHE_MODE_IGNORE,
	CACHE_MODE_REUSE,
	CACHE_MODE_REPLACE,
	CACHE_MODE_IGNORE_DEEP,
	CACHE_MODE_REPLACE_DEEP,
};

} // namespace ResourceLoaderConstants



// --- End core/io/resource_loader_constants.h ---
#include "core/os/mutex.h"

namespace CoreBind
{
class ResourceLoader;
}

class ConditionVariable;

template <int Tag> class SafeBinaryMutex;

class ResourceFormatLoader : public RefCounted
{
public:
	using CacheMode = ResourceLoaderConstants::CacheMode;
	static constexpr CacheMode CACHE_MODE_IGNORE = ResourceLoaderConstants::CACHE_MODE_IGNORE;
	static constexpr CacheMode CACHE_MODE_REUSE = ResourceLoaderConstants::CACHE_MODE_REUSE;
	static constexpr CacheMode CACHE_MODE_REPLACE = ResourceLoaderConstants::CACHE_MODE_REPLACE;
	static constexpr CacheMode CACHE_MODE_IGNORE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_IGNORE_DEEP;
	static constexpr CacheMode CACHE_MODE_REPLACE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_REPLACE_DEEP;

public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE);
	virtual bool exists(const String& p_path) const;
	virtual void get_recognized_extensions(List<String>* p_extensions) const;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const;
	virtual bool recognize_path(const String& p_path, const String& p_for_type = String()) const;
	virtual bool handles_type(const String& p_type) const;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	virtual String get_resource_type(const String& p_path) const;
	virtual String get_resource_script_class(const String& p_path) const;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const;
	virtual bool has_custom_uid_support() const;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);
	virtual Error rename_dependencies(const String& p_path, const HashMap<String, String>& p_map);

	virtual bool is_import_valid(const String& p_path) const { return true; }

	virtual bool is_imported(const String& p_path) const { return false; }

	virtual int get_import_order(const String& p_path) const { return 0; }

	virtual String get_import_group_file(const String& p_path) const { return ""; } // no group

	virtual ~ResourceFormatLoader() = default;
};

typedef void (*ResourceLoadErrorNotify)(const String& p_text);
typedef void (*DependencyErrorNotify)(
	const String& p_loading, const String& p_which, const String& p_type);

typedef Error (*ResourceLoaderImport)(const String& p_path);
typedef void (*ResourceLoadedCallback)(Ref<Resource> p_resource, const String& p_path);

class ResourceLoader
{
	friend class LoadToken;
	friend class CoreBind::ResourceLoader;

	enum
	{
		MAX_LOADERS = 64
	};

	struct ThreadLoadTask;

public:
	using CacheMode = ResourceLoaderConstants::CacheMode;
	static constexpr CacheMode CACHE_MODE_IGNORE = ResourceLoaderConstants::CACHE_MODE_IGNORE;
	static constexpr CacheMode CACHE_MODE_REUSE = ResourceLoaderConstants::CACHE_MODE_REUSE;
	static constexpr CacheMode CACHE_MODE_REPLACE = ResourceLoaderConstants::CACHE_MODE_REPLACE;
	static constexpr CacheMode CACHE_MODE_IGNORE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_IGNORE_DEEP;
	static constexpr CacheMode CACHE_MODE_REPLACE_DEEP =
		ResourceLoaderConstants::CACHE_MODE_REPLACE_DEEP;

	enum ThreadLoadStatus
	{
		THREAD_LOAD_INVALID_RESOURCE,
		THREAD_LOAD_IN_PROGRESS,
		THREAD_LOAD_FAILED,
		THREAD_LOAD_LOADED
	};

	enum LoadThreadMode
	{
		LOAD_THREAD_FROM_CURRENT,
		LOAD_THREAD_SPAWN_SINGLE,
		LOAD_THREAD_DISTRIBUTE,
	};

	struct LoadToken : public RefCounted
	{
		String local_path;
		String user_path;
		uint32_t user_rc = 0; // Having user RC implies regular RC incremented in one, until the
							  // user RC reaches zero.
		ThreadLoadTask* task_if_unregistered = nullptr;

		void clear();

		virtual ~LoadToken();
	};

	static const int BINARY_MUTEX_TAG = 1;

	static Ref<LoadToken> _load_start(const String& p_path, const String& p_type_hint,
		LoadThreadMode p_thread_mode, CacheMode p_cache_mode, bool p_for_user = false);
	static Ref<Resource> _load_complete(LoadToken& p_load_token, Error* r_error);

private:
	static LoadToken* _load_threaded_request_reuse_user_token(const String& p_path);
	static void _load_threaded_request_setup_user_token(LoadToken* p_token, const String& p_path);

	static Ref<Resource> _load_complete_inner(LoadToken& p_load_token, Error* r_error,
		MutexLock<SafeBinaryMutex<BINARY_MUTEX_TAG>>& p_thread_load_lock);

	static Ref<ResourceFormatLoader> loader[MAX_LOADERS];
	static int loader_count;
	static bool timestamp_on_load;

	static void* err_notify_ud;
	static ResourceLoadErrorNotify err_notify;
	static void* dep_err_notify_ud;
	static DependencyErrorNotify dep_err_notify;
	static bool abort_on_missing_resource;
	static bool create_missing_resources_if_class_unavailable;
	static HashMap<String, Vector<String>> translation_remaps;

	static String _path_remap(const String& p_path, bool* r_translation_remapped = nullptr);
	friend class Resource;

	static SelfList<Resource>::List remapped_list;

	friend class ResourceFormatImporter;

	static Ref<Resource> _load(const String& p_path, const String& p_original_path,
		const String& p_type_hint, CacheMode p_cache_mode, Error* r_error, bool p_use_sub_threads,
		float* r_progress);

	static ResourceLoadedCallback _loaded_callback;

	static Ref<ResourceFormatLoader> _find_custom_resource_format_loader(const String& p_path);

	struct ThreadLoadTask
	{
		Thread::ID thread_id =
			0; // Used if running on an user thread (e.g., simple non-threaded load).
		int thread_index = -1;
		ConditionVariable* cond_var = nullptr; // In not in the worker pool or already awaiting,
											   // this is used as a secondary awaiting mechanism.
		uint32_t awaiters_count = 0;
		LoadToken* load_token = nullptr;
		String local_path;
		String type_hint;
		float progress = 0.0f;
		float max_reported_progress = 0.0f;
		uint64_t last_progress_check_main_thread_frame = UINT64_MAX;
		ThreadLoadStatus status = THREAD_LOAD_IN_PROGRESS;
		CacheMode cache_mode = CACHE_MODE_REUSE;
		Error error = OK;
		Ref<Resource> resource;
		LocalVector<Ref<Resource>> resource_dependencies; // We need to keep these alive for as long
														  // as the task is alive at least.
		ThreadLoadTask* parent_task = nullptr;
		HashSet<String> sub_tasks;

		bool awaited : 1; // If it's in the pool, this helps not awaiting from more than one
						  // dependent thread.
		bool need_wait : 1;
		bool in_progress_check : 1; // Measure against recursion cycles in progress reporting.
									// Cycles are not expected, but can happen due to how it's
									// currently implemented.
		bool use_sub_threads : 1;
		bool started_load : 1;
		bool finished_load : 1;
		bool connections_propagated : 1;

		struct ResourceChangedConnection
		{
			Resource* source = nullptr;
			uint32_t flags = 0;
		};

		LocalVector<ResourceChangedConnection> resource_changed_connections;

		ThreadLoadTask()
			: awaited(false), need_wait(true), in_progress_check(false), use_sub_threads(false),
			  started_load(false), finished_load(false), connections_propagated(false)
		{
		}
	};

	static void _run_load_task(void* p_userdata);

	static thread_local bool import_thread;
	static thread_local int load_nesting;
	static thread_local HashMap<int, HashMap<String, Ref<Resource>>>
		res_ref_overrides; // Outermost key is nesting level.
	static thread_local ThreadLoadTask* curr_load_task;

	static SafeBinaryMutex<BINARY_MUTEX_TAG> thread_load_mutex;
	friend SafeBinaryMutex<BINARY_MUTEX_TAG>& _get_res_loader_mutex();

	static HashMap<String, ThreadLoadTask> thread_load_tasks;
	static HashMap<int, String> thread_waiting_on;
	static LocalVector<int> yielders;
	static bool cleaning_tasks;

	static HashMap<String, LoadToken*> user_load_tokens;

	static float _dependency_get_progress(const String& p_path);

	static bool _ensure_load_progress();

	static String _validate_local_path(const String& p_path);

public:
	static Error load_threaded_request(const String& p_path, const String& p_type_hint = "",
		bool p_use_sub_threads = false, CacheMode p_cache_mode = CACHE_MODE_REUSE);
	static ThreadLoadStatus load_threaded_get_status(
		const String& p_path, float* r_progress = nullptr);
	static Ref<Resource> load_threaded_get(const String& p_path, Error* r_error = nullptr);

	static bool is_within_load() { return load_nesting > 0; }

	static void resource_changed_emit(Resource* p_source);

	static Ref<Resource> load(const String& p_path, const String& p_type_hint = "",
		CacheMode p_cache_mode = CACHE_MODE_REUSE, Error* r_error = nullptr);
	static bool exists(const String& p_path, const String& p_type_hint = "");

	static void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions);
	static void add_resource_format_loader(
		Ref<ResourceFormatLoader> p_format_loader, bool p_at_front = false);
	static void remove_resource_format_loader(Ref<ResourceFormatLoader> p_format_loader);
	static void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	static String get_resource_type(const String& p_path);
	static String get_resource_script_class(const String& p_path);
	static ResourceUID::ID get_resource_uid(const String& p_path);
	static bool should_create_uid_file(const String& p_path);
	static void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);
	static Error rename_dependencies(const String& p_path, const HashMap<String, String>& p_map);
	static bool is_import_valid(const String& p_path);
	static String get_import_group_file(const String& p_path);
	static bool is_imported(const String& p_path);

	static void set_is_import_thread(bool p_import_thread);

	static void set_timestamp_on_load(bool p_timestamp) { timestamp_on_load = p_timestamp; }

	static bool get_timestamp_on_load() { return timestamp_on_load; }

	// Loaders can safely use this regardless which thread they are running on.
	static void notify_load_error(const String& p_err);

	static void set_error_notify_func(ResourceLoadErrorNotify p_err_notify)
	{
		err_notify = p_err_notify;
	}

	// Loaders can safely use this regardless which thread they are running on.
	static void notify_dependency_error(
		const String& p_path, const String& p_dependency, const String& p_type);

	static void set_dependency_error_notify_func(DependencyErrorNotify p_err_notify)
	{
		dep_err_notify = p_err_notify;
	}

	static void set_abort_on_missing_resources(bool p_abort)
	{
		abort_on_missing_resource = p_abort;
	}

	static bool get_abort_on_missing_resources() { return abort_on_missing_resource; }

	static String path_remap(const String& p_path);
	static String import_remap(const String& p_path);

	static void reload_translation_remaps();
	static void load_translation_remaps();
	static void clear_translation_remaps();

	static void clear_thread_load_tasks();

	static void set_load_callback(ResourceLoadedCallback p_callback);
	static ResourceLoaderImport import;

	static bool add_custom_resource_format_loader(const String& p_script_path);
	static void add_custom_loaders();
	static void remove_custom_loaders();

	static void set_create_missing_resources_if_class_unavailable(bool p_enable);

	_FORCE_INLINE_ static bool is_creating_missing_resources_if_class_unavailable_enabled()
	{
		return create_missing_resources_if_class_unavailable;
	}

	static Ref<Resource> ensure_resource_ref_override_for_outer_load(
		const String& p_path, const String& p_res_type);
	static Ref<Resource> get_resource_ref_override(const String& p_path);

	static bool is_cleaning_tasks();

	static Vector<String> list_directory(const String& p_directory);

	static void initialize();
	static void finalize();
};

// --- Folded from core/io/image_resource_format.h ---
// --- Folded from core/io/image_loader.h ---
#include "core/io/file_access.h"
// --- Folded from core/io/image.h ---
#include "core/types.h"
#include "core/math/vector2.h"

/**
 * Image storage class. This is used to store an image in user memory, as well as
 * providing some basic methods for image manipulation.
 * Images can be loaded from a file, or registered into the Render object as textures.
 */

class Image;

// Function pointer prototypes.

typedef Error (*SavePNGFunc)(const String& p_path, const Ref<Image>& p_img);
typedef Vector<uint8_t> (*SavePNGBufferFunc)(const Ref<Image>& p_img);

typedef Error (*SaveJPGFunc)(const String& p_path, const Ref<Image>& p_img, float p_quality);
typedef Vector<uint8_t> (*SaveJPGBufferFunc)(const Ref<Image>& p_img, float p_quality);

typedef Ref<Image> (*ImageMemLoadFunc)(const uint8_t* p_data, int p_size);
typedef Ref<Image> (*ScalableImageMemLoadFunc)(const uint8_t* p_data, int p_size, float p_scale);

typedef Error (*SaveWebPFunc)(
	const String& p_path, const Ref<Image>& p_img, const bool p_lossy, const float p_quality);
typedef Vector<uint8_t> (*SaveWebPBufferFunc)(
	const Ref<Image>& p_img, const bool p_lossy, const float p_quality);

typedef Error (*SaveEXRFunc)(const String& p_path, const Ref<Image>& p_img, bool p_grayscale,
	bool p_color_image, float p_max_value);
typedef Vector<uint8_t> (*SaveEXRBufferFunc)(
	const Ref<Image>& p_img, bool p_grayscale, bool p_color_image, float p_max_value);

typedef Error (*SaveDDSFunc)(const String& p_path, const Ref<Image>& p_img);
typedef Vector<uint8_t> (*SaveDDSBufferFunc)(const Ref<Image>& p_img);

class Image : public Resource
{
public:
	enum
	{
		MAX_WIDTH = (1 << 24),	// Force a limit somehow.
		MAX_HEIGHT = (1 << 24), // Force a limit somehow.
		MAX_PIXELS = 268435456	// 16384 ^ 2
	};

	enum Format : int32_t
	{
		FORMAT_L8,	// Luminance
		FORMAT_LA8, // Luminance-Alpha
		FORMAT_R8,
		FORMAT_RG8,
		FORMAT_RGB8,
		FORMAT_RGBA8,
		FORMAT_RGBA4444,
		FORMAT_RGB565,
		FORMAT_RF, // Float
		FORMAT_RGF,
		FORMAT_RGBF,
		FORMAT_RGBAF,
		FORMAT_RH, // Half
		FORMAT_RGH,
		FORMAT_RGBH,
		FORMAT_RGBAH,
		FORMAT_RGBE9995,
		FORMAT_DXT1,	   // BC1
		FORMAT_DXT3,	   // BC2
		FORMAT_DXT5,	   // BC3
		FORMAT_RGTC_R,	   // BC4
		FORMAT_RGTC_RG,	   // BC5
		FORMAT_BPTC_RGBA,  // BC7
		FORMAT_BPTC_RGBF,  // BC6 Signed
		FORMAT_BPTC_RGBFU, // BC6 Unsigned
		FORMAT_ETC,		   // ETC1
		FORMAT_ETC2_R11,
		FORMAT_ETC2_R11S, // Signed, NOT srgb.
		FORMAT_ETC2_RG11,
		FORMAT_ETC2_RG11S, // Signed, NOT srgb.
		FORMAT_ETC2_RGB8,
		FORMAT_ETC2_RGBA8,
		FORMAT_ETC2_RGB8A1,
		FORMAT_ETC2_RA_AS_RG, // ETC2 RGBA with a RA-RG swizzle for normal maps.
		FORMAT_DXT5_RA_AS_RG, // BC3 with a RA-RG swizzle for normal maps.
		FORMAT_ASTC_4x4,
		FORMAT_ASTC_4x4_HDR,
		FORMAT_ASTC_8x8,
		FORMAT_ASTC_8x8_HDR,
		FORMAT_R16,
		FORMAT_RG16,
		FORMAT_RGB16,
		FORMAT_RGBA16,
		FORMAT_R16I,
		FORMAT_RG16I,
		FORMAT_RGB16I,
		FORMAT_RGBA16I,
		FORMAT_ASTC_6x6,
		FORMAT_ASTC_6x6_HDR,
		FORMAT_MAX
	};

	static const char* format_names[FORMAT_MAX];

	enum Interpolation
	{
		INTERPOLATE_NEAREST,
		INTERPOLATE_BILINEAR,
		INTERPOLATE_CUBIC,
		INTERPOLATE_TRILINEAR,
		INTERPOLATE_LANCZOS,
		// INTERPOLATE_TRICUBIC,
		// INTERPOLATE_GAUSS
	};

	// Used for obtaining optimal compression quality.
	enum UsedChannels
	{
		USED_CHANNELS_L,
		USED_CHANNELS_LA,
		USED_CHANNELS_R,
		USED_CHANNELS_RG,
		USED_CHANNELS_RGB,
		USED_CHANNELS_RGBA,
	};

#ifndef DISABLE_DEPRECATED
	// ASTC supports block formats other than 4x4.
	enum ASTCFormat
	{
		ASTC_FORMAT_4x4,
		ASTC_FORMAT_8x8,
	};
#endif

	enum BPTCFormat
	{
		BPTC_DETECT,
		BPTC_FORCE_SIGNED,
		BPTC_FORCE_UNSIGNED,
	};

	enum RoughnessChannel
	{
		ROUGHNESS_CHANNEL_R,
		ROUGHNESS_CHANNEL_G,
		ROUGHNESS_CHANNEL_B,
		ROUGHNESS_CHANNEL_A,
		ROUGHNESS_CHANNEL_L,
	};

	enum Image3DValidateError
	{
		VALIDATE_3D_OK,
		VALIDATE_3D_ERR_IMAGE_EMPTY,
		VALIDATE_3D_ERR_MISSING_IMAGES,
		VALIDATE_3D_ERR_EXTRA_IMAGES,
		VALIDATE_3D_ERR_IMAGE_SIZE_MISMATCH,
		VALIDATE_3D_ERR_IMAGE_FORMAT_MISMATCH,
		VALIDATE_3D_ERR_IMAGE_HAS_MIPMAPS,
	};

	enum CompressMode
	{
		COMPRESS_S3TC,
		COMPRESS_ETC,
		COMPRESS_ETC2,
		COMPRESS_BPTC,
		COMPRESS_ASTC,
		COMPRESS_MAX,
	};

	enum CompressSource
	{
		COMPRESS_SOURCE_GENERIC,
		COMPRESS_SOURCE_SRGB,
		COMPRESS_SOURCE_NORMAL,
		COMPRESS_SOURCE_MAX,
	};

	enum CompressProfile
	{
		COMPRESS_PROFILE_AUTOMATIC,
		COMPRESS_PROFILE_MAX_QUALITY,
		COMPRESS_PROFILE_COMPRESSED,
		COMPRESS_PROFILE_MAX_COMPRESSION,
		COMPRESS_PROFILE_MAX
	};

	enum AlphaMode
	{
		ALPHA_NONE,
		ALPHA_BIT,
		ALPHA_BLEND
	};

	struct BasisUniversalPackerParams
	{
		int uastc_level = 0;
		float rdo_quality_loss = 0;
	};

	// External saver function pointers.

	static inline SavePNGFunc save_png_func = nullptr;
	static inline SaveJPGFunc save_jpg_func = nullptr;
	static inline SaveEXRFunc save_exr_func = nullptr;
	static inline SaveWebPFunc save_webp_func = nullptr;
	static inline SaveDDSFunc save_dds_func = nullptr;
	static inline SavePNGBufferFunc save_png_buffer_func = nullptr;
	static inline SaveEXRBufferFunc save_exr_buffer_func = nullptr;
	static inline SaveJPGBufferFunc save_jpg_buffer_func = nullptr;
	static inline SaveWebPBufferFunc save_webp_buffer_func = nullptr;
	static inline SaveDDSBufferFunc save_dds_buffer_func = nullptr;

	// External loader function pointers.

	static inline ImageMemLoadFunc _png_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _png_mem_unpacker_func = nullptr;
	static inline ImageMemLoadFunc _jpg_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _webp_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _tga_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _bmp_mem_loader_func = nullptr;
	static inline ScalableImageMemLoadFunc _svg_scalable_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _ktx_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _dds_mem_loader_func = nullptr;
	static inline ImageMemLoadFunc _exr_mem_loader_func = nullptr;

	// External VRAM compression function pointers.

	static void (*_image_compress_bc_func)(Image*, UsedChannels p_channels);
	static void (*_image_compress_bptc_func)(
		Image*, UsedChannels p_channels, BPTCFormat p_bptc_format);
	static void (*_image_compress_etc1_func)(Image*);
	static void (*_image_compress_etc2_func)(Image*, UsedChannels p_channels);
	static void (*_image_compress_astc_func)(
		Image*, UsedChannels p_channels, CompressProfile p_profile);

	static Error (*_image_compress_bptc_rd_func)(
		Image*, UsedChannels p_channels, BPTCFormat p_bptc_format);
	static Error (*_image_compress_bc_rd_func)(Image*, UsedChannels p_channels);

	// External VRAM decompression function pointers.

	static void (*_image_decompress_bc)(Image*);
	static void (*_image_decompress_bptc)(Image*);
	static void (*_image_decompress_etc1)(Image*);
	static void (*_image_decompress_etc2)(Image*);
	static void (*_image_decompress_astc)(Image*);

	// External packer function pointers.

	static Vector<uint8_t> (*webp_lossy_packer)(const Ref<Image>& p_image, float p_quality);
	static Vector<uint8_t> (*webp_lossless_packer)(const Ref<Image>& p_image);
	static Vector<uint8_t> (*png_packer)(const Ref<Image>& p_image);
	static Vector<uint8_t> (*basis_universal_packer)(const Ref<Image>& p_image,
		UsedChannels p_channels, const BasisUniversalPackerParams& p_basisu_params);

	static Ref<Image> (*webp_unpacker)(const Vector<uint8_t>& p_buffer);
	static Ref<Image> (*png_unpacker)(const Vector<uint8_t>& p_buffer);
	static Ref<Image> (*basis_universal_unpacker)(const Vector<uint8_t>& p_buffer);
	static Ref<Image> (*basis_universal_unpacker_ptr)(const uint8_t* p_data, int p_size);

protected:

#ifndef DISABLE_DEPRECATED
	Error _compress_bind_compat_115003(
		CompressMode p_mode, CompressSource p_source, ASTCFormat p_format);
	Error _compress_from_channels_bind_compat_115003(
		CompressMode p_mode, UsedChannels p_channels, ASTCFormat p_format);
	Vector<uint8_t> _save_exr_to_buffer_bind_compat_117800(bool p_grayscale = false) const;
	Error _save_exr_bind_compat_117800(const String& p_path, bool p_grayscale = false) const;

	static void _bind_compatibility_methods();
#endif

private:
	Format format = FORMAT_L8;
	Vector<uint8_t> data;
	int width = 0;
	int height = 0;
	bool mipmaps = false;

	void _copy_internals_from(const Image& p_image);

	_FORCE_INLINE_ Color _get_color_at_ofs(const uint8_t* p_ptr, uint32_t p_ofs) const;
	_FORCE_INLINE_ void _set_color_at_ofs(uint8_t* r_ptr, uint32_t p_ofs, const Color& p_color);

	_FORCE_INLINE_ void _get_mipmap_offset_and_size(int p_mipmap, int64_t& r_offset, int& r_width,
		int& r_height) const; // Get where the mipmap begins in data.

	static int64_t _get_dst_image_size(int p_width, int p_height, Format p_format, int& r_mipmaps,
		int p_mipmaps = -1, int* r_mm_width = nullptr, int* r_mm_height = nullptr);

	_FORCE_INLINE_ void _get_clipped_src_and_dest_rects(const Ref<Image>& p_src,
		const Rect2i& p_src_rect, const Point2i& p_dest, Rect2i& r_clipped_src_rect,
		Rect2i& r_clipped_dest_rect) const;

	_FORCE_INLINE_ void _put_pixelb(
		int p_x, int p_y, uint32_t p_pixel_size, uint8_t* p_data, const uint8_t* p_pixel);
	_FORCE_INLINE_ void _get_pixelb(
		int p_x, int p_y, uint32_t p_pixel_size, const uint8_t* p_data, uint8_t* p_pixel);

	_FORCE_INLINE_ void _repeat_pixel_over_subsequent_memory(
		uint8_t* p_pixel, int p_pixel_size, int p_count);

	Error _load_from_buffer(const Vector<uint8_t>& p_array, ImageMemLoadFunc p_loader);

	_FORCE_INLINE_ void _generate_mipmap_from_format(Image::Format p_format, const uint8_t* p_src,
		uint8_t* p_dst, uint32_t p_width, uint32_t p_height, bool p_renormalize = false);

	static void average_4_uint8(uint8_t& p_out, const uint8_t& p_a, const uint8_t& p_b,
		const uint8_t& p_c, const uint8_t& p_d);
	static void average_4_float(
		float& p_out, const float& p_a, const float& p_b, const float& p_c, const float& p_d);
	static void average_4_half(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);
	static void average_4_rgbe9995(uint32_t& p_out, const uint32_t& p_a, const uint32_t& p_b,
		const uint32_t& p_c, const uint32_t& p_d);
	static void average_4_uint16(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);
	static void average_4_rgba4444(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);
	static void average_4_rgb565(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
		const uint16_t& p_c, const uint16_t& p_d);

	static void renormalize_uint8(uint8_t* p_rgb);
	static void renormalize_float(float* p_rgb);
	static void renormalize_half(uint16_t* p_rgb);
	static void renormalize_uint16(uint16_t* p_rgb);

public:
	int get_width() const;
	int get_height() const;
	Size2i get_size() const;
	bool has_mipmaps() const;
	int get_mipmap_count() const;

	// Convert the image to another format, conversion only to raw byte format.
	void convert(Format p_new_format);

	Format get_format() const;

	// Get where the mipmap begins in data.
	int64_t get_mipmap_offset(int p_mipmap) const;
	void get_mipmap_offset_and_size(int p_mipmap, int64_t& r_ofs, int64_t& r_size) const;
	void get_mipmap_offset_size_and_dimensions(
		int p_mipmap, int64_t& r_ofs, int64_t& r_size, int& p_w, int& p_h) const;

	static Image3DValidateError validate_3d_image(Format p_format, int p_width, int p_height,
		int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_images);
	static String get_3d_image_validation_error_text(Image3DValidateError p_error);

	// Resize the image, using the preferred interpolation method.
	void resize_to_po2(bool p_square = false, Interpolation p_interpolation = INTERPOLATE_BILINEAR);
	void resize(int p_width, int p_height, Interpolation p_interpolation = INTERPOLATE_BILINEAR);
	void shrink_x2();
	bool is_size_po2() const;

	// Crop the image to a specific size, if larger, then the image is filled by black.
	void crop_from_point(int p_x, int p_y, int p_width, int p_height);
	void crop(int p_width, int p_height);

	void rotate_90(ClockDirection p_direction);
	void rotate_180();

	void flip_x();
	void flip_y();

	// Generate a mipmap chain of an image (creates an image 1/4 the size, with averaging of 4->1).
	Error generate_mipmaps(bool p_renormalize = false);

	void clear_mipmaps();
	void normalize();

	// Creates new internal image data of a given size and format. Current image will be lost.
	void initialize_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format);
	void initialize_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format,
		const Vector<uint8_t>& p_data);
	void initialize_data(const char** p_xpm);

	// Returns true when the image is empty (0,0) in size.
	bool is_empty() const;

	const Vector<uint8_t>& get_data() const _LIFETIME_BOUND_;

	Error load(const String& p_path);
	static Ref<Image> load_from_file(const String& p_path);
	Error save_png(const String& p_path) const;
	Error save_jpg(const String& p_path, float p_quality = 0.75) const;
	Error save_dds(const String& p_path) const;
	Vector<uint8_t> save_png_to_buffer() const;
	Vector<uint8_t> save_jpg_to_buffer(float p_quality = 0.75) const;
	Vector<uint8_t> save_exr_to_buffer(
		bool p_grayscale = false, bool p_color_image = false, float p_max_value = -1.0f) const;
	Vector<uint8_t> save_dds_to_buffer() const;
	Error save_exr(const String& p_path, bool p_grayscale = false, bool p_color_image = false,
		float p_max_value = -1.0f) const;
	Error save_webp(
		const String& p_path, const bool p_lossy = false, const float p_quality = 0.75f) const;
	Vector<uint8_t> save_webp_to_buffer(
		const bool p_lossy = false, const float p_quality = 0.75f) const;

	static Ref<Image> create_empty(int p_width, int p_height, bool p_use_mipmaps, Format p_format);
	static Ref<Image> create_from_data(int p_width, int p_height, bool p_use_mipmaps,
		Format p_format, const Vector<uint8_t>& p_data);
	void set_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format,
		const Vector<uint8_t>& p_data);

	Image() = default; // Create an empty image.
	Image(int p_width, int p_height, bool p_use_mipmaps,
		Format p_format); // Create an empty image of a specific size and format.
	Image(int p_width, int p_height, bool p_mipmaps, Format p_format,
		const Vector<uint8_t>&
			p_data); // Import an image of a specific size and format from a byte vector.
	Image(
		const uint8_t* p_mem_png_jpg, int p_len = -1); // Import either a png or jpg from a pointer.
	Image(const char** p_xpm);						   // Import an XPM image.

	AlphaMode detect_alpha() const;
	bool is_invisible() const;

	static int get_format_pixel_size(Format p_format);
	static uint64_t get_format_pixels_shifted(Format p_format, uint64_t p_pixels);
	static int get_format_block_size(Format p_format);
	static void get_format_min_pixel_size(Format p_format, int& r_w, int& r_h);

	static int64_t get_image_data_size(
		int p_width, int p_height, Format p_format, bool p_mipmaps = false);
	static int get_image_required_mipmaps(int p_width, int p_height, Format p_format);
	static Size2i get_image_mipmap_size(int p_width, int p_height, Format p_format, int p_mipmap);
	static int64_t get_image_mipmap_offset(
		int p_width, int p_height, Format p_format, int p_mipmap);
	static int64_t get_image_mipmap_offset_and_dimensions(
		int p_width, int p_height, Format p_format, int p_mipmap, int& r_w, int& r_h);

	Error compress(CompressMode p_mode, CompressSource p_source = COMPRESS_SOURCE_GENERIC,
		CompressProfile p_profile = COMPRESS_PROFILE_AUTOMATIC);
	Error compress_from_channels(CompressMode p_mode, UsedChannels p_channels,
		CompressProfile p_profile = COMPRESS_PROFILE_AUTOMATIC);
	Error _compress_from_channels(CompressMode p_mode, UsedChannels p_channels,
		CompressProfile p_profile, BPTCFormat p_bptc_format);
	Error decompress();
	bool is_compressed() const;
	static bool is_format_compressed(Format p_format);

	static bool can_decompress(const String& p_format_tag);

	void fix_alpha_edges();
	void premultiply_alpha();
	void srgb_to_linear();
	void linear_to_srgb();
	void normal_map_to_xy();
	Ref<Image> rgbe_to_srgb();
	Ref<Image> get_image_from_mipmap(int p_mipmap) const;
	void bump_map_to_normal_map(float p_bump_scale = 1.0);

	bool detect_signed(bool p_include_mips = true) const;

	void blit_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest);
	void blit_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask, const Rect2i& p_src_rect,
		const Point2i& p_dest);
	void blend_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest);
	void blend_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask,
		const Rect2i& p_src_rect, const Point2i& p_dest);
	void fill(const Color& p_color);
	void fill_rect(const Rect2i& p_rect, const Color& p_color);

	Rect2i get_used_rect() const;
	Ref<Image> get_region(const Rect2i& p_area) const;

	static String get_format_name(Format p_format);
	static uint32_t get_format_component_mask(Format p_format);

	Error load_png_from_buffer(const Vector<uint8_t>& p_array);
	Error load_jpg_from_buffer(const Vector<uint8_t>& p_array);
	Error load_webp_from_buffer(const Vector<uint8_t>& p_array);
	Error load_tga_from_buffer(const Vector<uint8_t>& p_array);
	Error load_bmp_from_buffer(const Vector<uint8_t>& p_array);
	Error load_ktx_from_buffer(const Vector<uint8_t>& p_array);
	Error load_dds_from_buffer(const Vector<uint8_t>& p_array);
	Error load_exr_from_buffer(const Vector<uint8_t>& p_array);

	Error load_svg_from_buffer(const Vector<uint8_t>& p_array, float p_scale = 1.0);
	Error load_svg_from_string(const String& p_svg_str, float p_scale = 1.0);

	void convert_rg_to_ra_rgba8();
	void convert_ra_rgba8_to_rg();
	void convert_rgba8_to_bgra8();

	UsedChannels detect_used_channels(CompressSource p_source = COMPRESS_SOURCE_GENERIC) const;
	void optimize_channels();

	Color get_pixelv(const Point2i& p_point) const;
	Color get_pixel(int p_x, int p_y) const;
	void set_pixelv(const Point2i& p_point, const Color& p_color);
	void set_pixel(int p_x, int p_y, const Color& p_color);

	const uint8_t* ptr() const;
	uint8_t* ptrw();
	int64_t get_data_size() const;

	void adjust_bcs(float p_brightness, float p_contrast, float p_saturation);

	void set_as_black();

	void copy_internals_from(const Ref<Image>& p_image);
};
// --- End core/io/image.h ---
#include "core/string/ustring.h"
#include "core/templates/list.h"

class ImageLoader;

class ImageFormatLoader : public RefCounted
{
	friend class ImageLoader;
	friend class ResourceFormatLoaderImage;

public:
	enum LoaderFlags
	{
		FLAG_NONE = 0,
		FLAG_FORCE_LINEAR = 1,
		FLAG_CONVERT_COLORS = 2,
	};

protected:

	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess,
		uint32_t p_flags = FLAG_NONE, float p_scale = 1.0) = 0;
	virtual void get_recognized_extensions(List<String>* p_extensions) const = 0;
	bool recognize(const String& p_extension) const;

public:
	virtual ~ImageFormatLoader() {}
};

class ImageFormatLoaderExtension : public ImageFormatLoader
{
protected:

public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess,
		uint32_t p_flags = FLAG_NONE, float p_scale = 1.0) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;

	void add_format_loader();
	void remove_format_loader();
};

class ImageLoader
{
	static inline Vector<Ref<ImageFormatLoader>> loader;
	friend class ResourceFormatLoaderImage;

protected:
public:
	static Error load_image(const String& p_file, Ref<Image> p_image,
		Ref<FileAccess> p_custom = Ref<FileAccess>(),
		uint32_t p_flags = ImageFormatLoader::FLAG_NONE,
		float p_scale = 1.0);
	static void get_recognized_extensions(List<String>* p_extensions);
	static Ref<ImageFormatLoader> recognize(const String& p_extension);

	static void add_image_format_loader(Ref<ImageFormatLoader> p_loader);
	static void remove_image_format_loader(Ref<ImageFormatLoader> p_loader);

	static void cleanup();
};

// --- Folded from modules/svg/image_loader_svg.h ---
class ImageLoaderSVG : public ImageFormatLoader {
	static HashMap<Color, Color> forced_color_map;

	static void _replace_color_property(const HashMap<Color, Color> &p_color_map, const String &p_prefix, String &r_string);

	static Ref<Image> load_mem_svg(const uint8_t *p_svg, int p_size, float p_scale);

public:
	static void set_forced_color_map(const HashMap<Color, Color> &p_color_map);

	static Error create_image_from_utf8_buffer(Ref<Image> p_image, const uint8_t *p_buffer, int p_buffer_size, float p_scale, bool p_upsample);
	static Error create_image_from_utf8_buffer(Ref<Image> p_image, const PackedByteArray &p_buffer, float p_scale, bool p_upsample);

	static Error create_image_from_string(Ref<Image> p_image, String p_string, float p_scale, bool p_upsample, const HashMap<Color, Color> &p_color_map);

	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess, uint32_t p_flags, float p_scale) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;

	ImageLoaderSVG();
};
// --- End core/io/image_loader.h ---


class ResourceFormatLoaderImage : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
};



// --- End core/io/image_resource_format.h ---

// --- Folded from core/io/resource_format_binary.h ---
#include "core/io/file_access.h"

// --- Folded from core/io/resource_saver.h ---
#include "core/types.h"

class ResourceFormatSaver : public RefCounted
{
protected:

public:
	virtual Error save(const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0);
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid);
	virtual bool recognize(const Ref<Resource>& p_resource) const;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const;
	virtual bool recognize_path(const Ref<Resource>& p_resource, const String& p_path) const;

	virtual ~ResourceFormatSaver() {}
};

typedef void (*ResourceSavedCallback)(Ref<Resource> p_resource, const String& p_path);
typedef ResourceUID::ID (*ResourceSaverGetResourceIDForPath)(const String& p_path, bool p_generate);

class ResourceSaver
{
	enum
	{
		MAX_SAVERS = 64
	};

	static Ref<ResourceFormatSaver> saver[MAX_SAVERS];
	static int saver_count;
	static bool timestamp_on_save;
	static ResourceSavedCallback save_callback;
	static ResourceSaverGetResourceIDForPath save_get_id_for_path;

	static Ref<ResourceFormatSaver> _find_custom_resource_format_saver(const String& p_path);

public:
	enum SaverFlags
	{
		FLAG_NONE = 0,
		FLAG_RELATIVE_PATHS = 1,
		FLAG_BUNDLE_RESOURCES = 2,
		FLAG_CHANGE_PATH = 4,
		FLAG_OMIT_EDITOR_PROPERTIES = 8,
		FLAG_SAVE_BIG_ENDIAN = 16,
		FLAG_COMPRESS = 32,
		FLAG_REPLACE_SUBRESOURCE_PATHS = 64,
	};

	static Error save(
		Resource* rp_resource, const String& p_path = "", uint32_t p_flags = (uint32_t)FLAG_NONE);
	static void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions);
	static void add_resource_format_saver(
		Ref<ResourceFormatSaver> p_format_saver, bool p_at_front = false);
	static void remove_resource_format_saver(Ref<ResourceFormatSaver> p_format_saver);

	static Error set_uid(const String& p_path, ResourceUID::ID p_uid);

	static void set_timestamp_on_save(bool p_timestamp) { timestamp_on_save = p_timestamp; }

	static bool get_timestamp_on_save() { return timestamp_on_save; }

	static ResourceUID::ID get_resource_id_for_path(const String& p_path, bool p_generate = false);

	static void set_save_callback(ResourceSavedCallback p_callback);
	static void set_get_resource_id_for_path(ResourceSaverGetResourceIDForPath p_callback);

	static bool add_custom_resource_format_saver(const String& p_script_path);
	static void add_custom_savers();
	static void remove_custom_savers();
};
// --- End core/io/resource_saver.h ---
#include "core/types.h"

class ResourceLoaderBinary
{
	bool translation_remapped = false;
	String local_path;
	String res_path;
	String type;
	Ref<Resource> resource;
	uint32_t ver_format = 0;

	Ref<FileAccess> f;

	uint64_t importmd_ofs = 0;

	ResourceUID::ID uid = ResourceUID::INVALID_ID;

	Vector<char> str_buf;
	List<Ref<Resource>> resource_cache;

	Vector<String> string_map;

	String _get_string();

	struct ExtResource
	{
		String path;
		String type;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
		Ref<ResourceLoader::LoadToken> load_token;
	};

	bool using_named_scene_ids = false;
	bool using_uids = false;
	String script_class;
	bool use_sub_threads = false;
	float* progress = nullptr;
	Vector<ExtResource> external_resources;

	struct IntResource
	{
		String path;
		uint64_t offset;
	};

	Vector<IntResource> internal_resources;
	HashMap<String, Ref<Resource>> internal_index_cache;

	String get_unicode_string();
	void _advance_padding(uint32_t p_len);

	HashMap<String, String> remaps;
	Error error = OK;

	ResourceFormatLoader::CacheMode cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
	ResourceFormatLoader::CacheMode cache_mode_for_external =
		ResourceFormatLoader::CACHE_MODE_REUSE;

	friend class ResourceFormatLoaderBinary;

	HashMap<String, Ref<Resource>> dependency_cache;

public:
	Ref<Resource> get_resource();
	Error load();
	void set_translation_remapped(bool p_remapped);

	void set_remaps(const HashMap<String, String>& p_remaps) { remaps = p_remaps; }

	void open(Ref<FileAccess> p_file, bool p_no_resources = false, bool p_keep_uuid_paths = false);
	String recognize(Ref<FileAccess> p_file);
	String recognize_script_class(Ref<FileAccess> p_file);
	void get_dependencies(Ref<FileAccess> p_file, List<String>* p_dependencies, bool p_add_types);
	void get_classes_used(Ref<FileAccess> p_file, HashSet<String>* p_classes);
};

class ResourceFormatLoaderBinary : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
	virtual String get_resource_script_class(const String& p_path) const override;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override;
	virtual bool has_custom_uid_support() const override;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override;
	virtual Error rename_dependencies(
		const String& p_path, const HashMap<String, String>& p_map) override;
};

class ResourceFormatSaverBinaryInstance
{
	String local_path;
	String path;

	bool relative_paths;
	bool bundle_resources;
	bool skip_editor;
	bool big_endian;
	bool takeover_paths;
	String magic;
	HashSet<Ref<Resource>> resource_set;

	struct NonPersistentKey
	{ // for resource properties generated on the fly
		Ref<Resource> base;
		String property;

		bool operator<(const NonPersistentKey& p_key) const
		{
			return base == p_key.base ? property < p_key.property : base < p_key.base;
		}
	};

	HashMap<String, int> string_map;
	Vector<String> strings;

	HashMap<Ref<Resource>, int> external_resources;
	List<Ref<Resource>> saved_resources;

	struct Property
	{
		int name_idx;
	};

	struct ResourceData
	{
		String type;
		List<Property> properties;
	};

	static void _pad_buffer(Ref<FileAccess> r_file, int p_bytes);
	static void save_unicode_string(
		Ref<FileAccess> r_file, const String& p_string, bool p_bit_on_len = false);
	int get_string_index(const String& p_string);

public:
	enum
	{
		FORMAT_FLAG_NAMED_SCENE_IDS = 1,
		FORMAT_FLAG_UIDS = 2,
		FORMAT_FLAG_REAL_T_IS_DOUBLE = 4,
		FORMAT_FLAG_HAS_SCRIPT_CLASS = 8,

		// Amount of reserved 32-bit fields in resource header
		RESERVED_FIELDS = 11
	};

	Error save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags = 0);
	Error set_uid(const String& p_path, ResourceUID::ID p_uid);
};

class ResourceFormatSaverBinary : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceFormatSaverBinary() = default;
};



// --- End core/io/resource_format_binary.h ---

// --- Folded from core/io/missing_resource.h ---
#include "core/types.h"

#define META_PROPERTY_MISSING_RESOURCES "metadata/_missing_resources"
#define META_MISSING_RESOURCES "_missing_resources"

class MissingResource : public Resource
{
	String original_class;
	bool recording_properties = false;

protected:

public:
	void set_original_class(const String& p_class);
	String get_original_class() const;

	void set_recording_properties(bool p_enable);
	bool is_recording_properties() const;

	virtual String get_save_class() const;
};



// --- End core/io/missing_resource.h ---

// --- Folded from drivers/png/png_driver_common.h ---


namespace PNGDriverCommon {

// Attempt to load png from buffer (p_source, p_size) into p_image
Error png_to_image(const uint8_t *p_source, size_t p_size, bool p_force_linear, Ref<Image> p_image);

// Append p_image, as a png, to p_buffer.
// Contents of p_buffer is unspecified if error returned.
Error image_to_png(const Ref<Image> &p_image, Vector<uint8_t> &p_buffer);
} // namespace PNGDriverCommon
// --- End drivers/png/png_driver_common.h ---

// --- Folded from core/io/resource_importer.h ---



class ResourceImporter;
class ResourceFormatImporter;

typedef Ref<Resource> (*ResourceFormatImporterLoadOnStartup)(
	const String& p_path, Error* r_error, bool p_use_sub_threads, float* r_progress,
	ResourceFormatLoader::CacheMode p_cache_mode);

class ResourceFormatImporter final
{
public:
	using CacheMode = ResourceFormatLoader::CacheMode;

private:
	struct PathAndType
	{
		String path;
		String type;
		String importer;
		String group_file;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
	};

	static Error _get_path_and_type(const String& p_path, PathAndType& r_path_and_type, bool p_load,
		bool* r_valid = nullptr);

	// need them to stay in order to compute the settings hash
	struct SortImporterByName
	{
		bool operator()(const Ref<ResourceImporter>& p_a, const Ref<ResourceImporter>& p_b) const;
	};

	struct Data
	{
		Vector<Ref<ResourceImporter>> importers;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE);
	static Ref<Resource> load_internal(const String& p_path, Error* r_error, bool p_use_sub_threads,
		float* r_progress, CacheMode p_cache_mode, bool p_silence_errors);
	static void get_recognized_extensions(List<String>* p_extensions);
	static void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions);
	static bool recognize_path(
		const String& p_path, const String& p_for_type = String());
	static bool handles_type(const String& p_type);
	static String get_resource_type(const String& p_path);
	static String get_resource_script_class(const String& p_path);
	static ResourceUID::ID get_resource_uid(const String& p_path);
	static bool has_custom_uid_support();
	static bool is_import_valid(const String& p_path);
	static void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);

	static bool is_imported(const String& p_path) { return recognize_path(p_path); }

	static String get_import_group_file(const String& p_path);
	static void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	static bool exists(const String& p_path);

	static void get_build_dependencies(const String& p_path, HashSet<String>* r_dependencies);

	static int get_import_order(const String& p_path);

	static Error get_import_order_threads_and_importer(
		const String& p_path, int& r_order, bool& r_can_threads, String& r_importer);

	static String get_internal_resource_path(const String& p_path);
	static void get_internal_resource_path_list(const String& p_path, List<String>* r_paths);

	static void add_importer(const Ref<ResourceImporter>& p_importer, bool p_first_priority = false);

	static void remove_importer(const Ref<ResourceImporter>& p_importer)
	{
		if (data) {
			data->importers.erase(p_importer);
		}
	}

	static Ref<ResourceImporter> get_importer_by_name(const String& p_name);
	static Ref<ResourceImporter> get_importer_by_file(const String& p_file);
	static void get_importers_for_file(const String& p_file, List<Ref<ResourceImporter>>* r_importers);
	static void get_importers(List<Ref<ResourceImporter>>* r_importers);

	static bool are_import_settings_valid(const String& p_path);
	static String get_import_settings_hash();

	static String get_import_base_path(const String& p_for_file);
	static Error get_resource_import_info(const String& p_path, String& r_type, ResourceUID::ID& r_uid,
		String& r_import_group_file);

	ResourceFormatImporter() = delete;
	ResourceFormatImporter(const ResourceFormatImporter&) = delete;
	ResourceFormatImporter& operator=(const ResourceFormatImporter&) = delete;
	~ResourceFormatImporter() = delete;
};

class ResourceFormatImporterLoader : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override
	{
		return ResourceFormatImporter::load(
			p_path, p_original_path, r_error, p_use_sub_threads, r_progress, p_cache_mode);
	}
	virtual void get_recognized_extensions(List<String>* p_extensions) const override
	{
		ResourceFormatImporter::get_recognized_extensions(p_extensions);
	}
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override
	{
		ResourceFormatImporter::get_recognized_extensions_for_type(p_type, p_extensions);
	}
	virtual bool recognize_path(
		const String& p_path, const String& p_for_type = String()) const override
	{
		return ResourceFormatImporter::recognize_path(p_path, p_for_type);
	}
	virtual bool handles_type(const String& p_type) const override
	{
		return ResourceFormatImporter::handles_type(p_type);
	}
	virtual String get_resource_type(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_type(p_path);
	}
	virtual String get_resource_script_class(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_script_class(p_path);
	}
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_uid(p_path);
	}
	virtual bool has_custom_uid_support() const override
	{
		return ResourceFormatImporter::has_custom_uid_support();
	}
	virtual bool is_import_valid(const String& p_path) const override
	{
		return ResourceFormatImporter::is_import_valid(p_path);
	}
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override
	{
		ResourceFormatImporter::get_dependencies(p_path, p_dependencies, p_add_types);
	}
	virtual bool is_imported(const String& p_path) const override
	{
		return ResourceFormatImporter::is_imported(p_path);
	}
	virtual String get_import_group_file(const String& p_path) const override
	{
		return ResourceFormatImporter::get_import_group_file(p_path);
	}
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override
	{
		ResourceFormatImporter::get_classes_used(p_path, r_classes);
	}
	virtual bool exists(const String& p_path) const override
	{
		return ResourceFormatImporter::exists(p_path);
	}
	virtual int get_import_order(const String& p_path) const override
	{
		return ResourceFormatImporter::get_import_order(p_path);
	}
};

class ResourceImporter : public RefCounted
{
protected:

public:
	static inline ResourceFormatImporterLoadOnStartup load_on_startup = nullptr;

	virtual String get_importer_name() const = 0;
	virtual String get_visible_name() const = 0;
	virtual void get_recognized_extensions(List<String>* p_extensions) const = 0;
	virtual String get_save_extension() const = 0;
	virtual String get_resource_type() const = 0;

	virtual float get_priority() const { return 1.0; }

	virtual int get_import_order() const { return IMPORT_ORDER_DEFAULT; }

	virtual int get_format_version() const { return 0; }

	struct ImportOption
	{
		ImportOption() {}
	};

	enum ImportOrder
	{
		IMPORT_ORDER_DEFAULT = 0,
		IMPORT_ORDER_SCENE = 100,
	};

	virtual bool has_advanced_options() const { return false; }

	virtual void show_advanced_options(const String& p_path) {}

	virtual int get_preset_count() const { return 0; }

	virtual String get_preset_name(int p_idx) const { return String(); }

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const = 0;

	virtual String get_option_group_file() const { return String(); }

	virtual bool can_import_threaded() const { return false; }

	virtual void import_threaded_begin() {}

	virtual void import_threaded_end() {}

	virtual String get_import_settings_string() const { return String(); }

	virtual void get_build_dependencies(
		const String& p_path, HashSet<String>* r_build_dependencies);
};

class ResourceFormatImporterSaver : public ResourceFormatSaver
{
public:
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
};
// --- End core/io/resource_importer.h ---

// --- Folded from core/io/marshalls.h ---
#include "core/typedefs.h"
#include "core/types.h"

// uintr_t is only for pairing with real_t, and we only need it in here.
#ifdef REAL_T_IS_DOUBLE
typedef uint64_t uintr_t;
#else
typedef uint32_t uintr_t;
#endif

/**
 * Miscellaneous helpers for marshaling data types, and encoding
 * in an endian independent way
 */

union MarshallFloat
{
	uint32_t i; ///< int
	float f;	///< float
};

union MarshallDouble
{
	uint64_t l; ///< long long
	double d;	///< double
};

// Behaves like one of the above, depending on compilation setting.
union MarshallReal
{
	uintr_t i;
	real_t r;
};

static inline unsigned int encode_uint16(uint16_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 2; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint16_t);
}

static inline unsigned int encode_uint32(uint32_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 4; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint32_t);
}

static inline unsigned int encode_half(float p_float, uint8_t* p_arr)
{
	encode_uint16(Math::make_half_float(p_float), p_arr);

	return sizeof(uint16_t);
}

static inline unsigned int encode_float(float p_float, uint8_t* p_arr)
{
	MarshallFloat mf;
	mf.f = p_float;
	encode_uint32(mf.i, p_arr);

	return sizeof(uint32_t);
}

static inline unsigned int encode_uint64(uint64_t p_uint, uint8_t* p_arr)
{
	for (int i = 0; i < 8; i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uint64_t);
}

static inline unsigned int encode_double(double p_double, uint8_t* p_arr)
{
	MarshallDouble md;
	md.d = p_double;
	encode_uint64(md.l, p_arr);

	return sizeof(uint64_t);
}

static inline unsigned int encode_uintr(uintr_t p_uint, uint8_t* p_arr)
{
	for (size_t i = 0; i < sizeof(uintr_t); i++) {
		*p_arr = p_uint & 0xFF;
		p_arr++;
		p_uint >>= 8;
	}

	return sizeof(uintr_t);
}

static inline unsigned int encode_real(real_t p_real, uint8_t* p_arr)
{
	MarshallReal mr;
	mr.r = p_real;
	encode_uintr(mr.i, p_arr);

	return sizeof(uintr_t);
}

static inline int encode_cstring(const char* p_string, uint8_t* p_data)
{
	int len = 0;

	while (*p_string) {
		if (p_data) {
			*p_data = (uint8_t)*p_string;
			p_data++;
		}
		p_string++;
		len++;
	}

	if (p_data) {
		*p_data = 0;
	}
	return len + 1;
}

static inline uint16_t decode_uint16(const uint8_t* p_arr)
{
	uint16_t u = 0;

	for (int i = 0; i < 2; i++) {
		uint16_t b = *p_arr;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline uint32_t decode_uint32(const uint8_t* p_arr)
{
	uint32_t u = 0;

	for (int i = 0; i < 4; i++) {
		uint32_t b = *p_arr;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline float decode_half(const uint8_t* p_arr)
{
	return Math::half_to_float(decode_uint16(p_arr));
}

static inline float decode_float(const uint8_t* p_arr)
{
	MarshallFloat mf;
	mf.i = decode_uint32(p_arr);
	return mf.f;
}

static inline uint64_t decode_uint64(const uint8_t* p_arr)
{
	uint64_t u = 0;

	for (int i = 0; i < 8; i++) {
		uint64_t b = (*p_arr) & 0xFF;
		b <<= (i * 8);
		u |= b;
		p_arr++;
	}

	return u;
}

static inline double decode_double(const uint8_t* p_arr)
{
	MarshallDouble md;
	md.l = decode_uint64(p_arr);
	return md.d;
}

class EncodedObjectAsID : public RefCounted
{
protected:
};

Vector<float> vector3_to_float32_array(const Vector3* p_vecs, size_t p_count);
// --- End core/io/marshalls.h ---

// --- Folded from core/io/config_file.h ---
#include "core/io/file_access.h"
#include "core/types.h"

class ConfigFile : public RefCounted
{
	Error _internal_load(const String& p_path, Ref<FileAccess> p_file);
	Error _internal_save(Ref<FileAccess> p_file);

	Error _parse(const String& p_path);

public:
	bool has_section(const String& p_section) const;
	bool has_section_key(const String& p_section, const String& p_key) const;

	Vector<String> get_sections() const;
	Vector<String> get_section_keys(const String& p_section) const;

	void erase_section(const String& p_section);
	void erase_section_key(const String& p_section, const String& p_key);

	Error save(const String& p_path);
	Error load(const String& p_path);
	Error parse(const String& p_data);

	String encode_to_text() const; // used by exporter

	void clear();

	Error load_encrypted(const String& p_path, const Vector<uint8_t>& p_key);
	Error load_encrypted_pass(const String& p_path, const String& p_pass);

	Error save_encrypted(const String& p_path, const Vector<uint8_t>& p_key);
	Error save_encrypted_pass(const String& p_path, const String& p_pass);
};

// --- Folded from core/io/json_resource_format.h ---

#include "core/types.h"

class ResourceFormatLoaderJSON : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;

	// Treat JSON as a text file, do not generate a `*.json.uid` file.
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceUID::INVALID_ID;
	}

	virtual bool has_custom_uid_support() const override { return true; }
};

class ResourceFormatSaverJSON : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
};



// --- End core/io/json_resource_format.h ---

// --- Folded from core/io/plist.h ---
// Property list file format (application/x-plist) parser, property list ASN-1 serialization.

#include "core/io/file_access.h"

class PListNode;

class PList : public RefCounted
{
	friend class PListNode;

public:
	enum PLNodeType
	{
		PL_NODE_TYPE_NIL,
		PL_NODE_TYPE_STRING,
		PL_NODE_TYPE_ARRAY,
		PL_NODE_TYPE_DICT,
		PL_NODE_TYPE_BOOLEAN,
		PL_NODE_TYPE_INTEGER,
		PL_NODE_TYPE_REAL,
		PL_NODE_TYPE_DATA,
		PL_NODE_TYPE_DATE,
	};

private:
	struct PListTrailer
	{
		uint8_t offset_size;
		uint8_t ref_size;
		uint64_t object_num;
		uint64_t root_offset_idx;
		uint64_t offset_table_start;
	};

	PListTrailer trailer;
	Ref<PListNode> root;

	uint64_t read_bplist_var_size_int(Ref<FileAccess> p_file, uint8_t p_size);
	Ref<PListNode> read_bplist_obj(Ref<FileAccess> p_file, uint64_t p_offset_idx);

public:
	PList();
	PList(const String& p_string);

	bool load_file(const String& p_filename);
	bool load_string(const String& p_string, String& r_err_out);

	Vector<uint8_t> save_asn1() const;
	String save_text() const;

	Ref<PListNode> get_root();
};

/*************************************************************************/

class PListNode : public RefCounted
{
	static int _asn1_size_len(uint8_t p_len_octets);

public:
	PList::PLNodeType data_type = PList::PLNodeType::PL_NODE_TYPE_NIL;

	CharString data_string;
	Vector<Ref<PListNode>> data_array;
	HashMap<String, Ref<PListNode>> data_dict;

	union
	{
		int64_t data_int;
		bool data_bool;
		double data_real;
	};

	PList::PLNodeType get_type() const;

	static Ref<PListNode> new_array();
	static Ref<PListNode> new_dict();
	static Ref<PListNode> new_string(const String& p_string);
	static Ref<PListNode> new_data(const String& p_string);
	static Ref<PListNode> new_date(const String& p_string);
	static Ref<PListNode> new_bool(bool p_bool);
	static Ref<PListNode> new_int(int64_t p_int);
	static Ref<PListNode> new_real(double p_real);

	bool push_subnode(const Ref<PListNode>& p_node, const String& p_key = "");

	size_t get_asn1_size(uint8_t p_len_octets) const;

	void store_asn1_size(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const;
	bool store_asn1(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const;
	void store_text(String& p_stream, uint8_t p_indent) const;
};



// --- End core/io/plist.h ---

// --- Folded from core/io/translation_loader_po.h ---
#include "core/io/file_access.h"


class TranslationLoaderPO : public ResourceFormatLoader
{
public:
	static Ref<Resource> load_translation(Ref<FileAccess> r_file, Error* r_error = nullptr);
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;

	// Treat translations as text/binary files, do not generate a `*.{po,mo}.uid` file.
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceUID::INVALID_ID;
	}

	virtual bool has_custom_uid_support() const override { return true; }
};



// --- End core/io/translation_loader_po.h ---

// --- Folded from core/io/json.h ---
#include "core/types.h"

class JSON : public Resource
{
	enum TokenType
	{
		TK_CURLY_BRACKET_OPEN,
		TK_CURLY_BRACKET_CLOSE,
		TK_BRACKET_OPEN,
		TK_BRACKET_CLOSE,
		TK_IDENTIFIER,
		TK_STRING,
		TK_NUMBER,
		TK_COLON,
		TK_COMMA,
		TK_EOF,
		TK_MAX
	};

	enum Expecting
	{
		EXPECT_OBJECT,
		EXPECT_OBJECT_KEY,
		EXPECT_COLON,
		EXPECT_OBJECT_VALUE,
	};

	struct Token
	{
		TokenType type;
	};

	String text;
	String err_str;
	int err_line = 0;

	static const char* tk_name[];

	static void _add_indent(String& r_result, const String& p_indent, int p_size);
	static Error _get_token(const char32_t* p_str, int& r_index, int p_len, Token& r_token,
		int& r_line, String& r_err_str);

protected:

public:
	Error parse(const String& p_json_string, bool p_keep_text = false);
	String get_parsed_text() const;

	_FORCE_INLINE_ int get_error_line() const { return err_line; }

	_FORCE_INLINE_ String get_error_message() const { return err_str; }
};
// --- End core/io/json.h ---

// --- Folded from core/io/xml_parser.h ---
#include "core/string/ustring.h"
#include "core/types.h"

/*
  Based on irrXML (see their zlib license). Added mainly for compatibility with their Collada
  loader.
*/

class XMLParser : public RefCounted
{
public:
	//! Enumeration of all supported source text file formats
	enum SourceFormat
	{
		SOURCE_ASCII,
		SOURCE_UTF8,
		SOURCE_UTF16_BE,
		SOURCE_UTF16_LE,
		SOURCE_UTF32_BE,
		SOURCE_UTF32_LE
	};

	enum NodeType
	{
		NODE_NONE,
		NODE_ELEMENT,
		NODE_ELEMENT_END,
		NODE_TEXT,
		NODE_COMMENT,
		NODE_CDATA,
		NODE_UNKNOWN
	};

private:
	char* data_copy = nullptr;
	const char* data = nullptr;
	const char* P = nullptr;
	uint64_t length = 0;
	uint64_t current_line = 0;
	String node_name;
	bool node_empty = false;
	NodeType node_type = NODE_NONE;
	uint64_t node_offset = 0;

	struct Attribute
	{
		String name;
		String value;
	};

	Vector<Attribute> attributes;

	bool _set_text(const char* p_start, const char* p_end);
	void _parse_closing_xml_element();
	void _ignore_definition();
	bool _parse_cdata();
	void _parse_comment();
	void _parse_opening_xml_element();
	void _parse_current_node();

	_FORCE_INLINE_ void next_char()
	{
		if (*P == '\n') {
			current_line++;
		}
		P++;
	}


public:
	Error read();
	NodeType get_node_type();
	String get_node_name() const;
	String get_node_data() const;
	uint64_t get_node_offset() const;
	int get_attribute_count() const;
	String get_attribute_name(int p_idx) const;
	String get_attribute_value(int p_idx) const;
	bool has_attribute(const String& p_name) const;
	String get_named_attribute_value(const String& p_name) const;
	String get_named_attribute_value_safe(
		const String& p_name) const; // do not print error if doesn't exist
	bool is_empty() const;
	int get_current_line() const;

	void skip_section();
	Error seek(uint64_t p_pos);

	Error open(const String& p_path);
	Error open_buffer(const Vector<uint8_t>& p_buffer);
	Error _open_buffer(const uint8_t* p_buffer, size_t p_size);

	void close();

	~XMLParser();
};
// --- End core/io/xml_parser.h ---
// --- End core/io/config_file.h ---
