/**************************************************************************/
/*  openxr_spatial_plane_tracking.cpp                                     */
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

#include "../../openxr_api.h"
#include "core/config/project_settings.h"
#include "openxr_spatial_entity_extension.h"
#include "openxr_spatial_plane_tracking.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "servers/xr/xr_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"

void OpenXRSpatialPlaneTrackingCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialPlaneTrackingCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

#endif

bool OpenXRSpatialCapabilityConfigurationPlaneTracking::has_valid_configuration() const
{
	ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), false);

	return OpenXRSpatialPlaneTrackingCapability::is_supported();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationPlaneTracking::get_configuration()
{
	ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), nullptr);

	if (OpenXRSpatialPlaneTrackingCapability::is_supported()) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		plane_enabled_components.clear();

		// Guaranteed components:
		plane_enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);
		plane_enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT);

		// Optional components:
		if (get_supports_mesh_2d()) {
			plane_enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT);
		}
		else if (get_supports_polygons()) {
			plane_enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT);
		}
		if (get_supports_labels()) {
			plane_enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT);
		}

		// Set up our enabled components.
		plane_config.enabledComponentCount = plane_enabled_components.size();
		plane_config.enabledComponents = plane_enabled_components.ptr();

		// and return this.
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&plane_config;
	}

	return nullptr;
}

bool OpenXRSpatialCapabilityConfigurationPlaneTracking::get_supports_mesh_2d()
{
	if (supports_mesh_2d == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_mesh_2d =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT, XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT)
				? 1
				: 0;
	}

	return supports_mesh_2d == 1;
}

bool OpenXRSpatialCapabilityConfigurationPlaneTracking::get_supports_polygons()
{
	if (supports_polygons == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_polygons =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT, XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT)
				? 1
				: 0;
	}

	return supports_polygons == 1;
}

bool OpenXRSpatialCapabilityConfigurationPlaneTracking::get_supports_labels()
{
	if (supports_labels == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_labels =
			OpenXRSpatialEntityExtension::supports_component_type(XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
				XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT)
				? 1
				: 0;
	}

	return supports_labels == 1;
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationPlaneTracking::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : plane_enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

void OpenXRSpatialComponentPlaneAlignmentList::set_capacity(uint32_t p_capacity)
{
	plane_alignment_data.resize(p_capacity);

	plane_alignment_list.planeAlignmentCount = uint32_t(plane_alignment_data.size());
	plane_alignment_list.planeAlignments = plane_alignment_data.ptrw();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentPlaneAlignmentList::get_component_type() const
{
	return XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT;
}

void* OpenXRSpatialComponentPlaneAlignmentList::get_structure_data(void* p_next)
{
	plane_alignment_list.next = p_next;
	return &plane_alignment_list;
}

XrSpatialPlaneAlignmentEXT OpenXRSpatialComponentPlaneAlignmentList::get_plane_alignment(
	int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, plane_alignment_data.size(), XR_SPATIAL_PLANE_ALIGNMENT_MAX_ENUM_EXT);

	return plane_alignment_data[p_index];
}

OpenXRSpatialComponentPlaneAlignmentList::PlaneAlignment
OpenXRSpatialComponentPlaneAlignmentList::_get_plane_alignment(int64_t p_index) const
{
	return (PlaneAlignment)get_plane_alignment(p_index);
}

void OpenXRSpatialComponentPolygon2DList::set_capacity(uint32_t p_capacity)
{
	polygon2d_data.resize(p_capacity);

	polygon2d_list.polygonCount = uint32_t(polygon2d_data.size());
	polygon2d_list.polygons = polygon2d_data.ptrw();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentPolygon2DList::get_component_type() const
{
	return XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT;
}

void* OpenXRSpatialComponentPolygon2DList::get_structure_data(void* p_next)
{
	polygon2d_list.next = p_next;
	return &polygon2d_list;
}

Transform3D OpenXRSpatialComponentPolygon2DList::get_transform(int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, polygon2d_data.size(), Transform3D());

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	return OpenXRAPI::transform_from_pose(polygon2d_data[p_index].origin);
}

PackedVector2Array OpenXRSpatialComponentPolygon2DList::get_vertices(
	RID p_snapshot, int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, polygon2d_data.size(), PackedVector2Array());

	const XrSpatialBufferEXT& buffer = polygon2d_data[p_index].vertexBuffer;
	if (buffer.bufferId == XR_NULL_SPATIAL_BUFFER_ID_EXT) {
		// We don't have data (yet).
		return PackedVector2Array();
	}

	ERR_FAIL_COND_V(buffer.bufferType != XR_SPATIAL_BUFFER_TYPE_VECTOR2F_EXT, PackedVector2Array());

	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), PackedVector2Array());

	return OpenXRSpatialEntityExtension::get_vector2_buffer(p_snapshot, buffer.bufferId);
}

void OpenXRSpatialComponentPlaneSemanticLabelList::set_capacity(uint32_t p_capacity)
{
	plane_semantic_label_data.resize(p_capacity);

	plane_semantic_label_list.semanticLabelCount = uint32_t(plane_semantic_label_data.size());
	plane_semantic_label_list.semanticLabels = plane_semantic_label_data.ptrw();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentPlaneSemanticLabelList::get_component_type() const
{
	return XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT;
}

void* OpenXRSpatialComponentPlaneSemanticLabelList::get_structure_data(void* p_next)
{
	plane_semantic_label_list.next = p_next;
	return &plane_semantic_label_list;
}

XrSpatialPlaneSemanticLabelEXT
OpenXRSpatialComponentPlaneSemanticLabelList::get_plane_semantic_label(int64_t p_index) const
{
	ERR_FAIL_INDEX_V(
		p_index, plane_semantic_label_data.size(), XR_SPATIAL_PLANE_SEMANTIC_LABEL_MAX_ENUM_EXT);

	return plane_semantic_label_data[p_index];
}

OpenXRSpatialComponentPlaneSemanticLabelList::PlaneSemanticLabel
OpenXRSpatialComponentPlaneSemanticLabelList::_get_plane_semantic_label(int64_t p_index) const
{
	return (PlaneSemanticLabel)get_plane_semantic_label(p_index);
}

HashMap<String, bool*> OpenXRSpatialPlaneTrackingCapability::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_plane_tracking")) {
		request_extensions[XR_EXT_SPATIAL_PLANE_TRACKING_EXTENSION_NAME] =
			&data->spatial_plane_tracking_ext;
	}

	return request_extensions;
}

bool OpenXRSpatialPlaneTrackingCapability::is_supported()
{ return data != nullptr && data->spatial_plane_tracking_supported; }

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		// Trigger new discovery.
		data->need_discovery = true;
	}
}

void OpenXRSpatialPlaneTrackingCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialPlaneTrackingCapability::on_session_destroyed() {}

void OpenXRSpatialPlaneTrackingCapability::on_process() {}


