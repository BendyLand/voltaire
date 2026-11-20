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
#include "../../openxr_structure.h"
#include "scene/resources/mesh.h"
#include "servers/xr/xr_positional_tracker.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif

#define XR_NULL_ENTITY 0x7FFFFFFF

// Wrapper class for XrSpatialCapabilityConfigurationBaseHeaderEXT
class OpenXRSpatialCapabilityConfigurationBaseHeader : public RefCounted
{
public:
	virtual bool has_valid_configuration() const;
	uint64_t _get_configurationgd();
	virtual XrSpatialCapabilityConfigurationBaseHeaderEXT* get_configuration();

	OpenXRSpatialCapabilityConfigurationBaseHeader();
	~OpenXRSpatialCapabilityConfigurationBaseHeader();
};

// Wrapper class for our spatial component data returned by discovery queries
class OpenXRSpatialComponentData : public RefCounted
{
public:
	enum PlaneAlignment
	{
		PLANE_ALIGNMENT_HORIZONTAL_UPWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_UPWARD_EXT,
		PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD_EXT,
		PLANE_ALIGNMENT_VERTICAL = XR_SPATIAL_PLANE_ALIGNMENT_VERTICAL_EXT,
		PLANE_ALIGNMENT_ARBITRARY = XR_SPATIAL_PLANE_ALIGNMENT_ARBITRARY_EXT,
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

	virtual void set_capacity(uint32_t p_capacity);
	int64_t _get_component_typegd() const;
	virtual XrSpatialComponentTypeEXT get_component_type() const;
	virtual void* get_structure_data(void* p_next);
	OpenXRSpatialComponentData();
	~OpenXRSpatialComponentData();
};

// Unified tracker for spatial entities (generic, anchor, plane, marker)
class OpenXRSpatialEntityTracker : public XRPositionalTracker
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
	virtual ~OpenXRSpatialEntityTracker();

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

class OpenXRSpatialComponentBounded2DList : public OpenXRSpatialComponentData
{
public:
	virtual void set_capacity(uint32_t p_capacity) override;
	virtual XrSpatialComponentTypeEXT get_component_type() const override;
	virtual void* get_structure_data(void* p_next) override;

	Transform3D get_center_pose(int64_t p_index) const;
	Vector2 get_size(int64_t p_index) const;

private:
	Vector<XrSpatialBounded2DDataEXT> bounded2d_data;

	XrSpatialComponentBounded2DListEXT bounded2d_list = {
		XR_TYPE_SPATIAL_COMPONENT_BOUNDED_2D_LIST_EXT, nullptr, 0, nullptr};
};

class OpenXRSpatialComponentBounded3DList : public OpenXRSpatialComponentData
{
public:
	virtual void set_capacity(uint32_t p_capacity) override;
	virtual XrSpatialComponentTypeEXT get_component_type() const override;
	virtual void* get_structure_data(void* p_next) override;

	Transform3D get_center_pose(int64_t p_index) const;
	Vector3 get_size(int64_t p_index) const;

private:
	Vector<XrBoxf> bounded3d_data;

	XrSpatialComponentBounded3DListEXT bounded3d_list = {
		XR_TYPE_SPATIAL_COMPONENT_BOUNDED_3D_LIST_EXT, nullptr, 0, nullptr};
};

class OpenXRSpatialComponentParentList : public OpenXRSpatialComponentData
{
public:
	virtual void set_capacity(uint32_t p_capacity) override;
	virtual XrSpatialComponentTypeEXT get_component_type() const override;
	virtual void* get_structure_data(void* p_next) override;

	RID get_parent(int64_t p_index) const;

private:
	Vector<XrSpatialEntityIdEXT> parent_data;

	XrSpatialComponentParentListEXT parent_list = {
		XR_TYPE_SPATIAL_COMPONENT_PARENT_LIST_EXT, nullptr, 0, nullptr};
};

class OpenXRSpatialComponentMesh2DList : public OpenXRSpatialComponentData
{
public:
	virtual void set_capacity(uint32_t p_capacity) override;
	virtual XrSpatialComponentTypeEXT get_component_type() const override;
	virtual void* get_structure_data(void* p_next) override;

	Transform3D get_transform(int64_t p_index) const;
	PackedVector2Array get_vertices(RID p_snapshot, int64_t p_index) const;
	PackedInt32Array get_indices(RID p_snapshot, int64_t p_index) const;

private:
	Vector<XrSpatialMeshDataEXT> mesh2d_data;

	XrSpatialComponentMesh2DListEXT mesh2d_list = {
		XR_TYPE_SPATIAL_COMPONENT_MESH_2D_LIST_EXT, nullptr, 0, nullptr};
};

class OpenXRSpatialComponentMesh3DList : public OpenXRSpatialComponentData
{
public:
	virtual void set_capacity(uint32_t p_capacity) override;
	virtual XrSpatialComponentTypeEXT get_component_type() const override;
	virtual void* get_structure_data(void* p_next) override;

	Transform3D get_transform(int64_t p_index) const;
	Ref<Mesh> get_mesh(int64_t p_index) const;

private:
	Vector<XrSpatialMeshDataEXT> mesh3d_data;

	XrSpatialComponentMesh3DListEXT mesh3d_list = {
		XR_TYPE_SPATIAL_COMPONENT_MESH_3D_LIST_EXT, nullptr, 0, nullptr};
};

class OpenXRSpatialQueryResultData : public OpenXRSpatialComponentData
{
public:
	virtual void set_capacity(uint32_t p_capacity) override;
	virtual XrSpatialComponentTypeEXT get_component_type() const override;
	virtual void* get_structure_data(void* p_next) override;

	int64_t get_capacity() const { return entity_ids.size(); }

	XrSpatialEntityIdEXT get_entity_id(int64_t p_index) const;
	XrSpatialEntityTrackingStateEXT get_entity_state(int64_t p_index) const;

	static String get_entity_tracking_state_name(XrSpatialEntityTrackingStateEXT p_tracking_state);

private:
	Vector<XrSpatialEntityIdEXT> entity_ids;
	Vector<XrSpatialEntityTrackingStateEXT> entity_states;

	XrSpatialComponentDataQueryResultEXT query_result = {
		XR_TYPE_SPATIAL_COMPONENT_DATA_QUERY_RESULT_EXT, nullptr, 0, 0, nullptr, 0, 0, nullptr};

	uint64_t _get_entity_id(int64_t p_index) const;
	OpenXRSpatialEntityTracker::EntityTrackingState _get_entity_state(int64_t p_index) const;
};


