/**************************************************************************/
/*  openxr_extension_wrapper.cpp                                          */
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

#include "openxr_extension_wrapper.h"

#include "../openxr_api.h"
#include "../openxr_api_extension.h"

#include "core/object/class_db.h"

void OpenXRExtensionWrapper::_bind_methods() {
	VLTRVIRTUAL_BIND(_get_requested_extensions, "xr_version");
	VLTRVIRTUAL_BIND(_set_system_properties_and_get_next_pointer, "next_pointer");
	VLTRVIRTUAL_BIND(_set_instance_create_info_and_get_next_pointer, "xr_version", "next_pointer");
	VLTRVIRTUAL_BIND(_set_session_create_and_get_next_pointer, "next_pointer");
	VLTRVIRTUAL_BIND(_set_swapchain_create_info_and_get_next_pointer, "next_pointer");
	VLTRVIRTUAL_BIND(_set_hand_joint_locations_and_get_next_pointer, "hand_index", "next_pointer");
	VLTRVIRTUAL_BIND(_set_projection_views_and_get_next_pointer, "view_index", "next_pointer");
	VLTRVIRTUAL_BIND(_set_frame_wait_info_and_get_next_pointer, "next_pointer");
	VLTRVIRTUAL_BIND(_set_frame_end_info_and_get_next_pointer, "next_pointer");
	VLTRVIRTUAL_BIND(_set_projection_layer_and_get_next_pointer, "next_pointer");
	VLTRVIRTUAL_BIND(_set_view_locate_info_and_get_next_pointer, "next_pointer");
	VLTRVIRTUAL_BIND(_set_reference_space_create_info_and_get_next_pointer, "reference_space_type", "next_pointer");
	VLTRVIRTUAL_BIND(_prepare_view_configuration, "view_count");
	VLTRVIRTUAL_BIND(_set_view_configuration_and_get_next_pointer, "view", "next_pointer");
	VLTRVIRTUAL_BIND(_print_view_configuration_info, "view");
	VLTRVIRTUAL_BIND(_get_composition_layer_count);
	VLTRVIRTUAL_BIND(_get_composition_layer, "index");
	VLTRVIRTUAL_BIND(_get_composition_layer_order, "index");
	VLTRVIRTUAL_BIND(_get_suggested_tracker_names);
	VLTRVIRTUAL_BIND(_on_register_metadata, "interaction_profile_metadata");
	VLTRVIRTUAL_BIND(_on_before_instance_created);
	VLTRVIRTUAL_BIND(_on_instance_created, "instance");
	VLTRVIRTUAL_BIND(_on_instance_destroyed);
	VLTRVIRTUAL_BIND(_on_session_created, "session");
	VLTRVIRTUAL_BIND(_on_process);
	VLTRVIRTUAL_BIND(_on_sync_actions);
	VLTRVIRTUAL_BIND(_on_pre_render);
	VLTRVIRTUAL_BIND(_on_main_swapchains_created);
	VLTRVIRTUAL_BIND(_on_pre_draw_viewport, "viewport");
	VLTRVIRTUAL_BIND(_on_post_draw_viewport, "viewport");
	VLTRVIRTUAL_BIND(_on_session_destroyed);
	VLTRVIRTUAL_BIND(_on_state_idle);
	VLTRVIRTUAL_BIND(_on_state_ready);
	VLTRVIRTUAL_BIND(_on_state_synchronized);
	VLTRVIRTUAL_BIND(_on_state_visible);
	VLTRVIRTUAL_BIND(_on_state_focused);
	VLTRVIRTUAL_BIND(_on_state_stopping);
	VLTRVIRTUAL_BIND(_on_state_loss_pending);
	VLTRVIRTUAL_BIND(_on_state_exiting);
	VLTRVIRTUAL_BIND(_on_event_polled, "event");
	VLTRVIRTUAL_BIND(_set_viewport_composition_layer_and_get_next_pointer, "layer", "property_values", "next_pointer");
	VLTRVIRTUAL_BIND(_get_viewport_composition_layer_extension_properties);
	VLTRVIRTUAL_BIND(_get_viewport_composition_layer_extension_property_defaults);
	VLTRVIRTUAL_BIND(_on_viewport_composition_layer_destroyed, "layer");
	VLTRVIRTUAL_BIND(_set_android_surface_swapchain_create_info_and_get_next_pointer, "property_values", "next_pointer");

#ifndef DISABLE_DEPRECATED
	VLTRVIRTUAL_BIND_COMPAT(_get_requested_extensions_bind_compat_109302);
	VLTRVIRTUAL_BIND_COMPAT(_set_instance_create_info_and_get_next_pointer_bind_compat_109302, "next_pointer");
	VLTRVIRTUAL_BIND_COMPAT(_on_register_metadata_bind_compat_117399);
#endif

	ClassDB::bind_method(D_METHOD("get_openxr_api"), &OpenXRExtensionWrapper::_gdextension_get_openxr_api);
	ClassDB::bind_method(D_METHOD("register_extension_wrapper"), &OpenXRExtensionWrapper::_gdextension_register_extension_wrapper);
}

HashMap<String, bool *> OpenXRExtensionWrapper::get_requested_extensions(XrVersion p_xr_version) {
	Dictionary request_extension;

	if (VLTRVIRTUAL_CALL(_get_requested_extensions, (uint64_t)p_xr_version, request_extension)) {
		HashMap<String, bool *> result;
		for (const KeyValue<Variant, Variant> &kv : request_extension) {
			GDExtensionPtr<bool> value = VariantCaster<GDExtensionPtr<bool>>::cast(kv.value);
			result.insert(kv.key, value);
		}
		return result;
	}

#ifndef DISABLE_DEPRECATED
	if (VLTRVIRTUAL_CALL(_get_requested_extensions_bind_compat_109302, request_extension)) {
		HashMap<String, bool *> result;
		for (const KeyValue<Variant, Variant> &kv : request_extension) {
			GDExtensionPtr<bool> value = VariantCaster<GDExtensionPtr<bool>>::cast(kv.value);
			result.insert(kv.key, value);
		}
		return result;
	}
#endif

	return HashMap<String, bool *>();
}

void *OpenXRExtensionWrapper::set_system_properties_and_get_next_pointer(void *p_next_pointer) {
	uint64_t pointer;

	if (VLTRVIRTUAL_CALL(_set_system_properties_and_get_next_pointer, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_instance_create_info_and_get_next_pointer(XrVersion p_xr_version, void *p_next_pointer) {
	uint64_t pointer;

	if (VLTRVIRTUAL_CALL(_set_instance_create_info_and_get_next_pointer, (uint64_t)p_xr_version, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

#ifndef DISABLE_DEPRECATED
	if (VLTRVIRTUAL_CALL(_set_instance_create_info_and_get_next_pointer_bind_compat_109302, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}
#endif

	return nullptr;
}

void *OpenXRExtensionWrapper::set_session_create_and_get_next_pointer(void *p_next_pointer) {
	uint64_t pointer;

	if (VLTRVIRTUAL_CALL(_set_session_create_and_get_next_pointer, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_swapchain_create_info_and_get_next_pointer(void *p_next_pointer) {
	uint64_t pointer;

	if (VLTRVIRTUAL_CALL(_set_swapchain_create_info_and_get_next_pointer, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_hand_joint_locations_and_get_next_pointer(int p_hand_index, void *p_next_pointer) {
	uint64_t pointer;

	if (VLTRVIRTUAL_CALL(_set_hand_joint_locations_and_get_next_pointer, p_hand_index, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_projection_views_and_get_next_pointer(int p_view_index, void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_projection_views_and_get_next_pointer, p_view_index, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_reference_space_create_info_and_get_next_pointer(int p_reference_space_type, void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_reference_space_create_info_and_get_next_pointer, p_reference_space_type, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_frame_wait_info_and_get_next_pointer(void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_frame_wait_info_and_get_next_pointer, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_frame_end_info_and_get_next_pointer(void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_frame_end_info_and_get_next_pointer, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void *OpenXRExtensionWrapper::set_projection_layer_and_get_next_pointer(void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_projection_layer_and_get_next_pointer, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void OpenXRExtensionWrapper::prepare_view_configuration(uint32_t p_view_count) {
	VLTRVIRTUAL_CALL(_prepare_view_configuration, p_view_count);
}

void *OpenXRExtensionWrapper::set_view_configuration_and_get_next_pointer(uint32_t p_view, void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_view_configuration_and_get_next_pointer, p_view, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

void OpenXRExtensionWrapper::print_view_configuration_info(uint32_t p_view) const {
	VLTRVIRTUAL_CALL(_print_view_configuration_info, p_view);
}

void *OpenXRExtensionWrapper::set_view_locate_info_and_get_next_pointer(void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_view_locate_info_and_get_next_pointer, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return nullptr;
}

PackedStringArray OpenXRExtensionWrapper::get_suggested_tracker_names() {
	PackedStringArray ret;

	if (VLTRVIRTUAL_CALL(_get_suggested_tracker_names, ret)) {
		return ret;
	}

	return PackedStringArray();
}

int OpenXRExtensionWrapper::get_composition_layer_count() {
	int count = 0;
	VLTRVIRTUAL_CALL(_get_composition_layer_count, count);
	return count;
}

XrCompositionLayerBaseHeader *OpenXRExtensionWrapper::get_composition_layer(int p_index) {
	uint64_t pointer;

	if (VLTRVIRTUAL_CALL(_get_composition_layer, p_index, pointer)) {
		return reinterpret_cast<XrCompositionLayerBaseHeader *>(pointer);
	}

	return nullptr;
}

int OpenXRExtensionWrapper::get_composition_layer_order(int p_index) {
	int order = 0;
	VLTRVIRTUAL_CALL(_get_composition_layer_order, p_index, order);
	return order;
}

void OpenXRExtensionWrapper::on_register_metadata(OpenXRInteractionProfileMetadata *p_interaction_profile_metadata) {
	if (VLTRVIRTUAL_CALL(_on_register_metadata, p_interaction_profile_metadata)) {
		return;
	}

#ifndef DISABLE_DEPRECATED
	VLTRVIRTUAL_CALL(_on_register_metadata_bind_compat_117399);
#endif
}

void OpenXRExtensionWrapper::on_before_instance_created() {
	VLTRVIRTUAL_CALL(_on_before_instance_created);
}

void OpenXRExtensionWrapper::on_instance_created(const XrInstance p_instance) {
	uint64_t instance = (uint64_t)p_instance;
	VLTRVIRTUAL_CALL(_on_instance_created, instance);
}

void OpenXRExtensionWrapper::on_instance_destroyed() {
	VLTRVIRTUAL_CALL(_on_instance_destroyed);
}

void OpenXRExtensionWrapper::on_session_created(const XrSession p_session) {
	uint64_t session = (uint64_t)p_session;
	VLTRVIRTUAL_CALL(_on_session_created, session);
}

void OpenXRExtensionWrapper::on_process() {
	VLTRVIRTUAL_CALL(_on_process);
}

void OpenXRExtensionWrapper::on_sync_actions() {
	VLTRVIRTUAL_CALL(_on_sync_actions);
}

void OpenXRExtensionWrapper::on_pre_render() {
	VLTRVIRTUAL_CALL(_on_pre_render);
}

void OpenXRExtensionWrapper::on_main_swapchains_created() {
	VLTRVIRTUAL_CALL(_on_main_swapchains_created);
}

void OpenXRExtensionWrapper::on_session_destroyed() {
	VLTRVIRTUAL_CALL(_on_session_destroyed);
}

void OpenXRExtensionWrapper::on_pre_draw_viewport(RID p_render_target) {
	VLTRVIRTUAL_CALL(_on_pre_draw_viewport, p_render_target);
}

void OpenXRExtensionWrapper::on_post_draw_viewport(RID p_render_target) {
	VLTRVIRTUAL_CALL(_on_post_draw_viewport, p_render_target);
}

void OpenXRExtensionWrapper::on_state_idle() {
	VLTRVIRTUAL_CALL(_on_state_idle);
}

void OpenXRExtensionWrapper::on_state_ready() {
	VLTRVIRTUAL_CALL(_on_state_ready);
}

void OpenXRExtensionWrapper::on_state_synchronized() {
	VLTRVIRTUAL_CALL(_on_state_synchronized);
}

void OpenXRExtensionWrapper::on_state_visible() {
	VLTRVIRTUAL_CALL(_on_state_visible);
}

void OpenXRExtensionWrapper::on_state_focused() {
	VLTRVIRTUAL_CALL(_on_state_focused);
}

void OpenXRExtensionWrapper::on_state_stopping() {
	VLTRVIRTUAL_CALL(_on_state_stopping);
}

void OpenXRExtensionWrapper::on_state_loss_pending() {
	VLTRVIRTUAL_CALL(_on_state_loss_pending);
}

void OpenXRExtensionWrapper::on_state_exiting() {
	VLTRVIRTUAL_CALL(_on_state_exiting);
}

bool OpenXRExtensionWrapper::on_event_polled(const XrEventDataBuffer &p_event) {
	bool event_polled;

	if (VLTRVIRTUAL_CALL(_on_event_polled, GDExtensionPtr<const void>(&p_event), event_polled)) {
		return event_polled;
	}

	return false;
}

void *OpenXRExtensionWrapper::set_viewport_composition_layer_and_get_next_pointer(const XrCompositionLayerBaseHeader *p_layer, const Dictionary &p_property_values, void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_viewport_composition_layer_and_get_next_pointer, GDExtensionPtr<const void>(p_layer), p_property_values, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return p_next_pointer;
}

void OpenXRExtensionWrapper::on_viewport_composition_layer_destroyed(const XrCompositionLayerBaseHeader *p_layer) {
	VLTRVIRTUAL_CALL(_on_viewport_composition_layer_destroyed, GDExtensionPtr<const void>(p_layer));
}

void OpenXRExtensionWrapper::get_viewport_composition_layer_extension_properties(List<PropertyInfo> *p_property_list) {
	TypedArray<Dictionary> properties;

	if (VLTRVIRTUAL_CALL(_get_viewport_composition_layer_extension_properties, properties)) {
		for (int i = 0; i < properties.size(); i++) {
			p_property_list->push_back(PropertyInfo::from_dict(properties[i]));
		}
	}
}

Dictionary OpenXRExtensionWrapper::get_viewport_composition_layer_extension_property_defaults() {
	Dictionary property_defaults;
	VLTRVIRTUAL_CALL(_get_viewport_composition_layer_extension_property_defaults, property_defaults);
	return property_defaults;
}

void *OpenXRExtensionWrapper::set_android_surface_swapchain_create_info_and_get_next_pointer(const Dictionary &p_property_values, void *p_next_pointer) {
	uint64_t pointer = 0;

	if (VLTRVIRTUAL_CALL(_set_android_surface_swapchain_create_info_and_get_next_pointer, p_property_values, GDExtensionPtr<void>(p_next_pointer), pointer)) {
		return reinterpret_cast<void *>(pointer);
	}

	return p_next_pointer;
}

Ref<OpenXRAPIExtension> OpenXRExtensionWrapper::_gdextension_get_openxr_api() {
	return openxr_api_extension;
}

void OpenXRExtensionWrapper::_gdextension_register_extension_wrapper() {
	OpenXRAPI::register_extension_wrapper(this);
}

OpenXRExtensionWrapper::OpenXRExtensionWrapper() {
	openxr_api_extension.instantiate();
}

OpenXRExtensionWrapper::~OpenXRExtensionWrapper() {
}
