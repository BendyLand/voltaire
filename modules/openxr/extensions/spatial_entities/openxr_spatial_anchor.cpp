/**************************************************************************/
/*  openxr_spatial_anchor.cpp                                             */
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
#include "../../openxr_util.h"
#include "core/config/project_settings.h"
#include "openxr_spatial_anchor.compat.inc"
#include "openxr_spatial_anchor.h"
#include "openxr_spatial_entity_extension.h"
#include "servers/xr/xr_server.h"

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


////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationAnchor

bool OpenXRSpatialCapabilityConfigurationAnchor::has_valid_configuration() const
{
	ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), false);

	return OpenXRSpatialAnchorCapability::is_spatial_anchor_supported();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationAnchor::get_configuration()
{
	ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), nullptr);

	if (OpenXRSpatialAnchorCapability::is_spatial_anchor_supported()) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		anchor_enabled_components.clear();

		// Guaranteed components:
		anchor_enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT);

		// enable optional components
		if (OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()) {
			anchor_enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT);
		}

		anchor_config.enabledComponentCount = anchor_enabled_components.size();
		anchor_config.enabledComponents = anchor_enabled_components.ptr();

		// and return this.
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&anchor_config;
	}

	return nullptr;
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationAnchor::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : anchor_enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialComponentAnchorList

void OpenXRSpatialComponentAnchorList::set_capacity(uint32_t p_capacity)
{
	entity_poses.resize(p_capacity);

	anchor_list.locationCount = uint32_t(entity_poses.size());
	anchor_list.locations = entity_poses.ptrw();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentAnchorList::get_component_type() const
{
	return XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT;
}

void* OpenXRSpatialComponentAnchorList::get_structure_data(void* p_next)
{
	anchor_list.next = p_next;
	return &anchor_list;
}

Transform3D OpenXRSpatialComponentAnchorList::get_entity_pose(int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, entity_poses.size(), Transform3D());

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	return OpenXRAPI::transform_from_pose(entity_poses[p_index]);
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialComponentPersistenceList

void OpenXRSpatialComponentPersistenceList::set_capacity(uint32_t p_capacity)
{
	persist_data.resize(p_capacity);

	persistence_list.persistDataCount = uint32_t(persist_data.size());
	persistence_list.persistData = persist_data.ptrw();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentPersistenceList::get_component_type() const
{
	return XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT;
}

void* OpenXRSpatialComponentPersistenceList::get_structure_data(void* p_next)
{
	persistence_list.next = p_next;
	return &persistence_list;
}

XrUuid OpenXRSpatialComponentPersistenceList::get_persistent_uuid(int64_t p_index) const
{
	XrUuid null_uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	ERR_FAIL_INDEX_V(p_index, persist_data.size(), null_uuid);

	return persist_data[p_index].persistUuid;
}

String OpenXRSpatialComponentPersistenceList::_get_persistent_uuid(int64_t p_index) const
{
	return OpenXRUtil::string_from_xruuid(get_persistent_uuid(p_index));
}

XrSpatialPersistenceStateEXT OpenXRSpatialComponentPersistenceList::get_persistent_state(
	int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, persist_data.size(), XR_SPATIAL_PERSISTENCE_STATE_MAX_ENUM_EXT);

	return persist_data[p_index].persistState;
}

uint64_t OpenXRSpatialComponentPersistenceList::_get_persistent_state(int64_t p_index) const
{
	// TODO make a Godot constant that mirrors XrSpatialPersistenceStateEXT and return that
	return (uint64_t)get_persistent_state(p_index);
}

String OpenXRSpatialComponentPersistenceList::get_persistence_state_name(
	XrSpatialPersistenceStateEXT p_state)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceStateEXT, p_state)
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialAnchorCapability

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

		// TODO REMOVE THIS WORKAROUND ONCE POSSIBLE
		// There has been some back and forth between stores and scopes. Scopes won out.
		XrResult xr_result =
			OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceScopesEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		if (xr_result != XR_SUCCESS) {
			// Check for stores for compatibility with beta runtimes.
			// Lucky for us, related structs and enums are compatible.
			print_verbose("OpenXR: xrEnumerateSpatialPersistenceScopesEXT is not supported, "
						  "falling back to xrEnumerateSpatialPersistenceStoresEXT!");
			xr_result = OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceStoresEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		}
		ERR_FAIL_COND(XR_FAILED(xr_result));
		// EXT_INIT_XR_FUNC(xrEnumerateSpatialPersistenceScopesEXT);

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
{ return data != nullptr && data->spatial_anchor_supported; }

bool OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()
{
	// Need anchor support for persistence to be usable
	if (!is_spatial_anchor_supported()) {
		return false;
	}

	return data->spatial_persistence_ext;
}

////////////////////////////////////////////////////////////////////////////
// Persistence scopes

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
				data->supported_persistence_scopes.size(), &size, data->supported_persistence_scopes.ptrw());
			if (XR_FAILED(result)) {
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to query persistence scopes [" +
										  OpenXRAPI::get_error_string(result) + "]");
			}
		}

		if (is_print_verbose_enabled()) {
			if (!data->supported_persistence_scopes.is_empty()) {
				print_verbose("OpenXR: Supported spatial persistence scopes:");
				for (const XrSpatialPersistenceScopeEXT& scope : data->supported_persistence_scopes) {
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

uint64_t OpenXRSpatialAnchorCapability::_get_persistence_context_handle(
	RID p_persistence_context)
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
		// Destroy our spatial context
		XrResult result =
			xrDestroySpatialPersistenceContextEXT(persistence_context_data->persistence_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the persistence context [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		persistence_context_data->persistence_context = XR_NULL_HANDLE;
	}

	// And remove our RID.
	data->persistence_context_owner.free(p_persistence_context);
}

////////////////////////////////////////////////////////////////////////////
// Discovery logic

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
		// Trigger new discovery.
		data->need_discovery = true;
	}
}

////////////////////////////////////////////////////////////////////////////
// Anchor creation

Ref<OpenXRAnchorTracker> OpenXRSpatialAnchorCapability::create_new_anchor(
	const Transform3D& p_transform, RID p_spatial_context, Ref<OpenXRStructureBase> p_next)
{
	Ref<OpenXRAnchorTracker> tracker;

	ERR_FAIL_COND_V_MSG(!is_spatial_anchor_supported(), tracker,
		"OpenXR: Spatial entity anchor capability is not supported on this hardware!");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), tracker);
	ERR_FAIL_COND_V(!XRServer::is_initialized(), tracker);
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), tracker);

	// TODO reverse apply world scale and reference frame to transform.

	XrPosef pose = OpenXRAPI::pose_from_transform(p_transform);

	RID sc = p_spatial_context.is_valid() ? p_spatial_context : data->spatial_context;
	ERR_FAIL_COND_V(sc.is_null(), tracker);

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	XrSpatialAnchorCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_EXT,	  // type
		next,									  // next
		OpenXRAPI::get_play_space(),			  // baseSpace
		OpenXRAPI::get_predicted_display_time(), // time
		pose									  // pose
	};
	XrSpatialEntityIdEXT entity_id;
	XrSpatialEntityEXT entity;
	XrResult result = xrCreateSpatialAnchorEXT(
		OpenXRSpatialEntityExtension::get_spatial_context_handle(sc), &create_info, &entity_id, &entity);
	if (XR_FAILED(result)) { // Did our xrCreateSpatialContextCompleteEXT call fail?
		ERR_FAIL_V_MSG(tracker,
			"OpenXR: Failed to create anchor [" + OpenXRAPI::get_error_string(result) + "]");
	}

	tracker.instantiate();
	tracker->set_entity_type(OpenXRSpatialEntityTracker::ENTITY_ANCHOR);
	tracker->set_spatial_context(sc);
	tracker->set_entity(OpenXRSpatialEntityExtension::add_spatial_entity(sc, entity_id, entity));
	tracker->set_tracker_desc("Anchor");
	tracker->set_pose(SNAME("default"), p_transform, Vector3(), Vector3());
	tracker->add_next(p_next);

	// Remember our tracker.
	data->anchor_trackers[sc][entity_id] = tracker;
	XRServer::add_tracker(tracker);

	return tracker;
}

void OpenXRSpatialAnchorCapability::remove_anchor(Ref<OpenXRAnchorTracker> p_anchor_tracker)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	ERR_FAIL_COND(!XRServer::is_initialized());
	ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

	// We check for this here. We could do this asynchronous but the caller may than wrongly expect
	// this method to be instant.
	ERR_FAIL_COND_MSG(p_anchor_tracker->has_uuid(),
		"OpenXR: This anchor is persistent. It must first be made unpersistent.");

	// Attempt to unregister it from our xr_server.
	XRServer::remove_tracker(p_anchor_tracker);

	// Get our entity.
	RID entity = p_anchor_tracker->get_entity();
	ERR_FAIL_COND(entity.is_null());

	// Get our entity id.
	XrSpatialEntityIdEXT entity_id = OpenXRSpatialEntityExtension::get_spatial_entity_id(entity);
	ERR_FAIL_COND(entity_id == XR_NULL_ENTITY);

	// Remove it from our entity list.
	for (KeyValue<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRAnchorTracker>>>& anchors :
		data->anchor_trackers) {
		if (anchors.value.has(entity_id)) {
			anchors.value.erase(entity_id);
			break;
		}
	}

	// Clear our entity, this will free it as well.
	p_anchor_tracker->set_entity(RID());

	// The anchor tracker will be cleaned up once its fully dereferenced.
}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_scope_name(
	XrSpatialPersistenceScopeEXT p_scope){XR_ENUM_SWITCH(XrSpatialPersistenceScopeEXT, p_scope)}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_context_result_name(
	XrSpatialPersistenceContextResultEXT p_result){
	XR_ENUM_SWITCH(XrSpatialPersistenceContextResultEXT, p_result)}

Ref<OpenXRFutureResult> OpenXRSpatialAnchorCapability::_create_spatial_context()
{
	Ref<OpenXRFutureResult> oxfr = memnew(OpenXRFutureResult);
	return oxfr;
}

void OpenXRSpatialAnchorCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialAnchorCapability::on_session_destroyed() {}

void OpenXRSpatialAnchorCapability::on_process() {}


