/**************************************************************************/
/*  rendering_device_driver_metal3.h                                      */
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

// --- Inlined/Folded from drivers/metal/metal3_objects.h ---
/**************************************************************************/
/*                                                                        */
/* Portions of this code were derived from MoltenVK.                      */
/*                                                                        */
/* Copyright (c) 2015-2023 The Brenwill Workshop Ltd.                     */
/* (http://www.brenwill.com)                                              */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/


#include "servers/rendering/rendering_device.h"

#include <Metal/Metal.hpp>

#include <initializer_list>
#include <optional>

namespace MTL3 {

// These types are defined in the global namespace (metal_objects_shared.h / rendering_device_driver_metal.h)
using ::MDAttachment;
using ::MDAttachmentType;
using ::MDCommandBufferBase;
using ::MDCommandBufferStateType;
using ::MDFrameBuffer;
using ::MDRenderPass;
using ::MDRingBuffer;
using ::MDSubpass;
using ::RenderStateBase;

using ::DynamicOffsetLayout;
using ::MDComputePipeline;
using ::MDComputeShader;
using ::MDLibrary;
using ::MDPipeline;
using ::MDPipelineType;
using ::MDRenderPipeline;
using ::MDRenderShader;
using ::MDShader;
using ::MDUniformSet;
using ::ShaderCacheEntry;
using ::ShaderLoadStrategy;
using ::UniformInfo;
using ::UniformSet;

using RDM = ::RenderingDeviceDriverMetal;

struct ResourceUsageEntry {
	StageResourceUsage usage = ResourceUnused;
	uint32_t unused = 0;

	ResourceUsageEntry() {}
	ResourceUsageEntry(StageResourceUsage p_usage) :
			usage(p_usage) {}
};

} // namespace MTL3

template <>
struct is_zero_constructible<MTL3::ResourceUsageEntry> : std::true_type {};

namespace MTL3 {

/*! Track the cumulative usage for a resource during a render or compute pass */
typedef HashMap<MTL::Resource *, ResourceUsageEntry> ResourceToStageUsage;

/*! Track resource and ensure they are resident prior to dispatch or draw commands.
 *
 * The primary purpose of this data structure is to track all the resources that must be made resident prior
 * to issuing the next dispatch or draw command. It aggregates all resources used from argument buffers.
 *
 * As an optimization, this data structure also tracks previous usage for resources, so that
 * it may avoid binding them again in later commands if the resource is already resident and its usage flagged.
 */
struct API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) ResourceTracker {
	// A constant specifying how many iterations a resource can remain in
	// the _previous HashSet before it will be removed permanently.
	//
	// Keeping them in the _previous HashMap reduces churn if resources are regularly
	// bound. 256 is arbitrary, but if an object remains unused for 256 encoders,
	// it will be released.
	static constexpr uint32_t RESOURCE_UNUSED_CLEANUP_COUNT = 256;

	// Used as a scratch buffer to periodically clean up resources from _previous.
	ResourceVector _scratch;
	// Tracks all resources and their prior usage for the duration of the encoder.
	ResourceToStageUsage _previous;
	// Tracks resources for the current command that must be made resident
	ResourceUsageMap _current;

	void merge_from(const ::ResourceUsageMap &p_from);
	void encode(MTL::RenderCommandEncoder *p_enc);
	void encode(MTL::ComputeCommandEncoder *p_enc);
	void reset();
};

struct BindingCache {
	struct BufferBinding {
		MTL::Buffer *buffer = nullptr;
		NS::UInteger offset = 0;

		bool operator!=(const BufferBinding &p_other) const {
			return buffer != p_other.buffer || offset != p_other.offset;
		}
	};

	LocalVector<MTL::Texture *> textures;
	LocalVector<MTL::SamplerState *> samplers;
	LocalVector<BufferBinding> buffers;

	_FORCE_INLINE_ void clear() {
		textures.clear();
		samplers.clear();
		buffers.clear();
	}

private:
	template <typename T>
	_FORCE_INLINE_ void ensure_size(LocalVector<T> &p_vec, uint32_t p_required) {
		if (p_vec.size() < p_required) {
			p_vec.resize_initialized(p_required);
		}
	}

public:
	_FORCE_INLINE_ bool update(NS::Range p_range, MTL::Texture *const *p_values) {
		if (p_range.length == 0) {
			return false;
		}
		uint32_t required = (uint32_t)(p_range.location + p_range.length);
		ensure_size(textures, required);
		bool changed = false;
		for (NS::UInteger i = 0; i < p_range.length; ++i) {
			uint32_t slot = (uint32_t)(p_range.location + i);
			MTL::Texture *value = p_values[i];
			if (textures[slot] != value) {
				textures[slot] = value;
				changed = true;
			}
		}
		return changed;
	}

	_FORCE_INLINE_ bool update(NS::Range p_range, MTL::SamplerState *const *p_values) {
		if (p_range.length == 0) {
			return false;
		}
		uint32_t required = (uint32_t)(p_range.location + p_range.length);
		ensure_size(samplers, required);
		bool changed = false;
		for (NS::UInteger i = 0; i < p_range.length; ++i) {
			uint32_t slot = (uint32_t)(p_range.location + i);
			MTL::SamplerState *value = p_values[i];
			if (samplers[slot] != value) {
				samplers[slot] = value;
				changed = true;
			}
		}
		return changed;
	}

	_FORCE_INLINE_ bool update(NS::Range p_range, MTL::Buffer *const *p_values, const NS::UInteger *p_offsets) {
		if (p_range.length == 0) {
			return false;
		}
		uint32_t required = (uint32_t)(p_range.location + p_range.length);
		ensure_size(buffers, required);
		BufferBinding *buffers_ptr = buffers.ptr() + p_range.location;
		bool changed = false;
		for (NS::UInteger i = 0; i < p_range.length; ++i) {
			BufferBinding &binding = *buffers_ptr;
			BufferBinding new_binding = {
				.buffer = p_values[i],
				.offset = p_offsets[i],
			};
			if (binding != new_binding) {
				binding = new_binding;
				changed = true;
			}
			++buffers_ptr;
		}
		return changed;
	}

	_FORCE_INLINE_ bool update(MTL::Buffer *p_buffer, NS::UInteger p_offset, uint32_t p_index) {
		uint32_t required = p_index + 1;
		ensure_size(buffers, required);
		BufferBinding &binding = buffers.ptr()[p_index];
		BufferBinding new_binding = {
			.buffer = p_buffer,
			.offset = p_offset,
		};
		if (binding != new_binding) {
			binding = new_binding;
			return true;
		}
		return false;
	}
};

// A type used to encode resources directly to a MTLCommandEncoder
struct DirectEncoder {
	MTL::CommandEncoder *encoder;
	BindingCache &cache;
	enum Mode {
		RENDER,
		COMPUTE
	};
	Mode mode;

	void set(MTL::Buffer **p_buffers, const NS::UInteger *p_offsets, NS::Range p_range);
	void set(MTL::Buffer *p_buffer, NS::UInteger p_offset, uint32_t p_index);
	void set(MTL::Texture **p_textures, NS::Range p_range);
	void set(MTL::SamplerState **p_samplers, NS::Range p_range);

	DirectEncoder(MTL::CommandEncoder *p_encoder, BindingCache &p_cache, Mode p_mode) :
			encoder(p_encoder), cache(p_cache), mode(p_mode) {}
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) MDCommandBuffer : public MDCommandBufferBase {
	friend class MDUniformSet;

private:
#pragma mark - Common State

	BindingCache binding_cache;

#pragma mark - Argument Buffer Ring Allocator

	using Alloc = MDRingBuffer::Allocation;

	// Used for argument buffers that contain dynamic uniforms.
	MDRingBuffer _scratch;

	/// Allocates from the ring buffer for dynamic argument buffers.
	Alloc allocate_arg_buffer(uint32_t p_size);

	struct {
		NS::SharedPtr<MTL::ResidencySet> rs;
	} _frame_state;

#pragma mark - Synchronization

	enum {
		STAGE_RENDER,
		STAGE_COMPUTE,
		STAGE_BLIT,
		STAGE_MAX,
	};
	bool use_barriers = false;
	MTL::Stages pending_after_stages[STAGE_MAX] = { 0, 0, 0 };
	MTL::Stages pending_before_queue_stages[STAGE_MAX] = { 0, 0, 0 };
	void _encode_barrier(MTL::CommandEncoder *p_enc);

	void reset();

	MTL::CommandQueue *queue = nullptr;
	NS::SharedPtr<MTL::CommandBuffer> commandBuffer;
	bool state_begin = false;

	MTL::CommandBuffer *command_buffer();

	void _end_compute_dispatch();
	void _end_blit();
	MTL::BlitCommandEncoder *_ensure_blit_encoder();

	enum class CopySource {
		Buffer,
		Texture,
	};
	void _copy_texture_buffer(CopySource p_source,
			RDD::TextureID p_texture,
			RDD::BufferID p_buffer,
			VectorView<RDD::BufferTextureCopyRegion> p_regions);

#pragma mark - Render

	void _render_set_dirty_state();
	void _render_bind_uniform_sets();
	void _bind_uniforms_argument_buffers(MDUniformSet *p_set, MDShader *p_shader, uint32_t p_set_index, uint32_t p_dynamic_offsets);
	void _bind_uniforms_direct(MDUniformSet *p_set, MDShader *p_shader, DirectEncoder p_enc, uint32_t p_set_index, uint32_t p_dynamic_offsets);

#pragma mark - Compute

	void _compute_set_dirty_state();
	void _compute_bind_uniform_sets();
	void _bind_uniforms_argument_buffers_compute(MDUniformSet *p_set, MDShader *p_shader, uint32_t p_set_index, uint32_t p_dynamic_offsets);

protected:
	void mark_push_constants_dirty() override;
	RenderStateBase &get_render_state_base() override { return render; }
	uint32_t get_current_view_count() const override { return render.get_subpass().view_count; }
	MDRenderPass *get_render_pass() const override { return render.pass; }
	MDFrameBuffer *get_frame_buffer() const override { return render.frameBuffer; }
	const MDSubpass &get_current_subpass() const override { return render.get_subpass(); }
	LocalVector<RDD::RenderPassClearValue> &get_clear_values() override { return render.clear_values; }
	const Rect2i &get_render_area() const override { return render.render_area; }
	void end_render_encoding() override { render.end_encoding(); }

public:
	struct RenderState : public RenderStateBase {
		MDRenderPass *pass = nullptr;
		MDFrameBuffer *frameBuffer = nullptr;
		MDRenderPipeline *pipeline = nullptr;
		LocalVector<RDD::RenderPassClearValue> clear_values;
		uint32_t current_subpass = UINT32_MAX;
		Rect2i render_area;
		bool is_rendering_entire_area = false;
		NS::SharedPtr<MTL::RenderPassDescriptor> desc;
		NS::SharedPtr<MTL::RenderCommandEncoder> encoder;
		MTL::Buffer *index_buffer = nullptr; // Buffer is owned by RDD.
		MTL::IndexType index_type = MTL::IndexTypeUInt16;
		uint32_t index_offset = 0;
		LocalVector<MTL::Buffer *> vertex_buffers;
		LocalVector<NS::UInteger> vertex_offsets;
		ResourceTracker resource_tracker;

		LocalVector<MDUniformSet *> uniform_sets;
		uint32_t dynamic_offsets = 0;
		// Bit mask of the uniform sets that are dirty, to prevent redundant binding.
		uint64_t uniform_set_mask = 0;

		_FORCE_INLINE_ void reset();
		void end_encoding();

		_ALWAYS_INLINE_ const MDSubpass &get_subpass() const {
			DEV_ASSERT(pass != nullptr);
			return pass->subpasses[current_subpass];
		}

		_FORCE_INLINE_ void mark_viewport_dirty() {
			if (viewports.is_empty()) {
				return;
			}
			dirty.set_flag(DirtyFlag::DIRTY_VIEWPORT);
		}

		_FORCE_INLINE_ void mark_scissors_dirty() {
			if (scissors.is_empty()) {
				return;
			}
			dirty.set_flag(DirtyFlag::DIRTY_SCISSOR);
		}

		_FORCE_INLINE_ void mark_vertex_dirty() {
			if (vertex_buffers.is_empty()) {
				return;
			}
			dirty.set_flag(DirtyFlag::DIRTY_VERTEX);
		}

		_FORCE_INLINE_ void mark_uniforms_dirty(std::initializer_list<uint32_t> l) {
			if (uniform_sets.is_empty()) {
				return;
			}
			for (uint32_t i : l) {
				if (i < uniform_sets.size() && uniform_sets[i] != nullptr) {
					uniform_set_mask |= 1 << i;
				}
			}
			dirty.set_flag(DirtyFlag::DIRTY_UNIFORMS);
		}

		_FORCE_INLINE_ void mark_uniforms_dirty() {
			if (uniform_sets.is_empty()) {
				return;
			}
			for (uint32_t i = 0; i < uniform_sets.size(); i++) {
				if (uniform_sets[i] != nullptr) {
					uniform_set_mask |= 1 << i;
				}
			}
			dirty.set_flag(DirtyFlag::DIRTY_UNIFORMS);
		}

		_FORCE_INLINE_ void mark_blend_dirty() {
			if (!blend_constants.has_value()) {
				return;
			}
			dirty.set_flag(DirtyFlag::DIRTY_BLEND);
		}

		MTL::ScissorRect clip_to_render_area(MTL::ScissorRect p_rect) const {
			uint32_t raLeft = render_area.position.x;
			uint32_t raRight = raLeft + render_area.size.width;
			uint32_t raBottom = render_area.position.y;
			uint32_t raTop = raBottom + render_area.size.height;

			p_rect.x = CLAMP(p_rect.x, raLeft, MAX(raRight - 1, raLeft));
			p_rect.y = CLAMP(p_rect.y, raBottom, MAX(raTop - 1, raBottom));
			p_rect.width = MIN(p_rect.width, raRight - p_rect.x);
			p_rect.height = MIN(p_rect.height, raTop - p_rect.y);

			return p_rect;
		}

		Rect2i clip_to_render_area(Rect2i p_rect) const {
			int32_t raLeft = render_area.position.x;
			int32_t raRight = raLeft + render_area.size.width;
			int32_t raBottom = render_area.position.y;
			int32_t raTop = raBottom + render_area.size.height;

			p_rect.position.x = CLAMP(p_rect.position.x, raLeft, MAX(raRight - 1, raLeft));
			p_rect.position.y = CLAMP(p_rect.position.y, raBottom, MAX(raTop - 1, raBottom));
			p_rect.size.width = MIN(p_rect.size.width, raRight - p_rect.position.x);
			p_rect.size.height = MIN(p_rect.size.height, raTop - p_rect.position.y);

			return p_rect;
		}

	} render;

	// State specific for a compute pass.
	struct ComputeState {
		MDComputePipeline *pipeline = nullptr;
		NS::SharedPtr<MTL::ComputeCommandEncoder> encoder;
		ResourceTracker resource_tracker;
		// clang-format off
		enum DirtyFlag: uint16_t {
			DIRTY_NONE     = 0,
			DIRTY_PIPELINE = 1 << 0, //! pipeline state
			DIRTY_UNIFORMS = 1 << 1, //! uniform sets
			DIRTY_PUSH     = 1 << 2, //! push constants
			DIRTY_ALL      = (1 << 3) - 1,
		};
		// clang-format on
		uint32_t dirty = DIRTY_NONE;

		LocalVector<MDUniformSet *> uniform_sets;
		uint32_t dynamic_offsets = 0;
		// Bit mask of the uniform sets that are dirty, to prevent redundant binding.
		uint64_t uniform_set_mask = 0;

		_FORCE_INLINE_ void reset();
		void end_encoding();

		_FORCE_INLINE_ void mark_uniforms_dirty() {
			if (uniform_sets.is_empty()) {
				return;
			}
			for (uint32_t i = 0; i < uniform_sets.size(); i++) {
				if (uniform_sets[i] != nullptr) {
					uniform_set_mask |= 1 << i;
				}
			}
			dirty.set_flag(DirtyFlag::DIRTY_UNIFORMS);
		}
	} compute;

	// State specific to a blit pass.
	struct {
		NS::SharedPtr<MTL::BlitCommandEncoder> encoder;
		_FORCE_INLINE_ void reset() {
			encoder.reset();
		}
	} blit;

	_FORCE_INLINE_ MTL::CommandBuffer *get_command_buffer() const {
		return commandBuffer.get();
	}

	void begin() override;
	void commit() override;
	void end() override;

	void bind_pipeline(RDD::PipelineID p_pipeline) override;

#pragma mark - Render Commands

	void render_bind_uniform_sets(VectorView<RDD::UniformSetID> p_uniform_sets, RDD::ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) override;
	void render_clear_attachments(VectorView<RDD::AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) override;
	void render_begin_pass(RDD::RenderPassID p_render_pass,
			RDD::FramebufferID p_frameBuffer,
			RDD::CommandBufferType p_cmd_buffer_type,
			const Rect2i &p_rect,
			VectorView<RDD::RenderPassClearValue> p_clear_values) override;
	void render_next_subpass() override;
	void render_draw(uint32_t p_vertex_count,
			uint32_t p_instance_count,
			uint32_t p_base_vertex,
			uint32_t p_first_instance) override;
	void render_bind_vertex_buffers(uint32_t p_binding_count, const RDD::BufferID *p_buffers, const uint64_t *p_offsets, uint64_t p_dynamic_offsets) override;
	void render_bind_index_buffer(RDD::BufferID p_buffer, RDD::IndexBufferFormat p_format, uint64_t p_offset) override;

	void render_draw_indexed(uint32_t p_index_count,
			uint32_t p_instance_count,
			uint32_t p_first_index,
			int32_t p_vertex_offset,
			uint32_t p_first_instance) override;

	void render_draw_indexed_indirect(RDD::BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) override;
	void render_draw_indexed_indirect_count(RDD::BufferID p_indirect_buffer, uint64_t p_offset, RDD::BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) override;
	void render_draw_indirect(RDD::BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) override;
	void render_draw_indirect_count(RDD::BufferID p_indirect_buffer, uint64_t p_offset, RDD::BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) override;

	void render_end_pass() override;

#pragma mark - Compute Commands

	void compute_bind_uniform_sets(VectorView<RDD::UniformSetID> p_uniform_sets, RDD::ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) override;
	void compute_dispatch(uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) override;
	void compute_dispatch_indirect(RDD::BufferID p_indirect_buffer, uint64_t p_offset) override;

#pragma mark - Transfer

private:
	MTL::RenderCommandEncoder *get_new_render_encoder_with_descriptor(MTL::RenderPassDescriptor *p_desc);

public:
	void resolve_texture(RDD::TextureID p_src_texture, RDD::TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, RDD::TextureID p_dst_texture, RDD::TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) override;
	void clear_color_texture(RDD::TextureID p_texture, RDD::TextureLayout p_texture_layout, const Color &p_color, const RDD::TextureSubresourceRange &p_subresources) override;
	void clear_depth_stencil_texture(RDD::TextureID p_texture, RDD::TextureLayout p_texture_layout, float p_depth, uint8_t p_stencil, const RDD::TextureSubresourceRange &p_subresources) override;
	void clear_buffer(RDD::BufferID p_buffer, uint64_t p_offset, uint64_t p_size) override;
	void copy_buffer(RDD::BufferID p_src_buffer, RDD::BufferID p_dst_buffer, VectorView<RDD::BufferCopyRegion> p_regions) override;
	void copy_texture(RDD::TextureID p_src_texture, RDD::TextureID p_dst_texture, VectorView<RDD::TextureCopyRegion> p_regions) override;
	void copy_buffer_to_texture(RDD::BufferID p_src_buffer, RDD::TextureID p_dst_texture, VectorView<RDD::BufferTextureCopyRegion> p_regions) override;
	void copy_texture_to_buffer(RDD::TextureID p_src_texture, RDD::BufferID p_dst_buffer, VectorView<RDD::BufferTextureCopyRegion> p_regions) override;

#pragma mark - Synchronization

	void pipeline_barrier(uint32_t p_src_stages,
			uint32_t p_dst_stages,
			VectorView<RDD::MemoryAccessBarrier> p_memory_barriers,
			VectorView<RDD::BufferBarrier> p_buffer_barriers,
			VectorView<RDD::TextureBarrier> p_texture_barriers,
			VectorView<RDD::AccelerationStructureBarrier> p_acceleration_structure_barriers) override;

#pragma mark - Debugging

	void begin_label(const char *p_label_name, const Color &p_color) override;
	void end_label() override;

	MDCommandBuffer(MTL::CommandQueue *p_queue, ::RenderingDeviceDriverMetal *p_device_driver);
	MDCommandBuffer() = default;
};

} // namespace MTL3

// C++ helper to get mipmap level size from texture
_FORCE_INLINE_ static MTL::Size mipmapLevelSizeFromTexture(MTL::Texture *p_tex, NS::UInteger p_level) {
	MTL::Size lvlSize;
	lvlSize.width = MAX(p_tex->width() >> p_level, 1UL);
	lvlSize.height = MAX(p_tex->height() >> p_level, 1UL);
	lvlSize.depth = MAX(p_tex->depth() >> p_level, 1UL);
	return lvlSize;
}
// --- Inlined/Folded from drivers/metal/rendering_device_driver_metal.h ---
// --- Inlined/Folded from drivers/metal/metal_objects_shared.h ---
// --- Inlined/Folded from drivers/metal/pixel_formats.h ---
/**************************************************************************/
/*                                                                        */
/* Portions of this code were derived from MoltenVK.                      */
/*                                                                        */
/* Copyright (c) 2015-2023 The Brenwill Workshop Ltd.                     */
/* (http://www.brenwill.com)                                              */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

#include "core/typedefs.h"

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")
// --- Folded from drivers/metal/inflection_map.h ---
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

#include <iterator>

/// An unordered map that splits elements between a fast-access vector of LinearCount consecutively
/// indexed elements, and a slower-access map holding sparse indexes larger than LinearCount.
///
/// \tparam KeyType is used to lookup values, and must be a type that is convertible to an unsigned integer.
/// \tparam ValueType must have an empty constructor (default or otherwise).
/// \tparam LinearCount
/// \tparam IndexType must be a type that is convertible to an unsigned integer (eg. uint8_t...uint64_t), and which is large enough to represent the number of values in this map.
template <typename KeyType, typename ValueType, size_t LinearCount, typename IndexType = uint16_t>
class InflectionMap {
public:
	using value_type = ValueType;
	class Iterator {
		InflectionMap *map;
		IndexType index;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = ValueType;
		using pointer = value_type *;
		using reference = value_type &;

		Iterator() :
				map(nullptr), index(0) {}
		Iterator(InflectionMap &p_m, const IndexType p_i) :
				map(&p_m), index(p_i) {}

		Iterator &operator=(const Iterator &p_it) {
			map = p_it.map;
			index = p_it.index;
			return *this;
		}

		ValueType *operator->() { return &map->_values[index]; }
		ValueType &operator*() { return map->_values[index]; }
		operator ValueType *() { return &map->_values[index]; }

		bool operator==(const Iterator &p_it) const { return map == p_it.map && index == p_it.index; }
		bool operator!=(const Iterator &p_it) const { return map != p_it.map || index != p_it.index; }

		Iterator &operator++() {
			index++;
			return *this;
		}
		Iterator operator++(int) {
			Iterator t = *this;
			index++;
			return t;
		}

		bool is_valid() const { return index < map->_values.size(); }
	};

	const ValueType &operator[](const KeyType p_idx) const { return get_value(p_idx); }
	ValueType &operator[](const KeyType p_idx) { return get_value(p_idx); }

	Iterator begin() { return Iterator(*this, 0); }
	Iterator end() { return Iterator(*this, _values.size()); }

	bool is_empty() { return _values.is_empty(); }
	size_t size() { return _values.size(); }
	void reserve(size_t p_new_cap) { _values.reserve(p_new_cap); }

protected:
	static constexpr IndexType INVALID = std::numeric_limits<IndexType>::max();
	typedef struct IndexValue {
		IndexType value = INVALID;
	} IndexValue;

	// Returns a reference to the value at the index.
	// If the index has not been initialized, add an empty element at
	// the end of the values array, and set the index to its position.
	ValueType &get_value(KeyType p_idx) {
		IndexValue *val_idx = p_idx < LinearCount ? &_linear_indexes[p_idx] : _inflection_indexes.getptr(p_idx);
		if (val_idx == nullptr || val_idx->value == INVALID) {
			_values.push_back({});
			if (val_idx == nullptr) {
				val_idx = &_inflection_indexes.insert(p_idx, {})->value;
			}
			val_idx->value = _values.size() - 1;
		}
		return _values[val_idx->value];
	}

	TightLocalVector<ValueType> _values;
	HashMap<KeyType, IndexValue> _inflection_indexes;
	IndexValue _linear_indexes[LinearCount];
};

// --- End drivers/metal/inflection_map.h ---
// --- Folded from drivers/metal/metal_device_properties.h ---
/**************************************************************************/
/*                                                                        */
/* Portions of this code were derived from MoltenVK.                      */
/*                                                                        */
/* Copyright (c) 2015-2023 The Brenwill Workshop Ltd.                     */
/* (http://www.brenwill.com)                                              */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

#include "servers/rendering/rendering_device.h"

#include <Metal/Metal.hpp>

#include <cstddef>

/** The buffer index to use for vertex content. */
const static uint32_t VERT_CONTENT_BUFFER_INDEX = 0;
const static uint32_t MAX_COLOR_ATTACHMENT_COUNT = 8;

enum SampleCount : NS::UInteger {
	SampleCount1 = (1UL << 0),
	SampleCount2 = (1UL << 1),
	SampleCount4 = (1UL << 2),
	SampleCount8 = (1UL << 3),
	SampleCount16 = (1UL << 4),
	SampleCount32 = (1UL << 5),
	SampleCount64 = (1UL << 6),
};

_FORCE_INLINE_ SampleCount operator|(SampleCount a, SampleCount b) {
	return static_cast<SampleCount>(static_cast<NS::UInteger>(a) | static_cast<NS::UInteger>(b));
}

_FORCE_INLINE_ SampleCount &operator|=(SampleCount &a, SampleCount b) {
	return a = a | b;
}

_FORCE_INLINE_ SampleCount operator<<(SampleCount a, int shift) {
	return static_cast<SampleCount>(static_cast<NS::UInteger>(a) << shift);
}

_FORCE_INLINE_ SampleCount &operator<<=(SampleCount &a, int shift) {
	return a = a << shift;
}

struct API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) MetalFeatures {
	/// Maximum version of the Metal Shading Language version available.
	uint32_t msl_max_version = 0;
	/*! @brief Target version of the Metal Shading Language used to translate shaders.
	 *
	 * This can be used to override the features used to generate shaders. Primarily
	 * for engine developers for testing.
	 */
	uint32_t msl_target_version = 0;
	MTL::GPUFamily highestFamily = MTL::GPUFamilyApple4;
	bool supportsBCTextureCompression = false;
	bool supportsDepth24Stencil8 = false;
	bool supports32BitFloatFiltering = false;
	bool supports32BitMSAA = false;
	bool supportsMac = TARGET_OS_OSX;
	SampleCount supportedSampleCounts = SampleCount1;
	long hostMemoryPageSize = 0;
	bool layeredRendering = false;
	bool multisampleLayeredRendering = false;
	bool quadPermute = false; /**< If true, quadgroup permutation functions (vote, ballot, shuffle) are supported in shaders. */
	bool simdPermute = false; /**< If true, SIMD-group permutation functions (vote, ballot, shuffle) are supported in shaders. */
	bool simdReduction = false; /**< If true, SIMD-group reduction functions (arithmetic) are supported in shaders. */
	bool tessellationShader = false; /**< If true, tessellation shaders are supported. */
	bool imageCubeArray = false; /**< If true, image cube arrays are supported. */
	MTL::ArgumentBuffersTier argument_buffers_tier = MTL::ArgumentBuffersTier1;
	bool needs_arg_encoders = true; /**< If true, argument encoders are required to encode arguments into an argument buffer. */
	bool use_argument_buffers = true; /**< If true, argument buffers are can be used instead of slot binding, if available. */
	bool metal_fx_spatial = false; /**< If true, Metal FX spatial functions are supported. */
	bool metal_fx_temporal = false; /**< If true, Metal FX temporal functions are supported. */
	bool supports_gpu_address = false; /**< If true, referencing a GPU address in a shader is supported. */
	bool supports_image_atomic_32_bit = false; /**< If true, 32-bit atomic operations on images are supported by the GPU. */
	bool supports_image_atomic_64_bit = false; /**< If true, 64-bit atomic operations on images are supported by the GPU. */
	bool supports_native_image_atomics = false; /**< If true, native image atomic operations are supported by the OS. */
	bool supports_border_color = false; /**< If true, sampler border color (clamp-to-border) is supported. Requires Apple7+. */
	bool supports_residency_sets = false; /**< If true, residency sets (MTLResidencySet) are supported by the OS. */
	bool supports_msaa_depth_resolve = false;

	/*!
	 * Check if argument buffers are fully supported, which requires tier 2 support and no need for argument encoders.
	 */
	_FORCE_INLINE_ bool argument_buffers_supported() const {
		return argument_buffers_tier == MTL::ArgumentBuffersTier2 && needs_arg_encoders == false;
	}

	/*!
	 * Check if argument buffers can be used, which requires that they are supported and that the user has enabled their use.
	 */
	_FORCE_INLINE_ bool argument_buffers_enabled() const {
		return use_argument_buffers && argument_buffers_supported();
	}
};

struct MetalLimits {
	uint64_t maxImageArrayLayers;
	uint64_t maxFramebufferHeight;
	uint64_t maxFramebufferWidth;
	uint64_t maxImageDimension1D;
	uint64_t maxImageDimension2D;
	uint64_t maxImageDimension3D;
	uint64_t maxImageDimensionCube;
	uint64_t maxViewportDimensionX;
	uint64_t maxViewportDimensionY;
	MTL::Size maxThreadsPerThreadGroup;
	MTL::Size maxComputeWorkGroupCount;
	uint64_t maxBoundDescriptorSets;
	uint64_t maxColorAttachments;
	uint64_t maxTexturesPerArgumentBuffer;
	uint64_t maxSamplersPerArgumentBuffer;
	uint64_t maxBuffersPerArgumentBuffer;
	uint64_t maxBufferLength;
	uint64_t minUniformBufferOffsetAlignment;
	uint64_t maxVertexDescriptorLayoutStride;
	uint16_t maxViewports;
	uint32_t maxPerStageBufferCount; /**< The total number of per-stage Metal buffers available for shader uniform content and attributes. */
	uint32_t maxPerStageTextureCount; /**< The total number of per-stage Metal textures available for shader uniform content. */
	uint32_t maxPerStageSamplerCount; /**< The total number of per-stage Metal samplers available for shader uniform content. */
	uint32_t maxVertexInputAttributes;
	uint32_t maxVertexInputBindings;
	uint32_t maxVertexInputBindingStride;
	uint32_t maxDrawIndexedIndexValue;
	uint32_t maxShaderVaryings;
	uint32_t maxThreadGroupMemoryAllocation;

	double temporalScalerInputContentMinScale;
	double temporalScalerInputContentMaxScale;

	uint32_t minSubgroupSize; /**< The minimum number of threads in a SIMD-group. */
	uint32_t maxSubgroupSize; /**< The maximum number of threads in a SIMD-group. */
	uint32_t subgroupSupportedShaderStages;
	uint32_t subgroupSupportedOperations; /**< The subgroup operations supported by the device. */
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) MetalDeviceProperties {
private:
	void init_features(MTL::Device *p_device);
	void init_limits(MTL::Device *p_device);
	void init_os_props();

public:
	MetalFeatures features;
	MetalLimits limits;

	// maj * 10000 + min * 100 + patch
	uint32_t os_version;

	SampleCount find_nearest_supported_sample_count(RDD::TextureSamples p_samples) const;

	MetalDeviceProperties(MTL::Device *p_device);
	~MetalDeviceProperties();

private:
	static const SampleCount sample_count[RDD::TextureSamples::TEXTURE_SAMPLES_MAX];
};

// --- End drivers/metal/metal_device_properties.h ---
#include "servers/rendering/rendering_device.h"

#ifdef __OBJC__
#include <Metal/Metal.h>
#endif
#include <Metal/Metal.hpp>

#include <iterator>

#pragma mark -
#pragma mark Metal format capabilities

using RDC = RenderingDeviceCommons;

typedef enum : uint16_t {

	kMTLFmtCapsNone = 0,
	/*! The format can be used in a shader read operation. */
	kMTLFmtCapsRead = (1 << 0),
	/*! The format can be used in a shader filter operation during sampling. */
	kMTLFmtCapsFilter = (1 << 1),
	/*! The format can be used in a shader write operation. */
	kMTLFmtCapsWrite = (1 << 2),
	/*! The format can be used with atomic operations. */
	kMTLFmtCapsAtomic = (1 << 3),
	/*! The format can be used as a color attachment. */
	kMTLFmtCapsColorAtt = (1 << 4),
	/*! The format can be used as a depth-stencil attachment. */
	kMTLFmtCapsDSAtt = (1 << 5),
	/*! The format can be used with blend operations. */
	kMTLFmtCapsBlend = (1 << 6),
	/*! The format can be used as a destination for multisample antialias (MSAA) data. */
	kMTLFmtCapsMSAA = (1 << 7),
	/*! The format can be used as a resolve attachment. */
	kMTLFmtCapsResolve = (1 << 8),
	kMTLFmtCapsVertex = (1 << 9),

	kMTLFmtCapsRF = (kMTLFmtCapsRead | kMTLFmtCapsFilter),
	kMTLFmtCapsRC = (kMTLFmtCapsRead | kMTLFmtCapsColorAtt),
	kMTLFmtCapsRCB = (kMTLFmtCapsRC | kMTLFmtCapsBlend),
	kMTLFmtCapsRCM = (kMTLFmtCapsRC | kMTLFmtCapsMSAA),
	kMTLFmtCapsRCMB = (kMTLFmtCapsRCM | kMTLFmtCapsBlend),
	kMTLFmtCapsRWC = (kMTLFmtCapsRC | kMTLFmtCapsWrite),
	kMTLFmtCapsRWCB = (kMTLFmtCapsRWC | kMTLFmtCapsBlend),
	kMTLFmtCapsRWCM = (kMTLFmtCapsRWC | kMTLFmtCapsMSAA),
	kMTLFmtCapsRWCMB = (kMTLFmtCapsRWCM | kMTLFmtCapsBlend),
	kMTLFmtCapsRFCMRB = (kMTLFmtCapsRCMB | kMTLFmtCapsFilter | kMTLFmtCapsResolve),
	kMTLFmtCapsRFWCMB = (kMTLFmtCapsRWCMB | kMTLFmtCapsFilter),
	kMTLFmtCapsAll = (kMTLFmtCapsRFWCMB | kMTLFmtCapsResolve),

	kMTLFmtCapsDRM = (kMTLFmtCapsDSAtt | kMTLFmtCapsRead | kMTLFmtCapsMSAA),
	kMTLFmtCapsDRFM = (kMTLFmtCapsDRM | kMTLFmtCapsFilter),
	kMTLFmtCapsDRMR = (kMTLFmtCapsDRM | kMTLFmtCapsResolve),
	kMTLFmtCapsDRFMR = (kMTLFmtCapsDRFM | kMTLFmtCapsResolve),

	kMTLFmtCapsChromaSubsampling = kMTLFmtCapsRF,
	kMTLFmtCapsMultiPlanar = kMTLFmtCapsChromaSubsampling,
} MTLFmtCaps;

inline MTLFmtCaps operator|(MTLFmtCaps p_left, MTLFmtCaps p_right) {
	return static_cast<MTLFmtCaps>(static_cast<uint32_t>(p_left) | p_right);
}

inline MTLFmtCaps &operator|=(MTLFmtCaps &p_left, MTLFmtCaps p_right) {
	return (p_left = p_left | p_right);
}

#pragma mark -
#pragma mark Metal view classes

enum class MTLViewClass : uint8_t {
	None,
	Color8,
	Color16,
	Color32,
	Color64,
	Color128,
	PVRTC_RGB_2BPP,
	PVRTC_RGB_4BPP,
	PVRTC_RGBA_2BPP,
	PVRTC_RGBA_4BPP,
	EAC_R11,
	EAC_RG11,
	EAC_RGBA8,
	ETC2_RGB8,
	ETC2_RGB8A1,
	ASTC_4x4,
	ASTC_5x4,
	ASTC_5x5,
	ASTC_6x5,
	ASTC_6x6,
	ASTC_8x5,
	ASTC_8x6,
	ASTC_8x8,
	ASTC_10x5,
	ASTC_10x6,
	ASTC_10x8,
	ASTC_10x10,
	ASTC_12x10,
	ASTC_12x12,
	BC1_RGBA,
	BC2_RGBA,
	BC3_RGBA,
	BC4_R,
	BC5_RG,
	BC6H_RGB,
	BC7_RGBA,
	Depth24_Stencil8,
	Depth32_Stencil8,
	BGRA10_XR,
	BGR10_XR
};

#pragma mark -
#pragma mark Format descriptors

/** Enumerates the data type of a format. */
enum class MTLFormatType {
	None, /**< Format type is unknown. */
	ColorHalf, /**< A 16-bit floating point color. */
	ColorFloat, /**< A 32-bit floating point color. */
	ColorInt8, /**< A signed 8-bit integer color. */
	ColorUInt8, /**< An unsigned 8-bit integer color. */
	ColorInt16, /**< A signed 16-bit integer color. */
	ColorUInt16, /**< An unsigned 16-bit integer color. */
	ColorInt32, /**< A signed 32-bit integer color. */
	ColorUInt32, /**< An unsigned 32-bit integer color. */
	DepthStencil, /**< A depth and stencil value. */
	Compressed, /**< A block-compressed color. */
};

struct Extent2D {
	uint32_t width;
	uint32_t height;
};

struct ComponentMapping {
	RDC::TextureSwizzle r = RDC::TEXTURE_SWIZZLE_IDENTITY;
	RDC::TextureSwizzle g = RDC::TEXTURE_SWIZZLE_IDENTITY;
	RDC::TextureSwizzle b = RDC::TEXTURE_SWIZZLE_IDENTITY;
	RDC::TextureSwizzle a = RDC::TEXTURE_SWIZZLE_IDENTITY;
};

/** Describes the properties of a DataFormat, including the corresponding Metal pixel and vertex format. */
struct DataFormatDesc {
	RDC::DataFormat dataFormat;
	MTL::PixelFormat mtlPixelFormat;
	MTL::PixelFormat mtlPixelFormatSubstitute;
	MTL::VertexFormat mtlVertexFormat;
	MTL::VertexFormat mtlVertexFormatSubstitute;
	uint8_t chromaSubsamplingPlaneCount;
	uint8_t chromaSubsamplingComponentBits;
	Extent2D blockTexelSize;
	uint32_t bytesPerBlock;
	MTLFormatType formatType;
	ComponentMapping componentMapping;
	const char *name;
	bool hasReportedSubstitution;

	inline double bytesPerTexel() const { return (double)bytesPerBlock / (double)(blockTexelSize.width * blockTexelSize.height); }

	inline bool isSupported() const { return (mtlPixelFormat != MTL::PixelFormatInvalid || chromaSubsamplingPlaneCount > 1); }
	inline bool isSupportedOrSubstitutable() const { return isSupported() || (mtlPixelFormatSubstitute != MTL::PixelFormatInvalid); }

	inline bool vertexIsSupported() const { return (mtlVertexFormat != MTL::VertexFormatInvalid); }
	inline bool vertexIsSupportedOrSubstitutable() const { return vertexIsSupported() || (mtlVertexFormatSubstitute != MTL::VertexFormatInvalid); }

	bool needsSwizzle() const {
		return (componentMapping.r != RDC::TEXTURE_SWIZZLE_IDENTITY ||
				componentMapping.g != RDC::TEXTURE_SWIZZLE_IDENTITY ||
				componentMapping.b != RDC::TEXTURE_SWIZZLE_IDENTITY ||
				componentMapping.a != RDC::TEXTURE_SWIZZLE_IDENTITY);
	}
};

/** Describes the properties of a MTL::PixelFormat or MTL::VertexFormat. */
struct MTLFormatDesc {
	union {
		MTL::PixelFormat mtlPixelFormat;
		MTL::VertexFormat mtlVertexFormat;
	};
	RDC::DataFormat dataFormat = RDC::DATA_FORMAT_MAX;
	MTLFmtCaps mtlFmtCaps;
	MTLViewClass mtlViewClass;
	MTL::PixelFormat mtlPixelFormatLinear;
	const char *name = nullptr;

	inline bool isSupported() const { return (mtlPixelFormat != MTL::PixelFormatInvalid) && (mtlFmtCaps != kMTLFmtCapsNone); }
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) PixelFormats {
	using DataFormat = RDC::DataFormat;

public:
	/** Returns whether the DataFormat is supported by the GPU bound to this instance. */
	bool isSupported(DataFormat p_format);

	/** Returns whether the DataFormat is supported by this implementation, or can be substituted by one that is. */
	bool isSupportedOrSubstitutable(DataFormat p_format);

	/** Returns whether the specified Metal MTL::PixelFormat can be used as a depth format. */
	_FORCE_INLINE_ bool isDepthFormat(MTL::PixelFormat p_format) {
		switch (p_format) {
			case MTL::PixelFormatDepth32Float:
			case MTL::PixelFormatDepth16Unorm:
			case MTL::PixelFormatDepth32Float_Stencil8:
#if TARGET_OS_OSX
			case MTL::PixelFormatDepth24Unorm_Stencil8:
#endif
				return true;
			default:
				return false;
		}
	}

	/** Returns whether the specified Metal MTL::PixelFormat can be used as a stencil format. */
	_FORCE_INLINE_ bool isStencilFormat(MTL::PixelFormat p_format) {
		switch (p_format) {
			case MTL::PixelFormatStencil8:
#if TARGET_OS_OSX
			case MTL::PixelFormatDepth24Unorm_Stencil8:
			case MTL::PixelFormatX24_Stencil8:
#endif
			case MTL::PixelFormatDepth32Float_Stencil8:
			case MTL::PixelFormatX32_Stencil8:
				return true;
			default:
				return false;
		}
	}

	/** Returns whether the specified Metal MTL::PixelFormat is a PVRTC format. */
	bool isPVRTCFormat(MTL::PixelFormat p_format);

	/** Returns the format type corresponding to the specified Godot pixel format, */
	MTLFormatType getFormatType(DataFormat p_format);

	/** Returns the format type corresponding to the specified Metal MTL::PixelFormat, */
	MTLFormatType getFormatType(MTL::PixelFormat p_format);

	/**
	 * Returns the Metal MTL::PixelFormat corresponding to the specified Godot pixel
	 * or returns MTL::PixelFormatInvalid if no corresponding MTL::PixelFormat exists.
	 */
	MTL::PixelFormat getMTLPixelFormat(DataFormat p_format);

	/**
	 * Returns the DataFormat corresponding to the specified Metal MTL::PixelFormat,
	 * or returns DATA_FORMAT_MAX if no corresponding DataFormat exists.
	 */
	DataFormat getDataFormat(MTL::PixelFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel block of the specified Godot pixel.
	 * For uncompressed formats, the returned value corresponds to the size in bytes of a single texel.
	 */
	uint32_t getBytesPerBlock(DataFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel block of the specified Metal format.
	 * For uncompressed formats, the returned value corresponds to the size in bytes of a single texel.
	 */
	uint32_t getBytesPerBlock(MTL::PixelFormat p_format);

	/** Returns the number of planes of the specified chroma-subsampling (YCbCr) DataFormat */
	uint8_t getChromaSubsamplingPlaneCount(DataFormat p_format);

	/** Returns the number of bits per channel of the specified chroma-subsampling (YCbCr) DataFormat */
	uint8_t getChromaSubsamplingComponentBits(DataFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel of the specified Godot format.
	 * The returned value may be fractional for certain compressed formats.
	 */
	float getBytesPerTexel(DataFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel of the specified Metal format.
	 * The returned value may be fractional for certain compressed formats.
	 */
	float getBytesPerTexel(MTL::PixelFormat p_format);

	/**
	 * Returns the size, in bytes, of a row of texels of the specified Godot pixel format.
	 *
	 * For compressed formats, this takes into consideration the compression block size,
	 * and p_texels_per_row should specify the width in texels, not blocks. The result is rounded
	 * up if p_texels_per_row is not an integer multiple of the compression block width.
	 */
	size_t getBytesPerRow(DataFormat p_format, uint32_t p_texels_per_row);

	/**
	 * Returns the size, in bytes, of a row of texels of the specified Metal format.
	 *
	 * For compressed formats, this takes into consideration the compression block size,
	 * and texelsPerRow should specify the width in texels, not blocks. The result is rounded
	 * up if texelsPerRow is not an integer multiple of the compression block width.
	 */
	size_t getBytesPerRow(MTL::PixelFormat p_format, uint32_t p_texels_per_row);

	/**
	 * Returns the size, in bytes, of a texture layer of the specified Godot pixel format.
	 *
	 * For compressed formats, this takes into consideration the compression block size,
	 * and p_texel_rows_per_layer should specify the height in texels, not blocks. The result is
	 * rounded up if p_texel_rows_per_layer is not an integer multiple of the compression block height.
	 */
	size_t getBytesPerLayer(DataFormat p_format, size_t p_bytes_per_row, uint32_t p_texel_rows_per_layer);

	/**
	 * Returns the size, in bytes, of a texture layer of the specified Metal format.
	 * For compressed formats, this takes into consideration the compression block size,
	 * and p_texel_rows_per_layer should specify the height in texels, not blocks. The result is
	 * rounded up if p_texel_rows_per_layer is not an integer multiple of the compression block height.
	 */
	size_t getBytesPerLayer(MTL::PixelFormat p_format, size_t p_bytes_per_row, uint32_t p_texel_rows_per_layer);

	/** Returns whether or not the specified Godot format requires swizzling to use with Metal. */
	bool needsSwizzle(DataFormat p_format);

	/** Returns the Metal format capabilities supported by the specified Godot format, without substitution. */
	MTLFmtCaps getCapabilities(DataFormat p_format, bool p_extended = false);

	/** Returns the Metal format capabilities supported by the specified Metal format. */
	MTLFmtCaps getCapabilities(MTL::PixelFormat p_format, bool p_extended = false);

	/**
	 * Returns the Metal MTL::VertexFormat corresponding to the specified
	 * DataFormat as used as a vertex attribute format.
	 */
	MTL::VertexFormat getMTLVertexFormat(DataFormat p_format);

#pragma mark Construction

	explicit PixelFormats(MTL::Device *p_device, const MetalFeatures &p_feat);
	~PixelFormats();

protected:
	DataFormatDesc &getDataFormatDesc(DataFormat p_format);
	DataFormatDesc &getDataFormatDesc(MTL::PixelFormat p_format);
	MTLFormatDesc &getMTLPixelFormatDesc(MTL::PixelFormat p_format);
	MTLFmtCaps &getMTLPixelFormatCapsIf(MTL::PixelFormat mtlPixFmt, bool cond);
	MTLFormatDesc &getMTLVertexFormatDesc(MTL::VertexFormat p_format);

	void initDataFormatCapabilities();
	void initMTLPixelFormatCapabilities();
	void initMTLVertexFormatCapabilities(const MetalFeatures &p_feat);
	void modifyMTLFormatCapabilities(const MetalFeatures &p_feat);
	void buildDFFormatMaps();
	void addMTLPixelFormatDescImpl(MTL::PixelFormat p_pix_fmt, MTL::PixelFormat p_pix_fmt_linear,
			MTLViewClass p_view_class, MTLFmtCaps p_fmt_caps, const char *p_name);
	void addMTLVertexFormatDescImpl(MTL::VertexFormat p_vert_fmt, MTLFmtCaps p_vert_caps, const char *name);

	MTL::Device *device;
	InflectionMap<DataFormat, DataFormatDesc, RDC::DATA_FORMAT_MAX> _data_format_descs;
	InflectionMap<uint16_t, MTLFormatDesc, MTL::PixelFormatX32_Stencil8 + 2> _mtl_pixel_format_descs; // The actual last enum value is not available on iOS.
	TightLocalVector<MTLFormatDesc> _mtl_vertex_format_descs;
};

GODOT_CLANG_WARNING_POP
// --- Inlined/Folded from drivers/metal/metal_utils.h ---
#include "core/typedefs.h"

#include <os/log.h>

#include <cstdint>
#include <functional>
#include <type_traits>

/// Godot limits the number of dynamic buffers to 8.
///
/// This is a minimum guarantee for Vulkan.
constexpr uint32_t MAX_DYNAMIC_BUFFERS = 8;

// From rendering/rendering_device/vsync/frame_queue_size
static constexpr uint32_t MAX_FRAME_COUNT = 3;

#pragma mark - Boolean flags

namespace flags {

/*! Sets the flags within the value parameter specified by the mask parameter. */
template <typename Tv, typename Tm>
void set(Tv &p_value, Tm p_mask) {
	using T = std::underlying_type_t<Tv>;
	p_value = static_cast<Tv>(static_cast<T>(p_value) | static_cast<T>(p_mask));
}

/*! Clears the flags within the value parameter specified by the mask parameter. */
template <typename Tv, typename Tm>
void clear(Tv &p_value, Tm p_mask) {
	using T = std::underlying_type_t<Tv>;
	p_value = static_cast<Tv>(static_cast<T>(p_value) & ~static_cast<T>(p_mask));
}

/*! Returns whether the specified value has any of the bits specified in mask set to 1. */
template <typename Tv, typename Tm>
static constexpr bool any(Tv p_value, const Tm p_mask) {
	return ((p_value & p_mask) != 0);
}

/*! Returns whether the specified value has all of the bits specified in mask set to 1. */
template <typename Tv, typename Tm>
static constexpr bool all(Tv p_value, const Tm p_mask) {
	return ((p_value & p_mask) == p_mask);
}

} //namespace flags

#pragma mark - Alignment and Offsets

static constexpr bool is_power_of_two(uint64_t p_value) {
	return p_value && ((p_value & (p_value - 1)) == 0);
}

static constexpr uint64_t round_up_to_alignment(uint64_t p_value, uint64_t p_alignment) {
	DEV_ASSERT(is_power_of_two(p_alignment));

	if (p_alignment == 0) {
		return p_value;
	}

	uint64_t mask = p_alignment - 1;
	uint64_t aligned_value = (p_value + mask) & ~mask;

	return aligned_value;
}

template <typename F>
class Defer {
public:
	explicit Defer(F &&f) :
			func_(std::forward<F>(f)) {}
	~Defer() { func_(); }

	// Non-copyable (correct RAII semantics)
	Defer(const Defer &) = delete;
	Defer &operator=(const Defer &) = delete;

	// Movable
	Defer(Defer &&) = default;
	Defer &operator=(Defer &&) = default;

private:
	F func_;
};

// C++17 class template argument deduction.
template <typename F>
Defer(F &&) -> Defer<std::decay_t<F>>;

#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)
#define DEFER const auto &CONCAT(defer__, __LINE__) = Defer

extern os_log_t LOG_DRIVER;
// Used for dynamic tracing.
extern os_log_t LOG_INTERVALS;

_FORCE_INLINE_ static constexpr uint32_t make_msl_version(uint32_t p_major, uint32_t p_minor = 0, uint32_t p_patch = 0) {
	return (p_major * 10000) + (p_minor * 100) + p_patch;
}

_FORCE_INLINE_ static constexpr void parse_msl_version(uint32_t p_version, uint32_t &r_major, uint32_t &r_minor) {
	r_major = p_version / 10000;
	r_minor = (p_version % 10000) / 100;
}

constexpr uint32_t MSL_VERSION_23 = make_msl_version(2, 3);
constexpr uint32_t MSL_VERSION_24 = make_msl_version(2, 4);
constexpr uint32_t MSL_VERSION_30 = make_msl_version(3, 0);
constexpr uint32_t MSL_VERSION_31 = make_msl_version(3, 1);
constexpr uint32_t MSL_VERSION_32 = make_msl_version(3, 2);
constexpr uint32_t MSL_VERSION_40 = make_msl_version(4, 0);

/* MSL Language version table
 *
 * | Version |  macOS  |   iOS   |
 * |---------|---------|---------|
 * |   1.0   |         |   9.0   |
 * |   1.1   |  10.11  |   9.0   |
 * |   1.2   |  10.12  |  10.0   |
 * |   2.0   |  10.13  |  11.0   |
 * |   2.1   |  10.14  |  12.0   |
 * |   2.2   |  10.15  |  13.0   |
 * |   2.3   |  11.0   |  14.0   |
 * |   2.4   |  12.0   |  15.0   |
 * |   3.0   |  13.0   |  16.0   |
 * |   3.1   |  14.0   |  17.0   |
 * |   3.2   |  15.0   |  18.0   |
 * |   4.0   |  26.0   |  26.0   |
 * |---------|---------|---------|
 */

// --- Folded from drivers/metal/sha256_digest.h ---
#include "core/templates/hashfuncs.h"
#include "core/templates/local_vector.h"

#include <CommonCrypto/CommonDigest.h>
#include <simd/simd.h>
#include <zlib.h>

struct SHA256Digest {
	unsigned char data[CC_SHA256_DIGEST_LENGTH];

	static constexpr size_t serialized_size() { return CC_SHA256_DIGEST_LENGTH; }

	uint32_t hash() const {
		uint32_t c = crc32(0, data, CC_SHA256_DIGEST_LENGTH);
		return c;
	}

	SHA256Digest() {
		bzero(data, CC_SHA256_DIGEST_LENGTH);
	}

	SHA256Digest(const char *p_hash) {
		memcpy(data, p_hash, CC_SHA256_DIGEST_LENGTH);
	}

	SHA256Digest(const char *p_data, size_t p_length) {
		CC_SHA256(p_data, (CC_LONG)p_length, data);
	}

	_FORCE_INLINE_ uint32_t short_sha() const {
		return __builtin_bswap32(*(uint32_t *)&data[0]);
	}

	LocalVector<uint8_t> serialize() const {
		LocalVector<uint8_t> result;
		result.resize(CC_SHA256_DIGEST_LENGTH);
		memcpy(result.ptr(), data, CC_SHA256_DIGEST_LENGTH);
		return result;
	}

	static SHA256Digest deserialize(LocalVector<uint8_t> p_ser) {
		return SHA256Digest((const char *)p_ser.ptr());
	}
};

template <>
struct HashMapComparatorDefault<SHA256Digest> {
	static bool compare(const SHA256Digest &p_lhs, const SHA256Digest &p_rhs) {
		return memcmp(p_lhs.data, p_rhs.data, CC_SHA256_DIGEST_LENGTH) == 0;
	}
};

// --- End drivers/metal/sha256_digest.h ---

#include <CoreFoundation/CoreFoundation.h>

#include <memory>
#include <optional>

class RenderingDeviceDriverMetal;

using RDC = RenderingDeviceCommons;

enum ShaderStageUsage : uint32_t {
	None = 0,
	Vertex = RDD::SHADER_STAGE_VERTEX_BIT,
	Fragment = RDD::SHADER_STAGE_FRAGMENT_BIT,
	TesselationControl = RDD::SHADER_STAGE_TESSELATION_CONTROL_BIT,
	TesselationEvaluation = RDD::SHADER_STAGE_TESSELATION_EVALUATION_BIT,
	Compute = RDD::SHADER_STAGE_COMPUTE_BIT,
};

_FORCE_INLINE_ ShaderStageUsage &operator|=(ShaderStageUsage &p_a, int p_b) {
	p_a = ShaderStageUsage(uint32_t(p_a) | uint32_t(p_b));
	return p_a;
}

struct ClearAttKey {
	const static uint32_t COLOR_COUNT = MAX_COLOR_ATTACHMENT_COUNT;
	const static uint32_t DEPTH_INDEX = COLOR_COUNT;
	const static uint32_t STENCIL_INDEX = DEPTH_INDEX + 1;
	const static uint32_t ATTACHMENT_COUNT = STENCIL_INDEX + 1;

	enum Flags : uint16_t {
		CLEAR_FLAGS_NONE = 0,
		CLEAR_FLAGS_LAYERED = 1 << 0,
	};

	Flags flags = CLEAR_FLAGS_NONE;
	uint16_t sample_count = 0;
	uint16_t pixel_formats[ATTACHMENT_COUNT] = { 0 };

	_FORCE_INLINE_ void set_color_format(uint32_t p_idx, MTL::PixelFormat p_fmt) { pixel_formats[p_idx] = p_fmt; }
	_FORCE_INLINE_ void set_depth_format(MTL::PixelFormat p_fmt) { pixel_formats[DEPTH_INDEX] = p_fmt; }
	_FORCE_INLINE_ void set_stencil_format(MTL::PixelFormat p_fmt) { pixel_formats[STENCIL_INDEX] = p_fmt; }
	_FORCE_INLINE_ MTL::PixelFormat depth_format() const { return (MTL::PixelFormat)pixel_formats[DEPTH_INDEX]; }
	_FORCE_INLINE_ MTL::PixelFormat stencil_format() const { return (MTL::PixelFormat)pixel_formats[STENCIL_INDEX]; }
	_FORCE_INLINE_ void enable_layered_rendering() { flags::set(flags, CLEAR_FLAGS_LAYERED); }

	_FORCE_INLINE_ bool is_enabled(uint32_t p_idx) const { return pixel_formats[p_idx] != 0; }
	_FORCE_INLINE_ bool is_depth_enabled() const { return pixel_formats[DEPTH_INDEX] != 0; }
	_FORCE_INLINE_ bool is_stencil_enabled() const { return pixel_formats[STENCIL_INDEX] != 0; }
	_FORCE_INLINE_ bool is_layered_rendering_enabled() const { return flags::any(flags, CLEAR_FLAGS_LAYERED); }

	_FORCE_INLINE_ bool operator==(const ClearAttKey &p_rhs) const {
		return memcmp(this, &p_rhs, sizeof(ClearAttKey)) == 0;
	}

	uint32_t hash() const {
		uint32_t h = hash_murmur3_one_32(flags);
		h = hash_murmur3_one_32(sample_count, h);
		h = hash_murmur3_buffer(pixel_formats, ATTACHMENT_COUNT * sizeof(pixel_formats[0]), h);
		return hash_fmix32(h);
	}
};

#pragma mark - Ring Buffer

/// A ring buffer backed by MTLBuffer instances for transient GPU allocations.
/// Allocations are 16-byte aligned with a minimum size of 16 bytes.
/// When the current buffer is exhausted, a new buffer is allocated.
class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDRingBuffer {
public:
	static constexpr uint32_t DEFAULT_BUFFER_SIZE = 512 * 1024;
	static constexpr uint32_t MIN_BLOCK_SIZE = 16;
	static constexpr uint32_t ALIGNMENT = 16;

	struct Allocation {
		void *ptr = nullptr;
		MTL::Buffer *buffer = nullptr;
		uint64_t gpu_address = 0;
		uint32_t offset = 0;

		_FORCE_INLINE_ bool is_valid() const { return ptr != nullptr; }
	};

private:
	MTL::Device *device = nullptr;
	LocalVector<MTL::Buffer *> buffers;
	LocalVector<uint32_t> heads;
	uint32_t current_segment = 0;
	uint32_t buffer_size = DEFAULT_BUFFER_SIZE;
	bool changed = false;

	_FORCE_INLINE_ uint32_t alloc_segment() {
		MTL::Buffer *buffer = device->newBuffer(buffer_size, MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked);
		buffers.push_back(buffer);
		heads.push_back(0);
		changed = true;

		return buffers.size() - 1;
	}

public:
	MDRingBuffer() = default;

	MDRingBuffer(MTL::Device *p_device, uint32_t p_buffer_size = DEFAULT_BUFFER_SIZE) :
			device(p_device), buffer_size(p_buffer_size) {}

	~MDRingBuffer() {
		for (MTL::Buffer *buffer : buffers) {
			buffer->release();
		}
	}

	/// Allocates a block of memory from the ring buffer.
	/// Returns an Allocation with the pointer, buffer, and offset.
	_FORCE_INLINE_ Allocation allocate(uint32_t p_size) {
		p_size = MAX(p_size, MIN_BLOCK_SIZE);
		p_size = (p_size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

		if (buffers.is_empty()) {
			alloc_segment();
		}

		uint32_t aligned_head = (heads[current_segment] + ALIGNMENT - 1) & ~(ALIGNMENT - 1);

		if (aligned_head + p_size > buffer_size) {
			// Current segment exhausted, try to find one with space or allocate new.
			bool found = false;
			for (uint32_t i = 0; i < buffers.size(); i++) {
				uint32_t ah = (heads[i] + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
				if (ah + p_size <= buffer_size) {
					current_segment = i;
					aligned_head = ah;
					found = true;
					break;
				}
			}

			if (!found) {
				current_segment = alloc_segment();
				aligned_head = 0;
			}
		}

		MTL::Buffer *buffer = buffers[current_segment];
		Allocation alloc;
		alloc.buffer = buffer;
		alloc.offset = aligned_head;
		alloc.ptr = static_cast<uint8_t *>(buffer->contents()) + aligned_head;
		if (__builtin_available(macOS 13.0, iOS 16.0, tvOS 16.0, *)) {
			alloc.gpu_address = buffer->gpuAddress() + aligned_head;
		}
		heads[current_segment] = aligned_head + p_size;

		return alloc;
	}

	/// Resets all segments for reuse. Call at frame boundaries when GPU work is complete.
	_FORCE_INLINE_ void reset() {
		for (uint32_t &head : heads) {
			head = 0;
		}
		current_segment = 0;
	}

	/// Returns true if buffers were added or removed since last clear_changed().
	_FORCE_INLINE_ bool is_changed() const { return changed; }

	/// Clears the changed flag.
	_FORCE_INLINE_ void clear_changed() { changed = false; }

	/// Returns a Span of all backing buffers.
	_FORCE_INLINE_ Span<MTL::Buffer *const> get_buffers() const {
		return Span<MTL::Buffer *const>(buffers.ptr(), buffers.size());
	}

	/// Returns the number of buffer segments currently allocated.
	_FORCE_INLINE_ uint32_t get_segment_count() const {
		return buffers.size();
	}
};

#pragma mark - Resource Factory

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDResourceFactory {
private:
	MTL::Device *device;
	PixelFormats &pixel_formats;
	uint32_t max_buffer_count;

	NS::SharedPtr<MTL::Function> new_func(NS::String *p_source, NS::String *p_name, NS::Error **p_error);
	NS::SharedPtr<MTL::Function> new_clear_vert_func(ClearAttKey &p_key);
	NS::SharedPtr<MTL::Function> new_clear_frag_func(ClearAttKey &p_key);
	const char *get_format_type_string(MTL::PixelFormat p_fmt) const;

	_FORCE_INLINE_ uint32_t get_vertex_buffer_index(uint32_t p_binding) {
		return (max_buffer_count - 1) - p_binding;
	}

public:
	NS::SharedPtr<MTL::RenderPipelineState> new_clear_pipeline_state(ClearAttKey &p_key, NS::Error **p_error);
	NS::SharedPtr<MTL::RenderPipelineState> new_empty_draw_pipeline_state(ClearAttKey &p_key, NS::Error **p_error);
	NS::SharedPtr<MTL::DepthStencilState> new_depth_stencil_state(bool p_use_depth, bool p_use_stencil);

	MDResourceFactory(MTL::Device *p_device, PixelFormats &p_pixel_formats, uint32_t p_max_buffer_count) :
			device(p_device), pixel_formats(p_pixel_formats), max_buffer_count(p_max_buffer_count) {}
	~MDResourceFactory() = default;
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDResourceCache {
private:
	typedef HashMap<ClearAttKey, NS::SharedPtr<MTL::RenderPipelineState>> HashMap;
	std::unique_ptr<MDResourceFactory> resource_factory;
	HashMap clear_states;
	HashMap empty_draw_states;

	struct {
		NS::SharedPtr<MTL::DepthStencilState> all;
		NS::SharedPtr<MTL::DepthStencilState> depth_only;
		NS::SharedPtr<MTL::DepthStencilState> stencil_only;
		NS::SharedPtr<MTL::DepthStencilState> none;
	} clear_depth_stencil_state;

public:
	MTL::RenderPipelineState *get_clear_render_pipeline_state(ClearAttKey &p_key, NS::Error **p_error);
	MTL::RenderPipelineState *get_empty_draw_pipeline_state(ClearAttKey &p_key, NS::Error **p_error);
	MTL::DepthStencilState *get_depth_stencil_state(bool p_use_depth, bool p_use_stencil);

	explicit MDResourceCache(MTL::Device *p_device, PixelFormats &p_pixel_formats, uint32_t p_max_buffer_count) :
			resource_factory(new MDResourceFactory(p_device, p_pixel_formats, p_max_buffer_count)) {}
	~MDResourceCache() = default;
};

/**
 * Returns an index that can be used to map a shader stage to an index in a fixed-size array that is used for
 * a single pipeline type.
 */
_FORCE_INLINE_ static uint32_t to_index(RDD::ShaderStage p_s) {
	switch (p_s) {
		case RenderingDeviceCommons::SHADER_STAGE_VERTEX:
		case RenderingDeviceCommons::SHADER_STAGE_TESSELATION_CONTROL:
		case RenderingDeviceCommons::SHADER_STAGE_TESSELATION_EVALUATION:
		case RenderingDeviceCommons::SHADER_STAGE_COMPUTE:
		default:
			return 0;
		case RenderingDeviceCommons::SHADER_STAGE_FRAGMENT:
			return 1;
	}
}

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDFrameBuffer {
	Vector<MTL::Texture *> textures;

public:
	Size2i size;
	MDFrameBuffer(Vector<MTL::Texture *> p_textures, Size2i p_size) :
			textures(p_textures), size(p_size) {}
	MDFrameBuffer() {}

	/// Returns the texture at the given index.
	_ALWAYS_INLINE_ MTL::Texture *get_texture(uint32_t p_idx) const {
		return textures[p_idx];
	}

	/// Returns true if the texture at the given index is not nil.
	_ALWAYS_INLINE_ bool has_texture(uint32_t p_idx) const {
		return textures[p_idx] != nullptr;
	}

	/// Set the texture at the given index.
	_ALWAYS_INLINE_ void set_texture(uint32_t p_idx, MTL::Texture *p_texture) {
		textures.write[p_idx] = p_texture;
	}

	/// Unset or nil the texture at the given index.
	_ALWAYS_INLINE_ void unset_texture(uint32_t p_idx) {
		textures.write[p_idx] = nullptr;
	}

	/// Resizes buffers to the specified size.
	_ALWAYS_INLINE_ void set_texture_count(uint32_t p_size) {
		textures.resize(p_size);
	}

	virtual ~MDFrameBuffer() = default;
};

template <>
struct HashMapComparatorDefault<RDD::ShaderID> {
	static bool compare(const RDD::ShaderID &p_lhs, const RDD::ShaderID &p_rhs) {
		return p_lhs.id == p_rhs.id;
	}
};

template <>
struct HashMapComparatorDefault<RDD::BufferID> {
	static bool compare(const RDD::BufferID &p_lhs, const RDD::BufferID &p_rhs) {
		return p_lhs.id == p_rhs.id;
	}
};

template <>
struct HashMapComparatorDefault<RDD::TextureID> {
	static bool compare(const RDD::TextureID &p_lhs, const RDD::TextureID &p_rhs) {
		return p_lhs.id == p_rhs.id;
	}
};

template <>
struct HashMapHasherDefaultImpl<RDD::BufferID> {
	static _FORCE_INLINE_ uint32_t hash(const RDD::BufferID &p_value) {
		return HashMapHasherDefaultImpl<uint64_t>::hash(p_value.id);
	}
};

template <>
struct HashMapHasherDefaultImpl<RDD::TextureID> {
	static _FORCE_INLINE_ uint32_t hash(const RDD::TextureID &p_value) {
		return HashMapHasherDefaultImpl<uint64_t>::hash(p_value.id);
	}
};

namespace rid {

template <typename T>
_FORCE_INLINE_ T *get(RDD::ID p_id) {
	return reinterpret_cast<T *>(p_id.id);
}

template <typename T>
_FORCE_INLINE_ T *get(uint64_t p_id) {
	return reinterpret_cast<T *>(p_id);
}

} // namespace rid

#pragma mark - Render Pass Types

class MDRenderPass;

enum class MDAttachmentType : uint8_t {
	None = 0,
	Color = 1 << 0,
	Depth = 1 << 1,
	Stencil = 1 << 2,
};

_FORCE_INLINE_ MDAttachmentType &operator|=(MDAttachmentType &p_a, MDAttachmentType p_b) {
	flags::set(p_a, p_b);
	return p_a;
}

_FORCE_INLINE_ bool operator&(MDAttachmentType p_a, MDAttachmentType p_b) {
	return uint8_t(p_a) & uint8_t(p_b);
}

struct MDSubpass {
	uint32_t subpass_index = 0;
	uint32_t view_count = 0;
	LocalVector<RDD::AttachmentReference> input_references;
	LocalVector<RDD::AttachmentReference> color_references;
	RDD::AttachmentReference depth_stencil_reference;
	LocalVector<RDD::AttachmentReference> resolve_references;
	RDD::AttachmentReference depth_resolve_reference;

	MTLFmtCaps getRequiredFmtCapsForAttachmentAt(uint32_t p_index) const;
};

struct API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDAttachment {
private:
	uint32_t index = 0;
	uint32_t firstUseSubpassIndex = 0;
	uint32_t lastUseSubpassIndex = 0;

public:
	MTL::PixelFormat format = MTL::PixelFormatInvalid;
	MDAttachmentType type = MDAttachmentType::None;
	MTL::LoadAction loadAction = MTL::LoadActionDontCare;
	MTL::StoreAction storeAction = MTL::StoreActionDontCare;
	MTL::LoadAction stencilLoadAction = MTL::LoadActionDontCare;
	MTL::StoreAction stencilStoreAction = MTL::StoreActionDontCare;
	uint32_t samples = 1;

	/*!
	 * @brief Returns true if this attachment is first used in the given subpass.
	 * @param p_subpass
	 * @return
	 */
	_FORCE_INLINE_ bool isFirstUseOf(const MDSubpass &p_subpass) const {
		return p_subpass.subpass_index == firstUseSubpassIndex;
	}

	/*!
	 * @brief Returns true if this attachment is last used in the given subpass.
	 * @param p_subpass
	 * @return
	 */
	_FORCE_INLINE_ bool isLastUseOf(const MDSubpass &p_subpass) const {
		return p_subpass.subpass_index == lastUseSubpassIndex;
	}

	void linkToSubpass(const MDRenderPass &p_pass);

	MTL::StoreAction getMTLStoreAction(const MDSubpass &p_subpass,
			bool p_is_rendering_entire_area,
			bool p_has_resolve,
			bool p_can_resolve,
			bool p_is_stencil) const;
	bool configureDescriptor(MTL::RenderPassAttachmentDescriptor *p_desc,
			PixelFormats &p_pf,
			const MDSubpass &p_subpass,
			MTL::Texture *p_attachment,
			bool p_is_rendering_entire_area,
			bool p_has_resolve,
			bool p_can_resolve,
			bool p_is_stencil) const {
		p_desc->setTexture(p_attachment);

		MTL::LoadAction load;
		if (!p_is_rendering_entire_area || !isFirstUseOf(p_subpass)) {
			load = MTL::LoadActionLoad;
		} else {
			load = p_is_stencil ? (MTL::LoadAction)stencilLoadAction : (MTL::LoadAction)loadAction;
		}

		p_desc->setLoadAction(load);

		MTL::PixelFormat mtlFmt = p_attachment->pixelFormat();
		bool isDepthFormat = p_pf.isDepthFormat(mtlFmt);
		bool isStencilFormat = p_pf.isStencilFormat(mtlFmt);
		if (isStencilFormat && !p_is_stencil && !isDepthFormat) {
			p_desc->setStoreAction(MTL::StoreActionDontCare);
		} else {
			p_desc->setStoreAction(getMTLStoreAction(p_subpass, p_is_rendering_entire_area, p_has_resolve, p_can_resolve, p_is_stencil));
		}

		return load == MTL::LoadActionClear;
	}

	/** Returns whether this attachment should be cleared in the subpass. */
	bool shouldClear(const MDSubpass &p_subpass, bool p_is_stencil) const;
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDRenderPass {
public:
	Vector<MDAttachment> attachments;
	Vector<MDSubpass> subpasses;

	uint32_t get_sample_count() const {
		return attachments.is_empty() ? 1 : attachments[0].samples;
	}

	MDRenderPass(Vector<MDAttachment> &p_attachments, Vector<MDSubpass> &p_subpasses);
};

#pragma mark - Command Buffer Helpers

_FORCE_INLINE_ static MTL::Size MTLSizeFromVector3i(Vector3i p_size) {
	return MTL::Size{ (NS::UInteger)p_size.x, (NS::UInteger)p_size.y, (NS::UInteger)p_size.z };
}

_FORCE_INLINE_ static MTL::Origin MTLOriginFromVector3i(Vector3i p_origin) {
	return MTL::Origin{ (NS::UInteger)p_origin.x, (NS::UInteger)p_origin.y, (NS::UInteger)p_origin.z };
}

// Clamps the size so that the sum of the origin and size do not exceed the maximum size.
_FORCE_INLINE_ static MTL::Size clampMTLSize(MTL::Size p_size, MTL::Origin p_origin, MTL::Size p_max_size) {
	MTL::Size clamped;
	clamped.width = MIN(p_size.width, p_max_size.width - p_origin.x);
	clamped.height = MIN(p_size.height, p_max_size.height - p_origin.y);
	clamped.depth = MIN(p_size.depth, p_max_size.depth - p_origin.z);
	return clamped;
}

API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0))
_FORCE_INLINE_ static bool isArrayTexture(MTL::TextureType p_type) {
	return (p_type == MTL::TextureType3D ||
			p_type == MTL::TextureType2DArray ||
			p_type == MTL::TextureType2DMultisampleArray ||
			p_type == MTL::TextureType1DArray);
}

_FORCE_INLINE_ static bool operator==(MTL::Size p_a, MTL::Size p_b) {
	return p_a.width == p_b.width && p_a.height == p_b.height && p_a.depth == p_b.depth;
}

#pragma mark - Pipeline Stage Conversion

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")

_FORCE_INLINE_ static MTL::Stages convert_src_pipeline_stages_to_metal(uint32_t p_stages) {
	p_stages.clear_flag(RDD::PIPELINE_STAGE_TOP_OF_PIPE_BIT);

	// BOTTOM_OF_PIPE or ALL_COMMANDS means "all prior work must complete".
	if (p_stages & (RDD::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | RDD::PIPELINE_STAGE_ALL_COMMANDS_BIT)) {
		return MTL::StageAll;
	}

	MTL::Stages mtlStages = 0;

	// Vertex stage mappings.
	if (p_stages & (RDD::PIPELINE_STAGE_DRAW_INDIRECT_BIT | RDD::PIPELINE_STAGE_VERTEX_INPUT_BIT | RDD::PIPELINE_STAGE_VERTEX_SHADER_BIT | RDD::PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | RDD::PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | RDD::PIPELINE_STAGE_GEOMETRY_SHADER_BIT)) {
		mtlStages |= MTL::StageVertex;
	}

	// Fragment stage mappings.
	// Includes resolve and clear_storage, which on Metal use the render pipeline.
	if (p_stages & (RDD::PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RDD::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | RDD::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | RDD::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | RDD::PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT | RDD::PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT | RDD::PIPELINE_STAGE_RESOLVE_BIT | RDD::PIPELINE_STAGE_CLEAR_STORAGE_BIT)) {
		mtlStages |= MTL::StageFragment;
	}

	// Compute stage.
	if (p_stages & RDD::PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
		mtlStages |= MTL::StageDispatch;
	}

	// Blit stage (transfer operations).
	if (p_stages & RDD::PIPELINE_STAGE_COPY_BIT) {
		mtlStages |= MTL::StageBlit;
	}

	// ALL_GRAPHICS_BIT special case.
	if (p_stages & RDD::PIPELINE_STAGE_ALL_GRAPHICS_BIT) {
		mtlStages |= (MTL::StageVertex | MTL::StageFragment);
	}

	return mtlStages;
}

_FORCE_INLINE_ static MTL::Stages convert_dst_pipeline_stages_to_metal(uint32_t p_stages) {
	p_stages.clear_flag(RDD::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

	// TOP_OF_PIPE or ALL_COMMANDS means "wait before any work starts".
	if (p_stages & (RDD::PIPELINE_STAGE_ALL_COMMANDS_BIT | RDD::PIPELINE_STAGE_TOP_OF_PIPE_BIT)) {
		return MTL::StageAll;
	}

	MTL::Stages mtlStages = 0;

	// Vertex stage mappings.
	if (p_stages & (RDD::PIPELINE_STAGE_DRAW_INDIRECT_BIT | RDD::PIPELINE_STAGE_VERTEX_INPUT_BIT | RDD::PIPELINE_STAGE_VERTEX_SHADER_BIT | RDD::PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | RDD::PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | RDD::PIPELINE_STAGE_GEOMETRY_SHADER_BIT)) {
		mtlStages |= MTL::StageVertex;
	}

	// Fragment stage mappings.
	// Includes resolve and clear_storage, which on Metal use the render pipeline.
	if (p_stages & (RDD::PIPELINE_STAGE_FRAGMENT_SHADER_BIT | RDD::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | RDD::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | RDD::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | RDD::PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT | RDD::PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT | RDD::PIPELINE_STAGE_RESOLVE_BIT | RDD::PIPELINE_STAGE_CLEAR_STORAGE_BIT)) {
		mtlStages |= MTL::StageFragment;
	}

	// Compute stage.
	if (p_stages & RDD::PIPELINE_STAGE_COMPUTE_SHADER_BIT) {
		mtlStages |= MTL::StageDispatch;
	}

	// Blit stage (transfer operations).
	if (p_stages & RDD::PIPELINE_STAGE_COPY_BIT) {
		mtlStages |= MTL::StageBlit;
	}

	// ALL_GRAPHICS_BIT special case.
	if (p_stages & RDD::PIPELINE_STAGE_ALL_GRAPHICS_BIT) {
		mtlStages |= (MTL::StageVertex | MTL::StageFragment);
	}

	return mtlStages;
}

GODOT_CLANG_WARNING_POP

#pragma mark - Command Buffer Base

enum class MDCommandBufferStateType {
	None,
	Render,
	Compute,
	Blit, // Only used by Metal 3
};

/// Base struct for render state shared between MTL3 and MTL4 implementations.
struct RenderStateBase {
	LocalVector<MTL::Viewport> viewports;
	LocalVector<MTL::ScissorRect> scissors;
	std::optional<Color> blend_constants;

	// clang-format off
	enum DirtyFlag : uint16_t {
		DIRTY_NONE     = 0,
		DIRTY_PIPELINE = 1 << 0,
		DIRTY_UNIFORMS = 1 << 1,
		DIRTY_PUSH     = 1 << 2,
		DIRTY_DEPTH    = 1 << 3,
		DIRTY_VERTEX   = 1 << 4,
		DIRTY_VIEWPORT = 1 << 5,
		DIRTY_SCISSOR  = 1 << 6,
		DIRTY_BLEND    = 1 << 7,
		DIRTY_RASTER   = 1 << 8,
		DIRTY_ALL      = (1 << 9) - 1,
	};
	// clang-format on
	uint32_t dirty = DIRTY_NONE;
};

/// Abstract base class for Metal command buffers, shared between MTL3 and MTL4 implementations.
class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDCommandBufferBase {
	LocalVector<CFTypeRef> _retained_resources;

protected:
	// From RenderingDevice
	static constexpr uint32_t MAX_PUSH_CONSTANT_SIZE = 128;

	MDCommandBufferStateType type = MDCommandBufferStateType::None;

	uint8_t push_constant_data[MAX_PUSH_CONSTANT_SIZE];
	uint32_t push_constant_data_len = 0;
	uint32_t push_constant_binding = UINT32_MAX;

	::RenderingDeviceDriverMetal *device_driver = nullptr;

	void release_resources();

	/// Called when push constants are modified to mark the appropriate dirty flags.
	virtual void mark_push_constants_dirty() = 0;

	/// Returns a reference to the render state base for viewport/scissor/blend operations.
	virtual RenderStateBase &get_render_state_base() = 0;

	/// Returns the view count for the current subpass.
	virtual uint32_t get_current_view_count() const = 0;

	/// Accessors for render pass state.
	virtual MDRenderPass *get_render_pass() const = 0;
	virtual MDFrameBuffer *get_frame_buffer() const = 0;
	virtual const MDSubpass &get_current_subpass() const = 0;
	virtual LocalVector<RDD::RenderPassClearValue> &get_clear_values() = 0;
	virtual const Rect2i &get_render_area() const = 0;
	virtual void end_render_encoding() = 0;

	void _populate_vertices(simd::float4 *p_vertices, Size2i p_fb_size, VectorView<Rect2i> p_rects);
	uint32_t _populate_vertices(simd::float4 *p_vertices, uint32_t p_index, const Rect2i &p_rect, Size2i p_fb_size);
	void _end_render_pass();
	void _render_clear_render_area();

public:
	virtual ~MDCommandBufferBase() { release_resources(); }

	virtual void begin() = 0;
	virtual void commit() = 0;
	virtual void end() = 0;

	virtual void bind_pipeline(RDD::PipelineID p_pipeline) = 0;
	void encode_push_constant_data(RDD::ShaderID p_shader, VectorView<uint32_t> p_data);

	void retain_resource(CFTypeRef p_resource);

#pragma mark - Render Commands

	virtual void render_bind_uniform_sets(VectorView<RDD::UniformSetID> p_uniform_sets, RDD::ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) = 0;
	virtual void render_clear_attachments(VectorView<RDD::AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) = 0;
	void render_set_viewport(VectorView<Rect2i> p_viewports);
	void render_set_scissor(VectorView<Rect2i> p_scissors);
	void render_set_blend_constants(const Color &p_constants);
	virtual void render_begin_pass(RDD::RenderPassID p_render_pass,
			RDD::FramebufferID p_frameBuffer,
			RDD::CommandBufferType p_cmd_buffer_type,
			const Rect2i &p_rect,
			VectorView<RDD::RenderPassClearValue> p_clear_values) = 0;
	virtual void render_next_subpass() = 0;
	virtual void render_draw(uint32_t p_vertex_count,
			uint32_t p_instance_count,
			uint32_t p_base_vertex,
			uint32_t p_first_instance) = 0;
	virtual void render_bind_vertex_buffers(uint32_t p_binding_count, const RDD::BufferID *p_buffers, const uint64_t *p_offsets, uint64_t p_dynamic_offsets) = 0;
	virtual void render_bind_index_buffer(RDD::BufferID p_buffer, RDD::IndexBufferFormat p_format, uint64_t p_offset) = 0;

	virtual void render_draw_indexed(uint32_t p_index_count,
			uint32_t p_instance_count,
			uint32_t p_first_index,
			int32_t p_vertex_offset,
			uint32_t p_first_instance) = 0;

	virtual void render_draw_indexed_indirect(RDD::BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) = 0;
	virtual void render_draw_indexed_indirect_count(RDD::BufferID p_indirect_buffer, uint64_t p_offset, RDD::BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) = 0;
	virtual void render_draw_indirect(RDD::BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) = 0;
	virtual void render_draw_indirect_count(RDD::BufferID p_indirect_buffer, uint64_t p_offset, RDD::BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) = 0;

	virtual void render_end_pass() = 0;

#pragma mark - Compute Commands

	virtual void compute_bind_uniform_sets(VectorView<RDD::UniformSetID> p_uniform_sets, RDD::ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) = 0;
	virtual void compute_dispatch(uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) = 0;
	virtual void compute_dispatch_indirect(RDD::BufferID p_indirect_buffer, uint64_t p_offset) = 0;

#pragma mark - Transfer

	virtual void resolve_texture(RDD::TextureID p_src_texture, RDD::TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, RDD::TextureID p_dst_texture, RDD::TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) = 0;
	virtual void clear_color_texture(RDD::TextureID p_texture, RDD::TextureLayout p_texture_layout, const Color &p_color, const RDD::TextureSubresourceRange &p_subresources) = 0;
	virtual void clear_depth_stencil_texture(RDD::TextureID p_texture, RDD::TextureLayout p_texture_layout, float p_depth, uint8_t p_stencil, const RDD::TextureSubresourceRange &p_subresources) = 0;
	virtual void clear_buffer(RDD::BufferID p_buffer, uint64_t p_offset, uint64_t p_size) = 0;
	virtual void copy_buffer(RDD::BufferID p_src_buffer, RDD::BufferID p_dst_buffer, VectorView<RDD::BufferCopyRegion> p_regions) = 0;
	virtual void copy_texture(RDD::TextureID p_src_texture, RDD::TextureID p_dst_texture, VectorView<RDD::TextureCopyRegion> p_regions) = 0;
	virtual void copy_buffer_to_texture(RDD::BufferID p_src_buffer, RDD::TextureID p_dst_texture, VectorView<RDD::BufferTextureCopyRegion> p_regions) = 0;
	virtual void copy_texture_to_buffer(RDD::TextureID p_src_texture, RDD::BufferID p_dst_buffer, VectorView<RDD::BufferTextureCopyRegion> p_regions) = 0;

#pragma mark - Synchronization

	virtual void pipeline_barrier(uint32_t p_src_stages,
			uint32_t p_dst_stages,
			VectorView<RDD::MemoryAccessBarrier> p_memory_barriers,
			VectorView<RDD::BufferBarrier> p_buffer_barriers,
			VectorView<RDD::TextureBarrier> p_texture_barriers,
			VectorView<RDD::AccelerationStructureBarrier> p_acceleration_structure_barriers) = 0;

#pragma mark - Debugging

	virtual void begin_label(const char *p_label_name, const Color &p_color) = 0;
	virtual void end_label() = 0;
};

#pragma mark - Uniform Types

struct API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) UniformInfo {
	uint32_t binding;
	uint32_t active_stages;
	MTL::DataType dataType = MTL::DataTypeNone;
	MTL::BindingAccess access = MTL::BindingAccessReadOnly;
	MTL::ResourceUsage usage = 0;
	MTL::TextureType textureType = MTL::TextureType2D;
	uint32_t imageFormat = 0;
	uint32_t arrayLength = 0;
	bool isMultisampled = false;

	struct Indexes {
		uint32_t buffer = UINT32_MAX;
		uint32_t texture = UINT32_MAX;
		uint32_t sampler = UINT32_MAX;
	};
	Indexes slot;
	Indexes arg_buffer;

	enum class IndexType {
		SLOT,
		ARG,
	};

	_FORCE_INLINE_ Indexes &get_indexes(IndexType p_type) {
		switch (p_type) {
			case IndexType::SLOT:
				return slot;
			case IndexType::ARG:
				return arg_buffer;
		}
	}
};

struct API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) UniformSet {
	LocalVector<UniformInfo> uniforms;
	LocalVector<uint32_t> dynamic_uniforms;
	uint32_t buffer_size = 0;
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) DynamicOffsetLayout {
	struct Data {
		uint8_t offset : 4;
		uint8_t count : 4;
	};

	union {
		Data data[MAX_DYNAMIC_BUFFERS];
		uint64_t _val = 0;
	};

public:
	_FORCE_INLINE_ bool is_empty() const { return _val == 0; }

	_FORCE_INLINE_ uint32_t get_count(uint32_t p_set_index) const {
		return data[p_set_index].count;
	}

	_FORCE_INLINE_ uint32_t get_offset(uint32_t p_set_index) const {
		return data[p_set_index].offset;
	}

	_FORCE_INLINE_ void set_offset_count(uint32_t p_set_index, uint8_t p_offset, uint8_t p_count) {
		data[p_set_index].offset = p_offset;
		data[p_set_index].count = p_count;
	}

	_FORCE_INLINE_ uint32_t get_offset_index_shift(uint32_t p_set_index, uint32_t p_dynamic_index = 0) const {
		return (data[p_set_index].offset + p_dynamic_index) * 4u;
	}
};

#pragma mark - Shader Types

class MDLibrary; // Forward declaration for C++ code
struct ShaderCacheEntry; // Forward declaration for C++ code

enum class ShaderLoadStrategy {
	IMMEDIATE,
	LAZY,

	/// The default strategy is to load the shader immediately.
	DEFAULT = IMMEDIATE,
};

/// A Metal shader library.
class MDLibrary : public std::enable_shared_from_this<MDLibrary> {
protected:
	ShaderCacheEntry *_entry = nullptr;
#ifdef DEV_ENABLED
	NS::SharedPtr<NS::String> _original_source = nullptr;
#endif

	MDLibrary(ShaderCacheEntry *p_entry
#ifdef DEV_ENABLED
			,
			NS::String *p_source
#endif
	);

public:
	virtual ~MDLibrary();

	virtual MTL::Library *get_library() = 0;
	virtual NS::Error *get_error() = 0;
	virtual void set_label(NS::String *p_label);
#ifdef DEV_ENABLED
	NS::String *get_original_source() const { return _original_source.get(); }
#endif

	static std::shared_ptr<MDLibrary> create(ShaderCacheEntry *p_entry,
			MTL::Device *p_device,
			NS::String *p_source,
			MTL::CompileOptions *p_options,
			ShaderLoadStrategy p_strategy);

	static std::shared_ptr<MDLibrary> create(ShaderCacheEntry *p_entry,
			MTL::Device *p_device,
#ifdef DEV_ENABLED
			NS::String *p_source,
#endif
			dispatch_data_t p_data);
};

/// A cache entry for a Metal shader library.
struct ShaderCacheEntry {
	RenderingDeviceDriverMetal &owner;
	/// A hash of the Metal shader source code.
	SHA256Digest key;
	CharString name;
	RDC::ShaderStage stage = RDC::SHADER_STAGE_VERTEX;
	/// Weak reference to the library; allows cache lookup without preventing cleanup.
	std::weak_ptr<MDLibrary> library;

	/// Notify the cache that this entry is no longer needed.
	void notify_free();

	ShaderCacheEntry(RenderingDeviceDriverMetal &p_owner, SHA256Digest p_key) :
			owner(p_owner), key(p_key) {
	}
	~ShaderCacheEntry() = default;
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDShader {
public:
	CharString name;
	Vector<UniformSet> sets;
	struct {
		uint32_t stages = {};
		uint32_t binding = UINT32_MAX;
		uint32_t size = 0;
	} push_constants;
	DynamicOffsetLayout dynamic_offset_layout;
	bool uses_argument_buffers = true;

	MDShader(CharString p_name, Vector<UniformSet> p_sets, bool p_uses_argument_buffers) :
			name(p_name), sets(p_sets), uses_argument_buffers(p_uses_argument_buffers) {}
	virtual ~MDShader() = default;
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDComputeShader final : public MDShader {
public:
	MTL::Size local = {};

	std::shared_ptr<MDLibrary> kernel;

	MDComputeShader(CharString p_name, Vector<UniformSet> p_sets, bool p_uses_argument_buffers, std::shared_ptr<MDLibrary> p_kernel);
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDRenderShader final : public MDShader {
public:
	bool needs_view_mask_buffer = false;

	std::shared_ptr<MDLibrary> vert;
	std::shared_ptr<MDLibrary> frag;

	MDRenderShader(CharString p_name,
			Vector<UniformSet> p_sets,
			bool p_needs_view_mask_buffer,
			bool p_uses_argument_buffers,
			std::shared_ptr<MDLibrary> p_vert, std::shared_ptr<MDLibrary> p_frag);
};

#pragma mark - Uniform Set

enum StageResourceUsage : uint32_t {
	ResourceUnused = 0,
	VertexRead = (MTL::ResourceUsageRead << RDD::SHADER_STAGE_VERTEX * 2),
	VertexWrite = (MTL::ResourceUsageWrite << RDD::SHADER_STAGE_VERTEX * 2),
	FragmentRead = (MTL::ResourceUsageRead << RDD::SHADER_STAGE_FRAGMENT * 2),
	FragmentWrite = (MTL::ResourceUsageWrite << RDD::SHADER_STAGE_FRAGMENT * 2),
	TesselationControlRead = (MTL::ResourceUsageRead << RDD::SHADER_STAGE_TESSELATION_CONTROL * 2),
	TesselationControlWrite = (MTL::ResourceUsageWrite << RDD::SHADER_STAGE_TESSELATION_CONTROL * 2),
	TesselationEvaluationRead = (MTL::ResourceUsageRead << RDD::SHADER_STAGE_TESSELATION_EVALUATION * 2),
	TesselationEvaluationWrite = (MTL::ResourceUsageWrite << RDD::SHADER_STAGE_TESSELATION_EVALUATION * 2),
	ComputeRead = (MTL::ResourceUsageRead << RDD::SHADER_STAGE_COMPUTE * 2),
	ComputeWrite = (MTL::ResourceUsageWrite << RDD::SHADER_STAGE_COMPUTE * 2),
};

typedef LocalVector<MTL::Resource *> ResourceVector;
typedef HashMap<StageResourceUsage, ResourceVector> ResourceUsageMap;

_FORCE_INLINE_ StageResourceUsage &operator|=(StageResourceUsage &p_a, uint32_t p_b) {
	p_a = StageResourceUsage(uint32_t(p_a) | p_b);
	return p_a;
}

_FORCE_INLINE_ StageResourceUsage stage_resource_usage(RDC::ShaderStage p_stage, MTL::ResourceUsage p_usage) {
	return StageResourceUsage(p_usage << (p_stage * 2));
}

_FORCE_INLINE_ MTL::ResourceUsage resource_usage_for_stage(StageResourceUsage p_usage, RDC::ShaderStage p_stage) {
	return MTL::ResourceUsage((p_usage >> (p_stage * 2)) & 0b11);
}

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDUniformSet {
public:
	NS::SharedPtr<MTL::Buffer> arg_buffer;
	Vector<uint8_t> arg_buffer_data; // Stored for dynamic uniform sets.
	ResourceUsageMap usage_to_resources; // Used by Metal 3 for resource tracking.
	Vector<RDD::BoundUniform> uniforms;
};

#pragma mark - Pipeline Types

enum class MDPipelineType {
	None,
	Render,
	Compute,
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDPipeline {
public:
	MDPipelineType type;

	explicit MDPipeline(MDPipelineType p_type) :
			type(p_type) {}
	virtual ~MDPipeline() = default;
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDRenderPipeline final : public MDPipeline {
public:
	NS::SharedPtr<MTL::RenderPipelineState> state;
	NS::SharedPtr<MTL::DepthStencilState> depth_stencil;
	uint32_t push_constant_size = 0;
	uint32_t push_constant_stages_mask = 0;
	SampleCount sample_count = SampleCount1;

	struct {
		MTL::CullMode cull_mode = MTL::CullModeNone;
		MTL::TriangleFillMode fill_mode = MTL::TriangleFillModeFill;
		MTL::DepthClipMode clip_mode = MTL::DepthClipModeClip;
		MTL::Winding winding = MTL::WindingClockwise;
		MTL::PrimitiveType render_primitive = MTL::PrimitiveTypePoint;

		struct {
			bool enabled = false;
		} depth_test;

		struct {
			bool enabled = false;
			float depth_bias = 0.0;
			float slope_scale = 0.0;
			float clamp = 0.0;

			template <typename T>
			_FORCE_INLINE_ void apply(T *p_enc) const {
				if (!enabled) {
					return;
				}
				p_enc->setDepthBias(depth_bias, slope_scale, clamp);
			}
		} depth_bias;

		struct {
			bool enabled = false;
			uint32_t front_reference = 0;
			uint32_t back_reference = 0;

			template <typename T>
			_FORCE_INLINE_ void apply(T *p_enc) const {
				if (!enabled) {
					return;
				}
				p_enc->setStencilReferenceValues(front_reference, back_reference);
			}
		} stencil;

		struct {
			bool enabled = false;
			float r = 0.0;
			float g = 0.0;
			float b = 0.0;
			float a = 0.0;

			template <typename T>
			_FORCE_INLINE_ void apply(T *p_enc) const {
				p_enc->setBlendColor(r, g, b, a);
			}
		} blend;

		template <typename T>
		_FORCE_INLINE_ void apply(T *p_enc) const {
			p_enc->setCullMode(cull_mode);
			p_enc->setTriangleFillMode(fill_mode);
			p_enc->setDepthClipMode(clip_mode);
			p_enc->setFrontFacingWinding(winding);
			depth_bias.apply(p_enc);
			stencil.apply(p_enc);
			blend.apply(p_enc);
		}

	} raster_state;

	MDRenderShader *shader = nullptr;

	MDRenderPipeline() :
			MDPipeline(MDPipelineType::Render) {}
	~MDRenderPipeline() final = default;
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(2.0)) MDComputePipeline final : public MDPipeline {
public:
	NS::SharedPtr<MTL::ComputePipelineState> state;
	struct {
		MTL::Size local = {};
	} compute_state;

	MDComputeShader *shader = nullptr;

	explicit MDComputePipeline(NS::SharedPtr<MTL::ComputePipelineState> p_state) :
			MDPipeline(MDPipelineType::Compute), state(std::move(p_state)) {}
	~MDComputePipeline() final = default;
};

// --- Folded from drivers/metal/metal_device_profile.h ---
#include "core/os/mutex.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/typedefs.h"

class MinOsVersion {
	uint32_t version;

public:
	String to_compiler_os_version() const;
	bool is_null() const { return version == UINT32_MAX; }
	bool is_valid() const { return version != UINT32_MAX; }

	MinOsVersion(const String &p_version);
	constexpr explicit MinOsVersion(uint32_t p_version) :
			version(p_version) {}
	constexpr MinOsVersion(uint32_t p_major, uint32_t p_minor, uint32_t p_patch = 0) :
			version(p_major * 10000 + p_minor * 100 + p_patch) {}
	constexpr MinOsVersion() :
			version(UINT32_MAX) {}

	bool operator>(uint32_t p_other) {
		return version > p_other;
	}
	constexpr operator uint32_t() const { return version; }
};

namespace os_version {

constexpr MinOsVersion MACOS_26_0(26'00'00);
constexpr MinOsVersion MACOS_15_0(15'00'00);
constexpr MinOsVersion MACOS_14_0(14'00'00);
constexpr MinOsVersion MACOS_13_0(13'00'00);
constexpr MinOsVersion MACOS_12_0(12'00'00);
constexpr MinOsVersion MACOS_11_0(11'00'00);

constexpr MinOsVersion IOS_26_0(26'00'00);
constexpr MinOsVersion IOS_18_0(18'00'00);
constexpr MinOsVersion IOS_17_0(17'00'00);
constexpr MinOsVersion IOS_16_0(16'00'00);
constexpr MinOsVersion IOS_15_0(15'00'00);

constexpr MinOsVersion VISIONOS_26_0(26'00'00);
constexpr MinOsVersion VISIONOS_02_4(02'04'00);

} //namespace os_version

/// @brief A minimal structure that defines a device profile for Metal.
///
/// This structure is used by the `RenderingShaderContainerMetal` class to
/// determine options for compiling SPIR-V to Metal source. It currently only
/// contains the minimum properties required to transform shaders from SPIR-V to Metal
/// and potentially compile to a `.metallib`.
struct MetalDeviceProfile {
	enum class Platform : uint32_t {
		macOS = 0,
		iOS = 1,
		visionOS = 2,
	};

	/*! @brief The GPU family.
	 *
	 * NOTE: These values match Apple's MTLGPUFamily
	 */
	enum class GPU : uint32_t {
		Apple1 = 1001,
		Apple2 = 1002,
		Apple3 = 1003,
		Apple4 = 1004,
		Apple5 = 1005,
		Apple6 = 1006,
		Apple7 = 1007,
		Apple8 = 1008,
		Apple9 = 1009,
	};

	enum class ArgumentBuffersTier : uint32_t {
		Tier1 = 0,
		Tier2 = 1,
	};

	struct Features {
		uint32_t msl_version = 0;
		bool use_argument_buffers = false;
		bool simdPermute = false;
	};

	Platform platform = Platform::macOS;
	GPU gpu = GPU::Apple4;
	MinOsVersion min_os_version;
	Features features;

	struct MinimumRequirements {
		GPU gpu = GPU::Apple1;
		uint32_t msl_version = 0;

		bool operator>(const MinimumRequirements &p_other) const {
			return gpu > p_other.gpu || msl_version > p_other.msl_version;
		}
	};

	MinimumRequirements get_minimum_requirements() const {
		return { gpu, features.msl_version };
	}

	static const MetalDeviceProfile *get_profile(Platform p_platform, GPU p_gpu, MinOsVersion p_min_os_version);

	MetalDeviceProfile() = default;

private:
	static Mutex profiles_lock; ///< Mutex to protect access to the profiles map.

	struct ProfileKey {
		friend struct HashMapHasherDefaultImpl<ProfileKey>;
		union {
			struct {
				uint32_t min_os_version;
				uint16_t platform;
				uint16_t gpu;
			};
			uint64_t value = 0;
		};

		ProfileKey() = default;
		ProfileKey(MinOsVersion p_min_os_version, Platform p_platform, GPU p_gpu) :
				min_os_version(p_min_os_version), platform((uint16_t)p_platform), gpu((uint16_t)p_gpu) {}

		_FORCE_INLINE_ uint32_t hash() const {
			return hash_one_uint64(value);
		}

		bool operator==(const ProfileKey &p_other) const {
			return value == p_other.value;
		}
	};

	static HashMap<ProfileKey, MetalDeviceProfile> profiles;
};

// --- End drivers/metal/metal_device_profile.h ---

// --- Folded from drivers/metal/rendering_shader_container_metal.h ---
#include "servers/rendering/rendering_device.h"

constexpr uint32_t R32UI_ALIGNMENT_CONSTANT_ID = 65535;
/// Metal buffer index for the view mask when rendering multi-view.
const uint32_t VIEW_MASK_BUFFER_INDEX = 24;

class RenderingShaderContainerFormatMetal;

class RenderingShaderContainerMetal : public RenderingShaderContainer {

public:
	struct HeaderData {
		enum Flags : uint32_t {
			NONE = 0,
			NEEDS_VIEW_MASK_BUFFER = 1 << 0,
			USES_ARGUMENT_BUFFERS = 1 << 1,
			NEEDS_DEBUG_LOGGING = 1 << 2,

			/// A special value indicating that the shader failed to compile, so the compiled library data is invalid and should be ignored.
			INVALID_SHADER = 0xFFFFFFFF,
		};

		/// The base profile that was used to generate this shader.
		MetalDeviceProfile profile;

		/// The Metal language version specified when compiling SPIR-V to MSL.
		/// Format is major * 10000 + minor * 100 + patch.
		uint32_t msl_version = UINT32_MAX;
		/*! @brief The minimum supported OS version for shaders baked to a `.metallib`.
		 *
		 * NOTE: This property is only valid when shaders are baked to a .metalllib
		 *
		 * Format is major * 10000 + minor * 100 + patch.
		 */
		MinOsVersion os_min_version;
		uint32_t flags = NONE;
		uint32_t push_constant_binding = UINT32_MAX; ///< Metal binding slot for the push constant data

		/// @brief Returns `true` if the shader is compiled with multi-view support.
		bool needs_view_mask_buffer() const {
			return flags & NEEDS_VIEW_MASK_BUFFER;
		}

		void set_needs_view_mask_buffer(bool p_value) {
			if (p_value) {
				flags |= NEEDS_VIEW_MASK_BUFFER;
			} else {
				flags &= ~NEEDS_VIEW_MASK_BUFFER;
			}
		}

		/// @brief Returns `true` if the shader was compiled with argument buffer support.
		bool uses_argument_buffers() const {
			return flags & USES_ARGUMENT_BUFFERS;
		}

		void set_uses_argument_buffers(bool p_value) {
			if (p_value) {
				flags |= USES_ARGUMENT_BUFFERS;
			} else {
				flags &= ~USES_ARGUMENT_BUFFERS;
			}
		}

		/// Returns `true` if the shader was compiled with the GL_EXT_debug_printf extension enabled.
		bool needs_debug_logging() const {
			return flags & NEEDS_DEBUG_LOGGING;
		}

		void set_needs_debug_logging(bool p_value) {
			if (p_value) {
				flags |= NEEDS_DEBUG_LOGGING;
			} else {
				flags &= ~NEEDS_DEBUG_LOGGING;
			}
		}

		bool is_invalid() const {
			return flags == INVALID_SHADER;
		}

		void mark_invalid() {
			flags = INVALID_SHADER;
		}
	};

	/// @brief Returns `true` if the shader failed to compile and the compiled library data is invalid.
	bool is_invalid() const {
		return mtl_reflection_data.is_invalid();
	}

	struct StageData {
		uint32_t vertex_input_binding_mask = 0;
		uint32_t is_position_invariant = 0; ///< <c>true</c> if the position output is invariant
		uint32_t supports_fast_math = 0;
		SHA256Digest hash; ///< SHA 256 hash of the shader code
		uint32_t source_size = 0; ///< size of the source code in the returned bytes
		uint32_t library_size = 0; ///< size of the compiled library in the returned bytes, 0 if it is not compiled
	};

	struct UniformData {
		uint32_t active_stages = 0;
		uint32_t uniform_type = 0; // UniformType
		uint32_t data_type = 0; // MTLDataTypeNone
		uint32_t access = 0; // MTLBindingAccessReadOnly
		uint32_t usage = 0; // MTLResourceUsage (none)
		uint32_t texture_type = 2; // MTLTextureType2D
		uint32_t image_format = 0;
		uint32_t array_length = 0;
		uint32_t is_multisampled = 0;

		struct Indexes {
			uint32_t buffer = UINT32_MAX;
			uint32_t texture = UINT32_MAX;
			uint32_t sampler = UINT32_MAX;
		};
		Indexes slot;
		Indexes arg_buffer;

		enum class IndexType {
			SLOT,
			ARG,
		};

		_FORCE_INLINE_ Indexes &get_indexes(IndexType p_type) {
			switch (p_type) {
				case IndexType::SLOT:
					return slot;
				case IndexType::ARG:
					return arg_buffer;
				default:
					CRASH_NOW_MSG("Unreachable");
			}
		}
	};

	HeaderData mtl_reflection_data; // compliment to reflection_data
	Vector<StageData> mtl_shaders; // compliment to shaders

private:
	struct ToolchainProperties {
		MinOsVersion os_version_min_required;
		uint32_t metal_version = UINT32_MAX;

		_FORCE_INLINE_ bool is_null() const { return os_version_min_required.is_null() || metal_version == UINT32_MAX; }
		_FORCE_INLINE_ bool is_valid() const { return !is_null(); }
	};

	ToolchainProperties compiler_props;

	void _initialize_toolchain_properties();

private:
	const MetalDeviceProfile *device_profile = nullptr;
	bool export_mode = false;

	Vector<UniformData> mtl_reflection_binding_set_uniforms_data; // compliment to reflection_binding_set_uniforms_data

	Error compile_metal_source(const char *p_source, const StageData &p_stage_data, Vector<uint8_t> &r_binary_data);

	MetalDeviceProfile::MinimumRequirements inspect_spirv(const ReflectShader &p_shader);

public:
	static constexpr uint32_t FORMAT_VERSION = 2;

	void set_export_mode(bool p_export_mode) { export_mode = p_export_mode; }
	void set_device_profile(const MetalDeviceProfile *p_device_profile) { device_profile = p_device_profile; }

	struct MetalShaderReflection {
		Vector<Vector<UniformData>> uniform_sets;
	};

	MetalShaderReflection get_metal_shader_reflection() const;

protected:
	virtual uint32_t _from_bytes_reflection_extra_data(const uint8_t *p_bytes) override;
	virtual uint32_t _from_bytes_reflection_binding_uniform_extra_data_start(const uint8_t *p_bytes) override;
	virtual uint32_t _from_bytes_reflection_binding_uniform_extra_data(const uint8_t *p_bytes, uint32_t p_index) override;
	virtual uint32_t _from_bytes_shader_extra_data_start(const uint8_t *p_bytes) override;
	virtual uint32_t _from_bytes_shader_extra_data(const uint8_t *p_bytes, uint32_t p_index) override;

	virtual uint32_t _to_bytes_reflection_extra_data(uint8_t *p_bytes) const override;
	virtual uint32_t _to_bytes_reflection_binding_uniform_extra_data(uint8_t *p_bytes, uint32_t p_index) const override;
	virtual uint32_t _to_bytes_shader_extra_data(uint8_t *p_bytes, uint32_t p_index) const override;

	virtual uint32_t _format() const override;
	virtual uint32_t _format_version() const override;
	virtual bool _set_code_from_spirv(const ReflectShader &p_shader) override;
};

class RenderingShaderContainerFormatMetal : public RenderingShaderContainerFormat {
	bool export_mode = false;

	const MetalDeviceProfile *device_profile = nullptr;

public:
	virtual Ref<RenderingShaderContainer> create_container() const override;
	virtual ShaderLanguageVersion get_shader_language_version() const override;
	virtual ShaderSpirvVersion get_shader_spirv_version() const override;
	RenderingShaderContainerFormatMetal(const MetalDeviceProfile *p_device_profile, bool p_export = false);
	virtual ~RenderingShaderContainerFormatMetal() = default;
};
#include "servers/rendering/rendering_device.h"

#include <Metal/Metal.hpp>

#include <optional>
#include <variant>

class RenderingShaderContainerFormatMetal;

#ifdef DEBUG_ENABLED
#ifndef _DEBUG
#define _DEBUG
#endif
#endif

class RenderingContextDriverMetal;

namespace MTL3 {
class MDCommandBuffer;
}
namespace MTL4 {
class MDCommandBuffer;
}

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) RenderingDeviceDriverMetal : public RenderingDeviceDriver {
	friend struct ShaderCacheEntry;
	friend class MTL3::MDCommandBuffer;
	friend class MTL4::MDCommandBuffer;
	friend class MDUniformSet;

	template <typename T>
	using Result = std::variant<T, Error>;

#pragma mark - Generic

protected:
	RenderingContextDriverMetal *context_driver = nullptr;
	RenderingContextDriver::Device context_device;
	MTL::Device *device = nullptr;

	uint32_t _frame_count = 1;
	/// frame_index is a cyclic counter derived from the current frame number modulo frame_count,
	/// cycling through values from 0 to frame_count - 1
	uint32_t _frame_index = 0;
	uint32_t _frames_drawn = 0;

	MetalDeviceProperties *device_properties = nullptr;
	MetalDeviceProfile device_profile;
	RenderingShaderContainerFormatMetal *shader_container_format = nullptr;
	PixelFormats *pixel_formats = nullptr;
	std::unique_ptr<MDResourceCache> resource_cache;

	RDD::Capabilities capabilities;
	RDD::MultiviewCapabilities multiview_capabilities;
	RDD::FragmentShadingRateCapabilities fsr_capabilities;
	RDD::FragmentDensityMapCapabilities fdm_capabilities;

	NS::SharedPtr<MTL::BinaryArchive> archive;
	uint32_t archive_count = 0;
	// DEV: When true, attempting to create a pipeline will fail if it cannot use the archive.
	bool archive_fail_on_miss = false;

	/// Resources to be added to the `main_residency_set`.
	LocalVector<MTL::Resource *> _residency_add;
	/// Resources to be removed from the `main_residency_set`.
	LocalVector<MTL::Resource *> _residency_del;

#pragma mark - Copy Queue

	Mutex copy_queue_mutex;
	/// A command queue used for internal copy operations.
	NS::SharedPtr<MTL::CommandQueue> copy_queue;
	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
	NS::SharedPtr<MTL::ResidencySet> copy_queue_rs;
	GODOT_CLANG_WARNING_POP
	// If this is not nullptr, there are pending copy operations.
	NS::SharedPtr<MTL::CommandBuffer> copy_queue_command_buffer;
	NS::SharedPtr<MTL::BlitCommandEncoder> copy_queue_blit_encoder;
	NS::SharedPtr<MTL::Buffer> copy_queue_buffer;
	NS::UInteger copy_queue_buffer_offset = 0;

	_FORCE_INLINE_ NS::UInteger _copy_queue_buffer_available() const {
		return copy_queue_buffer.get()->length() - copy_queue_buffer_offset;
	}

	/// Marks p_size bytes as consumed from the copy queue buffer, aligning the new offset to 16 bytes.
	_FORCE_INLINE_ void _copy_queue_buffer_consume(NS::UInteger p_size) {
		// Round up the end of the consumed region so the next copy starts aligned and the offset
		// never exceeds the buffer length (which would underflow _copy_queue_buffer_available()).
		copy_queue_buffer_offset = round_up_to_alignment(copy_queue_buffer_offset + p_size, 16);
	}

	/// Returns a pointer to the current position in the copy queue buffer.
	_FORCE_INLINE_ void *_copy_queue_buffer_ptr() const {
		return static_cast<uint8_t *>(copy_queue_buffer.get()->contents()) + copy_queue_buffer_offset;
	}

	_FORCE_INLINE_ MTL::CommandBuffer *_copy_queue_command_buffer() {
		if (!copy_queue_command_buffer) {
			DEV_ASSERT(!copy_queue_blit_encoder);
			copy_queue_command_buffer = NS::RetainPtr(copy_queue.get()->commandBufferWithUnretainedReferences());
		}
		return copy_queue_command_buffer.get();
	}

	_FORCE_INLINE_ MTL::BlitCommandEncoder *_copy_queue_blit_encoder() {
		if (!copy_queue_blit_encoder) {
			MTL::BlitCommandEncoder *enc = _copy_queue_command_buffer()->blitCommandEncoder();
			copy_queue_blit_encoder = NS::RetainPtr(enc);
		}
		return copy_queue_blit_encoder.get();
	}

	void _copy_queue_copy_to_buffer(Span<uint8_t> p_src_data, MTL::Buffer *p_dst_buffer, uint64_t p_dst_offset = 0);
	void _copy_queue_flush();
	Error _copy_queue_initialize();

	NS::SharedPtr<MTL::CaptureScope> device_scope;

	String pipeline_cache_id;

	virtual MTL::CommandQueue *get_command_queue() const = 0;
	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
	virtual void add_residency_set_to_main_queue(MTL::ResidencySet *p_set) = 0;
	virtual void remove_residency_set_to_main_queue(MTL::ResidencySet *p_set) = 0;
	NS::SharedPtr<MTL::ResidencySet> main_residency_set;
	GODOT_CLANG_WARNING_POP

	bool use_barriers = false;
	MTL::ResourceOptions base_hazard_tracking = MTL::ResourceHazardTrackingModeTracked;

	virtual Error _create_device();
	virtual void _track_resource(MTL::Resource *p_resource);
	virtual void _untrack_resource(MTL::Resource *p_resource);
	void _check_capabilities();
	Error _initialize(uint32_t p_device_index, uint32_t p_frame_count);

#pragma mark - Shader Cache

	ShaderLoadStrategy _shader_load_strategy = ShaderLoadStrategy::DEFAULT;

	/**
	 * The shader cache is a map of hashes of the Metal source to shader cache entries.
	 *
	 * To prevent unbounded growth of the cache, cache entries are automatically freed when
	 * there are no more references to the MDLibrary associated with the cache entry.
	 */
	HashMap<SHA256Digest, ShaderCacheEntry *> _shader_cache;
	Mutex _shader_cache_lock;
	void shader_cache_free_entry(ShaderCacheEntry *p_entry);
	std::optional<std::shared_ptr<MDLibrary>> shader_cache_get_library(const SHA256Digest &key);
	void shader_cache_set_entry(const SHA256Digest &key, ShaderCacheEntry *p_entry);

public:
	virtual Error initialize(uint32_t p_device_index, uint32_t p_frame_count) override = 0;

#pragma mark - Memory

#pragma mark - Buffers

public:
	struct BufferInfo {
		NS::SharedPtr<MTL::Buffer> metal_buffer;

		_FORCE_INLINE_ bool is_dynamic() const { return _frame_idx != UINT32_MAX; }
		_FORCE_INLINE_ uint32_t frame_index() const { return _frame_idx; }
		_FORCE_INLINE_ void set_frame_index(uint32_t p_frame_index) { _frame_idx = p_frame_index; }

	protected:
		// If dynamic buffer, then its range is [0; RenderingDeviceDriverMetal::frame_count)
		// else it's UINT32_MAX.
		uint32_t _frame_idx = UINT32_MAX;
	};

	virtual BufferID buffer_create(uint64_t p_size, uint32_t p_usage, MemoryAllocationType p_allocation_type, uint64_t p_frames_drawn) override final;
	virtual bool buffer_set_texel_format(BufferID p_buffer, DataFormat p_format) override final;
	virtual void buffer_free(BufferID p_buffer) override final;
	virtual uint64_t buffer_get_allocation_size(BufferID p_buffer) override final;
	virtual uint8_t *buffer_map(BufferID p_buffer) override final;
	virtual void buffer_unmap(BufferID p_buffer) override final;
	virtual uint8_t *buffer_persistent_map_advance(BufferID p_buffer, uint64_t p_frames_drawn) override final;
	virtual uint64_t buffer_get_dynamic_offsets(Span<BufferID> p_buffers) override final;
	virtual uint64_t buffer_get_device_address(BufferID p_buffer) override final;

#pragma mark - Texture

private:
	// Returns true if the texture is a valid linear format.
	bool is_valid_linear(const TextureFormat &p_format) const;

public:
	virtual TextureID texture_create(const TextureFormat &p_format, const TextureView &p_view) override final;
	virtual TextureID texture_create_from_extension(uint64_t p_native_texture, TextureType p_type, DataFormat p_format, uint32_t p_array_layers, bool p_depth_stencil, uint32_t p_mipmaps) override final;
	virtual TextureID texture_create_shared(TextureID p_original_texture, const TextureView &p_view) override final;
	virtual TextureID texture_create_shared_from_slice(TextureID p_original_texture, const TextureView &p_view, TextureSliceType p_slice_type, uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps) override final;
	virtual void texture_free(TextureID p_texture) override final;
	virtual uint64_t texture_get_allocation_size(TextureID p_texture) override final;
	virtual void texture_get_copyable_layout(TextureID p_texture, const TextureSubresource &p_subresource, TextureCopyableLayout *r_layout) override final;
	virtual Vector<uint8_t> texture_get_data(TextureID p_texture, uint32_t p_layer) override final;
	virtual uint32_t texture_get_usages_supported_by_format(DataFormat p_format, bool p_cpu_readable) override final;
	virtual bool texture_can_make_shared_with_format(TextureID p_texture, DataFormat p_format, bool &r_raw_reinterpretation) override final;

#pragma mark - Sampler

public:
	virtual SamplerID sampler_create(const SamplerState &p_state) final override;
	virtual void sampler_free(SamplerID p_sampler) final override;
	virtual bool sampler_is_format_supported_for_filter(DataFormat p_format, SamplerFilter p_filter) override final;

#pragma mark - Vertex Array

private:
public:
	virtual VertexFormatID vertex_format_create(Span<VertexAttribute> p_vertex_attribs, const VertexAttributeBindingsMap &p_vertex_bindings) override final;
	virtual void vertex_format_free(VertexFormatID p_vertex_format) override final;

#pragma mark - Barriers

public:
	virtual void command_pipeline_barrier(
			CommandBufferID p_cmd_buffer,
			uint32_t p_src_stages,
			uint32_t p_dst_stages,
			VectorView<MemoryAccessBarrier> p_memory_barriers,
			VectorView<BufferBarrier> p_buffer_barriers,
			VectorView<TextureBarrier> p_texture_barriers,
			VectorView<AccelerationStructureBarrier> p_acceleration_structure_barriers) override final;

#pragma mark - Fences

public:
	virtual FenceID fence_create() override = 0;
	virtual Error fence_wait(FenceID p_fence) override = 0;
	virtual void fence_free(FenceID p_fence) override = 0;

#pragma mark - Semaphores

public:
	virtual SemaphoreID semaphore_create() override = 0;
	virtual void semaphore_free(SemaphoreID p_semaphore) override = 0;

#pragma mark - Commands
	// ----- QUEUE FAMILY -----

	virtual CommandQueueFamilyID command_queue_family_get(uint32_t p_cmd_queue_family_bits, RenderingContextDriver::SurfaceID p_surface = 0) override final;

	// ----- QUEUE -----

public:
	virtual CommandQueueID command_queue_create(CommandQueueFamilyID p_cmd_queue_family, bool p_identify_as_main_queue = false) override = 0;
	virtual Error command_queue_execute_and_present(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains) override = 0;
	virtual void command_queue_free(CommandQueueID p_cmd_queue) override = 0;

	// ----- POOL -----

	virtual CommandPoolID command_pool_create(CommandQueueFamilyID p_cmd_queue_family, CommandBufferType p_cmd_buffer_type) override = 0;
	virtual bool command_pool_reset(CommandPoolID p_cmd_pool) override = 0;
	virtual void command_pool_free(CommandPoolID p_cmd_pool) override = 0;

	// ----- BUFFER -----

public:
	virtual CommandBufferID command_buffer_create(CommandPoolID p_cmd_pool) override = 0;
	virtual bool command_buffer_begin(CommandBufferID p_cmd_buffer) override final;
	virtual bool command_buffer_begin_secondary(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, uint32_t p_subpass, FramebufferID p_framebuffer) override final;
	virtual void command_buffer_end(CommandBufferID p_cmd_buffer) override final;
	virtual void command_buffer_execute_secondary(CommandBufferID p_cmd_buffer, VectorView<CommandBufferID> p_secondary_cmd_buffers) override final;

#pragma mark - Swapchain

protected:
	struct SwapChain {
		RenderingContextDriver::SurfaceID surface = RenderingContextDriver::SurfaceID();
		RenderPassID render_pass;
		DataFormat data_format = DATA_FORMAT_MAX;
		ColorSpace color_space = COLOR_SPACE_MAX;
	};

	RenderPassID _swap_chain_create_render_pass(DataFormat p_format);
	void _swap_chain_release(SwapChain *p_swap_chain);
	void _swap_chain_release_buffers(SwapChain *p_swap_chain);

public:
	virtual SwapChainID swap_chain_create(RenderingContextDriver::SurfaceID p_surface) override final;
	virtual Error swap_chain_resize(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, uint32_t p_desired_framebuffer_count) override final;
	virtual FramebufferID swap_chain_acquire_framebuffer(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, bool &r_resize_required) override final;
	virtual RenderPassID swap_chain_get_render_pass(SwapChainID p_swap_chain) override final;
	virtual DataFormat swap_chain_get_format(SwapChainID p_swap_chain) override final;
	virtual ColorSpace swap_chain_get_color_space(SwapChainID p_swap_chain) override final;
	virtual bool swap_chain_get_hdr_output_supported(SwapChainID p_swap_chain) override final;
	virtual void swap_chain_set_max_fps(SwapChainID p_swap_chain, int p_max_fps) override final;
	virtual void swap_chain_free(SwapChainID p_swap_chain) override final;

#pragma mark - Frame Buffer

	virtual FramebufferID framebuffer_create(RenderPassID p_render_pass, VectorView<TextureID> p_attachments, uint32_t p_width, uint32_t p_height) override final;
	virtual void framebuffer_free(FramebufferID p_framebuffer) override final;

#pragma mark - Shader

private:
	// Serialization types need access to private state.

	friend struct ShaderStageData;
	friend struct SpecializationConstantData;
	friend struct UniformData;
	friend struct ShaderBinaryData;
	friend struct PushConstantData;

public:
	virtual ShaderID shader_create_from_container(const Ref<RenderingShaderContainer> &p_shader_container, const Vector<ImmutableSampler> &p_immutable_samplers) override final;
	virtual void shader_free(ShaderID p_shader) override final;
	virtual void shader_destroy_modules(ShaderID p_shader) override final;
	virtual const RenderingShaderContainerFormat &get_shader_container_format() const override final;

#pragma mark - Uniform Set

public:
	virtual UniformSetID uniform_set_create(VectorView<BoundUniform> p_uniforms, ShaderID p_shader, uint32_t p_set_index, int p_linear_pool_index) override final;
	virtual void uniform_set_free(UniformSetID p_uniform_set) override final;
	virtual uint32_t uniform_sets_get_dynamic_offsets(VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) const override final;

#pragma mark - Commands

	virtual void command_uniform_set_prepare_for_use(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) override final;

#pragma mark Transfer

public:
	virtual void command_clear_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, uint64_t p_offset, uint64_t p_size) override final;
	virtual void command_copy_buffer(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, BufferID p_dst_buffer, VectorView<BufferCopyRegion> p_regions) override final;

	virtual void command_copy_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<TextureCopyRegion> p_regions) override final;
	virtual void command_resolve_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) override final;
	virtual void command_clear_color_texture(CommandBufferID p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, const Color &p_color, const TextureSubresourceRange &p_subresources) override final;
	virtual void command_clear_depth_stencil_texture(CommandBufferID p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, float p_depth, uint8_t p_stencil, const TextureSubresourceRange &p_subresources) override final;

	virtual void command_copy_buffer_to_texture(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<BufferTextureCopyRegion> p_regions) override final;
	virtual void command_copy_texture_to_buffer(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, BufferID p_dst_buffer, VectorView<BufferTextureCopyRegion> p_regions) override final;

#pragma mark Pipeline

private:
	Result<NS::SharedPtr<MTL::Function>> _create_function(MDLibrary *p_library, NS::String *p_name, VectorView<PipelineSpecializationConstant> &p_specialization_constants);

public:
	virtual void pipeline_free(PipelineID p_pipeline_id) override final;

	// ----- BINDING -----

	virtual void command_bind_push_constants(CommandBufferID p_cmd_buffer, ShaderID p_shader, uint32_t p_first_index, VectorView<uint32_t> p_data) override final;

	// ----- CACHE -----
private:
	String _pipeline_get_cache_path() const;

public:
	virtual bool pipeline_cache_create(const Vector<uint8_t> &p_data) override final;
	virtual void pipeline_cache_free() override final;
	virtual size_t pipeline_cache_query_size() override final;
	virtual Vector<uint8_t> pipeline_cache_serialize() override final;

#pragma mark Rendering

	// ----- SUBPASS -----

	virtual RenderPassID render_pass_create(VectorView<Attachment> p_attachments, VectorView<Subpass> p_subpasses, VectorView<SubpassDependency> p_subpass_dependencies, uint32_t p_view_count, AttachmentReference p_fragment_density_map_attachment) override final;
	virtual void render_pass_free(RenderPassID p_render_pass) override final;

	// ----- COMMANDS -----

public:
	virtual void command_begin_render_pass(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, FramebufferID p_framebuffer, CommandBufferType p_cmd_buffer_type, const Rect2i &p_rect, VectorView<RenderPassClearValue> p_clear_values) override final;
	virtual void command_end_render_pass(CommandBufferID p_cmd_buffer) override final;
	virtual void command_next_render_subpass(CommandBufferID p_cmd_buffer, CommandBufferType p_cmd_buffer_type) override final;
	virtual void command_render_set_viewport(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_viewports) override final;
	virtual void command_render_set_scissor(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_scissors) override final;
	virtual void command_render_clear_attachments(CommandBufferID p_cmd_buffer, VectorView<AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) override final;

	// Binding.
	virtual void command_bind_render_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) override final;
	virtual void command_bind_render_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) override final;

	// Drawing.
	virtual void command_render_draw(CommandBufferID p_cmd_buffer, uint32_t p_vertex_count, uint32_t p_instance_count, uint32_t p_base_vertex, uint32_t p_first_instance) override final;
	virtual void command_render_draw_indexed(CommandBufferID p_cmd_buffer, uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index, int32_t p_vertex_offset, uint32_t p_first_instance) override final;
	virtual void command_render_draw_indexed_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) override final;
	virtual void command_render_draw_indexed_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) override final;
	virtual void command_render_draw_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) override final;
	virtual void command_render_draw_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) override final;

	// Buffer binding.
	virtual void command_render_bind_vertex_buffers(CommandBufferID p_cmd_buffer, uint32_t p_binding_count, const BufferID *p_buffers, const uint64_t *p_offsets, uint64_t p_dynamic_offsets) override final;
	virtual void command_render_bind_index_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, IndexBufferFormat p_format, uint64_t p_offset) override final;

	// Dynamic state.
	virtual void command_render_set_blend_constants(CommandBufferID p_cmd_buffer, const Color &p_constants) override final;
	virtual void command_render_set_line_width(CommandBufferID p_cmd_buffer, float p_width) override final;

	// ----- PIPELINE -----

	virtual PipelineID render_pipeline_create(
			ShaderID p_shader,
			VertexFormatID p_vertex_format,
			RenderPrimitive p_render_primitive,
			PipelineRasterizationState p_rasterization_state,
			PipelineMultisampleState p_multisample_state,
			PipelineDepthStencilState p_depth_stencil_state,
			PipelineColorBlendState p_blend_state,
			VectorView<int32_t> p_color_attachments,
			uint32_t p_dynamic_state,
			RenderPassID p_render_pass,
			uint32_t p_render_subpass,
			VectorView<PipelineSpecializationConstant> p_specialization_constants) override final;

#pragma mark - Compute

	// ----- COMMANDS -----

	// Binding.
	virtual void command_bind_compute_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) override final;
	virtual void command_bind_compute_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) override final;

	// Dispatching.
	virtual void command_compute_dispatch(CommandBufferID p_cmd_buffer, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) override final;
	virtual void command_compute_dispatch_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset) override final;

	// ----- PIPELINE -----

	virtual PipelineID compute_pipeline_create(ShaderID p_shader, VectorView<PipelineSpecializationConstant> p_specialization_constants) override final;

#pragma mark - Raytracing

	// ----- ACCELERATION STRUCTURE -----

	virtual AccelerationStructureID blas_create(VectorView<AccelerationStructureGeometry> p_geometries, uint32_t p_flags) override final;
	virtual AccelerationStructureID tlas_create(uint32_t p_max_instance_count, uint32_t p_flags) override final;
	virtual void acceleration_structure_instance_write(uint8_t *r_driver_instance, const AccelerationStructureInstance &p_instance) override final;
	virtual void acceleration_structure_free(AccelerationStructureID p_acceleration_structure) override final;
	virtual uint32_t acceleration_structure_get_scratch_size_bytes(AccelerationStructureID p_acceleration_structure) override final;

	// ----- PIPELINE -----

	virtual RaytracingPipelineID raytracing_pipeline_create(VectorView<PipelineShader> p_shaders, VectorView<uint32_t> p_raygen_shader_indices, VectorView<uint32_t> p_miss_shader_indices, VectorView<HitGroup> p_hit_groups, uint32_t p_max_trace_recursion_depth, ShaderID p_layout_defining_shader) override final;
	virtual void raytracing_pipeline_free(RaytracingPipelineID p_pipeline) override final;

	virtual bool raytracing_pipeline_get_shader_group_handles(RaytracingPipelineID p_pipeline, uint32_t p_group_index_offset, VectorView<uint32_t> p_group_indices, uint8_t *r_data, uint32_t p_data_stride_bytes) override final;

	// ----- COMMANDS -----

	virtual void command_build_blas(CommandBufferID p_cmd_buffer, AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer) override final;
	virtual void command_build_tlas(CommandBufferID p_cmd_buffer, AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer, BufferID p_instance_buffer, uint32_t p_instance_offset, uint32_t p_instance_count) override final;
	virtual void command_bind_raytracing_pipeline(CommandBufferID p_cmd_buffer, RaytracingPipelineID p_pipeline) override final;
	virtual void command_bind_raytracing_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) override final;
	virtual void command_trace_rays(CommandBufferID p_cmd_buffer, const ShaderBindingTable &p_raygen_sbt, const ShaderBindingTable &p_miss_sbt, const ShaderBindingTable &p_hit_sbt, uint32_t p_width, uint32_t p_height, uint32_t p_depth) override final;

#pragma mark - Queries

	// ----- TIMESTAMP -----

	// Basic.
	virtual QueryPoolID timestamp_query_pool_create(uint32_t p_query_count) override final;
	virtual void timestamp_query_pool_free(QueryPoolID p_pool_id) override final;
	virtual void timestamp_query_pool_get_results(QueryPoolID p_pool_id, uint32_t p_query_count, uint64_t *r_results) override final;
	virtual uint64_t timestamp_query_result_to_time(uint64_t p_result) override final;

	// Commands.
	virtual void command_timestamp_query_pool_reset(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_query_count) override final;
	virtual void command_timestamp_write(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_index) override final;

#pragma mark - Labels

	virtual void command_begin_label(CommandBufferID p_cmd_buffer, const char *p_label_name, const Color &p_color) override final;
	virtual void command_end_label(CommandBufferID p_cmd_buffer) override final;

#pragma mark - Debug

	virtual void command_insert_breadcrumb(CommandBufferID p_cmd_buffer, uint32_t p_data) override final;

#pragma mark - Submission

	virtual void begin_segment(uint32_t p_frame_index, uint32_t p_frames_drawn) override final;
	virtual void end_segment() override final;

#pragma mark - Miscellaneous

	virtual void set_object_name(ObjectType p_type, ID p_driver_id, const String &p_name) override final;
	virtual uint64_t get_resource_native_handle(DriverResource p_type, ID p_driver_id) override final;
	virtual uint64_t get_total_memory_used() override final;
	virtual uint64_t get_lazily_memory_used() override final;
	virtual uint64_t limit_get(Limit p_limit) override final;
	virtual uint64_t api_trait_get(ApiTrait p_trait) override final;
	virtual bool has_feature(Features p_feature) override final;
	virtual const MultiviewCapabilities &get_multiview_capabilities() override final;
	virtual const FragmentShadingRateCapabilities &get_fragment_shading_rate_capabilities() override final;
	virtual const FragmentDensityMapCapabilities &get_fragment_density_map_capabilities() override final;
	virtual String get_api_version() const override final;
	virtual String get_pipeline_cache_uuid() const override final;
	virtual const Capabilities &get_capabilities() const override final;
	virtual bool is_composite_alpha_supported(CommandQueueID p_queue) const override final;

	// Metal-specific.
	MTL::Device *get_device() const { return device; }
	PixelFormats &get_pixel_formats() const { return *pixel_formats; }
	MDResourceCache &get_resource_cache() const { return *resource_cache; }
	const MetalDeviceProperties &get_device_properties() const { return *device_properties; }

	_FORCE_INLINE_ uint32_t get_metal_buffer_index_for_vertex_attribute_binding(uint32_t p_binding) {
		return (device_properties->limits.maxPerStageBufferCount - 1) - p_binding;
	}

	size_t get_texel_buffer_alignment_for_format(RDD::DataFormat p_format) const;
	size_t get_texel_buffer_alignment_for_format(MTL::PixelFormat p_format) const;

	_FORCE_INLINE_ uint32_t frame_count() const { return _frame_count; }
	_FORCE_INLINE_ uint32_t frame_index() const { return _frame_index; }
	_FORCE_INLINE_ uint32_t frames_drawn() const { return _frames_drawn; }

	/******************/
	RenderingDeviceDriverMetal(RenderingContextDriverMetal *p_context_driver);
	~RenderingDeviceDriverMetal();
};

// Defined outside because we need to forward declare it in metal3_objects.h
struct API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) MetalBufferDynamicInfo : public RenderingDeviceDriverMetal::BufferInfo {
	uint64_t size_bytes; // Contains the real buffer size / frame_count.
	uint32_t next_frame_index(uint32_t p_frame_count) {
		// This is the next frame index to use for this buffer.
		_frame_idx = (_frame_idx + 1u) % p_frame_count;
		return _frame_idx;
	}
#ifdef DEBUG_ENABLED
	// For tracking that a persistent buffer isn't mapped twice in the same frame.
	uint64_t last_frame_mapped = 0;
#endif
};

#include <Metal/Metal.hpp>

namespace MTL3 {

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) RenderingDeviceDriverMetal final : public ::RenderingDeviceDriverMetal {
	friend class MDCommandBuffer;
#pragma mark - Generic

	NS::SharedPtr<MTL::CommandQueue> device_queue;

	struct Fence {
		virtual void signal(MTL::CommandBuffer *p_cmd_buffer) = 0;
		virtual Error wait(uint32_t p_timeout_ms) = 0;
		virtual ~Fence() = default;
	};

	struct FenceEvent : Fence {
		NS::SharedPtr<MTL::SharedEvent> event;
		uint64_t value = 0;
		FenceEvent(NS::SharedPtr<MTL::SharedEvent> p_event) :
				event(p_event) {}
		void signal(MTL::CommandBuffer *p_cb) override;
		Error wait(uint32_t p_timeout_ms) override;
	};

	struct FenceSemaphore : Fence {
		dispatch_semaphore_t semaphore;
		FenceSemaphore() :
				semaphore(dispatch_semaphore_create(0)) {}
		void signal(MTL::CommandBuffer *p_cb) override;
		Error wait(uint32_t p_timeout_ms) override;
	};

	struct Semaphore {
		NS::SharedPtr<MTL::Event> event;
		uint64_t value = 0;
		Semaphore(NS::SharedPtr<MTL::Event> p_event) :
				event(p_event) {}
	};

	Vector<MDCommandBuffer *> command_buffers;

	Error _create_device() override;
	Error _execute_and_present_barriers(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains);
	Error _execute_and_present(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains);

protected:
	MTL::CommandQueue *get_command_queue() const override { return device_queue.get(); }
	void add_residency_set_to_main_queue(MTL::ResidencySet *p_set) override;
	void remove_residency_set_to_main_queue(MTL::ResidencySet *p_set) override;

public:
	Error initialize(uint32_t p_device_index, uint32_t p_frame_count) override;

	FenceID fence_create() override;
	Error fence_wait(FenceID p_fence) override;
	void fence_free(FenceID p_fence) override;

	SemaphoreID semaphore_create() override;
	void semaphore_free(SemaphoreID p_semaphore) override;

	CommandQueueID command_queue_create(CommandQueueFamilyID p_cmd_queue_family, bool p_identify_as_main_queue = false) override;
	Error command_queue_execute_and_present(CommandQueueID p_cmd_queue, VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers, VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence, VectorView<SwapChainID> p_swap_chains) override;
	void command_queue_free(CommandQueueID p_cmd_queue) override;

	CommandPoolID command_pool_create(CommandQueueFamilyID p_cmd_queue_family, CommandBufferType p_cmd_buffer_type) override;
	bool command_pool_reset(CommandPoolID p_cmd_pool) override;
	void command_pool_free(CommandPoolID p_cmd_pool) override;

	CommandBufferID command_buffer_create(CommandPoolID p_cmd_pool) override;

#pragma mark - Miscellaneous

	String get_api_name() const override { return "Metal"; }

	RenderingDeviceDriverMetal(RenderingContextDriverMetal *p_context_driver);
	~RenderingDeviceDriverMetal();
};

} // namespace MTL3

// --- Folded from drivers/metal/rendering_context_driver_metal.h ---
#ifdef METAL_ENABLED

#include "servers/rendering/rendering_device.h"

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace MTL3 {
class MDCommandBuffer;
}

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) RenderingContextDriverMetal : public RenderingContextDriver {
	bool capture_available = false;

protected:
	MTL::Device *metal_device = nullptr;
	Device device; // There is only one device on Apple Silicon.

public:
	Error initialize() final override;
	const Device &device_get(uint32_t p_device_index) const final override;
	uint32_t device_get_count() const final override;
	bool device_supports_present(uint32_t p_device_index, SurfaceID p_surface) const final override { return true; }
	RenderingDeviceDriver *driver_create() final override;
	void driver_free(RenderingDeviceDriver *p_driver) final override;
	SurfaceID surface_create(const void *p_platform_data) final override;
	void surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) final override;
	void surface_set_vsync_mode(SurfaceID p_surface, DisplayServerEnums::VSyncMode p_vsync_mode) final override;
	DisplayServerEnums::VSyncMode surface_get_vsync_mode(SurfaceID p_surface) const final override;
	virtual void surface_set_hdr_output_enabled(SurfaceID p_surface, bool p_enabled) final override;
	virtual bool surface_get_hdr_output_enabled(SurfaceID p_surface) const final override;
	virtual void surface_set_hdr_output_reference_luminance(SurfaceID p_surface, float p_reference_luminance) final override;
	virtual float surface_get_hdr_output_reference_luminance(SurfaceID p_surface) const final override;
	virtual void surface_set_hdr_output_max_luminance(SurfaceID p_surface, float p_max_luminance) final override;
	virtual float surface_get_hdr_output_max_luminance(SurfaceID p_surface) const final override;
	virtual void surface_set_hdr_output_linear_luminance_scale(SurfaceID p_surface, float p_linear_luminance_scale) final override;
	virtual float surface_get_hdr_output_linear_luminance_scale(SurfaceID p_surface) const final override;
	virtual float surface_get_hdr_output_max_value(SurfaceID p_surface) const final override;
	uint32_t surface_get_width(SurfaceID p_surface) const final override;
	uint32_t surface_get_height(SurfaceID p_surface) const final override;
	void surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) final override;
	bool surface_get_needs_resize(SurfaceID p_surface) const final override;
	void surface_destroy(SurfaceID p_surface) final override;
	bool is_debug_utils_enabled() const final override { return capture_available; }

#pragma mark - Metal-specific methods

	// Platform-specific data for the Windows embedded in this driver.
	struct WindowPlatformData {
		CA::MetalLayer *layer;
	};

	class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) Surface {
	protected:
		MTL::Device *device;

	public:
		uint32_t width = 0;
		uint32_t height = 0;
		DisplayServerEnums::VSyncMode vsync_mode = DisplayServerEnums::VSYNC_ENABLED;
		double present_minimum_duration = 0.0;
		MTL::PixelFormat pixel_format = MTL::PixelFormatBGRA8Unorm;

		// BT.2408 recommendation of 203 nits for HDR Reference White, rounded to 200
		// to be a more pleasant player-facing value.
		float hdr_reference_luminance = 200.0f;
		float hdr_max_luminance = 1000.0f;
		// linear_luminance_scale must always equal hdr_reference_luminance on Apple platforms.
		float hdr_linear_luminance_scale = 200.0f;
		bool needs_resize = false;
		bool hdr_output = false;

		Surface(MTL::Device *p_device) :
				device(p_device) {}
		virtual ~Surface() = default;

		MTL::PixelFormat get_pixel_format() const { return pixel_format; }
		void set_hdr_output_enabled(bool p_enabled) {
			if (hdr_output != p_enabled) {
				hdr_output = p_enabled;
				needs_resize = true;
			}
		}

		bool is_hdr_output_enabled() const {
			return hdr_output;
		}
		virtual Error resize(uint32_t p_desired_framebuffer_count, RDD::DataFormat &r_format, RDD::ColorSpace &r_color_space) = 0;
		virtual RDD::FramebufferID acquire_next_frame_buffer() = 0;
		virtual void present(MTL3::MDCommandBuffer *p_cmd_buffer) = 0;
		virtual MTL::Drawable *next_drawable() = 0;
		API_AVAILABLE(macos(26.0), ios(26.0))
		virtual MTL::ResidencySet *get_residency_set() const = 0;
		void set_max_fps(int p_max_fps) { present_minimum_duration = p_max_fps ? 1.0 / p_max_fps : 0.0; }
	};

	MTL::Device *get_metal_device() const {
		return metal_device;
	}

#pragma mark - Initialization

	RenderingContextDriverMetal();
	~RenderingContextDriverMetal() override;
};

#endif // METAL_ENABLED
// --- End drivers/metal/rendering_context_driver_metal.h ---
