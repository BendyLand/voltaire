/**************************************************************************/
/*  renderer_viewport.h                                                   */
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
#include "servers/display/display_server_enums.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/render_scene_buffers.h"

#pragma once

#include "core/templates/rid_owner.h"
#include "servers/display/display_server_enums.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/render_scene_buffers.h"

class RendererViewport final
{
public:
	struct CanvasBase
	{
	};

	struct Viewport
	{
		RID self;
		RID parent;

		bool use_xr = false;

		Size2i internal_size;
		Size2i size;
		uint32_t view_count = 1;
		RID camera;
		RID scenario;

		RSE::ViewportScaling3DMode scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_BILINEAR;
		float scaling_3d_scale = 1.0f;
		float fsr_sharpness = 0.2f;
		float texture_mipmap_bias = 0.0f;
		RSE::ViewportAnisotropicFiltering anisotropic_filtering_level = RSE::VIEWPORT_ANISOTROPY_4X;
		bool fsr_enabled = false;
		uint32_t jitter_phase_count = 0;
		RSE::ViewportUpdateMode update_mode = RSE::VIEWPORT_UPDATE_WHEN_VISIBLE;
		RID render_target;
		RID render_target_texture;
		Ref<RenderSceneBuffers> render_buffers;

		RSE::ViewportMSAA msaa_2d = RSE::VIEWPORT_MSAA_DISABLED;
		RSE::ViewportMSAA msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
		RSE::ViewportScreenSpaceAA screen_space_aa = RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED;
		bool use_taa = false;
		bool use_debanding = false;
		bool force_motion_vectors = false;

		RendererSceneRender::CameraData prev_camera_data;
		uint64_t prev_camera_data_frame = 0;

		bool use_occlusion_culling = false;
		bool occlusion_buffer_dirty = true;

		DisplayServerEnums::WindowID viewport_to_screen = DisplayServerEnums::INVALID_WINDOW_ID;
		Rect2 viewport_to_screen_rect;
		bool viewport_render_direct_to_screen = false;

		bool disable_2d = false;
		RSE::ViewportEnvironmentMode disable_environment = RSE::VIEWPORT_ENVIRONMENT_INHERIT;
		bool disable_3d = false;
		bool measure_render_time = false;

		bool snap_2d_transforms_to_pixel = false;
		bool snap_2d_vertices_to_pixel = false;

		uint64_t time_cpu_begin = 0;
		uint64_t time_cpu_end = 0;

		uint64_t time_gpu_begin = 0;
		uint64_t time_gpu_end = 0;

		RID shadow_atlas;
		int shadow_atlas_size = 0;
		bool shadow_atlas_16_bits = true;

		bool sdf_active = false;

		float mesh_lod_threshold = 1.0f;

		uint64_t last_pass = 0;

		RSE::ViewportDebugDraw debug_draw = RSE::VIEWPORT_DEBUG_DRAW_DISABLED;
		RSE::ViewportClearMode clear_mode = RSE::VIEWPORT_CLEAR_ALWAYS;

		RSE::CanvasItemTextureFilter texture_filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR;
		RSE::CanvasItemTextureRepeat texture_repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED;

		bool transparent_bg = false;
		bool use_hdr_2d = false;
		float window_output_max_value = 1.0f;

		uint32_t canvas_cull_mask = 0xffffffff;

		struct CanvasKey
		{
			int64_t stacking = 0;
			RID canvas;

			bool operator<(const CanvasKey& p_canvas) const
			{
				if (stacking == p_canvas.stacking) {
					return canvas < p_canvas.canvas;
				}
				return stacking < p_canvas.stacking;
			}

			CanvasKey() = default;

			CanvasKey(const RID& p_canvas, int p_layer, int p_sublayer)
			{
				canvas = p_canvas;
				int64_t sign = p_layer < 0 ? -1 : 1;
				stacking = sign * (((int64_t)Math::abs(p_layer)) << 32) + p_sublayer;
			}

			int get_layer() const { return stacking >> 32; }
		};


		struct CanvasData
		{
			CanvasBase* canvas = nullptr;
			Transform2D transform;
			int layer = 0;
			int sublayer = 0;
		};

		Transform2D global_transform;
		HashMap<RID, CanvasData> canvas_map;
		RenderingServerTypes::RenderInfo render_info;

		Viewport() = default;
	};

	struct ViewportSort
	{
		_FORCE_INLINE_ bool operator()(const RendererViewport::Viewport* p_left,
			const RendererViewport::Viewport* p_right) const
		{
			bool left_is_parent_of_right = p_right->parent == p_left->self;
			if (left_is_parent_of_right) {
				return true;
			}
			bool right_is_parent_of_left = p_left->parent == p_right->self;
			if (right_is_parent_of_left) {
				return false;
			}
			return p_left->parent < p_right->parent;
		}
	};

private:
	HashMap<String, RID> timestamp_vp_map;
	uint64_t draw_viewports_pass = 0;

	mutable RID_Owner<Viewport, true> viewport_owner;

	Vector<Viewport*> active_viewports;
	Vector<Viewport*> sorted_active_viewports;
	bool sorted_active_viewports_dirty = false;

	int total_objects_drawn = 0;
	int total_vertices_drawn = 0;
	int total_draw_calls_used = 0;
	int num_viewports_with_motion_vectors = 0;
	int occlusion_rays_per_thread = 512;

	Vector<Viewport*> _sort_active_viewports();
	void _viewport_set_size(Viewport* p_viewport, int p_width, int p_height, uint32_t p_view_count);
	bool _viewport_requires_motion_vectors(Viewport* p_viewport);
	void _viewport_set_force_motion_vectors(Viewport* p_viewport, bool p_force_motion_vectors);
	void _configure_3d_render_buffers(Viewport* p_viewport);
	void _draw_3d(Viewport* p_viewport);
	void _draw_viewport(Viewport* p_viewport);
	DisplayServerEnums::WindowID _get_containing_window(Viewport* p_viewport);
	void _resize_occlusion_culling_buffer(const Size2i& p_size);

public:
	RendererViewport() = default;
	~RendererViewport() = default;

	// Prevent accidental copying; this coordinator owns unique resource tables
	RendererViewport(const RendererViewport&) = delete;
	RendererViewport& operator=(const RendererViewport&) = delete;

	RendererViewport(RendererViewport&&) noexcept = default;
	RendererViewport& operator=(RendererViewport&&) noexcept = default;

	RID viewport_allocate();
	void viewport_initialize(RID p_rid);

#ifndef XR_DISABLED
	void viewport_set_use_xr(RID p_viewport, bool p_use_xr);
#endif

	void viewport_set_size(RID p_viewport, int p_width, int p_height, int p_view_count = 1);
	void viewport_attach_to_screen(RID p_viewport, const Rect2& p_rect = Rect2(),
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable);

	void viewport_set_active(RID p_viewport, bool p_active);
	void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport);

	void viewport_set_scaling_3d_mode(RID p_viewport, RSE::ViewportScaling3DMode p_mode);
	void viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale);
	void viewport_set_fsr_sharpness(RID p_viewport, float p_sharpness);
	void viewport_set_texture_mipmap_bias(RID p_viewport, float p_mipmap_bias);
	void viewport_set_anisotropic_filtering_level(
		RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level);

	void viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode);
	RSE::ViewportUpdateMode viewport_get_update_mode(RID p_viewport) const;
	void viewport_set_vflip(RID p_viewport, bool p_enable);

	void viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode);

	RID viewport_get_render_target(RID p_viewport) const;
	RID viewport_get_texture(RID p_viewport) const;
	RID viewport_get_occluder_debug_texture(RID p_viewport) const;

	void viewport_set_prev_camera_data(
		RID p_viewport, const RendererSceneRender::CameraData* p_camera_data);
	const RendererSceneRender::CameraData* viewport_get_prev_camera_data(RID p_viewport);

	void viewport_set_disable_2d(RID p_viewport, bool p_disable);
	void viewport_set_environment_mode(RID p_viewport, RSE::ViewportEnvironmentMode p_mode);
	void viewport_set_disable_3d(RID p_viewport, bool p_disable);

	bool viewport_is_environment_disabled(Viewport* viewport);

	void viewport_attach_camera(RID p_viewport, RID p_camera);
	void viewport_set_scenario(RID p_viewport, RID p_scenario);
	void viewport_attach_canvas(RID p_viewport, RID p_canvas);
	void viewport_remove_canvas(RID p_viewport, RID p_canvas);
	void viewport_set_canvas_transform(RID p_viewport, RID p_canvas, const Transform2D& p_offset);
	void viewport_set_transparent_background(RID p_viewport, bool p_enabled);
	void viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr_2d);

	bool viewport_is_using_hdr_2d(RID p_viewport) const;

	void viewport_set_global_canvas_transform(RID p_viewport, const Transform2D& p_transform);
	void viewport_set_canvas_stacking(RID p_viewport, RID p_canvas, int p_layer, int p_sublayer);

	void viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask);

	void viewport_set_positional_shadow_atlas_size(
		RID p_viewport, int p_size, bool p_16_bits = true);
	void viewport_set_positional_shadow_atlas_quadrant_subdivision(
		RID p_viewport, int p_quadrant, int p_subdiv);

	void viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa);
	void viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa);
	void viewport_set_screen_space_aa(RID p_viewport, RSE::ViewportScreenSpaceAA p_mode);
	void viewport_set_use_taa(RID p_viewport, bool p_use_taa);
	void viewport_set_use_debanding(RID p_viewport, bool p_use_debanding);
	void viewport_set_force_motion_vectors(RID p_viewport, bool p_force_motion_vectors);
	void viewport_set_use_occlusion_culling(RID p_viewport, bool p_use_occlusion_culling);
	void viewport_set_occlusion_rays_per_thread(int p_rays_per_thread);
	void viewport_set_occlusion_culling_build_quality(
		RSE::ViewportOcclusionCullingBuildQuality p_quality);
	void viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels);

	int viewport_get_render_info(
		RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info);
	void viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw);

	void viewport_set_measure_render_time(RID p_viewport, bool p_enable);
	float viewport_get_measured_render_time_cpu(RID p_viewport) const;
	float viewport_get_measured_render_time_gpu(RID p_viewport) const;

	void viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled);

	void viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled);

	void viewport_set_default_canvas_item_texture_filter(
		RID p_viewport, RSE::CanvasItemTextureFilter p_filter);
	void viewport_set_default_canvas_item_texture_repeat(
		RID p_viewport, RSE::CanvasItemTextureRepeat p_repeat);

	void viewport_set_sdf_oversize_and_scale(
		RID p_viewport, RSE::ViewportSDFOversize p_over_size, RSE::ViewportSDFScale p_scale);

	RID viewport_find_from_screen_attachment(
		DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID) const;

	void viewport_set_vrs_mode(RID p_viewport, RSE::ViewportVRSMode p_mode);
	void viewport_set_vrs_update_mode(RID p_viewport, RSE::ViewportVRSUpdateMode p_mode);
	void viewport_set_vrs_texture(RID p_viewport, RID p_texture);

	void handle_timestamp(String p_timestamp, uint64_t p_cpu_time, uint64_t p_gpu_time);

	void draw_viewports(bool p_swap_buffers);

	bool free(RID p_rid);

	int get_total_objects_drawn() const;
	int get_total_primitives_drawn() const;
	int get_total_draw_calls_used() const;
	int get_num_viewports_with_motion_vectors() const;

	void call_set_vsync_mode(
		DisplayServerEnums::VSyncMode p_mode, DisplayServerEnums::WindowID p_window);
};

// --- Folded from servers/rendering/renderer_canvas_cull.h ---
#include "core/templates/paged_allocator.h"
#include "servers/rendering/renderer_scene_cull.h"
#include "servers/rendering/renderer_canvas_render.h"

#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"

class RendererCanvasCull
{
	static void _dependency_changed(
		Dependency::DependencyChangedNotification p_notification, DependencyTracker* p_tracker);
	static void _dependency_deleted(const RID& p_dependency, DependencyTracker* p_tracker);

public:
	struct Item : public RCR::Item
	{
		RID parent; // canvas it belongs to
		RID self;
		List<Item*>::Element* E;
		int z_index;
		bool z_relative;
		bool sort_y;
		Color modulate;
		Color self_modulate;
		bool use_parent_material;
		int index;
		bool children_order_dirty;
		int ysort_children_count;
		Color ysort_modulate;
		Transform2D ysort_xform; // Relative to y-sorted subtree's root item (identity for such
								 // root). Its `origin.y` is used for sorting.
		int ysort_index;
		int ysort_parent_abs_z_index; // Absolute Z index of parent. Only populated and used when
									  // y-sorting.
		uint32_t visibility_layer = 0xffffffff;

		Vector<Item*> child_items;

		struct VisibilityNotifierData
		{
			Rect2 area;
			bool just_visible = false;
			uint64_t visible_in_frame = 0;
			SelfList<VisibilityNotifierData> visible_element;

			VisibilityNotifierData() : visible_element(this) {}
		};

		VisibilityNotifierData* visibility_notifier = nullptr;

		DependencyTracker dependency_tracker;
		InstanceUniforms instance_uniforms;
		SelfList<Item> update_item;

		bool update_dependencies = false;

		Item() : update_item(this)
		{
			children_order_dirty = true;
			E = nullptr;
			z_index = 0;
			modulate = Color(1, 1, 1, 1);
			self_modulate = Color(1, 1, 1, 1);
			sort_y = false;
			use_parent_material = false;
			z_relative = true;
			index = 0;
			ysort_children_count = -1;
			ysort_xform = Transform2D();
			ysort_index = 0;
			ysort_parent_abs_z_index = 0;

			dependency_tracker.userdata = this;
			dependency_tracker.changed_callback = &RendererCanvasCull::_dependency_changed;
			dependency_tracker.deleted_callback = &RendererCanvasCull::_dependency_deleted;
		}
	};

	void _item_queue_update(Item* p_item, bool p_update_dependencies);
	SelfList<Item>::List _item_update_list;

	struct ItemIndexSort
	{
		_FORCE_INLINE_ bool operator()(const Item* p_left, const Item* p_right) const
		{
			return p_left->index < p_right->index;
		}
	};

	struct ItemYSort
	{
		_FORCE_INLINE_ bool operator()(const Item* p_left, const Item* p_right) const
		{
			const real_t left_y = p_left->ysort_xform.columns[2].y;
			const real_t right_y = p_right->ysort_xform.columns[2].y;
			if (Math::is_equal_approx(left_y, right_y)) {
				return p_left->ysort_index < p_right->ysort_index;
			}

			return left_y < right_y;
		}
	};

	struct LightOccluderPolygon
	{
		bool active;
		Rect2 aabb;
		RSE::CanvasOccluderPolygonCullMode cull_mode;
		RID occluder;
		HashSet<RendererCanvasRender::LightOccluderInstance*> owners;

		LightOccluderPolygon()
		{
			active = false;
			cull_mode = RSE::CANVAS_OCCLUDER_POLYGON_CULL_DISABLED;
		}
	};

	RID_Owner<LightOccluderPolygon, true> canvas_light_occluder_polygon_owner;

	RID_Owner<RendererCanvasRender::LightOccluderInstance, true> canvas_light_occluder_owner;

	struct Canvas : public RendererViewport::CanvasBase
	{
		HashSet<RID> viewports;

		struct ChildItem
		{
			Item* item = nullptr;

			bool operator<(const ChildItem& p_item) const
			{
				return item->index < p_item.item->index;
			}
		};

		HashSet<RendererCanvasRender::Light*> lights;
		HashSet<RendererCanvasRender::Light*> directional_lights;

		HashSet<RendererCanvasRender::LightOccluderInstance*> occluders;

		bool children_order_dirty;
		Vector<ChildItem> child_items;
		Color modulate;
		RID parent;
		float parent_scale;

		int find_item(Item* p_item)
		{
			for (int i = 0; i < child_items.size(); i++) {
				if (child_items[i].item == p_item) {
					return i;
				}
			}
			return -1;
		}

		void erase_item(Item* p_item)
		{
			int idx = find_item(p_item);
			if (idx >= 0) {
				child_items.remove_at(idx);
			}
		}

		Canvas()
		{
			modulate = Color(1, 1, 1, 1);
			children_order_dirty = true;
			parent_scale = 1.0;
		}
	};

	mutable RID_Owner<Canvas, true> canvas_owner;
	RID_Owner<Item, true> canvas_item_owner{65536, 4194304};
	RID_Owner<RendererCanvasRender::Light, true> canvas_light_owner;

	bool disable_scale;
	bool sdf_used = false;
	bool snapping_2d_transforms_to_pixel = false;

	bool debug_redraw = false;
	double debug_redraw_time = 0;
	Color debug_redraw_color;

	PagedAllocator<Item::VisibilityNotifierData> visibility_notifier_allocator;
	SelfList<Item::VisibilityNotifierData>::List visibility_notifier_list;

	_FORCE_INLINE_ void _attach_canvas_item_for_draw(Item* ci, Item* p_canvas_clip,
		RCR::Item** r_z_list, RCR::Item** r_z_last_list,
		const Transform2D& p_transform, const Rect2& p_clip_rect, Rect2 p_global_rect,
		const Color& modulate, int p_z, RendererCanvasCull::Item* p_material_owner,
		bool p_use_canvas_group, RCR::Item* r_canvas_group_from);

private:
	void _render_canvas_item_tree(RID p_to_render_target, Canvas::ChildItem* p_child_items,
		int p_child_item_count, const Transform2D& p_transform, const Rect2& p_clip_rect,
		const Color& p_modulate, RendererCanvasRender::Light* p_lights,
		RendererCanvasRender::Light* p_directional_lights,
		RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
		uint32_t p_canvas_cull_mask, RenderingServerTypes::RenderInfo* r_render_info = nullptr);
	void _cull_canvas_item(Item* p_canvas_item, const Transform2D& p_parent_xform,
		const Rect2& p_clip_rect, const Color& p_modulate, int p_z,
		RCR::Item** r_z_list, RCR::Item** r_z_last_list,
		Item* p_canvas_clip, Item* p_material_owner, bool p_is_already_y_sorted,
		uint32_t p_canvas_cull_mask, const Point2& p_repeat_size, int p_repeat_times,
		RCR::Item* p_repeat_source_item);

	void _collect_ysort_children(RendererCanvasCull::Item* p_canvas_item,
		RendererCanvasCull::Item* p_material_owner, const Color& p_modulate,
		RendererCanvasCull::Item** r_items, int& r_index, int& r_ysort_children_count, int p_z,
		uint32_t p_canvas_cull_mask);
	int _count_ysort_children(RendererCanvasCull::Item* p_canvas_item);
	void _mark_ysort_dirty(RendererCanvasCull::Item* ysort_owner);

	static constexpr int z_range = RSE::CANVAS_ITEM_Z_MAX - RSE::CANVAS_ITEM_Z_MIN + 1;

	RCR::Item** z_list = nullptr;
	RCR::Item** z_last_list = nullptr;

	Transform2D _current_camera_transform;

public:
	void render_canvas(RID p_render_target, Canvas* p_canvas, const Transform2D& p_transform,
		RendererCanvasRender::Light* p_lights, RendererCanvasRender::Light* p_directional_lights,
		const Rect2& p_clip_rect, RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_transforms_to_pixel,
		bool p_snap_2d_vertices_to_pixel, uint32_t p_canvas_cull_mask,
		RenderingServerTypes::RenderInfo* r_render_info = nullptr);

	bool was_sdf_used();

	RID canvas_allocate();
	void canvas_initialize(RID p_rid);

	void canvas_set_item_mirroring(RID p_canvas, RID p_item, const Point2& p_mirroring);
	void canvas_set_item_repeat(RID p_item, const Point2& p_repeat_size, int p_repeat_times);
	void canvas_set_modulate(RID p_canvas, const Color& p_color);
	void canvas_set_parent(RID p_canvas, RID p_parent, float p_scale);
	void canvas_set_disable_scale(bool p_disable);

	RID canvas_item_allocate();
	void canvas_item_initialize(RID p_rid);

	void canvas_item_set_parent(RID p_item, RID p_parent);

	void canvas_item_set_visible(RID p_item, bool p_visible);
	void canvas_item_set_light_mask(RID p_item, int p_mask);

	void canvas_item_set_visibility_layer(RID p_item, uint32_t p_layer);
	uint32_t canvas_item_get_visibility_layer(RID p_item);

	void canvas_item_set_transform(RID p_item, const Transform2D& p_transform);
	void canvas_item_set_clip(RID p_item, bool p_clip);
	void canvas_item_set_distance_field_mode(RID p_item, bool p_enable);
	void canvas_item_set_custom_rect(RID p_item, bool p_custom_rect, const Rect2& p_rect = Rect2());
	void canvas_item_set_modulate(RID p_item, const Color& p_color);
	void canvas_item_set_self_modulate(RID p_item, const Color& p_color);

	void canvas_item_set_draw_behind_parent(RID p_item, bool p_enable);
	void canvas_item_set_use_identity_transform(RID p_item, bool p_enable);

	void canvas_item_set_update_when_visible(RID p_item, bool p_update);

	float canvas_item_get_compensated_antialiasing_width(float p_width) const;

	void canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
		const Color& p_color, float p_width = -1.0, bool p_antialiased = false);
	void canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	void canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	void canvas_item_add_rect(
		RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased);
	void canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major, float p_minor,
		const Color& p_color, bool p_antialiased = false);
	void canvas_item_add_circle(
		RID p_item, const Point2& p_pos, float p_radius, const Color& p_color, bool p_antialiased);
	void canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect, RID p_texture,
		bool p_tile = false, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false);
	void canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = false);
	void canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), int p_outline_size = 0,
		float p_px_range = 1.0, float p_scale = 1.0);
	void canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1));
	void canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect, const Rect2& p_source,
		RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
		RSE::NinePatchAxisMode p_x_axis_mode = RSE::NINE_PATCH_STRETCH,
		RSE::NinePatchAxisMode p_y_axis_mode = RSE::NINE_PATCH_STRETCH, bool p_draw_center = true,
		const Color& p_modulate = Color(1, 1, 1));
	void canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture);
	void canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		RID p_texture = RID());
	void canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
		const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs = Vector<Point2>(), const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), RID p_texture = RID(), int p_count = -1);
	void canvas_item_add_mesh(RID p_item, const RID& p_mesh,
		const Transform2D& p_transform = Transform2D(), const Color& p_modulate = Color(1, 1, 1),
		RID p_texture = RID());
	void canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture = RID());
	void canvas_item_add_particles(RID p_item, RID p_particles, RID p_texture);
	void canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform);
	void canvas_item_add_clip_ignore(RID p_item, bool p_ignore);
	void canvas_item_add_animation_slice(RID p_item, double p_animation_length,
		double p_slice_begin, double p_slice_end, double p_offset);

	void canvas_item_set_sort_children_by_y(RID p_item, bool p_enable);
	void canvas_item_set_z_index(RID p_item, int p_z);
	void canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable);
	void canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2& p_rect);
	void canvas_item_attach_skeleton(RID p_item, RID p_skeleton);

	void canvas_item_clear(RID p_item);
	void canvas_item_set_draw_index(RID p_item, int p_index);

	void canvas_item_set_material(RID p_item, RID p_material);

	void canvas_item_set_use_parent_material(RID p_item, bool p_enable);

	void canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
		float p_clear_margin = 5.0, bool p_fit_empty = false, float p_fit_margin = 0.0,
		bool p_blur_mipmaps = false);

	void canvas_item_set_debug_redraw(bool p_enabled);
	bool canvas_item_get_debug_redraw() const;

	void canvas_item_set_interpolated(RID p_item, bool p_interpolated);
	void canvas_item_reset_physics_interpolation(RID p_item);
	void canvas_item_transform_physics_interpolation(RID p_item, const Transform2D& p_transform);

	RID canvas_light_allocate();
	void canvas_light_initialize(RID p_rid);

	void update();

	void canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode);
	void canvas_light_attach_to_canvas(RID p_light, RID p_canvas);
	void canvas_light_set_enabled(RID p_light, bool p_enabled);
	void canvas_light_set_texture_scale(RID p_light, float p_scale);
	void canvas_light_set_transform(RID p_light, const Transform2D& p_transform);
	void canvas_light_set_texture(RID p_light, RID p_texture);
	void canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset);
	void canvas_light_set_color(RID p_light, const Color& p_color);
	void canvas_light_set_height(RID p_light, float p_height);
	void canvas_light_set_energy(RID p_light, float p_energy);
	void canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z);
	void canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer);
	void canvas_light_set_item_cull_mask(RID p_light, int p_mask);
	void canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask);
	void canvas_light_set_directional_distance(RID p_light, float p_distance);

	void canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode);

	void canvas_light_set_shadow_enabled(RID p_light, bool p_enabled);
	void canvas_light_set_shadow_filter(RID p_light, RSE::CanvasLightShadowFilter p_filter);
	void canvas_light_set_shadow_color(RID p_light, const Color& p_color);
	void canvas_light_set_shadow_smooth(RID p_light, float p_smooth);

	void canvas_light_set_interpolated(RID p_light, bool p_interpolated);
	void canvas_light_reset_physics_interpolation(RID p_light);
	void canvas_light_transform_physics_interpolation(RID p_light, const Transform2D& p_transform);

	RID canvas_light_occluder_allocate();
	void canvas_light_occluder_initialize(RID p_rid);

	void canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas);
	void canvas_light_occluder_set_enabled(RID p_occluder, bool p_enabled);
	void canvas_light_occluder_set_polygon(RID p_occluder, RID p_polygon);
	void canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable);
	void canvas_light_occluder_set_transform(RID p_occluder, const Transform2D& p_xform);
	void canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask);

	void canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated);
	void canvas_light_occluder_reset_physics_interpolation(RID p_occluder);
	void canvas_light_occluder_transform_physics_interpolation(
		RID p_occluder, const Transform2D& p_transform);

	RID canvas_occluder_polygon_allocate();
	void canvas_occluder_polygon_initialize(RID p_rid);

	void canvas_occluder_polygon_set_shape(
		RID p_occluder_polygon, const Vector<Vector2>& p_shape, bool p_closed);

	void canvas_occluder_polygon_set_cull_mode(
		RID p_occluder_polygon, RSE::CanvasOccluderPolygonCullMode p_mode);

	void canvas_set_shadow_texture_size(int p_size);

	RID canvas_texture_allocate();
	void canvas_texture_initialize(RID p_rid);

	void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
	void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess);

	void canvas_texture_set_texture_filter(RID p_item, RSE::CanvasItemTextureFilter p_filter);
	void canvas_texture_set_texture_repeat(RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

	void canvas_item_set_default_texture_filter(RID p_item, RSE::CanvasItemTextureFilter p_filter);
	void canvas_item_set_default_texture_repeat(RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

	void update_visibility_notifiers();

	void _update_dirty_item(Item* p_item);

	Rect2 _debug_canvas_item_get_rect(RID p_item);

	void finalize();

	/* INTERPOLATION */

	void tick();
	void update_interpolation_tick(bool p_process = true);

	void set_physics_interpolation_enabled(bool p_enabled)
	{
		_interpolation_data.interpolation_enabled = p_enabled;
	}

	struct InterpolationData
	{
		void notify_free_canvas_item(RID p_rid, RendererCanvasCull::Item& r_canvas_item);
		void notify_free_canvas_light(RID p_rid, RendererCanvasRender::Light& r_canvas_light);
		void notify_free_canvas_light_occluder(
			RID p_rid, RendererCanvasRender::LightOccluderInstance& r_canvas_light_occluder);

		LocalVector<RID> canvas_item_transform_update_lists[2];
		LocalVector<RID>* canvas_item_transform_update_list_curr =
			&canvas_item_transform_update_lists[0];
		LocalVector<RID>* canvas_item_transform_update_list_prev =
			&canvas_item_transform_update_lists[1];

		LocalVector<RID> canvas_light_transform_update_lists[2];
		LocalVector<RID>* canvas_light_transform_update_list_curr =
			&canvas_light_transform_update_lists[0];
		LocalVector<RID>* canvas_light_transform_update_list_prev =
			&canvas_light_transform_update_lists[1];

		LocalVector<RID> canvas_light_occluder_transform_update_lists[2];
		LocalVector<RID>* canvas_light_occluder_transform_update_list_curr =
			&canvas_light_occluder_transform_update_lists[0];
		LocalVector<RID>* canvas_light_occluder_transform_update_list_prev =
			&canvas_light_occluder_transform_update_lists[1];

		bool interpolation_enabled = false;
	} _interpolation_data;

	RendererCanvasCull();
	~RendererCanvasCull();
};



// --- End servers/rendering/renderer_canvas_cull.h ---
