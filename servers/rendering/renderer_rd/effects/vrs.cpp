/**************************************************************************/
/*  vrs.cpp                                                               */
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

#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "vrs.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

using namespace RendererRD;

VRS::~VRS() { vrs_shader.shader.version_free(vrs_shader.shader_version); }

Size2i VRS::get_vrs_texture_size(const Size2i p_base_size) const
{
	Size2i vrs_texel_size = RD::get_singleton()->vrs_get_texel_size();
	return Size2i((p_base_size.x + vrs_texel_size.x - 1) / vrs_texel_size.x,
		(p_base_size.y + vrs_texel_size.y - 1) / vrs_texel_size.y);
}

void VRS::update_vrs_texture(RID p_vrs_fb, RID p_render_target)
{
	TextureStorage* texture_storage = TextureStorage::get_singleton();
	RSE::ViewportVRSMode vrs_mode = texture_storage->render_target_get_vrs_mode(p_render_target);
	RSE::ViewportVRSUpdateMode vrs_update_mode =
		texture_storage->render_target_get_vrs_update_mode(p_render_target);

	if (vrs_mode != RSE::VIEWPORT_VRS_DISABLED &&
		vrs_update_mode != RSE::VIEWPORT_VRS_UPDATE_DISABLED) {
		RD::get_singleton()->draw_command_begin_label("VRS Setup");

		if (vrs_mode == RSE::VIEWPORT_VRS_TEXTURE) {
			RID vrs_texture = texture_storage->render_target_get_vrs_texture(p_render_target);
			if (vrs_texture.is_valid()) {
				RID rd_texture = texture_storage->texture_get_rd_texture(vrs_texture);
				int layers = texture_storage->texture_get_layers(vrs_texture);
				if (rd_texture.is_valid()) {
					// Copy into our density buffer
					copy_vrs(rd_texture, p_vrs_fb, layers > 1);
				}
			}
#ifndef XR_DISABLED
		}
		else if (vrs_mode == RSE::VIEWPORT_VRS_XR) {
			Ref<XRInterface> interface = XRServer::get_singleton()->get_primary_interface();
			if (interface.is_valid() &&
				interface->get_vrs_texture_format() == XRInterface::XR_VRS_TEXTURE_FORMAT_UNIFIED) {
				RID vrs_texture = interface->get_vrs_texture();
				if (vrs_texture.is_valid()) {
					RID rd_texture = texture_storage->texture_get_rd_texture(vrs_texture);
					int layers = texture_storage->texture_get_layers(vrs_texture);

					if (rd_texture.is_valid()) {
						// Copy into our density buffer
						copy_vrs(rd_texture, p_vrs_fb, layers > 1);
					}
				}
			}
#endif // XR_DISABLED
		}

		if (vrs_update_mode == RSE::VIEWPORT_VRS_UPDATE_ONCE) {
			texture_storage->render_target_set_vrs_update_mode(
				p_render_target, RSE::VIEWPORT_VRS_UPDATE_DISABLED);
		}

		RD::get_singleton()->draw_command_end_label();
	}
}


