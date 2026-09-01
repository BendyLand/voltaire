/**************************************************************************/
/*  voxel_gi.cpp                                                          */
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

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/3d/voxelizer.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/camera_attributes.h"
#include "servers/rendering/rendering_server.h"
#include "voxel_gi.h"





void VoxelGIData::allocate(const Transform3D& p_to_cell_xform, const AABB& p_aabb,
	const Vector3& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	RS::get_singleton()->voxel_gi_allocate_data(probe, p_to_cell_xform, p_aabb, p_octree_size,
		p_octree_cells, p_data_cells, p_distance_field, p_level_counts);
	bounds = p_aabb;
	to_cell_xform = p_to_cell_xform;
	octree_size = p_octree_size;
}

AABB VoxelGIData::get_bounds() const { return bounds; }

Vector3 VoxelGIData::get_octree_size() const { return octree_size; }

Vector<uint8_t> VoxelGIData::get_octree_cells() const
{
	return RS::get_singleton()->voxel_gi_get_octree_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_data_cells() const
{
	return RS::get_singleton()->voxel_gi_get_data_cells(probe);
}

Vector<uint8_t> VoxelGIData::get_distance_field() const
{
	return RS::get_singleton()->voxel_gi_get_distance_field(probe);
}

Vector<int> VoxelGIData::get_level_counts() const
{
	return RS::get_singleton()->voxel_gi_get_level_counts(probe);
}

Transform3D VoxelGIData::get_to_cell_xform() const { return to_cell_xform; }

void VoxelGIData::set_dynamic_range(float p_range)
{
	RS::get_singleton()->voxel_gi_set_dynamic_range(probe, p_range);
	dynamic_range = p_range;
}

float VoxelGIData::get_dynamic_range() const { return dynamic_range; }

void VoxelGIData::set_propagation(float p_propagation)
{
	RS::get_singleton()->voxel_gi_set_propagation(probe, p_propagation);
	propagation = p_propagation;
}

float VoxelGIData::get_propagation() const { return propagation; }

void VoxelGIData::set_energy(float p_energy)
{
	RS::get_singleton()->voxel_gi_set_energy(probe, p_energy);
	energy = p_energy;
}

float VoxelGIData::get_energy() const { return energy; }

void VoxelGIData::set_bias(float p_bias)
{
	RS::get_singleton()->voxel_gi_set_bias(probe, p_bias);
	bias = p_bias;
}

float VoxelGIData::get_bias() const { return bias; }

void VoxelGIData::set_normal_bias(float p_normal_bias)
{
	RS::get_singleton()->voxel_gi_set_normal_bias(probe, p_normal_bias);
	normal_bias = p_normal_bias;
}

float VoxelGIData::get_normal_bias() const { return normal_bias; }

void VoxelGIData::set_interior(bool p_enable)
{
	RS::get_singleton()->voxel_gi_set_interior(probe, p_enable);
	interior = p_enable;
}

bool VoxelGIData::is_interior() const { return interior; }

void VoxelGIData::set_use_two_bounces(bool p_enable)
{
	RS::get_singleton()->voxel_gi_set_use_two_bounces(probe, p_enable);
	use_two_bounces = p_enable;
}

bool VoxelGIData::is_using_two_bounces() const { return use_two_bounces; }

RID VoxelGIData::get_rid() const { return probe; }

VoxelGIData::VoxelGIData() { probe = RS::get_singleton()->voxel_gi_create(); }

VoxelGIData::~VoxelGIData()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(probe);
}

//////////////////////
//////////////////////

void VoxelGI::set_probe_data(const Ref<VoxelGIData>& p_data)
{
	if (p_data.is_valid()) {
		RS::get_singleton()->instance_set_base(get_instance(), p_data->get_rid());
		RS::get_singleton()->voxel_gi_set_baked_exposure_normalization(
			p_data->get_rid(), _get_camera_exposure_normalization());
	}
	else {
		RS::get_singleton()->instance_set_base(get_instance(), RID());
	}

	probe_data = p_data;
	update_configuration_warnings();
}

Ref<VoxelGIData> VoxelGI::get_probe_data() const { return probe_data; }

void VoxelGI::set_subdiv(Subdiv p_subdiv)
{
	ERR_FAIL_INDEX(p_subdiv, SUBDIV_MAX);
	subdiv = p_subdiv;
	update_gizmos();
}

VoxelGI::Subdiv VoxelGI::get_subdiv() const { return subdiv; }

void VoxelGI::set_size(const Vector3& p_size)
{
	// Prevent very small size dimensions as these breaks baking if other size dimensions are set
	// very high.
	size = p_size.maxf(1.0);
	update_gizmos();
}

Vector3 VoxelGI::get_size() const { return size; }

void VoxelGI::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;

	if (probe_data.is_valid()) {
		RS::get_singleton()->voxel_gi_set_baked_exposure_normalization(
			probe_data->get_rid(), _get_camera_exposure_normalization());
	}
}

Ref<CameraAttributes> VoxelGI::get_camera_attributes() const { return camera_attributes; }





VoxelGI::BakeBeginFunc VoxelGI::bake_begin_function = nullptr;
VoxelGI::BakeStepFunc VoxelGI::bake_step_function = nullptr;
VoxelGI::BakeEndFunc VoxelGI::bake_end_function = nullptr;

static int voxelizer_plot_bake_base = 0;
static int voxelizer_plot_bake_total = 0;

static bool voxelizer_plot_bake_step_function(int current, int)
{
	return VoxelGI::bake_step_function(
		(voxelizer_plot_bake_base + current) * 500 / voxelizer_plot_bake_total,
		RTR("Plotting Meshes"));
}

static bool voxelizer_sdf_bake_step_function(int current, int total)
{
	return VoxelGI::bake_step_function(
		500 + current * 500 / total, RTR("Generating Distance Field"));
}

Vector3i VoxelGI::get_estimated_cell_size() const
{
	static const int subdiv_value[SUBDIV_MAX] = {6, 7, 8, 9};
	int cell_subdiv = subdiv_value[subdiv];
	int axis_cell_size[3];
	AABB bounds = AABB(-size / 2, size);
	int longest_axis = bounds.get_longest_axis_index();
	axis_cell_size[longest_axis] = 1 << cell_subdiv;

	for (int i = 0; i < 3; i++) {
		if (i == longest_axis) {
			continue;
		}

		axis_cell_size[i] = axis_cell_size[longest_axis];
		float axis_size = bounds.size[longest_axis];

		// shrink until fit subdiv
		while (axis_size / 2.0 >= bounds.size[i]) {
			axis_size /= 2.0;
			axis_cell_size[i] >>= 1;
		}
	}

	return Vector3i(axis_cell_size[0], axis_cell_size[1], axis_cell_size[2]);
}

void VoxelGI::_debug_bake() { bake(nullptr, true); }

float VoxelGI::_get_camera_exposure_normalization()
{
	float exposure_normalization = 1.0;
	if (camera_attributes.is_valid()) {
		exposure_normalization = camera_attributes->get_exposure_multiplier();
		if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
			exposure_normalization = camera_attributes->calculate_exposure_normalization();
		}
	}
	return exposure_normalization;
}

AABB VoxelGI::get_aabb() const { return AABB(-size / 2, size); }

PackedStringArray VoxelGI::get_configuration_warnings() const
{
	PackedStringArray warnings = VisualInstance3D::get_configuration_warnings();

	if (OS::get_singleton()->get_current_rendering_method() == "gl_compatibility") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Compatibility "
							   "renderer yet. Support will be added in a future release."));
	}
	else if (OS::get_singleton()->get_current_rendering_method() == "dummy") {
		warnings.push_back(RTR("VoxelGI nodes are not supported when using the Dummy renderer."));
	}
	else if (probe_data.is_null()) {
		warnings.push_back(RTR(
			"No VoxelGI data set, so this node is disabled. Bake static objects to enable GI."));
	}
	return warnings;
}

VoxelGI::VoxelGI()
{
	voxel_gi = RS::get_singleton()->voxel_gi_create();
	set_disable_scale(true);
}

VoxelGI::~VoxelGI()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(voxel_gi);
}


