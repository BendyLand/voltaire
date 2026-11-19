/**************************************************************************/
/*  gpu_particles_collision_3d.h                                          */
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

#include "core/templates/local_vector.h"
#include "scene/3d/visual_instance_3d.h"

class Mesh;

class GPUParticlesCollision3D : public Node3D
{
public:
	enum ShapeType
	{
		SHAPE_SPHERE,
		SHAPE_BOX,
		SHAPE_SDF,
		SHAPE_HEIGHT_FIELD,
	};

	enum SDFResolution
	{
		SDF_RESOLUTION_16,
		SDF_RESOLUTION_32,
		SDF_RESOLUTION_64,
		SDF_RESOLUTION_128,
		SDF_RESOLUTION_256,
		SDF_RESOLUTION_512,
		SDF_RESOLUTION_MAX,
	};

	enum HeightFieldResolution
	{
		HEIGHTFIELD_RESOLUTION_256,
		HEIGHTFIELD_RESOLUTION_512,
		HEIGHTFIELD_RESOLUTION_1024,
		HEIGHTFIELD_RESOLUTION_2048,
		HEIGHTFIELD_RESOLUTION_4096,
		HEIGHTFIELD_RESOLUTION_8192,
		HEIGHTFIELD_RESOLUTION_MAX,
	};

	enum UpdateMode
	{
		UPDATE_MODE_WHEN_MOVED,
		UPDATE_MODE_ALWAYS,
	};

	typedef void (*BakeBeginFunc)(int);
	typedef void (*BakeStepFunc)(int, const String&);
	typedef void (*BakeEndFunc)();

	RenderInstance3D render_instance;

private:
	ShapeType shape_type = SHAPE_BOX;
	uint32_t cull_mask = 0xFFFFFFFF;
	RID collision;

	real_t radius = 1.0;
	Vector3 size = Vector3(2, 2, 2);

	// SDF parameters
	SDFResolution sdf_resolution = SDF_RESOLUTION_64;
	uint32_t bake_mask = 0xFFFFFFFF;
	Ref<Texture3D> texture;
	float thickness = 1.0f;

	// HeightField parameters
	uint32_t heightfield_mask = (1 << 20) - 1;
	HeightFieldResolution heightfield_resolution = HEIGHTFIELD_RESOLUTION_1024;
	bool follow_camera_mode = false;
	UpdateMode update_mode = UPDATE_MODE_WHEN_MOVED;

	void _apply_shape_type();

protected:
	_FORCE_INLINE_ RID _get_collision() const { return collision; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	static BakeBeginFunc bake_begin_function;
	static BakeStepFunc bake_step_function;
	static BakeEndFunc bake_end_function;

	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_shape_type(ShapeType p_type);
	ShapeType get_shape_type() const { return shape_type; }

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_size(const Vector3& p_size);
	Vector3 get_size() const { return size; }

	void set_thickness(float p_thickness);
	float get_thickness() const { return thickness; }

	SDFResolution get_sdf_resolution() const { return sdf_resolution; }
	uint32_t get_bake_mask() const { return bake_mask; }
	bool get_bake_mask_value(int p_layer_number) const;

	void set_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_texture() const { return texture; }

	Vector3i get_estimated_cell_size() const;

	HeightFieldResolution get_heightfield_resolution() const { return heightfield_resolution; }

	void set_update_mode(UpdateMode p_update_mode);
	UpdateMode get_update_mode() const { return update_mode; }

	void set_heightfield_mask(uint32_t p_heightfield_mask);
	uint32_t get_heightfield_mask() const { return heightfield_mask; }

	void set_heightfield_mask_value(int p_layer_number, bool p_value);
	bool get_heightfield_mask_value(int p_layer_number) const;

	void set_follow_camera_enabled(bool p_enabled);
	bool is_follow_camera_enabled() const { return follow_camera_mode; }

	AABB get_aabb() const;
	virtual PackedStringArray get_configuration_warnings() const override;

	GPUParticlesCollision3D(ShapeType p_type = SHAPE_BOX);
	~GPUParticlesCollision3D();
};

class GPUParticlesAttractor3D : public Node3D
{
public:
	enum AttractorType
	{
		ATTRACTOR_SPHERE,
		ATTRACTOR_BOX,
		ATTRACTOR_VECTOR_FIELD,
	};

	RenderInstance3D render_instance;

private:
	AttractorType attractor_type = ATTRACTOR_SPHERE;
	uint32_t cull_mask = 0xFFFFFFFF;
	RID collision;
	real_t strength = 1.0;
	real_t attenuation = 1.0;
	real_t directionality = 0.0;

	real_t radius = 1.0;
	Vector3 size = Vector3(2, 2, 2);
	Ref<Texture3D> texture;

	void _apply_attractor_type();

protected:
	_FORCE_INLINE_ RID _get_collision() const { return collision; }
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_attractor_type(AttractorType p_type);
	AttractorType get_attractor_type() const { return attractor_type; }

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_strength(real_t p_strength);
	real_t get_strength() const;

	void set_attenuation(real_t p_attenuation);
	real_t get_attenuation() const;

	real_t get_directionality() const;

	void set_radius(real_t p_radius);
	real_t get_radius() const { return radius; }

	void set_size(const Vector3& p_size);
	Vector3 get_size() const { return size; }

	void set_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_texture() const { return texture; }

	AABB get_aabb() const;

	GPUParticlesAttractor3D(AttractorType p_type = ATTRACTOR_SPHERE);
	~GPUParticlesAttractor3D();
};
