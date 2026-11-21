/**************************************************************************/
/*  rasterizer_dummy.cpp                                                  */
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

#include "servers/rendering/dummy/rasterizer_dummy.h"

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/storage/camera_attributes_storage.h"
#include "servers/rendering/storage/particles_storage.h"

/* LightStorage */

void RendererDummy::LightStorage::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void RendererDummy::LightStorage::finalize()
{
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

bool RendererDummy::LightStorage::free(RID p_rid)
{
	if (owns_lightmap(p_rid)) {
		lightmap_free(p_rid);
		return true;
	}
	else if (owns_lightmap_instance(p_rid)) {
		lightmap_instance_free(p_rid);
		return true;
	}

	return false;
}

RID RendererDummy::LightStorage::lightmap_allocate()
{
	return _get_data()->lightmap_owner.allocate_rid();
}

void RendererDummy::LightStorage::lightmap_initialize(RID p_lightmap)
{
	_get_data()->lightmap_owner.initialize_rid(p_lightmap, Lightmap());
}

void RendererDummy::LightStorage::lightmap_free(RID p_rid)
{
	if (!data) {
		return;
	}
	lightmap_set_textures(p_rid, RID(), false);
	data->lightmap_owner.free(p_rid);
}

RID RendererDummy::LightStorage::lightmap_instance_create(RID p_lightmap)
{
	LightmapInstance li;
	li.lightmap = p_lightmap;
	return _get_data()->lightmap_instance_owner.make_rid(li);
}

void RendererDummy::LightStorage::lightmap_instance_free(RID p_lightmap)
{
	if (!data) {
		return;
	}
	data->lightmap_instance_owner.free(p_lightmap);
}

/* MaterialStorage */

void RendererDummy::MaterialStorage::global_shader_parameter_remove(const String& p_name)
{
	Data* d = _get_data();
	if (!d->global_shader_variables.has(p_name)) {
		return;
	}

	d->global_shader_variables.erase(p_name);
}

Vector<String> RendererDummy::MaterialStorage::global_shader_parameter_get_list()
{
	Data* d = _get_data();
	Vector<String> names;
	for (const KeyValue<String, RSE::GlobalShaderParameterType>& E : d->global_shader_variables) {
		names.push_back(E.key);
	}
	names.sort();
	return names;
}

RID RendererDummy::MaterialStorage::shader_allocate()
{
	return _get_data()->shader_owner.allocate_rid();
}

void RendererDummy::MaterialStorage::shader_initialize(RID p_rid, bool p_embedded)
{
	_get_data()->shader_owner.initialize_rid(p_rid, DummyShader());
}

void RendererDummy::MaterialStorage::shader_free(RID p_rid)
{
	if (!data) {
		return;
	}
	DummyShader* shader = data->shader_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(shader);

	data->shader_owner.free(p_rid);
}

void RendererDummy::MaterialStorage::shader_set_code(RID p_shader, const String& p_code)
{
	Data* d = _get_data();
	DummyShader* shader = d->shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);
	if (p_code.is_empty()) {
		return;
	}

	String mode_string = ShaderLanguage::get_shader_type(p_code);

	RSE::ShaderMode new_mode;
	if (mode_string == "canvas_item") {
		new_mode = RSE::SHADER_CANVAS_ITEM;
	}
	else if (mode_string == "particles") {
		new_mode = RSE::SHADER_PARTICLES;
	}
	else if (mode_string == "spatial") {
		new_mode = RSE::SHADER_SPATIAL;
	}
	else if (mode_string == "sky") {
		new_mode = RSE::SHADER_SKY;
	}
	else if (mode_string == "fog") {
		new_mode = RSE::SHADER_FOG;
	}
	else if (mode_string == "texture_blit") {
		new_mode = RSE::SHADER_TEXTURE_BLIT;
	}
	else {
		new_mode = RSE::SHADER_MAX;
		ERR_FAIL_MSG("Shader type " + mode_string + " not supported in Dummy renderer.");
	}
	ShaderCompiler::IdentifierActions actions;
	actions.uniforms = &shader->uniforms;
	ShaderCompiler::GeneratedCode gen_code;

	Error err = d->dummy_compiler->compile(new_mode, p_code, &actions, "", gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");
}

RID RendererDummy::MaterialStorage::material_allocate()
{
	return _get_data()->material_owner.allocate_rid();
}

void RendererDummy::MaterialStorage::material_initialize(RID p_rid)
{
	_get_data()->material_owner.initialize_rid(p_rid, DummyMaterial());
}

void RendererDummy::MaterialStorage::material_free(RID p_rid)
{
	if (!data) {
		return;
	}
	DummyMaterial* material = data->material_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(material);

	data->material_owner.free(p_rid);
}

void RendererDummy::MaterialStorage::material_set_shader(RID p_material, RID p_shader)
{
	DummyMaterial* material = _get_data()->material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	material->shader = p_shader;
}

void RendererDummy::MaterialStorage::material_set_next_pass(RID p_material, RID p_next_material)
{
	DummyMaterial* material = _get_data()->material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	material->next_pass = p_next_material;
}

RSE::GlobalShaderParameterType RendererDummy::MaterialStorage::global_shader_parameter_get_type(
	const String& p_name)
{
	return RSE::GLOBAL_VAR_TYPE_MAX;
}

void RendererDummy::MaterialStorage::global_shader_parameters_load_settings(bool p_load_uniforms) {}

void RendererDummy::MaterialStorage::material_get_instance_shader_parameters(
	RID p_material, List<InstanceShaderParam>* r_parameters)
{
}

void RendererDummy::MaterialStorage::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
	if (!data->dummy_compiler) {
		data->dummy_compiler = memnew(ShaderCompiler);
		ShaderCompiler::DefaultIdentifierActions actions;
		data->dummy_compiler->initialize(actions);
	}
}

void RendererDummy::MaterialStorage::finalize()
{
	if (!data) {
		return;
	}
	if (data->dummy_compiler) {
		memdelete(data->dummy_compiler);
		data->dummy_compiler = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

/* Utilities */

RSE::InstanceType RendererDummy::Utilities::get_base_type(RID p_rid)
{
	if (RendererDummy::MeshStorage::owns_mesh(p_rid)) {
		return RSE::INSTANCE_MESH;
	}
	else if (RendererDummy::MeshStorage::owns_multimesh(p_rid)) {
		return RSE::INSTANCE_MULTIMESH;
	}
	else if (RendererDummy::LightStorage::owns_lightmap(p_rid)) {
		return RSE::INSTANCE_LIGHTMAP;
	}
	return RSE::INSTANCE_NONE;
}

bool RendererDummy::Utilities::free(RID p_rid)
{
	if (RendererDummy::LightStorage::free(p_rid)) {
		return true;
	}
	else if (RendererDummy::TextureStorage::owns_texture(p_rid)) {
		RendererDummy::TextureStorage::texture_free(p_rid);
		return true;
	}
	else if (RendererDummy::TextureStorage::owns_canvas_texture(p_rid)) {
		RendererDummy::TextureStorage::canvas_texture_free(p_rid);
		return true;
	}
	else if (RendererDummy::TextureStorage::owns_render_target(p_rid)) {
		RendererDummy::TextureStorage::render_target_free(p_rid);
		return true;
	}
	else if (RendererDummy::MeshStorage::owns_mesh(p_rid)) {
		RendererDummy::MeshStorage::mesh_free(p_rid);
		return true;
	}
	else if (RendererDummy::MeshStorage::owns_multimesh(p_rid)) {
		RendererDummy::MeshStorage::multimesh_free(p_rid);
		return true;
	}
	else if (RendererDummy::MaterialStorage::owns_shader(p_rid)) {
		RendererDummy::MaterialStorage::shader_free(p_rid);
		return true;
	}
	else if (RendererDummy::MaterialStorage::owns_material(p_rid)) {
		RendererDummy::MaterialStorage::material_free(p_rid);
		return true;
	}
	return false;
}

void RendererDummy::Utilities::base_update_dependency(RID p_base, DependencyTracker* p_instance)
{
	if (RendererDummy::MeshStorage::owns_mesh(p_base)) {
		DummyMesh* mesh = RendererDummy::MeshStorage::get_mesh(p_base);
		p_instance->update_dependency(&mesh->dependency);
	}
}

/* RasterizerSceneDummy */

bool RasterizerSceneDummy::free(RID p_rid)
{
	if (is_environment(p_rid)) {
		environment_free(p_rid);
		return true;
	}
	else if (is_compositor(p_rid)) {
		compositor_free(p_rid);
		return true;
	}
	else if (is_compositor_effect(p_rid)) {
		compositor_effect_free(p_rid);
		return true;
	}
	else if (RendererCameraAttributes::owns_camera_attributes(p_rid)) {
		RendererCameraAttributes::camera_attributes_free(p_rid);
		return true;
	}
	else {
		return false;
	}
}

/* RasterizerDummy */

void RasterizerDummy::initialize()
{
	RendererDummy::Utilities::initialize();
	RendererDummy::TextureStorage::initialize();
	RendererDummy::MaterialStorage::initialize();
	RendererDummy::MeshStorage::initialize();
	RendererDummy::LightStorage::initialize();
	RendererDummy::ParticlesStorage::initialize();
	RendererDummy::GI::initialize();
	RendererDummy::Fog::initialize();
	RasterizerCanvasDummy::initialize();

	if (!scene) {
		scene = memnew(RasterizerSceneDummy);
	}
	if (RendererSceneCull::is_initialized()) {
		RendererSceneCull::set_scene_render(scene);
	}
}

void RasterizerDummy::begin_frame(double frame_step)
{
	frame++;
	delta = frame_step;
	time += frame_step;

	if (RendererCanvasRender::set_time) {
		RendererCanvasRender::set_time(time);
	}
	if (scene) {
		scene->set_time(time, frame_step);
	}
}

void RasterizerDummy::end_frame(bool p_present)
{
	if (p_present) {
		DisplayServer::swap_buffers();
	}
}

void RasterizerDummy::finalize()
{
	if (RendererSceneCull::is_initialized() && RendererSceneCull::get_scene_render() == scene) {
		RendererSceneCull::set_scene_render(nullptr);
	}
	if (scene) {
		memdelete(scene);
		scene = nullptr;
	}

	RasterizerCanvasDummy::finalize();
	RendererDummy::Fog::finalize();
	RendererDummy::GI::finalize();
	RendererDummy::ParticlesStorage::finalize();
	RendererDummy::LightStorage::finalize();
	RendererDummy::MeshStorage::finalize();
	RendererDummy::MaterialStorage::finalize();
	RendererDummy::TextureStorage::finalize();
	RendererDummy::Utilities::finalize();
}

void RasterizerDummy::make_current()
{
	RendererCompositor::low_end = false;
	RendererCompositor::bind_compositor<RasterizerDummy>();
}

void RasterizerDummy::bind_texture_storage()
{
	RendererTextureStorage::bind<RendererDummy::TextureStorage>();
}

void RasterizerDummy::bind_material_storage()
{
	RendererMaterialStorage::bind<RendererDummy::MaterialStorage>();
}

void RasterizerDummy::bind_canvas()
{
	RendererCanvasRender::bind<RasterizerCanvasDummy>();
	RendererCanvasRender::initialize = &RasterizerCanvasDummy::initialize;
	RendererCanvasRender::finalize = &RasterizerCanvasDummy::finalize;
	RendererCanvasRender::set_time = &RasterizerCanvasDummy::set_time;
	RendererCanvasRender::update = &RasterizerCanvasDummy::update;
	RendererCanvasRender::free = &RasterizerCanvasDummy::free;
}

void RasterizerDummy::bind_particles_storage()
{
	RendererParticlesStorage::bind<RendererDummy::ParticlesStorage>();
}

void RasterizerDummy::bind_utilities()
{
	RendererUtilities::visibility_notifier_allocate =
		&RendererDummy::Utilities::visibility_notifier_allocate;
	RendererUtilities::visibility_notifier_initialize =
		&RendererDummy::Utilities::visibility_notifier_initialize;
	RendererUtilities::visibility_notifier_free =
		&RendererDummy::Utilities::visibility_notifier_free;
	RendererUtilities::visibility_notifier_set_aabb =
		&RendererDummy::Utilities::visibility_notifier_set_aabb;
	RendererUtilities::visibility_notifier_get_aabb =
		&RendererDummy::Utilities::visibility_notifier_get_aabb;
	RendererUtilities::visibility_notifier_call =
		&RendererDummy::Utilities::visibility_notifier_call;

	RendererUtilities::free = &RendererDummy::Utilities::free;
	RendererUtilities::get_base_type = &RendererDummy::Utilities::get_base_type;
	RendererUtilities::base_update_dependency = &RendererDummy::Utilities::base_update_dependency;

	RendererUtilities::capture_timestamps_begin =
		&RendererDummy::Utilities::capture_timestamps_begin;
	RendererUtilities::capture_timestamp = &RendererDummy::Utilities::capture_timestamp;
	RendererUtilities::get_captured_timestamps_count =
		&RendererDummy::Utilities::get_captured_timestamps_count;
	RendererUtilities::get_captured_timestamps_frame =
		&RendererDummy::Utilities::get_captured_timestamps_frame;
	RendererUtilities::get_captured_timestamp_gpu_time =
		&RendererDummy::Utilities::get_captured_timestamp_gpu_time;
	RendererUtilities::get_captured_timestamp_cpu_time =
		&RendererDummy::Utilities::get_captured_timestamp_cpu_time;
	RendererUtilities::get_captured_timestamp_name =
		&RendererDummy::Utilities::get_captured_timestamp_name;

	RendererUtilities::update_dirty_resources = &RendererDummy::Utilities::update_dirty_resources;
	RendererUtilities::set_debug_generate_wireframes =
		&RendererDummy::Utilities::set_debug_generate_wireframes;
	RendererUtilities::has_os_feature = &RendererDummy::Utilities::has_os_feature;
	RendererUtilities::update_memory_info = &RendererDummy::Utilities::update_memory_info;

	RendererUtilities::get_rendering_info = &RendererDummy::Utilities::get_rendering_info;
	RendererUtilities::get_video_adapter_name = &RendererDummy::Utilities::get_video_adapter_name;
	RendererUtilities::get_video_adapter_vendor =
		&RendererDummy::Utilities::get_video_adapter_vendor;
	RendererUtilities::get_video_adapter_type = &RendererDummy::Utilities::get_video_adapter_type;
	RendererUtilities::get_video_adapter_api_version =
		&RendererDummy::Utilities::get_video_adapter_api_version;

	RendererUtilities::get_maximum_viewport_size =
		&RendererDummy::Utilities::get_maximum_viewport_size;
	RendererUtilities::get_maximum_shader_varyings =
		&RendererDummy::Utilities::get_maximum_shader_varyings;
	RendererUtilities::get_maximum_uniform_buffer_size =
		&RendererDummy::Utilities::get_maximum_uniform_buffer_size;
}

RendererSceneRender* RasterizerDummy::get_scene() { return scene; }

RendererDummy::Utilities* RasterizerDummy::get_utilities() { return utilities; }
