/**************************************************************************/
/*  export_plugin.cpp                                                     */
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

#include "editor/editor_node.h"
#include "export_plugin.h"
#include "logo_svg.gen.h"
#include "main/splash.gen.h"
#include "run_icon_svg.gen.h"

Vector<String> EditorExportPlatformIOS::device_types({"iPhone", "iPad"});

void EditorExportPlatformIOS::initialize()
{
	if (EditorNode::get_singleton()) {
		EditorExportPlatformAppleEmbedded::_initialize(_ios_logo_svg, _ios_run_icon_svg);
#ifdef MACOS_ENABLED
		_start_remote_device_poller_thread();
#endif
	}
}

EditorExportPlatformIOS::~EditorExportPlatformIOS()
{
#ifdef MACOS_ENABLED
	_stop_remote_device_poller_thread();
#endif
}

Vector<EditorExportPlatformAppleEmbedded::IconInfo> EditorExportPlatformIOS::get_icon_infos() const
{
	return {
		// Settings on iPhone, iPad Pro, iPad, iPad mini
		{PNAME("icons/settings_58x58"), "universal", "Icon-58", "58", "2x", "29x29", false},
		{PNAME("icons/settings_87x87"), "universal", "Icon-87", "87", "3x", "29x29", false},

		// Notifications on iPhone, iPad Pro, iPad, iPad mini
		{PNAME("icons/notification_40x40"), "universal", "Icon-40", "40", "2x", "20x20", false},
		{PNAME("icons/notification_60x60"), "universal", "Icon-60", "60", "3x", "20x20", false},
		{PNAME("icons/notification_76x76"), "universal", "Icon-76", "76", "2x", "38x38", false},
		{PNAME("icons/notification_114x114"), "universal", "Icon-114", "114", "3x", "38x38", false},

		// Spotlight on iPhone, iPad Pro, iPad, iPad mini
		{PNAME("icons/spotlight_80x80"), "universal", "Icon-80", "80", "2x", "40x40", false},
		{PNAME("icons/spotlight_120x120"), "universal", "Icon-120", "120", "3x", "40x40", false},

		// Home Screen on iPhone
		{PNAME("icons/iphone_120x120"), "universal", "Icon-120-1", "120", "2x", "60x60", false},
		{PNAME("icons/iphone_180x180"), "universal", "Icon-180", "180", "3x", "60x60", false},

		// Home Screen on iPad Pro
		{PNAME("icons/ipad_167x167"), "universal", "Icon-167", "167", "2x", "83.5x83.5", false},

		// Home Screen on iPad, iPad mini
		{PNAME("icons/ipad_152x152"), "universal", "Icon-152", "152", "2x", "76x76", false},

		{PNAME("icons/ios_128x128"), "universal", "Icon-128", "128", "2x", "64x64", false},
		{PNAME("icons/ios_192x192"), "universal", "Icon-192", "192", "3x", "64x64", false},

		{PNAME("icons/ios_136x136"), "universal", "Icon-136", "136", "2x", "68x68", false},

		// App Store
		{PNAME("icons/app_store_1024x1024"), "universal", "Icon-1024", "1024", "1x", "1024x1024",
			true},
	};
}


