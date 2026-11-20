/**************************************************************************/
/*  primitive_meshes.h                                                    */
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

#include <memory>

#include "scene/resources/curve.h"
#include "scene/resources/font.h"
#include "scene/resources/mesh.h"
#include "servers/text/text_server.h"

/**
	Parameterized procedural single-surface mesh covering box, capsule, cylinder,
	plane, quad, prism, sphere, torus, point, tube trail, ribbon trail, and 3D text.
*/
class PrimitiveMesh : public Mesh
{
public:
	enum MeshType
	{
		MESH_BOX,
		MESH_CAPSULE,
		MESH_CYLINDER,
		MESH_PLANE,
		MESH_QUAD,
		MESH_PRISM,
		MESH_SPHERE,
		MESH_TORUS,
		MESH_POINT,
		MESH_TUBE_TRAIL,
		MESH_RIBBON_TRAIL,
		MESH_TEXT,
	};

	enum Orientation
	{
		FACE_X,
		FACE_Y,
		FACE_Z,
	};

	enum RibbonShape
	{
		SHAPE_FLAT,
		SHAPE_CROSS,
	};

private:
	struct BoxPrismParams
	{
		Vector3 size = Vector3(1.0f, 1.0f, 1.0f);
		float left_to_right = 0.5f;
		int subdivide_w = 0;
		int subdivide_h = 0;
		int subdivide_d = 0;
	};

	struct PlaneParams
	{
		Size2 size = Size2(2.0f, 2.0f);
		Vector3 center_offset;
		Orientation orientation = FACE_Y;
	};

	struct RadialParams
	{
		float radius = 0.5f;
		float top_radius = 0.5f;
		float bottom_radius = 0.5f;
		float inner_radius = 0.5f;
		float outer_radius = 1.0f;
		float height = 2.0f;
		int radial_segments = 64;
		int rings = 8;
		int ring_segments = 32;
		bool cap_top = true;
		bool cap_bottom = true;
		bool is_hemisphere = false;
	};

	struct TrailParams
	{
		float ribbon_size = 1.0f;
		int radial_steps = 8;
		int sections = 5;
		float section_length = 0.2f;
		int section_rings = 3;
		int section_segments = 3;
		RibbonShape ribbon_shape = SHAPE_CROSS;
		Ref<Curve> curve;
	};

	struct TextMeshState
	{
		struct ContourPoint
		{
			Vector2 point;
			bool sharp = false;

			ContourPoint() = default;
			ContourPoint(const Vector2& p_pt, bool p_sharp) : point(p_pt), sharp(p_sharp) {}
		};

		struct ContourInfo
		{
			real_t length = 0.0;
			bool ccw = true;

			ContourInfo() = default;
			ContourInfo(real_t p_len, bool p_ccw) : length(p_len), ccw(p_ccw) {}
		};

		struct GlyphMeshKey
		{
			uint64_t font_id = 0;
			uint32_t gl_id = 0;

			bool operator==(const GlyphMeshKey& p_b) const
			{
				return (font_id == p_b.font_id) && (gl_id == p_b.gl_id);
			}

			GlyphMeshKey(uint64_t p_font_id, uint32_t p_gl_id) :
					font_id(p_font_id), gl_id(p_gl_id) {}
		};

		struct GlyphMeshKeyHasher
		{
			_FORCE_INLINE_ static uint32_t hash(const GlyphMeshKey& p_a)
			{
				return hash_murmur3_buffer(&p_a, sizeof(GlyphMeshKey));
			}
		};

		struct GlyphMeshData
		{
			Vector<Vector2> triangles;
			Vector<Vector<ContourPoint>> contours;
			Vector<ContourInfo> contours_info;
			Vector2 min_p = Vector2(Math::INF, Math::INF);
			Vector2 max_p = Vector2(-Math::INF, -Math::INF);
		};

		mutable HashMap<GlyphMeshKey, GlyphMeshData, GlyphMeshKeyHasher> cache;

		mutable RID text_rid;
		mutable Vector<RID> lines_rid;

		String text;
		String xl_text;

		int font_size = 16;
		Ref<Font> font_override;

		TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
		uint32_t jst_flags = TextServer::JUSTIFICATION_WORD_BOUND |
							 TextServer::JUSTIFICATION_KASHIDA |
							 TextServer::JUSTIFICATION_SKIP_LAST_LINE |
							 TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
		float width = 500.0f;
		float line_spacing = 0.0f;
		Point2 lbl_offset;

		HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER;
		VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_CENTER;
		bool uppercase = false;
		String language;
		TextServer::Direction text_direction = TextServer::DIRECTION_AUTO;
		TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;

		real_t depth = 0.05;
		real_t pixel_size = 0.01;
		real_t curve_step = 0.5;

		mutable bool dirty_lines = true;
		mutable bool dirty_text = true;
		mutable bool dirty_font = true;
		mutable bool dirty_cache = true;

		RID ensure_shaped_text() const;
		~TextMeshState();
	};

	MeshType mesh_type = MESH_BOX;

	RID mesh;
	mutable AABB aabb;
	AABB custom_aabb;

	mutable int array_len = 0;
	mutable int index_array_len = 0;

	Ref<Material> material;
	bool flip_faces = false;

	bool add_uv2 = false;
	float uv2_padding = 2.0f;

	mutable bool pending_request = true;
	Mesh::PrimitiveType primitive_type = Mesh::PRIMITIVE_TRIANGLES;
	float texel_size = 0.2f;

	BoxPrismParams box_prism;
	PlaneParams plane;
	RadialParams radial;
	TrailParams trail;
	std::unique_ptr<TextMeshState> text_state;

	void _apply_mesh_type_defaults(MeshType p_type);
	TextMeshState& _ensure_text_state();
	const TextMeshState& _get_text_state() const;

	Vector2 get_uv2_scale(Vector2 p_margin_scale = Vector2(1.0f, 1.0f)) const;
	float get_lightmap_texel_size() const;

	void _update() const;
	void _update_lightmap_size();
	void _on_settings_changed();
	void _curve_changed();
	void _font_changed();

public:
	PrimitiveMesh(MeshType p_type = MESH_BOX);
	virtual ~PrimitiveMesh() override;

	void set_mesh_type(MeshType p_type);
	MeshType get_mesh_type() const { return mesh_type; }

	virtual int get_surface_count() const override;
	virtual int surface_get_array_len(int p_idx) const override;
	virtual int surface_get_array_index_len(int p_idx) const override;
	virtual uint32_t surface_get_format(int p_idx) const override;
	virtual Mesh::PrimitiveType surface_get_primitive_type(int p_idx) const override;
	virtual void surface_set_material(int p_idx, const Ref<Material>& p_material) override;
	virtual Ref<Material> surface_get_material(int p_idx) const override;
	virtual int get_blend_shape_count() const override;
	virtual String get_blend_shape_name(int p_index) const override;
	virtual void set_blend_shape_name(int p_index, const String& p_name) override;
	virtual AABB get_aabb() const override;
	virtual RID get_rid() const override;
	virtual int get_builtin_bind_pose_count() const override;
	virtual Transform3D get_builtin_bind_pose(int p_index) const override;

	void set_material(const Ref<Material>& p_material);
	Ref<Material> get_material() const;

	void set_custom_aabb(const AABB& p_custom);
	AABB get_custom_aabb() const;

	void set_flip_faces(bool p_enable);
	bool get_flip_faces() const;

	void set_add_uv2(bool p_enable);
	bool get_add_uv2() const { return add_uv2; }

	void set_uv2_padding(float p_padding);
	float get_uv2_padding() const { return uv2_padding; }

	void request_update();

	// Box / Prism / Plane / Quad parameters.
	void set_size(const Vector3& p_size);
	Vector3 get_size() const;

	void set_plane_size(const Size2& p_size);
	Size2 get_plane_size() const;
	void set_size(const Size2& p_size) { set_plane_size(p_size); }

	void set_subdivide_width(int p_divisions);
	int get_subdivide_width() const;

	void set_subdivide_height(int p_divisions);
	int get_subdivide_height() const;

	void set_subdivide_depth(int p_divisions);
	int get_subdivide_depth() const;

	void set_center_offset(const Vector3& p_offset);
	Vector3 get_center_offset() const;

	void set_orientation(Orientation p_orientation);
	Orientation get_orientation() const;

	void set_left_to_right(float p_left_to_right);
	float get_left_to_right() const;

	// Capsule / Cylinder / Sphere / Torus parameters.
	void set_radius(float p_radius);
	float get_radius() const;

	void set_top_radius(float p_radius);
	float get_top_radius() const;

	void set_bottom_radius(float p_radius);
	float get_bottom_radius() const;

	void set_inner_radius(float p_inner_radius);
	float get_inner_radius() const;

	void set_outer_radius(float p_outer_radius);
	float get_outer_radius() const;

	void set_height(float p_height);
	float get_height() const;

	void set_radial_segments(int p_segments);
	int get_radial_segments() const;

	void set_rings(int p_rings);
	int get_rings() const;

	void set_ring_segments(int p_ring_segments);
	int get_ring_segments() const;

	void set_cap_top(bool p_cap_top);
	bool is_cap_top() const;

	void set_cap_bottom(bool p_cap_bottom);
	bool is_cap_bottom() const;

	void set_is_hemisphere(bool p_is_hemisphere);
	bool get_is_hemisphere() const;

	// TubeTrail / RibbonTrail parameters.
	void set_radial_steps(int p_radial_steps);
	int get_radial_steps() const;

	void set_sections(int p_sections);
	int get_sections() const;

	void set_section_length(float p_section_length);
	float get_section_length() const;

	void set_section_rings(int p_section_rings);
	int get_section_rings() const;

	void set_section_segments(int p_section_segments);
	int get_section_segments() const;

	void set_ribbon_size(float p_size);
	float get_ribbon_size() const;

	void set_ribbon_shape(RibbonShape p_shape);
	RibbonShape get_ribbon_shape() const;
	void set_shape(RibbonShape p_shape) { set_ribbon_shape(p_shape); }
	RibbonShape get_shape() const { return get_ribbon_shape(); }

	void set_curve(const Ref<Curve>& p_curve);
	Ref<Curve> get_curve() const;

	// Text parameters.
	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_text(const String& p_string);
	String get_text() const;

	void set_font(const Ref<Font>& p_font);
	Ref<Font> get_font() const;
	Ref<Font> _get_font_or_default() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_line_spacing(float p_line_spacing);
	float get_line_spacing() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_text_direction(TextServer::Direction p_text_direction);
	TextServer::Direction get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_width(real_t p_width);
	real_t get_width() const;

	void set_depth(real_t p_depth);
	real_t get_depth() const;

	void set_curve_step(real_t p_step);
	real_t get_curve_step() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;
};
