/**************************************************************************/
/*  upnp.cpp                                                              */
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

#include "upnp.h"
#include "register_types.h"

#ifndef WEB_ENABLED
#include <cstdlib>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/miniwget.h>
#include <miniupnpc/upnpcommands.h>
#endif

// UPNPDevice

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

// UPNP

bool UPNP::is_common_device(const String& dev) const
{
	return dev.is_empty() || dev.contains("InternetGatewayDevice") ||
		   dev.contains("WANIPConnection") || dev.contains("WANPPPConnection") ||
		   dev.contains("rootdevice");
}

int UPNP::discover(int timeout, int ttl, const String& device_filter)
{
#ifndef WEB_ENABLED
	ERR_FAIL_COND_V_MSG(
		timeout < 0, UPNP_RESULT_INVALID_PARAM, "The response's wait time can't be negative.");
	ERR_FAIL_COND_V_MSG(ttl < 0 || ttl > 255, UPNP_RESULT_INVALID_PARAM,
		"The time-to-live must be set between 0 and 255 (inclusive).");

	devices.clear();

	int error = 0;
	struct UPNPDev* devlist;

	CharString cs = discover_multicast_if.utf8();
	const char* m_if = cs.length() ? cs.get_data() : nullptr;
	if (is_common_device(device_filter)) {
		devlist =
			upnpDiscover(timeout, m_if, nullptr, discover_local_port, discover_ipv6, ttl, &error);
	}
	else {
		devlist = upnpDiscoverAll(
			timeout, m_if, nullptr, discover_local_port, discover_ipv6, ttl, &error);
	}

	if (error != UPNPDISCOVER_SUCCESS) {
		switch (error) {
		case UPNPDISCOVER_SOCKET_ERROR:
			return UPNP_RESULT_SOCKET_ERROR;
		case UPNPDISCOVER_MEMORY_ERROR:
			return UPNP_RESULT_MEM_ALLOC_ERROR;
		default:
			return UPNP_RESULT_UNKNOWN_ERROR;
		}
	}

	if (!devlist) {
		return UPNP_RESULT_NO_DEVICES;
	}

	struct UPNPDev* dev = devlist;

	while (dev) {
		if (device_filter.is_empty() || strstr(dev->st, device_filter.utf8().get_data())) {
			add_device_to_list(dev, devlist);
		}

		dev = dev->pNext;
	}

	freeUPNPDevlist(devlist);

	return UPNP_RESULT_SUCCESS;
#else
	return UPNP_RESULT_UNKNOWN_ERROR;
#endif
}

void UPNP::add_device_to_list(UPNPDev* dev, UPNPDev* devlist)
{
#ifndef WEB_ENABLED
	Ref<UPNPDevice> new_device;
	new_device.instantiate();

	new_device->set_description_url(dev->descURL);
	new_device->set_service_type(dev->st);

	parse_igd(new_device, devlist);

	devices.push_back(new_device);
#endif
}

char* UPNP::load_description(const String& url, int* size, int* status_code) const
{
#ifndef WEB_ENABLED
	return (char*)miniwget(url.utf8().get_data(), size, 0, status_code);
#else
	return nullptr;
#endif
}

void UPNP::parse_igd(Ref<UPNPDevice> dev, UPNPDev* devlist)
{
#ifndef WEB_ENABLED
	int size = 0;
	int status_code = -1;
	char* xml = load_description(dev->get_description_url(), &size, &status_code);

	if (status_code != 200) {
		dev->set_igd_status(UPNPDevice::IGD_STATUS_HTTP_ERROR);
		return;
	}

	if (!xml || size < 1) {
		dev->set_igd_status(UPNPDevice::IGD_STATUS_HTTP_EMPTY);
		return;
	}

	struct UPNPUrls urls = {};
	struct IGDdatas data;

	parserootdesc(xml, size, &data);
	free(xml);
	xml = nullptr;

	GetUPNPUrls(&urls, &data, dev->get_description_url().utf8().get_data(), 0);

	char addr[16];
#if MINIUPNPC_API_VERSION >= 18
	int i = UPNP_GetValidIGD(devlist, &urls, &data, (char*)&addr, 16, nullptr, 0);
#else
	int i = UPNP_GetValidIGD(devlist, &urls, &data, (char*)&addr, 16);
#endif

	if (i != 1) {
		FreeUPNPUrls(&urls);

		switch (i) {
		case 0:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_NO_IGD);
			return;
		case 2:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_DISCONNECTED);
			return;
		case 3:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_UNKNOWN_DEVICE);
			return;
		default:
			dev->set_igd_status(UPNPDevice::IGD_STATUS_UNKNOWN_ERROR);
			return;
		}
	}

	if (urls.controlURL[0] == '\0') {
		FreeUPNPUrls(&urls);
		dev->set_igd_status(UPNPDevice::IGD_STATUS_INVALID_CONTROL);
		return;
	}

	dev->set_igd_control_url(urls.controlURL);
	dev->set_igd_service_type(data.first.servicetype);
	dev->set_igd_our_addr(addr);
	dev->set_igd_status(UPNPDevice::IGD_STATUS_OK);

	FreeUPNPUrls(&urls);
#endif
}

int UPNP::upnp_result(int in)
{
#ifndef WEB_ENABLED
	switch (in) {
	case UPNPCOMMAND_SUCCESS:
		return UPNP_RESULT_SUCCESS;
	case UPNPCOMMAND_UNKNOWN_ERROR:
		return UPNP_RESULT_UNKNOWN_ERROR;
	case UPNPCOMMAND_INVALID_ARGS:
		return UPNP_RESULT_INVALID_ARGS;
	case UPNPCOMMAND_HTTP_ERROR:
		return UPNP_RESULT_HTTP_ERROR;
	case UPNPCOMMAND_INVALID_RESPONSE:
		return UPNP_RESULT_INVALID_RESPONSE;
	case UPNPCOMMAND_MEM_ALLOC_ERROR:
		return UPNP_RESULT_MEM_ALLOC_ERROR;

	case 402:
		return UPNP_RESULT_INVALID_ARGS;
	case 403:
		return UPNP_RESULT_NOT_AUTHORIZED;
	case 501:
		return UPNP_RESULT_ACTION_FAILED;
	case 606:
		return UPNP_RESULT_NOT_AUTHORIZED;
	case 714:
		return UPNP_RESULT_NO_SUCH_ENTRY_IN_ARRAY;
	case 715:
		return UPNP_RESULT_SRC_IP_WILDCARD_NOT_PERMITTED;
	case 716:
		return UPNP_RESULT_EXT_PORT_WILDCARD_NOT_PERMITTED;
	case 718:
		return UPNP_RESULT_CONFLICT_WITH_OTHER_MAPPING;
	case 724:
		return UPNP_RESULT_SAME_PORT_VALUES_REQUIRED;
	case 725:
		return UPNP_RESULT_ONLY_PERMANENT_LEASE_SUPPORTED;
	case 726:
		return UPNP_RESULT_REMOTE_HOST_MUST_BE_WILDCARD;
	case 727:
		return UPNP_RESULT_EXT_PORT_MUST_BE_WILDCARD;
	case 728:
		return UPNP_RESULT_NO_PORT_MAPS_AVAILABLE;
	case 729:
		return UPNP_RESULT_CONFLICT_WITH_OTHER_MECHANISM;
	case 732:
		return UPNP_RESULT_INT_PORT_WILDCARD_NOT_PERMITTED;
	case 733:
		return UPNP_RESULT_INCONSISTENT_PARAMETERS;
	}
#endif

	return UPNP_RESULT_UNKNOWN_ERROR;
}

int UPNP::get_device_count() const { return devices.size(); }

Ref<UPNPDevice> UPNP::get_device(int index) const
{
	ERR_FAIL_INDEX_V(index, devices.size(), nullptr);

	return devices.get(index);
}

void UPNP::add_device(Ref<UPNPDevice> device)
{
	ERR_FAIL_COND(device.is_null());

	devices.push_back(device);
}

void UPNP::set_device(int index, Ref<UPNPDevice> device)
{
	ERR_FAIL_INDEX(index, devices.size());
	ERR_FAIL_COND(device.is_null());

	devices.set(index, device);
}

void UPNP::remove_device(int index)
{
	ERR_FAIL_INDEX(index, devices.size());

	devices.remove_at(index);
}

void UPNP::clear_devices() { devices.clear(); }

Ref<UPNPDevice> UPNP::get_gateway() const
{
	ERR_FAIL_COND_V_MSG(devices.is_empty(), nullptr, "Couldn't find any UPNPDevices.");

	for (int i = 0; i < devices.size(); i++) {
		Ref<UPNPDevice> dev = get_device(i);

		if (dev.is_valid() && dev->is_valid_gateway()) {
			return dev;
		}
	}

	return nullptr;
}

void UPNP::set_discover_multicast_if(const String& m_if) { discover_multicast_if = m_if; }

String UPNP::get_discover_multicast_if() const { return discover_multicast_if; }

void UPNP::set_discover_local_port(int port) { discover_local_port = port; }

int UPNP::get_discover_local_port() const { return discover_local_port; }

void UPNP::set_discover_ipv6(bool ipv6) { discover_ipv6 = ipv6; }

bool UPNP::is_discover_ipv6() const { return discover_ipv6; }

String UPNP::query_external_address() const
{
	Ref<UPNPDevice> dev = get_gateway();

	if (dev.is_null()) {
		return "";
	}

	return dev->query_external_address();
}

int UPNP::add_port_mapping(
	int port, int port_internal, String desc, String proto, int duration) const
{
	Ref<UPNPDevice> dev = get_gateway();

	if (dev.is_null()) {
		return UPNP_RESULT_NO_GATEWAY;
	}

	return dev->add_port_mapping(port, port_internal, desc, proto, duration);
}

int UPNP::delete_port_mapping(int port, String proto) const
{
	Ref<UPNPDevice> dev = get_gateway();

	if (dev.is_null()) {
		return UPNP_RESULT_NO_GATEWAY;
	}

	return dev->delete_port_mapping(port, proto);
}

// Module registration

void initialize_upnp_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_upnp_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
