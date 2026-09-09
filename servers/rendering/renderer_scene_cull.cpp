/**************************************************************************/
/*  renderer_scene_cull.cpp                                               */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/geometry_3d.h"
#include "renderer_scene_cull.h"
#include "servers/rendering/rendering_light_culler.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/rendering_server_default.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#endif

// #define DEBUG_CULL_TIME
#ifdef DEBUG_CULL_TIME
#include "core/os/os.h"
#endif

/* HALTON SEQUENCE */

#ifndef _3D_DISABLED
static float get_halton_value(int p_index, int p_base)
{
	float f = 1;
	float r = 0;
	while (p_index > 0) {
		f = f / static_cast<float>(p_base);
		r = r + f * (p_index % p_base);
		p_index = p_index / p_base;
	}
	return r * 2.0f - 1.0f;
}
#endif // _3D_DISABLED

/* EVENT QUEUING */

void RendererSceneCull::tick()
{
	if (_interpolation_data.interpolation_enabled) {
		update_interpolation_tick(true);
	}
}

void RendererSceneCull::pre_draw(bool p_will_draw)
{
	if (_interpolation_data.interpolation_enabled) {
		update_interpolation_frame(p_will_draw);
	}
}

/* CAMERA API */

RID RendererSceneCull::camera_allocate() { return camera_owner.allocate_rid(); }

void RendererSceneCull::camera_initialize(RID p_rid) { camera_owner.initialize_rid(p_rid); }

void RendererSceneCull::camera_set_perspective(
	RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);
	camera->type = Camera::PERSPECTIVE;
	camera->fov = p_fovy_degrees;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RendererSceneCull::camera_set_orthogonal(
	RID p_camera, float p_size, float p_z_near, float p_z_far)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);
	camera->type = Camera::ORTHOGONAL;
	camera->size = p_size;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RendererSceneCull::camera_set_frustum(
	RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);
	camera->type = Camera::FRUSTUM;
	camera->size = p_size;
	camera->offset = p_offset;
	camera->znear = p_z_near;
	camera->zfar = p_z_far;
}

void RendererSceneCull::camera_set_transform(RID p_camera, const Transform3D& p_transform)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);

	camera->transform = p_transform.orthonormalized();
}

void RendererSceneCull::camera_set_cull_mask(RID p_camera, uint32_t p_layers)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);

	camera->visible_layers = p_layers;
}

void RendererSceneCull::camera_set_environment(RID p_camera, RID p_env)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);
	camera->env = p_env;
}

void RendererSceneCull::camera_set_camera_attributes(RID p_camera, RID p_attributes)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);
	camera->attributes = p_attributes;
}

void RendererSceneCull::camera_set_compositor(RID p_camera, RID p_compositor)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);
	camera->compositor = p_compositor;
}

void RendererSceneCull::camera_set_use_vertical_aspect(RID p_camera, bool p_enable)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	ERR_FAIL_NULL(camera);
	camera->vaspect = p_enable;
}

bool RendererSceneCull::is_camera(RID p_camera) const { return camera_owner.owns(p_camera); }

/* OCCLUDER API */

RID RendererSceneCull::occluder_allocate()
{
	return RendererSceneOcclusionCull::get_singleton()->occluder_allocate();
}

void RendererSceneCull::occluder_initialize(RID p_rid)
{
	RendererSceneOcclusionCull::get_singleton()->occluder_initialize(p_rid);
}

void RendererSceneCull::occluder_set_mesh(
	RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices)
{
	RendererSceneOcclusionCull::get_singleton()->occluder_set_mesh(
		p_occluder, p_vertices, p_indices);
}

/* SCENARIO API */

void RendererSceneCull::_instance_pair(Instance* p_A, Instance* p_B)
{
	RendererSceneCull* self = (RendererSceneCull*)singleton;
	Instance* A = p_A;
	Instance* B = p_B;

	// instance indices are designed so greater always contains lesser
	if (A->base_type > B->base_type) {
		SWAP(A, B); // lesser always first
	}

	if (B->base_type == RSE::INSTANCE_LIGHT &&
		((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightData* light = static_cast<InstanceLightData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		if (!(light->cull_mask & A->layer_mask)) {
			// Early return if the object's layer mask doesn't match the light's cull mask.
			return;
		}

		geom->lights.insert(B);
		light->geometries.insert(A);

		if (geom->can_cast_shadows) {
			light->make_shadow_dirty();
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_LIGHTING_DIRTY;
		}

		if (light->uses_projector) {
			geom->projector_count++;
			if (geom->projector_count == 1) {
				InstanceData& idata = A->scenario->instance_data[A->array_index];
				idata.flags |= InstanceData::FLAG_GEOM_PROJECTOR_SOFTSHADOW_DIRTY;
			}
		}

		if (light->uses_softshadow) {
			geom->softshadow_count++;
			if (geom->softshadow_count == 1) {
				InstanceData& idata = A->scenario->instance_data[A->array_index];
				idata.flags |= InstanceData::FLAG_GEOM_PROJECTOR_SOFTSHADOW_DIRTY;
			}
		}

	}
	else if (self->geometry_instance_pair_mask & (1 << RSE::INSTANCE_REFLECTION_PROBE) &&
			   B->base_type == RSE::INSTANCE_REFLECTION_PROBE &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		if (!(A->layer_mask & RSG::light_storage->reflection_probe_get_reflection_mask(B->base))) {
			// Early return if the object's layer mask doesn't match the reflection mask.
			return;
		}

		InstanceReflectionProbeData* reflection_probe =
			static_cast<InstanceReflectionProbeData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		geom->reflection_probes.insert(B);
		reflection_probe->geometries.insert(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_REFLECTION_DIRTY;
		}

	}
	else if (self->geometry_instance_pair_mask & (1 << RSE::INSTANCE_DECAL) &&
			   B->base_type == RSE::INSTANCE_DECAL &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceDecalData* decal = static_cast<InstanceDecalData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		if (!(decal->cull_mask & A->layer_mask)) {
			// Early return if the object's layer mask doesn't match the decal's cull mask.
			return;
		}

		geom->decals.insert(B);
		decal->geometries.insert(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_DECAL_DIRTY;
		}

	}
	else if (B->base_type == RSE::INSTANCE_LIGHTMAP &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightmapData* lightmap_data = static_cast<InstanceLightmapData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		if (A->dynamic_gi) {
			geom->lightmap_captures.insert(B);
			lightmap_data->geometries.insert(A);

			if (A->scenario && A->array_index >= 0) {
				InstanceData& idata = A->scenario->instance_data[A->array_index];
				idata.flags |= InstanceData::FLAG_LIGHTMAP_CAPTURE;
			}
			((RendererSceneCull*)self)
				->_instance_queue_update(A, false, false); // need to update capture
		}

	}
	else if (self->geometry_instance_pair_mask & (1 << RSE::INSTANCE_VOXEL_GI) &&
			   B->base_type == RSE::INSTANCE_VOXEL_GI &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		geom->voxel_gi_instances.insert(B);

		if (A->dynamic_gi) {
			voxel_gi->dynamic_geometries.insert(A);
		}
		else {
			voxel_gi->geometries.insert(A);
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_VOXEL_GI_DIRTY;
		}

	}
	else if (B->base_type == RSE::INSTANCE_VOXEL_GI && A->base_type == RSE::INSTANCE_LIGHT) {
		InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(B->base_data);
		voxel_gi->lights.insert(A);
	}
	else if (B->base_type == RSE::INSTANCE_PARTICLES_COLLISION &&
			   A->base_type == RSE::INSTANCE_PARTICLES) {
		InstanceParticlesCollisionData* collision =
			static_cast<InstanceParticlesCollisionData*>(B->base_data);

		if ((collision->cull_mask & A->layer_mask)) {
			RSG::particles_storage->particles_add_collision(A->base, collision->instance);
		}
	}
}

void RendererSceneCull::_instance_unpair(Instance* p_A, Instance* p_B)
{
	RendererSceneCull* self = singleton;
	Instance* A = p_A;
	Instance* B = p_B;

	// instance indices are designed so greater always contains lesser
	if (A->base_type > B->base_type) {
		SWAP(A, B); // lesser always first
	}

	if (B->base_type == RSE::INSTANCE_LIGHT &&
		((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightData* light = static_cast<InstanceLightData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		if (!(light->cull_mask & A->layer_mask)) {
			// Early return if the object's layer mask doesn't match the light's cull mask.
			return;
		}

		geom->lights.erase(B);
		light->geometries.erase(A);

		if (geom->can_cast_shadows) {
			light->make_shadow_dirty();
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_LIGHTING_DIRTY;
		}

		if (light->uses_projector) {
#ifdef DEBUG_ENABLED
			if (geom->projector_count == 0) {
				ERR_PRINT("geom->projector_count==0 - BUG!");
			}
#endif
			geom->projector_count--;
			if (geom->projector_count == 0) {
				InstanceData& idata = A->scenario->instance_data[A->array_index];
				idata.flags |= InstanceData::FLAG_GEOM_PROJECTOR_SOFTSHADOW_DIRTY;
			}
		}

		if (light->uses_softshadow) {
#ifdef DEBUG_ENABLED
			if (geom->softshadow_count == 0) {
				ERR_PRINT("geom->softshadow_count==0 - BUG!");
			}
#endif
			geom->softshadow_count--;
			if (geom->softshadow_count == 0) {
				InstanceData& idata = A->scenario->instance_data[A->array_index];
				idata.flags |= InstanceData::FLAG_GEOM_PROJECTOR_SOFTSHADOW_DIRTY;
			}
		}

	}
	else if (self->geometry_instance_pair_mask & (1 << RSE::INSTANCE_REFLECTION_PROBE) &&
			   B->base_type == RSE::INSTANCE_REFLECTION_PROBE &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceReflectionProbeData* reflection_probe =
			static_cast<InstanceReflectionProbeData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		geom->reflection_probes.erase(B);
		reflection_probe->geometries.erase(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_REFLECTION_DIRTY;
		}

	}
	else if (self->geometry_instance_pair_mask & (1 << RSE::INSTANCE_DECAL) &&
			   B->base_type == RSE::INSTANCE_DECAL &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceDecalData* decal = static_cast<InstanceDecalData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		if (!(decal->cull_mask & A->layer_mask)) {
			// Early return if the object's layer mask doesn't match the decal's cull mask.
			return;
		}

		geom->decals.erase(B);
		decal->geometries.erase(A);

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_DECAL_DIRTY;
		}

	}
	else if (B->base_type == RSE::INSTANCE_LIGHTMAP &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceLightmapData* lightmap_data = static_cast<InstanceLightmapData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);
		if (A->dynamic_gi) {
			geom->lightmap_captures.erase(B);

			if (geom->lightmap_captures.is_empty() && A->scenario && A->array_index >= 0) {
				InstanceData& idata = A->scenario->instance_data[A->array_index];
				idata.flags &= ~InstanceData::FLAG_LIGHTMAP_CAPTURE;
			}

			lightmap_data->geometries.erase(A);
			self->_instance_queue_update(A, false, false); // need to update capture
		}

	}
	else if (self->geometry_instance_pair_mask & (1 << RSE::INSTANCE_VOXEL_GI) &&
			   B->base_type == RSE::INSTANCE_VOXEL_GI &&
			   ((1 << A->base_type) & RSE::INSTANCE_GEOMETRY_MASK)) {
		InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(B->base_data);
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(A->base_data);

		geom->voxel_gi_instances.erase(B);
		if (A->dynamic_gi) {
			voxel_gi->dynamic_geometries.erase(A);
		}
		else {
			voxel_gi->geometries.erase(A);
		}

		if (A->scenario && A->array_index >= 0) {
			InstanceData& idata = A->scenario->instance_data[A->array_index];
			idata.flags |= InstanceData::FLAG_GEOM_VOXEL_GI_DIRTY;
		}

	}
	else if (B->base_type == RSE::INSTANCE_VOXEL_GI && A->base_type == RSE::INSTANCE_LIGHT) {
		InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(B->base_data);
		voxel_gi->lights.erase(A);
	}
	else if (B->base_type == RSE::INSTANCE_PARTICLES_COLLISION &&
			   A->base_type == RSE::INSTANCE_PARTICLES) {
		InstanceParticlesCollisionData* collision =
			static_cast<InstanceParticlesCollisionData*>(B->base_data);

		if ((collision->cull_mask & A->layer_mask)) {
			RSG::particles_storage->particles_remove_collision(A->base, collision->instance);
		}
	}
}

RID RendererSceneCull::scenario_allocate() { return scenario_owner.allocate_rid(); }

void RendererSceneCull::scenario_initialize(RID p_rid)
{
	scenario_owner.initialize_rid(p_rid);

	Scenario* scenario = scenario_owner.get_or_null(p_rid);
	scenario->self = p_rid;

	scenario->reflection_probe_shadow_atlas = RSG::light_storage->shadow_atlas_create();
	RSG::light_storage->shadow_atlas_set_size(scenario->reflection_probe_shadow_atlas,
		1024); // make enough shadows for close distance, don't bother with rest
	RSG::light_storage->shadow_atlas_set_quadrant_subdivision(
		scenario->reflection_probe_shadow_atlas, 0, 4);
	RSG::light_storage->shadow_atlas_set_quadrant_subdivision(
		scenario->reflection_probe_shadow_atlas, 1, 4);
	RSG::light_storage->shadow_atlas_set_quadrant_subdivision(
		scenario->reflection_probe_shadow_atlas, 2, 4);
	RSG::light_storage->shadow_atlas_set_quadrant_subdivision(
		scenario->reflection_probe_shadow_atlas, 3, 8);

	scenario->reflection_atlas = RSG::light_storage->reflection_atlas_create();

	scenario->instance_aabbs.set_page_pool(&instance_aabb_page_pool);
	scenario->instance_data.set_page_pool(&instance_data_page_pool);
	scenario->instance_visibility.set_page_pool(&instance_visibility_data_page_pool);

	RendererSceneOcclusionCull::get_singleton()->add_scenario(p_rid);
}

void RendererSceneCull::scenario_set_environment(RID p_scenario, RID p_environment)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL(scenario);
	scenario->environment = p_environment;
}

void RendererSceneCull::scenario_set_camera_attributes(RID p_scenario, RID p_camera_attributes)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL(scenario);
	scenario->camera_attributes = p_camera_attributes;
}

void RendererSceneCull::scenario_set_compositor(RID p_scenario, RID p_compositor)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL(scenario);
	scenario->compositor = p_compositor;
}

void RendererSceneCull::scenario_set_fallback_environment(RID p_scenario, RID p_environment)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL(scenario);
	scenario->fallback_environment = p_environment;
}

void RendererSceneCull::scenario_set_reflection_atlas_size(
	RID p_scenario, int p_reflection_size, int p_reflection_count)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL(scenario);
	RSG::light_storage->reflection_atlas_set_size(
		scenario->reflection_atlas, p_reflection_size, p_reflection_count);
}

bool RendererSceneCull::is_scenario(RID p_scenario) const
{
	return scenario_owner.owns(p_scenario);
}

RID RendererSceneCull::scenario_get_environment(RID p_scenario)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL_V(scenario, RID());
	return scenario->environment;
}

void RendererSceneCull::scenario_remove_viewport_visibility_mask(RID p_scenario, RID p_viewport)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL(scenario);
	if (!scenario->viewport_visibility_masks.has(p_viewport)) {
		return;
	}

	uint64_t mask = scenario->viewport_visibility_masks[p_viewport];
	scenario->used_viewport_visibility_bits &= ~mask;
	scenario->viewport_visibility_masks.erase(p_viewport);
}

void RendererSceneCull::scenario_add_viewport_visibility_mask(RID p_scenario, RID p_viewport)
{
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	ERR_FAIL_NULL(scenario);
	ERR_FAIL_COND(scenario->viewport_visibility_masks.has(p_viewport));

	uint64_t new_mask = 1;
	while (new_mask & scenario->used_viewport_visibility_bits) {
		new_mask <<= 1;
	}

	if (new_mask == 0) {
		ERR_PRINT("Only 64 viewports per scenario allowed when using visibility ranges.");
		new_mask = ((uint64_t)1) << 63;
	}

	scenario->viewport_visibility_masks[p_viewport] = new_mask;
	scenario->used_viewport_visibility_bits |= new_mask;
}

/* INSTANCING API */

void RendererSceneCull::_instance_queue_update(
	Instance* p_instance, bool p_update_aabb, bool p_update_dependencies) const
{
	if (p_update_aabb) {
		p_instance->update_aabb = true;
	}
	if (p_update_dependencies) {
		p_instance->update_dependencies = true;
	}

	if (p_instance->update_item.in_list()) {
		return;
	}

	_instance_update_list.add(&p_instance->update_item);
}

RID RendererSceneCull::instance_allocate() { return instance_owner.allocate_rid(); }

void RendererSceneCull::instance_initialize(RID p_rid)
{
	instance_owner.initialize_rid(p_rid);
	Instance* instance = instance_owner.get_or_null(p_rid);
	instance->self = p_rid;
}

void RendererSceneCull::_instance_update_mesh_instance(Instance* p_instance) const
{
	bool needs_instance =
		RSG::mesh_storage->mesh_needs_instance(p_instance->base, p_instance->skeleton.is_valid());
	if (needs_instance != p_instance->mesh_instance.is_valid()) {
		if (needs_instance) {
			p_instance->mesh_instance = RSG::mesh_storage->mesh_instance_create(p_instance->base);

		}
		else {
			RSG::mesh_storage->mesh_instance_free(p_instance->mesh_instance);
			p_instance->mesh_instance = RID();
		}

		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(p_instance->base_data);
		geom->geometry_instance->set_mesh_instance(p_instance->mesh_instance);

		if (p_instance->scenario && p_instance->array_index >= 0) {
			InstanceData& idata = p_instance->scenario->instance_data[p_instance->array_index];
			if (p_instance->mesh_instance.is_valid()) {
				idata.flags |= InstanceData::FLAG_USES_MESH_INSTANCE;
			}
			else {
				idata.flags &= ~InstanceData::FLAG_USES_MESH_INSTANCE;
			}
		}
	}

	if (p_instance->mesh_instance.is_valid()) {
		RSG::mesh_storage->mesh_instance_set_skeleton(
			p_instance->mesh_instance, p_instance->skeleton);
	}
}

void RendererSceneCull::instance_set_base(RID p_instance, RID p_base)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	Scenario* scenario = instance->scenario;

	if (instance->base_type != RSE::INSTANCE_NONE) {
		// free anything related to that base

		if (scenario && instance->indexer_id.is_valid()) {
			_unpair_instance(instance);
		}

		if (instance->mesh_instance.is_valid()) {
			RSG::mesh_storage->mesh_instance_free(instance->mesh_instance);
			instance->mesh_instance = RID();
			// no need to set instance data flag here, as it was freed above
		}

		switch (instance->base_type) {
		case RSE::INSTANCE_MESH:
		case RSE::INSTANCE_MULTIMESH:
		case RSE::INSTANCE_PARTICLES: {
			InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
			scene_render->geometry_instance_free(geom->geometry_instance);
		} break;
		case RSE::INSTANCE_LIGHT: {
			InstanceLightData* light = static_cast<InstanceLightData*>(instance->base_data);

			if (scenario && instance->visible &&
				RSG::light_storage->light_get_type(instance->base) != RSE::LIGHT_DIRECTIONAL &&
				light->bake_mode == RSE::LIGHT_BAKE_DYNAMIC) {
				scenario->dynamic_lights.erase(light->instance);
			}

#ifdef DEBUG_ENABLED
			if (light->geometries.size()) {
				ERR_PRINT("BUG, indexing did not unpair geometries from light.");
			}
#endif
			if (scenario && light->D) {
				scenario->directional_lights.erase(light->D);
				light->D = nullptr;
			}
			RSG::light_storage->light_instance_free(light->instance);
		} break;
		case RSE::INSTANCE_PARTICLES_COLLISION: {
			InstanceParticlesCollisionData* collision =
				static_cast<InstanceParticlesCollisionData*>(instance->base_data);
			RSG::utilities->free(collision->instance);
		} break;
		case RSE::INSTANCE_FOG_VOLUME: {
			InstanceFogVolumeData* volume =
				static_cast<InstanceFogVolumeData*>(instance->base_data);
			scene_render->free(volume->instance);
		} break;
		case RSE::INSTANCE_VISIBLITY_NOTIFIER: {
			// none
		} break;
		case RSE::INSTANCE_REFLECTION_PROBE: {
			InstanceReflectionProbeData* reflection_probe =
				static_cast<InstanceReflectionProbeData*>(instance->base_data);
			RSG::light_storage->reflection_probe_instance_free(reflection_probe->instance);
			if (reflection_probe->update_list.in_list()) {
				reflection_probe_render_list.remove(&reflection_probe->update_list);
			}
		} break;
		case RSE::INSTANCE_DECAL: {
			InstanceDecalData* decal = static_cast<InstanceDecalData*>(instance->base_data);
			RSG::texture_storage->decal_instance_free(decal->instance);

		} break;
		case RSE::INSTANCE_LIGHTMAP: {
			InstanceLightmapData* lightmap_data =
				static_cast<InstanceLightmapData*>(instance->base_data);
			// erase dependencies, since no longer a lightmap
			while (lightmap_data->users.begin()) {
				instance_geometry_set_lightmap(
					(*lightmap_data->users.begin())->self, RID(), Rect2(), 0);
			}
			RSG::light_storage->lightmap_instance_free(lightmap_data->instance);
		} break;
		case RSE::INSTANCE_VOXEL_GI: {
			InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(instance->base_data);
#ifdef DEBUG_ENABLED
			if (voxel_gi->geometries.size()) {
				ERR_PRINT("BUG, indexing did not unpair geometries from VoxelGI.");
			}
#endif
#ifdef DEBUG_ENABLED
			if (voxel_gi->lights.size()) {
				ERR_PRINT("BUG, indexing did not unpair lights from VoxelGI.");
			}
#endif
			if (voxel_gi->update_element.in_list()) {
				voxel_gi_update_list.remove(&voxel_gi->update_element);
			}

			scene_render->free(voxel_gi->probe_instance);

		} break;
		case RSE::INSTANCE_OCCLUDER: {
			if (scenario && instance->visible) {
				RendererSceneOcclusionCull::get_singleton()->scenario_remove_instance(
					instance->scenario->self, p_instance);
			}
		} break;
		default: {
		}
		}

		if (instance->base_data) {
			memdelete(instance->base_data);
			instance->base_data = nullptr;
		}

		instance->materials.clear();
	}

	instance->base_type = RSE::INSTANCE_NONE;
	instance->base = RID();

	if (p_base.is_valid()) {
		instance->base_type = RSG::utilities->get_base_type(p_base);

		// fix up a specific malfunctioning case before the switch, so it can be handled
		if (instance->base_type == RSE::INSTANCE_NONE &&
			RendererSceneOcclusionCull::get_singleton()->is_occluder(p_base)) {
			instance->base_type = RSE::INSTANCE_OCCLUDER;
		}

		switch (instance->base_type) {
		case RSE::INSTANCE_NONE: {
			ERR_PRINT_ONCE("unimplemented base type encountered in renderer scene cull");
			return;
		}
		case RSE::INSTANCE_LIGHT: {
			InstanceLightData* light = memnew(InstanceLightData);

			if (scenario && RSG::light_storage->light_get_type(p_base) == RSE::LIGHT_DIRECTIONAL) {
				light->D = scenario->directional_lights.push_back(instance);
			}

			light->instance = RSG::light_storage->light_instance_create(p_base);

			instance->base_data = light;
		} break;
		case RSE::INSTANCE_MESH:
		case RSE::INSTANCE_MULTIMESH:
		case RSE::INSTANCE_PARTICLES: {
			InstanceGeometryData* geom = memnew(InstanceGeometryData);
			instance->base_data = geom;
			geom->geometry_instance = scene_render->geometry_instance_create(p_base);

			ERR_FAIL_NULL(geom->geometry_instance);

			geom->geometry_instance->set_skeleton(instance->skeleton);
			geom->geometry_instance->set_material_override(instance->material_override);
			geom->geometry_instance->set_material_overlay(instance->material_overlay);
			geom->geometry_instance->set_surface_materials(instance->materials);
			geom->geometry_instance->set_transform(
				instance->transform, instance->aabb, instance->transformed_aabb);
			geom->geometry_instance->set_layer_mask(instance->layer_mask);
			geom->geometry_instance->set_pivot_data(
				instance->sorting_offset, instance->use_aabb_center);
			geom->geometry_instance->set_lod_bias(instance->lod_bias);
			geom->geometry_instance->set_transparency(instance->transparency);
			geom->geometry_instance->set_use_baked_light(instance->baked_light);
			geom->geometry_instance->set_use_dynamic_gi(instance->dynamic_gi);
			geom->geometry_instance->set_use_lightmap(
				RID(), instance->lightmap_uv_scale, instance->lightmap_slice_index);
			geom->geometry_instance->set_instance_shader_uniforms_offset(
				instance->instance_uniforms.location());
			geom->geometry_instance->set_cast_double_sided_shadows(
				instance->cast_shadows == RSE::SHADOW_CASTING_SETTING_DOUBLE_SIDED);
			if (instance->lightmap_sh.size() == 9) {
				geom->geometry_instance->set_lightmap_capture(instance->lightmap_sh.ptr());
			}

			for (Instance* E : instance->visibility_dependencies) {
				Instance* dep_instance = E;
				ERR_CONTINUE(dep_instance->array_index == -1);
				ERR_CONTINUE(dep_instance->scenario->instance_data[dep_instance->array_index]
								 .parent_array_index != -1);
				dep_instance->scenario->instance_data[dep_instance->array_index]
					.parent_array_index = instance->array_index;
			}
		} break;
		case RSE::INSTANCE_PARTICLES_COLLISION: {
			InstanceParticlesCollisionData* collision = memnew(InstanceParticlesCollisionData);
			collision->instance =
				RSG::particles_storage->particles_collision_instance_create(p_base);
			RSG::particles_storage->particles_collision_instance_set_active(
				collision->instance, instance->visible);
			instance->base_data = collision;
		} break;
		case RSE::INSTANCE_FOG_VOLUME: {
			InstanceFogVolumeData* volume = memnew(InstanceFogVolumeData);
			volume->instance = scene_render->fog_volume_instance_create(p_base);
			scene_render->fog_volume_instance_set_active(volume->instance, instance->visible);
			instance->base_data = volume;
		} break;
		case RSE::INSTANCE_VISIBLITY_NOTIFIER: {
			InstanceVisibilityNotifierData* vnd = memnew(InstanceVisibilityNotifierData);
			vnd->base = p_base;
			instance->base_data = vnd;
		} break;
		case RSE::INSTANCE_REFLECTION_PROBE: {
			InstanceReflectionProbeData* reflection_probe = memnew(InstanceReflectionProbeData);
			reflection_probe->owner = instance;
			instance->base_data = reflection_probe;

			reflection_probe->instance =
				RSG::light_storage->reflection_probe_instance_create(p_base);
		} break;
		case RSE::INSTANCE_DECAL: {
			InstanceDecalData* decal = memnew(InstanceDecalData);
			decal->owner = instance;
			instance->base_data = decal;

			decal->instance = RSG::texture_storage->decal_instance_create(p_base);
			RSG::texture_storage->decal_instance_set_sorting_offset(
				decal->instance, instance->sorting_offset);
		} break;
		case RSE::INSTANCE_LIGHTMAP: {
			InstanceLightmapData* lightmap_data = memnew(InstanceLightmapData);
			instance->base_data = lightmap_data;
			lightmap_data->instance = RSG::light_storage->lightmap_instance_create(p_base);
		} break;
		case RSE::INSTANCE_VOXEL_GI: {
			InstanceVoxelGIData* voxel_gi = memnew(InstanceVoxelGIData);
			instance->base_data = voxel_gi;
			voxel_gi->owner = instance;

			if (scenario && !voxel_gi->update_element.in_list()) {
				voxel_gi_update_list.add(&voxel_gi->update_element);
			}

			voxel_gi->probe_instance = scene_render->voxel_gi_instance_create(p_base);

		} break;
		case RSE::INSTANCE_OCCLUDER: {
			if (scenario) {
				RendererSceneOcclusionCull::get_singleton()->scenario_set_instance(
					scenario->self, p_instance, p_base, instance->transform, instance->visible);
			}
		} break;
		default: {
		}
		}

		instance->base = p_base;

		if (instance->base_type == RSE::INSTANCE_MESH) {
			_instance_update_mesh_instance(instance);
		}

		// forcefully update the dependency now, so if for some reason it gets removed, we can
		// immediately clear it
		RSG::utilities->base_update_dependency(p_base, &instance->dependency_tracker);
	}

	_instance_queue_update(instance, true, true);
}

void RendererSceneCull::instance_set_scenario(RID p_instance, RID p_scenario)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (instance->scenario) {
		instance->scenario->instances.remove(&instance->scenario_item);

		if (instance->indexer_id.is_valid()) {
			_unpair_instance(instance);
		}

		switch (instance->base_type) {
		case RSE::INSTANCE_LIGHT: {
			InstanceLightData* light = static_cast<InstanceLightData*>(instance->base_data);
			if (instance->visible &&
				RSG::light_storage->light_get_type(instance->base) != RSE::LIGHT_DIRECTIONAL &&
				light->bake_mode == RSE::LIGHT_BAKE_DYNAMIC) {
				instance->scenario->dynamic_lights.erase(light->instance);
			}

#ifdef DEBUG_ENABLED
			if (light->geometries.size()) {
				ERR_PRINT("BUG, indexing did not unpair geometries from light.");
			}
#endif
			if (light->D) {
				instance->scenario->directional_lights.erase(light->D);
				light->D = nullptr;
			}
		} break;
		case RSE::INSTANCE_REFLECTION_PROBE: {
			InstanceReflectionProbeData* reflection_probe =
				static_cast<InstanceReflectionProbeData*>(instance->base_data);
			RSG::light_storage->reflection_probe_release_atlas_index(reflection_probe->instance);

		} break;
		case RSE::INSTANCE_PARTICLES_COLLISION: {
			heightfield_particle_colliders_update_list.erase(instance);
		} break;
		case RSE::INSTANCE_VOXEL_GI: {
			InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(instance->base_data);

#ifdef DEBUG_ENABLED
			if (voxel_gi->geometries.size()) {
				ERR_PRINT("BUG, indexing did not unpair geometries from VoxelGI.");
			}
#endif
#ifdef DEBUG_ENABLED
			if (voxel_gi->lights.size()) {
				ERR_PRINT("BUG, indexing did not unpair lights from VoxelGI.");
			}
#endif

			if (voxel_gi->update_element.in_list()) {
				voxel_gi_update_list.remove(&voxel_gi->update_element);
			}
		} break;
		case RSE::INSTANCE_OCCLUDER: {
			if (instance->visible) {
				RendererSceneOcclusionCull::get_singleton()->scenario_remove_instance(
					instance->scenario->self, p_instance);
			}
		} break;
		default: {
		}
		}

		instance->scenario = nullptr;
	}

	if (p_scenario.is_valid()) {
		Scenario* scenario = scenario_owner.get_or_null(p_scenario);
		ERR_FAIL_NULL(scenario);

		instance->scenario = scenario;

		scenario->instances.add(&instance->scenario_item);

		switch (instance->base_type) {
		case RSE::INSTANCE_LIGHT: {
			InstanceLightData* light = static_cast<InstanceLightData*>(instance->base_data);

			if (RSG::light_storage->light_get_type(instance->base) == RSE::LIGHT_DIRECTIONAL) {
				light->D = scenario->directional_lights.push_back(instance);
			}
		} break;
		case RSE::INSTANCE_VOXEL_GI: {
			InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(instance->base_data);
			if (!voxel_gi->update_element.in_list()) {
				voxel_gi_update_list.add(&voxel_gi->update_element);
			}
		} break;
		case RSE::INSTANCE_OCCLUDER: {
			RendererSceneOcclusionCull::get_singleton()->scenario_set_instance(
				scenario->self, p_instance, instance->base, instance->transform, instance->visible);
		} break;
		default: {
		}
		}

		_instance_queue_update(instance, true, true);
	}
}

void RendererSceneCull::instance_set_layer_mask(RID p_instance, uint32_t p_mask)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (instance->layer_mask == p_mask) {
		return;
	}

	// Particles always need to be unpaired. Geometry may need to be unpaired, but only if lights or
	// decals use pairing. Needs to happen before layer mask changes so we can avoid attempting to
	// unpair something that was never paired.
	if (instance->base_type == RSE::INSTANCE_PARTICLES ||
		(((geometry_instance_pair_mask & (1 << RSE::INSTANCE_LIGHT)) ||
			 (geometry_instance_pair_mask & (1 << RSE::INSTANCE_DECAL))) &&
			((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK))) {
		_unpair_instance(instance);
		singleton->_instance_queue_update(instance, false, false);
	}

	instance->layer_mask = p_mask;
	if (instance->scenario && instance->array_index >= 0) {
		instance->scenario->instance_data[instance->array_index].layer_mask = p_mask;
	}

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_layer_mask(p_mask);

		if (geom->can_cast_shadows) {
			for (HashSet<RendererSceneCull::Instance*>::Iterator I = geom->lights.begin();
				 I != geom->lights.end(); ++I) {
				InstanceLightData* light = static_cast<InstanceLightData*>((*I)->base_data);
				light->make_shadow_dirty();
			}
		}
	}
}

void RendererSceneCull::instance_set_pivot_data(
	RID p_instance, float p_sorting_offset, bool p_use_aabb_center)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->sorting_offset = p_sorting_offset;
	instance->use_aabb_center = p_use_aabb_center;

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_pivot_data(p_sorting_offset, p_use_aabb_center);
	}
	else if (instance->base_type == RSE::INSTANCE_DECAL && instance->base_data) {
		InstanceDecalData* decal = static_cast<InstanceDecalData*>(instance->base_data);
		RSG::texture_storage->decal_instance_set_sorting_offset(
			decal->instance, instance->sorting_offset);
	}
}

void RendererSceneCull::instance_geometry_set_transparency(RID p_instance, float p_transparency)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->transparency = p_transparency;

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_transparency(p_transparency);
	}
}

void RendererSceneCull::instance_set_transform(RID p_instance, const Transform3D& p_transform)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (instance->transform == p_transform) {
		return; // Must be checked to avoid worst evil.
	}

#ifdef DEBUG_ENABLED

	for (int i = 0; i < 4; i++) {
		const Vector3& v = i < 3 ? p_transform.basis.rows[i] : p_transform.origin;
		ERR_FAIL_COND(!v.is_finite());
	}

#endif
	instance->transform = p_transform;
	_instance_queue_update(instance, true);
}

void RendererSceneCull::instance_set_surface_override_material(
	RID p_instance, int p_surface, RID p_material)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (instance->base_type == RSE::INSTANCE_MESH) {
		// may not have been updated yet, may also have not been set yet. When updated will be
		// correcte, worst case
		instance->materials.resize(
			MAX(p_surface + 1, RSG::mesh_storage->mesh_get_surface_count(instance->base)));
	}

	ERR_FAIL_INDEX(p_surface, instance->materials.size());

	instance->materials.write[p_surface] = p_material;

	_instance_queue_update(instance, false, true);
}

void RendererSceneCull::instance_set_visible(RID p_instance, bool p_visible)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (instance->visible == p_visible) {
		return;
	}

	instance->visible = p_visible;

	if (p_visible) {
		if (instance->scenario != nullptr) {
			_instance_queue_update(instance, true, false);
		}
	}
	else if (instance->indexer_id.is_valid()) {
		_unpair_instance(instance);
	}

	if (instance->base_type == RSE::INSTANCE_LIGHT) {
		InstanceLightData* light = static_cast<InstanceLightData*>(instance->base_data);
		if (instance->scenario &&
			RSG::light_storage->light_get_type(instance->base) != RSE::LIGHT_DIRECTIONAL &&
			light->bake_mode == RSE::LIGHT_BAKE_DYNAMIC) {
			if (p_visible) {
				instance->scenario->dynamic_lights.push_back(light->instance);
			}
			else {
				instance->scenario->dynamic_lights.erase(light->instance);
			}
		}
	}

	if (instance->base_type == RSE::INSTANCE_PARTICLES_COLLISION) {
		InstanceParticlesCollisionData* collision =
			static_cast<InstanceParticlesCollisionData*>(instance->base_data);
		RSG::particles_storage->particles_collision_instance_set_active(
			collision->instance, p_visible);
	}

	if (instance->base_type == RSE::INSTANCE_FOG_VOLUME) {
		InstanceFogVolumeData* volume = static_cast<InstanceFogVolumeData*>(instance->base_data);
		scene_render->fog_volume_instance_set_active(volume->instance, p_visible);
	}

	if (instance->base_type == RSE::INSTANCE_OCCLUDER) {
		if (instance->scenario) {
			RendererSceneOcclusionCull::get_singleton()->scenario_set_instance(
				instance->scenario->self, p_instance, instance->base, instance->transform,
				p_visible);
		}
	}
}

void RendererSceneCull::instance_teleport(RID p_instance)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);
	instance->teleported = true;
}

void RendererSceneCull::instance_set_custom_aabb(RID p_instance, AABB p_aabb)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (p_aabb != AABB()) {
		// Set custom AABB
		if (instance->custom_aabb == nullptr) {
			instance->custom_aabb = memnew(AABB);
		}
		*instance->custom_aabb = p_aabb;

	}
	else {
		// Clear custom AABB
		if (instance->custom_aabb != nullptr) {
			memdelete(instance->custom_aabb);
			instance->custom_aabb = nullptr;
		}
	}

	if (instance->scenario) {
		_instance_queue_update(instance, true, false);
	}
}

void RendererSceneCull::instance_attach_skeleton(RID p_instance, RID p_skeleton)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (instance->skeleton == p_skeleton) {
		return;
	}

	instance->skeleton = p_skeleton;

	if (p_skeleton.is_valid()) {
		// update the dependency now, so if cleared, we remove it
		RSG::mesh_storage->skeleton_update_dependency(p_skeleton, &instance->dependency_tracker);
	}

	_instance_queue_update(instance, true, true);

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		_instance_update_mesh_instance(instance);

		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_skeleton(p_skeleton);
	}
}

void RendererSceneCull::instance_set_extra_visibility_margin(RID p_instance, real_t p_margin)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->extra_margin = p_margin;
	_instance_queue_update(instance, true, false);
}

void RendererSceneCull::instance_set_ignore_culling(RID p_instance, bool p_enabled)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);
	instance->ignore_all_culling = p_enabled;

	if (instance->scenario && instance->array_index >= 0) {
		InstanceData& idata = instance->scenario->instance_data[instance->array_index];
		if (instance->ignore_all_culling) {
			idata.flags |= InstanceData::FLAG_IGNORE_ALL_CULLING;
		}
		else {
			idata.flags &= ~InstanceData::FLAG_IGNORE_ALL_CULLING;
		}
	}
}

void RendererSceneCull::instance_geometry_set_flag(
	RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	// ERR_FAIL_COND(((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK));

	switch (p_flags) {
	case RSE::INSTANCE_FLAG_USE_BAKED_LIGHT: {
		instance->baked_light = p_enabled;

		if (instance->scenario && instance->array_index >= 0) {
			InstanceData& idata = instance->scenario->instance_data[instance->array_index];
			if (instance->baked_light) {
				idata.flags |= InstanceData::FLAG_USES_BAKED_LIGHT;
			}
			else {
				idata.flags &= ~InstanceData::FLAG_USES_BAKED_LIGHT;
			}
		}

		if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
			InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
			ERR_FAIL_NULL(geom->geometry_instance);
			geom->geometry_instance->set_use_baked_light(p_enabled);
		}

	} break;
	case RSE::INSTANCE_FLAG_USE_DYNAMIC_GI: {
		if (p_enabled == instance->dynamic_gi) {
			// bye, redundant
			return;
		}

		if (instance->indexer_id.is_valid()) {
			_unpair_instance(instance);
			_instance_queue_update(instance, true, true);
		}

		// once out of octree, can be changed
		instance->dynamic_gi = p_enabled;

		if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
			InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
			ERR_FAIL_NULL(geom->geometry_instance);
			geom->geometry_instance->set_use_dynamic_gi(p_enabled);
		}

	} break;
	case RSE::INSTANCE_FLAG_DRAW_NEXT_FRAME_IF_VISIBLE: {
		instance->redraw_if_visible = p_enabled;

		if (instance->scenario && instance->array_index >= 0) {
			InstanceData& idata = instance->scenario->instance_data[instance->array_index];
			if (instance->redraw_if_visible) {
				idata.flags |= InstanceData::FLAG_REDRAW_IF_VISIBLE;
			}
			else {
				idata.flags &= ~InstanceData::FLAG_REDRAW_IF_VISIBLE;
			}
		}

	} break;
	case RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING: {
		instance->ignore_occlusion_culling = p_enabled;

		if (instance->scenario && instance->array_index >= 0) {
			InstanceData& idata = instance->scenario->instance_data[instance->array_index];
			if (instance->ignore_occlusion_culling) {
				idata.flags |= InstanceData::FLAG_IGNORE_OCCLUSION_CULLING;
			}
			else {
				idata.flags &= ~InstanceData::FLAG_IGNORE_OCCLUSION_CULLING;
			}
		}
	} break;
	default: {
	}
	}
}

void RendererSceneCull::instance_geometry_set_cast_shadows_setting(
	RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->cast_shadows = p_shadow_casting_setting;

	if (instance->scenario && instance->array_index >= 0) {
		InstanceData& idata = instance->scenario->instance_data[instance->array_index];

		if (instance->cast_shadows != RSE::SHADOW_CASTING_SETTING_OFF) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS;
		}
		else {
			idata.flags &= ~InstanceData::FLAG_CAST_SHADOWS;
		}

		if (instance->cast_shadows == RSE::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS_ONLY;
		}
		else {
			idata.flags &= ~InstanceData::FLAG_CAST_SHADOWS_ONLY;
		}
	}

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);

		geom->geometry_instance->set_cast_double_sided_shadows(
			instance->cast_shadows == RSE::SHADOW_CASTING_SETTING_DOUBLE_SIDED);
	}

	_instance_queue_update(instance, false, true);
}

void RendererSceneCull::instance_geometry_set_material_override(RID p_instance, RID p_material)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->material_override = p_material;
	_instance_queue_update(instance, false, true);

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_material_override(p_material);
	}
}

void RendererSceneCull::instance_geometry_set_material_overlay(RID p_instance, RID p_material)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->material_overlay = p_material;
	_instance_queue_update(instance, false, true);

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_material_overlay(p_material);
	}
}

void RendererSceneCull::instance_geometry_set_visibility_range(RID p_instance, float p_min,
	float p_max, float p_min_margin, float p_max_margin, RSE::VisibilityRangeFadeMode p_fade_mode)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->visibility_range_begin = p_min;
	instance->visibility_range_end = p_max;
	instance->visibility_range_begin_margin = p_min_margin;
	instance->visibility_range_end_margin = p_max_margin;
	instance->visibility_range_fade_mode = p_fade_mode;

	_update_instance_visibility_dependencies(instance);

	if (instance->scenario && instance->visibility_index != -1) {
		InstanceVisibilityData& vd =
			instance->scenario->instance_visibility[instance->visibility_index];
		vd.range_begin = instance->visibility_range_begin;
		vd.range_end = instance->visibility_range_end;
		vd.range_begin_margin = instance->visibility_range_begin_margin;
		vd.range_end_margin = instance->visibility_range_end_margin;
		vd.fade_mode = p_fade_mode;
	}
}

void RendererSceneCull::instance_set_visibility_parent(RID p_instance, RID p_parent_instance)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	Instance* old_parent = instance->visibility_parent;
	if (old_parent) {
		old_parent->visibility_dependencies.erase(instance);
		instance->visibility_parent = nullptr;
		_update_instance_visibility_depth(old_parent);
	}

	Instance* parent = instance_owner.get_or_null(p_parent_instance);
	ERR_FAIL_COND(p_parent_instance.is_valid() && !parent);

	if (parent) {
		parent->visibility_dependencies.insert(instance);
		instance->visibility_parent = parent;

		bool cycle_detected = _update_instance_visibility_depth(parent);
		if (cycle_detected) {
			ERR_PRINT("Cycle detected in the visibility dependencies tree. The latest change to "
					  "visibility_parent will have no effect.");
			parent->visibility_dependencies.erase(instance);
			instance->visibility_parent = nullptr;
		}
	}

	_update_instance_visibility_dependencies(instance);
}

bool RendererSceneCull::_update_instance_visibility_depth(Instance* p_instance)
{
	bool cycle_detected = false;
	HashSet<Instance*> traversed_nodes;

	{
		Instance* instance = p_instance;
		while (instance) {
			if (!instance->visibility_dependencies.is_empty()) {
				uint32_t depth = 0;
				for (const Instance* E : instance->visibility_dependencies) {
					depth = MAX(depth, E->visibility_dependencies_depth);
				}
				instance->visibility_dependencies_depth = depth + 1;
			}
			else {
				instance->visibility_dependencies_depth = 0;
			}

			if (instance->scenario && instance->visibility_index != -1) {
				instance->scenario->instance_visibility.move(
					instance->visibility_index, instance->visibility_dependencies_depth);
			}

			traversed_nodes.insert(instance);

			instance = instance->visibility_parent;
			if (traversed_nodes.has(instance)) {
				cycle_detected = true;
				break;
			}
		}
	}

	return cycle_detected;
}

void RendererSceneCull::_update_instance_visibility_dependencies(Instance* p_instance) const
{
	bool is_geometry_instance =
		((1 << p_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) && p_instance->base_data;
	bool has_visibility_range =
		p_instance->visibility_range_begin > 0.0 || p_instance->visibility_range_end > 0.0;
	bool needs_visibility_cull =
		has_visibility_range && is_geometry_instance && p_instance->array_index != -1;

	if (!needs_visibility_cull && p_instance->visibility_index != -1) {
		p_instance->scenario->instance_visibility.remove_at(p_instance->visibility_index);
		p_instance->visibility_index = -1;
	}
	else if (needs_visibility_cull && p_instance->visibility_index == -1) {
		InstanceVisibilityData vd;
		vd.instance = p_instance;
		vd.range_begin = p_instance->visibility_range_begin;
		vd.range_end = p_instance->visibility_range_end;
		vd.range_begin_margin = p_instance->visibility_range_begin_margin;
		vd.range_end_margin = p_instance->visibility_range_end_margin;
		vd.position = p_instance->transformed_aabb.get_center();
		vd.array_index = p_instance->array_index;
		vd.fade_mode = p_instance->visibility_range_fade_mode;

		p_instance->scenario->instance_visibility.insert(
			vd, p_instance->visibility_dependencies_depth);
	}

	if (p_instance->scenario && p_instance->array_index != -1) {
		InstanceData& idata = p_instance->scenario->instance_data[p_instance->array_index];
		idata.visibility_index = p_instance->visibility_index;

		if (is_geometry_instance) {
			if (has_visibility_range &&
				p_instance->visibility_range_fade_mode == RSE::VISIBILITY_RANGE_FADE_SELF) {
				bool begin_enabled = p_instance->visibility_range_begin > 0.0f;
				float begin_min =
					p_instance->visibility_range_begin - p_instance->visibility_range_begin_margin;
				float begin_max =
					p_instance->visibility_range_begin + p_instance->visibility_range_begin_margin;
				bool end_enabled = p_instance->visibility_range_end > 0.0f;
				float end_min =
					p_instance->visibility_range_end - p_instance->visibility_range_end_margin;
				float end_max =
					p_instance->visibility_range_end + p_instance->visibility_range_end_margin;
				idata.instance_geometry->set_fade_range(
					begin_enabled, begin_min, begin_max, end_enabled, end_min, end_max);
			}
			else {
				idata.instance_geometry->set_fade_range(false, 0.0f, 0.0f, false, 0.0f, 0.0f);
			}
		}

		if ((has_visibility_range || p_instance->visibility_parent) &&
			(p_instance->visibility_index == -1 ||
				p_instance->visibility_dependencies_depth == 0)) {
			idata.flags |= InstanceData::FLAG_VISIBILITY_DEPENDENCY_NEEDS_CHECK;
		}
		else {
			idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_NEEDS_CHECK;
		}

		if (p_instance->visibility_parent) {
			idata.parent_array_index = p_instance->visibility_parent->array_index;
		}
		else {
			idata.parent_array_index = -1;
			if (is_geometry_instance) {
				idata.instance_geometry->set_parent_fade_alpha(1.0f);
			}
		}
	}
}

void RendererSceneCull::instance_geometry_set_lightmap(
	RID p_instance, RID p_lightmap, const Rect2& p_lightmap_uv_scale, int p_slice_index)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	if (instance->lightmap) {
		InstanceLightmapData* lightmap_data =
			static_cast<InstanceLightmapData*>(((Instance*)instance->lightmap)->base_data);
		lightmap_data->users.erase(instance);
		instance->lightmap = nullptr;
	}

	Instance* lightmap_instance = instance_owner.get_or_null(p_lightmap);

	instance->lightmap = lightmap_instance;
	instance->lightmap_uv_scale = p_lightmap_uv_scale;
	instance->lightmap_slice_index = p_slice_index;

	RID lightmap_instance_rid;

	if (lightmap_instance) {
		InstanceLightmapData* lightmap_data =
			static_cast<InstanceLightmapData*>(lightmap_instance->base_data);
		lightmap_data->users.insert(instance);
		lightmap_instance_rid = lightmap_data->instance;
	}

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_use_lightmap(
			lightmap_instance_rid, p_lightmap_uv_scale, p_slice_index);
	}
}

void RendererSceneCull::instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias)
{
	Instance* instance = instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(instance);

	instance->lod_bias = p_lod_bias;

	if ((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK && instance->base_data) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);
		geom->geometry_instance->set_lod_bias(p_lod_bias);
	}
}

void RendererSceneCull::mesh_generate_pipelines(RID p_mesh, bool p_background_compilation)
{
	scene_render->mesh_generate_pipelines(p_mesh, p_background_compilation);
}

uint32_t RendererSceneCull::get_pipeline_compilations(RSE::PipelineSource p_source)
{
	return scene_render->get_pipeline_compilations(p_source);
}

void RendererSceneCull::_update_instance(Instance* p_instance) const
{
	p_instance->version++;

	// When not using interpolation the transform is used straight.
	const Transform3D* instance_xform = &p_instance->transform;

	// Can possibly use the most up to date current transform here when using physics interpolation
	// ... uncomment the next line for this..
	// if (_interpolation_data.interpolation_enabled && p_instance->interpolated) {
	//    instance_xform = &p_instance->transform_curr;
	//}
	// However it does seem that using the interpolated transform (transform) works for keeping
	// AABBs up to date to avoid culling errors.

	if (p_instance->base_type == RSE::INSTANCE_LIGHT) {
		InstanceLightData* light = static_cast<InstanceLightData*>(p_instance->base_data);

		RSG::light_storage->light_instance_set_transform(light->instance, *instance_xform);
		RSG::light_storage->light_instance_set_aabb(
			light->instance, instance_xform->xform(p_instance->aabb));
		light->make_shadow_dirty();

		RSE::LightBakeMode bake_mode = RSG::light_storage->light_get_bake_mode(p_instance->base);
		if (RSG::light_storage->light_get_type(p_instance->base) != RSE::LIGHT_DIRECTIONAL &&
			bake_mode != light->bake_mode) {
			if (p_instance->visible && p_instance->scenario &&
				light->bake_mode == RSE::LIGHT_BAKE_DYNAMIC) {
				p_instance->scenario->dynamic_lights.erase(light->instance);
			}

			light->bake_mode = bake_mode;

			if (p_instance->visible && p_instance->scenario &&
				light->bake_mode == RSE::LIGHT_BAKE_DYNAMIC) {
				p_instance->scenario->dynamic_lights.push_back(light->instance);
			}
		}

		uint32_t max_sdfgi_cascade =
			RSG::light_storage->light_get_max_sdfgi_cascade(p_instance->base);
		if (light->max_sdfgi_cascade != max_sdfgi_cascade) {
			light->max_sdfgi_cascade =
				max_sdfgi_cascade; // should most likely make sdfgi dirty in scenario
		}
		light->cull_mask = RSG::light_storage->light_get_cull_mask(p_instance->base);
	}
	else if (p_instance->base_type == RSE::INSTANCE_REFLECTION_PROBE) {
		InstanceReflectionProbeData* reflection_probe =
			static_cast<InstanceReflectionProbeData*>(p_instance->base_data);

		RSG::light_storage->reflection_probe_instance_set_transform(
			reflection_probe->instance, *instance_xform);

		if (p_instance->scenario && p_instance->array_index >= 0) {
			InstanceData& idata = p_instance->scenario->instance_data[p_instance->array_index];
			idata.flags |= InstanceData::FLAG_REFLECTION_PROBE_DIRTY;
		}
	}
	else if (p_instance->base_type == RSE::INSTANCE_DECAL) {
		InstanceDecalData* decal = static_cast<InstanceDecalData*>(p_instance->base_data);

		RSG::texture_storage->decal_instance_set_transform(decal->instance, *instance_xform);
		decal->cull_mask = RSG::texture_storage->decal_get_cull_mask(p_instance->base);
	}
	else if (p_instance->base_type == RSE::INSTANCE_LIGHTMAP) {
		InstanceLightmapData* lightmap = static_cast<InstanceLightmapData*>(p_instance->base_data);

		RSG::light_storage->lightmap_instance_set_transform(lightmap->instance, *instance_xform);
	}
	else if (p_instance->base_type == RSE::INSTANCE_VOXEL_GI) {
		InstanceVoxelGIData* voxel_gi = static_cast<InstanceVoxelGIData*>(p_instance->base_data);

		scene_render->voxel_gi_instance_set_transform_to_data(
			voxel_gi->probe_instance, *instance_xform);
	}
	else if (p_instance->base_type == RSE::INSTANCE_PARTICLES) {
		RSG::particles_storage->particles_set_emission_transform(p_instance->base, *instance_xform);
	}
	else if (p_instance->base_type == RSE::INSTANCE_PARTICLES_COLLISION) {
		InstanceParticlesCollisionData* collision =
			static_cast<InstanceParticlesCollisionData*>(p_instance->base_data);

		// remove materials no longer used and un-own them
		if (RSG::particles_storage->particles_collision_is_heightfield(p_instance->base)) {
			heightfield_particle_colliders_update_list.insert(p_instance);
		}
		RSG::particles_storage->particles_collision_instance_set_transform(
			collision->instance, *instance_xform);
		collision->cull_mask =
			RSG::particles_storage->particles_collision_get_cull_mask(p_instance->base);
	}
	else if (p_instance->base_type == RSE::INSTANCE_FOG_VOLUME) {
		InstanceFogVolumeData* volume = static_cast<InstanceFogVolumeData*>(p_instance->base_data);
		scene_render->fog_volume_instance_set_transform(volume->instance, *instance_xform);
	}
	else if (p_instance->base_type == RSE::INSTANCE_OCCLUDER) {
		if (p_instance->scenario) {
			RendererSceneOcclusionCull::get_singleton()->scenario_set_instance(
				p_instance->scenario->self, p_instance->self, p_instance->base, *instance_xform,
				p_instance->visible);
		}
	}
	else if (p_instance->base_type == RSE::INSTANCE_NONE) {
		return;
	}

	if (!p_instance->aabb.has_surface()) {
		return;
	}

	if (p_instance->base_type == RSE::INSTANCE_LIGHTMAP) {
		// if this moved, update the captured objects
		InstanceLightmapData* lightmap_data =
			static_cast<InstanceLightmapData*>(p_instance->base_data);
		// erase dependencies, since no longer a lightmap

		for (Instance* E : lightmap_data->geometries) {
			Instance* geom = E;
			_instance_queue_update(geom, true, false);
		}
	}

	AABB new_aabb;
	new_aabb = instance_xform->xform(p_instance->aabb);
	p_instance->transformed_aabb = new_aabb;

	if ((1 << p_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) {
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(p_instance->base_data);
		// make sure lights are updated if it casts shadow

		if (geom->can_cast_shadows) {
			for (const Instance* E : geom->lights) {
				InstanceLightData* light = static_cast<InstanceLightData*>(E->base_data);
				light->make_shadow_dirty();
			}
		}

		if (!p_instance->lightmap && geom->lightmap_captures.size()) {
			// affected by lightmap captures, must update capture info!
			_update_instance_lightmap_captures(p_instance);
		}
		else {
			if (!p_instance->lightmap_sh.is_empty()) {
				p_instance->lightmap_sh.clear();		// don't need SH
				p_instance->lightmap_target_sh.clear(); // don't need SH
				ERR_FAIL_NULL(geom->geometry_instance);
				geom->geometry_instance->set_lightmap_capture(nullptr);
			}
		}

		ERR_FAIL_NULL(geom->geometry_instance);

		geom->geometry_instance->set_transform(
			*instance_xform, p_instance->aabb, p_instance->transformed_aabb);
		if (p_instance->teleported) {
			geom->geometry_instance->reset_motion_vectors();
		}
	}

	// note: we had to remove is equal approx check here, it meant that det == 0.000004 won't work,
	// which is the case for some of our scenes.
	if (p_instance->scenario == nullptr || !p_instance->visible ||
		instance_xform->basis.determinant() == 0) {
		p_instance->prev_transformed_aabb = p_instance->transformed_aabb;
		return;
	}

	// quantize to improve moving object performance
	AABB bvh_aabb = p_instance->transformed_aabb;

	if (p_instance->indexer_id.is_valid() && bvh_aabb != p_instance->prev_transformed_aabb) {
		// assume motion, see if bounds need to be quantized
		AABB motion_aabb = bvh_aabb.merge(p_instance->prev_transformed_aabb);
		float motion_longest_axis = motion_aabb.get_longest_axis_size();
		float longest_axis = p_instance->transformed_aabb.get_longest_axis_size();

		if (motion_longest_axis < longest_axis * 2) {
			// moved but not a lot, use motion aabb quantizing
			float quantize_size =
				Math::pow(2.0, Math::ceil(Math::log(motion_longest_axis) / Math::log(2.0))) *
				0.5; // one fifth
			bvh_aabb.quantize(quantize_size);
		}
	}

	if (!p_instance->indexer_id.is_valid()) {
		if ((1 << p_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) {
			p_instance->indexer_id =
				p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY].insert(
					bvh_aabb, p_instance);
		}
		else {
			p_instance->indexer_id =
				p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES].insert(
					bvh_aabb, p_instance);
		}

		p_instance->array_index = p_instance->scenario->instance_data.size();
		InstanceData idata;
		idata.instance = p_instance;
		idata.layer_mask = p_instance->layer_mask;
		idata.flags = p_instance->base_type; // changing it means de-indexing, so this never needs
											 // to be changed later
		idata.base_rid = p_instance->base;
		idata.parent_array_index =
			p_instance->visibility_parent ? p_instance->visibility_parent->array_index : -1;
		idata.visibility_index = p_instance->visibility_index;
		idata.occlusion_timeout = 0;

		for (Instance* E : p_instance->visibility_dependencies) {
			Instance* dep_instance = E;
			if (dep_instance->array_index != -1) {
				dep_instance->scenario->instance_data[dep_instance->array_index]
					.parent_array_index = p_instance->array_index;
			}
		}

		switch (p_instance->base_type) {
		case RSE::INSTANCE_MESH:
		case RSE::INSTANCE_MULTIMESH:
		case RSE::INSTANCE_PARTICLES: {
			InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(p_instance->base_data);
			idata.instance_geometry = geom->geometry_instance;
		} break;
		case RSE::INSTANCE_LIGHT: {
			InstanceLightData* light_data = static_cast<InstanceLightData*>(p_instance->base_data);
			idata.instance_data_rid = light_data->instance.get_id();
			light_data->uses_projector = RSG::light_storage->light_has_projector(p_instance->base);
			light_data->uses_softshadow =
				RSG::light_storage->light_get_type(p_instance->base) == RSE::LIGHT_AREA ||
				RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_SIZE) >
					CMP_EPSILON;
		} break;
		case RSE::INSTANCE_REFLECTION_PROBE: {
			idata.instance_data_rid =
				static_cast<InstanceReflectionProbeData*>(p_instance->base_data)->instance.get_id();
		} break;
		case RSE::INSTANCE_DECAL: {
			idata.instance_data_rid =
				static_cast<InstanceDecalData*>(p_instance->base_data)->instance.get_id();
		} break;
		case RSE::INSTANCE_LIGHTMAP: {
			idata.instance_data_rid =
				static_cast<InstanceLightmapData*>(p_instance->base_data)->instance.get_id();
		} break;
		case RSE::INSTANCE_VOXEL_GI: {
			idata.instance_data_rid =
				static_cast<InstanceVoxelGIData*>(p_instance->base_data)->probe_instance.get_id();
		} break;
		case RSE::INSTANCE_FOG_VOLUME: {
			idata.instance_data_rid =
				static_cast<InstanceFogVolumeData*>(p_instance->base_data)->instance.get_id();
		} break;
		case RSE::INSTANCE_VISIBLITY_NOTIFIER: {
			idata.visibility_notifier =
				static_cast<InstanceVisibilityNotifierData*>(p_instance->base_data);
		} break;
		default: {
		}
		}

		if (p_instance->base_type == RSE::INSTANCE_REFLECTION_PROBE) {
			// always dirty when added
			idata.flags |= InstanceData::FLAG_REFLECTION_PROBE_DIRTY;
		}
		if (p_instance->cast_shadows != RSE::SHADOW_CASTING_SETTING_OFF) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS;
		}
		if (p_instance->cast_shadows == RSE::SHADOW_CASTING_SETTING_SHADOWS_ONLY) {
			idata.flags |= InstanceData::FLAG_CAST_SHADOWS_ONLY;
		}
		if (p_instance->redraw_if_visible) {
			idata.flags |= InstanceData::FLAG_REDRAW_IF_VISIBLE;
		}
		// dirty flags should not be set here, since no pairing has happened
		if (p_instance->baked_light) {
			idata.flags |= InstanceData::FLAG_USES_BAKED_LIGHT;
		}
		if (p_instance->mesh_instance.is_valid()) {
			idata.flags |= InstanceData::FLAG_USES_MESH_INSTANCE;
		}
		if (p_instance->ignore_occlusion_culling) {
			idata.flags |= InstanceData::FLAG_IGNORE_OCCLUSION_CULLING;
		}
		if (p_instance->ignore_all_culling) {
			idata.flags |= InstanceData::FLAG_IGNORE_ALL_CULLING;
		}

		p_instance->scenario->instance_data.push_back(idata);
		p_instance->scenario->instance_aabbs.push_back(
			InstanceBounds(p_instance->transformed_aabb));
		_update_instance_visibility_dependencies(p_instance);
	}
	else {
		if ((1 << p_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) {
			p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY].update(
				p_instance->indexer_id, bvh_aabb);
		}
		else {
			p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES].update(
				p_instance->indexer_id, bvh_aabb);
		}
		p_instance->scenario->instance_aabbs[p_instance->array_index] =
			InstanceBounds(p_instance->transformed_aabb);
	}

	if (p_instance->visibility_index != -1) {
		p_instance->scenario->instance_visibility[p_instance->visibility_index].position =
			p_instance->transformed_aabb.get_center();
	}

	// move instance and repair
	pair_pass++;

	PairInstances pair;

	pair.instance = p_instance;
	pair.pair_allocator = &pair_allocator;
	pair.pair_pass = pair_pass;
	pair.pair_mask = 0;

	if ((1 << p_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) {
		pair.pair_mask |= 1 << RSE::INSTANCE_LIGHT;
		pair.pair_mask |= 1 << RSE::INSTANCE_VOXEL_GI;
		pair.pair_mask |= 1 << RSE::INSTANCE_LIGHTMAP;
		if (p_instance->base_type == RSE::INSTANCE_PARTICLES) {
			pair.pair_mask |= 1 << RSE::INSTANCE_PARTICLES_COLLISION;
		}

		pair.pair_mask |= geometry_instance_pair_mask;

		pair.bvh2 = &p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES];
	}
	else if (p_instance->base_type == RSE::INSTANCE_LIGHT) {
		pair.pair_mask |= RSE::INSTANCE_GEOMETRY_MASK;
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];

		RSE::LightBakeMode bake_mode = RSG::light_storage->light_get_bake_mode(p_instance->base);
		if (bake_mode == RSE::LIGHT_BAKE_STATIC || bake_mode == RSE::LIGHT_BAKE_DYNAMIC) {
			pair.pair_mask |= (1 << RSE::INSTANCE_VOXEL_GI);
			pair.bvh2 = &p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES];
		}
	}
	else if (p_instance->base_type == RSE::INSTANCE_LIGHTMAP) {
		pair.pair_mask = RSE::INSTANCE_GEOMETRY_MASK;
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
	}
	else if (geometry_instance_pair_mask & (1 << RSE::INSTANCE_REFLECTION_PROBE) &&
			   (p_instance->base_type == RSE::INSTANCE_REFLECTION_PROBE)) {
		pair.pair_mask = RSE::INSTANCE_GEOMETRY_MASK;
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
	}
	else if (geometry_instance_pair_mask & (1 << RSE::INSTANCE_DECAL) &&
			   (p_instance->base_type == RSE::INSTANCE_DECAL)) {
		pair.pair_mask = RSE::INSTANCE_GEOMETRY_MASK;
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
	}
	else if (p_instance->base_type == RSE::INSTANCE_PARTICLES_COLLISION) {
		pair.pair_mask = (1 << RSE::INSTANCE_PARTICLES);
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
	}
	else if (p_instance->base_type == RSE::INSTANCE_VOXEL_GI) {
		// lights and geometries
		pair.pair_mask = RSE::INSTANCE_GEOMETRY_MASK | (1 << RSE::INSTANCE_LIGHT);
		pair.bvh = &p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY];
		pair.bvh2 = &p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES];
	}

	pair.pair();

	p_instance->prev_transformed_aabb = p_instance->transformed_aabb;
}

void RendererSceneCull::_unpair_instance(Instance* p_instance)
{
	if (!p_instance->indexer_id.is_valid()) {
		return; // nothing to do
	}

	while (p_instance->pairs.first()) {
		InstancePair* pair = p_instance->pairs.first()->self();
		Instance* other_instance = p_instance == pair->a ? pair->b : pair->a;
		_instance_unpair(p_instance, other_instance);
		pair_allocator.free(pair);
	}

	if ((1 << p_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) {
		p_instance->scenario->indexers[Scenario::INDEXER_GEOMETRY].remove(p_instance->indexer_id);
	}
	else {
		p_instance->scenario->indexers[Scenario::INDEXER_VOLUMES].remove(p_instance->indexer_id);
	}

	p_instance->indexer_id = DynamicBVH::ID();

	// replace this by last
	int32_t swap_with_index = p_instance->scenario->instance_data.size() - 1;
	if (swap_with_index != p_instance->array_index) {
		Instance* swapped_instance = p_instance->scenario->instance_data[swap_with_index].instance;
		swapped_instance->array_index = p_instance->array_index; // swap
		p_instance->scenario->instance_data[p_instance->array_index] =
			p_instance->scenario->instance_data[swap_with_index];
		p_instance->scenario->instance_aabbs[p_instance->array_index] =
			p_instance->scenario->instance_aabbs[swap_with_index];

		if (swapped_instance->visibility_index != -1) {
			swapped_instance->scenario->instance_visibility[swapped_instance->visibility_index]
				.array_index = swapped_instance->array_index;
		}

		for (Instance* E : swapped_instance->visibility_dependencies) {
			Instance* dep_instance = E;
			if (dep_instance != p_instance && dep_instance->array_index != -1) {
				dep_instance->scenario->instance_data[dep_instance->array_index]
					.parent_array_index = swapped_instance->array_index;
			}
		}
	}

	// pop last
	p_instance->scenario->instance_data.pop_back();
	p_instance->scenario->instance_aabbs.pop_back();

	// uninitialize
	p_instance->array_index = -1;
	if ((1 << p_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) {
		// Clear these now because the InstanceData containing the dirty flags is gone
		InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(p_instance->base_data);
		ERR_FAIL_NULL(geom->geometry_instance);

		geom->geometry_instance->clear_light_instances();
		geom->geometry_instance->pair_reflection_probe_instances(nullptr, 0);
		geom->geometry_instance->pair_decal_instances(nullptr, 0);
		geom->geometry_instance->pair_voxel_gi_instances(nullptr, 0);
	}

	for (Instance* E : p_instance->visibility_dependencies) {
		Instance* dep_instance = E;
		if (dep_instance->array_index != -1) {
			dep_instance->scenario->instance_data[dep_instance->array_index].parent_array_index =
				-1;
			if ((1 << dep_instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) {
				dep_instance->scenario->instance_data[dep_instance->array_index]
					.instance_geometry->set_parent_fade_alpha(1.0f);
			}
		}
	}

	_update_instance_visibility_dependencies(p_instance);
}

void RendererSceneCull::_update_instance_aabb(Instance* p_instance) const
{
	AABB new_aabb;

	ERR_FAIL_COND(p_instance->base_type != RSE::INSTANCE_NONE && !p_instance->base.is_valid());

	switch (p_instance->base_type) {
	case RSE::INSTANCE_NONE: {
		// do nothing
	} break;
	case RSE::INSTANCE_MESH: {
		if (p_instance->custom_aabb) {
			new_aabb = *p_instance->custom_aabb;
		}
		else {
			new_aabb = RSG::mesh_storage->mesh_get_aabb(p_instance->base, p_instance->skeleton);
		}

	} break;

	case RSE::INSTANCE_MULTIMESH: {
		if (p_instance->custom_aabb) {
			new_aabb = *p_instance->custom_aabb;
		}
		else {
			new_aabb = RSG::mesh_storage->multimesh_get_aabb(p_instance->base);
		}

	} break;
	case RSE::INSTANCE_PARTICLES: {
		if (p_instance->custom_aabb) {
			new_aabb = *p_instance->custom_aabb;
		}
		else {
			new_aabb = RSG::particles_storage->particles_get_aabb(p_instance->base);
		}

	} break;
	case RSE::INSTANCE_PARTICLES_COLLISION: {
		new_aabb = RSG::particles_storage->particles_collision_get_aabb(p_instance->base);

	} break;
	case RSE::INSTANCE_FOG_VOLUME: {
		new_aabb = RSG::fog->fog_volume_get_aabb(p_instance->base);
	} break;
	case RSE::INSTANCE_VISIBLITY_NOTIFIER: {
		new_aabb = RSG::utilities->visibility_notifier_get_aabb(p_instance->base);
	} break;
	case RSE::INSTANCE_LIGHT: {
		new_aabb = RSG::light_storage->light_get_aabb(p_instance->base);

	} break;
	case RSE::INSTANCE_REFLECTION_PROBE: {
		new_aabb = RSG::light_storage->reflection_probe_get_aabb(p_instance->base);

	} break;
	case RSE::INSTANCE_DECAL: {
		new_aabb = RSG::texture_storage->decal_get_aabb(p_instance->base);

	} break;
	case RSE::INSTANCE_VOXEL_GI: {
		new_aabb = RSG::gi->voxel_gi_get_bounds(p_instance->base);

	} break;
	case RSE::INSTANCE_LIGHTMAP: {
		new_aabb = RSG::light_storage->lightmap_get_aabb(p_instance->base);

	} break;
	default: {
	}
	}

	if (p_instance->extra_margin) {
		new_aabb.grow_by(p_instance->extra_margin);
	}

	p_instance->aabb = new_aabb;
}

void RendererSceneCull::_update_instance_lightmap_captures(Instance* p_instance) const
{
	bool first_set = p_instance->lightmap_sh.is_empty();
	p_instance->lightmap_sh.resize(9);		  // using SH
	p_instance->lightmap_target_sh.resize(9); // using SH
	Color* instance_sh = p_instance->lightmap_target_sh.ptrw();
	bool inside = false;
	Color accum_sh[9];
	float accum_blend = 0.0;

	InstanceGeometryData* geom = static_cast<InstanceGeometryData*>(p_instance->base_data);
	// Don't blend if only one lightmap affects the dynamic object.
	// This prevents ambient light from being pure black until the object has fully entered the
	// LightmapGI's AABB.
	const bool use_blending = geom->lightmap_captures.size() >= 2;

	for (Instance* E : geom->lightmap_captures) {
		Instance* lightmap = E;

		bool interior = RSG::light_storage->lightmap_is_interior(lightmap->base);

		if (inside && !interior) {
			continue; // we are inside, ignore exteriors
		}

		Transform3D to_bounds = lightmap->transform.affine_inverse();
		Vector3 center =
			p_instance->transform.xform(p_instance->aabb.get_center()); // use aabb center

		Vector3 lm_pos = to_bounds.xform(center);

		AABB bounds = RSG::light_storage->lightmap_get_aabb(lightmap->base);

		Color sh[9];
		RSG::light_storage->lightmap_tap_sh_light(lightmap->base, lm_pos, sh);

		// rotate it
		Basis rot = lightmap->transform.basis.orthonormalized();
		for (int i = 0; i < 3; i++) {
			real_t csh[9];
			for (int j = 0; j < 9; j++) {
				csh[j] = sh[j][i];
			}
			rot.rotate_sh(csh);
			for (int j = 0; j < 9; j++) {
				sh[j][i] = csh[j];
			}
		}

		Vector3 inner_pos =
			((lm_pos - bounds.position) / bounds.size) * 2.0 - Vector3(1.0, 1.0, 1.0);

		real_t blend = 1.0;
		if (use_blending) {
			blend =
				MAX(Math::abs(inner_pos.x), MAX(Math::abs(inner_pos.y), Math::abs(inner_pos.z)));
			// Make blend more rounded.
			blend = Math::lerp(inner_pos.length(), blend, blend);
			blend *= blend;
			blend = MAX(0.0, 1.0 - blend);
		}

		if (interior && !inside) {
			// do not blend, just replace
			for (int j = 0; j < 9; j++) {
				accum_sh[j] = sh[j] * blend;
			}
			accum_blend = blend;
			inside = true;
		}
		else {
			for (int j = 0; j < 9; j++) {
				accum_sh[j] += sh[j] * blend;
			}
			accum_blend += blend;
		}
	}

	if (accum_blend > 0.0) {
		for (int j = 0; j < 9; j++) {
			instance_sh[j] = accum_sh[j] / accum_blend;
			if (first_set) {
				p_instance->lightmap_sh.write[j] = instance_sh[j];
			}
		}
	}

	ERR_FAIL_NULL(geom->geometry_instance);
	geom->geometry_instance->set_lightmap_capture(p_instance->lightmap_sh.ptr());
}

void RendererSceneCull::_light_instance_setup_directional_shadow(int p_shadow_index,
	Instance* p_instance, const Transform3D p_cam_transform, const Projection& p_cam_projection,
	bool p_cam_orthogonal, bool p_cam_vaspect)
{
	// For later tight culling, the light culler needs to know the details of the directional light.
	light_culler->prepare_directional_light(p_instance, p_shadow_index);

	InstanceLightData* light = static_cast<InstanceLightData*>(p_instance->base_data);

	Transform3D light_transform = p_instance->transform;
	light_transform.orthonormalize(); // scale does not count on lights

	real_t max_distance = p_cam_projection.get_z_far();
	real_t shadow_max =
		RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE);
	if (shadow_max > 0 && !p_cam_orthogonal) { // its impractical (and leads to unwanted behaviors)
											   // to set max distance in orthogonal camera
		max_distance = MIN(shadow_max, max_distance);
	}
	max_distance = MAX(max_distance, p_cam_projection.get_z_near() + 0.001);
	real_t min_distance = MIN(p_cam_projection.get_z_near(), max_distance);

	real_t pancake_size =
		RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE);

	real_t range = max_distance - min_distance;

	int splits = 0;
	switch (RSG::light_storage->light_directional_get_shadow_mode(p_instance->base)) {
	case RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL:
		splits = 1;
		break;
	case RSE::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS:
		splits = 2;
		break;
	case RSE::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS:
		splits = 4;
		break;
	}

	real_t distances[5];

	distances[0] = min_distance;
	for (int i = 0; i < splits; i++) {
		distances[i + 1] =
			min_distance + RSG::light_storage->light_get_param(p_instance->base,
							   RSE::LightParam(RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET + i)) *
							   range;
	};

	distances[splits] = max_distance;

	real_t texture_size = RSG::light_storage->get_directional_light_shadow_size(light->instance);

	bool overlap = RSG::light_storage->light_directional_get_blend_splits(p_instance->base);

	cull.shadow_count = p_shadow_index + 1;
	cull.shadows[p_shadow_index].cascade_count = splits;
	cull.shadows[p_shadow_index].light_instance = light->instance;
	cull.shadows[p_shadow_index].caster_mask =
		RSG::light_storage->light_get_shadow_caster_mask(p_instance->base);

	for (int i = 0; i < splits; i++) {
		RENDER_TIMESTAMP("Cull DirectionalLight3D, Split " + itos(i));

		// setup a camera matrix for that range!
		Projection camera_matrix;

		real_t aspect = p_cam_projection.get_aspect();

		if (p_cam_orthogonal) {
			Vector2 vp_he = p_cam_projection.get_viewport_half_extents();

			camera_matrix.set_orthogonal(vp_he.y * 2.0, aspect,
				distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], false);
		}
		else {
			real_t fov =
				p_cam_projection
					.get_fov(); // this is actually yfov, because set aspect tries to keep it
			camera_matrix.set_perspective(
				fov, aspect, distances[(i == 0 || !overlap) ? i : i - 1], distances[i + 1], true);
		}

		// obtain the frustum endpoints

		Vector3 endpoints[8]; // frustum plane endpoints
		bool res = camera_matrix.get_endpoints(p_cam_transform, endpoints);
		ERR_CONTINUE(!res);

		// obtain the light frustum ranges (given endpoints)

		Transform3D transform = light_transform; // discard scale and stabilize light

		Vector3 x_vec = transform.basis.get_column(Vector3::AXIS_X).normalized();
		Vector3 y_vec = transform.basis.get_column(Vector3::AXIS_Y).normalized();
		Vector3 z_vec = transform.basis.get_column(Vector3::AXIS_Z).normalized();
		// z_vec points against the camera, like in default opengl

		real_t x_min = 0.f, x_max = 0.f;
		real_t y_min = 0.f, y_max = 0.f;
		real_t z_min = 0.f, z_max = 0.f;

		// FIXME: z_max_cam is defined, computed, but not used below when setting up
		// ortho_camera. Commented out for now to fix warnings but should be investigated.
		real_t x_min_cam = 0.f, x_max_cam = 0.f;
		real_t y_min_cam = 0.f, y_max_cam = 0.f;
		real_t z_min_cam = 0.f;
		// real_t z_max_cam = 0.f;

		// real_t bias_scale = 1.0;
		// real_t aspect_bias_scale = 1.0;

		// used for culling

		for (int j = 0; j < 8; j++) {
			real_t d_x = x_vec.dot(endpoints[j]);
			real_t d_y = y_vec.dot(endpoints[j]);
			real_t d_z = z_vec.dot(endpoints[j]);

			if (j == 0 || d_x < x_min) {
				x_min = d_x;
			}
			if (j == 0 || d_x > x_max) {
				x_max = d_x;
			}

			if (j == 0 || d_y < y_min) {
				y_min = d_y;
			}
			if (j == 0 || d_y > y_max) {
				y_max = d_y;
			}

			if (j == 0 || d_z < z_min) {
				z_min = d_z;
			}
			if (j == 0 || d_z > z_max) {
				z_max = d_z;
			}
		}

		real_t radius = 0;
		real_t soft_shadow_expand = 0;
		Vector3 center;

		{
			// camera viewport stuff

			for (int j = 0; j < 8; j++) {
				center += endpoints[j];
			}
			center /= 8.0;

			// center=x_vec*(x_max-x_min)*0.5 + y_vec*(y_max-y_min)*0.5 + z_vec*(z_max-z_min)*0.5;

			for (int j = 0; j < 8; j++) {
				real_t d = center.distance_to(endpoints[j]);
				if (d > radius) {
					radius = d;
				}
			}

			radius *= texture_size / (texture_size - 2.0); // add a texel by each side

			z_min_cam = z_vec.dot(center) - radius;

			{
				float soft_shadow_angle =
					RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_SIZE);

				if (soft_shadow_angle > 0.0) {
					float z_range = (z_vec.dot(center) + radius + pancake_size) - z_min_cam;
					soft_shadow_expand = Math::tan(Math::deg_to_rad(soft_shadow_angle)) * z_range;

					x_max += soft_shadow_expand;
					y_max += soft_shadow_expand;

					x_min -= soft_shadow_expand;
					y_min -= soft_shadow_expand;
				}
			}

			// This trick here is what stabilizes the shadow (make potential jaggies to not move)
			// at the cost of some wasted resolution. Still, the quality increase is very well worth
			// it.
			const real_t unit = (radius + soft_shadow_expand) * 4.0 / texture_size;
			x_max_cam = Math::snapped(x_vec.dot(center) + radius + soft_shadow_expand, unit);
			x_min_cam = Math::snapped(x_vec.dot(center) - radius - soft_shadow_expand, unit);
			y_max_cam = Math::snapped(y_vec.dot(center) + radius + soft_shadow_expand, unit);
			y_min_cam = Math::snapped(y_vec.dot(center) - radius - soft_shadow_expand, unit);
		}

		// now that we know all ranges, we can proceed to make the light frustum planes, for culling
		// octree

		Vector<Plane> light_frustum_planes;
		light_frustum_planes.resize(6);

		// right/left
		light_frustum_planes.write[0] = Plane(x_vec, x_max);
		light_frustum_planes.write[1] = Plane(-x_vec, -x_min);
		// top/bottom
		light_frustum_planes.write[2] = Plane(y_vec, y_max);
		light_frustum_planes.write[3] = Plane(-y_vec, -y_min);
		// near/far
		light_frustum_planes.write[4] = Plane(z_vec, z_max + 1e6);
		light_frustum_planes.write[5] = Plane(-z_vec,
			-z_min); // z_min is ok, since casters further than far-light plane are not needed

		// a pre pass will need to be needed to determine the actual z-near to be used

		z_max = z_vec.dot(center) + radius + pancake_size;

		{
			Projection ortho_camera;
			real_t half_x = (x_max_cam - x_min_cam) * 0.5;
			real_t half_y = (y_max_cam - y_min_cam) * 0.5;

			ortho_camera.set_orthogonal(-half_x, half_x, -half_y, half_y, 0, (z_max - z_min_cam));

			Vector2 uv_scale(1.0 / (x_max_cam - x_min_cam), 1.0 / (y_max_cam - y_min_cam));

			Transform3D ortho_transform;
			ortho_transform.basis = transform.basis;
			ortho_transform.origin =
				x_vec * (x_min_cam + half_x) + y_vec * (y_min_cam + half_y) + z_vec * z_max;

			cull.shadows[p_shadow_index].cascades[i].frustum = Frustum(light_frustum_planes);
			cull.shadows[p_shadow_index].cascades[i].projection = ortho_camera;
			cull.shadows[p_shadow_index].cascades[i].transform = ortho_transform;
			cull.shadows[p_shadow_index].cascades[i].zfar = z_max - z_min_cam;
			cull.shadows[p_shadow_index].cascades[i].split = distances[i + 1];
			cull.shadows[p_shadow_index].cascades[i].shadow_texel_size =
				radius * 2.0 / texture_size;
			cull.shadows[p_shadow_index].cascades[i].bias_scale = (z_max - z_min_cam);
			cull.shadows[p_shadow_index].cascades[i].range_begin =
				z_max - z_vec.dot(p_cam_transform.origin);
			cull.shadows[p_shadow_index].cascades[i].uv_scale = uv_scale;
		}
	}
}

bool RendererSceneCull::_light_instance_update_shadow(Instance* p_instance,
	const Transform3D p_cam_transform, const Projection& p_cam_projection, bool p_cam_orthogonal,
	bool p_cam_vaspect, RID p_shadow_atlas, Scenario* p_scenario, float p_screen_mesh_lod_threshold,
	uint32_t p_visible_layers)
{
	InstanceLightData* light = static_cast<InstanceLightData*>(p_instance->base_data);

	Transform3D light_transform = p_instance->transform;
	light_transform.orthonormalize(); // scale does not count on lights

	bool animated_material_found = false;

	switch (RSG::light_storage->light_get_type(p_instance->base)) {
	case RSE::LIGHT_DIRECTIONAL: {
	} break;
	case RSE::LIGHT_OMNI: {
		RSE::LightOmniShadowMode shadow_mode =
			RSG::light_storage->light_omni_get_shadow_mode(p_instance->base);

		if (shadow_mode == RSE::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID ||
			!RSG::light_storage->light_instances_can_render_shadow_cube()) {
			if (max_shadows_used + 2 > MAX_UPDATE_SHADOWS) {
				return true;
			}
			for (int i = 0; i < 2; i++) {
				// using this one ensures that raster deferred will have it
				RENDER_TIMESTAMP("Cull OmniLight3D Shadow Paraboloid, Half " + itos(i));

				real_t radius =
					RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_RANGE);

				real_t z = i == 0 ? -1 : 1;
				Vector<Plane> planes;
				planes.resize(6);
				planes.write[0] = light_transform.xform(Plane(Vector3(0, 0, z), radius));
				planes.write[1] =
					light_transform.xform(Plane(Vector3(1, 0, z).normalized(), radius));
				planes.write[2] =
					light_transform.xform(Plane(Vector3(-1, 0, z).normalized(), radius));
				planes.write[3] =
					light_transform.xform(Plane(Vector3(0, 1, z).normalized(), radius));
				planes.write[4] =
					light_transform.xform(Plane(Vector3(0, -1, z).normalized(), radius));
				planes.write[5] = light_transform.xform(Plane(Vector3(0, 0, -z), 0));

				instance_shadow_cull_result.clear();

				Vector<Vector3> points =
					Geometry3D::compute_convex_mesh_points(&planes[0], planes.size());

				struct CullConvex
				{
					PagedArray<Instance*>* result;

					_FORCE_INLINE_ bool operator()(void* p_data)
					{
						Instance* p_instance = (Instance*)p_data;
						result->push_back(p_instance);
						return false;
					}
				};

				CullConvex cull_convex;
				cull_convex.result = &instance_shadow_cull_result;

				p_scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(
					planes.ptr(), planes.size(), points.ptr(), points.size(), cull_convex);

				RendererSceneRender::RenderShadowData& shadow_data =
					render_shadow_data[max_shadows_used++];

				if (!light->is_shadow_update_full()) {
					light_culler->cull_regular_light(instance_shadow_cull_result);
				}

				for (int j = 0; j < (int)instance_shadow_cull_result.size(); j++) {
					Instance* instance = instance_shadow_cull_result[j];
					const bool is_inactive_particle =
						(instance->base_type == RSE::INSTANCE_PARTICLES) &&
						RSG::particles_storage->particles_is_inactive(instance->base);
					if (!instance->visible ||
						!((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) ||
						!static_cast<InstanceGeometryData*>(instance->base_data)
							 ->can_cast_shadows ||
						!(p_visible_layers & instance->layer_mask &
							RSG::light_storage->light_get_shadow_caster_mask(p_instance->base)) ||
						is_inactive_particle) {
						continue;
					}
					else {
						if (static_cast<InstanceGeometryData*>(instance->base_data)
								->material_is_animated) {
							animated_material_found = true;
						}

						if (instance->mesh_instance.is_valid()) {
							RSG::mesh_storage->mesh_instance_check_for_update(
								instance->mesh_instance);
						}
					}

					shadow_data.instances.push_back(
						static_cast<InstanceGeometryData*>(instance->base_data)->geometry_instance);
				}

				RSG::mesh_storage->update_mesh_instances();

				RSG::light_storage->light_instance_set_shadow_transform(
					light->instance, Projection(), light_transform, radius, 0, i, 0);
				shadow_data.light = light->instance;
				shadow_data.pass = i;
			}
		}
		else { // shadow cube

			if (max_shadows_used + 6 > MAX_UPDATE_SHADOWS) {
				return true;
			}

			real_t radius =
				RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_RANGE);
			real_t z_near = MIN(0.025f, radius);
			Projection cm;
			cm.set_perspective(90, 1, z_near, radius);

			for (int i = 0; i < 6; i++) {
				RENDER_TIMESTAMP("Cull OmniLight3D Shadow Cube, Side " + itos(i));
				// using this one ensures that raster deferred will have it

				static const Vector3 view_normals[6] = {Vector3(+1, 0, 0), Vector3(-1, 0, 0),
					Vector3(0, -1, 0), Vector3(0, +1, 0), Vector3(0, 0, +1), Vector3(0, 0, -1)};
				static const Vector3 view_up[6] = {Vector3(0, -1, 0), Vector3(0, -1, 0),
					Vector3(0, 0, -1), Vector3(0, 0, +1), Vector3(0, -1, 0), Vector3(0, -1, 0)};

				Transform3D xform =
					light_transform * Transform3D().looking_at(view_normals[i], view_up[i]);

				Vector<Plane> planes = cm.get_projection_planes(xform);

				instance_shadow_cull_result.clear();

				Vector<Vector3> points =
					Geometry3D::compute_convex_mesh_points(&planes[0], planes.size());

				struct CullConvex
				{
					PagedArray<Instance*>* result;

					_FORCE_INLINE_ bool operator()(void* p_data)
					{
						Instance* p_instance = (Instance*)p_data;
						result->push_back(p_instance);
						return false;
					}
				};

				CullConvex cull_convex;
				cull_convex.result = &instance_shadow_cull_result;

				p_scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(
					planes.ptr(), planes.size(), points.ptr(), points.size(), cull_convex);

				RendererSceneRender::RenderShadowData& shadow_data =
					render_shadow_data[max_shadows_used++];

				if (!light->is_shadow_update_full()) {
					light_culler->cull_regular_light(instance_shadow_cull_result);
				}

				for (int j = 0; j < (int)instance_shadow_cull_result.size(); j++) {
					Instance* instance = instance_shadow_cull_result[j];
					const bool is_inactive_particle =
						(instance->base_type == RSE::INSTANCE_PARTICLES) &&
						RSG::particles_storage->particles_is_inactive(instance->base);
					if (!instance->visible ||
						!((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) ||
						!static_cast<InstanceGeometryData*>(instance->base_data)
							 ->can_cast_shadows ||
						!(p_visible_layers & instance->layer_mask &
							RSG::light_storage->light_get_shadow_caster_mask(p_instance->base)) ||
						is_inactive_particle) {
						continue;
					}
					else {
						if (static_cast<InstanceGeometryData*>(instance->base_data)
								->material_is_animated) {
							animated_material_found = true;
						}
						if (instance->mesh_instance.is_valid()) {
							RSG::mesh_storage->mesh_instance_check_for_update(
								instance->mesh_instance);
						}
					}

					shadow_data.instances.push_back(
						static_cast<InstanceGeometryData*>(instance->base_data)->geometry_instance);
				}

				RSG::mesh_storage->update_mesh_instances();
				RSG::light_storage->light_instance_set_shadow_transform(
					light->instance, cm, xform, radius, 0, i, 0);

				shadow_data.light = light->instance;
				shadow_data.pass = i;
			}

			// restore the regular DP matrix
			// RSG::light_storage->light_instance_set_shadow_transform(light->instance,
			// Projection(), light_transform, radius, 0, 0, 0);
		}

	} break;
	case RSE::LIGHT_SPOT: {
		RENDER_TIMESTAMP("Cull SpotLight3D Shadow");

		if (max_shadows_used + 1 > MAX_UPDATE_SHADOWS) {
			return true;
		}

		real_t radius =
			RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_RANGE);
		real_t angle =
			RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_SPOT_ANGLE);
		real_t z_near = MIN(0.025f, radius);

		Projection cm;
		cm.set_perspective(angle * 2.0, 1.0, z_near, radius);

		Vector<Plane> planes = cm.get_projection_planes(light_transform);

		instance_shadow_cull_result.clear();

		Vector<Vector3> points = Geometry3D::compute_convex_mesh_points(&planes[0], planes.size());

		struct CullConvex
		{
			PagedArray<Instance*>* result;

			_FORCE_INLINE_ bool operator()(void* p_data)
			{
				Instance* p_instance = (Instance*)p_data;
				result->push_back(p_instance);
				return false;
			}
		};

		CullConvex cull_convex;
		cull_convex.result = &instance_shadow_cull_result;

		p_scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(
			planes.ptr(), planes.size(), points.ptr(), points.size(), cull_convex);

		RendererSceneRender::RenderShadowData& shadow_data = render_shadow_data[max_shadows_used++];

		if (!light->is_shadow_update_full()) {
			light_culler->cull_regular_light(instance_shadow_cull_result);
		}

		for (int j = 0; j < (int)instance_shadow_cull_result.size(); j++) {
			Instance* instance = instance_shadow_cull_result[j];
			const bool is_inactive_particle =
				(instance->base_type == RSE::INSTANCE_PARTICLES) &&
				RSG::particles_storage->particles_is_inactive(instance->base);
			if (!instance->visible || !((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) ||
				!static_cast<InstanceGeometryData*>(instance->base_data)->can_cast_shadows ||
				!(p_visible_layers & instance->layer_mask &
					RSG::light_storage->light_get_shadow_caster_mask(p_instance->base)) ||
				is_inactive_particle) {
				continue;
			}
			else {
				if (static_cast<InstanceGeometryData*>(instance->base_data)->material_is_animated) {
					animated_material_found = true;
				}

				if (instance->mesh_instance.is_valid()) {
					RSG::mesh_storage->mesh_instance_check_for_update(instance->mesh_instance);
				}
			}
			shadow_data.instances.push_back(
				static_cast<InstanceGeometryData*>(instance->base_data)->geometry_instance);
		}

		RSG::mesh_storage->update_mesh_instances();

		RSG::light_storage->light_instance_set_shadow_transform(
			light->instance, cm, light_transform, radius, 0, 0, 0);
		shadow_data.light = light->instance;
		shadow_data.pass = 0;

	} break;
	case RSE::LIGHT_AREA: {
		if (max_shadows_used + 1 > MAX_UPDATE_SHADOWS) {
			return true;
		}
		RENDER_TIMESTAMP("Cull AreaLight3D Shadow Paraboloid");

		real_t radius =
			RSG::light_storage->light_get_param(p_instance->base, RSE::LIGHT_PARAM_RANGE);
		Vector2 half_size = RSG::light_storage->light_area_get_size(p_instance->base) / 2.0;

		real_t z = -1;
		Vector<Plane> planes;
		planes.resize(6);
		planes.write[0] = light_transform.xform(Plane(Vector3(0, 0, z), radius));
		planes.write[1] =
			light_transform.xform(Plane(Vector3(1, 0, 0).normalized(), radius + half_size.x));
		planes.write[2] =
			light_transform.xform(Plane(Vector3(-1, 0, 0).normalized(), radius + half_size.x));
		planes.write[3] =
			light_transform.xform(Plane(Vector3(0, 1, 0).normalized(), radius + half_size.y));
		planes.write[4] =
			light_transform.xform(Plane(Vector3(0, -1, 0).normalized(), radius + half_size.y));
		planes.write[5] = light_transform.xform(Plane(Vector3(0, 0, -z), 0));

		instance_shadow_cull_result.clear();

		Vector<Vector3> points = Geometry3D::compute_convex_mesh_points(&planes[0], planes.size());

		struct CullConvex
		{
			PagedArray<Instance*>* result;

			_FORCE_INLINE_ bool operator()(void* p_data)
			{
				Instance* p_instance = (Instance*)p_data;
				result->push_back(p_instance);
				return false;
			}
		};

		CullConvex cull_convex;
		cull_convex.result = &instance_shadow_cull_result;

		p_scenario->indexers[Scenario::INDEXER_GEOMETRY].convex_query(
			planes.ptr(), planes.size(), points.ptr(), points.size(), cull_convex);

		RendererSceneRender::RenderShadowData& shadow_data = render_shadow_data[max_shadows_used++];

		if (!light->is_shadow_update_full()) {
			light_culler->cull_regular_light(instance_shadow_cull_result);
		}

		for (int j = 0; j < (int)instance_shadow_cull_result.size(); j++) {
			Instance* instance = instance_shadow_cull_result[j];
			const bool is_inactive_particle =
				(instance->base_type == RSE::INSTANCE_PARTICLES) &&
				RSG::particles_storage->particles_is_inactive(instance->base);
			if (!instance->visible || !((1 << instance->base_type) & RSE::INSTANCE_GEOMETRY_MASK) ||
				!static_cast<InstanceGeometryData*>(instance->base_data)->can_cast_shadows ||
				!(p_visible_layers & instance->layer_mask &
					RSG::light_storage->light_get_shadow_caster_mask(p_instance->base)) ||
				is_inactive_particle) {
				continue;
			}
			else {
				if (static_cast<InstanceGeometryData*>(instance->base_data)->material_is_animated) {
					animated_material_found = true;
				}

				if (instance->mesh_instance.is_valid()) {
					RSG::mesh_storage->mesh_instance_check_for_update(instance->mesh_instance);
				}
			}

			shadow_data.instances.push_back(
				static_cast<InstanceGeometryData*>(instance->base_data)->geometry_instance);
		}

		RSG::mesh_storage->update_mesh_instances();

		RSG::light_storage->light_instance_set_shadow_transform(
			light->instance, Projection(), light_transform, radius, 0, 0, 0);
		shadow_data.light = light->instance;
		shadow_data.pass = 0;
	}
	}

	return animated_material_found;
}

void RendererSceneCull::_visibility_cull(
	const VisibilityCullData& cull_data, uint64_t p_from, uint64_t p_to)
{
	Scenario* scenario = cull_data.scenario;
	for (unsigned int i = p_from; i < p_to; i++) {
		InstanceVisibilityData& vd = scenario->instance_visibility[i];
		InstanceData& idata = scenario->instance_data[vd.array_index];

		if (idata.parent_array_index >= 0) {
			uint32_t parent_flags = scenario->instance_data[idata.parent_array_index].flags;

			if ((parent_flags & InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN) ||
				!(parent_flags & (InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE |
									 InstanceData::FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN))) {
				idata.flags |= InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN;
				idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE;
				idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN;
				continue;
			}
		}

		int range_check =
			_visibility_range_check<true>(vd, cull_data.camera_position, cull_data.viewport_mask);

		if (range_check == -1) {
			idata.flags |= InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN;
			idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE;
			idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN;
		}
		else if (range_check == 1) {
			idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN;
			idata.flags |= InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE;
			idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN;
		}
		else {
			idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN;
			idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE;
			if (range_check == 2) {
				idata.flags |= InstanceData::FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN;
			}
			else {
				idata.flags &= ~InstanceData::FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN;
			}
		}
	}
}

template <bool p_fade_check>
int RendererSceneCull::_visibility_range_check(
	InstanceVisibilityData& r_vis_data, const Vector3& p_camera_pos, uint64_t p_viewport_mask)
{
	float dist = p_camera_pos.distance_to(r_vis_data.position);
	const RSE::VisibilityRangeFadeMode& fade_mode = r_vis_data.fade_mode;

	float begin_offset = -r_vis_data.range_begin_margin;
	float end_offset = r_vis_data.range_end_margin;

	if (fade_mode == RSE::VISIBILITY_RANGE_FADE_DISABLED &&
		!(p_viewport_mask & r_vis_data.viewport_state)) {
		begin_offset = -begin_offset;
		end_offset = -end_offset;
	}

	if (r_vis_data.range_end > 0.0f && dist > r_vis_data.range_end + end_offset) {
		r_vis_data.viewport_state &= ~p_viewport_mask;
		return -1;
	}
	else if (r_vis_data.range_begin > 0.0f && dist < r_vis_data.range_begin + begin_offset) {
		r_vis_data.viewport_state &= ~p_viewport_mask;
		return 1;
	}
	else {
		r_vis_data.viewport_state |= p_viewport_mask;
		if (p_fade_check) {
			if (fade_mode != RSE::VISIBILITY_RANGE_FADE_DISABLED) {
				r_vis_data.children_fade_alpha = 1.0f;
				if (r_vis_data.range_end > 0.0f && dist > r_vis_data.range_end - end_offset) {
					if (fade_mode == RSE::VISIBILITY_RANGE_FADE_DEPENDENCIES) {
						r_vis_data.children_fade_alpha =
							MIN(1.0f, (dist - (r_vis_data.range_end - end_offset)) /
										  (2.0f * r_vis_data.range_end_margin));
					}
					return 2;
				}
				else if (r_vis_data.range_begin > 0.0f &&
						   dist < r_vis_data.range_begin - begin_offset) {
					if (fade_mode == RSE::VISIBILITY_RANGE_FADE_DEPENDENCIES) {
						r_vis_data.children_fade_alpha =
							MIN(1.0f, 1.0 - (dist - (r_vis_data.range_begin + begin_offset)) /
												(2.0f * r_vis_data.range_begin_margin));
					}
					return 2;
				}
			}
		}
		return 0;
	}
}

bool RendererSceneCull::_visibility_parent_check(
	const CullData& p_cull_data, const InstanceData& p_instance_data)
{
	if (p_instance_data.parent_array_index == -1) {
		return true;
	}
	const uint32_t& parent_flags =
		p_cull_data.scenario->instance_data[p_instance_data.parent_array_index].flags;
	return ((parent_flags & InstanceData::FLAG_VISIBILITY_DEPENDENCY_NEEDS_CHECK) ==
			   InstanceData::FLAG_VISIBILITY_DEPENDENCY_HIDDEN_CLOSE_RANGE) ||
		   (parent_flags & InstanceData::FLAG_VISIBILITY_DEPENDENCY_FADE_CHILDREN);
}

void RendererSceneCull::_scene_particles_set_view_axis(
	RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis)
{
	RSG::particles_storage->particles_set_view_axis(p_particles, p_axis, p_up_axis);
}

RID RendererSceneCull::_render_get_environment(RID p_camera, RID p_scenario)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	if (camera && scene_render->is_environment(camera->env)) {
		return camera->env;
	}

	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	if (!scenario) {
		return RID();
	}
	if (scene_render->is_environment(scenario->environment)) {
		return scenario->environment;
	}

	if (scene_render->is_environment(scenario->fallback_environment)) {
		return scenario->fallback_environment;
	}

	return RID();
}

RID RendererSceneCull::_render_get_compositor(RID p_camera, RID p_scenario)
{
	Camera* camera = camera_owner.get_or_null(p_camera);
	if (camera && scene_render->is_compositor(camera->compositor)) {
		return camera->compositor;
	}

	Scenario* scenario = scenario_owner.get_or_null(p_scenario);
	if (scenario && scene_render->is_compositor(scenario->compositor)) {
		return scenario->compositor;
	}

	return RID();
}

void RendererSceneCull::render_empty_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
	RID p_scenario, RID p_shadow_atlas, float p_window_output_max_value)
{
#ifndef _3D_DISABLED
	Scenario* scenario = scenario_owner.get_or_null(p_scenario);

	RID environment;
	if (scenario->environment.is_valid()) {
		environment = scenario->environment;
	}
	else {
		environment = scenario->fallback_environment;
	}
	RID compositor = scenario->compositor;
	RENDER_TIMESTAMP("Render Empty 3D Scene");

	RendererSceneRender::CameraData camera_data;
	camera_data.set_camera(Transform3D(), Projection(), true, false);

	scene_render->render_scene(p_render_buffers, &camera_data, &camera_data,
		PagedArray<RenderGeometryInstance*>(), PagedArray<RID>(), PagedArray<RID>(),
		PagedArray<RID>(), PagedArray<RID>(), PagedArray<RID>(), PagedArray<RID>(), environment,
		RID(), compositor, p_shadow_atlas, RID(), scenario->reflection_atlas, RID(), 0, 0, nullptr,
		0, nullptr, 0, p_window_output_max_value, nullptr);
#endif
}

void RendererSceneCull::render_probes()
{
	/* REFLECTION PROBES */

	SelfList<InstanceReflectionProbeData>* ref_probe = reflection_probe_render_list.first();
	Vector<SelfList<InstanceReflectionProbeData>*> done_list;

	bool busy = false;

	if (ref_probe) {
		RENDER_TIMESTAMP("Render ReflectionProbes");

		while (ref_probe) {
			SelfList<InstanceReflectionProbeData>* next = ref_probe->next();
			RID base = ref_probe->self()->owner->base;

			switch (RSG::light_storage->reflection_probe_get_update_mode(base)) {
			case RSE::REFLECTION_PROBE_UPDATE_ONCE: {
				if (busy) { // Already rendering something.
					break;
				}

				bool done = _render_reflection_probe_step(
					ref_probe->self()->owner, ref_probe->self()->render_step);
				if (done) {
					done_list.push_back(ref_probe);
				}
				else {
					ref_probe->self()->render_step++;
				}

				busy = true; // Do not render another one of this kind.
			} break;
			case RSE::REFLECTION_PROBE_UPDATE_ALWAYS: {
				int step = 0;
				bool done = false;
				while (!done) {
					done = _render_reflection_probe_step(ref_probe->self()->owner, step);
					step++;
				}

				done_list.push_back(ref_probe);
			} break;
			}

			ref_probe = next;
		}

		// Now remove from our list
		for (SelfList<InstanceReflectionProbeData>* rp : done_list) {
			reflection_probe_render_list.remove(rp);
		}
	}

	/* VOXEL GIS */

	SelfList<InstanceVoxelGIData>* voxel_gi = voxel_gi_update_list.first();

	if (voxel_gi) {
		RENDER_TIMESTAMP("Render VoxelGI");
	}

	while (voxel_gi) {
		SelfList<InstanceVoxelGIData>* next = voxel_gi->next();

		InstanceVoxelGIData* probe = voxel_gi->self();
		// Instance *instance_probe = probe->owner;

		// check if probe must be setup, but don't do if on the lighting thread

		bool cache_dirty = false;
		int cache_count = 0;
		{
			int light_cache_size = probe->light_cache.size();
			const InstanceVoxelGIData::LightCache* caches = probe->light_cache.ptr();
			const RID* instance_caches = probe->light_instances.ptr();

			int idx = 0; // must count visible lights
			for (Instance* E : probe->lights) {
				Instance* instance = E;
				InstanceLightData* instance_light = (InstanceLightData*)instance->base_data;
				if (!instance->visible) {
					continue;
				}
				if (cache_dirty) {
					// do nothing, since idx must count all visible lights anyway
				}
				else if (idx >= light_cache_size) {
					cache_dirty = true;
				}
				else {
					const InstanceVoxelGIData::LightCache* cache = &caches[idx];

					if (instance_caches[idx] != instance_light->instance ||
						cache->has_shadow != RSG::light_storage->light_has_shadow(instance->base) ||
						cache->type != RSG::light_storage->light_get_type(instance->base) ||
						cache->transform != instance->transform ||
						cache->color != RSG::light_storage->light_get_color(instance->base) ||
						cache->energy != RSG::light_storage->light_get_param(
											 instance->base, RSE::LIGHT_PARAM_ENERGY) ||
						cache->intensity != RSG::light_storage->light_get_param(
												instance->base, RSE::LIGHT_PARAM_INTENSITY) ||
						cache->bake_energy != RSG::light_storage->light_get_param(instance->base,
												  RSE::LIGHT_PARAM_INDIRECT_ENERGY) ||
						cache->radius != RSG::light_storage->light_get_param(
											 instance->base, RSE::LIGHT_PARAM_RANGE) ||
						cache->attenuation != RSG::light_storage->light_get_param(
												  instance->base, RSE::LIGHT_PARAM_ATTENUATION) ||
						cache->spot_angle != RSG::light_storage->light_get_param(
												 instance->base, RSE::LIGHT_PARAM_SPOT_ANGLE) ||
						cache->spot_attenuation !=
							RSG::light_storage->light_get_param(
								instance->base, RSE::LIGHT_PARAM_SPOT_ATTENUATION) ||
						cache->area_size !=
							RSG::light_storage->light_area_get_size(instance->base) ||
						cache->area_normalize_energy !=
							RSG::light_storage->light_area_get_normalize_energy(instance->base) ||
						cache->area_texture !=
							RSG::light_storage->light_area_get_texture(instance->base)) {
						cache_dirty = true;
					}
				}

				idx++;
			}

			for (const Instance* instance : probe->owner->scenario->directional_lights) {
				InstanceLightData* instance_light = (InstanceLightData*)instance->base_data;
				if (!instance->visible) {
					continue;
				}
				if (cache_dirty) {
					// do nothing, since idx must count all visible lights anyway
				}
				else if (idx >= light_cache_size) {
					cache_dirty = true;
				}
				else {
					const InstanceVoxelGIData::LightCache* cache = &caches[idx];

					if (instance_caches[idx] != instance_light->instance ||
						cache->has_shadow != RSG::light_storage->light_has_shadow(instance->base) ||
						cache->type != RSG::light_storage->light_get_type(instance->base) ||
						cache->transform != instance->transform ||
						cache->color != RSG::light_storage->light_get_color(instance->base) ||
						cache->energy != RSG::light_storage->light_get_param(
											 instance->base, RSE::LIGHT_PARAM_ENERGY) ||
						cache->intensity != RSG::light_storage->light_get_param(
												instance->base, RSE::LIGHT_PARAM_INTENSITY) ||
						cache->bake_energy != RSG::light_storage->light_get_param(instance->base,
												  RSE::LIGHT_PARAM_INDIRECT_ENERGY) ||
						cache->radius != RSG::light_storage->light_get_param(
											 instance->base, RSE::LIGHT_PARAM_RANGE) ||
						cache->attenuation != RSG::light_storage->light_get_param(
												  instance->base, RSE::LIGHT_PARAM_ATTENUATION) ||
						cache->spot_angle != RSG::light_storage->light_get_param(
												 instance->base, RSE::LIGHT_PARAM_SPOT_ANGLE) ||
						cache->spot_attenuation !=
							RSG::light_storage->light_get_param(
								instance->base, RSE::LIGHT_PARAM_SPOT_ATTENUATION) ||
						cache->sky_mode !=
							RSG::light_storage->light_directional_get_sky_mode(instance->base)) {
						cache_dirty = true;
					}
				}

				idx++;
			}

			if (idx != light_cache_size) {
				cache_dirty = true;
			}

			cache_count = idx;
		}

		bool update_lights = scene_render->voxel_gi_needs_update(probe->probe_instance);

		if (cache_dirty) {
			probe->light_cache.resize(cache_count);
			probe->light_instances.resize(cache_count);

			if (cache_count) {
				InstanceVoxelGIData::LightCache* caches = probe->light_cache.ptrw();
				RID* instance_caches = probe->light_instances.ptrw();

				int idx = 0; // must count visible lights
				for (Instance* E : probe->lights) {
					Instance* instance = E;
					InstanceLightData* instance_light = (InstanceLightData*)instance->base_data;
					if (!instance->visible) {
						continue;
					}

					InstanceVoxelGIData::LightCache* cache = &caches[idx];

					instance_caches[idx] = instance_light->instance;
					cache->has_shadow = RSG::light_storage->light_has_shadow(instance->base);
					cache->type = RSG::light_storage->light_get_type(instance->base);
					cache->transform = instance->transform;
					cache->color = RSG::light_storage->light_get_color(instance->base);
					cache->energy = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_ENERGY);
					cache->intensity = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_INTENSITY);
					cache->bake_energy = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
					cache->radius =
						RSG::light_storage->light_get_param(instance->base, RSE::LIGHT_PARAM_RANGE);
					cache->attenuation = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_ATTENUATION);
					cache->spot_angle = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_SPOT_ANGLE);
					cache->spot_attenuation = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_SPOT_ATTENUATION);
					cache->area_size = RSG::light_storage->light_area_get_size(instance->base);
					cache->area_normalize_energy =
						RSG::light_storage->light_area_get_normalize_energy(instance->base);
					cache->area_texture =
						RSG::light_storage->light_area_get_texture(instance->base);
					idx++;
				}
				for (const Instance* instance : probe->owner->scenario->directional_lights) {
					InstanceLightData* instance_light = (InstanceLightData*)instance->base_data;
					if (!instance->visible) {
						continue;
					}

					InstanceVoxelGIData::LightCache* cache = &caches[idx];

					instance_caches[idx] = instance_light->instance;
					cache->has_shadow = RSG::light_storage->light_has_shadow(instance->base);
					cache->type = RSG::light_storage->light_get_type(instance->base);
					cache->transform = instance->transform;
					cache->color = RSG::light_storage->light_get_color(instance->base);
					cache->energy = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_ENERGY);
					cache->intensity = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_INTENSITY);
					cache->bake_energy = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
					cache->radius =
						RSG::light_storage->light_get_param(instance->base, RSE::LIGHT_PARAM_RANGE);
					cache->attenuation = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_ATTENUATION);
					cache->spot_angle = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_SPOT_ANGLE);
					cache->spot_attenuation = RSG::light_storage->light_get_param(
						instance->base, RSE::LIGHT_PARAM_SPOT_ATTENUATION);
					cache->sky_mode =
						RSG::light_storage->light_directional_get_sky_mode(instance->base);

					idx++;
				}
			}

			update_lights = true;
		}

		scene_cull_result.geometry_instances.clear();

		RID instance_pair_buffer[MAX_INSTANCE_PAIRS];

		for (Instance* E : probe->dynamic_geometries) {
			Instance* ins = E;
			if (!ins->visible) {
				continue;
			}
			InstanceGeometryData* geom = (InstanceGeometryData*)ins->base_data;

			if (ins->scenario && ins->array_index >= 0 &&
				(ins->scenario->instance_data[ins->array_index].flags &
					InstanceData::FLAG_GEOM_VOXEL_GI_DIRTY)) {
				uint32_t idx = 0;
				for (const Instance* F : geom->voxel_gi_instances) {
					InstanceVoxelGIData* voxel_gi2 =
						static_cast<InstanceVoxelGIData*>(F->base_data);

					instance_pair_buffer[idx++] = voxel_gi2->probe_instance;
					if (idx == MAX_INSTANCE_PAIRS) {
						break;
					}
				}

				ERR_FAIL_NULL(geom->geometry_instance);
				geom->geometry_instance->pair_voxel_gi_instances(instance_pair_buffer, idx);

				ins->scenario->instance_data[ins->array_index].flags &=
					~InstanceData::FLAG_GEOM_VOXEL_GI_DIRTY;
			}

			ERR_FAIL_NULL(geom->geometry_instance);
			scene_cull_result.geometry_instances.push_back(geom->geometry_instance);
		}

		scene_render->voxel_gi_update(probe->probe_instance, update_lights, probe->light_instances,
			scene_cull_result.geometry_instances);

		voxel_gi_update_list.remove(voxel_gi);

		voxel_gi = next;
	}
}

void RendererSceneCull::render_particle_colliders()
{
	while (heightfield_particle_colliders_update_list.begin()) {
		Instance* hfpc = *heightfield_particle_colliders_update_list.begin();

		if (hfpc->scenario && hfpc->base_type == RSE::INSTANCE_PARTICLES_COLLISION &&
			RSG::particles_storage->particles_collision_is_heightfield(hfpc->base)) {
			// update heightfield
			instance_cull_result.clear();
			scene_cull_result.geometry_instances.clear();

			struct CullAABB
			{
				PagedArray<Instance*>* result;
				uint32_t heightfield_mask;

				_FORCE_INLINE_ bool operator()(void* p_data)
				{
					Instance* p_instance = (Instance*)p_data;
					if (p_instance->layer_mask & heightfield_mask) {
						result->push_back(p_instance);
					}
					return false;
				}
			};

			CullAABB cull_aabb;
			cull_aabb.result = &instance_cull_result;
			cull_aabb.heightfield_mask =
				RSG::particles_storage->particles_collision_get_height_field_mask(hfpc->base);
			hfpc->scenario->indexers[Scenario::INDEXER_GEOMETRY].aabb_query(
				hfpc->transformed_aabb, cull_aabb);
			hfpc->scenario->indexers[Scenario::INDEXER_VOLUMES].aabb_query(
				hfpc->transformed_aabb, cull_aabb);

			for (int i = 0; i < (int)instance_cull_result.size(); i++) {
				Instance* instance = instance_cull_result[i];
				if (!instance ||
					!((1 << instance->base_type) &
						(RSE::INSTANCE_GEOMETRY_MASK &
							(~(1 << RSE::INSTANCE_PARTICLES))))) { // all but particles to avoid
																   // self collision
					continue;
				}
				InstanceGeometryData* geom =
					static_cast<InstanceGeometryData*>(instance->base_data);
				ERR_FAIL_NULL(geom->geometry_instance);
				scene_cull_result.geometry_instances.push_back(geom->geometry_instance);
			}

			scene_render->render_particle_collider_heightfield(
				hfpc->base, hfpc->transform, scene_cull_result.geometry_instances);
		}
		heightfield_particle_colliders_update_list.remove(
			heightfield_particle_colliders_update_list.begin());
	}
}

void RendererSceneCull::update_visibility_notifiers()
{
	SelfList<InstanceVisibilityNotifierData>* E = visible_notifier_list.first();
	while (E) {
		SelfList<InstanceVisibilityNotifierData>* N = E->next();

		InstanceVisibilityNotifierData* visibility_notifier = E->self();
		if (visibility_notifier->just_visible) {
			visibility_notifier->just_visible = false;

			RSG::utilities->visibility_notifier_call(
				visibility_notifier->base, true, RSG::threaded);
		}
		else {
			if (visibility_notifier->visible_in_frame != RSG::rasterizer->get_frame_number()) {
				visible_notifier_list.remove(E);

				RSG::utilities->visibility_notifier_call(
					visibility_notifier->base, false, RSG::threaded);
			}
		}

		E = N;
	}
}

RendererSceneCull* RendererSceneCull::singleton = nullptr;

void RendererSceneCull::set_scene_render(RendererSceneRender* p_scene_render)
{
	scene_render = p_scene_render;
	geometry_instance_pair_mask = scene_render->geometry_instance_get_pair_mask();
}

/* INTERPOLATION API */

void RendererSceneCull::update_interpolation_tick(bool p_process)
{
	// MultiMesh: Update interpolation in storage.
	RSG::mesh_storage->update_interpolation_tick(p_process);
}

void RendererSceneCull::update_interpolation_frame(bool p_process)
{
	// MultiMesh: Update interpolation in storage.
	RSG::mesh_storage->update_interpolation_frame(p_process);
}

void RendererSceneCull::set_physics_interpolation_enabled(bool p_enabled)
{
	_interpolation_data.interpolation_enabled = p_enabled;
}

RendererSceneCull::~RendererSceneCull()
{
	instance_cull_result.reset();
	instance_shadow_cull_result.reset();

	for (uint32_t i = 0; i < MAX_UPDATE_SHADOWS; i++) {
		render_shadow_data[i].instances.reset();
	}
	for (uint32_t i = 0; i < SDFGI_MAX_CASCADES * SDFGI_MAX_REGIONS_PER_CASCADE; i++) {
		render_sdfgi_data[i].instances.reset();
	}

	scene_cull_result.reset();
	for (InstanceCullResult& thread : scene_cull_result_threads) {
		thread.reset();
	}
	scene_cull_result_threads.clear();

	memdelete(dummy_occlusion_culling);

	if (light_culler) {
		memdelete(light_culler);
		light_culler = nullptr;
	}
}


