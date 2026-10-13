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

#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/utilities.h"

class RendererMaterialStorage
{
public:
	struct InstanceShaderParam
	{
		int index;
	};

	struct API
	{
		/* GLOBAL SHADER UNIFORM API */
		void (*global_shader_parameter_remove)(const StringName& p_name) = nullptr;
		Vector<StringName> (*global_shader_parameter_get_list)() = nullptr;
		RSE::GlobalShaderParameterType (*global_shader_parameter_get_type)(const StringName& p_name) = nullptr;
		void (*global_shader_parameters_load_settings)(bool p_load_textures) = nullptr;
		void (*global_shader_parameters_clear)() = nullptr;
		int32_t (*global_shader_parameters_instance_allocate)(RID p_instance) = nullptr;
		void (*global_shader_parameters_instance_free)(RID p_instance) = nullptr;
		void (*finalize)() = nullptr;

		/* SHADER API */
		RID (*shader_allocate)() = nullptr;
		void (*shader_initialize)(RID p_rid, bool p_embedded) = nullptr;
		void (*shader_free)(RID p_rid) = nullptr;
		void (*shader_set_code)(RID p_shader, const String& p_code) = nullptr;
		void (*shader_set_path_hint)(RID p_shader, const String& p_path) = nullptr;
		String (*shader_get_code)(RID p_shader) = nullptr;
		void (*shader_set_default_texture_parameter)(RID p_shader, const StringName& p_name, RID p_texture, int p_index) = nullptr;
		RID (*shader_get_default_texture_parameter)(RID p_shader, const StringName& p_name, int p_index) = nullptr;
		RenderingServerTypes::ShaderNativeSourceCode (*shader_get_native_source_code)(RID p_shader) = nullptr;
		void (*shader_embedded_set_lock)() = nullptr;
		const HashSet<RID>& (*shader_embedded_set_get)() = nullptr;
		void (*shader_embedded_set_unlock)() = nullptr;

		/* MATERIAL API */
		RID (*material_allocate)() = nullptr;
		void (*material_initialize)(RID p_rid) = nullptr;
		void (*material_free)(RID p_rid) = nullptr;
		void (*material_set_render_priority)(RID p_material, int priority) = nullptr;
		void (*material_set_shader)(RID p_shader_material, RID p_shader) = nullptr;
		void (*material_set_next_pass)(RID p_material, RID p_next_material) = nullptr;
		bool (*material_is_animated)(RID p_material) = nullptr;
		bool (*material_casts_shadows)(RID p_material) = nullptr;
		RSE::CullMode (*material_get_cull_mode)(RID p_material) = nullptr;
		void (*material_get_instance_shader_parameters)(RID p_material, List<InstanceShaderParam>* r_parameters) = nullptr;
		void (*material_update_dependency)(RID p_material, DependencyTracker* p_instance) = nullptr;
	};

private:
	API api;

public:
	RendererMaterialStorage() = default;
	~RendererMaterialStorage() = default;

	template <typename StorageImpl>
	void bind()
	{
		/* GLOBAL SHADER UNIFORM API */
		api.global_shader_parameter_remove = &StorageImpl::global_shader_parameter_remove;
		api.global_shader_parameter_get_list = &StorageImpl::global_shader_parameter_get_list;
		api.global_shader_parameter_get_type = &StorageImpl::global_shader_parameter_get_type;
		api.global_shader_parameters_load_settings = &StorageImpl::global_shader_parameters_load_settings;
		api.global_shader_parameters_clear = &StorageImpl::global_shader_parameters_clear;
		api.global_shader_parameters_instance_allocate = &StorageImpl::global_shader_parameters_instance_allocate;
		api.global_shader_parameters_instance_free = &StorageImpl::global_shader_parameters_instance_free;

		/* SHADER API */
		api.shader_allocate = &StorageImpl::shader_allocate;
		api.shader_initialize = &StorageImpl::shader_initialize;
		api.shader_free = &StorageImpl::shader_free;
		api.shader_set_code = &StorageImpl::shader_set_code;
		api.shader_set_path_hint = &StorageImpl::shader_set_path_hint;
		api.shader_get_code = &StorageImpl::shader_get_code;
		api.shader_set_default_texture_parameter = &StorageImpl::shader_set_default_texture_parameter;
		api.shader_get_default_texture_parameter = &StorageImpl::shader_get_default_texture_parameter;
		api.shader_get_native_source_code = &StorageImpl::shader_get_native_source_code;
		api.shader_embedded_set_lock = &StorageImpl::shader_embedded_set_lock;
		api.shader_embedded_set_get = &StorageImpl::shader_embedded_set_get;
		api.shader_embedded_set_unlock = &StorageImpl::shader_embedded_set_unlock;

		/* MATERIAL API */
		api.material_allocate = &StorageImpl::material_allocate;
		api.material_initialize = &StorageImpl::material_initialize;
		api.material_free = &StorageImpl::material_free;
		api.material_set_render_priority = &StorageImpl::material_set_render_priority;
		api.material_set_shader = &StorageImpl::material_set_shader;
		api.material_set_next_pass = &StorageImpl::material_set_next_pass;
		api.material_is_animated = &StorageImpl::material_is_animated;
		api.material_casts_shadows = &StorageImpl::material_casts_shadows;
		api.material_get_cull_mode = &StorageImpl::material_get_cull_mode;
		api.material_get_instance_shader_parameters = &StorageImpl::material_get_instance_shader_parameters;
		api.material_update_dependency = &StorageImpl::material_update_dependency;
	}

	/* GLOBAL SHADER UNIFORM API */
	_FORCE_INLINE_ void global_shader_parameter_remove(const StringName& p_name)
	{
		api.global_shader_parameter_remove(p_name);
	}

	_FORCE_INLINE_ Vector<StringName> global_shader_parameter_get_list() const
	{
		return api.global_shader_parameter_get_list();
	}

	_FORCE_INLINE_ RSE::GlobalShaderParameterType global_shader_parameter_get_type(
		const StringName& p_name) const
	{
		return api.global_shader_parameter_get_type(p_name);
	}

	_FORCE_INLINE_ void global_shader_parameters_load_settings(bool p_load_textures = true)
	{
		api.global_shader_parameters_load_settings(p_load_textures);
	}

	_FORCE_INLINE_ void global_shader_parameters_clear()
	{
		api.global_shader_parameters_clear();
	}

	_FORCE_INLINE_ int32_t global_shader_parameters_instance_allocate(RID p_instance)
	{
		return api.global_shader_parameters_instance_allocate(p_instance);
	}

	_FORCE_INLINE_ void global_shader_parameters_instance_free(RID p_instance)
	{
		api.global_shader_parameters_instance_free(p_instance);
	}

	/* SHADER API */
	_FORCE_INLINE_ RID shader_allocate()
	{
		return api.shader_allocate();
	}

	_FORCE_INLINE_ void shader_initialize(RID p_rid, bool p_embedded = true)
	{
		api.shader_initialize(p_rid, p_embedded);
	}

	_FORCE_INLINE_ void shader_free(RID p_rid)
	{
		api.shader_free(p_rid);
	}

	_FORCE_INLINE_ void shader_set_code(RID p_shader, const String& p_code)
	{
		api.shader_set_code(p_shader, p_code);
	}

	_FORCE_INLINE_ void shader_set_path_hint(RID p_shader, const String& p_path)
	{
		api.shader_set_path_hint(p_shader, p_path);
	}

	_FORCE_INLINE_ String shader_get_code(RID p_shader) const
	{
		return api.shader_get_code(p_shader);
	}

	_FORCE_INLINE_ void shader_set_default_texture_parameter(
		RID p_shader, const StringName& p_name, RID p_texture, int p_index)
	{
		api.shader_set_default_texture_parameter(p_shader, p_name, p_texture, p_index);
	}

	_FORCE_INLINE_ RID shader_get_default_texture_parameter(
		RID p_shader, const StringName& p_name, int p_index) const
	{
		return api.shader_get_default_texture_parameter(p_shader, p_name, p_index);
	}

	_FORCE_INLINE_ RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(
		RID p_shader) const
	{
		return api.shader_get_native_source_code(p_shader);
	}

	_FORCE_INLINE_ void shader_embedded_set_lock()
	{
		api.shader_embedded_set_lock();
	}

	_FORCE_INLINE_ const HashSet<RID>& shader_embedded_set_get() const
	{
		return api.shader_embedded_set_get();
	}

	_FORCE_INLINE_ void shader_embedded_set_unlock()
	{
		api.shader_embedded_set_unlock();
	}

	/* MATERIAL API */
	_FORCE_INLINE_ RID material_allocate()
	{
		return api.material_allocate();
	}

	_FORCE_INLINE_ void material_initialize(RID p_rid)
	{
		api.material_initialize(p_rid);
	}

	_FORCE_INLINE_ void material_free(RID p_rid)
	{
		api.material_free(p_rid);
	}

	_FORCE_INLINE_ void material_set_render_priority(RID p_material, int priority)
	{
		api.material_set_render_priority(p_material, priority);
	}

	_FORCE_INLINE_ void material_set_shader(RID p_shader_material, RID p_shader)
	{
		api.material_set_shader(p_shader_material, p_shader);
	}

	_FORCE_INLINE_ void material_set_next_pass(RID p_material, RID p_next_material)
	{
		api.material_set_next_pass(p_material, p_next_material);
	}

	_FORCE_INLINE_ bool material_is_animated(RID p_material)
	{
		return api.material_is_animated(p_material);
	}

	_FORCE_INLINE_ bool material_casts_shadows(RID p_material)
	{
		return api.material_casts_shadows(p_material);
	}

	_FORCE_INLINE_ RSE::CullMode material_get_cull_mode(RID p_material) const
	{
		return api.material_get_cull_mode(p_material);
	}

	_FORCE_INLINE_ void material_get_instance_shader_parameters(
		RID p_material, List<InstanceShaderParam>* r_parameters)
	{
		api.material_get_instance_shader_parameters(p_material, r_parameters);
	}

	_FORCE_INLINE_ void material_update_dependency(RID p_material, DependencyTracker* p_instance)
	{
		api.material_update_dependency(p_material, p_instance);
	}

	_FORCE_INLINE_ void finalize()
    {
        if (api.finalize) {
            api.finalize();
        }
    }
};
