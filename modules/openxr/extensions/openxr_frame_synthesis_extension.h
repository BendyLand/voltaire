/**************************************************************************/
/*  openxr_frame_synthesis_extension.h                                    */
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
#include "../openxr_api.h"
#include "core/types.h"

class OpenXRFrameSynthesisExtension final
{
public:

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static void prepare_view_configuration(uint32_t p_view_count);
	static void* set_view_configuration_and_get_next_pointer(
		uint32_t p_view, void* p_next_pointer);
	static void print_view_configuration_info(uint32_t p_view);

	static void on_session_destroyed();

	static void on_main_swapchains_created();
	static void on_pre_draw_viewport(RID p_render_target);
	static void on_post_draw_viewport(RID p_render_target);
	static void* set_projection_views_and_get_next_pointer(int p_view_index, void* p_next_pointer);

	static bool is_available();

	static bool is_enabled();
	static void set_enabled(bool p_enabled);

	static bool get_relax_frame_interval();
	static void set_relax_frame_interval(bool p_relax_frame_interval);

	static void skip_next_frame();

protected:

	static void _set_render_state_enabled_rt(bool p_enabled);
	static void _set_relax_frame_interval_rt(bool p_relax_frame_interval);
	static void _set_skip_next_frame_rt();

private:
	enum SwapchainTypes
	{
		SWAPCHAIN_MOTION_VECTOR,
		SWAPCHAIN_DEPTH,
		SWAPCHAIN_MAX
	};

	static void free_swapchains();

	// Frame synthesis render state, only accessible on render thread
	struct RenderState
	{
		bool enabled = true;
		bool relax_frame_interval = false;
		bool skip_next_frame = false;

		LocalVector<XrFrameSynthesisConfigViewEXT> config_views;
		OpenXRAPI::OpenXRSwapChainInfo swapchains[SWAPCHAIN_MAX];
		LocalVector<XrFrameSynthesisInfoEXT> frame_synthesis_info;
		Transform3D previous_transform;
	};

public:
	struct Data
	{
		bool frame_synthesis_ext = false;
		bool enabled = true;
		bool relax_frame_interval = false;
		RenderState render_state;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFrameSynthesisExtension() = delete;
	OpenXRFrameSynthesisExtension(const OpenXRFrameSynthesisExtension&) = delete;
	OpenXRFrameSynthesisExtension& operator=(const OpenXRFrameSynthesisExtension&) = delete;
	~OpenXRFrameSynthesisExtension() = delete;
};


