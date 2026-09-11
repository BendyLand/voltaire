/**************************************************************************/
/*  compositor_storage.cpp                                                */
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

#include "compositor_storage.h"

RendererCompositorStorage* RendererCompositorStorage::singleton = nullptr;

RendererCompositorStorage::RendererCompositorStorage() { singleton = this; }

RendererCompositorStorage::~RendererCompositorStorage() { singleton = nullptr; }

RID RendererCompositorStorage::compositor_effect_allocate()
{
	return compositor_effects_owner.allocate_rid();
}

void RendererCompositorStorage::compositor_effect_initialize(RID p_rid)
{
	compositor_effects_owner.initialize_rid(p_rid, CompositorEffect());
}

bool RendererCompositorStorage::compositor_effect_get_enabled(RID p_effect) const
{
	CompositorEffect* effect = compositor_effects_owner.get_or_null(p_effect);
	ERR_FAIL_NULL_V(effect, false);

	return effect->is_enabled;
}

RSE::CompositorEffectCallbackType RendererCompositorStorage::compositor_effect_get_callback_type(
	RID p_effect) const
{
	CompositorEffect* effect = compositor_effects_owner.get_or_null(p_effect);
	ERR_FAIL_NULL_V(effect, RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_MAX);

	return effect->callback_type;
}

RID RendererCompositorStorage::compositor_allocate() { return compositor_owner.allocate_rid(); }

void RendererCompositorStorage::compositor_initialize(RID p_rid)
{
	compositor_owner.initialize_rid(p_rid, Compositor());
}

void RendererCompositorStorage::compositor_free(RID p_rid) { compositor_owner.free(p_rid); }

void RendererCompositorStorage::compositor_set_compositor_effects(
	RID p_compositor, const Vector<RID>& p_effects)
{
	Compositor* compositor = compositor_owner.get_or_null(p_compositor);
	ERR_FAIL_NULL(compositor);

	compositor->compositor_effects.clear();
	for (const RID& effect : p_effects) {
		if (is_compositor_effect(effect)) {
			compositor->compositor_effects.push_back(effect);
		}
	}
}

Vector<RID> RendererCompositorStorage::compositor_get_compositor_effects(
	RID p_compositor, RSE::CompositorEffectCallbackType p_callback_type, bool p_enabled_only) const
{
	Compositor* compositor = compositor_owner.get_or_null(p_compositor);
	ERR_FAIL_NULL_V(compositor, Vector<RID>());

	if (p_enabled_only || p_callback_type != RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY) {
		Vector<RID> effects;

		for (RID rid : compositor->compositor_effects) {
			if ((!p_enabled_only || compositor_effect_get_enabled(rid)) &&
				(p_callback_type == RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY ||
					compositor_effect_get_callback_type(rid) == p_callback_type)) {
				effects.push_back(rid);
			}
		}

		return effects;
	}
	else {
		return compositor->compositor_effects;
	}
}


