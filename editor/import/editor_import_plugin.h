/**************************************************************************/
/*  editor_import_plugin.h                                                */
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

#include "core/io/resource_importer.h"

class EditorImportPlugin : public ResourceImporter
{
protected:

public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_preset_name(int p_idx) const override;
	virtual int get_preset_count() const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual float get_priority() const override;
	virtual int get_import_order() const override;
	virtual int get_format_version() const override;
	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset) const override;
	virtual bool can_import_threaded() const override;
};

// --- Folded from editor/import/resource_importer_csv_translation.h ---
#include "core/io/resource_importer.h"

class ResourceImporterCSVTranslation : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

	virtual bool can_import_threaded() const override { return true; }
};



// --- End editor/import/resource_importer_csv_translation.h ---

// --- Folded from editor/import/resource_importer_texture.h ---
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_importer.h"
#include "servers/rendering/rendering_server_enums.h"

class CompressedTexture2D;

class LayeredTextureImport final : public RefCounted
{
public:
	Image::CompressSource* csource = nullptr;
	String save_path;
	List<String>* platform_variants = nullptr;
	Ref<Image> image = nullptr;
	Vector<Ref<Image>>* slices = nullptr;
	int compress_mode = 0;
	float lossy = 1.0;

	Image::BasisUniversalPackerParams basisu_params;

	int hdr_compression = 0;
	bool mipmaps = true;
	bool high_quality = false;
	Image::CompressProfile compression_profile = Image::COMPRESS_PROFILE_AUTOMATIC;
	Image::UsedChannels used_channels = Image::USED_CHANNELS_RGBA;
};

class ResourceImporterTexture final : public ResourceImporter
{
public:
	enum ImportTarget
	{
		TARGET_TEXTURE_2D,
		TARGET_LAYERED_2D_ARRAY,
		TARGET_CUBEMAP,
		TARGET_CUBEMAP_ARRAY,
		TARGET_TEXTURE_3D,
		TARGET_TEXTURE_ATLAS,
		TARGET_IMAGE,
		TARGET_BITMAP,
		TARGET_SVG,
	};

	enum Mode
	{
		MODE_2D_ARRAY,
		MODE_CUBEMAP,
		MODE_CUBEMAP_ARRAY,
		MODE_3D,
	};

	enum CubemapFormat
	{
		CUBEMAP_FORMAT_1X6,
		CUBEMAP_FORMAT_2X3,
		CUBEMAP_FORMAT_3X2,
		CUBEMAP_FORMAT_6X1,
	};

	enum TextureFlags
	{
		TEXTURE_FLAGS_MIPMAPS = 1
	};

	enum ImportMode
	{
		IMPORT_MODE_REGION,
		IMPORT_MODE_2D_MESH
	};

	enum CompressMode
	{
		COMPRESS_LOSSLESS,
		COMPRESS_LOSSY,
		COMPRESS_VRAM_COMPRESSED,
		COMPRESS_VRAM_UNCOMPRESSED,
		COMPRESS_BASIS_UNIVERSAL
	};

	enum ChannelRemap
	{
		REMAP_R,
		REMAP_G,
		REMAP_B,
		REMAP_A,
		REMAP_INV_R,
		REMAP_INV_G,
		REMAP_INV_B,
		REMAP_INV_A,
		REMAP_UNUSED,
		REMAP_0,
		REMAP_1,
	};

	enum Preset
	{
		PRESET_DETECT,
		PRESET_2D,
		PRESET_3D,
	};

	static inline ResourceImporterTexture* singleton = nullptr;

private:
	struct PackData
	{
		Rect2 region;
		bool is_cropped = false;
		bool is_mesh = false;
		Vector<int> chart_pieces;				// one for region, many for mesh
		Vector<Vector<Vector2>> chart_vertices; // for mesh
		Ref<Image> image;
	};

	enum
	{
		MAKE_3D_FLAG = 1,
		MAKE_ROUGHNESS_FLAG = 2,
		MAKE_NORMAL_FLAG = 4
	};

	ImportTarget target = TARGET_TEXTURE_2D;

	static inline Mutex mutex;

	struct MakeInfo
	{
		int flags = 0;
		String normal_path_for_roughness;
		RSE::TextureDetectRoughnessChannel channel_for_roughness = RSE::TEXTURE_DETECT_ROUGHNESS_R;
	};

	static inline HashMap<String, MakeInfo> make_flags;

	static void _texture_reimport_roughness(const Ref<CompressedTexture2D>& p_tex,
		const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_channel);
	static void _texture_reimport_3d(const Ref<CompressedTexture2D>& p_tex);
	static void _texture_reimport_normal(const Ref<CompressedTexture2D>& p_tex);

	static const char* compression_formats[];

	static inline void _remap_channels(Ref<Image>& r_image, ChannelRemap p_options[4]);
	static inline void _clamp_hdr_exposure(Ref<Image>& r_image);
	static inline void _invert_y_channel(Ref<Image>& r_image);

	Mode _get_layered_mode() const;

public:
	static void save_to_ctex_format(Ref<FileAccess> f, const Ref<Image>& p_image,
		CompressMode p_compress_mode, Image::UsedChannels p_channels,
		Image::CompressMode p_compress_format, Image::CompressProfile p_compress_profile,
		float p_lossy_quality, const Image::BasisUniversalPackerParams& p_basisu_params,
		Image::BPTCFormat p_bptc_format);

	void _check_compress_ctex(
		const String& p_source_file, Ref<LayeredTextureImport> r_texture_import);

	void _save_tex(Vector<Ref<Image>> p_images, const String& p_to_path, int p_compress_mode,
		float p_lossy, const Image::BasisUniversalPackerParams& p_basisu_params,
		Image::CompressMode p_vram_compression, Image::CompressProfile p_vram_compression_profile,
		Image::CompressSource p_csource, Image::UsedChannels used_channels, bool p_mipmaps,
		bool p_force_po2, Image::BPTCFormat p_bptc_format);

	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;
	virtual String get_option_group_file() const override;

	virtual bool can_import_threaded() const override { return true; }

	void update_imports();

	virtual String get_import_settings_string() const override;

	ImportTarget get_target() const { return target; }
	void set_target(ImportTarget p_target) { target = p_target; }
	void set_mode(Mode p_mode);

	ResourceImporterTexture(bool p_singleton = false);
	ResourceImporterTexture(ImportTarget p_target, bool p_singleton = false);
	~ResourceImporterTexture();
};

// --- End editor/import/resource_importer_texture.h ---
