/**************************************************************************/
/*  openxr_spatial_marker_tracking.cpp                                    */
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

#include "../../openxr_api.h"
#include "core/config/project_settings.h"
#include "openxr_spatial_entity_extension.h"
#include "openxr_spatial_marker_tracking.h"
#include "servers/xr/xr_server.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationQrCode


bool OpenXRSpatialCapabilityConfigurationQrCode::has_valid_configuration() const
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, false);

	return capability->is_qrcode_supported();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationQrCode::get_configuration()
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, nullptr);

	if (capability->is_qrcode_supported()) {
		OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
		ERR_FAIL_NULL_V(se_extension, nullptr);

		// Guaranteed components:
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		// Set up our enabled components.
		marker_config.enabledComponentCount = enabled_components.size();
		marker_config.enabledComponents = enabled_components.ptr();

		// and return this.
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&marker_config;
	}

	return nullptr;
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationQrCode::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationMicroQrCode


bool OpenXRSpatialCapabilityConfigurationMicroQrCode::has_valid_configuration() const
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, false);

	return capability->is_micro_qrcode_supported();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationMicroQrCode::get_configuration()
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, nullptr);

	if (capability->is_micro_qrcode_supported()) {
		OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
		ERR_FAIL_NULL_V(se_extension, nullptr);

		// Guaranteed components:
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		// Set up our enabled components.
		marker_config.enabledComponentCount = enabled_components.size();
		marker_config.enabledComponents = enabled_components.ptr();

		// and return this.
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&marker_config;
	}

	return nullptr;
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationMicroQrCode::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationAruco

OpenXRSpatialCapabilityConfigurationAruco::OpenXRSpatialCapabilityConfigurationAruco()
{
	int aruco_dict = GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/aruco_dict");
	set_aruco_dict(
		(XrSpatialMarkerArucoDictEXT)(XR_SPATIAL_MARKER_ARUCO_DICT_4X4_50_EXT + aruco_dict));
}

OpenXRSpatialCapabilityConfigurationAruco::~OpenXRSpatialCapabilityConfigurationAruco() {}


bool OpenXRSpatialCapabilityConfigurationAruco::has_valid_configuration() const
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, false);

	return capability->is_aruco_supported();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationAruco::get_configuration()
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, nullptr);

	if (capability->is_aruco_supported()) {
		OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
		ERR_FAIL_NULL_V(se_extension, nullptr);

		// Guaranteed components:
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		// Set up our enabled components.
		marker_config.enabledComponentCount = enabled_components.size();
		marker_config.enabledComponents = enabled_components.ptr();

		// and return this.
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&marker_config;
	}

	return nullptr;
}

void OpenXRSpatialCapabilityConfigurationAruco::set_aruco_dict(XrSpatialMarkerArucoDictEXT p_dict)
{
	marker_config.arUcoDict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationAruco::_set_aruco_dict(ArucoDict p_dict)
{
	set_aruco_dict((XrSpatialMarkerArucoDictEXT)p_dict);
}

XrSpatialMarkerArucoDictEXT OpenXRSpatialCapabilityConfigurationAruco::get_aruco_dict() const
{
	return marker_config.arUcoDict;
}

OpenXRSpatialCapabilityConfigurationAruco::ArucoDict
OpenXRSpatialCapabilityConfigurationAruco::_get_aruco_dict() const
{
	return (ArucoDict)get_aruco_dict();
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationAruco::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationAprilTag

OpenXRSpatialCapabilityConfigurationAprilTag::OpenXRSpatialCapabilityConfigurationAprilTag()
{
	int april_tag_dict =
		GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/april_tag_dict");
	set_april_dict((XrSpatialMarkerAprilTagDictEXT)(XR_SPATIAL_MARKER_APRIL_TAG_DICT_16H5_EXT +
													april_tag_dict));
}

OpenXRSpatialCapabilityConfigurationAprilTag::~OpenXRSpatialCapabilityConfigurationAprilTag() {}


bool OpenXRSpatialCapabilityConfigurationAprilTag::has_valid_configuration() const
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, false);

	return capability->is_april_tag_supported();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationAprilTag::get_configuration()
{
	OpenXRSpatialMarkerTrackingCapability* capability =
		OpenXRSpatialMarkerTrackingCapability::get_singleton();
	ERR_FAIL_NULL_V(capability, nullptr);

	if (capability->is_april_tag_supported()) {
		OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
		ERR_FAIL_NULL_V(se_extension, nullptr);

		// Guaranteed components:
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		// Set up our enabled components.
		marker_config.enabledComponentCount = enabled_components.size();
		marker_config.enabledComponents = enabled_components.ptr();

		// and return this.
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&marker_config;
	}

	return nullptr;
}

void OpenXRSpatialCapabilityConfigurationAprilTag::set_april_dict(
	XrSpatialMarkerAprilTagDictEXT p_dict)
{
	marker_config.aprilDict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationAprilTag::_set_april_dict(AprilTagDict p_dict)
{
	set_april_dict((XrSpatialMarkerAprilTagDictEXT)p_dict);
}

XrSpatialMarkerAprilTagDictEXT OpenXRSpatialCapabilityConfigurationAprilTag::get_april_dict() const
{
	return marker_config.aprilDict;
}

OpenXRSpatialCapabilityConfigurationAprilTag::AprilTagDict
OpenXRSpatialCapabilityConfigurationAprilTag::_get_april_dict() const
{
	return (AprilTagDict)get_april_dict();
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationAprilTag::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialComponentMarkerList


void OpenXRSpatialComponentMarkerList::set_capacity(uint32_t p_capacity)
{
	marker_data.resize(p_capacity);

	marker_list.markerCount = uint32_t(marker_data.size());
	marker_list.markers = marker_data.ptrw();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentMarkerList::get_component_type() const
{
	return XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT;
}

void* OpenXRSpatialComponentMarkerList::get_structure_data(void* p_next)
{
	marker_list.next = p_next;
	return &marker_list;
}

OpenXRSpatialComponentMarkerList::MarkerType OpenXRSpatialComponentMarkerList::get_marker_type(
	int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, marker_data.size(), MARKER_TYPE_UNKNOWN);

	// We can't simply cast these.
	// This may give us problems in the future if we get new types through vendor extensions.
	switch (marker_data[p_index].capability) {
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT: {
		return MARKER_TYPE_QRCODE;
	} break;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT: {
		return MARKER_TYPE_MICRO_QRCODE;
	} break;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT: {
		return MARKER_TYPE_ARUCO;
	} break;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT: {
		return MARKER_TYPE_APRIL_TAG;
	} break;
	default: {
		return MARKER_TYPE_UNKNOWN;
	} break;
	}
}

uint32_t OpenXRSpatialComponentMarkerList::get_marker_id(int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, marker_data.size(), 0);

	return marker_data[p_index].markerId;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRMarkerTracker

void OpenXRMarkerTracker::set_bounds_size(const Vector2& p_bounds_size)
{
	bounds_size = p_bounds_size;
}

Vector2 OpenXRMarkerTracker::get_bounds_size() const { return bounds_size; }

void OpenXRMarkerTracker::set_marker_type(
	OpenXRSpatialComponentMarkerList::MarkerType p_marker_type)
{
	marker_type = p_marker_type;
}

OpenXRSpatialComponentMarkerList::MarkerType OpenXRMarkerTracker::get_marker_type() const
{
	return marker_type;
}

void OpenXRMarkerTracker::set_marker_id(uint32_t p_id) { marker_id = p_id; }

uint32_t OpenXRMarkerTracker::get_marker_id() const { return marker_id; }

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialMarkerTrackingCapability

OpenXRSpatialMarkerTrackingCapability* OpenXRSpatialMarkerTrackingCapability::singleton = nullptr;

OpenXRSpatialMarkerTrackingCapability* OpenXRSpatialMarkerTrackingCapability::get_singleton()
{
	return singleton;
}

OpenXRSpatialMarkerTrackingCapability::OpenXRSpatialMarkerTrackingCapability() { singleton = this; }

OpenXRSpatialMarkerTrackingCapability::~OpenXRSpatialMarkerTrackingCapability()
{
	singleton = nullptr;
}

HashMap<String, bool*> OpenXRSpatialMarkerTrackingCapability::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_marker_tracking")) {
		request_extensions[XR_EXT_SPATIAL_MARKER_TRACKING_EXTENSION_NAME] =
			&spatial_marker_tracking_ext;
	}

	return request_extensions;
}

bool OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported()
{
	if (!spatial_marker_tracking_ext) {
		return false;
	}

	OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
	ERR_FAIL_NULL_V(se_extension, false);

	return se_extension->supports_capability(XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported()
{
	OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
	ERR_FAIL_NULL_V(se_extension, false);

	return se_extension->supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_aruco_supported()
{
	OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
	ERR_FAIL_NULL_V(se_extension, false);

	return se_extension->supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported()
{
	OpenXRSpatialEntityExtension* se_extension = OpenXRSpatialEntityExtension::get_singleton();
	ERR_FAIL_NULL_V(se_extension, false);

	return se_extension->supports_capability(XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT);
}

////////////////////////////////////////////////////////////////////////////
// Discovery logic

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	spatial_context = p_spatial_context;
	need_discovery = true;
}

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == spatial_context) {
		// Trigger new discovery.
		need_discovery = true;
	}
}


