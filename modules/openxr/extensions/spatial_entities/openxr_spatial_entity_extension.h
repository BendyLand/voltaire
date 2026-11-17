/**************************************************************************/
/*  openxr_spatial_entity_extension.h                                     */
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

#include "../../openxr_structure.h"
#include "../openxr_future_extension.h"
#include "core/templates/rid_owner.h"
#include "openxr_spatial_entities.h"

// Spatial entity extension
class OpenXRSpatialEntityExtension final
{
public:
	enum Capability
	{
		CAPABILITY_PLANE_TRACKING = XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
		CAPABILITY_MARKER_TRACKING_QR_CODE = XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT,
		CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE =
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT,
		CAPABILITY_MARKER_TRACKING_ARUCO_MARKER =
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT,
		CAPABILITY_MARKER_TRACKING_APRIL_TAG = XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT,
		CAPABILITY_ANCHOR = XR_SPATIAL_CAPABILITY_ANCHOR_EXT,
	};

	enum ComponentType
	{
		COMPONENT_TYPE_BOUNDED_2D = XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT,
		COMPONENT_TYPE_BOUNDED_3D = XR_SPATIAL_COMPONENT_TYPE_BOUNDED_3D_EXT,
		COMPONENT_TYPE_PARENT = XR_SPATIAL_COMPONENT_TYPE_PARENT_EXT,
		COMPONENT_TYPE_MESH_3D = XR_SPATIAL_COMPONENT_TYPE_MESH_3D_EXT,
		COMPONENT_TYPE_PLANE_ALIGNMENT = XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT,
		COMPONENT_TYPE_MESH_2D = XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT,
		COMPONENT_TYPE_POLYGON_2D = XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT,
		COMPONENT_TYPE_PLANE_SEMANTIC_LABEL = XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT,
		COMPONENT_TYPE_MARKER = XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT,
		COMPONENT_TYPE_ANCHOR = XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT,
		COMPONENT_TYPE_PERSISTENCE = XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT,
	};


	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_destroyed();

	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool get_active();
	static bool supports_capability(XrSpatialCapabilityEXT p_capability);
	static bool supports_component_type(
		XrSpatialCapabilityEXT p_capability, XrSpatialComponentTypeEXT p_component_type);

	// Spatial contexts
	static bool get_spatial_context_ready(RID p_spatial_context);
	static void free_spatial_context(RID p_spatial_context);
	static XrSpatialContextEXT get_spatial_context_handle(RID p_spatial_context);

	// Update query
	static RID update_spatial_entities(RID p_spatial_context, const LocalVector<RID>& p_entities,
		const LocalVector<XrSpatialComponentTypeEXT>& p_component_types,
		Ref<OpenXRStructureBase> p_next);

	// Snapshot data
	static void free_spatial_snapshot(RID p_spatial_snapshot);
	static XrSpatialSnapshotEXT get_spatial_snapshot_handle(RID p_spatial_snapshot);
	static RID get_spatial_snapshot_context(RID p_spatial_snapshot);

	// Buffers from snapshot
	static String get_string(RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedByteArray get_uint8_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static Vector<uint16_t> get_uint16_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static Vector<uint32_t> get_uint32_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedFloat32Array get_float_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedVector2Array get_vector2_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedVector3Array get_vector3_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);

	// Entities
	static RID find_spatial_entity(XrSpatialEntityIdEXT p_entity_id);
	static RID add_spatial_entity(
		RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id, XrSpatialEntityEXT p_entity);
	static RID make_spatial_entity(RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id);
	static XrSpatialEntityIdEXT get_spatial_entity_id(RID p_entity);
	static RID get_spatial_entity_context(RID p_entity);
	static void free_spatial_entity(RID p_entity);

	static String get_spatial_capability_name(XrSpatialCapabilityEXT p_capability);
	static String get_spatial_component_type_name(XrSpatialComponentTypeEXT p_component_type);
	static String get_spatial_feature_name(XrSpatialCapabilityFeatureEXT p_feature);

protected:

private:

	// Capabilities
	struct SpatialEntityCapabality
	{
		Vector<XrSpatialComponentTypeEXT> component_types;
		Vector<XrSpatialCapabilityFeatureEXT> features;
	}; // 0 = no, 1 = yes, 2 = failed
	static bool _load_capabilities();

	static bool _supports_capability(Capability p_capability);
	static bool _supports_component_type(Capability p_capability, ComponentType p_component_type);

	// Spatial context
	struct SpatialContextData
	{
		XrSpatialContextEXT spatial_context = XR_NULL_HANDLE;
	};

	static uint64_t _get_spatial_context_handle(RID p_spatial_context);

	// Snapshot data
	struct SpatialSnapshotData
	{
		RID spatial_context;
		XrSpatialSnapshotEXT spatial_snapshot = XR_NULL_HANDLE;
	};

	static uint64_t _get_spatial_snapshot_handle(RID p_spatial_snapshot);

	// Buffers from snapshot
	static String _get_string(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedByteArray _get_uint8_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedInt32Array _get_uint16_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedInt32Array _get_uint32_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedFloat32Array _get_float_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedVector2Array _get_vector2_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedVector3Array _get_vector3_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);

	// Entities
	struct SpatialEntityData
	{
		RID spatial_context;
		XrSpatialEntityIdEXT entity_id = XR_NULL_ENTITY;
		XrSpatialEntityEXT entity = XR_NULL_HANDLE;
	};

	static RID _find_entity(uint64_t p_entity_id);
	static RID _add_entity(RID p_spatial_context, uint64_t p_entity_id, uint64_t p_entity);
	static RID _make_entity(RID p_spatial_context, uint64_t p_entity_id);
	static uint64_t _get_entity_id(RID p_entity);

	// OpenXR API call wrappers

	// Spatial entities
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateSpatialCapabilitiesEXT, (XrInstance), instance,
		(XrSystemId), system_id, (uint32_t), capability_capacity_input, (uint32_t*),
		capability_count_output, (XrSpatialCapabilityEXT*), capabilities);
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSpatialCapabilityComponentTypesEXT, (XrInstance), instance,
		(XrSystemId), systemId, (XrSpatialCapabilityEXT), capability,
		(XrSpatialCapabilityComponentTypesEXT*), capability_components);
	EXT_PROTO_XRRESULT_FUNC6(xrEnumerateSpatialCapabilityFeaturesEXT, (XrInstance), instance,
		(XrSystemId), systemId, (XrSpatialCapabilityEXT), capability, (uint32_t),
		capability_feature_capacity_input, (uint32_t*), capability_feature_count_output,
		(XrSpatialCapabilityFeatureEXT*), capability_features);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialContextAsyncEXT, (XrSession), session,
		(const XrSpatialContextCreateInfoEXT*), create_info, (XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialContextCompleteEXT, (XrSession), session, (XrFutureEXT),
		future, (XrCreateSpatialContextCompletionEXT*), completion);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialContextEXT, (XrSpatialContextEXT), spatial_context);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialDiscoverySnapshotAsyncEXT, (XrSpatialContextEXT),
		spatial_context, (const XrSpatialDiscoverySnapshotCreateInfoEXT*), create_info,
		(XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialDiscoverySnapshotCompleteEXT, (XrSpatialContextEXT),
		spatial_context, (const XrCreateSpatialDiscoverySnapshotCompletionInfoEXT*),
		create_snapshot_completion_info, (XrCreateSpatialDiscoverySnapshotCompletionEXT*),
		completion);
	EXT_PROTO_XRRESULT_FUNC3(xrQuerySpatialComponentDataEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialComponentDataQueryConditionEXT*), query_condition,
		(XrSpatialComponentDataQueryResultEXT*), query_result);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialSnapshotEXT, (XrSpatialSnapshotEXT), snapshot);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialEntityFromIdEXT, (XrSpatialContextEXT), spatial_context,
		(const XrSpatialEntityFromIdCreateInfoEXT*), create_info, (XrSpatialEntityEXT*),
		spatial_entity);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialEntityEXT, (XrSpatialEntityEXT), spatial_entity);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialUpdateSnapshotEXT, (XrSpatialContextEXT),
		spatial_context, (const XrSpatialUpdateSnapshotCreateInfoEXT*), createInfo,
		(XrSpatialSnapshotEXT*), snapshot);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferStringEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (char*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint8EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint8_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint16EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint16_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint32EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint32_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferFloatEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (float*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferVector2fEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (XrVector2f*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferVector3fEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (XrVector3f*), buffer);

public:
	struct Data
	{
		bool spatial_entity_ext = false;
		HashMap<XrSpatialCapabilityEXT, SpatialEntityCapabality> supported_capabilities;
		int capabilities_load_state = 0;
		RID_Owner<SpatialContextData> spatial_context_owner;
		RID_Owner<SpatialSnapshotData> spatial_snapshot_owner;
		RID_Owner<SpatialEntityData> spatial_entity_owner;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialEntityExtension() = delete;
	OpenXRSpatialEntityExtension(const OpenXRSpatialEntityExtension&) = delete;
	OpenXRSpatialEntityExtension& operator=(const OpenXRSpatialEntityExtension&) = delete;
	~OpenXRSpatialEntityExtension() = delete;
};


