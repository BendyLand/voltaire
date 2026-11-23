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
// --- Folded from scene/resources/sprite_frames.h ---
#include "scene/resources/texture.h"

static const float SPRITE_FRAME_MINIMUM_DURATION = 0.01;

class SpriteFrames : public Resource
{
public:
	enum LoopMode
	{
		LOOP_NONE,
		LOOP_LINEAR,
		LOOP_PINGPONG,
	};

private:
	struct Frame
	{
		Ref<Texture2D> texture;
		float duration = 1.0;
	};

	struct Anim
	{
		double speed = 5.0;
		LoopMode loop = LoopMode::LOOP_LINEAR;
		Vector<Frame> frames;
	};

	HashMap<String, Anim> animations;

protected:

public:
	void add_animation(const String& p_anim);
	bool has_animation(const String& p_anim) const;
	void duplicate_animation(const String& p_from, const String& p_to);
	void remove_animation(const String& p_anim);
	void rename_animation(const String& p_prev, const String& p_next);

	void get_animation_list(List<String>* r_animations) const;
	Vector<String> get_animation_names() const;

	void set_animation_speed(const String& p_anim, double p_fps);
	double get_animation_speed(const String& p_anim) const;

#ifndef DISABLE_DEPRECATED
	void set_animation_loop(const String& p_anim, bool p_loop);
	bool get_animation_loop(const String& p_anim) const;
#endif

	void set_animation_loop_mode(const String& p_anim, LoopMode p_loop_mode);
	LoopMode get_animation_loop_mode(const String& p_anim) const;

	void add_frame(const String& p_anim, const Ref<Texture2D>& p_texture,
		float p_duration = 1.0, int p_at_pos = -1);
	void set_frame(const String& p_anim, int p_idx, const Ref<Texture2D>& p_texture,
		float p_duration = 1.0);
	void remove_frame(const String& p_anim, int p_idx);

	int get_frame_count(const String& p_anim) const;

	_FORCE_INLINE_ Ref<Texture2D> get_frame_texture(const String& p_anim, int p_idx) const
	{
		HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
		ERR_FAIL_COND_V_MSG(
			!E, Ref<Texture2D>(), "Animation '" + String(p_anim) + "' doesn't exist.");
		ERR_FAIL_COND_V(p_idx < 0, Ref<Texture2D>());
		if (p_idx >= E->value.frames.size()) {
			return Ref<Texture2D>();
		}

		return E->value.frames[p_idx].texture;
	}

	_FORCE_INLINE_ float get_frame_duration(const String& p_anim, int p_idx) const
	{
		HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
		ERR_FAIL_COND_V_MSG(!E, 1.0, "Animation '" + String(p_anim) + "' doesn't exist.");
		ERR_FAIL_COND_V(p_idx < 0, 1.0);
		if (p_idx >= E->value.frames.size()) {
			return 1.0;
		}

		return E->value.frames[p_idx].duration;
	}

	void clear(const String& p_anim);
	void clear_all();

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	SpriteFrames();
};



// --- End scene/resources/sprite_frames.h ---

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

// --- Folded from scene/3d/light_3d.h ---


class Light3D : public Node3D
{
public:
	enum Param
	{
		PARAM_ENERGY = RSE::LIGHT_PARAM_ENERGY,
		PARAM_INDIRECT_ENERGY = RSE::LIGHT_PARAM_INDIRECT_ENERGY,
		PARAM_VOLUMETRIC_FOG_ENERGY = RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY,
		PARAM_SPECULAR = RSE::LIGHT_PARAM_SPECULAR,
		PARAM_RANGE = RSE::LIGHT_PARAM_RANGE,
		PARAM_SIZE = RSE::LIGHT_PARAM_SIZE,
		PARAM_ATTENUATION = RSE::LIGHT_PARAM_ATTENUATION,
		PARAM_SPOT_ANGLE = RSE::LIGHT_PARAM_SPOT_ANGLE,
		PARAM_SPOT_ATTENUATION = RSE::LIGHT_PARAM_SPOT_ATTENUATION,
		PARAM_SHADOW_MAX_DISTANCE = RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE,
		PARAM_SHADOW_SPLIT_1_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET,
		PARAM_SHADOW_SPLIT_2_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET,
		PARAM_SHADOW_SPLIT_3_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET,
		PARAM_SHADOW_FADE_START = RSE::LIGHT_PARAM_SHADOW_FADE_START,
		PARAM_SHADOW_NORMAL_BIAS = RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS,
		PARAM_SHADOW_BIAS = RSE::LIGHT_PARAM_SHADOW_BIAS,
		PARAM_SHADOW_PANCAKE_SIZE = RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE,
		PARAM_SHADOW_OPACITY = RSE::LIGHT_PARAM_SHADOW_OPACITY,
		PARAM_SHADOW_BLUR = RSE::LIGHT_PARAM_SHADOW_BLUR,
		PARAM_TRANSMITTANCE_BIAS = RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS,
		PARAM_INTENSITY = RSE::LIGHT_PARAM_INTENSITY,
		PARAM_MAX = RSE::LIGHT_PARAM_MAX
	};

	enum BakeMode
	{
		BAKE_DISABLED,
		BAKE_STATIC,
		BAKE_DYNAMIC,
	};

	enum DirectionalShadowMode
	{
		DIRECTIONAL_SHADOW_ORTHOGONAL,
		DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS,
		DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS,
	};

	enum SkyMode
	{
		SKY_MODE_LIGHT_AND_SKY,
		SKY_MODE_LIGHT_ONLY,
		SKY_MODE_SKY_ONLY,
	};

	enum OmniShadowMode
	{
		OMNI_SHADOW_DUAL_PARABOLOID,
		OMNI_SHADOW_CUBE,
	};

	RenderInstance3D render_instance;

private:
	RSE::LightType type = RSE::LIGHT_DIRECTIONAL;
	RID light;

	Color color = Color(1, 1, 1);
	real_t param[PARAM_MAX] = {};
	bool shadow = false;
	bool negative = false;
	bool reverse_cull = false;
	uint32_t cull_mask = 0xFFFFFFFF;
	uint32_t shadow_caster_mask = 0xFFFFFFFF;
	bool distance_fade_enabled = false;
	real_t distance_fade_begin = 40.0;
	real_t distance_fade_shadow = 50.0;
	real_t distance_fade_length = 10.0;
	bool editor_only = false;
	BakeMode bake_mode = BAKE_DYNAMIC;
	Ref<Texture2D> projector;
	Color correlated_color = Color(1.0, 1.0, 1.0);
	float temperature = 6500.0;

	// Directional-specific state
	bool blend_splits = false;
	DirectionalShadowMode directional_shadow_mode = DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS;
	SkyMode sky_mode = SKY_MODE_LIGHT_AND_SKY;

	// Omni-specific state
	OmniShadowMode omni_shadow_mode = OMNI_SHADOW_CUBE;

	// Area-specific state
	Vector2 area_size = Vector2(1.0, 1.0);
	Ref<Texture2D> area_texture;
	bool area_normalize_energy = true;

	void _create_light_rid();
	void _free_light_rid();
	void _update_visibility();

	virtual void owner_changed_notify() override;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_light_type(RSE::LightType p_type);
	RSE::LightType get_light_type() const { return type; }

	void set_editor_only(bool p_editor_only);
	bool is_editor_only() const;

	void set_param(Param p_param, real_t p_value);
	real_t get_param(Param p_param) const;

	void set_shadow(bool p_enable);
	bool has_shadow() const;

	void set_negative(bool p_enable);
	bool is_negative() const;

	void set_enable_distance_fade(bool p_enable);
	bool is_distance_fade_enabled() const;

	void set_distance_fade_begin(real_t p_distance);
	real_t get_distance_fade_begin() const;

	void set_distance_fade_shadow(real_t p_distance);
	real_t get_distance_fade_shadow() const;

	void set_distance_fade_length(real_t p_length);
	real_t get_distance_fade_length() const;

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_shadow_reverse_cull_face(bool p_enable);
	bool get_shadow_reverse_cull_face() const;

	void set_shadow_caster_mask(uint32_t p_caster_mask);
	uint32_t get_shadow_caster_mask() const;

	void set_bake_mode(BakeMode p_mode);
	BakeMode get_bake_mode() const;

	void set_projector(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_projector() const;

	float get_temperature() const;
	Color get_correlated_color() const;

	// Directional light parameters
	void set_directional_shadow_mode(DirectionalShadowMode p_mode);
	DirectionalShadowMode get_directional_shadow_mode() const { return directional_shadow_mode; }

	void set_blend_splits(bool p_enable);
	bool is_blend_splits_enabled() const { return blend_splits; }

	void set_sky_mode(SkyMode p_mode);
	SkyMode get_sky_mode() const { return sky_mode; }

	// Omni light parameters
	void set_omni_shadow_mode(OmniShadowMode p_mode);
	OmniShadowMode get_omni_shadow_mode() const { return omni_shadow_mode; }

	// Area light parameters
	void set_area_size(const Vector2& p_size);
	Vector2 get_area_size() const { return area_size; }

	void set_area_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_area_texture() const { return area_texture; }

	void set_area_normalize_energy(bool p_enable);
	bool is_area_normalizing_energy() const { return area_normalize_energy; }

	AABB get_aabb() const;
	virtual PackedStringArray get_configuration_warnings() const override;

	Light3D(RSE::LightType p_type = RSE::LIGHT_DIRECTIONAL);
	~Light3D();
};

// --- End scene/3d/light_3d.h ---

// --- Folded from scene/3d/lightmapper.h ---
#include "core/io/image.h"
#include "core/templates/hash_set.h"
#include "core/types.h"

class Image;

class LightmapDenoiser : public RefCounted
{
protected:
	static Ref<LightmapDenoiser> (*create_function)();

public:
	virtual Ref<Image> denoise_image(const Ref<Image>& p_image) = 0;
	static Ref<LightmapDenoiser> create();
};

class LightmapRaycaster : public RefCounted
{
public:
	struct EmbreeState;

private:
	EmbreeState* embree_state = nullptr;

public:
	// Compatible with embree4 rays.
	struct alignas(16) Ray
	{
		const static unsigned int INVALID_GEOMETRY_ID = ((unsigned int)-1); // from rtcore_common.h

		/*! Default construction does nothing. */
		_FORCE_INLINE_ Ray() : geomID(INVALID_GEOMETRY_ID) {}

		/*! Constructs a ray from origin, direction, and ray segment. Near
		 *  has to be smaller than far. */
		_FORCE_INLINE_ Ray(const Vector3& p_org, const Vector3& p_dir, float p_tnear = 0.0f,
			float p_tfar = Math::INF)
			: org(p_org), tnear(p_tnear), dir(p_dir), time(0.0f), tfar(p_tfar), mask(-1), u(0.0),
			  v(0.0), primID(INVALID_GEOMETRY_ID), geomID(INVALID_GEOMETRY_ID),
			  instID(INVALID_GEOMETRY_ID)
		{
		}

		/*! Tests if we hit something. */
		_FORCE_INLINE_ explicit operator bool() const { return geomID != INVALID_GEOMETRY_ID; }

	public:
		Vector3 org;		//!< Ray origin + tnear
		float tnear;		//!< Start of ray segment
		Vector3 dir;		//!< Ray direction + tfar
		float time;			//!< Time of this ray for motion blur.
		float tfar;			//!< End of ray segment
		unsigned int mask;	//!< used to mask out objects during traversal
		unsigned int id;	//!< ray ID
		unsigned int flags; //!< ray flags

		Vector3 normal;		 //!< Not normalized geometry normal
		float u;			 //!< Barycentric u coordinate of hit
		float v;			 //!< Barycentric v coordinate of hit
		unsigned int primID; //!< primitive ID
		unsigned int geomID; //!< geometry ID
		unsigned int instID; //!< instance ID
	};

	bool intersect(Ray& p_ray);
	void intersect(Vector<Ray>& r_rays);

	void add_mesh(const Vector<Vector3>& p_vertices, const Vector<Vector3>& p_normals,
		const Vector<Vector2>& p_uv2s, unsigned int p_id);
	void set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id);
	void commit();

	void set_mesh_filter(const HashSet<int>& p_mesh_ids);
	void clear_mesh_filter();

	static Ref<LightmapRaycaster> create();

	LightmapRaycaster();
	~LightmapRaycaster();
};

class Lightmapper : public RefCounted
{
public:
	struct RDState;

private:
	RDState* rd_state = nullptr;

public:
	enum GenerateProbes
	{
		GENERATE_PROBES_DISABLED,
		GENERATE_PROBES_SUBDIV_4,
		GENERATE_PROBES_SUBDIV_8,
		GENERATE_PROBES_SUBDIV_16,
		GENERATE_PROBES_SUBDIV_32,
	};

	enum LightType
	{
		LIGHT_TYPE_DIRECTIONAL,
		LIGHT_TYPE_OMNI,
		LIGHT_TYPE_SPOT,
		LIGHT_TYPE_AREA,
	};

	enum BakeError
	{
		BAKE_OK,
		BAKE_ERROR_TEXTURE_EXCEEDS_MAX_SIZE,
		BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES,
		BAKE_ERROR_ATLAS_TOO_SMALL,
		BAKE_ERROR_USER_ABORTED,
	};

	enum BakeQuality
	{
		BAKE_QUALITY_LOW,
		BAKE_QUALITY_MEDIUM,
		BAKE_QUALITY_HIGH,
		BAKE_QUALITY_ULTRA,
	};

	typedef bool (*BakeStepFunc)(
		float, const String&, void*, bool); // step index, step total, step description, userdata

	struct MeshData
	{
		// triangle data
		Vector<Vector3> points;
		Vector<Vector2> uv2;
		Vector<Vector3> normal;
		Vector<RID> material;
		Ref<Image> albedo_on_uv2;
		Ref<Image> emission_on_uv2;
	};

	void add_mesh(const MeshData& p_mesh);
	void add_directional_light(const String& p_name, bool p_static, const Vector3& p_direction,
		const Color& p_color, float p_energy, float p_indirect_energy, float p_angular_distance,
		float p_shadow_blur);
	void add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
		float p_attenuation, float p_size, float p_shadow_blur);
	void add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
		float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation,
		float p_size, float p_shadow_blur);
	void add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
		float p_range, float p_attenuation, const Vector3& p_area_width,
		const Vector3& p_area_height, float p_size, float p_shadow_blur,
		const Rect2& p_texture_rect, float p_max_mipmap);
	void add_area_light_atlas(
		const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data);
	void add_probe(const Vector3& p_position);
	BakeError bake(BakeQuality p_quality, bool p_use_denoiser, float p_denoiser_strength,
		int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy, float p_bias,
		int p_max_texture_size, bool p_bake_sh, bool p_bake_shadowmask, bool p_texture_for_bounces,
		GenerateProbes p_generate_probes, const Ref<Image>& p_environment_panorama,
		const Basis& p_environment_transform, BakeStepFunc p_step_function = nullptr,
		void* p_step_userdata = nullptr, float p_exposure_normalization = 1.0,
		float p_supersampling_factor = 1.0);

	int get_bake_texture_count() const;
	Ref<Image> get_bake_texture(int p_index) const;
	int get_shadowmask_texture_count() const;
	Ref<Image> get_shadowmask_texture(int p_index) const;
	int get_bake_mesh_count() const;
	Rect2 get_bake_mesh_uv_scale(int p_index) const;
	int get_bake_mesh_texture_slice(int p_index) const;

	int get_bake_probe_count() const;
	Vector3 get_bake_probe_point(int p_probe) const;
	Vector<Color> get_bake_probe_sh(int p_probe) const;

	static Ref<Lightmapper> create();

	Lightmapper();
	~Lightmapper();
};

// --- End scene/3d/lightmapper.h ---

// --- Folded from scene/3d/multimesh_instance_3d.h ---

// --- Inlined/Folded from scene/resources/multimesh.h ---
#include "scene/resources/mesh.h"
#include "servers/rendering/rendering_server_enums.h"

class MultiMesh : public Resource
{
public:
	enum TransformFormat
	{
		TRANSFORM_2D = RSE::MULTIMESH_TRANSFORM_2D,
		TRANSFORM_3D = RSE::MULTIMESH_TRANSFORM_3D
	};

	enum PhysicsInterpolationQuality
	{
		INTERP_QUALITY_FAST,
		INTERP_QUALITY_HIGH,
	};

private:
	Ref<Mesh> mesh;
	RID multimesh;
	TransformFormat transform_format = TRANSFORM_2D;
	AABB custom_aabb;
	bool use_colors = false;
	bool use_custom_data = false;
	int instance_count = 0;
	int visible_instance_count = -1;
	PhysicsInterpolationQuality _physics_interpolation_quality = INTERP_QUALITY_FAST;

protected:

#ifndef DISABLE_DEPRECATED
	// Kept for compatibility from 3.x to 4.0.
	void _set_transform_array(const Vector<Vector3>& p_array);
	Vector<Vector3> _get_transform_array() const;

	void _set_transform_2d_array(const Vector<Vector2>& p_array);
	Vector<Vector2> _get_transform_2d_array() const;

	void _set_color_array(const Vector<Color>& p_array);
	Vector<Color> _get_color_array() const;

	void _set_custom_data_array(const Vector<Color>& p_array);
	Vector<Color> _get_custom_data_array() const;
#endif
	void set_buffer(const Vector<float>& p_buffer);
	Vector<float> get_buffer() const;

	void set_buffer_interpolated(
		const Vector<float>& p_buffer_curr, const Vector<float>& p_buffer_prev);

public:
	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_use_colors(bool p_enable);
	bool is_using_colors() const;

	void set_use_custom_data(bool p_enable);
	bool is_using_custom_data() const;

	void set_transform_format(TransformFormat p_transform_format);
	TransformFormat get_transform_format() const;

	void set_instance_count(int p_count);
	int get_instance_count() const;

	void set_visible_instance_count(int p_count);
	int get_visible_instance_count() const;

	void set_physics_interpolation_quality(PhysicsInterpolationQuality p_quality);

	PhysicsInterpolationQuality get_physics_interpolation_quality() const
	{
		return _physics_interpolation_quality;
	}

	void set_instance_transform(int p_instance, const Transform3D& p_transform);
	void set_instance_transform_2d(int p_instance, const Transform2D& p_transform);
	Transform3D get_instance_transform(int p_instance) const;
	Transform2D get_instance_transform_2d(int p_instance) const;

	void set_instance_color(int p_instance, const Color& p_color);
	Color get_instance_color(int p_instance) const;

	void set_instance_custom_data(int p_instance, const Color& p_custom_data);
	Color get_instance_custom_data(int p_instance) const;

	void reset_instance_physics_interpolation(int p_instance);
	void reset_instances_physics_interpolation();

	void set_physics_interpolated(bool p_interpolated);

	void set_custom_aabb(const AABB& p_custom);
	AABB get_custom_aabb() const;

	virtual AABB get_aabb() const;

	virtual RID get_rid() const override;

	MultiMesh();
	~MultiMesh();
};

#ifndef NAVIGATION_3D_DISABLED
class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#endif // NAVIGATION_3D_DISABLED

class MultiMeshInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Ref<MultiMesh> multimesh;

	void _refresh_interpolated();

protected:
	virtual void _physics_interpolated_changed() override;
	virtual void fti_update_servers_xform() override;
	void _notification(int p_what);

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_multimesh(const Ref<MultiMesh>& p_multimesh);
	Ref<MultiMesh> get_multimesh() const;

	AABB get_aabb() const;

private:
#ifndef NAVIGATION_3D_DISABLED
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

public:
#ifndef NAVIGATION_3D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	MultiMeshInstance3D() = default;
	~MultiMeshInstance3D() = default;
};

// --- End scene/3d/multimesh_instance_3d.h ---
