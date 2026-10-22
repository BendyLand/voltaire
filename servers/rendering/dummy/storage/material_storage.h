/**************************************************************************/
/*  material_storage.h                                                    */
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

#pragma once

#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/storage/material_storage.h"

class DependencyTracker;

namespace RendererDummy
{

class MaterialStorage final
{
private:
	static inline HashMap<StringName, RSE::GlobalShaderParameterType> global_shader_variables;

	struct DummyShader
	{
		HashMap<StringName, ShaderLanguage::ShaderNode::Uniform> uniforms;
	};

	static inline RID_Owner<DummyShader> shader_owner;

	static inline ShaderCompiler* dummy_compiler = nullptr;
	static inline HashSet<RID> dummy_embedded_set;

	struct DummyMaterial
	{
		RID shader;
		RID next_pass;
	};

	static inline RID_Owner<DummyMaterial> material_owner;

public:
	MaterialStorage() = delete;
	MaterialStorage(const MaterialStorage&) = delete;
	MaterialStorage& operator=(const MaterialStorage&) = delete;
	~MaterialStorage() = delete;

	static void initialize();
	static void finalize();

	/* GLOBAL SHADER UNIFORM API */
	static void global_shader_parameter_remove(const StringName& p_name);
	static Vector<StringName> global_shader_parameter_get_list();

	static RSE::GlobalShaderParameterType global_shader_parameter_get_type(
		const StringName& p_name);

	static void global_shader_parameters_load_settings(bool p_load_textures = true);

	static void global_shader_parameters_clear() {}

	static int32_t global_shader_parameters_instance_allocate(RID p_instance)
	{
		return 0;
	}

	static void global_shader_parameters_instance_free(RID p_instance) {}

	/* SHADER API */
	static bool owns_shader(RID p_rid) { return shader_owner.owns(p_rid); }

	static RID shader_allocate();
	static void shader_initialize(RID p_rid, bool p_embedded = true);
	static void shader_free(RID p_rid);

	static void shader_set_code(RID p_shader, const String& p_code);

	static void shader_set_path_hint(RID p_shader, const String& p_code) {}

	static String shader_get_code(RID p_shader) { return ""; }

	static void shader_set_default_texture_parameter(
		RID p_shader, const StringName& p_name, RID p_texture, int p_index)
	{
	}

	static RID shader_get_default_texture_parameter(
		RID p_shader, const StringName& p_name, int p_index)
	{
		return RID();
	}

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(
		RID p_shader)
	{
		return RenderingServerTypes::ShaderNativeSourceCode();
	}

	static void shader_embedded_set_lock() {}

	static const HashSet<RID>& shader_embedded_set_get()
	{
		return dummy_embedded_set;
	}

	static void shader_embedded_set_unlock() {}

	/* MATERIAL API */
	static bool owns_material(RID p_rid) { return material_owner.owns(p_rid); }

	static RID material_allocate();
	static void material_initialize(RID p_rid);
	static void material_free(RID p_rid);

	static void material_set_render_priority(RID p_material, int priority) {}

	static void material_set_shader(RID p_shader_material, RID p_shader);

	static void material_set_next_pass(RID p_material, RID p_next_material);

	static bool material_is_animated(RID p_material) { return false; }

	static bool material_casts_shadows(RID p_material) { return false; }

	static RSE::CullMode material_get_cull_mode(RID p_material)
	{
		return RSE::CULL_MODE_DISABLED;
	}

	static void material_get_instance_shader_parameters(
		RID p_material, List<InstanceShaderParam>* r_parameters);

	static void material_update_dependency(RID p_material, DependencyTracker* p_instance)
	{
	}
};

} // namespace RendererDummy
