/**************************************************************************/
/*  visual_instance_3d.h                                                  */
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

#include "scene/3d/node_3d.h"
#include "scene/resources/environment.h"
#include "scene/resources/material.h"
#include "servers/rendering/rendering_server_enums.h"

class TriangleMesh;

struct RenderInstance3D
{
	enum GetFacesFlags
	{
		FACES_SOLID = 1,
		FACES_ENCLOSING = 2,
		FACES_DYNAMIC = 4,
	};

	enum ShadowCastingSetting
	{
		SHADOW_CASTING_SETTING_OFF = RSE::SHADOW_CASTING_SETTING_OFF,
		SHADOW_CASTING_SETTING_ON = RSE::SHADOW_CASTING_SETTING_ON,
		SHADOW_CASTING_SETTING_DOUBLE_SIDED = RSE::SHADOW_CASTING_SETTING_DOUBLE_SIDED,
		SHADOW_CASTING_SETTING_SHADOWS_ONLY = RSE::SHADOW_CASTING_SETTING_SHADOWS_ONLY,
	};

	enum GIMode
	{
		GI_MODE_DISABLED,
		GI_MODE_STATIC,
		GI_MODE_DYNAMIC,
	};

	enum LightmapScale
	{
		LIGHTMAP_SCALE_1X,
		LIGHTMAP_SCALE_2X,
		LIGHTMAP_SCALE_4X,
		LIGHTMAP_SCALE_8X,
		LIGHTMAP_SCALE_MAX,
	};

	enum VisibilityRangeFadeMode
	{
		VISIBILITY_RANGE_FADE_DISABLED = RSE::VISIBILITY_RANGE_FADE_DISABLED,
		VISIBILITY_RANGE_FADE_SELF = RSE::VISIBILITY_RANGE_FADE_SELF,
		VISIBILITY_RANGE_FADE_DEPENDENCIES = RSE::VISIBILITY_RANGE_FADE_DEPENDENCIES,
	};

	RID base;
	RID instance;
	uint32_t layers = 1;
	float sorting_offset = 0.0f;
	bool sorting_use_aabb_center = true;

	ShadowCastingSetting shadow_casting_setting = SHADOW_CASTING_SETTING_ON;
	Ref<Material> material_override;
	Ref<Material> material_overlay;

	float visibility_range_begin = 0.0f;
	float visibility_range_end = 0.0f;
	float visibility_range_begin_margin = 0.0f;
	float visibility_range_end_margin = 0.0f;
	VisibilityRangeFadeMode visibility_range_fade_mode = VISIBILITY_RANGE_FADE_DISABLED;

	float transparency = 0.0f;
	float lod_bias = 1.0f;
	mutable HashMap<String, String> instance_shader_parameter_property_remap;

	float extra_cull_margin = 0.0f;
	AABB custom_aabb;
	float lightmap_texel_scale = 1.0f;
	GIMode gi_mode = GI_MODE_STATIC;
	bool ignore_occlusion_culling = false;

	void update_visibility(Node3D* p_owner);
	void set_instance_use_identity_transform(Node3D* p_owner, bool p_enable);
	void fti_update_servers_xform(Node3D* p_owner);
	void notification(Node3D* p_owner, int p_what);

	RID get_instance() const;

	void set_base(const RID& p_base);
	RID get_base() const;

	void set_layer_mask(uint32_t p_mask);
	uint32_t get_layer_mask() const;

	void set_layer_mask_value(int p_layer_number, bool p_enable);
	bool get_layer_mask_value(int p_layer_number) const;

	void set_sorting_offset(float p_offset);
	float get_sorting_offset() const;

	void set_sorting_use_aabb_center(bool p_enabled);
	bool is_sorting_use_aabb_center() const;

	void set_cast_shadows_setting(ShadowCastingSetting p_shadow_casting_setting);
	ShadowCastingSetting get_cast_shadows_setting() const;

	float get_transparency() const;
	float get_visibility_range_begin() const;
	float get_visibility_range_end() const;
	float get_visibility_range_begin_margin() const;
	float get_visibility_range_end_margin() const;
	VisibilityRangeFadeMode get_visibility_range_fade_mode() const;

	void set_material_override(const Ref<Material>& p_material);
	Ref<Material> get_material_override() const;

	void set_material_overlay(const Ref<Material>& p_material);
	Ref<Material> get_material_overlay() const;

	void set_extra_cull_margin(float p_margin);
	float get_extra_cull_margin() const;

	void set_lod_bias(float p_bias);
	float get_lod_bias() const;

	void set_gi_mode(GIMode p_mode);
	GIMode get_gi_mode() const;

	void set_lightmap_texel_scale(float p_scale);
	float get_lightmap_texel_scale() const;

	AABB get_custom_aabb() const;

	void set_ignore_occlusion_culling(bool p_enabled);
	bool is_ignoring_occlusion_culling() const;

	const String* _instance_uniform_get_remap(const String& p_name) const;
	void append_geometry_warnings(PackedStringArray& r_warnings) const;

	RenderInstance3D() = default;
	~RenderInstance3D();
};

// --- Folded from scene/3d/decal.h ---


class Decal : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DecalTexture
	{
		TEXTURE_ALBEDO,
		TEXTURE_NORMAL,
		TEXTURE_ORM,
		TEXTURE_EMISSION,
		TEXTURE_MAX
	};

private:
	RID decal;
	Vector3 size = Vector3(2, 2, 2);
	Ref<Texture2D> textures[TEXTURE_MAX];
	real_t emission_energy = 1.0;
	real_t albedo_mix = 1.0;
	Color modulate = Color(1, 1, 1, 1);
	uint32_t cull_mask = (1 << 20) - 1;
	real_t normal_fade = 0.0;
	real_t upper_fade = 0.3;
	real_t lower_fade = 0.3;
	bool distance_fade_enabled = false;
	real_t distance_fade_begin = 40.0;
	real_t distance_fade_length = 10.0;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	virtual PackedStringArray get_configuration_warnings() const override;

	Vector3 get_size() const;

	void set_texture(DecalTexture p_type, const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture(DecalTexture p_type) const;

	void set_emission_energy(real_t p_energy);
	real_t get_emission_energy() const;

	void set_albedo_mix(real_t p_mix);
	real_t get_albedo_mix() const;

	void set_modulate(Color p_modulate);
	Color get_modulate() const;

	void set_upper_fade(real_t p_energy);
	real_t get_upper_fade() const;

	void set_lower_fade(real_t p_fade);
	real_t get_lower_fade() const;

	void set_normal_fade(real_t p_fade);
	real_t get_normal_fade() const;

	void set_enable_distance_fade(bool p_enable);
	bool is_distance_fade_enabled() const;

	void set_distance_fade_begin(real_t p_distance);
	real_t get_distance_fade_begin() const;

	void set_distance_fade_length(real_t p_length);
	real_t get_distance_fade_length() const;

	uint32_t get_cull_mask() const;

	AABB get_aabb() const;

	Decal();
	~Decal();
};



// --- End scene/3d/decal.h ---

// --- Folded from scene/3d/label_3d.h ---

#include "scene/resources/font.h"
#include "scene/resources/material.h"
#include "servers/text/text_server.h"

class Label3D : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DrawFlags
	{
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX
	};

	enum AlphaCutMode
	{
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS,
		ALPHA_CUT_HASH,
		ALPHA_CUT_MAX
	};

private:
	real_t pixel_size = 0.005;
	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	float alpha_scissor_threshold = 0.5;
	float alpha_hash_scale = 1.0;
	StandardMaterial3D::AlphaAntiAliasing alpha_antialiasing_mode =
		StandardMaterial3D::ALPHA_ANTIALIASING_OFF;
	float alpha_antialiasing_edge = 0.0f;

	AABB aabb;

	mutable Ref<TriangleMesh> triangle_mesh;
	RID mesh;

	struct SurfaceData
	{
		PackedVector3Array mesh_vertices;
		PackedVector3Array mesh_normals;
		PackedFloat32Array mesh_tangents;
		PackedColorArray mesh_colors;
		PackedVector2Array mesh_uvs;
		PackedInt32Array indices;
		int offset = 0;
		float z_shift = 0.0;
		RID material;
	};

	struct SurfaceKey
	{
		uint64_t texture_id;
		int32_t priority;
		int32_t outline_size;

		bool operator==(const SurfaceKey& p_b) const
		{
			return (texture_id == p_b.texture_id) && (priority == p_b.priority) &&
				   (outline_size == p_b.outline_size);
		}

		SurfaceKey(uint64_t p_texture_id, int p_priority, int p_outline_size)
		{
			texture_id = p_texture_id;
			priority = p_priority;
			outline_size = p_outline_size;
		}
	};

	struct SurfaceKeyHasher
	{
		_FORCE_INLINE_ static uint32_t hash(const SurfaceKey& p_a)
		{
			return hash_murmur3_buffer(&p_a, sizeof(SurfaceKey));
		}
	};

	HashMap<SurfaceKey, SurfaceData, SurfaceKeyHasher> surfaces;

	HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER;
	VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_CENTER;
	String text;
	String xl_text;
	bool uppercase = false;

	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
	uint32_t autowrap_flags_trim =
		TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	uint32_t jst_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
						 TextServer::JUSTIFICATION_SKIP_LAST_LINE |
						 TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	float width = 500.0;

	int font_size = 32;
	Ref<Font> font_override;
	mutable Ref<Font> theme_font;
	Color modulate = Color(1, 1, 1, 1);
	Point2 lbl_offset;
	int outline_render_priority = -1;
	int render_priority = 0;

	int outline_size = 12;
	Color outline_modulate = Color(0, 0, 0, 1);

	float line_spacing = 0.f;

	String language;
	TextServer::Direction text_direction = TextServer::DIRECTION_AUTO;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;

	RID text_rid;
	Vector<RID> lines_rid;

	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter =
		StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;

	bool pending_update = false;

	bool dirty_lines = true;
	bool dirty_font = true;
	bool dirty_text = true;

	void _generate_glyph_surfaces(const Glyph& p_glyph, Vector2& r_offset, const Color& p_modulate,
		int p_priority = 0, int p_outline_size = 0);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;
	void _font_changed();
	void _queue_update();

	void _shape();

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }
	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	void set_outline_render_priority(int p_priority);
	int get_outline_render_priority() const;

	void set_text(const String& p_string);
	String get_text() const;

	void set_text_direction(TextServer::Direction p_text_direction);
	TextServer::Direction get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_font(const Ref<Font>& p_font);
	Ref<Font> get_font() const;
	Ref<Font> _get_font_or_default() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_outline_size(int p_size);
	int get_outline_size() const;

	void set_line_spacing(float p_size);
	float get_line_spacing() const;

	void set_modulate(const Color& p_color);
	Color get_modulate() const;

	void set_outline_modulate(const Color& p_color);
	Color get_outline_modulate() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_autowrap_trim_flags(uint32_t p_flags);
	uint32_t get_autowrap_trim_flags() const;

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_width(float p_width);
	float get_width() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_alpha_hash_scale(float p_hash_scale);
	float get_alpha_hash_scale() const;

	void set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa);
	StandardMaterial3D::AlphaAntiAliasing get_alpha_antialiasing() const;

	void set_alpha_antialiasing_edge(float p_edge);
	float get_alpha_antialiasing_edge() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	AABB get_aabb() const;
	Ref<TriangleMesh> generate_triangle_mesh() const;

	Label3D();
	~Label3D();
};



// --- End scene/3d/label_3d.h ---

// --- Folded from scene/3d/occluder_instance_3d.h ---


class ArrayMesh;

class Occluder3D : public Resource
{
public:
	enum OccluderType
	{
		OCCLUDER_ARRAY,
		OCCLUDER_QUAD,
		OCCLUDER_BOX,
		OCCLUDER_SPHERE,
		OCCLUDER_POLYGON,
	};

private:
	static constexpr int RINGS = 7;
	static constexpr int RADIAL_SEGMENTS = 7;

	OccluderType occluder_type = OCCLUDER_ARRAY;

	RID occluder;
	PackedVector3Array vertices;
	PackedInt32Array indices;
	AABB aabb;

	PackedVector3Array array_vertices;
	PackedInt32Array array_indices;
	Size2 quad_size = Vector2(1.0f, 1.0f);
	Vector3 box_size = Vector3(1.0f, 1.0f, 1.0f);
	float radius = 1.0f;
	Vector<Vector2> polygon;

	mutable Ref<ArrayMesh> debug_mesh;
	mutable Vector<Vector3> debug_lines;

	bool _has_editable_3d_polygon_no_depth() const;

protected:
	void _update();
	void _update_arrays(PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

public:
	void set_occluder_type(OccluderType p_type);
	OccluderType get_occluder_type() const;

	void set_arrays(PackedVector3Array p_vertices, PackedInt32Array p_indices);
	void set_vertices(PackedVector3Array p_vertices);
	void set_indices(PackedInt32Array p_indices);

	PackedVector3Array get_vertices() const;
	PackedInt32Array get_indices() const;

	void set_quad_size(const Size2& p_size);
	Size2 get_quad_size() const;

	void set_box_size(const Vector3& p_size);
	Vector3 get_box_size() const;

	void set_radius(float p_radius);
	float get_radius() const;

	void set_polygon(const Vector<Vector2>& p_polygon);
	Vector<Vector2> get_polygon() const;

	Vector<Vector3> get_debug_lines() const;
	Ref<ArrayMesh> get_debug_mesh() const;
	AABB get_aabb() const;

	virtual RID get_rid() const override;
	Occluder3D(OccluderType p_type = OCCLUDER_ARRAY);
	virtual ~Occluder3D();
};

class OccluderInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Ref<Occluder3D> occluder;
	uint32_t bake_mask = 0xFFFFFFFF;
	float bake_simplification_dist = 0.1f;

	void _bake_node(Node* p_node, PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

	bool _is_editable_3d_polygon() const;
	Ref<Resource> _get_editable_3d_polygon_resource() const;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	virtual PackedStringArray get_configuration_warnings() const override;

	enum BakeError
	{
		BAKE_ERROR_OK,
		BAKE_ERROR_NO_SAVE_PATH,
		BAKE_ERROR_NO_MESHES,
		BAKE_ERROR_CANT_SAVE,
	};

	void set_occluder(const Ref<Occluder3D>& p_occluder);
	Ref<Occluder3D> get_occluder() const;

	AABB get_aabb() const;

	uint32_t get_bake_mask() const;

	void set_bake_simplification_distance(float p_dist);
	float get_bake_simplification_distance() const;

	bool get_bake_mask_value(int p_layer_number) const;

	BakeError bake_scene(Node* p_from_node, String p_occluder_path = "");
	static void bake_single_node(const Node3D* p_node, float p_simplification_distance,
		PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

	OccluderInstance3D();
	~OccluderInstance3D();
};



// --- End scene/3d/occluder_instance_3d.h ---

// --- Folded from scene/3d/sprite_3d.h ---

#include "scene/resources/material.h"
#include "scene/resources/sprite_frames.h"

class Sprite3D : public Node3D
{
public:
	enum DrawFlags
	{
		FLAG_TRANSPARENT,
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX
	};

	enum AlphaCutMode
	{
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS,
		ALPHA_CUT_HASH,
		ALPHA_CUT_MAX
	};

	RenderInstance3D render_instance;

private:
	mutable Ref<TriangleMesh> triangle_mesh;

	bool color_dirty = true;
	Color color_accum;

	Sprite3D* parent_sprite = nullptr;
	List<Sprite3D*> children;
	List<Sprite3D*>::Element* pI = nullptr;

	bool centered = true;
	Point2 offset;

	bool hflip = false;
	bool vflip = false;

	Color modulate = Color(1, 1, 1, 1);
	int render_priority = 0;

	Vector3::Axis axis = Vector3::AXIS_Z;
	real_t pixel_size = 0.01;
	AABB aabb;

	RID mesh;
	RID material;

	RID last_shader;
	RID last_texture;

	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	float alpha_scissor_threshold = 0.5;
	float alpha_hash_scale = 1.0;
	StandardMaterial3D::AlphaAntiAliasing alpha_antialiasing_mode =
		StandardMaterial3D::ALPHA_ANTIALIASING_OFF;
	float alpha_antialiasing_edge = 0.0f;
	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter =
		StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;
	bool redraw_needed = false;
	bool pending_update = false;

	uint32_t mesh_surface_offsets[RSE::ARRAY_MAX];
	PackedByteArray vertex_buffer;
	PackedByteArray attribute_buffer;
	uint32_t vertex_stride = 0;
	uint32_t normal_tangent_stride = 0;
	uint32_t attrib_stride = 0;
	uint32_t skin_stride = 0;
	uint32_t mesh_surface_format = 0;

	// Single-texture sprite state
	Ref<Texture2D> texture;
	bool region = false;
	Rect2 region_rect;
	int frame = 0;
	int vframes = 1;
	int hframes = 1;

	// Optional SpriteFrames animation state (used when frames.is_valid())
	Ref<SpriteFrames> frames;
	String autoplay;
	bool playing = false;
	String animation = SceneString(default_);
	float speed_scale = 1.0;
	float custom_speed_scale = 1.0;
	real_t frame_speed_scale = 1.0;
	real_t frame_progress = 0.0;

	void _im_update();
	void _propagate_color_changed();
	Color _get_color_accum();
	void _draw();
	void draw_texture_rect(Ref<Texture2D> p_texture, Rect2 p_dst_rect, Rect2 p_src_rect);
	void _queue_redraw();

	double _get_frame_duration();
	void _calc_frame_speed_scale();
	void _stop_internal(bool p_reset);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	void set_modulate(const Color& p_color);
	Color get_modulate() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_axis(Vector3::Axis p_axis);
	Vector3::Axis get_axis() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_alpha_hash_scale(float p_hash_scale);
	float get_alpha_hash_scale() const;

	void set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa);
	StandardMaterial3D::AlphaAntiAliasing get_alpha_antialiasing() const;

	void set_alpha_antialiasing_edge(float p_edge);
	float get_alpha_antialiasing_edge() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	// Static/sheet texture API
	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_region_enabled(bool p_region);
	bool is_region_enabled() const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_frame_coords(const Vector2i& p_coord);
	Vector2i get_frame_coords() const;

	void set_vframes(int p_amount);
	int get_vframes() const;

	void set_hframes(int p_amount);
	int get_hframes() const;

	// SpriteFrames animation API
	void set_sprite_frames(const Ref<SpriteFrames>& p_frames);
	Ref<SpriteFrames> get_sprite_frames() const;

	void play(const String& p_name = String(), float p_custom_scale = 1.0f, bool p_from_end = false);
	void play_backwards(const String& p_name = String());
	void pause();
	void stop();

	bool is_playing() const;

	void set_animation(const String& p_name);
	String get_animation() const;

	void set_autoplay(const String& p_name);
	String get_autoplay() const;

	void set_frame_progress(real_t p_progress);
	real_t get_frame_progress() const;

	void set_speed_scale(float p_speed_scale);
	float get_speed_scale() const;
	float get_playing_speed() const;

	Rect2 get_item_rect() const;
	AABB get_aabb() const;
	Ref<TriangleMesh> generate_triangle_mesh() const;

	virtual PackedStringArray get_configuration_warnings() const override;

#ifdef TOOLS_ENABLED
	void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	Sprite3D();
	~Sprite3D();
};

// --- End scene/3d/sprite_3d.h ---
