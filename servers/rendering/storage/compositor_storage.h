/**************************************************************************/
/*  compositor_storage.h                                                  */
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

#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererCompositorStorage final
{
public:
	RendererCompositorStorage() = delete;
	~RendererCompositorStorage() = delete;
	RendererCompositorStorage(const RendererCompositorStorage&) = delete;
	RendererCompositorStorage& operator=(const RendererCompositorStorage&) = delete;

private:
	static inline bool _initialized = false;
	static inline int num_compositor_effects_with_motion_vectors = 0;

	// Compositor effect
	struct CompositorEffect
	{
		bool is_enabled = true;
		RSE::CompositorEffectCallbackType callback_type;

		uint32_t flags = {};
	};

	static inline RID_Owner<CompositorEffect, true> compositor_effects_owner;

	// Compositor
	struct Compositor
	{
		// Compositor effects
		Vector<RID> compositor_effects;
	};

	static inline RID_Owner<Compositor, true> compositor_owner;

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	static int get_num_compositor_effects_with_motion_vectors()
	{
		return num_compositor_effects_with_motion_vectors;
	}

	// Compositor effect
	static RID compositor_effect_allocate();
	static void compositor_effect_initialize(RID p_rid);
	static void compositor_effect_free(RID p_rid);

	static bool is_compositor_effect(RID p_effect)
	{
		return compositor_effects_owner.owns(p_effect);
	}

	static void compositor_effect_set_enabled(RID p_effect, bool p_enabled);
	static bool compositor_effect_get_enabled(RID p_effect);

	static RSE::CompositorEffectCallbackType compositor_effect_get_callback_type(RID p_effect);

	static void compositor_effect_set_flag(
		RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set);
	static bool compositor_effect_get_flag(RID p_effect, RSE::CompositorEffectFlags p_flag);

	// Compositor
	static RID compositor_allocate();
	static void compositor_initialize(RID p_rid);
	static void compositor_free(RID p_rid);

	static bool is_compositor(RID p_compositor) { return compositor_owner.owns(p_compositor); }

	static void compositor_set_compositor_effects(RID p_compositor, const Vector<RID>& p_effects);
	static Vector<RID> compositor_get_compositor_effects(RID p_compositor,
		RSE::CompositorEffectCallbackType p_callback_type =
			RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY,
		bool p_enabled_only = true);
};
