/**************************************************************************/
/*  particles_2d_editor_plugin.cpp                                        */
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

#include "core/io/image_loader.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "particles_2d_editor_plugin.h"
#include "scene/2d/cpu_particles_2d.h"
#include "scene/2d/gpu_particles_2d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/option_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/particle_process_material.h"

void GPUParticles2DEditorPlugin::_menu_callback(int p_idx)
{
	if (p_idx == MENU_GENERATE_VISIBILITY_RECT) {
		if (need_show_lifetime_dialog(generate_seconds)) {
			generate_visibility_rect->popup_centered();
		}
		else {
			_generate_visibility_rect();
		}
	}
	else {
		Particles2DEditorPlugin::_menu_callback(p_idx);
	}
}

void GPUParticles2DEditorPlugin::_add_menu_options(PopupMenu* p_menu)
{
	Particles2DEditorPlugin::_add_menu_options(p_menu);
	p_menu->add_item(TTR("Generate Visibility Rect"), MENU_GENERATE_VISIBILITY_RECT);
}

void Particles2DEditorPlugin::_browse_mask_texture_pressed()
{
	browsing_texture_type = TEXTURE_TYPE_MASK;
	file_dialog->popup_file_dialog();
}

void Particles2DEditorPlugin::_browse_direction_texture_pressed()
{
	browsing_texture_type = TEXTURE_TYPE_DIRECTION;
	file_dialog->popup_centered();
}

void Particles2DEditorPlugin::_file_selected(const String& p_file)
{
	switch (browsing_texture_type) {
	case TEXTURE_TYPE_MASK: {
		mask_img_path_line_edit->set_text(p_file);
		break;
	}
	case TEXTURE_TYPE_DIRECTION: {
		direction_img_path_line_edit->set_text(p_file);
		break;
	}
	}

	_validate_textures();
}

void Particles2DEditorPlugin::_process_emission_masks(PackedVector2Array& r_valid_positions,
	PackedVector2Array& r_valid_normals, PackedByteArray& r_valid_colors, Vector2i& r_image_size)
{
	Ref<Image> mask_img;
	mask_img.instantiate();
	Error err = ImageLoader::load_image(mask_img_path_line_edit->get_text(), mask_img);
	ERR_FAIL_COND_MSG(
		err != OK, vformat("Error loading image '%s'.", mask_img_path_line_edit->get_text()));

	if (mask_img->is_compressed()) {
		mask_img->decompress();
	}
	mask_img->convert(Image::FORMAT_RGBA8);
	ERR_FAIL_COND(mask_img->get_format() != Image::FORMAT_RGBA8);
	Size2i mask_img_size = mask_img->get_size();
	ERR_FAIL_COND(mask_img_size.width == 0 || mask_img_size.height == 0);

	r_image_size = mask_img_size;

	r_valid_positions.resize(mask_img_size.width * mask_img_size.height);

	MaskMode emission_mode = static_cast<MaskMode>(emission_mask_mode->get_selected());
	DirectionMode direction_mode =
		static_cast<DirectionMode>(emission_direction_mode->get_selected());

	if (direction_mode != DIRECTION_MODE_NONE) {
		r_valid_normals.resize(mask_img_size.width * mask_img_size.height);
	}

	bool capture_colors = emission_mask_colors->is_pressed();

	if (capture_colors) {
		r_valid_colors.resize(mask_img_size.width * mask_img_size.height * 4);
	}

	int valid_point_count = 0;

	{
		Vector<uint8_t> mask_img_data = mask_img->get_data();
		const uint8_t* mask_img_ptr = mask_img_data.ptr();

		for (int mask_img_x = 0; mask_img_x < mask_img_size.width; mask_img_x++) {
			for (int mask_img_y = 0; mask_img_y < mask_img_size.height; mask_img_y++) {
				uint8_t mask_alpha =
					mask_img_ptr[(mask_img_y * mask_img_size.width + mask_img_x) * 4 + 3];

				if (mask_alpha <= 128) {
					continue;
				}

				if (emission_mode == MASK_MODE_SOLID) {
					r_valid_positions.write[valid_point_count++] = Point2(mask_img_x, mask_img_y);
				}
				else {
					bool pixel_is_on_border = false;
					for (int x = mask_img_x - 1; x <= mask_img_x + 1; x++) {
						for (int y = mask_img_y - 1; y <= mask_img_y + 1; y++) {
							if (x < 0 || y < 0 || x >= mask_img_size.width ||
								y >= mask_img_size.height ||
								mask_img_ptr[(y * mask_img_size.width + x) * 4 + 3] <= 128) {
								pixel_is_on_border = true;
								break;
							}
						}

						if (pixel_is_on_border) {
							break;
						}
					}

					if (!pixel_is_on_border) {
						continue;
					}

					r_valid_positions.write[valid_point_count] = Point2(mask_img_x, mask_img_y);

					if (direction_mode == DIRECTION_MODE_GENERATE) {
						Vector2 normal;
						for (int x = mask_img_x - 2; x <= mask_img_x + 2; x++) {
							for (int y = mask_img_y - 2; y <= mask_img_y + 2; y++) {
								if (x == mask_img_x && y == mask_img_y) {
									continue;
								}

								if (x < 0 || y < 0 || x >= mask_img_size.width ||
									y >= mask_img_size.height ||
									mask_img_ptr[(y * mask_img_size.width + x) * 4 + 3] <= 128) {
									normal += Vector2(x - mask_img_x, y - mask_img_y).normalized();
								}
							}
						}

						normal.normalize();
						r_valid_normals.write[valid_point_count] = normal;
					}

					valid_point_count++;
				}
			}
		}

		if (capture_colors) {
			for (int i = 0; i < valid_point_count; ++i) {
				const Point2i point = r_valid_positions.get(i);
				r_valid_colors.write[i * 4 + 0] =
					mask_img_ptr[(point.y * mask_img_size.width + point.x) * 4 + 0];
				r_valid_colors.write[i * 4 + 1] =
					mask_img_ptr[(point.y * mask_img_size.width + point.x) * 4 + 1];
				r_valid_colors.write[i * 4 + 2] =
					mask_img_ptr[(point.y * mask_img_size.width + point.x) * 4 + 2];
				r_valid_colors.write[i * 4 + 3] =
					mask_img_ptr[(point.y * mask_img_size.width + point.x) * 4 + 3];
			}
		}
	}

	if (direction_mode == DIRECTION_MODE_TEXTURE) {
		Ref<Image> normal_img;
		normal_img.instantiate();
		err = ImageLoader::load_image(direction_img_path_line_edit->get_text(), normal_img);
		ERR_FAIL_COND_MSG(err != OK,
			vformat("Error loading image '%s'.", direction_img_path_line_edit->get_text()));

		if (normal_img->is_compressed()) {
			normal_img->decompress();
		}
		normal_img->convert(Image::FORMAT_RGB8);
		ERR_FAIL_COND(normal_img->get_format() != Image::FORMAT_RGB8);
		Size2i normal_img_size = normal_img->get_size();
		ERR_FAIL_COND(normal_img_size.width == 0 || normal_img_size.height == 0);
		ERR_FAIL_COND_MSG(
			normal_img_size != mask_img_size, "Mask and Normal texture must have the same size.");

		Vector<uint8_t> normal_img_data = normal_img->get_data();
		const uint8_t* normal_img_ptr = normal_img_data.ptr();

		for (int i = 0; i < valid_point_count; ++i) {
			const Point2i point = r_valid_positions.get(i);
			const uint8_t normal_r =
				normal_img_ptr[(point.y * normal_img_size.width + point.x) * 3 + 0];
			const uint8_t normal_g =
				normal_img_ptr[(point.y * normal_img_size.width + point.x) * 3 + 1];

			Vector2 normal;
			normal.x = static_cast<float>(normal_r) / 255.0f - 0.5f;
			normal.y = static_cast<float>(normal_g) / 255.0f - 0.5f;

			normal.normalize();

			r_valid_normals.write[i] = normal;
		}
	}

	r_valid_positions.resize(valid_point_count);
	if (!r_valid_normals.is_empty()) {
		r_valid_normals.resize(valid_point_count);
	}
}

void Particles2DEditorPlugin::_theme_changed()
{
	mask_browse_button->set_button_icon(
		mask_browse_button->get_editor_theme_icon(SNAME("FileBrowse")));
	direction_browse_button->set_button_icon(
		direction_browse_button->get_editor_theme_icon(SNAME("FileBrowse")));
}

void Particles2DEditorPlugin::_add_menu_options(PopupMenu* p_menu)
{
	p_menu->add_item(TTR("Load Emission Mask"), MENU_LOAD_EMISSION_MASK);
}

void Particles2DEditorPlugin::_validate_textures()
{
	DirectionMode direction_mode =
		static_cast<DirectionMode>(emission_direction_mode->get_selected());
	direction_img_label->set_visible(direction_mode == DIRECTION_MODE_TEXTURE);
	direction_img_hbox->set_visible(direction_mode == DIRECTION_MODE_TEXTURE);

	error_message->hide();
	emission_mask_dialog->get_ok_button()->set_disabled(true);

	if (mask_img_path_line_edit->get_text().is_empty()) {
		emission_mask_dialog->reset_size();
		return;
	}

	Ref<Image> mask_img;
	mask_img.instantiate();
	Error err = ImageLoader::load_image(mask_img_path_line_edit->get_text(), mask_img);
	if (err != OK) {
		error_message->show();
		error_message->set_text(TTRC("Failed to load mask texture."));
		emission_mask_dialog->reset_size();
		return;
	}

	if (mask_img->is_compressed()) {
		mask_img->decompress();
	}
	mask_img->convert(Image::FORMAT_RGBA8);

	if (mask_img->get_format() != Image::FORMAT_RGBA8) {
		error_message->show();
		error_message->set_text(TTRC("Failed to convert mask texture to RGBA8."));
		emission_mask_dialog->reset_size();
		return;
	}

	Size2i mask_img_size = mask_img->get_size();
	if (mask_img_size.width == 0 || mask_img_size.height == 0) {
		error_message->show();
		error_message->set_text(TTRC("Mask texture has an invalid size."));
		emission_mask_dialog->reset_size();
		return;
	}

	if (direction_mode == DIRECTION_MODE_TEXTURE) {
		if (direction_img_path_line_edit->get_text().is_empty()) {
			return;
		}

		Ref<Image> direction_img;
		direction_img.instantiate();
		err = ImageLoader::load_image(direction_img_path_line_edit->get_text(), direction_img);

		if (err != OK) {
			error_message->show();
			error_message->set_text(TTRC("Failed to load direction texture."));
			emission_mask_dialog->reset_size();
			return;
		}

		if (direction_img->is_compressed()) {
			direction_img->decompress();
		}
		direction_img->convert(Image::FORMAT_RGBA8);

		if (direction_img->get_format() != Image::FORMAT_RGBA8) {
			error_message->show();
			error_message->set_text(TTRC("Failed to convert direction texture to RGBA8."));
			emission_mask_dialog->reset_size();
			return;
		}

		Size2i direction_img_size = direction_img->get_size();

		if (direction_img_size.width == 0 || direction_img_size.height == 0 ||
			direction_img_size != mask_img_size) {
			error_message->show();
			error_message->set_text(TTRC("Direction texture has an invalid size. It must have the "
										 "same size as the mask texture."));
			emission_mask_dialog->reset_size();
			return;
		}
	}

	emission_mask_dialog->get_ok_button()->set_disabled(false);
	emission_mask_dialog->reset_size();
}

void Particles2DEditorPlugin::_emission_mask_mode_item_changed(int p_idx) const
{
	emission_direction_mode->set_item_disabled(
		DIRECTION_MODE_GENERATE, p_idx == static_cast<int>(MASK_MODE_SOLID));

	if (emission_direction_mode->get_selected() == DIRECTION_MODE_GENERATE) {
		emission_direction_mode->select(DIRECTION_MODE_NONE);
	}
}

CPUParticles2DEditorPlugin::CPUParticles2DEditorPlugin()
{
	handled_type = TTRC("CPUParticles2D");
	conversion_option_name = TTR("Convert to GPUParticles2D");
}


