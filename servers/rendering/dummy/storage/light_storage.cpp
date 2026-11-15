/**************************************************************************/
/*  light_storage.cpp                                                     */
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

#include "servers/rendering/dummy/storage/light_storage.h"

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

/* LIGHTMAP API */

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

/* LIGHTMAP INSTANCE */

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


