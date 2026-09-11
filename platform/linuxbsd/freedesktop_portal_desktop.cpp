/**************************************************************************/
/*  freedesktop_portal_desktop.cpp                                        */
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

#include "freedesktop_portal_desktop.h"

#ifdef DBUS_ENABLED

#include "core/crypto/crypto_core.h"
#include "core/error/error_macros.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "servers/display/display_server.h"

#ifdef SOWRAP_ENABLED
#include "dbus-so_wrap.h"
#else
#include <dbus/dbus.h>
#endif

#include <unistd.h>

#define BUS_OBJECT_NAME "org.freedesktop.portal.Desktop"
#define BUS_OBJECT_PATH "/org/freedesktop/portal/desktop"

#define BUS_INTERFACE_PROPERTIES "org.freedesktop.DBus.Properties"
#define BUS_INTERFACE_SETTINGS "org.freedesktop.portal.Settings"
#define BUS_INTERFACE_FILE_CHOOSER "org.freedesktop.portal.FileChooser"
#define BUS_INTERFACE_SCREENSHOT "org.freedesktop.portal.Screenshot"
#define BUS_INTERFACE_INHIBIT "org.freedesktop.portal.Inhibit"
#define BUS_INTERFACE_REQUEST "org.freedesktop.portal.Request"

#define INHIBIT_FLAG_IDLE 8

bool FreeDesktopPortalDesktop::try_parse_variant(
	DBusMessage* p_reply_message, ReadVariantType p_type, void* r_value)
{
	DBusMessageIter iter[3];

	dbus_message_iter_init(p_reply_message, &iter[0]);
	if (dbus_message_iter_get_arg_type(&iter[0]) != DBUS_TYPE_VARIANT) {
		return false;
	}

	dbus_message_iter_recurse(&iter[0], &iter[1]);
	if (dbus_message_iter_get_arg_type(&iter[1]) != DBUS_TYPE_VARIANT) {
		return false;
	}

	dbus_message_iter_recurse(&iter[1], &iter[2]);
	if (p_type == VAR_TYPE_COLOR) {
		if (dbus_message_iter_get_arg_type(&iter[2]) != DBUS_TYPE_STRUCT) {
			return false;
		}
		DBusMessageIter struct_iter;
		dbus_message_iter_recurse(&iter[2], &struct_iter);
		int idx = 0;
		while (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_DOUBLE) {
			double value = 0.0;
			dbus_message_iter_get_basic(&struct_iter, &value);
			if (value < 0.0 || value > 1.0) {
				return false;
			}
			if (idx == 0) {
				static_cast<Color*>(r_value)->r = value;
			}
			else if (idx == 1) {
				static_cast<Color*>(r_value)->g = value;
			}
			else if (idx == 2) {
				static_cast<Color*>(r_value)->b = value;
			}
			idx++;
			if (!dbus_message_iter_next(&struct_iter)) {
				break;
			}
		}
		if (idx != 3) {
			return false;
		}
	}
	else if (p_type == VAR_TYPE_UINT32) {
		if (dbus_message_iter_get_arg_type(&iter[2]) != DBUS_TYPE_UINT32) {
			return false;
		}
		dbus_message_iter_get_basic(&iter[2], r_value);
	}
	else if (p_type == VAR_TYPE_BOOL) {
		if (dbus_message_iter_get_arg_type(&iter[2]) != DBUS_TYPE_BOOLEAN) {
			return false;
		}
		dbus_message_iter_get_basic(&iter[2], r_value);
	}
	return true;
}

bool FreeDesktopPortalDesktop::read_setting(
	const char* p_namespace, const char* p_key, ReadVariantType p_type, void* r_value)
{
	if (unsupported) {
		return false;
	}

	DBusError error;
	dbus_error_init(&error);

	DBusConnection* bus = dbus_bus_get(DBUS_BUS_SESSION, &error);
	if (dbus_error_is_set(&error)) {
		if (OS::get_singleton()->is_stdout_verbose()) {
			ERR_PRINT(vformat("Error opening D-Bus connection: %s", String::utf8(error.message)));
		}
		dbus_error_free(&error);
		unsupported = true;
		return false;
	}

	DBusMessage* message = dbus_message_new_method_call(
		BUS_OBJECT_NAME, BUS_OBJECT_PATH, BUS_INTERFACE_SETTINGS, "Read");
	dbus_message_append_args(
		message, DBUS_TYPE_STRING, &p_namespace, DBUS_TYPE_STRING, &p_key, DBUS_TYPE_INVALID);

	DBusMessage* reply = dbus_connection_send_with_reply_and_block(bus, message, 50, &error);
	dbus_message_unref(message);
	if (dbus_error_is_set(&error)) {
		if (OS::get_singleton()->is_stdout_verbose()) {
			ERR_PRINT(vformat("Failed to read setting %s %s: %s", p_namespace, p_key,
				String::utf8(error.message)));
		}
		dbus_error_free(&error);
		dbus_connection_unref(bus);
		return false;
	}

	bool success = try_parse_variant(reply, p_type, r_value);

	dbus_message_unref(reply);
	dbus_connection_unref(bus);

	return success;
}

uint32_t FreeDesktopPortalDesktop::get_appearance_color_scheme()
{
	if (unsupported) {
		return 0;
	}

	uint32_t value = 0;
	if (read_setting("org.freedesktop.appearance", "color-scheme", VAR_TYPE_UINT32, &value)) {
		return value;
	}
	else {
		return 0;
	}
}

Color FreeDesktopPortalDesktop::get_appearance_accent_color()
{
	if (unsupported) {
		return Color(0, 0, 0, 0);
	}

	Color value;
	if (read_setting("org.freedesktop.appearance", "accent-color", VAR_TYPE_COLOR, &value)) {
		return value;
	}
	else {
		return Color(0, 0, 0, 0);
	}
}

uint32_t FreeDesktopPortalDesktop::get_high_contrast()
{
	if (unsupported) {
		return -1;
	}

	dbus_bool_t value = false;
	if (read_setting("org.gnome.desktop.a11y.interface", "high-contrast", VAR_TYPE_BOOL, &value)) {
		return value;
	}
	return -1;
}

static const char* cs_empty = "";

void FreeDesktopPortalDesktop::append_dbus_string(DBusMessageIter* p_iter, const String& p_string)
{
	CharString cs = p_string.utf8();
	const char* cs_ptr = cs.ptr();
	if (cs_ptr) {
		dbus_message_iter_append_basic(p_iter, DBUS_TYPE_STRING, &cs_ptr);
	}
	else {
		dbus_message_iter_append_basic(p_iter, DBUS_TYPE_STRING, &cs_empty);
	}
}

void FreeDesktopPortalDesktop::append_dbus_dict_string(
	DBusMessageIter* p_iter, const String& p_key, const String& p_value, bool p_as_byte_array)
{
	DBusMessageIter dict_iter;
	DBusMessageIter var_iter;
	dbus_message_iter_open_container(p_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &dict_iter);
	append_dbus_string(&dict_iter, p_key);

	if (p_as_byte_array) {
		DBusMessageIter arr_iter;
		dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, "ay", &var_iter);
		dbus_message_iter_open_container(&var_iter, DBUS_TYPE_ARRAY, "y", &arr_iter);
		CharString cs = p_value.utf8();
		const char* cs_ptr = cs.get_data();
		do {
			dbus_message_iter_append_basic(&arr_iter, DBUS_TYPE_BYTE, cs_ptr);
		} while (*cs_ptr++);
		dbus_message_iter_close_container(&var_iter, &arr_iter);
	}
	else {
		dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, "s", &var_iter);
		append_dbus_string(&var_iter, p_value);
	}

	dbus_message_iter_close_container(&dict_iter, &var_iter);
	dbus_message_iter_close_container(p_iter, &dict_iter);
}

void FreeDesktopPortalDesktop::append_dbus_dict_bool(
	DBusMessageIter* p_iter, const String& p_key, bool p_value)
{
	DBusMessageIter dict_iter;
	DBusMessageIter var_iter;
	dbus_message_iter_open_container(p_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &dict_iter);
	append_dbus_string(&dict_iter, p_key);

	dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_VARIANT, "b", &var_iter);
	{
		int val = p_value;
		dbus_message_iter_append_basic(&var_iter, DBUS_TYPE_BOOLEAN, &val);
	}

	dbus_message_iter_close_container(&dict_iter, &var_iter);
	dbus_message_iter_close_container(p_iter, &dict_iter);
}

bool FreeDesktopPortalDesktop::_is_interface_supported(
	const char* p_iface, uint32_t p_minimum_version)
{
	bool supported = false;
	DBusError err;
	dbus_error_init(&err);
	DBusConnection* bus = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
		dbus_error_free(&err);
	}
	else {
		DBusMessage* message = dbus_message_new_method_call(
			BUS_OBJECT_NAME, BUS_OBJECT_PATH, BUS_INTERFACE_PROPERTIES, "Get");
		if (message) {
			const char* name_space = p_iface;
			const char* key = "version";
			dbus_message_append_args(
				message, DBUS_TYPE_STRING, &name_space, DBUS_TYPE_STRING, &key, DBUS_TYPE_INVALID);
			DBusMessage* reply = dbus_connection_send_with_reply_and_block(bus, message, 250, &err);
			if (dbus_error_is_set(&err)) {
				dbus_error_free(&err);
			}
			else if (reply) {
				DBusMessageIter iter;
				if (dbus_message_iter_init(reply, &iter)) {
					DBusMessageIter iter_ver;
					dbus_message_iter_recurse(&iter, &iter_ver);
					dbus_uint32_t ver_code;
					dbus_message_iter_get_basic(&iter_ver, &ver_code);
					print_verbose(
						vformat("PortalDesktop: %s version %d detected, version %d required.",
							p_iface, ver_code, p_minimum_version));
					supported = ver_code >= p_minimum_version;
				}
				dbus_message_unref(reply);
			}
			dbus_message_unref(message);
		}
		dbus_connection_unref(bus);
	}
	return supported;
}

bool FreeDesktopPortalDesktop::is_file_chooser_supported()
{
	static int supported = -1;
	if (supported == -1) {
		supported = _is_interface_supported(BUS_INTERFACE_FILE_CHOOSER, 3);
	}
	return supported;
}

bool FreeDesktopPortalDesktop::is_settings_supported()
{
	static int supported = -1;
	if (supported == -1) {
		supported = _is_interface_supported(BUS_INTERFACE_SETTINGS, 1);
	}
	return supported;
}

bool FreeDesktopPortalDesktop::is_screenshot_supported()
{
	static int supported = -1;
	if (supported == -1) {
		supported = _is_interface_supported(BUS_INTERFACE_SCREENSHOT, 1);
	}
	return supported;
}

bool FreeDesktopPortalDesktop::is_inhibit_supported()
{
	static int supported = -1;
	if (supported == -1) {
		// If not sandboxed, prefer to use org.freedesktop.ScreenSaver
		supported = OS::get_singleton()->is_sandboxed() &&
					_is_interface_supported(BUS_INTERFACE_INHIBIT, 1);
	}
	return supported;
}

Error FreeDesktopPortalDesktop::make_request_token(String& r_token)
{
	uint8_t uuid[64];
	Error rng_err = CryptoCore::generate_random(uuid, 64);
	ERR_FAIL_COND_V_MSG(rng_err, rng_err, "Failed to generate unique token.");

	r_token = String::hex_encode_buffer(uuid, 64);
	return OK;
}

bool FreeDesktopPortalDesktop::send_request(DBusMessage* p_message, const String& r_token,
	String& r_response_path, String& r_response_filter)
{
	String dbus_unique_name = String::utf8(dbus_bus_get_unique_name(monitor_connection));

	r_response_path = vformat("/org/freedesktop/portal/desktop/request/%s/%s",
		dbus_unique_name.replace_char('.', '_').remove_char(':'), r_token);
	r_response_filter =
		vformat("type='signal',sender='org.freedesktop.portal.Desktop',path='%s',interface='org."
				"freedesktop.portal.Request',member='Response',destination='%s'",
			r_response_path, dbus_unique_name);

	DBusError err;
	dbus_error_init(&err);

	dbus_bus_add_match(monitor_connection, r_response_filter.utf8().get_data(), &err);
	if (dbus_error_is_set(&err)) {
		ERR_PRINT(vformat("Failed to add DBus match: %s.", String::utf8(err.message)));
		dbus_error_free(&err);
		return false;
	}

	DBusMessage* reply = dbus_connection_send_with_reply_and_block(
		monitor_connection, p_message, DBUS_TIMEOUT_INFINITE, &err);
	dbus_message_unref(p_message);

	if (!reply || dbus_error_is_set(&err)) {
		ERR_PRINT(vformat("Failed to send DBus message: %s.", String::utf8(err.message)));
		dbus_error_free(&err);
		dbus_bus_remove_match(monitor_connection, r_response_filter.utf8().get_data(), &err);
		return false;
	}

	// Check request path matches our expectation
	{
		DBusMessageIter iter;
		if (dbus_message_iter_init(reply, &iter)) {
			if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_OBJECT_PATH) {
				const char* new_path = nullptr;
				dbus_message_iter_get_basic(&iter, &new_path);
				if (String::utf8(new_path) != r_response_path) {
					ERR_PRINT(vformat("Expected request path %s but actual path was %s.",
						r_response_path, new_path));
					dbus_bus_remove_match(
						monitor_connection, r_response_filter.utf8().get_data(), &err);
					if (dbus_error_is_set(&err)) {
						ERR_PRINT(
							vformat("Failed to remove DBus match: %s.", String::utf8(err.message)));
						dbus_error_free(&err);
					}
					return false;
				}
			}
		}
	}
	dbus_message_unref(reply);
	return true;
}

bool FreeDesktopPortalDesktop::inhibit(const String& p_xid)
{
	if (unsupported) {
		return false;
	}

	MutexLock lock(inhibit_mutex);
	ERR_FAIL_COND_V_MSG(
		!inhibit_path.is_empty(), false, "Another inhibit request is already open.");

	String token;
	if (make_request_token(token) != OK) {
		return false;
	}

	DBusMessage* message = dbus_message_new_method_call(
		BUS_OBJECT_NAME, BUS_OBJECT_PATH, BUS_INTERFACE_INHIBIT, "Inhibit");
	{
		DBusMessageIter iter;
		dbus_message_iter_init_append(message, &iter);

		append_dbus_string(&iter, p_xid);

		dbus_uint32_t flags = INHIBIT_FLAG_IDLE;
		dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &flags);

		{
			DBusMessageIter arr_iter;
			dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &arr_iter);

			append_dbus_dict_string(&arr_iter, "handle_token", token);

			const char* reason = "Running Godot Engine Project";
			append_dbus_dict_string(&arr_iter, "reason", reason);

			dbus_message_iter_close_container(&iter, &arr_iter);
		}
	}

	if (!send_request(message, token, inhibit_path, inhibit_filter)) {
		return false;
	}

	return true;
}

void FreeDesktopPortalDesktop::uninhibit()
{
	if (unsupported) {
		return;
	}

	MutexLock lock(inhibit_mutex);
	ERR_FAIL_COND_MSG(inhibit_path.is_empty(), "No inhibit request is active.");

	DBusError error;
	dbus_error_init(&error);

	DBusMessage* message = dbus_message_new_method_call(
		BUS_OBJECT_NAME, inhibit_path.utf8().get_data(), BUS_INTERFACE_REQUEST, "Close");
	DBusMessage* reply = dbus_connection_send_with_reply_and_block(
		monitor_connection, message, DBUS_TIMEOUT_USE_DEFAULT, &error);
	dbus_message_unref(message);
	if (dbus_error_is_set(&error)) {
		ERR_PRINT(vformat("Failed to uninhibit: %s.", String::utf8(error.message)));
		dbus_error_free(&error);
	}
	else if (reply) {
		dbus_message_unref(reply);
	}

	dbus_bus_remove_match(monitor_connection, inhibit_filter.utf8().get_data(), &error);
	if (dbus_error_is_set(&error)) {
		ERR_PRINT(vformat("Failed to remove match: %s.", String::utf8(error.message)));
		dbus_error_free(&error);
	}

	inhibit_path.clear();
	inhibit_filter.clear();
}

FreeDesktopPortalDesktop::FreeDesktopPortalDesktop()
{
	DBusError err;
	dbus_error_init(&err);
	monitor_connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
		dbus_error_free(&err);
	}
	else {
		theme_path = "type='signal',sender='org.freedesktop.portal.Desktop',interface='org."
					 "freedesktop.portal.Settings',member='SettingChanged'";
		dbus_bus_add_match(monitor_connection, theme_path.utf8().get_data(), &err);
		if (dbus_error_is_set(&err)) {
			dbus_error_free(&err);
			dbus_connection_unref(monitor_connection);
			monitor_connection = nullptr;
		}
		dbus_connection_read_write(monitor_connection, 0);
	}

	if (!unsupported) {
		monitor_thread_abort.clear();
		monitor_thread.start(FreeDesktopPortalDesktop::_thread_monitor, this);
	}
}

FreeDesktopPortalDesktop::~FreeDesktopPortalDesktop()
{
	monitor_thread_abort.set();
	if (monitor_thread.is_started()) {
		monitor_thread.wait_to_finish();
	}

	if (monitor_connection) {
		DBusError err;
		for (FreeDesktopPortalDesktop::FileDialogData& fd : file_dialogs) {
			dbus_error_init(&err);
			dbus_bus_remove_match(monitor_connection, fd.filter.utf8().get_data(), &err);
			dbus_error_free(&err);
		}
		dbus_error_init(&err);
		dbus_bus_remove_match(monitor_connection, theme_path.utf8().get_data(), &err);
		dbus_error_free(&err);
		dbus_connection_unref(monitor_connection);
	}
}

#endif // DBUS_ENABLED


