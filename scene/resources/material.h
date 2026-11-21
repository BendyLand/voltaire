/**************************************************************************/
/*  material.h                                                            */
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

#include "core/io/resource.h"
#include "core/templates/self_list.h"
#include "scene/resources/shader.h"
#include "servers/rendering/rendering_server_enums.h"

class Material : public Resource
{
	mutable RID material;
	Ref<Material> next_pass;
	int render_priority;
	bool is_placeholder = false;

	enum
	{
		INIT_STATE_UNINITIALIZED,
		INIT_STATE_INITIALIZING,
		INIT_STATE_READY,
	} init_state = INIT_STATE_UNINITIALIZED;

	void inspect_native_shader_code();

protected:
	_FORCE_INLINE_ void _set_material(RID p_material) const { material = p_material; }

	_FORCE_INLINE_ RID _get_material() const { return material; }

	static void _bind_methods();
	virtual bool _can_do_next_pass() const;
	virtual bool _can_use_render_priority() const;

	void _mark_ready();

public:
	enum
	{
		RENDER_PRIORITY_MAX = RSE::MATERIAL_RENDER_PRIORITY_MAX,
		RENDER_PRIORITY_MIN = RSE::MATERIAL_RENDER_PRIORITY_MIN,
	};

	bool _is_initialized() { return init_state == INIT_STATE_READY; }

	void set_next_pass(const Ref<Material>& p_pass);
	Ref<Material> get_next_pass() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	virtual RID get_rid() const override;
	virtual RID get_shader_rid() const;
	virtual Shader::Mode get_shader_mode() const;

	virtual Ref<Resource> create_placeholder() const;
	bool is_placeholder_material() const { return is_placeholder; }

	Material();
	virtual ~Material();
};

class ShaderMaterial final : public Material
{
	Ref<Shader> shader;

	mutable HashMap<String, String> remap_cache;
	mutable Mutex material_rid_mutex;

protected:
	bool _property_can_revert(const String& p_name) const;

	static void _bind_methods();

#ifdef TOOLS_ENABLED
	void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	virtual bool _can_do_next_pass() const override;
	virtual bool _can_use_render_priority() const override;

	void _shader_changed();
	void _check_material_rid() const;

public:
	void set_shader(const Ref<Shader>& p_shader);
	Ref<Shader> get_shader() const;

	virtual Shader::Mode get_shader_mode() const override;

	virtual RID get_rid() const override;
	virtual RID get_shader_rid() const override;

	ShaderMaterial();
	~ShaderMaterial();
};

class StandardMaterial3D final : public Material
{
private:
	mutable Mutex material_rid_mutex;

public:
	enum TextureParam
	{
		TEXTURE_ALBEDO,
		TEXTURE_METALLIC,
		TEXTURE_ROUGHNESS,
		TEXTURE_EMISSION,
		TEXTURE_NORMAL,
		TEXTURE_RIM,
		TEXTURE_CLEARCOAT,
		TEXTURE_FLOWMAP,
		TEXTURE_AMBIENT_OCCLUSION,
		TEXTURE_HEIGHTMAP,
		TEXTURE_SUBSURFACE_SCATTERING,
		TEXTURE_SUBSURFACE_TRANSMITTANCE,
		TEXTURE_BACKLIGHT,
		TEXTURE_REFRACTION,
		TEXTURE_DETAIL_MASK,
		TEXTURE_DETAIL_ALBEDO,
		TEXTURE_DETAIL_NORMAL,
		TEXTURE_ORM,
		TEXTURE_BENT_NORMAL,
		TEXTURE_MAX
	};

	enum TextureFilter
	{
		TEXTURE_FILTER_NEAREST,
		TEXTURE_FILTER_LINEAR,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
		TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC,
		TEXTURE_FILTER_MAX
	};

	enum DetailUV
	{
		DETAIL_UV_1,
		DETAIL_UV_2,
		DETAIL_UV_MAX
	};

	enum Transparency
	{
		TRANSPARENCY_DISABLED,
		TRANSPARENCY_ALPHA,
		TRANSPARENCY_ALPHA_SCISSOR,
		TRANSPARENCY_ALPHA_HASH,
		TRANSPARENCY_ALPHA_DEPTH_PRE_PASS,
		TRANSPARENCY_MAX,
	};

	enum AlphaAntiAliasing
	{
		ALPHA_ANTIALIASING_OFF,
		ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE,
		ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE,
		ALPHA_ANTIALIASING_MAX
	};

	enum ShadingMode
	{
		SHADING_MODE_UNSHADED,
		SHADING_MODE_PER_PIXEL,
		SHADING_MODE_PER_VERTEX,
		SHADING_MODE_MAX
	};

	enum Feature
	{
		FEATURE_EMISSION,
		FEATURE_NORMAL_MAPPING,
		FEATURE_RIM,
		FEATURE_CLEARCOAT,
		FEATURE_ANISOTROPY,
		FEATURE_AMBIENT_OCCLUSION,
		FEATURE_HEIGHT_MAPPING,
		FEATURE_SUBSURFACE_SCATTERING,
		FEATURE_SUBSURFACE_TRANSMITTANCE,
		FEATURE_BACKLIGHT,
		FEATURE_REFRACTION,
		FEATURE_DETAIL,
		FEATURE_BENT_NORMAL_MAPPING,
		FEATURE_MAX
	};

	enum BlendMode
	{
		BLEND_MODE_MIX,
		BLEND_MODE_ADD,
		BLEND_MODE_SUB,
		BLEND_MODE_MUL,
		BLEND_MODE_PREMULT_ALPHA,
		BLEND_MODE_MAX
	};

	enum DepthDrawMode
	{
		DEPTH_DRAW_OPAQUE_ONLY,
		DEPTH_DRAW_ALWAYS,
		DEPTH_DRAW_DISABLED,
		DEPTH_DRAW_MAX
	};

	enum DepthTest
	{
		DEPTH_TEST_DEFAULT,
		DEPTH_TEST_INVERTED,
		DEPTH_TEST_MAX
	};

	enum CullMode
	{
		CULL_BACK,
		CULL_FRONT,
		CULL_DISABLED,
		CULL_MAX
	};

	enum Flags
	{
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_ALBEDO_FROM_VERTEX_COLOR,
		FLAG_SRGB_VERTEX_COLOR,
		FLAG_USE_POINT_SIZE,
		FLAG_FIXED_SIZE,
		FLAG_BILLBOARD_KEEP_SCALE,
		FLAG_UV1_USE_TRIPLANAR,
		FLAG_UV2_USE_TRIPLANAR,
		FLAG_UV1_USE_WORLD_TRIPLANAR,
		FLAG_UV2_USE_WORLD_TRIPLANAR,
		FLAG_AO_ON_UV2,
		FLAG_EMISSION_ON_UV2,
		FLAG_ALBEDO_TEXTURE_FORCE_SRGB,
		FLAG_DONT_RECEIVE_SHADOWS,
		FLAG_DISABLE_AMBIENT_LIGHT,
		FLAG_USE_SHADOW_TO_OPACITY,
		FLAG_USE_TEXTURE_REPEAT,
		FLAG_INVERT_HEIGHTMAP,
		FLAG_SUBSURFACE_MODE_SKIN,
		FLAG_PARTICLE_TRAILS_MODE,
		FLAG_ALBEDO_TEXTURE_MSDF,
		FLAG_DISABLE_FOG,
		FLAG_DISABLE_SPECULAR_OCCLUSION,
		FLAG_USE_Z_CLIP_SCALE,
		FLAG_USE_FOV_OVERRIDE,
		FLAG_MAX
	};

	enum DiffuseMode
	{
		DIFFUSE_BURLEY,
		DIFFUSE_LAMBERT,
		DIFFUSE_LAMBERT_WRAP,
		DIFFUSE_TOON,
		DIFFUSE_MAX
	};

	enum SpecularMode
	{
		SPECULAR_SCHLICK_GGX,
		SPECULAR_TOON,
		SPECULAR_DISABLED,
		SPECULAR_MAX
	};

	enum BillboardMode
	{
		BILLBOARD_DISABLED,
		BILLBOARD_ENABLED,
		BILLBOARD_FIXED_Y,
		BILLBOARD_PARTICLES,
		BILLBOARD_MAX
	};

	enum TextureChannel
	{
		TEXTURE_CHANNEL_RED,
		TEXTURE_CHANNEL_GREEN,
		TEXTURE_CHANNEL_BLUE,
		TEXTURE_CHANNEL_ALPHA,
		TEXTURE_CHANNEL_GRAYSCALE,
		TEXTURE_CHANNEL_MAX
	};

	enum EmissionOperator
	{
		EMISSION_OP_ADD,
		EMISSION_OP_MULTIPLY,
		EMISSION_OP_MAX
	};

	enum DistanceFadeMode
	{
		DISTANCE_FADE_DISABLED,
		DISTANCE_FADE_PIXEL_ALPHA,
		DISTANCE_FADE_PIXEL_DITHER,
		DISTANCE_FADE_OBJECT_DITHER,
		DISTANCE_FADE_MAX
	};

	enum StencilMode
	{
		STENCIL_MODE_DISABLED,
		STENCIL_MODE_OUTLINE,
		STENCIL_MODE_XRAY,
		STENCIL_MODE_CUSTOM,
		STENCIL_MODE_MAX // Not an actual mode, just the amount of modes.
	};

	enum StencilFlags
	{
		STENCIL_FLAG_READ = 1,
		STENCIL_FLAG_WRITE = 2,
		STENCIL_FLAG_WRITE_DEPTH_FAIL = 4,

		STENCIL_FLAG_NUM_BITS = 3 // Not an actual mode, just the amount of bits.
	};

	enum StencilCompare
	{
		STENCIL_COMPARE_ALWAYS,
		STENCIL_COMPARE_LESS,
		STENCIL_COMPARE_EQUAL,
		STENCIL_COMPARE_LESS_OR_EQUAL,
		STENCIL_COMPARE_GREATER,
		STENCIL_COMPARE_NOT_EQUAL,
		STENCIL_COMPARE_GREATER_OR_EQUAL,
		STENCIL_COMPARE_MAX // Not an actual operator, just the amount of operators.
	};

private:
	struct MaterialKey
	{
		// enum values
		uint64_t texture_filter : Math::get_num_bits(TEXTURE_FILTER_MAX - 1);
		uint64_t detail_uv : Math::get_num_bits(DETAIL_UV_MAX - 1);
		uint64_t transparency : Math::get_num_bits(TRANSPARENCY_MAX - 1);
		uint64_t alpha_antialiasing_mode : Math::get_num_bits(ALPHA_ANTIALIASING_MAX - 1);
		uint64_t shading_mode : Math::get_num_bits(SHADING_MODE_MAX - 1);
		uint64_t blend_mode : Math::get_num_bits(BLEND_MODE_MAX - 1);
		uint64_t depth_draw_mode : Math::get_num_bits(DEPTH_DRAW_MAX - 1);
		uint64_t depth_test : Math::get_num_bits(DEPTH_TEST_MAX - 1);
		uint64_t cull_mode : Math::get_num_bits(CULL_MAX - 1);
		uint64_t diffuse_mode : Math::get_num_bits(DIFFUSE_MAX - 1);
		uint64_t specular_mode : Math::get_num_bits(SPECULAR_MAX - 1);
		uint64_t billboard_mode : Math::get_num_bits(BILLBOARD_MAX - 1);
		uint64_t detail_blend_mode : Math::get_num_bits(BLEND_MODE_MAX - 1);
		uint64_t roughness_channel : Math::get_num_bits(TEXTURE_CHANNEL_MAX - 1);
		uint64_t emission_op : Math::get_num_bits(EMISSION_OP_MAX - 1);
		uint64_t distance_fade : Math::get_num_bits(DISTANCE_FADE_MAX - 1);

		// stencil
		uint64_t stencil_mode : Math::get_num_bits(STENCIL_MODE_MAX - 1);
		uint64_t stencil_flags : STENCIL_FLAG_NUM_BITS;
		uint64_t stencil_compare : Math::get_num_bits(STENCIL_COMPARE_MAX - 1);
		uint64_t stencil_reference : 8;

		// booleans
		uint64_t invalid_key : 1;
		uint64_t deep_parallax : 1;
		uint64_t grow : 1;
		uint64_t proximity_fade : 1;
		uint64_t orm : 1;

		// flag bitfield
		uint32_t feature_mask;
		uint32_t flags;

		MaterialKey() { memset(this, 0, sizeof(MaterialKey)); }

		static uint32_t hash(const MaterialKey& p_key)
		{
			return hash_djb2_buffer((const uint8_t*)&p_key, sizeof(MaterialKey));
		}

		bool operator==(const MaterialKey& p_key) const
		{
			return memcmp(this, &p_key, sizeof(MaterialKey)) == 0;
		}

		bool operator<(const MaterialKey& p_key) const
		{
			return memcmp(this, &p_key, sizeof(MaterialKey)) < 0;
		}
	};

	struct ShaderData
	{
		RID shader;
		int users = 0;
	};

	static HashMap<MaterialKey, ShaderData, MaterialKey> shader_map;
	static Mutex shader_map_mutex;

	MaterialKey current_key;

	_FORCE_INLINE_ MaterialKey _compute_key() const
	{
		MaterialKey mk;

		mk.detail_uv = detail_uv;
		mk.blend_mode = blend_mode;
		mk.depth_draw_mode = depth_draw_mode;
		mk.depth_test = depth_test;
		mk.cull_mode = cull_mode;
		mk.texture_filter = texture_filter;
		mk.transparency = transparency;
		mk.shading_mode = shading_mode;
		mk.roughness_channel = roughness_texture_channel;
		mk.detail_blend_mode = detail_blend_mode;
		mk.diffuse_mode = diffuse_mode;
		mk.specular_mode = specular_mode;
		mk.billboard_mode = billboard_mode;
		mk.deep_parallax = deep_parallax;
		mk.grow = grow_enabled;
		mk.proximity_fade = proximity_fade_enabled;
		mk.distance_fade = distance_fade;
		mk.emission_op = emission_op;
		mk.alpha_antialiasing_mode = alpha_antialiasing_mode;
		mk.orm = orm;

		mk.stencil_mode = stencil_mode;
		mk.stencil_flags = stencil_flags;
		mk.stencil_compare = stencil_compare;
		mk.stencil_reference = stencil_reference;

		for (int i = 0; i < FEATURE_MAX; i++) {
			if (features[i]) {
				mk.feature_mask |= ((uint64_t)1 << i);
			}
		}

		for (int i = 0; i < FLAG_MAX; i++) {
			if (flags[i]) {
				mk.flags |= ((uint64_t)1 << i);
			}
		}

		return mk;
	}

	struct ShaderNames
	{
		String albedo;
		String specular;
		String metallic;
		String roughness;
		String emission;
		String emission_energy;
		String normal_scale;
		String rim;
		String rim_tint;
		String clearcoat;
		String clearcoat_roughness;
		String anisotropy;
		String heightmap_scale;
		String subsurface_scattering_strength;
		String transmittance_color;
		String transmittance_depth;
		String transmittance_boost;
		String backlight;
		String refraction;
		String point_size;
		String uv1_scale;
		String uv1_offset;
		String uv2_scale;
		String uv2_offset;
		String particles_anim_h_frames;
		String particles_anim_v_frames;
		String particles_anim_loop;
		String heightmap_min_layers;
		String heightmap_max_layers;
		String heightmap_flip;
		String uv1_blend_sharpness;
		String uv2_blend_sharpness;
		String grow;
		String proximity_fade_distance;
		String msdf_pixel_range;
		String msdf_outline_size;
		String distance_fade_min;
		String distance_fade_max;
		String ao_light_affect;

		String metallic_texture_channel;
		String ao_texture_channel;
		String clearcoat_texture_channel;
		String rim_texture_channel;
		String heightmap_texture_channel;
		String refraction_texture_channel;

		String texture_names[TEXTURE_MAX];

		String alpha_scissor_threshold;
		String alpha_hash_scale;

		String alpha_antialiasing_edge;
		String albedo_texture_size;
		String z_clip_scale;
		String fov_override;
	};

	static Mutex material_mutex;
	static SelfList<StandardMaterial3D>::List dirty_materials;
	static ShaderNames* shader_names;

	SelfList<StandardMaterial3D> element;

	void _update_shader();
	_FORCE_INLINE_ void _queue_shader_change();
	void _check_material_rid();

	bool orm = false;
	RID shader_rid;

	Color albedo;
	float specular = 0.0f;
	float metallic = 0.0f;
	float roughness = 0.0f;
	Color emission;
	float emission_energy_multiplier = 1.0f;
	float emission_intensity = 1000.0f; // In nits, equivalent to indoor lighting.
	float normal_scale = 0.0f;
	float rim = 0.0f;
	float rim_tint = 0.0f;
	float clearcoat = 0.0f;
	float clearcoat_roughness = 0.0f;
	float anisotropy = 0.0f;
	float heightmap_scale = 0.0f;
	float subsurface_scattering_strength = 0.0f;
	Color transmittance_color;
	float transmittance_depth = 0.0f;
	float transmittance_boost = 0.0f;

	Color backlight;
	float refraction = 0.0f;
	float point_size = 0.0f;
	float alpha_scissor_threshold = 0.0f;
	float alpha_hash_scale = 0.0f;
	float alpha_antialiasing_edge = 0.0f;
	bool grow_enabled = false;
	float ao_light_affect = 0.0f;
	float grow = 0.0f;
	int particles_anim_h_frames = 0;
	int particles_anim_v_frames = 0;
	bool particles_anim_loop = false;
	Transparency transparency = TRANSPARENCY_DISABLED;
	ShadingMode shading_mode = SHADING_MODE_PER_PIXEL;

	TextureFilter texture_filter = TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;

	Vector3 uv1_scale;
	Vector3 uv1_offset;
	float uv1_triplanar_sharpness = 0.0f;

	Vector3 uv2_scale;
	Vector3 uv2_offset;
	float uv2_triplanar_sharpness = 0.0f;

	DetailUV detail_uv = DETAIL_UV_1;

	bool deep_parallax = false;
	int deep_parallax_min_layers = 0;
	int deep_parallax_max_layers = 0;
	bool heightmap_parallax_flip_tangent = false;
	bool heightmap_parallax_flip_binormal = false;

	bool proximity_fade_enabled = false;
	float proximity_fade_distance = 0.0f;

	float msdf_pixel_range = 4.f;
	float msdf_outline_size = 0.f;

	DistanceFadeMode distance_fade = DISTANCE_FADE_DISABLED;
	float distance_fade_max_distance = 0.0f;
	float distance_fade_min_distance = 0.0f;

	BlendMode blend_mode = BLEND_MODE_MIX;
	BlendMode detail_blend_mode = BLEND_MODE_MIX;
	DepthDrawMode depth_draw_mode = DEPTH_DRAW_OPAQUE_ONLY;
	DepthTest depth_test = DEPTH_TEST_DEFAULT;
	CullMode cull_mode = CULL_BACK;
	bool flags[FLAG_MAX] = {};
	SpecularMode specular_mode = SPECULAR_SCHLICK_GGX;
	DiffuseMode diffuse_mode = DIFFUSE_BURLEY;
	BillboardMode billboard_mode;
	EmissionOperator emission_op = EMISSION_OP_ADD;

	TextureChannel metallic_texture_channel;
	TextureChannel roughness_texture_channel;
	TextureChannel ao_texture_channel;
	TextureChannel refraction_texture_channel;

	AlphaAntiAliasing alpha_antialiasing_mode = ALPHA_ANTIALIASING_OFF;

	float z_clip_scale = 1.0;
	float fov_override = 75.0;

	StencilMode stencil_mode = STENCIL_MODE_DISABLED;
	int stencil_flags = 0;
	StencilCompare stencil_compare = STENCIL_COMPARE_ALWAYS;
	int stencil_reference = 1;

	Color stencil_effect_color;
	float stencil_effect_outline_thickness = 0.01f;

	bool features[FEATURE_MAX] = {};

	Ref<Texture2D> textures[TEXTURE_MAX];

	void _prepare_stencil_effect();
	Ref<StandardMaterial3D> _get_stencil_next_pass() const;

	static HashMap<uint64_t, Ref<StandardMaterial3D>>
		materials_for_2d; // used by Sprite3D, Label3D and other stuff

protected:
	static void _bind_methods();

	virtual bool _can_do_next_pass() const override { return true; }

	virtual bool _can_use_render_priority() const override { return true; }

public:
	void set_albedo(const Color& p_albedo);
	Color get_albedo() const;

	void set_specular(float p_specular);
	float get_specular() const;

	void set_metallic(float p_metallic);
	float get_metallic() const;

	void set_roughness(float p_roughness);
	float get_roughness() const;

	void set_emission(const Color& p_emission);
	Color get_emission() const;

	void set_emission_energy_multiplier(float p_emission_energy_multiplier);
	float get_emission_energy_multiplier() const;

	void set_emission_intensity(float p_emission_intensity);
	float get_emission_intensity() const;

	void set_normal_scale(float p_normal_scale);
	float get_normal_scale() const;

	void set_rim(float p_rim);
	float get_rim() const;

	void set_rim_tint(float p_rim_tint);
	float get_rim_tint() const;

	void set_ao_light_affect(float p_ao_light_affect);
	float get_ao_light_affect() const;

	void set_clearcoat(float p_clearcoat);
	float get_clearcoat() const;

	void set_clearcoat_roughness(float p_clearcoat_roughness);
	float get_clearcoat_roughness() const;

	void set_anisotropy(float p_anisotropy);
	float get_anisotropy() const;

	void set_heightmap_scale(float p_heightmap_scale);
	float get_heightmap_scale() const;

	void set_heightmap_deep_parallax(bool p_enable);
	bool is_heightmap_deep_parallax_enabled() const;

	void set_heightmap_deep_parallax_min_layers(int p_layer);
	int get_heightmap_deep_parallax_min_layers() const;

	void set_heightmap_deep_parallax_max_layers(int p_layer);
	int get_heightmap_deep_parallax_max_layers() const;

	void set_heightmap_deep_parallax_flip_tangent(bool p_flip);
	bool get_heightmap_deep_parallax_flip_tangent() const;

	void set_heightmap_deep_parallax_flip_binormal(bool p_flip);
	bool get_heightmap_deep_parallax_flip_binormal() const;

	void set_subsurface_scattering_strength(float p_subsurface_scattering_strength);
	float get_subsurface_scattering_strength() const;

	void set_transmittance_color(const Color& p_color);
	Color get_transmittance_color() const;

	void set_transmittance_depth(float p_depth);
	float get_transmittance_depth() const;

	void set_transmittance_boost(float p_boost);
	float get_transmittance_boost() const;

	void set_backlight(const Color& p_backlight);
	Color get_backlight() const;

	void set_refraction(float p_refraction);
	float get_refraction() const;

	void set_point_size(float p_point_size);
	float get_point_size() const;

	void set_transparency(Transparency p_transparency);
	Transparency get_transparency() const;

	void set_alpha_antialiasing(AlphaAntiAliasing p_alpha_aa);
	AlphaAntiAliasing get_alpha_antialiasing() const;

	void set_alpha_antialiasing_edge(float p_edge);
	float get_alpha_antialiasing_edge() const;

	void set_shading_mode(ShadingMode p_shading_mode);
	ShadingMode get_shading_mode() const;

	void set_detail_uv(DetailUV p_detail_uv);
	DetailUV get_detail_uv() const;

	void set_blend_mode(BlendMode p_mode);
	BlendMode get_blend_mode() const;

	void set_detail_blend_mode(BlendMode p_mode);
	BlendMode get_detail_blend_mode() const;

	void set_depth_draw_mode(DepthDrawMode p_mode);
	DepthDrawMode get_depth_draw_mode() const;

	void set_depth_test(DepthTest p_func);
	DepthTest get_depth_test() const;

	void set_cull_mode(CullMode p_mode);
	CullMode get_cull_mode() const;

	void set_diffuse_mode(DiffuseMode p_mode);
	DiffuseMode get_diffuse_mode() const;

	void set_specular_mode(SpecularMode p_mode);
	SpecularMode get_specular_mode() const;

	void set_flag(Flags p_flag, bool p_enabled);
	bool get_flag(Flags p_flag) const;

	void set_texture(TextureParam p_param, const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture(TextureParam p_param) const;
	// Used only for shader material conversion
	Ref<Texture2D> get_texture_by_name(const String& p_name) const;

	void set_texture_filter(TextureFilter p_filter);
	TextureFilter get_texture_filter() const;

	void set_feature(Feature p_feature, bool p_enabled);
	bool get_feature(Feature p_feature) const;

	void set_uv1_scale(const Vector3& p_scale);
	Vector3 get_uv1_scale() const;

	void set_uv1_offset(const Vector3& p_offset);
	Vector3 get_uv1_offset() const;

	void set_uv1_triplanar_blend_sharpness(float p_sharpness);
	float get_uv1_triplanar_blend_sharpness() const;

	void set_uv2_scale(const Vector3& p_scale);
	Vector3 get_uv2_scale() const;

	void set_uv2_offset(const Vector3& p_offset);
	Vector3 get_uv2_offset() const;

	void set_uv2_triplanar_blend_sharpness(float p_sharpness);
	float get_uv2_triplanar_blend_sharpness() const;

	void set_billboard_mode(BillboardMode p_mode);
	BillboardMode get_billboard_mode() const;

	void set_particles_anim_h_frames(int p_frames);
	int get_particles_anim_h_frames() const;
	void set_particles_anim_v_frames(int p_frames);
	int get_particles_anim_v_frames() const;

	void set_particles_anim_loop(bool p_loop);
	bool get_particles_anim_loop() const;

	void set_grow_enabled(bool p_enable);
	bool is_grow_enabled() const;

	void set_grow(float p_grow);
	float get_grow() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_alpha_hash_scale(float p_scale);
	float get_alpha_hash_scale() const;

	void set_on_top_of_alpha();

	void set_proximity_fade_enabled(bool p_enable);
	bool is_proximity_fade_enabled() const;

	void set_proximity_fade_distance(float p_distance);
	float get_proximity_fade_distance() const;

	void set_msdf_pixel_range(float p_range);
	float get_msdf_pixel_range() const;

	void set_msdf_outline_size(float p_size);
	float get_msdf_outline_size() const;

	void set_distance_fade(DistanceFadeMode p_mode);
	DistanceFadeMode get_distance_fade() const;

	void set_distance_fade_max_distance(float p_distance);
	float get_distance_fade_max_distance() const;

	void set_distance_fade_min_distance(float p_distance);
	float get_distance_fade_min_distance() const;

	void set_emission_operator(EmissionOperator p_op);
	EmissionOperator get_emission_operator() const;

	void set_stencil_mode(StencilMode p_stencil_mode);
	StencilMode get_stencil_mode() const;

	void set_stencil_flags(int p_stencil_flags);
	int get_stencil_flags() const;

	void set_stencil_compare(StencilCompare p_op);
	StencilCompare get_stencil_compare() const;

	void set_stencil_reference(int p_reference);
	int get_stencil_reference() const;

	void set_stencil_effect_color(const Color& p_color);
	Color get_stencil_effect_color() const;

	void set_stencil_effect_outline_thickness(float p_outline_thickness);
	float get_stencil_effect_outline_thickness() const;

	void set_metallic_texture_channel(TextureChannel p_channel);
	TextureChannel get_metallic_texture_channel() const;
	void set_roughness_texture_channel(TextureChannel p_channel);
	TextureChannel get_roughness_texture_channel() const;
	void set_ao_texture_channel(TextureChannel p_channel);
	TextureChannel get_ao_texture_channel() const;
	void set_refraction_texture_channel(TextureChannel p_channel);
	TextureChannel get_refraction_texture_channel() const;

	void set_z_clip_scale(float p_z_clip_scale);
	float get_z_clip_scale() const;
	void set_fov_override(float p_fov_override);
	float get_fov_override() const;

	static void init_shaders();
	static void finish_shaders();
	static void flush_changes();

	static Ref<Material> get_material_for_2d(bool p_shaded, Transparency p_transparency,
		bool p_double_sided, bool p_billboard = false, bool p_billboard_y = false,
		bool p_msdf = false, bool p_no_depth = false, bool p_fixed_size = false,
		TextureFilter p_filter = TEXTURE_FILTER_LINEAR_WITH_MIPMAPS,
		AlphaAntiAliasing p_alpha_antialiasing_mode = ALPHA_ANTIALIASING_OFF,
		bool p_texture_repeat = false, RID* r_shader_rid = nullptr);

	void set_orm_mode(bool p_orm);
	bool is_orm_mode() const { return orm; }

	virtual RID get_rid() const override;
	virtual RID get_shader_rid() const override;

	virtual Shader::Mode get_shader_mode() const override;

	StandardMaterial3D(bool p_orm = false);
	virtual ~StandardMaterial3D();
};

//////////////////////

// --- Folded from scene/resources/canvas_item_material.h ---


class CanvasItemMaterial final : public Material
{
public:
	enum BlendMode
	{
		BLEND_MODE_MIX,
		BLEND_MODE_ADD,
		BLEND_MODE_SUB,
		BLEND_MODE_MUL,
		BLEND_MODE_PREMULT_ALPHA,
		BLEND_MODE_DISABLED
	};

	enum LightMode
	{
		LIGHT_MODE_NORMAL,
		LIGHT_MODE_UNSHADED,
		LIGHT_MODE_LIGHT_ONLY
	};

private:
	union MaterialKey
	{
		struct
		{
			uint32_t blend_mode : 4;
			uint32_t light_mode : 4;
			uint32_t particles_animation : 1;
			uint32_t invalid_key : 1;
		};

		uint32_t key = 0;

		static uint32_t hash(const MaterialKey& p_key) { return hash_murmur3_one_32(p_key.key); }

		bool operator==(const MaterialKey& p_key) const { return key == p_key.key; }
	};

	struct ShaderNames
	{
		String particles_anim_h_frames;
		String particles_anim_v_frames;
		String particles_anim_loop;
	};

	static ShaderNames* shader_names;

	struct ShaderData
	{
		RID shader;
		int users = 0;
	};

	static HashMap<MaterialKey, ShaderData, MaterialKey> shader_map;

	MaterialKey current_key;

	_FORCE_INLINE_ MaterialKey _compute_key() const
	{
		MaterialKey mk;
		mk.key = 0;
		mk.blend_mode = blend_mode;
		mk.light_mode = light_mode;
		mk.particles_animation = particles_animation;
		return mk;
	}

	static Mutex material_mutex;
	static SelfList<CanvasItemMaterial>::List dirty_materials;
	SelfList<CanvasItemMaterial> element;

	void _update_shader();
	_FORCE_INLINE_ void _queue_shader_change();

	static Mutex blit_shader_mutex;
	static RID blit_shader_cache[6];
	static void _update_blit_shader(BlendMode p_blend);
	mutable bool blit_shader_set = false;
	bool texture_blit = false;

	BlendMode blend_mode = BLEND_MODE_MIX;
	LightMode light_mode = LIGHT_MODE_NORMAL;
	bool particles_animation = false;

	// Proper values set in constructor.
	int particles_anim_h_frames = 0;
	int particles_anim_v_frames = 0;
	bool particles_anim_loop = false;

public:
	void set_texture_blit(bool p_enable);
	bool is_texture_blit() const { return texture_blit; }

	void set_blend_mode(BlendMode p_blend_mode);
	BlendMode get_blend_mode() const;

	void set_light_mode(LightMode p_light_mode);
	LightMode get_light_mode() const;

	void set_particles_animation(bool p_particles_anim);
	bool get_particles_animation() const;

	void set_particles_anim_h_frames(int p_frames);
	int get_particles_anim_h_frames() const;
	void set_particles_anim_v_frames(int p_frames);
	int get_particles_anim_v_frames() const;

	void set_particles_anim_loop(bool p_loop);
	bool get_particles_anim_loop() const;

	static void init_shaders();
	static void finish_shaders();
	static void cleanup_blit_shaders();
	static void flush_changes();

	virtual RID get_rid() const override;
	virtual RID get_shader_rid() const override;

	virtual Shader::Mode get_shader_mode() const override;

	CanvasItemMaterial(bool p_texture_blit = false);
	virtual ~CanvasItemMaterial();
};

// --- End scene/resources/canvas_item_material.h ---

// --- Folded from scene/resources/particle_process_material.h ---
#include "core/templates/rb_set.h"
#include "core/templates/rid.h"
#include "core/templates/self_list.h"
// --- Folded from scene/resources/curve_texture.h ---
#include "scene/resources/curve.h"
#include "scene/resources/texture.h"

class CurveTexture final : public Texture2D
{
public:
	enum TextureMode
	{
		TEXTURE_MODE_RGB,
		TEXTURE_MODE_RED,
		TEXTURE_MODE_XYZ,
	};

private:
	mutable RID _texture;
	Ref<Curve> _curve;
	Ref<Curve> _curve_x;
	Ref<Curve> _curve_y;
	Ref<Curve> _curve_z;
	int _width = 256;
	int _current_width = 0;
	TextureMode texture_mode = TEXTURE_MODE_RGB;
	TextureMode _current_texture_mode = TEXTURE_MODE_RGB;

	void _update();

public:
	void set_width(int p_width);
	int get_width() const override;

	void set_texture_mode(TextureMode p_mode);
	TextureMode get_texture_mode() const;

	void ensure_default_setup(float p_min = 0, float p_max = 1);

	void set_curve(Ref<Curve> p_curve);
	Ref<Curve> get_curve() const;

	void set_curve_x(Ref<Curve> p_curve);
	Ref<Curve> get_curve_x() const;

	void set_curve_y(Ref<Curve> p_curve);
	Ref<Curve> get_curve_y() const;

	void set_curve_z(Ref<Curve> p_curve);
	Ref<Curve> get_curve_z() const;

	virtual RID get_rid() const override;

	virtual int get_height() const override { return 1; }

	virtual bool has_alpha() const override { return false; }

	explicit CurveTexture(TextureMode p_mode = TEXTURE_MODE_RGB);
	~CurveTexture();
};

// --- End scene/resources/curve_texture.h ---


/*
 TODO:
-Path following
-Emitter positions deformable by bones
-Proper trails
*/

class ParticleProcessMaterial final : public Material
{
public:
	enum Parameter
	{
		PARAM_INITIAL_LINEAR_VELOCITY,
		PARAM_ANGULAR_VELOCITY,
		PARAM_ORBIT_VELOCITY,
		PARAM_LINEAR_ACCEL,
		PARAM_RADIAL_ACCEL,
		PARAM_TANGENTIAL_ACCEL,
		PARAM_DAMPING,
		PARAM_ANGLE,
		PARAM_SCALE,
		PARAM_HUE_VARIATION,
		PARAM_ANIM_SPEED,
		PARAM_ANIM_OFFSET,
		PARAM_TURB_INFLUENCE_OVER_LIFE,
		PARAM_TURB_VEL_INFLUENCE,
		PARAM_TURB_INIT_DISPLACEMENT,
		PARAM_RADIAL_VELOCITY,
		PARAM_DIRECTIONAL_VELOCITY,
		PARAM_SCALE_OVER_VELOCITY,
		PARAM_MAX
	};

	// When extending, make sure not to overflow the size of the MaterialKey below.
	enum ParticleFlags
	{
		PARTICLE_FLAG_ALIGN_Y_TO_VELOCITY,
		PARTICLE_FLAG_ROTATE_Y,
		PARTICLE_FLAG_DISABLE_Z,
		PARTICLE_FLAG_DAMPING_AS_FRICTION,
		PARTICLE_FLAG_INHERIT_EMITTER_SCALE,
		PARTICLE_FLAG_MAX
	};

	// When extending, make sure not to overflow the size of the MaterialKey below.
	enum EmissionShape
	{
		EMISSION_SHAPE_POINT,
		EMISSION_SHAPE_SPHERE,
		EMISSION_SHAPE_SPHERE_SURFACE,
		EMISSION_SHAPE_BOX,
		EMISSION_SHAPE_POINTS,
		EMISSION_SHAPE_DIRECTED_POINTS,
		EMISSION_SHAPE_RING,
		EMISSION_SHAPE_MAX
	};

	// When extending, make sure not to overflow the size of the MaterialKey below.
	enum SubEmitterMode
	{
		SUB_EMITTER_DISABLED,
		SUB_EMITTER_CONSTANT,
		SUB_EMITTER_AT_END,
		SUB_EMITTER_AT_COLLISION,
		SUB_EMITTER_AT_START,
		SUB_EMITTER_MAX
	};

	// When extending, make sure not to overflow the size of the MaterialKey below.
	enum CollisionMode
	{
		COLLISION_DISABLED,
		COLLISION_RIGID,
		COLLISION_HIDE_ON_CONTACT,
		COLLISION_MAX
	};

private:
	struct MaterialKey
	{
		// The bit size of the struct must be kept below or equal to 64 bits.
		// Consider this when extending ParticleFlags, EmissionShape, or SubEmitterMode.
		uint64_t texture_mask : PARAM_MAX;
		uint64_t texture_color : 1;
		uint64_t particle_flags : PARTICLE_FLAG_MAX;
		uint64_t emission_shape : 3;
		uint64_t invalid_key : 1;
		uint64_t has_emission_color : 1;
		uint64_t sub_emitter : 3;
		uint64_t attractor_enabled : 1;
		uint64_t collision_mode : 2;
		uint64_t collision_scale : 1;
		uint64_t turbulence_enabled : 1;
		uint64_t limiter_curve : 1;
		uint64_t alpha_curve : 1;
		uint64_t emission_curve : 1;
		uint64_t has_initial_ramp : 1;
		uint64_t orbit_uses_curve_xyz : 1;
		uint64_t use_scale_3d : 1;
		uint64_t use_rotation_3d : 1;
		uint64_t use_rotation_velocity_3d : 1;
		uint64_t use_rotation_velocity_3d_curve : 1;

		MaterialKey() { memset(this, 0, sizeof(MaterialKey)); }

		static uint32_t hash(const MaterialKey& p_key)
		{
			return hash_djb2_buffer((const uint8_t*)&p_key, sizeof(MaterialKey));
		}

		bool operator==(const MaterialKey& p_key) const
		{
			return memcmp(this, &p_key, sizeof(MaterialKey)) == 0;
		}

		bool operator<(const MaterialKey& p_key) const
		{
			return memcmp(this, &p_key, sizeof(MaterialKey)) < 0;
		}
	};

	struct ShaderData
	{
		RID shader;
		int users = 0;
	};

	static Mutex shader_map_mutex;
	static HashMap<MaterialKey, ShaderData, MaterialKey> shader_map;
	static RBSet<String> min_max_properties;

	MaterialKey current_key;
	RID shader_rid;

	static Mutex dirty_materials_mutex;
	static SelfList<ParticleProcessMaterial>::List dirty_materials;

	struct ShaderNames
	{
		String direction;
		String spread;
		String flatness;
		String initial_linear_velocity_min;
		String initial_angle_min;
		String angular_velocity_min;
		String orbit_velocity_min;
		String radial_velocity_min;
		String linear_accel_min;
		String radial_accel_min;
		String tangent_accel_min;
		String damping_min;
		String scale_min;
		String scale_over_velocity_min;
		String hue_variation_min;
		String anim_speed_min;
		String anim_offset_min;
		String directional_velocity_min;
		String scale_3d_min;
		String rotation_3d_min;
		String rotation_velocity_3d_min;

		String initial_linear_velocity_max;
		String initial_angle_max;
		String angular_velocity_max;
		String orbit_velocity_max;
		String radial_velocity_max;
		String linear_accel_max;
		String radial_accel_max;
		String tangent_accel_max;
		String damping_max;
		String scale_max;
		String scale_over_velocity_max;
		String hue_variation_max;
		String anim_speed_max;
		String anim_offset_max;
		String directional_velocity_max;
		String scale_3d_max;
		String rotation_3d_max;
		String rotation_velocity_3d_max;

		String angle_texture;
		String angular_velocity_texture;
		String orbit_velocity_texture;
		String radial_velocity_texture;
		String linear_accel_texture;
		String radial_accel_texture;
		String tangent_accel_texture;
		String damping_texture;
		String scale_texture;
		String scale_over_velocity_texture;
		String hue_variation_texture;
		String anim_speed_texture;
		String anim_offset_texture;
		String velocity_limiter_texture;
		String directional_velocity_texture;
		String rotation_velocity_3d_curve;

		String color;
		String color_ramp;
		String alpha_ramp;
		String emission_ramp;
		String color_initial_ramp;

		String velocity_limit_curve;
		String velocity_pivot;

		String emission_sphere_radius;
		String emission_box_extents;
		String emission_texture_point_count;
		String emission_texture_points;
		String emission_texture_normal;
		String emission_texture_color;
		String emission_ring_axis;
		String emission_ring_height;
		String emission_ring_radius;
		String emission_ring_inner_radius;
		String emission_ring_cone_angle;
		String emission_shape_offset;
		String emission_shape_scale;

		String turbulence_enabled;
		String turbulence_noise_strength;
		String turbulence_noise_scale;
		String turbulence_noise_speed;
		String turbulence_noise_speed_random;
		String turbulence_influence_over_life;
		String turbulence_influence_min;
		String turbulence_influence_max;
		String turbulence_initial_displacement_min;
		String turbulence_initial_displacement_max;

		String gravity;
		String inherit_emitter_velocity_ratio;

		String lifetime_randomness;

		String sub_emitter_frequency;
		String sub_emitter_amount_at_end;
		String sub_emitter_amount_at_collision;
		String sub_emitter_amount_at_start;
		String sub_emitter_keep_velocity;

		String collision_friction;
		String collision_bounce;
	};

	static ShaderNames* shader_names;

	SelfList<ParticleProcessMaterial> element;

	void _update_shader();
	_FORCE_INLINE_ void _queue_shader_change();

	Vector3 direction;
	float spread = 0.0f;
	float flatness = 0.0f;

	float params_min[PARAM_MAX] = {};
	float params_max[PARAM_MAX] = {};
	float params[PARAM_MAX] = {};

	Ref<Texture2D> tex_parameters[PARAM_MAX];
	Color color;
	Ref<Texture2D> color_ramp;
	Ref<Texture2D> alpha_curve;
	Ref<Texture2D> emission_curve;
	Ref<Texture2D> color_initial_ramp;
	Ref<Texture2D> velocity_limit_curve;

	bool directional_velocity_global = false;
	Vector3 velocity_pivot;

	bool particle_flags[PARTICLE_FLAG_MAX];

	EmissionShape emission_shape;
	float emission_sphere_radius = 0.0f;
	Vector3 emission_box_extents;
	Ref<Texture2D> emission_point_texture;
	Ref<Texture2D> emission_normal_texture;
	Ref<Texture2D> emission_color_texture;
	Vector3 emission_ring_axis;
	real_t emission_ring_height = 0.0f;
	real_t emission_ring_radius = 0.0f;
	real_t emission_ring_inner_radius = 0.0f;
	real_t emission_ring_cone_angle = 0.0f;
	int emission_point_count = 1;
	Vector3 emission_shape_offset;
	Vector3 emission_shape_scale;

	bool turbulence_enabled;
	Vector3 turbulence_noise_speed;
	Ref<Texture2D> turbulence_color_ramp;
	float turbulence_noise_strength = 0.0f;
	float turbulence_noise_scale = 0.0f;
	float turbulence_noise_speed_random = 0.0f;

	Vector3 gravity;

	double lifetime_randomness = 0.0;
	double inherit_emitter_velocity_ratio = 0.0;

	bool use_rotation_3d = false;
	bool use_scale_3d = false;
	Vector3 scale_3d_min;
	Vector3 scale_3d_max;
	Vector3 rotation_3d_min;
	Vector3 rotation_3d_max;
	bool using_rotation_velocity_3d = false;
	Vector3 rotation_velocity_3d_min;
	Vector3 rotation_velocity_3d_max;
	Ref<Texture2D> rotation_velocity_3d_curve;

	SubEmitterMode sub_emitter_mode;
	double sub_emitter_frequency = 0.0;
	int sub_emitter_amount_at_end = 0;
	int sub_emitter_amount_at_collision = 0;
	int sub_emitter_amount_at_start = 0;
	bool sub_emitter_keep_velocity = false;
	// do not save emission points here

	bool attractor_interaction_enabled = false;
	CollisionMode collision_mode;
	bool collision_scale = false;
	float collision_friction = 0.0f;
	float collision_bounce = 0.0f;

public:
	static bool has_min_max_property(const String& p_name);

	void set_direction(Vector3 p_direction);
	Vector3 get_direction() const;

	void set_spread(float p_spread);
	float get_spread() const;

	void set_flatness(float p_flatness);
	float get_flatness() const;

	void set_velocity_pivot(const Vector3& p_pivot);
	Vector3 get_velocity_pivot();

	void set_param(Parameter p_param, const Vector2& p_value);
	Vector2 get_param(Parameter p_param) const;

	void set_param_min(Parameter p_param, float p_value);
	float get_param_min(Parameter p_param) const;

	void set_param_max(Parameter p_param, float p_value);
	float get_param_max(Parameter p_param) const;

	void set_param_texture(Parameter p_param, const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_param_texture(Parameter p_param) const;

	void set_velocity_limit_curve(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_velocity_limit_curve() const;

	void set_alpha_curve(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_alpha_curve() const;
	void set_color(const Color& p_color);
	Color get_color() const;

	void set_color_ramp(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_color_ramp() const;

	void set_color_initial_ramp(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_color_initial_ramp() const;

	void set_emission_curve(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_emission_curve() const;

	void set_use_scale_3d(const bool p_use_scale_3d);
	bool is_using_scale_3d() const;

	void set_scale_3d_min(const Vector3& p_scale_3d_min);
	Vector3 get_scale_3d_min() const;

	void set_scale_3d_max(const Vector3& p_scale_3d_max);
	Vector3 get_scale_3d_max() const;

	void set_use_rotation_3d(const bool p_use_scale_3d);
	bool is_using_rotation_3d() const;

	void set_rotation_3d_min(const Vector3& p_rotation_3d_min);
	Vector3 get_rotation_3d_min() const;

	void set_rotation_3d_max(const Vector3& p_rotation_3d_max);
	Vector3 get_rotation_3d_max() const;

	void set_particle_flag(ParticleFlags p_particle_flag, bool p_enable);
	bool get_particle_flag(ParticleFlags p_particle_flag) const;

	void set_emission_shape(EmissionShape p_shape);
	void set_emission_sphere_radius(real_t p_radius);
	void set_emission_box_extents(Vector3 p_extents);
	void set_emission_point_texture(const Ref<Texture2D>& p_points);
	void set_emission_normal_texture(const Ref<Texture2D>& p_normals);
	void set_emission_color_texture(const Ref<Texture2D>& p_colors);
	void set_emission_ring_axis(Vector3 p_axis);
	void set_emission_ring_height(real_t p_height);
	void set_emission_ring_radius(real_t p_radius);
	void set_emission_ring_inner_radius(real_t p_radius);
	void set_emission_ring_cone_angle(real_t p_angle);
	void set_emission_point_count(int p_count);

	EmissionShape get_emission_shape() const;
	real_t get_emission_sphere_radius() const;
	Vector3 get_emission_box_extents() const;
	Ref<Texture2D> get_emission_point_texture() const;
	Ref<Texture2D> get_emission_normal_texture() const;
	Ref<Texture2D> get_emission_color_texture() const;
	Vector3 get_emission_ring_axis() const;
	real_t get_emission_ring_height() const;
	real_t get_emission_ring_radius() const;
	real_t get_emission_ring_inner_radius() const;
	real_t get_emission_ring_cone_angle() const;
	int get_emission_point_count() const;

	void set_turbulence_enabled(bool p_turbulence_enabled);
	void set_turbulence_noise_strength(float p_turbulence_noise_strength);
	void set_turbulence_noise_scale(float p_turbulence_noise_scale);
	void set_turbulence_noise_speed_random(float p_turbulence_noise_speed_random);
	void set_turbulence_noise_speed(const Vector3& p_turbulence_noise_speed);

	bool get_turbulence_enabled() const;
	float get_turbulence_noise_strength() const;
	float get_turbulence_noise_scale() const;
	float get_turbulence_noise_speed_random() const;
	Vector3 get_turbulence_noise_speed() const;

	void set_gravity(const Vector3& p_gravity);
	Vector3 get_gravity() const;

	void set_lifetime_randomness(double p_lifetime);
	double get_lifetime_randomness() const;

	void set_inherit_velocity_ratio(double p_ratio);
	double get_inherit_velocity_ratio();

	void set_use_rotation_velocity_3d(bool p_use_rotation_velocity_3d);
	bool is_using_rotation_velocity_3d() const;
	void set_rotation_velocity_3d_min(const Vector3& p_rotation_velocity_3d_min);
	Vector3 get_rotation_velocity_3d_min() const;
	void set_rotation_velocity_3d_max(const Vector3& p_rotation_velocity_3d_max);
	Vector3 get_rotation_velocity_3d_max() const;
	void set_rotation_velocity_3d_curve(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_rotation_velocity_3d_curve() const;

	void set_attractor_interaction_enabled(bool p_enable);
	bool is_attractor_interaction_enabled() const;

	void set_collision_mode(CollisionMode p_collision_mode);
	CollisionMode get_collision_mode() const;

	void set_collision_use_scale(bool p_scale);
	bool is_collision_using_scale() const;

	void set_collision_friction(float p_friction);
	float get_collision_friction() const;

	void set_collision_bounce(float p_bounce);
	float get_collision_bounce() const;

	static void init_shaders();
	static void finish_shaders();
	static void flush_changes();

	void set_sub_emitter_mode(SubEmitterMode p_sub_emitter_mode);
	SubEmitterMode get_sub_emitter_mode() const;

	void set_sub_emitter_frequency(double p_frequency);
	double get_sub_emitter_frequency() const;

	void set_sub_emitter_amount_at_end(int p_amount);
	int get_sub_emitter_amount_at_end() const;

	void set_sub_emitter_amount_at_collision(int p_amount);
	int get_sub_emitter_amount_at_collision() const;

	void set_sub_emitter_amount_at_start(int p_amount);
	int get_sub_emitter_amount_at_start() const;

	void set_sub_emitter_keep_velocity(bool p_enable);
	bool get_sub_emitter_keep_velocity() const;

	void set_emission_shape_offset(const Vector3& p_emission_shape_offset);
	Vector3 get_emission_shape_offset() const;

	void set_emission_shape_scale(const Vector3& p_emission_shape_scale);
	Vector3 get_emission_shape_scale() const;

	virtual RID get_rid() const override;
	virtual RID get_shader_rid() const override;

	virtual Shader::Mode get_shader_mode() const override;

	ParticleProcessMaterial();
	~ParticleProcessMaterial();
};




// --- End scene/resources/particle_process_material.h ---

// --- Folded from scene/resources/camera_attributes.h ---
#include "core/io/resource.h"
#include "core/templates/rid.h"

class CameraAttributes : public Resource
{
private:
	RID camera_attributes;

protected:

	float exposure_multiplier = 1.0;
	float exposure_sensitivity = 100.0; // In ISO.
	void _update_exposure();

	bool auto_exposure_enabled = false;
	float auto_exposure_min = 0.01;
	float auto_exposure_max = 64.0;
	float auto_exposure_speed = 0.5;
	float auto_exposure_scale = 0.4;

	virtual void _update_auto_exposure() {}

public:
	virtual RID get_rid() const override;

	virtual float calculate_exposure_normalization() const { return 1.0; }

	void set_exposure_multiplier(float p_multiplier);
	float get_exposure_multiplier() const;
	void set_exposure_sensitivity(float p_sensitivity);
	float get_exposure_sensitivity() const;

	void set_auto_exposure_enabled(bool p_enabled);
	bool is_auto_exposure_enabled() const;
	void set_auto_exposure_speed(float p_auto_exposure_speed);
	float get_auto_exposure_speed() const;
	void set_auto_exposure_scale(float p_auto_exposure_scale);
	float get_auto_exposure_scale() const;

	CameraAttributes();
	~CameraAttributes();
};

class CameraAttributesPractical : public CameraAttributes
{
private:
	// DOF blur
	bool dof_blur_far_enabled = false;
	float dof_blur_far_distance = 10.0;
	float dof_blur_far_transition = 5.0;

	bool dof_blur_near_enabled = false;
	float dof_blur_near_distance = 2.0;
	float dof_blur_near_transition = 1.0;

	float dof_blur_amount = 0.1;
	void _update_dof_blur();

	virtual void _update_auto_exposure() override;

protected:

public:
	// DOF blur
	void set_dof_blur_far_enabled(bool p_enabled);
	bool is_dof_blur_far_enabled() const;
	void set_dof_blur_far_distance(float p_distance);
	float get_dof_blur_far_distance() const;
	void set_dof_blur_far_transition(float p_distance);
	float get_dof_blur_far_transition() const;

	void set_dof_blur_near_enabled(bool p_enabled);
	bool is_dof_blur_near_enabled() const;
	void set_dof_blur_near_distance(float p_distance);
	float get_dof_blur_near_distance() const;
	void set_dof_blur_near_transition(float p_distance);
	float get_dof_blur_near_transition() const;
	void set_dof_blur_amount(float p_amount);
	float get_dof_blur_amount() const;

	void set_auto_exposure_min_sensitivity(float p_min);
	float get_auto_exposure_min_sensitivity() const;
	void set_auto_exposure_max_sensitivity(float p_max);
	float get_auto_exposure_max_sensitivity() const;

	virtual float calculate_exposure_normalization() const override;

	CameraAttributesPractical();
	~CameraAttributesPractical();
};

class CameraAttributesPhysical : public CameraAttributes
{
private:
	// Exposure
	float exposure_aperture = 16.0;		  // In f-stops;
	float exposure_shutter_speed = 100.0; // In 1 / seconds;

	// Camera properties.
	float frustum_focal_length = 35.0;	 // In millimeters.
	float frustum_focus_distance = 10.0; // In Meters.
	real_t frustum_near = 0.05;
	real_t frustum_far = 4000.0;
	real_t frustum_fov = 75.0;
	void _update_frustum();

	virtual void _update_auto_exposure() override;

protected:

public:
	void set_aperture(float p_aperture);
	float get_aperture() const;

	void set_shutter_speed(float p_shutter_speed);
	float get_shutter_speed() const;

	void set_focal_length(float p_focal_length);
	float get_focal_length() const;

	void set_focus_distance(float p_focus_distance);
	float get_focus_distance() const;

	void set_near(real_t p_near);
	real_t get_near() const;

	void set_far(real_t p_far);
	real_t get_far() const;

	real_t get_fov() const;

	void set_auto_exposure_min_exposure_value(float p_min);
	float get_auto_exposure_min_exposure_value() const;
	void set_auto_exposure_max_exposure_value(float p_max);
	float get_auto_exposure_max_exposure_value() const;

	virtual float calculate_exposure_normalization() const override;

	CameraAttributesPhysical();
	~CameraAttributesPhysical();
};



// --- End scene/resources/camera_attributes.h ---
