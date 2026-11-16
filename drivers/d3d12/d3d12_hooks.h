/**************************************************************************/
/*  d3d12_hooks.h                                                         */
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

#include <drivers/d3d12/godot_d3dx12.h>

class D3D12Hooks final
{
private:
	static inline bool _is_bound = false;

	static D3D_FEATURE_LEVEL _default_get_feature_level() { return D3D_FEATURE_LEVEL_11_0; }
	static LUID _default_get_adapter_luid() { return {}; }
	static void _default_set_device(ID3D12Device* p_device) { (void)p_device; }
	static void _default_set_command_queue(ID3D12CommandQueue* p_queue) { (void)p_queue; }
	static void _default_cleanup_device() {}

public:
	static bool is_bound() { return _is_bound; }

	inline static D3D_FEATURE_LEVEL (*get_feature_level)() = &_default_get_feature_level;
	inline static LUID (*get_adapter_luid)() = &_default_get_adapter_luid;
	inline static void (*set_device)(ID3D12Device* p_device) = &_default_set_device;
	inline static void (*set_command_queue)(
		ID3D12CommandQueue* p_queue) = &_default_set_command_queue;
	inline static void (*cleanup_device)() = &_default_cleanup_device;

	template <typename Impl> static void bind()
	{
		get_feature_level = &Impl::get_feature_level;
		get_adapter_luid = &Impl::get_adapter_luid;
		set_device = &Impl::set_device;
		set_command_queue = &Impl::set_command_queue;
		cleanup_device = &Impl::cleanup_device;
		_is_bound = true;
	}

	static void reset();

	D3D12Hooks() = delete;
	D3D12Hooks(const D3D12Hooks&) = delete;
	D3D12Hooks& operator=(const D3D12Hooks&) = delete;
	~D3D12Hooks() = delete;
};
