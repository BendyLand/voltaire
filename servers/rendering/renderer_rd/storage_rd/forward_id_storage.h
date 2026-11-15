/**************************************************************************/
/*  forward_id_storage.h                                                  */
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

#include <cstdint>

class RendererSceneRenderRD;

namespace RendererRD
{

typedef int32_t ForwardID;

enum ForwardIDType
{
	FORWARD_ID_TYPE_OMNI_LIGHT,
	FORWARD_ID_TYPE_SPOT_LIGHT,
	FORWARD_ID_TYPE_REFLECTION_PROBE,
	FORWARD_ID_TYPE_DECAL,
	FORWARD_ID_TYPE_AREA_LIGHT,
	FORWARD_ID_MAX,
};

class ForwardIDStorage final
{
private:
	static ForwardID _default_allocate_forward_id(ForwardIDType p_type) { return -1; }
	static void _default_free_forward_id(ForwardIDType p_type, ForwardID p_id) {}
	static void _default_map_forward_id(
		ForwardIDType p_type, ForwardID p_id, uint32_t p_index, uint64_t p_last_pass)
	{
	}
	static bool _default_uses_forward_ids() { return false; }

public:
	inline static ForwardID (*allocate_forward_id)(ForwardIDType p_type) =
		&_default_allocate_forward_id;
	inline static void (*free_forward_id)(ForwardIDType p_type, ForwardID p_id) =
		&_default_free_forward_id;
	inline static void (*map_forward_id)(ForwardIDType p_type, ForwardID p_id, uint32_t p_index,
		uint64_t p_last_pass) = &_default_map_forward_id;
	inline static bool (*uses_forward_ids)() = &_default_uses_forward_ids;

	template <typename Impl> static void bind()
	{
		allocate_forward_id = &Impl::allocate_forward_id;
		free_forward_id = &Impl::free_forward_id;
		map_forward_id = &Impl::map_forward_id;
		uses_forward_ids = &Impl::uses_forward_ids;
	}

	static void reset()
	{
		allocate_forward_id = &_default_allocate_forward_id;
		free_forward_id = &_default_free_forward_id;
		map_forward_id = &_default_map_forward_id;
		uses_forward_ids = &_default_uses_forward_ids;
	}

	ForwardIDStorage() = delete;
	ForwardIDStorage(const ForwardIDStorage&) = delete;
	ForwardIDStorage& operator=(const ForwardIDStorage&) = delete;
	~ForwardIDStorage() = delete;
};

} // namespace RendererRD


