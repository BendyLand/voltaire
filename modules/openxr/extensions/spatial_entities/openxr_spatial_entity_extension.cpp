/**************************************************************************/
/*  openxr_spatial_entity_extension.cpp                                   */
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
#include "../../openxr_util.h"
#include "core/config/project_settings.h"
#include "openxr_spatial_entity_extension.h"

OpenXRSpatialEntityExtension* OpenXRSpatialEntityExtension::singleton = nullptr;

OpenXRSpatialEntityExtension* OpenXRSpatialEntityExtension::get_singleton() { return singleton; }

void OpenXRSpatialEntityExtension::_bind_methods() {}

OpenXRSpatialEntityExtension::OpenXRSpatialEntityExtension() { singleton = this; }

OpenXRSpatialEntityExtension::~OpenXRSpatialEntityExtension() { singleton = nullptr; }

HashMap<String, bool*> OpenXRSpatialEntityExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled")) {
		request_extensions[XR_EXT_SPATIAL_ENTITY_EXTENSION_NAME] = &spatial_entity_ext;
	}

	return request_extensions;
}

void OpenXRSpatialEntityExtension::on_instance_created(const XrInstance p_instance)
{
	if (spatial_entity_ext) {
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
	supported_capabilities.clear();
	capabilities_load_state = 0;

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

bool OpenXRSpatialEntityExtension::get_active() const { return spatial_entity_ext; }

bool OpenXRSpatialEntityExtension::_load_capabilities()
{
	if (capabilities_load_state == 0) {
		if (!spatial_entity_ext) {
			return false;
		}

		OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
		ERR_FAIL_NULL_V(openxr_api, false);

		XrInstance instance = openxr_api->get_instance();
		ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);
		XrSystemId system_id = openxr_api->get_system_id();
		ERR_FAIL_COND_V(system_id == 0, false);

		// If we fail before this point, this may be called too early.
		// Assume failure so we don't keep trying this unless we succeed.
		capabilities_load_state = 2;

		// Check our capabilities.
		Vector<XrSpatialCapabilityEXT> capabilities;
		uint32_t capability_size = 0;
		XrResult result =
			xrEnumerateSpatialCapabilitiesEXT(instance, system_id, 0, &capability_size, nullptr);
		if (XR_FAILED(result)) {
			// Not successful? then exit.
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capability count [" +
									  openxr_api->get_error_string(result) + "]");
		}

		if (capability_size > 0) {
			capabilities.resize(capability_size);
			result = xrEnumerateSpatialCapabilitiesEXT(
				instance, system_id, capabilities.size(), &capability_size, capabilities.ptrw());
			if (XR_FAILED(result)) {
				// Not successful? then exit.
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capabilities [" +
										  openxr_api->get_error_string(result) + "]");
			}

			// Loop through capabilities
			for (const XrSpatialCapabilityEXT& capability : capabilities) {
				print_verbose("OpenXR: Found spatial entity capability " +
							  get_spatial_capability_name(capability) + ".");

				SpatialEntityCapabality& spatial_entity_capability =
					supported_capabilities[capability];

				// retrieve component types for this capability
				XrSpatialCapabilityComponentTypesEXT component_types = {
					XR_TYPE_SPATIAL_CAPABILITY_COMPONENT_TYPES_EXT, // type
					nullptr,										// next
					0,												// componentTypeCapacityInput
					0,												// componentTypeCountOutput
					nullptr											// componentTypes
				};
				result = xrEnumerateSpatialCapabilityComponentTypesEXT(
					instance, system_id, capability, &component_types);
				if (XR_FAILED(result)) {
					// Not successful? just keep going.
					ERR_PRINT("OpenXR: Failed to get spatial entity component type count [" +
							  openxr_api->get_error_string(result) + "]");
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
						// Not successful? just keep going.
						ERR_PRINT("OpenXR: Failed to get spatial entity component types [" +
								  openxr_api->get_error_string(result) + "]");
					}
					else if (is_print_verbose_enabled()) {
						for (const XrSpatialComponentTypeEXT& component_type :
							spatial_entity_capability.component_types) {
							print_verbose("- component type " +
										  get_spatial_component_type_name(component_type));
						}
					}
				}

				// Retrieve features for this capability
				result = xrEnumerateSpatialCapabilityFeaturesEXT(
					instance, system_id, capability, 0, &capability_size, nullptr);
				if (XR_FAILED(result)) {
					// Not successful? just keep going.
					ERR_PRINT("OpenXR: Failed to get spatial entity feature count [" +
							  openxr_api->get_error_string(result) + "]");
				}
				else if (capability_size > 0) {
					spatial_entity_capability.features.resize(capability_size);
					result = xrEnumerateSpatialCapabilityFeaturesEXT(instance, system_id,
						capability, spatial_entity_capability.features.size(), &capability_size,
						spatial_entity_capability.features.ptrw());
					if (XR_FAILED(result)) {
						// Not successful? just keep going.
						ERR_PRINT("OpenXR: Failed to get spatial entity features [" +
								  openxr_api->get_error_string(result) + "]");
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

		capabilities_load_state = 1; // success!
	}

	return capabilities_load_state == 1;
}

bool OpenXRSpatialEntityExtension::supports_capability(XrSpatialCapabilityEXT p_capability)
{
	if (!_load_capabilities()) {
		return false;
	}

	return supported_capabilities.has(p_capability);
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

	if (supported_capabilities.has(p_capability)) {
		return supported_capabilities[p_capability].component_types.has(p_component_type);
	}
	return false;
}

bool OpenXRSpatialEntityExtension::_supports_component_type(
	Capability p_capability, ComponentType p_component_type)
{
	return supports_component_type(
		(XrSpatialCapabilityEXT)p_capability, (XrSpatialComponentTypeEXT)p_component_type);
}

bool OpenXRSpatialEntityExtension::get_spatial_context_ready(RID p_spatial_context) const
{
	SpatialContextData* context_data = spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, false);

	return context_data->spatial_context != XR_NULL_HANDLE;
}

void OpenXRSpatialEntityExtension::free_spatial_context(RID p_spatial_context)
{
	SpatialContextData* context_data = spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL(context_data);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);

	if (context_data->spatial_context != XR_NULL_HANDLE) {
		// Destroy our spatial context
		XrResult result = xrDestroySpatialContextEXT(context_data->spatial_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial context [" +
					   openxr_api->get_error_string(result) + "]");
		}
		context_data->spatial_context = XR_NULL_HANDLE;

		// And remove our RID.
		spatial_context_owner.free(p_spatial_context);
	}
}

XrSpatialContextEXT OpenXRSpatialEntityExtension::get_spatial_context_handle(
	RID p_spatial_context) const
{
	SpatialContextData* context_data = spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, XR_NULL_HANDLE);

	return context_data->spatial_context;
}

// For exposing this to GDExtension
uint64_t OpenXRSpatialEntityExtension::_get_spatial_context_handle(RID p_spatial_context) const
{
	return (uint64_t)get_spatial_context_handle(p_spatial_context);
}

// For calls from GDExtension

RID OpenXRSpatialEntityExtension::update_spatial_entities(RID p_spatial_context,
	const LocalVector<RID>& p_entities,
	const LocalVector<XrSpatialComponentTypeEXT>& p_component_types,
	Ref<OpenXRStructureBase> p_next)
{
	if (!get_active()) {
		return RID();
	}

	ERR_FAIL_COND_V(p_entities.is_empty(), RID());

	SpatialContextData* context_data = spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, RID());

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, RID());

	// Convert our entity RIDs to XrSpatialEntityEXT
	thread_local LocalVector<XrSpatialEntityEXT> entities;

	entities.resize(p_entities.size());
	XrSpatialEntityEXT* ptr = entities.ptr();
	for (const RID& rid : p_entities) {
		SpatialEntityData* entity_data = spatial_entity_owner.get_or_null(rid);
		*ptr = entity_data ? entity_data->entity : XR_NULL_HANDLE;
		ptr++;
	}

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	SpatialSnapshotData spatial_snapshot_data;

	// Store the context we used for this discovery query
	spatial_snapshot_data.spatial_context = p_spatial_context;

	// Do update
	XrSpatialUpdateSnapshotCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_UPDATE_SNAPSHOT_CREATE_INFO_EXT,				  // type
		next,															  // next
		(uint32_t)entities.size(),										  // entityCount,
		entities.ptr(),													  // entities
		(uint32_t)p_component_types.size(),								  // componentTypeCount
		p_component_types.is_empty() ? nullptr : p_component_types.ptr(), // componentTypes
		openxr_api->get_play_space(),									  // baseSpace
		openxr_api->get_predicted_display_time()						  // time
	};
	XrResult result = xrCreateSpatialUpdateSnapshotEXT(
		context_data->spatial_context, &create_info, &spatial_snapshot_data.spatial_snapshot);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create update snapshot [" +
								  openxr_api->get_error_string(result) + "]");
	}

	// Store our snapshot in an RID and return.
	return spatial_snapshot_owner.make_rid(spatial_snapshot_data);
}

void OpenXRSpatialEntityExtension::free_spatial_snapshot(RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL(snapshot_data);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);

	if (snapshot_data->spatial_snapshot != XR_NULL_HANDLE) {
		// Destroy our spatial context
		XrResult result = xrDestroySpatialSnapshotEXT(snapshot_data->spatial_snapshot);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial snapshot [" +
					   openxr_api->get_error_string(result) + "]");
		}
		snapshot_data->spatial_snapshot = XR_NULL_HANDLE;
	}

	// And remove our RID.
	spatial_snapshot_owner.free(p_spatial_snapshot);
}

XrSpatialSnapshotEXT OpenXRSpatialEntityExtension::get_spatial_snapshot_handle(
	RID p_spatial_snapshot) const
{
	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, XR_NULL_HANDLE);

	return snapshot_data->spatial_snapshot;
}

RID OpenXRSpatialEntityExtension::get_spatial_snapshot_context(RID p_spatial_snapshot) const
{
	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, RID());

	return snapshot_data->spatial_context;
}

// For exposing this to GDExtension
uint64_t OpenXRSpatialEntityExtension::_get_spatial_snapshot_handle(RID p_spatial_snapshot) const
{
	return (uint64_t)get_spatial_snapshot_handle(p_spatial_snapshot);
}

String OpenXRSpatialEntityExtension::get_string(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id) const
{
	String ret;

	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT, // type
		nullptr,							 // next
		p_buffer_id,						 // bufferId
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferStringEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + openxr_api->get_error_string(result) + "]");
	}

	LocalVector<char> buffer;
	buffer.resize(count + 1);
	buffer[count] =
		'\0'; // + 1 and setting a zero terminator just in case runtime is not including this.

	result = xrGetSpatialBufferStringEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}

	ret = String::utf8(buffer.ptr());
	return ret;
}

PackedByteArray OpenXRSpatialEntityExtension::get_uint8_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id) const
{
	PackedByteArray ret;

	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT, // type
		nullptr,							 // next
		p_buffer_id,						 // bufferId
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint8EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + openxr_api->get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint8EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (uint8_t*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedByteArray(),
			"OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint16_t> OpenXRSpatialEntityExtension::get_uint16_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id) const
{
	Vector<uint16_t> ret;

	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT, // type
		nullptr,							 // next
		p_buffer_id,						 // bufferId
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint16EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + openxr_api->get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint16EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint16_t>(),
			"OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint32_t> OpenXRSpatialEntityExtension::get_uint32_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id) const
{
	Vector<uint32_t> ret;

	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT, // type
		nullptr,							 // next
		p_buffer_id,						 // bufferId
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint32EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + openxr_api->get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint32EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint32_t>(),
			"OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}

	return ret;
}

PackedFloat32Array OpenXRSpatialEntityExtension::get_float_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id) const
{
	PackedFloat32Array ret;

	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT, // type
		nullptr,							 // next
		p_buffer_id,						 // bufferId
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferFloatEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + openxr_api->get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferFloatEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedFloat32Array(),
			"OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}

	return ret;
}

PackedVector2Array OpenXRSpatialEntityExtension::get_vector2_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id) const
{
	PackedVector2Array ret;

	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT, // type
		nullptr,							 // next
		p_buffer_id,						 // bufferId
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector2fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + openxr_api->get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	// OpenXR XrVector2f is using floats, Godot Vector2 is using double, so we need to do a copy.
	LocalVector<XrVector2f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector2* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
	}
#else
	// OpenXR's XrVector2f and Godots Vector2 should be interchangeable.
	ret.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector2f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector2Array(),
			"OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}
#endif

	return ret;
}

PackedVector3Array OpenXRSpatialEntityExtension::get_vector3_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id) const
{
	PackedVector3Array ret;

	SpatialSnapshotData* snapshot_data = spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT, // type
		nullptr,							 // next
		p_buffer_id,						 // bufferId
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector3fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + openxr_api->get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	// OpenXR XrVector3f is using floats, Godot Vector3 is using double, so we need to do a copy.
	LocalVector<XrVector3f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector3* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
		ptr[i].z = buffer[i].z;
	}
#else
	// OpenXR's XrVector3f and Godots Vector3 should be interchangeable.
	ret.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector3f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector3Array(),
			"OpenXR: Failed to get buffer [" + openxr_api->get_error_string(result) + "]");
	}
#endif

	return ret;
}

String OpenXRSpatialEntityExtension::_get_string(RID p_spatial_snapshot, uint64_t p_buffer_id) const
{
	return get_string(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedByteArray OpenXRSpatialEntityExtension::_get_uint8_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id) const
{
	return get_uint8_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedInt32Array OpenXRSpatialEntityExtension::_get_uint16_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id) const
{
	PackedInt32Array ret;
	Vector<uint16_t> buffer =
		get_uint16_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		// We don't have PackedInt16Array so we convert to PackedInt32Array

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
	RID p_spatial_snapshot, uint64_t p_buffer_id) const
{
	PackedInt32Array ret;
	Vector<uint32_t> buffer =
		get_uint32_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		// Note, we don't have a UINT32 array that we can use with GDScript and using an INT64 array
		// is overkill. Bit wasteful this but...

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
	RID p_spatial_snapshot, uint64_t p_buffer_id) const
{
	return get_float_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector2Array OpenXRSpatialEntityExtension::_get_vector2_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id) const
{
	return get_vector2_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector3Array OpenXRSpatialEntityExtension::_get_vector3_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id) const
{
	return get_vector3_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

RID OpenXRSpatialEntityExtension::find_spatial_entity(XrSpatialEntityIdEXT p_entity_id) const
{
	ERR_FAIL_COND_V(!get_active(), RID());

	LocalVector<RID> entities = spatial_entity_owner.get_owned_list();
	for (const RID& entity : entities) {
		SpatialEntityData* entity_data = spatial_entity_owner.get_or_null(entity);
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

	// Entity has been created elsewhere, we just register it
	SpatialEntityData spatial_entity_data;

	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	spatial_entity_data.entity = p_entity;

	return spatial_entity_owner.make_rid(spatial_entity_data);
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
	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL_V(openxr_api, RID());

	SpatialEntityData spatial_entity_data;

	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	XrSpatialEntityFromIdCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ENTITY_FROM_ID_CREATE_INFO_EXT, // type
		nullptr,										// next
		p_entity_id										// entityId
	};
	XrResult result = xrCreateSpatialEntityFromIdEXT(
		get_spatial_context_handle(p_spatial_context), &create_info, &spatial_entity_data.entity);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create spatial entity [" +
								  openxr_api->get_error_string(result) + "]");
	}

	return spatial_entity_owner.make_rid(spatial_entity_data);
}

RID OpenXRSpatialEntityExtension::_make_entity(RID p_spatial_context, uint64_t p_entity_id)
{
	return make_spatial_entity(p_spatial_context, (XrSpatialEntityIdEXT)p_entity_id);
}

XrSpatialEntityIdEXT OpenXRSpatialEntityExtension::get_spatial_entity_id(RID p_entity) const
{
	SpatialEntityData* entity_data = spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, XR_NULL_ENTITY);

	return entity_data->entity_id;
}

uint64_t OpenXRSpatialEntityExtension::_get_entity_id(RID p_entity) const
{
	return (uint64_t)get_spatial_entity_id(p_entity);
}

RID OpenXRSpatialEntityExtension::get_spatial_entity_context(RID p_entity) const
{
	SpatialEntityData* entity_data = spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, RID());

	return entity_data->spatial_context;
}

void OpenXRSpatialEntityExtension::free_spatial_entity(RID p_entity)
{
	SpatialEntityData* entity_data = spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL(entity_data);
	ERR_FAIL_COND(entity_data->entity == XR_NULL_HANDLE);

	OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
	ERR_FAIL_NULL(openxr_api);

	XrResult result = xrDestroySpatialEntityEXT_ptr(entity_data->entity);
	if (XR_FAILED(result)) {
		WARN_PRINT("OpenXR: Failed to destroy spatial entity [" +
				   openxr_api->get_error_string(result) + "]");
	}

	// And remove our RID.
	spatial_entity_owner.free(p_entity);
}

String OpenXRSpatialEntityExtension::get_spatial_capability_name(
	XrSpatialCapabilityEXT p_capability){XR_ENUM_SWITCH(XrSpatialCapabilityEXT, p_capability)}

String OpenXRSpatialEntityExtension::get_spatial_component_type_name(
	XrSpatialComponentTypeEXT p_component_type){
	XR_ENUM_SWITCH(XrSpatialComponentTypeEXT, p_component_type)}

String OpenXRSpatialEntityExtension::get_spatial_feature_name(
	XrSpatialCapabilityFeatureEXT p_feature)
{
	XR_ENUM_SWITCH(XrSpatialCapabilityFeatureEXT, p_feature)
}


