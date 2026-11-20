/**************************************************************************/
/*  openxr_spatial_anchor.h                                               */
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

// Persistence configuration (folded into OpenXRStructureBase)
using OpenXRSpatialContextPersistenceConfig = OpenXRStructureBase;

// Anchor tracker (folded into OpenXRSpatialEntityTracker)
using OpenXRAnchorTracker = OpenXRSpatialEntityTracker;

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

	// Persistence scopes
	struct PersistenceContextData
	{
		XrSpatialPersistenceScopeEXT scope;
		XrSpatialPersistenceContextEXT persistence_context = XR_NULL_HANDLE;
	};

	static bool _is_persistence_scope_supported(PersistenceScope p_scope);

	static uint64_t _get_persistence_context_handle(RID p_persistence_context);

	// Discovery logic
	static void _on_persistence_context_completed(RID p_persistence_context);

	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);

	static void _on_spatial_discovery_recommended(RID p_spatial_context);

	// OpenXR API call wrappers
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
