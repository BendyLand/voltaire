/**************************************************************************/
/*  rendering_device.h                                                    */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/os/condition_variable.h"
#include "core/os/thread_safe.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"
#include "core/templates/rid_owner.h"
#include "servers/display/display_server_enums.h"
#include "servers/rendering/rendering_device_commons.h"
#include "servers/rendering/rendering_device_driver.h"
#include "servers/rendering/rendering_device_enums.h"
#include "servers/rendering/rendering_device_graph.h"

class RDTextureFormat;
class RDTextureView;
class RDAttachmentFormat;
class RDSamplerState;
class RDVertexAttribute;
class RDShaderSource;
class RDShaderSPIRV;
class RDUniform;
class RDPipelineRasterizationState;
class RDPipelineMultisampleState;
class RDPipelineDepthStencilState;
class RDPipelineColorBlendState;
class RDFramebufferPass;
class RDPipelineSpecializationConstant;
class RDAccelerationStructureGeometry;
class RDAccelerationStructureInstance;
class RDPipelineShader;
class RDHitGroup;

class RenderingDevice final : public RenderingDeviceCommons
{
	static inline BinaryMutex _thread_safe_mutex;

public:
	using DrawListID = int64_t;
	using ComputeListID = int64_t;
	using RaytracingListID = int64_t;
	using FramebufferFormatID = int64_t;
	using VertexFormatID = int64_t;
	using HitShaderBindingTableRange = int64_t;
	using InvalidationCallback = void (*)(void*);

	enum
	{
		INVALID_FORMAT_ID = -1
	};

	enum IDType
	{
		ID_TYPE_FRAMEBUFFER_FORMAT,
		ID_TYPE_VERTEX_FORMAT,
		ID_TYPE_DRAW_LIST,
		ID_TYPE_COMPUTE_LIST = 4,
		ID_TYPE_RAYTRACING_LIST = 5,
		ID_TYPE_MAX,
		ID_BASE_SHIFT = 58,
		ID_MASK = (ID_BASE_SHIFT - 1),
	};

	enum BufferCreationBits
	{
		BUFFER_CREATION_DEVICE_ADDRESS_BIT = (1 << 0),
		BUFFER_CREATION_AS_STORAGE_BIT = (1 << 1),
		BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT = (1 << 2),
		BUFFER_CREATION_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT = (1 << 3),
	};

	enum StorageBufferUsage
	{
		STORAGE_BUFFER_USAGE_DISPATCH_INDIRECT = (1 << 0),
	};

	enum CallbackResourceType
	{
		CALLBACK_RESOURCE_TYPE_TEXTURE,
		CALLBACK_RESOURCE_TYPE_BUFFER,
	};

	enum CallbackResourceUsage
	{
		CALLBACK_RESOURCE_USAGE_NONE,
		CALLBACK_RESOURCE_USAGE_COPY_FROM,
		CALLBACK_RESOURCE_USAGE_COPY_TO,
		CALLBACK_RESOURCE_USAGE_RESOLVE_FROM,
		CALLBACK_RESOURCE_USAGE_RESOLVE_TO,
		CALLBACK_RESOURCE_USAGE_UNIFORM_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_INDIRECT_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_TEXTURE_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_STORAGE_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_VERTEX_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_INDEX_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE,
		CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ,
		CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_FRAGMENT_SHADING_RATE_READ,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_FRAGMENT_DENSITY_MAP_READ,
		CALLBACK_RESOURCE_USAGE_GENERAL,
		CALLBACK_RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ,
		CALLBACK_RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_MAX
	};

	enum VRSMethod
	{
		VRS_METHOD_NONE,
		VRS_METHOD_FRAGMENT_SHADING_RATE,
		VRS_METHOD_FRAGMENT_DENSITY_MAP,
	};

	enum DrawFlags
	{
		DRAW_DEFAULT_ALL = 0,
		DRAW_CLEAR_COLOR_0 = (1 << 0),
		DRAW_CLEAR_COLOR_1 = (1 << 1),
		DRAW_CLEAR_COLOR_2 = (1 << 2),
		DRAW_CLEAR_COLOR_3 = (1 << 3),
		DRAW_CLEAR_COLOR_4 = (1 << 4),
		DRAW_CLEAR_COLOR_5 = (1 << 5),
		DRAW_CLEAR_COLOR_6 = (1 << 6),
		DRAW_CLEAR_COLOR_7 = (1 << 7),
		DRAW_CLEAR_COLOR_MASK = 0xFF,
		DRAW_CLEAR_COLOR_ALL = DRAW_CLEAR_COLOR_MASK,
		DRAW_IGNORE_COLOR_0 = (1 << 8),
		DRAW_IGNORE_COLOR_1 = (1 << 9),
		DRAW_IGNORE_COLOR_2 = (1 << 10),
		DRAW_IGNORE_COLOR_3 = (1 << 11),
		DRAW_IGNORE_COLOR_4 = (1 << 12),
		DRAW_IGNORE_COLOR_5 = (1 << 13),
		DRAW_IGNORE_COLOR_6 = (1 << 14),
		DRAW_IGNORE_COLOR_7 = (1 << 15),
		DRAW_IGNORE_COLOR_MASK = 0xFF00,
		DRAW_IGNORE_COLOR_ALL = DRAW_IGNORE_COLOR_MASK,
		DRAW_CLEAR_DEPTH = (1 << 16),
		DRAW_IGNORE_DEPTH = (1 << 17),
		DRAW_CLEAR_STENCIL = (1 << 18),
		DRAW_IGNORE_STENCIL = (1 << 19),
		DRAW_CLEAR_ALL = DRAW_CLEAR_COLOR_ALL | DRAW_CLEAR_DEPTH | DRAW_CLEAR_STENCIL,
		DRAW_IGNORE_ALL = DRAW_IGNORE_COLOR_ALL | DRAW_IGNORE_DEPTH | DRAW_IGNORE_STENCIL
	};

	enum MemoryType
	{
		MEMORY_TEXTURES,
		MEMORY_BUFFERS,
		MEMORY_TOTAL
	};

	struct CallbackResource
	{
		RID rid;
		CallbackResourceType type = CALLBACK_RESOURCE_TYPE_TEXTURE;
		CallbackResourceUsage usage = CALLBACK_RESOURCE_USAGE_NONE;
	};

	struct TextureView
	{
		DataFormat format_override = DATA_FORMAT_MAX;
		TextureSwizzle swizzle_r = TEXTURE_SWIZZLE_R;
		TextureSwizzle swizzle_g = TEXTURE_SWIZZLE_G;
		TextureSwizzle swizzle_b = TEXTURE_SWIZZLE_B;
		TextureSwizzle swizzle_a = TEXTURE_SWIZZLE_A;

		bool operator==(const TextureView& p_other) const
		{
			if (format_override != p_other.format_override) {
				return false;
			}
			else if (swizzle_r != p_other.swizzle_r) {
				return false;
			}
			else if (swizzle_g != p_other.swizzle_g) {
				return false;
			}
			else if (swizzle_b != p_other.swizzle_b) {
				return false;
			}
			else if (swizzle_a != p_other.swizzle_a) {
				return false;
			}
			else {
				return true;
			}
		}
	};

	struct AttachmentFormat
	{
		enum : uint32_t
		{
			UNUSED_ATTACHMENT = 0xFFFFFFFF
		};

		DataFormat format;
		TextureSamples samples;
		uint32_t usage_flags;

		AttachmentFormat()
		{
			format = DATA_FORMAT_R8G8B8A8_UNORM;
			samples = TEXTURE_SAMPLES_1;
			usage_flags = 0;
		}
	};

	struct FramebufferPass
	{
		Vector<int32_t> color_attachments;
		Vector<int32_t> input_attachments;
		Vector<int32_t> resolve_attachments;
		Vector<int32_t> preserve_attachments;
		int32_t depth_attachment = ATTACHMENT_UNUSED;
		int32_t depth_resolve_attachment = ATTACHMENT_UNUSED;
	};

	struct Uniform
	{
		UniformType uniform_type = UNIFORM_TYPE_IMAGE;
		uint32_t binding = 0;
		bool immutable_sampler = false;

	private:
		RID id;
		Vector<RID> ids;

	public:
		_FORCE_INLINE_ uint32_t get_id_count() const { return (id.is_valid() ? 1 : ids.size()); }

		_FORCE_INLINE_ RID get_id(uint32_t p_idx) const
		{
			if (id.is_valid()) {
				ERR_FAIL_COND_V(p_idx != 0, RID());
				return id;
			}
			else {
				return ids[p_idx];
			}
		}

		_FORCE_INLINE_ void set_id(uint32_t p_idx, RID p_id)
		{
			if (id.is_valid()) {
				ERR_FAIL_COND(p_idx != 0);
				id = p_id;
			}
			else {
				ids.write[p_idx] = p_id;
			}
		}

		_FORCE_INLINE_ void append_id(RID p_id)
		{
			if (ids.is_empty()) {
				if (id == RID()) {
					id = p_id;
				}
				else {
					ids.push_back(id);
					ids.push_back(p_id);
					id = RID();
				}
			}
			else {
				ids.push_back(p_id);
			}
		}

		_FORCE_INLINE_ void clear_ids()
		{
			id = RID();
			ids.clear();
		}

		_FORCE_INLINE_ Uniform(UniformType p_type, int p_binding, RID p_id)
		{
			uniform_type = p_type;
			binding = p_binding;
			id = p_id;
		}

		_FORCE_INLINE_ Uniform(UniformType p_type, int p_binding, const Vector<RID>& p_ids)
		{
			uniform_type = p_type;
			binding = p_binding;
			ids = p_ids;
		}

		_FORCE_INLINE_ Uniform() = default;
	};

	static void _texture_ensure_shareable_format(
		RID p_texture, const DataFormat& p_shareable_format);

	using PipelineImmutableSampler = Uniform;

	struct PipelineShader
	{
		RID shader;
		Vector<PipelineSpecializationConstant> specialization_constants;

		bool is_valid() const { return shader.is_valid(); }
	};

	struct HitGroup
	{
		PipelineShader closest_hit_shader;
		PipelineShader any_hit_shader;
		PipelineShader intersection_shader;
	};

	struct AccelerationStructureGeometry
	{
		uint32_t flags = 0;
		RID vertex_buffer;
		uint32_t vertex_offset = 0;
		uint32_t vertex_stride = 0;
		uint32_t vertex_count = 0;
		DataFormat vertex_format = DATA_FORMAT_MAX;
		RID index_buffer;
		uint32_t index_offset = 0;
		uint32_t index_count = 0;
	};

	struct AccelerationStructureInstance
	{
		Transform3D transform;
		uint32_t id = 0;
		uint8_t mask = 0xFF;
		HitShaderBindingTableRange hit_sbt_range = 0;
		uint32_t flags = 0;
		RID blas;
	};

public:
	// --- Lifecycle & System Management ---
	static void make_current();

	static RenderingDeviceDriver* get_device_driver() { return data->driver; }

	static RenderingContextDriver* get_context_driver() { return data->context; }

	static const RDD::Capabilities& get_device_capabilities()
	{
		return data->driver->get_capabilities();
	}

	static void free_rid(RID p_rid);
	static void _free_internal(RID p_id);
	static void _execute_frame(bool p_present);
	template <typename T> static void _free_rids(T& p_owner, const char* p_type);

	static void swap_buffers(bool p_present);
	static void sync();
	static void _set_max_fps(int p_max_fps);
	static uint32_t get_frame_delay();

	static uint64_t get_frames_drawn() { return data->frames_drawn; }

	static bool has_pending_resources_for_processing()
	{
		return data->frames_pending_resources_for_processing != 0u;
	}

	static bool has_feature(const Features p_feature);

	// --- Buffer Operations ---
	static RID vertex_buffer_create(
		uint32_t p_size_bytes, Span<uint8_t> p_data = {}, uint32_t p_creation_bits = 0);
	static VertexFormatID vertex_format_create(
		const Vector<VertexAttribute>& p_vertex_descriptions);
	static RID vertex_array_create(uint32_t p_vertex_count, VertexFormatID p_vertex_format,
		const Vector<RID>& p_src_buffers, const Vector<uint64_t>& p_offsets = Vector<uint64_t>());
	static RID index_array_create(
		RID p_index_buffer, uint32_t p_index_offset, uint32_t p_index_count);

	static RID storage_buffer_create(
		uint32_t p_size_bytes, Span<uint8_t> p_data = {}, uint32_t p_creation_bits = 0);
	static RID texture_buffer_create(
		uint32_t p_size_elements, DataFormat p_format, Span<uint8_t> p_data = {});

	static Error buffer_copy(RID p_src_buffer, RID p_dst_buffer, uint32_t p_src_offset,
		uint32_t p_dst_offset, uint32_t p_size);
	static Error buffer_clear(RID p_buffer, uint32_t p_offset, uint32_t p_size);
	static Vector<uint8_t> buffer_get_data(
		RID p_buffer, uint32_t p_offset = 0, uint32_t p_size = 0);
	static Error buffer_get_data_async(RID p_buffer, uint32_t p_offset = 0, uint32_t p_size = 0);
	static uint64_t buffer_get_device_address(RID p_buffer);
	static uint8_t* buffer_persistent_map_advance(RID p_buffer);
	static void buffer_flush(RID p_buffer);

	// --- Texture Operations ---
	static RID texture_create(const TextureFormat& p_format, const TextureView& p_view,
		const Vector<Vector<uint8_t>>& p_data = Vector<Vector<uint8_t>>());
	static RID texture_create_shared(const TextureView& p_view, RID p_with_texture);
	static RID texture_create_from_extension(TextureType p_type, DataFormat p_format,
		TextureSamples p_samples, uint32_t p_usage, uint64_t p_image, uint64_t p_width,
		uint64_t p_height, uint64_t p_depth, uint64_t p_layers, uint64_t p_mipmaps = 1);
	static RID texture_create_shared_from_slice(const TextureView& p_view, RID p_with_texture,
		uint32_t p_layer, uint32_t p_mipmap, uint32_t p_mipmaps = 1,
		TextureSliceType p_slice_type = TEXTURE_SLICE_2D, uint32_t p_layers = 0);
	static Error texture_update(RID p_texture, uint32_t p_layer, const Vector<uint8_t>& p_data);
	static Vector<uint8_t> texture_get_data(RID p_texture, uint32_t p_layer);
	static Error texture_get_data_async(RID p_texture, uint32_t p_layer);

	static bool texture_is_format_supported_for_usage(DataFormat p_format, uint32_t p_usage);
	static bool texture_is_shared(RID p_texture);
	static bool texture_is_valid(RID p_texture);
	static TextureFormat texture_get_format(RID p_texture);
	static Size2i texture_size(RID p_texture);

	static Error texture_copy(RID p_from_texture, RID p_to_texture, const Vector3& p_from,
		const Vector3& p_to, const Vector3& p_size, uint32_t p_src_mipmap, uint32_t p_dst_mipmap,
		uint32_t p_src_layer, uint32_t p_dst_layer);
	static Error texture_clear(RID p_texture, const Color& p_color, uint32_t p_base_mipmap,
		uint32_t p_mipmaps, uint32_t p_base_layer, uint32_t p_layers);
	static Error texture_resolve_multisample(RID p_from_texture, RID p_to_texture);

	static void texture_set_discardable(RID p_texture, bool p_discardable);
	static bool texture_is_discardable(RID p_texture);

	// --- VRS & Framebuffers ---
	static VRSMethod vrs_get_method();
	static DataFormat vrs_get_format();
	static Size2i vrs_get_texel_size();

	static FramebufferFormatID framebuffer_format_create(const Vector<AttachmentFormat>& p_format,
		uint32_t p_view_count = 1, int32_t p_vrs_attachment = -1);
	static FramebufferFormatID framebuffer_format_create_multipass(
		const Vector<AttachmentFormat>& p_format, const Vector<FramebufferPass>& p_passes,
		uint32_t p_view_count = 1, int32_t p_vrs_attachment = -1);
	static FramebufferFormatID framebuffer_format_create_empty(
		TextureSamples p_samples = TEXTURE_SAMPLES_1);
	static TextureSamples framebuffer_format_get_texture_samples(
		FramebufferFormatID p_format, uint32_t p_pass = 0);

	static RID framebuffer_create(const Vector<RID>& p_texture_attachments,
		FramebufferFormatID p_format_check = INVALID_ID, uint32_t p_view_count = 1);
	static RID framebuffer_create_multipass(const Vector<RID>& p_texture_attachments,
		const Vector<FramebufferPass>& p_passes, FramebufferFormatID p_format_check = INVALID_ID,
		uint32_t p_view_count = 1);
	static RID framebuffer_create_empty(const Size2i& p_size,
		TextureSamples p_samples = TEXTURE_SAMPLES_1,
		FramebufferFormatID p_format_check = INVALID_ID);
	static bool framebuffer_is_valid(RID p_framebuffer);
	static void framebuffer_set_invalidation_callback(
		RID p_framebuffer, InvalidationCallback p_callback, void* p_userdata);
	static FramebufferFormatID framebuffer_get_format(RID p_framebuffer);
	static Size2 framebuffer_get_size(RID p_framebuffer);

	// --- Samplers & Shaders ---
	static RID sampler_create(const SamplerState& p_state);
	static bool sampler_is_format_supported_for_filter(
		DataFormat p_format, SamplerFilter p_sampler_filter);

	static Vector<uint8_t> shader_compile_spirv_from_source(ShaderStage p_stage,
		const String& p_source_code, ShaderLanguage p_language = SHADER_LANGUAGE_GLSL,
		String* r_error = nullptr, bool p_allow_cache = true);
	static Vector<uint8_t> shader_compile_binary_from_spirv(
		const Vector<ShaderStageSPIRVData>& p_spirv, const String& p_shader_name = "");
	static RID shader_create_from_bytecode(
		const Vector<uint8_t>& p_shader_binary, RID p_placeholder = RID());
	static RID shader_create_placeholder();
	static void shader_destroy_modules(RID p_shader);
	static uint64_t shader_get_vertex_input_attribute_mask(RID p_shader);

	static RID uniform_set_create(const VectorView<Uniform>& p_uniforms, RID p_shader,
		uint32_t p_shader_set, bool p_linear_pool = false);
	static bool uniform_set_is_valid(RID p_uniform_set);
	static void uniform_set_set_invalidation_callback(
		RID p_uniform_set, InvalidationCallback p_callback, void* p_userdata);
	static bool uniform_sets_have_linear_pools();

	// --- Pipelines ---
	static RID render_pipeline_create(RID p_shader, FramebufferFormatID p_framebuffer_format,
		VertexFormatID p_vertex_format, RenderPrimitive p_render_primitive,
		const PipelineRasterizationState& p_rasterization_state,
		const PipelineMultisampleState& p_multisample_state,
		const PipelineDepthStencilState& p_depth_stencil_state,
		const PipelineColorBlendState& p_blend_state, uint32_t p_dynamic_state_flags = 0,
		uint32_t p_for_render_pass = 0,
		const Vector<PipelineSpecializationConstant>& p_specialization_constants =
			Vector<PipelineSpecializationConstant>());
	static bool render_pipeline_is_valid(RID p_pipeline);

	static RID compute_pipeline_create(
		RID p_shader, const Vector<PipelineSpecializationConstant>& p_specialization_constants =
						  Vector<PipelineSpecializationConstant>());
	static bool compute_pipeline_is_valid(RID p_pipeline);

	static RID raytracing_pipeline_create(Span<PipelineShader> p_raygen_shaders,
		Span<PipelineShader> p_miss_shaders, Span<HitGroup> p_hit_groups,
		uint32_t p_max_trace_recursion_depth);
	static bool raytracing_pipeline_is_valid(RID p_pipeline);

	// --- Raytracing & Acceleration Structures ---
	static RID blas_create(Span<AccelerationStructureGeometry> p_geometries, uint32_t p_flags);
	static RID tlas_create(uint32_t p_max_instance_count, uint32_t p_flags);
	static Error blas_build(RID p_blas);
	static Error tlas_build(RID p_tlas, Span<AccelerationStructureInstance> p_instances);

	static RID hit_sbt_create(RID p_raytracing_pipeline, uint32_t p_initial_hit_group_capacity);
	static Error hit_sbt_set_pipeline(RID p_hit_sbt, RID p_raytracing_pipeline);
	static HitShaderBindingTableRange hit_sbt_range_alloc(
		RID p_hit_sbt, uint32_t p_hit_group_count);
	static Error hit_sbt_range_free(RID p_hit_sbt, HitShaderBindingTableRange p_range);
	static Error hit_sbt_range_update(RID p_hit_sbt, HitShaderBindingTableRange p_range,
		uint32_t p_hit_group_offset, Span<uint32_t> p_hit_group_indices);

	// --- Command Lists ---
	static DrawListID draw_list_begin_for_screen(
		DisplayServerEnums::WindowID p_screen = 0, const Color& p_clear_color = Color());
	static DrawListID draw_list_begin(RID p_framebuffer, uint32_t p_draw_flags = DRAW_DEFAULT_ALL,
		const Vector<Color>& p_clear_color_values = Vector<Color>(),
		float p_clear_depth_value = 1.0f, uint32_t p_clear_stencil_value = 0,
		const Rect2& p_region = Rect2(), uint32_t p_breadcrumb = 0);
	static void draw_list_set_blend_constants(DrawListID p_list, const Color& p_color);
	static void draw_list_bind_render_pipeline(DrawListID p_list, RID p_render_pipeline);
	static void draw_list_bind_uniform_set(DrawListID p_list, RID p_uniform_set, uint32_t p_index);
	static void draw_list_bind_vertex_array(DrawListID p_list, RID p_vertex_array);
	static void draw_list_bind_index_array(DrawListID p_list, RID p_index_array);
	static void draw_list_set_line_width(DrawListID p_list, float p_width);
	static void draw_list_set_push_constant(
		DrawListID p_list, const void* p_data, uint32_t p_data_size);
	static void draw_list_draw(DrawListID p_list, bool p_use_indices, uint32_t p_instances = 1,
		uint32_t p_procedural_vertices = 0);
	static void draw_list_draw_indirect(DrawListID p_list, bool p_use_indices, RID p_buffer,
		uint32_t p_offset = 0, uint32_t p_draw_count = 1, uint32_t p_stride = 0);
	static void draw_list_set_viewport(DrawListID p_list, const Rect2& p_rect);
	static void draw_list_enable_scissor(DrawListID p_list, const Rect2& p_rect);
	static void draw_list_disable_scissor(DrawListID p_list);
	static uint32_t draw_list_get_current_pass();
	static DrawListID draw_list_switch_to_next_pass();
	static void draw_list_end();

	static ComputeListID compute_list_begin();
	static void compute_list_bind_uniform_set(
		ComputeListID p_list, RID p_uniform_set, uint32_t p_index);
	static void compute_list_set_push_constant(
		ComputeListID p_list, const void* p_data, uint32_t p_data_size);
	static void compute_list_dispatch(
		ComputeListID p_list, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups);
	static void compute_list_dispatch_threads(
		ComputeListID p_list, uint32_t p_x_threads, uint32_t p_y_threads, uint32_t p_z_threads);
	static void compute_list_dispatch_indirect(
		ComputeListID p_list, RID p_buffer, uint32_t p_offset);
	static void compute_list_add_barrier(ComputeListID p_list);
	static void compute_list_end();

	static RaytracingListID raytracing_list_begin();
	static void raytracing_list_bind_raytracing_pipeline(
		RaytracingListID p_list, RID p_raytracing_pipeline);
	static void raytracing_list_bind_uniform_set(
		RaytracingListID p_list, RID p_uniform_set, uint32_t p_index);
	static void raytracing_list_set_push_constant(
		RaytracingListID p_list, const void* p_data, uint32_t p_data_size);
	static void raytracing_list_trace_rays(RaytracingListID p_list, uint32_t p_raygen_shader_index,
		RID p_hit_sbt, uint32_t p_width, uint32_t p_height, uint32_t p_depth);
	static void raytracing_list_end();

	// --- Screen Presentation ---
	static Error screen_create(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static Error screen_prepare_for_drawing(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static int screen_get_width(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static int screen_get_height(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static int screen_get_pre_rotation_degrees(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static FramebufferFormatID screen_get_framebuffer_format(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static ColorSpace screen_get_color_space(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool screen_get_hdr_output_supported(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static Error screen_free(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool is_composite_alpha_supported();

	// --- Diagnostics & Profiling ---
	static Error driver_callback_add(
		RDD::DriverCallback p_callback, void* p_userdata, VectorView<CallbackResource> p_resources);
	static void set_resource_name(RID p_id, const String& p_name);
	static void _draw_command_begin_label(
		String p_label_name, const Color& p_color = Color(1, 1, 1, 1));
	static void draw_command_begin_label(
		const Span<char> p_label_name, const Color& p_color = Color(1, 1, 1, 1));
	static void draw_command_end_label();

	static void capture_timestamp(const String& p_name);
	static uint32_t get_captured_timestamps_count();
	static uint64_t get_captured_timestamps_frame();
	static uint64_t get_captured_timestamp_gpu_time(uint32_t p_index);
	static uint64_t get_captured_timestamp_cpu_time(uint32_t p_index);
	static String get_captured_timestamp_name(uint32_t p_index);

	static uint64_t limit_get(Limit p_limit);
	static uint64_t get_memory_usage(MemoryType p_type);
	static String get_perf_report();
	static String get_device_vendor_name();
	static String get_device_name();
	static RenderingDeviceEnums::DeviceType get_device_type();
	static String get_device_api_name();
	static String get_device_api_version();
	static String get_device_pipeline_cache_uuid();
	static DriverWorkarounds get_driver_workarounds();
	static uint64_t get_driver_resource(
		DriverResource p_resource, RID p_rid = RID(), uint64_t p_index = 0);
	static String get_driver_and_device_memory_report();
	static String get_tracked_object_name(uint32_t p_type_index);
	static uint64_t get_tracked_object_type_count();
	static uint64_t get_driver_total_memory();
	static uint64_t get_driver_allocation_count();
	static uint64_t get_driver_memory_by_object_type(uint32_t p_type);
	static uint64_t get_driver_allocs_by_object_type(uint32_t p_type);
	static uint64_t get_device_total_memory();
	static uint64_t get_device_allocation_count();
	static uint64_t get_device_memory_by_object_type(uint32_t p_type);
	static uint64_t get_device_allocs_by_object_type(uint32_t p_type);

private:
	static void _add_dependency(RID p_id, RID p_depends_on);
	static void _remove_dependency(RID p_id, RID p_depends_on);
	static void _free_dependencies(RID p_id);

	enum StagingRequiredAction
	{
		STAGING_REQUIRED_ACTION_NONE,
		STAGING_REQUIRED_ACTION_FLUSH_AND_STALL_ALL,
		STAGING_REQUIRED_ACTION_STALL_PREVIOUS,
	};

	struct StagingBufferBlock
	{
		RDD::BufferID driver_id;
		uint64_t frame_used = 0;
		uint32_t fill_amount = 0;
		uint8_t* data_ptr = nullptr;
	};

	struct StagingBuffers
	{
		Vector<StagingBufferBlock> blocks;
		int current = 0;
		uint32_t block_size = 0;
		uint64_t max_size = 0;
		uint32_t usage_bits = 0;
		bool used = false;
	};

	static Error _staging_buffer_allocate(StagingBuffers& p_staging_buffers, uint32_t p_amount,
		uint32_t p_required_align, uint32_t& r_alloc_offset, uint32_t& r_alloc_size,
		StagingRequiredAction& r_required_action, bool p_can_segment = true);
	static void _staging_buffer_execute_required_action(
		StagingBuffers& p_staging_buffers, StagingRequiredAction p_required_action);
	static Error _insert_staging_block(StagingBuffers& p_staging_buffers);

	struct Buffer
	{
		RDD::BufferID driver_id;
		uint32_t size = 0;
		uint32_t usage = 0;
		RDG::ResourceTracker* draw_tracker = nullptr;
		int32_t transfer_worker_index = -1;
		uint64_t transfer_worker_operation = 0;
	};

	static Buffer* _get_buffer_from_owner(RID p_buffer);
	static Error _buffer_initialize(
		Buffer* p_buffer, Span<uint8_t> p_data, uint32_t p_required_align = 32);

	static void update_perf_report();

	struct BufferGetDataRequest
	{
		uint32_t frame_local_index = 0;
		uint32_t frame_local_count = 0;
		uint32_t size = 0;
	};

	struct Texture
	{
		struct SharedFallback
		{
			uint32_t revision = 1;
			RDD::TextureID texture;
			RDG::ResourceTracker* texture_tracker = nullptr;
			RDD::BufferID buffer;
			RDG::ResourceTracker* buffer_tracker = nullptr;
			bool raw_reinterpretation = false;
		};

		RDD::TextureID driver_id;
		TextureType type = TEXTURE_TYPE_MAX;
		DataFormat format = DATA_FORMAT_MAX;
		TextureSamples samples = TEXTURE_SAMPLES_MAX;
		TextureSliceType slice_type = TEXTURE_SLICE_MAX;
		Rect2i slice_rect;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		uint32_t layers = 0;
		uint32_t mipmaps = 0;
		uint32_t usage_flags = 0;
		uint32_t base_mipmap = 0;
		uint32_t base_layer = 0;

		Vector<DataFormat> allowed_shared_formats;
		bool is_resolve_buffer = false;
		bool is_discardable = false;
		bool is_subsampled = false;
		bool has_initial_data = false;
		bool pending_clear = false;

		uint32_t read_aspect_flags = 0;
		uint32_t barrier_aspect_flags = 0;
		bool bound = false;
		RID owner;

		RDG::ResourceTracker* draw_tracker = nullptr;
		HashMap<Rect2i, RDG::ResourceTracker*>* slice_trackers = nullptr;
		SharedFallback* shared_fallback = nullptr;
		int32_t transfer_worker_index = -1;
		uint64_t transfer_worker_operation = 0;

		RDD::TextureSubresourceRange barrier_range() const
		{
			RDD::TextureSubresourceRange r;
			r.aspect = barrier_aspect_flags;
			r.base_mipmap = base_mipmap;
			r.mipmap_count = mipmaps;
			r.base_layer = base_layer;
			r.layer_count = layers;
			return r;
		}

		TextureFormat texture_format() const
		{
			TextureFormat tf;
			tf.format = format;
			tf.width = width;
			tf.height = height;
			tf.depth = depth;
			tf.array_layers = layers;
			tf.mipmaps = mipmaps;
			tf.texture_type = type;
			tf.samples = samples;
			tf.usage_bits = usage_flags;
			tf.shareable_formats = allowed_shared_formats;
			tf.is_resolve_buffer = is_resolve_buffer;
			tf.is_discardable = is_discardable;
			return tf;
		}
	};

	static uint32_t _texture_layer_count(Texture* p_texture);
	static uint32_t _texture_alignment(Texture* p_texture);
	static Error _texture_initialize(RID p_texture, uint32_t p_layer, const Vector<uint8_t>& p_data,
		RDD::TextureLayout p_dst_layout, bool p_immediate_flush);
	static void _texture_check_shared_fallback(Texture* p_texture);
	static void _texture_update_shared_fallback(
		RID p_texture_rid, Texture* p_texture, bool p_for_writing);
	static void _texture_free_shared_fallback(Texture* p_texture);
	static void _texture_copy_shared(RID p_src_texture_rid, Texture* p_src_texture,
		RID p_dst_texture_rid, Texture* p_dst_texture);
	static void _texture_create_reinterpret_buffer(Texture* p_texture);
	static void _texture_clear_color(RID p_texture_rid, Texture* p_texture, const Color& p_color,
		uint32_t p_base_mipmap, uint32_t p_mipmaps, uint32_t p_base_layer, uint32_t p_layers);
	static uint32_t _texture_vrs_method_to_usage_bits();

	struct TextureGetDataRequest
	{
		uint32_t frame_local_index = 0;
		uint32_t frame_local_count = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		uint32_t mipmaps = 0;
		RDD::DataFormat format = RDD::DATA_FORMAT_MAX;
	};

	static RDG::ResourceUsage _vrs_usage_from_method(VRSMethod p_method);
	static RDD::PipelineStageBits _vrs_stages_from_method(VRSMethod p_method);
	static RDD::TextureLayout _vrs_layout_from_method(VRSMethod p_method);
	static void _vrs_detect_method();

	struct FramebufferFormatKey
	{
		Vector<AttachmentFormat> attachments;
		Vector<FramebufferPass> passes;
		uint32_t view_count = 1;
		VRSMethod vrs_method = VRS_METHOD_NONE;
		int32_t vrs_attachment = ATTACHMENT_UNUSED;
		Size2i vrs_texel_size;

		bool operator<(const FramebufferFormatKey& p_key) const
		{
			if (vrs_texel_size != p_key.vrs_texel_size) {
				return vrs_texel_size < p_key.vrs_texel_size;
			}
			if (vrs_attachment != p_key.vrs_attachment) {
				return vrs_attachment < p_key.vrs_attachment;
			}
			if (vrs_method != p_key.vrs_method) {
				return vrs_method < p_key.vrs_method;
			}
			if (view_count != p_key.view_count) {
				return view_count < p_key.view_count;
			}
			uint32_t pass_size = passes.size();
			uint32_t key_pass_size = p_key.passes.size();
			if (pass_size != key_pass_size) {
				return pass_size < key_pass_size;
			}
			const FramebufferPass* pass_ptr = passes.ptr();
			const FramebufferPass* key_pass_ptr = p_key.passes.ptr();

			for (uint32_t i = 0; i < pass_size; i++) {
				uint32_t attachment_size = pass_ptr[i].color_attachments.size();
				uint32_t key_attachment_size = key_pass_ptr[i].color_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				const int32_t* pass_attachment_ptr = pass_ptr[i].color_attachments.ptr();
				const int32_t* key_pass_attachment_ptr = key_pass_ptr[i].color_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				attachment_size = pass_ptr[i].input_attachments.size();
				key_attachment_size = key_pass_ptr[i].input_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				pass_attachment_ptr = pass_ptr[i].input_attachments.ptr();
				key_pass_attachment_ptr = key_pass_ptr[i].input_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				attachment_size = pass_ptr[i].resolve_attachments.size();
				key_attachment_size = key_pass_ptr[i].resolve_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				pass_attachment_ptr = pass_ptr[i].resolve_attachments.ptr();
				key_pass_attachment_ptr = key_pass_ptr[i].resolve_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				attachment_size = pass_ptr[i].preserve_attachments.size();
				key_attachment_size = key_pass_ptr[i].preserve_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				pass_attachment_ptr = pass_ptr[i].preserve_attachments.ptr();
				key_pass_attachment_ptr = key_pass_ptr[i].preserve_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				if (pass_ptr[i].depth_attachment != key_pass_ptr[i].depth_attachment) {
					return pass_ptr[i].depth_attachment < key_pass_ptr[i].depth_attachment;
				}
			}

			int as = attachments.size();
			int bs = p_key.attachments.size();
			if (as != bs) {
				return as < bs;
			}

			const AttachmentFormat* af_a = attachments.ptr();
			const AttachmentFormat* af_b = p_key.attachments.ptr();
			for (int i = 0; i < as; i++) {
				const AttachmentFormat& a = af_a[i];
				const AttachmentFormat& b = af_b[i];
				if (a.format != b.format) {
					return a.format < b.format;
				}
				if (a.samples != b.samples) {
					return a.samples < b.samples;
				}
				if (a.usage_flags != b.usage_flags) {
					return a.usage_flags < b.usage_flags;
				}
			}

			return false;
		}
	};

	static RDD::RenderPassID _render_pass_create(RenderingDeviceDriver* p_driver,
		const Vector<AttachmentFormat>& p_attachments, const Vector<FramebufferPass>& p_passes,
		VectorView<RDD::AttachmentLoadOp> p_load_ops,
		VectorView<RDD::AttachmentStoreOp> p_store_ops, uint32_t p_view_count = 1,
		VRSMethod p_vrs_method = VRS_METHOD_NONE, int32_t p_vrs_attachment = -1,
		Size2i p_vrs_texel_size = Size2i(), Vector<TextureSamples>* r_samples = nullptr);
	static RDD::RenderPassID _render_pass_create_from_graph(RenderingDeviceDriver* p_driver,
		VectorView<RDD::AttachmentLoadOp> p_load_ops,
		VectorView<RDD::AttachmentStoreOp> p_store_ops, void* p_user_data);

	struct FramebufferFormat
	{
		const RBMap<FramebufferFormatKey, FramebufferFormatID>::Element* E;
		RDD::RenderPassID render_pass;
		Vector<TextureSamples> pass_samples;
		uint32_t view_count = 1;
	};

	struct Framebuffer
	{
		FramebufferFormatID format_id;
		uint32_t storage_mask = 0;
		Vector<RID> texture_ids;
		InvalidationCallback invalidated_callback = nullptr;
		void* invalidated_callback_userdata = nullptr;
		RDG::FramebufferCache* framebuffer_cache = nullptr;
		Size2 size;
		uint32_t view_count;
	};

	struct VertexDescriptionKey
	{
		Vector<VertexAttribute> vertex_formats;

		bool operator==(const VertexDescriptionKey& p_key) const
		{
			int vdc = vertex_formats.size();
			int vdck = p_key.vertex_formats.size();
			if (vdc != vdck) {
				return false;
			}
			const VertexAttribute* a_ptr = vertex_formats.ptr();
			const VertexAttribute* b_ptr = p_key.vertex_formats.ptr();
			for (int i = 0; i < vdc; i++) {
				const VertexAttribute& a = a_ptr[i];
				const VertexAttribute& b = b_ptr[i];
				if (a.location != b.location || a.offset != b.offset || a.format != b.format ||
					a.stride != b.stride || a.frequency != b.frequency) {
					return false;
				}
			}
			return true;
		}

		uint32_t hash() const
		{
			int vdc = vertex_formats.size();
			uint32_t h = hash_murmur3_one_32(vdc);
			const VertexAttribute* ptr = vertex_formats.ptr();
			for (int i = 0; i < vdc; i++) {
				const VertexAttribute& vd = ptr[i];
				h = hash_murmur3_one_32(vd.location, h);
				h = hash_murmur3_one_32(vd.offset, h);
				h = hash_murmur3_one_32(vd.format, h);
				h = hash_murmur3_one_32(vd.stride, h);
				h = hash_murmur3_one_32(vd.frequency, h);
			}
			return hash_fmix32(h);
		}
	};

	struct VertexDescriptionHash
	{
		static _FORCE_INLINE_ uint32_t hash(const VertexDescriptionKey& p_key)
		{
			return p_key.hash();
		}
	};

	struct VertexDescriptionCache
	{
		Vector<VertexAttribute> vertex_formats;
		VertexAttributeBindingsMap bindings;
		RDD::VertexFormatID driver_id;
	};

	struct VertexArray
	{
		RID buffer;
		VertexFormatID description;
		int vertex_count = 0;
		uint32_t max_instances_allowed = 0;
		Vector<RDD::BufferID> buffers;
		Vector<RDG::ResourceTracker*> draw_trackers;
		Vector<uint64_t> offsets;
		Vector<int32_t> transfer_worker_indices;
		Vector<uint64_t> transfer_worker_operations;
		HashSet<RID> untracked_buffers;
	};

	struct IndexBuffer : public Buffer
	{
		uint32_t max_index = 0;
		uint32_t index_count = 0;
		IndexBufferFormat format = INDEX_BUFFER_FORMAT_UINT16;
		bool supports_restart_indices = false;
	};

	struct IndexArray
	{
		uint32_t max_index = 0;
		RDD::BufferID driver_id;
		RDG::ResourceTracker* draw_tracker = nullptr;
		uint32_t offset = 0;
		uint32_t indices = 0;
		IndexBufferFormat format = INDEX_BUFFER_FORMAT_UINT16;
		bool supports_restart_indices = false;
		int32_t transfer_worker_index = -1;
		uint64_t transfer_worker_operation = 0;
	};

	static uint32_t _creation_to_usage_bits(uint32_t p_creation_bits);

	struct UniformSetFormat
	{
		Vector<ShaderUniform> uniforms;

		_FORCE_INLINE_ bool operator<(const UniformSetFormat& p_other) const
		{
			if (uniforms.size() != p_other.uniforms.size()) {
				return uniforms.size() < p_other.uniforms.size();
			}
			for (int i = 0; i < uniforms.size(); i++) {
				if (uniforms[i] < p_other.uniforms[i]) {
					return true;
				}
				else if (p_other.uniforms[i] < uniforms[i]) {
					return false;
				}
			}
			return false;
		}
	};

	struct Shader : public ShaderReflection
	{
		String name;
		RDD::ShaderID driver_id;
		uint32_t layout_hash = 0;
		uint32_t stage_bits = 0;
		Vector<uint32_t> set_formats;
	};

	static String _shader_uniform_debug(RID p_shader, int p_set = -1);

	static const uint32_t MAX_UNIFORM_SETS = 16;
	static const uint32_t MAX_PUSH_CONSTANT_SIZE = 128;

	struct UniformSet
	{
		uint32_t format = 0;
		RID shader_id;
		uint32_t shader_set = 0;
		RDD::UniformSetID driver_id;

		struct AttachableTexture
		{
			uint32_t bind = 0;
			RID texture;
		};

		struct SharedTexture
		{
			uint32_t writing = 0;
			RID texture;
		};

		LocalVector<AttachableTexture> attachable_textures;
		Vector<RDG::ResourceTracker*> draw_trackers;
		Vector<RDG::ResourceUsage> draw_trackers_usage;
		HashMap<RID, RDG::ResourceUsage> untracked_usage;
		LocalVector<SharedTexture> shared_textures_to_update;
		LocalVector<RID> pending_clear_textures;
		Vector<RID> acceleration_structures;
		InvalidationCallback invalidated_callback = nullptr;
		void* invalidated_callback_userdata = nullptr;
	};

	static void _uniform_set_update_shared(UniformSet* p_uniform_set);

	struct RenderPipeline
	{
#ifdef DEBUG_ENABLED
		struct Validation
		{
			FramebufferFormatID framebuffer_format;
			uint32_t render_pass = 0;
			uint32_t dynamic_state = 0;
			VertexFormatID vertex_format;
			bool uses_restart_indices = false;
			uint32_t primitive_minimum = 0;
			uint32_t primitive_divisor = 0;
		} validation;
#endif
		RID shader;
		RDD::ShaderID shader_driver_id;
		uint32_t shader_layout_hash = 0;
		Vector<uint32_t> set_formats;
		RDD::PipelineID driver_id;
		uint32_t stage_bits = 0;
		uint32_t push_constant_size = 0;
	};

	static Vector<uint8_t> _load_pipeline_cache();
	static void _save_pipeline_cache(void* p_data);

	struct ComputePipeline
	{
		RID shader;
		RDD::ShaderID shader_driver_id;
		uint32_t shader_layout_hash = 0;
		Vector<uint32_t> set_formats;
		RDD::PipelineID driver_id;
		uint32_t push_constant_size = 0;
		uint32_t local_group_size[3] = {0, 0, 0};
	};

	struct RaytracingPipeline
	{
		RID layout_defining_shader;
		RDD::ShaderID layout_defining_shader_driver_id;
		uint32_t layout_defining_shader_layout_hash = 0;
		Vector<uint32_t> set_formats;
		RDD::RaytracingPipelineID driver_id;
		uint32_t push_constant_size = 0;
		Buffer sbt_buffer;
		uint32_t raygen_shader_count = 0;
		uint32_t miss_shader_count = 0;
		uint32_t hit_group_count = 0;
	};

	static Error _raytracing_pipeline_create_sbt_buffer(
		RDD::RaytracingPipelineID p_raytracing_pipeline, uint32_t p_raygen_shader_count,
		uint32_t p_miss_shader_count, Buffer& r_sbt_buffer);

	static uint32_t _get_swap_chain_desired_count();

	struct AccelerationStructure
	{
		RDD::AccelerationStructureID driver_id;
		RDD::AccelerationStructureType type = {};
		RDG::ResourceTracker* draw_tracker = nullptr;
		HashSet<RID> acceleration_structure_dependencies;
		bool invalidated = true;
		RDD::BufferID scratch_buffer;

		Vector<RDG::ResourceTracker*> draw_trackers;
		HashSet<RID> untracked_buffers;

		uint32_t max_instance_count = 0;

		struct InstanceBuffer
		{
			RDD::BufferID driver_id;
			uint64_t frame_used = 0;
			uint32_t used_size = 0;
			uint8_t* data_ptr = nullptr;
		};

		LocalVector<InstanceBuffer> instance_buffers;
	};

	static Error _acceleration_structure_scratch_buffer_create(
		AccelerationStructure* p_acceleration_structure);
	static void _blas_remove_tlas_dependencies(AccelerationStructure* p_blas, RID p_blas_id);
	static void _tlas_remove_blas_dependencies(AccelerationStructure* p_tlas, RID p_tlas_id);

	struct HitShaderBindingTable : Buffer
	{
		RID raytracing_pipeline_id;
		RDD::RaytracingPipelineID raytracing_pipeline;
		uint32_t index_offset = 0;
		uint32_t raytracing_pipeline_hit_group_count = 0;
		Vector<uint32_t> hit_group_indices;
		uint32_t used_hit_group_count = 0;
		uint32_t first_dirty_index = UINT32_MAX;
		uint32_t last_dirty_index = 0;

		using FreeList = RBMap<uint32_t, RBSet<uint32_t>>;
		using ReverseFreeList = RBMap<uint32_t, uint32_t>;
		FreeList free_list;
		ReverseFreeList reverse_free_list;
	};

	static RDD::BufferID _hit_sbt_buffer_create(uint32_t p_buffer_size);
	static void _hit_sbt_add_dirty_range(
		HitShaderBindingTable* p_hit_sbt, uint32_t p_offset, uint32_t p_count);

	struct DrawList
	{
		Rect2i viewport;
		bool active = false;

		struct SetState
		{
			uint32_t pipeline_expected_format = 0;
			uint32_t uniform_set_format = 0;
			RDD::UniformSetID uniform_set_driver_id;
			RID uniform_set;
			bool bound = false;
		};

		struct State
		{
			SetState sets[MAX_UNIFORM_SETS];
			uint32_t set_count = 0;
			RID pipeline;
			RID pipeline_shader;
			RDD::ShaderID pipeline_shader_driver_id;
			uint32_t pipeline_shader_layout_hash = 0;
			uint32_t pipeline_push_constant_size = 0;
			RID vertex_array;
			RID index_array;
			uint32_t draw_count = 0;
		} state;

#ifdef DEBUG_ENABLED
		struct Validation
		{
			uint32_t dynamic_state = 0;
			VertexFormatID vertex_format = INVALID_ID;
			uint32_t vertex_array_size = 0;
			uint32_t vertex_max_instances_allowed = 0xFFFFFFFF;
			bool index_buffer_uses_restart_indices = false;
			uint32_t index_array_count = 0;
			uint32_t index_array_max_index = 0;
			Vector<uint32_t> set_formats;
			Vector<bool> set_bound;
			Vector<RID> set_rids;
			bool pipeline_active = false;
			uint32_t pipeline_dynamic_state = 0;
			VertexFormatID pipeline_vertex_format = INVALID_ID;
			RID pipeline_shader;
			bool pipeline_uses_restart_indices = false;
			uint32_t pipeline_primitive_divisor = 0;
			uint32_t pipeline_primitive_minimum = 0;
			uint32_t pipeline_push_constant_size = 0;
			bool pipeline_push_constant_supplied = false;
		} validation;
#else
		struct Validation
		{
			uint32_t vertex_array_size = 0;
			uint32_t index_array_count = 0;
		} validation;
#endif
	};

	static void _draw_list_start(const Rect2i& p_viewport);
	static void _draw_list_end(Rect2i* r_last_viewport = nullptr);

	struct RaytracingList
	{
		bool active = false;

		struct SetState
		{
			uint32_t pipeline_expected_format = 0;
			uint32_t uniform_set_format = 0;
			RDD::UniformSetID uniform_set_driver_id;
			RID uniform_set;
			bool bound = false;
		};

		struct State
		{
			SetState sets[MAX_UNIFORM_SETS];
			uint32_t set_count = 0;
			RID pipeline;
			RDD::RaytracingPipelineID pipeline_driver_id;
			RID layout_defining_shader;
			RDD::ShaderID layout_defining_shader_driver_id;
			uint32_t layout_defining_shader_layout_hash = 0;
			uint8_t push_constant_data[MAX_PUSH_CONSTANT_SIZE] = {};
			uint32_t push_constant_size = 0;
			RDD::BufferID sbt_buffer;
			uint32_t raygen_shader_count = 0;
			uint32_t miss_shader_count = 0;
			uint32_t trace_count = 0;
		} state;

#ifdef DEBUG_ENABLED
		struct Validation
		{
			bool active = true;
			Vector<uint32_t> set_formats;
			Vector<bool> set_bound;
			Vector<RID> set_rids;
			bool pipeline_active = false;
			RID pipeline_shader;
			uint32_t invalid_set_from = 0;
			uint32_t pipeline_push_constant_size = 0;
			bool pipeline_push_constant_supplied = false;
		} validation;
#endif
	};

	struct ComputeList
	{
		bool active = false;

		struct SetState
		{
			uint32_t pipeline_expected_format = 0;
			uint32_t uniform_set_format = 0;
			RDD::UniformSetID uniform_set_driver_id;
			RID uniform_set;
			bool bound = false;
		};

		struct State
		{
			SetState sets[MAX_UNIFORM_SETS];
			uint32_t set_count = 0;
			RID pipeline;
			RID pipeline_shader;
			RDD::ShaderID pipeline_shader_driver_id;
			uint32_t pipeline_shader_layout_hash = 0;
			uint32_t local_group_size[3] = {0, 0, 0};
			uint8_t push_constant_data[MAX_PUSH_CONSTANT_SIZE] = {};
			uint32_t push_constant_size = 0;
			uint32_t dispatch_count = 0;
		} state;

#ifdef DEBUG_ENABLED
		struct Validation
		{
			Vector<uint32_t> set_formats;
			Vector<bool> set_bound;
			Vector<RID> set_rids;
			bool pipeline_active = false;
			RID pipeline_shader;
			uint32_t invalid_set_from = 0;
			uint32_t pipeline_push_constant_size = 0;
			bool pipeline_push_constant_supplied = false;
		} validation;
#endif
	};

	struct TransferWorker
	{
		uint32_t index = 0;
		RDD::BufferID staging_buffer;
		uint32_t max_transfer_size = 0;
		uint32_t staging_buffer_size_in_use = 0;
		uint32_t staging_buffer_size_allocated = 0;
		RDD::CommandBufferID command_buffer;
		RDD::CommandPoolID command_pool;
		RDD::FenceID command_fence;
		LocalVector<RDD::TextureBarrier> texture_barriers;
		bool recording = false;
		bool submitted = false;
		BinaryMutex thread_mutex;
		uint64_t operations_processed = 0;
		uint64_t operations_submitted = 0;
		uint64_t operations_counter = 0;
		BinaryMutex operations_mutex;
	};

	static TransferWorker* _acquire_transfer_worker(
		uint32_t p_transfer_size, uint32_t p_required_align, uint32_t& r_staging_offset);
	static void _release_transfer_worker(TransferWorker* p_transfer_worker);
	static void _end_transfer_worker(TransferWorker* p_transfer_worker);
	static void _submit_transfer_worker(TransferWorker* p_transfer_worker,
		VectorView<RDD::SemaphoreID> p_signal_semaphores = VectorView<RDD::SemaphoreID>());
	static void _wait_for_transfer_worker(TransferWorker* p_transfer_worker);
	static void _flush_barriers_for_transfer_worker(TransferWorker* p_transfer_worker);
	static void _check_transfer_worker_operation(
		uint32_t p_transfer_worker_index, uint64_t p_transfer_worker_operation);
	static void _check_transfer_worker_buffer(Buffer* p_buffer);
	static void _check_transfer_worker_texture(Texture* p_texture);
	static void _check_transfer_worker_vertex_array(VertexArray* p_vertex_array);
	static void _check_transfer_worker_index_array(IndexArray* p_index_array);
	static void _submit_transfer_workers(
		RDD::CommandBufferID p_draw_command_buffer = RDD::CommandBufferID());
	static void _submit_transfer_barriers(RDD::CommandBufferID p_draw_command_buffer);
	static void _wait_for_transfer_workers();
	static void _free_transfer_workers();

	static bool _texture_make_mutable(Texture* p_texture, RID p_texture_id);
	static bool _buffer_make_mutable(Buffer* p_buffer, RID p_buffer_id);
	static bool _vertex_array_make_mutable(
		VertexArray* p_vertex_array, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _index_array_make_mutable(
		IndexArray* p_index_array, RDG::ResourceTracker* p_resource_tracker);
	static bool _uniform_set_make_mutable(
		UniformSet* p_uniform_set, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _acceleration_structure_make_mutable(
		AccelerationStructure* p_acceleration_structure, RID p_resource_id,
		RDG::ResourceTracker* p_resource_tracker);
	static bool _dependency_make_mutable(
		RID p_id, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _dependencies_make_mutable_recursive(
		RID p_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _dependencies_make_mutable(RID p_id, RDG::ResourceTracker* p_resource_tracker);

	struct Frame
	{
		List<Buffer> buffers_to_dispose_of;
		List<Texture> textures_to_dispose_of;
		List<Framebuffer> framebuffers_to_dispose_of;
		List<RDD::SamplerID> samplers_to_dispose_of;
		List<Shader> shaders_to_dispose_of;
		List<UniformSet> uniform_sets_to_dispose_of;
		List<RenderPipeline> render_pipelines_to_dispose_of;
		List<ComputePipeline> compute_pipelines_to_dispose_of;
		List<AccelerationStructure> acceleration_structures_to_dispose_of;
		List<RaytracingPipeline> raytracing_pipelines_to_dispose_of;

		LocalVector<RDD::BufferID> download_buffer_staging_buffers;
		LocalVector<RDD::BufferCopyRegion> download_buffer_copy_regions;
		LocalVector<BufferGetDataRequest> download_buffer_get_data_requests;

		LocalVector<RDD::BufferID> download_texture_staging_buffers;
		LocalVector<RDD::BufferTextureCopyRegion> download_buffer_texture_copy_regions;
		LocalVector<uint32_t> download_texture_mipmap_offsets;
		LocalVector<TextureGetDataRequest> download_texture_get_data_requests;

		RDD::CommandPoolID command_pool;
		RDD::CommandBufferID command_buffer;
		RDD::SemaphoreID semaphore;
		RDD::FenceID fence;
		bool fence_signaled = false;

		LocalVector<RDD::SemaphoreID> semaphores_to_wait_on;
		LocalVector<RDD::SwapChainID> swap_chains_to_present;
		TightLocalVector<RDD::SemaphoreID> transfer_worker_semaphores;
		RDG::CommandBufferPool command_buffer_pool;

		struct Timestamp
		{
			String description;
			uint64_t value = 0;
		};

		RDD::QueryPoolID timestamp_pool;
		TightLocalVector<String> timestamp_names;
		TightLocalVector<uint64_t> timestamp_cpu_values;
		uint32_t timestamp_count = 0;
		TightLocalVector<String> timestamp_result_names;
		TightLocalVector<uint64_t> timestamp_cpu_result_values;
		TightLocalVector<uint64_t> timestamp_result_values;
		uint32_t timestamp_result_count = 0;
		uint64_t index = 0;
	};

	static void _free_pending_resources(int p_frame);

	static void execute_chained_cmds(bool p_present_swap_chain,
		RenderingDeviceDriver::FenceID p_draw_fence,
		RenderingDeviceDriver::SemaphoreID p_dst_draw_semaphore_to_signal);

private:
	struct Data
	{
		Thread::ID render_thread_id;

		RenderingContextDriver* context = nullptr;
		RenderingDeviceDriver* driver = nullptr;
		RenderingContextDriver::Device device;

		bool local_device_processing = false;
		bool is_main_instance = false;

		HashMap<RID, HashSet<RID>> dependency_map;
		HashMap<RID, HashSet<RID>> reverse_dependency_map;

		StagingBuffers upload_staging_buffers;
		StagingBuffers download_staging_buffers;

		bool descriptor_set_batching = true;
		bool split_swapchain_into_its_own_cmd_buffer = true;
		uint32_t gpu_copy_count = 0;
		uint32_t direct_copy_count = 0;
		uint32_t copy_bytes_count = 0;
		uint32_t prev_gpu_copy_count = 0;
		uint32_t prev_copy_bytes_count = 0;

		RID_Owner<Buffer, true> uniform_buffer_owner;
		RID_Owner<Buffer, true> storage_buffer_owner;
		RID_Owner<Buffer, true> texture_buffer_owner;

		RID_Owner<Texture, true> texture_owner;
		uint32_t texture_upload_region_size_px = 0;
		uint32_t texture_download_region_size_px = 0;

		VRSMethod vrs_method = VRS_METHOD_NONE;
		DataFormat vrs_format = DATA_FORMAT_MAX;
		Size2i vrs_texel_size;

		RBMap<FramebufferFormatKey, FramebufferFormatID> framebuffer_format_cache;
		HashMap<FramebufferFormatID, FramebufferFormat> framebuffer_formats;

		RID_Owner<Framebuffer, true> framebuffer_owner;
		RID_Owner<RDD::SamplerID, true> sampler_owner;
		RID_Owner<Buffer, true> vertex_buffer_owner;

		HashMap<VertexDescriptionKey, VertexFormatID, VertexDescriptionHash> vertex_format_cache;
		HashMap<VertexFormatID, VertexDescriptionCache> vertex_formats;

		RID_Owner<VertexArray, true> vertex_array_owner;
		RID_Owner<IndexBuffer, true> index_buffer_owner;
		RID_Owner<IndexArray, true> index_array_owner;

		RBMap<UniformSetFormat, uint32_t> uniform_set_format_cache;
		RID_Owner<Shader, true> shader_owner;

		RID_Owner<UniformSet, true> uniform_set_owner;
		RID_Owner<RenderPipeline, true> render_pipeline_owner;

		bool pipeline_cache_enabled = false;
		size_t pipeline_cache_size = 0;
		String pipeline_cache_file_path;

		RID_Owner<ComputePipeline, true> compute_pipeline_owner;
		RID_Owner<RaytracingPipeline, true> raytracing_pipeline_owner;

		HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID> screen_swap_chains;
		HashMap<DisplayServerEnums::WindowID, RDD::FramebufferID> screen_framebuffers;

		RID_Owner<AccelerationStructure, true> acceleration_structure_owner;
		RID_Owner<HitShaderBindingTable, true> hit_sbt_owner;

		DrawList draw_list;
		uint32_t draw_list_subpass_count = 0;
#ifdef DEBUG_ENABLED
		FramebufferFormatID draw_list_framebuffer_format = INVALID_ID;
#endif
		uint32_t draw_list_current_subpass = 0;
		LocalVector<RID> draw_list_bound_textures;

		RaytracingList raytracing_list;
		RaytracingList::State raytracing_list_barrier_state;

		ComputeList compute_list;
		ComputeList::State compute_list_barrier_state;

		TightLocalVector<TransferWorker*> transfer_worker_pool;
		uint32_t transfer_worker_pool_size = 0;
		uint32_t transfer_worker_pool_max_size = 1;
		TightLocalVector<uint64_t> transfer_worker_operation_used_by_draw;
		LocalVector<uint32_t> transfer_worker_pool_available_list;
		LocalVector<RDD::TextureBarrier> transfer_worker_pool_texture_barriers;
		BinaryMutex transfer_worker_pool_mutex;
		BinaryMutex transfer_worker_pool_texture_barriers_mutex;
		ConditionVariable transfer_worker_pool_condition;

		RenderingDeviceGraph draw_graph;
		RDD::CommandQueueFamilyID main_queue_family;
		RDD::CommandQueueFamilyID transfer_queue_family;
		RDD::CommandQueueFamilyID present_queue_family;
		RDD::CommandQueueID main_queue;
		RDD::CommandQueueID transfer_queue;
		RDD::CommandQueueID present_queue;

		uint32_t max_timestamp_query_elements = 0;
		int frame = 0;
		TightLocalVector<Frame> frames;
		uint64_t frames_drawn = 0;
		uint32_t frames_pending_resources_for_processing = 0u;

		SafeNumeric<uint64_t> texture_memory;
		SafeNumeric<uint64_t> buffer_memory;

#ifdef DEV_ENABLED
		HashMap<RID, String> resource_names;
#endif
	};

public:
	static inline Data* data = nullptr;

	static Error initialize(
		RenderingContextDriver* p_context, RenderingContextDriver::Device p_device);
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

	RenderingDevice() = delete;
	RenderingDevice(const RenderingDevice&) = delete;
	RenderingDevice& operator=(const RenderingDevice&) = delete;
	~RenderingDevice() = delete;
};

using RD = RenderingDevice;


