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
#include "../../openxr_util.h"
#include "core/config/project_settings.h"
#include "openxr_spatial_anchor.h"
#include "openxr_spatial_entities.h"
#include "openxr_spatial_entity_extension.h"
#include "openxr_spatial_marker_tracking.h"
#include "openxr_spatial_plane_tracking.h"
#include "servers/xr/xr_server.h"

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
	set_tracker_type(XRServer::TrackerType::TRACKER_ANCHOR);
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
