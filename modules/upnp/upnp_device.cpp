/**************************************************************************/
/*  upnp_device.cpp                                                       */
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

#include "upnp_device.h"
#include "upnp.h"

#ifndef WEB_ENABLED
#include <miniupnpc/upnpcommands.h>
#endif

String UPNPDevice::query_external_address() const
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(!is_valid_gateway(), "", "The Internet Gateway Device must be valid.");

	char addr[16];
	int i = UPNP_GetExternalIPAddress(
		igd_control_url.utf8().get_data(), igd_service_type.utf8().get_data(), (char*)&addr);

	ERR_FAIL_COND_V_MSG(i != UPNPCOMMAND_SUCCESS, "", "Couldn't get external IP address.");

	return String(addr);
#else
	return String();
#endif
}

int UPNPDevice::add_port_mapping(
	int port, int port_internal, String desc, String proto, int duration) const
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(!is_valid_gateway(), UPNP::UPNP_RESULT_INVALID_GATEWAY,
		"The Internet Gateway Device must be valid.");
	ERR_FAIL_COND_V_MSG(port < 1 || port > 65535, UPNP::UPNP_RESULT_INVALID_PORT,
		"The port number must be set between 1 and 65535 (inclusive).");
	ERR_FAIL_COND_V_MSG(port_internal < 0 || port_internal > 65535, UPNP::UPNP_RESULT_INVALID_PORT,
		"The port number must be set between 0 and 65535 (inclusive)."); // Needs to allow 0 because
																		 // 0 signifies "use
																		 // external port as
																		 // internal port"
	ERR_FAIL_COND_V_MSG(proto != "UDP" && proto != "TCP", UPNP::UPNP_RESULT_INVALID_PROTOCOL,
		"The protocol must be either TCP or UDP.");
	ERR_FAIL_COND_V_MSG(duration < 0, UPNP::UPNP_RESULT_INVALID_DURATION,
		"The port mapping's lease duration can't be negative.");

	if (port_internal < 1) {
		port_internal = port;
	}

	int i = UPNP_AddPortMapping(igd_control_url.utf8().get_data(),
		igd_service_type.utf8().get_data(), itos(port).utf8().get_data(),
		itos(port_internal).utf8().get_data(), igd_our_addr.utf8().get_data(),
		desc.is_empty() ? nullptr : desc.utf8().get_data(), proto.utf8().get_data(),
		nullptr, // Remote host, always nullptr as IGDs don't support it
		duration > 0 ? itos(duration).utf8().get_data() : nullptr);

	ERR_FAIL_COND_V_MSG(
		i != UPNPCOMMAND_SUCCESS, UPNP::upnp_result(i), "Couldn't add port mapping.");

	return UPNP::UPNP_RESULT_SUCCESS;
#else
	return UPNP::UPNP_RESULT_UNKNOWN_ERROR;
#endif
}

int UPNPDevice::delete_port_mapping(int port, String proto) const
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(port < 1 || port > 65535, UPNP::UPNP_RESULT_INVALID_PORT,
		"The port number must be set between 1 and 65535 (inclusive).");
	ERR_FAIL_COND_V_MSG(proto != "UDP" && proto != "TCP", UPNP::UPNP_RESULT_INVALID_PROTOCOL,
		"The protocol must be either TCP or UDP.");

	int i = UPNP_DeletePortMapping(igd_control_url.utf8().get_data(),
		igd_service_type.utf8().get_data(), itos(port).utf8().get_data(), proto.utf8().get_data(),
		nullptr // Remote host, always nullptr as IGDs don't support it
	);

	ERR_FAIL_COND_V_MSG(
		i != UPNPCOMMAND_SUCCESS, UPNP::upnp_result(i), "Couldn't delete port mapping.");

	return UPNP::UPNP_RESULT_SUCCESS;
#else
	return UPNP::UPNP_RESULT_UNKNOWN_ERROR;
#endif
}

void UPNPDevice::set_description_url(const String& url) { description_url = url; }

String UPNPDevice::get_description_url() const { return description_url; }

void UPNPDevice::set_service_type(const String& type) { service_type = type; }

String UPNPDevice::get_service_type() const { return service_type; }

void UPNPDevice::set_igd_control_url(const String& url) { igd_control_url = url; }

String UPNPDevice::get_igd_control_url() const { return igd_control_url; }

void UPNPDevice::set_igd_service_type(const String& type) { igd_service_type = type; }

String UPNPDevice::get_igd_service_type() const { return igd_service_type; }

void UPNPDevice::set_igd_our_addr(const String& addr) { igd_our_addr = addr; }

String UPNPDevice::get_igd_our_addr() const { return igd_our_addr; }

void UPNPDevice::set_igd_status(IGDStatus status) { igd_status = status; }

UPNPDevice::IGDStatus UPNPDevice::get_igd_status() const { return igd_status; }

bool UPNPDevice::is_valid_gateway() const { return igd_status == IGD_STATUS_OK; }
