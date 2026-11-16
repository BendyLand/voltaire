/**************************************************************************/
/*  vulkan_hooks.h                                                        */
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

#include <drivers/vulkan/godot_vulkan.h>
#include "core/math/vector2i.h"
#include "core/templates/local_vector.h"

class VulkanHooks final
{
private:
	static inline bool _is_bound = false;

	static bool _default_create_vulkan_instance(
		const VkInstanceCreateInfo* p_vulkan_create_info, VkInstance* r_instance)
	{
		(void)p_vulkan_create_info;
		(void)r_instance;
		return false;
	}
	static bool _default_get_physical_device(VkPhysicalDevice* r_device)
	{
		(void)r_device;
		return false;
	}
	static bool _default_create_vulkan_device(
		const VkDeviceCreateInfo* p_device_create_info, VkDevice* r_device)
	{
		(void)p_device_create_info;
		(void)r_device;
		return false;
	}
	static void _default_set_direct_queue_family_and_index(
		uint32_t p_queue_family_index, uint32_t p_queue_index)
	{
		(void)p_queue_family_index;
		(void)p_queue_index;
	}
	static bool _default_use_fragment_density_offsets() { return false; }
	static void _default_get_fragment_density_offsets(
		LocalVector<VkOffset2D>& r_offsets, const Vector2i& p_granularity)
	{
		(void)r_offsets;
		(void)p_granularity;
	}
	static bool _default_use_subsampled_images() { return false; }

public:
	static bool is_bound() { return _is_bound; }

	inline static bool (*create_vulkan_instance)(const VkInstanceCreateInfo* p_vulkan_create_info,
		VkInstance* r_instance) = &_default_create_vulkan_instance;
	inline static bool (*get_physical_device)(
		VkPhysicalDevice* r_device) = &_default_get_physical_device;
	inline static bool (*create_vulkan_device)(const VkDeviceCreateInfo* p_device_create_info,
		VkDevice* r_device) = &_default_create_vulkan_device;
	inline static void (*set_direct_queue_family_and_index)(uint32_t p_queue_family_index,
		uint32_t p_queue_index) = &_default_set_direct_queue_family_and_index;
	inline static bool (*use_fragment_density_offsets)() = &_default_use_fragment_density_offsets;
	inline static void (*get_fragment_density_offsets)(LocalVector<VkOffset2D>& r_offsets,
		const Vector2i& p_granularity) = &_default_get_fragment_density_offsets;
	inline static bool (*use_subsampled_images)() = &_default_use_subsampled_images;

	template <typename Impl> static void bind()
	{
		create_vulkan_instance = &Impl::create_vulkan_instance;
		get_physical_device = &Impl::get_physical_device;
		create_vulkan_device = &Impl::create_vulkan_device;
		set_direct_queue_family_and_index = &Impl::set_direct_queue_family_and_index;
		use_fragment_density_offsets = &Impl::use_fragment_density_offsets;
		get_fragment_density_offsets = &Impl::get_fragment_density_offsets;
		use_subsampled_images = &Impl::use_subsampled_images;
		_is_bound = true;
	}

	static void reset();

	VulkanHooks() = delete;
	VulkanHooks(const VulkanHooks&) = delete;
	VulkanHooks& operator=(const VulkanHooks&) = delete;
	~VulkanHooks() = delete;
};


