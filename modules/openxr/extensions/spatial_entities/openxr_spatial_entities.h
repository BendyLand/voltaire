/**************************************************************************/
/*  openxr_spatial_entities.h                                             */
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

#include <openxr/openxr.h>
#include "../../openxr_api.h"
#include "../openxr_extensions.h"
#include "core/templates/rid_owner.h"
#include "scene/resources/mesh.h"
#include "servers/xr/xr_tracker.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif

#define XR_NULL_ENTITY 0x7FFFFFFF

// Persistence configuration (folded into OpenXRStructureBase)
using OpenXRSpatialContextPersistenceConfig = OpenXRStructureBase;

// Consolidated capability configuration wrapper for XrSpatialCapabilityConfigurationBaseHeaderEXT
class OpenXRSpatialCapabilityConfigurationBaseHeader final : public RefCounted
{
public:
	enum CapabilityType
	{
		CAPABILITY_NONE,
		CAPABILITY_ANCHOR,
		CAPABILITY_PLANE_TRACKING,
		CAPABILITY_QR_CODE,
		CAPABILITY_MICRO_QR_CODE,
		CAPABILITY_ARUCO,
		CAPABILITY_APRIL_TAG,
	};

	enum ArucoDict
	{
		ARUCO_DICT_4X4_50 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_50_EXT,
		ARUCO_DICT_4X4_100 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_100_EXT,
		ARUCO_DICT_4X4_250 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_250_EXT,
		ARUCO_DICT_4X4_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_1000_EXT,
		ARUCO_DICT_5X5_50 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_50_EXT,
		ARUCO_DICT_5X5_100 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_100_EXT,
		ARUCO_DICT_5X5_250 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_250_EXT,
		ARUCO_DICT_5X5_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_1000_EXT,
		ARUCO_DICT_6X6_50 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_50_EXT,
		ARUCO_DICT_6X6_100 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_100_EXT,
		ARUCO_DICT_6X6_250 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_250_EXT,
		ARUCO_DICT_6X6_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_1000_EXT,
		ARUCO_DICT_7X7_50 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_50_EXT,
		ARUCO_DICT_7X7_100 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_100_EXT,
		ARUCO_DICT_7X7_250 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_250_EXT,
		ARUCO_DICT_7X7_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_1000_EXT,
	};

	enum AprilTagDict
	{
		APRIL_TAG_DICT_16H5 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_16H5_EXT,
		APRIL_TAG_DICT_25H9 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_25H9_EXT,
		APRIL_TAG_DICT_36H10 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H10_EXT,
		APRIL_TAG_DICT_36H11 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H11_EXT,
	};

	OpenXRSpatialCapabilityConfigurationBaseHeader(CapabilityType p_capability_type = CAPABILITY_NONE);
	~OpenXRSpatialCapabilityConfigurationBaseHeader();

	void set_capability_type(CapabilityType p_capability_type);
	CapabilityType get_capability_type() const { return capability_type; }

	bool has_valid_configuration() const;
	uint64_t _get_configurationgd();
	XrSpatialCapabilityConfigurationBaseHeaderEXT* get_configuration();

	Vector<XrSpatialComponentTypeEXT> get_enabled_components() const { return enabled_components; }

	// Plane tracking configuration queries
	bool get_supports_mesh_2d();
	bool get_supports_polygons();
	bool get_supports_labels();

	// ArUco dictionary configuration
	void set_aruco_dict(XrSpatialMarkerArucoDictEXT p_dict);
	XrSpatialMarkerArucoDictEXT get_aruco_dict() const;

	// AprilTag dictionary configuration
	void set_april_dict(XrSpatialMarkerAprilTagDictEXT p_dict);
	XrSpatialMarkerAprilTagDictEXT get_april_dict() const;

private:
	CapabilityType capability_type = CAPABILITY_NONE;
	int supports_mesh_2d = -1;
	int supports_polygons = -1;
	int supports_labels = -1;
	XrSpatialMarkerArucoDictEXT aruco_dict = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_1000_EXT;
	XrSpatialMarkerAprilTagDictEXT april_dict = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H11_EXT;

	Vector<XrSpatialComponentTypeEXT> enabled_components;

	union ConfigPayload
	{
		XrSpatialCapabilityConfigurationBaseHeaderEXT base;
		XrSpatialCapabilityConfigurationAnchorEXT anchor;
		XrSpatialCapabilityConfigurationPlaneTrackingEXT plane;
		XrSpatialCapabilityConfigurationQrCodeEXT qrcode;
		XrSpatialCapabilityConfigurationMicroQrCodeEXT micro_qrcode;
		XrSpatialCapabilityConfigurationArucoMarkerEXT aruco;
		XrSpatialCapabilityConfigurationAprilTagEXT april_tag;

		ConfigPayload()
			: base{XR_TYPE_UNKNOWN, nullptr, XR_SPATIAL_CAPABILITY_MAX_ENUM_EXT, 0, nullptr}
		{
		}
	} config;

	PackedInt64Array _get_enabled_components() const;
	void _set_aruco_dict(ArucoDict p_dict);
	ArucoDict _get_aruco_dict() const;
	void _set_april_dict(AprilTagDict p_dict);
	AprilTagDict _get_april_dict() const;
};

// Consolidated wrapper class for spatial component data returned by discovery and update queries
class OpenXRSpatialComponentData final : public RefCounted
{
public:
	enum ComponentKind
	{
		COMPONENT_NONE,
		COMPONENT_QUERY_RESULT,
		COMPONENT_BOUNDED_2D,
		COMPONENT_BOUNDED_3D,
		COMPONENT_PARENT,
		COMPONENT_MESH_2D,
		COMPONENT_MESH_3D,
		COMPONENT_ANCHOR,
		COMPONENT_PERSISTENCE,
		COMPONENT_PLANE_ALIGNMENT,
		COMPONENT_POLYGON_2D,
		COMPONENT_PLANE_SEMANTIC_LABEL,
		COMPONENT_MARKER,
	};

	enum PlaneAlignment
	{
		PLANE_ALIGNMENT_HORIZONTAL_UPWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_UPWARD_EXT,
		PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD_EXT,
		PLANE_ALIGNMENT_VERTICAL = XR_SPATIAL_PLANE_ALIGNMENT_VERTICAL_EXT,
		PLANE_ALIGNMENT_ARBITRARY = XR_SPATIAL_PLANE_ALIGNMENT_ARBITRARY_EXT,
	};

	enum PlaneSemanticLabel
	{
		PLANE_SEMANTIC_LABEL_UNCATEGORIZED = XR_SPATIAL_PLANE_SEMANTIC_LABEL_UNCATEGORIZED_EXT,
		PLANE_SEMANTIC_LABEL_FLOOR = XR_SPATIAL_PLANE_SEMANTIC_LABEL_FLOOR_EXT,
		PLANE_SEMANTIC_LABEL_WALL = XR_SPATIAL_PLANE_SEMANTIC_LABEL_WALL_EXT,
		PLANE_SEMANTIC_LABEL_CEILING = XR_SPATIAL_PLANE_SEMANTIC_LABEL_CEILING_EXT,
		PLANE_SEMANTIC_LABEL_TABLE = XR_SPATIAL_PLANE_SEMANTIC_LABEL_TABLE_EXT,
	};

	enum MarkerType
	{
		MARKER_TYPE_UNKNOWN,
		MARKER_TYPE_QRCODE,
		MARKER_TYPE_MICRO_QRCODE,
		MARKER_TYPE_ARUCO,
		MARKER_TYPE_APRIL_TAG,
		MARKER_TYPE_MAX
	};

	OpenXRSpatialComponentData(ComponentKind p_kind = COMPONENT_NONE);
	~OpenXRSpatialComponentData();

	OpenXRSpatialComponentData(const OpenXRSpatialComponentData&) = delete;
	OpenXRSpatialComponentData& operator=(const OpenXRSpatialComponentData&) = delete;

	void set_component_kind(ComponentKind p_kind);
	ComponentKind get_component_kind() const { return component_kind; }

	void set_capacity(uint32_t p_capacity);
	int64_t get_capacity() const { return capacity; }

	int64_t _get_component_typegd() const;
	XrSpatialComponentTypeEXT get_component_type() const;
	void* get_structure_data(void* p_next);

	// COMPONENT_QUERY_RESULT accessors
	XrSpatialEntityIdEXT get_entity_id(int64_t p_index) const;
	XrSpatialEntityTrackingStateEXT get_entity_state(int64_t p_index) const;
	static String get_entity_tracking_state_name(XrSpatialEntityTrackingStateEXT p_tracking_state);

	// COMPONENT_BOUNDED_2D / COMPONENT_BOUNDED_3D accessors
	Transform3D get_center_pose(int64_t p_index) const;
	Transform3D get_center_pose_2d(int64_t p_index) const;
	Transform3D get_center_pose_3d(int64_t p_index) const;
	Vector2 get_size_2d(int64_t p_index) const;
	Vector3 get_size_3d(int64_t p_index) const;

	// COMPONENT_PARENT accessors
	RID get_parent(int64_t p_index) const;

	// COMPONENT_MESH_2D / COMPONENT_MESH_3D / COMPONENT_POLYGON_2D accessors
	Transform3D get_transform(int64_t p_index) const;
	PackedVector2Array get_vertices(RID p_snapshot, int64_t p_index) const;
	PackedInt32Array get_indices(RID p_snapshot, int64_t p_index) const;
	Ref<Mesh> get_mesh(int64_t p_index) const;

	// COMPONENT_ANCHOR accessors
	Transform3D get_entity_pose(int64_t p_index) const;

	// COMPONENT_PERSISTENCE accessors
	XrUuid get_persistent_uuid(int64_t p_index) const;
	XrSpatialPersistenceStateEXT get_persistent_state(int64_t p_index) const;
	static String get_persistence_state_name(XrSpatialPersistenceStateEXT p_state);

	// COMPONENT_PLANE_ALIGNMENT accessors
	XrSpatialPlaneAlignmentEXT get_plane_alignment(int64_t p_index) const;

	// COMPONENT_PLANE_SEMANTIC_LABEL accessors
	XrSpatialPlaneSemanticLabelEXT get_plane_semantic_label(int64_t p_index) const;

	// COMPONENT_MARKER accessors
	MarkerType get_marker_type(int64_t p_index) const;
	uint32_t get_marker_id(int64_t p_index) const;
	XrSpatialBufferEXT get_marker_data(int64_t p_index) const;

private:
	ComponentKind component_kind = COMPONENT_NONE;
	uint32_t capacity = 0;
	void* data_ptr = nullptr;
	XrSpatialEntityTrackingStateEXT* entity_states_ptr = nullptr;

	union StructurePayload
	{
		XrSpatialComponentDataQueryResultEXT query_result;
		XrSpatialComponentBounded2DListEXT bounded2d_list;
		XrSpatialComponentBounded3DListEXT bounded3d_list;
		XrSpatialComponentParentListEXT parent_list;
		XrSpatialComponentMesh2DListEXT mesh2d_list;
		XrSpatialComponentMesh3DListEXT mesh3d_list;
		XrSpatialComponentAnchorListEXT anchor_list;
		XrSpatialComponentPersistenceListEXT persistence_list;
		XrSpatialComponentPlaneAlignmentListEXT plane_alignment_list;
		XrSpatialComponentPolygon2DListEXT polygon2d_list;
		XrSpatialComponentPlaneSemanticLabelListEXT plane_semantic_label_list;
		XrSpatialComponentMarkerListEXT marker_list;

		StructurePayload()
			: query_result{XR_TYPE_SPATIAL_COMPONENT_DATA_QUERY_RESULT_EXT, nullptr, 0, 0, nullptr,
				  0, 0, nullptr}
		{
		}
	} structure;

	void _free_buffers();
	void _init_structure_header();

	uint64_t _get_entity_id(int64_t p_index) const;
	String _get_persistent_uuid(int64_t p_index) const;
	uint64_t _get_persistent_state(int64_t p_index) const;
	PlaneAlignment _get_plane_alignment(int64_t p_index) const;
	PlaneSemanticLabel _get_plane_semantic_label(int64_t p_index) const;
};

// Unified tracker for spatial entities (generic, anchor, plane, marker), composing XRTracker
class OpenXRSpatialEntityTracker final : public RefCounted
{
public:
	enum EntityType
	{
		ENTITY_GENERIC,
		ENTITY_ANCHOR,
		ENTITY_PLANE,
		ENTITY_MARKER,
	};

	enum EntityTrackingState
	{
		ENTITY_TRACKING_STATE_STOPPED = XR_SPATIAL_ENTITY_TRACKING_STATE_STOPPED_EXT,
		ENTITY_TRACKING_STATE_PAUSED = XR_SPATIAL_ENTITY_TRACKING_STATE_PAUSED_EXT,
		ENTITY_TRACKING_STATE_TRACKING = XR_SPATIAL_ENTITY_TRACKING_STATE_TRACKING_EXT,
	};

	OpenXRSpatialEntityTracker(EntityType p_entity_type = ENTITY_GENERIC);
	~OpenXRSpatialEntityTracker() override;

	const Ref<XRTracker>& get_xr_tracker() const { return xr_tracker; }

	// Composed XRTracker forwarding helpers
	void set_tracker_type(XRServer::TrackerType p_type) { xr_tracker->set_tracker_type(p_type); }
	XRServer::TrackerType get_tracker_type() const { return xr_tracker->get_tracker_type(); }
	void set_tracker_name(const String& p_name) { xr_tracker->set_tracker_name(p_name); }
	String get_tracker_name() const { return xr_tracker->get_tracker_name(); }
	void set_tracker_desc(const String& p_desc) { xr_tracker->set_tracker_desc(p_desc); }
	String get_tracker_desc() const { return xr_tracker->get_tracker_desc(); }
	void set_has_tracking_data(bool p_has_tracking_data)
	{
		xr_tracker->set_has_tracking_data(p_has_tracking_data);
	}
	bool get_has_tracking_data() const { return xr_tracker->get_has_tracking_data(); }
	bool has_pose(const String& p_action_name) const { return xr_tracker->has_pose(p_action_name); }
	Ref<XRPose> get_pose(const String& p_action_name) const
	{
		return xr_tracker->get_pose(p_action_name);
	}
	void invalidate_pose(const String& p_action_name)
	{
		xr_tracker->invalidate_pose(p_action_name);
	}
	void set_pose(const String& p_action_name, const Transform3D& p_transform,
		const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
		const XRPose::TrackingConfidence p_tracking_confidence =
			XRPose::XR_TRACKING_CONFIDENCE_HIGH)
	{
		xr_tracker->set_pose(p_action_name, p_transform, p_linear_velocity, p_angular_velocity,
			p_tracking_confidence);
	}

	void set_entity_type(EntityType p_entity_type) { entity_type = p_entity_type; }
	EntityType get_entity_type() const { return entity_type; }

	void set_spatial_context(const RID& p_spatial_context);
	RID get_spatial_context() const;

	void set_entity(const RID& p_entity);
	RID get_entity() const;

	void set_spatial_tracking_state(const XrSpatialEntityTrackingStateEXT p_state);
	XrSpatialEntityTrackingStateEXT get_spatial_tracking_state() const;

	void add_next(Ref<OpenXRStructureBase> p_next);
	void remove_next(Ref<OpenXRStructureBase> p_next);
	Ref<OpenXRStructureBase> get_next() const;

	// Anchor fields & methods
	bool has_uuid() const;
	XrUuid get_uuid() const;
	void set_uuid(const XrUuid& p_uuid);

	// Shared Plane / Marker bounds
	void set_bounds_size(const Vector2& p_bounds_size);
	Vector2 get_bounds_size() const;

	// Plane fields & methods
	void set_plane_alignment(OpenXRSpatialComponentData::PlaneAlignment p_plane_alignment);
	OpenXRSpatialComponentData::PlaneAlignment get_plane_alignment() const;

	void set_plane_label(const String& p_plane_label);
	String get_plane_label() const;

	void set_mesh_data(const Transform3D& p_origin, const PackedVector2Array& p_vertices,
		const PackedInt32Array& p_indices = PackedInt32Array());
	void clear_mesh_data();

	Transform3D get_mesh_offset() const;
	Ref<Mesh> get_mesh() { return mesh.mesh; }
#ifndef PHYSICS_3D_DISABLED
	Ref<Shape3D> get_shape(real_t p_thickness = 0.01);
#endif

	// Marker fields & methods
	void set_marker_type(OpenXRSpatialComponentData::MarkerType p_marker_type);
	OpenXRSpatialComponentData::MarkerType get_marker_type() const;

	void set_marker_id(uint32_t p_id);
	uint32_t get_marker_id() const;

private:
	Ref<XRTracker> xr_tracker;
	EntityType entity_type = ENTITY_GENERIC;
	RID spatial_context;
	RID spatial_entity;
	XrSpatialEntityTrackingStateEXT spatial_tracking_state =
		XR_SPATIAL_ENTITY_TRACKING_STATE_PAUSED_EXT;
	Ref<OpenXRStructureBase> next;

	// Anchor data
	XrUuid uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	// Plane & Marker data
	Vector2 bounds_size;
	OpenXRSpatialComponentData::PlaneAlignment plane_alignment =
		OpenXRSpatialComponentData::PLANE_ALIGNMENT_HORIZONTAL_UPWARD;
	String plane_label;

	struct MeshData
	{
		bool has_mesh_data = false;
		Transform3D origin;
		PackedVector2Array vertices;
		PackedInt32Array indices;

		Ref<Mesh> mesh;
#ifndef PHYSICS_3D_DISABLED
		Ref<Shape3D> shape3d;
#endif
	} mesh;

	struct Edge
	{
		int32_t a;
		int32_t b;

		static _FORCE_INLINE_ uint32_t hash(const Edge& p_edge)
		{
			uint32_t h = hash_murmur3_one_32(p_edge.a);
			return hash_murmur3_one_32(p_edge.b, h);
		}

		bool operator==(const Edge& p_edge) const { return (a == p_edge.a && b == p_edge.b); }

		Edge(int32_t p_a = 0, int32_t p_b = 0)
		{
			a = p_a;
			b = p_b;
			if (a < b) {
				SWAP(a, b);
			}
		}
	};

	OpenXRSpatialComponentData::MarkerType marker_type =
		OpenXRSpatialComponentData::MARKER_TYPE_UNKNOWN;
	uint32_t marker_id = 0;

	void _set_spatial_tracking_state(const EntityTrackingState p_state);
	EntityTrackingState _get_spatial_tracking_state() const;

	String _get_uuid() const;
	void _set_uuid(const String& p_uuid);
	static bool uuid_is_equal(const XrUuid& p_a, const XrUuid& p_b);
};

using OpenXRAnchorTracker = OpenXRSpatialEntityTracker;
using OpenXRMarkerTracker = OpenXRSpatialEntityTracker;
using OpenXRPlaneTracker = OpenXRSpatialEntityTracker;

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

private:
	// Capabilities
	struct SpatialEntityCapabality
	{
		Vector<XrSpatialComponentTypeEXT> component_types;
		Vector<XrSpatialCapabilityFeatureEXT> features;
	};
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

// (Persistent) anchor logic
class OpenXRSpatialAnchorCapability final
{
public:
	enum PersistenceScope
	{
		PERSISTENCE_SCOPE_SYSTEM_MANAGED = XR_SPATIAL_PERSISTENCE_SCOPE_SYSTEM_MANAGED_EXT,
		PERSISTENCE_SCOPE_LOCAL_ANCHORS = XR_SPATIAL_PERSISTENCE_SCOPE_LOCAL_ANCHORS_EXT,
	};

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_spatial_anchor_supported();
	static bool is_spatial_persistence_supported();

	// Persistence scopes
	static bool is_persistence_scope_supported(XrSpatialPersistenceScopeEXT p_scope);

	static XrSpatialPersistenceContextEXT get_persistence_context_handle(RID p_persistence_context);
	static void free_persistence_context(RID p_persistence_context);

	static Ref<OpenXRAnchorTracker> create_new_anchor(const Transform3D& p_transform,
		RID p_spatial_context = RID(),
		Ref<OpenXRStructureBase> p_next = Ref<OpenXRStructureBase>());
	static void remove_anchor(Ref<OpenXRAnchorTracker> p_anchor_tracker);

	static String get_spatial_persistence_scope_name(XrSpatialPersistenceScopeEXT p_scope);
	static String get_spatial_persistence_context_result_name(
		XrSpatialPersistenceContextResultEXT p_result);

private:
	static bool _load_supported_persistence_scopes();

	struct PersistenceContextData
	{
		XrSpatialPersistenceScopeEXT scope;
		XrSpatialPersistenceContextEXT persistence_context = XR_NULL_HANDLE;
	};

	static bool _is_persistence_scope_supported(PersistenceScope p_scope);
	static uint64_t _get_persistence_context_handle(RID p_persistence_context);

	static void _on_persistence_context_completed(RID p_persistence_context);
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

	EXT_PROTO_XRRESULT_FUNC4(xrCreateSpatialAnchorEXT, (XrSpatialContextEXT), spatialContext,
		(const XrSpatialAnchorCreateInfoEXT*), create_info, (XrSpatialEntityIdEXT*),
		anchor_entity_id, (XrSpatialEntityEXT*), anchor_entity);

	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateSpatialPersistenceScopesEXT, (XrInstance), instance,
		(XrSystemId), system_id, (uint32_t), persistence_scope_capacity_input, (uint32_t*),
		persistence_scope_count_output, (XrSpatialPersistenceScopeEXT*), persistence_scopes);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialPersistenceContextAsyncEXT, (XrSession), session,
		(const XrSpatialPersistenceContextCreateInfoEXT*), create_info, (XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialPersistenceContextCompleteEXT, (XrSession), session,
		(XrFutureEXT), future, (XrCreateSpatialPersistenceContextCompletionEXT*), completion);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialPersistenceContextEXT,
		(XrSpatialPersistenceContextEXT), persistence_context);

	EXT_PROTO_XRRESULT_FUNC3(xrPersistSpatialEntityAsyncEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (const XrSpatialEntityPersistInfoEXT*), persist_info, (XrFutureEXT*),
		future);
	EXT_PROTO_XRRESULT_FUNC3(xrPersistSpatialEntityCompleteEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (XrFutureEXT), future, (XrPersistSpatialEntityCompletionEXT*),
		completion);
	EXT_PROTO_XRRESULT_FUNC3(xrUnpersistSpatialEntityAsyncEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (const XrSpatialEntityUnpersistInfoEXT*), unpersist_info,
		(XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrUnpersistSpatialEntityCompleteEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (XrFutureEXT), future, (XrUnpersistSpatialEntityCompletionEXT*),
		completion);

public:
	struct Data
	{
		bool spatial_anchor_ext = false;
		bool spatial_persistence_ext = false;
		bool spatial_persistence_operations_ext = false;
		bool spatial_anchor_supported = false;
		RID spatial_context;
		RID persistence_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> anchor_configuration;
		Ref<OpenXRSpatialContextPersistenceConfig> persistence_configuration;
		Vector<XrSpatialPersistenceScopeEXT> supported_persistence_scopes;
		RID_Owner<PersistenceContextData> persistence_context_owner;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRAnchorTracker>>> anchor_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialAnchorCapability() = delete;
	OpenXRSpatialAnchorCapability(const OpenXRSpatialAnchorCapability&) = delete;
	OpenXRSpatialAnchorCapability& operator=(const OpenXRSpatialAnchorCapability&) = delete;
	~OpenXRSpatialAnchorCapability() = delete;
};

// Marker tracking logic
class OpenXRSpatialMarkerTrackingCapability final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_qrcode_supported();
	static bool is_micro_qrcode_supported();
	static bool is_aruco_supported();
	static bool is_april_tag_supported();

private:
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

public:
	struct Data
	{
		bool spatial_marker_tracking_ext = false;
		RID spatial_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> qrcode_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> micro_qrcode_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> aruco_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> april_tag_configuration;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRMarkerTracker>>> marker_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialMarkerTrackingCapability() = delete;
	OpenXRSpatialMarkerTrackingCapability(const OpenXRSpatialMarkerTrackingCapability&) = delete;
	OpenXRSpatialMarkerTrackingCapability& operator=(
		const OpenXRSpatialMarkerTrackingCapability&) = delete;
	~OpenXRSpatialMarkerTrackingCapability() = delete;
};

// Plane tracking logic
class OpenXRSpatialPlaneTrackingCapability final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_supported();

private:
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

public:
	struct Data
	{
		bool spatial_plane_tracking_ext = false;
		bool spatial_plane_tracking_supported = false;
		RID spatial_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> plane_configuration;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRPlaneTracker>>> plane_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialPlaneTrackingCapability() = delete;
	OpenXRSpatialPlaneTrackingCapability(const OpenXRSpatialPlaneTrackingCapability&) = delete;
	OpenXRSpatialPlaneTrackingCapability& operator=(
		const OpenXRSpatialPlaneTrackingCapability&) = delete;
	~OpenXRSpatialPlaneTrackingCapability() = delete;
};
