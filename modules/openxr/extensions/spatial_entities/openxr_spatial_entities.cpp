/**************************************************************************/
/*  openxr_spatial_entities.cpp                                           */
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

#include <cstring>
#include <openxr/openxr.h>
#include "../../openxr_api.h"
#include "core/config/project_settings.h"
#include "openxr_spatial_entities.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "servers/xr/xr_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationBaseHeader

OpenXRSpatialCapabilityConfigurationBaseHeader::OpenXRSpatialCapabilityConfigurationBaseHeader(
	CapabilityType p_capability_type)
{
	set_capability_type(p_capability_type);
}

OpenXRSpatialCapabilityConfigurationBaseHeader::~OpenXRSpatialCapabilityConfigurationBaseHeader() {}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_capability_type(
	CapabilityType p_capability_type)
{
	capability_type = p_capability_type;
	enabled_components.clear();

	if (capability_type == CAPABILITY_ARUCO) {
		int dict = GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/aruco_dict");
		aruco_dict = (XrSpatialMarkerArucoDictEXT)(XR_SPATIAL_MARKER_ARUCO_DICT_4X4_50_EXT + dict);
	}
	else if (capability_type == CAPABILITY_APRIL_TAG) {
		int dict = GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/april_tag_dict");
		april_dict =
			(XrSpatialMarkerAprilTagDictEXT)(XR_SPATIAL_MARKER_APRIL_TAG_DICT_16H5_EXT + dict);
	}
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::has_valid_configuration() const
{
	switch (capability_type) {
	case CAPABILITY_ANCHOR: {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), false);
		return OpenXRSpatialAnchorCapability::is_spatial_anchor_supported();
	}
	case CAPABILITY_PLANE_TRACKING: {
		ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), false);
		return OpenXRSpatialPlaneTrackingCapability::is_supported();
	}
	case CAPABILITY_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported();
	}
	case CAPABILITY_MICRO_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported();
	}
	case CAPABILITY_ARUCO: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_aruco_supported();
	}
	case CAPABILITY_APRIL_TAG: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported();
	}
	case CAPABILITY_NONE:
	default:
		return false;
	}
}

uint64_t OpenXRSpatialCapabilityConfigurationBaseHeader::_get_configurationgd()
{
	return (uint64_t)get_configuration();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationBaseHeader::get_configuration()
{
	switch (capability_type) {
	case CAPABILITY_ANCHOR: {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialAnchorCapability::is_spatial_anchor_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT);
		if (OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT);
		}

		config.anchor = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_ANCHOR_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_ANCHOR_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.anchor;
	}
	case CAPABILITY_PLANE_TRACKING: {
		ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialPlaneTrackingCapability::is_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT);

		if (get_supports_mesh_2d()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT);
		}
		else if (get_supports_polygons()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT);
		}
		if (get_supports_labels()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT);
		}

		config.plane = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_PLANE_TRACKING_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.plane;
	}
	case CAPABILITY_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.qrcode = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_QR_CODE_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.qrcode;
	}
	case CAPABILITY_MICRO_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.micro_qrcode = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_MICRO_QR_CODE_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.micro_qrcode;
	}
	case CAPABILITY_ARUCO: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_aruco_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.aruco = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_ARUCO_MARKER_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
			aruco_dict,
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.aruco;
	}
	case CAPABILITY_APRIL_TAG: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.april_tag = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_APRIL_TAG_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
			april_dict,
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.april_tag;
	}
	case CAPABILITY_NONE:
	default:
		return nullptr;
	}
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_mesh_2d()
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

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_polygons()
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

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_labels()
{
	if (supports_labels == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_labels =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
				XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT)
				? 1
				: 0;
	}

	return supports_labels == 1;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_aruco_dict(
	XrSpatialMarkerArucoDictEXT p_dict)
{
	aruco_dict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::_set_aruco_dict(ArucoDict p_dict)
{
	set_aruco_dict((XrSpatialMarkerArucoDictEXT)p_dict);
}

XrSpatialMarkerArucoDictEXT OpenXRSpatialCapabilityConfigurationBaseHeader::get_aruco_dict() const
{
	return aruco_dict;
}

OpenXRSpatialCapabilityConfigurationBaseHeader::ArucoDict
OpenXRSpatialCapabilityConfigurationBaseHeader::_get_aruco_dict() const
{
	return (ArucoDict)get_aruco_dict();
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_april_dict(
	XrSpatialMarkerAprilTagDictEXT p_dict)
{
	april_dict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::_set_april_dict(AprilTagDict p_dict)
{
	set_april_dict((XrSpatialMarkerAprilTagDictEXT)p_dict);
}

XrSpatialMarkerAprilTagDictEXT
OpenXRSpatialCapabilityConfigurationBaseHeader::get_april_dict() const
{
	return april_dict;
}

OpenXRSpatialCapabilityConfigurationBaseHeader::AprilTagDict
OpenXRSpatialCapabilityConfigurationBaseHeader::_get_april_dict() const
{
	return (AprilTagDict)get_april_dict();
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationBaseHeader::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialComponentData

OpenXRSpatialComponentData::OpenXRSpatialComponentData(ComponentKind p_kind)
	: component_kind(p_kind)
{
	_init_structure_header();
}

OpenXRSpatialComponentData::~OpenXRSpatialComponentData() { _free_buffers(); }

void OpenXRSpatialComponentData::_free_buffers()
{
	if (data_ptr != nullptr) {
		memfree(data_ptr);
		data_ptr = nullptr;
	}
	if (entity_states_ptr != nullptr) {
		memfree(entity_states_ptr);
		entity_states_ptr = nullptr;
	}
	capacity = 0;
}

void OpenXRSpatialComponentData::_init_structure_header()
{
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		structure.query_result = {
			XR_TYPE_SPATIAL_COMPONENT_DATA_QUERY_RESULT_EXT,
			nullptr,
			capacity,
			0,
			static_cast<XrSpatialEntityIdEXT*>(data_ptr),
			capacity,
			0,
			entity_states_ptr,
		};
		break;
	case COMPONENT_BOUNDED_2D:
		structure.bounded2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_BOUNDED_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialBounded2DDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_BOUNDED_3D:
		structure.bounded3d_list = {
			XR_TYPE_SPATIAL_COMPONENT_BOUNDED_3D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrBoxf*>(data_ptr),
		};
		break;
	case COMPONENT_PARENT:
		structure.parent_list = {
			XR_TYPE_SPATIAL_COMPONENT_PARENT_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialEntityIdEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MESH_2D:
		structure.mesh2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_MESH_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMeshDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MESH_3D:
		structure.mesh3d_list = {
			XR_TYPE_SPATIAL_COMPONENT_MESH_3D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMeshDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_ANCHOR:
		structure.anchor_list = {
			XR_TYPE_SPATIAL_COMPONENT_ANCHOR_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrPosef*>(data_ptr),
		};
		break;
	case COMPONENT_PERSISTENCE:
		structure.persistence_list = {
			XR_TYPE_SPATIAL_COMPONENT_PERSISTENCE_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPersistenceDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_PLANE_ALIGNMENT:
		structure.plane_alignment_list = {
			XR_TYPE_SPATIAL_COMPONENT_PLANE_ALIGNMENT_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPlaneAlignmentEXT*>(data_ptr),
		};
		break;
	case COMPONENT_POLYGON_2D:
		structure.polygon2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_POLYGON_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPolygon2DDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		structure.plane_semantic_label_list = {
			XR_TYPE_SPATIAL_COMPONENT_PLANE_SEMANTIC_LABEL_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPlaneSemanticLabelEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MARKER:
		structure.marker_list = {
			XR_TYPE_SPATIAL_COMPONENT_MARKER_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMarkerDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_NONE:
	default:
		break;
	}
}

void OpenXRSpatialComponentData::set_component_kind(ComponentKind p_kind)
{
	if (component_kind == p_kind) {
		return;
	}
	_free_buffers();
	component_kind = p_kind;
	_init_structure_header();
}

void OpenXRSpatialComponentData::set_capacity(uint32_t p_capacity)
{
	if (component_kind == COMPONENT_NONE) {
		_free_buffers();
		return;
	}

	if (p_capacity == 0) {
		_free_buffers();
		_init_structure_header();
		return;
	}

	size_t elem_size = 0;
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		elem_size = sizeof(XrSpatialEntityIdEXT);
		break;
	case COMPONENT_BOUNDED_2D:
		elem_size = sizeof(XrSpatialBounded2DDataEXT);
		break;
	case COMPONENT_BOUNDED_3D:
		elem_size = sizeof(XrBoxf);
		break;
	case COMPONENT_PARENT:
		elem_size = sizeof(XrSpatialEntityIdEXT);
		break;
	case COMPONENT_MESH_2D:
	case COMPONENT_MESH_3D:
		elem_size = sizeof(XrSpatialMeshDataEXT);
		break;
	case COMPONENT_ANCHOR:
		elem_size = sizeof(XrPosef);
		break;
	case COMPONENT_PERSISTENCE:
		elem_size = sizeof(XrSpatialPersistenceDataEXT);
		break;
	case COMPONENT_PLANE_ALIGNMENT:
		elem_size = sizeof(XrSpatialPlaneAlignmentEXT);
		break;
	case COMPONENT_POLYGON_2D:
		elem_size = sizeof(XrSpatialPolygon2DDataEXT);
		break;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		elem_size = sizeof(XrSpatialPlaneSemanticLabelEXT);
		break;
	case COMPONENT_MARKER:
		elem_size = sizeof(XrSpatialMarkerDataEXT);
		break;
	case COMPONENT_NONE:
	default:
		return;
	}

	uint32_t old_capacity = capacity;
	data_ptr = memrealloc(data_ptr, elem_size * p_capacity);
	ERR_FAIL_NULL(data_ptr);
	if (p_capacity > old_capacity) {
		memset(static_cast<uint8_t*>(data_ptr) + elem_size * old_capacity, 0,
			elem_size * (p_capacity - old_capacity));
	}

	if (component_kind == COMPONENT_QUERY_RESULT) {
		entity_states_ptr = static_cast<XrSpatialEntityTrackingStateEXT*>(memrealloc(
			entity_states_ptr, sizeof(XrSpatialEntityTrackingStateEXT) * p_capacity));
		ERR_FAIL_NULL(entity_states_ptr);
		if (p_capacity > old_capacity) {
			memset(entity_states_ptr + old_capacity, 0,
				sizeof(XrSpatialEntityTrackingStateEXT) * (p_capacity - old_capacity));
		}
	}

	capacity = p_capacity;
	_init_structure_header();
}

int64_t OpenXRSpatialComponentData::_get_component_typegd() const
{
	return (int64_t)get_component_type();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentData::get_component_type() const
{
	switch (component_kind) {
	case COMPONENT_BOUNDED_2D:
		return XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT;
	case COMPONENT_BOUNDED_3D:
		return XR_SPATIAL_COMPONENT_TYPE_BOUNDED_3D_EXT;
	case COMPONENT_PARENT:
		return XR_SPATIAL_COMPONENT_TYPE_PARENT_EXT;
	case COMPONENT_MESH_2D:
		return XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT;
	case COMPONENT_MESH_3D:
		return XR_SPATIAL_COMPONENT_TYPE_MESH_3D_EXT;
	case COMPONENT_ANCHOR:
		return XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT;
	case COMPONENT_PERSISTENCE:
		return XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT;
	case COMPONENT_PLANE_ALIGNMENT:
		return XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT;
	case COMPONENT_POLYGON_2D:
		return XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		return XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT;
	case COMPONENT_MARKER:
		return XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT;
	case COMPONENT_QUERY_RESULT:
	case COMPONENT_NONE:
	default:
		return XR_SPATIAL_COMPONENT_TYPE_MAX_ENUM_EXT;
	}
}

void* OpenXRSpatialComponentData::get_structure_data(void* p_next)
{
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		structure.query_result.next = p_next;
		structure.query_result.entityIdCountOutput = 0;
		structure.query_result.entityStateCountOutput = 0;
		return &structure.query_result;
	case COMPONENT_BOUNDED_2D:
		structure.bounded2d_list.next = p_next;
		return &structure.bounded2d_list;
	case COMPONENT_BOUNDED_3D:
		structure.bounded3d_list.next = p_next;
		return &structure.bounded3d_list;
	case COMPONENT_PARENT:
		structure.parent_list.next = p_next;
		return &structure.parent_list;
	case COMPONENT_MESH_2D:
		structure.mesh2d_list.next = p_next;
		return &structure.mesh2d_list;
	case COMPONENT_MESH_3D:
		structure.mesh3d_list.next = p_next;
		return &structure.mesh3d_list;
	case COMPONENT_ANCHOR:
		structure.anchor_list.next = p_next;
		return &structure.anchor_list;
	case COMPONENT_PERSISTENCE:
		structure.persistence_list.next = p_next;
		return &structure.persistence_list;
	case COMPONENT_PLANE_ALIGNMENT:
		structure.plane_alignment_list.next = p_next;
		return &structure.plane_alignment_list;
	case COMPONENT_POLYGON_2D:
		structure.polygon2d_list.next = p_next;
		return &structure.polygon2d_list;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		structure.plane_semantic_label_list.next = p_next;
		return &structure.plane_semantic_label_list;
	case COMPONENT_MARKER:
		structure.marker_list.next = p_next;
		return &structure.marker_list;
	case COMPONENT_NONE:
	default:
		return nullptr;
	}
}

XrSpatialEntityIdEXT OpenXRSpatialComponentData::get_entity_id(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_QUERY_RESULT, XR_NULL_ENTITY);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_NULL_ENTITY);

	return static_cast<const XrSpatialEntityIdEXT*>(data_ptr)[p_index];
}

uint64_t OpenXRSpatialComponentData::_get_entity_id(int64_t p_index) const
{
	return (uint64_t)get_entity_id(p_index);
}

XrSpatialEntityTrackingStateEXT OpenXRSpatialComponentData::get_entity_state(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_QUERY_RESULT, XR_SPATIAL_ENTITY_TRACKING_STATE_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_ENTITY_TRACKING_STATE_MAX_ENUM_EXT);

	return entity_states_ptr[p_index];
}

String OpenXRSpatialComponentData::get_entity_tracking_state_name(
	XrSpatialEntityTrackingStateEXT p_tracking_state)
{
	XR_ENUM_SWITCH(XrSpatialEntityTrackingStateEXT, p_tracking_state)
}

Transform3D OpenXRSpatialComponentData::get_center_pose(int64_t p_index) const
{
	if (component_kind == COMPONENT_BOUNDED_2D) {
		return get_center_pose_2d(p_index);
	}
	if (component_kind == COMPONENT_BOUNDED_3D) {
		return get_center_pose_3d(p_index);
	}
	ERR_FAIL_V(Transform3D());
}

Transform3D OpenXRSpatialComponentData::get_center_pose_2d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_2D, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	const XrSpatialBounded2DDataEXT* arr =
		static_cast<const XrSpatialBounded2DDataEXT*>(data_ptr);
	return OpenXRAPI::transform_from_pose(arr[p_index].center);
}

Transform3D OpenXRSpatialComponentData::get_center_pose_3d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_3D, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	const XrBoxf* arr = static_cast<const XrBoxf*>(data_ptr);
	return OpenXRAPI::transform_from_pose(arr[p_index].center);
}

Vector2 OpenXRSpatialComponentData::get_size_2d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_2D, Vector2());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Vector2());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Vector2());

	const XrExtent2Df& extents =
		static_cast<const XrSpatialBounded2DDataEXT*>(data_ptr)[p_index].extents;
	return Vector2(extents.width, extents.height);
}

Vector3 OpenXRSpatialComponentData::get_size_3d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_3D, Vector3());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Vector3());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Vector3());

	const XrExtent3Df& extents = static_cast<const XrBoxf*>(data_ptr)[p_index].extents;
	return Vector3(extents.width, extents.height, extents.depth);
}

RID OpenXRSpatialComponentData::get_parent(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_PARENT, RID());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, RID());
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), RID());

	const XrSpatialEntityIdEXT* arr = static_cast<const XrSpatialEntityIdEXT*>(data_ptr);
	return OpenXRSpatialEntityExtension::find_spatial_entity(arr[p_index]);
}

Transform3D OpenXRSpatialComponentData::get_transform(int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	switch (component_kind) {
	case COMPONENT_MESH_2D:
	case COMPONENT_MESH_3D:
		return OpenXRAPI::transform_from_pose(
			static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].origin);
	case COMPONENT_POLYGON_2D:
		return OpenXRAPI::transform_from_pose(
			static_cast<const XrSpatialPolygon2DDataEXT*>(data_ptr)[p_index].origin);
	default:
		ERR_FAIL_V(Transform3D());
	}
}

PackedVector2Array OpenXRSpatialComponentData::get_vertices(RID p_snapshot, int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, PackedVector2Array());

	const XrSpatialBufferEXT* buffer = nullptr;
	if (component_kind == COMPONENT_MESH_2D) {
		buffer = &static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].vertexBuffer;
	}
	else if (component_kind == COMPONENT_POLYGON_2D) {
		buffer = &static_cast<const XrSpatialPolygon2DDataEXT*>(data_ptr)[p_index].vertexBuffer;
	}
	else {
		ERR_FAIL_V(PackedVector2Array());
	}

	if (buffer->bufferId == XR_NULL_SPATIAL_BUFFER_ID_EXT) {
		return PackedVector2Array();
	}

	ERR_FAIL_COND_V(
		buffer->bufferType != XR_SPATIAL_BUFFER_TYPE_VECTOR2F_EXT, PackedVector2Array());
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), PackedVector2Array());

	return OpenXRSpatialEntityExtension::get_vector2_buffer(p_snapshot, buffer->bufferId);
}

PackedInt32Array OpenXRSpatialComponentData::get_indices(RID p_snapshot, int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MESH_2D, PackedInt32Array());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, PackedInt32Array());

	const XrSpatialBufferEXT& buffer =
		static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].indexBuffer;
	if (buffer.bufferId == XR_NULL_SPATIAL_BUFFER_ID_EXT) {
		return PackedInt32Array();
	}

	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), PackedInt32Array());

	PackedInt32Array ret;

	switch (buffer.bufferType) {
	case XR_SPATIAL_BUFFER_TYPE_UINT8_EXT: {
		PackedByteArray data =
			OpenXRSpatialEntityExtension::get_uint8_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	case XR_SPATIAL_BUFFER_TYPE_UINT16_EXT: {
		Vector<uint16_t> data =
			OpenXRSpatialEntityExtension::get_uint16_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	case XR_SPATIAL_BUFFER_TYPE_UINT32_EXT: {
		Vector<uint32_t> data =
			OpenXRSpatialEntityExtension::get_uint32_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	default: {
		ERR_FAIL_V_MSG(PackedInt32Array(), "OpenXR: Unsupported buffer type for indices.");
	} break;
	}

	return ret;
}

Ref<Mesh> OpenXRSpatialComponentData::get_mesh(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MESH_3D, nullptr);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, nullptr);

	// TODO implement, need to convert mesh data to Godot mesh resource
	return nullptr;
}

Transform3D OpenXRSpatialComponentData::get_entity_pose(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_ANCHOR, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	return OpenXRAPI::transform_from_pose(static_cast<const XrPosef*>(data_ptr)[p_index]);
}

XrUuid OpenXRSpatialComponentData::get_persistent_uuid(int64_t p_index) const
{
	XrUuid null_uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	ERR_FAIL_COND_V(component_kind != COMPONENT_PERSISTENCE, null_uuid);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, null_uuid);

	return static_cast<const XrSpatialPersistenceDataEXT*>(data_ptr)[p_index].persistUuid;
}

String OpenXRSpatialComponentData::_get_persistent_uuid(int64_t p_index) const
{
	return OpenXRUtil::string_from_xruuid(get_persistent_uuid(p_index));
}

XrSpatialPersistenceStateEXT OpenXRSpatialComponentData::get_persistent_state(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_PERSISTENCE, XR_SPATIAL_PERSISTENCE_STATE_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PERSISTENCE_STATE_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPersistenceDataEXT*>(data_ptr)[p_index].persistState;
}

uint64_t OpenXRSpatialComponentData::_get_persistent_state(int64_t p_index) const
{
	return (uint64_t)get_persistent_state(p_index);
}

String OpenXRSpatialComponentData::get_persistence_state_name(XrSpatialPersistenceStateEXT p_state)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceStateEXT, p_state)
}

XrSpatialPlaneAlignmentEXT OpenXRSpatialComponentData::get_plane_alignment(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_PLANE_ALIGNMENT, XR_SPATIAL_PLANE_ALIGNMENT_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PLANE_ALIGNMENT_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPlaneAlignmentEXT*>(data_ptr)[p_index];
}

OpenXRSpatialComponentData::PlaneAlignment OpenXRSpatialComponentData::_get_plane_alignment(
	int64_t p_index) const
{
	return (PlaneAlignment)get_plane_alignment(p_index);
}

XrSpatialPlaneSemanticLabelEXT OpenXRSpatialComponentData::get_plane_semantic_label(
	int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_PLANE_SEMANTIC_LABEL,
		XR_SPATIAL_PLANE_SEMANTIC_LABEL_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PLANE_SEMANTIC_LABEL_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPlaneSemanticLabelEXT*>(data_ptr)[p_index];
}

OpenXRSpatialComponentData::PlaneSemanticLabel
OpenXRSpatialComponentData::_get_plane_semantic_label(int64_t p_index) const
{
	return (PlaneSemanticLabel)get_plane_semantic_label(p_index);
}

OpenXRSpatialComponentData::MarkerType OpenXRSpatialComponentData::get_marker_type(
	int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, MARKER_TYPE_UNKNOWN);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, MARKER_TYPE_UNKNOWN);

	switch (static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].capability) {
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT:
		return MARKER_TYPE_QRCODE;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT:
		return MARKER_TYPE_MICRO_QRCODE;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT:
		return MARKER_TYPE_ARUCO;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT:
		return MARKER_TYPE_APRIL_TAG;
	default:
		return MARKER_TYPE_UNKNOWN;
	}
}

uint32_t OpenXRSpatialComponentData::get_marker_id(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, 0);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, 0);

	return static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].markerId;
}

XrSpatialBufferEXT OpenXRSpatialComponentData::get_marker_data(int64_t p_index) const
{
	XrSpatialBufferEXT null_buffer = {
		XR_NULL_SPATIAL_BUFFER_ID_EXT, XR_SPATIAL_BUFFER_TYPE_UNKNOWN_EXT};

	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, null_buffer);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, null_buffer);

	return static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].data;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialEntityTracker

OpenXRSpatialEntityTracker::OpenXRSpatialEntityTracker(EntityType p_entity_type)
	: entity_type(p_entity_type)
{
	xr_tracker.instantiate(XRServer::TrackerType::TRACKER_ANCHOR);
}

OpenXRSpatialEntityTracker::~OpenXRSpatialEntityTracker()
{
	if (spatial_entity.is_valid()) {
		if (OpenXRSpatialEntityExtension::is_initialized()) {
			OpenXRSpatialEntityExtension::free_spatial_entity(spatial_entity);
			spatial_entity = RID();
		}
	}
}

void OpenXRSpatialEntityTracker::set_spatial_context(const RID& p_spatial_context)
{
	// Trackers shouldn't be switching spatial contexts; always create a new tracker for new
	// contexts
	ERR_FAIL_COND(spatial_context.is_valid() && spatial_context != p_spatial_context);
	spatial_context = p_spatial_context;
}

RID OpenXRSpatialEntityTracker::get_spatial_context() const { return spatial_context; }

void OpenXRSpatialEntityTracker::set_entity(const RID& p_entity)
{
	if (spatial_entity.is_valid()) {
		if (spatial_entity == p_entity) {
			return;
		}

		if (OpenXRSpatialEntityExtension::is_initialized()) {
			OpenXRSpatialEntityExtension::free_spatial_entity(spatial_entity);
			spatial_entity = RID();
		}
	}

	spatial_entity = p_entity;

	if (p_entity.is_valid()) {
		ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

		XrSpatialEntityIdEXT entity_id = OpenXRSpatialEntityExtension::get_spatial_entity_id(p_entity);

		String tracker_name = String("openxr/spatial_entity/") + String::num_int64(entity_id);
		set_tracker_name(tracker_name);
	}
	else {
		set_tracker_name("openxr/spatial_entity/null");
	}
}

RID OpenXRSpatialEntityTracker::get_entity() const { return spatial_entity; }

void OpenXRSpatialEntityTracker::set_spatial_tracking_state(
	const XrSpatialEntityTrackingStateEXT p_state)
{
	if (spatial_tracking_state != p_state) {
		spatial_tracking_state = p_state;
	}
}

void OpenXRSpatialEntityTracker::_set_spatial_tracking_state(const EntityTrackingState p_state)
{
	set_spatial_tracking_state((XrSpatialEntityTrackingStateEXT)p_state);
}

XrSpatialEntityTrackingStateEXT OpenXRSpatialEntityTracker::get_spatial_tracking_state() const
{
	return spatial_tracking_state;
}

OpenXRSpatialEntityTracker::EntityTrackingState
OpenXRSpatialEntityTracker::_get_spatial_tracking_state() const
{
	return (EntityTrackingState)get_spatial_tracking_state();
}

void OpenXRSpatialEntityTracker::add_next(Ref<OpenXRStructureBase> p_next)
{
	// Prepend p_next to next
	if (p_next.is_valid()) {
		if (next.is_valid()) {
			p_next->set_next(next);
		}

		next = p_next;
	}
}

void OpenXRSpatialEntityTracker::remove_next(Ref<OpenXRStructureBase> p_next)
{
	if (p_next.is_null()) {
		return;
	}

	Ref<OpenXRStructureBase> prev;
	for (Ref<OpenXRStructureBase> n = next; n.is_valid(); n = n->get_next()) {
		if (n == p_next) {
			if (prev.is_null()) {
				next = p_next->get_next();
			}
			else {
				prev->set_next(p_next->get_next());
			}

			p_next->set_next(nullptr);
			break;
		}

		prev = n;
	}
}

Ref<OpenXRStructureBase> OpenXRSpatialEntityTracker::get_next() const { return next; }

bool OpenXRSpatialEntityTracker::has_uuid() const
{
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		if (uuid.data[i] != 0) {
			return true;
		}
	}

	return false;
}

XrUuid OpenXRSpatialEntityTracker::get_uuid() const { return uuid; }

void OpenXRSpatialEntityTracker::set_uuid(const XrUuid& p_uuid)
{
	if (uuid_is_equal(uuid, p_uuid)) {
		return;
	}

	uuid = p_uuid;
}

String OpenXRSpatialEntityTracker::_get_uuid() const { return OpenXRUtil::string_from_xruuid(uuid); }

void OpenXRSpatialEntityTracker::_set_uuid(const String& p_uuid)
{
	set_uuid(OpenXRUtil::xruuid_from_string(p_uuid));
}

bool OpenXRSpatialEntityTracker::uuid_is_equal(const XrUuid& p_a, const XrUuid& p_b)
{
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		if (p_a.data[i] != p_b.data[i]) {
			return false;
		}
	}

	return true;
}

void OpenXRSpatialEntityTracker::set_bounds_size(const Vector2& p_bounds_size)
{
	if (entity_type == ENTITY_PLANE) {
		if (Math::abs(bounds_size.x - p_bounds_size.x) > 0.001 ||
			Math::abs(bounds_size.y - p_bounds_size.y) > 0.001) {
			bounds_size = p_bounds_size;

			if (!mesh.has_mesh_data) {
				// Bounds changing only effects mesh data if we don't have polygon data.
				clear_mesh_data();
			}
		}
	}
	else {
		bounds_size = p_bounds_size;
	}
}

Vector2 OpenXRSpatialEntityTracker::get_bounds_size() const { return bounds_size; }

void OpenXRSpatialEntityTracker::set_plane_alignment(
	OpenXRSpatialComponentData::PlaneAlignment p_plane_alignment)
{
	if (plane_alignment != p_plane_alignment) {
		plane_alignment = p_plane_alignment;
	}
}

OpenXRSpatialComponentData::PlaneAlignment OpenXRSpatialEntityTracker::get_plane_alignment() const
{
	return plane_alignment;
}

void OpenXRSpatialEntityTracker::set_plane_label(const String& p_plane_label)
{
	if (plane_label != p_plane_label) {
		plane_label = p_plane_label;

		// Also copy to description, should do something nicer here.
		set_tracker_desc(plane_label);
	}
}

String OpenXRSpatialEntityTracker::get_plane_label() const { return plane_label; }

void OpenXRSpatialEntityTracker::set_mesh_data(const Transform3D& p_origin,
	const PackedVector2Array& p_vertices, const PackedInt32Array& p_indices)
{
	if (p_vertices.size() < 3) {
		if (mesh.has_mesh_data) {
			clear_mesh_data();
		}
	}
	else {
		bool has_changed = !mesh.has_mesh_data;

		mesh.has_mesh_data = true;
		mesh.origin = p_origin;

		if (mesh.vertices.size() != p_vertices.size()) {
			has_changed = true;
		}
		else {
			// Compare the vertices with a bit of margin, we ignore small jittering on vertices.
			for (uint32_t i = 0; i < p_vertices.size() && !has_changed; i++) {
				const Vector2& a = p_vertices[i];
				const Vector2& b = mesh.vertices[i];
				has_changed = (Math::abs(a.x - b.x) > 0.001) || (Math::abs(a.y - b.y) > 0.001);
			}
		}
		if (has_changed) {
			mesh.vertices = p_vertices;
		}

		if (p_indices.is_empty()) {
			// Assume polygon, turn into triangle strip...
			int count = (p_vertices.size() - 2) * 3;

			if (has_changed || mesh.indices.size() != count) {
				has_changed = true;

				int offset = 1;
				mesh.indices.resize(count);
				int32_t* idx = mesh.indices.ptrw();
				for (int i = 0; i < count; i += 3) {
					idx[i + 0] = 0;
					idx[i + 2] = offset++;
					idx[i + 1] = offset;
				}
			}
		}
		else {
			if (mesh.indices.size() != p_indices.size()) {
				has_changed = true;
			}
			else {
				for (uint32_t i = 0; i < p_indices.size() && !has_changed; i++) {
					has_changed = mesh.indices[i] != p_indices[i];
				}
			}
			if (has_changed) {
				mesh.indices = p_indices;
			}
		}

		if (has_changed) {
			mesh.mesh.unref();
#ifndef PHYSICS_3D_DISABLED
			mesh.shape3d.unref();
#endif
		}
	}
}

void OpenXRSpatialEntityTracker::clear_mesh_data()
{
	mesh.mesh.unref();
#ifndef PHYSICS_3D_DISABLED
	mesh.shape3d.unref();
#endif

	if (mesh.has_mesh_data) {
		mesh.has_mesh_data = false;
		mesh.origin = Transform3D();
		mesh.vertices.clear();
		mesh.indices.clear();
	}
}

Transform3D OpenXRSpatialEntityTracker::get_mesh_offset() const
{
	Transform3D offset;

	if (mesh.has_mesh_data) {
		offset = mesh.origin;

		Ref<XRPose> pose = get_pose(SNAME("default"));
		if (pose.is_valid()) {
			offset = pose->get_transform().inverse() * offset;
		}

		if (XRServer::is_initialized()) {
			offset.origin *= XRServer::get_world_scale();
		}
	}

	return offset;
}

#ifndef PHYSICS_3D_DISABLED
Ref<Shape3D> OpenXRSpatialEntityTracker::get_shape(real_t p_thickness)
{
	if (mesh.shape3d.is_valid()) {
		return mesh.shape3d;
	}

	if (mesh.has_mesh_data) {
		Ref<Shape3D> shape;
		Vector<Vector3> faces;

		int isize = mesh.indices.size();
		const Vector2* vr = mesh.vertices.ptr();
		const int32_t* ir = mesh.indices.ptr();

		HashMap<Edge, int, Edge> edge_counts;
		for (int i = 0; i < isize; i += 3) {
			for (int j = 0; j < 3; j++) {
				Edge e(ir[i + j], ir[i + ((j + 1) % 3)]);
				edge_counts[e]++;
			}
		}

		thread_local LocalVector<Edge> outer_edges;
		outer_edges.clear();
		for (const KeyValue<Edge, int>& e : edge_counts) {
			if (e.value > 1) {
				outer_edges.push_back(e.key);
			}
		}

		faces.resize(2 * isize + 6 * outer_edges.size());
		Vector3* write = faces.ptrw();

		for (int i = 0; i < isize; i += 3) {
			Vector3 a = Vector3(vr[ir[i]].x, vr[ir[i]].y, 0.0);
			Vector3 b = Vector3(vr[ir[i + 1]].x, vr[ir[i + 1]].y, 0.0);
			Vector3 c = Vector3(vr[ir[i + 2]].x, vr[ir[i + 2]].y, 0.0);

			*write++ = a;
			*write++ = b;
			*write++ = c;

			a.z = -p_thickness;
			b.z = -p_thickness;
			c.z = -p_thickness;

			*write++ = a;
			*write++ = c;
			*write++ = b;
		}

		for (const Edge& edge : outer_edges) {
			Vector3 a = Vector3(vr[edge.a].x, vr[edge.a].y, 0.0);
			Vector3 b = Vector3(vr[edge.b].x, vr[edge.b].y, 0.0);
			Vector3 c = b + Vector3(0.0, 0.0, -p_thickness);
			Vector3 d = a + Vector3(0.0, 0.0, -p_thickness);

			*write++ = a;
			*write++ = b;
			*write++ = c;

			*write++ = a;
			*write++ = c;
			*write++ = d;
		}

		shape = memnew(Shape3D(Shape3D::SHAPE_CONCAVE_POLYGON));
		shape->set_faces(faces);

		mesh.shape3d = shape;
	}
	else if (bounds_size.x > 0.0 && bounds_size.y > 0.0) {
		Ref<Shape3D> box_shape = memnew(Shape3D(Shape3D::SHAPE_BOX));
		box_shape->set_size(Vector3(bounds_size.x, bounds_size.y, p_thickness));

		mesh.shape3d = box_shape;
	}

	return mesh.shape3d;
}
#endif // PHYSICS_3D_DISABLED

void OpenXRSpatialEntityTracker::set_marker_type(
	OpenXRSpatialComponentData::MarkerType p_marker_type)
{
	marker_type = p_marker_type;
}

OpenXRSpatialComponentData::MarkerType OpenXRSpatialEntityTracker::get_marker_type() const
{
	return marker_type;
}

void OpenXRSpatialEntityTracker::set_marker_id(uint32_t p_id) { marker_id = p_id; }

uint32_t OpenXRSpatialEntityTracker::get_marker_id() const { return marker_id; }


////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialEntityExtension

void OpenXRSpatialEntityExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialEntityExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialEntityExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled")) {
		request_extensions[XR_EXT_SPATIAL_ENTITY_EXTENSION_NAME] = &data->spatial_entity_ext;
	}

	return request_extensions;
}

void OpenXRSpatialEntityExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->spatial_entity_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilitiesEXT);
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilityComponentTypesEXT);
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilityFeaturesEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialContextAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialContextCompleteEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialContextEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialDiscoverySnapshotAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialDiscoverySnapshotCompleteEXT);
		EXT_INIT_XR_FUNC(xrQuerySpatialComponentDataEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialSnapshotEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialEntityFromIdEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialEntityEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialUpdateSnapshotEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferStringEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint8EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint16EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint32EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferFloatEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferVector2fEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferVector3fEXT);
	}
}

void OpenXRSpatialEntityExtension::on_instance_destroyed()
{
	data->supported_capabilities.clear();
	data->capabilities_load_state = 0;

	xrEnumerateSpatialCapabilitiesEXT_ptr = nullptr;
	xrEnumerateSpatialCapabilityComponentTypesEXT_ptr = nullptr;
	xrEnumerateSpatialCapabilityFeaturesEXT_ptr = nullptr;
	xrCreateSpatialContextAsyncEXT_ptr = nullptr;
	xrCreateSpatialContextCompleteEXT_ptr = nullptr;
	xrDestroySpatialContextEXT_ptr = nullptr;
	xrCreateSpatialDiscoverySnapshotAsyncEXT_ptr = nullptr;
	xrCreateSpatialDiscoverySnapshotCompleteEXT_ptr = nullptr;
	xrQuerySpatialComponentDataEXT_ptr = nullptr;
	xrDestroySpatialSnapshotEXT_ptr = nullptr;
	xrCreateSpatialEntityFromIdEXT_ptr = nullptr;
	xrDestroySpatialEntityEXT_ptr = nullptr;
	xrCreateSpatialUpdateSnapshotEXT_ptr = nullptr;
	xrGetSpatialBufferStringEXT_ptr = nullptr;
	xrGetSpatialBufferUint8EXT_ptr = nullptr;
	xrGetSpatialBufferUint16EXT_ptr = nullptr;
	xrGetSpatialBufferUint32EXT_ptr = nullptr;
	xrGetSpatialBufferFloatEXT_ptr = nullptr;
	xrGetSpatialBufferVector2fEXT_ptr = nullptr;
	xrGetSpatialBufferVector3fEXT_ptr = nullptr;
}

bool OpenXRSpatialEntityExtension::get_active()
{
	return data != nullptr && data->spatial_entity_ext;
}

bool OpenXRSpatialEntityExtension::_load_capabilities()
{
	if (data->capabilities_load_state == 0) {
		if (!data->spatial_entity_ext) {
			return false;
		}

		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		XrInstance instance = OpenXRAPI::get_instance();
		ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);
		XrSystemId system_id = OpenXRAPI::get_system_id();
		ERR_FAIL_COND_V(system_id == 0, false);

		data->capabilities_load_state = 2;

		Vector<XrSpatialCapabilityEXT> capabilities;
		uint32_t capability_size = 0;
		XrResult result =
			xrEnumerateSpatialCapabilitiesEXT(instance, system_id, 0, &capability_size, nullptr);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capability count [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}

		if (capability_size > 0) {
			capabilities.resize(capability_size);
			result = xrEnumerateSpatialCapabilitiesEXT(
				instance, system_id, capabilities.size(), &capability_size, capabilities.ptrw());
			if (XR_FAILED(result)) {
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capabilities [" +
										  OpenXRAPI::get_error_string(result) + "]");
			}

			for (const XrSpatialCapabilityEXT& capability : capabilities) {
				print_verbose("OpenXR: Found spatial entity capability " +
							  get_spatial_capability_name(capability) + ".");

				SpatialEntityCapabality& spatial_entity_capability =
					data->supported_capabilities[capability];

				XrSpatialCapabilityComponentTypesEXT component_types = {
					XR_TYPE_SPATIAL_CAPABILITY_COMPONENT_TYPES_EXT,
					nullptr,
					0,
					0,
					nullptr,
				};
				result = xrEnumerateSpatialCapabilityComponentTypesEXT(
					instance, system_id, capability, &component_types);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to get spatial entity component type count [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
				else if (component_types.componentTypeCountOutput > 0) {
					spatial_entity_capability.component_types.resize(
						component_types.componentTypeCountOutput);
					component_types.componentTypeCapacityInput =
						spatial_entity_capability.component_types.size();
					component_types.componentTypeCountOutput = 0;
					component_types.componentTypes =
						spatial_entity_capability.component_types.ptrw();
					result = xrEnumerateSpatialCapabilityComponentTypesEXT(
						instance, system_id, capability, &component_types);
					if (XR_FAILED(result)) {
						ERR_PRINT("OpenXR: Failed to get spatial entity component types [" +
								  OpenXRAPI::get_error_string(result) + "]");
					}
					else if (is_print_verbose_enabled()) {
						for (const XrSpatialComponentTypeEXT& component_type :
							spatial_entity_capability.component_types) {
							print_verbose("- component type " +
										  get_spatial_component_type_name(component_type));
						}
					}
				}

				result = xrEnumerateSpatialCapabilityFeaturesEXT(
					instance, system_id, capability, 0, &capability_size, nullptr);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to get spatial entity feature count [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
				else if (capability_size > 0) {
					spatial_entity_capability.features.resize(capability_size);
					result = xrEnumerateSpatialCapabilityFeaturesEXT(instance, system_id,
						capability, spatial_entity_capability.features.size(), &capability_size,
						spatial_entity_capability.features.ptrw());
					if (XR_FAILED(result)) {
						ERR_PRINT("OpenXR: Failed to get spatial entity features [" +
								  OpenXRAPI::get_error_string(result) + "]");
					}
					else if (is_print_verbose_enabled()) {
						for (const XrSpatialCapabilityFeatureEXT& feature :
							spatial_entity_capability.features) {
							print_verbose("- feature " + get_spatial_feature_name(feature));
						}
					}
				}
			}
		}

		data->capabilities_load_state = 1;
	}

	return data->capabilities_load_state == 1;
}

bool OpenXRSpatialEntityExtension::supports_capability(XrSpatialCapabilityEXT p_capability)
{
	if (!_load_capabilities()) {
		return false;
	}

	return data->supported_capabilities.has(p_capability);
}

bool OpenXRSpatialEntityExtension::_supports_capability(Capability p_capability)
{
	return supports_capability((XrSpatialCapabilityEXT)p_capability);
}

bool OpenXRSpatialEntityExtension::supports_component_type(
	XrSpatialCapabilityEXT p_capability, XrSpatialComponentTypeEXT p_component_type)
{
	if (!_load_capabilities()) {
		return false;
	}

	if (data->supported_capabilities.has(p_capability)) {
		return data->supported_capabilities[p_capability].component_types.has(p_component_type);
	}
	return false;
}

bool OpenXRSpatialEntityExtension::_supports_component_type(
	Capability p_capability, ComponentType p_component_type)
{
	return supports_component_type(
		(XrSpatialCapabilityEXT)p_capability, (XrSpatialComponentTypeEXT)p_component_type);
}

bool OpenXRSpatialEntityExtension::get_spatial_context_ready(RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, false);

	return context_data->spatial_context != XR_NULL_HANDLE;
}

void OpenXRSpatialEntityExtension::free_spatial_context(RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL(context_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (context_data->spatial_context != XR_NULL_HANDLE) {
		XrResult result = xrDestroySpatialContextEXT(context_data->spatial_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial context [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		context_data->spatial_context = XR_NULL_HANDLE;

		data->spatial_context_owner.free(p_spatial_context);
	}
}

XrSpatialContextEXT OpenXRSpatialEntityExtension::get_spatial_context_handle(
	RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, XR_NULL_HANDLE);

	return context_data->spatial_context;
}

uint64_t OpenXRSpatialEntityExtension::_get_spatial_context_handle(RID p_spatial_context)
{
	return (uint64_t)get_spatial_context_handle(p_spatial_context);
}

RID OpenXRSpatialEntityExtension::update_spatial_entities(RID p_spatial_context,
	const LocalVector<RID>& p_entities,
	const LocalVector<XrSpatialComponentTypeEXT>& p_component_types,
	Ref<OpenXRStructureBase> p_next)
{
	if (!get_active()) {
		return RID();
	}

	ERR_FAIL_COND_V(p_entities.is_empty(), RID());

	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, RID());

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	thread_local LocalVector<XrSpatialEntityEXT> entities;

	entities.resize(p_entities.size());
	XrSpatialEntityEXT* ptr = entities.ptr();
	for (const RID& rid : p_entities) {
		SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(rid);
		*ptr = entity_data ? entity_data->entity : XR_NULL_HANDLE;
		ptr++;
	}

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	SpatialSnapshotData spatial_snapshot_data;
	spatial_snapshot_data.spatial_context = p_spatial_context;

	XrSpatialUpdateSnapshotCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_UPDATE_SNAPSHOT_CREATE_INFO_EXT,
		next,
		(uint32_t)entities.size(),
		entities.ptr(),
		(uint32_t)p_component_types.size(),
		p_component_types.is_empty() ? nullptr : p_component_types.ptr(),
		OpenXRAPI::get_play_space(),
		OpenXRAPI::get_predicted_display_time(),
	};
	XrResult result = xrCreateSpatialUpdateSnapshotEXT(
		context_data->spatial_context, &create_info, &spatial_snapshot_data.spatial_snapshot);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create update snapshot [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	return data->spatial_snapshot_owner.make_rid(spatial_snapshot_data);
}

void OpenXRSpatialEntityExtension::free_spatial_snapshot(RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL(snapshot_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (snapshot_data->spatial_snapshot != XR_NULL_HANDLE) {
		XrResult result = xrDestroySpatialSnapshotEXT(snapshot_data->spatial_snapshot);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial snapshot [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		snapshot_data->spatial_snapshot = XR_NULL_HANDLE;
	}

	data->spatial_snapshot_owner.free(p_spatial_snapshot);
}

XrSpatialSnapshotEXT OpenXRSpatialEntityExtension::get_spatial_snapshot_handle(
	RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, XR_NULL_HANDLE);

	return snapshot_data->spatial_snapshot;
}

RID OpenXRSpatialEntityExtension::get_spatial_snapshot_context(RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, RID());

	return snapshot_data->spatial_context;
}

uint64_t OpenXRSpatialEntityExtension::_get_spatial_snapshot_handle(RID p_spatial_snapshot)
{
	return (uint64_t)get_spatial_snapshot_handle(p_spatial_snapshot);
}

String OpenXRSpatialEntityExtension::get_string(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	String ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferStringEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	LocalVector<char> buffer;
	buffer.resize(count + 1);
	buffer[count] = '\0';

	result = xrGetSpatialBufferStringEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret = String::utf8(buffer.ptr());
	return ret;
}

PackedByteArray OpenXRSpatialEntityExtension::get_uint8_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedByteArray ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint8EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint8EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (uint8_t*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedByteArray(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint16_t> OpenXRSpatialEntityExtension::get_uint16_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	Vector<uint16_t> ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint16EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint16EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint16_t>(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint32_t> OpenXRSpatialEntityExtension::get_uint32_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	Vector<uint32_t> ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint32EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint32EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint32_t>(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

PackedFloat32Array OpenXRSpatialEntityExtension::get_float_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedFloat32Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferFloatEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferFloatEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedFloat32Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

PackedVector2Array OpenXRSpatialEntityExtension::get_vector2_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedVector2Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector2fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	LocalVector<XrVector2f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector2* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
	}
#else
	ret.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector2f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector2Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}
#endif

	return ret;
}

PackedVector3Array OpenXRSpatialEntityExtension::get_vector3_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedVector3Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector3fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	LocalVector<XrVector3f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector3* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
		ptr[i].z = buffer[i].z;
	}
#else
	ret.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector3f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector3Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}
#endif

	return ret;
}

String OpenXRSpatialEntityExtension::_get_string(RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_string(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedByteArray OpenXRSpatialEntityExtension::_get_uint8_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_uint8_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedInt32Array OpenXRSpatialEntityExtension::_get_uint16_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	PackedInt32Array ret;
	Vector<uint16_t> buffer =
		get_uint16_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		ret.resize(buffer.size());

		int size = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < size; i++) {
			ptr[i] = buffer[i];
		}
	}

	return ret;
}

PackedInt32Array OpenXRSpatialEntityExtension::_get_uint32_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	PackedInt32Array ret;
	Vector<uint32_t> buffer =
		get_uint32_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		ret.resize(buffer.size());

		int size = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < size; i++) {
			ptr[i] = buffer[i];
		}
	}

	return ret;
}

PackedFloat32Array OpenXRSpatialEntityExtension::_get_float_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_float_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector2Array OpenXRSpatialEntityExtension::_get_vector2_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_vector2_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector3Array OpenXRSpatialEntityExtension::_get_vector3_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_vector3_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

RID OpenXRSpatialEntityExtension::find_spatial_entity(XrSpatialEntityIdEXT p_entity_id)
{
	ERR_FAIL_COND_V(!get_active(), RID());

	LocalVector<RID> entities = data->spatial_entity_owner.get_owned_list();
	for (const RID& entity : entities) {
		SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(entity);
		ERR_FAIL_NULL_V(entity_data, RID());

		if (entity_data->entity_id == p_entity_id) {
			return entity;
		}
	}

	return RID();
}

RID OpenXRSpatialEntityExtension::_find_entity(uint64_t p_entity_id)
{
	return find_spatial_entity((XrSpatialEntityIdEXT)p_entity_id);
}

RID OpenXRSpatialEntityExtension::add_spatial_entity(
	RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id, XrSpatialEntityEXT p_entity)
{
	ERR_FAIL_COND_V(!get_active(), RID());

	SpatialEntityData spatial_entity_data;
	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	spatial_entity_data.entity = p_entity;

	return data->spatial_entity_owner.make_rid(spatial_entity_data);
}

RID OpenXRSpatialEntityExtension::_add_entity(
	RID p_spatial_context, uint64_t p_entity_id, uint64_t p_entity)
{
	return add_spatial_entity(
		p_spatial_context, (XrSpatialEntityIdEXT)p_entity_id, (XrSpatialEntityEXT)p_entity);
}

RID OpenXRSpatialEntityExtension::make_spatial_entity(
	RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id)
{
	ERR_FAIL_COND_V(!get_active(), RID());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	SpatialEntityData spatial_entity_data;
	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	XrSpatialEntityFromIdCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ENTITY_FROM_ID_CREATE_INFO_EXT,
		nullptr,
		p_entity_id,
	};
	XrResult result = xrCreateSpatialEntityFromIdEXT(
		get_spatial_context_handle(p_spatial_context), &create_info, &spatial_entity_data.entity);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create spatial entity [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	return data->spatial_entity_owner.make_rid(spatial_entity_data);
}

RID OpenXRSpatialEntityExtension::_make_entity(RID p_spatial_context, uint64_t p_entity_id)
{
	return make_spatial_entity(p_spatial_context, (XrSpatialEntityIdEXT)p_entity_id);
}

XrSpatialEntityIdEXT OpenXRSpatialEntityExtension::get_spatial_entity_id(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, XR_NULL_ENTITY);

	return entity_data->entity_id;
}

uint64_t OpenXRSpatialEntityExtension::_get_entity_id(RID p_entity)
{
	return (uint64_t)get_spatial_entity_id(p_entity);
}

RID OpenXRSpatialEntityExtension::get_spatial_entity_context(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, RID());

	return entity_data->spatial_context;
}

void OpenXRSpatialEntityExtension::free_spatial_entity(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL(entity_data);
	ERR_FAIL_COND(entity_data->entity == XR_NULL_HANDLE);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	XrResult result = xrDestroySpatialEntityEXT_ptr(entity_data->entity);
	if (XR_FAILED(result)) {
		WARN_PRINT("OpenXR: Failed to destroy spatial entity [" +
				   OpenXRAPI::get_error_string(result) + "]");
	}

	data->spatial_entity_owner.free(p_entity);
}

String OpenXRSpatialEntityExtension::get_spatial_capability_name(
	XrSpatialCapabilityEXT p_capability)
{
	XR_ENUM_SWITCH(XrSpatialCapabilityEXT, p_capability)
}

String OpenXRSpatialEntityExtension::get_spatial_component_type_name(
	XrSpatialComponentTypeEXT p_component_type)
{
	XR_ENUM_SWITCH(XrSpatialComponentTypeEXT, p_component_type)
}

String OpenXRSpatialEntityExtension::get_spatial_feature_name(
	XrSpatialCapabilityFeatureEXT p_feature)
{
	XR_ENUM_SWITCH(XrSpatialCapabilityFeatureEXT, p_feature)
}

void OpenXRSpatialEntityExtension::on_session_destroyed() {}

bool OpenXRSpatialEntityExtension::on_event_polled(const XrEventDataBuffer& p_event)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialAnchorCapability

void OpenXRSpatialAnchorCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialAnchorCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialAnchorCapability::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_spatial_anchors")) {
		request_extensions[XR_EXT_SPATIAL_ANCHOR_EXTENSION_NAME] = &data->spatial_anchor_ext;
		if (GLOBAL_GET_CACHED(
				bool, "xr/openxr/extensions/spatial_entity/enable_persistent_anchors")) {
			request_extensions[XR_EXT_SPATIAL_PERSISTENCE_EXTENSION_NAME] =
				&data->spatial_persistence_ext;
			request_extensions[XR_EXT_SPATIAL_PERSISTENCE_OPERATIONS_EXTENSION_NAME] =
				&data->spatial_persistence_operations_ext;
		}
	}

	return request_extensions;
}

void OpenXRSpatialAnchorCapability::on_instance_created(const XrInstance p_instance)
{
	ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

	if (data->spatial_anchor_ext) {
		EXT_INIT_XR_FUNC(xrCreateSpatialAnchorEXT);
	}

	if (data->spatial_persistence_ext) {
		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		XrResult xr_result =
			OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceScopesEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		if (xr_result != XR_SUCCESS) {
			print_verbose("OpenXR: xrEnumerateSpatialPersistenceScopesEXT is not supported, "
						  "falling back to xrEnumerateSpatialPersistenceStoresEXT!");
			xr_result = OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceStoresEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		}
		ERR_FAIL_COND(XR_FAILED(xr_result));

		EXT_INIT_XR_FUNC(xrCreateSpatialPersistenceContextAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialPersistenceContextCompleteEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialPersistenceContextEXT);
	}

	if (data->spatial_persistence_operations_ext) {
		EXT_INIT_XR_FUNC(xrPersistSpatialEntityAsyncEXT);
		EXT_INIT_XR_FUNC(xrPersistSpatialEntityCompleteEXT);
		EXT_INIT_XR_FUNC(xrUnpersistSpatialEntityAsyncEXT);
		EXT_INIT_XR_FUNC(xrUnpersistSpatialEntityCompleteEXT);
	}
}

void OpenXRSpatialAnchorCapability::on_instance_destroyed()
{
	xrCreateSpatialAnchorEXT_ptr = nullptr;

	xrEnumerateSpatialPersistenceScopesEXT_ptr = nullptr;
	xrCreateSpatialPersistenceContextAsyncEXT_ptr = nullptr;
	xrCreateSpatialPersistenceContextCompleteEXT_ptr = nullptr;
	xrDestroySpatialPersistenceContextEXT_ptr = nullptr;
}

bool OpenXRSpatialAnchorCapability::is_spatial_anchor_supported()
{
	return data != nullptr && data->spatial_anchor_supported;
}

bool OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()
{
	if (!is_spatial_anchor_supported()) {
		return false;
	}

	return data->spatial_persistence_ext;
}

bool OpenXRSpatialAnchorCapability::_load_supported_persistence_scopes()
{
	ERR_FAIL_COND_V(!is_spatial_persistence_supported(), false);

	if (data->supported_persistence_scopes.is_empty()) {
		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		uint32_t size;
		XrInstance instance = OpenXRAPI::get_instance();
		XrSystemId system_id = OpenXRAPI::get_system_id();

		ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

		XrResult result =
			xrEnumerateSpatialPersistenceScopesEXT(instance, system_id, 0, &size, nullptr);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to query persistence scope count [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}

		if (size > 0) {
			data->supported_persistence_scopes.resize(size);
			result = xrEnumerateSpatialPersistenceScopesEXT(instance, system_id,
				data->supported_persistence_scopes.size(), &size,
				data->supported_persistence_scopes.ptrw());
			if (XR_FAILED(result)) {
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to query persistence scopes [" +
										  OpenXRAPI::get_error_string(result) + "]");
			}
		}

		if (is_print_verbose_enabled()) {
			if (!data->supported_persistence_scopes.is_empty()) {
				print_verbose("OpenXR: Supported spatial persistence scopes:");
				for (const XrSpatialPersistenceScopeEXT& scope :
					data->supported_persistence_scopes) {
					print_verbose(" - " + get_spatial_persistence_scope_name(scope));
				}
			}
			else {
				WARN_PRINT("OpenXR: No persistence scopes found!");
			}
		}
	}

	return true;
}

bool OpenXRSpatialAnchorCapability::is_persistence_scope_supported(
	XrSpatialPersistenceScopeEXT p_scope)
{
	if (!is_spatial_persistence_supported()) {
		return false;
	}

	if (!_load_supported_persistence_scopes()) {
		return false;
	}

	return data->supported_persistence_scopes.has(p_scope);
}

bool OpenXRSpatialAnchorCapability::_is_persistence_scope_supported(PersistenceScope p_scope)
{
	return is_persistence_scope_supported((XrSpatialPersistenceScopeEXT)p_scope);
}

XrSpatialPersistenceContextEXT OpenXRSpatialAnchorCapability::get_persistence_context_handle(
	RID p_persistence_context)
{
	PersistenceContextData* persistence_context_data =
		data->persistence_context_owner.get_or_null(p_persistence_context);
	ERR_FAIL_NULL_V(persistence_context_data, XR_NULL_HANDLE);

	return persistence_context_data->persistence_context;
}

uint64_t OpenXRSpatialAnchorCapability::_get_persistence_context_handle(RID p_persistence_context)
{
	return (uint64_t)get_persistence_context_handle(p_persistence_context);
}

void OpenXRSpatialAnchorCapability::free_persistence_context(RID p_persistence_context)
{
	PersistenceContextData* persistence_context_data =
		data->persistence_context_owner.get_or_null(p_persistence_context);
	ERR_FAIL_NULL(persistence_context_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (persistence_context_data->persistence_context != XR_NULL_HANDLE) {
		XrResult result =
			xrDestroySpatialPersistenceContextEXT(persistence_context_data->persistence_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the persistence context [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		persistence_context_data->persistence_context = XR_NULL_HANDLE;
	}

	data->persistence_context_owner.free(p_persistence_context);
}

void OpenXRSpatialAnchorCapability::_on_persistence_context_completed(RID p_persistence_context)
{
	data->persistence_context = p_persistence_context;

	_create_spatial_context();
}

void OpenXRSpatialAnchorCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialAnchorCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

Ref<OpenXRAnchorTracker> OpenXRSpatialAnchorCapability::create_new_anchor(
	const Transform3D& p_transform, RID p_spatial_context, Ref<OpenXRStructureBase> p_next)
{
	Ref<OpenXRAnchorTracker> tracker;

	ERR_FAIL_COND_V_MSG(!is_spatial_anchor_supported(), tracker,
		"OpenXR: Spatial entity anchor capability is not supported on this hardware!");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), tracker);
	ERR_FAIL_COND_V(!XRServer::is_initialized(), tracker);
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), tracker);

	XrPosef pose = OpenXRAPI::pose_from_transform(p_transform);

	RID sc = p_spatial_context.is_valid() ? p_spatial_context : data->spatial_context;
	ERR_FAIL_COND_V(sc.is_null(), tracker);

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	XrSpatialAnchorCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_EXT,
		next,
		OpenXRAPI::get_play_space(),
		OpenXRAPI::get_predicted_display_time(),
		pose,
	};
	XrSpatialEntityIdEXT entity_id;
	XrSpatialEntityEXT entity;
	XrResult result = xrCreateSpatialAnchorEXT(
		OpenXRSpatialEntityExtension::get_spatial_context_handle(sc), &create_info, &entity_id,
		&entity);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(tracker,
			"OpenXR: Failed to create anchor [" + OpenXRAPI::get_error_string(result) + "]");
	}

	tracker.instantiate(OpenXRSpatialEntityTracker::ENTITY_ANCHOR);
	tracker->set_spatial_context(sc);
	tracker->set_entity(OpenXRSpatialEntityExtension::add_spatial_entity(sc, entity_id, entity));
	tracker->set_tracker_desc("Anchor");
	tracker->set_pose(SNAME("default"), p_transform, Vector3(), Vector3());
	tracker->add_next(p_next);

	data->anchor_trackers[sc][entity_id] = tracker;
	XRServer::add_tracker(tracker->get_xr_tracker());

	return tracker;
}

void OpenXRSpatialAnchorCapability::remove_anchor(Ref<OpenXRAnchorTracker> p_anchor_tracker)
{
	ERR_FAIL_COND(p_anchor_tracker.is_null());
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	ERR_FAIL_COND(!XRServer::is_initialized());
	ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

	ERR_FAIL_COND_MSG(p_anchor_tracker->has_uuid(),
		"OpenXR: This anchor is persistent. It must first be made unpersistent.");

	XRServer::remove_tracker(p_anchor_tracker->get_xr_tracker());

	RID entity = p_anchor_tracker->get_entity();
	ERR_FAIL_COND(entity.is_null());

	XrSpatialEntityIdEXT entity_id = OpenXRSpatialEntityExtension::get_spatial_entity_id(entity);
	ERR_FAIL_COND(entity_id == XR_NULL_ENTITY);

	for (KeyValue<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRAnchorTracker>>>& anchors :
		data->anchor_trackers) {
		if (anchors.value.has(entity_id)) {
			anchors.value.erase(entity_id);
			break;
		}
	}

	p_anchor_tracker->set_entity(RID());
}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_scope_name(
	XrSpatialPersistenceScopeEXT p_scope)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceScopeEXT, p_scope)
}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_context_result_name(
	XrSpatialPersistenceContextResultEXT p_result)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceContextResultEXT, p_result)
}

Ref<OpenXRFutureResult> OpenXRSpatialAnchorCapability::_create_spatial_context()
{
	Ref<OpenXRFutureResult> oxfr = memnew(OpenXRFutureResult);
	return oxfr;
}

void OpenXRSpatialAnchorCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialAnchorCapability::on_session_destroyed() {}

void OpenXRSpatialAnchorCapability::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialMarkerTrackingCapability

void OpenXRSpatialMarkerTrackingCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialMarkerTrackingCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialMarkerTrackingCapability::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_marker_tracking")) {
		request_extensions[XR_EXT_SPATIAL_MARKER_TRACKING_EXTENSION_NAME] =
			&data->spatial_marker_tracking_ext;
	}

	return request_extensions;
}

bool OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported()
{
	if (!data->spatial_marker_tracking_ext) {
		return false;
	}

	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_aruco_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT);
}

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

void OpenXRSpatialMarkerTrackingCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialMarkerTrackingCapability::on_session_destroyed() {}

void OpenXRSpatialMarkerTrackingCapability::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialPlaneTrackingCapability

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
{
	return data != nullptr && data->spatial_plane_tracking_supported;
}

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

void OpenXRSpatialPlaneTrackingCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialPlaneTrackingCapability::on_session_destroyed() {}

void OpenXRSpatialPlaneTrackingCapability::on_process() {}
