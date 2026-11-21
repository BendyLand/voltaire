/**************************************************************************/
/*  rd_caches.h                                                           */
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

#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
#include "servers/rendering/rendering_device.h"
#include "core/os/spin_lock.h"
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"
#include "servers/rendering/rendering_server_enums.h"

class FramebufferCacheRD final
{
public:
    struct Cache
    {
        Cache* prev = nullptr;
        Cache* next = nullptr;
        uint32_t hash = 0;
        RID cache;
        LocalVector<RID> textures;
        LocalVector<RD::FramebufferPass> passes;
        uint32_t views = 0;
    };

    enum
    {
        HASH_TABLE_SIZE = 16381 // Prime
    };

    struct Data
    {
        PagedAllocator<Cache> cache_allocator;
        Cache* hash_table[HASH_TABLE_SIZE] = {};
        uint32_t cache_instances_used = 0;
    };

    static inline Data* data = nullptr;

    static void initialize();
    static void finalize();
    static bool is_initialized() { return data != nullptr; }

private:
    static _FORCE_INLINE_ uint32_t _hash_pass(const RD::FramebufferPass& p, uint32_t h)
    {
        h = hash_murmur3_one_32(p.depth_attachment, h);
        h = hash_murmur3_one_32(p.depth_resolve_attachment, h);

        h = hash_murmur3_one_32(p.color_attachments.size(), h);
        for (int i = 0; i < p.color_attachments.size(); i++) {
            h = hash_murmur3_one_32(p.color_attachments[i], h);
        }

        h = hash_murmur3_one_32(p.resolve_attachments.size(), h);
        for (int i = 0; i < p.resolve_attachments.size(); i++) {
            h = hash_murmur3_one_32(p.resolve_attachments[i], h);
        }

        h = hash_murmur3_one_32(p.preserve_attachments.size(), h);
        for (int i = 0; i < p.preserve_attachments.size(); i++) {
            h = hash_murmur3_one_32(p.preserve_attachments[i], h);
        }

        return h;
    }

    static _FORCE_INLINE_ bool _compare_pass(
        const RD::FramebufferPass& a, const RD::FramebufferPass& b)
    {
        if (a.depth_attachment != b.depth_attachment) {
            return false;
        }

        if (a.depth_resolve_attachment != b.depth_resolve_attachment) {
            return false;
        }

        if (a.color_attachments.size() != b.color_attachments.size()) {
            return false;
        }

        for (int i = 0; i < a.color_attachments.size(); i++) {
            if (a.color_attachments[i] != b.color_attachments[i]) {
                return false;
            }
        }

        if (a.resolve_attachments.size() != b.resolve_attachments.size()) {
            return false;
        }

        for (int i = 0; i < a.resolve_attachments.size(); i++) {
            if (a.resolve_attachments[i] != b.resolve_attachments[i]) {
                return false;
            }
        }

        if (a.preserve_attachments.size() != b.preserve_attachments.size()) {
            return false;
        }

        for (int i = 0; i < a.preserve_attachments.size(); i++) {
            if (a.preserve_attachments[i] != b.preserve_attachments[i]) {
                return false;
            }
        }

        return true;
    }

    static _FORCE_INLINE_ uint32_t _hash_rids(uint32_t h, const RID& arg)
    {
        return hash_murmur3_one_64(arg.get_id(), h);
    }

    template <typename... Args> static uint32_t _hash_rids(uint32_t h, const RID& arg, Args... args)
    {
        h = hash_murmur3_one_64(arg.get_id(), h);
        return _hash_rids(h, args...);
    }

    static _FORCE_INLINE_ bool _compare_args(
        uint32_t idx, const LocalVector<RID>& textures, const RID& arg)
    {
        return textures[idx] == arg;
    }

    template <typename... Args>
    static _FORCE_INLINE_ bool _compare_args(
        uint32_t idx, const LocalVector<RID>& textures, const RID& arg, Args... args)
    {
        if (textures[idx] != arg) {
            return false;
        }
        return _compare_args(idx + 1, textures, args...);
    }

    static void _invalidate(Cache* p_cache);
    static void _framebuffer_invalidation_callback(void* p_userdata);

    static RID _allocate_from_data(uint32_t p_views, uint32_t p_hash, uint32_t p_table_idx,
        const Vector<RID>& p_textures, const Vector<RD::FramebufferPass>& p_passes)
    {
        ERR_FAIL_COND_V(data == nullptr, RID());

        RID rid;
        if (p_passes.size()) {
            rid = RD::framebuffer_create_multipass(
                p_textures, p_passes, RDC::INVALID_ID, p_views);
        }
        else {
            rid = RD::framebuffer_create(p_textures, RDC::INVALID_ID, p_views);
        }

        ERR_FAIL_COND_V(rid.is_null(), rid);

        Cache* c = data->cache_allocator.alloc();
        c->views = p_views;
        c->cache = rid;
        c->hash = p_hash;
        c->textures.resize(p_textures.size());
        for (uint32_t i = 0; i < c->textures.size(); i++) {
            c->textures[i] = p_textures[i];
        }
        c->passes.resize(p_passes.size());
        for (uint32_t i = 0; i < c->passes.size(); i++) {
            c->passes[i] = p_passes[i];
        }
        c->prev = nullptr;
        c->next = data->hash_table[p_table_idx];
        if (data->hash_table[p_table_idx]) {
            data->hash_table[p_table_idx]->prev = c;
        }
        data->hash_table[p_table_idx] = c;

        RD::framebuffer_set_invalidation_callback(
            rid, _framebuffer_invalidation_callback, c);

        data->cache_instances_used++;

        return rid;
    }

public:
    template <typename... Args> static RID get_cache(Args... args)
    {
        ERR_FAIL_COND_V(data == nullptr, RID());

        uint32_t h = hash_murmur3_one_32(1); // 1 view
        h = hash_murmur3_one_32(sizeof...(Args), h);
        h = _hash_rids(h, args...);
        h = hash_murmur3_one_32(0, h); // 0 passes
        h = hash_fmix32(h);

        uint32_t table_idx = h % HASH_TABLE_SIZE;
        {
            const Cache* c = data->hash_table[table_idx];

            while (c) {
                if (c->hash == h && c->passes.is_empty() && c->textures.size() == sizeof...(Args) &&
                    c->views == 1 && _compare_args(0, c->textures, args...)) {
                    return c->cache;
                }
                c = c->next;
            }
        }

        // Not in cache, create:
        return _allocate_from_data(
            1, h, table_idx, Vector<RID>{args...}, Vector<RD::FramebufferPass>());
    }

    template <typename... Args> static RID get_cache_multiview(uint32_t p_views, Args... args)
    {
        ERR_FAIL_COND_V(data == nullptr, RID());

        uint32_t h = hash_murmur3_one_32(p_views);
        h = hash_murmur3_one_32(sizeof...(Args), h);
        h = _hash_rids(h, args...);
        h = hash_murmur3_one_32(0, h); // 0 passes
        h = hash_fmix32(h);

        uint32_t table_idx = h % HASH_TABLE_SIZE;
        {
            const Cache* c = data->hash_table[table_idx];

            while (c) {
                if (c->hash == h && c->passes.is_empty() && c->textures.size() == sizeof...(Args) &&
                    c->views == p_views && _compare_args(0, c->textures, args...)) {
                    return c->cache;
                }
                c = c->next;
            }
        }

        // Not in cache, create:
        return _allocate_from_data(
            p_views, h, table_idx, Vector<RID>{args...}, Vector<RD::FramebufferPass>());
    }

    static RID get_cache_multipass(const Vector<RID>& p_textures,
        const Vector<RD::FramebufferPass>& p_passes, uint32_t p_views = 1)
    {
        ERR_FAIL_COND_V(data == nullptr, RID());

        uint32_t h = hash_murmur3_one_32(p_views);
        h = hash_murmur3_one_32(p_textures.size(), h);
        for (int i = 0; i < p_textures.size(); i++) {
            h = hash_murmur3_one_64(p_textures[i].get_id(), h);
        }
        h = hash_murmur3_one_32(p_passes.size(), h);
        for (int i = 0; i < p_passes.size(); i++) {
            h = _hash_pass(p_passes[i], h);
        }

        h = hash_fmix32(h);

        uint32_t table_idx = h % HASH_TABLE_SIZE;
        {
            const Cache* c = data->hash_table[table_idx];

            while (c) {
                if (c->hash == h && c->views == p_views &&
                    c->textures.size() == (uint32_t)p_textures.size() &&
                    c->passes.size() == (uint32_t)p_passes.size()) {
                    bool all_ok = true;

                    for (int i = 0; i < p_textures.size(); i++) {
                        if (p_textures[i] != c->textures[i]) {
                            all_ok = false;
                            break;
                        }
                    }

                    if (all_ok) {
                        for (int i = 0; i < p_passes.size(); i++) {
                            if (!_compare_pass(p_passes[i], c->passes[i])) {
                                all_ok = false;
                                break;
                            }
                        }
                    }

                    if (all_ok) {
                        return c->cache;
                    }
                }
                c = c->next;
            }
        }

        // Not in cache, create:
        return _allocate_from_data(p_views, h, table_idx, p_textures, p_passes);
    }

    FramebufferCacheRD() = delete;
    FramebufferCacheRD(const FramebufferCacheRD&) = delete;
    FramebufferCacheRD& operator=(const FramebufferCacheRD&) = delete;
    ~FramebufferCacheRD() = delete;
};

class PipelineCacheRD final
{
	SpinLock spin_lock;

	RID shader;

	RDC::RenderPrimitive render_primitive;
	RDC::PipelineRasterizationState rasterization_state;
	RDC::PipelineMultisampleState multisample_state;
	RDC::PipelineDepthStencilState depth_stencil_state;
	RDC::PipelineColorBlendState blend_state;
	int dynamic_state_flags = 0;
	Vector<RDC::PipelineSpecializationConstant> base_specialization_constants;

	struct Version
	{
		RD::VertexFormatID vertex_id;
		RD::FramebufferFormatID framebuffer_id;
		uint32_t render_pass;
		bool wireframe;
		uint32_t bool_specializations;
		RID pipeline;
	};

	Version* versions = nullptr;
	uint32_t version_count;

	RID _generate_version(RD::VertexFormatID p_vertex_format_id,
		RD::FramebufferFormatID p_framebuffer_format_id, bool p_wireframe, uint32_t p_render_pass,
		uint32_t p_bool_specializations = 0);

	void _clear();

public:
	void setup(RID p_shader, RDC::RenderPrimitive p_primitive,
		const RDC::PipelineRasterizationState& p_rasterization_state,
		RDC::PipelineMultisampleState p_multisample,
		const RDC::PipelineDepthStencilState& p_depth_stencil_state,
		const RDC::PipelineColorBlendState& p_blend_state, int p_dynamic_state_flags = 0,
		const Vector<RDC::PipelineSpecializationConstant>& p_base_specialization_constants =
			Vector<RDC::PipelineSpecializationConstant>());
	void update_specialization_constants(
		const Vector<RDC::PipelineSpecializationConstant>& p_base_specialization_constants);
	void update_shader(RID p_shader);

	_FORCE_INLINE_ RID get_render_pipeline(RD::VertexFormatID p_vertex_format_id,
		RD::FramebufferFormatID p_framebuffer_format_id, bool p_wireframe = false,
		uint32_t p_render_pass = 0, uint32_t p_bool_specializations = 0)
	{
#ifdef DEBUG_ENABLED
		ERR_FAIL_COND_V_MSG(
			shader.is_null(), RID(), "Attempted to use an unused shader variant (shader is null),");
#endif

		spin_lock.lock();
		p_wireframe |= rasterization_state.wireframe;

		RID result;
		for (uint32_t i = 0; i < version_count; i++) {
			if (versions[i].vertex_id == p_vertex_format_id &&
				versions[i].framebuffer_id == p_framebuffer_format_id &&
				versions[i].wireframe == p_wireframe && versions[i].render_pass == p_render_pass &&
				versions[i].bool_specializations == p_bool_specializations) {
				result = versions[i].pipeline;
				spin_lock.unlock();
				return result;
			}
		}
		result = _generate_version(p_vertex_format_id, p_framebuffer_format_id, p_wireframe,
			p_render_pass, p_bool_specializations);
		spin_lock.unlock();
		return result;
	}

	_FORCE_INLINE_ uint64_t get_vertex_input_mask()
	{
		ERR_FAIL_COND_V(shader.is_null(), 0);
		return RD::shader_get_vertex_input_attribute_mask(shader);
	}

	void clear();
	PipelineCacheRD();
	~PipelineCacheRD();
};

class UniformSetCacheRD final
{
public:
	struct Cache
	{
		Cache* prev = nullptr;
		Cache* next = nullptr;
		uint32_t hash = 0;
		RID shader;
		uint32_t set = 0;
		RID cache;
		LocalVector<RD::Uniform> uniforms;
	};

	enum
	{
		HASH_TABLE_SIZE = 16381 // Prime
	};

	struct Data
	{
		PagedAllocator<Cache> cache_allocator;
		Cache* hash_table[HASH_TABLE_SIZE] = {};
		uint32_t cache_instances_used = 0;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

private:
	static _FORCE_INLINE_ uint32_t _hash_uniform(const RD::Uniform& u, uint32_t h)
	{
		h = hash_murmur3_one_32(u.uniform_type, h);
		h = hash_murmur3_one_32(u.binding, h);
		uint32_t rsize = u.get_id_count();
		for (uint32_t j = 0; j < rsize; j++) {
			h = hash_murmur3_one_64(u.get_id(j).get_id(), h);
		}
		return hash_fmix32(h);
	}

	static _FORCE_INLINE_ bool _compare_uniform(const RD::Uniform& a, const RD::Uniform& b)
	{
		if (a.binding != b.binding) {
			return false;
		}
		if (a.uniform_type != b.uniform_type) {
			return false;
		}
		uint32_t rsize = a.get_id_count();
		if (rsize != b.get_id_count()) {
			return false;
		}
		for (uint32_t j = 0; j < rsize; j++) {
			if (a.get_id(j) != b.get_id(j)) {
				return false;
			}
		}
		return true;
	}

	static _FORCE_INLINE_ uint32_t _hash_args(uint32_t h, const RD::Uniform& arg)
	{
		return _hash_uniform(arg, h);
	}

	template <typename... Args>
	static uint32_t _hash_args(uint32_t h, const RD::Uniform& arg, Args... args)
	{
		h = _hash_uniform(arg, h);
		return _hash_args(h, args...);
	}

	static _FORCE_INLINE_ bool _compare_args(
		uint32_t idx, const LocalVector<RD::Uniform>& uniforms, const RD::Uniform& arg)
	{
		return _compare_uniform(uniforms[idx], arg);
	}

	template <typename... Args>
	static _FORCE_INLINE_ bool _compare_args(uint32_t idx, const LocalVector<RD::Uniform>& uniforms,
		const RD::Uniform& arg, Args... args)
	{
		if (!_compare_uniform(uniforms[idx], arg)) {
			return false;
		}
		return _compare_args(idx + 1, uniforms, args...);
	}

	static void _invalidate(Cache* p_cache);
	static void _uniform_set_invalidation_callback(void* p_userdata);

	template <typename Collection>
	static RID _allocate_from_uniforms(RID p_shader, uint32_t p_set, uint32_t p_hash,
		uint32_t p_table_idx, const Collection& p_uniforms)
	{
		ERR_FAIL_COND_V(data == nullptr, RID());

		RID rid = RD::uniform_set_create(p_uniforms, p_shader, p_set);
		ERR_FAIL_COND_V(rid.is_null(), rid);

		Cache* c = data->cache_allocator.alloc();
		c->hash = p_hash;
		c->set = p_set;
		c->shader = p_shader;
		c->cache = rid;
		c->uniforms.resize(p_uniforms.size());
		for (uint32_t i = 0; i < c->uniforms.size(); i++) {
			c->uniforms[i] = p_uniforms[i];
		}
		c->prev = nullptr;
		c->next = data->hash_table[p_table_idx];
		if (data->hash_table[p_table_idx]) {
			data->hash_table[p_table_idx]->prev = c;
		}
		data->hash_table[p_table_idx] = c;

		RD::uniform_set_set_invalidation_callback(rid, _uniform_set_invalidation_callback, c);

		data->cache_instances_used++;

		return rid;
	}

public:
	template <typename... Args> static RID get_cache(RID p_shader, uint32_t p_set, Args... args)
	{
		ERR_FAIL_COND_V(data == nullptr, RID());

		uint32_t h = hash_murmur3_one_64(p_shader.get_id());
		h = hash_murmur3_one_32(p_set, h);
		h = _hash_args(h, args...);

		uint32_t table_idx = h % HASH_TABLE_SIZE;
		{
			const Cache* c = data->hash_table[table_idx];

			while (c) {
				if (c->hash == h && c->set == p_set && c->shader == p_shader &&
					sizeof...(Args) == c->uniforms.size() &&
					_compare_args(0, c->uniforms, args...)) {
					return c->cache;
				}
				c = c->next;
			}
		}

		// Not in cache, create:
		return _allocate_from_uniforms(p_shader, p_set, h, table_idx, Vector<RD::Uniform>{args...});
	}

	static RID get_cache_vec(
		RID p_shader, uint32_t p_set, const LocalVector<RD::Uniform>& p_uniforms)
	{
		ERR_FAIL_COND_V(data == nullptr, RID());

		uint32_t h = hash_murmur3_one_64(p_shader.get_id());
		h = hash_murmur3_one_32(p_set, h);
		for (uint32_t i = 0; i < p_uniforms.size(); i++) {
			h = _hash_uniform(p_uniforms[i], h);
		}

		h = hash_fmix32(h);

		uint32_t table_idx = h % HASH_TABLE_SIZE;
		{
			const Cache* c = data->hash_table[table_idx];

			while (c) {
				if (c->hash == h && c->set == p_set && c->shader == p_shader &&
					(uint32_t)p_uniforms.size() == c->uniforms.size()) {
					bool all_ok = true;
					for (uint32_t i = 0; i < p_uniforms.size(); i++) {
						if (!_compare_uniform(p_uniforms[i], c->uniforms[i])) {
							all_ok = false;
							break;
						}
					}

					if (all_ok) {
						return c->cache;
					}
				}
				c = c->next;
			}
		}

		// Not in cache, create:
		return _allocate_from_uniforms(p_shader, p_set, h, table_idx, p_uniforms);
	}

	UniformSetCacheRD() = delete;
	UniformSetCacheRD(const UniformSetCacheRD&) = delete;
	UniformSetCacheRD& operator=(const UniformSetCacheRD&) = delete;
	~UniformSetCacheRD() = delete;
};

// Helper class for automatically deferring compilation of a pipeline to a background task.
// When attempting to retrieve the pipeline with the getter, the caller will automatically
// wait for it to be ready.

class PipelineDeferredRD final
{
protected:
	struct CreationParameters
	{
		RID shader;
		RD::FramebufferFormatID framebuffer_format;
		RD::VertexFormatID vertex_format;
		RDC::RenderPrimitive render_primitive;
		RDC::PipelineRasterizationState rasterization_state;
		RDC::PipelineMultisampleState multisample_state;
		RDC::PipelineDepthStencilState depth_stencil_state;
		RDC::PipelineColorBlendState blend_state;
		uint32_t dynamic_state_flags;
		uint32_t for_render_pass;
		Vector<RDC::PipelineSpecializationConstant> specialization_constants;
		bool is_compute;
	};

	RID pipeline;

	void _create(const CreationParameters& c)
	{
		if (c.is_compute) {
			pipeline =
				RD::compute_pipeline_create(c.shader, c.specialization_constants);
		}
		else {
			pipeline = RD::render_pipeline_create(c.shader, c.framebuffer_format,
				c.vertex_format, c.render_primitive, c.rasterization_state, c.multisample_state,
				c.depth_stencil_state, c.blend_state, c.dynamic_state_flags, c.for_render_pass,
				c.specialization_constants);
		}
	}

	void _start(const CreationParameters& c) { free(); }

public:
	PipelineDeferredRD()
	{
		// Default constructor.
	}

	~PipelineDeferredRD()
	{
#ifdef DEV_ENABLED
		ERR_FAIL_COND_MSG(
			pipeline.is_valid(), "'free()' must be called manually before deconstruction and "
								 "before the corresponding shader is freed.");
#endif
	}

	void create_render_pipeline(RID p_shader, RD::FramebufferFormatID p_framebuffer_format,
		RD::VertexFormatID p_vertex_format, RDC::RenderPrimitive p_render_primitive,
		const RDC::PipelineRasterizationState& p_rasterization_state,
		const RDC::PipelineMultisampleState& p_multisample_state,
		const RDC::PipelineDepthStencilState& p_depth_stencil_state,
		const RDC::PipelineColorBlendState& p_blend_state, uint32_t())
	{
		CreationParameters c;
		c.shader = p_shader;
		c.framebuffer_format = p_framebuffer_format;
		c.vertex_format = p_vertex_format;
		c.render_primitive = p_render_primitive;
		c.rasterization_state = p_rasterization_state;
		c.multisample_state = p_multisample_state;
		c.depth_stencil_state = p_depth_stencil_state;
		c.blend_state = p_blend_state;
		c.is_compute = false;
		_start(c);
	}

	void create_compute_pipeline(
		RID p_shader, const Vector<RDC::PipelineSpecializationConstant>& p_specialization_constants =
						  Vector<RDC::PipelineSpecializationConstant>())
	{
		CreationParameters c = {};
		c.shader = p_shader;
		c.specialization_constants = p_specialization_constants;
		c.is_compute = true;
		_start(c);
	}

	RID get_rid() { return pipeline; }

	void free()
	{
		if (pipeline.is_valid()) {
#ifdef DEV_ENABLED
			ERR_FAIL_COND_MSG(!(RD::render_pipeline_is_valid(pipeline) ||
								  RD::compute_pipeline_is_valid(pipeline)),
				"`free()` must be called  manually before the dependent shader is freed.");
#endif
			RD::free_rid(pipeline);
			pipeline = RID();
		}
	}
};

#define PRINT_PIPELINE_COMPILATION_KEYS 0

template <typename Key, typename CreationClass, typename CreationFunction> class PipelineHashMapRD final
{
private:
	CreationClass* creation_object = nullptr;
	CreationFunction creation_function = nullptr;
	Mutex* compilations_mutex = nullptr;
	uint32_t* compilations = nullptr;
	RBMap<uint32_t, RID> hash_map;
	LocalVector<Pair<uint32_t, RID>> compiled_queue;
	Mutex compiled_queue_mutex;
	RBSet<uint32_t> compilation_set;
	Mutex local_mutex;

	bool _add_new_pipelines_to_map()
	{
		bool added = false;
		MutexLock lock(compiled_queue_mutex);
		for (const Pair<uint32_t, RID>& pair : compiled_queue) {
			hash_map[pair.first] = pair.second;
			added = true;
		}
		compiled_queue.clear();
		return added;
	}

public:
	void add_compiled_pipeline(uint32_t p_hash, RID p_pipeline)
	{
		compiled_queue_mutex.lock();
		compiled_queue.push_back({p_hash, p_pipeline});
		compiled_queue_mutex.unlock();
	}

	// Start compilation of a pipeline ahead of time in the background. Returns true if the
	// compilation was started, false if it wasn't required. Source is only used for collecting
	// statistics.
	void compile_pipeline(
		const Key& p_key, uint32_t p_key_hash, RSE::PipelineSource p_source, bool p_high_priority)
	{
		DEV_ASSERT(
			(creation_object != nullptr) && (creation_function != nullptr) &&
			"Creation object and function was not set before attempting to compile a pipeline.");

		{
			MutexLock local_lock(local_mutex);
			if (compilation_set.has(p_key_hash)) {
				// Check if the pipeline was already submitted.
				return;
			}

			// Record the pipeline as submitted, a task can't be started for it again.
			compilation_set.insert(p_key_hash);

			if (compilations_mutex != nullptr) {
				MutexLock compilations_lock(*compilations_mutex);
				compilations[p_source]++;
			}
		}

#if PRINT_PIPELINE_COMPILATION_KEYS
		String source_name = "UNKNOWN";
		switch (p_source) {
		case RSE::PIPELINE_SOURCE_CANVAS:
			source_name = "CANVAS";
			break;
		case RSE::PIPELINE_SOURCE_MESH:
			source_name = "MESH";
			break;
		case RSE::PIPELINE_SOURCE_SURFACE:
			source_name = "SURFACE";
			break;
		case RSE::PIPELINE_SOURCE_DRAW:
			source_name = "DRAW";
			break;
		case RSE::PIPELINE_SOURCE_SPECIALIZATION:
			source_name = "SPECIALIZATION";
			break;
		}

		print_line("HASH:", p_key_hash, "SOURCE:", source_name);
#endif

		(creation_object->*creation_function)(p_key);
	}

	RID get_pipeline(const Key& p_key, uint32_t p_key_hash, bool p_wait_for_compilation,
		RSE::PipelineSource p_source)
	{
		RBMap<uint32_t, RID>::Element* e = hash_map.find(p_key_hash);

		if (e == nullptr) {
			if (_add_new_pipelines_to_map()) {
				e = hash_map.find(p_key_hash);
			}
		}

		if (e == nullptr) {
			compile_pipeline(p_key, p_key_hash, p_source, p_wait_for_compilation);
			_add_new_pipelines_to_map();

			e = hash_map.find(p_key_hash);
			if (e != nullptr) {
				return e->value();
			}
			else if (p_wait_for_compilation) {
				hash_map[p_key_hash] = RID();
				return RID();
			}
			else {
				return RID();
			}
		}
		else {
			return e->value();
		}
	}

	void clear_pipelines()
	{
		_add_new_pipelines_to_map();

		if (RD::is_initialized()) {
			for (KeyValue<uint32_t, RID> entry : hash_map) {
				if (entry.value.is_valid()) {
					RD::free_rid(entry.value);
				}
			}
		}

		hash_map.clear();
		compilation_set.clear();
	}

	// Set the external pipeline compilations array to increase the counters on every time a
	// pipeline is compiled.
	void set_compilations(uint32_t* p_compilations, Mutex* p_compilations_mutex)
	{
		compilations = p_compilations;
		compilations_mutex = p_compilations_mutex;
	}

	void set_creation_object_and_function(
		CreationClass* p_creation_object, CreationFunction p_creation_function)
	{
		creation_object = p_creation_object;
		creation_function = p_creation_function;
	}

	PipelineHashMapRD() {}

	~PipelineHashMapRD() { clear_pipelines(); }
};
