/**************************************************************************/
/*  editor_import_plugin.cpp                                              */
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

#include "editor/file_system/editor_file_system.h"
#include "editor_import_plugin.h"

bool EditorImportPlugin::can_import_threaded() const
{
	return ResourceImporter::can_import_threaded();
}

String EditorImportPlugin::get_importer_name() const { return ""; }

String EditorImportPlugin::get_visible_name() const { return ""; }

String EditorImportPlugin::get_save_extension() const { return ""; }

String EditorImportPlugin::get_resource_type() const { return ""; }

int EditorImportPlugin::get_preset_count() const { return 0; }

float EditorImportPlugin::get_priority() const { return 1.0; }

int EditorImportPlugin::get_import_order() const { return 0; }

int EditorImportPlugin::get_format_version() const { return 0; }

String EditorImportPlugin::get_preset_name(int p_idx) const { return String(); }

void EditorImportPlugin::get_recognized_extensions(List<String>* r_extensions) const {}

void EditorImportPlugin::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}



// ============================================================================
// Merged from editor/import/editor_atlas_packer.cpp
// ============================================================================
// --- Inlined from editor/import/editor_atlas_packer.h ---
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/templates/vector.h"

class EditorAtlasPacker {
public:
	struct Chart {
		Vector<Vector2> vertices;
		struct Face {
			int vertex[3] = { 0 };
		};
		Vector<Face> faces;
		bool can_transpose = false;

		Vector2 final_offset;
		bool transposed = false;
	};

private:
	struct PlottedBitmap {
		int chart_index = 0;
		Vector2i offset;
		int area = 0;
		Vector<int> top_heights;
		Vector<int> bottom_heights;
		bool transposed = false;

		Vector2 final_pos;

		bool operator<(const PlottedBitmap &p_bm) const {
			return area > p_bm.area;
		}
	};

public:
	static void chart_pack(Vector<Chart> &charts, int &r_width, int &r_height, int p_atlas_max_size = 2048, int p_cell_resolution = 4);
};

// --- End editor/import/editor_atlas_packer.h ---

#include "core/math/geometry_2d.h"
#include "core/math/math_funcs_binary.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "scene/resources/bit_map.h"

void EditorAtlasPacker::chart_pack(Vector<Chart> &charts, int &r_width, int &r_height, int p_atlas_max_size, int p_cell_resolution) {
	int divide_by = MIN(64, p_cell_resolution);
	Vector<PlottedBitmap> bitmaps;

	int max_w = 0;

	for (int i = 0; i < charts.size(); i++) {
		const Chart &chart = charts[i];

		//generate aabb

		Rect2i aabb;
		int vertex_count = chart.vertices.size();
		const Vector2 *vertices = chart.vertices.ptr();

		for (int j = 0; j < vertex_count; j++) {
			if (j == 0) {
				aabb.position = vertices[j];
			} else {
				aabb.expand_to(vertices[j]);
			}
		}

		Ref<BitMap> src_bitmap;
		src_bitmap.instantiate();
		src_bitmap->create((aabb.size + Vector2(divide_by - 1, divide_by - 1)) / divide_by);

		int w = src_bitmap->get_size().width;
		int h = src_bitmap->get_size().height;

		//plot triangles, using divisor

		for (int j = 0; j < chart.faces.size(); j++) {
			Vector2i v[3];
			for (int k = 0; k < 3; k++) {
				Vector2 vtx = chart.vertices[chart.faces[j].vertex[k]];
				vtx -= aabb.position;
				vtx /= divide_by;
				vtx = vtx.min(Vector2(w - 1, h - 1));
				v[k] = vtx;
			}

			for (int k = 0; k < 3; k++) {
				int l = k == 0 ? 2 : k - 1;
				Vector<Point2i> points = Geometry2D::bresenham_line(v[k], v[l]);
				for (Point2i point : points) {
					src_bitmap->set_bitv(point, true);
				}
			}
		}

		//src_bitmap->convert_to_image()->save_png("bitmap" + itos(i) + ".png");

		//grow by 1 for each side

		int bmw = src_bitmap->get_size().width + 2;
		int bmh = src_bitmap->get_size().height + 2;

		int heights_size = -1;
		bool transpose = false;
		if (chart.can_transpose && bmh > bmw) {
			heights_size = bmh;
			transpose = true;
		} else {
			heights_size = bmw;
		}

		max_w = MAX(max_w, heights_size);

		Vector<int> top_heights;
		Vector<int> bottom_heights;
		top_heights.resize(heights_size);
		bottom_heights.resize(heights_size);

		for (int x = 0; x < heights_size; x++) {
			top_heights.write[x] = -1;
			bottom_heights.write[x] = 0x7FFFFFFF;
		}

		for (int x = 0; x < bmw; x++) {
			for (int y = 0; y < bmh; y++) {
				bool found_pixel = false;
				for (int lx = x - 1; lx < x + 2 && !found_pixel; lx++) {
					for (int ly = y - 1; ly < y + 2 && !found_pixel; ly++) {
						int px = lx - 1;
						if (px < 0 || px >= w) {
							continue;
						}
						int py = ly - 1;
						if (py < 0 || py >= h) {
							continue;
						}

						if (src_bitmap->get_bit(px, py)) {
							found_pixel = true;
						}
					}
				}
				if (found_pixel) {
					if (transpose) {
						if (x > top_heights[y]) {
							top_heights.write[y] = x;
						}
						if (x < bottom_heights[y]) {
							bottom_heights.write[y] = x;
						}
					} else {
						if (y > top_heights[x]) {
							top_heights.write[x] = y;
						}
						if (y < bottom_heights[x]) {
							bottom_heights.write[x] = y;
						}
					}
				}
			}
		}

		String row;
		for (int j = 0; j < top_heights.size(); j++) {
			row += "(" + itos(top_heights[j]) + "-" + itos(bottom_heights[j]) + "),";
		}

		PlottedBitmap plotted_bitmap;
		plotted_bitmap.offset = aabb.position;
		plotted_bitmap.top_heights = top_heights;
		plotted_bitmap.bottom_heights = bottom_heights;
		plotted_bitmap.chart_index = i;
		plotted_bitmap.transposed = transpose;
		plotted_bitmap.area = bmw * bmh;

		bitmaps.push_back(plotted_bitmap);
	}

	bitmaps.sort();

	int atlas_max_width = Math::nearest_power_of_2_templated(p_atlas_max_size) / divide_by;
	int atlas_w = Math::nearest_power_of_2_templated(max_w);
	int atlas_h;
	while (true) {
		atlas_h = 0;

		//do a tetris
		Vector<int> heights;
		heights.resize(atlas_w);
		for (int i = 0; i < atlas_w; i++) {
			heights.write[i] = 0;
		}

		int *atlas_ptr = heights.ptrw();

		for (int i = 0; i < bitmaps.size(); i++) {
			int best_height = 0x7FFFFFFF;
			int best_height_offset = -1;
			int w = bitmaps[i].top_heights.size();

			const int *top_heights = bitmaps[i].top_heights.ptr();
			const int *bottom_heights = bitmaps[i].bottom_heights.ptr();

			for (int j = 0; j <= atlas_w - w; j++) {
				int height = 0;

				for (int k = 0; k < w; k++) {
					int pixmap_h = bottom_heights[k];
					if (pixmap_h == 0x7FFFFFFF) {
						continue; //no pixel here, anything is fine
					}

					int h = MAX(0, atlas_ptr[j + k] - pixmap_h);
					if (h > height) {
						height = h;
					}
				}

				if (height < best_height) {
					best_height = height;
					best_height_offset = j;
				}
			}

			for (int j = 0; j < w; j++) { //add
				if (top_heights[j] == -1) { //unused
					continue;
				}
				int height = best_height + top_heights[j] + 1;
				atlas_ptr[j + best_height_offset] = height;
				atlas_h = MAX(atlas_h, height);
			}

			// set
			Vector2 offset = bitmaps[i].offset;
			if (bitmaps[i].transposed) {
				SWAP(offset.x, offset.y);
			}

			Vector2 final_pos = Vector2(best_height_offset * divide_by, best_height * divide_by) + Vector2(divide_by, divide_by) - offset;
			charts.write[bitmaps[i].chart_index].final_offset = final_pos;
			charts.write[bitmaps[i].chart_index].transposed = bitmaps[i].transposed;
		}

		if (atlas_h <= atlas_w * 2 || atlas_w >= atlas_max_width) {
			break; //ok this one is enough
		}

		//try again
		atlas_w *= 2;
	}

	r_width = atlas_w * divide_by;
	r_height = atlas_h * divide_by;
}

// ============================================================================
// Merged from editor/import/resource_importer_csv_translation.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "core/string/translation.h"
#include "core/string/translation_server.h"
#include "editor/import/editor_import_plugin.h"

String ResourceImporterCSVTranslation::get_importer_name() const { return "csv_translation"; }

String ResourceImporterCSVTranslation::get_visible_name() const { return "CSV Translation"; }

void ResourceImporterCSVTranslation::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("csv");
}

String ResourceImporterCSVTranslation::get_save_extension() const
{
	return ""; // does not save a single resource
}

String ResourceImporterCSVTranslation::get_resource_type() const { return "Translation"; }

int ResourceImporterCSVTranslation::get_preset_count() const { return 0; }

String ResourceImporterCSVTranslation::get_preset_name(int p_idx) const { return ""; }

void ResourceImporterCSVTranslation::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}



// ============================================================================
// Merged from editor/import/resource_importer_font.cpp
// ============================================================================
// --- Inlined from editor/import/resource_importer_font.h ---
#include "core/io/resource_importer.h"

class ResourceImporterFont final : public ResourceImporter
{
public:
	enum Mode
	{
		MODE_DYNAMIC,
		MODE_BMFONT,
		MODE_IMAGEFONT,
	};

private:
	enum Presets
	{
		PRESET_DYNAMIC,
		PRESET_MSDF,
		PRESET_MAX
	};

	Mode mode = MODE_DYNAMIC;

public:
	explicit ResourceImporterFont(Mode p_mode = MODE_DYNAMIC);

	Mode get_mode() const { return mode; }
	void set_mode(Mode p_mode) { mode = p_mode; }

	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual void get_build_dependencies(
		const String& p_path, HashSet<String>* r_build_dependencies) override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;
	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;

	virtual bool can_import_threaded() const override { return true; }
};

// --- End editor/import/resource_importer_font.h ---

#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/font.h"
#include "servers/text/text_server.h"

ResourceImporterFont::ResourceImporterFont(Mode p_mode) :
	mode(p_mode)
{
}

String ResourceImporterFont::get_importer_name() const
{
	switch (mode) {
	case MODE_DYNAMIC:
		return "font_data_dynamic";
	case MODE_BMFONT:
		return "font_data_bmfont";
	case MODE_IMAGEFONT:
		return "font_data_image";
	}
	ERR_FAIL_V(String());
}

String ResourceImporterFont::get_visible_name() const
{
	switch (mode) {
	case MODE_DYNAMIC:
		return "Font Data (Dynamic Font)";
	case MODE_BMFONT:
		return "Font Data (AngelCode BMFont)";
	case MODE_IMAGEFONT:
		return "Font Data (Image Font)";
	}
	ERR_FAIL_V(String());
}

void ResourceImporterFont::get_recognized_extensions(List<String>* p_extensions) const
{
	if (!p_extensions) {
		return;
	}
	switch (mode) {
	case MODE_DYNAMIC: {
		p_extensions->push_back("ttf");
		p_extensions->push_back("ttc");
		p_extensions->push_back("otf");
		p_extensions->push_back("otc");
		p_extensions->push_back("woff");
		p_extensions->push_back("woff2");
		p_extensions->push_back("pfb");
		p_extensions->push_back("pfm");
	} break;
	case MODE_BMFONT: {
		p_extensions->push_back("font");
		p_extensions->push_back("fnt");
	} break;
	case MODE_IMAGEFONT: {
		ImageLoader::get_recognized_extensions(p_extensions);
	} break;
	}
}

String ResourceImporterFont::get_save_extension() const { return "fontdata"; }

String ResourceImporterFont::get_resource_type() const { return "FontFile"; }

void ResourceImporterFont::get_build_dependencies(
	const String& p_path, HashSet<String>* r_dependencies)
{
	if (mode == MODE_DYNAMIC) {
		Ref<FontFile> font = ResourceLoader::load(p_path);
		if (font.is_valid() && font->is_multichannel_signed_distance_field()) {
			r_dependencies->insert("module_msdfgen_enabled");
		}
	}
}

int ResourceImporterFont::get_preset_count() const
{
	if (mode == MODE_DYNAMIC) {
		return PRESET_MAX;
	}
	return 0;
}

String ResourceImporterFont::get_preset_name(int p_idx) const
{
	if (mode == MODE_DYNAMIC) {
		switch (p_idx) {
		case PRESET_DYNAMIC:
			return TTR("Dynamically rendered TrueType/OpenType font");
		case PRESET_MSDF:
			return TTR("Prerendered multichannel(+true) signed distance field");
		default:
			return String();
		}
	}
	return String();
}

bool ResourceImporterFont::has_advanced_options() const { return false; }

void ResourceImporterFont::show_advanced_options(const String& p_path) { (void)p_path; }

void ResourceImporterFont::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
	(void)p_path;
	(void)r_options;
	(void)p_preset;
}

// ============================================================================
// Merged from editor/import/resource_importer_shader_file.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
// --- Inlined from editor/import/resource_importer_shader_file.h ---
#include "core/io/resource_importer.h"

class ResourceImporterShaderFile : public ResourceImporter
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



// --- End editor/import/resource_importer_shader_file.h ---
#include "servers/rendering/rendering_device_binds.h"

String ResourceImporterShaderFile::get_importer_name() const { return "glsl"; }

String ResourceImporterShaderFile::get_visible_name() const { return "GLSL Shader File"; }

void ResourceImporterShaderFile::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("glsl");
}

String ResourceImporterShaderFile::get_save_extension() const { return "res"; }

String ResourceImporterShaderFile::get_resource_type() const { return "RDShaderFile"; }

int ResourceImporterShaderFile::get_preset_count() const { return 0; }

String ResourceImporterShaderFile::get_preset_name(int p_idx) const { return String(); }

void ResourceImporterShaderFile::get_import_options(
	const String& p_path, List<ImportOption>* r_options, int p_preset) const
{
}

static String _include_function(const String& p_path, void* userpointer)
{
	Error err;

	String* base_path = (String*)userpointer;

	String include = p_path;
	if (include.is_relative_path()) {
		include = base_path->path_join(include);
	}

	Ref<FileAccess> file_inc = FileAccess::open(include, FileAccess::READ, &err);
	if (err != OK) {
		return String();
	}
	return file_inc->get_as_utf8_string();
}



// ============================================================================
// Merged from editor/import/resource_importer_texture.cpp
// ============================================================================

#include "editor/import/editor_import_plugin.h"

#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/config_file.h"
#include "core/io/image_loader.h"
#include "core/io/resource_saver.h"
#include "core/math/geometry_2d.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/import/atlas_import_failed.xpm"

#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/compressed_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/mesh.h"

void ResourceImporterTexture::_texture_reimport_roughness(const Ref<CompressedTexture2D>& p_tex,
	const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_channel)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(mutex);
	String path = p_tex->get_path();

	if (!make_flags.has(path)) {
		make_flags[path] = MakeInfo();
	}

	make_flags[path].flags |= MAKE_ROUGHNESS_FLAG;
	make_flags[path].channel_for_roughness = p_channel;
	make_flags[path].normal_path_for_roughness = p_normal_path;
}

void ResourceImporterTexture::_texture_reimport_3d(const Ref<CompressedTexture2D>& p_tex)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(mutex);
	String path = p_tex->get_path();

	if (!make_flags.has(path)) {
		make_flags[path] = MakeInfo();
	}

	make_flags[path].flags |= MAKE_3D_FLAG;
}

void ResourceImporterTexture::_texture_reimport_normal(const Ref<CompressedTexture2D>& p_tex)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(mutex);
	String path = p_tex->get_path();

	if (!make_flags.has(path)) {
		make_flags[path] = MakeInfo();
	}

	make_flags[path].flags |= MAKE_NORMAL_FLAG;
}

void ResourceImporterTexture::set_mode(Mode p_mode)
{
	switch (p_mode) {
	case MODE_2D_ARRAY:
		target = TARGET_LAYERED_2D_ARRAY;
		break;
	case MODE_CUBEMAP:
		target = TARGET_CUBEMAP;
		break;
	case MODE_CUBEMAP_ARRAY:
		target = TARGET_CUBEMAP_ARRAY;
		break;
	case MODE_3D:
		target = TARGET_TEXTURE_3D;
		break;
	}
}

ResourceImporterTexture::Mode ResourceImporterTexture::_get_layered_mode() const
{
	switch (target) {
	case TARGET_LAYERED_2D_ARRAY:
		return MODE_2D_ARRAY;
	case TARGET_CUBEMAP:
		return MODE_CUBEMAP;
	case TARGET_CUBEMAP_ARRAY:
		return MODE_CUBEMAP_ARRAY;
	case TARGET_TEXTURE_3D:
		return MODE_3D;
	default:
		return MODE_CUBEMAP;
	}
}

String ResourceImporterTexture::get_importer_name() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "texture";
	case TARGET_LAYERED_2D_ARRAY:
		return "2d_array_texture";
	case TARGET_CUBEMAP:
		return "cubemap_texture";
	case TARGET_CUBEMAP_ARRAY:
		return "cubemap_array_texture";
	case TARGET_TEXTURE_3D:
		return "3d_texture";
	case TARGET_TEXTURE_ATLAS:
		return "texture_atlas";
	case TARGET_IMAGE:
		return "image";
	case TARGET_BITMAP:
		return "bitmap";
	case TARGET_SVG:
		return "svg";
	}
	ERR_FAIL_V(String());
}

String ResourceImporterTexture::get_visible_name() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "Texture2D";
	case TARGET_LAYERED_2D_ARRAY:
		return "Texture2DArray";
	case TARGET_CUBEMAP:
		return "Cubemap";
	case TARGET_CUBEMAP_ARRAY:
		return "CubemapArray";
	case TARGET_TEXTURE_3D:
		return "Texture3D";
	case TARGET_TEXTURE_ATLAS:
		return "TextureAtlas";
	case TARGET_IMAGE:
		return "Image";
	case TARGET_BITMAP:
		return "BitMap";
	case TARGET_SVG:
		return "ImageTexture";
	}
	ERR_FAIL_V(String());
}

void ResourceImporterTexture::get_recognized_extensions(List<String>* p_extensions) const
{
	if (!p_extensions) {
		return;
	}
	if (target == TARGET_SVG) {
		p_extensions->push_back("svg");
		return;
	}
	ImageLoader::get_recognized_extensions(p_extensions);
}

String ResourceImporterTexture::get_save_extension() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "ctex";
	case TARGET_LAYERED_2D_ARRAY:
		return "ctexarray";
	case TARGET_CUBEMAP:
		return "ccube";
	case TARGET_CUBEMAP_ARRAY:
		return "ccubearray";
	case TARGET_TEXTURE_3D:
		return "ctex3d";
	case TARGET_TEXTURE_ATLAS:
		return "res";
	case TARGET_IMAGE:
		return "image";
	case TARGET_BITMAP:
		return "res";
	case TARGET_SVG:
		return "dpitex";
	}
	ERR_FAIL_V(String());
}

String ResourceImporterTexture::get_resource_type() const
{
	switch (target) {
	case TARGET_TEXTURE_2D:
		return "CompressedTexture2D";
	case TARGET_LAYERED_2D_ARRAY:
	case TARGET_CUBEMAP:
	case TARGET_CUBEMAP_ARRAY:
		return "CompressedTextureLayered";
	case TARGET_TEXTURE_3D:
		return "CompressedTexture3D";
	case TARGET_TEXTURE_ATLAS:
		return "Texture2D";
	case TARGET_IMAGE:
		return "Image";
	case TARGET_BITMAP:
		return "BitMap";
	case TARGET_SVG:
		return "ImageTexture";
	}
	ERR_FAIL_V(String());
}

int ResourceImporterTexture::get_preset_count() const
{
	if (target == TARGET_TEXTURE_2D) {
		return 3;
	}
	return 0;
}

String ResourceImporterTexture::get_preset_name(int p_idx) const
{
	if (target == TARGET_TEXTURE_2D && p_idx >= 0 && p_idx < 3) {
		static const char* preset_names[] = {
			TTRC("2D/3D (Auto-Detect)"),
			TTRC("2D"),
			TTRC("3D"),
		};

		return TTRGET(preset_names[p_idx]);
	}
	return String();
}

String ResourceImporterTexture::get_option_group_file() const
{
	if (target == TARGET_TEXTURE_ATLAS) {
		return "atlas_file";
	}
	return String();
}

void ResourceImporterTexture::_remap_channels(Ref<Image>& r_image, ChannelRemap p_options[4])
{
	ERR_FAIL_COND(r_image->is_compressed());

	// Currently HDR inverted remapping is not allowed.
	bool attempted_hdr_inverted = false;
	if (r_image->get_format() >= Image::FORMAT_RF &&
		r_image->get_format() <= Image::FORMAT_RGBE9995) {
		// Formats which can hold HDR data cannot be inverted the same way as unsigned normalized
		// ones (1.0 - channel).
		for (int i = 0; i < 4; i++) {
			switch (p_options[i]) {
			case REMAP_INV_R:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_R;
				break;
			case REMAP_INV_G:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_G;
				break;
			case REMAP_INV_B:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_B;
				break;
			case REMAP_INV_A:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_A;
				break;
			default:
				break;
			}
		}
	}

	if (attempted_hdr_inverted) {
		WARN_PRINT("Attempted to use an inverted channel remap on an HDR image. The remap has been "
				   "changed to its uninverted equivalent.");
	}

	// Optimization: Set the remap from 'unused' to either 0 or 1 to avoid repeated checks in the
	// conversion loop.
	for (int i = 0; i < 4; i++) {
		if (p_options[i] == REMAP_UNUSED) {
			p_options[i] = i == 3 ? REMAP_1 : REMAP_0;
		}
	}

	// Expand the image's channel count in the event that the current set of channels doesn't allow
	// for the desired remap.
	const Image::Format original_format = r_image->get_format();
	const uint32_t channel_mask = Image::get_format_component_mask(original_format);

	// Whether a channel is supported by the format itself.
	const bool has_channel_r = channel_mask & 0x1;
	const bool has_channel_g = channel_mask & 0x2;
	const bool has_channel_b = channel_mask & 0x4;
	const bool has_channel_a = channel_mask & 0x8;

	// Whether a certain channel needs to be remapped.
	const bool remap_r =
		p_options[0] != REMAP_R ? !(!has_channel_r && p_options[0] == REMAP_0) : false;
	const bool remap_g =
		p_options[1] != REMAP_G ? !(!has_channel_g && p_options[1] == REMAP_0) : false;
	const bool remap_b =
		p_options[2] != REMAP_B ? !(!has_channel_b && p_options[2] == REMAP_0) : false;
	const bool remap_a =
		p_options[3] != REMAP_A ? !(!has_channel_a && p_options[3] == REMAP_1) : false;

	if (!(remap_r || remap_g || remap_b || remap_a)) {
		// Default color map, do nothing.
		return;
	}

	// Whether a certain channel set is needed, either from the source or the remap.
	const bool needs_rg = remap_g || has_channel_g;
	const bool needs_rgb = remap_b || has_channel_b;
	const bool needs_rgba = remap_a || has_channel_a;

	bool could_not_expand = false;
	switch (original_format) {
	case Image::FORMAT_R8:
	case Image::FORMAT_RG8:
	case Image::FORMAT_RGB8: {
		// Convert to either RGBA8, RGB8 or RG8.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBA8);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGB8);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RG8);
		}
	} break;
	case Image::FORMAT_RH:
	case Image::FORMAT_RGH:
	case Image::FORMAT_RGBH: {
		// Convert to either RGBAH, RGBH or RGH.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBAH);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGBH);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RGH);
		}
	} break;
	case Image::FORMAT_RF:
	case Image::FORMAT_RGF:
	case Image::FORMAT_RGBF: {
		// Convert to either RGBAF, RGBF or RGF.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBAF);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGBF);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RGF);
		}
	} break;
	case Image::FORMAT_L8: {
		const bool uniform_rgb = (p_options[0] == p_options[1] && p_options[1] == p_options[2]) ||
								 !(remap_r || remap_g || remap_b);
		if (uniform_rgb) {
			// Uniform RGB.
			if (needs_rgba) {
				r_image->convert(Image::FORMAT_LA8);
			}
		}
		else {
			// Non-uniform RGB.
			if (needs_rgba) {
				r_image->convert(Image::FORMAT_RGBA8);
			}
			else {
				r_image->convert(Image::FORMAT_RGB8);
			}
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_LA8: {
		const bool uniform_rgb = (p_options[0] == p_options[1] && p_options[1] == p_options[2]) ||
								 !(remap_r || remap_g || remap_b);
		if (!uniform_rgb) {
			// Non-uniform RGB.
			r_image->convert(Image::FORMAT_RGBA8);
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_RGB565: {
		if (needs_rgba) {
			// RGB565 doesn't have an alpha expansion, convert to RGBA8.
			r_image->convert(Image::FORMAT_RGBA8);
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_RGBE9995: {
		if (needs_rgba) {
			// RGB9995 doesn't have an alpha expansion, convert to RGBAH.
			r_image->convert(Image::FORMAT_RGBAH);
			could_not_expand = true;
		}
	} break;

	default: {
	} break;
	}

	if (could_not_expand) {
		WARN_PRINT(vformat("Unable to expand image format %s's channels (the target format does "
						   "not exist), converting to %s as a fallback.",
			Image::get_format_name(original_format),
			Image::get_format_name(r_image->get_format())));
	}

	// Remap the channels.
	for (int x = 0; x < r_image->get_width(); x++) {
		for (int y = 0; y < r_image->get_height(); y++) {
			Color src = r_image->get_pixel(x, y);
			Color dst;

			for (int i = 0; i < 4; i++) {
				switch (p_options[i]) {
				case REMAP_R:
					dst[i] = src.r;
					break;
				case REMAP_G:
					dst[i] = src.g;
					break;
				case REMAP_B:
					dst[i] = src.b;
					break;
				case REMAP_A:
					dst[i] = src.a;
					break;

				case REMAP_INV_R:
					dst[i] = 1.0f - src.r;
					break;
				case REMAP_INV_G:
					dst[i] = 1.0f - src.g;
					break;
				case REMAP_INV_B:
					dst[i] = 1.0f - src.b;
					break;
				case REMAP_INV_A:
					dst[i] = 1.0f - src.a;
					break;

				case REMAP_0:
					dst[i] = 0.0f;
					break;
				case REMAP_1:
					dst[i] = 1.0f;
					break;

				default:
					break;
				}
			}

			r_image->set_pixel(x, y, dst);
		}
	}
}

void ResourceImporterTexture::_invert_y_channel(Ref<Image>& r_image)
{
	// Inverting the green channel can be used to flip a normal map's direction.
	// There's no standard when it comes to normal map Y direction, so this is
	// sometimes needed when using a normal map exported from another program.
	// See <http://wiki.polycount.com/wiki/Normal_Map_Technical_Details#Common_Swizzle_Coordinates>.
	const int height = r_image->get_height();
	const int width = r_image->get_width();

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			const Color color = r_image->get_pixel(i, j);
			r_image->set_pixel(i, j, Color(color.r, 1 - color.g, color.b, color.a));
		}
	}
}

void ResourceImporterTexture::_clamp_hdr_exposure(Ref<Image>& r_image)
{
	// Clamp HDR exposure following Filament's tonemapping formula.
	// This can be used to reduce fireflies in environment maps or reduce the influence
	// of the sun from an HDRI panorama on environment lighting (when a DirectionalLight3D is used
	// instead).
	const int height = r_image->get_height();
	const int width = r_image->get_width();

	// These values are chosen arbitrarily and seem to produce good results with 4,096 samples.
	const float linear = 4096.0;
	const float compressed = 16384.0;

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			const Color color = r_image->get_pixel(i, j);
			const float luma = color.get_luminance();

			Color clamped_color;
			if (luma <= linear) {
				clamped_color = color;
			}
			else {
				clamped_color =
					(color / luma) * ((linear * linear - compressed * luma) /
										 (2 * linear - compressed - luma));
			}

			r_image->set_pixel(i, j, clamped_color);
		}
	}
}

void ResourceImporterTexture::_save_tex(Vector<Ref<Image>> p_images, const String& p_to_path,
	int p_compress_mode, float p_lossy, const Image::BasisUniversalPackerParams& p_basisu_params,
	Image::CompressMode p_vram_compression, Image::CompressProfile p_vram_compression_profile,
	Image::CompressSource p_csource, Image::UsedChannels used_channels, bool p_mipmaps,
	bool p_force_po2, Image::BPTCFormat p_bptc_format)
{
	Vector<Ref<Image>> mipmap_images; // for 3D
	Mode layered_mode = _get_layered_mode();

	if (layered_mode == MODE_3D) {
		// 3D saves in its own way

		for (int i = 0; i < p_images.size(); i++) {
			if (p_images.write[i]->has_mipmaps()) {
				p_images.write[i]->clear_mipmaps();
			}

			if (p_force_po2) {
				p_images.write[i]->resize_to_po2();
			}
		}

		if (p_mipmaps) {
			Vector<Ref<Image>> parent_images = p_images;
			// create 3D mipmaps, this is horrible, though not used very often
			int w = p_images[0]->get_width();
			int h = p_images[0]->get_height();
			int d = p_images.size();

			while (w > 1 || h > 1 || d > 1) {
				Vector<Ref<Image>> mipmaps;
				int mm_w = MAX(1, w >> 1);
				int mm_h = MAX(1, h >> 1);
				int mm_d = MAX(1, d >> 1);

				for (int i = 0; i < mm_d; i++) {
					Ref<Image> mm =
						Image::create_empty(mm_w, mm_h, false, p_images[0]->get_format());
					Vector3 pos;
					pos.z = float(i) * float(d) / float(mm_d) + 0.5;
					for (int x = 0; x < mm_w; x++) {
						for (int y = 0; y < mm_h; y++) {
							pos.x = float(x) * float(w) / float(mm_w) + 0.5;
							pos.y = float(y) * float(h) / float(mm_h) + 0.5;

							Vector3i posi = Vector3i(pos);
							Vector3 fract = pos - Vector3(posi);
							Vector3i posi_n = posi;
							if (posi_n.x < w - 1) {
								posi_n.x++;
							}
							if (posi_n.y < h - 1) {
								posi_n.y++;
							}
							if (posi_n.z < d - 1) {
								posi_n.z++;
							}

							Color c000 = parent_images[posi.z]->get_pixel(posi.x, posi.y);
							Color c100 = parent_images[posi.z]->get_pixel(posi_n.x, posi.y);
							Color c010 = parent_images[posi.z]->get_pixel(posi.x, posi_n.y);
							Color c110 = parent_images[posi.z]->get_pixel(posi_n.x, posi_n.y);
							Color c001 = parent_images[posi_n.z]->get_pixel(posi.x, posi.y);
							Color c101 = parent_images[posi_n.z]->get_pixel(posi_n.x, posi.y);
							Color c011 = parent_images[posi_n.z]->get_pixel(posi.x, posi_n.y);
							Color c111 = parent_images[posi_n.z]->get_pixel(posi_n.x, posi_n.y);

							Color cx00 = c000.lerp(c100, fract.x);
							Color cx01 = c001.lerp(c101, fract.x);
							Color cx10 = c010.lerp(c110, fract.x);
							Color cx11 = c011.lerp(c111, fract.x);

							Color cy0 = cx00.lerp(cx10, fract.y);
							Color cy1 = cx01.lerp(cx11, fract.y);

							Color cz = cy0.lerp(cy1, fract.z);

							mm->set_pixel(x, y, cz);
						}
					}

					mipmaps.push_back(mm);
				}

				w = mm_w;
				h = mm_h;
				d = mm_d;

				mipmap_images.append_array(mipmaps);
				parent_images = mipmaps;
			}
		}
	}
	else {
		for (int i = 0; i < p_images.size(); i++) {
			if (p_force_po2) {
				p_images.write[i]->resize_to_po2();
			}

			if (p_mipmaps) {
				p_images.write[i]->generate_mipmaps(p_csource == Image::COMPRESS_SOURCE_NORMAL);
			}
			else {
				p_images.write[i]->clear_mipmaps();
			}
		}
	}

	Ref<FileAccess> f = FileAccess::open(p_to_path, FileAccess::WRITE);
	f->store_8('G');
	f->store_8('S');
	f->store_8('T');
	f->store_8('L');

	f->store_32(CompressedTextureLayered::FORMAT_VERSION);
	f->store_32(p_images.size()); // For 2d layers or 3d depth.
	f->store_32(layered_mode);
	f->store_32(0);

	f->store_32(0);
	f->store_32(mipmap_images.size()); // Adjust the amount of mipmaps.
	f->store_32(0);
	f->store_32(0);

	if ((p_compress_mode == COMPRESS_LOSSLESS || p_compress_mode == COMPRESS_LOSSY) &&
		p_images[0]->get_format() >= Image::FORMAT_RF) {
		p_compress_mode = COMPRESS_VRAM_UNCOMPRESSED; // These can't go as lossy.
	}

	for (int i = 0; i < p_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, p_images[i],
			ResourceImporterTexture::CompressMode(p_compress_mode), used_channels,
			p_vram_compression, p_vram_compression_profile, p_lossy, p_basisu_params,
			p_bptc_format);
	}

	for (int i = 0; i < mipmap_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, mipmap_images[i],
			ResourceImporterTexture::CompressMode(p_compress_mode), used_channels,
			p_vram_compression, p_vram_compression_profile, p_lossy, p_basisu_params,
			p_bptc_format);
	}
}

// FIXME: Rasterization has issues, see
// https://github.com/godotengine/godot/issues/68350#issuecomment-1305610290
[[maybe_unused]] static void _plot_triangle(Vector2i* p_vertices, const Vector2i& p_offset,
	bool p_transposed, Ref<Image> p_image, const Ref<Image>& p_src_image)
{
	int width = p_image->get_width();
	int height = p_image->get_height();
	int src_width = p_src_image->get_width();
	int src_height = p_src_image->get_height();

	int x[3];
	int y[3];

	for (int j = 0; j < 3; j++) {
		x[j] = p_vertices[j].x;
		y[j] = p_vertices[j].y;
	}

	// sort the points vertically
	if (y[1] > y[2]) {
		SWAP(x[1], x[2]);
		SWAP(y[1], y[2]);
	}
	if (y[0] > y[1]) {
		SWAP(x[0], x[1]);
		SWAP(y[0], y[1]);
	}
	if (y[1] > y[2]) {
		SWAP(x[1], x[2]);
		SWAP(y[1], y[2]);
	}

	double dx_far = double(x[2] - x[0]) / (y[2] - y[0] + 1);
	double dx_upper = double(x[1] - x[0]) / (y[1] - y[0] + 1);
	double dx_low = double(x[2] - x[1]) / (y[2] - y[1] + 1);
	double xf = x[0];
	double xt = x[0] + dx_upper; // if y[0] == y[1], special case
	int max_y = MIN(y[2], p_transposed ? (width - p_offset.x - 1) : (height - p_offset.y - 1));
	for (int yi = y[0]; yi < max_y; yi++) {
		if (yi >= 0) {
			for (int xi = (xf > 0 ? int(xf) : 0); xi < (xt <= src_width ? xt : src_width); xi++) {
				int px = xi, py = yi;
				int sx = px, sy = py;
				sx = CLAMP(sx, 0, src_width - 1);
				sy = CLAMP(sy, 0, src_height - 1);
				Color color = p_src_image->get_pixel(sx, sy);
				if (p_transposed) {
					SWAP(px, py);
				}
				px += p_offset.x;
				py += p_offset.y;

				// may have been cropped, so don't blit what is not visible?
				if (px < 0 || px >= width) {
					continue;
				}
				if (py < 0 || py >= height) {
					continue;
				}
				p_image->set_pixel(px, py, color);
			}

			for (int xi = (xf < src_width ? int(xf) : src_width - 1); xi >= (xt > 0 ? xt : 0);
				 xi--) {
				int px = xi, py = yi;
				int sx = px, sy = py;
				sx = CLAMP(sx, 0, src_width - 1);
				sy = CLAMP(sy, 0, src_height - 1);
				Color color = p_src_image->get_pixel(sx, sy);
				if (p_transposed) {
					SWAP(px, py);
				}
				px += p_offset.x;
				py += p_offset.y;

				// may have been cropped, so don't blit what is not visible?
				if (px < 0 || px >= width) {
					continue;
				}
				if (py < 0 || py >= height) {
					continue;
				}
				p_image->set_pixel(px, py, color);
			}
		}
		xf += dx_far;
		if (yi < y[1]) {
			xt += dx_upper;
		}
		else {
			xt += dx_low;
		}
	}
}

const char* ResourceImporterTexture::compression_formats[] = {"s3tc_bptc", "etc2_astc", nullptr};

ResourceImporterTexture::ResourceImporterTexture(bool p_singleton) :
	ResourceImporterTexture(TARGET_TEXTURE_2D, p_singleton)
{
}

ResourceImporterTexture::ResourceImporterTexture(ImportTarget p_target, bool p_singleton) :
	target(p_target)
{
	if (p_singleton) {
		singleton = this;
	}
	if (target == TARGET_TEXTURE_2D) {
		CompressedTexture2D::request_3d_callback = _texture_reimport_3d;
		CompressedTexture2D::request_roughness_callback = _texture_reimport_roughness;
		CompressedTexture2D::request_normal_callback = _texture_reimport_normal;
	}
}

ResourceImporterTexture::~ResourceImporterTexture()
{
	if (singleton == this) {
		singleton = nullptr;
	}
}

void ResourceImporterTexture::save_to_ctex_format(Ref<FileAccess> f, const Ref<Image>& p_image,
	CompressMode p_compress_mode, Image::UsedChannels p_channels,
	Image::CompressMode p_compress_format, Image::CompressProfile p_compress_profile,
	float p_lossy_quality, const Image::BasisUniversalPackerParams& p_basisu_params,
	Image::BPTCFormat p_bptc_format)
{
}

void ResourceImporterTexture::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
	(void)p_path;
	(void)r_options;
	(void)p_preset;
}

String ResourceImporterTexture::get_import_settings_string() const { return String(); }

// ============================================================================
// Merged from editor/import/resource_importer_wav.cpp
// ============================================================================

#include "core/io/resource_saver.h"
// --- Inlined from editor/import/resource_importer_wav.h ---
#include "core/io/resource_importer.h"

class ResourceImporterWAV : public ResourceImporter
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



// --- End editor/import/resource_importer_wav.h ---
#include "scene/resources/audio_stream_wav.h"

String ResourceImporterWAV::get_importer_name() const { return "wav"; }

String ResourceImporterWAV::get_visible_name() const { return "Microsoft WAV"; }

void ResourceImporterWAV::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("wav");
}

String ResourceImporterWAV::get_save_extension() const { return "sample"; }

String ResourceImporterWAV::get_resource_type() const { return "AudioStreamWAV"; }

int ResourceImporterWAV::get_preset_count() const { return 0; }

String ResourceImporterWAV::get_preset_name(int p_idx) const { return String(); }

void ResourceImporterWAV::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options, int p_preset) const
{
}


