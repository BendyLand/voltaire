/**************************************************************************/
/*  canvas_item_material.cpp                                              */
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

#include "canvas_item_material.h"
#include "core/version.h"
#include "servers/rendering/renderer.h"

Mutex CanvasItemMaterial::material_mutex;
SelfList<CanvasItemMaterial>::List CanvasItemMaterial::dirty_materials;
HashMap<CanvasItemMaterial::MaterialKey, CanvasItemMaterial::ShaderData,
	CanvasItemMaterial::MaterialKey>
	CanvasItemMaterial::shader_map;
CanvasItemMaterial::ShaderNames* CanvasItemMaterial::shader_names = nullptr;
Mutex CanvasItemMaterial::blit_shader_mutex;
RID CanvasItemMaterial::blit_shader_cache[6];

void CanvasItemMaterial::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->particles_anim_h_frames = "particles_anim_h_frames";
	shader_names->particles_anim_v_frames = "particles_anim_v_frames";
	shader_names->particles_anim_loop = "particles_anim_loop";
}

void CanvasItemMaterial::cleanup_blit_shaders()
{
	for (int i = 0; i < 6; i++) {
		if (blit_shader_cache[i].is_valid()) {
			Renderer::free_rid(blit_shader_cache[i]);
			blit_shader_cache[i] = RID();
		}
	}
}

void CanvasItemMaterial::finish_shaders()
{
	cleanup_blit_shaders();

	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void CanvasItemMaterial::_update_blit_shader(BlendMode p_blend)
{
	ERR_FAIL_INDEX(int(p_blend), 6);
	MutexLock shader_lock(blit_shader_mutex);
	int index = int(p_blend);
	if (blit_shader_cache[index].is_null()) {
		blit_shader_cache[index] = Renderer::shader_create();
		String code = "// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
					  " " VLTR_VERSION_FULL_CONFIG "'s CanvasItemMaterial (TextureBlit).\n\n";

		code += "shader_type texture_blit;\nrender_mode ";
		switch (p_blend) {
		case BLEND_MODE_MIX:
			code += "blend_mix";
			break;
		case BLEND_MODE_ADD:
			code += "blend_add";
			break;
		case BLEND_MODE_SUB:
			code += "blend_sub";
			break;
		case BLEND_MODE_MUL:
			code += "blend_mul";
			break;
		case BLEND_MODE_PREMULT_ALPHA:
			// texture_blit shaders support mix, add, sub, mul, disabled.
			code += "blend_mix";
			break;
		case BLEND_MODE_DISABLED:
			code += "blend_disabled";
			break;
		default:
			code += "blend_mix";
			break;
		}
		code += ";\n\n";

		code += "uniform sampler2D source_texture0 : hint_blit_source0;\n";
		code += "uniform sampler2D source_texture1 : hint_blit_source1;\n";
		code += "uniform sampler2D source_texture2 : hint_blit_source2;\n";
		code += "uniform sampler2D source_texture3 : hint_blit_source3;\n\n";

		code += "void blit() {\n";
		code += "	// Copies from each whole source texture to a rect on each output texture.\n";
		code += "	COLOR0 = texture(source_texture0, UV) * MODULATE;\n";
		code += "	COLOR1 = texture(source_texture1, UV) * MODULATE;\n";
		code += "	COLOR2 = texture(source_texture2, UV) * MODULATE;\n";
		code += "	COLOR3 = texture(source_texture3, UV) * MODULATE;\n}";
		Renderer::shader_set_code(blit_shader_cache[index], code);
	}
}

void CanvasItemMaterial::_update_shader()
{
	if (texture_blit) {
		return;
	}

	MaterialKey mk = _compute_key();
	if (mk.key == current_key.key) {
		return; // no update required in the end
	}

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}
	}

	current_key = mk;

	if (shader_map.has(mk)) {
		Renderer::material_set_shader(_get_material(), shader_map[mk].shader);
		shader_map[mk].users++;
		return;
	}

	// must create a shader!

	// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
	String code = "// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
				  " " VLTR_VERSION_FULL_CONFIG "'s CanvasItemMaterial.\n\n";

	code += "shader_type canvas_item;\nrender_mode ";
	switch (blend_mode) {
	case BLEND_MODE_MIX:
		code += "blend_mix";
		break;
	case BLEND_MODE_ADD:
		code += "blend_add";
		break;
	case BLEND_MODE_SUB:
		code += "blend_sub";
		break;
	case BLEND_MODE_MUL:
		code += "blend_mul";
		break;
	case BLEND_MODE_PREMULT_ALPHA:
		code += "blend_premul_alpha";
		break;
	case BLEND_MODE_DISABLED:
		code += "blend_disabled";
		break;
	}

	switch (light_mode) {
	case LIGHT_MODE_NORMAL:
		break;
	case LIGHT_MODE_UNSHADED:
		code += ",unshaded";
		break;
	case LIGHT_MODE_LIGHT_ONLY:
		code += ",light_only";
		break;
	}

	code += ";\n";

	if (particles_animation) {
		code += "uniform int particles_anim_h_frames;\n";
		code += "uniform int particles_anim_v_frames;\n";
		code += "uniform bool particles_anim_loop;\n\n";

		code += "void vertex() {\n";
		code += "	float h_frames = float(particles_anim_h_frames);\n";
		code += "	float v_frames = float(particles_anim_v_frames);\n";
		code += "	VERTEX.xy /= vec2(h_frames, v_frames);\n";
		code += "	float particle_total_frames = float(particles_anim_h_frames * "
				"particles_anim_v_frames);\n";
		code +=
			"	float particle_frame = floor(INSTANCE_CUSTOM.z * float(particle_total_frames));\n";
		code += "	if (!particles_anim_loop) {\n";
		code +=
			"		particle_frame = clamp(particle_frame, 0.0, particle_total_frames - 1.0);\n";
		code += "	} else {\n";
		code += "		particle_frame = mod(particle_frame, particle_total_frames);\n";
		code += "	}";
		code += "	UV /= vec2(h_frames, v_frames);\n";
		code += "	UV += vec2(mod(particle_frame, h_frames) / h_frames, floor((particle_frame + "
				"0.5) / h_frames) / v_frames);\n";
		code += "}\n";
	}

	ShaderData shader_data;
	shader_data.shader = Renderer::shader_create();
	shader_data.users = 1;

	Renderer::shader_set_code(shader_data.shader, code);

	shader_map[mk] = shader_data;

	Renderer::material_set_shader(_get_material(), shader_data.shader);
}

void CanvasItemMaterial::flush_changes()
{
	MutexLock lock(material_mutex);

	while (dirty_materials.first()) {
		dirty_materials.first()->self()->_update_shader();
		dirty_materials.first()->remove_from_list();
	}
}

void CanvasItemMaterial::_queue_shader_change()
{
	if (texture_blit || !_is_initialized()) {
		return;
	}

	MutexLock lock(material_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

void CanvasItemMaterial::set_texture_blit(bool p_enable)
{
	if (texture_blit == p_enable) {
		return;
	}

	if (!texture_blit) {
		MutexLock lock(material_mutex);
		if (element.in_list()) {
			element.remove_from_list();
		}
		if (shader_map.has(current_key)) {
			shader_map[current_key].users--;
			if (shader_map[current_key].users == 0) {
				Renderer::free_rid(shader_map[current_key].shader);
				shader_map.erase(current_key);
			}
		}
		current_key.key = 0;
		current_key.invalid_key = 1;
	}

	texture_blit = p_enable;
	blit_shader_set = false;

	if (texture_blit) {
		_update_blit_shader(blend_mode);
		Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
		blit_shader_set = true;
	}
	else {
		Renderer::material_set_shader(_get_material(), RID());
		_queue_shader_change();
	}
}

void CanvasItemMaterial::set_blend_mode(BlendMode p_blend_mode)
{
	ERR_FAIL_INDEX(int(p_blend_mode), 6);
	blend_mode = p_blend_mode;
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		if (blit_shader_set) {
			Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
		}
	}
	else {
		_queue_shader_change();
	}
}

CanvasItemMaterial::BlendMode CanvasItemMaterial::get_blend_mode() const { return blend_mode; }

void CanvasItemMaterial::set_light_mode(LightMode p_light_mode)
{
	light_mode = p_light_mode;
	_queue_shader_change();
}

CanvasItemMaterial::LightMode CanvasItemMaterial::get_light_mode() const { return light_mode; }

void CanvasItemMaterial::set_particles_animation(bool p_particles_anim)
{
	particles_animation = p_particles_anim;
	_queue_shader_change();
}

bool CanvasItemMaterial::get_particles_animation() const { return particles_animation; }

void CanvasItemMaterial::set_particles_anim_h_frames(int p_frames)
{
	particles_anim_h_frames = p_frames;
}

int CanvasItemMaterial::get_particles_anim_h_frames() const { return particles_anim_h_frames; }

void CanvasItemMaterial::set_particles_anim_v_frames(int p_frames)
{
	particles_anim_v_frames = p_frames;
}

int CanvasItemMaterial::get_particles_anim_v_frames() const { return particles_anim_v_frames; }

void CanvasItemMaterial::set_particles_anim_loop(bool p_loop) { particles_anim_loop = p_loop; }

bool CanvasItemMaterial::get_particles_anim_loop() const { return particles_anim_loop; }

RID CanvasItemMaterial::get_rid() const
{
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		if (!blit_shader_set) {
			Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
			blit_shader_set = true;
		}
	}
	return _get_material();
}

RID CanvasItemMaterial::get_shader_rid() const
{
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		return blit_shader_cache[int(blend_mode)];
	}
	ERR_FAIL_COND_V(!shader_map.has(current_key), RID());
	return shader_map[current_key].shader;
}

Shader::Mode CanvasItemMaterial::get_shader_mode() const
{
	return texture_blit ? Shader::MODE_TEXTURE_BLIT : Shader::MODE_CANVAS_ITEM;
}

CanvasItemMaterial::CanvasItemMaterial(bool p_texture_blit)
	: element(this), texture_blit(p_texture_blit)
{
	_set_material(Renderer::material_create());
	current_key.invalid_key = 1;
	set_particles_anim_h_frames(1);
	set_particles_anim_v_frames(1);
	set_particles_anim_loop(false);
	_mark_ready();
	if (texture_blit) {
		_update_blit_shader(blend_mode);
	}
	else {
		_queue_shader_change();
	}
}

CanvasItemMaterial::~CanvasItemMaterial()
{
	MutexLock lock(material_mutex);

	ERR_FAIL_NULL(Renderer::data);

	if (!texture_blit && shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}

		Renderer::material_set_shader(_get_material(), RID());
	}
}
