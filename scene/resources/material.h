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
#include "core/types.h"
// --- Folded from scene/resources/shader.h ---
#include "core/io/resource.h"
// --- Folded from scene/resources/shader_include.h ---
#include "core/io/resource.h"
#include "core/templates/hash_set.h"

class ShaderInclude : public Resource
{
private:
	String code;
	String include_path;
	HashSet<Ref<ShaderInclude>> dependencies;
	void _dependency_changed();

protected:

public:
	void set_code(const String& p_text);
	String get_code() const;

	void set_include_path(const String& p_path);
};



// --- End scene/resources/shader_include.h ---

using Texture = Resource;
class Texture2D;

class Shader : public Resource
{
public:
	// Must be kept in sync with the List<String> of shader types in
	// `servers/rendering/shader_types.cpp`.
	enum Mode
	{
		MODE_SPATIAL,
		MODE_CANVAS_ITEM,
		MODE_PARTICLES,
		MODE_SKY,
		MODE_FOG,
		MODE_TEXTURE_BLIT,
		MODE_MAX
	};

private:
	mutable RID shader_rid;
	mutable String preprocessed_code;
	mutable Mutex shader_rid_mutex;

	Mode mode = MODE_SPATIAL;
	HashSet<Ref<ShaderInclude>> include_dependencies;
	String code;
	String include_path;

	HashMap<String, HashMap<int, Ref<Texture>>> default_textures;

	void _check_shader_rid() const;
	void _dependency_changed();
	void _recompile();
	virtual void _update_shader() const; // used for visual shader

protected:
#ifndef DISABLE_DEPRECATED
	void _set_default_texture_parameter_bind_compat_95126(
		const String& p_name, const Ref<Texture2D>& p_texture, int p_index = 0);
	Ref<Texture2D> _get_default_texture_parameter_bind_compat_95126(
		const String& p_name, int p_index = 0) const;
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED


public:
	// void set_mode(Mode p_mode);
	virtual Mode get_mode() const;

	virtual void set_path(const String& p_path, bool p_take_over = false) override;
	void set_include_path(const String& p_path);

	void set_code(const String& p_code);
	String get_code() const;

	void inspect_native_shader_code();


	void set_default_texture_parameter(
		const String& p_name, const Ref<Texture>& p_texture, int p_index = 0);
	Ref<Texture> get_default_texture_parameter(const String& p_name, int p_index = 0) const;
	void get_default_texture_parameter_list(List<String>* r_textures) const;

	virtual bool is_text_shader() const;

	virtual RID get_rid() const override;

	Shader();
	~Shader();
};

// --- Folded from scene/resources/shader_resource_format.h ---
#include "core/io/resource_loader.h"

class ResourceFormatLoaderShader final : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
};

class ResourceFormatSaverShader final : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
};

// --- End scene/resources/shader_resource_format.h ---
// --- End scene/resources/shader.h ---
#include "scene/resources/texture.h"

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
#include "core/templates/hashfuncs.h"
#include "core/types.h"
// --- Folded from scene/resources/curve_texture.h ---
// --- Folded from scene/resources/curve.h ---
#include "core/io/resource.h"
#include "core/templates/hash_map.h"

// y(x) curve
class Curve : public Resource
{
public:
	static const char* SIGNAL_RANGE_CHANGED;
	static const char* SIGNAL_DOMAIN_CHANGED;

	enum TangentMode
	{
		TANGENT_FREE = 0,
		TANGENT_LINEAR,
		TANGENT_MODE_COUNT
	};

	struct Point
	{
		Vector2 position;
		real_t left_tangent = 0.0;
		real_t right_tangent = 0.0;
		TangentMode left_mode = TANGENT_FREE;
		TangentMode right_mode = TANGENT_FREE;

		Point() {}

		Point(const Vector2& p_position, real_t p_left = 0.0, real_t p_right = 0.0,
			TangentMode p_left_mode = TANGENT_FREE, TangentMode p_right_mode = TANGENT_FREE)
		{
			position = p_position;
			left_tangent = p_left;
			right_tangent = p_right;
			left_mode = p_left_mode;
			right_mode = p_right_mode;
		}
	};

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	Curve() = default;

	int get_point_count() const { return _points.size(); }

	void set_point_count(int p_count);

	int add_point(Vector2 p_position, real_t p_left_tangent = 0, real_t p_right_tangent = 0,
		TangentMode p_left_mode = TANGENT_FREE, TangentMode p_right_mode = TANGENT_FREE);
	int add_point_no_update(Vector2 p_position, real_t p_left_tangent = 0,
		real_t p_right_tangent = 0, TangentMode p_left_mode = TANGENT_FREE,
		TangentMode p_right_mode = TANGENT_FREE);
	void remove_point(int p_index);
	void clear_points();

	int get_index(real_t p_offset) const;

	void set_point_value(int p_index, real_t p_position);
	int set_point_offset(int p_index, real_t p_offset);
	Vector2 get_point_position(int p_index) const;

	Point get_point(int p_index) const;

	real_t get_min_value() const { return _min_value; }

	void set_min_value(real_t p_min) {}

	real_t get_max_value() const { return _max_value; }

	void set_max_value(real_t p_max) {}

	real_t get_value_range() const { return _max_value - _min_value; }

	real_t get_min_domain() const { return _min_domain; }

	void set_min_domain(real_t p_min);

	real_t get_max_domain() const { return _max_domain; }

	void set_max_domain(real_t p_max);

	real_t get_domain_range() const { return _max_domain - _min_domain; }

	real_t sample(real_t p_offset) const;
	real_t sample_local_nocheck(int p_index, real_t p_local_offset) const;

	void clean_dupes();

	void set_point_left_tangent(int p_index, real_t p_tangent);
	void set_point_right_tangent(int p_index, real_t p_tangent);
	void set_point_left_mode(int p_index, TangentMode p_mode);
	void set_point_right_mode(int p_index, TangentMode p_mode);

	real_t get_point_left_tangent(int p_index) const;
	real_t get_point_right_tangent(int p_index) const;
	TangentMode get_point_left_mode(int p_index) const;
	TangentMode get_point_right_mode(int p_index) const;

	void update_auto_tangents(int p_index);

	void bake();
	void _bake() const;

	int get_bake_resolution() const { return _bake_resolution; }

	void set_bake_resolution(int p_resolution);
	real_t sample_baked(real_t p_offset) const;

	void ensure_default_setup(real_t p_min, real_t p_max);

protected:
	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}


private:
	bool _filter_property(const String& p_name, int p_index) const;

	void mark_dirty();
	int _add_point(Vector2 p_position, real_t p_left_tangent = 0, real_t p_right_tangent = 0,
		TangentMode p_left_mode = TANGENT_FREE, TangentMode p_right_mode = TANGENT_FREE,
		bool p_mark_dirty = true);
	void _remove_point(int p_index, bool p_mark_dirty = true);
	void _set_point_position(int p_index, const Vector2& p_position);

	LocalVector<Point> _points;
	mutable bool _baked_cache_dirty = false;
	mutable Vector<real_t> _baked_cache;
	int _bake_resolution = 100;
	real_t _min_value = 0.0;
	real_t _max_value = 1.0;
	real_t _min_domain = 0.0;
	real_t _max_domain = 1.0;
};

class Curve2D : public Resource
{
	struct Point
	{
		Vector2 in;
		Vector2 out;
		Vector2 position;
	};

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	LocalVector<Point> points;

	struct BakedPoint
	{
		real_t ofs = 0.0;
		Vector2 point;
	};

	mutable bool baked_cache_dirty = false;
	mutable PackedVector2Array baked_point_cache;
	mutable PackedVector2Array baked_forward_vector_cache;
	mutable Vector<real_t> baked_dist_cache;
	mutable real_t baked_max_ofs = 0.0;

	void mark_dirty();

	static Vector2 _calculate_tangent(const Vector2& p_begin, const Vector2& p_control_1,
		const Vector2& p_control_2, const Vector2& p_end, const real_t p_t);
	void _bake() const;

	real_t bake_interval = 5.0;

	struct Interval
	{
		int idx;
		real_t frac;
	};

	Interval _find_interval(real_t p_offset) const;
	Vector2 _sample_baked(Interval p_interval, bool p_cubic) const;
	Transform2D _sample_posture(Interval p_interval) const;

	void _bake_segment2d(RBMap<real_t, Vector2>& r_bake, real_t p_begin, real_t p_end,
		const Vector2& p_a, const Vector2& p_out, const Vector2& p_b, const Vector2& p_in,
		int p_depth, int p_max_depth, real_t p_tol) const;
	void _bake_segment2d_even_length(RBMap<real_t, Vector2>& r_bake, real_t p_begin, real_t p_end,
		const Vector2& p_a, const Vector2& p_out, const Vector2& p_b, const Vector2& p_in,
		int p_depth, int p_max_depth, real_t p_length) const;

	void _add_point(const Vector2& p_position, const Vector2& p_in = Vector2(),
		const Vector2& p_out = Vector2(), int p_atpos = -1);
	void _remove_point(int p_index);

	Vector<RBMap<real_t, Vector2>> _tessellate_even_length(
		int p_max_stages = 5, real_t p_length = 0.2) const;
	bool _filter_property(const String& p_name, int p_index) const;

protected:
	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}


public:
	int get_point_count() const;
	void set_point_count(int p_count);
	void add_point(const Vector2& p_position, const Vector2& p_in = Vector2(),
		const Vector2& p_out = Vector2(), int p_atpos = -1);
	void set_point_position(int p_index, const Vector2& p_position);
	Vector2 get_point_position(int p_index) const;
	void set_point_in(int p_index, const Vector2& p_in);
	Vector2 get_point_in(int p_index) const;
	void set_point_out(int p_index, const Vector2& p_out);
	Vector2 get_point_out(int p_index) const;
	void remove_point(int p_index);
	void clear_points();

	Vector2 sample(int p_index, real_t p_offset) const;
	Vector2 samplef(real_t p_findex) const;

	void set_bake_interval(real_t p_tolerance);
	real_t get_bake_interval() const;

	real_t get_baked_length() const;
	Vector2 sample_baked(real_t p_offset, bool p_cubic = false) const;
	Transform2D sample_baked_with_rotation(real_t p_offset, bool p_cubic = false) const;
	PackedVector2Array get_points() const;
	PackedVector2Array get_baked_points() const; // useful for going through
	Vector2 get_closest_point(const Vector2& p_to_point) const;
	real_t get_closest_offset(const Vector2& p_to_point) const;

	PackedVector2Array tessellate(
		int p_max_stages = 5, real_t p_tolerance = 4) const; // useful for display
	PackedVector2Array tessellate_even_length(
		int p_max_stages = 5, real_t p_length = 20.0) const; // Useful for baking.

	Curve2D();
};

class Curve3D : public Resource
{
	struct Point
	{
		Vector3 in;
		Vector3 out;
		Vector3 position;
		real_t tilt = 0.0;
	};

	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;

	LocalVector<Point> points;
#ifdef TOOLS_ENABLED
	// For Path3DGizmo.
	mutable Vector<size_t> points_in_cache;
#endif

	bool closed = false;

	mutable bool baked_cache_dirty = false;
	mutable PackedVector3Array baked_point_cache;
	mutable Vector<real_t> baked_tilt_cache;
	mutable PackedVector3Array baked_up_vector_cache;
	mutable PackedVector3Array baked_forward_vector_cache;
	mutable Vector<real_t> baked_dist_cache;
	mutable real_t baked_max_ofs = 0.0;

	void mark_dirty();

	static Vector3 _calculate_tangent(const Vector3& p_begin, const Vector3& p_control_1,
		const Vector3& p_control_2, const Vector3& p_end, const real_t p_t);
	void _bake() const;

	struct Interval
	{
		int idx;
		real_t frac;
	};

	Interval _find_interval(real_t p_offset) const;
	Vector3 _sample_baked(Interval p_interval, bool p_cubic) const;
	real_t _sample_baked_tilt(Interval p_interval) const;
	Basis _sample_posture(Interval p_interval, bool p_apply_tilt = false) const;
	Basis _compose_posture(int p_index) const;

	real_t bake_interval = 0.2;
	bool up_vector_enabled = true;

	void _bake_segment3d(RBMap<real_t, Vector3>& r_bake, real_t p_begin, real_t p_end,
		const Vector3& p_a, const Vector3& p_out, const Vector3& p_b, const Vector3& p_in,
		int p_depth, int p_max_depth, real_t p_tol) const;
	void _bake_segment3d_even_length(RBMap<real_t, Vector3>& r_bake, real_t p_begin, real_t p_end,
		const Vector3& p_a, const Vector3& p_out, const Vector3& p_b, const Vector3& p_in,
		int p_depth, int p_max_depth, real_t p_length) const;

	void _add_point(const Vector3& p_position, const Vector3& p_in = Vector3(),
		const Vector3& p_out = Vector3(), int p_atpos = -1);
	void _remove_point(int p_index);

	Vector<RBMap<real_t, Vector3>> _tessellate_even_length(
		int p_max_stages = 5, real_t p_length = 0.2) const;
	bool _filter_property(const String& p_name, int p_index) const;

protected:
	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}


public:
#ifdef TOOLS_ENABLED
	// For Path3DGizmo.
	Basis get_point_baked_posture(int p_index, bool p_apply_tilt = false) const;
#endif

	int get_point_count() const;
	void set_point_count(int p_count);
	void add_point(const Vector3& p_position, const Vector3& p_in = Vector3(),
		const Vector3& p_out = Vector3(), int p_atpos = -1);
	void set_point_position(int p_index, const Vector3& p_position);
	Vector3 get_point_position(int p_index) const;
	void set_point_tilt(int p_index, real_t p_tilt);
	real_t get_point_tilt(int p_index) const;
	void set_point_in(int p_index, const Vector3& p_in);
	Vector3 get_point_in(int p_index) const;
	void set_point_out(int p_index, const Vector3& p_out);
	Vector3 get_point_out(int p_index) const;
	void remove_point(int p_index);
	void clear_points();

	Vector3 sample(int p_index, real_t p_offset) const;
	Vector3 samplef(real_t p_findex) const;

	void set_closed(bool p_closed);
	bool is_closed() const;
	void set_bake_interval(real_t p_tolerance);
	real_t get_bake_interval() const;
	void set_up_vector_enabled(bool p_enable);
	bool is_up_vector_enabled() const;

	real_t get_baked_length() const;
	Vector3 sample_baked(real_t p_offset, bool p_cubic = false) const;
	Transform3D sample_baked_with_rotation(
		real_t p_offset, bool p_cubic = false, bool p_apply_tilt = false) const;
	real_t sample_baked_tilt(real_t p_offset) const;
	Vector3 sample_baked_up_vector(real_t p_offset, bool p_apply_tilt = false) const;
	PackedVector3Array get_baked_points() const; // Useful for going through.
	Vector<real_t> get_baked_tilts() const;		 // useful for going through
	PackedVector3Array get_baked_up_vectors() const;
	Vector<real_t> get_baked_dist_cache() const;
	Vector3 get_closest_point(const Vector3& p_to_point) const;
	real_t get_closest_offset(const Vector3& p_to_point) const;
	PackedVector3Array get_points() const;

	PackedVector3Array tessellate(
		int p_max_stages = 5, real_t p_tolerance = 4) const; // Useful for display.
	PackedVector3Array tessellate_even_length(
		int p_max_stages = 5, real_t p_length = 0.2) const; // Useful for baking.

	Curve3D();
};
// --- End scene/resources/curve.h ---
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
#include "core/templates/hashfuncs.h"

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

// --- Folded from scene/resources/physics_material.h ---
#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
#include "core/io/resource.h"

class PhysicsMaterial : public Resource
{
	real_t friction = 1.0;
	bool rough = false;
	real_t bounce = 0.0;
	bool absorbent = false;

protected:

public:
	void set_friction(real_t p_val);

	_FORCE_INLINE_ real_t get_friction() const { return friction; }

	void set_rough(bool p_val);

	_FORCE_INLINE_ bool is_rough() const { return rough; }

	_FORCE_INLINE_ real_t computed_friction() const { return rough ? -friction : friction; }

	void set_bounce(real_t p_val);

	_FORCE_INLINE_ real_t get_bounce() const { return bounce; }

	void set_absorbent(bool p_val);

	_FORCE_INLINE_ bool is_absorbent() const { return absorbent; }

	_FORCE_INLINE_ real_t computed_bounce() const { return absorbent ? -bounce : bounce; }
};
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
// --- End scene/resources/physics_material.h ---

// --- Folded from scene/resources/gradient.h ---
#include <thirdparty/misc/ok_color.h>
#include "core/io/resource.h"

class Gradient : public Resource
{
public:
	enum InterpolationMode
	{
		GRADIENT_INTERPOLATE_LINEAR,
		GRADIENT_INTERPOLATE_CONSTANT,
		GRADIENT_INTERPOLATE_CUBIC,
	};

	enum ColorSpace
	{
		GRADIENT_COLOR_SPACE_SRGB,
		GRADIENT_COLOR_SPACE_LINEAR_SRGB,
		GRADIENT_COLOR_SPACE_OKLAB,
	};

	struct Point
	{
		float offset = 0.0;
		Color color;

		bool operator<(const Point& p_point) const { return offset < p_point.offset; }
	};

private:
	LocalVector<Point> points;
	bool is_sorted = true;
	InterpolationMode interpolation_mode = GRADIENT_INTERPOLATE_LINEAR;
	ColorSpace interpolation_color_space = GRADIENT_COLOR_SPACE_SRGB;

	_FORCE_INLINE_ void _update_sorting()
	{
		if (!is_sorted) {
			points.sort();
			is_sorted = true;
		}
	}

	_FORCE_INLINE_ Color transform_color_space(const Color p_color) const
	{
		switch (interpolation_color_space) {
		case GRADIENT_COLOR_SPACE_SRGB:
		default:
			return p_color;

		case GRADIENT_COLOR_SPACE_LINEAR_SRGB:
			return p_color.srgb_to_linear();

		case GRADIENT_COLOR_SPACE_OKLAB:
			Color linear_color = p_color.srgb_to_linear();
			ok_color::RGB rgb{};
			rgb.r = linear_color.r;
			rgb.g = linear_color.g;
			rgb.b = linear_color.b;

			ok_color ok_color;
			ok_color::Lab lab_color = ok_color.linear_srgb_to_oklab(rgb);

			// Constructs an RGB color using the Lab values directly. This allows reusing the
			// interpolation code.
			return {lab_color.L, lab_color.a, lab_color.b, linear_color.a};
		}
	}

	_FORCE_INLINE_ Color inv_transform_color_space(const Color p_color) const
	{
		switch (interpolation_color_space) {
		case GRADIENT_COLOR_SPACE_SRGB:
		default:
			return p_color;

		case GRADIENT_COLOR_SPACE_LINEAR_SRGB:
			return p_color.linear_to_srgb();

		case GRADIENT_COLOR_SPACE_OKLAB:
			ok_color::Lab lab{};
			lab.L = p_color.r;
			lab.a = p_color.g;
			lab.b = p_color.b;

			ok_color new_ok_color;
			ok_color::RGB ok_rgb = new_ok_color.oklab_to_linear_srgb(lab);
			Color linear{ok_rgb.r, ok_rgb.g, ok_rgb.b, p_color.a};
			return linear.linear_to_srgb();
		}
	}

protected:

public:
	Gradient();
	virtual ~Gradient();

	void add_point(float p_offset, const Color& p_color);
	void remove_point(int p_index);
	void reverse();

	void set_offset(int pos, const float offset);
	float get_offset(int pos);

	void set_color(int pos, const Color& color);
	Color get_color(int pos);

	void set_offsets(const Vector<float>& p_offsets);
	Vector<float> get_offsets() const;

	void set_colors(const Vector<Color>& p_colors);
	Vector<Color> get_colors() const;

	void set_interpolation_mode(InterpolationMode p_interp_mode);
	InterpolationMode get_interpolation_mode();

	void set_interpolation_color_space(Gradient::ColorSpace p_color_space);
	ColorSpace get_interpolation_color_space();

	_FORCE_INLINE_ Color get_color_at_offset(float p_offset)
	{
		if (points.is_empty()) {
			return Color(0, 0, 0, 1);
		}

		_update_sorting();

		// Binary search.
		int low = 0;
		int high = points.size() - 1;
		int middle = 0;

#ifdef DEBUG_ENABLED
		if (low > high) {
			ERR_PRINT("low > high, this may be a bug");
		}
#endif

		while (low <= high) {
			middle = (low + high) / 2;
			const Point& point = points[middle];
			if (point.offset > p_offset) {
				high = middle - 1; // search low end of array
			}
			else if (point.offset < p_offset) {
				low = middle + 1; // search high end of array
			}
			else {
				return point.color;
			}
		}

		// Return sampled value.
		if (points[middle].offset > p_offset) {
			middle--;
		}
		int first = middle;
		int second = middle + 1;
		if (second >= (int)points.size()) {
			return points[points.size() - 1].color;
		}
		if (first < 0) {
			return points[0].color;
		}
		const Point& point1 = points[first];
		const Point& point2 = points[second];
		float weight = (p_offset - point1.offset) / (point2.offset - point1.offset);

		switch (interpolation_mode) {
		case GRADIENT_INTERPOLATE_CONSTANT: {
			return point1.color;
		}
		case GRADIENT_INTERPOLATE_LINEAR:
		default: { // Fallback to linear interpolation.
			Color color1 = transform_color_space(point1.color);
			Color color2 = transform_color_space(point2.color);

			Color interpolated = color1.lerp(color2, weight);
			return inv_transform_color_space(interpolated);
		}
		case GRADIENT_INTERPOLATE_CUBIC: {
			int p0 = first - 1;
			int p3 = second + 1;
			if (p3 >= (int)points.size()) {
				p3 = second;
			}
			if (p0 < 0) {
				p0 = first;
			}
			const Point& point0 = points[p0];
			const Point& point3 = points[p3];

			Color color0 = transform_color_space(point0.color);
			Color color1 = transform_color_space(point1.color);
			Color color2 = transform_color_space(point2.color);
			Color color3 = transform_color_space(point3.color);

			Color interpolated;
			interpolated[0] =
				Math::cubic_interpolate(color1[0], color2[0], color0[0], color3[0], weight);
			interpolated[1] =
				Math::cubic_interpolate(color1[1], color2[1], color0[1], color3[1], weight);
			interpolated[2] =
				Math::cubic_interpolate(color1[2], color2[2], color0[2], color3[2], weight);
			interpolated[3] =
				Math::cubic_interpolate(color1[3], color2[3], color0[3], color3[3], weight);

			return inv_transform_color_space(interpolated);
		}
		}
	}

	int get_point_count() const;
};

// --- Folded from scene/resources/gradient_texture.h ---
#include "scene/resources/texture.h"

class GradientTexture2D final : public Texture2D
{
public:
	enum Fill
	{
		FILL_LINEAR,
		FILL_RADIAL,
		FILL_SQUARE,
		FILL_CONIC,
	};

	enum Repeat
	{
		REPEAT_NONE,
		REPEAT,
		REPEAT_MIRROR,
	};

private:
	Ref<Gradient> gradient;
	mutable RID texture;

	int width = 64;
	int height = 64;

	bool is_1d = false;
	bool use_hdr = false;

	Vector2 fill_from;
	Vector2 fill_to = Vector2(1, 0);

	Fill fill = FILL_LINEAR;
	Repeat repeat = REPEAT_NONE;

	float _get_gradient_offset_at(int x, int y) const;

	mutable bool update_pending = false;
	void _queue_update();
	void _update() const;

public:
	void set_gradient(Ref<Gradient> p_gradient);
	Ref<Gradient> get_gradient() const;

	void set_width(int p_width);
	virtual int get_width() const override;
	void set_height(int p_height);
	virtual int get_height() const override;

	bool is_1d_texture() const { return is_1d; }

	void set_use_hdr(bool p_enabled);
	bool is_using_hdr() const;

	void set_fill(Fill p_fill);
	Fill get_fill() const;
	void set_fill_from(Vector2 p_fill_from);
	Vector2 get_fill_from() const;
	void set_fill_to(Vector2 p_fill_to);
	Vector2 get_fill_to() const;

	void set_repeat(Repeat p_repeat);
	Repeat get_repeat() const;

	virtual RID get_rid() const override;

	virtual bool has_alpha() const override { return true; }

	virtual Ref<Image> get_image() const override;
	void update_now() const;

	explicit GradientTexture2D(bool p_1d = false);
	virtual ~GradientTexture2D();
};
// --- End scene/resources/gradient.h ---

// --- Folded from scene/resources/environment.h ---
#include "core/io/resource.h"
#include "scene/resources/texture.h"

class Sky;

class Environment : public Resource
{
public:
	enum BGMode
	{
		BG_CLEAR_COLOR,
		BG_COLOR,
		BG_SKY,
		BG_CANVAS,
		BG_KEEP,
		BG_CAMERA_FEED,
		BG_MAX
	};

	enum AmbientSource
	{
		AMBIENT_SOURCE_BG,
		AMBIENT_SOURCE_DISABLED,
		AMBIENT_SOURCE_COLOR,
		AMBIENT_SOURCE_SKY,
	};

	enum ReflectionSource
	{
		REFLECTION_SOURCE_BG,
		REFLECTION_SOURCE_DISABLED,
		REFLECTION_SOURCE_SKY,
	};

	enum ToneMapper
	{
		TONE_MAPPER_LINEAR,
		TONE_MAPPER_REINHARDT,
		TONE_MAPPER_FILMIC,
		TONE_MAPPER_ACES,
		TONE_MAPPER_AGX,
	};

	enum SDFGIYScale
	{
		SDFGI_Y_SCALE_50_PERCENT,
		SDFGI_Y_SCALE_75_PERCENT,
		SDFGI_Y_SCALE_100_PERCENT,
	};

	enum FogMode
	{
		FOG_MODE_EXPONENTIAL,
		FOG_MODE_DEPTH,
	};

	enum GlowBlendMode
	{
		GLOW_BLEND_MODE_ADDITIVE,
		GLOW_BLEND_MODE_SCREEN,
		GLOW_BLEND_MODE_SOFTLIGHT,
		GLOW_BLEND_MODE_REPLACE,
		GLOW_BLEND_MODE_MIX,
	};

private:
	RID environment;

	// Background
	BGMode bg_mode = BG_CLEAR_COLOR;
	Ref<Sky> bg_sky;
	float bg_sky_custom_fov = 0.0;
	Vector3 bg_sky_rotation;
	Color bg_color;
	int bg_canvas_max_layer = 0;
	int bg_camera_feed_id = 1;
	float bg_energy_multiplier = 1.0;
	float bg_intensity = 30000.0; // Measured in nits or candela/m^2
	void _update_bg_energy();

	// Ambient light
	Color ambient_color;
	AmbientSource ambient_source = AMBIENT_SOURCE_BG;
	float ambient_energy = 1.0;
	float ambient_sky_contribution = 1.0;
	ReflectionSource reflection_source = REFLECTION_SOURCE_BG;
	void _update_ambient_light();

	// Tonemap
	ToneMapper tone_mapper = TONE_MAPPER_LINEAR;
	float tonemap_exposure = 1.0;
	float tonemap_white = 1.0;
	float tonemap_agx_white = 16.29;   // Default to Blender's AgX white.
	float tonemap_agx_contrast = 1.25; // Default to approximately Blender's AgX contrast.
	void _update_tonemap();

	// SSR
	bool ssr_enabled = false;
	int ssr_max_steps = 64;
	float ssr_fade_in = 0.15;
	float ssr_fade_out = 2.0;
	float ssr_depth_tolerance = 0.5;
	void _update_ssr();

	// SSAO
	bool ssao_enabled = false;
	float ssao_radius = 1.0;
	float ssao_intensity = 2.0;
	float ssao_power = 1.5;
	float ssao_detail = 0.5;
	float ssao_horizon = 0.06;
	float ssao_sharpness = 0.98;
	float ssao_direct_light_affect = 0.0;
	float ssao_ao_channel_affect = 0.0;
	void _update_ssao();

	// SSIL
	bool ssil_enabled = false;
	float ssil_radius = 5.0;
	float ssil_intensity = 1.0;
	float ssil_sharpness = 0.98;
	float ssil_normal_rejection = 1.0;

	void _update_ssil();

	// SDFGI
	bool sdfgi_enabled = false;
	int sdfgi_cascades = 4;
	float sdfgi_min_cell_size = 0.2;
	SDFGIYScale sdfgi_y_scale = SDFGI_Y_SCALE_75_PERCENT;
	bool sdfgi_use_occlusion = false;
	float sdfgi_bounce_feedback = 0.5;
	bool sdfgi_read_sky_light = true;
	float sdfgi_energy = 1.0;
	float sdfgi_normal_bias = 1.1;
	float sdfgi_probe_bias = 1.1;
	void _update_sdfgi();

	// Glow
	bool glow_enabled = false;
	Vector<float> glow_levels;
	bool glow_normalize_levels = false;
	float glow_intensity = 0.3;
	float glow_strength = 1.0;
	float glow_mix = 0.05;
	float glow_bloom = 0.0;
	GlowBlendMode glow_blend_mode = GLOW_BLEND_MODE_SCREEN;
	float glow_hdr_bleed_threshold = 1.0;
	float glow_hdr_bleed_scale = 2.0;
	float glow_hdr_luminance_cap = 12.0;
	float glow_map_strength = 0.8f;
	Ref<Texture> glow_map;
	void _update_glow();

	// Fog
	bool fog_enabled = false;
	FogMode fog_mode = FOG_MODE_EXPONENTIAL;
	Color fog_light_color = Color(0.518, 0.553, 0.608);
	float fog_light_energy = 1.0;
	float fog_sun_scatter = 0.0;
	float fog_density = 0.01;
	float fog_height = 0.0;
	float fog_height_density = 0.0; // can be negative to invert effect
	float fog_aerial_perspective = 0.0;
	float fog_sky_affect = 1.0;

	void _update_fog();

	// Depth Fog
	float fog_depth_curve = 1.0;
	float fog_depth_begin = 10.0;
	float fog_depth_end = 100.0;

	void _update_fog_depth();

	// Volumetric Fog
	bool volumetric_fog_enabled = false;
	float volumetric_fog_density = 0.05;
	Color volumetric_fog_albedo = Color(1.0, 1.0, 1.0);
	Color volumetric_fog_emission = Color(0.0, 0.0, 0.0);
	float volumetric_fog_emission_energy = 1.0;
	float volumetric_fog_anisotropy = 0.2;
	float volumetric_fog_length = 64.0;
	float volumetric_fog_detail_spread = 2.0;
	float volumetric_fog_gi_inject = 1.0;
	float volumetric_fog_ambient_inject = 0.0;
	float volumetric_fog_sky_affect = 1.0;
	bool volumetric_fog_temporal_reproject = true;
	float volumetric_fog_temporal_reproject_amount = 0.9;
	void _update_volumetric_fog();

	// Adjustment
	bool adjustment_enabled = false;
	float adjustment_brightness = 1.0;
	float adjustment_contrast = 1.0;
	float adjustment_saturation = 1.0;
	bool use_1d_color_correction = true;
	Ref<Texture> adjustment_color_correction;
	void _update_adjustment();

protected:

public:
	virtual RID get_rid() const override;

	// Background
	void set_background(BGMode p_bg);
	BGMode get_background() const;
	void set_sky(const Ref<Sky>& p_sky);
	Ref<Sky> get_sky() const;
	void set_sky_custom_fov(float p_scale);
	float get_sky_custom_fov() const;
	void set_sky_rotation(const Vector3& p_rotation);
	Vector3 get_sky_rotation() const;
	void set_bg_color(const Color& p_color);
	Color get_bg_color() const;
	void set_bg_energy_multiplier(float p_energy);
	float get_bg_energy_multiplier() const;
	void set_bg_intensity(float p_energy);
	float get_bg_intensity() const;
	void set_canvas_max_layer(int p_max_layer);
	int get_canvas_max_layer() const;
	void set_camera_feed_id(int p_id);
	int get_camera_feed_id() const;

	// Ambient light
	void set_ambient_light_color(const Color& p_color);
	Color get_ambient_light_color() const;
	void set_ambient_source(AmbientSource p_source);
	AmbientSource get_ambient_source() const;
	void set_ambient_light_energy(float p_energy);
	float get_ambient_light_energy() const;
	void set_ambient_light_sky_contribution(float p_ratio);
	float get_ambient_light_sky_contribution() const;
	void set_reflection_source(ReflectionSource p_source);
	ReflectionSource get_reflection_source() const;

	// Tonemap
	void set_tonemapper(ToneMapper p_tone_mapper);
	ToneMapper get_tonemapper() const;
	void set_tonemap_exposure(float p_exposure);
	float get_tonemap_exposure() const;
	void set_tonemap_white(float p_white);
	float get_tonemap_white() const;
	void set_tonemap_agx_white(float p_white);
	float get_tonemap_agx_white() const;
	void set_tonemap_agx_contrast(float p_agx_contrast);
	float get_tonemap_agx_contrast() const;

	// SSR
	void set_ssr_enabled(bool p_enabled);
	bool is_ssr_enabled() const;
	void set_ssr_max_steps(int p_steps);
	int get_ssr_max_steps() const;
	void set_ssr_fade_in(float p_fade_in);
	float get_ssr_fade_in() const;
	void set_ssr_fade_out(float p_fade_out);
	float get_ssr_fade_out() const;
	void set_ssr_depth_tolerance(float p_depth_tolerance);
	float get_ssr_depth_tolerance() const;

	// SSAO
	void set_ssao_enabled(bool p_enabled);
	bool is_ssao_enabled() const;
	void set_ssao_radius(float p_radius);
	float get_ssao_radius() const;
	void set_ssao_intensity(float p_intensity);
	float get_ssao_intensity() const;
	void set_ssao_power(float p_power);
	float get_ssao_power() const;
	void set_ssao_detail(float p_detail);
	float get_ssao_detail() const;
	void set_ssao_horizon(float p_horizon);
	float get_ssao_horizon() const;
	void set_ssao_sharpness(float p_sharpness);
	float get_ssao_sharpness() const;
	void set_ssao_direct_light_affect(float p_direct_light_affect);
	float get_ssao_direct_light_affect() const;
	void set_ssao_ao_channel_affect(float p_ao_channel_affect);
	float get_ssao_ao_channel_affect() const;

	// SSIL
	void set_ssil_enabled(bool p_enabled);
	bool is_ssil_enabled() const;
	void set_ssil_radius(float p_radius);
	float get_ssil_radius() const;
	void set_ssil_intensity(float p_intensity);
	float get_ssil_intensity() const;
	void set_ssil_sharpness(float p_sharpness);
	float get_ssil_sharpness() const;
	void set_ssil_normal_rejection(float p_normal_rejection);
	float get_ssil_normal_rejection() const;

	// SDFGI
	void set_sdfgi_enabled(bool p_enabled);
	bool is_sdfgi_enabled() const;
	void set_sdfgi_cascades(int p_cascades);
	int get_sdfgi_cascades() const;
	void set_sdfgi_min_cell_size(float p_size);
	float get_sdfgi_min_cell_size() const;
	void set_sdfgi_max_distance(float p_distance);
	float get_sdfgi_max_distance() const;
	void set_sdfgi_cascade0_distance(float p_distance);
	float get_sdfgi_cascade0_distance() const;
	void set_sdfgi_y_scale(SDFGIYScale p_y_scale);
	SDFGIYScale get_sdfgi_y_scale() const;
	void set_sdfgi_use_occlusion(bool p_enabled);
	bool is_sdfgi_using_occlusion() const;
	void set_sdfgi_bounce_feedback(float p_amount);
	float get_sdfgi_bounce_feedback() const;
	void set_sdfgi_read_sky_light(bool p_enabled);
	bool is_sdfgi_reading_sky_light() const;
	void set_sdfgi_energy(float p_energy);
	float get_sdfgi_energy() const;
	void set_sdfgi_normal_bias(float p_bias);
	float get_sdfgi_normal_bias() const;
	void set_sdfgi_probe_bias(float p_bias);
	float get_sdfgi_probe_bias() const;

	// Glow
	void set_glow_enabled(bool p_enabled);
	bool is_glow_enabled() const;
	void set_glow_level(int p_level, float p_intensity);
	float get_glow_level(int p_level) const;
	void set_glow_normalized(bool p_normalized);
	bool is_glow_normalized() const;
	void set_glow_intensity(float p_intensity);
	float get_glow_intensity() const;
	void set_glow_strength(float p_strength);
	float get_glow_strength() const;
	void set_glow_mix(float p_mix);
	float get_glow_mix() const;
	void set_glow_bloom(float p_threshold);
	float get_glow_bloom() const;
	void set_glow_blend_mode(GlowBlendMode p_mode);
	GlowBlendMode get_glow_blend_mode() const;
	void set_glow_hdr_bleed_threshold(float p_threshold);
	float get_glow_hdr_bleed_threshold() const;
	void set_glow_hdr_bleed_scale(float p_scale);
	float get_glow_hdr_bleed_scale() const;
	void set_glow_hdr_luminance_cap(float p_amount);
	float get_glow_hdr_luminance_cap() const;
	void set_glow_map_strength(float p_strength);
	float get_glow_map_strength() const;
	void set_glow_map(Ref<Texture> p_glow_map);
	Ref<Texture> get_glow_map() const;

	// Fog

	void set_fog_enabled(bool p_enabled);
	bool is_fog_enabled() const;
	void set_fog_mode(FogMode p_mode);
	FogMode get_fog_mode() const;
	void set_fog_light_color(const Color& p_light_color);
	Color get_fog_light_color() const;
	void set_fog_light_energy(float p_amount);
	float get_fog_light_energy() const;
	void set_fog_sun_scatter(float p_amount);
	float get_fog_sun_scatter() const;

	void set_fog_density(float p_amount);
	float get_fog_density() const;
	void set_fog_height(float p_amount);
	float get_fog_height() const;
	void set_fog_height_density(float p_amount);
	float get_fog_height_density() const;
	void set_fog_aerial_perspective(float p_aerial_perspective);
	float get_fog_aerial_perspective() const;
	void set_fog_sky_affect(float p_sky_affect);
	float get_fog_sky_affect() const;

	// Depth Fog
	void set_fog_depth_curve(float p_curve);
	float get_fog_depth_curve() const;
	void set_fog_depth_begin(float p_begin);
	float get_fog_depth_begin() const;
	void set_fog_depth_end(float p_end);
	float get_fog_depth_end() const;

	// Volumetric Fog
	void set_volumetric_fog_enabled(bool p_enable);
	bool is_volumetric_fog_enabled() const;
	void set_volumetric_fog_density(float p_density);
	float get_volumetric_fog_density() const;
	void set_volumetric_fog_albedo(Color p_color);
	Color get_volumetric_fog_albedo() const;
	void set_volumetric_fog_emission(Color p_color);
	Color get_volumetric_fog_emission() const;
	void set_volumetric_fog_emission_energy(float p_begin);
	float get_volumetric_fog_emission_energy() const;
	void set_volumetric_fog_anisotropy(float p_anisotropy);
	float get_volumetric_fog_anisotropy() const;
	void set_volumetric_fog_length(float p_length);
	float get_volumetric_fog_length() const;
	void set_volumetric_fog_detail_spread(float p_detail_spread);
	float get_volumetric_fog_detail_spread() const;
	void set_volumetric_fog_gi_inject(float p_gi_inject);
	float get_volumetric_fog_gi_inject() const;
	void set_volumetric_fog_ambient_inject(float p_ambient_inject);
	float get_volumetric_fog_ambient_inject() const;
	void set_volumetric_fog_sky_affect(float p_sky_affect);
	float get_volumetric_fog_sky_affect() const;
	void set_volumetric_fog_temporal_reprojection_enabled(bool p_enable);
	bool is_volumetric_fog_temporal_reprojection_enabled() const;
	void set_volumetric_fog_temporal_reprojection_amount(float p_amount);
	float get_volumetric_fog_temporal_reprojection_amount() const;

	// Adjustment
	void set_adjustment_enabled(bool p_enabled);
	bool is_adjustment_enabled() const;
	void set_adjustment_brightness(float p_brightness);
	float get_adjustment_brightness() const;
	void set_adjustment_contrast(float p_contrast);
	float get_adjustment_contrast() const;
	void set_adjustment_saturation(float p_saturation);
	float get_adjustment_saturation() const;
	void set_adjustment_color_correction(Ref<Texture> p_color_correction);
	Ref<Texture> get_adjustment_color_correction() const;

	Environment();
	~Environment();
};

// --- Folded from scene/resources/compositor.h ---
#include "core/io/resource.h"

class RenderDataRD;

/* Compositor Effect */

class CompositorEffect : public Resource
{
public:
	enum EffectCallbackType
	{
		EFFECT_CALLBACK_TYPE_PRE_OPAQUE,
		EFFECT_CALLBACK_TYPE_POST_OPAQUE,
		EFFECT_CALLBACK_TYPE_POST_SKY,
		EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT,
		EFFECT_CALLBACK_TYPE_POST_TRANSPARENT,
		EFFECT_CALLBACK_TYPE_MAX
	};

private:
	RID rid;
	bool enabled = true;
	EffectCallbackType effect_callback_type = EFFECT_CALLBACK_TYPE_POST_TRANSPARENT;

	bool access_resolved_color = false;
	bool access_resolved_depth = false;
	bool needs_motion_vectors = false;
	bool needs_normal_roughness = false;
	bool needs_separate_specular = false;

protected:

	void _call_render_callback(int p_effect_callback_type, const RenderDataRD* p_render_data);

public:
	virtual RID get_rid() const override { return rid; }

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	void set_effect_callback_type(EffectCallbackType p_callback_type);
	EffectCallbackType get_effect_callback_type() const;

	void set_access_resolved_color(bool p_enabled);
	bool get_access_resolved_color() const;

	void set_access_resolved_depth(bool p_enabled);
	bool get_access_resolved_depth() const;

	void set_needs_motion_vectors(bool p_enabled);
	bool get_needs_motion_vectors() const;

	void set_needs_normal_roughness(bool p_enabled);
	bool get_needs_normal_roughness() const;

	void set_needs_separate_specular(bool p_enabled);
	bool get_needs_separate_specular() const;

	CompositorEffect();
	~CompositorEffect();
};


/* Compositor */

class Compositor : public Resource
{
private:
	RID compositor;

	// Compositor effects
	LocalVector<Ref<CompositorEffect>> effects;

protected:

public:
	virtual RID get_rid() const override { return compositor; }

	Compositor();
	~Compositor();
};



// --- End scene/resources/compositor.h ---

// --- Folded from scene/resources/sky.h ---
#include "core/io/resource.h"


class Sky : public Resource
{
public:
	enum RadianceSize
	{
		RADIANCE_SIZE_32,
		RADIANCE_SIZE_64,
		RADIANCE_SIZE_128,
		RADIANCE_SIZE_256,
		RADIANCE_SIZE_512,
		RADIANCE_SIZE_1024,
		RADIANCE_SIZE_2048,
		RADIANCE_SIZE_MAX
	};

	enum ProcessMode
	{
		PROCESS_MODE_AUTOMATIC,
		PROCESS_MODE_QUALITY,
		PROCESS_MODE_INCREMENTAL,
		PROCESS_MODE_REALTIME
	};

private:
	RID sky;
	ProcessMode mode = PROCESS_MODE_AUTOMATIC;
	RadianceSize radiance_size = RADIANCE_SIZE_256;
	Ref<Material> sky_material;

protected:

public:
	void set_radiance_size(RadianceSize p_size);
	RadianceSize get_radiance_size() const;

	void set_process_mode(ProcessMode p_mode);
	ProcessMode get_process_mode() const;

	void set_material(const Ref<Material>& p_material);
	Ref<Material> get_material() const;

	virtual RID get_rid() const override;

	Sky();
	~Sky();
};



// --- End scene/resources/sky.h ---
// --- End scene/resources/environment.h ---

// --- Folded from scene/resources/image_texture.h ---
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
// --- End scene/resources/image_texture.h ---
