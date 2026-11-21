/**************************************************************************/
/*  rendering_device.cpp                                                  */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/utils.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/templates/fixed_vector.h"
#include "modules/modules_enabled.gen.h"
#include "rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/rendering_device_commons.h"
#include "servers/rendering/rendering_shader_container.h"
#include "servers/rendering/shader_include_db.h"

#ifdef MODULE_GLSLANG_ENABLED
#include "modules/glslang/shader_compile.h"
#endif

#define FORCE_SEPARATE_PRESENT_QUEUE 0
#define PRINT_FRAMEBUFFER_FORMAT 0

#define ERR_RENDER_THREAD_MSG                                                                      \
	String("This function (") + String(__func__) +                                                 \
		String(") can only be called from the render thread. ")
#define ERR_RENDER_THREAD_GUARD()                                                                  \
	ERR_FAIL_COND_MSG(data->render_thread_id != Thread::get_caller_id(), ERR_RENDER_THREAD_MSG);
#define ERR_RENDER_THREAD_GUARD_V(m_ret)                                                           \
	ERR_FAIL_COND_V_MSG(                                                                           \
		data->render_thread_id != Thread::get_caller_id(), (m_ret), ERR_RENDER_THREAD_MSG);

static String _get_device_vendor_name(const RenderingContextDriver::Device& p_device)
{
	switch (p_device.vendor) {
	case RenderingContextDriver::Vendor::VENDOR_AMD:
		return "AMD";
	case RenderingContextDriver::Vendor::VENDOR_IMGTEC:
		return "ImgTec";
	case RenderingContextDriver::Vendor::VENDOR_APPLE:
		return "Apple";
	case RenderingContextDriver::Vendor::VENDOR_NVIDIA:
		return "NVIDIA";
	case RenderingContextDriver::Vendor::VENDOR_ARM:
		return "ARM";
	case RenderingContextDriver::Vendor::VENDOR_MICROSOFT:
		return "Microsoft";
	case RenderingContextDriver::Vendor::VENDOR_QUALCOMM:
		return "Qualcomm";
	case RenderingContextDriver::Vendor::VENDOR_INTEL:
		return "Intel";
	default:
		return "Unknown";
	}
}

static String _get_device_type_name(const RenderingContextDriver::Device& p_device)
{
	switch (p_device.type) {
	case RenderingContextDriver::DEVICE_TYPE_INTEGRATED_GPU:
		return "Integrated";
	case RenderingContextDriver::DEVICE_TYPE_DISCRETE_GPU:
		return "Discrete";
	case RenderingContextDriver::DEVICE_TYPE_VIRTUAL_GPU:
		return "Virtual";
	case RenderingContextDriver::DEVICE_TYPE_CPU:
		return "CPU";
	case RenderingContextDriver::DEVICE_TYPE_OTHER:
	default:
		return "Other";
	}
}

static uint32_t _get_device_type_score(const RenderingContextDriver::Device& p_device)
{
	static const bool prefer_integrated = OS::get_user_prefers_integrated_gpu();
	switch (p_device.type) {
	case RenderingContextDriver::DEVICE_TYPE_INTEGRATED_GPU:
		return prefer_integrated ? 5 : 4;
	case RenderingContextDriver::DEVICE_TYPE_DISCRETE_GPU:
		return prefer_integrated ? 4 : 5;
	case RenderingContextDriver::DEVICE_TYPE_VIRTUAL_GPU:
		return 3;
	case RenderingContextDriver::DEVICE_TYPE_CPU:
		return 2;
	case RenderingContextDriver::DEVICE_TYPE_OTHER:
	default:
		return 1;
	}
}

static uint32_t _decode_hit_sbt_range_offset(RD::HitShaderBindingTableRange p_range)
{
	return uint32_t(uint64_t(p_range) & 0xFFFFFFFF);
}

static uint32_t _decode_hit_sbt_range_count(RD::HitShaderBindingTableRange p_range)
{
	return uint32_t((uint64_t(p_range) >> 32) & 0xFFFFFFFF);
}

static RD::HitShaderBindingTableRange _encode_hit_sbt_range(uint32_t p_offset, uint32_t p_count)
{
	return RD::HitShaderBindingTableRange((uint64_t(p_count) << 32) | uint64_t(p_offset));
}

// When true, the command graph will attempt to reorder the rendering commands submitted by the user
// based on the dependencies detected from the commands automatically. This should improve rendering
// performance in most scenarios at the cost of some extra CPU overhead.
//
// This behavior can be disabled if it's suspected that the graph is not detecting dependencies
// correctly and more control over the order of the commands is desired (e.g. debugging).

#define RENDER_GRAPH_REORDER 1

// Synchronization barriers are issued between the graph's levels only with the necessary amount of
// detail to achieve the correct result. If it's suspected that the graph is not doing this
// correctly, full barriers can be issued instead that will block all types of operations between
// the synchronization levels. This setting will have a very negative impact on performance when
// enabled, so it's only intended for debugging purposes.

#define RENDER_GRAPH_FULL_BARRIERS 0

// The command graph can automatically issue secondary command buffers and record them on background
// threads when they reach an arbitrary size threshold. This can be very beneficial towards reducing
// the time the main thread takes to record all the rendering commands. However, this setting is not
// enabled by default as it's been shown to cause some strange issues with certain IHVs that have
// yet to be understood.

#define SECONDARY_COMMAND_BUFFERS_PER_FRAME 0

void RenderingDevice::_add_dependency(RID p_id, RID p_depends_on)
{
	HashSet<RID>* set = data->dependency_map.getptr(p_depends_on);
	if (set == nullptr) {
		set = &data->dependency_map.insert(p_depends_on, HashSet<RID>())->value;
	}
	set->insert(p_id);

	set = data->reverse_dependency_map.getptr(p_id);
	if (set == nullptr) {
		set = &data->reverse_dependency_map.insert(p_id, HashSet<RID>())->value;
	}
	set->insert(p_depends_on);
}

void RenderingDevice::_remove_dependency(RID p_id, RID p_depends_on)
{
	HashSet<RID>* set = data->dependency_map.getptr(p_depends_on);
	if (set != nullptr) {
		set->erase(p_id);
	}

	set = data->reverse_dependency_map.getptr(p_id);
	if (set != nullptr) {
		set->erase(p_depends_on);
	}
}

void RenderingDevice::_free_dependencies(RID p_id)
{
	Vector<RID> dependencies_to_free;

	{
		MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

		// Direct dependencies must be freed.
		HashMap<RID, HashSet<RID>>::Iterator E = data->dependency_map.find(p_id);
		if (E) {
			for (const RID& dep : E->value) {
				if (dep.is_valid()) {
					dependencies_to_free.push_back(dep);
				}
			}
			data->dependency_map.remove(E);
		}

		// Reverse dependencies must be unreferenced.
		E = data->reverse_dependency_map.find(p_id);
		if (E) {
			for (const RID& F : E->value) {
				HashMap<RID, HashSet<RID>>::Iterator G = data->dependency_map.find(F);
				if (G && G->value.has(p_id)) {
					G->value.erase(p_id);
				}
			}

			data->reverse_dependency_map.remove(E);
		}
	}

	for (const RID& dep : dependencies_to_free) {
		free_rid(dep);
	}
}

Vector<uint8_t> RenderingDevice::shader_compile_spirv_from_source(RDC::ShaderStage p_stage,
	const String& p_source_code, RDC::ShaderLanguage p_language, String* r_error,
	bool p_allow_cache)
{
	switch (p_language) {
#ifdef MODULE_GLSLANG_ENABLED
	case RDC::ShaderLanguage::SHADER_LANGUAGE_GLSL: {
		RDC::ShaderLanguageVersion language_version =
			data->driver->get_shader_container_format().get_shader_language_version();
		RDC::ShaderSpirvVersion spirv_version =
			data->driver->get_shader_container_format().get_shader_spirv_version();
		return compile_glslang_shader(p_stage, ShaderIncludeDB::parse_include_files(p_source_code),
			language_version, spirv_version, r_error);
	}
#endif
	default:
		ERR_FAIL_V_MSG(Vector<uint8_t>(), "Shader language is not supported.");
	}
}

Error RenderingDevice::_acceleration_structure_scratch_buffer_create(
	AccelerationStructure* p_acceleration_structure)
{
	uint64_t scratch_size = data->driver->acceleration_structure_get_scratch_size_bytes(
		p_acceleration_structure->driver_id);

	if (p_acceleration_structure->scratch_buffer) {
		uint64_t scratch_buffer_size =
			data->driver->buffer_get_allocation_size(p_acceleration_structure->scratch_buffer);
		if (scratch_buffer_size < scratch_size) {
			Buffer to_dispose = Buffer();
			to_dispose.driver_id = p_acceleration_structure->scratch_buffer;
			DEV_ASSERT(scratch_buffer_size <= UINT32_MAX);
			to_dispose.size = scratch_buffer_size;
			data->frames[data->frame].buffers_to_dispose_of.push_back(to_dispose);
			p_acceleration_structure->scratch_buffer = RDD::BufferID();
		}
	}

	if (!p_acceleration_structure->scratch_buffer) {
		p_acceleration_structure->scratch_buffer = data->driver->buffer_create(scratch_size,
			RDD::BUFFER_USAGE_STORAGE_BIT | RDD::BUFFER_USAGE_DEVICE_ADDRESS_BIT,
			RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
		ERR_FAIL_COND_V(!p_acceleration_structure->scratch_buffer, ERR_CANT_CREATE);

		data->buffer_memory.add(scratch_size);
	}

	return OK;
}

void RenderingDevice::_blas_remove_tlas_dependencies(AccelerationStructure* p_blas, RID p_blas_id)
{
	for (RID id : p_blas->acceleration_structure_dependencies) {
		AccelerationStructure* tlas = data->acceleration_structure_owner.get_or_null(id);
		ERR_FAIL_NULL(tlas);
		ERR_FAIL_COND(tlas->type != RDC::ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL);

		tlas->invalidated = true;
		tlas->acceleration_structure_dependencies.erase(p_blas_id);
	}

	p_blas->acceleration_structure_dependencies.clear();
}

void RenderingDevice::_tlas_remove_blas_dependencies(AccelerationStructure* p_tlas, RID p_tlas_id)
{
	for (RID id : p_tlas->acceleration_structure_dependencies) {
		AccelerationStructure* blas = data->acceleration_structure_owner.get_or_null(id);
		ERR_FAIL_NULL(blas);
		ERR_FAIL_COND(blas->type != RDC::ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL);

		blas->acceleration_structure_dependencies.erase(p_tlas_id);
	}

	p_tlas->acceleration_structure_dependencies.clear();
}

RID RenderingDevice::blas_create(Span<AccelerationStructureGeometry> p_geometries, uint32_t p_flags)
{
	ERR_FAIL_COND_V_MSG(
		!has_feature(RDC::SUPPORTS_RAYTRACING_PIPELINE) && !has_feature(RDC::SUPPORTS_RAY_QUERY),
		RID(),
		"The current rendering device has neither raytracing pipeline nor ray query support.");

	thread_local LocalVector<RDD::AccelerationStructureGeometry> rdd_geometries;
	rdd_geometries.resize(p_geometries.size());

	Vector<RDG::ResourceTracker*> draw_trackers;
	HashSet<RID> untracked_buffers;

	for (uint64_t i = 0; i < p_geometries.size(); i++) {
		const AccelerationStructureGeometry& rd_geometry = p_geometries[i];
		if (rd_geometry.index_buffer.is_valid()) {
			ERR_FAIL_COND_V_MSG((rd_geometry.index_count % 3) != 0, RID(),
				"An indexed geometry must have an index count that is a multiple of 3.");
		}
		else {
			ERR_FAIL_COND_V_MSG((rd_geometry.vertex_count % 3) != 0, RID(),
				"A non-indexed geometry must have a vertex count that is a multiple of 3.");
		}

		RDD::AccelerationStructureGeometry& rdd_geometry = rdd_geometries[i];
		rdd_geometry = {};
		rdd_geometry.flags = rd_geometry.flags;

		Buffer* vertex_buffer = data->vertex_buffer_owner.get_or_null(rd_geometry.vertex_buffer);
		ERR_FAIL_NULL_V(vertex_buffer, RID());

		ERR_FAIL_COND_V_MSG(
			(rd_geometry.vertex_offset + rd_geometry.vertex_count * rd_geometry.vertex_stride) >
				vertex_buffer->size,
			RID(),
			"The specified vertex offset and count are outside the range of the vertex buffer.");
		ERR_FAIL_COND_V_MSG(rd_geometry.vertex_format >= RDC::DataFormat::DATA_FORMAT_MAX, RID(),
			"An invalid vertex format was specified.");

		rdd_geometry.vertex_buffer = vertex_buffer->driver_id;
		rdd_geometry.vertex_offset = rd_geometry.vertex_offset;
		rdd_geometry.vertex_stride = rd_geometry.vertex_stride;
		rdd_geometry.vertex_count = rd_geometry.vertex_count;
		rdd_geometry.vertex_format = rd_geometry.vertex_format;

		if (vertex_buffer->draw_tracker != nullptr) {
			draw_trackers.push_back(vertex_buffer->draw_tracker);
		}
		else {
			untracked_buffers.insert(rd_geometry.vertex_buffer);
		}

		_check_transfer_worker_buffer(vertex_buffer);

		if (rd_geometry.index_buffer.is_valid()) {
			IndexBuffer* index_buffer =
				data->index_buffer_owner.get_or_null(rd_geometry.index_buffer);
			ERR_FAIL_NULL_V(index_buffer, RID());

			uint32_t index_stride =
				(index_buffer->format == RDC::INDEX_BUFFER_FORMAT_UINT32 ? sizeof(uint32_t)
																		 : sizeof(uint16_t));
			ERR_FAIL_COND_V_MSG((rd_geometry.index_offset +
									rd_geometry.index_count * index_stride) > index_buffer->size,
				RID(),
				"The specified index offset and count are outside the range of the index buffer.");
			ERR_FAIL_COND_V_MSG(index_buffer->max_index >= rd_geometry.vertex_count, RID(),
				"The index buffer contains an index that is outside the specified vertex range.");

			rdd_geometry.index_buffer = index_buffer->driver_id;
			rdd_geometry.index_offset = rd_geometry.index_offset;
			rdd_geometry.index_count = rd_geometry.index_count;
			rdd_geometry.index_format = index_buffer->format;

			if (index_buffer->draw_tracker != nullptr) {
				draw_trackers.push_back(index_buffer->draw_tracker);
			}
			else {
				untracked_buffers.insert(rd_geometry.index_buffer);
			}

			_check_transfer_worker_buffer(index_buffer);
		}
	}

	AccelerationStructure acceleration_structure;
	acceleration_structure.type = RDC::ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	acceleration_structure.driver_id = data->driver->blas_create(rdd_geometries, p_flags);
	ERR_FAIL_COND_V_MSG(!acceleration_structure.driver_id, RID(), "Failed to create BLAS.");

	acceleration_structure.draw_tracker = RDG::resource_tracker_create();
	acceleration_structure.draw_tracker->acceleration_structure_driver_id =
		acceleration_structure.driver_id;
	// Assume we are going to build this acceleration structure
	acceleration_structure.draw_tracker->usage =
		RDG::RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE;

	acceleration_structure.draw_trackers = std::move(draw_trackers);
	acceleration_structure.untracked_buffers = std::move(untracked_buffers);

	RID id = data->acceleration_structure_owner.make_rid(acceleration_structure);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif

	for (uint32_t i = 0; i < p_geometries.size(); i++) {
		const AccelerationStructureGeometry& geometry = p_geometries[i];
		_add_dependency(id, geometry.vertex_buffer);

		if (geometry.index_buffer.is_valid()) {
			_add_dependency(id, geometry.index_buffer);
		}
	}

	return id;
}

RID RenderingDevice::tlas_create(uint32_t p_max_instance_count, uint32_t p_flags)
{
	ERR_FAIL_COND_V_MSG(
		!has_feature(RDC::SUPPORTS_RAYTRACING_PIPELINE) && !has_feature(RDC::SUPPORTS_RAY_QUERY),
		RID(),
		"The current rendering device has neither raytracing pipeline nor ray query support.");

	AccelerationStructure acceleration_structure;
	acceleration_structure.type = RDC::ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	acceleration_structure.driver_id = data->driver->tlas_create(p_max_instance_count, p_flags);
	ERR_FAIL_COND_V_MSG(!acceleration_structure.driver_id, RID(), "Failed to create TLAS.");

	acceleration_structure.draw_tracker = RDG::resource_tracker_create();
	acceleration_structure.draw_tracker->acceleration_structure_driver_id =
		acceleration_structure.driver_id;
	// Assume we are going to build this acceleration structure
	acceleration_structure.draw_tracker->usage =
		RDG::RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE;

	acceleration_structure.max_instance_count = p_max_instance_count;

	RID id = data->acceleration_structure_owner.make_rid(acceleration_structure);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	return id;
}

Error RenderingDevice::blas_build(RID p_blas)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	ERR_FAIL_COND_V_MSG(data->draw_list.active, ERR_INVALID_PARAMETER,
		"Building BLAS is forbidden during creation of a draw list.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, ERR_INVALID_PARAMETER,
		"Building BLAS is forbidden during creation of a compute list.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, ERR_INVALID_PARAMETER,
		"Building BLAS is forbidden during creation of a raytracing list.");

	AccelerationStructure* blas = data->acceleration_structure_owner.get_or_null(p_blas);
	ERR_FAIL_NULL_V_MSG(blas, ERR_INVALID_PARAMETER, "BLAS argument is not valid.");

	Error err = _acceleration_structure_scratch_buffer_create(blas);
	ERR_FAIL_COND_V(err != OK, err);

	data->draw_graph.add_blas_build(
		blas->driver_id, blas->scratch_buffer, blas->draw_tracker, blas->draw_trackers);

	blas->invalidated = false;
	_blas_remove_tlas_dependencies(blas, p_blas);

	return OK;
}

Error RenderingDevice::tlas_build(RID p_tlas, Span<AccelerationStructureInstance> p_instances)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	ERR_FAIL_COND_V_MSG(data->draw_list.active, ERR_INVALID_PARAMETER,
		"Building TLAS is forbidden during creation of a draw list.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, ERR_INVALID_PARAMETER,
		"Building TLAS is forbidden during creation of a compute list.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, ERR_INVALID_PARAMETER,
		"Building TLAS is forbidden during creation of a raytracing list.");

	AccelerationStructure* tlas = data->acceleration_structure_owner.get_or_null(p_tlas);
	ERR_FAIL_NULL_V_MSG(tlas, ERR_INVALID_PARAMETER, "TLAS argument is not valid.");
	ERR_FAIL_COND_V_MSG(p_instances.size() > tlas->max_instance_count, ERR_INVALID_PARAMETER,
		"Exceeded the maximum amount of instances allowed in the TLAS.");

	Error err = _acceleration_structure_scratch_buffer_create(tlas);
	ERR_FAIL_COND_V(err != OK, err);

	uint32_t instance_size =
		data->driver->api_trait_get(RDD::API_TRAIT_ACCELERATION_STRUCTURE_INSTANCE_SIZE);
	uint32_t instance_buffer_size = instance_size * tlas->max_instance_count;

	uint32_t instance_buffer_offset = 0;
	uint32_t instance_buffer_used_size = instance_size * p_instances.size();

	uint32_t instance_buffer_index = UINT32_MAX;
	for (uint32_t i = 0; i < tlas->instance_buffers.size(); i++) {
		AccelerationStructure::InstanceBuffer& instance_buffer = tlas->instance_buffers[i];

		if ((data->frames_drawn - instance_buffer.frame_used) >= data->frames.size()) {
			instance_buffer.frame_used = data->frames_drawn;
			instance_buffer.used_size = 0;
		}

		if (data->frames_drawn == instance_buffer.frame_used &&
			(instance_buffer.used_size + instance_buffer_used_size) <= instance_buffer_size) {
			instance_buffer_offset = instance_buffer.used_size;
			instance_buffer.used_size += instance_buffer_used_size;
			instance_buffer_index = i;
			break;
		}
	}

	if (instance_buffer_index == UINT32_MAX) {
		RDD::BufferID instance_buffer_driver_id = data->driver->buffer_create(instance_buffer_size,
			RDD::BUFFER_USAGE_TRANSFER_FROM_BIT | RDD::BUFFER_USAGE_DEVICE_ADDRESS_BIT |
				RDD::BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT,
			RDD::MEMORY_ALLOCATION_TYPE_CPU, data->frames_drawn);
		ERR_FAIL_COND_V(!instance_buffer_driver_id, ERR_CANT_CREATE);

		uint8_t* data_ptr = data->driver->buffer_map(instance_buffer_driver_id);
		if (unlikely(!data_ptr)) {
			data->driver->buffer_free(instance_buffer_driver_id);
			ERR_FAIL_V(ERR_CANT_CREATE);
		}

		data->buffer_memory.add(instance_buffer_size);

		AccelerationStructure::InstanceBuffer instance_buffer;
		instance_buffer.driver_id = instance_buffer_driver_id;
		instance_buffer.frame_used = data->frames_drawn;
		instance_buffer.used_size = 0;
		instance_buffer.data_ptr = data_ptr;

		instance_buffer_index = tlas->instance_buffers.size();
		tlas->instance_buffers.push_back(instance_buffer);
	}

	AccelerationStructure::InstanceBuffer& instance_buffer =
		tlas->instance_buffers[instance_buffer_index];

	_tlas_remove_blas_dependencies(tlas, p_tlas);

	thread_local LocalVector<RDG::ResourceTracker*> draw_trackers;
	draw_trackers.clear();

	for (uint32_t i = 0; i < p_instances.size(); i++) {
		const AccelerationStructureInstance& rd_instance = p_instances[i];

		RDD::AccelerationStructureInstance rdd_instance;
		rdd_instance.transform = rd_instance.transform;
		rdd_instance.id = rd_instance.id;
		rdd_instance.mask = rd_instance.mask;
		rdd_instance.hit_sbt_offset = _decode_hit_sbt_range_offset(rd_instance.hit_sbt_range);
		rdd_instance.flags = rd_instance.flags;

		if (rd_instance.blas.is_valid()) {
			ERR_FAIL_COND_V_MSG(!rd_instance.hit_sbt_range, ERR_INVALID_PARAMETER,
				"Instance " + itos(i) + " has an invalid hit shader binding table range.");

			AccelerationStructure* blas =
				data->acceleration_structure_owner.get_or_null(rd_instance.blas);
			ERR_FAIL_NULL_V(blas, ERR_INVALID_PARAMETER);
			ERR_FAIL_COND_V(
				blas->type != RDC::ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL, ERR_INVALID_PARAMETER);
			ERR_FAIL_COND_V_MSG(blas->invalidated, ERR_INVALID_PARAMETER,
				"BLAS either has not been built yet, or has been invalidated by an operation and "
				"needs to be rebuilt.");
			rdd_instance.blas = blas->driver_id;

			if (!tlas->acceleration_structure_dependencies.has(rd_instance.blas)) {
				tlas->acceleration_structure_dependencies.insert(rd_instance.blas);
				blas->acceleration_structure_dependencies.insert(p_tlas);

				if (blas->draw_tracker != nullptr) {
					draw_trackers.push_back(blas->draw_tracker);
				}
			}
		}

		data->driver->acceleration_structure_instance_write(
			instance_buffer.data_ptr + instance_buffer_offset + (instance_size * i), rdd_instance);
	}

	data->draw_graph.add_tlas_build(tlas->driver_id, tlas->scratch_buffer,
		instance_buffer.driver_id, instance_buffer_offset, p_instances.size(), tlas->draw_tracker,
		draw_trackers);

	tlas->invalidated = false;

	return OK;
}

RDD::BufferID RenderingDevice::_hit_sbt_buffer_create(uint32_t p_buffer_size)
{
	RDD::BufferID buffer = data->driver->buffer_create(p_buffer_size,
		RDD::BUFFER_USAGE_TRANSFER_TO_BIT | RDD::BUFFER_USAGE_DEVICE_ADDRESS_BIT |
			RDD::BUFFER_USAGE_SHADER_BINDING_TABLE_BIT,
		RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	if (buffer) {
		data->buffer_memory.add(p_buffer_size);
	}

	return buffer;
}

void RenderingDevice::_hit_sbt_add_dirty_range(
	HitShaderBindingTable* p_hit_sbt, uint32_t p_offset, uint32_t p_count)
{
	p_hit_sbt->first_dirty_index = MIN(p_hit_sbt->first_dirty_index, p_offset);
	p_hit_sbt->last_dirty_index = MAX(p_hit_sbt->last_dirty_index, p_offset + p_count);
}

RID RenderingDevice::hit_sbt_create(
	RID p_raytracing_pipeline, uint32_t p_initial_hit_group_capacity)
{
	ERR_FAIL_COND_V_MSG(
		!has_feature(RDC::SUPPORTS_RAYTRACING_PIPELINE) && !has_feature(RDC::SUPPORTS_RAY_QUERY),
		RID(),
		"The current rendering device has neither raytracing pipeline nor ray query support.");

	RaytracingPipeline* raytracing_pipeline =
		data->raytracing_pipeline_owner.get_or_null(p_raytracing_pipeline);
	ERR_FAIL_NULL_V(raytracing_pipeline, RID());
	ERR_FAIL_COND_V(p_initial_hit_group_capacity == 0, RID());

	uint32_t shader_group_size =
		data->driver->api_trait_get(RDD::API_TRAIT_SHADER_GROUP_HANDLE_SIZE);

	HitShaderBindingTable hit_sbt;
	hit_sbt.size = p_initial_hit_group_capacity * shader_group_size;
	hit_sbt.driver_id = _hit_sbt_buffer_create(hit_sbt.size);
	ERR_FAIL_COND_V(!hit_sbt.driver_id, RID());

	hit_sbt.draw_tracker = RDG::resource_tracker_create();
	hit_sbt.draw_tracker->buffer_driver_id = hit_sbt.driver_id;

	hit_sbt.raytracing_pipeline_id = p_raytracing_pipeline;
	hit_sbt.raytracing_pipeline = raytracing_pipeline->driver_id;
	hit_sbt.index_offset =
		raytracing_pipeline->raygen_shader_count + raytracing_pipeline->miss_shader_count;
	hit_sbt.raytracing_pipeline_hit_group_count = raytracing_pipeline->hit_group_count;
	hit_sbt.hit_group_indices.resize_initialized(p_initial_hit_group_capacity);

	RID id = data->hit_sbt_owner.make_rid(hit_sbt);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	_add_dependency(id, p_raytracing_pipeline);

	return id;
}

Error RenderingDevice::hit_sbt_set_pipeline(RID p_hit_sbt, RID p_raytracing_pipeline)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	HitShaderBindingTable* hit_sbt = data->hit_sbt_owner.get_or_null(p_hit_sbt);
	ERR_FAIL_NULL_V(hit_sbt, ERR_INVALID_PARAMETER);

	if (hit_sbt->raytracing_pipeline_id != p_raytracing_pipeline) {
		RaytracingPipeline* raytracing_pipeline =
			data->raytracing_pipeline_owner.get_or_null(p_raytracing_pipeline);
		ERR_FAIL_NULL_V(raytracing_pipeline, ERR_INVALID_PARAMETER);
		ERR_FAIL_COND_V_MSG(
			raytracing_pipeline->hit_group_count < hit_sbt->raytracing_pipeline_hit_group_count,
			ERR_INVALID_PARAMETER, "The new pipeline must be a superset of the existing pipeline.");

		_remove_dependency(p_hit_sbt, hit_sbt->raytracing_pipeline_id);

		hit_sbt->raytracing_pipeline_id = p_raytracing_pipeline;
		hit_sbt->raytracing_pipeline = raytracing_pipeline->driver_id;
		hit_sbt->index_offset =
			raytracing_pipeline->raygen_shader_count + raytracing_pipeline->miss_shader_count;
		hit_sbt->raytracing_pipeline_hit_group_count = raytracing_pipeline->hit_group_count;

		_add_dependency(p_hit_sbt, p_raytracing_pipeline);

		_hit_sbt_add_dirty_range(hit_sbt, 0, hit_sbt->used_hit_group_count);
	}

	return OK;
}

RD::HitShaderBindingTableRange RenderingDevice::hit_sbt_range_alloc(
	RID p_hit_sbt, uint32_t p_hit_group_count)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	HitShaderBindingTable* hit_sbt = data->hit_sbt_owner.get_or_null(p_hit_sbt);
	ERR_FAIL_NULL_V(hit_sbt, RD::HitShaderBindingTableRange());
	ERR_FAIL_COND_V(p_hit_group_count == 0, RD::HitShaderBindingTableRange());

	uint32_t offset;

	HitShaderBindingTable::FreeList::Element* free_list_elem =
		hit_sbt->free_list.lower_bound(p_hit_group_count);
	if (free_list_elem) {
		uint32_t free_count = free_list_elem->key();
		RBSet<uint32_t>& free_offsets = free_list_elem->value();
		RBSet<uint32_t>::Element* free_offset_elem = free_offsets.front();
		DEV_ASSERT(free_offset_elem);

		offset = free_offset_elem->get();

		free_offsets.erase(free_offset_elem);
		if (free_offsets.is_empty()) {
			hit_sbt->free_list.erase(free_list_elem);
		}
		hit_sbt->reverse_free_list.erase(offset);

		uint32_t remaining = free_count - p_hit_group_count;
		if (remaining > 0) {
			uint32_t new_offset = offset + p_hit_group_count;
			hit_sbt->free_list[remaining].insert(new_offset);
			hit_sbt->reverse_free_list[new_offset] = remaining;
		}
	}
	else {
		offset = hit_sbt->used_hit_group_count;
		hit_sbt->used_hit_group_count += p_hit_group_count;

		if (hit_sbt->hit_group_indices.size() < hit_sbt->used_hit_group_count) {
			hit_sbt->hit_group_indices.resize_initialized(
				MAX(hit_sbt->hit_group_indices.size() * 2, hit_sbt->used_hit_group_count));
		}
	}

	_hit_sbt_add_dirty_range(hit_sbt, offset, p_hit_group_count);

	return _encode_hit_sbt_range(offset, p_hit_group_count);
}

Error RenderingDevice::hit_sbt_range_free(RID p_hit_sbt, HitShaderBindingTableRange p_range)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	HitShaderBindingTable* hit_sbt = data->hit_sbt_owner.get_or_null(p_hit_sbt);
	ERR_FAIL_NULL_V(hit_sbt, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(!p_range, ERR_INVALID_PARAMETER);

	uint32_t offset = _decode_hit_sbt_range_offset(p_range);
	uint32_t count = _decode_hit_sbt_range_count(p_range);

	ERR_FAIL_COND_V((offset + count) > hit_sbt->used_hit_group_count, ERR_INVALID_PARAMETER);

	HitShaderBindingTable::ReverseFreeList::Element* next_reverse_free_list_elem =
		hit_sbt->reverse_free_list.lower_bound(offset);
	if (next_reverse_free_list_elem) {
		HitShaderBindingTable::ReverseFreeList::Element* prev_reverse_free_list_elem =
			next_reverse_free_list_elem->prev();
		if (prev_reverse_free_list_elem) {
			uint32_t prev_free_offset = prev_reverse_free_list_elem->key();
			uint32_t prev_free_count = prev_reverse_free_list_elem->value();

			if ((prev_free_offset + prev_free_count) == offset) {
				offset = prev_free_offset;
				count += prev_free_count;

				HitShaderBindingTable::FreeList::Element* prev_free_list_elem =
					hit_sbt->free_list.find(prev_free_count);
				DEV_ASSERT(prev_free_list_elem);

				prev_free_list_elem->value().erase(prev_free_offset);
				if (prev_free_list_elem->value().is_empty()) {
					hit_sbt->free_list.erase(prev_free_list_elem);
				}

				hit_sbt->reverse_free_list.erase(prev_reverse_free_list_elem);
			}
		}

		uint32_t next_free_offset = next_reverse_free_list_elem->key();
		uint32_t next_free_count = next_reverse_free_list_elem->value();

		if ((offset + count) == next_free_offset) {
			count += next_free_count;

			HitShaderBindingTable::FreeList::Element* next_free_list_elem =
				hit_sbt->free_list.find(next_free_count);
			DEV_ASSERT(next_free_list_elem);

			next_free_list_elem->value().erase(next_free_offset);
			if (next_free_list_elem->value().is_empty()) {
				hit_sbt->free_list.erase(next_free_list_elem);
			}

			hit_sbt->reverse_free_list.erase(next_reverse_free_list_elem);
		}
	}

	if ((offset + count) == hit_sbt->used_hit_group_count) {
		hit_sbt->used_hit_group_count = offset;
	}
	else {
		hit_sbt->free_list[count].insert(offset);
		hit_sbt->reverse_free_list[offset] = count;
	}

	return OK;
}

Error RenderingDevice::hit_sbt_range_update(RID p_hit_sbt, HitShaderBindingTableRange p_range,
	uint32_t p_offset, Span<uint32_t> p_hit_group_indices)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	HitShaderBindingTable* hit_sbt = data->hit_sbt_owner.get_or_null(p_hit_sbt);
	ERR_FAIL_NULL_V(hit_sbt, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(!p_range, ERR_INVALID_PARAMETER);

	uint32_t offset = _decode_hit_sbt_range_offset(p_range);
	uint32_t count = _decode_hit_sbt_range_count(p_range);

	ERR_FAIL_COND_V((offset + count) > hit_sbt->used_hit_group_count, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V((p_offset + p_hit_group_indices.size()) > count, ERR_INVALID_PARAMETER);

#ifdef DEBUG_ENABLED
	for (uint32_t i = 0; i < p_hit_group_indices.size(); i++) {
		ERR_FAIL_COND_V(p_hit_group_indices[i] >= hit_sbt->raytracing_pipeline_hit_group_count,
			ERR_INVALID_PARAMETER);
	}
#endif

	memcpy(hit_sbt->hit_group_indices.ptrw() + (offset + p_offset), p_hit_group_indices.ptr(),
		sizeof(uint32_t) * p_hit_group_indices.size());

	_hit_sbt_add_dirty_range(hit_sbt, offset + p_offset, p_hit_group_indices.size());

	return OK;
}

RenderingDevice::Buffer* RenderingDevice::_get_buffer_from_owner(RID p_buffer)
{
	Buffer* buffer = nullptr;
	if (data->vertex_buffer_owner.owns(p_buffer)) {
		buffer = data->vertex_buffer_owner.get_or_null(p_buffer);
	}
	else if (data->index_buffer_owner.owns(p_buffer)) {
		buffer = data->index_buffer_owner.get_or_null(p_buffer);
	}
	else if (data->uniform_buffer_owner.owns(p_buffer)) {
		buffer = data->uniform_buffer_owner.get_or_null(p_buffer);
	}
	else if (data->texture_buffer_owner.owns(p_buffer)) {
		buffer = data->texture_buffer_owner.get_or_null(p_buffer);
	}
	else if (data->storage_buffer_owner.owns(p_buffer)) {
		buffer = data->storage_buffer_owner.get_or_null(p_buffer);
	}
	return buffer;
}

Error RenderingDevice::_buffer_initialize(
	Buffer* p_buffer, Span<uint8_t> p_data, uint32_t p_required_align)
{
	uint32_t transfer_worker_offset;
	TransferWorker* transfer_worker =
		_acquire_transfer_worker(p_data.size(), p_required_align, transfer_worker_offset);
	p_buffer->transfer_worker_index = transfer_worker->index;

	{
		MutexLock lock(transfer_worker->operations_mutex);
		p_buffer->transfer_worker_operation = ++transfer_worker->operations_counter;
	}

	// Copy to the worker's staging buffer.
	uint8_t* data_ptr = data->driver->buffer_map(transfer_worker->staging_buffer);
	ERR_FAIL_NULL_V(data_ptr, ERR_CANT_CREATE);

	memcpy(data_ptr + transfer_worker_offset, p_data.ptr(), p_data.size());
	data->driver->buffer_unmap(transfer_worker->staging_buffer);

	// Copy from the staging buffer to the real buffer.
	RDD::BufferCopyRegion region;
	region.src_offset = transfer_worker_offset;
	region.dst_offset = 0;
	region.size = p_data.size();
	data->driver->command_copy_buffer(transfer_worker->command_buffer,
		transfer_worker->staging_buffer, p_buffer->driver_id, region);

	_release_transfer_worker(transfer_worker);

	return OK;
}

Error RenderingDevice::_insert_staging_block(StagingBuffers& p_staging_buffers)
{
	StagingBufferBlock block;

	block.driver_id = data->driver->buffer_create(p_staging_buffers.block_size,
		p_staging_buffers.usage_bits, RDD::MEMORY_ALLOCATION_TYPE_CPU, data->frames_drawn);
	ERR_FAIL_COND_V(!block.driver_id, ERR_CANT_CREATE);

	block.frame_used = 0;
	block.fill_amount = 0;
	block.data_ptr = data->driver->buffer_map(block.driver_id);

	if (block.data_ptr == nullptr) {
		data->driver->buffer_free(block.driver_id);
		return ERR_CANT_CREATE;
	}

	p_staging_buffers.blocks.insert(p_staging_buffers.current, block);
	return OK;
}

Error RenderingDevice::_staging_buffer_allocate(StagingBuffers& p_staging_buffers,
	uint32_t p_amount, uint32_t p_required_align, uint32_t& r_alloc_offset, uint32_t& r_alloc_size,
	StagingRequiredAction& r_required_action, bool p_can_segment)
{
	// Determine a block to use.

	r_alloc_size = p_amount;
	r_required_action = STAGING_REQUIRED_ACTION_NONE;

	while (true) {
		r_alloc_offset = 0;

		// See if we can use current block.
		if (p_staging_buffers.blocks[p_staging_buffers.current].frame_used == data->frames_drawn) {
			// We used this block this frame, let's see if there is still room.

			uint32_t write_from = p_staging_buffers.blocks[p_staging_buffers.current].fill_amount;

			{
				uint32_t align_remainder = write_from % p_required_align;
				if (align_remainder != 0) {
					write_from += p_required_align - align_remainder;
				}
			}

			int32_t available_bytes = int32_t(p_staging_buffers.block_size) - int32_t(write_from);

			if ((int32_t)p_amount < available_bytes) {
				// All is good, we should be ok, all will fit.
				r_alloc_offset = write_from;
			}
			else if (p_can_segment && available_bytes >= (int32_t)p_required_align) {
				// Ok all won't fit but at least we can fit a chunkie.
				// All is good, update what needs to be written to.
				r_alloc_offset = write_from;
				r_alloc_size = available_bytes - (available_bytes % p_required_align);

			}
			else {
				// Can't fit it into this buffer.
				// Will need to try next buffer.

				p_staging_buffers.current =
					(p_staging_buffers.current + 1) % p_staging_buffers.blocks.size();

				// Before doing anything, though, let's check that we didn't manage to fill all
				// blocks. Possible in a single frame.
				if (p_staging_buffers.blocks[p_staging_buffers.current].frame_used ==
					data->frames_drawn) {
					// Guess we did.. ok, let's see if we can insert a new block.
					if ((uint64_t)p_staging_buffers.blocks.size() * p_staging_buffers.block_size <
						p_staging_buffers.max_size) {
						// We can, so we are safe.
						Error err = _insert_staging_block(p_staging_buffers);
						if (err) {
							return err;
						}
						// Claim for this frame.
						p_staging_buffers.blocks.write[p_staging_buffers.current].frame_used =
							data->frames_drawn;
					}
					else {
						// Ok, worst case scenario, all the staging buffers belong to this frame
						// and this frame is not even done.
						// If this is the main thread, it means the user is likely loading a lot of
						// resources at once,. Otherwise, the thread should just be blocked until
						// the next frame (currently unimplemented).
						r_required_action = STAGING_REQUIRED_ACTION_FLUSH_AND_STALL_ALL;
					}

				}
				else {
					// Not from current frame, so continue and try again.
					continue;
				}
			}

		}
		else if (p_staging_buffers.blocks[p_staging_buffers.current].frame_used <=
				   data->frames_drawn - data->frames.size()) {
			// This is an old block, which was already processed, let's reuse.
			p_staging_buffers.blocks.write[p_staging_buffers.current].frame_used =
				data->frames_drawn;
			p_staging_buffers.blocks.write[p_staging_buffers.current].fill_amount = 0;
		}
		else {
			// This block may still be in use, let's not touch it unless we have to, so.. can we
			// create a new one?
			if ((uint64_t)p_staging_buffers.blocks.size() * p_staging_buffers.block_size <
				p_staging_buffers.max_size) {
				// We are still allowed to create a new block, so let's do that and insert it for
				// current pos.
				Error err = _insert_staging_block(p_staging_buffers);
				if (err) {
					return err;
				}
				// Claim for this frame.
				p_staging_buffers.blocks.write[p_staging_buffers.current].frame_used =
					data->frames_drawn;
			}
			else {
				// Oops, we are out of room and we can't create more.
				// Let's flush older frames.
				// The logic here is that if a game is loading a lot of data from the main thread,
				// it will need to be stalled anyway. If loading from a separate thread, we can
				// block that thread until next frame when more room is made (not currently
				// implemented, though).
				r_required_action = STAGING_REQUIRED_ACTION_STALL_PREVIOUS;
			}
		}

		// All was good, break.
		break;
	}

	p_staging_buffers.used = true;

	return OK;
}

void RenderingDevice::_staging_buffer_execute_required_action(
	StagingBuffers& p_staging_buffers, StagingRequiredAction p_required_action)
{
	switch (p_required_action) {
	case STAGING_REQUIRED_ACTION_FLUSH_AND_STALL_ALL: {
		_flush_and_stall_for_all_frames();
		p_staging_buffers.blocks.write[p_staging_buffers.current].frame_used = data->frames_drawn;
		p_staging_buffers.blocks.write[p_staging_buffers.current].fill_amount = 0;
	} break;
	case STAGING_REQUIRED_ACTION_STALL_PREVIOUS: {
		_stall_for_previous_frames();
		p_staging_buffers.blocks.write[p_staging_buffers.current].frame_used = data->frames_drawn;
		p_staging_buffers.blocks.write[p_staging_buffers.current].fill_amount = 0;
	} break;
	case STAGING_REQUIRED_ACTION_NONE: {
		// Do nothing.
	} break;
	default: {
		DEV_ASSERT(false && "Unknown required action.");
	} break;
	}
}

Error RenderingDevice::_buffer_update(
	Buffer* p_buffer, RID p_buffer_id, uint32_t p_offset, uint32_t p_size, const void* p_data)
{
	data->copy_bytes_count += p_size;

	if (p_buffer->usage & RDD::BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT) {
		uint8_t* dst_data = data->driver->buffer_persistent_map_advance(
			p_buffer->driver_id, data->frames_drawn);

		memcpy(dst_data + p_offset, p_data, p_size);
		data->direct_copy_count++;
		buffer_flush(p_buffer_id);
		return OK;
	}

	_check_transfer_worker_buffer(p_buffer);

	size_t to_submit = p_size;
	size_t submit_from = 0;

	thread_local LocalVector<RDG::RecordedBufferCopy> command_buffer_copies_vector;
	command_buffer_copies_vector.clear();

	const uint8_t* src_data = reinterpret_cast<const uint8_t*>(p_data);
	const uint32_t required_align = 32;
	while (to_submit > 0) {
		uint32_t block_write_offset;
		uint32_t block_write_amount;
		StagingRequiredAction required_action;

		Error err = _staging_buffer_allocate(data->upload_staging_buffers,
			MIN(to_submit, data->upload_staging_buffers.block_size), required_align,
			block_write_offset, block_write_amount, required_action);
		if (err) {
			return err;
		}

		if (!command_buffer_copies_vector.is_empty() &&
			required_action == STAGING_REQUIRED_ACTION_FLUSH_AND_STALL_ALL) {
			if (_buffer_make_mutable(p_buffer, p_buffer_id)) {
				data->draw_graph.add_synchronization();
			}

			data->draw_graph.add_buffer_update(
				p_buffer->driver_id, p_buffer->draw_tracker, command_buffer_copies_vector);
			command_buffer_copies_vector.clear();
		}

		_staging_buffer_execute_required_action(data->upload_staging_buffers, required_action);

		memcpy(data->upload_staging_buffers.blocks[data->upload_staging_buffers.current].data_ptr +
				   block_write_offset,
			src_data + submit_from, block_write_amount);

		RDD::BufferCopyRegion region;
		region.src_offset = block_write_offset;
		region.dst_offset = submit_from + p_offset;
		region.size = block_write_amount;

		RDG::RecordedBufferCopy buffer_copy;
		buffer_copy.source =
			data->upload_staging_buffers.blocks[data->upload_staging_buffers.current].driver_id;
		buffer_copy.region = region;
		command_buffer_copies_vector.push_back(buffer_copy);

		data->upload_staging_buffers.blocks.write[data->upload_staging_buffers.current]
			.fill_amount = block_write_offset + block_write_amount;

		to_submit -= block_write_amount;
		submit_from += block_write_amount;
	}

	if (!command_buffer_copies_vector.is_empty()) {
		if (_buffer_make_mutable(p_buffer, p_buffer_id)) {
			data->draw_graph.add_synchronization();
		}

		data->draw_graph.add_buffer_update(
			p_buffer->driver_id, p_buffer->draw_tracker, command_buffer_copies_vector);
	}

	data->gpu_copy_count++;

	return OK;
}

Error RenderingDevice::buffer_update(
	RID p_buffer, uint32_t p_offset, uint32_t p_size, const void* p_data, bool p_skip_check)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	ERR_FAIL_COND_V_MSG(data->draw_list.active && !p_skip_check, ERR_INVALID_PARAMETER,
		"Updating buffers is forbidden during creation of a draw list.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active && !p_skip_check, ERR_INVALID_PARAMETER,
		"Updating buffers is forbidden during creation of a compute list.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active && !p_skip_check, ERR_INVALID_PARAMETER,
		"Updating buffers is forbidden during creation of a raytracing list.");

	Buffer* buffer = _get_buffer_from_owner(p_buffer);
	ERR_FAIL_NULL_V_MSG(
		buffer, ERR_INVALID_PARAMETER, "Buffer argument is not a valid buffer of any type.");
	ERR_FAIL_COND_V_MSG(p_offset + p_size > buffer->size, ERR_INVALID_PARAMETER,
		"Attempted to write buffer (" + itos((p_offset + p_size) - buffer->size) +
			" bytes) past the end.");

	return _buffer_update(buffer, p_buffer, p_offset, p_size, p_data);
}

Error RenderingDevice::buffer_copy(RID p_src_buffer, RID p_dst_buffer, uint32_t p_src_offset,
	uint32_t p_dst_offset, uint32_t p_size)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	ERR_FAIL_COND_V_MSG(data->draw_list.active, ERR_INVALID_PARAMETER,
		"Copying buffers is forbidden during creation of a draw list.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, ERR_INVALID_PARAMETER,
		"Copying buffers is forbidden during creation of a compute list.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, ERR_INVALID_PARAMETER,
		"Copying buffers is forbidden during creation of a raytracing list.");

	Buffer* src_buffer = _get_buffer_from_owner(p_src_buffer);
	if (!src_buffer) {
		ERR_FAIL_V_MSG(
			ERR_INVALID_PARAMETER, "Source buffer argument is not a valid buffer of any type.");
	}

	Buffer* dst_buffer = _get_buffer_from_owner(p_dst_buffer);
	if (!dst_buffer) {
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER,
			"Destination buffer argument is not a valid buffer of any type.");
	}

	// Validate the copy's dimensions for both buffers.
	ERR_FAIL_COND_V_MSG((p_size + p_src_offset) > src_buffer->size, ERR_INVALID_PARAMETER,
		"Size is larger than the source buffer.");
	ERR_FAIL_COND_V_MSG((p_size + p_dst_offset) > dst_buffer->size, ERR_INVALID_PARAMETER,
		"Size is larger than the destination buffer.");

	_check_transfer_worker_buffer(src_buffer);
	_check_transfer_worker_buffer(dst_buffer);

	// Perform the copy.
	RDD::BufferCopyRegion region;
	region.src_offset = p_src_offset;
	region.dst_offset = p_dst_offset;
	region.size = p_size;

	if (_buffer_make_mutable(dst_buffer, p_dst_buffer)) {
		// The destination buffer must be mutable to be used as a copy destination.
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_buffer_copy(src_buffer->driver_id, src_buffer->draw_tracker,
		dst_buffer->driver_id, dst_buffer->draw_tracker, region);

	return OK;
}

Error RenderingDevice::driver_callback_add(
	RDD::DriverCallback p_callback, void* p_userdata, VectorView<CallbackResource> p_resources)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	ERR_FAIL_COND_V_MSG(data->draw_list.active, ERR_INVALID_PARAMETER,
		"Driver callback is forbidden during creation of a draw list.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, ERR_INVALID_PARAMETER,
		"Driver callback is forbidden during creation of a compute list.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, ERR_INVALID_PARAMETER,
		"Driver callback is forbidden during creation of a raytracing list.");

	thread_local LocalVector<RDG::ResourceTracker*> trackers;
	thread_local LocalVector<RDG::ResourceUsage> usages;

	uint32_t resource_count = p_resources.size();
	trackers.resize(resource_count);
	usages.resize(resource_count);

	if (resource_count > 0) {
		for (uint32_t i = 0; i < p_resources.size(); i++) {
			const CallbackResource& cr = p_resources[i];
			switch (cr.type) {
			case CALLBACK_RESOURCE_TYPE_BUFFER: {
				Buffer* buffer = _get_buffer_from_owner(cr.rid);
				if (!buffer) {
					ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER,
						vformat("Argument %d is not a valid buffer of any type.", i));
				}
				if (_buffer_make_mutable(buffer, cr.rid)) {
					data->draw_graph.add_synchronization();
				}
				trackers[i] = buffer->draw_tracker;
				usages[i] = (RDG::ResourceUsage)cr.usage;
			} break;
			case CALLBACK_RESOURCE_TYPE_TEXTURE: {
				Texture* texture = data->texture_owner.get_or_null(cr.rid);
				if (!texture) {
					ERR_FAIL_V_MSG(
						ERR_INVALID_PARAMETER, vformat("Argument %d is not a valid texture.", i));
				}
				if (_texture_make_mutable(texture, cr.rid)) {
					data->draw_graph.add_synchronization();
				}
				trackers[i] = texture->draw_tracker;
				usages[i] = (RDG::ResourceUsage)cr.usage;
			} break;
			default: {
				CRASH_NOW_MSG("Invalid callback resource type.");
			} break;
			}
		}
	}

	data->draw_graph.add_driver_callback(p_callback, p_userdata, trackers, usages);

	return OK;
}

RID RenderingDevice::uniform_buffer_create(
	uint32_t p_size_bytes, Span<uint8_t> p_data, uint32_t p_creation_bits)
{
	ERR_FAIL_COND_V(p_data.size() && (uint32_t)p_data.size() != p_size_bytes, RID());

	Buffer buffer;
	buffer.size = p_size_bytes;
	buffer.usage = (RDD::BUFFER_USAGE_TRANSFER_TO_BIT | RDD::BUFFER_USAGE_UNIFORM_BIT);
	if (p_creation_bits & BUFFER_CREATION_DEVICE_ADDRESS_BIT) {
		buffer.usage |= RDD::BUFFER_USAGE_DEVICE_ADDRESS_BIT;
	}
	if (p_creation_bits & BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT) {
		buffer.usage |= RDD::BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT;
		buffer.usage &= ~uint32_t(RDD::BUFFER_USAGE_TRANSFER_TO_BIT);
	}
	buffer.driver_id = data->driver->buffer_create(
		buffer.size, buffer.usage, RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	ERR_FAIL_COND_V(!buffer.driver_id, RID());

	if (p_data.is_empty()) {
		buffer.draw_tracker = RDG::resource_tracker_create();
		buffer.draw_tracker->buffer_driver_id = buffer.driver_id;
	}

	if (p_data.size()) {
		_buffer_initialize(&buffer, p_data);
	}

	data->buffer_memory.add(buffer.size);

	RID id = data->uniform_buffer_owner.make_rid(buffer);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	return id;
}

RID RenderingDevice::storage_buffer_create(
	uint32_t p_size_bytes, Span<uint8_t> p_data, uint32_t p_creation_bits)
{
	ERR_FAIL_COND_V(p_data.size() && (uint32_t)p_data.size() != p_size_bytes, RID());

	Buffer buffer;
	buffer.size = p_size_bytes;
	buffer.usage = (RDD::BUFFER_USAGE_TRANSFER_FROM_BIT | RDD::BUFFER_USAGE_TRANSFER_TO_BIT |
					RDD::BUFFER_USAGE_STORAGE_BIT);
	if (p_creation_bits & BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT) {
		buffer.usage |= RDD::BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT;
		buffer.usage &= ~uint32_t(RDD::BUFFER_USAGE_TRANSFER_TO_BIT);
	}
	if (p_creation_bits & BUFFER_CREATION_DEVICE_ADDRESS_BIT) {
		buffer.usage |= RDD::BUFFER_USAGE_DEVICE_ADDRESS_BIT;
	}
	if (p_creation_bits & BUFFER_CREATION_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT) {
		buffer.usage |= RDD::BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT;
	}

	buffer.driver_id = data->driver->buffer_create(
		buffer.size, buffer.usage, RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	ERR_FAIL_COND_V(!buffer.driver_id, RID());

	buffer.draw_tracker = RDG::resource_tracker_create();
	buffer.draw_tracker->buffer_driver_id = buffer.driver_id;

	if (p_data.size()) {
		_buffer_initialize(&buffer, p_data);
	}

	data->buffer_memory.add(buffer.size);

	RID id = data->storage_buffer_owner.make_rid(buffer);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	return id;
}

String RenderingDevice::get_perf_report()
{
	String perf_report_text;
	perf_report_text += " gpu:" + String::num_int64(data->gpu_copy_count);
	perf_report_text += " direct:" + String::num_int64(data->direct_copy_count);
	perf_report_text += " bytes:" + String::num_int64(data->copy_bytes_count);

	perf_report_text +=
		" lazily alloc:" + String::num_int64(data->driver->get_lazily_memory_used());
	return perf_report_text;
}

void RenderingDevice::update_perf_report()
{
	data->prev_gpu_copy_count = data->gpu_copy_count;
	data->prev_copy_bytes_count = data->copy_bytes_count;
	data->gpu_copy_count = 0;
	data->direct_copy_count = 0;
	data->copy_bytes_count = 0;
}

Vector<uint8_t> RenderingDevice::buffer_get_data(RID p_buffer, uint32_t p_offset, uint32_t p_size)
{
	ERR_RENDER_THREAD_GUARD_V(Vector<uint8_t>());

	Buffer* buffer = _get_buffer_from_owner(p_buffer);
	if (!buffer) {
		ERR_FAIL_V_MSG(Vector<uint8_t>(),
			"Buffer is either invalid or this type of buffer can't be retrieved.");
	}

	if (!p_size) {
		p_size = buffer->size;
	}
	else {
		ERR_FAIL_COND_V_MSG(
			p_size + p_offset > buffer->size, Vector<uint8_t>(), "Size is larger than the buffer.");
	}

	_check_transfer_worker_buffer(buffer);

	RDD::BufferID tmp_buffer = data->driver->buffer_create(buffer->size,
		RDD::BUFFER_USAGE_TRANSFER_TO_BIT, RDD::MEMORY_ALLOCATION_TYPE_CPU, data->frames_drawn);
	ERR_FAIL_COND_V(!tmp_buffer, Vector<uint8_t>());

	RDD::BufferCopyRegion region;
	region.src_offset = p_offset;
	region.size = p_size;

	data->draw_graph.add_buffer_get_data(
		buffer->driver_id, buffer->draw_tracker, tmp_buffer, region);

	_flush_and_stall_for_all_frames();

	uint8_t* buffer_mem = data->driver->buffer_map(tmp_buffer);
	ERR_FAIL_NULL_V(buffer_mem, Vector<uint8_t>());

	Vector<uint8_t> buffer_data;
	{
		buffer_data.resize(p_size);
		uint8_t* w = buffer_data.ptrw();
		memcpy(w, buffer_mem, p_size);
	}

	data->driver->buffer_unmap(tmp_buffer);
	data->driver->buffer_free(tmp_buffer);

	return buffer_data;
}

Error RenderingDevice::buffer_clear(RID p_buffer, uint32_t p_offset, uint32_t p_size)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	ERR_FAIL_COND_V_MSG(
		(p_size % 4) != 0, ERR_INVALID_PARAMETER, "Size must be a multiple of four.");
	ERR_FAIL_COND_V_MSG(data->draw_list.active, ERR_INVALID_PARAMETER,
		"Updating buffers in is forbidden during creation of a draw list.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, ERR_INVALID_PARAMETER,
		"Updating buffers is forbidden during creation of a compute list.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, ERR_INVALID_PARAMETER,
		"Updating buffers is forbidden during creation of a raytracing list.");

	Buffer* buffer = _get_buffer_from_owner(p_buffer);
	if (!buffer) {
		ERR_FAIL_V_MSG(ERR_INVALID_PARAMETER, "Buffer argument is not a valid buffer of any type.");
	}

	ERR_FAIL_COND_V_MSG(p_offset + p_size > buffer->size, ERR_INVALID_PARAMETER,
		"Attempted to write buffer (" + itos((p_offset + p_size) - buffer->size) +
			" bytes) past the end.");

	_check_transfer_worker_buffer(buffer);

	if (_buffer_make_mutable(buffer, p_buffer)) {
		// The destination buffer must be mutable to be used as a clear destination.
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_buffer_clear(buffer->driver_id, buffer->draw_tracker, p_offset, p_size);

	return OK;
}

uint8_t* RenderingDevice::buffer_persistent_map_advance(RID p_buffer)
{
	ERR_RENDER_THREAD_GUARD_V(0);

	Buffer* buffer = _get_buffer_from_owner(p_buffer);
	ERR_FAIL_NULL_V_MSG(buffer, nullptr, "Buffer argument is not a valid buffer of any type.");
	data->direct_copy_count++;
	return data->driver->buffer_persistent_map_advance(buffer->driver_id, data->frames_drawn);
}

void RenderingDevice::buffer_flush(RID p_buffer)
{
	ERR_RENDER_THREAD_GUARD();

	Buffer* buffer = _get_buffer_from_owner(p_buffer);
	ERR_FAIL_NULL_MSG(buffer, "Buffer argument is not a valid buffer of any type.");
	data->driver->buffer_flush(buffer->driver_id);
}

RID RenderingDevice::texture_buffer_create(
	uint32_t p_size_elements, RDC::DataFormat p_format, Span<uint8_t> p_data)
{
	uint32_t element_size = RDC::get_format_vertex_size(p_format);
	ERR_FAIL_COND_V_MSG(
		element_size == 0, RID(), "Format requested is not supported for texture buffers");
	uint64_t size_bytes = uint64_t(element_size) * p_size_elements;

	ERR_FAIL_COND_V(p_data.size() && (uint32_t)p_data.size() != size_bytes, RID());

	Buffer texture_buffer;
	texture_buffer.size = size_bytes;
	uint32_t usage = (RDD::BUFFER_USAGE_TRANSFER_FROM_BIT | RDD::BUFFER_USAGE_TRANSFER_TO_BIT |
					  RDD::BUFFER_USAGE_TEXEL_BIT);
	texture_buffer.driver_id = data->driver->buffer_create(
		size_bytes, usage, RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	ERR_FAIL_COND_V(!texture_buffer.driver_id, RID());

	// Texture buffers are assumed to be immutable unless they don't have initial data.
	if (p_data.is_empty()) {
		texture_buffer.draw_tracker = RDG::resource_tracker_create();
		texture_buffer.draw_tracker->buffer_driver_id = texture_buffer.driver_id;
	}

	bool ok = data->driver->buffer_set_texel_format(texture_buffer.driver_id, p_format);
	if (!ok) {
		data->driver->buffer_free(texture_buffer.driver_id);
		ERR_FAIL_V(RID());
	}

	if (p_data.size()) {
		_buffer_initialize(&texture_buffer, p_data);
	}

	data->buffer_memory.add(size_bytes);

	RID id = data->texture_buffer_owner.make_rid(texture_buffer);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	return id;
}

static _ALWAYS_INLINE_ void _copy_region(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_x, uint32_t p_src_y, uint32_t p_src_w, uint32_t p_src_h, uint32_t p_src_full_w,
	uint32_t p_dst_pitch, uint32_t p_unit_size)
{
	uint32_t src_offset = (p_src_y * p_src_full_w + p_src_x) * p_unit_size;
	uint32_t dst_offset = 0;
	for (uint32_t y = p_src_h; y > 0; y--) {
		const uint8_t* __restrict src = p_src + src_offset;
		uint8_t* __restrict dst = p_dst + dst_offset;
		for (uint32_t x = p_src_w * p_unit_size; x > 0; x--) {
			*dst = *src;
			src++;
			dst++;
		}
		src_offset += p_src_full_w * p_unit_size;
		dst_offset += p_dst_pitch;
	}
}

static _ALWAYS_INLINE_ void _copy_region_block_or_regular(const uint8_t* p_read_ptr,
	uint8_t* p_write_ptr, uint32_t p_x, uint32_t p_y, uint32_t p_width, uint32_t p_region_w,
	uint32_t p_region_h, uint32_t p_block_w, uint32_t p_block_h, uint32_t p_dst_pitch,
	uint32_t p_pixel_size, uint32_t p_block_size)
{
	if (p_block_w != 1 || p_block_h != 1) {
		// Block format.
		uint32_t xb = p_x / p_block_w;
		uint32_t yb = p_y / p_block_h;
		uint32_t wb = p_width / p_block_w;
		uint32_t region_wb = p_region_w / p_block_w;
		uint32_t region_hb = p_region_h / p_block_h;
		_copy_region(
			p_read_ptr, p_write_ptr, xb, yb, region_wb, region_hb, wb, p_dst_pitch, p_block_size);
	}
	else {
		// Regular format.
		_copy_region(p_read_ptr, p_write_ptr, p_x, p_y, p_region_w, p_region_h, p_width,
			p_dst_pitch, p_pixel_size);
	}
}

Error RenderingDevice::_texture_initialize(RID p_texture, uint32_t p_layer,
	const Vector<uint8_t>& p_data, RDD::TextureLayout p_dst_layout, bool p_immediate_flush)
{
	Texture* texture = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, ERR_INVALID_PARAMETER);

	if (texture->owner != RID()) {
		p_texture = texture->owner;
		texture = data->texture_owner.get_or_null(texture->owner);
		ERR_FAIL_NULL_V(texture, ERR_BUG);
	}

	uint32_t layer_count = _texture_layer_count(texture);
	ERR_FAIL_COND_V(p_layer >= layer_count, ERR_INVALID_PARAMETER);

	uint32_t width, height;
	uint32_t tight_mip_size = RDC::get_image_format_required_size(texture->format, texture->width,
		texture->height, texture->depth, texture->mipmaps, &width, &height);
	uint32_t required_size = tight_mip_size;
	uint32_t required_align = _texture_alignment(texture);

	ERR_FAIL_COND_V_MSG(required_size != (uint32_t)p_data.size(), ERR_INVALID_PARAMETER,
		"Required size for texture update (" + itos(required_size) +
			") does not match data supplied size (" + itos(p_data.size()) + ").");

	uint32_t block_w, block_h;
	RDC::get_compressed_image_format_block_dimensions(texture->format, block_w, block_h);

	uint32_t pixel_size = RDC::get_image_format_pixel_size(texture->format);
	uint32_t block_size = RDC::get_compressed_image_format_block_byte_size(texture->format);

	uint32_t staging_worker_offset = 0;
	uint32_t staging_local_offset = 0;
	TransferWorker* transfer_worker = nullptr;
	const uint8_t* read_ptr = p_data.ptr();
	uint8_t* write_ptr = nullptr;
	for (uint32_t pass = 0; pass < 2; pass++) {
		const bool copy_pass = (pass == 1);
		if (copy_pass) {
			transfer_worker = _acquire_transfer_worker(
				staging_local_offset, required_align, staging_worker_offset);
			texture->transfer_worker_index = transfer_worker->index;

			{
				MutexLock lock(transfer_worker->operations_mutex);
				texture->transfer_worker_operation = ++transfer_worker->operations_counter;
			}

			staging_local_offset = 0;

			write_ptr = data->driver->buffer_map(transfer_worker->staging_buffer);
			ERR_FAIL_NULL_V(write_ptr, ERR_CANT_CREATE);

			if (data->driver->api_trait_get(RDD::API_TRAIT_HONORS_PIPELINE_BARRIERS)) {
				RDD::TextureBarrier tb;
				tb.texture = texture->driver_id;
				tb.dst_access = RDD::BARRIER_ACCESS_COPY_WRITE_BIT;
				tb.prev_layout = RDD::TEXTURE_LAYOUT_UNDEFINED;
				tb.next_layout = p_dst_layout;
				tb.subresources.aspect = texture->barrier_aspect_flags;
				tb.subresources.mipmap_count = texture->mipmaps;
				tb.subresources.base_layer = p_layer;
				tb.subresources.layer_count = 1;
				data->driver->command_pipeline_barrier(transfer_worker->command_buffer,
					RDD::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, RDD::PIPELINE_STAGE_COPY_BIT, {}, {},
					tb, {});
			}
		}

		uint32_t mipmap_offset = 0;
		uint32_t logic_width = texture->width;
		uint32_t logic_height = texture->height;
		for (uint32_t mm_i = 0; mm_i < texture->mipmaps; mm_i++) {
			uint32_t depth = 0;
			uint32_t image_total = RDC::get_image_format_required_size(texture->format,
				texture->width, texture->height, texture->depth, mm_i + 1, &width, &height, &depth);

			const uint8_t* read_ptr_mipmap = read_ptr + mipmap_offset;
			tight_mip_size = image_total - mipmap_offset;

			for (uint32_t z = 0; z < depth; z++) {
				if (required_align > 0) {
					uint32_t align_offset = staging_local_offset % required_align;
					if (align_offset != 0) {
						staging_local_offset += required_align - align_offset;
					}
				}

				uint32_t pitch = RDC::get_compressed_image_format_pixels_shifted(
					texture->format, width * pixel_size * block_w);
				uint32_t pitch_step =
					data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP);
				pitch = STEPIFY(pitch, pitch_step);
				uint32_t to_allocate = RDC::get_compressed_image_format_pixels_shifted(
					texture->format, pitch * height);

				if (copy_pass) {
					const uint8_t* read_ptr_mipmap_layer =
						read_ptr_mipmap + (tight_mip_size / depth) * z;
					uint64_t staging_buffer_offset = staging_worker_offset + staging_local_offset;
					uint8_t* write_ptr_mipmap_layer = write_ptr + staging_buffer_offset;
					_copy_region_block_or_regular(read_ptr_mipmap_layer, write_ptr_mipmap_layer, 0,
						0, width, width, height, block_w, block_h, pitch, pixel_size, block_size);

					RDD::BufferTextureCopyRegion copy_region;
					copy_region.buffer_offset = staging_buffer_offset;
					copy_region.row_pitch = pitch;
					copy_region.texture_subresource.aspect =
						(texture->read_aspect_flags & RDD::TEXTURE_ASPECT_DEPTH_BIT)
							? RDD::TEXTURE_ASPECT_DEPTH
							: RDD::TEXTURE_ASPECT_COLOR;
					copy_region.texture_subresource.mipmap = mm_i;
					copy_region.texture_subresource.layer = p_layer;
					copy_region.texture_offset = Vector3i(0, 0, z);
					copy_region.texture_region_size = Vector3i(logic_width, logic_height, 1);
					data->driver->command_copy_buffer_to_texture(transfer_worker->command_buffer,
						transfer_worker->staging_buffer, texture->driver_id, p_dst_layout,
						copy_region);
				}

				staging_local_offset += to_allocate;
			}

			mipmap_offset = image_total;
			logic_width = MAX(1u, logic_width >> 1);
			logic_height = MAX(1u, logic_height >> 1);
		}

		if (copy_pass) {
			data->driver->buffer_unmap(transfer_worker->staging_buffer);

			if (texture->draw_tracker == nullptr &&
				data->driver->api_trait_get(RDD::API_TRAIT_HONORS_PIPELINE_BARRIERS)) {
				RDD::TextureBarrier tb;
				tb.texture = texture->driver_id;
				tb.src_access = RDD::BARRIER_ACCESS_COPY_WRITE_BIT;
				tb.prev_layout = p_dst_layout;
				tb.next_layout = RDD::TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				tb.subresources.aspect = texture->barrier_aspect_flags;
				tb.subresources.mipmap_count = texture->mipmaps;
				tb.subresources.base_layer = p_layer;
				tb.subresources.layer_count = 1;
				transfer_worker->texture_barriers.push_back(tb);
			}

			if (p_immediate_flush) {
				_end_transfer_worker(transfer_worker);
				_submit_transfer_worker(transfer_worker);
				_wait_for_transfer_worker(transfer_worker);
			}

			_release_transfer_worker(transfer_worker);
		}
	}

	return OK;
}

RID RenderingDevice::texture_create(const RDC::TextureFormat& p_format, const TextureView& p_view,
	const Vector<Vector<uint8_t>>& p_data)
{
	ERR_RENDER_THREAD_GUARD_V(RID());

	RDC::TextureFormat format = p_format;

	if (format.shareable_formats.size()) {
		ERR_FAIL_COND_V_MSG(!format.shareable_formats.has(format.format), RID(),
			"If supplied a list of shareable formats, the current format must be present in the "
			"list");
		ERR_FAIL_COND_V_MSG(p_view.format_override != RDC::DATA_FORMAT_MAX &&
								!format.shareable_formats.has(p_view.format_override),
			RID(),
			"If supplied a list of shareable formats, the current view format override must be "
			"present in the list");
	}

	ERR_FAIL_INDEX_V(format.texture_type, RDC::TEXTURE_TYPE_MAX, RID());
	ERR_FAIL_COND_V_MSG(
		format.width < 1, RID(), "Width must be equal or greater than 1 for all textures");

	if (format.texture_type != RDC::TEXTURE_TYPE_1D &&
		format.texture_type != RDC::TEXTURE_TYPE_1D_ARRAY) {
		ERR_FAIL_COND_V_MSG(format.height < 1, RID(),
			"Height must be equal or greater than 1 for 2D and 3D textures");
	}

	if (format.texture_type == RDC::TEXTURE_TYPE_3D) {
		ERR_FAIL_COND_V_MSG(
			format.depth < 1, RID(), "Depth must be equal or greater than 1 for 3D textures");
	}

	ERR_FAIL_COND_V(format.mipmaps < 1, RID());

	if (format.texture_type == RDC::TEXTURE_TYPE_1D_ARRAY ||
		format.texture_type == RDC::TEXTURE_TYPE_2D_ARRAY ||
		format.texture_type == RDC::TEXTURE_TYPE_CUBE_ARRAY ||
		format.texture_type == RDC::TEXTURE_TYPE_CUBE) {
		ERR_FAIL_COND_V_MSG(format.array_layers < 1, RID(),
			"Number of layers must be equal or greater than 1 for arrays and cubemaps.");
		ERR_FAIL_COND_V_MSG((format.texture_type == RDC::TEXTURE_TYPE_CUBE_ARRAY ||
								format.texture_type == RDC::TEXTURE_TYPE_CUBE) &&
								(format.array_layers % 6) != 0,
			RID(),
			"Cubemap and cubemap array textures must provide a layer number that is multiple of 6");
		ERR_FAIL_COND_V_MSG((format.texture_type == RDC::TEXTURE_TYPE_CUBE_ARRAY ||
								format.texture_type == RDC::TEXTURE_TYPE_CUBE) &&
								(format.width != format.height),
			RID(), "Cubemap and cubemap array textures must have equal width and height.");
	}
	else {
		format.array_layers = 1;
	}

	ERR_FAIL_INDEX_V(format.samples, RDC::TEXTURE_SAMPLES_MAX, RID());
	ERR_FAIL_COND_V_MSG(
		format.usage_bits == 0, RID(), "No usage bits specified (at least one is needed)");

	format.height = format.texture_type != RDC::TEXTURE_TYPE_1D &&
							format.texture_type != RDC::TEXTURE_TYPE_1D_ARRAY
						? format.height
						: 1;
	format.depth = format.texture_type == RDC::TEXTURE_TYPE_3D ? format.depth : 1;

	Vector<Vector<uint8_t>> tex_data = p_data;
	bool immediate_flush = false;

	if (tex_data.is_empty() && (p_format.usage_bits & RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT)) {
		immediate_flush = true;
		for (uint32_t i = 0; i < format.array_layers; i++) {
			uint32_t required_size = RDC::get_image_format_required_size(
				format.format, format.width, format.height, format.depth, format.mipmaps);
			Vector<uint8_t> layer;
			layer.resize(required_size);
			layer.fill(255);
			tex_data.push_back(layer);
		}
	}

	uint32_t forced_usage_bits = _texture_vrs_method_to_usage_bits();
	if (tex_data.size()) {
		ERR_FAIL_COND_V_MSG(tex_data.size() != (int)format.array_layers, RID(),
			"Default supplied data for image format is of invalid length (" +
				itos(tex_data.size()) + "), should be (" + itos(format.array_layers) + ").");

		for (uint32_t i = 0; i < format.array_layers; i++) {
			uint32_t required_size = RDC::get_image_format_required_size(
				format.format, format.width, format.height, format.depth, format.mipmaps);
			ERR_FAIL_COND_V_MSG((uint32_t)tex_data[i].size() != required_size, RID(),
				"Data for slice index " + itos(i) + " differs in size (" +
					itos(tex_data[i].size()) + ") than required (" + itos(required_size) + ").");
		}

		if (!(format.usage_bits & RDC::TEXTURE_USAGE_CAN_UPDATE_BIT)) {
			forced_usage_bits |= RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		}
	}

	RDD::TextureView tv;
	if (p_view.format_override == RDC::DATA_FORMAT_MAX) {
		tv.format = format.format;
	}
	else {
		ERR_FAIL_INDEX_V(p_view.format_override, RDC::DATA_FORMAT_MAX, RID());
		tv.format = p_view.format_override;
	}
	tv.swizzle_r = p_view.swizzle_r;
	tv.swizzle_g = p_view.swizzle_g;
	tv.swizzle_b = p_view.swizzle_b;
	tv.swizzle_a = p_view.swizzle_a;

	Texture texture;
	format.usage_bits |= forced_usage_bits;
	texture.driver_id = data->driver->texture_create(format, tv);
	ERR_FAIL_COND_V(!texture.driver_id, RID());
	texture.type = format.texture_type;
	texture.format = format.format;
	texture.width = format.width;
	texture.height = format.height;
	texture.depth = format.depth;
	texture.layers = format.array_layers;
	texture.mipmaps = format.mipmaps;
	texture.base_mipmap = 0;
	texture.base_layer = 0;
	texture.is_resolve_buffer = format.is_resolve_buffer;
	texture.is_discardable = format.is_discardable;
	texture.is_subsampled = format.is_subsampled;
	texture.usage_flags = format.usage_bits & ~forced_usage_bits;
	texture.samples = format.samples;
	texture.allowed_shared_formats = format.shareable_formats;
	if (!texture.allowed_shared_formats.has(texture.format)) {
		texture.allowed_shared_formats.push_back(texture.format);
	}
	texture.has_initial_data = !tex_data.is_empty();

	if (data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_OUTPUTS_REQUIRE_CLEARS)) {
		texture.pending_clear =
			!texture.has_initial_data &&
			(format.usage_bits &
				(RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
					RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));
	}

	if ((format.usage_bits & (RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
								 RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT))) {
		texture.read_aspect_flags |= RDD::TEXTURE_ASPECT_DEPTH_BIT;
		texture.barrier_aspect_flags |= RDD::TEXTURE_ASPECT_DEPTH_BIT;
		if (RDC::format_has_stencil(format.format)) {
			texture.barrier_aspect_flags |= RDD::TEXTURE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		texture.read_aspect_flags |= RDD::TEXTURE_ASPECT_COLOR_BIT;
		texture.barrier_aspect_flags |= RDD::TEXTURE_ASPECT_COLOR_BIT;
	}

	texture.bound = false;

	bool texture_mutable_by_default =
		texture.usage_flags &
		(RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
			RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
			RDC::TEXTURE_USAGE_STORAGE_ATOMIC_BIT);
	if (tex_data.is_empty() || texture_mutable_by_default) {
		_texture_make_mutable(&texture, RID());
	}

	data->texture_memory.add(data->driver->texture_get_allocation_size(texture.driver_id));

	RID id = data->texture_owner.make_rid(texture);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif

	if (tex_data.size()) {
		const bool use_general_in_copy_queues =
			data->driver->api_trait_get(RDD::API_TRAIT_USE_GENERAL_IN_COPY_QUEUES);
		const RDD::TextureLayout dst_layout = use_general_in_copy_queues
												  ? RDD::TEXTURE_LAYOUT_GENERAL
												  : RDD::TEXTURE_LAYOUT_COPY_DST_OPTIMAL;
		for (uint32_t i = 0; i < format.array_layers; i++) {
			_texture_initialize(id, i, tex_data[i], dst_layout, immediate_flush);
		}

		Texture* stored_tex = data->texture_owner.get_or_null(id);
		if (stored_tex && stored_tex->draw_tracker != nullptr) {
			stored_tex->draw_tracker->usage = use_general_in_copy_queues
												  ? RDG::RESOURCE_USAGE_GENERAL
												  : RDG::RESOURCE_USAGE_COPY_TO;
		}
	}

	return id;
}

RID RenderingDevice::texture_create(const RDC::TextureFormat& p_format,
	const RDD::TextureView& p_view, const Vector<Vector<uint8_t>>& p_data)
{
	TextureView view;
	view.format_override = p_view.format;
	view.swizzle_r = p_view.swizzle_r;
	view.swizzle_g = p_view.swizzle_g;
	view.swizzle_b = p_view.swizzle_b;
	view.swizzle_a = p_view.swizzle_a;
	return texture_create(p_format, view, p_data);
}

Vector<uint8_t> RenderingDevice::texture_get_data(RID p_texture, uint32_t p_layer)
{
	ERR_RENDER_THREAD_GUARD_V(Vector<uint8_t>());

	Texture* tex = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Vector<uint8_t>());

	ERR_FAIL_COND_V_MSG(tex->bound, Vector<uint8_t>(),
		"Texture can't be retrieved while a draw list that uses it as part of a framebuffer is "
		"being created.");
	ERR_FAIL_COND_V_MSG(!(tex->usage_flags & RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT),
		Vector<uint8_t>(),
		"Texture requires `TEXTURE_USAGE_CAN_COPY_FROM_BIT` to be set to be retrieved.");

	ERR_FAIL_COND_V(p_layer >= tex->layers, Vector<uint8_t>());

	_texture_check_pending_clear(p_texture, tex);
	_check_transfer_worker_texture(tex);

	if (tex->usage_flags & RDC::TEXTURE_USAGE_CPU_READ_BIT) {
		return data->driver->texture_get_data(tex->driver_id, p_layer);
	}

	RDD::TextureAspect aspect = (tex->read_aspect_flags & RDD::TEXTURE_ASPECT_DEPTH_BIT)
									? RDD::TEXTURE_ASPECT_DEPTH
									: RDD::TEXTURE_ASPECT_COLOR;
	uint32_t mip_alignment =
		data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT);
	uint32_t buffer_size = 0;

	thread_local LocalVector<RDD::TextureCopyableLayout> mip_layouts;
	thread_local LocalVector<RDD::BufferTextureCopyRegion> copy_regions;
	mip_layouts.resize(tex->mipmaps);
	copy_regions.resize(tex->mipmaps);

	for (uint32_t i = 0; i < tex->mipmaps; i++) {
		RDD::TextureSubresource subres;
		subres.aspect = aspect;
		subres.layer = p_layer;
		subres.mipmap = i;

		RDD::TextureCopyableLayout& mip_layout = mip_layouts[i];
		data->driver->texture_get_copyable_layout(tex->driver_id, subres, &mip_layout);

		uint32_t mip_offset = STEPIFY(buffer_size, mip_alignment);
		buffer_size = mip_offset + mip_layout.size;

		RDD::BufferTextureCopyRegion& copy_region = copy_regions[i];
		copy_region.buffer_offset = mip_offset;
		copy_region.row_pitch = mip_layout.row_pitch;
		copy_region.texture_subresource.aspect = aspect;
		copy_region.texture_subresource.mipmap = i;
		copy_region.texture_subresource.layer = p_layer;
		copy_region.texture_region_size.x = MAX(1u, tex->width >> i);
		copy_region.texture_region_size.y = MAX(1u, tex->height >> i);
		copy_region.texture_region_size.z = MAX(1u, tex->depth >> i);
	}

	RDD::BufferID tmp_buffer = data->driver->buffer_create(buffer_size,
		RDD::BUFFER_USAGE_TRANSFER_TO_BIT, RDD::MEMORY_ALLOCATION_TYPE_CPU, data->frames_drawn);
	ERR_FAIL_COND_V(!tmp_buffer, Vector<uint8_t>());

	if (_texture_make_mutable(tex, p_texture)) {
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_texture_get_data(
		tex->driver_id, tex->draw_tracker, tmp_buffer, copy_regions);

	_flush_and_stall_for_all_frames();

	const uint8_t* read_ptr = data->driver->buffer_map(tmp_buffer);
	ERR_FAIL_NULL_V(read_ptr, Vector<uint8_t>());

	uint32_t block_w = 0;
	uint32_t block_h = 0;
	RDC::get_compressed_image_format_block_dimensions(tex->format, block_w, block_h);

	Vector<uint8_t> buffer_data;
	uint32_t tight_buffer_size = RDC::get_image_format_required_size(
		tex->format, tex->width, tex->height, tex->depth, tex->mipmaps);
	buffer_data.resize(tight_buffer_size);

	uint8_t* write_ptr = buffer_data.ptrw();

	for (uint32_t i = 0; i < tex->mipmaps; i++) {
		uint32_t width = 0, height = 0, depth = 0;

		uint32_t tight_mip_size = RDC::get_image_format_required_size(tex->format,
			MAX(1u, tex->width >> i), MAX(1u, tex->height >> i), MAX(1u, tex->depth >> i), 1,
			&width, &height, &depth);

		uint32_t row_count = (height / block_h) * depth;
		uint32_t tight_row_pitch = tight_mip_size / row_count;

		const uint8_t* rp = read_ptr + copy_regions[i].buffer_offset;
		uint32_t row_pitch = mip_layouts[i].row_pitch;

		if (tight_row_pitch == row_pitch) {
			memcpy(write_ptr, rp, tight_mip_size);
			write_ptr += tight_mip_size;
		}
		else {
			for (uint32_t j = 0; j < row_count; j++) {
				memcpy(write_ptr, rp, tight_row_pitch);
				rp += row_pitch;
				write_ptr += tight_row_pitch;
			}
		}
	}

	data->driver->buffer_unmap(tmp_buffer);
	data->driver->buffer_free(tmp_buffer);

	return buffer_data;
}

RID RenderingDevice::texture_create_shared(const TextureView& p_view, RID p_with_texture)
{
	ERR_RENDER_THREAD_GUARD_V(RID());

	Texture* src_texture = data->texture_owner.get_or_null(p_with_texture);
	ERR_FAIL_NULL_V(src_texture, RID());

	Texture texture = *src_texture;

	RDD::TextureView tv;
	bool create_shared = true;
	bool raw_reinterpretation = false;

	if (p_view.format_override == RDC::DATA_FORMAT_MAX ||
		p_view.format_override == texture.format) {
		tv.format = texture.format;
	}
	else {
		ERR_FAIL_INDEX_V(p_view.format_override, RDC::DATA_FORMAT_MAX, RID());

		ERR_FAIL_COND_V_MSG(!texture.allowed_shared_formats.has(p_view.format_override), RID(),
			"Format override is not in the list of allowed shareable formats for original "
			"texture.");

		tv.format = p_view.format_override;
		create_shared = data->driver->texture_can_make_shared_with_format(
			texture.driver_id, p_view.format_override, raw_reinterpretation);
	}

	tv.swizzle_r = p_view.swizzle_r;
	tv.swizzle_g = p_view.swizzle_g;
	tv.swizzle_b = p_view.swizzle_b;
	tv.swizzle_a = p_view.swizzle_a;

	if (create_shared) {
		texture.driver_id = data->driver->texture_create_shared(texture.driver_id, tv);
	}
	else {
		RDD::TextureView regular_view = tv;
		regular_view.format = src_texture->format;
		texture.driver_id = data->driver->texture_create_shared(texture.driver_id, regular_view);

		RDC::TextureFormat alias_format = texture.texture_format();
		alias_format.format = tv.format;
		alias_format.usage_bits =
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;

		_texture_check_shared_fallback(src_texture);
		_texture_check_shared_fallback(&texture);

		texture.shared_fallback->texture = data->driver->texture_create(alias_format, tv);
		texture.shared_fallback->raw_reinterpretation = raw_reinterpretation;
		data->texture_memory.add(
			data->driver->texture_get_allocation_size(texture.shared_fallback->texture));
	}

	return data->texture_owner.make_rid(texture);
}

uint32_t RenderingDevice::_texture_layer_count(Texture* p_texture)
{
	switch (p_texture->type) {
	case RDC::TEXTURE_TYPE_CUBE:
	case RDC::TEXTURE_TYPE_CUBE_ARRAY:
		return p_texture->layers * 6;
	default:
		return p_texture->layers;
	}
}

uint32_t greatest_common_denominator(uint32_t a, uint32_t b)
{
	// Euclidean algorithm.
	uint32_t t;
	while (b != 0) {
		t = b;
		b = a % b;
		a = t;
	}

	return a;
}

uint32_t least_common_multiple(uint32_t a, uint32_t b)
{
	if (a == 0 || b == 0) {
		return 0;
	}

	return (a / greatest_common_denominator(a, b)) * b;
}

uint32_t RenderingDevice::_texture_alignment(Texture* p_texture)
{
	uint32_t alignment = RDC::get_compressed_image_format_block_byte_size(p_texture->format);
	if (alignment == 1) {
		alignment = RDC::get_image_format_pixel_size(p_texture->format);
	}

	return least_common_multiple(
		alignment, data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT));
}

void RenderingDevice::_texture_check_shared_fallback(Texture* p_texture)
{
	if (p_texture->shared_fallback == nullptr) {
		p_texture->shared_fallback = memnew(Texture::SharedFallback);
	}
}

void RenderingDevice::_texture_update_shared_fallback(
	RID p_texture_rid, Texture* p_texture, bool p_for_writing)
{
	if (p_texture->shared_fallback == nullptr) {
		// This texture does not use any of the shared texture fallbacks.
		return;
	}

	if (p_texture->owner.is_valid()) {
		Texture* owner_texture = data->texture_owner.get_or_null(p_texture->owner);
		ERR_FAIL_NULL(owner_texture);
		if (p_for_writing) {
			// Only the main texture is used for writing when using the shared fallback.
			owner_texture->shared_fallback->revision++;
		}
		else if (p_texture->shared_fallback->revision !=
				   owner_texture->shared_fallback->revision) {
			// Copy the contents of the main texture into the shared texture fallback slice. Update
			// the revision.
			_texture_copy_shared(p_texture->owner, owner_texture, p_texture_rid, p_texture);
			p_texture->shared_fallback->revision = owner_texture->shared_fallback->revision;
		}
	}
	else if (p_for_writing) {
		// Increment the revision of the texture so shared texture fallback slices must be updated.
		p_texture->shared_fallback->revision++;
	}
}

void RenderingDevice::_texture_free_shared_fallback(Texture* p_texture)
{
	if (p_texture->shared_fallback != nullptr) {
		if (p_texture->shared_fallback->texture_tracker != nullptr) {
			RDG::resource_tracker_free(p_texture->shared_fallback->texture_tracker);
		}

		if (p_texture->shared_fallback->buffer_tracker != nullptr) {
			RDG::resource_tracker_free(p_texture->shared_fallback->buffer_tracker);
		}

		if (p_texture->shared_fallback->texture.id != 0) {
			data->texture_memory.sub(
				data->driver->texture_get_allocation_size(p_texture->shared_fallback->texture));
			data->driver->texture_free(p_texture->shared_fallback->texture);
		}

		if (p_texture->shared_fallback->buffer.id != 0) {
			data->buffer_memory.sub(
				data->driver->buffer_get_allocation_size(p_texture->shared_fallback->buffer));
			data->driver->buffer_free(p_texture->shared_fallback->buffer);
		}

		memdelete(p_texture->shared_fallback);
		p_texture->shared_fallback = nullptr;
	}
}

void RenderingDevice::_texture_copy_shared(
	RID p_src_texture_rid, Texture* p_src_texture, RID p_dst_texture_rid, Texture* p_dst_texture)
{
	// The only type of copying allowed is from the main texture to the slice texture, as slice
	// textures are not allowed to be used for writing when using this fallback.
	DEV_ASSERT(p_src_texture != nullptr);
	DEV_ASSERT(p_dst_texture != nullptr);
	DEV_ASSERT(p_src_texture->owner.is_null());
	DEV_ASSERT(p_dst_texture->owner == p_src_texture_rid);

	bool src_made_mutable = _texture_make_mutable(p_src_texture, p_src_texture_rid);
	bool dst_made_mutable = _texture_make_mutable(p_dst_texture, p_dst_texture_rid);
	if (src_made_mutable || dst_made_mutable) {
		data->draw_graph.add_synchronization();
	}

	if (p_dst_texture->shared_fallback->raw_reinterpretation) {
		// If one of the textures is a main texture and they have a reinterpret buffer, we prefer
		// using that as it's guaranteed to be big enough to hold anything and it's how the shared
		// textures that don't use slices are created.
		bool src_has_buffer = p_src_texture->shared_fallback->buffer.id != 0;
		bool dst_has_buffer = p_dst_texture->shared_fallback->buffer.id != 0;
		bool from_src = p_src_texture->owner.is_null() && src_has_buffer;
		bool from_dst = p_dst_texture->owner.is_null() && dst_has_buffer;
		if (!from_src && !from_dst) {
			// If neither texture passed the condition, we just pick whichever texture has a
			// reinterpretation buffer.
			from_src = src_has_buffer;
			from_dst = dst_has_buffer;
		}

		// Pick the buffer and tracker to use from the right texture.
		RDD::BufferID shared_buffer;
		RDG::ResourceTracker* shared_buffer_tracker = nullptr;
		if (from_src) {
			shared_buffer = p_src_texture->shared_fallback->buffer;
			shared_buffer_tracker = p_src_texture->shared_fallback->buffer_tracker;
		}
		else if (from_dst) {
			shared_buffer = p_dst_texture->shared_fallback->buffer;
			shared_buffer_tracker = p_dst_texture->shared_fallback->buffer_tracker;
		}
		else {
			DEV_ASSERT(false && "This path should not be reachable.");
		}

		// Copying each mipmap from main texture to a buffer and then to the slice texture.
		thread_local LocalVector<RDD::BufferTextureCopyRegion> get_data_vector;
		thread_local LocalVector<RDG::RecordedBufferToTextureCopy> update_vector;
		get_data_vector.clear();
		update_vector.clear();

		uint32_t buffer_size = 0;
		uint32_t transfer_alignment =
			data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT);

		for (uint32_t i = 0; i < p_dst_texture->layers; i++) {
			for (uint32_t j = 0; j < p_dst_texture->mipmaps; j++) {
				// FIXME: When using reinterpretation buffers, the only texture aspect supported is
				// color. Depth or stencil contents won't get copied.
				RDD::TextureSubresource texture_subresource;
				texture_subresource.aspect = RDD::TEXTURE_ASPECT_COLOR;
				texture_subresource.layer = i;
				texture_subresource.mipmap = j;

				RDD::TextureCopyableLayout copyable_layout;
				data->driver->texture_get_copyable_layout(
					p_dst_texture->shared_fallback->texture, texture_subresource, &copyable_layout);

				uint32_t mipmap = p_dst_texture->base_mipmap + j;

				RDD::BufferTextureCopyRegion get_data_region;
				get_data_region.buffer_offset = STEPIFY(buffer_size, transfer_alignment);
				get_data_region.row_pitch = copyable_layout.row_pitch;
				get_data_region.texture_subresource.aspect = RDD::TEXTURE_ASPECT_COLOR;
				get_data_region.texture_subresource.layer = p_dst_texture->base_layer + i;
				get_data_region.texture_subresource.mipmap = mipmap;
				get_data_region.texture_region_size.x = MAX(1U, p_src_texture->width >> mipmap);
				get_data_region.texture_region_size.y = MAX(1U, p_src_texture->height >> mipmap);
				get_data_region.texture_region_size.z = MAX(1U, p_src_texture->depth >> mipmap);
				get_data_vector.push_back(get_data_region);

				RDG::RecordedBufferToTextureCopy update_copy;
				update_copy.from_buffer = shared_buffer;
				update_copy.region.buffer_offset = get_data_region.buffer_offset;
				update_copy.region.row_pitch = get_data_region.row_pitch;
				update_copy.region.texture_subresource.aspect = RDD ::TEXTURE_ASPECT_COLOR;
				update_copy.region.texture_subresource.layer = texture_subresource.layer;
				update_copy.region.texture_subresource.mipmap = texture_subresource.mipmap;
				update_copy.region.texture_region_size.x = get_data_region.texture_region_size.x;
				update_copy.region.texture_region_size.y = get_data_region.texture_region_size.y;
				update_copy.region.texture_region_size.z = get_data_region.texture_region_size.z;
				update_vector.push_back(update_copy);

				buffer_size = get_data_region.buffer_offset + copyable_layout.size;
			}
		}

		DEV_ASSERT(buffer_size <= driver->buffer_get_allocation_size(shared_buffer));

		data->draw_graph.add_texture_get_data(p_src_texture->driver_id, p_src_texture->draw_tracker,
			shared_buffer, get_data_vector, shared_buffer_tracker);
		data->draw_graph.add_texture_update(p_dst_texture->shared_fallback->texture,
			p_dst_texture->shared_fallback->texture_tracker, update_vector, shared_buffer_tracker);
	}
	else {
		// Raw reinterpretation is not required. Use a regular texture copy.
		RDD::TextureCopyRegion copy_region;
		copy_region.src_subresources.aspect = p_src_texture->read_aspect_flags;
		copy_region.src_subresources.base_layer = p_dst_texture->base_layer;
		copy_region.src_subresources.layer_count = p_dst_texture->layers;
		copy_region.dst_subresources.aspect = p_dst_texture->read_aspect_flags;
		copy_region.dst_subresources.base_layer = 0;
		copy_region.dst_subresources.layer_count = copy_region.src_subresources.layer_count;

		// Copying each mipmap from main texture to to the slice texture.
		thread_local LocalVector<RDD::TextureCopyRegion> region_vector;
		region_vector.clear();
		for (uint32_t i = 0; i < p_dst_texture->mipmaps; i++) {
			uint32_t mipmap = p_dst_texture->base_mipmap + i;
			copy_region.src_subresources.mipmap = mipmap;
			copy_region.dst_subresources.mipmap = i;
			copy_region.size.x = MAX(1U, p_src_texture->width >> mipmap);
			copy_region.size.y = MAX(1U, p_src_texture->height >> mipmap);
			copy_region.size.z = MAX(1U, p_src_texture->depth >> mipmap);
			region_vector.push_back(copy_region);
		}

		data->draw_graph.add_texture_copy(p_src_texture->driver_id, p_src_texture->draw_tracker,
			p_dst_texture->shared_fallback->texture,
			p_dst_texture->shared_fallback->texture_tracker, region_vector);
	}
}

void RenderingDevice::_texture_create_reinterpret_buffer(Texture* p_texture)
{
	uint64_t row_pitch_step =
		data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP);
	uint64_t transfer_alignment =
		data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT);
	uint32_t pixel_bytes = RDC::get_image_format_pixel_size(p_texture->format);
	uint32_t row_pitch = STEPIFY(p_texture->width * pixel_bytes, row_pitch_step);
	uint64_t buffer_size =
		STEPIFY(pixel_bytes * row_pitch * p_texture->height * p_texture->depth, transfer_alignment);
	p_texture->shared_fallback->buffer = data->driver->buffer_create(buffer_size,
		RDD::BUFFER_USAGE_TRANSFER_FROM_BIT | RDD::BUFFER_USAGE_TRANSFER_TO_BIT,
		RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	data->buffer_memory.add(
		data->driver->buffer_get_allocation_size(p_texture->shared_fallback->buffer));

	RDG::ResourceTracker* tracker = RDG::resource_tracker_create();
	tracker->buffer_driver_id = p_texture->shared_fallback->buffer;
	p_texture->shared_fallback->buffer_tracker = tracker;
}

uint32_t RenderingDevice::_texture_vrs_method_to_usage_bits()
{
	switch (data->vrs_method) {
	case VRS_METHOD_FRAGMENT_SHADING_RATE:
		return RDD::TEXTURE_USAGE_VRS_FRAGMENT_SHADING_RATE_BIT;
	case VRS_METHOD_FRAGMENT_DENSITY_MAP:
		return RDD::TEXTURE_USAGE_VRS_FRAGMENT_DENSITY_MAP_BIT;
	default:
		return 0;
	}
}

void RenderingDevice::_texture_ensure_shareable_format(
	RID p_texture, const RDC::DataFormat& p_shareable_format)
{
	Texture* texture = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);
	if (!texture->allowed_shared_formats.has(p_shareable_format)) {
		texture->allowed_shared_formats.push_back(p_shareable_format);
	}
}

void RenderingDevice::_texture_clear_color(RID p_texture_rid, Texture* p_texture,
	const Color& p_color, uint32_t p_base_mipmap, uint32_t p_mipmaps, uint32_t p_base_layer,
	uint32_t p_layers)
{
	_check_transfer_worker_texture(p_texture);

	RDD::TextureSubresourceRange range;
	range.aspect = RDD::TEXTURE_ASPECT_COLOR_BIT;
	range.base_mipmap = p_texture->base_mipmap + p_base_mipmap;
	range.mipmap_count = p_mipmaps;
	range.base_layer = p_texture->base_layer + p_base_layer;
	range.layer_count = p_layers;

	// Indicate the texture will get modified for the shared texture fallback.
	_texture_update_shared_fallback(p_texture_rid, p_texture, true);

	if (_texture_make_mutable(p_texture, p_texture_rid)) {
		// The texture must be mutable to be used as a clear destination.
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_texture_clear_color(
		p_texture->driver_id, p_texture->draw_tracker, p_color, range);
}

bool RenderingDevice::texture_is_shared(RID p_texture)
{
	ERR_RENDER_THREAD_GUARD_V(false);

	Texture* tex = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, false);
	return tex->owner.is_valid();
}

bool RenderingDevice::texture_is_valid(RID p_texture)
{
	ERR_RENDER_THREAD_GUARD_V(false);

	return data->texture_owner.owns(p_texture);
}

RDC::TextureFormat RenderingDevice::texture_get_format(RID p_texture)
{
	ERR_RENDER_THREAD_GUARD_V(RDC::TextureFormat());

	Texture* tex = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, RDC::TextureFormat());

	RDC::TextureFormat tf;

	tf.format = tex->format;
	tf.width = tex->width;
	tf.height = tex->height;
	tf.depth = tex->depth;


	tf.array_layers = tex->layers;
	tf.mipmaps = tex->mipmaps;

	tf.texture_type = tex->type;
	tf.samples = tex->samples;
	tf.usage_bits = tex->usage_flags;
	tf.shareable_formats = tex->allowed_shared_formats;
	tf.is_resolve_buffer = tex->is_resolve_buffer;
	tf.is_discardable = tex->is_discardable;

	return tf;
}

Size2i RenderingDevice::texture_size(RID p_texture)
{
	ERR_RENDER_THREAD_GUARD_V(Size2i());

	Texture* tex = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Size2i());
	return Size2i(tex->width, tex->height);
}

Error RenderingDevice::texture_copy(RID p_from_texture, RID p_to_texture, const Vector3& p_from,
	const Vector3& p_to, const Vector3& p_size, uint32_t p_src_mipmap, uint32_t p_dst_mipmap,
	uint32_t p_src_layer, uint32_t p_dst_layer)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	Texture* src_tex = data->texture_owner.get_or_null(p_from_texture);
	ERR_FAIL_NULL_V(src_tex, ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V_MSG(src_tex->bound, ERR_INVALID_PARAMETER,
		"Source texture can't be copied while a draw list that uses it as part of a framebuffer is "
		"being created. Ensure the draw list is finalized (and that the color/depth texture using "
		"it is not set to `RenderingDevice.FINAL_ACTION_CONTINUE`) to copy this texture.");
	ERR_FAIL_COND_V_MSG(!(src_tex->usage_flags & RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT),
		ERR_INVALID_PARAMETER,
		"Source texture requires the `RenderingDevice.TEXTURE_USAGE_CAN_COPY_FROM_BIT` to be set "
		"to be retrieved.");

	ERR_FAIL_COND_V(p_size.x == 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_size.y == 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_size.z == 0, ERR_INVALID_PARAMETER);

	uint32_t src_width, src_height, src_depth;
	RDC::get_image_format_required_size(src_tex->format, src_tex->width, src_tex->height,
		src_tex->depth, p_src_mipmap + 1, &src_width, &src_height, &src_depth);

	ERR_FAIL_COND_V(p_from.x < 0 || p_from.x + p_size.x > src_width, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_from.y < 0 || p_from.y + p_size.y > src_height, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_from.z < 0 || p_from.z + p_size.z > src_depth, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_src_mipmap >= src_tex->mipmaps, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_src_layer >= src_tex->layers, ERR_INVALID_PARAMETER);

	Texture* dst_tex = data->texture_owner.get_or_null(p_to_texture);
	ERR_FAIL_NULL_V(dst_tex, ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V_MSG(dst_tex->bound, ERR_INVALID_PARAMETER,
		"Destination texture can't be copied while a draw list that uses it as part of a "
		"framebuffer is being created. Ensure the draw list is finalized (and that the color/depth "
		"texture using it is not set to `RenderingDevice.FINAL_ACTION_CONTINUE`) to copy this "
		"texture.");
	ERR_FAIL_COND_V_MSG(!(dst_tex->usage_flags & RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT),
		ERR_INVALID_PARAMETER,
		"Destination texture requires the `RenderingDevice.TEXTURE_USAGE_CAN_COPY_TO_BIT` to be "
		"set to be retrieved.");

	uint32_t dst_width, dst_height, dst_depth;
	RDC::get_image_format_required_size(dst_tex->format, dst_tex->width, dst_tex->height,
		dst_tex->depth, p_dst_mipmap + 1, &dst_width, &dst_height, &dst_depth);

	ERR_FAIL_COND_V(p_to.x < 0 || p_to.x + p_size.x > dst_width, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_to.y < 0 || p_to.y + p_size.y > dst_height, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_to.z < 0 || p_to.z + p_size.z > dst_depth, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_dst_mipmap >= dst_tex->mipmaps, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_dst_layer >= dst_tex->layers, ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V_MSG(src_tex->read_aspect_flags != dst_tex->read_aspect_flags,
		ERR_INVALID_PARAMETER,
		"Source and destination texture must be of the same type (color or depth).");

	_check_transfer_worker_texture(src_tex);
	_check_transfer_worker_texture(dst_tex);

	RDD::TextureCopyRegion copy_region;
	copy_region.src_subresources.aspect = src_tex->read_aspect_flags;
	copy_region.src_subresources.mipmap = p_src_mipmap;
	copy_region.src_subresources.base_layer = p_src_layer;
	copy_region.src_subresources.layer_count = 1;
	copy_region.src_offset = p_from;

	copy_region.dst_subresources.aspect = dst_tex->read_aspect_flags;
	copy_region.dst_subresources.mipmap = p_dst_mipmap;
	copy_region.dst_subresources.base_layer = p_dst_layer;
	copy_region.dst_subresources.layer_count = 1;
	copy_region.dst_offset = p_to;

	copy_region.size = p_size;

	// Indicate the texture will get modified for the shared texture fallback.
	_texture_update_shared_fallback(p_to_texture, dst_tex, true);

	// The textures must be mutable to be used in the copy operation.
	bool src_made_mutable = _texture_make_mutable(src_tex, p_from_texture);
	bool dst_made_mutable = _texture_make_mutable(dst_tex, p_to_texture);
	if (src_made_mutable || dst_made_mutable) {
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_texture_copy(src_tex->driver_id, src_tex->draw_tracker, dst_tex->driver_id,
		dst_tex->draw_tracker, copy_region);

	return OK;
}

Error RenderingDevice::texture_resolve_multisample(RID p_from_texture, RID p_to_texture)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	Texture* src_tex = data->texture_owner.get_or_null(p_from_texture);
	ERR_FAIL_NULL_V(src_tex, ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V_MSG(src_tex->bound, ERR_INVALID_PARAMETER,
		"Source texture can't be copied while a draw list that uses it as part of a framebuffer is "
		"being created. Ensure the draw list is finalized (and that the color/depth texture using "
		"it is not set to `RenderingDevice.FINAL_ACTION_CONTINUE`) to copy this texture.");
	ERR_FAIL_COND_V_MSG(!(src_tex->usage_flags & RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT),
		ERR_INVALID_PARAMETER,
		"Source texture requires the `RenderingDevice.TEXTURE_USAGE_CAN_COPY_FROM_BIT` to be set "
		"to be retrieved.");

	ERR_FAIL_COND_V_MSG(src_tex->type != RDC::TEXTURE_TYPE_2D, ERR_INVALID_PARAMETER,
		"Source texture must be 2D (or a slice of a 3D/Cube texture)");
	ERR_FAIL_COND_V_MSG(src_tex->samples == RDC::TEXTURE_SAMPLES_1, ERR_INVALID_PARAMETER,
		"Source texture must be multisampled.");

	Texture* dst_tex = data->texture_owner.get_or_null(p_to_texture);
	ERR_FAIL_NULL_V(dst_tex, ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V_MSG(dst_tex->bound, ERR_INVALID_PARAMETER,
		"Destination texture can't be copied while a draw list that uses it as part of a "
		"framebuffer is being created. Ensure the draw list is finalized (and that the color/depth "
		"texture using it is not set to `RenderingDevice.FINAL_ACTION_CONTINUE`) to copy this "
		"texture.");
	ERR_FAIL_COND_V_MSG(!(dst_tex->usage_flags & RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT),
		ERR_INVALID_PARAMETER,
		"Destination texture requires the `RenderingDevice.TEXTURE_USAGE_CAN_COPY_TO_BIT` to be "
		"set to be retrieved.");

	ERR_FAIL_COND_V_MSG(dst_tex->type != RDC::TEXTURE_TYPE_2D, ERR_INVALID_PARAMETER,
		"Destination texture must be 2D (or a slice of a 3D/Cube texture).");
	ERR_FAIL_COND_V_MSG(dst_tex->samples != RDC::TEXTURE_SAMPLES_1, ERR_INVALID_PARAMETER,
		"Destination texture must not be multisampled.");

	ERR_FAIL_COND_V_MSG(src_tex->format != dst_tex->format, ERR_INVALID_PARAMETER,
		"Source and Destination textures must be the same format.");
	ERR_FAIL_COND_V_MSG(src_tex->width != dst_tex->width && src_tex->height != dst_tex->height &&
							src_tex->depth != dst_tex->depth,
		ERR_INVALID_PARAMETER, "Source and Destination textures must have the same dimensions.");

	ERR_FAIL_COND_V_MSG(src_tex->read_aspect_flags != dst_tex->read_aspect_flags,
		ERR_INVALID_PARAMETER,
		"Source and destination texture must be of the same type (color or depth).");

	// Indicate the texture will get modified for the shared texture fallback.
	_texture_update_shared_fallback(p_to_texture, dst_tex, true);

	_check_transfer_worker_texture(src_tex);
	_check_transfer_worker_texture(dst_tex);

	// The textures must be mutable to be used in the resolve operation.
	bool src_made_mutable = _texture_make_mutable(src_tex, p_from_texture);
	bool dst_made_mutable = _texture_make_mutable(dst_tex, p_to_texture);
	if (src_made_mutable || dst_made_mutable) {
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_texture_resolve(src_tex->driver_id, src_tex->draw_tracker,
		dst_tex->driver_id, dst_tex->draw_tracker, src_tex->base_layer, src_tex->base_mipmap,
		dst_tex->base_layer, dst_tex->base_mipmap);

	return OK;
}

void RenderingDevice::texture_set_discardable(RID p_texture, bool p_discardable)
{
	ERR_RENDER_THREAD_GUARD();

	Texture* texture = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);

	texture->is_discardable = p_discardable;

	if (texture->draw_tracker != nullptr) {
		texture->draw_tracker->is_discardable = p_discardable;
	}

	if (texture->shared_fallback != nullptr &&
		texture->shared_fallback->texture_tracker != nullptr) {
		texture->shared_fallback->texture_tracker->is_discardable = p_discardable;
	}
}

bool RenderingDevice::texture_is_discardable(RID p_texture)
{
	ERR_RENDER_THREAD_GUARD_V(false);

	Texture* texture = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, false);

	return texture->is_discardable;
}

Error RenderingDevice::texture_clear(RID p_texture, const Color& p_color, uint32_t p_base_mipmap,
	uint32_t p_mipmaps, uint32_t p_base_layer, uint32_t p_layers)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	Texture* src_tex = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(src_tex, ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V_MSG(src_tex->bound, ERR_INVALID_PARAMETER,
		"Source texture can't be cleared while a draw list that uses it as part of a framebuffer "
		"is being created. Ensure the draw list is finalized (and that the color/depth texture "
		"using it is not set to `RenderingDevice.FINAL_ACTION_CONTINUE`) to clear this texture.");

	ERR_FAIL_COND_V(p_layers == 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_mipmaps == 0, ERR_INVALID_PARAMETER);

	ERR_FAIL_COND_V_MSG(!(src_tex->usage_flags & RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT),
		ERR_INVALID_PARAMETER,
		"Source texture requires the `RenderingDevice.TEXTURE_USAGE_CAN_COPY_TO_BIT` to be set to "
		"be cleared.");

	ERR_FAIL_COND_V(p_base_mipmap + p_mipmaps > src_tex->mipmaps, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_base_layer + p_layers > src_tex->layers, ERR_INVALID_PARAMETER);

	_texture_clear_color(
		p_texture, src_tex, p_color, p_base_mipmap, p_mipmaps, p_base_layer, p_layers);

	return OK;
}

bool RenderingDevice::texture_is_format_supported_for_usage(
	RDC::DataFormat p_format, uint32_t p_usage)
{
	ERR_FAIL_INDEX_V(p_format, RDC::DATA_FORMAT_MAX, false);

	bool cpu_readable = (p_usage & RDC::TEXTURE_USAGE_CPU_READ_BIT);
	uint32_t supported =
		data->driver->texture_get_usages_supported_by_format(p_format, cpu_readable);
	bool any_unsupported = (((int64_t)supported) | ((int64_t)p_usage)) != ((int64_t)supported);
	return !any_unsupported;
}

RDD::RenderPassID RenderingDevice::_render_pass_create(RenderingDeviceDriver* p_driver,
	const Vector<AttachmentFormat>& p_attachments, const Vector<FramebufferPass>& p_passes,
	VectorView<RDD::AttachmentLoadOp> p_load_ops, VectorView<RDD::AttachmentStoreOp> p_store_ops,
	uint32_t p_view_count, VRSMethod p_vrs_method, int32_t p_vrs_attachment,
	Size2i p_vrs_texel_size, Vector<RDC::TextureSamples>* r_samples)
{
	// NOTE:
	// Before the refactor to RenderingDevice-RenderingDeviceDriver, there was commented out code to
	// specify dependencies to external subpasses. Since it had been unused for a long timel it
	// wasn't ported to the new architecture.

	LocalVector<int32_t> attachment_last_pass;
	attachment_last_pass.resize(p_attachments.size());

	if (p_view_count > 1) {
		const RDD::MultiviewCapabilities& capabilities = p_driver->get_multiview_capabilities();

		// This only works with multiview!
		ERR_FAIL_COND_V_MSG(
			!capabilities.is_supported, RDD::RenderPassID(), "Multiview not supported");

		// Make sure we limit this to the number of views we support.
		ERR_FAIL_COND_V_MSG(p_view_count > capabilities.max_view_count, RDD::RenderPassID(),
			"Hardware does not support requested number of views for Multiview render pass");
	}

	LocalVector<RDD::Attachment> attachments;
	LocalVector<uint32_t> attachment_remap;

	for (int i = 0; i < p_attachments.size(); i++) {
		if (p_attachments[i].usage_flags == AttachmentFormat::UNUSED_ATTACHMENT) {
			attachment_remap.push_back(RDD::AttachmentReference::UNUSED);
			continue;
		}

		ERR_FAIL_INDEX_V(p_attachments[i].format, RDC::DATA_FORMAT_MAX, RDD::RenderPassID());
		ERR_FAIL_INDEX_V(p_attachments[i].samples, RDC::TEXTURE_SAMPLES_MAX, RDD::RenderPassID());
		ERR_FAIL_COND_V_MSG(
			!(p_attachments[i].usage_flags & (RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
												 RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
												 RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT |
												 RDC::TEXTURE_USAGE_INPUT_ATTACHMENT_BIT |
												 RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT)),
			RDD::RenderPassID(),
			"Texture format for index (" + itos(i) +
				") requires an attachment (color, depth-stencil, input or VRS) bit set.");

		RDD::Attachment description;
		description.format = p_attachments[i].format;
		description.samples = p_attachments[i].samples;

		// We can setup a framebuffer where we write to our VRS texture to set it up.
		// We make the assumption here that if our texture is actually used as our VRS attachment.
		// It is used as such for each subpass. This is fairly certain seeing the restrictions on
		// subpasses.
		bool is_vrs = (p_attachments[i].usage_flags & RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT) &&
					  i == p_vrs_attachment;
		if (is_vrs) {
			description.load_op = RDD::ATTACHMENT_LOAD_OP_LOAD;
			description.store_op = RDD::ATTACHMENT_STORE_OP_DONT_CARE;
			description.stencil_load_op = RDD::ATTACHMENT_LOAD_OP_DONT_CARE;
			description.stencil_store_op = RDD::ATTACHMENT_STORE_OP_DONT_CARE;
			description.initial_layout = _vrs_layout_from_method(p_vrs_method);
			description.final_layout = _vrs_layout_from_method(p_vrs_method);
		}
		else {
			if (p_attachments[i].usage_flags & RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT) {
				description.load_op = p_load_ops[i];
				description.store_op = p_store_ops[i];
				description.stencil_load_op = RDD::ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencil_store_op = RDD::ATTACHMENT_STORE_OP_DONT_CARE;
				description.initial_layout = RDD::TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				description.final_layout = RDD::TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			else if (p_attachments[i].usage_flags &
					   RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				description.load_op = p_load_ops[i];
				description.store_op = p_store_ops[i];
				description.stencil_load_op = p_load_ops[i];
				description.stencil_store_op = p_store_ops[i];
				description.initial_layout = RDD::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				description.final_layout = RDD::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else if (p_attachments[i].usage_flags &
					   RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT) {
				description.load_op = p_load_ops[i];
				description.store_op = p_store_ops[i];
				description.stencil_load_op = p_load_ops[i];
				description.stencil_store_op = p_store_ops[i];
				description.initial_layout = RDD::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				description.final_layout = RDD::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			}
			else {
				description.load_op = RDD::ATTACHMENT_LOAD_OP_DONT_CARE;
				description.store_op = RDD::ATTACHMENT_STORE_OP_DONT_CARE;
				description.stencil_load_op = RDD::ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencil_store_op = RDD::ATTACHMENT_STORE_OP_DONT_CARE;
				description.initial_layout = RDD::TEXTURE_LAYOUT_UNDEFINED;
				description.final_layout = RDD::TEXTURE_LAYOUT_UNDEFINED;
			}
		}

		attachment_last_pass[i] = -1;
		attachment_remap.push_back(attachments.size());
		attachments.push_back(description);
	}

	LocalVector<RDD::Subpass> subpasses;
	subpasses.resize(p_passes.size());
	LocalVector<RDD::SubpassDependency> subpass_dependencies;

	for (int i = 0; i < p_passes.size(); i++) {
		const FramebufferPass* pass = &p_passes[i];
		RDD::Subpass& subpass = subpasses[i];

		RDC::TextureSamples texture_samples = RDC::TEXTURE_SAMPLES_1;
		bool is_multisample_first = true;

		for (int j = 0; j < pass->color_attachments.size(); j++) {
			int32_t attachment = pass->color_attachments[j];
			RDD::AttachmentReference reference;
			if (attachment == RDC::ATTACHMENT_UNUSED) {
				reference.attachment = RDD::AttachmentReference::UNUSED;
				reference.layout = RDD::TEXTURE_LAYOUT_UNDEFINED;
			}
			else {
				ERR_FAIL_INDEX_V_MSG(attachment, p_attachments.size(), RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), color attachment (" + itos(j) + ").");
				ERR_FAIL_COND_V_MSG(!(p_attachments[attachment].usage_flags &
										RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT),
					RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) +
						"), it's marked as depth, but it's not usable as color attachment.");
				ERR_FAIL_COND_V_MSG(attachment_last_pass[attachment] == i, RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), it already was used for something else before in this pass.");

				if (is_multisample_first) {
					texture_samples = p_attachments[attachment].samples;
					is_multisample_first = false;
				}
				else {
					ERR_FAIL_COND_V_MSG(texture_samples != p_attachments[attachment].samples,
						RDD::RenderPassID(),
						"Invalid framebuffer format attachment(" + itos(attachment) +
							"), in pass (" + itos(i) +
							"), if an attachment is marked as multisample, all of them should be "
							"multisample and use the same number of samples.");
				}
				reference.attachment = attachment_remap[attachment];
				reference.layout = RDD::TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				attachment_last_pass[attachment] = i;
			}
			reference.aspect = RDD::TEXTURE_ASPECT_COLOR_BIT;
			subpass.color_references.push_back(reference);
		}

		for (int j = 0; j < pass->input_attachments.size(); j++) {
			int32_t attachment = pass->input_attachments[j];
			RDD::AttachmentReference reference;
			if (attachment == RDC::ATTACHMENT_UNUSED) {
				reference.attachment = RDD::AttachmentReference::UNUSED;
				reference.layout = RDD::TEXTURE_LAYOUT_UNDEFINED;
			}
			else {
				ERR_FAIL_INDEX_V_MSG(attachment, p_attachments.size(), RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), input attachment (" + itos(j) + ").");
				ERR_FAIL_COND_V_MSG(!(p_attachments[attachment].usage_flags &
										RDC::TEXTURE_USAGE_INPUT_ATTACHMENT_BIT),
					RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), it isn't marked as an input texture.");
				ERR_FAIL_COND_V_MSG(attachment_last_pass[attachment] == i, RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), it already was used for something else before in this pass.");
				reference.attachment = attachment_remap[attachment];
				reference.layout = RDD::TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				attachment_last_pass[attachment] = i;
			}
			reference.aspect = RDD::TEXTURE_ASPECT_COLOR_BIT;
			subpass.input_references.push_back(reference);
		}

		if (pass->resolve_attachments.size() > 0) {
			ERR_FAIL_COND_V_MSG(pass->resolve_attachments.size() != pass->color_attachments.size(),
				RDD::RenderPassID(),
				"The amount of resolve attachments (" + itos(pass->resolve_attachments.size()) +
					") must match the number of color attachments (" +
					itos(pass->color_attachments.size()) + ").");
			ERR_FAIL_COND_V_MSG(texture_samples == RDC::TEXTURE_SAMPLES_1, RDD::RenderPassID(),
				"Resolve attachments specified, but color attachments are not multisample.");
		}
		for (int j = 0; j < pass->resolve_attachments.size(); j++) {
			int32_t attachment = pass->resolve_attachments[j];
			attachments[attachment].load_op = RDD::ATTACHMENT_LOAD_OP_DONT_CARE;

			RDD::AttachmentReference reference;
			if (attachment == RDC::ATTACHMENT_UNUSED) {
				reference.attachment = RDD::AttachmentReference::UNUSED;
				reference.layout = RDD::TEXTURE_LAYOUT_UNDEFINED;
			}
			else {
				ERR_FAIL_INDEX_V_MSG(attachment, p_attachments.size(), RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), resolve attachment (" + itos(j) + ").");
				ERR_FAIL_COND_V_MSG(pass->color_attachments[j] == RDC::ATTACHMENT_UNUSED,
					RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), resolve attachment (" + itos(j) +
						"), the respective color attachment is marked as unused.");
				ERR_FAIL_COND_V_MSG(!(p_attachments[attachment].usage_flags &
										RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT),
					RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), resolve attachment, it isn't marked as a color texture.");
				ERR_FAIL_COND_V_MSG(attachment_last_pass[attachment] == i, RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), it already was used for something else before in this pass.");
				bool multisample = p_attachments[attachment].samples > RDC::TEXTURE_SAMPLES_1;
				ERR_FAIL_COND_V_MSG(multisample, RDD::RenderPassID(),
					"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
						itos(i) + "), resolve attachments can't be multisample.");
				reference.attachment = attachment_remap[attachment];
				reference.layout = RDD::
					TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // RDD::TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				attachment_last_pass[attachment] = i;
			}
			reference.aspect = RDD::TEXTURE_ASPECT_COLOR_BIT;
			subpass.resolve_references.push_back(reference);
		}

		if (pass->depth_attachment != RDC::ATTACHMENT_UNUSED) {
			int32_t attachment = pass->depth_attachment;
			ERR_FAIL_INDEX_V_MSG(attachment, p_attachments.size(), RDD::RenderPassID(),
				"Invalid framebuffer depth format attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), depth attachment.");
			ERR_FAIL_COND_V_MSG(!(p_attachments[attachment].usage_flags &
									RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT),
				RDD::RenderPassID(),
				"Invalid framebuffer depth format attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), it's marked as depth, but it's not a depth attachment.");
			ERR_FAIL_COND_V_MSG(attachment_last_pass[attachment] == i, RDD::RenderPassID(),
				"Invalid framebuffer depth format attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), it already was used for something else before in this pass.");
			subpass.depth_stencil_reference.attachment = attachment_remap[attachment];
			subpass.depth_stencil_reference.layout =
				RDD::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachment_last_pass[attachment] = i;

			if (is_multisample_first) {
				texture_samples = p_attachments[attachment].samples;
				is_multisample_first = false;
			}
			else {
				ERR_FAIL_COND_V_MSG(texture_samples != p_attachments[attachment].samples,
					RDD::RenderPassID(),
					"Invalid framebuffer depth format attachment(" + itos(attachment) +
						"), in pass (" + itos(i) +
						"), if an attachment is marked as multisample, all of them should be "
						"multisample and use the same number of samples including the depth.");
			}

			if (pass->depth_resolve_attachment != RDC::ATTACHMENT_UNUSED) {
				attachment = pass->depth_resolve_attachment;

				// As our fallbacks are handled outside of our pass, we should never be setting up a
				// render pass with a depth resolve attachment when not supported.
				ERR_FAIL_COND_V_MSG(!p_driver->has_feature(RDC::SUPPORTS_FRAMEBUFFER_DEPTH_RESOLVE),
					RDD::RenderPassID(),
					"Invalid framebuffer depth format attachment(" + itos(attachment) +
						"), in pass (" + itos(i) +
						"), a depth resolve attachment was supplied when driver doesn't support "
						"this feature.");

				ERR_FAIL_INDEX_V_MSG(attachment, p_attachments.size(), RDD::RenderPassID(),
					"Invalid framebuffer depth resolve format attachment(" + itos(attachment) +
						"), in pass (" + itos(i) + "), depth resolve attachment.");
				ERR_FAIL_COND_V_MSG(!(p_attachments[attachment].usage_flags &
										RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT),
					RDD::RenderPassID(),
					"Invalid framebuffer depth resolve format attachment(" + itos(attachment) +
						"), in pass (" + itos(i) +
						"), it's marked as depth, but it's not a depth resolve attachment.");
				ERR_FAIL_COND_V_MSG(attachment_last_pass[attachment] == i, RDD::RenderPassID(),
					"Invalid framebuffer depth resolve format attachment(" + itos(attachment) +
						"), in pass (" + itos(i) +
						"), it already was used for something else before in this pass.");

				subpass.depth_resolve_reference.attachment = attachment_remap[attachment];
				subpass.depth_resolve_reference.layout =
					RDD::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				attachment_last_pass[attachment] = i;
			}

		}
		else {
			subpass.depth_stencil_reference.attachment = RDD::AttachmentReference::UNUSED;
			subpass.depth_stencil_reference.layout = RDD::TEXTURE_LAYOUT_UNDEFINED;
		}

		if (p_vrs_method == VRS_METHOD_FRAGMENT_SHADING_RATE && p_vrs_attachment >= 0) {
			int32_t attachment = p_vrs_attachment;
			ERR_FAIL_INDEX_V_MSG(attachment, p_attachments.size(), RDD::RenderPassID(),
				"Invalid framebuffer VRS format attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), VRS attachment.");
			ERR_FAIL_COND_V_MSG(
				!(p_attachments[attachment].usage_flags & RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT),
				RDD::RenderPassID(),
				"Invalid framebuffer VRS format attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), it's marked as VRS, but it's not a VRS attachment.");
			ERR_FAIL_COND_V_MSG(attachment_last_pass[attachment] == i, RDD::RenderPassID(),
				"Invalid framebuffer VRS attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), it already was used for something else before in this pass.");

			subpass.fragment_shading_rate_reference.attachment = attachment_remap[attachment];
			subpass.fragment_shading_rate_reference.layout =
				RDD::TEXTURE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL;
			subpass.fragment_shading_rate_texel_size = p_vrs_texel_size;

			attachment_last_pass[attachment] = i;
		}

		for (int j = 0; j < pass->preserve_attachments.size(); j++) {
			int32_t attachment = pass->preserve_attachments[j];

			ERR_FAIL_COND_V_MSG(attachment == RDC::ATTACHMENT_UNUSED, RDD::RenderPassID(),
				"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), preserve attachment (" + itos(j) +
					"). Preserve attachments can't be unused.");

			ERR_FAIL_INDEX_V_MSG(attachment, p_attachments.size(), RDD::RenderPassID(),
				"Invalid framebuffer format attachment(" + itos(attachment) + "), in pass (" +
					itos(i) + "), preserve attachment (" + itos(j) + ").");

			if (attachment_last_pass[attachment] != i) {
				// Preserve can still be used to keep depth or color from being discarded after use.
				attachment_last_pass[attachment] = i;
				subpasses[i].preserve_attachments.push_back(attachment);
			}
		}

		if (r_samples) {
			r_samples->push_back(texture_samples);
		}

		if (i > 0) {
			RDD::SubpassDependency dependency;
			dependency.src_subpass = i - 1;
			dependency.dst_subpass = i;
			dependency.src_stages = (RDD::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
									 RDD::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
									 RDD::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
			dependency.dst_stages = (RDD::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
									 RDD::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
									 RDD::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
									 RDD::PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			dependency.src_access = (RDD::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
									 RDD::BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
			dependency.dst_access = (RDD::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT |
									 RDD::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
									 RDD::BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
									 RDD::BARRIER_ACCESS_INPUT_ATTACHMENT_READ_BIT);
			subpass_dependencies.push_back(dependency);
		}
	}

	RDD::AttachmentReference fragment_density_map_attachment_reference;
	if (p_vrs_method == VRS_METHOD_FRAGMENT_DENSITY_MAP && p_vrs_attachment >= 0) {
		fragment_density_map_attachment_reference.attachment = p_vrs_attachment;
		fragment_density_map_attachment_reference.layout =
			RDD::TEXTURE_LAYOUT_FRAGMENT_DENSITY_MAP_ATTACHMENT_OPTIMAL;
	}

	RDD::RenderPassID render_pass = p_driver->render_pass_create(attachments, subpasses,
		subpass_dependencies, p_view_count, fragment_density_map_attachment_reference);
	ERR_FAIL_COND_V(!render_pass, RDD::RenderPassID());

	return render_pass;
}

RDD::RenderPassID RenderingDevice::_render_pass_create_from_graph(RenderingDeviceDriver* p_driver,
	VectorView<RDD::AttachmentLoadOp> p_load_ops, VectorView<RDD::AttachmentStoreOp> p_store_ops,
	void* p_user_data)
{
	DEV_ASSERT(p_driver != nullptr);
	DEV_ASSERT(p_user_data != nullptr);

	// The graph delegates the creation of the render pass to the user according to the load and
	// store ops that were determined as necessary after resolving the dependencies between
	// commands. This function creates a render pass for the framebuffer accordingly.
	const FramebufferFormatKey* key = (const FramebufferFormatKey*)(p_user_data);
	return _render_pass_create(p_driver, key->attachments, key->passes, p_load_ops, p_store_ops,
		key->view_count, key->vrs_method, key->vrs_attachment, key->vrs_texel_size);
}

RDG::ResourceUsage RenderingDevice::_vrs_usage_from_method(VRSMethod p_method)
{
	switch (p_method) {
	case VRS_METHOD_FRAGMENT_SHADING_RATE:
		return RDG::RESOURCE_USAGE_ATTACHMENT_FRAGMENT_SHADING_RATE_READ;
	case VRS_METHOD_FRAGMENT_DENSITY_MAP:
		return RDG::RESOURCE_USAGE_ATTACHMENT_FRAGMENT_DENSITY_MAP_READ;
	default:
		return RDG::RESOURCE_USAGE_NONE;
	}
}

RDD::PipelineStageBits RenderingDevice::_vrs_stages_from_method(VRSMethod p_method)
{
	switch (p_method) {
	case VRS_METHOD_FRAGMENT_SHADING_RATE:
		return RDD::PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT;
	case VRS_METHOD_FRAGMENT_DENSITY_MAP:
		return RDD::PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT;
	default:
		return RDD::PipelineStageBits(0);
	}
}

RDD::TextureLayout RenderingDevice::_vrs_layout_from_method(VRSMethod p_method)
{
	switch (p_method) {
	case VRS_METHOD_FRAGMENT_SHADING_RATE:
		return RDD::TEXTURE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL;
	case VRS_METHOD_FRAGMENT_DENSITY_MAP:
		return RDD::TEXTURE_LAYOUT_FRAGMENT_DENSITY_MAP_ATTACHMENT_OPTIMAL;
	default:
		return RDD::TEXTURE_LAYOUT_UNDEFINED;
	}
}

void RenderingDevice::_vrs_detect_method()
{
	const RDD::FragmentShadingRateCapabilities& fsr_capabilities =
		data->driver->get_fragment_shading_rate_capabilities();
	const RDD::FragmentDensityMapCapabilities& fdm_capabilities =
		data->driver->get_fragment_density_map_capabilities();
	if (fsr_capabilities.attachment_supported) {
		data->vrs_method = VRS_METHOD_FRAGMENT_SHADING_RATE;
	}
	else if (fdm_capabilities.attachment_supported) {
		data->vrs_method = VRS_METHOD_FRAGMENT_DENSITY_MAP;
	}

	switch (data->vrs_method) {
	case VRS_METHOD_FRAGMENT_SHADING_RATE:
		data->vrs_format = RDC::DATA_FORMAT_R8_UINT;
		data->vrs_texel_size = Vector2i(16, 16).clamp(
			fsr_capabilities.min_texel_size, fsr_capabilities.max_texel_size);
		break;
	case VRS_METHOD_FRAGMENT_DENSITY_MAP:
		data->vrs_format = RDC::DATA_FORMAT_R8G8_UNORM;
		data->vrs_texel_size = Vector2i(32, 32).clamp(
			fdm_capabilities.min_texel_size, fdm_capabilities.max_texel_size);
		break;
	default:
		break;
	}
}

RD::VRSMethod RenderingDevice::vrs_get_method() { return data->vrs_method; }

RDC::DataFormat RenderingDevice::vrs_get_format() { return data->vrs_format; }

Size2i RenderingDevice::vrs_get_texel_size() { return data->vrs_texel_size; }

RenderingDevice::FramebufferFormatID RenderingDevice::framebuffer_format_create(
	const Vector<AttachmentFormat>& p_format, uint32_t p_view_count,
	int32_t p_fragment_density_map_attachment)
{
	FramebufferPass pass;
	for (int i = 0; i < p_format.size(); i++) {
		if (p_format[i].usage_flags & RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			pass.depth_attachment = i;
		}
		else if (p_format[i].usage_flags & RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT) {
			pass.depth_resolve_attachment = i;
		}
		else {
			pass.color_attachments.push_back(i);
		}
	}

	Vector<FramebufferPass> passes;
	passes.push_back(pass);
	return framebuffer_format_create_multipass(
		p_format, passes, p_view_count, p_fragment_density_map_attachment);
}

RenderingDevice::FramebufferFormatID RenderingDevice::framebuffer_format_create_multipass(
	const Vector<AttachmentFormat>& p_attachments, const Vector<FramebufferPass>& p_passes,
	uint32_t p_view_count, int32_t p_vrs_attachment)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	FramebufferFormatKey key;
	key.attachments = p_attachments;
	key.passes = p_passes;
	key.view_count = p_view_count;
	key.vrs_method = data->vrs_method;
	key.vrs_attachment = p_vrs_attachment;
	key.vrs_texel_size = data->vrs_texel_size;

	const RBMap<FramebufferFormatKey, FramebufferFormatID>::Element* E =
		data->framebuffer_format_cache.find(key);
	if (E) {
		// Exists, return.
		return E->get();
	}

	Vector<RDC::TextureSamples> samples;
	LocalVector<RDD::AttachmentLoadOp> load_ops;
	LocalVector<RDD::AttachmentStoreOp> store_ops;
	for (int64_t i = 0; i < p_attachments.size(); i++) {
		load_ops.push_back(RDD::ATTACHMENT_LOAD_OP_CLEAR);
		store_ops.push_back(RDD::ATTACHMENT_STORE_OP_STORE);
	}

	RDD::RenderPassID render_pass = _render_pass_create(data->driver, p_attachments, p_passes,
		load_ops, store_ops, p_view_count, data->vrs_method, p_vrs_attachment, data->vrs_texel_size,
		&samples);		// Actions don't matter for this use case.
	if (!render_pass) { // Was likely invalid.
		return RDC::INVALID_ID;
	}

	FramebufferFormatID id =
		FramebufferFormatID(data->framebuffer_format_cache.size()) |
		(FramebufferFormatID(ID_TYPE_FRAMEBUFFER_FORMAT) << FramebufferFormatID(ID_BASE_SHIFT));
	E = data->framebuffer_format_cache.insert(key, id);

	FramebufferFormat fb_format;
	fb_format.E = E;
	fb_format.render_pass = render_pass;
	fb_format.pass_samples = samples;
	fb_format.view_count = p_view_count;
	data->framebuffer_formats[id] = fb_format;

#if PRINT_FRAMEBUFFER_FORMAT
	print_line("FRAMEBUFFER FORMAT:", id, "ATTACHMENTS:", p_attachments.size(),
		"PASSES:", p_passes.size());
	for (RD::AttachmentFormat attachment : p_attachments) {
		print_line("FORMAT:", attachment.format, "SAMPLES:", attachment.samples,
			"USAGE FLAGS:", attachment.usage_flags);
	}
#endif

	return id;
}

RenderingDevice::FramebufferFormatID RenderingDevice::framebuffer_format_create_empty(
	RDC::TextureSamples p_samples)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	FramebufferFormatKey key;
	key.passes.push_back(FramebufferPass());

	const RBMap<FramebufferFormatKey, FramebufferFormatID>::Element* E =
		data->framebuffer_format_cache.find(key);
	if (E) {
		// Exists, return.
		return E->get();
	}

	LocalVector<RDD::Subpass> subpass;
	subpass.resize(1);

	RDD::RenderPassID render_pass =
		data->driver->render_pass_create({}, subpass, {}, 1, RDD::AttachmentReference());
	ERR_FAIL_COND_V(!render_pass, FramebufferFormatID());

	FramebufferFormatID id =
		FramebufferFormatID(data->framebuffer_format_cache.size()) |
		(FramebufferFormatID(ID_TYPE_FRAMEBUFFER_FORMAT) << FramebufferFormatID(ID_BASE_SHIFT));

	E = data->framebuffer_format_cache.insert(key, id);

	FramebufferFormat fb_format;
	fb_format.E = E;
	fb_format.render_pass = render_pass;
	fb_format.pass_samples.push_back(p_samples);
	data->framebuffer_formats[id] = fb_format;

#if PRINT_FRAMEBUFFER_FORMAT
	print_line("FRAMEBUFFER FORMAT:", id, "ATTACHMENTS: EMPTY");
#endif

	return id;
}

RDC::TextureSamples RenderingDevice::framebuffer_format_get_texture_samples(
	FramebufferFormatID p_format, uint32_t p_pass)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	HashMap<FramebufferFormatID, FramebufferFormat>::Iterator E =
		data->framebuffer_formats.find(p_format);
	ERR_FAIL_COND_V(!E, RDC::TEXTURE_SAMPLES_1);
	ERR_FAIL_COND_V(p_pass >= uint32_t(E->value.pass_samples.size()), RDC::TEXTURE_SAMPLES_1);

	return E->value.pass_samples[p_pass];
}

RID RenderingDevice::framebuffer_create_empty(
	const Size2i& p_size, RDC::TextureSamples p_samples, FramebufferFormatID p_format_check)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	Framebuffer framebuffer;
	framebuffer.format_id = framebuffer_format_create_empty(p_samples);
	ERR_FAIL_COND_V(
		p_format_check != INVALID_FORMAT_ID && framebuffer.format_id != p_format_check, RID());
	framebuffer.size = p_size;
	framebuffer.view_count = 1;

	RDG::FramebufferCache* framebuffer_cache = RDG::framebuffer_cache_create();
	framebuffer_cache->width = p_size.width;
	framebuffer_cache->height = p_size.height;
	framebuffer.framebuffer_cache = framebuffer_cache;

	RID id = data->framebuffer_owner.make_rid(framebuffer);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif

	// This relies on the fact that HashMap will not change the address of an object after it's been
	// inserted into the container.
	framebuffer_cache->render_pass_creation_user_data =
		(void*)(&data->framebuffer_formats[framebuffer.format_id].E->key());

	return id;
}

RID RenderingDevice::framebuffer_create(const Vector<RID>& p_texture_attachments,
	FramebufferFormatID p_format_check, uint32_t p_view_count)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	FramebufferPass pass;

	for (int i = 0; i < p_texture_attachments.size(); i++) {
		Texture* texture = data->texture_owner.get_or_null(p_texture_attachments[i]);

		ERR_FAIL_COND_V_MSG(texture && texture->layers != p_view_count, RID(),
			"Layers of our texture doesn't match view count for this framebuffer");

		if (texture != nullptr) {
			_check_transfer_worker_texture(texture);
		}

		if (texture && texture->usage_flags & RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			pass.depth_attachment = i;
		}
		else if (texture &&
				   texture->usage_flags & RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT) {
			pass.depth_resolve_attachment = i;
		}
		else if (texture && texture->usage_flags & RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT) {
			// Prevent the VRS attachment from being added to the color_attachments.
		}
		else {
			if (texture && texture->is_resolve_buffer) {
				pass.resolve_attachments.push_back(i);
			}
			else {
				pass.color_attachments.push_back(texture ? i : RDC::ATTACHMENT_UNUSED);
			}
		}
	}

	Vector<FramebufferPass> passes;
	passes.push_back(pass);

	return framebuffer_create_multipass(
		p_texture_attachments, passes, p_format_check, p_view_count);
}

RID RenderingDevice::framebuffer_create_multipass(const Vector<RID>& p_texture_attachments,
	const Vector<FramebufferPass>& p_passes, FramebufferFormatID p_format_check,
	uint32_t p_view_count)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	Vector<AttachmentFormat> attachments;
	LocalVector<RDD::TextureID> textures;
	LocalVector<RDG::ResourceTracker*> trackers;
	int32_t vrs_attachment = -1;
	attachments.resize(p_texture_attachments.size());
	Size2i size;
	bool size_set = false;
	for (int i = 0; i < p_texture_attachments.size(); i++) {
		AttachmentFormat af;
		Texture* texture = data->texture_owner.get_or_null(p_texture_attachments[i]);
		if (!texture) {
			af.usage_flags = AttachmentFormat::UNUSED_ATTACHMENT;
			trackers.push_back(nullptr);
		}
		else {
			ERR_FAIL_COND_V_MSG(texture->layers != p_view_count, RID(),
				"Layers of our texture doesn't match view count for this framebuffer");

			_check_transfer_worker_texture(texture);

			if (i != 0 && texture->usage_flags & RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT) {
				// Detect if the texture is the fragment density map and it's not the first
				// attachment.
				vrs_attachment = i;
			}

			if (!size_set) {
				size.width = texture->width;
				size.height = texture->height;
				size_set = true;
			}
			else if (texture->usage_flags & RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT) {
				// If this is not the first attachment we assume this is used as the VRS attachment.
				// In this case this texture will be 1/16th the size of the color attachment.
				// So we skip the size check.
			}
			else {
				ERR_FAIL_COND_V_MSG((uint32_t)size.width != texture->width ||
										(uint32_t)size.height != texture->height,
					RID(), "All textures in a framebuffer should be the same size.");
			}

			af.format = texture->format;
			af.samples = texture->samples;
			af.usage_flags = texture->usage_flags;

			_texture_make_mutable(texture, p_texture_attachments[i]);

			textures.push_back(texture->driver_id);
			trackers.push_back(texture->draw_tracker);
		}
		attachments.write[i] = af;
	}

	ERR_FAIL_COND_V_MSG(!size_set, RID(), "All attachments unused.");

	FramebufferFormatID format_id =
		framebuffer_format_create_multipass(attachments, p_passes, p_view_count, vrs_attachment);
	if (format_id == RDC::INVALID_ID) {
		return RID();
	}

	ERR_FAIL_COND_V_MSG(p_format_check != RDC::INVALID_ID && format_id != p_format_check, RID(),
		"The format used to check this framebuffer differs from the intended framebuffer format.");

	Framebuffer framebuffer;
	framebuffer.format_id = format_id;
	framebuffer.texture_ids = p_texture_attachments;
	framebuffer.size = size;
	framebuffer.view_count = p_view_count;

	RDG::FramebufferCache* framebuffer_cache = RDG::framebuffer_cache_create();
	framebuffer_cache->width = size.width;
	framebuffer_cache->height = size.height;
	framebuffer_cache->textures = textures;
	framebuffer_cache->trackers = trackers;
	framebuffer.framebuffer_cache = framebuffer_cache;

	RID id = data->framebuffer_owner.make_rid(framebuffer);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif

	for (int i = 0; i < p_texture_attachments.size(); i++) {
		if (p_texture_attachments[i].is_valid()) {
			_add_dependency(id, p_texture_attachments[i]);
		}
	}

	// This relies on the fact that HashMap will not change the address of an object after it's been
	// inserted into the container.
	framebuffer_cache->render_pass_creation_user_data =
		(void*)(&data->framebuffer_formats[framebuffer.format_id].E->key());

	return id;
}

RenderingDevice::FramebufferFormatID RenderingDevice::framebuffer_get_format(RID p_framebuffer)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	Framebuffer* framebuffer = data->framebuffer_owner.get_or_null(p_framebuffer);
	ERR_FAIL_NULL_V(framebuffer, RDC::INVALID_ID);

	return framebuffer->format_id;
}

Size2 RenderingDevice::framebuffer_get_size(RID p_framebuffer)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	Framebuffer* framebuffer = data->framebuffer_owner.get_or_null(p_framebuffer);
	ERR_FAIL_NULL_V(framebuffer, Size2(0, 0));

	return framebuffer->size;
}

bool RenderingDevice::framebuffer_is_valid(RID p_framebuffer)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	return data->framebuffer_owner.owns(p_framebuffer);
}

void RenderingDevice::framebuffer_set_invalidation_callback(
	RID p_framebuffer, InvalidationCallback p_callback, void* p_userdata)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	Framebuffer* framebuffer = data->framebuffer_owner.get_or_null(p_framebuffer);
	ERR_FAIL_NULL(framebuffer);

	framebuffer->invalidated_callback = p_callback;
	framebuffer->invalidated_callback_userdata = p_userdata;
}

RID RenderingDevice::sampler_create(const RDC::SamplerState& p_state)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	ERR_FAIL_INDEX_V(p_state.repeat_u, RDC::SAMPLER_REPEAT_MODE_MAX, RID());
	ERR_FAIL_INDEX_V(p_state.repeat_v, RDC::SAMPLER_REPEAT_MODE_MAX, RID());
	ERR_FAIL_INDEX_V(p_state.repeat_w, RDC::SAMPLER_REPEAT_MODE_MAX, RID());
	ERR_FAIL_INDEX_V(p_state.compare_op, RDC::COMPARE_OP_MAX, RID());
	ERR_FAIL_INDEX_V(p_state.border_color, RDC::SAMPLER_BORDER_COLOR_MAX, RID());

	RDD::SamplerID sampler = data->driver->sampler_create(p_state);
	ERR_FAIL_COND_V(!sampler, RID());

	RID id = data->sampler_owner.make_rid(sampler);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	return id;
}

bool RenderingDevice::sampler_is_format_supported_for_filter(
	RDC::DataFormat p_format, RDC::SamplerFilter p_sampler_filter)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	ERR_FAIL_INDEX_V(p_format, RDC::DATA_FORMAT_MAX, false);

	return data->driver->sampler_is_format_supported_for_filter(p_format, p_sampler_filter);
}

// Internally reference counted, this ID is warranted to be unique for the same description, but
// needs to be freed as many times as it was allocated.
RenderingDevice::VertexFormatID RenderingDevice::vertex_format_create(
	const Vector<RDC::VertexAttribute>& p_vertex_descriptions)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	VertexDescriptionKey key;
	key.vertex_formats = p_vertex_descriptions;

	VertexFormatID* idptr = data->vertex_format_cache.getptr(key);
	if (idptr) {
		return *idptr;
	}

	RDC::VertexAttributeBindingsMap bindings;
	bool has_implicit = false;
	bool has_explicit = false;
	Vector<RDC::VertexAttribute> vertex_descriptions = p_vertex_descriptions;
	HashSet<int> used_locations;
	for (int i = 0; i < vertex_descriptions.size(); i++) {
		RDC::VertexAttribute& attr = vertex_descriptions.write[i];
		ERR_CONTINUE(attr.format >= RDC::DATA_FORMAT_MAX);
		ERR_FAIL_COND_V(used_locations.has(attr.location), RDC::INVALID_ID);

		ERR_FAIL_COND_V_MSG(RDC::get_format_vertex_size(attr.format) == 0, RDC::INVALID_ID,
			vformat("Data format for attribute (%d), '%s', is not valid for a vertex array.",
				attr.location, String(RDC::FORMAT_NAMES[attr.format])));

		if (attr.binding == UINT32_MAX) {
			attr.binding = i; // Implicitly assigned binding
			has_implicit = true;
		}
		else {
			has_explicit = true;
		}
		ERR_FAIL_COND_V_MSG(!(has_implicit ^ has_explicit), RDC::INVALID_ID,
			"Vertex attributes must use either all explicit or all implicit bindings.");

		const RDC::VertexAttributeBinding* existing = bindings.getptr(attr.binding);
		if (!existing) {
			bindings.insert(attr.binding, RDC::VertexAttributeBinding(attr.stride, attr.frequency));
		}
		else {
			ERR_FAIL_COND_V_MSG(existing->stride != attr.stride, RDC::INVALID_ID,
				vformat("Vertex attributes with binding (%d) have an inconsistent stride.",
					attr.binding));
			ERR_FAIL_COND_V_MSG(existing->frequency != attr.frequency, RDC::INVALID_ID,
				vformat("Vertex attributes with binding (%d) have an inconsistent frequency.",
					attr.binding));
		}

		used_locations.insert(attr.location);
	}

	RDD::VertexFormatID driver_id =
		data->driver->vertex_format_create(vertex_descriptions, bindings);
	ERR_FAIL_COND_V(!driver_id, RDC::INVALID_ID);

	VertexFormatID id =
		(data->vertex_format_cache.size() | ((int64_t)ID_TYPE_VERTEX_FORMAT << ID_BASE_SHIFT));
	data->vertex_format_cache[key] = id;
	VertexDescriptionCache& ce = data->vertex_formats.insert(id, VertexDescriptionCache())->value;
	ce.vertex_formats = vertex_descriptions;
	ce.bindings = std::move(bindings);
	ce.driver_id = driver_id;
	return id;
}

RID RenderingDevice::vertex_array_create(uint32_t p_vertex_count, VertexFormatID p_vertex_format,
	const Vector<RID>& p_src_buffers, const Vector<uint64_t>& p_offsets)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	ERR_FAIL_COND_V(!data->vertex_formats.has(p_vertex_format), RID());
	const VertexDescriptionCache& vd = data->vertex_formats[p_vertex_format];

	VertexArray vertex_array;

	if (p_offsets.is_empty()) {
		vertex_array.offsets.resize_initialized(p_src_buffers.size());
	}
	else {
		ERR_FAIL_COND_V(p_offsets.size() != p_src_buffers.size(), RID());
		vertex_array.offsets = p_offsets;
	}

	vertex_array.vertex_count = p_vertex_count;
	vertex_array.description = p_vertex_format;
	vertex_array.max_instances_allowed = 0xFFFFFFFF; // By default as many as you want.
	vertex_array.buffers.resize(p_src_buffers.size());

	HashSet<RID> unique_buffers;
	unique_buffers.reserve(p_src_buffers.size());

	for (const RDC::VertexAttribute& atf : vd.vertex_formats) {
		ERR_FAIL_COND_V_MSG(atf.binding >= p_src_buffers.size(), RID(),
			vformat("Vertex attribute location (%d) is missing a buffer for binding (%d).",
				atf.location, atf.binding));
		RID buf = p_src_buffers[atf.binding];
		ERR_FAIL_COND_V(!data->vertex_buffer_owner.owns(buf), RID());

		Buffer* buffer = data->vertex_buffer_owner.get_or_null(buf);

		// Validate with buffer.
		{
			uint32_t element_size = RDC::get_format_vertex_size(atf.format);
			ERR_FAIL_COND_V(
				element_size == 0, RID()); // Should never happen since this was prevalidated.

			if (atf.frequency == RDC::VERTEX_FREQUENCY_VERTEX) {
				// Validate size for regular drawing.
				uint64_t total_size =
					uint64_t(atf.stride) * (p_vertex_count - 1) + atf.offset + element_size;
				ERR_FAIL_COND_V_MSG(total_size > buffer->size, RID(),
					vformat("Vertex attribute (%d) will read past the end of the buffer.",
						atf.location));

			}
			else {
				// Validate size for instances drawing.
				uint64_t available = buffer->size - atf.offset;
				ERR_FAIL_COND_V_MSG(available < element_size, RID(),
					vformat("Vertex attribute (%d) uses instancing, but it's just too small.",
						atf.location));

				uint32_t instances_allowed = available / atf.stride;

				// The remaining space might be able to fit another instance.
				if ((available % atf.stride) >= element_size) {
					++instances_allowed;
				}

				vertex_array.max_instances_allowed =
					MIN(instances_allowed, vertex_array.max_instances_allowed);
			}
		}

		vertex_array.buffers.write[atf.binding] = buffer->driver_id;

		if (unique_buffers.has(buf)) {
			// No need to add dependencies multiple times.
			continue;
		}

		unique_buffers.insert(buf);

		if (buffer->draw_tracker != nullptr) {
			vertex_array.draw_trackers.push_back(buffer->draw_tracker);
		}
		else {
			vertex_array.untracked_buffers.insert(buf);
		}

		if (buffer->transfer_worker_index >= 0) {
			vertex_array.transfer_worker_indices.push_back(buffer->transfer_worker_index);
			vertex_array.transfer_worker_operations.push_back(buffer->transfer_worker_operation);
		}
	}

	RID id = data->vertex_array_owner.make_rid(vertex_array);
	for (const RID& buf : unique_buffers) {
		_add_dependency(id, buf);
	}

	return id;
}

RID RenderingDevice::index_array_create(
	RID p_index_buffer, uint32_t p_index_offset, uint32_t p_index_count)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	ERR_FAIL_COND_V(!data->index_buffer_owner.owns(p_index_buffer), RID());

	IndexBuffer* index_buffer = data->index_buffer_owner.get_or_null(p_index_buffer);

	ERR_FAIL_COND_V(p_index_count == 0, RID());
	ERR_FAIL_COND_V(p_index_offset + p_index_count > index_buffer->index_count, RID());

	IndexArray index_array;
	index_array.max_index = index_buffer->max_index;
	index_array.driver_id = index_buffer->driver_id;
	index_array.draw_tracker = index_buffer->draw_tracker;
	index_array.offset = p_index_offset;
	index_array.indices = p_index_count;
	index_array.format = index_buffer->format;
	index_array.supports_restart_indices = index_buffer->supports_restart_indices;
	index_array.transfer_worker_index = index_buffer->transfer_worker_index;
	index_array.transfer_worker_operation = index_buffer->transfer_worker_operation;

	RID id = data->index_array_owner.make_rid(index_array);
	_add_dependency(id, p_index_buffer);
	return id;
}

// Keep the values in sync with the `UniformType` enum (file rendering_device_commons.h).
static const char* SHADER_UNIFORM_NAMES[RDC::UNIFORM_TYPE_MAX] = {
	"Sampler",
	"CombinedSampler", // UNIFORM_TYPE_SAMPLER_WITH_TEXTURE
	"Texture",
	"Image",
	"TextureBuffer",
	"SamplerTextureBuffer",
	"ImageBuffer",
	"UniformBuffer",
	"StorageBuffer",
	"InputAttachment",
	"UniformBufferDynamic",
	"StorageBufferDynamic",
	"AccelerationStructure",
};

String RenderingDevice::_shader_uniform_debug(RID p_shader, int p_set)
{
	String ret;
	const Shader* shader = data->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, String());
	for (int i = 0; i < shader->uniform_sets.size(); i++) {
		if (p_set >= 0 && i != p_set) {
			continue;
		}
		for (int j = 0; j < shader->uniform_sets[i].size(); j++) {
			const RDC::ShaderUniform& ui = shader->uniform_sets[i][j];
			if (!ret.is_empty()) {
				ret += "\n";
			}
			ret += "Set: " + itos(i) + " Binding: " + itos(ui.binding) +
				   " Type: " + SHADER_UNIFORM_NAMES[ui.type] +
				   " Writable: " + (ui.writable ? "Y" : "N") + " Length: " + itos(ui.length);

			if (ui.texture_type != RDC::TEXTURE_TYPE_MAX) {
				ret += " Texture Type: ";
				ret += RDC::TEXTURE_TYPE_NAMES[ui.texture_type];
			}

			if (ui.texture_format != RDC::DATA_FORMAT_MAX) {
				ret += " Texture Format: ";
				ret += RDC::FORMAT_NAMES[ui.texture_format];
			}
		}
	}
	return ret;
}

Vector<uint8_t> RenderingDevice::shader_compile_binary_from_spirv(
	const Vector<RDC::ShaderStageSPIRVData>& p_spirv, const String& p_shader_name)
{
	const RenderingShaderContainerFormat& container_format =
		data->driver->get_shader_container_format();
	Ref<RenderingShaderContainer> shader_container = container_format.create_container();
	ERR_FAIL_COND_V(shader_container.is_null(), Vector<uint8_t>());

	// Compile shader binary from SPIR-V.
	bool code_compiled = shader_container->set_code_from_spirv(p_shader_name, p_spirv);
	ERR_FAIL_COND_V_MSG(
		!code_compiled, Vector<uint8_t>(), vformat("Failed to compile code to native for SPIR-V."));

	return shader_container->to_bytes();
}

void RenderingDevice::shader_destroy_modules(RID p_shader)
{
	Shader* shader = data->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);
	data->driver->shader_destroy_modules(shader->driver_id);
}

RID RenderingDevice::shader_create_placeholder()
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	Shader shader;
	return data->shader_owner.make_rid(shader);
}

uint64_t RenderingDevice::shader_get_vertex_input_attribute_mask(RID p_shader)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	const Shader* shader = data->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, 0);
	return shader->vertex_input_mask;
}

RID RenderingDevice::uniform_set_create(
	const VectorView<Uniform>& p_uniforms, RID p_shader, uint32_t p_shader_set, bool p_linear_pool)
{
	ERR_RENDER_THREAD_GUARD_V(RID());

	RDD::ShaderID shader_driver_id;
	uint32_t set_format = 0;
	{
		MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);
		const Shader* shader = data->shader_owner.get_or_null(p_shader);
		ERR_FAIL_NULL_V(shader, RID());
		ERR_FAIL_INDEX_V(p_shader_set, (uint32_t)shader->set_formats.size(), RID());
		shader_driver_id = shader->driver_id;
		set_format = shader->set_formats[p_shader_set];
	}

	UniformSet uniform_set;
	uniform_set.format = set_format;
	uniform_set.shader_id = p_shader;
	uniform_set.shader_set = p_shader_set;

	LocalVector<RDD::BoundUniform> bound_uniforms;
	bound_uniforms.resize(p_uniforms.size());

	Vector<RID> referenced_resources;

	for (uint32_t i = 0; i < p_uniforms.size(); i++) {
		const Uniform& u = p_uniforms[i];
		RDD::BoundUniform& bu = bound_uniforms[i];

		bu.binding = u.binding;
		bu.type = u.uniform_type;
		bu.immutable_sampler = u.immutable_sampler;

		switch (u.uniform_type) {
		case RDC::UNIFORM_TYPE_SAMPLER: {
			uint32_t id_count = u.get_id_count();
			for (uint32_t j = 0; j < id_count; j++) {
				RID s_rid = u.get_id(j);
				const RDD::SamplerID* sampler = data->sampler_owner.get_or_null(s_rid);
				ERR_FAIL_NULL_V(sampler, RID());
				bu.ids.push_back(*sampler);
				referenced_resources.push_back(s_rid);
			}
		} break;

		case RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
			uint32_t id_count = u.get_id_count();
			ERR_FAIL_COND_V(id_count % 2 != 0, RID());
			for (uint32_t j = 0; j < id_count; j += 2) {
				RID sampler_rid = u.get_id(j + 0);
				const RDD::SamplerID* sampler = data->sampler_owner.get_or_null(sampler_rid);
				ERR_FAIL_NULL_V(sampler, RID());
				bu.ids.push_back(*sampler);
				referenced_resources.push_back(sampler_rid);

				RID texture_rid = u.get_id(j + 1);
				Texture* texture = data->texture_owner.get_or_null(texture_rid);
				ERR_FAIL_NULL_V(texture, RID());
				bu.ids.push_back(texture->driver_id);
				referenced_resources.push_back(texture_rid);

				_texture_make_mutable(texture, texture_rid);
				if (texture->draw_tracker != nullptr) {
					uniform_set.draw_trackers.push_back(texture->draw_tracker);
					uniform_set.draw_trackers_usage.push_back(RDG::RESOURCE_USAGE_TEXTURE_SAMPLE);
				}

#ifdef DEBUG_ENABLED
				UniformSet::AttachableTexture at;
				at.bind = u.binding;
				at.texture = texture_rid;
				uniform_set.attachable_textures.push_back(at);
#endif
			}
		} break;

		case RDC::UNIFORM_TYPE_TEXTURE: {
			uint32_t id_count = u.get_id_count();
			for (uint32_t j = 0; j < id_count; j++) {
				RID texture_rid = u.get_id(j);
				Texture* texture = data->texture_owner.get_or_null(texture_rid);
				ERR_FAIL_NULL_V(texture, RID());
				bu.ids.push_back(texture->driver_id);
				referenced_resources.push_back(texture_rid);

				_texture_make_mutable(texture, texture_rid);
				if (texture->draw_tracker != nullptr) {
					uniform_set.draw_trackers.push_back(texture->draw_tracker);
					uniform_set.draw_trackers_usage.push_back(RDG::RESOURCE_USAGE_TEXTURE_SAMPLE);
				}

#ifdef DEBUG_ENABLED
				UniformSet::AttachableTexture at;
				at.bind = u.binding;
				at.texture = texture_rid;
				uniform_set.attachable_textures.push_back(at);
#endif
			}
		} break;

		case RDC::UNIFORM_TYPE_IMAGE: {
			uint32_t id_count = u.get_id_count();
			for (uint32_t j = 0; j < id_count; j++) {
				RID texture_rid = u.get_id(j);
				Texture* texture = data->texture_owner.get_or_null(texture_rid);
				ERR_FAIL_NULL_V(texture, RID());
				bu.ids.push_back(texture->driver_id);
				referenced_resources.push_back(texture_rid);

				_texture_make_mutable(texture, texture_rid);
				if (texture->draw_tracker != nullptr) {
					uniform_set.draw_trackers.push_back(texture->draw_tracker);
					uniform_set.draw_trackers_usage.push_back(
						RDG::RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE);
				}
			}
		} break;

		case RDC::UNIFORM_TYPE_TEXTURE_BUFFER: {
			uint32_t id_count = u.get_id_count();
			for (uint32_t j = 0; j < id_count; j++) {
				RID b_rid = u.get_id(j);
				const Buffer* buffer = data->texture_buffer_owner.get_or_null(b_rid);
				ERR_FAIL_NULL_V(buffer, RID());
				bu.ids.push_back(buffer->driver_id);
				referenced_resources.push_back(b_rid);
			}
		} break;

		case RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER: {
			uint32_t id_count = u.get_id_count();
			ERR_FAIL_COND_V(id_count % 2 != 0, RID());
			for (uint32_t j = 0; j < id_count; j += 2) {
				RID sampler_rid = u.get_id(j + 0);
				const RDD::SamplerID* sampler = data->sampler_owner.get_or_null(sampler_rid);
				ERR_FAIL_NULL_V(sampler, RID());
				bu.ids.push_back(*sampler);
				referenced_resources.push_back(sampler_rid);

				RID buffer_rid = u.get_id(j + 1);
				const Buffer* buffer = data->texture_buffer_owner.get_or_null(buffer_rid);
				ERR_FAIL_NULL_V(buffer, RID());
				bu.ids.push_back(buffer->driver_id);
				referenced_resources.push_back(buffer_rid);
			}
		} break;

		case RDC::UNIFORM_TYPE_UNIFORM_BUFFER: {
			uint32_t id_count = u.get_id_count();
			for (uint32_t j = 0; j < id_count; j++) {
				RID buffer_rid = u.get_id(j);
				Buffer* buffer = data->uniform_buffer_owner.get_or_null(buffer_rid);
				ERR_FAIL_NULL_V(buffer, RID());
				bu.ids.push_back(buffer->driver_id);
				referenced_resources.push_back(buffer_rid);

				_buffer_make_mutable(buffer, buffer_rid);
				if (buffer->draw_tracker != nullptr) {
					uniform_set.draw_trackers.push_back(buffer->draw_tracker);
					uniform_set.draw_trackers_usage.push_back(
						RDG::RESOURCE_USAGE_UNIFORM_BUFFER_READ);
				}
			}
		} break;

		case RDC::UNIFORM_TYPE_STORAGE_BUFFER: {
			uint32_t id_count = u.get_id_count();
			for (uint32_t j = 0; j < id_count; j++) {
				RID buffer_rid = u.get_id(j);
				Buffer* buffer = data->storage_buffer_owner.get_or_null(buffer_rid);
				ERR_FAIL_NULL_V(buffer, RID());
				bu.ids.push_back(buffer->driver_id);
				referenced_resources.push_back(buffer_rid);

				_buffer_make_mutable(buffer, buffer_rid);
				if (buffer->draw_tracker != nullptr) {
					uniform_set.draw_trackers.push_back(buffer->draw_tracker);
					uniform_set.draw_trackers_usage.push_back(
						RDG::RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE);
				}
			}
		} break;

		default: {
			ERR_FAIL_V_MSG(RID(), "Invalid uniform type: " + itos(u.uniform_type));
		} break;
		}
	}

	int linear_pool_index = p_linear_pool ? 0 : -1;
	uniform_set.driver_id = data->driver->uniform_set_create(
		bound_uniforms, shader_driver_id, p_shader_set, linear_pool_index);
	ERR_FAIL_COND_V(uniform_set.driver_id == RDD::UniformSetID(), RID());

	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);
	RID id = data->uniform_set_owner.make_rid(uniform_set);

	_add_dependency(id, p_shader);
	for (uint32_t i = 0; i < referenced_resources.size(); i++) {
		if (referenced_resources[i].is_valid()) {
			_add_dependency(id, referenced_resources[i]);
		}
	}

	return id;
}

void RenderingDevice::_uniform_set_update_shared(UniformSet* p_uniform_set)
{
	for (UniformSet::SharedTexture shared : p_uniform_set->shared_textures_to_update) {
		Texture* texture = data->texture_owner.get_or_null(shared.texture);
		ERR_CONTINUE(texture == nullptr);
		_texture_update_shared_fallback(shared.texture, texture, shared.writing);
	}
}

bool RenderingDevice::uniform_set_is_valid(RID p_uniform_set)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	return data->uniform_set_owner.owns(p_uniform_set);
}

void RenderingDevice::uniform_set_set_invalidation_callback(
	RID p_uniform_set, InvalidationCallback p_callback, void* p_userdata)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	UniformSet* us = data->uniform_set_owner.get_or_null(p_uniform_set);
	ERR_FAIL_NULL(us);
	us->invalidated_callback = p_callback;
	us->invalidated_callback_userdata = p_userdata;
}

bool RenderingDevice::uniform_sets_have_linear_pools()
{
	return data->driver->uniform_sets_have_linear_pools();
}

bool RenderingDevice::render_pipeline_is_valid(RID p_pipeline)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	return data->render_pipeline_owner.owns(p_pipeline);
}

bool RenderingDevice::compute_pipeline_is_valid(RID p_pipeline)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	return data->compute_pipeline_owner.owns(p_pipeline);
}

Error RenderingDevice::_raytracing_pipeline_create_sbt_buffer(
	RDD::RaytracingPipelineID p_raytracing_pipeline, uint32_t p_raygen_shader_count,
	uint32_t p_miss_shader_count, Buffer& r_sbt_buffer)
{
	// Ray generation and miss shader group handles are placed next to each other in the same
	// buffer.

	uint32_t shader_group_handle_size =
		data->driver->api_trait_get(RDD::API_TRAIT_SHADER_GROUP_HANDLE_SIZE);
	uint32_t shader_group_handle_alignment =
		data->driver->api_trait_get(RDD::API_TRAIT_SHADER_GROUP_HANDLE_ALIGNMENT);
	uint32_t shader_group_base_alignment =
		data->driver->api_trait_get(RDD::API_TRAIT_SHADER_GROUP_BASE_ALIGNMENT);

	uint32_t shader_group_stride = STEPIFY(shader_group_handle_size, shader_group_handle_alignment);
	uint32_t raygen_shader_group_stride = STEPIFY(shader_group_stride, shader_group_base_alignment);

	// Make sure to reserve a full base-aligned slot for every raygen shader.
	uint32_t raygen_sbt_size = p_raygen_shader_count * raygen_shader_group_stride;
	uint32_t miss_sbt_offset = STEPIFY(raygen_sbt_size, shader_group_base_alignment);
	// Reserve handle-aligned slots for miss shaders.
	uint32_t miss_sbt_size = p_miss_shader_count * shader_group_stride;

	r_sbt_buffer.size = miss_sbt_offset + miss_sbt_size;
	r_sbt_buffer.driver_id = data->driver->buffer_create(r_sbt_buffer.size,
		RDD::BUFFER_USAGE_TRANSFER_TO_BIT | RDD::
BUFFER_USAGE_SHADER_BINDING_TABLE_BIT |
			RDD::BUFFER_USAGE_DEVICE_ADDRESS_BIT,
		RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	ERR_FAIL_COND_V(!r_sbt_buffer.driver_id, ERR_CANT_CREATE);

	thread_local LocalVector<uint8_t> sbt_data;
	sbt_data.resize(r_sbt_buffer.size);

	thread_local LocalVector<uint32_t> raygen_shader_group_indices;
	raygen_shader_group_indices.resize(p_raygen_shader_count);
	for (uint32_t i = 0; i < p_raygen_shader_count; i++) {
		raygen_shader_group_indices[i] = i;
	}
	bool success = data->driver->raytracing_pipeline_get_shader_group_handles(p_raytracing_pipeline,

		0, VectorView<uint32_t>(raygen_shader_group_indices), sbt_data.ptr(),
		raygen_shader_group_stride);
	if (unlikely(!success)) {
		data->driver->buffer_free(r_sbt_buffer.driver_id);
		ERR_FAIL_V(ERR_CANT_CREATE);
	}

	thread_local LocalVector<uint32_t> miss_shader_group_indices;
	miss_shader_group_indices.resize(p_miss_shader_count);
	for (uint32_t i = 0; i < p_miss_shader_count; i++) {
		miss_shader_group_indices[i] = i;
	}
	success = data->driver->raytracing_pipeline_get_shader_group_handles(p_raytracing_pipeline,
		p_raygen_shader_count, VectorView<uint32_t>(miss_shader_group_indices),
		sbt_data.ptr() + miss_sbt_offset, shader_group_stride);
	if (unlikely(!success)) {
		data->driver->buffer_free(r_sbt_buffer.driver_id);
		ERR_FAIL_V(ERR_CANT_CREATE);
	}

	Error err = _buffer_initialize(&r_sbt_buffer, sbt_data);
	if (unlikely(err != OK)) {
		data->driver->buffer_free(r_sbt_buffer.driver_id);
		ERR_FAIL_V(err);
	}

	data->buffer_memory.add(r_sbt_buffer.size);

	return OK;
}

bool RenderingDevice::raytracing_pipeline_is_valid(RID p_pipeline)
{
	return data->raytracing_pipeline_owner.owns(p_pipeline);
}

uint32_t RenderingDevice::_get_swap_chain_desired_count()
{
	return MAX(2U, uint32_t(GLOBAL_GET_CACHED(
					   uint32_t, "rendering/rendering_device/vsync/swapchain_image_count")));
}

Error RenderingDevice::screen_create(DisplayServerEnums::WindowID p_screen)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	RenderingContextDriver::SurfaceID surface = data->context->surface_get_from_window(p_screen);
	ERR_FAIL_COND_V_MSG(surface == 0, ERR_CANT_CREATE, "A surface was not created for the screen.");

	HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID>::ConstIterator it =
		data->screen_swap_chains.find(p_screen);
	ERR_FAIL_COND_V_MSG(it != data->screen_swap_chains.end(), ERR_CANT_CREATE,
		"A swap chain was already created for the screen.");

	RDD::SwapChainID swap_chain = data->driver->swap_chain_create(surface);
	ERR_FAIL_COND_V_MSG(swap_chain.id == 0, ERR_CANT_CREATE, "Unable to create swap chain.");

	data->screen_swap_chains[p_screen] = swap_chain;

	return OK;
}

int RenderingDevice::screen_get_width(DisplayServerEnums::WindowID p_screen)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	RenderingContextDriver::SurfaceID surface = data->context->surface_get_from_window(p_screen);
	ERR_FAIL_COND_V_MSG(surface == 0, 0, "A surface was not created for the screen.");
	return data->context->surface_get_width(surface);
}

int RenderingDevice::screen_get_height(DisplayServerEnums::WindowID p_screen)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	RenderingContextDriver::SurfaceID surface = data->context->surface_get_from_window(p_screen);
	ERR_FAIL_COND_V_MSG(surface == 0, 0, "A surface was not created for the screen.");
	return data->context->surface_get_height(surface);
}

int RenderingDevice::screen_get_pre_rotation_degrees(DisplayServerEnums::WindowID p_screen)

{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID>::ConstIterator it =
		data->screen_swap_chains.find(p_screen);
	ERR_FAIL_COND_V_MSG(it == data->screen_swap_chains.end(), ERR_CANT_CREATE,
		"A swap chain was not created for the screen.");

	return data->driver->swap_chain_get_pre_rotation_degrees(it->value);
}

RenderingDevice::FramebufferFormatID RenderingDevice::screen_get_framebuffer_format(
	DisplayServerEnums::WindowID p_screen)
{
	RDC::DataFormat format;
	{
		MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);
		HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID>::ConstIterator it =
			data->screen_swap_chains.find(p_screen);
		ERR_FAIL_COND_V_MSG(
			it == data->screen_swap_chains.end(), RDC::INVALID_ID, "Screen was never prepared.");
		format = data->driver->swap_chain_get_format(it->value);
		ERR_FAIL_COND_V(format == RDC::DATA_FORMAT_MAX, RDC::INVALID_ID);
	}
	AttachmentFormat attachment;
	attachment.format = format;
	attachment.samples = RDC::TEXTURE_SAMPLES_1;
	attachment.usage_flags = RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
	Vector<AttachmentFormat> screen_attachment;
	screen_attachment.push_back(attachment);
	return framebuffer_format_create(screen_attachment);
}

RDC::ColorSpace RenderingDevice::screen_get_color_space(DisplayServerEnums::WindowID p_screen)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID>::ConstIterator it =
		data->screen_swap_chains.find(p_screen);
	ERR_FAIL_COND_V_MSG(
		it == data->screen_swap_chains.end(), RDC::COLOR_SPACE_MAX, "Screen was never prepared.");

	RDC::ColorSpace color_space = data->driver->swap_chain_get_color_space(it->value);
	ERR_FAIL_COND_V_MSG(
		color_space == RDC::COLOR_SPACE_MAX, RDC::COLOR_SPACE_MAX, "Unknown color space.");
	return color_space;
}

bool RenderingDevice::screen_get_hdr_output_supported(DisplayServerEnums::WindowID p_screen)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID>::ConstIterator it =
		data->screen_swap_chains.find(p_screen);
	ERR_FAIL_COND_V_MSG(it == data->screen_swap_chains.end(), false, "Screen was never prepared.");

	return data->driver->swap_chain_get_hdr_output_supported(it->value);
}

RenderingDevice::DrawListID RenderingDevice::draw_list_begin_for_screen(
	DisplayServerEnums::WindowID p_screen, const Color& p_clear_color)
{
	ERR_RENDER_THREAD_GUARD_V(RDC::INVALID_ID);

	ERR_FAIL_COND_V_MSG(data->draw_list.active, RDC::INVALID_ID,
		"Only one draw list can be active at the same time.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, RDC::INVALID_ID,
		"Only one draw/compute list can be active at the same time.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, RDC::INVALID_ID,
		"Only one draw/raytracing list can be active at the same time.");

	RenderingContextDriver::SurfaceID surface = data->context->surface_get_from_window(p_screen);
	HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID>::ConstIterator sc_it =
		data->screen_swap_chains.find(p_screen);
	HashMap<DisplayServerEnums::WindowID, RDD::FramebufferID>::ConstIterator fb_it =
		data->screen_framebuffers.find(p_screen);

	ERR_FAIL_COND_V_MSG(surface == 0, RDC::INVALID_ID, "A surface was not created for the screen.");
	ERR_FAIL_COND_V_MSG(
		sc_it == data->screen_swap_chains.end(), RDC::INVALID_ID, "Screen was never prepared.");
	ERR_FAIL_COND_V_MSG(fb_it == data->screen_framebuffers.end(), RDC::INVALID_ID,
		"Framebuffer was never prepared.");

	Rect2i viewport = Rect2i(0, 0, data->context->surface_get_width(surface),
		data->context->surface_get_height(surface));

	data->draw_list_bound_textures.clear();
	_draw_list_start(viewport);

#ifdef DEBUG_ENABLED
	data->draw_list_framebuffer_format = screen_get_framebuffer_format(p_screen);
#endif
	data->draw_list_subpass_count = 1;
	data->draw_list_current_subpass = 0;

	RDD::RenderPassClearValue clear_value;
	clear_value.color = p_clear_color;

	RDD::RenderPassID render_pass = data->driver->swap_chain_get_render_pass(sc_it->value);
	data->draw_graph.add_draw_list_begin(render_pass, fb_it->value, viewport,
		RDG::ATTACHMENT_OPERATION_CLEAR, clear_value,
		RDD::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, RDC::BreadcrumbMarker::BLIT_PASS,
		data->split_swapchain_into_its_own_cmd_buffer);

	data->draw_graph.add_draw_list_set_viewport(viewport);
	data->draw_graph.add_draw_list_set_scissor(viewport);

	return int64_t(ID_TYPE_DRAW_LIST) << ID_BASE_SHIFT;
}

void RenderingDevice::draw_list_set_blend_constants(DrawListID p_list, const Color& p_color)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

	data->draw_graph.add_draw_list_set_blend_constants(p_color);
}

void RenderingDevice::draw_list_bind_render_pipeline(DrawListID p_list, RID p_render_pipeline)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

	const RenderPipeline* pipeline = data->render_pipeline_owner.get_or_null(p_render_pipeline);
	ERR_FAIL_NULL(pipeline);
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND(pipeline->validation.framebuffer_format != data->draw_list_framebuffer_format &&
				  pipeline->validation.render_pass != data->draw_list_current_subpass);
#endif

	if (p_render_pipeline == data->draw_list.state.pipeline) {
		return; // Redundant state, return.
	}

	data->draw_list.state.pipeline = p_render_pipeline;

	data->draw_graph.add_draw_list_bind_pipeline(pipeline->driver_id, pipeline->stage_bits);

	if (data->draw_list.state.pipeline_shader != pipeline->shader) {
		// Shader changed, so descriptor sets may become incompatible.

		uint32_t pcount = pipeline->set_formats.size(); // Formats count in this pipeline.
		data->draw_list.state.set_count = MAX(data->draw_list.state.set_count, pcount);
		const uint32_t* pformats = pipeline->set_formats.ptr(); // Pipeline set formats.

		uint32_t first_invalid_set = UINT32_MAX; // All valid by default.
		if (pipeline->push_constant_size != data->draw_list.state.pipeline_push_constant_size) {
			// All sets must be invalidated as the pipeline layout is not compatible if the push
			// constant range is different.
			data->draw_list.state.pipeline_push_constant_size = pipeline->push_constant_size;
			first_invalid_set = 0;
		}
		else {
			switch (data->driver->api_trait_get(RDD::API_TRAIT_SHADER_CHANGE_INVALIDATION)) {
			case RDD::SHADER_CHANGE_INVALIDATION_ALL_BOUND_UNIFORM_SETS: {
				first_invalid_set = 0;
			} break;
			case RDD::SHADER_CHANGE_INVALIDATION_INCOMPATIBLE_SETS_PLUS_CASCADE: {
				for (uint32_t i = 0; i < pcount; i++) {
					if (data->draw_list.state.sets[i].pipeline_expected_format != pformats[i]) {
						first_invalid_set = i;
						break;
					}
				}
			} break;
			case RDD::SHADER_CHANGE_INVALIDATION_ALL_OR_NONE_ACCORDING_TO_LAYOUT_HASH: {
				if (data->draw_list.state.pipeline_shader_layout_hash !=
					pipeline->shader_layout_hash) {
					first_invalid_set = 0;
				}
			} break;
			}
		}

		if (pipeline->push_constant_size) {
#ifdef DEBUG_ENABLED
			data->draw_list.validation.pipeline_push_constant_supplied = false;
#endif
		}

		for (uint32_t i = 0; i < pcount; i++) {
			data->draw_list.state.sets[i].bound =
				data->draw_list.state.sets[i].bound && i < first_invalid_set;
			data->draw_list.state.sets[i].pipeline_expected_format = pformats[i];
		}

		for (uint32_t i = pcount; i < data->draw_list.state.set_count; i++) {
			// Unbind the ones above (not used) if exist.
			data->draw_list.state.sets[i].bound = false;
		}

		data->draw_list.state.set_count = pcount; // Update set count.

		data->draw_list.state.pipeline_shader = pipeline->shader;
		data->draw_list.state.pipeline_shader_driver_id = pipeline->shader_driver_id;
		data->draw_list.state.pipeline_shader_layout_hash = pipeline->shader_layout_hash;
	}

#ifdef DEBUG_ENABLED
	// Update render pass pipeline info.
	data->draw_list.validation.pipeline_active = true;
	data->draw_list.validation.pipeline_dynamic_state = pipeline->validation.dynamic_state;
	data->draw_list.validation.pipeline_vertex_format = pipeline->validation.vertex_format;
	data->draw_list.validation.pipeline_uses_restart_indices =
		pipeline->validation.uses_restart_indices;
	data->draw_list.validation.pipeline_primitive_divisor = pipeline->validation.primitive_divisor;
	data->draw_list.validation.pipeline_primitive_minimum = pipeline->validation.primitive_minimum;
	data->draw_list.validation.pipeline_push_constant_size = pipeline->push_constant_size;
#endif
}

void RenderingDevice::draw_list_bind_uniform_set(
	DrawListID p_list, RID p_uniform_set, uint32_t p_index)
{
	ERR_RENDER_THREAD_GUARD();

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(p_index >= data->driver->limit_get(RDC::LIMIT_MAX_BOUND_UNIFORM_SETS) ||
						  p_index >= MAX_UNIFORM_SETS,
		"Attempting to bind a descriptor set (" + itos(p_index) +
			") greater than what the hardware supports (" +
			itos(data->driver->limit_get(RDC::LIMIT_MAX_BOUND_UNIFORM_SETS)) + ").");
#endif

	ERR_FAIL_COND(!data->draw_list.active);

	const UniformSet* uniform_set = data->uniform_set_owner.get_or_null(p_uniform_set);
	ERR_FAIL_NULL(uniform_set);

	data->draw_list.state.set_count = MAX(data->draw_list.state.set_count, p_index + 1);

	data->draw_list.state.sets[p_index].uniform_set_driver_id =
		uniform_set->driver_id;						   // Update set pointer.
	data->draw_list.state.sets[p_index].bound = false; // Needs rebind.
	data->draw_list.state.sets[p_index].uniform_set_format = uniform_set->format;
	data->draw_list.state.sets[p_index].uniform_set = p_uniform_set;

#ifdef DEBUG_ENABLED
	{ // Validate that textures bound are not attached as framebuffer bindings.
		uint32_t attachable_count = uniform_set->attachable_textures.size();
		const UniformSet::AttachableTexture* attachable_ptr =
			uniform_set->attachable_textures.ptr();
		uint32_t bound_count = data->draw_list_bound_textures.size();
		const RID* bound_ptr = data->draw_list_bound_textures.ptr();
		for (uint32_t i = 0; i < attachable_count; i++) {
			for (uint32_t j = 0; j < bound_count; j++) {
				ERR_FAIL_COND_MSG(attachable_ptr[i].texture == bound_ptr[j],
					"Attempted to use the same texture in framebuffer attachment and a uniform "
					"(set: " +
						itos(p_index) + ", binding: " + itos(attachable_ptr[i].bind) +
						"), this is not allowed.");
			}
		}
	}
#endif
}

void RenderingDevice::draw_list_set_line_width(DrawListID p_list, float p_width)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

	data->draw_graph.add_draw_list_set_line_width(p_width);
}

void RenderingDevice::draw_list_set_push_constant(
	DrawListID p_list, const void* p_data, uint32_t p_data_size)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(p_data_size != data->draw_list.validation.pipeline_push_constant_size,
		"This render pipeline requires (" +
			itos(data->draw_list.validation.pipeline_push_constant_size) +
			") bytes of push constant data, supplied: (" + itos(p_data_size) + ")");
#endif

	data->draw_graph.add_draw_list_set_push_constant(
		data->draw_list.state.pipeline_shader_driver_id, p_data, p_data_size);

#ifdef DEBUG_ENABLED
	data->draw_list.validation.pipeline_push_constant_supplied = true;
#endif
}

void RenderingDevice::draw_list_draw(
	DrawListID p_list, bool p_use_indices, uint32_t p_instances, uint32_t p_procedural_vertices)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(!data->draw_list.validation.pipeline_active,
		"No render pipeline was set before attempting to draw.");
	if (data->draw_list.validation.pipeline_vertex_format != RDC::INVALID_ID) {
		// Pipeline uses vertices, validate format.
		ERR_FAIL_COND_MSG(data->draw_list.validation.vertex_format == RDC::INVALID_ID,
			"No vertex array was bound, and render pipeline expects vertices.");
		// Make sure format is right.
		ERR_FAIL_COND_MSG(data->draw_list.validation.pipeline_vertex_format !=
							  data->draw_list.validation.vertex_format,
			"The vertex format used to create the pipeline does not match the vertex format "
			"bound.");
		// Make sure number of instances is valid.
		ERR_FAIL_COND_MSG(p_instances > data->draw_list.validation.vertex_max_instances_allowed,
			"Number of instances requested (" + itos(p_instances) +
				") is larger than the maximum number supported by the bound vertex array (" +
				itos(data->draw_list.validation.vertex_max_instances_allowed) + ").");
	}

	if (data->draw_list.validation.pipeline_push_constant_size > 0) {
		// Using push constants, check that they were supplied.
		ERR_FAIL_COND_MSG(!data->draw_list.validation.pipeline_push_constant_supplied,
			"The shader in this pipeline requires a push constant to be set before drawing, but "
			"it's not present.");
	}

	for (uint32_t i = 0; i < data->draw_list.state.set_count; i++) {
		if (data->draw_list.state.sets[i].pipeline_expected_format == 0) {
			// Nothing expected by this pipeline.
			continue;
		}

		if (data->draw_list.state.sets[i].pipeline_expected_format !=
			data->draw_list.state.sets[i].uniform_set_format) {
			if (data->draw_list.state.sets[i].uniform_set_format == 0) {
				ERR_FAIL_MSG(vformat("Uniforms were never supplied for set (%d) at the time of "
									 "drawing, which are required by the pipeline.",
					i));
			}
			else if (data->uniform_set_owner.owns(data->draw_list.state.sets[i].uniform_set)) {
				UniformSet* us =
					data->uniform_set_owner.get_or_null(data->draw_list.state.sets[i].uniform_set);
				const String us_info =
					us ? vformat(
							 "(%d):\n%s\n", i, _shader_uniform_debug(us->shader_id, us->shader_set))
					   : vformat("(%d, which was just freed) ", i);
				ERR_FAIL_MSG(vformat(
					"Uniforms supplied for set %sare not the same format as required by the "
					"pipeline shader. Pipeline shader requires the following bindings:\n%s",
					us_info, _shader_uniform_debug(data->draw_list.state.pipeline_shader)));
			}
			else {
				ERR_FAIL_MSG(vformat("Uniforms supplied for set (%d, which was just freed) are not "
									 "the same format as required by the pipeline shader. Pipeline "
									 "shader requires the following bindings:\n%s",
					i, _shader_uniform_debug(data->draw_list.state.pipeline_shader)));
			}
		}
	}
#endif
	thread_local LocalVector<RDD::UniformSetID> valid_descriptor_ids;
	valid_descriptor_ids.clear();
	valid_descriptor_ids.resize(data->draw_list.state.set_count);
	uint32_t valid_set_count = 0;
	uint32_t first_set_index = 0;
	uint32_t last_set_index = 0;
	bool found_first_set = false;

	for (uint32_t i = 0; i < data->draw_list.state.set_count; i++) {
		if (data->draw_list.state.sets[i].pipeline_expected_format == 0) {
			continue; // Nothing expected by this pipeline.
		}

		if (!data->draw_list.state.sets[i].bound && !found_first_set) {
			first_set_index = i;
			found_first_set = true;
		}
		// Prepare descriptor sets if the API doesn't use pipeline barriers.
		if (!data->driver->api_trait_get(RDD::API_TRAIT_HONORS_PIPELINE_BARRIERS)) {
			data->draw_graph.add_draw_list_uniform_set_prepare_for_use(
				data->draw_list.state.pipeline_shader_driver_id,
				data->draw_list.state.sets[i].uniform_set_driver_id, i);
		}
	}

	// Bind descriptor sets.
	for (uint32_t i = first_set_index; i < data->draw_list.state.set_count; i++) {
		if (data->draw_list.state.sets[i].pipeline_expected_format == 0) {
			continue; // Nothing expected by this pipeline.
		}

		if (!data->draw_list.state.sets[i].bound) {
			UniformSet* uniform_set =
				data->uniform_set_owner.get_or_null(data->draw_list.state.sets[i].uniform_set);
			ERR_FAIL_NULL(uniform_set);
			_uniform_set_update_shared(uniform_set);

			data->draw_graph.add_draw_list_usages(
				uniform_set->draw_trackers, uniform_set->draw_trackers_usage);
			data->draw_list.state.sets[i].bound = true;

			// Batch contiguous descriptor sets in a single call.
			if (data->descriptor_set_batching) {
				// All good, see if this requires re-binding.
				if (i - last_set_index > 1) {
					data->draw_graph.add_draw_list_bind_uniform_sets(
						data->draw_list.state.pipeline_shader_driver_id, valid_descriptor_ids,
						first_set_index, valid_set_count);

					first_set_index = i;
					valid_set_count = 1;
					valid_descriptor_ids[0] = data->draw_list.state.sets[i].uniform_set_driver_id;
				}
				else {
					valid_descriptor_ids[valid_set_count] =
						data->draw_list.state.sets[i].uniform_set_driver_id;
					valid_set_count++;
				}

				last_set_index = i;
			}
			else {
				data->draw_graph.add_draw_list_bind_uniform_set(
					data->draw_list.state.pipeline_shader_driver_id,
					data->draw_list.state.sets[i].uniform_set_driver_id, i);
			}
		}
	}

	// Bind the remaining batch.
	if (data->descriptor_set_batching && valid_set_count > 0) {
		data->draw_graph.add_draw_list_bind_uniform_sets(
			data->draw_list.state.pipeline_shader_driver_id, valid_descriptor_ids, first_set_index,
			valid_set_count);
	}

	if (p_use_indices) {
#ifdef DEBUG_ENABLED
		ERR_FAIL_COND_MSG(
			p_procedural_vertices > 0, "Procedural vertices can't be used together with indices.");

		ERR_FAIL_COND_MSG(!data->draw_list.validation.index_array_count,
			"Draw command requested indices, but no index buffer was set.");

		ERR_FAIL_COND_MSG(data->draw_list.validation.pipeline_uses_restart_indices !=
							  data->draw_list.validation.index_buffer_uses_restart_indices,
			"The usage of restart indices in index buffer does not match the render primitive in "
			"the pipeline.");
#endif
		uint32_t to_draw = data->draw_list.validation.index_array_count;

#ifdef DEBUG_ENABLED
		ERR_FAIL_COND_MSG(to_draw < data->draw_list.validation.pipeline_primitive_minimum,
			"Too few indices (" + itos(to_draw) +
				") for the render primitive set in the render pipeline (" +
				itos(data->draw_list.validation.pipeline_primitive_minimum) + ").");

		ERR_FAIL_COND_MSG((to_draw % data->draw_list.validation.pipeline_primitive_divisor) != 0,
			"Index amount (" + itos(to_draw) +
				") must be a multiple of the amount of indices required by the render primitive (" +
				itos(data->draw_list.validation.pipeline_primitive_divisor) + ").");
#endif

		data->draw_graph.add_draw_list_draw_indexed(to_draw, p_instances, 0);
	}
	else {
		uint32_t to_draw;

		if (p_procedural_vertices > 0) {
			to_draw = p_procedural_vertices;
		}
		else {
#ifdef DEBUG_ENABLED
			ERR_FAIL_COND_MSG(data->draw_list.validation.pipeline_vertex_format == RDC::INVALID_ID,
				"Draw command lacks indices, but pipeline format does not use vertices.");
#endif
			to_draw = data->draw_list.validation.vertex_array_size;
		}

#ifdef DEBUG_ENABLED
		ERR_FAIL_COND_MSG(to_draw < data->draw_list.validation.pipeline_primitive_minimum,
			"Too few vertices (" + itos(to_draw) +
				") for the render primitive set in the render pipeline (" +
				itos(data->draw_list.validation.pipeline_primitive_minimum) + ").");

		ERR_FAIL_COND_MSG((to_draw % data->draw_list.validation.pipeline_primitive_divisor) != 0,
			"Vertex amount (" + itos(to_draw) +
				") must be a multiple of the amount of vertices required by the render primitive "
				"(" +
				itos(data->draw_list.validation.pipeline_primitive_divisor) + ").");
#endif

		data->draw_graph.add_draw_list_draw(to_draw, p_instances);
	}

	data->draw_list.state.draw_count++;
}

void RenderingDevice::draw_list_set_viewport(DrawListID p_list, const Rect2& p_rect)
{
	ERR_FAIL_COND(!data->draw_list.active);

	if (p_rect.get_area() == 0) {
		return;
	}

	data->draw_list.viewport = p_rect;
	data->draw_graph.add_draw_list_set_viewport(p_rect);
}

void RenderingDevice::draw_list_enable_scissor(DrawListID p_list, const Rect2& p_rect)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

	Rect2i rect = p_rect;
	rect.position += data->draw_list.viewport.position;

	rect = data->draw_list.viewport.intersection(rect);

	if (rect.get_area() == 0) {
		return;
	}

	data->draw_graph.add_draw_list_set_scissor(rect);
}

void RenderingDevice::draw_list_disable_scissor(DrawListID p_list)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

	data->draw_graph.add_draw_list_set_scissor(data->draw_list.viewport);
}

uint32_t RenderingDevice::draw_list_get_current_pass()
{
	ERR_RENDER_THREAD_GUARD_V(0);

	return data->draw_list_current_subpass;
}

RenderingDevice::RaytracingListID RenderingDevice::raytracing_list_begin()
{
	ERR_RENDER_THREAD_GUARD_V(RDC::INVALID_ID);

	ERR_FAIL_COND_V_MSG(!has_feature(RDC::SUPPORTS_RAYTRACING_PIPELINE), RDC::INVALID_ID,
		"The current rendering device has no raytracing pipeline support.");

	ERR_FAIL_COND_V_MSG(data->draw_list.active, RDC::INVALID_ID,
		"Only one draw/raytracing list can be active at the same time.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, RDC::INVALID_ID,
		"Only one compute/raytracing list can be active at the same time.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, RDC::INVALID_ID,
		"Only one raytracing list can be active at the same time.");

	data->raytracing_list.active = true;

	data->draw_graph.add_raytracing_list_begin();

	return ID_TYPE_RAYTRACING_LIST;
}

void RenderingDevice::raytracing_list_bind_uniform_set(
	RaytracingListID p_list, RID p_uniform_set, uint32_t p_index)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(p_list != ID_TYPE_RAYTRACING_LIST);
	ERR_FAIL_COND(!data->raytracing_list.active);

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(p_index >= data->driver->limit_get(RDC::LIMIT_MAX_BOUND_UNIFORM_SETS) ||
						  p_index >= MAX_UNIFORM_SETS,
		"Attempting to bind a descriptor set (" + itos(p_index) +
			") greater than what the hardware supports (" +
			itos(data->driver->limit_get(RDC::LIMIT_MAX_BOUND_UNIFORM_SETS)) + ").");
#endif

	UniformSet* uniform_set = data->uniform_set_owner.get_or_null(p_uniform_set);
	ERR_FAIL_NULL(uniform_set);

#ifdef DEBUG_ENABLED
	// Validate the acceleration structures.
	for (RID acceleration_structure_id : uniform_set->acceleration_structures) {
		AccelerationStructure* acceleration_structure =
			data->acceleration_structure_owner.get_or_null(acceleration_structure_id);
		ERR_FAIL_NULL(acceleration_structure);
		ERR_FAIL_COND_MSG(acceleration_structure->invalidated,
			"Acceleration structure either has not been built yet, or has been invalidated by an "
			"operation and needs to be rebuilt.");
	}
#endif

	if (p_index > data->raytracing_list.state.set_count) {
		data->raytracing_list.state.set_count = p_index;
	}

	data->raytracing_list.state.sets[p_index].uniform_set_driver_id =
		uniform_set->driver_id;								 // Update set pointer.
	data->raytracing_list.state.sets[p_index].bound = false; // Needs rebind.
	data->raytracing_list.state.sets[p_index].uniform_set_format = uniform_set->format;
	data->raytracing_list.state.sets[p_index].uniform_set = p_uniform_set;
}

void RenderingDevice::raytracing_list_set_push_constant(
	RaytracingListID p_list, const void* p_data, uint32_t p_data_size)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(p_list != ID_TYPE_RAYTRACING_LIST);
	ERR_FAIL_COND(!data->raytracing_list.active);

	ERR_FAIL_COND_MSG(p_data_size > MAX_PUSH_CONSTANT_SIZE,
		"Push constants can't be bigger than 128 bytes to maintain compatibility.");

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(p_data_size != data->raytracing_list.validation.pipeline_push_constant_size,
		"This raytracing pipeline requires (" +
			itos(data->raytracing_list.validation.pipeline_push_constant_size) +
			") bytes of push constant data, supplied: (" + itos(p_data_size) + ")");
#endif

	data->draw_graph.add_raytracing_list_set_push_constant(
		data->raytracing_list.state.layout_defining_shader_driver_id, p_data, p_data_size);

	// Store it in the state in case we need to restart the raytracing list.
	memcpy(data->raytracing_list.state.push_constant_data, p_data, p_data_size);
	data->raytracing_list.state.push_constant_size = p_data_size;

#ifdef DEBUG_ENABLED
	data->raytracing_list.validation.pipeline_push_constant_supplied = true;
#endif
}

void RenderingDevice::raytracing_list_end()
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->raytracing_list.active);

	data->draw_graph.add_raytracing_list_end();

	data->raytracing_list = RaytracingList();
}

RenderingDevice::ComputeListID RenderingDevice::compute_list_begin()
{
	ERR_RENDER_THREAD_GUARD_V(RDC::INVALID_ID);

	ERR_FAIL_COND_V_MSG(data->compute_list.active, RDC::INVALID_ID,
		"Only one compute list can be active at the same time.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, RDC::INVALID_ID,
		"Only one raytracing list can be active at the same time.");

	data->compute_list.active = true;

	data->draw_graph.add_compute_list_begin();

	return ID_TYPE_COMPUTE_LIST;
}

void RenderingDevice::compute_list_bind_compute_pipeline(
	ComputeListID p_list, RID p_compute_pipeline)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(p_list != ID_TYPE_COMPUTE_LIST);
	ERR_FAIL_COND(!data->compute_list.active);

	const ComputePipeline* pipeline = data->compute_pipeline_owner.get_or_null(p_compute_pipeline);
	ERR_FAIL_NULL(pipeline);

	if (p_compute_pipeline == data->compute_list.state.pipeline) {
		return; // Redundant state, return.
	}

	data->compute_list.state.pipeline = p_compute_pipeline;

	data->draw_graph.add_compute_list_bind_pipeline(pipeline->driver_id);

	if (data->compute_list.state.pipeline_shader != pipeline->shader) {
		uint32_t pcount = pipeline->set_formats.size();
		data->compute_list.state.set_count = MAX(data->compute_list.state.set_count, pcount);
		const uint32_t* pformats = pipeline->set_formats.ptr();

		uint32_t first_invalid_set = UINT32_MAX;
		switch (data->driver->api_trait_get(RDD::API_TRAIT_SHADER_CHANGE_INVALIDATION)) {
		case RDD::SHADER_CHANGE_INVALIDATION_ALL_BOUND_UNIFORM_SETS: {
			first_invalid_set = 0;
		} break;
		case RDD::SHADER_CHANGE_INVALIDATION_INCOMPATIBLE_SETS_PLUS_CASCADE: {
			for (uint32_t i = 0; i < pcount; i++) {
				if (data->compute_list.state.sets[i].pipeline_expected_format != pformats[i]) {
					first_invalid_set = i;
					break;
				}
			}
		} break;
		case RDD::SHADER_CHANGE_INVALIDATION_ALL_OR_NONE_ACCORDING_TO_LAYOUT_HASH: {
			if (data->compute_list.state.pipeline_shader_layout_hash !=
				pipeline->shader_layout_hash) {
				first_invalid_set = 0;
			}
		} break;
		}

		for (uint32_t i = 0; i < pcount; i++) {
			data->compute_list.state.sets[i].bound =
				data->compute_list.state.sets[i].bound && i < first_invalid_set;
			data->compute_list.state.sets[i].pipeline_expected_format = pformats[i];
		}

		for (uint32_t i = pcount; i < data->compute_list.state.set_count; i++) {
			data->compute_list.state.sets[i].bound = false;
		}

		data->compute_list.state.set_count = pcount;

		if (pipeline->push_constant_size) {
#ifdef DEBUG_ENABLED
			data->compute_list.validation.pipeline_push_constant_supplied = false;
#endif
		}

		data->compute_list.state.pipeline_shader = pipeline->shader;
		data->compute_list.state.pipeline_shader_driver_id = pipeline->shader_driver_id;
		data->compute_list.state.pipeline_shader_layout_hash = pipeline->shader_layout_hash;
		data->compute_list.state.local_group_size[0] = pipeline->local_group_size[0];
		data->compute_list.state.local_group_size[1] = pipeline->local_group_size[1];
		data->compute_list.state.local_group_size[2] = pipeline->local_group_size[2];
	}

#ifdef DEBUG_ENABLED
	data->compute_list.validation.pipeline_active = true;
	data->compute_list.validation.pipeline_push_constant_size = pipeline->push_constant_size;
#endif
}

void RenderingDevice::compute_list_bind_uniform_set(
	ComputeListID p_list, RID p_uniform_set, uint32_t p_index)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(p_list != ID_TYPE_COMPUTE_LIST);
	ERR_FAIL_COND(!data->compute_list.active);

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(p_index >= data->driver->limit_get(RDC::LIMIT_MAX_BOUND_UNIFORM_SETS) ||
						  p_index >= MAX_UNIFORM_SETS,
		"Attempting to bind a descriptor set (" + itos(p_index) +
			") greater than what the hardware supports (" +
			itos(data->driver->limit_get(RDC::LIMIT_MAX_BOUND_UNIFORM_SETS)) + ").");
#endif

	UniformSet* uniform_set = data->uniform_set_owner.get_or_null(p_uniform_set);
	ERR_FAIL_NULL(uniform_set);

	if (p_index > data->compute_list.state.set_count) {
		data->compute_list.state.set_count = p_index;
	}

	data->compute_list.state.sets[p_index].uniform_set_driver_id =
		uniform_set->driver_id;							  // Update set pointer.
	data->compute_list.state.sets[p_index].bound = false; // Needs rebind.
	data->compute_list.state.sets[p_index].uniform_set_format = uniform_set->format;
	data->compute_list.state.sets[p_index].uniform_set = p_uniform_set;

#if 0
	{ // Validate that textures bound are not attached as framebuffer bindings.
		uint32_t attachable_count = uniform_set->attachable_textures.size();
		const RID *attachable_ptr = uniform_set->attachable_textures.ptr();
		uint32_t bound_count = draw_list_bound_textures.size();
		const RID *bound_ptr = draw_list_bound_textures.ptr();
		for (uint32_t i = 0; i < attachable_count; i++) {
			for (uint32_t j = 0; j < bound_count; j++) {
				ERR_FAIL_COND_MSG(attachable_ptr[i] == bound_ptr[j],
						"Attempted to use the same texture in framebuffer attachment and a uniform set, this is not allowed.");
			}
		}
	}
#endif
}

void RenderingDevice::compute_list_set_push_constant(
	ComputeListID p_list, const void* p_data, uint32_t p_data_size)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(p_list != ID_TYPE_COMPUTE_LIST);
	ERR_FAIL_COND(!data->compute_list.active);
	ERR_FAIL_COND_MSG(p_data_size > MAX_PUSH_CONSTANT_SIZE,
		"Push constants can't be bigger than 128 bytes to maintain compatibility.");

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(p_data_size != data->compute_list.validation.pipeline_push_constant_size,
		"This compute pipeline requires (" +
			itos(data->compute_list.validation.pipeline_push_constant_size) +
			") bytes of push constant data, supplied: (" + itos(p_data_size) + ")");
#endif

	data->draw_graph.add_compute_list_set_push_constant(
		data->compute_list.state.pipeline_shader_driver_id, p_data, p_data_size);

	// Store it in the state in case we need to restart the compute list.
	memcpy(data->compute_list.state.push_constant_data, p_data, p_data_size);
	data->compute_list.state.push_constant_size = p_data_size;

#ifdef DEBUG_ENABLED
	data->compute_list.validation.pipeline_push_constant_supplied = true;
#endif
}

void RenderingDevice::compute_list_dispatch(
	ComputeListID p_list, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(p_list != ID_TYPE_COMPUTE_LIST);
	ERR_FAIL_COND(!data->compute_list.active);

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(
		p_x_groups == 0, "Dispatch amount of X compute groups (" + itos(p_x_groups) + ") is zero.");
	ERR_FAIL_COND_MSG(
		p_z_groups == 0, "Dispatch amount of Z compute groups (" + itos(p_z_groups) + ") is zero.");
	ERR_FAIL_COND_MSG(
		p_y_groups == 0, "Dispatch amount of Y compute groups (" + itos(p_y_groups) + ") is zero.");
	ERR_FAIL_COND_MSG(
		p_x_groups > data->driver->limit_get(RDC::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X),
		"Dispatch amount of X compute groups (" + itos(p_x_groups) +
			") is larger than device limit (" +
			itos(data->driver->limit_get(RDC::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X)) + ")");
	ERR_FAIL_COND_MSG(
		p_y_groups > data->driver->limit_get(RDC::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Y),
		"Dispatch amount of Y compute groups (" + itos(p_y_groups) +
			") is larger than device limit (" +
			itos(data->driver->limit_get(RDC::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Y)) + ")");
	ERR_FAIL_COND_MSG(
		p_z_groups > data->driver->limit_get(RDC::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Z),
		"Dispatch amount of Z compute groups (" + itos(p_z_groups) +
			") is larger than device limit (" +
			itos(data->driver->limit_get(RDC::LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Z)) + ")");
#endif

#ifdef DEBUG_ENABLED

	ERR_FAIL_COND_MSG(!data->compute_list.validation.pipeline_active,
		"No compute pipeline was set before attempting to draw.");

	if (data->compute_list.validation.pipeline_push_constant_size > 0) {
		// Using push constants, check that they were supplied.
		ERR_FAIL_COND_MSG(!data->compute_list.validation.pipeline_push_constant_supplied,
			"The shader in this pipeline requires a push constant to be set before drawing, but "
			"it's not present.");
	}

#endif

#ifdef DEBUG_ENABLED
	for (uint32_t i = 0; i < data->compute_list.state.set_count; i++) {
		if (data->compute_list.state.sets[i].pipeline_expected_format == 0) {
			// Nothing expected by this pipeline.
			continue;
		}

		if (data->compute_list.state.sets[i].pipeline_expected_format !=
			data->compute_list.state.sets[i].uniform_set_format) {
			if (data->compute_list.state.sets[i].uniform_set_format == 0) {
				ERR_FAIL_MSG("Uniforms were never supplied for set (" + itos(i) +
							 ") at the time of drawing, which are required by the pipeline.");
			}
			else if (data->uniform_set_owner.owns(data->compute_list.state.sets[i].uniform_set)) {
				UniformSet* us = data->uniform_set_owner.get_or_null(
					data->compute_list.state.sets[i].uniform_set);
				ERR_FAIL_MSG("Uniforms supplied for set (" + itos(i) + "):\n" +
							 _shader_uniform_debug(us->shader_id, us->shader_set) +
							 "\nare not the same format as required by the pipeline shader. "
							 "Pipeline shader requires the following bindings:\n" +
							 _shader_uniform_debug(data->compute_list.state.pipeline_shader));
			}
			else {
				ERR_FAIL_MSG("Uniforms supplied for set (" + itos(i) +
							 ", which was just freed) are not the same format as required by the "
							 "pipeline shader. Pipeline shader requires the following bindings:\n" +
							 _shader_uniform_debug(data->compute_list.state.pipeline_shader));
			}
		}
	}
#endif
	thread_local LocalVector<RDD::UniformSetID> valid_descriptor_ids;
	valid_descriptor_ids.clear();
	valid_descriptor_ids.resize(data->compute_list.state.set_count);

	uint32_t valid_set_count = 0;
	uint32_t first_set_index = 0;
	uint32_t last_set_index = 0;
	bool found_first_set = false;

	for (uint32_t i = 0; i < data->compute_list.state.set_count; i++) {
		if (data->compute_list.state.sets[i].pipeline_expected_format == 0) {
			// Nothing expected by this pipeline.
			continue;
		}

		if (!data->compute_list.state.sets[i].bound && !found_first_set) {
			first_set_index = i;
			found_first_set = true;
		}
		// Prepare descriptor sets if the API doesn't use pipeline barriers.
		if (!data->driver->api_trait_get(RDD::API_TRAIT_HONORS_PIPELINE_BARRIERS)) {
			data->draw_graph.add_compute_list_uniform_set_prepare_for_use(
				data->compute_list.state.pipeline_shader_driver_id,
				data->compute_list.state.sets[i].uniform_set_driver_id, i);
		}
	}

	// Bind descriptor sets.
	for (uint32_t i = first_set_index; i < data->compute_list.state.set_count; i++) {
		if (data->compute_list.state.sets[i].pipeline_expected_format == 0) {
			continue; // Nothing expected by this pipeline.
		}

		if (!data->compute_list.state.sets[i].bound) {
			// Descriptor set batching
			if (data->descriptor_set_batching) {
				// All good, see if this requires re-binding.
				if (i - last_set_index > 1) {
					// If the descriptor sets are not contiguous, bind the previous ones and start a
					// new batch.
					data->draw_graph.add_compute_list_bind_uniform_sets(
						data->compute_list.state.pipeline_shader_driver_id, valid_descriptor_ids,
						first_set_index, valid_set_count);

					first_set_index = i;
					valid_set_count = 1;
					valid_descriptor_ids[0] =
						data->compute_list.state.sets[i].uniform_set_driver_id;
				}
				else {
					// Otherwise, keep storing in the current batch.
					valid_descriptor_ids[valid_set_count] =
						data->compute_list.state.sets[i].uniform_set_driver_id;
					valid_set_count++;
				}

				last_set_index = i;
			}
			else {
				data->draw_graph.add_compute_list_bind_uniform_set(
					data->compute_list.state.pipeline_shader_driver_id,
					data->compute_list.state.sets[i].uniform_set_driver_id, i);
			}
			UniformSet* uniform_set =
				data->uniform_set_owner.get_or_null(data->compute_list.state.sets[i].uniform_set);
			_uniform_set_update_shared(uniform_set);

			data->draw_graph.add_compute_list_usages(
				uniform_set->draw_trackers, uniform_set->draw_trackers_usage);
			data->compute_list.state.sets[i].bound = true;
		}
	}

	// Bind the remaining batch.
	if (valid_set_count > 0) {
		data->draw_graph.add_compute_list_bind_uniform_sets(
			data->compute_list.state.pipeline_shader_driver_id, valid_descriptor_ids,
			first_set_index, valid_set_count);
	}
	data->draw_graph.add_compute_list_dispatch(p_x_groups, p_y_groups, p_z_groups);
	data->compute_list.state.dispatch_count++;
}

void RenderingDevice::compute_list_dispatch_threads(
	ComputeListID p_list, uint32_t p_x_threads, uint32_t p_y_threads, uint32_t p_z_threads)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(p_list != ID_TYPE_COMPUTE_LIST);
	ERR_FAIL_COND(!data->compute_list.active);

#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(p_x_threads == 0,
		"Dispatch amount of X compute threads (" + itos(p_x_threads) + ") is zero.");
	ERR_FAIL_COND_MSG(p_y_threads == 0,
		"Dispatch amount of Y compute threads (" + itos(p_y_threads) + ") is zero.");
	ERR_FAIL_COND_MSG(p_z_threads == 0,
		"Dispatch amount of Z compute threads (" + itos(p_z_threads) + ") is zero.");
#endif

#ifdef DEBUG_ENABLED

	ERR_FAIL_COND_MSG(!data->compute_list.validation.pipeline_active,
		"No compute pipeline was set before attempting to draw.");

	if (data->compute_list.validation.pipeline_push_constant_size > 0) {
		// Using push constants, check that they were supplied.
		ERR_FAIL_COND_MSG(!data->compute_list.validation.pipeline_push_constant_supplied,
			"The shader in this pipeline requires a push constant to be set before drawing, but "
			"it's not present.");
	}

#endif

	compute_list_dispatch(p_list,
		Math::division_round_up(p_x_threads, data->compute_list.state.local_group_size[0]),
		Math::division_round_up(p_y_threads, data->compute_list.state.local_group_size[1]),
		Math::division_round_up(p_z_threads, data->compute_list.state.local_group_size[2]));
}

void RenderingDevice::compute_list_end()
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->compute_list.active);

	data->draw_graph.add_compute_list_end();

	data->compute_list = ComputeList();
}

static uint32_t _get_alignment_offset(uint32_t p_offset, uint32_t p_required_align)
{
	uint32_t alignment_offset = (p_required_align > 0) ? (p_offset % p_required_align) : 0;
	if (alignment_offset != 0) {
		// If a particular alignment is required, add the offset as part of the required size.
		alignment_offset = p_required_align - alignment_offset;
	}

	return alignment_offset;
}

RenderingDevice::TransferWorker* RenderingDevice::_acquire_transfer_worker(
	uint32_t p_transfer_size, uint32_t p_required_align, uint32_t& r_staging_offset)
{
	// Find the first worker that is not currently executing anything and has enough size for the
	// transfer. If no workers are available, we make a new one. If we're not allowed to make new
	// ones, we wait until one of them is available.
	TransferWorker* transfer_worker = nullptr;
	uint32_t available_list_index = 0;
	bool transfer_worker_busy = true;
	bool transfer_worker_full = true;
	{
		MutexLock pool_lock(data->transfer_worker_pool_mutex);

		// If no workers are available and we've reached the max pool capacity, wait until one of
		// them becomes available.
		bool transfer_worker_pool_full =
			data->transfer_worker_pool_size >= data->transfer_worker_pool_max_size;
		while (data->transfer_worker_pool_available_list.is_empty() && transfer_worker_pool_full) {
			data->transfer_worker_pool_condition.wait(pool_lock);
		}

		// Look at all available workers first.
		for (uint32_t i = 0; i < data->transfer_worker_pool_available_list.size(); i++) {
			uint32_t worker_index = data->transfer_worker_pool_available_list[i];
			TransferWorker* candidate_worker = data->transfer_worker_pool[worker_index];
			candidate_worker->thread_mutex.lock();

			// Figure out if the worker can fit the transfer.
			uint32_t alignment_offset = _get_alignment_offset(
				candidate_worker->staging_buffer_size_in_use, p_required_align);
			uint32_t required_size =
				candidate_worker->staging_buffer_size_in_use + p_transfer_size + alignment_offset;
			bool candidate_worker_busy = candidate_worker->submitted;
			bool candidate_worker_full =
				required_size > candidate_worker->staging_buffer_size_allocated;
			bool pick_candidate = false;
			if (!candidate_worker_busy && !candidate_worker_full) {
				// A worker that can fit the transfer and is not waiting for a previous execution is
				// the best possible candidate.
				pick_candidate = true;
			}
			else if (!candidate_worker_busy) {
				// The worker can't fit the transfer but it's not currently doing anything.
				// We pick it as a possible candidate if the current one is busy.
				pick_candidate = transfer_worker_busy;
			}
			else if (!candidate_worker_full) {
				// The worker can fit the transfer but it's currently executing previous work.
				// We pick it as a possible candidate if the current one is both busy and full.
				pick_candidate = transfer_worker_busy && transfer_worker_full;
			}
			else if (transfer_worker == nullptr) {
				// The worker can't fit the transfer and it's currently executing work, so it's the
				// worst candidate. We only pick if no candidate has been picked yet.
				pick_candidate = true;
			}

			if (pick_candidate) {
				if (transfer_worker != nullptr) {
					// Release the lock for the worker that was picked previously.
					transfer_worker->thread_mutex.unlock();
				}

				// Keep the lock active for this worker.
				transfer_worker = candidate_worker;
				transfer_worker_busy = candidate_worker_busy;
				transfer_worker_full = candidate_worker_full;
				available_list_index = i;

				if (!transfer_worker_busy && !transfer_worker_full) {
					// Best possible candidate, stop searching early.
					break;
				}
			}
			else {
				// Release the lock for the candidate.
				candidate_worker->thread_mutex.unlock();
			}
		}

		if (transfer_worker != nullptr) {
			// A worker was picked, remove it from the available list.
			data->transfer_worker_pool_available_list.remove_at(available_list_index);
		}
		else {
			DEV_ASSERT(!transfer_worker_pool_full &&
					   "A transfer worker should never be created when the pool is full.");
			// No existing worker was picked, we create a new one.
			uint32_t transfer_worker_index = data->transfer_worker_pool_size;
			++data->transfer_worker_pool_size;

			transfer_worker = memnew(TransferWorker);
			transfer_worker->command_fence = data->driver->fence_create();
			transfer_worker->command_pool = data->driver->command_pool_create(
				data->transfer_queue_family, RDD::COMMAND_BUFFER_TYPE_PRIMARY);
			transfer_worker->command_buffer =
				data->driver->command_buffer_create(transfer_worker->command_pool);
			transfer_worker->index = transfer_worker_index;
			if (data->transfer_worker_pool.size() <= transfer_worker_index) {
				data->transfer_worker_pool.resize(transfer_worker_index + 1);
			}
			if (data->transfer_worker_operation_used_by_draw.size() <= transfer_worker_index) {
				data->transfer_worker_operation_used_by_draw.resize(transfer_worker_index + 1);
			}
			data->transfer_worker_pool[transfer_worker_index] = transfer_worker;
			data->transfer_worker_operation_used_by_draw[transfer_worker_index] = 0;
			transfer_worker->thread_mutex.lock();
		}
	}

	if (transfer_worker->submitted) {
		// Wait for the worker if the command buffer was submitted but it hasn't finished processing
		// yet.
		_wait_for_transfer_worker(transfer_worker);
	}

	uint32_t alignment_offset =
		_get_alignment_offset(transfer_worker->staging_buffer_size_in_use, p_required_align);
	transfer_worker->max_transfer_size = MAX(transfer_worker->max_transfer_size, p_transfer_size);

	uint32_t required_size =
		transfer_worker->staging_buffer_size_in_use + p_transfer_size + alignment_offset;
	if (required_size > transfer_worker->staging_buffer_size_allocated) {
		// If there's not enough bytes to use on the staging buffer, we submit everything pending
		// from the worker and wait for the work to be finished.
		if (transfer_worker->recording) {
			_end_transfer_worker(transfer_worker);
			_submit_transfer_worker(transfer_worker);
		}

		if (transfer_worker->submitted) {
			_wait_for_transfer_worker(transfer_worker);
		}

		alignment_offset = 0;

		// If the staging buffer can't fit the transfer, we recreate the buffer.
		const uint32_t expected_buffer_size_minimum = 16 * 1024;
		uint32_t expected_buffer_size =
			MAX(transfer_worker->max_transfer_size, expected_buffer_size_minimum);
		if (expected_buffer_size > transfer_worker->staging_buffer_size_allocated) {
			if (transfer_worker->staging_buffer.id != 0) {
				data->driver->buffer_free(transfer_worker->staging_buffer);
			}

			uint32_t new_staging_buffer_size = Math::next_power_of_2(expected_buffer_size);
			transfer_worker->staging_buffer_size_allocated = new_staging_buffer_size;
			transfer_worker->staging_buffer = data->driver->buffer_create(new_staging_buffer_size,
				RDD::BUFFER_USAGE_TRANSFER_FROM_BIT, RDD::MEMORY_ALLOCATION_TYPE_CPU,
				data->frames_drawn);
		}
	}

	// Add the alignment before storing the offset that will be returned.
	transfer_worker->staging_buffer_size_in_use += alignment_offset;

	// Store the offset to return and increment the current size.
	r_staging_offset = transfer_worker->staging_buffer_size_in_use;
	transfer_worker->staging_buffer_size_in_use += p_transfer_size;

	if (!transfer_worker->recording) {
		// Begin the command buffer if the worker wasn't recording yet.
		data->driver->command_buffer_begin(transfer_worker->command_buffer);
		transfer_worker->recording = true;
	}

	return transfer_worker;
}

void RenderingDevice::_release_transfer_worker(TransferWorker* p_transfer_worker)
{
	p_transfer_worker->thread_mutex.unlock();

	data->transfer_worker_pool_mutex.lock();
	data->transfer_worker_pool_available_list.push_back(p_transfer_worker->index);
	data->transfer_worker_pool_mutex.unlock();
	data->transfer_worker_pool_condition.notify_one();
}

void RenderingDevice::_end_transfer_worker(TransferWorker* p_transfer_worker)
{
	data->driver->command_buffer_end(p_transfer_worker->command_buffer);
	p_transfer_worker->recording = false;
}

void RenderingDevice::_submit_transfer_worker(
	TransferWorker* p_transfer_worker, VectorView<RDD::SemaphoreID> p_signal_semaphores)
{
	data->driver->command_queue_execute_and_present(data->transfer_queue, {},
		p_transfer_worker->command_buffer, p_signal_semaphores, p_transfer_worker->command_fence,
		{});

	for (uint32_t i = 0; i < p_signal_semaphores.size(); i++) {
		// Indicate the frame should wait on these semaphores before executing the main command
		// buffer.
		data->frames[data->frame].semaphores_to_wait_on.push_back(p_signal_semaphores[i]);
	}

	p_transfer_worker->submitted = true;

	{
		MutexLock lock(p_transfer_worker->operations_mutex);
		p_transfer_worker->operations_submitted = p_transfer_worker->operations_counter;
	}
}

void RenderingDevice::_wait_for_transfer_worker(TransferWorker* p_transfer_worker)
{
	data->driver->fence_wait(p_transfer_worker->command_fence);
	data->driver->command_pool_reset(p_transfer_worker->command_pool);
	p_transfer_worker->staging_buffer_size_in_use = 0;
	p_transfer_worker->submitted = false;

	{
		MutexLock lock(p_transfer_worker->operations_mutex);
		p_transfer_worker->operations_processed = p_transfer_worker->operations_submitted;
	}

	_flush_barriers_for_transfer_worker(p_transfer_worker);
}

void RenderingDevice::_flush_barriers_for_transfer_worker(TransferWorker* p_transfer_worker)
{
	// Caller must have already acquired the mutex for the worker.
	if (!p_transfer_worker->texture_barriers.is_empty()) {
		MutexLock transfer_worker_lock(data->transfer_worker_pool_texture_barriers_mutex);
		for (uint32_t i = 0; i < p_transfer_worker->texture_barriers.size(); i++) {
			data->transfer_worker_pool_texture_barriers.push_back(
				p_transfer_worker->texture_barriers[i]);
		}

		p_transfer_worker->texture_barriers.clear();
	}
}

void RenderingDevice::_check_transfer_worker_operation(
	uint32_t p_transfer_worker_index, uint64_t p_transfer_worker_operation)
{
	TransferWorker* transfer_worker = data->transfer_worker_pool[p_transfer_worker_index];
	MutexLock lock(transfer_worker->operations_mutex);
	uint64_t& dst_operation = data->transfer_worker_operation_used_by_draw[transfer_worker->index];
	dst_operation = MAX(dst_operation, p_transfer_worker_operation);
}

void RenderingDevice::_check_transfer_worker_buffer(Buffer* p_buffer)
{
	if (p_buffer->transfer_worker_index >= 0) {
		_check_transfer_worker_operation(
			p_buffer->transfer_worker_index, p_buffer->transfer_worker_operation);
		p_buffer->transfer_worker_index = -1;
	}
}

void RenderingDevice::_check_transfer_worker_texture(Texture* p_texture)
{
	if (p_texture->transfer_worker_index >= 0) {
		_check_transfer_worker_operation(
			p_texture->transfer_worker_index, p_texture->transfer_worker_operation);
		p_texture->transfer_worker_index = -1;
	}
}

void RenderingDevice::_check_transfer_worker_vertex_array(VertexArray* p_vertex_array)
{
	if (!p_vertex_array->transfer_worker_indices.is_empty()) {
		for (int i = 0; i < p_vertex_array->transfer_worker_indices.size(); i++) {
			_check_transfer_worker_operation(p_vertex_array->transfer_worker_indices[i],
				p_vertex_array->transfer_worker_operations[i]);
		}

		p_vertex_array->transfer_worker_indices.clear();
		p_vertex_array->transfer_worker_operations.clear();
	}
}

void RenderingDevice::_check_transfer_worker_index_array(IndexArray* p_index_array)
{
	if (p_index_array->transfer_worker_index >= 0) {
		_check_transfer_worker_operation(
			p_index_array->transfer_worker_index, p_index_array->transfer_worker_operation);
		p_index_array->transfer_worker_index = -1;
	}
}

void RenderingDevice::_submit_transfer_workers(RDD::CommandBufferID p_draw_command_buffer)
{
	MutexLock transfer_worker_lock(data->transfer_worker_pool_mutex);
	for (uint32_t i = 0; i < data->transfer_worker_pool_size; i++) {
		TransferWorker* worker = data->transfer_worker_pool[i];
		if (p_draw_command_buffer) {
			MutexLock lock(worker->operations_mutex);
			if (worker->operations_processed >=
				data->transfer_worker_operation_used_by_draw[worker->index]) {
				// The operation used by the draw has already been processed, we don't need to wait
				// on the worker.
				continue;
			}
		}

		{
			MutexLock lock(worker->thread_mutex);
			if (worker->recording) {
				VectorView<RDD::SemaphoreID> semaphores =
					p_draw_command_buffer ? data->frames[data->frame].transfer_worker_semaphores[i]
										  : VectorView<RDD::SemaphoreID>();
				_end_transfer_worker(worker);
				_submit_transfer_worker(worker, semaphores);
			}

			if (p_draw_command_buffer) {
				_flush_barriers_for_transfer_worker(worker);
			}
		}
	}
}

void RenderingDevice::_submit_transfer_barriers(RDD::CommandBufferID p_draw_command_buffer)
{
	MutexLock transfer_worker_lock(data->transfer_worker_pool_texture_barriers_mutex);
	if (!data->transfer_worker_pool_texture_barriers.is_empty()) {
		data->driver->command_pipeline_barrier(p_draw_command_buffer, RDD::PIPELINE_STAGE_COPY_BIT,
			RDD::PIPELINE_STAGE_ALL_COMMANDS_BIT, {}, {},
			data->transfer_worker_pool_texture_barriers, {});
		data->transfer_worker_pool_texture_barriers.clear();
	}
}

void RenderingDevice::_wait_for_transfer_workers()
{
	MutexLock transfer_worker_lock(data->transfer_worker_pool_mutex);
	for (uint32_t i = 0; i < data->transfer_worker_pool_size; i++) {
		TransferWorker* worker = data->transfer_worker_pool[i];
		MutexLock lock(worker->thread_mutex);
		if (worker->submitted) {
			_wait_for_transfer_worker(worker);
		}
	}
}

void RenderingDevice::_free_transfer_workers()
{
	MutexLock transfer_worker_lock(data->transfer_worker_pool_mutex);
	for (uint32_t i = 0; i < data->transfer_worker_pool_size; i++) {
		TransferWorker* worker = data->transfer_worker_pool[i];
		data->driver->fence_free(worker->command_fence);
		data->driver->buffer_free(worker->staging_buffer);
		data->driver->command_pool_free(worker->command_pool);
		memdelete(worker);
	}

	data->transfer_worker_pool_size = 0;
}

bool RenderingDevice::_texture_make_mutable(Texture* p_texture, RID p_texture_id)
{
	if (p_texture->draw_tracker != nullptr) {
		// Texture already has a tracker.
		return false;
	}
	else {
		if (p_texture->owner.is_valid()) {
			// Texture has an owner.
			Texture* owner_texture = data->texture_owner.get_or_null(p_texture->owner);
			ERR_FAIL_NULL_V(owner_texture, false);

			if (owner_texture->draw_tracker != nullptr) {
				// Create a tracker for this dependency in particular.
				if (p_texture->slice_type == RDC::TEXTURE_SLICE_MAX) {
					// Shared texture.
					p_texture->draw_tracker = owner_texture->draw_tracker;
					p_texture->draw_tracker->reference_count++;
				}
				else {
					// Slice texture.
					if (owner_texture->slice_trackers == nullptr) {
						owner_texture->slice_trackers =
							memnew((HashMap<Rect2i, RDG::ResourceTracker*>));
					}
					HashMap<Rect2i, RDG::ResourceTracker*>::ConstIterator draw_tracker_iterator =
						owner_texture->slice_trackers->find(p_texture->slice_rect);
					RDG::ResourceTracker* draw_tracker = nullptr;
					if (draw_tracker_iterator != owner_texture->slice_trackers->end()) {
						// Reuse the tracker at the matching rectangle.
						draw_tracker = draw_tracker_iterator->value;
					}
					else {
						// Create a new tracker and store it on the map.
						draw_tracker = RDG::resource_tracker_create();
						draw_tracker->parent = owner_texture->draw_tracker;
						draw_tracker->texture_driver_id = p_texture->driver_id;
						draw_tracker->texture_size = Size2i(p_texture->width, p_texture->height);
						draw_tracker->texture_subresources = p_texture->barrier_range();
						draw_tracker->texture_usage = p_texture->usage_flags;
						draw_tracker->texture_slice_or_dirty_rect = p_texture->slice_rect;
						(*owner_texture->slice_trackers)[p_texture->slice_rect] = draw_tracker;
					}

					p_texture->draw_tracker = draw_tracker;
					p_texture->draw_tracker->reference_count++;
				}

				if (p_texture_id.is_valid()) {
					_dependencies_make_mutable(p_texture_id, p_texture->draw_tracker);
				}
			}
			else {
				// Delegate this to the owner instead, as it'll make all its dependencies mutable.
				_texture_make_mutable(owner_texture, p_texture->owner);
			}
		}
		else {
			// Regular texture.
			p_texture->draw_tracker = RDG::resource_tracker_create();
			p_texture->draw_tracker->texture_driver_id = p_texture->driver_id;
			p_texture->draw_tracker->texture_size = Size2i(p_texture->width, p_texture->height);
			p_texture->draw_tracker->texture_subresources = p_texture->barrier_range();
			p_texture->draw_tracker->texture_usage = p_texture->usage_flags;
			p_texture->draw_tracker->is_discardable = p_texture->is_discardable;
			p_texture->draw_tracker->reference_count = 1;

			if (p_texture_id.is_valid()) {
				if (p_texture->has_initial_data) {
					// If the texture was initialized with initial data but wasn't made mutable from
					// the start, assume the texture sampling usage.
					p_texture->draw_tracker->usage = RDG::RESOURCE_USAGE_TEXTURE_SAMPLE;
				}

				_dependencies_make_mutable(p_texture_id, p_texture->draw_tracker);
			}
		}

		return true;
	}
}

bool RenderingDevice::_buffer_make_mutable(Buffer* p_buffer, RID p_buffer_id)
{
	if (p_buffer->draw_tracker != nullptr) {
		// Buffer already has a tracker.
		return false;
	}
	else {
		// Create a tracker for the buffer and make all its dependencies mutable.
		p_buffer->draw_tracker = RDG::resource_tracker_create();
		p_buffer->draw_tracker->buffer_driver_id = p_buffer->driver_id;
		if (p_buffer_id.is_valid()) {
			_dependencies_make_mutable(p_buffer_id, p_buffer->draw_tracker);
		}

		return true;
	}
}

bool RenderingDevice::_vertex_array_make_mutable(
	VertexArray* p_vertex_array, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker)
{
	if (!p_vertex_array->untracked_buffers.has(p_resource_id)) {
		// Vertex array thinks the buffer is already tracked or does not use it.
		return false;
	}
	else {
		// Vertex array is aware of the buffer but it isn't being tracked.
		p_vertex_array->draw_trackers.push_back(p_resource_tracker);
		p_vertex_array->untracked_buffers.erase(p_resource_id);
		return true;
	}
}

bool RenderingDevice::_index_array_make_mutable(
	IndexArray* p_index_array, RDG::ResourceTracker* p_resource_tracker)
{
	if (p_index_array->draw_tracker != nullptr) {
		// Index array already has a tracker.
		return false;
	}
	else {
		// Index array should assign the tracker from the buffer.
		p_index_array->draw_tracker = p_resource_tracker;
		return true;
	}
}

bool RenderingDevice::_uniform_set_make_mutable(
	UniformSet* p_uniform_set, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker)
{
	HashMap<RID, RDG::ResourceUsage>::Iterator E =
		p_uniform_set->untracked_usage.find(p_resource_id);
	if (!E) {
		// Uniform set thinks the resource is already tracked or does not use it.
		return false;
	}
	else {
		// Uniform set has seen the resource but hasn't added its tracker yet.
		p_uniform_set->draw_trackers.push_back(p_resource_tracker);
		p_uniform_set->draw_trackers_usage.push_back(E->value);
		p_uniform_set->untracked_usage.remove(E);
		return true;
	}
}

bool RenderingDevice::_acceleration_structure_make_mutable(
	AccelerationStructure* p_acceleration_structure, RID p_resource_id,
	RDG::ResourceTracker* p_resource_tracker)
{
	if (!p_acceleration_structure->untracked_buffers.has(p_resource_id)) {
		// Acceleration structure thinks the buffer is already tracked or does not use it.
		return false;
	}
	else {
		// Acceleration structure is aware of the buffer but it isn't being tracked.
		p_acceleration_structure->draw_trackers.push_back(p_resource_tracker);
		p_acceleration_structure->untracked_buffers.erase(p_resource_id);
		return true;
	}
}

bool RenderingDevice::_dependency_make_mutable(
	RID p_id, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker)
{
	if (data->texture_owner.owns(p_id)) {
		Texture* texture = data->texture_owner.get_or_null(p_id);
		return _texture_make_mutable(texture, p_id);
	}
	else if (data->vertex_array_owner.owns(p_id)) {
		VertexArray* vertex_array = data->vertex_array_owner.get_or_null(p_id);
		return _vertex_array_make_mutable(vertex_array, p_resource_id, p_resource_tracker);
	}
	else if (data->index_array_owner.owns(p_id)) {
		IndexArray* index_array = data->index_array_owner.get_or_null(p_id);
		return _index_array_make_mutable(index_array, p_resource_tracker);
	}
	else if (data->uniform_set_owner.owns(p_id)) {
		UniformSet* uniform_set = data->uniform_set_owner.get_or_null(p_id);
		return _uniform_set_make_mutable(uniform_set, p_resource_id, p_resource_tracker);
	}
	else if (data->acceleration_structure_owner.owns(p_id)) {
		AccelerationStructure* acceleration_structure =
			data->acceleration_structure_owner.get_or_null(p_id);
		return _acceleration_structure_make_mutable(
			acceleration_structure, p_resource_id, p_resource_tracker);
	}
	else {
		DEV_ASSERT(false && "Unknown resource type to make mutable.");
		return false;
	}
}

bool RenderingDevice::_dependencies_make_mutable_recursive(
	RID p_id, RDG::ResourceTracker* p_resource_tracker)
{
	bool made_mutable = false;
	HashMap<RID, HashSet<RID>>::Iterator E = data->dependency_map.find(p_id);
	if (E) {
		for (RID rid : E->value) {
			made_mutable = _dependency_make_mutable(rid, p_id, p_resource_tracker) || made_mutable;
		}
	}

	return made_mutable;
}

bool RenderingDevice::_dependencies_make_mutable(RID p_id, RDG::ResourceTracker* p_resource_tracker)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);
	return _dependencies_make_mutable_recursive(p_id, p_resource_tracker);
}

void RenderingDevice::free_rid(RID p_rid)
{
	ERR_RENDER_THREAD_GUARD();
	if (p_rid.is_null()) {
		return;
	}
	_free_dependencies(p_rid);
	_free_internal(p_rid);
}

void RenderingDevice::_free_internal(RID p_id)
{
#ifdef DEV_ENABLED
	String resource_name;
	if (resource_names.has(p_id)) {
		resource_name = resource_names[p_id];
		resource_names.erase(p_id);
	}
#endif

	// Push everything so it's disposed of next time this frame index is processed (means, it's safe
	// to do it).
	if (data->texture_owner.owns(p_id)) {
		Texture* texture = data->texture_owner.get_or_null(p_id);
		_check_transfer_worker_texture(texture);

		RDG::ResourceTracker* draw_tracker = texture->draw_tracker;
		if (draw_tracker != nullptr) {
			draw_tracker->reference_count--;
			if (draw_tracker->reference_count == 0) {
				RDG::resource_tracker_free(draw_tracker);

				if (texture->owner.is_valid() && (texture->slice_type != RDC::TEXTURE_SLICE_MAX)) {
					// If this was a texture slice, erase the tracker from the map.
					Texture* owner_texture = data->texture_owner.get_or_null(texture->owner);
					if (owner_texture != nullptr && owner_texture->slice_trackers != nullptr) {
						owner_texture->slice_trackers->erase(texture->slice_rect);

						if (owner_texture->slice_trackers->is_empty()) {
							memdelete(owner_texture->slice_trackers);
							owner_texture->slice_trackers = nullptr;
						}
					}
				}
			}
		}

		data->frames[data->frame].textures_to_dispose_of.push_back(*texture);
		data->texture_owner.free(p_id);
	}
	else if (data->framebuffer_owner.owns(p_id)) {
		Framebuffer* framebuffer = data->framebuffer_owner.get_or_null(p_id);
		data->frames[data->frame].framebuffers_to_dispose_of.push_back(*framebuffer);

		if (framebuffer->invalidated_callback != nullptr) {
			framebuffer->invalidated_callback(framebuffer->invalidated_callback_userdata);
		}

		data->framebuffer_owner.free(p_id);
	}
	else if (data->sampler_owner.owns(p_id)) {
		RDD::SamplerID sampler_driver_id = *data->sampler_owner.get_or_null(p_id);
		data->frames[data->frame].samplers_to_dispose_of.push_back(sampler_driver_id);
		data->sampler_owner.free(p_id);
	}
	else if (data->vertex_buffer_owner.owns(p_id)) {
		Buffer* vertex_buffer = data->vertex_buffer_owner.get_or_null(p_id);
		_check_transfer_worker_buffer(vertex_buffer);

		RDG::resource_tracker_free(vertex_buffer->draw_tracker);
		data->frames[data->frame].buffers_to_dispose_of.push_back(*vertex_buffer);
		data->vertex_buffer_owner.free(p_id);
	}
	else if (data->vertex_array_owner.owns(p_id)) {
		data->vertex_array_owner.free(p_id);
	}
	else if (data->index_buffer_owner.owns(p_id)) {
		IndexBuffer* index_buffer = data->index_buffer_owner.get_or_null(p_id);
		_check_transfer_worker_buffer(index_buffer);

		RDG::resource_tracker_free(index_buffer->draw_tracker);
		data->frames[data->frame].buffers_to_dispose_of.push_back(*index_buffer);
		data->index_buffer_owner.free(p_id);
	}
	else if (data->index_array_owner.owns(p_id)) {
		data->index_array_owner.free(p_id);
	}
	else if (data->shader_owner.owns(p_id)) {
		Shader* shader = data->shader_owner.get_or_null(p_id);
		if (shader->driver_id) { // Not placeholder?
			data->frames[data->frame].shaders_to_dispose_of.push_back(*shader);
		}
		data->shader_owner.free(p_id);
	}
	else if (data->uniform_buffer_owner.owns(p_id)) {
		Buffer* uniform_buffer = data->uniform_buffer_owner.get_or_null(p_id);
		_check_transfer_worker_buffer(uniform_buffer);

		RDG::resource_tracker_free(uniform_buffer->draw_tracker);
		data->frames[data->frame].buffers_to_dispose_of.push_back(*uniform_buffer);
		data->uniform_buffer_owner.free(p_id);
	}
	else if (data->texture_buffer_owner.owns(p_id)) {
		Buffer* texture_buffer = data->texture_buffer_owner.get_or_null(p_id);
		_check_transfer_worker_buffer(texture_buffer);

		RDG::resource_tracker_free(texture_buffer->draw_tracker);
		data->frames[data->frame].buffers_to_dispose_of.push_back(*texture_buffer);
		data->texture_buffer_owner.free(p_id);
	}
	else if (data->storage_buffer_owner.owns(p_id)) {
		Buffer* storage_buffer = data->storage_buffer_owner.get_or_null(p_id);
		_check_transfer_worker_buffer(storage_buffer);

		RDG::resource_tracker_free(storage_buffer->draw_tracker);
		data->frames[data->frame].buffers_to_dispose_of.push_back(*storage_buffer);
		data->storage_buffer_owner.free(p_id);
	}
	else if (data->uniform_set_owner.owns(p_id)) {
		UniformSet* uniform_set = data->uniform_set_owner.get_or_null(p_id);
		data->frames[data->frame].uniform_sets_to_dispose_of.push_back
(*uniform_set);
		data->uniform_set_owner.free(p_id);

		if (uniform_set->invalidated_callback != nullptr) {
			uniform_set->invalidated_callback(uniform_set->invalidated_callback_userdata);
		}
	}
	else if (data->render_pipeline_owner.owns(p_id)) {
		RenderPipeline* pipeline = data->render_pipeline_owner.get_or_null(p_id);
		data->frames[data->frame].render_pipelines_to_dispose_of.push_back(*pipeline);
		data->render_pipeline_owner.free(p_id);
	}
	else if (data->compute_pipeline_owner.owns(p_id)) {
		ComputePipeline* pipeline = data->compute_pipeline_owner.get_or_null(p_id);
		data->frames[data->frame].compute_pipelines_to_dispose_of.push_back(*pipeline);
		data->compute_pipeline_owner.free(p_id);
	}
	else if (data->acceleration_structure_owner.owns(p_id)) {
		AccelerationStructure* acceleration_structure =
			data->acceleration_structure_owner.get_or_null(p_id);
		if (acceleration_structure->type == RDC::ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL) {
			_blas_remove_tlas_dependencies(acceleration_structure, p_id);
		}
		else if (acceleration_structure->type == RDC::ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL) {
			_tlas_remove_blas_dependencies(acceleration_structure, p_id);
		}
		RDG::resource_tracker_free(acceleration_structure->draw_tracker);
		data->frames[data->frame].acceleration_structures_to_dispose_of.push_back(
			*acceleration_structure);
		data->acceleration_structure_owner.free(p_id);
	}
	else if (data->raytracing_pipeline_owner.owns(p_id)) {
		RaytracingPipeline* pipeline = data->raytracing_pipeline_owner.get_or_null(p_id);
		data->frames[data->frame].raytracing_pipelines_to_dispose_of.push_back(*pipeline);
		data->raytracing_pipeline_owner.free(p_id);
	}
	else if (data->hit_sbt_owner.owns(p_id)) {
		HitShaderBindingTable* hit_sbt = data->hit_sbt_owner.get_or_null(p_id);
		RDG::resource_tracker_free(hit_sbt->draw_tracker);
		data->frames[data->frame].buffers_to_dispose_of.push_back(*hit_sbt);
		data->hit_sbt_owner.free(p_id);
	}
	else {
#ifdef DEV_ENABLED
		ERR_PRINT("Attempted to free invalid ID: " + itos(p_id.get_id()) + " " + resource_name);
#else
		ERR_PRINT("Attempted to free invalid ID: " + itos(p_id.get_id()));
#endif
	}

	data->frames_pending_resources_for_processing = uint32_t(data->frames.size());
}

// The full list of resources that can be named is in the VkObjectType enum.
// We just expose the resources that are owned and can be accessed easily.
void RenderingDevice::set_resource_name(RID p_id, const String& p_name)
{
	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	if (data->texture_owner.owns(p_id)) {
		Texture* texture = data->texture_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_TEXTURE, texture->driver_id, p_name);
	}
	else if (data->framebuffer_owner.owns(p_id)) {
		// Framebuffer *framebuffer = data->framebuffer_owner.get_or_null(p_id);
		//  Not implemented for now as the relationship between Framebuffer and RenderPass is very
		//  complex.
	}
	else if (data->sampler_owner.owns(p_id)) {
		RDD::SamplerID sampler_driver_id = *data->sampler_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_SAMPLER, sampler_driver_id, p_name);
	}
	else if (data->vertex_buffer_owner.owns(p_id)) {
		Buffer* vertex_buffer = data->vertex_buffer_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_BUFFER, vertex_buffer->driver_id, p_name);
	}
	else if (data->index_buffer_owner.owns(p_id)) {
		IndexBuffer* index_buffer = data->index_buffer_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_BUFFER, index_buffer->driver_id, p_name);
	}
	else if (data->shader_owner.owns(p_id)) {
		Shader* shader = data->shader_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_SHADER, shader->driver_id, p_name);
	}
	else if (data->uniform_buffer_owner.owns(p_id)) {
		Buffer* uniform_buffer = data->uniform_buffer_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_BUFFER, uniform_buffer->driver_id, p_name);
	}
	else if (data->texture_buffer_owner.owns(p_id)) {
		Buffer* texture_buffer = data->texture_buffer_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_BUFFER, texture_buffer->driver_id, p_name);
	}
	else if (data->storage_buffer_owner.owns(p_id)) {
		Buffer* storage_buffer = data->storage_buffer_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_BUFFER, storage_buffer->driver_id, p_name);
	}
	else if (data->uniform_set_owner.owns(p_id)) {
		UniformSet* uniform_set = data->uniform_set_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_UNIFORM_SET, uniform_set->driver_id, p_name);
	}
	else if (data->render_pipeline_owner.owns(p_id)) {
		RenderPipeline* pipeline = data->render_pipeline_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_PIPELINE, pipeline->driver_id, p_name);
	}
	else if (data->compute_pipeline_owner.owns(p_id)) {
		ComputePipeline* pipeline = data->compute_pipeline_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_PIPELINE, pipeline->driver_id, p_name);
	}
	else if (data->acceleration_structure_owner.owns(p_id)) {
		AccelerationStructure* acceleration_structure =
			data->acceleration_structure_owner.get_or_null(p_id);
		data->driver->set_object_name(
			RDD::OBJECT_TYPE_ACCELERATION_STRUCTURE, acceleration_structure->driver_id, p_name);
	}
	else if (data->raytracing_pipeline_owner.owns(p_id)) {
		RaytracingPipeline* pipeline = data->raytracing_pipeline_owner.get_or_null(p_id);
		data->driver->set_object_name(
			RDD::OBJECT_TYPE_RAYTRACING_PIPELINE, pipeline->driver_id, p_name);
	}
	else if (data->hit_sbt_owner.owns(p_id)) {
		HitShaderBindingTable* hit_sbt = data->hit_sbt_owner.get_or_null(p_id);
		data->driver->set_object_name(RDD::OBJECT_TYPE_BUFFER, hit_sbt->driver_id, p_name);
	}
	else {
		ERR_PRINT("Attempted to name invalid ID: " + itos(p_id.get_id()));
		return;
	}
#ifdef DEV_ENABLED
	resource_names[p_id] = p_name;
#endif
}

void RenderingDevice::_draw_command_begin_label(String p_label_name, const Color& p_color)
{
	draw_command_begin_label(p_label_name.utf8().span(), p_color);
}

void RenderingDevice::draw_command_begin_label(const Span<char> p_label_name, const Color& p_color)
{
	ERR_RENDER_THREAD_GUARD();

	if (!data->context->is_debug_utils_enabled()) {
		return;
	}

	data->draw_graph.begin_label(p_label_name, p_color);
}

void RenderingDevice::draw_command_end_label()
{
	ERR_RENDER_THREAD_GUARD();

	data->draw_graph.end_label();
}

String RenderingDevice::get_device_vendor_name()
{
	if (!data) {
		return "Unknown";
	}
	return _get_device_vendor_name(data->device);
}

String RenderingDevice::get_device_name() { return data->device.name; }

RenderingDeviceEnums::DeviceType RenderingDevice::get_device_type()
{
	return RenderingDeviceEnums::DeviceType(data->device.type);
}

String RenderingDevice::get_device_api_name()
{
	if (!data) {
		return "Unknown";
	}
	return data->driver->get_api_name();
}

bool RenderingDevice::is_composite_alpha_supported()
{
	return data->driver->is_composite_alpha_supported(data->main_queue);
}

String RenderingDevice::get_device_api_version()
{
	if (!data) {
		return "Unknown";
	}
	return data->driver->get_api_version();
}

String RenderingDevice::get_device_pipeline_cache_uuid()
{
	if (!data) {
		return "Unknown";
	}
	return data->driver->get_pipeline_cache_uuid();
}

RDC::DriverWorkarounds RenderingDevice::get_driver_workarounds()
{
	return data->driver->get_driver_workarounds();
}

void RenderingDevice::_free_pending_resources(int p_frame)
{
	// Free in dependency usage order, so nothing weird happens.
	// Pipelines.
	while (data->frames[p_frame].render_pipelines_to_dispose_of.front()) {
		RenderPipeline* pipeline =
			&data->frames[p_frame].render_pipelines_to_dispose_of.front()->get();

		data->driver->pipeline_free(pipeline->driver_id);

		data->frames[p_frame].render_pipelines_to_dispose_of.pop_front();
	}

	while (data->frames[p_frame].compute_pipelines_to_dispose_of.front()) {
		ComputePipeline* pipeline =
			&data->frames[p_frame].compute_pipelines_to_dispose_of.front()->get();

		data->driver->pipeline_free(pipeline->driver_id);

		data->frames[p_frame].compute_pipelines_to_dispose_of.pop_front();
	}

	while (data->frames[p_frame].raytracing_pipelines_to_dispose_of.front()) {
		RaytracingPipeline* pipeline =
			&data->frames[p_frame].raytracing_pipelines_to_dispose_of.front()->get();

		data->driver->buffer_free(pipeline->sbt_buffer.driver_id);
		data->buffer_memory.sub(pipeline->sbt_buffer.size);

		data->driver->raytracing_pipeline_free(pipeline->driver_id);

		data->frames[p_frame].raytracing_pipelines_to_dispose_of.pop_front();
	}

	// Acceleration structures.
	while (data->frames[p_frame].acceleration_structures_to_dispose_of.front()) {
		AccelerationStructure& acceleration_structure =
			data->frames[p_frame].acceleration_structures_to_dispose_of.front()->get();

		data->driver->acceleration_structure_free(acceleration_structure.driver_id);

		if (acceleration_structure.scratch_buffer) {
			data->buffer_memory.sub(
				data->driver->buffer_get_allocation_size(acceleration_structure.scratch_buffer));
			data->driver->buffer_free(acceleration_structure.scratch_buffer);
		}

		if (!acceleration_structure.instance_buffers.is_empty()) {
			uint32_t instance_buffer_size =
				data->driver->api_trait_get(RDD::API_TRAIT_ACCELERATION_STRUCTURE_INSTANCE_SIZE) *
				acceleration_structure.max_instance_count;
			for (AccelerationStructure::InstanceBuffer& instance_buffer :
				acceleration_structure.instance_buffers) {
				data->driver->buffer_unmap(instance_buffer.driver_id);
				data->driver->buffer_free(instance_buffer.driver_id);
			}

			data->buffer_memory.sub(
				instance_buffer_size * acceleration_structure.instance_buffers.size());
		}

		data->frames[p_frame].acceleration_structures_to_dispose_of.pop_front();
	}

	// Uniform sets.
	while (data->frames[p_frame].uniform_sets_to_dispose_of.front()) {
		UniformSet* uniform_set = &data->frames[p_frame].uniform_sets_to_dispose_of.front()->get();

		data->driver->uniform_set_free(uniform_set->driver_id);

		data->frames[p_frame].uniform_sets_to_dispose_of.pop_front();
	}

	// Shaders.
	while (data->frames[p_frame].shaders_to_dispose_of.front()) {
		Shader* shader = &data->frames[p_frame].shaders_to_dispose_of.front()->get();

		data->driver->shader_free(shader->driver_id);

		data->frames[p_frame].shaders_to_dispose_of.pop_front();
	}

	// Samplers.
	while (data->frames[p_frame].samplers_to_dispose_of.front()) {
		RDD::SamplerID sampler = data->frames[p_frame].samplers_to_dispose_of.front()->get();

		data->driver->sampler_free(sampler);

		data->frames[p_frame].samplers_to_dispose_of.pop_front();
	}

	// Framebuffers.
	while (data->frames[p_frame].framebuffers_to_dispose_of.front()) {
		Framebuffer* framebuffer = &data->frames[p_frame].framebuffers_to_dispose_of.front()->get();
		data->draw_graph.framebuffer_cache_free(data->driver, framebuffer->framebuffer_cache);
		data->frames[p_frame].framebuffers_to_dispose_of.pop_front();
	}

	// Textures.
	while (data->frames[p_frame].textures_to_dispose_of.front()) {
		Texture* texture = &data->frames[p_frame].textures_to_dispose_of.front()->get();
		if (texture->bound) {
			WARN_PRINT("Deleted a texture while it was bound.");
		}

		_texture_free_shared_fallback(texture);

		data->texture_memory.sub(data->driver->texture_get_allocation_size(texture->driver_id));
		data->driver->texture_free(texture->driver_id);

		data->frames[p_frame].textures_to_dispose_of.pop_front();
	}

	// Buffers.
	while (data->frames[p_frame].buffers_to_dispose_of.front()) {
		Buffer& buffer = data->frames[p_frame].buffers_to_dispose_of.front()->get();
		data->driver->buffer_free(buffer.driver_id);
		data->buffer_memory.sub(buffer.size);

		data->frames[p_frame].buffers_to_dispose_of.pop_front();
	}

	if (data->frames_pending_resources_for_processing > 0u) {
		--data->frames_pending_resources_for_processing;
	}
}

uint32_t RenderingDevice::get_frame_delay() { return data->frames.size(); }

uint64_t RenderingDevice::get_memory_usage(MemoryType p_type)
{
	switch (p_type) {
	case MEMORY_BUFFERS: {
		return data->buffer_memory.get();
	}
	case MEMORY_TEXTURES: {
		return data->texture_memory.get();
	}
	case MEMORY_TOTAL: {
		return data->driver->get_total_memory_used();
	}
	default: {
		DEV_ASSERT(false);
		return 0;
	}
	}
}

void RenderingDevice::execute_chained_cmds(bool p_present_swap_chain,
	RenderingDeviceDriver::FenceID p_draw_fence,
	RenderingDeviceDriver::SemaphoreID p_dst_draw_semaphore_to_signal)
{
	// Execute command buffers and use semaphores to wait on the execution of the previous one.
	// Normally there's only one command buffer, but data->driver workarounds can force situations
	// where there'll be more.
	uint32_t command_buffer_count = 1;
	RDG::CommandBufferPool& buffer_pool = data->frames[data->frame].command_buffer_pool;
	if (buffer_pool.buffers_used > 0) {
		command_buffer_count += buffer_pool.buffers_used;
		buffer_pool.buffers_used = 0;
	}

	thread_local LocalVector<RDD::SwapChainID> swap_chains;
	swap_chains.clear();

	// Instead of having just one command; we have potentially many (which had to be split due to an
	// Adreno workaround on mobile, only if the workaround is active). Thus we must execute all of
	// them and chain them together via semaphores as dependent executions.
	thread_local LocalVector<RDD::SemaphoreID> wait_semaphores;
	wait_semaphores = data->frames[data->frame].semaphores_to_wait_on;

	for (uint32_t i = 0; i < command_buffer_count; i++) {
		RDD::CommandBufferID command_buffer;
		RDD::SemaphoreID signal_semaphore;
		RDD::FenceID signal_fence;
		if (i > 0) {
			command_buffer = buffer_pool.buffers[i - 1];
		}
		else {
			command_buffer = data->frames[data->frame].command_buffer;
		}

		if (i == (command_buffer_count - 1)) {
			// This is the last command buffer, it should signal the semaphore & fence.
			signal_semaphore = p_dst_draw_semaphore_to_signal;
			signal_fence = p_draw_fence;

			if (p_present_swap_chain) {
				// Just present the swap chains as part of the last command execution.
				swap_chains = data->frames[data->frame].swap_chains_to_present;
			}
		}
		else {
			signal_semaphore = buffer_pool.semaphores[i];
			// Semaphores always need to be signaled if it's not the last command buffer.
		}

		data->driver->command_queue_execute_and_present(data->main_queue, wait_semaphores,
			command_buffer, signal_semaphore ? signal_semaphore : VectorView<RDD::SemaphoreID>(),
			signal_fence, swap_chains);

		// Make the next command buffer wait on the semaphore signaled by this one.
		wait_semaphores.resize(1);
		wait_semaphores[0] = signal_semaphore;
	}

	data->frames[data->frame].semaphores_to_wait_on.clear();
}

void RenderingDevice::_execute_frame(bool p_present)
{
	// Check whether this data->frame should present the swap chains and in which queue.
	const bool frame_can_present =
		p_present && !data->frames[data->frame].swap_chains_to_present.is_empty();
	const bool separate_present_queue = data->main_queue != data->present_queue;

	// The semaphore is required if the data->frame can be presented and a separate present queue is
	// used; since the separate queue will wait for that semaphore before presenting.
	const RDD::SemaphoreID semaphore = (frame_can_present && separate_present_queue)
										   ? data->frames[data->frame].semaphore
										   : RDD::SemaphoreID(nullptr);
	const bool present_swap_chain = frame_can_present && !separate_present_queue;

	execute_chained_cmds(present_swap_chain, data->frames[data->frame].fence, semaphore);
	// Indicate the fence has been signaled so the next time the data->frame's contents need to be
	// used, the CPU needs to wait on the work to be completed.
	data->frames[data->frame].fence_signaled = true;

	if (frame_can_present) {
		if (separate_present_queue) {
			// Issue the presentation separately if the presentation queue is different from the
			// main queue.
			data->driver->command_queue_execute_and_present(data->present_queue,
				data->frames[data->frame].semaphore, {}, {}, {},
				data->frames[data->frame].swap_chains_to_present);
		}

		data->frames[data->frame].swap_chains_to_present.clear();
	}
}

void RenderingDevice::_save_pipeline_cache(void* p_data)
{
	RenderingDevice* self = static_cast<RenderingDevice*>(p_data);

	self->_thread_safe_mutex.lock();
	Vector<uint8_t> cache_blob = self->data->driver->pipeline_cache_serialize();
	self->_thread_safe_mutex.unlock();

	if (cache_blob.is_empty()) {
		return;
	}
	print_verbose(vformat("Updated PSO cache (%.1f MiB)", cache_blob.size() / (1024.0f * 1024.0f)));

	Ref<FileAccess> f =
		FileAccess::open(self->data->pipeline_cache_file_path, FileAccess::WRITE, nullptr);
	if (f.is_valid()) {
		f->store_buffer(cache_blob);
	}
}

template <typename T> void RenderingDevice::_free_rids(T& p_owner, const char* p_type)
{
	LocalVector<RID> owned = p_owner.get_owned_list();
	if (owned.size()) {
		if (owned.size() == 1) {
			WARN_PRINT(vformat("1 RID of type \"%s\" was leaked.", p_type));
		}
		else {
			WARN_PRINT(vformat("%d RIDs of type \"%s\" were leaked.", owned.size(), p_type));
		}
		for (const RID& rid : owned) {
#ifdef DEV_ENABLED
			if (resource_names.has(rid)) {
				print_line(String(" - ") + resource_names[rid]);
			}
#endif
			free_rid(rid);
		}
	}
}

void RenderingDevice::capture_timestamp(const String& p_name)
{
	if (data->max_timestamp_query_elements == 0) {
		return;
	}
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND_MSG(data->draw_list.active && data->draw_list.state.draw_count > 0,
		"Capturing timestamps during draw list creation is not allowed. Offending timestamp was: " +
			p_name);
	ERR_FAIL_COND_MSG(data->compute_list.active && data->compute_list.state.dispatch_count > 0,
		"Capturing timestamps during compute list creation is not allowed. Offending timestamp "
		"was: " +
			p_name);
	ERR_FAIL_COND_MSG(data->raytracing_list.active && data->raytracing_list.state.trace_count > 0,
		"Capturing timestamps during raytracing list creation is not allowed. Offending timestamp "
		"was: " +
			p_name);
	ERR_FAIL_COND_MSG(
		data->frames[data->frame].timestamp_count >= data->max_timestamp_query_elements,
		vformat("Tried capturing more timestamps than the configured maximum (%d). You can "
				"increase this limit in the project settings under 'Debug/Settings' called 'Max "
				"Timestamp Query Elements'.",
			data->max_timestamp_query_elements));

	data->draw_graph.add_capture_timestamp(
		data->frames[data->frame].timestamp_pool, data->frames[data->frame].timestamp_count);

	data->frames[data->frame].timestamp_names[data->frames[data->frame].timestamp_count] = p_name;
	data->frames[data->frame].timestamp_cpu_values[data->frames[data->frame].timestamp_count] =
		OS::get_ticks_usec();
	data->frames[data->frame].timestamp_count++;
}

uint64_t RenderingDevice::get_driver_resource(
	RDC::DriverResource p_resource, RID p_rid, uint64_t p_index)
{
	ERR_RENDER_THREAD_GUARD_V(0);

	uint64_t driver_id = 0;
	switch (p_resource) {
	case RDC::DRIVER_RESOURCE_LOGICAL_DEVICE:
	case RDC::DRIVER_RESOURCE_PHYSICAL_DEVICE:
	case RDC::DRIVER_RESOURCE_TOPMOST_OBJECT:
		break;
	case RDC::DRIVER_RESOURCE_COMMAND_QUEUE:
		driver_id = data->main_queue.id;
		break;
	case RDC::DRIVER_RESOURCE_QUEUE_FAMILY:
		driver_id = data->main_queue_family.id;
		break;
	case RDC::DRIVER_RESOURCE_TEXTURE:
	case RDC::DRIVER_RESOURCE_TEXTURE_VIEW:
	case RDC::DRIVER_RESOURCE_TEXTURE_DATA_FORMAT: {
		Texture* tex = data->texture_owner.get_or_null(p_rid);
		ERR_FAIL_NULL_V(tex, 0);

		driver_id = tex->driver_id.id;
	} break;
	case RDC::DRIVER_RESOURCE_SAMPLER: {
		RDD::SamplerID* sampler_driver_id = data->sampler_owner.get_or_null(p_rid);
		ERR_FAIL_NULL_V(sampler_driver_id, 0);

		driver_id = (*sampler_driver_id).id;
	} break;
	case RDC::DRIVER_RESOURCE_UNIFORM_SET: {
		UniformSet* uniform_set = data->uniform_set_owner.get_or_null(p_rid);
		ERR_FAIL_NULL_V(uniform_set, 0);

		driver_id = uniform_set->driver_id.id;
	} break;
	case RDC::DRIVER_RESOURCE_BUFFER: {
		Buffer* buffer = nullptr;
		if (data->vertex_buffer_owner.owns(p_rid)) {
			buffer = data->vertex_buffer_owner.get_or_null(p_rid);
		}
		else if (data->index_buffer_owner.owns(p_rid)) {
			buffer = data->index_buffer_owner.get_or_null(p_rid);
		}
		else if (data->uniform_buffer_owner.owns(p_rid)) {
			buffer = data->uniform_buffer_owner.get_or_null(p_rid);
		}
		else if (data->texture_buffer_owner.owns(p_rid)) {
			buffer = data->texture_buffer_owner.get_or_null(p_rid);
		}
		else if (data->storage_buffer_owner.owns(p_rid)) {
			buffer = data->storage_buffer_owner.get_or_null(p_rid);
		}
		ERR_FAIL_NULL_V(buffer, 0);

		driver_id = buffer->driver_id.id;
	} break;
	case RDC::DRIVER_RESOURCE_COMPUTE_PIPELINE: {
		ComputePipeline* compute_pipeline = data->compute_pipeline_owner.get_or_null(p_rid);
		ERR_FAIL_NULL_V(compute_pipeline, 0);

		driver_id = compute_pipeline->driver_id.id;
	} break;
	case RDC::DRIVER_RESOURCE_RENDER_PIPELINE: {
		RenderPipeline* render_pipeline = data->render_pipeline_owner.get_or_null(p_rid);
		ERR_FAIL_NULL_V(render_pipeline, 0);

		driver_id = render_pipeline->driver_id.id;
	} break;
	default: {
		ERR_FAIL_V(0);
	} break;
	}

	return data->driver->get_resource_native_handle(p_resource, driver_id);
}

String RenderingDevice::get_driver_and_device_memory_report()
{
	return data->context->get_driver_and_device_memory_report();
}

String RenderingDevice::get_tracked_object_name(uint32_t p_type_index)
{
	return data->context->get_tracked_object_name(p_type_index);
}

uint64_t RenderingDevice::get_tracked_object_type_count()
{
	return data->context->get_tracked_object_type_count();
}

uint64_t RenderingDevice::get_driver_total_memory()
{
	return data->context->get_driver_total_memory();
}

uint64_t RenderingDevice::get_driver_allocation_count()
{
	return data->context->get_driver_allocation_count();
}

uint64_t RenderingDevice::get_driver_memory_by_object_type(uint32_t p_type)
{
	return data->context->get_driver_memory_by_object_type(p_type);
}

uint64_t RenderingDevice::get_driver_allocs_by_object_type(uint32_t p_type)
{
	return data->context->get_driver_allocs_by_object_type(p_type);
}

uint64_t RenderingDevice::get_device_total_memory()
{
	return data->context->get_device_total_memory();
}

uint64_t RenderingDevice::get_device_allocation_count()
{
	return data->context->get_device_allocation_count();
}

uint64_t RenderingDevice::get_device_memory_by_object_type(uint32_t type)
{
	return data->context->get_device_memory_by_object_type(type);
}

uint64_t RenderingDevice::get_device_allocs_by_object_type(uint32_t type)
{
	return data->context->get_device_allocs_by_object_type(type);
}

uint32_t RenderingDevice::get_captured_timestamps_count()
{
	ERR_RENDER_THREAD_GUARD_V(0);
	return data->frames[data->frame].timestamp_result_count;
}

uint64_t RenderingDevice::get_captured_timestamps_frame()
{
	ERR_RENDER_THREAD_GUARD_V(0);
	return data->frames[data->frame].index;
}

uint64_t RenderingDevice::get_captured_timestamp_gpu_time(uint32_t p_index)
{
	ERR_RENDER_THREAD_GUARD_V(0);
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, data->frames[data->frame].timestamp_result_count, 0);
	return data->driver->timestamp_query_result_to_time(
		data->frames[data->frame].timestamp_result_values[p_index]);
}

uint64_t RenderingDevice::get_captured_timestamp_cpu_time(uint32_t p_index)
{
	ERR_RENDER_THREAD_GUARD_V(0);
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, data->frames[data->frame].timestamp_result_count, 0);
	return data->frames[data->frame].timestamp_cpu_result_values[p_index];
}

String RenderingDevice::get_captured_timestamp_name(uint32_t p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, data->frames[data->frame].timestamp_result_count, String());
	return data->frames[data->frame].timestamp_result_names[p_index];
}

uint64_t RenderingDevice::limit_get(RDC::Limit p_limit) { return data->driver->limit_get(p_limit); }

void RenderingDevice::_set_max_fps(int p_max_fps)
{
	for (const KeyValue<DisplayServerEnums::WindowID, RDD::SwapChainID>& it :
		data->screen_swap_chains) {
		data->driver->swap_chain_set_max_fps(it.value, p_max_fps);
	}
}

bool RenderingDevice::has_feature(const RDC::Features p_feature)
{
	// Some features can be deduced from the capabilities without querying the data->driver and
	// looking at the capabilities.
	switch (p_feature) {
	case RDC::SUPPORTS_MULTIVIEW: {
		const RDD::MultiviewCapabilities& multiview_capabilities =
			data->driver->get_multiview_capabilities();
		return multiview_capabilities.is_supported && multiview_capabilities.max_view_count > 1;
	}
	case RDC::SUPPORTS_ATTACHMENT_VRS: {
		const RDD::FragmentShadingRateCapabilities& fsr_capabilities =
			data->driver->get_fragment_shading_rate_capabilities();
		const RDD::FragmentDensityMapCapabilities& fdm_capabilities =
			data->driver->get_fragment_density_map_capabilities();
		return fsr_capabilities.attachment_supported || fdm_capabilities.attachment_supported;
	}
	default:
		return data->driver->has_feature(p_feature);
	}
}

void RenderingDevice::make_current() { data->render_thread_id = Thread::get_caller_id(); }

static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_NONE, RDG::RESOURCE_USAGE_NONE));
static_assert(
	ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_COPY_FROM, RDG::RESOURCE_USAGE_COPY_FROM));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_COPY_TO, RDG::RESOURCE_USAGE_COPY_TO));
static_assert(
	ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_RESOLVE_FROM, RDG::RESOURCE_USAGE_RESOLVE_FROM));
static_assert(
	ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_RESOLVE_TO, RDG::RESOURCE_USAGE_RESOLVE_TO));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_UNIFORM_BUFFER_READ, RDG::RESOURCE_USAGE_UNIFORM_BUFFER_READ));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_INDIRECT_BUFFER_READ, RDG::RESOURCE_USAGE_INDIRECT_BUFFER_READ));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_TEXTURE_BUFFER_READ, RDG::RESOURCE_USAGE_TEXTURE_BUFFER_READ));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE,
	RDG::RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_STORAGE_BUFFER_READ, RDG::RESOURCE_USAGE_STORAGE_BUFFER_READ));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE,
	RDG::RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_VERTEX_BUFFER_READ, RDG::RESOURCE_USAGE_VERTEX_BUFFER_READ));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_INDEX_BUFFER_READ, RDG::RESOURCE_USAGE_INDEX_BUFFER_READ));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE, RDG::RESOURCE_USAGE_TEXTURE_SAMPLE));
static_assert(ENUM_MEMBERS_EQUAL(
	RD::CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ, RDG::RESOURCE_USAGE_STORAGE_IMAGE_READ));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE,
	RDG::RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE,
	RDG::RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE,
	RDG::RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_GENERAL, RDG::RESOURCE_USAGE_GENERAL));
static_assert(ENUM_MEMBERS_EQUAL(RD::CALLBACK_RESOURCE_USAGE_MAX, RDG::RESOURCE_USAGE_MAX));
#ifndef VOLTAIRE_BUILD
Vector<uint8_t> compile_glslang_shader(RenderingDeviceCommons::ShaderStage p_stage,
	const String& p_source, RenderingDeviceCommons::ShaderLanguageVersion p_language,
	RenderingDeviceCommons::ShaderSpirvVersion p_spirv, String* r_error)
{
	if (r_error) {
		*r_error = "glslang integration is disabled.";
	}
	return Vector<uint8_t>();
}
#endif

void RenderingDevice::_stall_for_previous_frames()
{
	for (uint32_t i = 0; i < data->frames.size(); i++) {
		if (data->frames[i].fence_signaled) {
			data->driver->fence_wait(data->frames[i].fence);
			data->frames[i].fence_signaled = false;
		}
	}
}

void RenderingDevice::_flush_and_stall_for_all_frames()
{
	_stall_for_previous_frames();
	swap_buffers(false);
	_stall_for_previous_frames();
}

void RenderingDevice::swap_buffers(bool p_present)
{
	ERR_RENDER_THREAD_GUARD();

	if (p_present) {
		for (const KeyValue<DisplayServerEnums::WindowID, RDD::SwapChainID>& it :
			data->screen_swap_chains) {
			if (data->screen_framebuffers.has(it.key)) {
				data->frames[data->frame].swap_chains_to_present.push_back(it.value);
			}
		}
	}

	RDD::CommandBufferID command_buffer = data->frames[data->frame].command_buffer;
	data->driver->command_buffer_begin(command_buffer);

	_submit_transfer_workers(command_buffer);
	_submit_transfer_barriers(command_buffer);

	data->draw_graph.end(RENDER_GRAPH_REORDER, RENDER_GRAPH_FULL_BARRIERS, command_buffer,
		data->frames[data->frame].command_buffer_pool);

	data->driver->command_buffer_end(command_buffer);

	_execute_frame(p_present);

	if (p_present) {
		data->screen_framebuffers.clear();
	}
	data->frame = (data->frame + 1) % data->frames.size();
	data->frames_drawn++;

	if (data->frames[data->frame].fence_signaled) {
		data->driver->fence_wait(data->frames[data->frame].fence);
		data->frames[data->frame].fence_signaled = false;
	}

	data->driver->command_pool_reset(data->frames[data->frame].command_pool);
	if (p_present) {
		update_perf_report();
		data->driver->linear_uniform_set_pools_reset(data->frame);
	}
	_free_pending_resources(data->frame);

	if (data->upload_staging_buffers.used) {
		data->upload_staging_buffers.current =
			(data->upload_staging_buffers.current + 1) %
			data->upload_staging_buffers.blocks.size();
		data->upload_staging_buffers.used = false;
	}

	if (data->download_staging_buffers.used) {
		data->download_staging_buffers.current =
			(data->download_staging_buffers.current + 1) %
			data->download_staging_buffers.blocks.size();
		data->download_staging_buffers.used = false;
	}

	data->draw_graph.begin();
}

Error RenderingDevice::screen_free(int p_screen)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);
	if (data && data->screen_swap_chains.has(p_screen)) {
		_stall_for_previous_frames();
		data->driver->swap_chain_free(data->screen_swap_chains[p_screen]);
		data->screen_swap_chains.erase(p_screen);
		data->screen_framebuffers.erase(p_screen);
	}
	return OK;
}

RID RenderingDevice::render_pipeline_create(RID p_shader, FramebufferFormatID p_framebuffer_format,
	VertexFormatID p_vertex_format, RDC::RenderPrimitive p_render_primitive,
	const RDC::PipelineRasterizationState& p_rasterization_state,
	const RDC::PipelineMultisampleState& p_multisample_state,
	const RDC::PipelineDepthStencilState& p_depth_stencil_state,
	const RDC::PipelineColorBlendState& p_blend_state, uint32_t p_dynamic_state_flags,
	uint32_t p_for_render_pass,
	const Vector<RDC::PipelineSpecializationConstant>& p_specialization_constants)
{
	ERR_FAIL_NULL_V(data, RID());
	ERR_FAIL_COND_V(!data->shader_owner.owns(p_shader), RID());

	Shader* shader = data->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RID());

	ERR_FAIL_COND_V(!data->framebuffer_formats.has(p_framebuffer_format), RID());
	const FramebufferFormat& fb_format = data->framebuffer_formats[p_framebuffer_format];
	ERR_FAIL_COND_V(p_for_render_pass >= (uint32_t)fb_format.E->key().passes.size(), RID());

	RDD::RenderPassID render_pass = fb_format.render_pass;
	VectorView<int32_t> color_attachments =
		fb_format.E->key().passes[p_for_render_pass].color_attachments;

	RDD::VertexFormatID vertex_format;
	if (data->vertex_formats.has(p_vertex_format)) {
		vertex_format = data->vertex_formats[p_vertex_format].driver_id;
	}

	RDD::PipelineID pipeline_id = data->driver->render_pipeline_create(shader->driver_id,
		vertex_format, p_render_primitive, p_rasterization_state, p_multisample_state,
		p_depth_stencil_state, p_blend_state, color_attachments, p_dynamic_state_flags, render_pass,
		p_for_render_pass, p_specialization_constants);

	if (unlikely(!pipeline_id &&
				 data->driver->get_driver_workarounds()
					 .dont_print_on_render_pipeline_creation_failure)) {
		return RID();
	}

	ERR_FAIL_COND_V(!pipeline_id, RID());

	RenderPipeline pipeline;
	pipeline.driver_id = pipeline_id;
	pipeline.shader = p_shader;
	pipeline.shader_driver_id = shader->driver_id;
	pipeline.shader_layout_hash = shader->layout_hash;
	pipeline.set_formats = shader->set_formats;
	pipeline.push_constant_size = shader->push_constant_size;
	pipeline.stage_bits = shader->stage_bits;

#ifdef DEBUG_ENABLED
	pipeline.validation.framebuffer_format = p_framebuffer_format;
	pipeline.validation.render_pass = p_for_render_pass;
	pipeline.validation.dynamic_state = p_dynamic_state_flags;
	pipeline.validation.vertex_format = p_vertex_format;
	pipeline.validation.uses_restart_indices =
		p_render_primitive == RDC::RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_RESTART_INDEX;

	static const uint32_t primitive_divisor[RDC::RENDER_PRIMITIVE_MAX] = {
		1, 2, 1, 1, 1, 3, 1, 1, 1, 1, 1};
	pipeline.validation.primitive_divisor = primitive_divisor[p_render_primitive];
	static const uint32_t primitive_minimum[RDC::RENDER_PRIMITIVE_MAX] = {
		1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 1};
	pipeline.validation.primitive_minimum = primitive_minimum[p_render_primitive];
#endif

	RID id = data->render_pipeline_owner.make_rid(pipeline);
	{
		MutexLock<Mutex> _thread_safe_method_lock(_thread_safe_mutex);
#ifdef DEV_ENABLED
		set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
		_add_dependency(id, p_shader);
	}
	return id;
}

RID RenderingDevice::texture_create_from_extension(RDC::TextureType p_type,
	RDC::DataFormat p_format, RDC::TextureSamples p_samples, uint32_t p_usage, uint64_t p_image,
	uint64_t p_width, uint64_t p_height, uint64_t p_depth, uint64_t p_layers, uint64_t p_mipmaps)
{
	return RID();
}

Error RenderingDevice::texture_update(
	RID p_texture, uint32_t p_layer, const Vector<uint8_t>& p_data)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	ERR_FAIL_COND_V_MSG(data->draw_list.active, ERR_INVALID_PARAMETER,
		"Updating textures is forbidden during creation of a draw list.");
	ERR_FAIL_COND_V_MSG(data->compute_list.active, ERR_INVALID_PARAMETER,
		"Updating textures is forbidden during creation of a compute list.");
	ERR_FAIL_COND_V_MSG(data->raytracing_list.active, ERR_INVALID_PARAMETER,
		"Updating textures is forbidden during creation of a raytracing list.");

	Texture* texture = data->texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(texture, ERR_INVALID_PARAMETER);

	if (texture->owner != RID()) {
		p_texture = texture->owner;
		texture = data->texture_owner.get_or_null(texture->owner);
		ERR_FAIL_NULL_V(texture, ERR_BUG);
	}

	ERR_FAIL_COND_V_MSG(texture->bound, ERR_CANT_ACQUIRE_RESOURCE,
		"Texture can't be updated while bound to an active draw list framebuffer.");

	ERR_FAIL_COND_V_MSG(!(texture->usage_flags & RDC::TEXTURE_USAGE_CAN_UPDATE_BIT),
		ERR_INVALID_PARAMETER, "Texture requires `TEXTURE_USAGE_CAN_UPDATE_BIT` to be updatable.");

	uint32_t layer_count = _texture_layer_count(texture);
	ERR_FAIL_COND_V(p_layer >= layer_count, ERR_INVALID_PARAMETER);

	uint32_t width, height;
	uint32_t tight_mip_size = RDC::get_image_format_required_size(texture->format, texture->width,
		texture->height, texture->depth, texture->mipmaps, &width, &height);
	uint32_t required_size = tight_mip_size;
	uint32_t required_align = _texture_alignment(texture);

	ERR_FAIL_COND_V_MSG(required_size != (uint32_t)p_data.size(), ERR_INVALID_PARAMETER,
		"Required size for texture update (" + itos(required_size) +
			") does not match data supplied size (" + itos(p_data.size()) + ").");

	_texture_check_pending_clear(p_texture, texture);
	_check_transfer_worker_texture(texture);

	uint32_t block_w, block_h;
	RDC::get_compressed_image_format_block_dimensions(texture->format, block_w, block_h);

	uint32_t pixel_size = RDC::get_image_format_pixel_size(texture->format);
	uint32_t block_size = RDC::get_compressed_image_format_block_byte_size(texture->format);

	uint32_t region_size = MAX(64u, data->texture_upload_region_size_px);

	const uint8_t* read_ptr = p_data.ptr();

	thread_local LocalVector<RDG::RecordedBufferToTextureCopy>
		command_buffer_to_texture_copies_vector;
	command_buffer_to_texture_copies_vector.clear();

	_texture_update_shared_fallback(p_texture, texture, true);

	uint32_t mipmap_offset = 0;
	uint32_t logic_width = texture->width;
	uint32_t logic_height = texture->height;

	for (uint32_t mm_i = 0; mm_i < texture->mipmaps; mm_i++) {
		uint32_t depth = 0;
		uint32_t image_total = RDC::get_image_format_required_size(texture->format, texture->width,
			texture->height, texture->depth, mm_i + 1, &width, &height, &depth);

		const uint8_t* read_ptr_mipmap = read_ptr + mipmap_offset;
		tight_mip_size = image_total - mipmap_offset;

		for (uint32_t z = 0; z < depth; z++) {
			const uint8_t* read_ptr_mipmap_layer = read_ptr_mipmap + (tight_mip_size / depth) * z;
			for (uint32_t y = 0; y < height; y += region_size) {
				for (uint32_t x = 0; x < width; x += region_size) {
					uint32_t region_w = MIN(region_size, width - x);
					uint32_t region_h = MIN(region_size, height - y);

					uint32_t region_logic_w = MIN(region_size, logic_width - x);
					uint32_t region_logic_h = MIN(region_size, logic_height - y);

					uint32_t region_pitch = RDC::get_compressed_image_format_pixels_shifted(
						texture->format, region_w * pixel_size * block_w);
					uint32_t pitch_step =
						data->driver->api_trait_get(RDD::API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP);
					region_pitch = STEPIFY(region_pitch, pitch_step);
					uint32_t to_allocate = region_pitch * region_h;
					uint32_t alloc_offset = 0, alloc_size = 0;
					StagingRequiredAction required_action;
					Error err = _staging_buffer_allocate(data->upload_staging_buffers, to_allocate,
						required_align, alloc_offset, alloc_size, required_action, false);
					ERR_FAIL_COND_V(err, ERR_CANT_CREATE);

					if (!command_buffer_to_texture_copies_vector.is_empty() &&
						required_action == STAGING_REQUIRED_ACTION_FLUSH_AND_STALL_ALL) {
						if (_texture_make_mutable(texture, p_texture)) {
							data->draw_graph.add_synchronization();
						}

						data->draw_graph.add_texture_update(texture->driver_id,
							texture->draw_tracker, command_buffer_to_texture_copies_vector);
						command_buffer_to_texture_copies_vector.clear();
					}

					_staging_buffer_execute_required_action(
						data->upload_staging_buffers, required_action);

					uint8_t* write_ptr =
						data->upload_staging_buffers.blocks[data->upload_staging_buffers.current]
							.data_ptr +
						alloc_offset;

					ERR_FAIL_COND_V(region_w % block_w, ERR_BUG);
					ERR_FAIL_COND_V(region_h % block_h, ERR_BUG);

					_copy_region_block_or_regular(read_ptr_mipmap_layer, write_ptr, x, y, width,
						region_w, region_h, block_w, block_h, region_pitch, pixel_size, block_size);

					RDD::BufferTextureCopyRegion copy_region;
					copy_region.buffer_offset = alloc_offset;
					copy_region.row_pitch = region_pitch;
					copy_region.texture_subresource.aspect =
						(texture->read_aspect_flags & RDD::TEXTURE_ASPECT_DEPTH_BIT)
							? RDD::TEXTURE_ASPECT_DEPTH
							: RDD::TEXTURE_ASPECT_COLOR;
					copy_region.texture_subresource.mipmap = mm_i;
					copy_region.texture_subresource.layer = p_layer;
					copy_region.texture_offset = Vector3i(x, y, z);
					copy_region.texture_region_size = Vector3i(region_logic_w, region_logic_h, 1);

					RDG::RecordedBufferToTextureCopy buffer_to_texture_copy;
					buffer_to_texture_copy.from_buffer =
						data->upload_staging_buffers.blocks[data->upload_staging_buffers.current]
							.driver_id;
					buffer_to_texture_copy.region = copy_region;
					command_buffer_to_texture_copies_vector.push_back(buffer_to_texture_copy);

					data->upload_staging_buffers.blocks.write[data->upload_staging_buffers.current]
						.fill_amount = alloc_offset + alloc_size;
				}
			}
		}

		mipmap_offset = image_total;
		logic_width = MAX(1u, logic_width >> 1);
		logic_height = MAX(1u, logic_height >> 1);
	}

	if (_texture_make_mutable(texture, p_texture)) {
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_texture_update(
		texture->driver_id, texture->draw_tracker, command_buffer_to_texture_copies_vector);

	return OK;
}

void RenderingDevice::compute_list_add_barrier(ComputeListID p_list) {}

Error RenderingDevice::initialize(
	RenderingContextDriver* p_context, RenderingContextDriver::Device p_device)
{
	if (data == nullptr) {
		data = memnew(Data);
		data->render_thread_id = Thread::get_caller_id();
	}

	data->context = p_context;
	data->driver = data->context->driver_create();
	ERR_FAIL_NULL_V(data->driver, ERR_CANT_CREATE);

	data->device = p_device;

	uint32_t device_count = data->context->device_get_count();
	uint32_t device_index = 0;
	for (uint32_t i = 0; i < device_count; i++) {
		RenderingContextDriver::Device dev = data->context->device_get(i);
		if (memcmp(&dev, &p_device, sizeof(RenderingContextDriver::Device)) == 0) {
			device_index = i;
			break;
		}
	}

	const uint32_t frame_count = 3;
	data->frames.resize(frame_count);

	Error err = data->driver->initialize(device_index, frame_count);
	ERR_FAIL_COND_V(err != OK, err);

	data->main_queue_family =
		data->driver->command_queue_family_get(RDD::COMMAND_QUEUE_FAMILY_GRAPHICS_BIT);
	data->main_queue = data->driver->command_queue_create(data->main_queue_family, true);

	data->transfer_queue_family =
		data->driver->command_queue_family_get(RDD::COMMAND_QUEUE_FAMILY_TRANSFER_BIT);
	if (!data->transfer_queue_family) {
		data->transfer_queue_family = data->main_queue_family;
	}
	data->transfer_queue = data->driver->command_queue_create(data->transfer_queue_family, false);

	data->present_queue_family = data->main_queue_family;
	data->present_queue = data->main_queue;

	data->transfer_worker_pool_max_size = MAX(1u, uint32_t(OS::get_processor_count()));
	data->transfer_worker_pool.resize(data->transfer_worker_pool_max_size);
	data->transfer_worker_operation_used_by_draw.resize(data->transfer_worker_pool_max_size);
	for (uint32_t i = 0; i < data->transfer_worker_pool_max_size; i++) {
		data->transfer_worker_pool[i] = nullptr;
		data->transfer_worker_operation_used_by_draw[i] = 0;
	}

	for (uint32_t i = 0; i < frame_count; i++) {
		data->frames[i].command_pool = data->driver->command_pool_create(
			data->main_queue_family, RDD::COMMAND_BUFFER_TYPE_PRIMARY);
		data->frames[i].command_buffer_pool.pool = data->frames[i].command_pool;
		data->frames[i].command_buffer =
			data->driver->command_buffer_create(data->frames[i].command_pool);
		data->frames[i].semaphore = data->driver->semaphore_create();
		data->frames[i].fence = data->driver->fence_create();
		data->frames[i].fence_signaled = false;

		data->frames[i].transfer_worker_semaphores.resize(data->transfer_worker_pool_max_size);
		for (uint32_t j = 0; j < data->transfer_worker_pool_max_size; j++) {
			data->frames[i].transfer_worker_semaphores[j] = data->driver->semaphore_create();
		}
	}

	// Start from frame count so staging blocks and persistent dynamic buffers treat 0 as old.
	data->frames_drawn = frame_count;

	data->draw_graph.initialize(data->driver, &RenderingDevice::_render_pass_create_from_graph,
		frame_count, data->main_queue_family, 4);
	data->draw_graph.begin();

	data->upload_staging_buffers.block_size = 256 * 1024;
	data->upload_staging_buffers.max_size = 128 * 1024 * 1024;
	data->upload_staging_buffers.current = 0;
	data->upload_staging_buffers.used = false;
	data->upload_staging_buffers.usage_bits = RDD::BUFFER_USAGE_TRANSFER_FROM_BIT;

	data->download_staging_buffers.block_size = data->upload_staging_buffers.block_size;
	data->download_staging_buffers.max_size = data->upload_staging_buffers.max_size;
	data->download_staging_buffers.current = 0;
	data->download_staging_buffers.used = false;
	data->download_staging_buffers.usage_bits = RDD::BUFFER_USAGE_TRANSFER_TO_BIT;

	data->texture_upload_region_size_px = 64;
	data->texture_download_region_size_px = 64;

	for (uint32_t i = 0; i < frame_count; i++) {
		err = _insert_staging_block(data->upload_staging_buffers);
		ERR_FAIL_COND_V(err != OK, err);

		err = _insert_staging_block(data->download_staging_buffers);
		ERR_FAIL_COND_V(err != OK, err);
	}

	data->draw_list = DrawList();
	data->compute_list = ComputeList();
	data->raytracing_list = RaytracingList();

	_vrs_detect_method();

	return OK;
}

void RenderingDevice::finalize()
{
	if (data == nullptr) {
		return;
	}

	if (!data->frames.is_empty()) {
		_flush_and_stall_for_all_frames();
	}

	_submit_transfer_workers();
	_wait_for_transfer_workers();

	data->draw_graph.finalize();

	_free_rids(data->render_pipeline_owner, "Pipeline");
	_free_rids(data->compute_pipeline_owner, "Compute");
	_free_rids(data->raytracing_pipeline_owner, "RaytracingPipeline");
	_free_rids(data->acceleration_structure_owner, "AccelerationStructure");
	_free_rids(data->hit_sbt_owner, "HitShaderBindingTable");
	_free_rids(data->uniform_set_owner, "UniformSet");
	_free_rids(data->texture_buffer_owner, "TextureBuffer");
	_free_rids(data->storage_buffer_owner, "StorageBuffer");
	_free_rids(data->uniform_buffer_owner, "UniformBuffer");
	_free_rids(data->shader_owner, "Shader");
	_free_rids(data->index_array_owner, "IndexArray");
	_free_rids(data->index_buffer_owner, "IndexBuffer");
	_free_rids(data->vertex_array_owner, "VertexArray");
	_free_rids(data->vertex_buffer_owner, "VertexBuffer");
	_free_rids(data->framebuffer_owner, "Framebuffer");
	_free_rids(data->sampler_owner, "Sampler");
	{
		LocalVector<RID> owned = data->texture_owner.get_owned_list();
		if (owned.size()) {
			LocalVector<RID> owned_non_shared;
			for (const RID& texture_rid : owned) {
				if (texture_is_shared(texture_rid)) {
					free_rid(texture_rid);
				}
				else {
					owned_non_shared.push_back(texture_rid);
				}
			}
			for (const RID& texture_rid : owned_non_shared) {
				free_rid(texture_rid);
			}
		}
	}

	_free_transfer_workers();

	for (uint32_t i = 0; i < data->frames.size(); i++) {
		int f = (data->frame + i) % data->frames.size();
		_free_pending_resources(f);
		if (data->frames[i].command_pool) {
			data->driver->command_pool_free(data->frames[i].command_pool);
		}
		if (data->frames[i].timestamp_pool) {
			data->driver->timestamp_query_pool_free(data->frames[i].timestamp_pool);
		}
		if (data->frames[i].semaphore) {
			data->driver->semaphore_free(data->frames[i].semaphore);
		}
		if (data->frames[i].fence) {
			data->driver->fence_free(data->frames[i].fence);
		}

		RDG::CommandBufferPool& buffer_pool = data->frames[i].command_buffer_pool;
		for (uint32_t j = 0; j < buffer_pool.buffers.size(); j++) {
			data->driver->semaphore_free(buffer_pool.semaphores[j]);
		}

		for (uint32_t j = 0; j < data->frames[i].transfer_worker_semaphores.size(); j++) {
			if (data->frames[i].transfer_worker_semaphores[j]) {
				data->driver->semaphore_free(data->frames[i].transfer_worker_semaphores[j]);
			}
		}
	}

	data->frames.clear();

	for (int i = 0; i < data->upload_staging_buffers.blocks.size(); i++) {
		data->driver->buffer_unmap(data->upload_staging_buffers.blocks[i].driver_id);
		data->driver->buffer_free(data->upload_staging_buffers.blocks[i].driver_id);
	}

	for (int i = 0; i < data->download_staging_buffers.blocks.size(); i++) {
		data->driver->buffer_unmap(data->download_staging_buffers.blocks[i].driver_id);
		data->driver->buffer_free(data->download_staging_buffers.blocks[i].driver_id);
	}

	while (data->vertex_formats.size()) {
		HashMap<VertexFormatID, VertexDescriptionCache>::Iterator temp =
			data->vertex_formats.begin();
		data->driver->vertex_format_free(temp->value.driver_id);
		data->vertex_formats.remove(temp);
	}

	for (KeyValue<FramebufferFormatID, FramebufferFormat>& E : data->framebuffer_formats) {
		data->driver->render_pass_free(E.value.render_pass);
	}
	data->framebuffer_formats.clear();

	for (const KeyValue<DisplayServerEnums::WindowID, RDD::SwapChainID>& it :
		data->screen_swap_chains) {
		data->driver->swap_chain_free(it.value);
	}
	data->screen_swap_chains.clear();

	if (data->present_queue) {
		if (data->main_queue != data->present_queue) {
			data->driver->command_queue_free(data->present_queue);
		}
		data->present_queue = RDD::CommandQueueID();
	}

	if (data->transfer_queue) {
		if (data->main_queue != data->transfer_queue) {
			data->driver->command_queue_free(data->transfer_queue);
		}
		data->transfer_queue = RDD::CommandQueueID();
	}

	if (data->main_queue) {
		data->driver->command_queue_free(data->main_queue);
		data->main_queue = RDD::CommandQueueID();
	}

	if (data->driver != nullptr) {
		data->context->driver_free(data->driver);
		data->driver = nullptr;
	}

	memdelete(data);
	data = nullptr;
}

RID RenderingDevice::index_buffer_create(uint32_t p_index_count, RDC::IndexBufferFormat p_format,
	Span<uint8_t> p_data, bool p_use_restart_indices)
{
	MutexLock<Mutex> _thread_safe_method_lock(_thread_safe_mutex);
	IndexBuffer ib;
	uint32_t index_size = (p_format == RDC::INDEX_BUFFER_FORMAT_UINT16) ? 2 : 4;
	ib.size = p_index_count * index_size;
	ib.format = p_format;
	ib.index_count = p_index_count;
	ib.supports_restart_indices = p_use_restart_indices;
	ib.max_index = (p_format == RDC::INDEX_BUFFER_FORMAT_UINT16) ? 0xFFFF : 0xFFFFFFFF;
	ib.usage = RDD::BUFFER_USAGE_TRANSFER_FROM_BIT | RDD::BUFFER_USAGE_TRANSFER_TO_BIT |
			   RDD::BUFFER_USAGE_INDEX_BIT;

	ib.driver_id = data->driver->buffer_create(
		ib.size, ib.usage, RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	ERR_FAIL_COND_V(!ib.driver_id, RID());
	if (p_data.is_empty()) {
		ib.draw_tracker = RDG::resource_tracker_create();
		ib.draw_tracker->buffer_driver_id = ib.driver_id;
	}
	data->buffer_memory.add(ib.size);
	if (!p_data.is_empty()) {
		Error err = _buffer_initialize(&ib, p_data);
		if (err != OK) {
			data->driver->buffer_free(ib.driver_id);
			ERR_FAIL_V(RID());
		}
	}
	RID id = data->index_buffer_owner.make_rid(ib);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	return id;
}

RID RenderingDevice::vertex_buffer_create(
	uint32_t p_size_bytes, Span<uint8_t> p_data, uint32_t p_creation_bits)
{
	ERR_FAIL_COND_V(p_data.size() && (uint32_t)p_data.size() != p_size_bytes, RID());

	MutexLock<Mutex> _thread_safe_method_lock(_thread_safe_mutex);
	Buffer vb;
	vb.size = p_size_bytes;
	vb.usage = RDD::BUFFER_USAGE_TRANSFER_FROM_BIT | RDD::BUFFER_USAGE_TRANSFER_TO_BIT |
			   RDD::BUFFER_USAGE_VERTEX_BIT;
	if (p_creation_bits & BUFFER_CREATION_AS_STORAGE_BIT) {
		vb.usage |= RDD::BUFFER_USAGE_STORAGE_BIT;
	}
	if (p_creation_bits & BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT) {
		vb.usage |= RDD::BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT;
		vb.usage &= ~uint32_t(RDD::BUFFER_USAGE_TRANSFER_TO_BIT);
	}
	if (p_creation_bits & BUFFER_CREATION_DEVICE_ADDRESS_BIT) {
		vb.usage |= RDD::BUFFER_USAGE_DEVICE_ADDRESS_BIT;
	}
	if (p_creation_bits & BUFFER_CREATION_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT) {
		vb.usage |= RDD::BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT;
	}

	vb.driver_id = data->driver->buffer_create(
		vb.size, vb.usage, RDD::MEMORY_ALLOCATION_TYPE_GPU, data->frames_drawn);
	ERR_FAIL_COND_V(!vb.driver_id, RID());

	if (p_data.is_empty() || (p_creation_bits & BUFFER_CREATION_AS_STORAGE_BIT) ||
		(p_creation_bits & BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT)) {
		vb.draw_tracker = RDG::resource_tracker_create();
		vb.draw_tracker->buffer_driver_id = vb.driver_id;
	}

	data->buffer_memory.add(vb.size);
	if (!p_data.is_empty()) {
		Error err = _buffer_initialize(&vb, p_data);
		if (err != OK) {
			data->driver->buffer_free(vb.driver_id);
			ERR_FAIL_V(RID());
		}
	}
	RID id = data->vertex_buffer_owner.make_rid(vb);
#ifdef DEV_ENABLED
	set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
	return id;
}

RID RenderingDevice::compute_pipeline_create(
	RID p_shader, const Vector<RDC::PipelineSpecializationConstant>& p_specialization_constants)
{
	ERR_FAIL_NULL_V(data, RID());
	ERR_FAIL_COND_V(!data->shader_owner.owns(p_shader), RID());

	Shader* shader = data->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RID());

	RDD::PipelineID pipeline_id =
		data->driver->compute_pipeline_create(shader->driver_id, p_specialization_constants);
	ERR_FAIL_COND_V(!pipeline_id, RID());

	ComputePipeline pipeline;
	pipeline.driver_id = pipeline_id;
	pipeline.shader = p_shader;
	pipeline.shader_driver_id = shader->driver_id;
	pipeline.shader_layout_hash = shader->layout_hash;
	pipeline.set_formats = shader->set_formats;
	pipeline.push_constant_size = shader->push_constant_size;
	for (uint32_t i = 0; i < 3; i++) {
		pipeline.local_group_size[i] = shader->compute_local_size[i];
	}

	RID id = data->compute_pipeline_owner.make_rid(pipeline);
	{
		MutexLock<Mutex> _thread_safe_method_lock(_thread_safe_mutex);
#ifdef DEV_ENABLED
		set_resource_name(id, "RID:" + itos(id.get_id()));
#endif
		_add_dependency(id, p_shader);
	}
	return id;
}

void RenderingDevice::_draw_list_start(const Rect2i& p_viewport)
{
	data->draw_list.viewport = p_viewport;
	data->draw_list.active = true;
}

void RenderingDevice::_draw_list_end(Rect2i* r_last_viewport)
{
	if (r_last_viewport) {
		*r_last_viewport = data->draw_list.viewport;
	}

	data->draw_list = DrawList();
}

RenderingDevice::DrawListID RenderingDevice::draw_list_switch_to_next_pass()
{
	ERR_RENDER_THREAD_GUARD_V(RDC::INVALID_ID);

	ERR_FAIL_COND_V(!data->draw_list.active, RDC::INVALID_ID);
	ERR_FAIL_COND_V(
		data->draw_list_current_subpass >= data->draw_list_subpass_count - 1, RDC::INVALID_ID);

	data->draw_list_current_subpass++;

	Rect2i viewport;
	_draw_list_end(&viewport);

	data->draw_graph.add_draw_list_next_subpass(RDD::COMMAND_BUFFER_TYPE_PRIMARY);

	_draw_list_start(viewport);

	return int64_t(ID_TYPE_DRAW_LIST) << ID_BASE_SHIFT;
}

void RenderingDevice::draw_list_end()
{
	ERR_RENDER_THREAD_GUARD();
	if (!data->draw_list.active) {
		fprintf(stderr, "[CALLER] %p\n", __builtin_return_address(0));
		fflush(stderr);
	}

	ERR_FAIL_COND_MSG(!data->draw_list.active, "Immediate draw list is already inactive.");

	data->draw_graph.add_draw_list_end();

	_draw_list_end();

	for (uint32_t i = 0; i < data->draw_list_bound_textures.size(); i++) {
		Texture* texture = data->texture_owner.get_or_null(data->draw_list_bound_textures[i]);
		ERR_CONTINUE(!texture);
		if (texture->usage_flags & RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT) {
			texture->bound = false;
		}
		if (texture->usage_flags & RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			texture->bound = false;
		}
		if (texture->usage_flags & RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT) {
			texture->bound = false;
		}
	}

	data->draw_list_bound_textures.clear();
}

RID RenderingDevice::texture_create_shared_from_slice(const TextureView& p_view, RID p_with_texture,
	uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, RDC::TextureSliceType p_slice_type,
	uint32_t p_mipmaps)
{
	ERR_RENDER_THREAD_GUARD_V(RID());

	Texture* src_texture = data->texture_owner.get_or_null(p_with_texture);
	ERR_FAIL_NULL_V(src_texture, RID());

	ERR_FAIL_COND_V(p_slice_type == RDC::TEXTURE_SLICE_2D &&
						src_texture->type != RDC::TEXTURE_TYPE_2D &&
						src_texture->type != RDC::TEXTURE_TYPE_2D_ARRAY &&
						src_texture->type != RDC::TEXTURE_TYPE_CUBE &&
						src_texture->type != RDC::TEXTURE_TYPE_CUBE_ARRAY,
		RID());
	ERR_FAIL_COND_V(
		p_slice_type == RDC::TEXTURE_SLICE_3D && src_texture->type != RDC::TEXTURE_TYPE_3D, RID());
	ERR_FAIL_COND_V(p_mipmaps == 0, RID());
	ERR_FAIL_COND_V(p_mipmap + p_mipmaps > src_texture->mipmaps, RID());

	Texture texture = *src_texture;

	if (p_slice_type == RDC::TEXTURE_SLICE_2D) {
		texture.type = RDC::TEXTURE_TYPE_2D;
		texture.layers = (p_layers > 0) ? p_layers : 1;
	}
	else if (p_slice_type == RDC::TEXTURE_SLICE_CUBEMAP) {
		texture.type = RDC::TEXTURE_TYPE_CUBE;
		texture.layers = (p_layers > 0) ? p_layers : 6;
	}
	else if (p_slice_type == RDC::TEXTURE_SLICE_3D) {
		texture.type = RDC::TEXTURE_TYPE_3D;
		texture.layers = (p_layers > 0) ? p_layers : 1;
	}
	texture.mipmaps = p_mipmaps;

	RDD::TextureView tv;
	bool create_shared = true;
	bool raw_reinterpretation = false;

	if (p_view.format_override == RDC::DATA_FORMAT_MAX ||
		p_view.format_override == texture.format) {
		tv.format = texture.format;
	}
	else {
		ERR_FAIL_INDEX_V(p_view.format_override, RDC::DATA_FORMAT_MAX, RID());

		ERR_FAIL_COND_V_MSG(!texture.allowed_shared_formats.has(p_view.format_override), RID(),
			"Format override is not in the list of allowed shareable formats for original "
			"texture.");

		tv.format = p_view.format_override;
		create_shared = data->driver->texture_can_make_shared_with_format(
			texture.driver_id, p_view.format_override, raw_reinterpretation);
	}

	tv.swizzle_r = p_view.swizzle_r;
	tv.swizzle_g = p_view.swizzle_g;
	tv.swizzle_b = p_view.swizzle_b;
	tv.swizzle_a = p_view.swizzle_a;

	texture.driver_id = data->driver->texture_create_shared_from_slice(
		src_texture->driver_id, tv, p_slice_type, p_layer, texture.layers, p_mipmap, p_mipmaps);

	return data->texture_owner.make_rid(texture);
}

Error RenderingDevice::screen_prepare_for_drawing(DisplayServerEnums::WindowID p_screen)
{
	ERR_RENDER_THREAD_GUARD_V(ERR_UNAVAILABLE);

	RenderingContextDriver::SurfaceID surface = data->context->surface_get_from_window(p_screen);

	ERR_FAIL_COND_V(surface == 0, ERR_INVALID_PARAMETER);

	HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID>::ConstIterator sc_it =
		data->screen_swap_chains.find(p_screen);
	ERR_FAIL_COND_V_MSG(sc_it == data->screen_swap_chains.end(), ERR_INVALID_PARAMETER,
		"A swap chain was not created for the screen.");

	RDD::SwapChainID swap_chain = sc_it->value;

	data->screen_framebuffers.erase(p_screen);

	bool resize_required = false;
	RDD::FramebufferID framebuffer =
		data->driver->swap_chain_acquire_framebuffer(data->main_queue, swap_chain, resize_required);

	if (resize_required) {
		for (uint32_t i = 0; i < data->frames.size(); i++) {
			if (data->frames[i].fence_signaled) {
				data->driver->fence_wait(data->frames[i].fence);
				data->frames[i].fence_signaled = false;
			}
		}

		data->driver->swap_chain_resize(data->main_queue, swap_chain, 0);

		bool resize_still_required = false;
		framebuffer = data->driver->swap_chain_acquire_framebuffer(
			data->main_queue, swap_chain, resize_still_required);

		if (resize_still_required || framebuffer == RDD::FramebufferID()) {
			return ERR_BUSY;
		}
	}

	ERR_FAIL_COND_V_MSG(
		framebuffer == RDD::FramebufferID(), FAILED, "Unable to acquire framebuffer.");

	data->screen_framebuffers[p_screen] = framebuffer;

	return OK;
}

void RenderingDevice::draw_list_bind_index_array(DrawListID p_list, RID p_index_array)
{
    ERR_RENDER_THREAD_GUARD();

    ERR_FAIL_COND(!data->draw_list.active);

    if (data->draw_list.state.index_array == p_index_array) {
        return;
    }

    IndexArray* index_array = data->index_array_owner.get_or_null(p_index_array);
    ERR_FAIL_NULL(index_array);

    _check_transfer_worker_index_array(index_array);

    if (index_array->draw_tracker != nullptr) {
        data->draw_graph.add_draw_list_usage(
            index_array->draw_tracker, RDG::RESOURCE_USAGE_INDEX_BUFFER_READ);
    }

    data->draw_graph.add_draw_list_bind_index_buffer(index_array->driver_id, index_array->format,
        index_array->offset * (index_array->format == RDC::INDEX_BUFFER_FORMAT_UINT16 ? 2 : 4));

    data->draw_list.validation.index_array_count = index_array->indices;
#ifdef DEBUG_ENABLED
    data->draw_list.validation.index_array_max_index = index_array->max_index;
    data->draw_list.validation.index_buffer_uses_restart_indices =
        index_array->supports_restart_indices;
#endif

    data->draw_list.state.index_array = p_index_array;
}

void RenderingDevice::draw_list_bind_vertex_array(DrawListID p_list, RID p_vertex_array)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

	VertexArray* vertex_array = data->vertex_array_owner.get_or_null(p_vertex_array);
	ERR_FAIL_NULL(vertex_array);

	if (data->draw_list.state.vertex_array == p_vertex_array) {
		return;
	}

	_check_transfer_worker_vertex_array(vertex_array);

	data->draw_list.state.vertex_array = p_vertex_array;

#ifdef DEBUG_ENABLED
	data->draw_list.validation.vertex_format = vertex_array->description;
	data->draw_list.validation.vertex_max_instances_allowed = vertex_array->max_instances_allowed;
#endif
	data->draw_list.validation.vertex_array_size = vertex_array->vertex_count;

	data->draw_graph.add_draw_list_bind_vertex_buffers(vertex_array->buffers, vertex_array->offsets);

	for (int i = 0; i < vertex_array->draw_trackers.size(); i++) {
		data->draw_graph.add_draw_list_usage(
			vertex_array->draw_trackers[i], RDG::RESOURCE_USAGE_VERTEX_BUFFER_READ);
	}
}

void RenderingDevice::draw_list_bind_vertex_buffers_format(DrawListID p_list,
	VertexFormatID p_vertex_format, uint32_t p_vertex_count, const Span<RID>& p_vertex_buffers,
	const Span<uint64_t>& p_offsets)
{
	ERR_RENDER_THREAD_GUARD();

	ERR_FAIL_COND(!data->draw_list.active);

	const VertexDescriptionCache* vertex_description;
	{
		MutexLock<Mutex> _thread_safe_method_lock(_thread_safe_mutex);
		vertex_description = data->vertex_formats.getptr(p_vertex_format);
	}
	ERR_FAIL_NULL_MSG(vertex_description, "Supplied vertex format does not exist.");

	Span<uint64_t> offsets_span = p_offsets;
	FixedVector<uint64_t, 32> offsets;
	if (offsets_span.is_empty()) {
		offsets.resize_initialized(p_vertex_buffers.size());
		offsets_span = offsets;
	}
	else {
		ERR_FAIL_COND_MSG(offsets_span.size() != p_vertex_buffers.size(),
			"Number of vertex buffer offsets (" + itos(offsets_span.size()) +
				") does not match number of vertex buffers (" + itos(p_vertex_buffers.size()) +
				").");
	}

	FixedVector<RDD::BufferID, 32> driver_buffers;
	driver_buffers.resize_initialized(p_vertex_buffers.size());

	FixedVector<RDG::ResourceTracker*, 32> draw_trackers;

#ifdef DEBUG_ENABLED
	uint32_t max_instances_allowed = 0xFFFFFFFF;
#endif

	for (uint32_t i = 0; i < p_vertex_buffers.size(); i++) {
		RID buffer_rid = p_vertex_buffers[i];
		if (buffer_rid.is_null()) {
			continue;
		}
		ERR_FAIL_COND_MSG(!data->vertex_buffer_owner.owns(buffer_rid),
			"Vertex buffer at index " + itos(i) + " is invalid.");

		Buffer* buffer = data->vertex_buffer_owner.get_or_null(buffer_rid);
		ERR_FAIL_NULL(buffer);

		_check_transfer_worker_buffer(buffer);

#ifdef DEBUG_ENABLED
		uint64_t binding_offset = offsets_span[i];
		ERR_FAIL_COND_MSG(binding_offset > buffer->size,
			"Vertex buffer offset for attachment (" + itos(i) + ") exceeds buffer size.");

		const RDC::VertexAttribute& attribute = vertex_description->vertex_formats[i];
		uint32_t element_size = RDC::get_format_vertex_size(attribute.format);
		ERR_FAIL_COND_MSG(element_size == 0,
			"Vertex attribute format for attachment (" + itos(i) + ") is invalid.");

		uint64_t attribute_offset = binding_offset + attribute.offset;
		ERR_FAIL_COND_MSG(attribute_offset > buffer->size,
			"Vertex attribute offset for attachment (" + itos(i) + ") exceeds buffer size.");
		ERR_FAIL_COND_MSG(attribute_offset + element_size > buffer->size,
			"Vertex buffer (" + itos(i) + ") will read past the end of the buffer.");

		if (attribute.frequency == RDC::VERTEX_FREQUENCY_VERTEX) {
			ERR_FAIL_COND_MSG(p_vertex_count == 0,
				"Vertex count must be greater than 0 when binding vertex buffers.");

			uint64_t required_size = attribute_offset + element_size;
			if (p_vertex_count > 1) {
				required_size += uint64_t(attribute.stride) * (uint64_t(p_vertex_count) - 1);
			}

			ERR_FAIL_COND_MSG(required_size > buffer->size,
				"Vertex buffer (" + itos(i) + ") will read past the end of the buffer.");
		}
		else {
			uint64_t available = buffer->size - attribute_offset;
			ERR_FAIL_COND_MSG(available < element_size,
				"Vertex buffer (" + itos(i) + ") uses instancing, but it's just too small.");

			uint32_t instances_allowed =
				attribute.stride == 0 ? 0 : uint32_t(buffer->size / attribute.stride);
			max_instances_allowed = MIN(instances_allowed, max_instances_allowed);
		}
#endif

		driver_buffers[i] = buffer->driver_id;

		if (buffer->draw_tracker != nullptr) {
			draw_trackers.push_back(buffer->draw_tracker);
		}
	}

	data->draw_list.state.vertex_array = RID();

	data->draw_graph.add_draw_list_bind_vertex_buffers(driver_buffers, offsets_span);

	for (RDG::ResourceTracker* tracker : draw_trackers) {
		data->draw_graph.add_draw_list_usage(tracker, RDG::RESOURCE_USAGE_VERTEX_BUFFER_READ);
	}

	data->draw_list.validation.vertex_array_size = p_vertex_count;

#ifdef DEBUG_ENABLED
	data->draw_list.validation.vertex_format = p_vertex_format;
	data->draw_list.validation.vertex_max_instances_allowed = max_instances_allowed;
#endif
}

RID RenderingDevice::shader_create_from_bytecode(
	const Vector<uint8_t>& p_shader_binary, RID p_placeholder)
{
	ERR_RENDER_THREAD_GUARD_V(RID());

	Ref<RenderingShaderContainer> container =
		data->driver->get_shader_container_format().create_container();
	bool ok = container->from_bytes(p_shader_binary);
	ERR_FAIL_COND_V_MSG(!ok, RID(), "Failed to parse shader container binary.");

	Vector<RDD::ImmutableSampler> immutable_samplers;
	RDD::ShaderID driver_id =
		data->driver->shader_create_from_container(container, immutable_samplers);
	ERR_FAIL_COND_V(!driver_id, RID());

	MutexLock<Mutex> _thread_safe_method_lock(RenderingDevice::_thread_safe_mutex);

	Shader shader;
	static_cast<RDC::ShaderReflection&>(shader) = container->get_shader_reflection();
	shader.name.clear();
	shader.name.append_utf8(container->shader_name);
	shader.driver_id = driver_id;
	shader.layout_hash = data->driver->shader_get_layout_hash(driver_id);
	shader.stage_bits = 0;

	shader.set_formats.resize(shader.uniform_sets.size());
	for (int i = 0; i < shader.uniform_sets.size(); i++) {
		uint32_t format_id = 0;
		if (shader.uniform_sets[i].size()) {
			shader.uniform_sets.write[i].sort();

			UniformSetFormat usf;
			usf.uniforms = shader.uniform_sets[i];
			if (data->uniform_set_format_cache.has(usf)) {
				format_id = data->uniform_set_format_cache[usf];
			}
			else {
				format_id = data->uniform_set_format_cache.size() + 1;
				data->uniform_set_format_cache[usf] = format_id;
			}
		}
		shader.set_formats.write[i] = format_id;
	}

	for (RDC::ShaderStage stage : shader.stages_vector) {
		switch (stage) {
		case RDC::SHADER_STAGE_VERTEX:
			shader.stage_bits |= RDD::PIPELINE_STAGE_VERTEX_SHADER_BIT;
			break;
		case RDC::SHADER_STAGE_FRAGMENT:
			shader.stage_bits |= RDD::PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case RDC::SHADER_STAGE_TESSELATION_CONTROL:
			shader.stage_bits |= RDD::PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
			break;
		case RDC::SHADER_STAGE_TESSELATION_EVALUATION:
			shader.stage_bits |= RDD::PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
			break;
		case RDC::SHADER_STAGE_COMPUTE:
			shader.stage_bits |= RDD::PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			break;
		case RDC::SHADER_STAGE_RAYGEN:
		case RDC::SHADER_STAGE_ANY_HIT:
		case RDC::SHADER_STAGE_CLOSEST_HIT:
		case RDC::SHADER_STAGE_MISS:
		case RDC::SHADER_STAGE_INTERSECTION:
			shader.stage_bits |= RDD::PIPELINE_STAGE_RAY_TRACING_SHADER_BIT;
			break;
		default:
			DEV_ASSERT(false && "Unknown shader stage.");
			break;
		}
	}

	if (p_placeholder.is_valid()) {
		Shader* existing = data->shader_owner.get_or_null(p_placeholder);
		ERR_FAIL_NULL_V(existing, RID());
		*existing = shader;
		return p_placeholder;
	}

	return data->shader_owner.make_rid(shader);
}

RenderingDevice::DrawListID RenderingDevice::draw_list_begin(RID p_framebuffer,
	uint32_t p_draw_flags, const Vector<Color>& p_clear_color_values, float p_clear_depth_value,
	uint32_t p_clear_stencil_value, const Rect2& p_region, uint32_t p_breadcrumb)
{
	ERR_RENDER_THREAD_GUARD_V(RDC::INVALID_ID);

	ERR_FAIL_COND_V_MSG(data->draw_list.active, RDC::INVALID_ID,
		"Only one draw list can be active at the same time.");

	Framebuffer* framebuffer = data->framebuffer_owner.get_or_null(p_framebuffer);
	ERR_FAIL_NULL_V(framebuffer, RDC::INVALID_ID);

	const FramebufferFormatKey* framebuffer_key;
	framebuffer_key = &data->framebuffer_formats[framebuffer->format_id].E->key();

	Point2i viewport_offset;
	Point2i viewport_size = framebuffer->size;

	if (p_region != Rect2() &&
		p_region != Rect2(Vector2(), viewport_size)) { // Check custom region.
		Rect2i viewport(viewport_offset, viewport_size);
		Rect2i regioni = p_region;
		if (!((regioni.position.x >= viewport.position.x) &&
				(regioni.position.y >= viewport.position.y) &&
				((regioni.position.x + regioni.size.x) <=
					(viewport.position.x + viewport.size.x)) &&
				((regioni.position.y + regioni.size.y) <=
					(viewport.position.y + viewport.size.y)))) {
			ERR_FAIL_V_MSG(RDC::INVALID_ID, "When supplying a custom region, it must be contained "
											"within the framebuffer rectangle");
		}

		viewport_offset = regioni.position;
		viewport_size = regioni.size;
	}

	thread_local LocalVector<RDG::AttachmentOperation> operations;
	thread_local LocalVector<RDD::RenderPassClearValue> clear_values;
	thread_local LocalVector<RDG::ResourceTracker*> resource_trackers;
	thread_local LocalVector<RDG::ResourceUsage> resource_usages;
	uint32_t stages = 0;
	operations.resize(framebuffer->texture_ids.size());
	clear_values.resize(framebuffer->texture_ids.size());
	resource_trackers.clear();
	resource_usages.clear();

	uint32_t color_index = 0;
	for (int i = 0; i < framebuffer->texture_ids.size(); i++) {
		RID texture_rid = framebuffer->texture_ids[i];
		Texture* texture = data->texture_owner.get_or_null(texture_rid);
		if (texture == nullptr) {
			operations[i] = RDG::ATTACHMENT_OPERATION_DEFAULT;
			clear_values[i] = RDD::RenderPassClearValue();
			continue;
		}

		// Clear the texture if the driver requires it during its first use.
		_texture_check_pending_clear(texture_rid, texture);

		// Indicate the texture will get modified for the shared texture fallback.
		_texture_update_shared_fallback(texture_rid, texture, true);

		RDG::AttachmentOperation operation = RDG::ATTACHMENT_OPERATION_DEFAULT;
		RDD::RenderPassClearValue clear_value;
		if (framebuffer_key->vrs_attachment == i &&
			(texture->usage_flags & RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT)) {
			resource_trackers.push_back(texture->draw_tracker);
			resource_usages.push_back(_vrs_usage_from_method(framebuffer_key->vrs_method));
			stages |= _vrs_stages_from_method(framebuffer_key->vrs_method);
		}
		else if (texture->usage_flags & RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT) {
			if (p_draw_flags & (DRAW_CLEAR_COLOR_0 << color_index)) {
				ERR_FAIL_COND_V_MSG((int)color_index >= p_clear_color_values.size(),
					RDC::INVALID_ID,
					vformat("Color texture (%d) was specified to be cleared but no color value was "
							"provided.",
						color_index));
				operation = RDG::ATTACHMENT_OPERATION_CLEAR;
				clear_value.color = p_clear_color_values[color_index];
			}
			else if (p_draw_flags & (DRAW_IGNORE_COLOR_0 << color_index)) {
				operation = RDG::ATTACHMENT_OPERATION_IGNORE;
			}

			resource_trackers.push_back(texture->draw_tracker);
			resource_usages.push_back(RDG::RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE);
			stages |= RDD::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			color_index++;
		}
		else if (texture->usage_flags & (RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
											  RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT)) {
			if ((p_draw_flags & DRAW_CLEAR_DEPTH) || (p_draw_flags & DRAW_CLEAR_STENCIL)) {
				operation = RDG::ATTACHMENT_OPERATION_CLEAR;
				clear_value.depth = p_clear_depth_value;
				clear_value.stencil = p_clear_stencil_value;
			}
			else if ((p_draw_flags & DRAW_IGNORE_DEPTH) || (p_draw_flags & DRAW_IGNORE_STENCIL)) {
				operation = RDG::ATTACHMENT_OPERATION_IGNORE;
			}

			resource_trackers.push_back(texture->draw_tracker);
			resource_usages.push_back(RDG::RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE);
			stages |= RDD::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			stages |= RDD::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}

		operations[i] = operation;
		clear_values[i] = clear_value;
	}

	data->draw_graph.add_draw_list_begin(framebuffer->framebuffer_cache,
		Rect2i(viewport_offset, viewport_size), operations, clear_values, stages, p_breadcrumb);
	data->draw_graph.add_draw_list_usages(resource_trackers, resource_usages);

	// Mark textures as bound.
	data->draw_list_bound_textures.clear();

	for (int i = 0; i < framebuffer->texture_ids.size(); i++) {
		Texture* texture = data->texture_owner.get_or_null(framebuffer->texture_ids[i]);
		if (texture == nullptr) {
			continue;
		}

		texture->bound = true;
		data->draw_list_bound_textures.push_back(framebuffer->texture_ids[i]);
	}

	_draw_list_start(Rect2i(viewport_offset, viewport_size));
#ifdef DEBUG_ENABLED
	data->draw_list_framebuffer_format = framebuffer->format_id;
#endif
	data->draw_list_current_subpass = 0;
	data->draw_list_subpass_count = framebuffer_key->passes.size();

	Rect2i viewport_rect(viewport_offset, viewport_size);
	data->draw_graph.add_draw_list_set_viewport(viewport_rect);
	data->draw_graph.add_draw_list_set_scissor(viewport_rect);

	return int64_t(ID_TYPE_DRAW_LIST) << ID_BASE_SHIFT;
}

void RenderingDevice::_texture_check_pending_clear(RID p_texture_rid, Texture* p_texture)
{
	DEV_ASSERT(p_texture != nullptr);

	if (!p_texture->pending_clear) {
		return;
	}

	bool clear = true;
	p_texture->pending_clear = false;

	if (p_texture->owner.is_valid()) {
		// Check the owner texture instead if it exists.
		p_texture_rid = p_texture->owner;
		p_texture = data->texture_owner.get_or_null(p_texture_rid);
		clear = p_texture->pending_clear;
	}

	if (p_texture != nullptr && clear) {
		if (p_texture->usage_flags & RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			_texture_clear_depth_stencil(
				p_texture_rid, p_texture, 0.0f, 0, 0, p_texture->mipmaps, 0, p_texture->layers);
		}
		else {
			_texture_clear_color(
				p_texture_rid, p_texture, Color(), 0, p_texture->mipmaps, 0, p_texture->layers);
		}
		p_texture->pending_clear = false;
	}
}

void RenderingDevice::_texture_clear_depth_stencil(RID p_texture_rid, Texture* p_texture,
	float p_depth, uint8_t p_stencil, uint32_t p_base_mipmap, uint32_t p_mipmaps,
	uint32_t p_base_layer, uint32_t p_layers)
{
	_check_transfer_worker_texture(p_texture);

	RDD::TextureSubresourceRange range;

	if (RDC::format_has_depth(p_texture->format)) {
		range.aspect = Utils::set_flag(range.aspect, RDD::TEXTURE_ASPECT_DEPTH_BIT);
	}
	if (RDC::format_has_stencil(p_texture->format)) {
		range.aspect = Utils::set_flag(range.aspect, RDD::TEXTURE_ASPECT_STENCIL_BIT);
	}
	range.base_mipmap = p_texture->base_mipmap + p_base_mipmap;
	range.mipmap_count = p_mipmaps;
	range.base_layer = p_texture->base_layer + p_base_layer;
	range.layer_count = p_layers;

	// Indicate the texture will get modified for the shared texture fallback.
	_texture_update_shared_fallback(p_texture_rid, p_texture, true);

	if (_texture_make_mutable(p_texture, p_texture_rid)) {
		// The texture must be mutable to be used as a clear destination.
		data->draw_graph.add_synchronization();
	}

	data->draw_graph.add_texture_clear_depth_stencil(
		p_texture->driver_id, p_texture->draw_tracker, p_depth, p_stencil, range);
}

// ============================================================================
// Merged from servers/rendering/rendering_device_commons.cpp
// ============================================================================

#include "servers/rendering/rendering_device_commons.h"
#include "modules/modules_enabled.gen.h" // For glslang.
#include "servers/rendering/rendering_device_binds.h"
#ifdef MODULE_GLSLANG_ENABLED
#include "modules/glslang/shader_compile.h"
#endif

#include "servers/rendering/shader_include_db.h"
#include "servers/rendering/rendering_device_driver.h"
#include "servers/rendering/rendering_context_driver.h"
#include <thirdparty/spirv-reflect/spirv_reflect.h>
#include "core/io/compression.h"
#include "servers/rendering/rendering_shader_container.h"
#include "servers/rendering/renderer_rd/shader_rd.h"

// --- Merged from servers/rendering/rendering_device_commons.cpp ---

/*****************/
/**** GENERIC ****/
/*****************/

const char* const RenderingDeviceCommons::FORMAT_NAMES[DATA_FORMAT_MAX] = {
	"R4G4_Unorm_Pack8",
	"R4G4B4A4_Unorm_Pack16",
	"B4G4R4A4_Unorm_Pack16",
	"R5G6B5_Unorm_Pack16",
	"B5G6R5_Unorm_Pack16",
	"R5G5B5A1_Unorm_Pack16",
	"B5G5R5A1_Unorm_Pack16",
	"A1R5G5B5_Unorm_Pack16",
	"R8_Unorm",
	"R8_Snorm",
	"R8_Uscaled",
	"R8_Sscaled",
	"R8_Uint",
	"R8_Sint",
	"R8_Srgb",
	"R8G8_Unorm",
	"R8G8_Snorm",
	"R8G8_Uscaled",
	"R8G8_Sscaled",
	"R8G8_Uint",
	"R8G8_Sint",
	"R8G8_Srgb",
	"R8G8B8_Unorm",
	"R8G8B8_Snorm",
	"R8G8B8_Uscaled",
	"R8G8B8_Sscaled",
	"R8G8B8_Uint",
	"R8G8B8_Sint",
	"R8G8B8_Srgb",
	"B8G8R8_Unorm",
	"B8G8R8_Snorm",
	"B8G8R8_Uscaled",
	"B8G8R8_Sscaled",
	"B8G8R8_Uint",
	"B8G8R8_Sint",
	"B8G8R8_Srgb",
	"R8G8B8A8_Unorm",
	"R8G8B8A8_Snorm",
	"R8G8B8A8_Uscaled",
	"R8G8B8A8_Sscaled",
	"R8G8B8A8_Uint",
	"R8G8B8A8_Sint",
	"R8G8B8A8_Srgb",
	"B8G8R8A8_Unorm",
	"B8G8R8A8_Snorm",
	"B8G8R8A8_Uscaled",
	"B8G8R8A8_Sscaled",
	"B8G8R8A8_Uint",
	"B8G8R8A8_Sint",
	"B8G8R8A8_Srgb",
	"A8B8G8R8_Unorm_Pack32",
	"A8B8G8R8_Snorm_Pack32",
	"A8B8G8R8_Uscaled_Pack32",
	"A8B8G8R8_Sscaled_Pack32",
	"A8B8G8R8_Uint_Pack32",
	"A8B8G8R8_Sint_Pack32",
	"A8B8G8R8_Srgb_Pack32",
	"A2R10G10B10_Unorm_Pack32",
	"A2R10G10B10_Snorm_Pack32",
	"A2R10G10B10_Uscaled_Pack32",
	"A2R10G10B10_Sscaled_Pack32",
	"A2R10G10B10_Uint_Pack32",
	"A2R10G10B10_Sint_Pack32",
	"A2B10G10R10_Unorm_Pack32",
	"A2B10G10R10_Snorm_Pack32",
	"A2B10G10R10_Uscaled_Pack32",
	"A2B10G10R10_Sscaled_Pack32",
	"A2B10G10R10_Uint_Pack32",
	"A2B10G10R10_Sint_Pack32",
	"R16_Unorm",
	"R16_Snorm",
	"R16_Uscaled",
	"R16_Sscaled",
	"R16_Uint",
	"R16_Sint",
	"R16_Sfloat",
	"R16G16_Unorm",
	"R16G16_Snorm",
	"R16G16_Uscaled",
	"R16G16_Sscaled",
	"R16G16_Uint",
	"R16G16_Sint",
	"R16G16_Sfloat",
	"R16G16B16_Unorm",
	"R16G16B16_Snorm",
	"R16G16B16_Uscaled",
	"R16G16B16_Sscaled",
	"R16G16B16_Uint",
	"R16G16B16_Sint",
	"R16G16B16_Sfloat",
	"R16G16B16A16_Unorm",
	"R16G16B16A16_Snorm",
	"R16G16B16A16_Uscaled",
	"R16G16B16A16_Sscaled",
	"R16G16B16A16_Uint",
	"R16G16B16A16_Sint",
	"R16G16B16A16_Sfloat",
	"R32_Uint",
	"R32_Sint",
	"R32_Sfloat",
	"R32G32_Uint",
	"R32G32_Sint",
	"R32G32_Sfloat",
	"R32G32B32_Uint",
	"R32G32B32_Sint",
	"R32G32B32_Sfloat",
	"R32G32B32A32_Uint",
	"R32G32B32A32_Sint",
	"R32G32B32A32_Sfloat",
	"R64_Uint",
	"R64_Sint",
	"R64_Sfloat",
	"R64G64_Uint",
	"R64G64_Sint",
	"R64G64_Sfloat",
	"R64G64B64_Uint",
	"R64G64B64_Sint",
	"R64G64B64_Sfloat",
	"R64G64B64A64_Uint",
	"R64G64B64A64_Sint",
	"R64G64B64A64_Sfloat",
	"B10G11R11_Ufloat_Pack32",
	"E5B9G9R9_Ufloat_Pack32",
	"D16_Unorm",
	"X8_D24_Unorm_Pack32",
	"D32_Sfloat",
	"S8_Uint",
	"D16_Unorm_S8_Uint",
	"D24_Unorm_S8_Uint",
	"D32_Sfloat_S8_Uint",
	"Bc1_Rgb_Unorm_Block",
	"Bc1_Rgb_Srgb_Block",
	"Bc1_Rgba_Unorm_Block",
	"Bc1_Rgba_Srgb_Block",
	"Bc2_Unorm_Block",
	"Bc2_Srgb_Block",
	"Bc3_Unorm_Block",
	"Bc3_Srgb_Block",
	"Bc4_Unorm_Block",
	"Bc4_Snorm_Block",
	"Bc5_Unorm_Block",
	"Bc5_Snorm_Block",
	"Bc6H_Ufloat_Block",
	"Bc6H_Sfloat_Block",
	"Bc7_Unorm_Block",
	"Bc7_Srgb_Block",
	"Etc2_R8G8B8_Unorm_Block",
	"Etc2_R8G8B8_Srgb_Block",
	"Etc2_R8G8B8A1_Unorm_Block",
	"Etc2_R8G8B8A1_Srgb_Block",
	"Etc2_R8G8B8A8_Unorm_Block",
	"Etc2_R8G8B8A8_Srgb_Block",
	"Eac_R11_Unorm_Block",
	"Eac_R11_Snorm_Block",
	"Eac_R11G11_Unorm_Block",
	"Eac_R11G11_Snorm_Block",
	"Astc_4X4_Unorm_Block",
	"Astc_4X4_Srgb_Block",
	"Astc_5X4_Unorm_Block",
	"Astc_5X4_Srgb_Block",
	"Astc_5X5_Unorm_Block",
	"Astc_5X5_Srgb_Block",
	"Astc_6X5_Unorm_Block",
	"Astc_6X5_Srgb_Block",
	"Astc_6X6_Unorm_Block",
	"Astc_6X6_Srgb_Block",
	"Astc_8X5_Unorm_Block",
	"Astc_8X5_Srgb_Block",
	"Astc_8X6_Unorm_Block",
	"Astc_8X6_Srgb_Block",
	"Astc_8X8_Unorm_Block",
	"Astc_8X8_Srgb_Block",
	"Astc_10X5_Unorm_Block",
	"Astc_10X5_Srgb_Block",
	"Astc_10X6_Unorm_Block",
	"Astc_10X6_Srgb_Block",
	"Astc_10X8_Unorm_Block",
	"Astc_10X8_Srgb_Block",
	"Astc_10X10_Unorm_Block",
	"Astc_10X10_Srgb_Block",
	"Astc_12X10_Unorm_Block",
	"Astc_12X10_Srgb_Block",
	"Astc_12X12_Unorm_Block",
	"Astc_12X12_Srgb_Block",
	"G8B8G8R8_422_Unorm",
	"B8G8R8G8_422_Unorm",
	"G8_B8_R8_3Plane_420_Unorm",
	"G8_B8R8_2Plane_420_Unorm",
	"G8_B8_R8_3Plane_422_Unorm",
	"G8_B8R8_2Plane_422_Unorm",
	"G8_B8_R8_3Plane_444_Unorm",
	"R10X6_Unorm_Pack16",
	"R10X6G10X6_Unorm_2Pack16",
	"R10X6G10X6B10X6A10X6_Unorm_4Pack16",
	"G10X6B10X6G10X6R10X6_422_Unorm_4Pack16",
	"B10X6G10X6R10X6G10X6_422_Unorm_4Pack16",
	"G10X6_B10X6_R10X6_3Plane_420_Unorm_3Pack16",
	"G10X6_B10X6R10X6_2Plane_420_Unorm_3Pack16",
	"G10X6_B10X6_R10X6_3Plane_422_Unorm_3Pack16",
	"G10X6_B10X6R10X6_2Plane_422_Unorm_3Pack16",
	"G10X6_B10X6_R10X6_3Plane_444_Unorm_3Pack16",
	"R12X4_Unorm_Pack16",
	"R12X4G12X4_Unorm_2Pack16",
	"R12X4G12X4B12X4A12X4_Unorm_4Pack16",
	"G12X4B12X4G12X4R12X4_422_Unorm_4Pack16",
	"B12X4G12X4R12X4G12X4_422_Unorm_4Pack16",
	"G12X4_B12X4_R12X4_3Plane_420_Unorm_3Pack16",
	"G12X4_B12X4R12X4_2Plane_420_Unorm_3Pack16",
	"G12X4_B12X4_R12X4_3Plane_422_Unorm_3Pack16",
	"G12X4_B12X4R12X4_2Plane_422_Unorm_3Pack16",
	"G12X4_B12X4_R12X4_3Plane_444_Unorm_3Pack16",
	"G16B16G16R16_422_Unorm",
	"B16G16R16G16_422_Unorm",
	"G16_B16_R16_3Plane_420_Unorm",
	"G16_B16R16_2Plane_420_Unorm",
	"G16_B16_R16_3Plane_422_Unorm",
	"G16_B16R16_2Plane_422_Unorm",
	"G16_B16_R16_3Plane_444_Unorm",
	"Astc_4X4_Sfloat_Block",
	"Astc_5X4_Sfloat_Block",
	"Astc_5X5_Sfloat_Block",
	"Astc_6X5_Sfloat_Block",
	"Astc_6X6_Sfloat_Block",
	"Astc_8X5_Sfloat_Block",
	"Astc_8X6_Sfloat_Block",
	"Astc_8X8_Sfloat_Block",
	"Astc_10X5_Sfloat_Block",
	"Astc_10X6_Sfloat_Block",
	"Astc_10X8_Sfloat_Block",
	"Astc_10X10_Sfloat_Block",
	"Astc_12X10_Sfloat_Block",
	"Astc_12X12_Sfloat_Block",
};

/*****************/
/**** TEXTURE ****/
/*****************/

const char* const RenderingDeviceCommons::TEXTURE_TYPE_NAMES[TEXTURE_TYPE_MAX] = {
	"1D",
	"2D",
	"3D",
	"Cube",
	"1DArray",
	"2DArray",
	"CubeArray",
};

const uint32_t RenderingDeviceCommons::TEXTURE_SAMPLES_COUNT[TEXTURE_SAMPLES_MAX] = {
	1, 2, 4, 8, 16, 32, 64};

uint32_t RenderingDeviceCommons::get_image_format_pixel_size(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_R4G4_UNORM_PACK8:
		return 1;
	case DATA_FORMAT_R4G4B4A4_UNORM_PACK16:
	case DATA_FORMAT_B4G4R4A4_UNORM_PACK16:
	case DATA_FORMAT_R5G6B5_UNORM_PACK16:
	case DATA_FORMAT_B5G6R5_UNORM_PACK16:
	case DATA_FORMAT_R5G5B5A1_UNORM_PACK16:
	case DATA_FORMAT_B5G5R5A1_UNORM_PACK16:
	case DATA_FORMAT_A1R5G5B5_UNORM_PACK16:
		return 2;
	case DATA_FORMAT_R8_UNORM:
	case DATA_FORMAT_R8_SNORM:
	case DATA_FORMAT_R8_USCALED:
	case DATA_FORMAT_R8_SSCALED:
	case DATA_FORMAT_R8_UINT:
	case DATA_FORMAT_R8_SINT:
	case DATA_FORMAT_R8_SRGB:
		return 1;
	case DATA_FORMAT_R8G8_UNORM:
	case DATA_FORMAT_R8G8_SNORM:
	case DATA_FORMAT_R8G8_USCALED:
	case DATA_FORMAT_R8G8_SSCALED:
	case DATA_FORMAT_R8G8_UINT:
	case DATA_FORMAT_R8G8_SINT:
	case DATA_FORMAT_R8G8_SRGB:
		return 2;
	case DATA_FORMAT_R8G8B8_UNORM:
	case DATA_FORMAT_R8G8B8_SNORM:
	case DATA_FORMAT_R8G8B8_USCALED:
	case DATA_FORMAT_R8G8B8_SSCALED:
	case DATA_FORMAT_R8G8B8_UINT:
	case DATA_FORMAT_R8G8B8_SINT:
	case DATA_FORMAT_R8G8B8_SRGB:
	case DATA_FORMAT_B8G8R8_UNORM:
	case DATA_FORMAT_B8G8R8_SNORM:
	case DATA_FORMAT_B8G8R8_USCALED:
	case DATA_FORMAT_B8G8R8_SSCALED:
	case DATA_FORMAT_B8G8R8_UINT:
	case DATA_FORMAT_B8G8R8_SINT:
	case DATA_FORMAT_B8G8R8_SRGB:
		return 3;
	case DATA_FORMAT_R8G8B8A8_UNORM:
	case DATA_FORMAT_R8G8B8A8_SNORM:
	case DATA_FORMAT_R8G8B8A8_USCALED:
	case DATA_FORMAT_R8G8B8A8_SSCALED:
	case DATA_FORMAT_R8G8B8A8_UINT:
	case DATA_FORMAT_R8G8B8A8_SINT:
	case DATA_FORMAT_R8G8B8A8_SRGB:
	case DATA_FORMAT_B8G8R8A8_UNORM:
	case DATA_FORMAT_B8G8R8A8_SNORM:
	case DATA_FORMAT_B8G8R8A8_USCALED:
	case DATA_FORMAT_B8G8R8A8_SSCALED:
	case DATA_FORMAT_B8G8R8A8_UINT:
	case DATA_FORMAT_B8G8R8A8_SINT:
	case DATA_FORMAT_B8G8R8A8_SRGB:
		return 4;
	case DATA_FORMAT_A8B8G8R8_UNORM_PACK32:
	case DATA_FORMAT_A8B8G8R8_SNORM_PACK32:
	case DATA_FORMAT_A8B8G8R8_USCALED_PACK32:
	case DATA_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case DATA_FORMAT_A8B8G8R8_UINT_PACK32:
	case DATA_FORMAT_A8B8G8R8_SINT_PACK32:
	case DATA_FORMAT_A8B8G8R8_SRGB_PACK32:
	case DATA_FORMAT_A2R10G10B10_UNORM_PACK32:
	case DATA_FORMAT_A2R10G10B10_SNORM_PACK32:
	case DATA_FORMAT_A2R10G10B10_USCALED_PACK32:
	case DATA_FORMAT_A2R10G10B10_SSCALED_PACK32:
	case DATA_FORMAT_A2R10G10B10_UINT_PACK32:
	case DATA_FORMAT_A2R10G10B10_SINT_PACK32:
	case DATA_FORMAT_A2B10G10R10_UNORM_PACK32:
	case DATA_FORMAT_A2B10G10R10_SNORM_PACK32:
	case DATA_FORMAT_A2B10G10R10_USCALED_PACK32:
	case DATA_FORMAT_A2B10G10R10_SSCALED_PACK32:
	case DATA_FORMAT_A2B10G10R10_UINT_PACK32:
	case DATA_FORMAT_A2B10G10R10_SINT_PACK32:
		return 4;
	case DATA_FORMAT_R16_UNORM:
	case DATA_FORMAT_R16_SNORM:
	case DATA_FORMAT_R16_USCALED:
	case DATA_FORMAT_R16_SSCALED:
	case DATA_FORMAT_R16_UINT:
	case DATA_FORMAT_R16_SINT:
	case DATA_FORMAT_R16_SFLOAT:
		return 2;
	case DATA_FORMAT_R16G16_UNORM:
	case DATA_FORMAT_R16G16_SNORM:
	case DATA_FORMAT_R16G16_USCALED:
	case DATA_FORMAT_R16G16_SSCALED:
	case DATA_FORMAT_R16G16_UINT:
	case DATA_FORMAT_R16G16_SINT:
	case DATA_FORMAT_R16G16_SFLOAT:
		return 4;
	case DATA_FORMAT_R16G16B16_UNORM:
	case DATA_FORMAT_R16G16B16_SNORM:
	case DATA_FORMAT_R16G16B16_USCALED:
	case DATA_FORMAT_R16G16B16_SSCALED:
	case DATA_FORMAT_R16G16B16_UINT:
	case DATA_FORMAT_R16G16B16_SINT:
	case DATA_FORMAT_R16G16B16_SFLOAT:
		return 6;
	case DATA_FORMAT_R16G16B16A16_UNORM:
	case DATA_FORMAT_R16G16B16A16_SNORM:
	case DATA_FORMAT_R16G16B16A16_USCALED:
	case DATA_FORMAT_R16G16B16A16_SSCALED:
	case DATA_FORMAT_R16G16B16A16_UINT:
	case DATA_FORMAT_R16G16B16A16_SINT:
	case DATA_FORMAT_R16G16B16A16_SFLOAT:
		return 8;
	case DATA_FORMAT_R32_UINT:
	case DATA_FORMAT_R32_SINT:
	case DATA_FORMAT_R32_SFLOAT:
		return 4;
	case DATA_FORMAT_R32G32_UINT:
	case DATA_FORMAT_R32G32_SINT:
	case DATA_FORMAT_R32G32_SFLOAT:
		return 8;
	case DATA_FORMAT_R32G32B32_UINT:
	case DATA_FORMAT_R32G32B32_SINT:
	case DATA_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case DATA_FORMAT_R32G32B32A32_UINT:
	case DATA_FORMAT_R32G32B32A32_SINT:
	case DATA_FORMAT_R32G32B32A32_SFLOAT:
		return 16;
	case DATA_FORMAT_R64_UINT:
	case DATA_FORMAT_R64_SINT:
	case DATA_FORMAT_R64_SFLOAT:
		return 8;
	case DATA_FORMAT_R64G64_UINT:
	case DATA_FORMAT_R64G64_SINT:
	case DATA_FORMAT_R64G64_SFLOAT:
		return 16;
	case DATA_FORMAT_R64G64B64_UINT:
	case DATA_FORMAT_R64G64B64_SINT:
	case DATA_FORMAT_R64G64B64_SFLOAT:
		return 24;
	case DATA_FORMAT_R64G64B64A64_UINT:
	case DATA_FORMAT_R64G64B64A64_SINT:
	case DATA_FORMAT_R64G64B64A64_SFLOAT:
		return 32;
	case DATA_FORMAT_B10G11R11_UFLOAT_PACK32:
	case DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		return 4;
	case DATA_FORMAT_D16_UNORM:
		return 2;
	case DATA_FORMAT_X8_D24_UNORM_PACK32:
		return 4;
	case DATA_FORMAT_D32_SFLOAT:
		return 4;
	case DATA_FORMAT_S8_UINT:
		return 1;
	case DATA_FORMAT_D16_UNORM_S8_UINT:
		return 4;
	case DATA_FORMAT_D24_UNORM_S8_UINT:
		return 4;
	case DATA_FORMAT_D32_SFLOAT_S8_UINT:
		return 5; // ?
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case DATA_FORMAT_BC2_UNORM_BLOCK:
	case DATA_FORMAT_BC2_SRGB_BLOCK:
	case DATA_FORMAT_BC3_UNORM_BLOCK:
	case DATA_FORMAT_BC3_SRGB_BLOCK:
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
	case DATA_FORMAT_BC5_UNORM_BLOCK:
	case DATA_FORMAT_BC5_SNORM_BLOCK:
	case DATA_FORMAT_BC6H_UFLOAT_BLOCK:
	case DATA_FORMAT_BC6H_SFLOAT_BLOCK:
	case DATA_FORMAT_BC7_UNORM_BLOCK:
	case DATA_FORMAT_BC7_SRGB_BLOCK:
		return 1;
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		return 1;
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_SNORM_BLOCK:
		return 1;
	case DATA_FORMAT_ASTC_4x4_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
		return 1;
	case DATA_FORMAT_G8B8G8R8_422_UNORM:
	case DATA_FORMAT_B8G8R8G8_422_UNORM:
		return 4;
	case DATA_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
	case DATA_FORMAT_G8_B8R8_2PLANE_420_UNORM:
	case DATA_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
	case DATA_FORMAT_G8_B8R8_2PLANE_422_UNORM:
	case DATA_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
		return 4;
	case DATA_FORMAT_R10X6_UNORM_PACK16:
	case DATA_FORMAT_R10X6G10X6_UNORM_2PACK16:
	case DATA_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
	case DATA_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
	case DATA_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
	case DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
	case DATA_FORMAT_R12X4_UNORM_PACK16:
	case DATA_FORMAT_R12X4G12X4_UNORM_2PACK16:
	case DATA_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
	case DATA_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
	case DATA_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
	case DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
		return 2;
	case DATA_FORMAT_G16B16G16R16_422_UNORM:
	case DATA_FORMAT_B16G16R16G16_422_UNORM:
	case DATA_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
	case DATA_FORMAT_G16_B16R16_2PLANE_420_UNORM:
	case DATA_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
	case DATA_FORMAT_G16_B16R16_2PLANE_422_UNORM:
	case DATA_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
		return 8;
	default: {
		ERR_PRINT("Format not handled, bug");
	}
	}

	return 1;
}

// https://www.khronos.org/registry/DataFormat/specs/1.1/dataformat.1.1.pdf
void RenderingDeviceCommons::get_compressed_image_format_block_dimensions(
	DataFormat p_format, uint32_t& r_w, uint32_t& r_h)
{
	switch (p_format) {
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case DATA_FORMAT_BC2_UNORM_BLOCK:
	case DATA_FORMAT_BC2_SRGB_BLOCK:
	case DATA_FORMAT_BC3_UNORM_BLOCK:
	case DATA_FORMAT_BC3_SRGB_BLOCK:
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
	case DATA_FORMAT_BC5_UNORM_BLOCK:
	case DATA_FORMAT_BC5_SNORM_BLOCK:
	case DATA_FORMAT_BC6H_UFLOAT_BLOCK:
	case DATA_FORMAT_BC6H_SFLOAT_BLOCK:
	case DATA_FORMAT_BC7_UNORM_BLOCK:
	case DATA_FORMAT_BC7_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_SNORM_BLOCK:
	case DATA_FORMAT_ASTC_4x4_UNORM_BLOCK: // Again, not sure about astc.
	case DATA_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK: {
		r_w = 4;
		r_h = 4;
	} break;
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK: {
		r_w = 6;
		r_h = 6;
	} break;
	case DATA_FORMAT_ASTC_5x4_UNORM_BLOCK: // Unsupported
	case DATA_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SFLOAT_BLOCK: {
		r_w = 4;
		r_h = 4;
	} break;
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK: {
		r_w = 8;
		r_h = 8;
	} break;
	case DATA_FORMAT_ASTC_10x5_UNORM_BLOCK: // Unsupported
	case DATA_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
		r_w = 4;
		r_h = 4;
		return;
	default: {
		r_w = 1;
		r_h = 1;
	}
	}
}

uint32_t RenderingDeviceCommons::get_compressed_image_format_block_byte_size(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
		return 8;
	case DATA_FORMAT_BC2_UNORM_BLOCK:
	case DATA_FORMAT_BC2_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_BC3_UNORM_BLOCK:
	case DATA_FORMAT_BC3_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
		return 8;
	case DATA_FORMAT_BC5_UNORM_BLOCK:
	case DATA_FORMAT_BC5_SNORM_BLOCK:
		return 16;
	case DATA_FORMAT_BC6H_UFLOAT_BLOCK:
	case DATA_FORMAT_BC6H_SFLOAT_BLOCK:
		return 16;
	case DATA_FORMAT_BC7_UNORM_BLOCK:
	case DATA_FORMAT_BC7_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		return 8;
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		return 8;
	case DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
		return 8;
	case DATA_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_SNORM_BLOCK:
		return 16;
	case DATA_FORMAT_ASTC_4x4_UNORM_BLOCK: // Again, not sure about astc.
	case DATA_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
		return 16;
	default: {
	}
	}
	return 1;
}

uint32_t RenderingDeviceCommons::get_compressed_image_format_pixels_shifted(
	DataFormat p_format, uint32_t p_pixels)
{
	switch (p_format) {
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK: // These formats are half byte size, so rshift is 1.
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
		return (p_pixels >> 1);
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
		return (p_pixels * 4) / 9;
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
		return (p_pixels >> 2);
	default:
		return p_pixels;
	}
}

uint32_t RenderingDeviceCommons::get_image_format_required_size(DataFormat p_format,
	uint32_t p_width, uint32_t p_height, uint32_t p_depth, uint32_t p_mipmaps, uint32_t* r_blockw,
	uint32_t* r_blockh, uint32_t* r_depth)
{
	ERR_FAIL_COND_V(p_mipmaps == 0, 0);
	uint32_t w = p_width;
	uint32_t h = p_height;
	uint32_t d = p_depth;

	uint32_t size = 0;

	uint32_t pixel_size = get_image_format_pixel_size(p_format);
	uint32_t blockw = 0;
	uint32_t blockh = 0;
	get_compressed_image_format_block_dimensions(p_format, blockw, blockh);

	for (uint32_t i = 0; i < p_mipmaps; i++) {
		uint32_t bw = STEPIFY(w, blockw);
		uint32_t bh = STEPIFY(h, blockh);

		uint32_t s = bw * bh;

		s *= pixel_size;
		s = get_compressed_image_format_pixels_shifted(p_format, s);
		size += s * d;
		if (r_blockw) {
			*r_blockw = bw;
		}
		if (r_blockh) {
			*r_blockh = bh;
		}
		if (r_depth) {
			*r_depth = d;
		}
		w = MAX(blockw, w >> 1);
		h = MAX(blockh, h >> 1);
		d = MAX(1u, d >> 1);
	}

	return size;
}

uint32_t RenderingDeviceCommons::get_image_required_mipmaps(
	uint32_t p_width, uint32_t p_height, uint32_t p_depth)
{
	// Formats and block size don't really matter here since they can all go down to 1px (even if
	// block is larger).
	uint32_t w = p_width;
	uint32_t h = p_height;
	uint32_t d = p_depth;

	uint32_t mipmaps = 1;

	while (true) {
		if (w == 1 && h == 1 && d == 1) {
			break;
		}

		w = MAX(1u, w >> 1);
		h = MAX(1u, h >> 1);
		d = MAX(1u, d >> 1);

		mipmaps++;
	}

	return mipmaps;
}

bool RenderingDeviceCommons::format_has_depth(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_D16_UNORM:
	case DATA_FORMAT_X8_D24_UNORM_PACK32:
	case DATA_FORMAT_D32_SFLOAT:
	case DATA_FORMAT_D16_UNORM_S8_UINT:
	case DATA_FORMAT_D24_UNORM_S8_UINT:
	case DATA_FORMAT_D32_SFLOAT_S8_UINT:
		return true;
	default: {
	}
	}
	return false;
}

bool RenderingDeviceCommons::format_has_stencil(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_S8_UINT:
	case DATA_FORMAT_D16_UNORM_S8_UINT:
	case DATA_FORMAT_D24_UNORM_S8_UINT:
	case DATA_FORMAT_D32_SFLOAT_S8_UINT: {
		return true;
	}
	default: {
	}
	}
	return false;
}

uint32_t RenderingDeviceCommons::format_get_plane_count(DataFormat p_format)
{
	uint32_t planes = 1;
	switch (p_format) {
	case DATA_FORMAT_D16_UNORM_S8_UINT:
	case DATA_FORMAT_D24_UNORM_S8_UINT:
	case DATA_FORMAT_D32_SFLOAT_S8_UINT: {
		planes = 2;
		break;
	}
	default: {
	}
	}
	DEV_ASSERT(planes <= MAX_IMAGE_FORMAT_PLANES);
	return planes;
}

/*****************/
/**** SAMPLER ****/
/*****************/

const Color RenderingDeviceCommons::SAMPLER_BORDER_COLOR_VALUE[SAMPLER_BORDER_COLOR_MAX] = {
	Color(0, 0, 0, 0),
	Color(0, 0, 0, 0),
	Color(0, 0, 0, 1),
	Color(0, 0, 0, 1),
	Color(1, 1, 1, 1),
	Color(1, 1, 1, 1),
};

/**********************/
/**** VERTEX ARRAY ****/
/**********************/

uint32_t RenderingDeviceCommons::get_format_vertex_size(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_R8_UNORM:
	case DATA_FORMAT_R8_SNORM:
	case DATA_FORMAT_R8_UINT:
	case DATA_FORMAT_R8_SINT:
	case DATA_FORMAT_R8G8_UNORM:
	case DATA_FORMAT_R8G8_SNORM:
	case DATA_FORMAT_R8G8_UINT:
	case DATA_FORMAT_R8G8_SINT:
	case DATA_FORMAT_R8G8B8_UNORM:
	case DATA_FORMAT_R8G8B8_SNORM:
	case DATA_FORMAT_R8G8B8_UINT:
	case DATA_FORMAT_R8G8B8_SINT:
	case DATA_FORMAT_B8G8R8_UNORM:
	case DATA_FORMAT_B8G8R8_SNORM:
	case DATA_FORMAT_B8G8R8_UINT:
	case DATA_FORMAT_B8G8R8_SINT:
	case DATA_FORMAT_R8G8B8A8_UNORM:
	case DATA_FORMAT_R8G8B8A8_SNORM:
	case DATA_FORMAT_R8G8B8A8_UINT:
	case DATA_FORMAT_R8G8B8A8_SINT:
	case DATA_FORMAT_B8G8R8A8_UNORM:
	case DATA_FORMAT_B8G8R8A8_SNORM:
	case DATA_FORMAT_B8G8R8A8_UINT:
	case DATA_FORMAT_B8G8R8A8_SINT:
	case DATA_FORMAT_A2B10G10R10_UNORM_PACK32:
		return 4;
	case DATA_FORMAT_R16_UNORM:
	case DATA_FORMAT_R16_SNORM:
	case DATA_FORMAT_R16_UINT:
	case DATA_FORMAT_R16_SINT:
	case DATA_FORMAT_R16_SFLOAT:
		return 4;
	case DATA_FORMAT_R16G16_UNORM:
	case DATA_FORMAT_R16G16_SNORM:
	case DATA_FORMAT_R16G16_UINT:
	case DATA_FORMAT_R16G16_SINT:
	case DATA_FORMAT_R16G16_SFLOAT:
		return 4;
	case DATA_FORMAT_R16G16B16_UNORM:
	case DATA_FORMAT_R16G16B16_SNORM:
	case DATA_FORMAT_R16G16B16_UINT:
	case DATA_FORMAT_R16G16B16_SINT:
	case DATA_FORMAT_R16G16B16_SFLOAT:
		return 8;
	case DATA_FORMAT_R16G16B16A16_UNORM:
	case DATA_FORMAT_R16G16B16A16_SNORM:
	case DATA_FORMAT_R16G16B16A16_UINT:
	case DATA_FORMAT_R16G16B16A16_SINT:
	case DATA_FORMAT_R16G16B16A16_SFLOAT:
		return 8;
	case DATA_FORMAT_R32_UINT:
	case DATA_FORMAT_R32_SINT:
	case DATA_FORMAT_R32_SFLOAT:
		return 4;
	case DATA_FORMAT_R32G32_UINT:
	case DATA_FORMAT_R32G32_SINT:
	case DATA_FORMAT_R32G32_SFLOAT:
		return 8;
	case DATA_FORMAT_R32G32B32_UINT:
	case DATA_FORMAT_R32G32B32_SINT:
	case DATA_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case DATA_FORMAT_R32G32B32A32_UINT:
	case DATA_FORMAT_R32G32B32A32_SINT:
	case DATA_FORMAT_R32G32B32A32_SFLOAT:
		return 16;
	case DATA_FORMAT_R64_UINT:
	case DATA_FORMAT_R64_SINT:
	case DATA_FORMAT_R64_SFLOAT:
		return 8;
	case DATA_FORMAT_R64G64_UINT:
	case DATA_FORMAT_R64G64_SINT:
	case DATA_FORMAT_R64G64_SFLOAT:
		return 16;
	case DATA_FORMAT_R64G64B64_UINT:
	case DATA_FORMAT_R64G64B64_SINT:
	case DATA_FORMAT_R64G64B64_SFLOAT:
		return 24;
	case DATA_FORMAT_R64G64B64A64_UINT:
	case DATA_FORMAT_R64G64B64A64_SINT:
	case DATA_FORMAT_R64G64B64A64_SFLOAT:
		return 32;
	default:
		return 0;
	}
}

/****************/
/**** SHADER ****/
/****************/

const char* RenderingDeviceCommons::SHADER_STAGE_NAMES[SHADER_STAGE_MAX] = {
	"Vertex",
	"Fragment",
	"TesselationControl",
	"TesselationEvaluation",
	"Compute",
};

// --- Merged from servers/rendering/rendering_device_binds.cpp ---

Error RDShaderFile::parse_versions_from_text(const String& p_text, const String p_defines,
	OpenIncludeFunction p_include_func, void* p_include_func_userdata)
{
	Vector<String> lines = p_text.split("\n");

	bool reading_versions = false;
	bool stage_found[RDC::SHADER_STAGE_MAX] = {};
	RDC::ShaderStage stage = RDC::SHADER_STAGE_MAX;
	static const char* stage_str[RDC::SHADER_STAGE_MAX] = {
		"vertex",
		"fragment",
		"tesselation_control",
		"tesselation_evaluation",
		"compute",
		"raygen",
		"any_hit",
		"closest_hit",
		"miss",
		"intersection",
	};
	String stage_code[RDC::SHADER_STAGE_MAX];
	int stages_found = 0;
	HashMap<String, String> version_texts;

	versions.clear();
	base_error = "";

	for (int lidx = 0; lidx < lines.size(); lidx++) {
		String line = lines[lidx];

		{
			String ls = line.strip_edges();
			if (ls.begins_with("#[") && ls.ends_with("]")) {
				String section = ls.substr(2, ls.length() - 3).strip_edges();
				if (section == "versions") {
					if (stages_found) {
						base_error =
							"Invalid shader file, #[versions] must be the first section found.";
						break;
					}
					reading_versions = true;
				}
				else {
					for (int i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
						if (section == stage_str[i]) {
							if (stage_found[i]) {
								base_error = "Invalid shader file, stage appears twice: " + section;
								break;
							}

							stage_found[i] = true;
							stages_found++;

							stage = RDC::ShaderStage(i);
							reading_versions = false;
							break;
						}
					}

					if (!base_error.is_empty()) {
						break;
					}
				}

				continue;
			}
		}

		if (stage == RDC::SHADER_STAGE_MAX && !line.strip_edges().is_empty()) {
			line = line.strip_edges();
			if (line.begins_with("//") || line.begins_with("/*")) {
				continue; // assuming comment (single line)
			}
		}

		if (reading_versions) {
			String l = line.strip_edges();
			if (!l.is_empty()) {
				if (!l.contains_char('=')) {
					base_error = "Missing `=` in '" + l +
								 "'. Version syntax is `version = \"<defines with C escaping>\";`.";
					break;
				}
				if (!l.contains_char(';')) {
					// We don't require a semicolon per se, but it's needed for clang-format to
					// handle things properly.
					base_error = "Missing `;` in '" + l +
								 "'. Version syntax is `version = \"<defines with C escaping>\";`.";
					break;
				}
				Vector<String> slices = l.get_slicec(';', 0).split("=");
				String version = slices[0].strip_edges();
				if (!version.is_valid_ascii_identifier()) {
					base_error =
						"Version names must be valid identifiers, found '" + version + "' instead.";
					break;
				}
				String define = slices[1].strip_edges();
				if (!define.begins_with("\"") || !define.ends_with("\"")) {
					base_error =
						"Version text must be quoted using \"\", instead found '" + define + "'.";
					break;
				}
				define = "\n" + define.substr(1, define.length() - 2).c_unescape() +
						 "\n"; // Add newline before and after just in case.

				version_texts[version] = define + "\n" + p_defines;
			}
		}
		else {
			if (stage == RDC::SHADER_STAGE_MAX && !line.strip_edges().is_empty()) {
				base_error = "Text was found that does not belong to a valid section: " + line;
				break;
			}

			if (stage != RDC::SHADER_STAGE_MAX) {
				if (line.strip_edges().begins_with("#include")) {
					if (p_include_func) {
						// process include
						String include = line.replace("#include", "").strip_edges();
						if (!include.begins_with("\"") || !include.ends_with("\"")) {
							base_error = "Malformed #include syntax, expected #include \"<path>\", "
										 "found instead: " +
										 include;
							break;
						}
						include = include.substr(1, include.length() - 2).strip_edges();

						String include_code = ShaderIncludeDB::get_built_in_include_file(include);
						if (!include_code.is_empty()) {
							stage_code[stage] += "\n" + include_code + "\n";
						}
						else {
							String include_text = p_include_func(include, p_include_func_userdata);
							if (!include_text.is_empty()) {
								stage_code[stage] += "\n" + include_text + "\n";
							}
							else {
								base_error = "#include failed for file '" + include + "'.";
							}
						}
					}
					else {
						base_error = "#include used, but no include function provided.";
					}
				}
				else {
					stage_code[stage] += line + "\n";
				}
			}
		}
	}

	if (base_error.is_empty()) {
		if (stage_found[RDC::SHADER_STAGE_COMPUTE] && stages_found > 1) {
			ERR_FAIL_V_MSG(ERR_PARSE_ERROR,
				"When writing compute shaders, [compute] mustbe the only stage present.");
		}

		if (version_texts.is_empty()) {
			version_texts[""] = ""; // make sure a default version exists
		}

		bool errors_found = false;

		/* STEP 2, Compile the versions, add to shader file */

		for (const KeyValue<String, String>& E : version_texts) {
			Ref<RDShaderSPIRV> bytecode;
			bytecode.instantiate();

			for (int i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
				String code = stage_code[i];
				if (code.is_empty()) {
					continue;
				}
				code = code.replace("VERSION_DEFINES", E.value);
				String error;
#ifdef MODULE_GLSLANG_ENABLED
				Vector<uint8_t> spirv = compile_glslang_shader(RDC::ShaderStage(i),
					ShaderIncludeDB::parse_include_files(code),
					RDC::SHADER_LANGUAGE_VULKAN_VERSION_1_1, RDC::SHADER_SPIRV_VERSION_1_4, &error);
				bytecode->set_stage_bytecode(RDC::ShaderStage(i), spirv);
#else
				error = "Shader compilation is not supported because glslang was not enabled.";
#endif
				if (!error.is_empty()) {
					error += String() + "\n\nStage '" + stage_str[i] + "' source code: \n\n";
					Vector<String> sclines = code.split("\n");
					for (int j = 0; j < sclines.size(); j++)
 {
						error += itos(j + 1) + "\t\t" + sclines[j] + "\n";
					}
					errors_found = true;
				}
				bytecode->set_stage_compile_error(RDC::ShaderStage(i), error);
			}

			set_bytecode(bytecode, E.key);
		}

		return errors_found ? ERR_PARSE_ERROR : OK;
	}
	else {
		return ERR_PARSE_ERROR;
	}
}

// --- Merged from servers/rendering/rendering_device_driver.cpp ---

/**************/
/**** MISC ****/
/**************/

uint64_t RenderingDeviceDriver::api_trait_get(ApiTrait p_trait) {
	// Sensible canonical defaults.
	switch (p_trait) {
		case API_TRAIT_HONORS_PIPELINE_BARRIERS:
			return 1;
		case API_TRAIT_SHADER_CHANGE_INVALIDATION:
			return SHADER_CHANGE_INVALIDATION_ALL_BOUND_UNIFORM_SETS;
		case API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT:
			return 1;
		case API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP:
			return 1;
		case API_TRAIT_SECONDARY_VIEWPORT_SCISSOR:
			return 1;
		case API_TRAIT_CLEARS_WITH_COPY_ENGINE:
			return true;
		case API_TRAIT_USE_GENERAL_IN_COPY_QUEUES:
			return false;
		case API_TRAIT_BUFFERS_REQUIRE_TRANSITIONS:
			return false;
		case API_TRAIT_TEXTURE_OUTPUTS_REQUIRE_CLEARS:
			return false;
		default:
			ERR_FAIL_V(0);
	}
}

RenderingDeviceDriver::~RenderingDeviceDriver() {}

// --- Merged from servers/rendering/rendering_context_driver.cpp ---

RenderingContextDriver::~RenderingContextDriver() {}

RenderingContextDriver::SurfaceID RenderingContextDriver::surface_get_from_window(
	DisplayServerEnums::WindowID p_window) const
{
	// for (const KeyValue<DisplayServerEnums::WindowID, SurfaceID>& E : window_surface_map) {
	// 	printf("[MAP ENTRY] key=%d, surface=%llu\n", (int)E.key, (unsigned long long)E.value);
	// }

	HashMap<DisplayServerEnums::WindowID, SurfaceID>::ConstIterator it =
		window_surface_map.find(p_window);
	if (it != window_surface_map.end()) {
		// printf("[MAP HIT] found surface=%llu for window=%d\n", (unsigned long long)it->value,
		// 	(int)p_window);
		return it->value;
	}
	else {
		return SurfaceID();
	}
}

Error RenderingContextDriver::window_create(
	DisplayServerEnums::WindowID p_window, const void* p_platform_data)
{
	SurfaceID surface = surface_create(p_platform_data);
	if (surface != 0) {
		window_surface_map[p_window] = surface;
		return OK;
	}
	else {
		return ERR_CANT_CREATE;
	}
}

void RenderingContextDriver::window_set_size(
	DisplayServerEnums::WindowID p_window, uint32_t p_width, uint32_t p_height)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_size(surface, p_width, p_height);
	}
}

void RenderingContextDriver::window_set_vsync_mode(
	DisplayServerEnums::WindowID p_window, DisplayServerEnums::VSyncMode p_vsync_mode)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_vsync_mode(surface, p_vsync_mode);
	}
}

DisplayServerEnums::VSyncMode RenderingContextDriver::window_get_vsync_mode(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_vsync_mode(surface);
	}
	else {
		return DisplayServerEnums::VSYNC_DISABLED;
	}
}

void RenderingContextDriver::window_set_hdr_output_enabled(
	DisplayServerEnums::WindowID p_window, bool p_enabled)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_enabled(surface, p_enabled);
	}
}

bool RenderingContextDriver::window_get_hdr_output_enabled(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_enabled(surface);
	}
	else {
		return false;
	}
}

void RenderingContextDriver::window_set_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window, float p_reference_luminance)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_reference_luminance(surface, p_reference_luminance);
	}
}

float RenderingContextDriver::window_get_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_reference_luminance(surface);
	}
	else {
		return 0.0f;
	}
}

void RenderingContextDriver::window_set_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window, float p_max_luminance)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_max_luminance(surface, p_max_luminance);
	}
}

float RenderingContextDriver::window_get_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_max_luminance(surface);
	}
	else {
		return 0.0f;
	}
}

void RenderingContextDriver::window_set_hdr_output_linear_luminance_scale(
	DisplayServerEnums::WindowID p_window, float p_linear_luminance_scale)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_linear_luminance_scale(surface, p_linear_luminance_scale);
	}
}

float RenderingContextDriver::window_get_hdr_output_linear_luminance_scale(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_linear_luminance_scale(surface);
	}
	else {
		return 0.0f;
	}
}

float RenderingContextDriver::window_get_output_max_linear_value(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		if (surface_get_hdr_output_enabled(surface)) {
			return surface_get_hdr_output_max_value(surface);
		}
	}
	return 1.0f; // SDR
}

void RenderingContextDriver::window_destroy(DisplayServerEnums::WindowID p_window)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_destroy(surface);
	}

	window_surface_map.erase(p_window);
}

String RenderingContextDriver::get_driver_and_device_memory_report() const
{
	String report;

	const uint32_t num_tracked_obj_types = static_cast<uint32_t>(get_tracked_object_type_count());

	report += "=== Driver Memory Report ===";

	report += "\nLaunch with --extra-gpu-memory-tracking and build with "
			  "DEBUG_ENABLED for this functionality to work.";
	report += "\nDevice memory may be unavailable if the API does not support it"
			  "(e.g. VK_EXT_device_memory_report is unsupported).";
	report += "\n";

	report += "\nTotal Driver Memory:";
	report += String::num_real(double(get_driver_total_memory()) / (1024.0 * 1024.0));
	report += " MB";
	report += "\nTotal Driver Num Allocations: ";
	report += String::num_uint64(get_driver_allocation_count());

	report += "\nTotal Device Memory:";
	report += String::num_real(double(get_device_total_memory()) / (1024.0 * 1024.0));
	report += " MB";
	report += "\nTotal Device Num Allocations: ";
	report += String::num_uint64(get_device_allocation_count());

	report += "\n\nMemory use by object type (CSV format):";
	report += "\n\nCategory; Driver memory in MB; Driver Allocation Count; "
			  "Device memory in MB; Device Allocation Count";

	for (uint32_t i = 0u; i < num_tracked_obj_types; ++i) {
		report += "\n";
		report += get_tracked_object_name(i);
		report += ";";
		report += String::num_real(double(get_driver_memory_by_object_type(i)) / (1024.0 * 1024.0));
		report += ";";
		report += String::num_uint64(get_driver_allocs_by_object_type(i));
		report += ";";
		report += String::num_real(double(get_device_memory_by_object_type(i)) / (1024.0 * 1024.0));
		report += ";";
		report += String::num_uint64(get_device_allocs_by_object_type(i));
	}

	return report;
}

const char* RenderingContextDriver::get_tracked_object_name(uint32_t p_type_index) const
{
	return "Tracking Unsupported by API";
}

uint64_t RenderingContextDriver::get_tracked_object_type_count() const { return 0; }

uint64_t RenderingContextDriver::get_driver_total_memory() const { return 0; }

uint64_t RenderingContextDriver::get_driver_allocation_count() const { return 0; }

uint64_t RenderingContextDriver::get_driver_memory_by_object_type(uint32_t) const { return 0; }

uint64_t RenderingContextDriver::get_driver_allocs_by_object_type(uint32_t) const { return 0; }

uint64_t RenderingContextDriver::get_device_total_memory() const { return 0; }

uint64_t RenderingContextDriver::get_device_allocation_count() const { return 0; }

uint64_t RenderingContextDriver::get_device_memory_by_object_type(uint32_t) const { return 0; }

uint64_t RenderingContextDriver::get_device_allocs_by_object_type(uint32_t) const { return 0; }

// --- Merged from servers/rendering/rendering_shader_container.cpp ---

static inline uint32_t aligned_to(uint32_t p_size, uint32_t p_alignment)
{
	if (p_size % p_alignment) {
		return p_size + (p_alignment - (p_size % p_alignment));
	}
	else {
		return p_size;
	}
}

template <class T>
const T& RenderingShaderContainer::ReflectSymbol<T>::get_spv_reflect(RDC::ShaderStage p_stage) const
{
	const T* info = _spv_reflect[get_index_for_stage(p_stage)];
	DEV_ASSERT(info != nullptr); // Caller is expected to specify valid shader stages
	return *info;
}

RenderingShaderContainer::ReflectShaderStage::ReflectShaderStage()
{
	_module = memnew(SpvReflectShaderModule);
	memset(_module, 0, sizeof(SpvReflectShaderModule));
}

RenderingShaderContainer::ReflectShaderStage::~ReflectShaderStage()
{
	spvReflectDestroyShaderModule(_module);
	memdelete(_module);
	_module = nullptr;
}

const SpvReflectShaderModule& RenderingShaderContainer::ReflectShaderStage::module() const
{
	return *_module;
}

const Span<uint32_t> RenderingShaderContainer::ReflectShaderStage::spirv() const
{
	return _spirv_data.span().reinterpret<uint32_t>();
}

uint32_t RenderingShaderContainer::_from_bytes_header_extra_data(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_extra_data(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_binding_uniform_extra_data_start(
	const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_binding_uniform_extra_data(
	const uint8_t* p_bytes, uint32_t p_index)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_specialization_extra_data_start(
	const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_specialization_extra_data(
	const uint8_t* p_bytes, uint32_t p_index)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_shader_extra_data_start(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_shader_extra_data(
	const uint8_t* p_bytes, uint32_t p_index)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_footer_extra_data(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_header_extra_data(uint8_t*) const { return 0; }

uint32_t RenderingShaderContainer::_to_bytes_reflection_extra_data(uint8_t*) const { return 0; }

uint32_t RenderingShaderContainer::_to_bytes_reflection_binding_uniform_extra_data(
	uint8_t*, uint32_t) const
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_reflection_specialization_extra_data(
	uint8_t*, uint32_t) const
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_shader_extra_data(uint8_t*, uint32_t) const
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_footer_extra_data(uint8_t*) const { return 0; }

void RenderingShaderContainer::_set_from_shader_reflection_post(const ReflectShader& p_shader)
{
	// Do nothing.
}

static RenderingDeviceCommons::TextureType _spv_image_dim_to_texture_type(
	SpvDim p_dim, bool p_arrayed)
{
	switch (p_dim) {
	case SpvDim1D:
		return p_arrayed ? RenderingDeviceCommons::TEXTURE_TYPE_1D_ARRAY
						 : RenderingDeviceCommons::TEXTURE_TYPE_1D;
	case SpvDim2D:
		return p_arrayed ? RenderingDeviceCommons::TEXTURE_TYPE_2D_ARRAY
						 : RenderingDeviceCommons::TEXTURE_TYPE_2D;
	case SpvDim3D:
		return RenderingDeviceCommons::TEXTURE_TYPE_3D;
	case SpvDimCube:
		return p_arrayed ? RenderingDeviceCommons::TEXTURE_TYPE_CUBE_ARRAY
						 : RenderingDeviceCommons::TEXTURE_TYPE_CUBE;
	default:
		return RenderingDeviceCommons::TEXTURE_TYPE_MAX;
	}
}

static RenderingDeviceCommons::DataFormat _spv_image_format_to_data_format(
	const SpvImageFormat p_format)
{
	using RDC = RenderingDeviceCommons;
	switch (p_format) {
	case SpvImageFormatUnknown:
		return RDC::DATA_FORMAT_MAX;
	case SpvImageFormatRgba32f:
		return RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
	case SpvImageFormatRgba16f:
		return RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	case SpvImageFormatR32f:
		return RDC::DATA_FORMAT_R32_SFLOAT;
	case SpvImageFormatRgba8:
		return RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	case SpvImageFormatRgba8Snorm:
		return RDC::DATA_FORMAT_R8G8B8A8_SNORM;
	case SpvImageFormatRg32f:
		return RDC::DATA_FORMAT_R32G32_SFLOAT;
	case SpvImageFormatRg16f:
		return RDC::DATA_FORMAT_R16G16_SFLOAT;
	case SpvImageFormatR11fG11fB10f:
		return RDC::DATA_FORMAT_B10G11R11_UFLOAT_PACK32;
	case SpvImageFormatR16f:
		return RDC::DATA_FORMAT_R16_SFLOAT;
	case SpvImageFormatRgba16:
		return RDC::DATA_FORMAT_R16G16B16A16_UNORM;
	case SpvImageFormatRgb10A2:
		return RDC::DATA_FORMAT_A2B10G10R10_UNORM_PACK32;
	case SpvImageFormatRg16:
		return RDC::DATA_FORMAT_R16G16_UNORM;
	case SpvImageFormatRg8:
		return RDC::DATA_FORMAT_R8G8_UNORM;
	case SpvImageFormatR16:
		return RDC::DATA_FORMAT_R16_UNORM;
	case SpvImageFormatR8:
		return RDC::DATA_FORMAT_R8_UNORM;
	case SpvImageFormatRgba16Snorm:
		return RDC::DATA_FORMAT_R16G16B16A16_SNORM;
	case SpvImageFormatRg16Snorm:
		return RDC::DATA_FORMAT_R16G16_SNORM;
	case SpvImageFormatRg8Snorm:
		return RDC::DATA_FORMAT_R8G8_SNORM;
	case SpvImageFormatR16Snorm:
		return RDC::DATA_FORMAT_R16_SNORM;
	case SpvImageFormatR8Snorm:
		return RDC::DATA_FORMAT_R8_SNORM;
	case SpvImageFormatRgba32i:
		return RDC::DATA_FORMAT_R32G32B32A32_SINT;
	case SpvImageFormatRgba16i:
		return RDC::DATA_FORMAT_R16G16B16A16_SINT;
	case SpvImageFormatRgba8i:
		return RDC::DATA_FORMAT_R8G8B8A8_SINT;
	case SpvImageFormatR32i:
		return RDC::DATA_FORMAT_R32_SINT;
	case SpvImageFormatRg32i:
		return RDC::DATA_FORMAT_R32G32_SINT;
	case SpvImageFormatRg16i:
		return RDC::DATA_FORMAT_R16G16_SINT;
	case SpvImageFormatRg8i:
		return RDC::DATA_FORMAT_R8G8_SINT;
	case SpvImageFormatR16i:
		return RDC::DATA_FORMAT_R16_SINT;
	case SpvImageFormatR8i:
		return RDC::DATA_FORMAT_R8_SINT;
	case SpvImageFormatRgba32ui:
		return RDC::DATA_FORMAT_R32G32B32A32_UINT;
	case SpvImageFormatRgba16ui:
		return RDC::DATA_FORMAT_R16G16B16A16_UINT;
	case SpvImageFormatRgba8ui:
		return RDC::DATA_FORMAT_R8G8B8A8_UINT;
	case SpvImageFormatR32ui:
		return RDC::DATA_FORMAT_R32_UINT;
	case SpvImageFormatRgb10a2ui:
		return RDC::DATA_FORMAT_A2B10G10R10_UINT_PACK32;
	case SpvImageFormatRg32ui:
		return RDC::DATA_FORMAT_R32G32_UINT;
	case SpvImageFormatRg16ui:
		return RDC::DATA_FORMAT_R16G16_UINT;
	case SpvImageFormatRg8ui:
		return RDC::DATA_FORMAT_R8G8_UINT;
	case SpvImageFormatR16ui:
		return RDC::DATA_FORMAT_R16_UINT;
	case SpvImageFormatR8ui:
		return RDC::DATA_FORMAT_R8_UINT;
	case SpvImageFormatR64ui:
		return RDC::DATA_FORMAT_R64_UINT;
	case SpvImageFormatR64i:
		return RDC::DATA_FORMAT_R64_SINT;
	case SpvImageFormatMax:
		return RDC::DATA_FORMAT_MAX;
	}
	return RDC::DATA_FORMAT_MAX;
}

Error RenderingShaderContainer::reflect_spirv(
	const String& p_shader_name, Span<RDC::ShaderStageSPIRVData> p_spirv, ReflectShader& r_shader)
{
	ReflectShader& reflection = r_shader;

	shader_name = p_shader_name.utf8();

	const uint32_t spirv_size = p_spirv.size() + 0;

	LocalVector<ReflectShaderStage>& r_refl = r_shader.shader_stages;
	r_refl.resize(spirv_size);

	bool pipeline_type_detected = false;
	for (uint32_t i = 0; i < spirv_size; i++) {
		RDC::ShaderStage stage = p_spirv[i].shader_stage;
		RDC::ShaderStage stage_flag = (RDC::ShaderStage)(1 << stage);
		r_refl[i].shader_stage = stage;
		r_refl[i]._spirv_data = p_spirv[i].spirv;

		RDC::PipelineType pipeline_type = {};
		switch (stage) {
		case RDC::SHADER_STAGE_VERTEX:
		case RDC::SHADER_STAGE_FRAGMENT:
		case RDC::SHADER_STAGE_TESSELATION_CONTROL:
		case RDC::SHADER_STAGE_TESSELATION_EVALUATION:
			pipeline_type = RDC::PIPELINE_TYPE_RASTERIZATION;
			break;
		case RDC::SHADER_STAGE_COMPUTE:
			pipeline_type = RDC::PIPELINE_TYPE_COMPUTE;
			break;
		case RDC::SHADER_STAGE_RAYGEN:
		case RDC::SHADER_STAGE_ANY_HIT:
		case RDC::SHADER_STAGE_CLOSEST_HIT:
		case RDC::SHADER_STAGE_MISS:
		case RDC::SHADER_STAGE_INTERSECTION:
			pipeline_type = RDC::PIPELINE_TYPE_RAYTRACING;
			break;
		default:
			DEV_ASSERT(false && "Unknown shader stage.");
		}

		if (pipeline_type_detected) {
			ERR_FAIL_COND_V_MSG(r_shader.pipeline_type != pipeline_type, FAILED,
				"Shader stages of different pipeline types cannot be mixed in the same shader "
				"container.");
		}
		else {
			r_shader.pipeline_type = pipeline_type;
			pipeline_type_detected = true;
		}

		const Vector<uint64_t>& dynamic_buffers = p_spirv[i].dynamic_buffers;

		if (stage == RDC::SHADER_STAGE_COMPUTE) {
			ERR_FAIL_COND_V_MSG(spirv_size != 1, FAILED,
				"Compute shaders can only receive one stage, dedicated to compute.");
		}
		reflection.stages_bits |= (1 << stage);

		// We make all raytracing stages visible to make our lives easier when creating raytracing
		// pipelines. This makes no practical difference in current graphics drivers, since Vulkan
		// is the outlier.
		uint32_t uniform_stage_flags;
		if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
			uniform_stage_flags = RDC::SHADER_STAGE_RAYGEN | RDC::SHADER_STAGE_ANY_HIT |
								  RDC::SHADER_STAGE_CLOSEST_HIT | RDC::SHADER_STAGE_MISS |
								  RDC::SHADER_STAGE_INTERSECTION;
		}
		else {
			uniform_stage_flags = stage_flag;
		}

		{
			SpvReflectShaderModule& module = *r_refl.ptr()[i]._module;
			const uint8_t* spirv = p_spirv[i].spirv.ptr();
			SpvReflectResult result = spvReflectCreateShaderModule2(
				SPV_REFLECT_MODULE_FLAG_NO_COPY, p_spirv[i].spirv.size(), spirv, &module);
			ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
				"Reflection of SPIR-V shader stage '" +
					String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
					"' failed parsing shader.");

			for (uint32_t j = 0; j < module.capability_count; j++) {
				if (module.capabilities[j].value == SpvCapabilityMultiView) {
					reflection.has_multiview = true;
					break;
				}
			}

			if (reflection.is_compute()) {
				reflection.compute_local_size[0] = module.entry_points->local_size.x;
				reflection.compute_local_size[1] = module.entry_points->local_size.y;
				reflection.compute_local_size[2] = module.entry_points->local_size.z;
			}
			uint32_t binding_count = 0;
			result = spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
			ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
				"Reflection of SPIR-V shader stage '" +
					String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
					"' failed enumerating descriptor bindings.");

			if (binding_count > 0) {
				// Parse bindings.

				Vector<SpvReflectDescriptorBinding*> bindings;
				bindings.resize(binding_count);
				result =
					spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings.ptrw());

				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed getting descriptor bindings.");

				for (uint32_t j = 0; j < binding_count; j++) {
					const SpvReflectDescriptorBinding& binding = *bindings[j];

					ReflectUniform uniform;
					uniform.set_spv_reflect(stage, &binding);

					bool need_array_dimensions = false;
					bool need_block_size = false;
					bool may_be_writable = false;
					bool is_image = false;

					switch (binding.descriptor_type) {
					case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: {
						uniform.type = RDC::UNIFORM_TYPE_SAMPLER;
						need_array_dimensions = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
						uniform.type = RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
						uniform.type = RDC::UNIFORM_TYPE_TEXTURE;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
						uniform.type = RDC::UNIFORM_TYPE_IMAGE;
						need_array_dimensions = true;
						may_be_writable = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
						uniform.type = RDC::UNIFORM_TYPE_TEXTURE_BUFFER;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
						uniform.type = RDC::UNIFORM_TYPE_IMAGE_BUFFER;
						need_array_dimensions = true;
						may_be_writable = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
						const uint64_t key =
							ShaderRD::DynamicBuffer::encode(binding.set, binding.binding);
						if (dynamic_buffers.has(key)) {
							uniform.type = RDC::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC;
							reflection.has_dynamic_buffers = true;
						}
						else {
							uniform.type = RDC::UNIFORM_TYPE_UNIFORM_BUFFER;
						}
						need_block_size = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
						const uint64_t key =
							ShaderRD::DynamicBuffer::encode(binding.set, binding.binding);
						if (dynamic_buffers.has(key)) {
							uniform.type = RDC::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC;
							reflection.has_dynamic_buffers = true;
						}
						else {
							uniform.type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
						}
						need_block_size = true;
						may_be_writable = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: {
						ERR_PRINT("Dynamic uniform buffer not supported.");
						continue;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
						ERR_PRINT("Dynamic storage buffer not supported.");
						continue;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
						uniform.type = RDC::UNIFORM_TYPE_INPUT_ATTACHMENT;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
						uniform.type = RDC::UNIFORM_TYPE_ACCELERATION_STRUCTURE;
					} break;
					}

					if (need_array_dimensions) {
						uniform.length = 1;
						for (uint32_t k = 0; k < binding.array.dims_count; k++) {
							uniform.length *= binding.array.dims[k];
						}
					}
					else if (need_block_size) {
						uniform.length = binding.block.size;
					}
					else {
						uniform.length = 0;
					}

					if (may_be_writable) {
						if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
							uniform.writable =
								!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE);
						}
						else {
							uniform.writable =
								!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) &&
								!(binding.block.decoration_flags &
									SPV_REFLECT_DECORATION_NON_WRITABLE);
						}
					}
					else {
						uniform.writable = false;
					}

					// Gather the texture type and format for runtime validation when creating
					// uniform sets. We cannot enforce the type for input attachments since they do
					// not indicate whether the texture is 2D or 2D array for multiview.
					if (is_image &&
						binding.descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
						uniform.texture_type = _spv_image_dim_to_texture_type(
							binding.image.dim, binding.image.arrayed);
						uniform.texture_format =
							_spv_image_format_to_data_format(binding.image.image_format);

						ERR_FAIL_COND_V_MSG(uniform.texture_type == RDC::TEXTURE_TYPE_MAX, FAILED,
							"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
								"', uniform '" + binding.name +
								"' does not have a valid texture type.");
					}

					uniform.binding = binding.binding;
					uint32_t set = binding.set;

					ERR_FAIL_COND_V_MSG(set >= RDC::MAX_UNIFORM_SETS, FAILED,
						"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
							"', uniform '" + binding.name + "' uses a set (" + itos(set) +
							") index larger than what is supported (" +
							itos(RDC::MAX_UNIFORM_SETS) + ").");

					if (set < (uint32_t)reflection.uniform_sets.size()) {
						// Check if this already exists.
						bool exists = false;
						for (uint32_t k = 0; k < reflection.uniform_sets[set].size(); k++) {
							if (reflection.uniform_sets[set][k].binding == uniform.binding) {
								// Already exists, verify that it's the same type.
								ERR_FAIL_COND_V_MSG(
									reflection.uniform_sets[set][k].type != uniform.type, FAILED,
									"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
										"', uniform '" + binding.name +
										"' trying to reuse location for set=" + itos(set) +
										", binding=" + itos(uniform.binding) +
										" with different uniform type.");

								// Also, verify that it's the same size.
								ERR_FAIL_COND_V_MSG(
									reflection.uniform_sets[set][k].length != uniform.length,
									FAILED,
									"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
										"', uniform '" + binding.name +
										"' trying to reuse location for set=" + itos(set) +
										", binding=" + itos(uniform.binding) +
										" with different uniform size.");

								// Also, verify that it has the same writability.
								ERR_FAIL_COND_V_MSG(
									reflection.uniform_sets[set][k].writable != uniform.writable,
									FAILED,
									"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
										"', uniform '" + binding.name +
										"' trying to reuse location for set=" + itos(set) +
										", binding=" + itos(uniform.binding) +
										" with different writability.");

								// Just append stage mask and return.
								reflection.uniform_sets[set][k].stages |= uniform_stage_flags;
								exists = true;
								break;
							}
						}

						if (exists) {
							continue; // Merged.
						}
					}

					uniform.stages |= uniform_stage_flags;

					if (set >= (uint32_t)reflection.uniform_sets.size()) {
						reflection.uniform_sets.resize(set + 1);
					}

					reflection.uniform_sets[set].push_back(uniform);
				}
			}

			{
				// Specialization constants.

				uint32_t sc_count = 0;
				result = spvReflectEnumerateSpecializationConstants(&module, &sc_count, nullptr);
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed enumerating specialization constants.");

				if (sc_count) {
					Vector<SpvReflectSpecializationConstant*> spec_constants;
					spec_constants.resize(sc_count);

					result = spvReflectEnumerateSpecializationConstants(
						&module, &sc_count, spec_constants.ptrw());
					ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
						"Reflection of SPIR-V shader stage '" +
							String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
							"' failed obtaining specialization constants.");

					for (uint32_t j = 0; j < sc_count; j++) {
						int32_t existing = -1;
						ReflectSpecializationConstant sconst;
						SpvReflectSpecializationConstant* spc = spec_constants[j];
						sconst.set_spv_reflect(stage, spc);

						if (spc->default_value_size != 4) {
							ERR_FAIL_V_MSG(FAILED,
								vformat("Reflection of SPIR-V shader stage '%s' failed because the "
										"specialization constant #%d's default value is not 4 "
										"bytes long (%d) and is currently not supported.",
									RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage],
									spc->constant_id, spc->default_value_size));
						}

						sconst.constant_id = spc->constant_id;
						sconst.int_value = 0; // Clear previous value JIC.

						switch (spc->type_description->op) {
						case SpvOpTypeBool:
							sconst.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
							sconst.bool_value = *(uint32_t*)(spc->default_value);
							break;
						case SpvOpTypeInt:
							sconst.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
							sconst.int_value = *(uint32_t*)(spc->default_value);
							break;
						case SpvOpTypeFloat:
							sconst.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT;
							sconst.float_value = *(float*)(spc->default_value);
							break;
						default:
							ERR_FAIL_V_MSG(
								FAILED, vformat("Reflection of SPIR-V shader stage '%s' failed "
												"because the specialization constant #%d does not "
												"use a known operation (%d).",
											RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage],
											spc->constant_id, spc->type_description->op));
							break;
						}

						sconst.stages |= (1 << stage);

						for (uint32_t k = 0; k < reflection.specialization_constants.size(); k++) {
							if (reflection.specialization_constants[k].constant_id ==
								sconst.constant_id) {
								ERR_FAIL_COND_V_MSG(
									reflection.specialization_constants[k].type != sconst.type,
									FAILED,
									"More than one specialization constant used for id (" +
										itos(sconst.constant_id) + "), but their types differ.");
								ERR_FAIL_COND_V_MSG(
									reflection.specialization_constants[k].int_value !=
										sconst.int_value,
									FAILED,
									"More than one specialization constant used for id (" +
										itos(sconst.constant_id) +
										"), but their default values differ.");
								existing = k;
								break;
							}
						}

						if (existing >= 0) {
							reflection.specialization_constants[existing].stages |= (1 << stage);
						}
						else {
							reflection.specialization_constants.push_back(sconst);
						}
					}

					reflection.specialization_constants.sort();
				}
			}

			if (stage == RDC::SHADER_STAGE_VERTEX || stage == RDC::SHADER_STAGE_FRAGMENT) {
				uint32_t iv_count = 0;
				result = spvReflectEnumerateInputVariables(&module, &iv_count, nullptr);
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed enumerating input variables.");

				if (iv_count) {
					Vector<SpvReflectInterfaceVariable*> input_vars;
					input_vars.resize(iv_count);

					result =
						spvReflectEnumerateInputVariables(&module, &iv_count, input_vars.ptrw());
					ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
						"Reflection of SPIR-V shader stage '" +
							String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
							"' failed obtaining input variables.");

					for (const SpvReflectInterfaceVariable* v : input_vars) {
						if (!v) {
							continue;
						}
						if (stage == RDC::SHADER_STAGE_VERTEX) {
							if (v->decoration_flags == 0) { // Regular input.
								reflection.vertex_input_mask |= (((uint64_t)1) << v->location);
							}
						}
						if (v->built_in == SpvBuiltInViewIndex) {
							reflection.has_multiview = true;
						}
					}
				}
			}

			if (stage == RDC::SHADER_STAGE_FRAGMENT) {
				uint32_t ov_count = 0;
				result = spvReflectEnumerateOutputVariables(&module, &ov_count, nullptr);
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed enumerating output variables.");

				if (ov_count) {
					Vector<SpvReflectInterfaceVariable*> output_vars;
					output_vars.resize(ov_count);

					result =
						spvReflectEnumerateOutputVariables(&module, &ov_count, output_vars.ptrw());
					ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
						"Reflection of SPIR-V shader stage '" +
							String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
							"' failed obtaining output variables.");

					for (const SpvReflectInterfaceVariable* refvar : output_vars) {
						if (!refvar) {
							continue;
						}
						if (refvar->built_in != SpvBuiltInFragDepth) {
							reflection.fragment_output_mask |= 1 << refvar->location;
						}
					}
				}
			}

			uint32_t pc_count = 0;
			result = spvReflectEnumeratePushConstantBlocks(&module, &pc_count, nullptr);
			ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
				"Reflection of SPIR-V shader stage '" +
					String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
					"' failed enumerating push constants.");

			if (pc_count) {
				ERR_FAIL_COND_V_MSG(pc_count > 1, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"': Only one push constant is supported, which should be the same across "
						"shader stages.");

				Vector<SpvReflectBlockVariable*> pconstants;
				pconstants.resize(pc_count);
				result =
					spvReflectEnumeratePushConstantBlocks(&module, &pc_count, pconstants.ptrw());
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed obtaining push constants.");
#if 0
				if (pconstants[0] == nullptr) {
					Ref<FileAccess> f = FileAccess::open("res://popo.spv", FileAccess::WRITE);
					f->store_buffer((const uint8_t *)&SpirV[0], SpirV.size() * sizeof(uint32_t));
				}
#endif

				ERR_FAIL_COND_V_MSG(reflection.push_constant_size &&
										reflection.push_constant_size != pconstants[0]->size,
					FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"': Push constant block must be the same across shader stages.");

				reflection.push_constant_size = pconstants[0]->size;
				reflection.push_constant_stages |= uniform_stage_flags;

				// print_line("Stage: " + String(RDC::SHADER_STAGE_NAMES[stage]) + " push constant
				// of size=" + itos(push_constant.push_constant_size));
			}
		}
	}

	// Sort all uniform_sets by binding.
	for (uint32_t i = 0; i < reflection.uniform_sets.size(); i++) {
		reflection.uniform_sets[i].sort();
	}

	set_from_shader_reflection(reflection);

	return OK;
}

void RenderingShaderContainer::set_from_shader_reflection(const ReflectShader& p_reflection)
{
	reflection_binding_set_uniforms_count.clear();
	reflection_binding_set_uniforms_data.clear();
	reflection_specialization_data.clear();
	reflection_shader_stages.clear();

	reflection_data.vertex_input_mask = p_reflection.vertex_input_mask;
	reflection_data.fragment_output_mask = p_reflection.fragment_output_mask;
	reflection_data.specialization_constants_count = p_reflection.specialization_constants.size();
	reflection_data.pipeline_type = p_reflection.pipeline_type;
	reflection_data.has_multiview = p_reflection.has_multiview;
	reflection_data.has_dynamic_buffers = p_reflection.has_dynamic_buffers;
	reflection_data.compute_local_size[0] = p_reflection.compute_local_size[0];
	reflection_data.compute_local_size[1] = p_reflection.compute_local_size[1];
	reflection_data.compute_local_size[2] = p_reflection.compute_local_size[2];
	reflection_data.set_count = p_reflection.uniform_sets.size();
	reflection_data.push_constant_size = p_reflection.push_constant_size;
	reflection_data.push_constant_stages_mask = uint32_t(p_reflection.push_constant_stages);
	reflection_data.shader_name_len = shader_name.length();

	ReflectionBindingData binding_data;
	for (const ReflectDescriptorSet& uniform_set : p_reflection.uniform_sets) {
		for (const ReflectUniform& uniform : uniform_set) {
			binding_data.type = uint32_t(uniform.type);
			binding_data.binding = uniform.binding;
			binding_data.stages = uint32_t(uniform.stages);
			binding_data.length = uniform.length;
			binding_data.writable = uint32_t(uniform.writable);
			binding_data.texture_type = uniform.texture_type;
			binding_data.texture_format = uniform.texture_format;
			reflection_binding_set_uniforms_data.push_back(binding_data);
		}

		reflection_binding_set_uniforms_count.push_back(uniform_set.size());
	}

	ReflectionSpecializationData specialization_data;
	for (const ReflectSpecializationConstant& spec : p_reflection.specialization_constants) {
		specialization_data.type = uint32_t(spec.type);
		specialization_data.constant_id = spec.constant_id;
		specialization_data.int_value = spec.int_value;
		specialization_data.stage_flags = uint32_t(spec.stages);
		reflection_specialization_data.push_back(specialization_data);
	}

	for (uint32_t i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
		if ((p_reflection.stages_bits & (1U << i)) != 0) {
			reflection_shader_stages.push_back(RDC::ShaderStage(i));
		}
	}

	reflection_data.stage_count = reflection_shader_stages.size();

	_set_from_shader_reflection_post(p_reflection);
}

bool RenderingShaderContainer::set_code_from_spirv(
	const String& p_shader_name, Span<RDC::ShaderStageSPIRVData> p_spirv)
{
	ReflectShader shader;
	ERR_FAIL_COND_V(reflect_spirv(p_shader_name, p_spirv, shader) != OK, false);
	return _set_code_from_spirv(shader);
}

RenderingDeviceCommons::ShaderReflection RenderingShaderContainer::get_shader_reflection() const
{
	RDC::ShaderReflection shader_refl;
	shader_refl.push_constant_size = reflection_data.push_constant_size;
	shader_refl.push_constant_stages = reflection_data.push_constant_stages_mask;
	shader_refl.vertex_input_mask = reflection_data.vertex_input_mask;
	shader_refl.fragment_output_mask = reflection_data.fragment_output_mask;
	shader_refl.pipeline_type = reflection_data.pipeline_type;
	shader_refl.has_multiview = reflection_data.has_multiview;
	shader_refl.has_dynamic_buffers = reflection_data.has_dynamic_buffers;
	shader_refl.compute_local_size[0] = reflection_data.compute_local_size[0];
	shader_refl.compute_local_size[1] = reflection_data.compute_local_size[1];
	shader_refl.compute_local_size[2] = reflection_data.compute_local_size[2];
	shader_refl.uniform_sets.resize(reflection_data.set_count);
	shader_refl.specialization_constants.resize(reflection_data.specialization_constants_count);
	shader_refl.stages_vector.resize(reflection_data.stage_count);

	DEV_ASSERT(
		reflection_binding_set_uniforms_count.size() == reflection_data.set_count &&
		"The amount of elements in the reflection and the shader container can't be different.");
	uint32_t uniform_index = 0;
	for (uint32_t i = 0; i < reflection_data.set_count; i++) {
		Vector<RDC::ShaderUniform>& uniform_set = shader_refl.uniform_sets.ptrw()[i];
		uint32_t uniforms_count = reflection_binding_set_uniforms_count[i];
		uniform_set.resize(uniforms_count);
		for (uint32_t j = 0; j < uniforms_count; j++) {
			const ReflectionBindingData& binding =
				reflection_binding_set_uniforms_data[uniform_index++];
			RDC::ShaderUniform& uniform = uniform_set.ptrw()[j];
			uniform.type = RDC::UniformType(binding.type);
			uniform.writable = binding.writable;
			uniform.length = binding.length;
			uniform.binding = binding.binding;
			uniform.stages = binding.stages;
			uniform.texture_type = binding.texture_type;
			uniform.texture_format = binding.texture_format;
		}
	}

	shader_refl.specialization_constants.resize(reflection_data.specialization_constants_count);
	for (uint32_t i = 0; i < reflection_data.specialization_constants_count; i++) {
		const ReflectionSpecializationData& spec = reflection_specialization_data[i];
		RDC::ShaderSpecializationConstant& sc = shader_refl.specialization_constants.ptrw()[i];
		sc.type = RDC::PipelineSpecializationConstantType(spec.type);
		sc.constant_id = spec.constant_id;
		sc.int_value = spec.int_value;
		sc.stages = spec.stage_flags;
	}

	shader_refl.stages_vector.resize(reflection_data.stage_count);
	for (uint32_t i = 0; i < reflection_data.stage_count; i++) {
		shader_refl.stages_vector.set(i, reflection_shader_stages[i]);
		shader_refl.stages_bits |= (1U << reflection_shader_stages[i]);
	}

	return shader_refl;
}

bool RenderingShaderContainer::from_bytes(const PackedByteArray& p_bytes)
{
	const uint64_t alignment = sizeof(uint32_t);
	const uint8_t* bytes_ptr = p_bytes.ptr();
	uint64_t bytes_offset = 0;

	// Read container header.
	ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(ContainerHeader)) > p_bytes.size(), false,
		"Not enough bytes for a container header in shader container.");
	const ContainerHeader& container_header = *(const ContainerHeader*)(&bytes_ptr[bytes_offset]);
	bytes_offset += sizeof(ContainerHeader);
	bytes_offset += _from_bytes_header_extra_data(&bytes_ptr[bytes_offset]);

	ERR_FAIL_COND_V_MSG(container_header.magic_number != CONTAINER_MAGIC_NUMBER, false,
		"Incorrect magic number in shader container.");
	ERR_FAIL_COND_V_MSG(container_header.version > CONTAINER_VERSION, false,
		"Unsupported version in shader container.");
	ERR_FAIL_COND_V_MSG(
		container_header.format != _format(), false, "Incorrect format in shader container.");
	ERR_FAIL_COND_V_MSG(container_header.format_version > _format_version(), false,
		"Unsupported format version in shader container.");

	// Adjust shaders to the size indicated by the container header.
	shaders.resize(container_header.shader_count);

	// Read reflection data.
	ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(ReflectionData)) > p_bytes.size(), false,
		"Not enough bytes for reflection data in shader container.");
	reflection_data = *(const ReflectionData*)(&bytes_ptr[bytes_offset]);
	bytes_offset += sizeof(ReflectionData);
	bytes_offset += _from_bytes_reflection_extra_data(&bytes_ptr[bytes_offset]);

	// Read shader name.
	ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + reflection_data.shader_name_len) > p_bytes.size(),
		false, "Not enough bytes for shader name in shader container.");
	if (reflection_data.shader_name_len > 0) {
		String shader_name_str;
		shader_name_str.append_utf8(
			(const char*)(&bytes_ptr[bytes_offset]), reflection_data.shader_name_len);
		shader_name = shader_name_str.utf8();
		bytes_offset = aligned_to(bytes_offset + reflection_data.shader_name_len, alignment);
	}
	else {
		shader_name = CharString();
	}

	reflection_binding_set_uniforms_count.resize(reflection_data.set_count);
	reflection_binding_set_uniforms_data.clear();

	uint32_t uniform_index = 0;
	for (uint32_t i = 0; i < reflection_data.set_count; i++) {
		ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(uint32_t)) > p_bytes.size(), false,
			"Not enough bytes for uniform set count in shader container.");
		uint32_t uniforms_count = *(uint32_t*)(&bytes_ptr[bytes_offset]);
		reflection_binding_set_uniforms_count.ptrw()[i] = uniforms_count;
		bytes_offset += sizeof(uint32_t);

		reflection_binding_set_uniforms_data.resize(
			reflection_binding_set_uniforms_data.size() + uniforms_count);
		bytes_offset +=
			_from_bytes_reflection_binding_uniform_extra_data_start(&bytes_ptr[bytes_offset]);

		for (uint32_t j = 0; j < uniforms_count; j++) {
			ERR_FAIL_COND_V_MSG(
				int64_t(bytes_offset + sizeof(ReflectionBindingData)) > p_bytes.size(), false,
				"Not enough bytes for uniform in shader container.");
			memcpy(&reflection_binding_set_uniforms_data.ptrw()[uniform_index],
				&bytes_ptr[bytes_offset], sizeof(ReflectionBindingData));
			bytes_offset += sizeof(ReflectionBindingData);
			bytes_offset += _from_bytes_reflection_binding_uniform_extra_data(
				&bytes_ptr[bytes_offset], uniform_index);
			uniform_index++;
		}
	}

	reflection_specialization_data.resize(reflection_data.specialization_constants_count);
	bytes_offset +=
		_from_bytes_reflection_specialization_extra_data_start(&bytes_ptr[bytes_offset]);

	for (uint32_t i = 0; i < reflection_data.specialization_constants_count; i++) {
		ERR_FAIL_COND_V_MSG(
			int64_t(bytes_offset + sizeof(ReflectionSpecializationData)) > p_bytes.size(), false,
			"Not enough bytes for specialization in shader container.");
		memcpy(&reflection_specialization_data.ptrw()[i], &bytes_ptr[bytes_offset],
			sizeof(ReflectionSpecializationData));
		bytes_offset += sizeof(ReflectionSpecializationData);
		bytes_offset +=
			_from_bytes_reflection_specialization_extra_data(&bytes_ptr[bytes_offset], i);
	}

	const uint32_t stage_count = reflection_data.stage_count;
	if (stage_count > 0) {
		ERR_FAIL_COND_V_MSG(
			int64_t(bytes_offset + stage_count * sizeof(RDC::ShaderStage)) > p_bytes.size(), false,
			"Not enough bytes for stages in shader container.");
		reflection_shader_stages.resize(stage_count);
		bytes_offset += _from_bytes_shader_extra_data_start(&bytes_ptr[bytes_offset]);
		memcpy(reflection_shader_stages.ptrw(), &bytes_ptr[bytes_offset],
			stage_count * sizeof(RDC::ShaderStage));
		bytes_offset += stage_count * sizeof(RDC::ShaderStage);
	}

	// Read shaders.
	for (int64_t i = 0; i < shaders.size(); i++) {
		ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(ShaderHeader)) > p_bytes.size(), false,
			"Not enough bytes for shader header in shader container.");
		const ShaderHeader& header = *(const ShaderHeader*)(&bytes_ptr[bytes_offset]);
		bytes_offset += sizeof(ShaderHeader);

		ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + header.code_compressed_size) > p_bytes.size(),
			false, "Not enough bytes for a shader in shader container.");
		Shader& shader = shaders.ptrw()[i];
		shader.shader_stage = RDC::ShaderStage(header.shader_stage);
		shader.code_compression_flags = header.code_compression_flags;
		shader.code_decompressed_size = header.code_decompressed_size;
		shader.code_compressed_bytes.resize(header.code_compressed_size);
		memcpy(shader.code_compressed_bytes.ptrw(), &bytes_ptr[bytes_offset],
			header.code_compressed_size);
		bytes_offset = aligned_to(bytes_offset + header.code_compressed_size, alignment);
		bytes_offset += _from_bytes_shader_extra_data(&bytes_ptr[bytes_offset], i);
	}

	bytes_offset += _from_bytes_footer_extra_data(&bytes_ptr[bytes_offset]);

	ERR_FAIL_COND_V_MSG(bytes_offset != (uint64_t)p_bytes.size(), false,
		"Amount of bytes in the container does not match the amount of bytes read.");
	return true;
}

PackedByteArray RenderingShaderContainer::to_bytes() const
{
	// Compute the exact size the container will require for writing everything out.
	const uint64_t alignment = sizeof(uint32_t);
	uint64_t total_size = 0;
	total_size += sizeof(ContainerHeader) + _to_bytes_header_extra_data(nullptr);
	total_size += sizeof(ReflectionData) + _to_bytes_reflection_extra_data(nullptr);
	total_size += aligned_to(reflection_data.shader_name_len, alignment);
	total_size += reflection_binding_set_uniforms_count.size() * sizeof(uint32_t);
	total_size += reflection_binding_set_uniforms_data.size() * sizeof(ReflectionBindingData);
	total_size += reflection_specialization_data.size() * sizeof(ReflectionSpecializationData);
	total_size += reflection_shader_stages.size() * sizeof(RDC::ShaderStage);

	for (uint32_t i = 0; i < reflection_binding_set_uniforms_data.size(); i++) {
		total_size += _to_bytes_reflection_binding_uniform_extra_data(nullptr, i);
	}

	for (uint32_t i = 0; i < reflection_specialization_data.size(); i++) {
		total_size += _to_bytes_reflection_specialization_extra_data(nullptr, i);
	}

	for (uint32_t i = 0; i < shaders.size(); i++) {
		total_size += sizeof(ShaderHeader);
		total_size += shaders[i].code_compressed_bytes.size();
		total_size = aligned_to(total_size, alignment);
		total_size += _to_bytes_shader_extra_data(nullptr, i);
	}

	total_size += _to_bytes_footer_extra_data(nullptr);

	// Create the array that will hold all of the data.
	PackedByteArray bytes;
	bytes.resize_initialized(total_size);

	// Write out the data to the array.
	uint64_t bytes_offset = 0;
	uint8_t* bytes_ptr = bytes.ptrw();
	ContainerHeader& container_header = *(ContainerHeader*)(&bytes_ptr[bytes_offset]);
	container_header.magic_number = CONTAINER_MAGIC_NUMBER;
	container_header.version = CONTAINER_VERSION;
	container_header.format = _format();
	container_header.format_version = _format_version();
	container_header.shader_count = shaders.size();
	bytes_offset += sizeof(ContainerHeader);
	bytes_offset += _to_bytes_header_extra_data(&bytes_ptr[bytes_offset]);

	memcpy(&bytes_ptr[bytes_offset], &reflection_data, sizeof(ReflectionData));
	bytes_offset += sizeof(ReflectionData);
	bytes_offset += _to_bytes_reflection_extra_data(&bytes_ptr[bytes_offset]);

	if (shader_name.size() > 0) {
		memcpy(&bytes_ptr[bytes_offset], shader_name.ptr(), reflection_data.shader_name_len);
		bytes_offset = aligned_to(bytes_offset + reflection_data.shader_name_len, alignment);
	}

	uint32_t uniform_index = 0;
	for (uint32_t uniform_count : reflection_binding_set_uniforms_count) {
		memcpy(&bytes_ptr[bytes_offset], &uniform_count, sizeof(uniform_count));
		bytes_offset += sizeof(uint32_t);

		for (uint32_t i = 0; i < uniform_count; i++) {
			memcpy(&bytes_ptr[bytes_offset], &reflection_binding_set_uniforms_data[uniform_index],
				sizeof(ReflectionBindingData));
			bytes_offset += sizeof(ReflectionBindingData);
			bytes_offset += _to_bytes_reflection_binding_uniform_extra_data(
				&bytes_ptr[bytes_offset], uniform_index);
			uniform_index++;
		}
	}

	for (uint32_t i = 0; i < reflection_specialization_data.size(); i++) {
		memcpy(&bytes_ptr[bytes_offset], &reflection_specialization_data.ptr()[i],
			sizeof(ReflectionSpecializationData));
		bytes_offset += sizeof(ReflectionSpecializationData);
		bytes_offset += _to_bytes_reflection_specialization_extra_data(&bytes_ptr[bytes_offset], i);
	}

	if (!reflection_shader_stages.is_empty()) {
		uint32_t stage_count = reflection_shader_stages.size();
		memcpy(&bytes_ptr[bytes_offset], reflection_shader_stages.ptr(),
			stage_count * sizeof(RDC::ShaderStage));
		bytes_offset += stage_count * sizeof(RDC::ShaderStage);
	}

	for (uint32_t i = 0; i < shaders.size(); i++) {
		const Shader& shader = shaders[i];
		ShaderHeader& header = *(ShaderHeader*)(&bytes.ptr()[bytes_offset]);
		header.shader_stage = shader.shader_stage;
		header.code_compressed_size = uint32_t(shader.code_compressed_bytes.size());
		header.code_compression_flags = shader.code_compression_flags;
		header.code_decompressed_size = shader.code_decompressed_size;
		bytes_offset += sizeof(ShaderHeader);
		memcpy(&bytes.ptrw()[bytes_offset], shader.code_compressed_bytes.ptr(),
			shader.code_compressed_bytes.size());
		bytes_offset = aligned_to(bytes_offset + shader.code_compressed_bytes.size(), alignment);
		bytes_offset += _to_bytes_shader_extra_data(&bytes_ptr[bytes_offset], i);
	}

	bytes_offset += _to_bytes_footer_extra_data(&bytes_ptr[bytes_offset]);

	ERR_FAIL_COND_V_MSG(bytes_offset != total_size, PackedByteArray(),
		"Amount of bytes written does not match the amount of bytes reserved for the container.");
	return bytes;
}

bool RenderingShaderContainer::compress_code(const uint8_t* p_decompressed_bytes,
	uint32_t p_decompressed_size, uint8_t* p_compressed_bytes, uint32_t* r_compressed_size,
	uint32_t* r_compressed_flags) const
{
	DEV_ASSERT(p_decompressed_bytes != nullptr);
	DEV_ASSERT(p_decompressed_size > 0);
	DEV_ASSERT(p_compressed_bytes != nullptr);
	DEV_ASSERT(r_compressed_size != nullptr);
	DEV_ASSERT(r_compressed_flags != nullptr);

	*r_compressed_flags = 0;

	PackedByteArray zstd_bytes;
	const int64_t zstd_max_bytes =
		Compression::get_max_compressed_buffer_size(p_decompressed_size, Compression::MODE_ZSTD);
	zstd_bytes.resize(zstd_max_bytes);

	const int64_t zstd_size = Compression::compress(
		zstd_bytes.ptrw(), p_decompressed_bytes, p_decompressed_size, Compression::MODE_ZSTD);
	if (zstd_size > 0 && (uint32_t)(zstd_size) < p_decompressed_size) {
		// Only choose Zstd if it results in actual compression.
		memcpy(p_compressed_bytes, zstd_bytes.ptr(), zstd_size);
		*r_compressed_size = zstd_size;
		*r_compressed_flags |= COMPRESSION_FLAG_ZSTD;
	}
	else {
		// Just copy the input to the output directly.
		memcpy(p_compressed_bytes, p_decompressed_bytes, p_decompressed_size);
		*r_compressed_size = p_decompressed_size;
	}

	return true;
}

bool RenderingShaderContainer::decompress_code(const uint8_t* p_compressed_bytes,
	uint32_t p_compressed_size, uint32_t p_compressed_flags, uint8_t* p_decompressed_bytes,
	uint32_t p_decompressed_size) const
{
	DEV_ASSERT(p_compressed_bytes != nullptr);
	DEV_ASSERT(p_compressed_size > 0);
	DEV_ASSERT(p_decompressed_bytes != nullptr);
	DEV_ASSERT(p_decompressed_size > 0);

	bool uses_zstd = p_compressed_flags & COMPRESSION_FLAG_ZSTD;
	if (uses_zstd) {
		if (!Compression::decompress(p_decompressed_bytes, p_decompressed_size, p_compressed_bytes,
				p_compressed_size, Compression::MODE_ZSTD)) {
			ERR_FAIL_V_MSG(false, "Malformed zstd input for decompressing shader code.");
		}
	}
	else {
		memcpy(
			p_decompressed_bytes, p_compressed_bytes, MIN(p_compressed_size, p_decompressed_size));
	}

	return true;
}

RenderingShaderContainer::RenderingShaderContainer() {}

RenderingShaderContainer::~RenderingShaderContainer() {}

template <typename T>
void RenderingShaderContainer::ReflectSymbol<T>::set_spv_reflect(
	RDC::ShaderStage p_stage, const T* p_spv)
{
}

// ============================================================================
// Merged from servers/rendering/rendering_device_graph.cpp
// ============================================================================

#include "servers/rendering/rendering_device.h"

#define PRINT_RENDER_GRAPH 0
#define FORCE_FULL_ACCESS_BITS 0
#define PRINT_RESOURCE_TRACKER_TOTAL 0
#define PRINT_COMMAND_RECORDING 0

// Prints the total number of bytes used for draw lists in a frame.
#define PRINT_DRAW_LIST_STATS 0

RenderingDeviceGraph::RenderingDeviceGraph()
{
	driver_honors_barriers = false;
	driver_clears_with_copy_engine = false;
	driver_buffers_require_transitions = false;
}

RenderingDeviceGraph::~RenderingDeviceGraph() {}

String RenderingDeviceGraph::_usage_to_string(ResourceUsage p_usage)
{
	switch (p_usage) {
	case RESOURCE_USAGE_NONE:
		return "None";
	case RESOURCE_USAGE_COPY_FROM:
		return "Copy From";
	case RESOURCE_USAGE_COPY_TO:
		return "Copy To";
	case RESOURCE_USAGE_RESOLVE_FROM:
		return "Resolve From";
	case RESOURCE_USAGE_RESOLVE_TO:
		return "Resolve To";
	case RESOURCE_USAGE_UNIFORM_BUFFER_READ:
		return "Uniform Buffer Read";
	case RESOURCE_USAGE_INDIRECT_BUFFER_READ:
		return "Indirect Buffer Read";
	case RESOURCE_USAGE_TEXTURE_BUFFER_READ:
		return "Texture Buffer Read";
	case RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE:
		return "Texture Buffer Read Write";
	case RESOURCE_USAGE_STORAGE_BUFFER_READ:
		return "Storage Buffer Read";
	case RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE:
		return "Storage Buffer Read Write";
	case RESOURCE_USAGE_VERTEX_BUFFER_READ:
		return "Vertex Buffer Read";
	case RESOURCE_USAGE_INDEX_BUFFER_READ:
		return "Index Buffer Read";
	case RESOURCE_USAGE_TEXTURE_SAMPLE:
		return "Texture Sample";
	case RESOURCE_USAGE_STORAGE_IMAGE_READ:
		return "Storage Image Read";
	case RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE:
		return "Storage Image Read Write";
	case RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE:
		return "Attachment Color Read Write";
	case RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE:
		return "Attachment Depth Stencil Read Write";
	case RESOURCE_USAGE_GENERAL:
		return "General";
	default:
		ERR_FAIL_V_MSG("Invalid", vformat("Invalid resource usage %d.", p_usage));
	}
}

bool RenderingDeviceGraph::_is_write_usage(ResourceUsage p_usage)
{
	switch (p_usage) {
	case RESOURCE_USAGE_COPY_FROM:
	case RESOURCE_USAGE_RESOLVE_FROM:
	case RESOURCE_USAGE_UNIFORM_BUFFER_READ:
	case RESOURCE_USAGE_INDIRECT_BUFFER_READ:
	case RESOURCE_USAGE_TEXTURE_BUFFER_READ:
	case RESOURCE_USAGE_STORAGE_BUFFER_READ:
	case RESOURCE_USAGE_VERTEX_BUFFER_READ:
	case RESOURCE_USAGE_INDEX_BUFFER_READ:
	case RESOURCE_USAGE_TEXTURE_SAMPLE:
	case RESOURCE_USAGE_STORAGE_IMAGE_READ:
	case RESOURCE_USAGE_ATTACHMENT_FRAGMENT_SHADING_RATE_READ:
	case RESOURCE_USAGE_ATTACHMENT_FRAGMENT_DENSITY_MAP_READ:
	case RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ:
		return false;
	case RESOURCE_USAGE_COPY_TO:
	case RESOURCE_USAGE_RESOLVE_TO:
	case RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE:
	case RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE:
	case RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE:
	case RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE:
	case RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE:
	case RESOURCE_USAGE_GENERAL:
	case RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE:
		return true;
	default:
		DEV_ASSERT(false && "Invalid resource tracker usage.");
		return false;
	}
}

RDD::TextureLayout RenderingDeviceGraph::_usage_to_image_layout(ResourceUsage p_usage)
{
	switch (p_usage) {
	case RESOURCE_USAGE_COPY_FROM:
		return RDD::TEXTURE_LAYOUT_COPY_SRC_OPTIMAL;
	case RESOURCE_USAGE_COPY_TO:
		return RDD::TEXTURE_LAYOUT_COPY_DST_OPTIMAL;
	case RESOURCE_USAGE_RESOLVE_FROM:
		return RDD::TEXTURE_LAYOUT_RESOLVE_SRC_OPTIMAL;
	case RESOURCE_USAGE_RESOLVE_TO:
		return RDD::TEXTURE_LAYOUT_RESOLVE_DST_OPTIMAL;
	case RESOURCE_USAGE_TEXTURE_SAMPLE:
		return RDD::TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	case RESOURCE_USAGE_STORAGE_IMAGE_READ:
	case RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE:
		return RDD::TEXTURE_LAYOUT_STORAGE_OPTIMAL;
	case RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE:
		return RDD::TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	case RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE:
		return RDD::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	case RESOURCE_USAGE_ATTACHMENT_FRAGMENT_SHADING_RATE_READ:
		return RDD::TEXTURE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL;
	case RESOURCE_USAGE_ATTACHMENT_FRAGMENT_DENSITY_MAP_READ:
		return RDD::TEXTURE_LAYOUT_FRAGMENT_DENSITY_MAP_ATTACHMENT_OPTIMAL;
	case RESOURCE_USAGE_GENERAL:
		return RDD::TEXTURE_LAYOUT_GENERAL;
	case RESOURCE_USAGE_NONE:
		return RDD::TEXTURE_LAYOUT_UNDEFINED;
	default:
		DEV_ASSERT(false && "Invalid resource tracker usage or not an image usage.");
		return RDD::TEXTURE_LAYOUT_UNDEFINED;
	}
}

RDD::BarrierAccessBits RenderingDeviceGraph::_usage_to_access_bits(ResourceUsage p_usage)
{
#if FORCE_FULL_ACCESS_BITS
	return RDD::BarrierAccessBits(
		RDD::BARRIER_ACCESS_MEMORY_READ_BIT | RDD::BARRIER_ACCESS_MEMORY_WRITE_BIT);
#else
	switch (p_usage) {
	case RESOURCE_USAGE_NONE:
		return RDD::BarrierAccessBits(0);
	case RESOURCE_USAGE_COPY_FROM:
		return RDD::BARRIER_ACCESS_COPY_READ_BIT;
	case RESOURCE_USAGE_COPY_TO:
		return RDD::BARRIER_ACCESS_COPY_WRITE_BIT;
	case RESOURCE_USAGE_RESOLVE_FROM:
		return RDD::BARRIER_ACCESS_RESOLVE_READ_BIT;
	case RESOURCE_USAGE_RESOLVE_TO:
		return RDD::BARRIER_ACCESS_RESOLVE_WRITE_BIT;
	case RESOURCE_USAGE_UNIFORM_BUFFER_READ:
		return RDD::BARRIER_ACCESS_UNIFORM_READ_BIT;
	case RESOURCE_USAGE_INDIRECT_BUFFER_READ:
		return RDD::BARRIER_ACCESS_INDIRECT_COMMAND_READ_BIT;
	case RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ:
		return RDD::BARRIER_ACCESS_ACCELERATION_STRUCTURE_READ_BIT;
	case RESOURCE_USAGE_STORAGE_BUFFER_READ:
	case RESOURCE_USAGE_STORAGE_IMAGE_READ:
	case RESOURCE_USAGE_TEXTURE_BUFFER_READ:
	case RESOURCE_USAGE_TEXTURE_SAMPLE:
		return RDD::BARRIER_ACCESS_SHADER_READ_BIT;
	case RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE:
	case RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE:
	case RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE:
		return RDD::BarrierAccessBits(
			RDD::BARRIER_ACCESS_SHADER_READ_BIT | RDD::BARRIER_ACCESS_SHADER_WRITE_BIT);
	case RESOURCE_USAGE_VERTEX_BUFFER_READ:
		return RDD::BARRIER_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
	case RESOURCE_USAGE_INDEX_BUFFER_READ:
		return RDD::BARRIER_ACCESS_INDEX_READ_BIT;
	case RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE:
		return RDD::BarrierAccessBits(RDD::BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT |
									  RDD::BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	case RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE:
		return RDD::BarrierAccessBits(RDD::BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
									  RDD::BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
	case RESOURCE_USAGE_ATTACHMENT_FRAGMENT_SHADING_RATE_READ:
		return RDD::BARRIER_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT;
	case RESOURCE_USAGE_ATTACHMENT_FRAGMENT_DENSITY_MAP_READ:
		return RDD::BARRIER_ACCESS_FRAGMENT_DENSITY_MAP_ATTACHMENT_READ_BIT;
	case RESOURCE_USAGE_GENERAL:
		return RDD::BarrierAccessBits(
			RDD::BARRIER_ACCESS_MEMORY_READ_BIT | RDD::BARRIER_ACCESS_MEMORY_WRITE_BIT);
	case RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE:
		return RDD::BarrierAccessBits(RDD::BARRIER_ACCESS_ACCELERATION_STRUCTURE_READ_BIT |
									  RDD::BARRIER_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT);
	default:
		DEV_ASSERT(false && "Invalid usage.");
		return RDD::BarrierAccessBits(0);
	}
#endif
}

bool RenderingDeviceGraph::_check_command_intersection(ResourceTracker* p_resource_tracker,
	int32_t p_previous_command_index, int32_t p_command_index) const
{
	if (p_resource_tracker->usage != RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE &&
		p_resource_tracker->usage != RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE) {
		// We don't check possible intersections for usages that aren't consecutive color or depth
		// writes.
		return true;
	}

	const uint32_t previous_command_data_offset = command_data_offsets[p_previous_command_index];
	const uint32_t current_command_data_offset = command_data_offsets[p_command_index];
	const RecordedDrawListCommand& previous_draw_list_command =
		*reinterpret_cast<const RecordedDrawListCommand*>(
			&command_data[previous_command_data_offset]);
	const RecordedDrawListCommand& current_draw_list_command =
		*reinterpret_cast<const RecordedDrawListCommand*>(
			&command_data[current_command_data_offset]);
	if (previous_draw_list_command.header.type != RecordedCommand::TYPE_DRAW_LIST ||
		current_draw_list_command.header.type != RecordedCommand::TYPE_DRAW_LIST) {
		// We don't check possible intersections if both commands aren't draw lists.
		return true;
	}

	// We check if the region used by both draw lists have an intersection.
	return previous_draw_list_command.region.intersects(current_draw_list_command.region);
}

bool RenderingDeviceGraph::_check_command_partial_coverage(
	ResourceTracker* p_resource_tracker, int32_t p_command_index) const
{
	if (p_resource_tracker->usage != RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE &&
		p_resource_tracker->usage != RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE) {
		// We don't check for partial coverage in usages that aren't attachment writes.
		return false;
	}

	const uint32_t command_data_offset = command_data_offsets[p_command_index];
	const RecordedDrawListCommand& draw_list_command =
		*reinterpret_cast<const RecordedDrawListCommand*>(&command_data[command_data_offset]);
	if (draw_list_command.header.type != RecordedCommand::TYPE_DRAW_LIST) {
		// We don't check for partial coverage on commands that aren't draw lists.
		return false;
	}

	Rect2i texture_region(Point2i(0, 0), p_resource_tracker->texture_size);
	return !draw_list_command.region.encloses(texture_region);
}

int32_t RenderingDeviceGraph::_add_to_command_list(int32_t p_command_index, int32_t p_list_index)
{
	DEV_ASSERT(p_command_index < int32_t(command_count));
	DEV_ASSERT(p_list_index < int32_t(command_list_nodes.size()));

	int32_t next_index = int32_t(command_list_nodes.size());
	command_list_nodes.resize(next_index + 1);

	RecordedCommandListNode& new_node = command_list_nodes[next_index];
	new_node.command_index = p_command_index;
	new_node.next_list_index = p_list_index;
	return next_index;
}

void RenderingDeviceGraph::_add_adjacent_command(
	int32_t p_previous_command_index, int32_t p_command_index, RecordedCommand* r_command)
{
	const uint32_t previous_command_data_offset = command_data_offsets[p_previous_command_index];
	RecordedCommand& previous_command =
		*reinterpret_cast<RecordedCommand*>(&command_data[previous_command_data_offset]);
	previous_command.adjacent_command_list_index =
		_add_to_command_list(p_command_index, previous_command.adjacent_command_list_index);
	previous_command.next_stages = previous_command.next_stages | r_command->self_stages;
	r_command->previous_stages = r_command->previous_stages | previous_command.self_stages;
}

int32_t RenderingDeviceGraph::_add_to_slice_read_list(
	int32_t p_command_index, Rect2i p_subresources, int32_t p_list_index)
{
	DEV_ASSERT(p_command_index < int32_t(command_count));
	DEV_ASSERT(p_list_index < int32_t(read_slice_list_nodes.size()));

	int32_t next_index = int32_t(read_slice_list_nodes.size());
	read_slice_list_nodes.resize(next_index + 1);

	RecordedSliceListNode& new_node = read_slice_list_nodes[next_index];
	new_node.command_index = p_command_index;
	new_node.next_list_index = p_list_index;
	new_node.subresources = p_subresources;
	return next_index;
}

int32_t RenderingDeviceGraph::_add_to_write_list(
	int32_t p_command_index, Rect2i p_subresources, int32_t p_list_index, bool p_partial_coverage)
{
	DEV_ASSERT(p_command_index < int32_t(command_count));
	DEV_ASSERT(p_list_index < int32_t(write_slice_list_nodes.size()));

	int32_t next_index = int32_t(write_slice_list_nodes.size());
	write_slice_list_nodes.resize(next_index + 1);

	RecordedSliceListNode& new_node = write_slice_list_nodes[next_index];
	new_node.command_index = p_command_index;
	new_node.next_list_index = p_list_index;
	new_node.subresources = p_subresources;
	new_node.partial_coverage = p_partial_coverage;
	return next_index;
}

// Ensures all commands are 8-byte aligned.
#define GRAPH_ALIGN(x) (((x) + 7u) & 0xFFFFFFF8u)

RenderingDeviceGraph::RecordedCommand* RenderingDeviceGraph::_allocate_command(
	uint32_t p_command_size, int32_t& r_command_index)
{
	uint32_t command_data_offset = command_data.size();
	command_data_offset = GRAPH_ALIGN(command_data_offset);
	command_data_offsets.push_back(command_data_offset);
	command_data.resize(command_data_offset + p_command_size);
	r_command_index = command_count++;
	RecordedCommand* new_command =
		reinterpret_cast<RecordedCommand*>(&command_data[command_data_offset]);
	*new_command = RecordedCommand();
	return new_command;
}

RenderingDeviceGraph::DrawListInstruction* RenderingDeviceGraph::_allocate_draw_list_instruction(
	uint32_t p_instruction_size)
{
	uint32_t draw_list_data_offset = draw_instruction_list.list.data.size();
	draw_list_data_offset = GRAPH_ALIGN(draw_list_data_offset);
	draw_instruction_list.list.data.resize(draw_list_data_offset + p_instruction_size);
	return reinterpret_cast<DrawListInstruction*>(
		&draw_instruction_list.list.data[draw_list_data_offset]);
}

RenderingDeviceGraph::ComputeListInstruction*
RenderingDeviceGraph::_allocate_compute_list_instruction(uint32_t p_instruction_size)
{
	uint32_t compute_list_data_offset = compute_instruction_list.list.data.size();
	compute_list_data_offset = GRAPH_ALIGN(compute_list_data_offset);
	compute_instruction_list.list.data.resize(compute_list_data_offset + p_instruction_size);
	return reinterpret_cast<ComputeListInstruction*>(
		&compute_instruction_list.list.data[compute_list_data_offset]);
}

void RenderingDeviceGraph::_check_discardable_attachment_dependency(
	ResourceTracker* p_resource_tracker, int32_t p_previous_command_index, int32_t p_command_index)
{
	if (!p_resource_tracker->is_discardable) {
		return;
	}

	// Check if the command is a a draw list that clears the attachment completely. If it is, we
	// don't need to modify the previous draw list.
	uint32_t command_offset = command_data_offsets[p_command_index];
	RecordedDrawListCommand* draw_list_command =
		reinterpret_cast<RecordedDrawListCommand*>(&command_data[command_offset]);
	if (draw_list_command->header.type == RecordedCommand::TYPE_DRAW_LIST) {
		ResourceTracker** trackers = draw_list_command->trackers();
		for (uint32_t i = 0; i < draw_list_command->trackers_count; i++) {
			if (trackers[i] == p_resource_tracker &&
				draw_list_command->load_ops()[i] == RDD::ATTACHMENT_LOAD_OP_CLEAR) {
				return;
			}
		}
	}

	// Check if the previous command is a draw list.
	uint32_t previous_command_offset = command_data_offsets[p_previous_command_index];
	RecordedDrawListCommand* previous_draw_list_command =
		reinterpret_cast<RecordedDrawListCommand*>(&command_data[previous_command_offset]);
	if (previous_draw_list_command->header.type != RecordedCommand::TYPE_DRAW_LIST) {
		return;
	}

	// Search for the tracker inside the draw list command and modify the store operation
	// accordingly.
	ResourceTracker** trackers = previous_draw_list_command->trackers();
	for (uint32_t i = 0; i < previous_draw_list_command->trackers_count; i++) {
		if (trackers[i] == p_resource_tracker) {
			previous_draw_list_command->store_ops()[i] = RDD::ATTACHMENT_STORE_OP_STORE;
			return;
		}
	}
}

RenderingDeviceGraph::RaytracingListInstruction*
RenderingDeviceGraph::_allocate_raytracing_list_instruction(uint32_t p_instruction_size)
{
	uint32_t raytracing_list_data_offset = raytracing_instruction_list.list.data.size();
	raytracing_instruction_list.list.data.resize(raytracing_list_data_offset + p_instruction_size);
	return reinterpret_cast<RaytracingListInstruction*>(
		&raytracing_instruction_list.list.data[raytracing_list_data_offset]);
}

void RenderingDeviceGraph::_run_raytracing_list_command(RDD::CommandBufferID p_command_buffer,
	const uint8_t* p_instruction_data, uint32_t p_instruction_data_size)
{
	uint32_t instruction_data_cursor = 0;
	while (instruction_data_cursor < p_instruction_data_size) {
		DEV_ASSERT((instruction_data_cursor + sizeof(RaytracingListInstruction)) <=
				   p_instruction_data_size);

		const RaytracingListInstruction* instruction =
			reinterpret_cast<const RaytracingListInstruction*>(
				&p_instruction_data[instruction_data_cursor]);
		switch (instruction->type) {
		case RaytracingListInstruction::TYPE_BIND_PIPELINE: {
			const RaytracingListBindPipelineInstruction* bind_pipeline_instruction =
				reinterpret_cast<const RaytracingListBindPipelineInstruction*>(instruction);
			driver->command_bind_raytracing_pipeline(
				p_command_buffer, bind_pipeline_instruction->pipeline);
			instruction_data_cursor += sizeof(RaytracingListBindPipelineInstruction);
		} break;
		case RaytracingListInstruction::TYPE_BIND_UNIFORM_SET: {
			const RaytracingListBindUniformSetInstruction* bind_uniform_set_instruction =
				reinterpret_cast<const RaytracingListBindUniformSetInstruction*>(instruction);
			driver->command_bind_raytracing_uniform_set(p_command_buffer,
				bind_uniform_set_instruction->uniform_set, bind_uniform_set_instruction->shader,
				bind_uniform_set_instruction->set_index);
			instruction_data_cursor += sizeof(RaytracingListBindUniformSetInstruction);
		} break;
		case RaytracingListInstruction::TYPE_TRACE_RAYS: {
			const RaytracingListTraceRaysInstruction* trace_rays_instruction =
				reinterpret_cast<const RaytracingListTraceRaysInstruction*>(instruction);
			driver->command_trace_rays(p_command_buffer, trace_rays_instruction->raygen_sbt,
				trace_rays_instruction->miss_sbt, trace_rays_instruction->hit_sbt,
				trace_rays_instruction->width, trace_rays_instruction->height,
				trace_rays_instruction->depth);
			instruction_data_cursor += sizeof(RaytracingListTraceRaysInstruction);
		} break;
		case RaytracingListInstruction::TYPE_SET_PUSH_CONSTANT: {
			const RaytracingListSetPushConstantInstruction* set_push_constant_instruction =
				reinterpret_cast<const RaytracingListSetPushConstantInstruction*>(instruction);
			const VectorView push_constant_data_view(
				reinterpret_cast<const uint32_t*>(set_push_constant_instruction->data()),
				set_push_constant_instruction->size / sizeof(uint32_t));
			driver->command_bind_push_constants(p_command_buffer,
				set_push_constant_instruction->shader, 0, push_constant_data_view);
			instruction_data_cursor += sizeof(RaytracingListSetPushConstantInstruction);
			instruction_data_cursor += set_push_constant_instruction->size;
		} break;
		case RaytracingListInstruction::TYPE_UNIFORM_SET_PREPARE_FOR_USE: {
			const RaytracingListUniformSetPrepareForUseInstruction*
				uniform_set_prepare_for_use_instruction =
					reinterpret_cast<const RaytracingListUniformSetPrepareForUseInstruction*>(
						instruction);
			driver->command_uniform_set_prepare_for_use(p_command_buffer,
				uniform_set_prepare_for_use_instruction->uniform_set,
				uniform_set_prepare_for_use_instruction->shader,
				uniform_set_prepare_for_use_instruction->set_index);
			instruction_data_cursor += sizeof(RaytracingListUniformSetPrepareForUseInstruction);
		} break;
		default:
			DEV_ASSERT(false && "Unknown raytracing list instruction type.");
			return;
		}
	}
}

void RenderingDeviceGraph::_run_compute_list_command(RDD::CommandBufferID p_command_buffer,
	const uint8_t* p_instruction_data, uint32_t p_instruction_data_size)
{
	uint32_t instruction_data_cursor = 0;
	while (instruction_data_cursor < p_instruction_data_size) {
		DEV_ASSERT(
			(instruction_data_cursor + sizeof(ComputeListInstruction)) <= p_instruction_data_size);

		const ComputeListInstruction* instruction = reinterpret_cast<const ComputeListInstruction*>(
			&p_instruction_data[instruction_data_cursor]);
		switch (instruction->type) {
		case ComputeListInstruction::TYPE_BIND_PIPELINE: {
			const ComputeListBindPipelineInstruction* bind_pipeline_instruction =
				reinterpret_cast<const ComputeListBindPipelineInstruction*>(instruction);
			driver->command_bind_compute_pipeline(
				p_command_buffer, bind_pipeline_instruction->pipeline);
			instruction_data_cursor += sizeof(ComputeListBindPipelineInstruction);
		} break;
		case ComputeListInstruction::TYPE_BIND_UNIFORM_SETS: {
			const ComputeListBindUniformSetsInstruction* bind_uniform_sets_instruction =
				reinterpret_cast<const ComputeListBindUniformSetsInstruction*>(instruction);
			driver->command_bind_compute_uniform_sets(p_command_buffer,
				VectorView<RDD::UniformSetID>(bind_uniform_sets_instruction->uniform_set_ids(),
					bind_uniform_sets_instruction->set_count),
				bind_uniform_sets_instruction->shader,
				bind_uniform_sets_instruction->first_set_index,
				bind_uniform_sets_instruction->set_count,
				bind_uniform_sets_instruction->dynamic_offsets_mask);
			instruction_data_cursor +=
				sizeof(ComputeListBindUniformSetsInstruction) +
				sizeof(RDD::UniformSetID) * bind_uniform_sets_instruction->set_count;
		} break;
		case ComputeListInstruction::TYPE_DISPATCH: {
			const ComputeListDispatchInstruction* dispatch_instruction =
				reinterpret_cast<const ComputeListDispatchInstruction*>(instruction);
			driver->command_compute_dispatch(p_command_buffer, dispatch_instruction->x_groups,
				dispatch_instruction->y_groups, dispatch_instruction->z_groups);
			instruction_data_cursor += sizeof(ComputeListDispatchInstruction);
		} break;
		case ComputeListInstruction::TYPE_DISPATCH_INDIRECT: {
			const ComputeListDispatchIndirectInstruction* dispatch_indirect_instruction =
				reinterpret_cast<const ComputeListDispatchIndirectInstruction*>(instruction);
			driver->command_compute_dispatch_indirect(p_command_buffer,
				dispatch_indirect_instruction->buffer, dispatch_indirect_instruction->offset);
			instruction_data_cursor += sizeof(ComputeListDispatchIndirectInstruction);
		} break;
		case ComputeListInstruction::TYPE_SET_PUSH_CONSTANT: {
			const ComputeListSetPushConstantInstruction* set_push_constant_instruction =
				reinterpret_cast<const ComputeListSetPushConstantInstruction*>(instruction);
			const VectorView push_constant_data_view(
				reinterpret_cast<const uint32_t*>(set_push_constant_instruction->data()),
				set_push_constant_instruction->size / sizeof(uint32_t));
			driver->command_bind_push_constants(p_command_buffer,
				set_push_constant_instruction->shader, 0, push_constant_data_view);
			instruction_data_cursor += sizeof(ComputeListSetPushConstantInstruction);
			instruction_data_cursor += set_push_constant_instruction->size;
		} break;
		case ComputeListInstruction::TYPE_UNIFORM_SET_PREPARE_FOR_USE: {
			const ComputeListUniformSetPrepareForUseInstruction*
				uniform_set_prepare_for_use_instruction =
					reinterpret_cast<const ComputeListUniformSetPrepareForUseInstruction*>(
						instruction);
			driver->command_uniform_set_prepare_for_use(p_command_buffer,
				uniform_set_prepare_for_use_instruction->uniform_set,
				uniform_set_prepare_for_use_instruction->shader,
				uniform_set_prepare_for_use_instruction->set_index);
			instruction_data_cursor += sizeof(ComputeListUniformSetPrepareForUseInstruction);
		} break;
		default:
			DEV_ASSERT(false && "Unknown compute list instruction type.");
			return;
		}

		instruction_data_cursor = GRAPH_ALIGN(instruction_data_cursor);
	}
}

void RenderingDeviceGraph::_get_draw_list_render_pass_and_framebuffer(
	const RecordedDrawListCommand* p_draw_list_command, RDD::RenderPassID& r_render_pass,
	RDD::FramebufferID& r_framebuffer)
{
	DEV_ASSERT(p_draw_list_command->trackers_count <= 21 &&
			   "Max number of attachments that can be encoded into the key.");

	// Build a unique key from the load and store ops for each attachment.
	const RDD::AttachmentLoadOp* load_ops = p_draw_list_command->load_ops();
	const RDD::AttachmentStoreOp* store_ops = p_draw_list_command->store_ops();
	uint64_t key = 0;
	for (uint32_t i = 0; i < p_draw_list_command->trackers_count; i++) {
		key |= uint64_t(load_ops[i]) << (i * 3);
		key |= uint64_t(store_ops[i]) << (i * 3 + 2);
	}

	// Check the storage map if the render pass and the framebuffer needs to be created.
	FramebufferCache* framebuffer_cache = p_draw_list_command->framebuffer_cache;
	HashMap<uint64_t, FramebufferStorage>::Iterator it = framebuffer_cache->storage_map.find(key);
	if (it == framebuffer_cache->storage_map.end()) {
		FramebufferStorage storage;
		VectorView<RDD::AttachmentLoadOp> load_ops_view(
			load_ops, p_draw_list_command->trackers_count);
		VectorView<RDD::AttachmentStoreOp> store_ops_view(
			store_ops, p_draw_list_command->trackers_count);
		storage.render_pass = render_pass_creation_function(driver, load_ops_view, store_ops_view,
			framebuffer_cache->render_pass_creation_user_data);
		ERR_FAIL_COND(!storage.render_pass);

		storage.framebuffer = driver->framebuffer_create(storage.render_pass,
			framebuffer_cache->textures, framebuffer_cache->width, framebuffer_cache->height);
		ERR_FAIL_COND(!storage.framebuffer);

		it = framebuffer_cache->storage_map.insert(key, storage);
	}

	r_render_pass = it->value.render_pass;
	r_framebuffer = it->value.framebuffer;
}

#if PRINT_DRAW_LIST_STATS
static uint32_t draw_list_total_size = 0;
#endif

void RenderingDeviceGraph::_run_draw_list_command(RDD::CommandBufferID p_command_buffer,
	const uint8_t* p_instruction_data, uint32_t p_instruction_data_size)
{
#if PRINT_DRAW_LIST_STATS
	draw_list_total_size += p_instruction_data_size;
#endif

	uint32_t instruction_data_cursor = 0;
	while (instruction_data_cursor < p_instruction_data_size) {
		DEV_ASSERT(
			(instruction_data_cursor + sizeof(DrawListInstruction)) <= p_instruction_data_size);

		const DrawListInstruction* instruction = reinterpret_cast<const DrawListInstruction*>(
			&p_instruction_data[instruction_data_cursor]);
		switch (instruction->type) {
		case DrawListInstruction::TYPE_BIND_INDEX_BUFFER: {
			const DrawListBindIndexBufferInstruction* bind_index_buffer_instruction =
				reinterpret_cast<const DrawListBindIndexBufferInstruction*>(instruction);
			driver->command_render_bind_index_buffer(p_command_buffer,
				bind_index_buffer_instruction->buffer, bind_index_buffer_instruction->format,
				bind_index_buffer_instruction->offset);
			instruction_data_cursor += sizeof(DrawListBindIndexBufferInstruction);
		} break;
		case DrawListInstruction::TYPE_BIND_PIPELINE: {
			const DrawListBindPipelineInstruction* bind_pipeline_instruction =
				reinterpret_cast<const DrawListBindPipelineInstruction*>(instruction);
			driver->command_bind_render_pipeline(
				p_command_buffer, bind_pipeline_instruction->pipeline);
			instruction_data_cursor += sizeof(DrawListBindPipelineInstruction);
		} break;
		case DrawListInstruction::TYPE_BIND_UNIFORM_SETS: {
			const DrawListBindUniformSetsInstruction* bind_uniform_sets_instruction =
				reinterpret_cast<const DrawListBindUniformSetsInstruction*>(instruction);
			driver->command_bind_render_uniform_sets(p_command_buffer,
				VectorView<RDD::UniformSetID>(bind_uniform_sets_instruction->uniform_set_ids(),
					bind_uniform_sets_instruction->set_count),
				bind_uniform_sets_instruction->shader,
				bind_uniform_sets_instruction->first_set_index,
				bind_uniform_sets_instruction->set_count,
				bind_uniform_sets_instruction->dynamic_offsets_mask);
			instruction_data_cursor +=
				sizeof(DrawListBindUniformSetsInstruction) +
				sizeof(RDD::UniformSetID) * bind_uniform_sets_instruction->set_count;
		} break;
		case DrawListInstruction::TYPE_BIND_VERTEX_BUFFERS: {
			const DrawListBindVertexBuffersInstruction* bind_vertex_buffers_instruction =
				reinterpret_cast<const DrawListBindVertexBuffersInstruction*>(instruction);
			driver->command_render_bind_vertex_buffers(p_command_buffer,
				bind_vertex_buffers_instruction->vertex_buffers_count,
				bind_vertex_buffers_instruction->vertex_buffers(),
				bind_vertex_buffers_instruction->vertex_buffer_offsets(),
				bind_vertex_buffers_instruction->dynamic_offsets_mask);
			instruction_data_cursor += sizeof(DrawListBindVertexBuffersInstruction);
			instruction_data_cursor +=
				sizeof(RDD::BufferID) * bind_vertex_buffers_instruction->vertex_buffers_count;
			instruction_data_cursor +=
				sizeof(uint64_t) * bind_vertex_buffers_instruction->vertex_buffers_count;
		} break;
		case DrawListInstruction::TYPE_CLEAR_ATTACHMENTS: {
			const DrawListClearAttachmentsInstruction* clear_attachments_instruction =
				reinterpret_cast<const DrawListClearAttachmentsInstruction*>(instruction);
			const VectorView attachments_clear_view(
				clear_attachments_instruction->attachments_clear(),
				clear_attachments_instruction->attachments_clear_count);
			const VectorView attachments_clear_rect_view(
				clear_attachments_instruction->attachments_clear_rect(),
				clear_attachments_instruction->attachments_clear_rect_count);
			driver->command_render_clear_attachments(
				p_command_buffer, attachments_clear_view, attachments_clear_rect_view);
			instruction_data_cursor += sizeof(DrawListClearAttachmentsInstruction);
			instruction_data_cursor += sizeof(RDD::AttachmentClear) *
									   clear_attachments_instruction->attachments_clear_count;
			instruction_data_cursor +=
				sizeof(Rect2i) * clear_attachments_instruction->attachments_clear_rect_count;
		} break;
		case DrawListInstruction::TYPE_DRAW: {
			const DrawListDrawInstruction* draw_instruction =
				reinterpret_cast<const DrawListDrawInstruction*>(instruction);
			driver->command_render_draw(p_command_buffer, draw_instruction->vertex_count,
				draw_instruction->instance_count, 0, 0);
			instruction_data_cursor += sizeof(DrawListDrawInstruction);
		} break;
		case DrawListInstruction::TYPE_DRAW_INDEXED: {
			const DrawListDrawIndexedInstruction* draw_indexed_instruction =
				reinterpret_cast<const DrawListDrawIndexedInstruction*>(instruction);
			driver->command_render_draw_indexed(p_command_buffer,
				draw_indexed_instruction->index_count, draw_indexed_instruction->instance_count,
				draw_indexed_instruction->first_index, 0, 0);
			instruction_data_cursor += sizeof(DrawListDrawIndexedInstruction);
		} break;
		case DrawListInstruction::TYPE_DRAW_INDIRECT: {
			const DrawListDrawIndirectInstruction* draw_indirect_instruction =
				reinterpret_cast<const DrawListDrawIndirectInstruction*>(instruction);
			driver->command_render_draw_indirect(p_command_buffer,
				draw_indirect_instruction->buffer, draw_indirect_instruction->offset,
				draw_indirect_instruction->draw_count, draw_indirect_instruction->stride);
			instruction_data_cursor += sizeof(DrawListDrawIndirectInstruction);
		} break;
		case DrawListInstruction::TYPE_DRAW_INDEXED_INDIRECT: {
			const DrawListDrawIndexedIndirectInstruction* draw_indexed_indirect_instruction =
				reinterpret_cast<const DrawListDrawIndexedIndirectInstruction*>(instruction);
			driver->command_render_draw_indexed_indirect(p_command_buffer,
				draw_indexed_indirect_instruction->buffer,
				draw_indexed_indirect_instruction->offset,
				draw_indexed_indirect_instruction->draw_count,
				draw_indexed_indirect_instruction->stride);
			instruction_data_cursor += sizeof(DrawListDrawIndexedIndirectInstruction);
		} break;
		case DrawListInstruction::TYPE_EXECUTE_COMMANDS: {
			const DrawListExecuteCommandsInstruction* execute_commands_instruction =
				reinterpret_cast<const DrawListExecuteCommandsInstruction*>(instruction);
			driver->command_buffer_execute_secondary(
				p_command_buffer, execute_commands_instruction->command_buffer);
			instruction_data_cursor += sizeof(DrawListExecuteCommandsInstruction);
		} break;
		case DrawListInstruction::TYPE_NEXT_SUBPASS: {
			const DrawListNextSubpassInstruction* next_subpass_instruction =
				reinterpret_cast<const DrawListNextSubpassInstruction*>(instruction);
			driver->command_next_render_subpass(
				p_command_buffer, next_subpass_instruction->command_buffer_type);
			instruction_data_cursor += sizeof(DrawListNextSubpassInstruction);
		} break;
		case DrawListInstruction::TYPE_SET_BLEND_CONSTANTS: {
			const DrawListSetBlendConstantsInstruction* set_blend_constants_instruction =
				reinterpret_cast<const DrawListSetBlendConstantsInstruction*>(instruction);
			driver->command_render_set_blend_constants(
				p_command_buffer, set_blend_constants_instruction->color);
			instruction_data_cursor += sizeof(DrawListSetBlendConstantsInstruction);
		} break;
		case DrawListInstruction::TYPE_SET_LINE_WIDTH: {
			const DrawListSetLineWidthInstruction* set_line_width_instruction =
				reinterpret_cast<const DrawListSetLineWidthInstruction*>(instruction);
			driver->command_render_set_line_width(
				p_command_buffer, set_line_width_instruction->width);
			instruction_data_cursor += sizeof(DrawListSetLineWidthInstruction);
		} break;
		case DrawListInstruction::TYPE_SET_PUSH_CONSTANT: {
			const DrawListSetPushConstantInstruction* set_push_constant_instruction =
				reinterpret_cast<const DrawListSetPushConstantInstruction*>(instruction);
			const VectorView push_constant_data_view(
				reinterpret_cast<const uint32_t*>(set_push_constant_instruction->data()),
				set_push_constant_instruction->size / sizeof(uint32_t));
			driver->command_bind_push_constants(p_command_buffer,
				set_push_constant_instruction->shader, 0, push_constant_data_view);
			instruction_data_cursor += sizeof(DrawListSetPushConstantInstruction);
			instruction_data_cursor += set_push_constant_instruction->size;
		} break;
		case DrawListInstruction::TYPE_SET_SCISSOR: {
			const DrawListSetScissorInstruction* set_scissor_instruction =
				reinterpret_cast<const DrawListSetScissorInstruction*>(instruction);
			driver->command_render_set_scissor(p_command_buffer, set_scissor_instruction->rect);
			instruction_data_cursor += sizeof(DrawListSetScissorInstruction);
		} break;
		case DrawListInstruction::TYPE_SET_VIEWPORT: {
			const DrawListSetViewportInstruction* set_viewport_instruction =
				reinterpret_cast<const DrawListSetViewportInstruction*>(instruction);
			driver->command_render_set_viewport(p_command_buffer, set_viewport_instruction->rect);
			instruction_data_cursor += sizeof(DrawListSetViewportInstruction);
		} break;
		case DrawListInstruction::TYPE_UNIFORM_SET_PREPARE_FOR_USE: {
			const DrawListUniformSetPrepareForUseInstruction*
				uniform_set_prepare_for_use_instruction =
					reinterpret_cast<const DrawListUniformSetPrepareForUseInstruction*>(
						instruction);
			driver->command_uniform_set_prepare_for_use(p_command_buffer,
				uniform_set_prepare_for_use_instruction->uniform_set,
				uniform_set_prepare_for_use_instruction->shader,
				uniform_set_prepare_for_use_instruction->set_index);
			instruction_data_cursor += sizeof(DrawListUniformSetPrepareForUseInstruction);
		} break;
		default:
			DEV_ASSERT(false && "Unknown draw list instruction type.");
			return;
		}

		instruction_data_cursor = GRAPH_ALIGN(instruction_data_cursor);
	}
}

void RenderingDeviceGraph::_add_draw_list_begin(FramebufferCache* p_framebuffer_cache,
	RDD::RenderPassID p_render_pass, RDD::FramebufferID p_framebuffer, Rect2i p_region,
	VectorView<AttachmentOperation> p_attachment_operations,
	VectorView<RDD::RenderPassClearValue> p_attachment_clear_values, uint32_t p_stages,
	uint32_t p_breadcrumb, bool p_split_cmd_buffer)
{
	DEV_ASSERT(p_attachment_operations.size() == p_attachment_clear_values.size());

	draw_instruction_list.list.clear();
	draw_instruction_list.list.index++;
	draw_instruction_list.framebuffer_cache = p_framebuffer_cache;
	draw_instruction_list.render_pass = p_render_pass;
	draw_instruction_list.framebuffer = p_framebuffer;
	draw_instruction_list.region = p_region;
	draw_instruction_list.list.stages = p_stages;
	draw_instruction_list.attachment_operations.resize(p_attachment_operations.size());
	draw_instruction_list.attachment_clear_values.resize(p_attachment_clear_values.size());

	for (uint32_t i = 0; i < p_attachment_operations.size(); i++) {
		draw_instruction_list.attachment_operations[i] = p_attachment_operations[i];
		draw_instruction_list.attachment_clear_values[i] = p_attachment_clear_values[i];
	}

	draw_instruction_list.split_cmd_buffer = p_split_cmd_buffer;

#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
	draw_instruction_list.breadcrumb = p_breadcrumb;
#endif
}

void RenderingDeviceGraph::_run_secondary_command_buffer_task(
	const SecondaryCommandBuffer* p_secondary)
{
	driver->command_buffer_begin_secondary(
		p_secondary->command_buffer, p_secondary->render_pass, 0, p_secondary->framebuffer);
	_run_draw_list_command(p_secondary->command_buffer, p_secondary->instruction_data.ptr(),
		p_secondary->instruction_data.size());
	driver->command_buffer_end(p_secondary->command_buffer);
}

void RenderingDeviceGraph::_run_render_commands(int32_t p_level,
	const RecordedCommandSort* p_sorted_commands, uint32_t p_sorted_commands_count,
	RDD::CommandBufferID& r_command_buffer, CommandBufferPool& r_command_buffer_pool,
	int32_t& r_current_label_index, int32_t& r_current_label_level)
{
	for (uint32_t i = 0; i < p_sorted_commands_count; i++) {
		const uint32_t command_index = p_sorted_commands[i].index;
		const uint32_t command_data_offset = command_data_offsets[command_index];
		const RecordedCommand* command =
			reinterpret_cast<const RecordedCommand*>(&command_data[command_data_offset]);
		_run_label_command_change(r_command_buffer, command->label_index, p_level, false, true,
			&p_sorted_commands[i], p_sorted_commands_count - i, r_current_label_index,
			r_current_label_level);

		switch (command->type) {
		case RecordedCommand::TYPE_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_BUILD: {
			const RecordedBottomLevelAccelerationStructureBuildCommand* blas_build_command =
				reinterpret_cast<const RecordedBottomLevelAccelerationStructureBuildCommand*>(
					command);
			driver->command_build_blas(r_command_buffer, blas_build_command->acceleration_structure,
				blas_build_command->scratch_buffer);
		} break;
		case RecordedCommand::TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE_BUILD: {
			const RecordedTopLevelAccelerationStructureBuildCommand* tlas_build_command =
				reinterpret_cast<const RecordedTopLevelAccelerationStructureBuildCommand*>(command);
			driver->command_build_tlas(r_command_buffer, tlas_build_command->acceleration_structure,
				tlas_build_command->scratch_buffer, tlas_build_command->instance_buffer,
				tlas_build_command->instance_offset, tlas_build_command->instance_count);
		} break;
		case RecordedCommand::TYPE_BUFFER_CLEAR: {
			const RecordedBufferClearCommand* buffer_clear_command =
				reinterpret_cast<const RecordedBufferClearCommand*>(command);
			driver->command_clear_buffer(r_command_buffer, buffer_clear_command->buffer,
				buffer_clear_command->offset, buffer_clear_command->size);
		} break;
		case RecordedCommand::TYPE_BUFFER_COPY: {
			const RecordedBufferCopyCommand* buffer_copy_command =
				reinterpret_cast<const RecordedBufferCopyCommand*>(command);
			driver->command_copy_buffer(r_command_buffer, buffer_copy_command->source,
				buffer_copy_command->destination, buffer_copy_command->region);
		} break;
		case RecordedCommand::TYPE_BUFFER_GET_DATA: {
			const RecordedBufferGetDataCommand* buffer_get_data_command =
				reinterpret_cast<const RecordedBufferGetDataCommand*>(command);
			driver->command_copy_buffer(r_command_buffer, buffer_get_data_command->source,
				buffer_get_data_command->destination, buffer_get_data_command->region);
		} break;
		case RecordedCommand::TYPE_BUFFER_UPDATE: {
			const RecordedBufferUpdateCommand* buffer_update_command =
				reinterpret_cast<const RecordedBufferUpdateCommand*>(command);
			const RecordedBufferCopy* command_buffer_copies =
				buffer_update_command->buffer_copies();
			for (uint32_t j = 0; j < buffer_update_command->buffer_copies_count; j++) {
				driver->command_copy_buffer(r_command_buffer, command_buffer_copies[j].source,
					buffer_update_command->destination, command_buffer_copies[j].region);
			}
		} break;
		case RecordedCommand::TYPE_DRIVER_CALLBACK: {
			const RecordedDriverCallbackCommand* driver_callback_command =
				reinterpret_cast<const RecordedDriverCallbackCommand*>(command);
			driver_callback_command->callback(
				driver, r_command_buffer, driver_callback_command->userdata);
		} break;
		case RecordedCommand::TYPE_RAYTRACING_LIST: {
			const RecordedRaytracingListCommand* raytracing_list_command =
				reinterpret_cast<const RecordedRaytracingListCommand*>(command);
			_run_raytracing_list_command(r_command_buffer,
				raytracing_list_command->instruction_data(),
				raytracing_list_command->instruction_data_size);
		} break;
		case RecordedCommand::TYPE_COMPUTE_LIST: {
			if (driver_workarounds.avoid_compute_after_draw && workarounds_state.draw_list_found) {
				// Avoid compute after draw workaround. Refer to the comment that enables this in
				// the Vulkan driver for more information.
				workarounds_state.draw_list_found = false;

				// Create or reuse a command buffer and finish recording the current one.
				driver->command_buffer_end(r_command_buffer);

				while (r_command_buffer_pool.buffers_used >= r_command_buffer_pool.buffers.size()) {
					RDD::CommandBufferID command_buffer =
						driver->command_buffer_create(r_command_buffer_pool.pool);
					RDD::SemaphoreID command_semaphore = driver->semaphore_create();
					r_command_buffer_pool.buffers.push_back(command_buffer);
					r_command_buffer_pool.semaphores.push_back(command_semaphore);
				}

				// Start recording on the next usable command buffer from the pool.
				uint32_t command_buffer_index = r_command_buffer_pool.buffers_used++;
				r_command_buffer = r_command_buffer_pool.buffers[command_buffer_index];
				driver->command_buffer_begin(r_command_buffer);
			}

			const RecordedComputeListCommand* compute_list_command =
				reinterpret_cast<const RecordedComputeListCommand*>(command);
			_run_compute_list_command(r_command_buffer, compute_list_command->instruction_data(),
				compute_list_command->instruction_data_size);
		} break;
		case RecordedCommand::TYPE_DRAW_LIST: {
			if (driver_workarounds.avoid_compute_after_draw) {
				// Indicate that a draw list was encountered for the workaround.
				workarounds_state.draw_list_found = true;
			}

			const RecordedDrawListCommand* draw_list_command =
				reinterpret_cast<const RecordedDrawListCommand*>(command);

			if (draw_list_command->split_cmd_buffer) {
				// Create or reuse a command buffer and finish recording the current one.
				driver->command_buffer_end(r_command_buffer);

				while (r_command_buffer_pool.buffers_used >= r_command_buffer_pool.buffers.size()) {
					RDD::CommandBufferID command_buffer =
						driver->command_buffer_create(r_command_buffer_pool.pool);
					RDD::SemaphoreID command_semaphore = driver->semaphore_create();
					r_command_buffer_pool.buffers.push_back(command_buffer);
					r_command_buffer_pool.semaphores.push_back(command_semaphore);
				}

				// Start recording on the next usable command buffer from the pool.
				uint32_t command_buffer_index = r_command_buffer_pool.buffers_used++;
				r_command_buffer = r_command_buffer_pool.buffers[command_buffer_index];
				driver->command_buffer_begin(r_command_buffer);
			}

			const VectorView clear_values(
				draw_list_command->clear_values(), draw_list_command->clear_values_count);
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
			driver->command_insert_breadcrumb(r_command_buffer, draw_list_command->breadcrumb);
#endif
			RDD::RenderPassID render_pass;
			RDD::FramebufferID framebuffer;
			if (draw_list_command->framebuffer_cache != nullptr) {
				_get_draw_list_render_pass_and_framebuffer(
					draw_list_command, render_pass, framebuffer);
			}
			else {
				render_pass = draw_list_command->render_pass;
				framebuffer = draw_list_command->framebuffer;
			}

			if (framebuffer && render_pass) {
				driver->command_begin_render_pass(r_command_buffer, render_pass, framebuffer,
					draw_list_command->command_buffer_type, draw_list_command->region,
					clear_values);
				_run_draw_list_command(r_command_buffer, draw_list_command->instruction_data(),
					draw_list_command->instruction_data_size);
				driver->command_end_render_pass(r_command_buffer);
			}
		} break;
		case RecordedCommand::TYPE_TEXTURE_CLEAR_COLOR: {
			const RecordedTextureClearColorCommand* texture_clear_color_command =
				reinterpret_cast<const RecordedTextureClearColorCommand*>(command);
			driver->command_clear_color_texture(r_command_buffer,
				texture_clear_color_command->texture, RDD::TEXTURE_LAYOUT_COPY_DST_OPTIMAL,
				texture_clear_color_command->color, texture_clear_color_command->range);
		} break;
		case RecordedCommand::TYPE_TEXTURE_CLEAR_DEPTH_STENCIL: {
			const RecordedTextureClearDepthStencilCommand* texture_clear_depth_stencil_command =
				reinterpret_cast<const RecordedTextureClearDepthStencilCommand*>(command);
			driver->command_clear_depth_stencil_texture(r_command_buffer,
				texture_clear_depth_stencil_command->texture, RDD::TEXTURE_LAYOUT_COPY_DST_OPTIMAL,
				texture_clear_depth_stencil_command->depth,
				texture_clear_depth_stencil_command->stencil,
				texture_clear_depth_stencil_command->range);
		} break;
		case RecordedCommand::TYPE_TEXTURE_COPY: {
			const RecordedTextureCopyCommand* texture_copy_command =
				reinterpret_cast<const RecordedTextureCopyCommand*>(command);
			const VectorView<RDD::TextureCopyRegion> command_texture_copy_regions_view(
				texture_copy_command->texture_copy_regions(),
				texture_copy_command->texture_copy_regions_count);
			driver->command_copy_texture(r_command_buffer, texture_copy_command->from_texture,
				RDD::TEXTURE_LAYOUT_COPY_SRC_OPTIMAL, texture_copy_command->to_texture,
				RDD::TEXTURE_LAYOUT_COPY_DST_OPTIMAL, command_texture_copy_regions_view);
		} break;
		case RecordedCommand::TYPE_TEXTURE_GET_DATA: {
			const RecordedTextureGetDataCommand* texture_get_data_command =
				reinterpret_cast<const RecordedTextureGetDataCommand*>(command);
			const VectorView<RDD::BufferTextureCopyRegion> command_buffer_texture_copy_regions_view(
				texture_get_data_command->buffer_texture_copy_regions(),
				texture_get_data_command->buffer_texture_copy_regions_count);
			driver->command_copy_texture_to_buffer(r_command_buffer,
				texture_get_data_command->from_texture, RDD::TEXTURE_LAYOUT_COPY_SRC_OPTIMAL,
				texture_get_data_command->to_buffer, command_buffer_texture_copy_regions_view);
		} break;
		case RecordedCommand::TYPE_TEXTURE_RESOLVE: {
			const RecordedTextureResolveCommand* texture_resolve_command =
				reinterpret_cast<const RecordedTextureResolveCommand*>(command);
			driver->command_resolve_texture(r_command_buffer, texture_resolve_command->from_texture,
				RDD::TEXTURE_LAYOUT_RESOLVE_SRC_OPTIMAL, texture_resolve_command->src_layer,
				texture_resolve_command->src_mipmap, texture_resolve_command->to_texture,
				RDD::TEXTURE_LAYOUT_RESOLVE_DST_OPTIMAL, texture_resolve_command->dst_layer,
				texture_resolve_command->dst_mipmap);
		} break;
		case RecordedCommand::TYPE_TEXTURE_UPDATE: {
			const RecordedTextureUpdateCommand* texture_update_command =
				reinterpret_cast<const RecordedTextureUpdateCommand*>(command);
			const RecordedBufferToTextureCopy* command_buffer_to_texture_copies =
				texture_update_command->buffer_to_texture_copies();
			for (uint32_t j = 0; j < texture_update_command->buffer_to_texture_copies_count; j++) {
				driver->command_copy_buffer_to_texture(r_command_buffer,
					command_buffer_to_texture_copies[j].from_buffer,
					texture_update_command->to_texture, RDD::TEXTURE_LAYOUT_COPY_DST_OPTIMAL,
					command_buffer_to_texture_copies[j].region);
			}
		} break;
		case RecordedCommand::TYPE_CAPTURE_TIMESTAMP: {
			const RecordedCaptureTimestampCommand* texture_capture_timestamp_command =
				reinterpret_cast<const RecordedCaptureTimestampCommand*>(command);
			driver->command_timestamp_write(r_command_buffer,
				texture_capture_timestamp_command->pool, texture_capture_timestamp_command->index);
		} break;
		default: {
			DEV_ASSERT(false && "Unknown recorded command type.");
			return;
		}
		}
	}
}

void RenderingDeviceGraph::_run_label_command_change(RDD::CommandBufferID p_command_buffer,
	int32_t p_new_label_index, int32_t p_new_level, bool p_ignore_previous_value,
	bool p_use_label_for_empty, const RecordedCommandSort* p_sorted_commands,
	uint32_t p_sorted_commands_count, int32_t& r_current_label_index,
	int32_t& r_current_label_level)
{
	if (command_label_count == 0) {
		// Ignore any label operations if no labels were pushed.
		return;
	}

	if (p_ignore_previous_value || p_new_label_index != r_current_label_index ||
		p_new_level != r_current_label_level) {
		if (!p_ignore_previous_value &&
			(p_use_label_for_empty || r_current_label_index >= 0 || r_current_label_level >= 0)) {
			// End the current label.
			driver->command_end_label(p_command_buffer);
		}

		String label_name;
		Color label_color;
		if (p_new_label_index >= 0) {
			const char* label_chars =
				&command_label_chars[command_label_offsets[p_new_label_index]];
			label_name.append_utf8(label_chars);
			label_color = command_label_colors[p_new_label_index];
		}
		else if (p_use_label_for_empty) {
			label_name = "Command Graph";
			label_color = Color(1, 1, 1, 1);
		}
		else {
			return;
		}

		// Add the level to the name.
		label_name += " (L" + itos(p_new_level) + ")";

		if (p_sorted_commands != nullptr && p_sorted_commands_count > 0) {
			// Analyze the commands in the level that have the same label to detect what type of
			// operations are performed.
			bool copy_commands = false;
			bool compute_commands = false;
			bool draw_commands = false;
			bool custom_commands = false;
			for (uint32_t i = 0; i < p_sorted_commands_count; i++) {
				const uint32_t command_index = p_sorted_commands[i].index;
				const uint32_t command_data_offset = command_data_offsets[command_index];
				const RecordedCommand* command =
					reinterpret_cast<RecordedCommand*>(&command_data[command_data_offset]);
				if (command->label_index != p_new_label_index) {
					break;
				}

				switch (command->type) {
				case RecordedCommand::TYPE_BUFFER_CLEAR:
				case RecordedCommand::TYPE_BUFFER_COPY:
				case RecordedCommand::TYPE_BUFFER_GET_DATA:
				case RecordedCommand::TYPE_BUFFER_UPDATE:
				case RecordedCommand::TYPE_TEXTURE_CLEAR_COLOR:
				case RecordedCommand::TYPE_TEXTURE_CLEAR_DEPTH_STENCIL:
				case RecordedCommand::TYPE_TEXTURE_COPY:
				case RecordedCommand::TYPE_TEXTURE_GET_DATA:
				case RecordedCommand::TYPE_TEXTURE_RESOLVE:
				case RecordedCommand::TYPE_TEXTURE_UPDATE: {
					copy_commands = true;
				} break;
				case RecordedCommand::TYPE_COMPUTE_LIST: {
					compute_commands = true;
				} break;
				case RecordedCommand::TYPE_DRAW_LIST: {
					draw_commands = true;
				} break;
				case RecordedCommand::TYPE_DRIVER_CALLBACK: {
					custom_commands = true;
				} break;
				default: {
					// Ignore command.
				} break;
				}

				if (copy_commands && compute_commands && draw_commands && custom_commands) {
					// There's no more command types to find.
					break;
				}
			}

			if (copy_commands || compute_commands || draw_commands || custom_commands) {
				// Add the operations to the name.
				bool plus_after_copy =
					copy_commands && (compute_commands || draw_commands || custom_commands);
				bool plus_after_compute = compute_commands && (draw_commands || custom_commands);
				bool plus_after_draw = draw_commands && custom_commands;
				label_name += " (";
				label_name += copy_commands ? "Copy" : "";
				label_name += plus_after_copy ? "+" : "";
				label_name += compute_commands ? "Compute" : "";
				label_name += plus_after_compute ? "+" : "";
				label_name += draw_commands ? "Draw" : "";
				label_name += plus_after_draw ? "+" : "";
				label_name += custom_commands ? "Custom" : "";
				label_name += ")";
			}
		}

		// Start the new label.
		CharString label_name_utf8 = label_name.utf8();
		driver->command_begin_label(p_command_buffer, label_name_utf8.get_data(), label_color);

		r_current_label_index = p_new_label_index;
		r_current_label_level = p_new_level;
	}
}

void RenderingDeviceGraph::_boost_priority_for_render_commands(
	RecordedCommandSort* p_sorted_commands, uint32_t p_sorted_commands_count,
	uint32_t& r_boosted_priority)
{
	if (p_sorted_commands_count == 0) {
		return;
	}

	const uint32_t boosted_priority_value = 0;
	if (r_boosted_priority > 0) {
		bool perform_sort = false;
		for (uint32_t j = 0; j < p_sorted_commands_count; j++) {
			if (p_sorted_commands[j].priority == r_boosted_priority) {
				p_sorted_commands[j].priority = boosted_priority_value;
				perform_sort = true;
			}
		}

		if (perform_sort) {
			SortArray<RecordedCommandSort> command_sorter;
			command_sorter.sort(p_sorted_commands, p_sorted_commands_count);
		}
	}

	if (p_sorted_commands[p_sorted_commands_count - 1].priority != boosted_priority_value) {
		r_boosted_priority = p_sorted_commands[p_sorted_commands_count - 1].priority;
	}
}

void RenderingDeviceGraph::finalize()
{
	if (!frames.is_empty()) {
		_wait_for_secondary_command_buffer_tasks();
	}

	for (Frame& f : frames) {
		for (SecondaryCommandBuffer& secondary : f.secondary_command_buffers) {
			if (secondary.command_pool.id != 0) {
				driver->command_pool_free(secondary.command_pool);
			}
		}
	}

	frames.clear();
}

void RenderingDeviceGraph::begin()
{
	command_data.clear();
	command_data_offsets.clear();
	command_normalization_barriers.clear();
	command_transition_barriers.clear();
	command_buffer_barriers.clear();
	command_acceleration_structure_barriers.clear();
	command_label_chars.clear();
	command_label_colors.clear();
	command_label_offsets.clear();
	command_list_nodes.clear();
	read_slice_list_nodes.clear();
	write_slice_list_nodes.clear();
	command_count = 0;
	command_label_count = 0;
	command_timestamp_index = -1;
	command_synchronization_index = -1;
	command_synchronization_pending = false;
	command_label_index = -1;
	frames[frame].secondary_command_buffers_used = 0;
	draw_instruction_list.list.index = 0;
	compute_instruction_list.list.index = 0;
	raytracing_instruction_list.list.index = 0;
	tracking_frame++;

#ifdef DEV_ENABLED
	write_dependency_counters.clear();
#endif
}

void RenderingDeviceGraph::add_blas_build(RDD::AccelerationStructureID p_acceleration_structure,
	RDD::BufferID p_scratch_buffer, ResourceTracker* p_dst_tracker,
	VectorView<ResourceTracker*> p_src_trackers)
{
	int32_t command_index;
	RecordedBottomLevelAccelerationStructureBuildCommand* command =
		reinterpret_cast<RecordedBottomLevelAccelerationStructureBuildCommand*>(_allocate_command(
			sizeof(RecordedBottomLevelAccelerationStructureBuildCommand), command_index));
	command->header.type = RecordedCommand::TYPE_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_BUILD;
	command->header.self_stages = RDD::PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT;
	command->acceleration_structure = p_acceleration_structure;
	command->scratch_buffer = p_scratch_buffer;

	thread_local LocalVector<ResourceTracker*> trackers;
	thread_local LocalVector<ResourceUsage> usages;

	// Sources and destination.
	uint32_t resource_count = p_src_trackers.size() + 1;
	trackers.resize(resource_count);
	usages.resize(resource_count);

	for (uint32_t i = 0; i < p_src_trackers.size(); ++i) {
		trackers[i] = p_src_trackers[i];
		usages[i] = RESOURCE_USAGE_STORAGE_BUFFER_READ;
	}

	trackers[resource_count - 1] = p_dst_tracker;
	usages[resource_count - 1] = RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE;

	_add_command_to_graph(
		trackers.ptr(), usages.ptr(), usages.size(), command_index, &command->header);
}

void RenderingDeviceGraph::add_tlas_build(RDD::AccelerationStructureID p_acceleration_structure,
	RDD::BufferID p_scratch_buffer, RDD::BufferID p_instance_buffer, uint32_t p_instance_offset,
	uint32_t p_instance_count, ResourceTracker* p_dst_tracker,
	VectorView<ResourceTracker*> p_src_trackers)
{
	int32_t command_index;
	RecordedTopLevelAccelerationStructureBuildCommand* command =
		reinterpret_cast<RecordedTopLevelAccelerationStructureBuildCommand*>(_allocate_command(
			sizeof(RecordedTopLevelAccelerationStructureBuildCommand), command_index));
	command->header.type = RecordedCommand::TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE_BUILD;
	command->header.self_stages = RDD::PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT;
	command->acceleration_structure = p_acceleration_structure;
	command->scratch_buffer = p_scratch_buffer;
	command->instance_buffer = p_instance_buffer;
	command->instance_offset = p_instance_offset;
	command->instance_count = p_instance_count;

	thread_local LocalVector<ResourceTracker*> trackers;
	thread_local LocalVector<ResourceUsage> usages;

	// Sources and destination.
	uint32_t resource_count = p_src_trackers.size() + 1;
	trackers.resize(resource_count);
	usages.resize(resource_count);

	for (uint32_t i = 0; i < p_src_trackers.size(); ++i) {
		trackers[i] = p_src_trackers[i];
		usages[i] = RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ;
	}

	trackers[resource_count - 1] = p_dst_tracker;
	usages[resource_count - 1] = RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE;

	_add_command_to_graph(
		trackers.ptr(), usages.ptr(), usages.size(), command_index, &command->header);
}

void RenderingDeviceGraph::add_buffer_clear(
	RDD::BufferID p_dst, ResourceTracker* p_dst_tracker, uint32_t p_offset, uint32_t p_size)
{
	DEV_ASSERT(p_dst_tracker != nullptr);

	int32_t command_index;
	RecordedBufferClearCommand* command = reinterpret_cast<RecordedBufferClearCommand*>(
		_allocate_command(sizeof(RecordedBufferClearCommand), command_index));
	command->header.type = RecordedCommand::TYPE_BUFFER_CLEAR;
	command->buffer = p_dst;
	command->offset = p_offset;
	command->size = p_size;

	ResourceUsage usage;
	if (driver_clears_with_copy_engine) {
		command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
		usage = RESOURCE_USAGE_COPY_TO;
	}
	else {
		// If the driver is uncapable of using the copy engine for clearing the buffer (e.g. D3D12),
		// we must transition it to storage buffer read/write usage.
		command->header.self_stages = RDD::PIPELINE_STAGE_CLEAR_STORAGE_BIT;
		usage = RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE;
	}

	_add_command_to_graph(&p_dst_tracker, &usage, 1, command_index, &command->header);
}

void RenderingDeviceGraph::add_buffer_copy(RDD::BufferID p_src, ResourceTracker* p_src_tracker,
	RDD::BufferID p_dst, ResourceTracker* p_dst_tracker, RDD::BufferCopyRegion p_region)
{
	// Source tracker is allowed to be null as it could be a read-only buffer.
	DEV_ASSERT(p_dst_tracker != nullptr);

	int32_t command_index;
	RecordedBufferCopyCommand* command = reinterpret_cast<RecordedBufferCopyCommand*>(
		_allocate_command(sizeof(RecordedBufferCopyCommand), command_index));
	command->header.type = RecordedCommand::TYPE_BUFFER_COPY;
	command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
	command->source = p_src;
	command->destination = p_dst;
	command->region = p_region;

	ResourceTracker* trackers[2] = {p_dst_tracker, p_src_tracker};
	ResourceUsage usages[2] = {RESOURCE_USAGE_COPY_TO, RESOURCE_USAGE_COPY_FROM};
	_add_command_to_graph(
		trackers, usages, p_src_tracker != nullptr ? 2 : 1, command_index, &command->header);
}

void RenderingDeviceGraph::add_buffer_get_data(RDD::BufferID p_src, ResourceTracker* p_src_tracker,
	RDD::BufferID p_dst, RDD::BufferCopyRegion p_region)
{
	// Source tracker is allowed to be null as it could be a read-only buffer.
	int32_t command_index;
	RecordedBufferGetDataCommand* command = reinterpret_cast<RecordedBufferGetDataCommand*>(
		_allocate_command(sizeof(RecordedBufferGetDataCommand), command_index));
	command->header.type = RecordedCommand::TYPE_BUFFER_GET_DATA;
	command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
	command->source = p_src;
	command->destination = p_dst;
	command->region = p_region;

	if (p_src_tracker != nullptr) {
		ResourceUsage usage = RESOURCE_USAGE_COPY_FROM;
		_add_command_to_graph(&p_src_tracker, &usage, 1, command_index, &command->header);
	}
	else {
		_add_command_to_graph(nullptr, nullptr, 0, command_index, &command->header);
	}
}

void RenderingDeviceGraph::add_buffer_update(RDD::BufferID p_dst, ResourceTracker* p_dst_tracker,
	VectorView<RecordedBufferCopy> p_buffer_copies)
{
	DEV_ASSERT(p_dst_tracker != nullptr);

	size_t buffer_copies_size = p_buffer_copies.size() * sizeof(RecordedBufferCopy);
	uint64_t command_size = sizeof(RecordedBufferUpdateCommand) + buffer_copies_size;
	int32_t command_index;
	RecordedBufferUpdateCommand* command = reinterpret_cast<RecordedBufferUpdateCommand*>(
		_allocate_command(command_size, command_index));
	command->header.type = RecordedCommand::TYPE_BUFFER_UPDATE;
	command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
	command->destination = p_dst;
	command->buffer_copies_count = p_buffer_copies.size();

	RecordedBufferCopy* buffer_copies = command->buffer_copies();
	for (uint32_t i = 0; i < command->buffer_copies_count; i++) {
		buffer_copies[i] = p_buffer_copies[i];
	}

	ResourceUsage buffer_usage = RESOURCE_USAGE_COPY_TO;
	_add_command_to_graph(&p_dst_tracker, &buffer_usage, 1, command_index, &command->header);
}

void RenderingDeviceGraph::add_driver_callback(RDD::DriverCallback p_callback, void* p_userdata,
	VectorView<ResourceTracker*> p_trackers,
	VectorView<RenderingDeviceGraph::ResourceUsage> p_usages)
{
	DEV_ASSERT(p_trackers.size() == p_usages.size());

	int32_t command_index;
	RecordedDriverCallbackCommand* command = reinterpret_cast<RecordedDriverCallbackCommand*>(
		_allocate_command(sizeof(RecordedDriverCallbackCommand), command_index));
	command->header.type = RecordedCommand::TYPE_DRIVER_CALLBACK;
	command->callback = p_callback;
	command->userdata = p_userdata;
	_add_command_to_graph((ResourceTracker**)p_trackers.ptr(), (ResourceUsage*)p_usages.ptr(),
		p_trackers.size(), command_index, &command->header);
}

void RenderingDeviceGraph::add_raytracing_list_begin()
{
	raytracing_instruction_list.list.clear();
	raytracing_instruction_list.list.index++;
}

void RenderingDeviceGraph::add_raytracing_list_bind_pipeline(RDD::RaytracingPipelineID p_pipeline)
{
	RaytracingListBindPipelineInstruction* instruction =
		reinterpret_cast<RaytracingListBindPipelineInstruction*>(
			_allocate_raytracing_list_instruction(sizeof(RaytracingListBindPipelineInstruction)));
	instruction->header.type = RaytracingListInstruction::TYPE_BIND_PIPELINE;
	instruction->pipeline = p_pipeline;
	raytracing_instruction_list.list.stages |= RDD::PIPELINE_STAGE_RAY_TRACING_SHADER_BIT;
}

void RenderingDeviceGraph::add_raytracing_list_bind_uniform_set(
	RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index)
{
	RaytracingListBindUniformSetInstruction* instruction =
		reinterpret_cast<RaytracingListBindUniformSetInstruction*>(
			_allocate_raytracing_list_instruction(sizeof(RaytracingListBindUniformSetInstruction)));
	instruction->header.type = RaytracingListInstruction::TYPE_BIND_UNIFORM_SET;
	instruction->shader = p_shader;
	instruction->uniform_set = p_uniform_set;
	instruction->set_index = set_index;
}

void RenderingDeviceGraph::add_raytracing_list_set_push_constant(
	RDD::ShaderID p_shader, const void* p_data, uint32_t p_data_size)
{
	uint32_t instruction_size = sizeof(RaytracingListSetPushConstantInstruction) + p_data_size;
	RaytracingListSetPushConstantInstruction* instruction =
		reinterpret_cast<RaytracingListSetPushConstantInstruction*>(
			_allocate_raytracing_list_instruction(instruction_size));
	instruction->header.type = RaytracingListInstruction::TYPE_SET_PUSH_CONSTANT;
	instruction->size = p_data_size;
	instruction->shader = p_shader;
	memcpy(instruction->data(), p_data, p_data_size);
}

void RenderingDeviceGraph::add_raytracing_list_trace_rays(
	const RDD::ShaderBindingTable& p_raygen_sbt, const RDD::ShaderBindingTable& p_miss_sbt,
	const RDD::ShaderBindingTable& p_hit_sbt, uint32_t p_width, uint32_t p_height, uint32_t p_depth)
{
	RaytracingListTraceRaysInstruction* instruction =
		reinterpret_cast<RaytracingListTraceRaysInstruction*>(
			_allocate_raytracing_list_instruction(sizeof(RaytracingListTraceRaysInstruction)));
	instruction->header.type = RaytracingListInstruction::TYPE_TRACE_RAYS;
	instruction->raygen_sbt = p_raygen_sbt;
	instruction->miss_sbt = p_miss_sbt;
	instruction->hit_sbt = p_hit_sbt;
	instruction->width = p_width;
	instruction->height = p_height;
	instruction->depth = p_depth;
}

void RenderingDeviceGraph::add_raytracing_list_uniform_set_prepare_for_use(
	RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index)
{
	RaytracingListUniformSetPrepareForUseInstruction* instruction =
		reinterpret_cast<RaytracingListUniformSetPrepareForUseInstruction*>(
			_allocate_raytracing_list_instruction(
				sizeof(RaytracingListUniformSetPrepareForUseInstruction)));
	instruction->header.type = RaytracingListInstruction::TYPE_UNIFORM_SET_PREPARE_FOR_USE;
	instruction->shader = p_shader;
	instruction->uniform_set = p_uniform_set;
	instruction->set_index = set_index;
}

void RenderingDeviceGraph::add_raytracing_list_usage(
	ResourceTracker* p_tracker, ResourceUsage p_usage)
{
	DEV_ASSERT(p_tracker != nullptr);

	p_tracker->reset_if_outdated(tracking_frame);

	if (p_tracker->raytracing_list_index != raytracing_instruction_list.list.index) {
		raytracing_instruction_list.list.command_trackers.push_back(p_tracker);
		raytracing_instruction_list.list.command_tracker_usages.push_back(p_usage);
		p_tracker->raytracing_list_index = raytracing_instruction_list.list.index;
		p_tracker->raytracing_list_usage = p_usage;
	}
#ifdef DEV_ENABLED
	else if (p_tracker->raytracing_list_usage != p_usage) {
		ERR_FAIL_MSG(
			vformat("Tracker can't have more than one type of usage in the same raytracing list. "
					"Raytracing list usage is %d and the requested usage is %d.",
				p_tracker->raytracing_list_usage, p_usage));
	}
#endif
}

void RenderingDeviceGraph::add_raytracing_list_usages(
	VectorView<ResourceTracker*> p_trackers, VectorView<ResourceUsage> p_usages)
{
	DEV_ASSERT(p_trackers.size() == p_usages.size());

	for (uint32_t i = 0; i < p_trackers.size(); i++) {
		add_raytracing_list_usage(p_trackers[i], p_usages[i]);
	}
}

void RenderingDeviceGraph::add_raytracing_list_end()
{
	int32_t command_index;
	uint32_t instruction_data_size = raytracing_instruction_list.list.data.size();
	uint32_t command_size = sizeof(RecordedRaytracingListCommand) + instruction_data_size;
	RecordedRaytracingListCommand* command = reinterpret_cast<RecordedRaytracingListCommand*>(
		_allocate_command(command_size, command_index));
	command->header.type = RecordedCommand::TYPE_RAYTRACING_LIST;
	command->header.self_stages = raytracing_instruction_list.list.stages;
	command->instruction_data_size = instruction_data_size;
	memcpy(command->instruction_data(), raytracing_instruction_list.list.data.ptr(),
		instruction_data_size);
	_add_command_to_graph(raytracing_instruction_list.list.command_trackers.ptr(),
		raytracing_instruction_list.list.command_tracker_usages.ptr(),
		raytracing_instruction_list.list.command_trackers.size(), command_index, &command->header);
}

void RenderingDeviceGraph::add_compute_list_begin(
	RDC::BreadcrumbMarker p_phase, uint32_t p_breadcrumb_data)
{
	compute_instruction_list.list.clear();
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
	compute_instruction_list.breadcrumb = p_breadcrumb_data | (p_phase & ((1 << 16) - 1));
#endif
	compute_instruction_list.list.index++;
}

void RenderingDeviceGraph::add_compute_list_bind_pipeline(RDD::PipelineID p_pipeline)
{
	ComputeListBindPipelineInstruction* instruction =
		reinterpret_cast<ComputeListBindPipelineInstruction*>(
			_allocate_compute_list_instruction(sizeof(ComputeListBindPipelineInstruction)));
	instruction->header.type = ComputeListInstruction::TYPE_BIND_PIPELINE;
	instruction->pipeline = p_pipeline;
	compute_instruction_list.list.stages |= RDD::PIPELINE_STAGE_COMPUTE_SHADER_BIT;
}

void RenderingDeviceGraph::add_compute_list_bind_uniform_set(
	RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index)
{
	add_compute_list_bind_uniform_sets(p_shader, VectorView(&p_uniform_set, 1), set_index, 1);
}

void RenderingDeviceGraph::add_compute_list_bind_uniform_sets(RDD::ShaderID p_shader,
	VectorView<RDD::UniformSetID> p_uniform_sets, uint32_t p_first_set_index, uint32_t p_set_count)
{
	DEV_ASSERT(p_uniform_sets.size() >= p_set_count);

	uint32_t instruction_size =
		sizeof(ComputeListBindUniformSetsInstruction) + sizeof(RDD::UniformSetID) * p_set_count;
	ComputeListBindUniformSetsInstruction* instruction =
		reinterpret_cast<ComputeListBindUniformSetsInstruction*>(
			_allocate_compute_list_instruction(instruction_size));
	instruction->header.type = ComputeListInstruction::TYPE_BIND_UNIFORM_SETS;
	instruction->shader = p_shader;
	instruction->first_set_index = p_first_set_index;
	instruction->set_count = p_set_count;
	instruction->dynamic_offsets_mask = driver->uniform_sets_get_dynamic_offsets(
		p_uniform_sets, p_shader, p_first_set_index, p_set_count);

	RDD::UniformSetID* ids = instruction->uniform_set_ids();
	for (uint32_t i = 0; i < p_set_count; i++) {
		ids[i] = p_uniform_sets[i];
	}
}

void RenderingDeviceGraph::add_compute_list_dispatch(
	uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups)
{
	ComputeListDispatchInstruction* instruction = reinterpret_cast<ComputeListDispatchInstruction*>(
		_allocate_compute_list_instruction(sizeof(ComputeListDispatchInstruction)));
	instruction->header.type = ComputeListInstruction::TYPE_DISPATCH;
	instruction->x_groups = p_x_groups;
	instruction->y_groups = p_y_groups;
	instruction->z_groups = p_z_groups;
}

void RenderingDeviceGraph::add_compute_list_dispatch_indirect(
	RDD::BufferID p_buffer, uint32_t p_offset)
{
	ComputeListDispatchIndirectInstruction* instruction =
		reinterpret_cast<ComputeListDispatchIndirectInstruction*>(
			_allocate_compute_list_instruction(sizeof(ComputeListDispatchIndirectInstruction)));
	instruction->header.type = ComputeListInstruction::TYPE_DISPATCH_INDIRECT;
	instruction->buffer = p_buffer;
	instruction->offset = p_offset;
	compute_instruction_list.list.stages |= RDD::PIPELINE_STAGE_DRAW_INDIRECT_BIT;
}

void RenderingDeviceGraph::add_compute_list_set_push_constant(
	RDD::ShaderID p_shader, const void* p_data, uint32_t p_data_size)
{
	uint32_t instruction_size = sizeof(ComputeListSetPushConstantInstruction) + p_data_size;
	ComputeListSetPushConstantInstruction* instruction =
		reinterpret_cast<ComputeListSetPushConstantInstruction*>(
			_allocate_compute_list_instruction(instruction_size));
	instruction->header.type = ComputeListInstruction::TYPE_SET_PUSH_CONSTANT;
	instruction->size = p_data_size;
	instruction->shader = p_shader;
	memcpy(instruction->data(), p_data, p_data_size);
}

void RenderingDeviceGraph::add_compute_list_uniform_set_prepare_for_use(
	RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index)
{
	ComputeListUniformSetPrepareForUseInstruction* instruction =
		reinterpret_cast<ComputeListUniformSetPrepareForUseInstruction*>(
			_allocate_compute_list_instruction(
				sizeof(ComputeListUniformSetPrepareForUseInstruction)));
	instruction->header.type = ComputeListInstruction::TYPE_UNIFORM_SET_PREPARE_FOR_USE;
	instruction->shader = p_shader;
	instruction->uniform_set = p_uniform_set;
	instruction->set_index = set_index;
}

void RenderingDeviceGraph::add_compute_list_usage(ResourceTracker* p_tracker, ResourceUsage p_usage)
{
	DEV_ASSERT(p_tracker != nullptr);

	p_tracker->reset_if_outdated(tracking_frame);

	if (p_tracker->compute_list_index != compute_instruction_list.list.index) {
		compute_instruction_list.list.command_trackers.push_back(p_tracker);
		compute_instruction_list.list.command_tracker_usages.push_back(p_usage);
		p_tracker->compute_list_index = compute_instruction_list.list.index;
		p_tracker->compute_list_usage = p_usage;
	}
#ifdef DEV_ENABLED
	else if (p_tracker->compute_list_usage != p_usage) {
		ERR_FAIL_MSG(vformat("Tracker can't have more than one type of usage in the same compute "
							 "list. Compute list usage is %s and the requested usage is %s.",
			_usage_to_string(p_tracker->compute_list_usage), _usage_to_string(p_usage)));
	}
#endif
}

void RenderingDeviceGraph::add_compute_list_usages(
	VectorView<ResourceTracker*> p_trackers, VectorView<ResourceUsage> p_usages)
{
	DEV_ASSERT(p_trackers.size() == p_usages.size());

	for (uint32_t i = 0; i < p_trackers.size(); i++) {
		add_compute_list_usage(p_trackers[i], p_usages[i]);
	}
}

void RenderingDeviceGraph::add_compute_list_end()
{
	int32_t command_index;
	uint32_t instruction_data_size = compute_instruction_list.list.data.size();
	uint32_t command_size = sizeof(RecordedComputeListCommand) + instruction_data_size;
	RecordedComputeListCommand* command = reinterpret_cast<RecordedComputeListCommand*>(
		_allocate_command(command_size, command_index));
	command->header.type = RecordedCommand::TYPE_COMPUTE_LIST;
	command->header.self_stages = compute_instruction_list.list.stages;
	command->instruction_data_size = instruction_data_size;
	memcpy(command->instruction_data(), compute_instruction_list.list.data.ptr(),
		instruction_data_size);
	_add_command_to_graph(compute_instruction_list.list.command_trackers.ptr(),
		compute_instruction_list.list.command_tracker_usages.ptr(),
		compute_instruction_list.list.command_trackers.size(), command_index, &command->header);
}

void RenderingDeviceGraph::add_draw_list_begin(FramebufferCache* p_framebuffer_cache,
	Rect2i p_region, VectorView<AttachmentOperation> p_attachment_operations,
	VectorView<RDD::RenderPassClearValue> p_attachment_clear_values, uint32_t p_stages,
	uint32_t p_breadcrumb, bool p_split_cmd_buffer)
{
	_add_draw_list_begin(p_framebuffer_cache, RDD::RenderPassID(), RDD::FramebufferID(), p_region,
		p_attachment_operations, p_attachment_clear_values, p_stages, p_breadcrumb,
		p_split_cmd_buffer);
}

void RenderingDeviceGraph::add_draw_list_begin(RDD::RenderPassID p_render_pass,
	RDD::FramebufferID p_framebuffer, Rect2i p_region,
	VectorView<AttachmentOperation> p_attachment_operations,
	VectorView<RDD::RenderPassClearValue> p_attachment_clear_values, uint32_t p_stages,
	uint32_t p_breadcrumb, bool p_split_cmd_buffer)
{
	_add_draw_list_begin(nullptr, p_render_pass, p_framebuffer, p_region, p_attachment_operations,
		p_attachment_clear_values, p_stages, p_breadcrumb, p_split_cmd_buffer);
}

void RenderingDeviceGraph::add_draw_list_bind_index_buffer(RDD::BufferID p_buffer,
    RenderingDeviceCommons::IndexBufferFormat p_format, uint32_t p_offset)
{
    DrawListBindIndexBufferInstruction* instruction =
        reinterpret_cast<DrawListBindIndexBufferInstruction*>(
            _allocate_draw_list_instruction(sizeof(DrawListBindIndexBufferInstruction)));
    instruction->header.type = DrawListInstruction::TYPE_BIND_INDEX_BUFFER;
    instruction->buffer = p_buffer;
    instruction->format = p_format;
    instruction->offset = p_offset;
    draw_instruction_list.list.stages |= RDD::PIPELINE_STAGE_VERTEX_INPUT_BIT;
}

void RenderingDeviceGraph::add_draw_list_bind_pipeline(
	RDD::PipelineID p_pipeline, uint32_t p_pipeline_stage_bits)
{
	DrawListBindPipelineInstruction* instruction =
		reinterpret_cast<DrawListBindPipelineInstruction*>(
			_allocate_draw_list_instruction(sizeof(DrawListBindPipelineInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_BIND_PIPELINE;
	instruction->pipeline = p_pipeline;
	draw_instruction_list.list.stages = draw_instruction_list.list.stages | p_pipeline_stage_bits;
}

void RenderingDeviceGraph::add_draw_list_bind_uniform_set(
	RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index)
{
	add_draw_list_bind_uniform_sets(p_shader, VectorView(&p_uniform_set, 1), set_index, 1);
}

void RenderingDeviceGraph::add_draw_list_bind_uniform_sets(RDD::ShaderID p_shader,
	VectorView<RDD::UniformSetID> p_uniform_sets, uint32_t p_first_index, uint32_t p_set_count)
{
	DEV_ASSERT(p_uniform_sets.size() >= p_set_count);

	uint32_t instruction_size =
		sizeof(DrawListBindUniformSetsInstruction) + sizeof(RDD::UniformSetID) * p_set_count;
	DrawListBindUniformSetsInstruction* instruction =
		reinterpret_cast<DrawListBindUniformSetsInstruction*>(
			_allocate_draw_list_instruction(instruction_size));
	instruction->header.type = DrawListInstruction::TYPE_BIND_UNIFORM_SETS;
	instruction->shader = p_shader;
	instruction->first_set_index = p_first_index;
	instruction->set_count = p_set_count;
	instruction->dynamic_offsets_mask = driver->uniform_sets_get_dynamic_offsets(
		p_uniform_sets, p_shader, p_first_index, p_set_count);

	for (uint32_t i = 0; i < p_set_count; i++) {
		instruction->uniform_set_ids()[i] = p_uniform_sets[i];
	}
}

void RenderingDeviceGraph::add_draw_list_bind_vertex_buffers(
	Span<RDD::BufferID> p_vertex_buffers, Span<uint64_t> p_vertex_buffer_offsets)
{
	DEV_ASSERT(p_vertex_buffers.size() == p_vertex_buffer_offsets.size());

	uint32_t instruction_size = sizeof(DrawListBindVertexBuffersInstruction) +
								sizeof(RDD::BufferID) * p_vertex_buffers.size() +
								sizeof(uint64_t) * p_vertex_buffer_offsets.size();
	DrawListBindVertexBuffersInstruction* instruction =
		reinterpret_cast<DrawListBindVertexBuffersInstruction*>(
			_allocate_draw_list_instruction(instruction_size));
	instruction->header.type = DrawListInstruction::TYPE_BIND_VERTEX_BUFFERS;
	instruction->vertex_buffers_count = p_vertex_buffers.size();
	instruction->dynamic_offsets_mask = driver->buffer_get_dynamic_offsets(p_vertex_buffers);

	RDD::BufferID* vertex_buffers = instruction->vertex_buffers();
	uint64_t* vertex_buffer_offsets = instruction->vertex_buffer_offsets();
	for (uint32_t i = 0; i < instruction->vertex_buffers_count; i++) {
		vertex_buffers[i] = p_vertex_buffers[i];
		vertex_buffer_offsets[i] = p_vertex_buffer_offsets[i];
	}

	if (instruction->vertex_buffers_count > 0) {
		draw_instruction_list.list.stages |= RDD::PIPELINE_STAGE_VERTEX_INPUT_BIT;
	}
}

void RenderingDeviceGraph::add_draw_list_clear_attachments(
	VectorView<RDD::AttachmentClear> p_attachments_clear,
	VectorView<Rect2i> p_attachments_clear_rect)
{
	uint32_t instruction_size = sizeof(DrawListClearAttachmentsInstruction) +
								sizeof(RDD::AttachmentClear) * p_attachments_clear.size() +
								sizeof(Rect2i) * p_attachments_clear_rect.size();
	DrawListClearAttachmentsInstruction* instruction =
		reinterpret_cast<DrawListClearAttachmentsInstruction*>(
			_allocate_draw_list_instruction(instruction_size));
	instruction->header.type = DrawListInstruction::TYPE_CLEAR_ATTACHMENTS;
	instruction->attachments_clear_count = p_attachments_clear.size();
	instruction->attachments_clear_rect_count = p_attachments_clear_rect.size();

	RDD::AttachmentClear* attachments_clear = instruction->attachments_clear();
	Rect2i* attachments_clear_rect = instruction->attachments_clear_rect();
	for (uint32_t i = 0; i < instruction->attachments_clear_count; i++) {
		attachments_clear[i] = p_attachments_clear[i];
	}

	for (uint32_t i = 0; i < instruction->attachments_clear_rect_count; i++) {
		attachments_clear_rect[i] = p_attachments_clear_rect[i];
	}
}

void RenderingDeviceGraph::add_draw_list_draw(uint32_t p_vertex_count, uint32_t p_instance_count)
{
	DrawListDrawInstruction* instruction = reinterpret_cast<DrawListDrawInstruction*>(
		_allocate_draw_list_instruction(sizeof(DrawListDrawInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_DRAW;
	instruction->vertex_count = p_vertex_count;
	instruction->instance_count = p_instance_count;
}

void RenderingDeviceGraph::add_draw_list_draw_indexed(
	uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index)
{
	DrawListDrawIndexedInstruction* instruction = reinterpret_cast<DrawListDrawIndexedInstruction*>(
		_allocate_draw_list_instruction(sizeof(DrawListDrawIndexedInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_DRAW_INDEXED;
	instruction->index_count = p_index_count;
	instruction->instance_count = p_instance_count;
	instruction->first_index = p_first_index;
}

void RenderingDeviceGraph::add_draw_list_draw_indirect(
	RDD::BufferID p_buffer, uint32_t p_offset, uint32_t p_draw_count, uint32_t p_stride)
{
	DrawListDrawIndirectInstruction* instruction =
		reinterpret_cast<DrawListDrawIndirectInstruction*>(
			_allocate_draw_list_instruction(sizeof(DrawListDrawIndirectInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_DRAW_INDIRECT;
	instruction->buffer = p_buffer;
	instruction->offset = p_offset;
	instruction->draw_count = p_draw_count;
	instruction->stride = p_stride;
	draw_instruction_list.list.stages |= RDD::PIPELINE_STAGE_DRAW_INDIRECT_BIT;
}

void RenderingDeviceGraph::add_draw_list_draw_indexed_indirect(
	RDD::BufferID p_buffer, uint32_t p_offset, uint32_t p_draw_count, uint32_t p_stride)
{
	DrawListDrawIndexedIndirectInstruction* instruction =
		reinterpret_cast<DrawListDrawIndexedIndirectInstruction*>(
			_allocate_draw_list_instruction(sizeof(DrawListDrawIndexedIndirectInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_DRAW_INDEXED_INDIRECT;
	instruction->buffer = p_buffer;
	instruction->offset = p_offset;
	instruction->draw_count = p_draw_count;
	instruction->stride = p_stride;
	draw_instruction_list.list.stages |= RDD::PIPELINE_STAGE_DRAW_INDIRECT_BIT;
}

void RenderingDeviceGraph::add_draw_list_execute_commands(RDD::CommandBufferID p_command_buffer)
{
	DrawListExecuteCommandsInstruction* instruction =
		reinterpret_cast<DrawListExecuteCommandsInstruction*>(
			_allocate_draw_list_instruction(sizeof(DrawListExecuteCommandsInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_EXECUTE_COMMANDS;
	instruction->command_buffer = p_command_buffer;
}

void RenderingDeviceGraph::add_draw_list_next_subpass(RDD::CommandBufferType p_command_buffer_type)
{
	DrawListNextSubpassInstruction* instruction = reinterpret_cast<DrawListNextSubpassInstruction*>(
		_allocate_draw_list_instruction(sizeof(DrawListNextSubpassInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_NEXT_SUBPASS;
	instruction->command_buffer_type = p_command_buffer_type;
}

void RenderingDeviceGraph::add_draw_list_set_blend_constants(const Color& p_color)
{
	DrawListSetBlendConstantsInstruction* instruction =
		reinterpret_cast<DrawListSetBlendConstantsInstruction*>(
			_allocate_draw_list_instruction(sizeof(DrawListSetBlendConstantsInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_SET_BLEND_CONSTANTS;
	instruction->color = p_color;
}

void RenderingDeviceGraph::add_draw_list_set_line_width(float p_width)
{
	DrawListSetLineWidthInstruction* instruction =
		reinterpret_cast<DrawListSetLineWidthInstruction*>(
			_allocate_draw_list_instruction(sizeof(DrawListSetLineWidthInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_SET_LINE_WIDTH;
	instruction->width = p_width;
}

void RenderingDeviceGraph::add_draw_list_set_push_constant(
	RDD::ShaderID p_shader, const void* p_data, uint32_t p_data_size)
{
	uint32_t instruction_size = sizeof(DrawListSetPushConstantInstruction) + p_data_size;
	DrawListSetPushConstantInstruction* instruction =
		reinterpret_cast<DrawListSetPushConstantInstruction*>(
			_allocate_draw_list_instruction(instruction_size));
	instruction->header.type = DrawListInstruction::TYPE_SET_PUSH_CONSTANT;
	instruction->size = p_data_size;
	instruction->shader = p_shader;
	memcpy(instruction->data(), p_data, p_data_size);
}

void RenderingDeviceGraph::add_draw_list_set_scissor(Rect2i p_rect)
{
	DrawListSetScissorInstruction* instruction = reinterpret_cast<DrawListSetScissorInstruction*>(
		_allocate_draw_list_instruction(sizeof(DrawListSetScissorInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_SET_SCISSOR;
	instruction->rect = p_rect;
}

void RenderingDeviceGraph::add_draw_list_set_viewport(Rect2i p_rect)
{
	DrawListSetViewportInstruction* instruction = reinterpret_cast<DrawListSetViewportInstruction*>(
		_allocate_draw_list_instruction(sizeof(DrawListSetViewportInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_SET_VIEWPORT;
	instruction->rect = p_rect;
}

void RenderingDeviceGraph::add_draw_list_uniform_set_prepare_for_use(
	RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index)
{
	DrawListUniformSetPrepareForUseInstruction* instruction =
		reinterpret_cast<DrawListUniformSetPrepareForUseInstruction*>(
			_allocate_draw_list_instruction(sizeof(DrawListUniformSetPrepareForUseInstruction)));
	instruction->header.type = DrawListInstruction::TYPE_UNIFORM_SET_PREPARE_FOR_USE;
	instruction->shader = p_shader;
	instruction->uniform_set = p_uniform_set;
	instruction->set_index = set_index;
}

void RenderingDeviceGraph::add_draw_list_usage(ResourceTracker* p_tracker, ResourceUsage p_usage)
{
	p_tracker->reset_if_outdated(tracking_frame);

	if (p_tracker->draw_list_index != draw_instruction_list.list.index) {
		draw_instruction_list.list.command_trackers.push_back(p_tracker);
		draw_instruction_list.list.command_tracker_usages.push_back(p_usage);
		p_tracker->draw_list_index = draw_instruction_list.list.index;
		p_tracker->draw_list_usage = p_usage;
	}
#ifdef DEV_ENABLED
	else if (p_tracker->draw_list_usage != p_usage) {
		ERR_FAIL_MSG(vformat("Tracker can't have more than one type of usage in the same draw "
							 "list. Draw list usage is %s and the requested usage is %s.",
			_usage_to_string(p_tracker->draw_list_usage), _usage_to_string(p_usage)));
	}
#endif
}

void RenderingDeviceGraph::add_draw_list_usages(
	VectorView<ResourceTracker*> p_trackers, VectorView<ResourceUsage> p_usages)
{
	DEV_ASSERT(p_trackers.size() == p_usages.size());

	for (uint32_t i = 0; i < p_trackers.size(); i++) {
		add_draw_list_usage(p_trackers[i], p_usages[i]);
	}
}

void RenderingDeviceGraph::add_draw_list_end()
{
	FramebufferCache* framebuffer_cache = draw_instruction_list.framebuffer_cache;
	int32_t command_index;
	uint32_t clear_values_size =
		sizeof(RDD::RenderPassClearValue) * draw_instruction_list.attachment_clear_values.size();
	uint32_t trackers_count = framebuffer_cache != nullptr ? framebuffer_cache->trackers.size() : 0;
	uint32_t trackers_and_ops_size = (sizeof(ResourceTracker*) + sizeof(RDD::AttachmentLoadOp) +
										 sizeof(RDD::AttachmentStoreOp)) *
									 trackers_count;
	uint32_t instruction_data_size = draw_instruction_list.list.data.size();
	uint32_t command_size = sizeof(RecordedDrawListCommand) + clear_values_size +
							trackers_and_ops_size + instruction_data_size;
	RecordedDrawListCommand* command =
		reinterpret_cast<RecordedDrawListCommand*>(_allocate_command(command_size, command_index));
	command->header.type = RecordedCommand::TYPE_DRAW_LIST;
	command->header.self_stages = draw_instruction_list.list.stages;
	command->framebuffer_cache = framebuffer_cache;
	command->render_pass = draw_instruction_list.render_pass;
	command->framebuffer = draw_instruction_list.framebuffer;
	command->instruction_data_size = instruction_data_size;
	command->command_buffer_type = RDD::COMMAND_BUFFER_TYPE_PRIMARY;
	command->region = draw_instruction_list.region;
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
	command->breadcrumb = draw_instruction_list.breadcrumb;
#endif
	command->split_cmd_buffer = draw_instruction_list.split_cmd_buffer;
	command->clear_values_count = draw_instruction_list.attachment_clear_values.size();
	command->trackers_count = trackers_count;

	// Initialize the load and store operations to their default behaviors. The store behavior will
	// be modified if a command depends on the result of this render pass.
	uint32_t attachment_op_count = draw_instruction_list.attachment_operations.size();
	ResourceTracker** trackers = command->trackers();
	RDD::AttachmentLoadOp* load_ops = command->load_ops();
	RDD::AttachmentStoreOp* store_ops = command->store_ops();
	for (uint32_t i = 0; i < command->trackers_count; i++) {
		ResourceTracker* resource_tracker = framebuffer_cache->trackers[i];
		if (resource_tracker != nullptr) {
			if (i < command->clear_values_count && i < attachment_op_count &&
				draw_instruction_list.attachment_operations[i] == ATTACHMENT_OPERATION_CLEAR) {
				load_ops[i] = RDD::ATTACHMENT_LOAD_OP_CLEAR;
			}
			else if (i < attachment_op_count && draw_instruction_list.attachment_operations[i] ==
													  ATTACHMENT_OPERATION_IGNORE) {
				load_ops[i] = RDD::ATTACHMENT_LOAD_OP_DONT_CARE;
			}
			else if (resource_tracker->is_discardable) {
				bool resource_has_parent = resource_tracker->parent != nullptr;
				ResourceTracker* search_tracker =
					resource_has_parent ? resource_tracker->parent : resource_tracker;
				search_tracker->reset_if_outdated(tracking_frame);
				bool resource_was_modified_this_frame =
					search_tracker->write_command_or_list_index >= 0;
				load_ops[i] = resource_was_modified_this_frame ? RDD::ATTACHMENT_LOAD_OP_LOAD
															   : RDD::ATTACHMENT_LOAD_OP_DONT_CARE;
			}
			else {
				load_ops[i] = RDD::ATTACHMENT_LOAD_OP_LOAD;
			}

			store_ops[i] = resource_tracker->is_discardable ? RDD::ATTACHMENT_STORE_OP_DONT_CARE
															: RDD::ATTACHMENT_STORE_OP_STORE;
		}
		else {
			load_ops[i] = RDD::ATTACHMENT_LOAD_OP_DONT_CARE;
			store_ops[i] = RDD::ATTACHMENT_STORE_OP_DONT_CARE;
		}

		trackers[i] = resource_tracker;
	}

	RDD::RenderPassClearValue* clear_values = command->clear_values();
	for (uint32_t i = 0; i < command->clear_values_count; i++) {
		clear_values[i] = draw_instruction_list.attachment_clear_values[i];
	}

	memcpy(
		command->instruction_data(), draw_instruction_list.list.data.ptr(), instruction_data_size);
	_add_command_to_graph(draw_instruction_list.list.command_trackers.ptr(),
		draw_instruction_list.list.command_tracker_usages.ptr(),
		draw_instruction_list.list.command_trackers.size(), command_index, &command->header);
}

void RenderingDeviceGraph::add_texture_clear_color(RDD::TextureID p_dst,
	ResourceTracker* p_dst_tracker, const Color& p_color,
	const RDD::TextureSubresourceRange& p_range)
{
	DEV_ASSERT(p_dst_tracker != nullptr);

	int32_t command_index;
	RecordedTextureClearColorCommand* command = reinterpret_cast<RecordedTextureClearColorCommand*>(
		_allocate_command(sizeof(RecordedTextureClearColorCommand), command_index));
	command->header.type = RecordedCommand::TYPE_TEXTURE_CLEAR_COLOR;
	command->texture = p_dst;
	command->color = p_color;
	command->range = p_range;

	ResourceUsage usage;
	if (driver_clears_with_copy_engine) {
		command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
		usage = RESOURCE_USAGE_COPY_TO;
	}
	else {
		// If the driver is uncapable of using the copy engine for clearing the image (e.g. D3D12),
		// we must either transition the resource to a render target or a storage image as that's
		// the only two ways it can perform the operation.
		if (p_dst_tracker->texture_usage & RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT) {
			command->header.self_stages = RDD::PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			usage = RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE;
		}
		else {
			command->header.self_stages = RDD::PIPELINE_STAGE_CLEAR_STORAGE_BIT;
			usage = RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE;
		}
	}

	_add_command_to_graph(&p_dst_tracker, &usage, 1, command_index, &command->header);
}

void RenderingDeviceGraph::add_texture_clear_depth_stencil(RDD::TextureID p_dst,
	ResourceTracker* p_dst_tracker, float p_depth, uint8_t p_stencil,
	const RDD::TextureSubresourceRange& p_range)
{
	DEV_ASSERT(p_dst_tracker != nullptr);

	int32_t command_index;
	RecordedTextureClearDepthStencilCommand* command =
		reinterpret_cast<RecordedTextureClearDepthStencilCommand*>(
			_allocate_command(sizeof(RecordedTextureClearDepthStencilCommand), command_index));
	command->header.type = RecordedCommand::TYPE_TEXTURE_CLEAR_DEPTH_STENCIL;
	command->texture = p_dst;
	command->depth = p_depth;
	command->stencil = p_stencil;
	command->range = p_range;

	ResourceUsage usage;
	if (driver_clears_with_copy_engine) {
		command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
		usage = RESOURCE_USAGE_COPY_TO;
	}
	else {
		// If the driver is uncapable of using the copy engine for clearing the image (e.g. D3D12),
		// we must transition the resource to a depth stencil as that's the only way it can perform
		// the operation.
		command->header.self_stages = RDD::PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
									  RDD::PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		usage = RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE;
	}

	_add_command_to_graph(&p_dst_tracker, &usage, 1, command_index, &command->header);
}

void RenderingDeviceGraph::add_texture_copy(RDD::TextureID p_src, ResourceTracker* p_src_tracker,
	RDD::TextureID p_dst, ResourceTracker* p_dst_tracker,
	VectorView<RDD::TextureCopyRegion> p_texture_copy_regions)
{
	DEV_ASSERT(p_src_tracker != nullptr);
	DEV_ASSERT(p_dst_tracker != nullptr);

	int32_t command_index;
	uint64_t command_size = sizeof(RecordedTextureCopyCommand) +
							p_texture_copy_regions.size() * sizeof(RDD::TextureCopyRegion);
	RecordedTextureCopyCommand* command = reinterpret_cast<RecordedTextureCopyCommand*>(
		_allocate_command(command_size, command_index));
	command->header.type = RecordedCommand::TYPE_TEXTURE_COPY;
	command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
	command->from_texture = p_src;
	command->to_texture = p_dst;
	command->texture_copy_regions_count = p_texture_copy_regions.size();

	RDD::TextureCopyRegion* texture_copy_regions = command->texture_copy_regions();
	for (uint32_t i = 0; i < command->texture_copy_regions_count; i++) {
		texture_copy_regions[i] = p_texture_copy_regions[i];
	}

	ResourceTracker* trackers[2] = {p_dst_tracker, p_src_tracker};
	ResourceUsage usages[2] = {RESOURCE_USAGE_COPY_TO, RESOURCE_USAGE_COPY_FROM};
	_add_command_to_graph(trackers, usages, 2, command_index, &command->header);
}

void RenderingDeviceGraph::add_texture_get_data(RDD::TextureID p_src,
	ResourceTracker* p_src_tracker, RDD::BufferID p_dst,
	VectorView<RDD::BufferTextureCopyRegion> p_buffer_texture_copy_regions,
	ResourceTracker* p_dst_tracker)
{
	DEV_ASSERT(p_src_tracker != nullptr);

	int32_t command_index;
	uint64_t command_size =
		sizeof(RecordedTextureGetDataCommand) +
		p_buffer_texture_copy_regions.size() * sizeof(RDD::BufferTextureCopyRegion);
	RecordedTextureGetDataCommand* command = reinterpret_cast<RecordedTextureGetDataCommand*>(
		_allocate_command(command_size, command_index));
	command->header.type = RecordedCommand::TYPE_TEXTURE_GET_DATA;
	command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
	command->from_texture = p_src;
	command->to_buffer = p_dst;
	command->buffer_texture_copy_regions_count = p_buffer_texture_copy_regions.size();

	RDD::BufferTextureCopyRegion* buffer_texture_copy_regions =
		command->buffer_texture_copy_regions();
	for (uint32_t i = 0; i < command->buffer_texture_copy_regions_count; i++) {
		buffer_texture_copy_regions[i] = p_buffer_texture_copy_regions[i];
	}

	if (p_dst_tracker != nullptr) {
		// Add the optional destination tracker if it was provided.
		ResourceTracker* trackers[2] = {p_dst_tracker, p_src_tracker};
		ResourceUsage usages[2] = {RESOURCE_USAGE_COPY_TO, RESOURCE_USAGE_COPY_FROM};
		_add_command_to_graph(trackers, usages, 2, command_index, &command->header);
	}
	else {
		ResourceUsage usage = RESOURCE_USAGE_COPY_FROM;
		_add_command_to_graph(&p_src_tracker, &usage, 1, command_index, &command->header);
	}
}

void RenderingDeviceGraph::add_texture_resolve(RDD::TextureID p_src, ResourceTracker* p_src_tracker,
	RDD::TextureID p_dst, ResourceTracker* p_dst_tracker, uint32_t p_src_layer,
	uint32_t p_src_mipmap, uint32_t p_dst_layer, uint32_t p_dst_mipmap)
{
	DEV_ASSERT(p_src_tracker != nullptr);
	DEV_ASSERT(p_dst_tracker != nullptr);

	int32_t command_index;
	RecordedTextureResolveCommand* command = reinterpret_cast<RecordedTextureResolveCommand*>(
		_allocate_command(sizeof(RecordedTextureResolveCommand), command_index));
	command->header.type = RecordedCommand::TYPE_TEXTURE_RESOLVE;
	command->header.self_stages = RDD::PIPELINE_STAGE_RESOLVE_BIT;
	command->from_texture = p_src;
	command->to_texture = p_dst;
	command->src_layer = p_src_layer;
	command->src_mipmap = p_src_mipmap;
	command->dst_layer = p_dst_layer;
	command->dst_mipmap = p_dst_mipmap;

	ResourceTracker* trackers[2] = {p_dst_tracker, p_src_tracker};
	ResourceUsage usages[2] = {RESOURCE_USAGE_RESOLVE_TO, RESOURCE_USAGE_RESOLVE_FROM};
	_add_command_to_graph(trackers, usages, 2, command_index, &command->header);
}

void RenderingDeviceGraph::add_texture_update(RDD::TextureID p_dst, ResourceTracker* p_dst_tracker,
	VectorView<RecordedBufferToTextureCopy> p_buffer_copies,
	VectorView<ResourceTracker*> p_buffer_trackers)
{
	DEV_ASSERT(p_dst_tracker != nullptr);

	int32_t command_index;
	uint64_t command_size = sizeof(RecordedTextureUpdateCommand) +
							p_buffer_copies.size() * sizeof(RecordedBufferToTextureCopy);
	RecordedTextureUpdateCommand* command = reinterpret_cast<RecordedTextureUpdateCommand*>(
		_allocate_command(command_size, command_index));
	command->header.type = RecordedCommand::TYPE_TEXTURE_UPDATE;
	command->header.self_stages = RDD::PIPELINE_STAGE_COPY_BIT;
	command->to_texture = p_dst;
	command->buffer_to_texture_copies_count = p_buffer_copies.size();

	RecordedBufferToTextureCopy* buffer_to_texture_copies = command->buffer_to_texture_copies();
	for (uint32_t i = 0; i < command->buffer_to_texture_copies_count; i++) {
		buffer_to_texture_copies[i] = p_buffer_copies[i];
	}

	if (p_buffer_trackers.size() > 0) {
		// Add the optional buffer trackers if they were provided.
		thread_local LocalVector<ResourceTracker*> trackers;
		thread_local LocalVector<ResourceUsage> usages;
		trackers.clear();
		usages.clear();
		for (uint32_t i = 0; i < p_buffer_trackers.size(); i++) {
			trackers.push_back(p_buffer_trackers[i]);
			usages.push_back(RESOURCE_USAGE_COPY_FROM);
		}

		trackers.push_back(p_dst_tracker);
		usages.push_back(RESOURCE_USAGE_COPY_TO);

		_add_command_to_graph(
			trackers.ptr(), usages.ptr(), trackers.size(), command_index, &command->header);
	}
	else {
		ResourceUsage usage = RESOURCE_USAGE_COPY_TO;
		_add_command_to_graph(&p_dst_tracker, &usage, 1, command_index, &command->header);
	}
}

void RenderingDeviceGraph::add_capture_timestamp(RDD::QueryPoolID p_query_pool, uint32_t p_index)
{
	int32_t command_index;
	RecordedCaptureTimestampCommand* command = reinterpret_cast<RecordedCaptureTimestampCommand*>(
		_allocate_command(sizeof(RecordedCaptureTimestampCommand), command_index));
	command->header.type = RecordedCommand::TYPE_CAPTURE_TIMESTAMP;
	command->header.self_stages = 0;
	command->pool = p_query_pool;
	command->index = p_index;
	_add_command_to_graph(nullptr, nullptr, 0, command_index, &command->header);
}

void RenderingDeviceGraph::add_synchronization()
{
	// Synchronization is only acknowledged if commands have been recorded on the graph already.
	if (command_count > 0) {
		command_synchronization_pending = true;
	}
}

void RenderingDeviceGraph::begin_label(const Span<char>& p_label_name, const Color& p_color)
{
	uint32_t command_label_offset = command_label_chars.size();
	int command_label_size = p_label_name.size();
	command_label_chars.resize(command_label_offset + command_label_size + 1);
	memcpy(&command_label_chars[command_label_offset], p_label_name.ptr(), command_label_size);
	command_label_chars[command_label_offset + command_label_size] = '\0';
	command_label_colors.push_back(p_color);
	command_label_offsets.push_back(command_label_offset);
	command_label_index = command_label_count;
	command_label_count++;
}

void RenderingDeviceGraph::end_label() { command_label_index = -1; }

void RenderingDeviceGraph::end(bool p_reorder_commands, bool p_full_barriers,
	RDD::CommandBufferID& r_command_buffer, CommandBufferPool& r_command_buffer_pool)
{
	if (command_count == 0) {
		// No commands have been logged, do nothing.
		return;
	}

	thread_local LocalVector<RecordedCommandSort> commands_sorted;
	if (p_reorder_commands) {
		thread_local LocalVector<int64_t> command_stack;
		thread_local LocalVector<int32_t> sorted_command_indices;
		thread_local LocalVector<uint32_t> command_degrees;
		int32_t adjacency_list_index = 0;
		int32_t command_index;

		// Count all the incoming connections to every node by traversing their adjacency list.
		command_degrees.resize(command_count);
		memset(command_degrees.ptr(), 0, sizeof(uint32_t) * command_degrees.size());
		for (uint32_t i = 0; i < command_count; i++) {
			const RecordedCommand& recorded_command =
				*reinterpret_cast<const RecordedCommand*>(&command_data[command_data_offsets[i]]);
			adjacency_list_index = recorded_command.adjacent_command_list_index;
			while (adjacency_list_index >= 0) {
				const RecordedCommandListNode& command_list_node =
					command_list_nodes[adjacency_list_index];
				DEV_ASSERT((command_list_node.command_index != int32_t(i)) &&
						   "Command can't have itself as a dependency.");
				command_degrees[command_list_node.command_index] += 1;
				adjacency_list_index = command_list_node.next_list_index;
			}
		}

		// Push to the stack all nodes that have no incoming connections.
		command_stack.clear();
		for (uint32_t i = 0; i < command_count; i++) {
			if (command_degrees[i] == 0) {
				command_stack.push_back(i);
			}
		}

		sorted_command_indices.clear();
		while (!command_stack.is_empty()) {
			// Pop command from the stack.
			command_index = command_stack[command_stack.size() - 1];
			command_stack.resize(command_stack.size() - 1);

			// Add it to the sorted commands.
			sorted_command_indices.push_back(command_index);

			// Search for its adjacents and lower their degree for every visit. If the degree
			// reaches zero, we push the command to the stack.
			const uint32_t command_data_offset = command_data_offsets[command_index];
			const RecordedCommand& recorded_command =
				*reinterpret_cast<const RecordedCommand*>(&command_data[command_data_offset]);
			adjacency_list_index = recorded_command.adjacent_command_list_index;
			while (adjacency_list_index >= 0) {
				const RecordedCommandListNode& command_list_node =
					command_list_nodes[adjacency_list_index];
				uint32_t& command_degree = command_degrees[command_list_node.command_index];
				DEV_ASSERT(command_degree > 0);
				command_degree--;
				if (command_degree == 0) {
					command_stack.push_back(command_list_node.command_index);
				}

				adjacency_list_index = command_list_node.next_list_index;
			}
		}

		// Batch buffer, texture, draw lists and compute operations together.
		const uint32_t PriorityTable[] = {
			0, // TYPE_NONE
			6, // TYPE_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_BUILD
			6, // TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE_BUILD
			1, // TYPE_BUFFER_CLEAR
			1, // TYPE_BUFFER_COPY
			1, // TYPE_BUFFER_GET_DATA
			1, // TYPE_BUFFER_UPDATE
			4, // TYPE_COMPUTE_LIST
			7, // TYPE_RAYTRACING_LIST
			3, // TYPE_DRAW_LIST
			2, // TYPE_TEXTURE_CLEAR_COLOR
			2, // TYPE_TEXTURE_CLEAR_DEPTH_STENCIL
			2, // TYPE_TEXTURE_COPY
			2, // TYPE_TEXTURE_GET_DATA
			2, // TYPE_TEXTURE_RESOLVE
			2, // TYPE_TEXTURE_UPDATE
			2, // TYPE_CAPTURE_TIMESTAMP
			5, // TYPE_DRIVER_CALLBACK
		};
		static_assert(std_size(PriorityTable) == RecordedCommand::TYPE_MAX,
			"PriorityTable must have one entry per RecordedCommand::Type");

		commands_sorted.clear();
		commands_sorted.resize(command_count);

		for (uint32_t i = 0; i < command_count; i++) {
			const int32_t sorted_command_index = sorted_command_indices[i];
			const uint32_t command_data_offset = command_data_offsets[sorted_command_index];
			const RecordedCommand recorded_command =
				*reinterpret_cast<const RecordedCommand*>(&command_data[command_data_offset]);
			const uint32_t next_command_level = commands_sorted[sorted_command_index].level + 1;
			adjacency_list_index = recorded_command.adjacent_command_list_index;
			while (adjacency_list_index >= 0) {
				const RecordedCommandListNode& command_list_node =
					command_list_nodes[adjacency_list_index];
				uint32_t& adjacent_command_level =
					commands_sorted[command_list_node.command_index].level;
				if (adjacent_command_level < next_command_level) {
					adjacent_command_level = next_command_level;
				}

				adjacency_list_index = command_list_node.next_list_index;
			}

			commands_sorted[sorted_command_index].index = sorted_command_index;
			commands_sorted[sorted_command_index].priority = PriorityTable[recorded_command.type];
		}
	}
	else {
		commands_sorted.clear();
		commands_sorted.resize(command_count);

		for (uint32_t i = 0; i < command_count; i++) {
			commands_sorted[i].index = i;
		}
	}

	_wait_for_secondary_command_buffer_tasks();

	if (command_count > 0) {
		int32_t current_label_index = -1;
		int32_t current_label_level = -1;
		_run_label_command_change(r_command_buffer, -1, -1, true, true, nullptr, 0,
			current_label_index, current_label_level);

		if (driver_workarounds.avoid_compute_after_draw) {
			// Reset the state of the workaround.
			workarounds_state.draw_list_found = false;
		}

#if PRINT_DRAW_LIST_STATS
		draw_list_total_size = 0;
#endif

		if (p_reorder_commands) {
#if PRINT_RENDER_GRAPH
			print_line("BEFORE SORT");
			_print_render_commands(commands_sorted.ptr(), command_count);
#endif

			commands_sorted.sort();

#if PRINT_RENDER_GRAPH
			print_line("AFTER SORT");
			_print_render_commands(commands_sorted.ptr(), command_count);
#endif

#if PRINT_COMMAND_RECORDING
			print_line(vformat("Recording %d commands", command_count));
#endif

			uint32_t boosted_priority = 0;
			uint32_t current_level = commands_sorted[0].level;
			uint32_t current_level_start = 0;
			for (uint32_t i = 0; i < command_count; i++) {
				if (current_level != commands_sorted[i].level) {
					RecordedCommandSort* level_command_ptr = &commands_sorted[current_level_start];
					uint32_t level_command_count = i - current_level_start;
					_boost_priority_for_render_commands(
						level_command_ptr, level_command_count, boosted_priority);
					_group_barriers_for_render_commands(
						r_command_buffer, level_command_ptr, level_command_count, p_full_barriers);
					_run_render_commands(current_level, level_command_ptr, level_command_count,
						r_command_buffer, r_command_buffer_pool, current_label_index,
						current_label_level);
					current_level = commands_sorted[i].level;
					current_level_start = i;
				}
			}

			RecordedCommandSort* level_command_ptr = &commands_sorted[current_level_start];
			uint32_t level_command_count = command_count - current_level_start;
			_boost_priority_for_render_commands(
				level_command_ptr, level_command_count, boosted_priority);
			_group_barriers_for_render_commands(
				r_command_buffer, level_command_ptr, level_command_count, p_full_barriers);
			_run_render_commands(current_level, level_command_ptr, level_command_count,
				r_command_buffer, r_command_buffer_pool, current_label_index, current_label_level);

#if PRINT_RENDER_GRAPH
			print_line("COMMANDS", command_count, "LEVELS", current_level + 1);
#endif
		}
		else {
			for (uint32_t i = 0; i < command_count; i++) {
				_group_barriers_for_render_commands(
					r_command_buffer, &commands_sorted[i], 1, p_full_barriers);
				_run_render_commands(i, &commands_sorted[i], 1, r_command_buffer,
					r_command_buffer_pool, current_label_index, current_label_level);
			}
		}

		_run_label_command_change(r_command_buffer, -1, -1, false, false, nullptr, 0,
			current_label_index, current_label_level);

#if PRINT_DRAW_LIST_STATS
		print_line(vformat("Draw list %d bytes", draw_list_total_size));
#endif
#if PRINT_COMMAND_RECORDING
		print_line(vformat("Recorded %d commands", command_count));
#endif
	}

	// Advance the frame counter. It's not necessary to do this if no commands are recorded because
	// that means no secondary command buffers were used.
	frame = (frame + 1) % frames.size();
}

#if PRINT_RESOURCE_TRACKER_TOTAL
static uint32_t resource_tracker_total = 0;
#endif

RenderingDeviceGraph::ResourceTracker* RenderingDeviceGraph::resource_tracker_create()
{
#if PRINT_RESOURCE_TRACKER_TOTAL
	print_line("Resource trackers:", ++resource_tracker_total);
#endif
	return memnew(ResourceTracker);
}

void RenderingDeviceGraph::resource_tracker_free(ResourceTracker* p_tracker)
{
	if (p_tracker == nullptr) {
		return;
	}

	if (p_tracker->in_parent_dirty_list) {
		// Delete the tracker from the parent's dirty linked list.
		if (p_tracker->parent->dirty_shared_list == p_tracker) {
			p_tracker->parent->dirty_shared_list = p_tracker->next_shared;
		}
		else {
			ResourceTracker* node = p_tracker->parent->dirty_shared_list;
			while (node != nullptr) {
				if (node->next_shared == p_tracker) {
					node->next_shared = p_tracker->next_shared;
					node = nullptr;
				}
				else {
					node = node->next_shared;
				}
			}
		}
	}

	memdelete(p_tracker);

#if PRINT_RESOURCE_TRACKER_TOTAL
	print_line("Resource trackers:", --resource_tracker_total);
#endif
}

RenderingDeviceGraph::FramebufferCache* RenderingDeviceGraph::framebuffer_cache_create()
{
	return memnew(FramebufferCache);
}

void RenderingDeviceGraph::framebuffer_cache_free(RDD* p_driver, FramebufferCache* p_cache)
{
	DEV_ASSERT(p_driver != nullptr);

	if (p_cache == nullptr) {
		return;
	}

	for (KeyValue<uint64_t, FramebufferStorage>& E : p_cache->storage_map) {
		p_driver->framebuffer_free(E.value.framebuffer);
		p_driver->render_pass_free(E.value.render_pass);
	}

	memdelete(p_cache);
}

void RenderingDeviceGraph::_add_texture_barrier_to_command(RDD::TextureID p_texture_id,
	uint32_t p_src_access, uint32_t p_dst_access, ResourceUsage p_prev_usage,
	ResourceUsage p_next_usage, RDD::TextureSubresourceRange p_subresources,
	LocalVector<RDD::TextureBarrier>& r_barrier_vector, int32_t& r_barrier_index,
	int32_t& r_barrier_count)
{
	if (!driver_honors_barriers) {
		return;
	}

	if (r_barrier_index < 0) {
		r_barrier_index = r_barrier_vector.size();
	}

	RDD::TextureBarrier texture_barrier;
	texture_barrier.texture = p_texture_id;
	texture_barrier.src_access = p_src_access;
	texture_barrier.dst_access = p_dst_access;
	texture_barrier.prev_layout = _usage_to_image_layout(p_prev_usage);
	texture_barrier.next_layout = _usage_to_image_layout(p_next_usage);
	texture_barrier.subresources = p_subresources;
	r_barrier_vector.push_back(texture_barrier);
	r_barrier_count++;
}

#if USE_BUFFER_BARRIERS
void RenderingDeviceGraph::_add_buffer_barrier_to_command(RDD::BufferID p_buffer_id,
	uint32_t p_src_access, uint32_t p_dst_access, int32_t& r_barrier_index,
	int32_t& r_barrier_count)
{
	if (!driver_honors_barriers) {
		return;
	}

	if (r_barrier_index < 0) {
		r_barrier_index = command_buffer_barriers.size();
	}

	RDD::BufferBarrier buffer_barrier;
	buffer_barrier.buffer = p_buffer_id;
	buffer_barrier.src_access = p_src_access;
	buffer_barrier.dst_access = p_dst_access;
	buffer_barrier.offset = 0;
	buffer_barrier.size = RDD::BUFFER_WHOLE_SIZE;
	command_buffer_barriers.push_back(buffer_barrier);
	r_barrier_count++;
}
#endif

void RenderingDeviceGraph::_add_acceleration_structure_barrier_to_command(
	RDD::AccelerationStructureID p_acceleration_structure_id, uint32_t p_src_access,
	uint32_t p_dst_access, LocalVector<RDD::AccelerationStructureBarrier>& r_barrier_vector,
	int32_t& r_barrier_index, int32_t& r_barrier_count)
{
	if (!driver_honors_barriers) {
		return;
	}

	if (r_barrier_index < 0) {
		r_barrier_index = r_barrier_vector.size();
	}

	RDD::AccelerationStructureBarrier accel_barrier;
	accel_barrier.acceleration_structure = p_acceleration_structure_id;
	accel_barrier.src_access = p_src_access;
	accel_barrier.dst_access = p_dst_access;
	accel_barrier.offset = 0;
	accel_barrier.size = RDD::BUFFER_WHOLE_SIZE;
	r_barrier_vector.push_back(accel_barrier);
	r_barrier_count++;
}

void RenderingDeviceGraph::_add_command_to_graph(ResourceTracker** p_resource_trackers,
	ResourceUsage* p_resource_usages, uint32_t p_resource_count, int32_t p_command_index,
	RecordedCommand* r_command)
{
	// Assign the next stages derived from the stages the command requires first.
	r_command->next_stages = r_command->self_stages;

	if (command_label_index >= 0) {
		// If a label is active, tag the command with the label.
		r_command->label_index = command_label_index;
	}

	if (r_command->type == RecordedCommand::TYPE_CAPTURE_TIMESTAMP) {
		// All previous commands starting from the previous timestamp should be adjacent to this
		// command.
		int32_t start_command_index = uint32_t(MAX(command_timestamp_index, 0));
		for (int32_t i = start_command_index; i < p_command_index; i++) {
			_add_adjacent_command(i, p_command_index, r_command);
		}

		// Make this command the new active timestamp command.
		command_timestamp_index = p_command_index;
	}
	else if (command_timestamp_index >= 0) {
		// Timestamp command should be adjacent to this command.
		_add_adjacent_command(command_timestamp_index, p_command_index, r_command);
	}

	if (command_synchronization_pending) {
		// All previous commands should be adjacent to this command.
		int32_t start_command_index = uint32_t(MAX(command_synchronization_index, 0));
		for (int32_t i = start_command_index; i < p_command_index; i++) {
			_add_adjacent_command(i, p_command_index, r_command);
		}

		command_synchronization_index = p_command_index;
		command_synchronization_pending = false;
	}
	else if (command_synchronization_index >= 0) {
		// Synchronization command should be adjacent to this command.
		_add_adjacent_command(command_synchronization_index, p_command_index, r_command);
	}

	for (uint32_t i = 0; i < p_resource_count; i++) {
		ResourceTracker* resource_tracker = p_resource_trackers[i];
		DEV_ASSERT(resource_tracker != nullptr);

		resource_tracker->reset_if_outdated(tracking_frame);

		resource_tracker->command_index = p_command_index;
		resource_tracker->usage_index = i;
	}

	for (uint32_t i = 0; i < p_resource_count; i++) {
		ResourceTracker* resource_tracker = p_resource_trackers[i];

		const RDD::TextureSubresourceRange& subresources = resource_tracker->texture_subresources;
		const Rect2i resource_tracker_rect(subresources.base_mipmap, subresources.base_layer,
			subresources.mipmap_count, subresources.layer_count);
		Rect2i search_tracker_rect = resource_tracker_rect;

		ResourceUsage new_resource_usage = p_resource_usages[i];
		bool write_usage = _is_write_usage(new_resource_usage);
		uint32_t new_usage_access = _usage_to_access_bits(new_resource_usage);
		bool is_resource_a_slice = resource_tracker->parent != nullptr;
		if (is_resource_a_slice) {
			// This resource depends on a parent resource.
			resource_tracker->parent->reset_if_outdated(tracking_frame);

			// Quit early if the parent is already in this command.
			if (resource_tracker->parent->command_index == p_command_index) {
				DEV_ASSERT(resource_tracker->parent->usage_index != UINT32_MAX);

				ERR_FAIL_COND_MSG(
					p_resource_usages[resource_tracker->parent->usage_index] != new_resource_usage,
					"Using a full texture and its slices at the same time with different usages is "
					"not allowed.");

				continue;
			}

			if (resource_tracker->texture_slice_command_index != p_command_index) {
				// Indicate this slice has been used by this command.
				resource_tracker->texture_slice_command_index = p_command_index;
			}

			if (resource_tracker->parent->usage == RESOURCE_USAGE_NONE) {
				if (resource_tracker->parent->texture_driver_id.id != 0) {
					// If the resource is a texture, we transition it entirely to the layout
					// determined by the first slice that uses it.
					_add_texture_barrier_to_command(resource_tracker->parent->texture_driver_id, 0,
						new_usage_access, RDG::RESOURCE_USAGE_NONE, new_resource_usage,
						resource_tracker->parent->texture_subresources,
						command_normalization_barriers, r_command->normalization_barrier_index,
						r_command->normalization_barrier_count);
				}

				// If the parent hasn't been used yet, we assign the usage of the slice to the
				// entire resource.
				resource_tracker->parent->usage = new_resource_usage;

				// Also assign the usage to the slice and consider it a write operation. Consider
				// the parent's current usage access as its own.
				resource_tracker->usage = new_resource_usage;
				resource_tracker->usage_access = resource_tracker->parent->usage_access;
				write_usage = true;

				// Indicate the area that should be tracked is the entire resource.
				const RDD::TextureSubresourceRange& parent_subresources =
					resource_tracker->parent->texture_subresources;
				search_tracker_rect = Rect2i(parent_subresources.base_mipmap,
					parent_subresources.base_layer, parent_subresources.mipmap_count,
					parent_subresources.layer_count);
			}
			else if (resource_tracker->in_parent_dirty_list) {
				if (resource_tracker->parent->usage == new_resource_usage) {
					// The slice will be transitioned to the resource of the parent and can be
					// deleted from the dirty list.
					ResourceTracker* previous_tracker = nullptr;
					ResourceTracker* current_tracker = resource_tracker->parent->dirty_shared_list;
					bool initialized_dirty_rect = false;
					while (current_tracker != nullptr) {
						current_tracker->reset_if_outdated(tracking_frame);

						if (current_tracker == resource_tracker) {
							current_tracker->in_parent_dirty_list = false;

							if (previous_tracker != nullptr) {
								previous_tracker->next_shared = current_tracker->next_shared;
							}
							else {
								resource_tracker->parent->dirty_shared_list =
									current_tracker->next_shared;
							}

							current_tracker = current_tracker->next_shared;
						}
						else {
							if (initialized_dirty_rect) {
								resource_tracker->parent->texture_slice_or_dirty_rect =
									resource_tracker->parent->texture_slice_or_dirty_rect.merge(
										current_tracker->texture_slice_or_dirty_rect);
							}
							else {
								resource_tracker->parent->texture_slice_or_dirty_rect =
									current_tracker->texture_slice_or_dirty_rect;
								initialized_dirty_rect = true;
							}

							previous_tracker = current_tracker;
							current_tracker = current_tracker->next_shared;
						}
					}
				}
			}
			else {
				if (resource_tracker->parent->dirty_shared_list != nullptr &&
					resource_tracker->parent->texture_slice_or_dirty_rect.intersects(
						resource_tracker->texture_slice_or_dirty_rect)) {
					// There's an intersection with the current dirty area of the parent and the
					// slice. We must verify if the intersection is against a slice that was used in
					// this command or not. Any slice we can find that wasn't used by this command
					// must be reverted to the layout of the parent.
					ResourceTracker* previous_tracker = nullptr;
					ResourceTracker* current_tracker = resource_tracker->parent->dirty_shared_list;
					bool initialized_dirty_rect = false;
					while (current_tracker != nullptr) {
						current_tracker->reset_if_outdated(tracking_frame);

						if (current_tracker->texture_slice_or_dirty_rect.intersects(
								resource_tracker->texture_slice_or_dirty_rect)) {
							if (current_tracker->command_frame == tracking_frame &&
								current_tracker->texture_slice_command_index == p_command_index) {
								ERR_FAIL_MSG(
									"Texture slices that overlap can't be used in the same "
									"command.");
							}
							else {
								// Delete the slice from the dirty list and revert it to the usage
								// of the parent.
								if (current_tracker->texture_driver_id.id != 0) {
									_add_texture_barrier_to_command(
										current_tracker->texture_driver_id,
										current_tracker->usage_access, new_usage_access,
										current_tracker->usage, resource_tracker->parent->usage,
										current_tracker->texture_subresources,
										command_normalization_barriers,
										r_command->normalization_barrier_index,
										r_command->normalization_barrier_count);

									// Merge the area of the slice with the current tracking area of
									// the command and indicate it's a write usage as well.
									search_tracker_rect = search_tracker_rect.merge(
										current_tracker->texture_slice_or_dirty_rect);
									write_usage = true;
								}

								current_tracker->in_parent_dirty_list = false;

								if (previous_tracker != nullptr) {
									previous_tracker->next_shared = current_tracker->next_shared;
								}
								else {
									resource_tracker->parent->dirty_shared_list =
										current_tracker->next_shared;
								}

								current_tracker = current_tracker->next_shared;
							}
						}
						else {
							// Recalculate the dirty rect of the parent so the deleted slices are
							// excluded.
							if (initialized_dirty_rect) {
								resource_tracker->parent->texture_slice_or_dirty_rect =
									resource_tracker->parent->texture_slice_or_dirty_rect.merge(
										current_tracker->texture_slice_or_dirty_rect);
							}
							else {
								resource_tracker->parent->texture_slice_or_dirty_rect =
									current_tracker->texture_slice_or_dirty_rect;
								initialized_dirty_rect = true;
							}

							previous_tracker = current_tracker;
							current_tracker = current_tracker->next_shared;
						}
					}
				}

				// If it wasn't in the list, assume the usage is the same as the parent. Consider
				// the parent's current usage access as its own.
				resource_tracker->usage = resource_tracker->parent->usage;
				resource_tracker->usage_access = resource_tracker->parent->usage_access;

				if (resource_tracker->usage != new_resource_usage) {
					// Insert to the dirty list if the requested usage is different.
					resource_tracker->next_shared = resource_tracker->parent->dirty_shared_list;
					resource_tracker->parent->dirty_shared_list = resource_tracker;
					resource_tracker->in_parent_dirty_list = true;
					if (resource_tracker->parent->dirty_shared_list != nullptr) {
						resource_tracker->parent->texture_slice_or_dirty_rect =
							resource_tracker->parent->texture_slice_or_dirty_rect.merge(
								resource_tracker->texture_slice_or_dirty_rect);
					}
					else {
						resource_tracker->parent->texture_slice_or_dirty_rect =
							resource_tracker->texture_slice_or_dirty_rect;
					}
				}
			}
		}
		else {
			ResourceTracker* current_tracker = resource_tracker->dirty_shared_list;
			if (current_tracker != nullptr) {
				// Consider the usage as write if we must transition any of the slices.
				write_usage = true;
			}

			while (current_tracker != nullptr) {
				current_tracker->reset_if_outdated(tracking_frame);

				if (current_tracker->texture_driver_id.id != 0) {
					// Transition all slices to the layout of the parent resource.
					_add_texture_barrier_to_command(current_tracker->texture_driver_id,
						current_tracker->usage_access, new_usage_access, current_tracker->usage,
						resource_tracker->usage, current_tracker->texture_subresources,
						command_normalization_barriers, r_command->normalization_barrier_index,
						r_command->normalization_barrier_count);
				}

				current_tracker->in_parent_dirty_list = false;
				current_tracker = current_tracker->next_shared;
			}

			resource_tracker->dirty_shared_list = nullptr;
		}

		// Use the resource's parent tracker directly for all search operations.
		bool resource_has_parent = resource_tracker->parent != nullptr;
		ResourceTracker* search_tracker =
			resource_has_parent ? resource_tracker->parent : resource_tracker;
		bool different_usage = resource_tracker->usage != new_resource_usage;
		bool write_usage_after_write =
			(write_usage && search_tracker->write_command_or_list_index >= 0);
		if (different_usage || write_usage_after_write) {
			// A barrier must be pushed if the usage is different or it's a write usage and there
			// was already a command that wrote to this resource previously.
			if (resource_tracker->texture_driver_id.id != 0) {
				if (resource_tracker->usage_access == 0) {
					resource_tracker->usage_access = RDD::BARRIER_ACCESS_MEMORY_WRITE_BIT;
				}

				_add_texture_barrier_to_command(resource_tracker->texture_driver_id,
					resource_tracker->usage_access, new_usage_access, resource_tracker->usage,
					new_resource_usage, resource_tracker->texture_subresources,
					command_transition_barriers, r_command->transition_barrier_index,
					r_command->transition_barrier_count);
			}
			else if (resource_tracker->buffer_driver_id.id != 0) {
#if USE_BUFFER_BARRIERS
				_add_buffer_barrier_to_command(resource_tracker->buffer_driver_id,
					resource_tracker->usage_access, new_usage_access,
					r_command->buffer_barrier_index, r_command->buffer_barrier_count);
#endif
				// Memory barriers are pushed regardless of buffer barriers being used or not.
				r_command->memory_barrier.src_access =
					r_command->memory_barrier.src_access | resource_tracker->usage_access;
				r_command->memory_barrier.dst_access =
					r_command->memory_barrier.dst_access | new_usage_access;
			}
			else if (resource_tracker->acceleration_structure_driver_id.id != 0) {
				// Make sure the acceleration structure has been built before accessing it from
				// raytracing shaders.
				_add_acceleration_structure_barrier_to_command(
					resource_tracker->acceleration_structure_driver_id,
					resource_tracker->usage_access, new_usage_access,
					command_acceleration_structure_barriers,
					r_command->acceleration_structure_barrier_index,
					r_command->acceleration_structure_barrier_count);
				r_command->memory_barrier.src_access =
					r_command->memory_barrier.src_access | resource_tracker->usage_access;
				r_command->memory_barrier.dst_access =
					r_command->memory_barrier.dst_access | new_usage_access;
			}
			else {
				DEV_ASSERT(false && "Resource tracker does not contain a valid buffer or texture ID.");
			}
		}

		// Always update the access of the tracker according to the latest usage.
		resource_tracker->usage_access = new_usage_access;

		// Always accumulate the stages of the tracker with the commands that use it.
		search_tracker->current_frame_stages =
			search_tracker->current_frame_stages | r_command->self_stages;

		if (search_tracker->previous_frame_stages != 0) {
			// Add to the command the stages the tracker was used on in the previous frame.
			r_command->previous_stages =
				r_command->previous_stages | search_tracker->previous_frame_stages;
			search_tracker->previous_frame_stages = 0;
		}

		if (different_usage) {
			// Even if the usage of the resource isn't a write usage explicitly, a different usage
			// implies a transition and it should therefore be considered a write.
			write_usage = write_usage || (resource_tracker->texture_driver_id.id != 0) ||
						  driver_buffers_require_transitions;
			resource_tracker->usage = new_resource_usage;
		}

		bool write_usage_has_partial_coverage =
			!different_usage && _check_command_partial_coverage(resource_tracker, p_command_index);
		if (search_tracker->write_command_or_list_index >= 0) {
			if (search_tracker->write_command_list_enabled) {
				// Make this command adjacent to any commands that wrote to this resource and
				// intersect with the slice if it applies.
				int32_t previous_write_list_index = -1;
				int32_t write_list_index = search_tracker->write_command_or_list_index;
				while (write_list_index >= 0) {
					const RecordedSliceListNode& write_list_node =
						write_slice_list_nodes[write_list_index];
					if (!resource_has_parent ||
						search_tracker_rect.intersects(write_list_node.subresources)) {
						if (write_list_node.command_index == p_command_index) {
							ERR_FAIL_COND_MSG(
								!resource_has_parent, "Command can't have itself as a dependency.");
						}
						else if (!write_list_node.partial_coverage ||
								 _check_command_intersection(resource_tracker,
									 write_list_node.command_index, p_command_index)) {
							_check_discardable_attachment_dependency(
								search_tracker, write_list_node.command_index, p_command_index);

							// Command is dependent on this command. Add this command to the
							// adjacency list of the write command.
							_add_adjacent_command(
								write_list_node.command_index, p_command_index, r_command);

							if (resource_has_parent && write_usage &&
								search_tracker_rect.encloses(write_list_node.subresources) &&
								!write_usage_has_partial_coverage) {
								// Eliminate redundant writes from the list.
								if (previous_write_list_index >= 0) {
									RecordedSliceListNode& previous_list_node =
										write_slice_list_nodes[previous_write_list_index];
									previous_list_node.next_list_index =
										write_list_node.next_list_index;
								}
								else {
									search_tracker->write_command_or_list_index =
										write_list_node.next_list_index;
								}

								write_list_index = write_list_node.next_list_index;
								continue;
							}
						}
					}

					previous_write_list_index = write_list_index;
					write_list_index = write_list_node.next_list_index;
				}
			}
			else {
				// The index is just the latest command index that wrote to the resource.
				if (search_tracker->write_command_or_list_index == p_command_index) {
					ERR_FAIL_MSG("Command can't have itself as a dependency.");
				}
				else {
					_check_discardable_attachment_dependency(search_tracker,
						search_tracker->write_command_or_list_index, p_command_index);
					_add_adjacent_command(
						search_tracker->write_command_or_list_index, p_command_index, r_command);
				}
			}
		}

		if (write_usage) {
			bool use_write_list = resource_has_parent || write_usage_has_partial_coverage;
			if (use_write_list) {
				if (!search_tracker->write_command_list_enabled &&
					search_tracker->write_command_or_list_index >= 0) {
					const RDD::TextureSubresourceRange& tracker_subresources =
						search_tracker->texture_subresources;
					Rect2i tracker_rect(tracker_subresources.base_mipmap,
						tracker_subresources.base_layer, tracker_subresources.mipmap_count,
						tracker_subresources.layer_count);
					search_tracker->write_command_or_list_index = _add_to_write_list(
						search_tracker->write_command_or_list_index, tracker_rect, -1, false);
				}

				search_tracker->write_command_or_list_index =
					_add_to_write_list(p_command_index, search_tracker_rect,
						search_tracker->write_command_or_list_index, write_usage_has_partial_coverage);
				search_tracker->write_command_list_enabled = true;
			}
			else {
				search_tracker->write_command_or_list_index = p_command_index;
				search_tracker->write_command_list_enabled = false;
			}

			// We add this command to the adjacency list of all commands that were reading from the
			// entire resource.
			int32_t read_full_command_list_index = search_tracker->read_full_command_list_index;
			while (read_full_command_list_index >= 0) {
				int32_t read_full_command_index =
					command_list_nodes[read_full_command_list_index].command_index;
				int32_t read_full_next_index =
					command_list_nodes[read_full_command_list_index].next_list_index;
				if (read_full_command_index == p_command_index) {
					if (!resource_has_parent) {
						ERR_FAIL_MSG("Command can't have itself as a dependency.");
					}
				}
				else {
					_add_adjacent_command(read_full_command_index, p_command_index, r_command);
				}

				read_full_command_list_index = read_full_next_index;
			}

			if (!use_write_list) {
				search_tracker->read_full_command_list_index = -1;
			}

			// We add this command to the adjacency list of all commands that were reading from
			// resource slices.
			int32_t previous_slice_command_list_index = -1;
			int32_t read_slice_command_list_index = search_tracker->read_slice_command_list_index;
			while (read_slice_command_list_index >= 0) {
				const RecordedSliceListNode& read_list_node =
					read_slice_list_nodes[read_slice_command_list_index];
				if (!use_write_list || search_tracker_rect.encloses(read_list_node.subresources)) {
					if (previous_slice_command_list_index >= 0) {
						read_slice_list_nodes[previous_slice_command_list_index].next_list_index =
							read_list_node.next_list_index;
					}
					else {
						DEV_ASSERT(search_tracker->read_slice_command_list_index ==
								   read_slice_command_list_index);
						search_tracker->read_slice_command_list_index =
							read_list_node.next_list_index;
					}

					read_slice_command_list_index = read_list_node.next_list_index;
				}
				else {
					previous_slice_command_list_index = read_slice_command_list_index;
					read_slice_command_list_index = read_list_node.next_list_index;
				}

				if (!resource_has_parent ||
					search_tracker_rect.intersects(read_list_node.subresources)) {
					_add_adjacent_command(read_list_node.command_index, p_command_index, r_command);
				}
			}
		}
		else if (resource_has_parent) {
			search_tracker->read_slice_command_list_index = _add_to_slice_read_list(
				p_command_index, resource_tracker_rect, search_tracker->read_slice_command_list_index);
		}
		else {
			search_tracker->read_full_command_list_index = _add_to_command_list(
				p_command_index, search_tracker->read_full_command_list_index);
		}
	}
}

void RenderingDeviceGraph::_group_barriers_for_render_commands(
	RDD::CommandBufferID p_command_buffer, const RecordedCommandSort* p_sorted_commands,
	uint32_t p_sorted_commands_count, bool p_full_memory_barrier)
{
	if (!driver_honors_barriers) {
		return;
	}

	barrier_group.clear();
	barrier_group.src_stages = RDD::PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	barrier_group.dst_stages = RDD::PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	for (uint32_t i = 0; i < p_sorted_commands_count; i++) {
		const uint32_t command_index = p_sorted_commands[i].index;
		const uint32_t command_data_offset = command_data_offsets[command_index];
		const RecordedCommand* command =
			reinterpret_cast<RecordedCommand*>(&command_data[command_data_offset]);

#if PRINT_COMMAND_RECORDING
		print_line(vformat("Grouping barriers for #%d", command_index));
#endif

		// Merge command's stage bits with the barrier group.
		barrier_group.src_stages = barrier_group.src_stages | command->previous_stages;
		barrier_group.dst_stages = barrier_group.dst_stages | command->next_stages;

		// Merge command's memory barrier bits with the barrier group.
		barrier_group.memory_barrier.src_access =
			barrier_group.memory_barrier.src_access | command->memory_barrier.src_access;
		barrier_group.memory_barrier.dst_access =
			barrier_group.memory_barrier.dst_access | command->memory_barrier.dst_access;

		// Gather texture barriers.
		for (int32_t j = 0; j < command->normalization_barrier_count; j++) {
			const RDD::TextureBarrier& recorded_barrier =
				command_normalization_barriers[command->normalization_barrier_index + j];
			barrier_group.normalization_barriers.push_back(recorded_barrier);
#if PRINT_COMMAND_RECORDING
			print_line(vformat(
				"Normalization Barrier #%d", barrier_group.normalization_barriers.size() - 1));
#endif
		}

		for (int32_t j = 0; j < command->transition_barrier_count; j++) {
			const RDD::TextureBarrier& recorded_barrier =
				command_transition_barriers[command->transition_barrier_index + j];
			barrier_group.transition_barriers.push_back(recorded_barrier);
#if PRINT_COMMAND_RECORDING
			print_line(
				vformat("Transition Barrier #%d", barrier_group.transition_barriers.size() - 1));
#endif
		}

#if USE_BUFFER_BARRIERS
		// Gather buffer barriers.
		for (int32_t j = 0; j < command->buffer_barrier_count; j++) {
			const RDD::BufferBarrier& recorded_barrier =
				command_buffer_barriers[command->buffer_barrier_index + j];
			barrier_group.buffer_barriers.push_back(recorded_barrier);
		}
#endif

		// Gather acceleration structure barriers.
		for (int32_t j = 0; j < command->acceleration_structure_barrier_count; j++) {
			const RDD::AccelerationStructureBarrier& recorded_barrier =
				command_acceleration_structure_barriers
					[command->acceleration_structure_barrier_index + j];
			barrier_group.acceleration_structure_barriers.push_back(recorded_barrier);
		}
	}

	if (p_full_memory_barrier) {
		barrier_group.src_stages = RDD::PIPELINE_STAGE_ALL_COMMANDS_BIT;
		barrier_group.dst_stages = RDD::PIPELINE_STAGE_ALL_COMMANDS_BIT;
		barrier_group.memory_barrier.src_access =
			RDD::BARRIER_ACCESS_MEMORY_READ_BIT | RDD::BARRIER_ACCESS_MEMORY_WRITE_BIT;
		barrier_group.memory_barrier.dst_access =
			RDD::BARRIER_ACCESS_MEMORY_READ_BIT | RDD::BARRIER_ACCESS_MEMORY_WRITE_BIT;
	}

	const bool is_memory_barrier_empty = (barrier_group.memory_barrier.src_access == 0) &&
										 (barrier_group.memory_barrier.dst_access == 0);
	const bool are_texture_barriers_empty =
		barrier_group.normalization_barriers.is_empty() &&
		barrier_group.transition_barriers.is_empty();
#if USE_BUFFER_BARRIERS
	const bool are_buffer_barriers_empty = barrier_group.buffer_barriers.is_empty();
#else
	const bool are_buffer_barriers_empty = true;
#endif
	const bool are_acceleration_structure_barriers_empty =
		barrier_group.acceleration_structure_barriers.is_empty();
	if (is_memory_barrier_empty && are_texture_barriers_empty && are_buffer_barriers_empty &&
		are_acceleration_structure_barriers_empty) {
		// Commands don't require synchronization.
		return;
	}

	const VectorView<RDD::MemoryAccessBarrier> memory_barriers =
		!is_memory_barrier_empty ? barrier_group.memory_barrier
								 : VectorView<RDD::MemoryAccessBarrier>();
	const VectorView<RDD::TextureBarrier> texture_barriers =
		barrier_group.normalization_barriers.is_empty() ? barrier_group.transition_barriers
														: barrier_group.normalization_barriers;
#if USE_BUFFER_BARRIERS
	const VectorView<RDD::BufferBarrier> buffer_barriers =
		!are_buffer_barriers_empty ? barrier_group.buffer_barriers
								   : VectorView<RDD::BufferBarrier>();
#else
	const VectorView<RDD::BufferBarrier> buffer_barriers = VectorView<RDD::BufferBarrier>();
#endif
	const VectorView<RDD::AccelerationStructureBarrier> acceleration_structure_barriers =
		!are_acceleration_structure_barriers_empty
			? barrier_group.acceleration_structure_barriers
			: VectorView<RDD::AccelerationStructureBarrier>();

	driver->command_pipeline_barrier(p_command_buffer, barrier_group.src_stages,
		barrier_group.dst_stages, memory_barriers, buffer_barriers, texture_barriers,
		acceleration_structure_barriers);

	bool separate_texture_barriers = !barrier_group.normalization_barriers.is_empty() &&
									 !barrier_group.transition_barriers.is_empty();
	if (separate_texture_barriers) {
		driver->command_pipeline_barrier(p_command_buffer, barrier_group.src_stages,
			barrier_group.dst_stages, VectorView<RDD::MemoryAccessBarrier>(),
			VectorView<RDD::BufferBarrier>(), barrier_group.transition_barriers,
			VectorView<RDD::AccelerationStructureBarrier>());
	}
}

void RenderingDeviceGraph::_wait_for_secondary_command_buffer_tasks() {}

void RenderingDeviceGraph::initialize(RDD* p_driver,
	RenderPassCreationFunction p_render_pass_creation_function, uint32_t p_frame_count,
	RDD::CommandQueueFamilyID p_secondary_command_queue_family,
	uint32_t p_secondary_command_buffers_per_frame)
{
	driver = p_driver;
	driver_workarounds = p_driver->get_driver_workarounds();
	render_pass_creation_function = p_render_pass_creation_function;
	tracking_frame = 0;

	frames.resize(p_frame_count);

	if (p_secondary_command_buffers_per_frame > 0) {
		for (uint32_t i = 0; i < p_frame_count; i++) {
			frames[i].secondary_command_buffers.resize(p_secondary_command_buffers_per_frame);
			frames[i].secondary_command_buffers_used = 0;

			for (uint32_t j = 0; j < p_secondary_command_buffers_per_frame; j++) {
				frames[i].secondary_command_buffers[j].command_pool = driver->command_pool_create(
					p_secondary_command_queue_family, RDD::COMMAND_BUFFER_TYPE_SECONDARY);
				frames[i].secondary_command_buffers[j].command_buffer =
					driver->command_buffer_create(
						frames[i].secondary_command_buffers[j].command_pool);
			}
		}
	}

	driver_honors_barriers = driver->api_trait_get(RDD::API_TRAIT_HONORS_PIPELINE_BARRIERS);
	driver_clears_with_copy_engine = driver->api_trait_get(RDD::API_TRAIT_CLEARS_WITH_COPY_ENGINE);
	driver_buffers_require_transitions =
		driver->api_trait_get(RDD::API_TRAIT_BUFFERS_REQUIRE_TRANSITIONS);
}
