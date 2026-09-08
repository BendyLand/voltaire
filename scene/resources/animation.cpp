/**************************************************************************/
/*  animation.cpp                                                         */
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

#include "animation.compat.inc"
#include "animation.h"
#include "core/io/marshalls.h"

void Animation::reset_state() { clear(); }

int Animation::add_track(TrackType p_type, int p_at_pos)
{
	if ((uint32_t)p_at_pos >= tracks.size()) {
		p_at_pos = tracks.size();
	}

	switch (p_type) {
	case TYPE_POSITION_3D: {
		PositionTrack* tt = memnew(PositionTrack);
		tracks.insert(p_at_pos, tt);
	} break;
	case TYPE_ROTATION_3D: {
		RotationTrack* rt = memnew(RotationTrack);
		tracks.insert(p_at_pos, rt);
	} break;
	case TYPE_SCALE_3D: {
		ScaleTrack* st = memnew(ScaleTrack);
		tracks.insert(p_at_pos, st);
	} break;
	case TYPE_BLEND_SHAPE: {
		BlendShapeTrack* bst = memnew(BlendShapeTrack);
		tracks.insert(p_at_pos, bst);
	} break;
	case TYPE_VALUE: {
		tracks.insert(p_at_pos, memnew(ValueTrack));

	} break;
	case TYPE_METHOD: {
		tracks.insert(p_at_pos, memnew(MethodTrack));

	} break;
	case TYPE_BEZIER: {
		tracks.insert(p_at_pos, memnew(BezierTrack));

	} break;
	case TYPE_AUDIO: {
		tracks.insert(p_at_pos, memnew(AudioTrack));

	} break;
	case TYPE_ANIMATION: {
		tracks.insert(p_at_pos, memnew(AnimationTrack));

	} break;
	default: {
		ERR_PRINT("Unknown track type");
	}
	}
	emit_changed();
	return p_at_pos;
}

void Animation::remove_track(int p_track)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];

	switch (t->type) {
	case TYPE_POSITION_3D: {
		PositionTrack* tt = static_cast<PositionTrack*>(t);
		ERR_FAIL_COND_MSG(tt->compressed_track >= 0,
			"Compressed tracks can't be manually removed. Call clear() "
			"to get rid of compression first.");
		tt->positions.clear();

	} break;
	case TYPE_ROTATION_3D: {
		RotationTrack* rt = static_cast<RotationTrack*>(t);
		ERR_FAIL_COND_MSG(rt->compressed_track >= 0,
			"Compressed tracks can't be manually removed. Call clear() "
			"to get rid of compression first.");
		rt->rotations.clear();

	} break;
	case TYPE_SCALE_3D: {
		ScaleTrack* st = static_cast<ScaleTrack*>(t);
		ERR_FAIL_COND_MSG(st->compressed_track >= 0,
			"Compressed tracks can't be manually removed. Call clear() "
			"to get rid of compression first.");
		st->scales.clear();

	} break;
	case TYPE_BLEND_SHAPE: {
		BlendShapeTrack* bst = static_cast<BlendShapeTrack*>(t);
		ERR_FAIL_COND_MSG(bst->compressed_track >= 0,
			"Compressed tracks can't be manually removed. Call clear() "
			"to get rid of compression first.");
		bst->blend_shapes.clear();

	} break;
	case TYPE_METHOD: {
		MethodTrack* mt = static_cast<MethodTrack*>(t);
		mt->methods.clear();

	} break;
	case TYPE_BEZIER: {
		BezierTrack* bz = static_cast<BezierTrack*>(t);
		bz->values.clear();

	} break;
	case TYPE_AUDIO: {
		AudioTrack* ad = static_cast<AudioTrack*>(t);
		ad->values.clear();

	} break;
	case TYPE_ANIMATION: {
		AnimationTrack* an = static_cast<AnimationTrack*>(t);
		an->values.clear();

	} break;
	}

	memdelete(t);
	tracks.remove_at(p_track);
	emit_changed();
	_check_capture_included();
}

bool Animation::is_capture_included() const { return capture_included; }

void Animation::_check_capture_included()
{
	capture_included = false;
	for (uint32_t i = 0; i < tracks.size(); i++) {
		if (tracks[i]->type == TYPE_VALUE) {
			ValueTrack* vt = static_cast<ValueTrack*>(tracks[i]);
			if (vt->update_mode == UPDATE_CAPTURE) {
				capture_included = true;
				break;
			}
		}
	}
}

int Animation::get_track_count() const { return tracks.size(); }

Animation::TrackType Animation::track_get_type(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), TYPE_VALUE);
	return tracks[p_track]->type;
}

void Animation::track_set_path(int p_track, const NodePath& p_path)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->path = p_path;
	tracks[p_track]->concatenated_path = StringName(String(tracks[p_track]->path));
	emit_changed();
}

NodePath Animation::track_get_path(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), NodePath());
	return tracks[p_track]->path;
}

int Animation::find_track(const NodePath& p_path, const TrackType p_type) const
{
	for (uint32_t i = 0; i < tracks.size(); i++) {
		if (tracks[i]->path == p_path && tracks[i]->type == p_type) {
			return i;
		}
	};
	return -1;
}

Animation::TrackType Animation::get_cache_type(TrackType p_type)
{
	if (p_type == Animation::TYPE_BEZIER) {
		return Animation::TYPE_VALUE;
	}
	if (p_type == Animation::TYPE_ROTATION_3D || p_type == Animation::TYPE_SCALE_3D) {
		return Animation::TYPE_POSITION_3D; // Reference them as position3D tracks, even if they
											// modify rotation or scale.
	}
	return p_type;
}

Animation::TrackCacheID Animation::track_get_unique_id(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	return tracks[p_track]->get_unique_id();
}

void Animation::track_set_interpolation_type(int p_track, InterpolationType p_interp)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->interpolation = p_interp;
	emit_changed();
}

Animation::InterpolationType Animation::track_get_interpolation_type(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), INTERPOLATION_NEAREST);
	return tracks[p_track]->interpolation;
}

void Animation::track_set_interpolation_loop_wrap(int p_track, bool p_enable)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->loop_wrap = p_enable;
	emit_changed();
}

bool Animation::track_get_interpolation_loop_wrap(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	return tracks[p_track]->loop_wrap;
}

template <typename T, typename V> int Animation::_insert(double p_time, T& p_keys, const V& p_value)
{
	int idx = p_keys.size();

	while (true) {
		// Condition for replacement.
		if (idx > 0 && Math::is_equal_approx((double)p_keys[idx - 1].time, p_time)) {
			float transition = p_keys[idx - 1].transition;
			p_keys[idx - 1] = p_value;
			p_keys[idx - 1].transition = transition;
			return idx - 1;

			// Condition for insert.
		}
		else if (idx == 0 || p_keys[idx - 1].time < p_time) {
			p_keys.insert(idx, p_value);
			return idx;
		}

		idx--;
	}

	return -1;
}

int Animation::_marker_insert(
	double p_time, LocalVector<MarkerKey>& p_keys, const MarkerKey& p_value)
{
	int idx = p_keys.size();

	while (true) {
		// Condition for replacement.
		if (idx > 0 && Math::is_equal_approx((double)p_keys[idx - 1].time, p_time)) {
			p_keys[idx - 1] = p_value;
			return idx - 1;

			// Condition for insert.
		}
		else if (idx == 0 || p_keys[idx - 1].time < p_time) {
			p_keys.insert(idx, p_value);
			return idx;
		}

		idx--;
	}

	return -1;
}

////

int Animation::position_track_insert_key(int p_track, double p_time, const Vector3& p_position)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_POSITION_3D, -1);

	PositionTrack* tt = static_cast<PositionTrack*>(t);

	ERR_FAIL_COND_V(tt->compressed_track >= 0, -1);

	TKey<Vector3> tkey;
	tkey.time = p_time;
	tkey.value = p_position;

	int ret = _insert(p_time, tt->positions, tkey);
	emit_changed();
	return ret;
}

Error Animation::position_track_get_key(int p_track, int p_key, Vector3* r_position) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];

	PositionTrack* tt = static_cast<PositionTrack*>(t);
	ERR_FAIL_COND_V(t->type != TYPE_POSITION_3D, ERR_INVALID_PARAMETER);

	if (tt->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<3>(tt->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_position = _uncompress_pos_scale(tt->compressed_track, key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, tt->positions.size(), ERR_INVALID_PARAMETER);

	*r_position = tt->positions[p_key].value;

	return OK;
}

Error Animation::try_position_track_interpolate(
	int p_track, double p_time, Vector3* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_POSITION_3D, ERR_INVALID_PARAMETER);

	PositionTrack* tt = static_cast<PositionTrack*>(t);

	if (tt->compressed_track >= 0) {
		if (_pos_scale_interpolate_compressed(tt->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	Vector3 tk =
		_interpolate(tt->positions, p_time, tt->interpolation, tt->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

Vector3 Animation::position_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	Vector3 ret = Vector3(0, 0, 0);
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	bool err = try_position_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(
		err, ret, "3D Position Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

////

int Animation::rotation_track_insert_key(int p_track, double p_time, const Quaternion& p_rotation)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ROTATION_3D, -1);

	RotationTrack* rt = static_cast<RotationTrack*>(t);

	ERR_FAIL_COND_V(rt->compressed_track >= 0, -1);

	TKey<Quaternion> tkey;
	tkey.time = p_time;
	tkey.value = p_rotation;

	int ret = _insert(p_time, rt->rotations, tkey);
	emit_changed();
	return ret;
}

Error Animation::rotation_track_get_key(int p_track, int p_key, Quaternion* r_rotation) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];

	RotationTrack* rt = static_cast<RotationTrack*>(t);
	ERR_FAIL_COND_V(t->type != TYPE_ROTATION_3D, ERR_INVALID_PARAMETER);

	if (rt->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<3>(rt->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_rotation = _uncompress_quaternion(key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, rt->rotations.size(), ERR_INVALID_PARAMETER);

	*r_rotation = rt->rotations[p_key].value;

	return OK;
}

Error Animation::try_rotation_track_interpolate(
	int p_track, double p_time, Quaternion* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ROTATION_3D, ERR_INVALID_PARAMETER);

	RotationTrack* rt = static_cast<RotationTrack*>(t);

	if (rt->compressed_track >= 0) {
		if (_rotation_interpolate_compressed(rt->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	Quaternion tk =
		_interpolate(rt->rotations, p_time, rt->interpolation, rt->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

Quaternion Animation::rotation_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	Quaternion ret = Quaternion(0, 0, 0, 1);
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	Error err = try_rotation_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(err != OK, ret,
		"3D Rotation Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

////

int Animation::scale_track_insert_key(int p_track, double p_time, const Vector3& p_scale)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_SCALE_3D, -1);

	ScaleTrack* st = static_cast<ScaleTrack*>(t);

	ERR_FAIL_COND_V(st->compressed_track >= 0, -1);

	TKey<Vector3> tkey;
	tkey.time = p_time;
	tkey.value = p_scale;

	int ret = _insert(p_time, st->scales, tkey);
	emit_changed();
	return ret;
}

Error Animation::scale_track_get_key(int p_track, int p_key, Vector3* r_scale) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];

	ScaleTrack* st = static_cast<ScaleTrack*>(t);
	ERR_FAIL_COND_V(t->type != TYPE_SCALE_3D, ERR_INVALID_PARAMETER);

	if (st->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<3>(st->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_scale = _uncompress_pos_scale(st->compressed_track, key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, st->scales.size(), ERR_INVALID_PARAMETER);

	*r_scale = st->scales[p_key].value;

	return OK;
}

Error Animation::try_scale_track_interpolate(
	int p_track, double p_time, Vector3* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_SCALE_3D, ERR_INVALID_PARAMETER);

	ScaleTrack* st = static_cast<ScaleTrack*>(t);

	if (st->compressed_track >= 0) {
		if (_pos_scale_interpolate_compressed(st->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	Vector3 tk =
		_interpolate(st->scales, p_time, st->interpolation, st->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

Vector3 Animation::scale_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	Vector3 ret = Vector3(1, 1, 1);
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	bool err = try_scale_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(
		err, ret, "3D Scale Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

////

int Animation::blend_shape_track_insert_key(int p_track, double p_time, float p_blend_shape)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BLEND_SHAPE, -1);

	BlendShapeTrack* st = static_cast<BlendShapeTrack*>(t);

	ERR_FAIL_COND_V(st->compressed_track >= 0, -1);

	TKey<float> tkey;
	tkey.time = p_time;
	tkey.value = p_blend_shape;

	int ret = _insert(p_time, st->blend_shapes, tkey);
	emit_changed();
	return ret;
}

Error Animation::blend_shape_track_get_key(int p_track, int p_key, float* r_blend_shape) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];

	BlendShapeTrack* bst = static_cast<BlendShapeTrack*>(t);
	ERR_FAIL_COND_V(t->type != TYPE_BLEND_SHAPE, ERR_INVALID_PARAMETER);

	if (bst->compressed_track >= 0) {
		Vector3i key;
		double time;
		bool fetch_success = _fetch_compressed_by_index<1>(bst->compressed_track, p_key, key, time);
		if (!fetch_success) {
			return ERR_INVALID_PARAMETER;
		}

		*r_blend_shape = _uncompress_blend_shape(key);
		return OK;
	}

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, bst->blend_shapes.size(), ERR_INVALID_PARAMETER);

	*r_blend_shape = bst->blend_shapes[p_key].value;

	return OK;
}

Error Animation::try_blend_shape_track_interpolate(
	int p_track, double p_time, float* r_interpolation, bool p_backward) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ERR_INVALID_PARAMETER);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BLEND_SHAPE, ERR_INVALID_PARAMETER);

	BlendShapeTrack* bst = static_cast<BlendShapeTrack*>(t);

	if (bst->compressed_track >= 0) {
		if (_blend_shape_interpolate_compressed(bst->compressed_track, p_time, *r_interpolation)) {
			return OK;
		}
		else {
			return ERR_UNAVAILABLE;
		}
	}

	bool ok = false;

	float tk = _interpolate(
		bst->blend_shapes, p_time, bst->interpolation, bst->loop_wrap, &ok, p_backward);

	if (!ok) {
		return ERR_UNAVAILABLE;
	}
	*r_interpolation = tk;
	return OK;
}

float Animation::blend_shape_track_interpolate(int p_track, double p_time, bool p_backward) const
{
	float ret = 0;
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), ret);
	bool err = try_blend_shape_track_interpolate(p_track, p_time, &ret, p_backward);
	ERR_FAIL_COND_V_MSG(
		err, ret, "Blend Shape Track: '" + String(tracks[p_track]->path) + "' is unavailable.");
	return ret;
}

////

void Animation::track_remove_key_at_time(int p_track, double p_time)
{
	int idx = track_find_key(p_track, p_time, FIND_MODE_APPROX);
	ERR_FAIL_COND(idx < 0);
	track_remove_key(p_track, idx);
}

bool Animation::track_is_compressed(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	Track* t = tracks[p_track];

	switch (t->type) {
	case TYPE_POSITION_3D: {
		PositionTrack* tt = static_cast<PositionTrack*>(t);
		return tt->compressed_track >= 0;
	} break;
	case TYPE_ROTATION_3D: {
		RotationTrack* rt = static_cast<RotationTrack*>(t);
		return rt->compressed_track >= 0;
	} break;
	case TYPE_SCALE_3D: {
		ScaleTrack* st = static_cast<ScaleTrack*>(t);
		return st->compressed_track >= 0;
	} break;
	case TYPE_BLEND_SHAPE: {
		BlendShapeTrack* bst = static_cast<BlendShapeTrack*>(t);
		return bst->compressed_track >= 0;
	} break;
	default: {
		return false; // Animation does not really use transitions.
	} break;
	}
}

template <typename K>
int Animation::_find(
	const LocalVector<K>& p_keys, double p_time, bool p_backward, bool p_limit) const
{
	int len = p_keys.size();
	if (len == 0) {
		return -2;
	}

	int low = 0;
	int high = len - 1;
	int middle = 0;

#ifdef DEBUG_ENABLED
	if (low > high) {
		ERR_PRINT("low > high, this may be a bug.");
	}
#endif

	const K* keys = &p_keys[0];

	while (low <= high) {
		middle = (low + high) / 2;

		if (Math::is_equal_approx(p_time, (double)keys[middle].time)) { // match
			return middle;
		}
		else if (p_time < keys[middle].time) {
			high = middle - 1; // search low end of array
		}
		else {
			low = middle + 1; // search high end of array
		}
	}

	if (!p_backward) {
		if (keys[middle].time > p_time) {
			middle--;
		}
	}
	else {
		if (keys[middle].time < p_time) {
			middle++;
		}
	}

	if (p_limit && middle > -1 && middle < len) {
		double diff = length - keys[middle].time;
		if ((std::signbit(keys[middle].time) && !Math::is_zero_approx(keys[middle].time)) ||
			(std::signbit(diff) && !Math::is_zero_approx(diff))) {
			ERR_PRINT_ONCE_ED("Found the key outside the animation range. Consider using the "
							  "clean-up option in AnimationTrackEditor to fix it.");
			return -1;
		}
	}

	return middle;
}

// Linear interpolation for anytype.

Vector3 Animation::_interpolate(const Vector3& p_a, const Vector3& p_b, real_t p_c) const
{
	return p_a.lerp(p_b, p_c);
}

Quaternion Animation::_interpolate(const Quaternion& p_a, const Quaternion& p_b, real_t p_c) const
{
	return p_a.slerp(p_b, p_c);
}

// Cubic interpolation for anytype.

Vector3 Animation::_cubic_interpolate_in_time(const Vector3& p_pre_a, const Vector3& p_a,
	const Vector3& p_b, const Vector3& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
	real_t p_post_b_t) const
{
	return p_a.cubic_interpolate_in_time(p_b, p_pre_a, p_post_b, p_c, p_b_t, p_pre_a_t, p_post_b_t);
}

Quaternion Animation::_cubic_interpolate_in_time(const Quaternion& p_pre_a, const Quaternion& p_a,
	const Quaternion& p_b, const Quaternion& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
	real_t p_post_b_t) const
{
	return p_a.spherical_cubic_interpolate_in_time(
		p_b, p_pre_a, p_post_b, p_c, p_b_t, p_pre_a_t, p_post_b_t);
}

real_t Animation::_cubic_interpolate_in_time(const real_t& p_pre_a, const real_t& p_a,
	const real_t& p_b, const real_t& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
	real_t p_post_b_t) const
{
	return Math::cubic_interpolate_in_time(
		p_a, p_b, p_pre_a, p_post_b, p_c, p_b_t, p_pre_a_t, p_post_b_t);
}

template <typename T>
T Animation::_interpolate(const LocalVector<TKey<T>>& p_keys, double p_time,
	InterpolationType p_interp, bool p_loop_wrap, bool* p_ok, bool p_backward) const
{
	int len = _find(p_keys, length) + 1; // try to find last key (there may be more past the end)

	if (len <= 0) {
		// (-1 or -2 returned originally) (plus one above)
		// meaning no keys, or only key time is larger than length
		if (p_ok) {
			*p_ok = false;
		}
		return T();
	}
	else if (len == 1) { // one key found (0+1), return it

		if (p_ok) {
			*p_ok = true;
		}
		return p_keys[0].value;
	}

	int idx = _find(p_keys, p_time, p_backward);

	ERR_FAIL_COND_V(idx == -2, T());
	int maxi = len - 1;
	bool is_start_edge = p_backward ? idx >= len : idx == -1;
	bool is_end_edge = p_backward ? idx == 0 : idx >= maxi;

	real_t c = 0.0;
	// Prepare for all cases of interpolation.
	real_t delta = 0.0;
	real_t from = 0.0;

	int pre = -1;
	int next = -1;
	int post = -1;
	real_t pre_t = 0.0;
	real_t to_t = 0.0;
	real_t post_t = 0.0;

	bool use_cubic = p_interp == INTERPOLATION_CUBIC || p_interp == INTERPOLATION_CUBIC_ANGLE;

	if (!p_loop_wrap || loop_mode == LOOP_NONE) {
		if (is_start_edge) {
			idx = p_backward ? maxi : 0;
		}
		next = CLAMP(idx + (p_backward ? -1 : 1), 0, maxi);
		if (use_cubic) {
			pre = CLAMP(idx + (p_backward ? 1 : -1), 0, maxi);
			post = CLAMP(idx + (p_backward ? -2 : 2), 0, maxi);
		}
	}
	else if (loop_mode == LOOP_LINEAR) {
		if (is_start_edge) {
			idx = p_backward ? 0 : maxi;
		}
		next = Math::posmod(idx + (p_backward ? -1 : 1), len);
		if (use_cubic) {
			pre = Math::posmod(idx + (p_backward ? 1 : -1), len);
			post = Math::posmod(idx + (p_backward ? -2 : 2), len);
		}
		if (is_start_edge) {
			if (!p_backward) {
				real_t endtime = (length - p_keys[idx].time);
				if (endtime < 0) { // may be keys past the end
					endtime = 0;
				}
				delta = endtime + p_keys[next].time;
				from = endtime + p_time;
			}
			else {
				real_t endtime = p_keys[idx].time;
				if (endtime > length) { // may be keys past the end
					endtime = length;
				}
				delta = endtime + length - p_keys[next].time;
				from = endtime + length - p_time;
			}
		}
		else if (is_end_edge) {
			if (!p_backward) {
				delta = (length - p_keys[idx].time) + p_keys[next].time;
				from = p_time - p_keys[idx].time;
			}
			else {
				delta = p_keys[idx].time + (length - p_keys[next].time);
				from = (length - p_time) - (length - p_keys[idx].time);
			}
		}
	}
	else {
		if (is_start_edge) {
			idx = p_backward ? len : -1;
		}
		next = (int)Math::round(
			Math::pingpong((float)(idx + (p_backward ? -1 : 1)) + 0.5f, (float)len) - 0.5f);
		if (use_cubic) {
			pre = (int)Math::round(
				Math::pingpong((float)(idx + (p_backward ? 1 : -1)) + 0.5f, (float)len) - 0.5f);
			post = (int)Math::round(
				Math::pingpong((float)(idx + (p_backward ? -2 : 2)) + 0.5f, (float)len) - 0.5f);
		}
		idx = (int)Math::round(Math::pingpong((float)idx + 0.5f, (float)len) - 0.5f);
		if (is_start_edge) {
			if (!p_backward) {
				real_t endtime = p_keys[idx].time;
				if (endtime < 0) { // may be keys past the end
					endtime = 0;
				}
				delta = endtime + p_keys[next].time;
				from = endtime + p_time;
			}
			else {
				real_t endtime = length - p_keys[idx].time;
				if (endtime > length) { // may be keys past the end
					endtime = length;
				}
				delta = endtime + length - p_keys[next].time;
				from = endtime + length - p_time;
			}
		}
		else if (is_end_edge) {
			if (!p_backward) {
				delta = length * 2.0 - p_keys[idx].time - p_keys[next].time;
				from = p_time - p_keys[idx].time;
			}
			else {
				delta = p_keys[idx].time + p_keys[next].time;
				from = (length - p_time) - (length - p_keys[idx].time);
			}
		}
	}

	if (!is_start_edge && !is_end_edge) {
		if (!p_backward) {
			delta = p_keys[next].time - p_keys[idx].time;
			from = p_time - p_keys[idx].time;
		}
		else {
			delta = (length - p_keys[next].time) - (length - p_keys[idx].time);
			from = (length - p_time) - (length - p_keys[idx].time);
		}
	}

	if (Math::is_zero_approx(delta)) {
		c = 0;
	}
	else {
		c = from / delta;
	}

	if (p_ok) {
		*p_ok = true;
	}

	real_t tr = p_keys[idx].transition;
	if (tr == 0) {
		// Don't interpolate if not needed.
		return p_keys[idx].value;
	}

	if (tr != 1.0) {
		c = Math::ease(c, tr);
	}

	switch (p_interp) {
	case INTERPOLATION_NEAREST: {
		return p_keys[idx].value;
	} break;
	default:
		return p_keys[idx].value;
	}
	// do a barrel roll
}

void Animation::value_track_set_update_mode(int p_track, UpdateMode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_VALUE);
	ERR_FAIL_INDEX((int)p_mode, 3);

	ValueTrack* vt = static_cast<ValueTrack*>(t);
	vt->update_mode = p_mode;

	_check_capture_included();
	emit_changed();
}

Animation::UpdateMode Animation::value_track_get_update_mode(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), UPDATE_CONTINUOUS);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_VALUE, UPDATE_CONTINUOUS);

	ValueTrack* vt = static_cast<ValueTrack*>(t);
	return vt->update_mode;
}

template <typename T>
void Animation::_track_get_key_indices_in_range(const LocalVector<T>& p_array, double from_time,
	double to_time, LocalVector<int>* r_indices, bool p_is_backward) const
{
	int len = p_array.size();
	if (len == 0) {
		return;
	}

	int from = 0;
	int to = len - 1;

	if (!p_is_backward) {
		while (p_array[from].time < from_time ||
			   Math::is_equal_approx(p_array[from].time, from_time)) {
			from++;
			if (to < from) {
				return;
			}
		}
		while (p_array[to].time > to_time && !Math::is_equal_approx(p_array[to].time, to_time)) {
			to--;
			if (to < from) {
				return;
			}
		}
	}
	else {
		while (p_array[from].time < from_time &&
			   !Math::is_equal_approx(p_array[from].time, from_time)) {
			from++;
			if (to < from) {
				return;
			}
		}
		while (p_array[to].time > to_time || Math::is_equal_approx(p_array[to].time, to_time)) {
			to--;
			if (to < from) {
				return;
			}
		}
	}

	if (from == to) {
		r_indices->push_back(from);
		return;
	}

	if (!p_is_backward) {
		for (int i = from; i <= to; i++) {
			r_indices->push_back(i);
		}
	}
	else {
		for (int i = to; i >= from; i--) {
			r_indices->push_back(i);
		}
	}
}

void Animation::add_marker(const StringName& p_name, double p_time)
{
	int idx = _find(marker_names, p_time);

	if ((uint32_t)idx < marker_names.size() &&
		Math::is_equal_approx(p_time, marker_names[idx].time)) {
		marker_times.erase(marker_names[idx].name);
		marker_colors.erase(marker_names[idx].name);
		marker_names[idx].name = p_name;
		marker_times.insert(p_name, p_time);
		marker_colors.insert(p_name, Color(1, 1, 1));
	}
	else {
		_marker_insert(p_time, marker_names, MarkerKey(p_time, p_name));
		marker_times.insert(p_name, p_time);
		marker_colors.insert(p_name, Color(1, 1, 1));
	}
}

void Animation::remove_marker(const StringName& p_name)
{
	HashMap<StringName, double>::Iterator E = marker_times.find(p_name);
	ERR_FAIL_COND(!E);
	int idx = _find(marker_names, E->value);
	bool success = (uint32_t)idx < marker_names.size() &&
				   Math::is_equal_approx(marker_names[idx].time, E->value);
	ERR_FAIL_COND(!success);
	marker_names.remove_at(idx);
	marker_times.remove(E);
	marker_colors.erase(p_name);
}

bool Animation::has_marker(const StringName& p_name) const { return marker_times.has(p_name); }

StringName Animation::get_marker_at_time(double p_time) const
{
	int idx = _find(marker_names, p_time);

	if ((uint32_t)idx < marker_names.size() &&
		Math::is_equal_approx(marker_names[idx].time, p_time)) {
		return marker_names[idx].name;
	}

	return StringName();
}

StringName Animation::get_next_marker(double p_time) const
{
	int idx = _find(marker_names, p_time);

	if (idx >= -1 && idx < (int)marker_names.size() - 1) {
		// _find ensures that the time at idx is always the closest time to p_time that is also
		// smaller to it. So we add 1 to get the next marker.
		return marker_names[idx + 1].name;
	}
	return StringName();
}

StringName Animation::get_prev_marker(double p_time) const
{
	int idx = _find(marker_names, p_time);

	if ((uint32_t)idx < marker_names.size()) {
		return marker_names[idx].name;
	}
	return StringName();
}

double Animation::get_marker_time(const StringName& p_name) const
{
	ERR_FAIL_COND_V(!marker_times.has(p_name), -1);
	return marker_times.get(p_name);
}

// TODO: This needs to be a TypedArray<StringName> see this PR for rationale
// https://github.com/godotengine/godot/pull/110767/
PackedStringArray Animation::get_marker_names() const
{
	PackedStringArray names;
	// We iterate on marker_names so the result is sorted by time.
	for (const MarkerKey& marker_name : marker_names) {
		names.push_back(marker_name.name);
	}
	return names;
}

Color Animation::get_marker_color(const StringName& p_name) const
{
	ERR_FAIL_COND_V(!marker_colors.has(p_name), Color());
	return marker_colors[p_name];
}

void Animation::set_marker_color(const StringName& p_name, const Color& p_color)
{
	marker_colors[p_name] = p_color;
}

StringName Animation::method_track_get_name(int p_track, int p_key_idx) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), StringName());
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_METHOD, StringName());

	MethodTrack* pm = static_cast<MethodTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key_idx, pm->methods.size(), StringName());

	return pm->methods[p_key_idx].method;
}

int Animation::bezier_track_insert_key(int p_track, double p_time, real_t p_value,
	const Vector2& p_in_handle, const Vector2& p_out_handle)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, -1);

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	TKey<BezierKey> k;
	k.time = p_time;
	k.value.value = p_value;
	k.value.in_handle = p_in_handle;
	if (k.value.in_handle.x > 0) {
		k.value.in_handle.x = 0;
	}
	k.value.out_handle = p_out_handle;
	if (k.value.out_handle.x < 0) {
		k.value.out_handle.x = 0;
	}

	int key = _insert(p_time, bt->values, k);

	emit_changed();

	return key;
}

void Animation::bezier_track_set_key_value(int p_track, int p_index, real_t p_value)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, bt->values.size());

	bt->values[p_index].value.value = p_value;

	emit_changed();
}

void Animation::bezier_track_set_key_in_handle(
	int p_track, int p_index, const Vector2& p_handle, real_t p_balanced_value_time_ratio)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, bt->values.size());

	Vector2 in_handle = p_handle;
	if (in_handle.x > 0) {
		in_handle.x = 0;
	}
	bt->values[p_index].value.in_handle = in_handle;

#ifdef TOOLS_ENABLED
	if (bt->values[p_index].value.handle_mode == HANDLE_MODE_LINEAR) {
		bt->values[p_index].value.in_handle = Vector2();
		bt->values[p_index].value.out_handle = Vector2();
	}
	else if (bt->values[p_index].value.handle_mode == HANDLE_MODE_BALANCED) {
		Transform2D xform;
		xform.set_scale(Vector2(1.0, 1.0 / p_balanced_value_time_ratio));

		Vector2 vec_out = xform.xform(bt->values[p_index].value.out_handle);
		Vector2 vec_in = xform.xform(in_handle);

		bt->values[p_index].value.out_handle =
			xform.affine_inverse().xform(-vec_in.normalized() * vec_out.length());
	}
	else if (bt->values[p_index].value.handle_mode == HANDLE_MODE_MIRRORED) {
		bt->values[p_index].value.out_handle = -in_handle;
	}
#endif // TOOLS_ENABLED

	emit_changed();
}

void Animation::bezier_track_set_key_out_handle(
	int p_track, int p_index, const Vector2& p_handle, real_t p_balanced_value_time_ratio)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, bt->values.size());

	Vector2 out_handle = p_handle;
	if (out_handle.x < 0) {
		out_handle.x = 0;
	}
	bt->values[p_index].value.out_handle = out_handle;

#ifdef TOOLS_ENABLED
	if (bt->values[p_index].value.handle_mode == HANDLE_MODE_LINEAR) {
		bt->values[p_index].value.in_handle = Vector2();
		bt->values[p_index].value.out_handle = Vector2();
	}
	else if (bt->values[p_index].value.handle_mode == HANDLE_MODE_BALANCED) {
		Transform2D xform;
		xform.set_scale(Vector2(1.0, 1.0 / p_balanced_value_time_ratio));

		Vector2 vec_in = xform.xform(bt->values[p_index].value.in_handle);
		Vector2 vec_out = xform.xform(out_handle);

		bt->values[p_index].value.in_handle =
			xform.affine_inverse().xform(-vec_out.normalized() * vec_in.length());
	}
	else if (bt->values[p_index].value.handle_mode == HANDLE_MODE_MIRRORED) {
		bt->values[p_index].value.in_handle = -out_handle;
	}
#endif // TOOLS_ENABLED

	emit_changed();
}

real_t Animation::bezier_track_get_key_value(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, 0);

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, bt->values.size(), 0);

	return bt->values[p_index].value.value;
}

Vector2 Animation::bezier_track_get_key_in_handle(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), Vector2());
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, Vector2());

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, bt->values.size(), Vector2());

	return bt->values[p_index].value.in_handle;
}

Vector2 Animation::bezier_track_get_key_out_handle(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), Vector2());
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, Vector2());

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, bt->values.size(), Vector2());

	return bt->values[p_index].value.out_handle;
}

#ifdef TOOLS_ENABLED
void Animation::bezier_track_set_key_handle_mode(
	int p_track, int p_index, HandleMode p_mode, HandleSetMode p_set_mode)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_BEZIER);

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_index, bt->values.size());

	bt->values[p_index].value.handle_mode = p_mode;

	if (p_mode != HANDLE_MODE_FREE && p_set_mode != HANDLE_SET_MODE_NONE) {
		Vector2& in_handle = bt->values[p_index].value.in_handle;
		Vector2& out_handle = bt->values[p_index].value.out_handle;
		bezier_track_calculate_handles(
			p_track, p_index, p_mode, p_set_mode, &in_handle, &out_handle);
	}

	emit_changed();
}

Animation::HandleMode Animation::bezier_track_get_key_handle_mode(int p_track, int p_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), HANDLE_MODE_FREE);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, HANDLE_MODE_FREE);

	BezierTrack* bt = static_cast<BezierTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_index, bt->values.size(), HANDLE_MODE_FREE);

	return bt->values[p_index].value.handle_mode;
}

bool Animation::bezier_track_calculate_handles(int p_track, int p_index, HandleMode p_mode,
	HandleSetMode p_set_mode, Vector2* r_in_handle, Vector2* r_out_handle)
{
	ERR_FAIL_INDEX_V(p_track, (int)tracks.size(), false);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_BEZIER, false);

	BezierTrack* bt = static_cast<BezierTrack*>(t);
	ERR_FAIL_INDEX_V(p_index, (int)bt->values.size(), false);

	int prev_key = MAX(0, p_index - 1);
	int next_key = MIN((int)bt->values.size() - 1, p_index + 1);
	if (prev_key == next_key) {
		return false;
	}

	float time = bt->values[p_index].time;
	float prev_time = bt->values[prev_key].time;
	float prev_value = bt->values[prev_key].value.value;
	float next_time = bt->values[next_key].time;
	float next_value = bt->values[next_key].value.value;

	return bezier_track_calculate_handles(time, prev_time, prev_value, next_time, next_value,
		p_mode, p_set_mode, r_in_handle, r_out_handle);
}

bool Animation::bezier_track_calculate_handles(float p_time, float p_prev_time, float p_prev_value,
	float p_next_time, float p_next_value, HandleMode p_mode, HandleSetMode p_set_mode,
	Vector2* r_in_handle, Vector2* r_out_handle)
{
	ERR_FAIL_COND_V(p_mode == HANDLE_MODE_FREE, false);
	ERR_FAIL_COND_V(p_set_mode == HANDLE_SET_MODE_NONE, false);

	Vector2 in_handle;
	Vector2 out_handle;

	if (p_mode == HANDLE_MODE_LINEAR) {
		in_handle = Vector2(0, 0);
		out_handle = Vector2(0, 0);
	}
	else if (p_mode == HANDLE_MODE_BALANCED) {
		if (p_set_mode == HANDLE_SET_MODE_RESET) {
			real_t handle_length = 1.0 / 3.0;
			in_handle.x = (p_prev_time - p_time) * handle_length;
			in_handle.y = 0;
			out_handle.x = (p_next_time - p_time) * handle_length;
			out_handle.y = 0;
		}
		else if (p_set_mode == HANDLE_SET_MODE_AUTO) {
			real_t handle_length = 1.0 / 6.0;
			real_t tangent = (p_next_value - p_prev_value) / (p_next_time - p_prev_time);
			in_handle.x = (p_prev_time - p_time) * handle_length;
			in_handle.y = in_handle.x * tangent;
			out_handle.x = (p_next_time - p_time) * handle_length;
			out_handle.y = out_handle.x * tangent;
		}
	}
	else if (p_mode == HANDLE_MODE_MIRRORED) {
		real_t handle_length = 1.0 / 4.0;
		real_t prev_interval = Math::abs(p_time - p_prev_time);
		real_t next_interval = Math::abs(p_time - p_next_time);
		real_t min_time = 0;
		if (Math::is_zero_approx(prev_interval)) {
			min_time = next_interval;
		}
		else if (Math::is_zero_approx(next_interval)) {
			min_time = prev_interval;
		}
		else {
			min_time = MIN(prev_interval, next_interval);
		}
		if (p_set_mode == HANDLE_SET_MODE_RESET) {
			in_handle.x = -min_time * handle_length;
			in_handle.y = 0;
			out_handle.x = min_time * handle_length;
			out_handle.y = 0;
		}
		else if (p_set_mode == HANDLE_SET_MODE_AUTO) {
			real_t tangent = (p_next_value - p_prev_value) / min_time;
			in_handle.x = -min_time * handle_length;
			in_handle.y = in_handle.x * tangent;
			out_handle.x = min_time * handle_length;
			out_handle.y = out_handle.x * tangent;
		}
	}

	if (r_in_handle != nullptr) {
		*r_in_handle = in_handle;
	}

	if (r_out_handle != nullptr) {
		*r_out_handle = out_handle;
	}

	return true;
}

#endif // TOOLS_ENABLED

real_t Animation::bezier_track_interpolate(int p_track, double p_time) const
{
	// this uses a different interpolation scheme
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	Track* track = tracks[p_track];
	ERR_FAIL_COND_V(track->type != TYPE_BEZIER, 0);

	BezierTrack* bt = static_cast<BezierTrack*>(track);

	int len =
		_find(bt->values, length) + 1; // try to find last key (there may be more past the end)

	if (len <= 0) {
		// (-1 or -2 returned originally) (plus one above)
		return 0;
	}
	else if (len == 1) { // one key found (0+1), return it
		return bt->values[0].value.value;
	}

	int idx = _find(bt->values, p_time);

	ERR_FAIL_COND_V(idx == -2, 0);

	// there really is no looping interpolation on bezier

	if (idx < 0) {
		return bt->values[0].value.value;
	}

	if (idx >= (int)bt->values.size() - 1) {
		return bt->values[bt->values.size() - 1].value.value;
	}

	double t = p_time - bt->values[idx].time;

	int iterations = 10;

	real_t duration =
		bt->values[idx + 1].time - bt->values[idx].time; // time duration between our two keyframes
	real_t low = 0.0;									 // 0% of the current animation segment
	real_t high = 1.0;									 // 100% of the current animation segment

	Vector2 start(0, bt->values[idx].value.value);
	Vector2 start_out = start + bt->values[idx].value.out_handle;
	Vector2 end(duration, bt->values[idx + 1].value.value);
	Vector2 end_in = end + bt->values[idx + 1].value.in_handle;

	// narrow high and low as much as possible
	for (int i = 0; i < iterations; i++) {
		real_t middle = (low + high) / 2;

		Vector2 interp = start.bezier_interpolate(start_out, end_in, end, middle);

		if (interp.x < t) {
			low = middle;
		}
		else {
			high = middle;
		}
	}

	// interpolate the result:
	Vector2 low_pos = start.bezier_interpolate(start_out, end_in, end, low);
	Vector2 high_pos = start.bezier_interpolate(start_out, end_in, end, high);
	real_t c = (t - low_pos.x) / (high_pos.x - low_pos.x);

	return low_pos.lerp(high_pos, c).y;
}

int Animation::audio_track_insert_key(int p_track, double p_time, const Ref<Resource>& p_stream,
	real_t p_start_offset, real_t p_end_offset)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, -1);

	AudioTrack* at = static_cast<AudioTrack*>(t);

	TKey<AudioKey> k;
	k.time = p_time;
	k.value.stream = p_stream;
	k.value.start_offset = p_start_offset;
	if (k.value.start_offset < 0) {
		k.value.start_offset = 0;
	}
	k.value.end_offset = p_end_offset;
	if (k.value.end_offset < 0) {
		k.value.end_offset = 0;
	}

	int key = _insert(p_time, at->values, k);

	emit_changed();

	return key;
}

void Animation::audio_track_set_key_stream(int p_track, int p_key, const Ref<Resource>& p_stream)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	AudioTrack* at = static_cast<AudioTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, at->values.size());

	at->values[p_key].value.stream = p_stream;

	emit_changed();
}

void Animation::audio_track_set_key_start_offset(int p_track, int p_key, real_t p_offset)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	AudioTrack* at = static_cast<AudioTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, at->values.size());

	if (p_offset < 0) {
		p_offset = 0;
	}

	at->values[p_key].value.start_offset = p_offset;

	emit_changed();
}

void Animation::audio_track_set_key_end_offset(int p_track, int p_key, real_t p_offset)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	AudioTrack* at = static_cast<AudioTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, at->values.size());

	if (p_offset < 0) {
		p_offset = 0;
	}

	at->values[p_key].value.end_offset = p_offset;

	emit_changed();
}

Ref<Resource> Animation::audio_track_get_key_stream(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), Ref<Resource>());
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, Ref<Resource>());

	const AudioTrack* at = static_cast<const AudioTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, at->values.size(), Ref<Resource>());

	return at->values[p_key].value.stream;
}

real_t Animation::audio_track_get_key_start_offset(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, 0);

	const AudioTrack* at = static_cast<const AudioTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, at->values.size(), 0);

	return at->values[p_key].value.start_offset;
}

real_t Animation::audio_track_get_key_end_offset(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), 0);
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, 0);

	const AudioTrack* at = static_cast<const AudioTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, at->values.size(), 0);

	return at->values[p_key].value.end_offset;
}

void Animation::audio_track_set_use_blend(int p_track, bool p_enable)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_AUDIO);

	AudioTrack* at = static_cast<AudioTrack*>(t);

	at->use_blend = p_enable;
	emit_changed();
}

bool Animation::audio_track_is_use_blend(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_AUDIO, false);

	AudioTrack* at = static_cast<AudioTrack*>(t);

	return at->use_blend;
}

//

int Animation::animation_track_insert_key(int p_track, double p_time, const StringName& p_animation)
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), -1);
	Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ANIMATION, -1);

	AnimationTrack* at = static_cast<AnimationTrack*>(t);

	TKey<StringName> k;
	k.time = p_time;
	k.value = p_animation;

	int key = _insert(p_time, at->values, k);

	emit_changed();

	return key;
}

void Animation::animation_track_set_key_animation(
	int p_track, int p_key, const StringName& p_animation)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	Track* t = tracks[p_track];
	ERR_FAIL_COND(t->type != TYPE_ANIMATION);

	AnimationTrack* at = static_cast<AnimationTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_key, at->values.size());

	at->values[p_key].value = p_animation;

	emit_changed();
}

StringName Animation::animation_track_get_key_animation(int p_track, int p_key) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), StringName());
	const Track* t = tracks[p_track];
	ERR_FAIL_COND_V(t->type != TYPE_ANIMATION, StringName());

	const AnimationTrack* at = static_cast<const AnimationTrack*>(t);

	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_key, at->values.size(), StringName());

	return at->values[p_key].value;
}

void Animation::set_length(double p_length)
{
	if (p_length < ANIM_MIN_LENGTH) {
		p_length = ANIM_MIN_LENGTH;
	}
	length = p_length;
	emit_changed();
}

void Animation::set_loop_mode(Animation::LoopMode p_loop_mode)
{
	loop_mode = p_loop_mode;
	emit_changed();
}

void Animation::track_set_imported(int p_track, bool p_imported)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->imported = p_imported;
}

bool Animation::track_is_imported(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	return tracks[p_track]->imported;
}

void Animation::track_set_enabled(int p_track, bool p_enabled)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	tracks[p_track]->enabled = p_enabled;
	emit_changed();
}

bool Animation::track_is_enabled(int p_track) const
{
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_track, tracks.size(), false);
	return tracks[p_track]->enabled;
}

void Animation::track_move_up(int p_track)
{
	if (p_track < ((int)tracks.size() - 1)) {
		SWAP(tracks[p_track], tracks[p_track + 1]);
	}

	emit_changed();
}

void Animation::track_move_down(int p_track)
{
	if ((uint32_t)p_track < tracks.size()) {
		SWAP(tracks[p_track], tracks[p_track - 1]);
	}

	emit_changed();
}

void Animation::track_move_to(int p_track, int p_to_index)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_to_index, tracks.size() + 1);
	if (p_track == p_to_index || p_track == p_to_index - 1) {
		return;
	}

	Track* track = tracks[p_track];
	tracks.remove_at(p_track);
	// Take into account that the position of the tracks that come after the one removed will
	// change.
	tracks.insert(p_to_index > p_track ? p_to_index - 1 : p_to_index, track);

	emit_changed();
}

void Animation::track_swap(int p_track, int p_with_track)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_track, tracks.size());
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_with_track, tracks.size());
	if (p_track == p_with_track) {
		return;
	}
	SWAP(tracks[p_track], tracks[p_with_track]);

	emit_changed();
}

void Animation::set_step(real_t p_step)
{
	step = p_step;
	emit_changed();
}

real_t Animation::get_step() const { return step; }

void Animation::clear()
{
	for (uint32_t i = 0; i < tracks.size(); i++) {
		memdelete(tracks[i]);
	}
	tracks.clear();
	loop_mode = LOOP_NONE;
	length = 1;
	compression.enabled = false;
	compression.bounds.clear();
	compression.pages.clear();
	compression.fps = 120;
	emit_changed();
}

bool Animation::_float_track_optimize_key(const TKey<float> t0, const TKey<float> t1,
	const TKey<float> t2, real_t p_allowed_velocity_err, real_t p_allowed_precision_error,
	bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if (std::abs(t0.value - t1.value) < p_allowed_precision_error &&
		std::abs(t1.value - t2.value) < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}
	// Calc velocities.
	double v0 = (t1.value - t0.value) / (t1.time - t0.time);
	double v1 = (t2.value - t1.value) / (t2.time - t1.time);
	// Avoid zero div but check equality.
	if (std::abs(v0 - v1) < p_allowed_precision_error) {
		return true;
	}
	else if (std::abs(v0) < p_allowed_precision_error ||
			   std::abs(v1) < p_allowed_precision_error) {
		return false;
	}
	if (!std::signbit(v0 * v1)) {
		v0 = std::abs(v0);
		v1 = std::abs(v1);
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

bool Animation::_vector2_track_optimize_key(const TKey<Vector2> t0, const TKey<Vector2> t1,
	const TKey<Vector2> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
	real_t p_allowed_precision_error, bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if ((t0.value - t1.value).length() < p_allowed_precision_error &&
		(t1.value - t2.value).length() < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}
	// Calc velocities.
	Vector2 vc0 = (t1.value - t0.value) / (t1.time - t0.time);
	Vector2 vc1 = (t2.value - t1.value) / (t2.time - t1.time);
	double v0 = vc0.length();
	double v1 = vc1.length();
	// Avoid zero div but check equality.
	if (std::abs(v0 - v1) < p_allowed_precision_error) {
		return true;
	}
	else if (std::abs(v0) < p_allowed_precision_error ||
			   std::abs(v1) < p_allowed_precision_error) {
		return false;
	}
	// Check axis.
	if (vc0.normalized().dot(vc1.normalized()) >= 1.0 - p_allowed_angular_error * 2.0) {
		v0 = std::abs(v0);
		v1 = std::abs(v1);
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

bool Animation::_vector3_track_optimize_key(const TKey<Vector3> t0, const TKey<Vector3> t1,
	const TKey<Vector3> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
	real_t p_allowed_precision_error, bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if ((t0.value - t1.value).length() < p_allowed_precision_error &&
		(t1.value - t2.value).length() < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}

	// Calc velocities.
	Vector3 vc0 = (t1.value - t0.value) / (t1.time - t0.time);
	Vector3 vc1 = (t2.value - t1.value) / (t2.time - t1.time);
	double v0 = vc0.length();
	double v1 = vc1.length();
	// Avoid zero div but check equality.
	if (std::abs(v0 - v1) < p_allowed_precision_error) {
		return true;
	}
	else if (std::abs(v0) < p_allowed_precision_error ||
			   std::abs(v1) < p_allowed_precision_error) {
		return false;
	}
	// Check axis.
	if (vc0.normalized().dot(vc1.normalized()) >= 1.0 - p_allowed_angular_error * 2.0) {
		v0 = std::abs(v0);
		v1 = std::abs(v1);
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

bool Animation::_quaternion_track_optimize_key(const TKey<Quaternion> t0, const TKey<Quaternion> t1,
	const TKey<Quaternion> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
	real_t p_allowed_precision_error, bool p_is_nearest)
{
	// Remove overlapping keys.
	if (Math::is_equal_approx(t0.time, t1.time) || Math::is_equal_approx(t1.time, t2.time)) {
		return true;
	}
	if ((t0.value - t1.value).length() < p_allowed_precision_error &&
		(t1.value - t2.value).length() < p_allowed_precision_error) {
		return true;
	}
	if (p_is_nearest) {
		return false;
	}
	// Check axis.
	Quaternion q0 = t0.value * t1.value * t0.value.inverse();
	Quaternion q1 = t1.value * t2.value * t1.value.inverse();
	if (q0.get_axis().dot(q1.get_axis()) >= 1.0 - p_allowed_angular_error * 2.0) {
		double a0 = Math::acos(t0.value.dot(t1.value));
		double a1 = Math::acos(t1.value.dot(t2.value));
		if (a0 + a1 >= Math::PI / 2) {
			return false; // Rotation is more than 180 deg, keep key.
		}
		// Calc velocities.
		double v0 = a0 / (t1.time - t0.time);
		double v1 = a1 / (t2.time - t1.time);
		// Avoid zero div but check equality.
		if (std::abs(v0 - v1) < p_allowed_precision_error) {
			return true;
		}
		else if (std::abs(v0) < p_allowed_precision_error ||
				   std::abs(v1) < p_allowed_precision_error) {
			return false;
		}
		double ratio = v0 < v1 ? v0 / v1 : v1 / v0;
		if (ratio >= 1.0 - p_allowed_velocity_err) {
			return true;
		}
	}
	return false;
}

void Animation::_position_track_optimize(int p_idx, real_t p_allowed_velocity_err,
	real_t p_allowed_angular_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_POSITION_3D);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	PositionTrack* tt = static_cast<PositionTrack*>(tracks[p_idx]);
	int i = 0;
	while (i < (int)tt->positions.size() - 2) {
		TKey<Vector3> t0 = tt->positions[i];
		TKey<Vector3> t1 = tt->positions[i + 1];
		TKey<Vector3> t2 = tt->positions[i + 2];
		bool erase = _vector3_track_optimize_key(t0, t1, t2, p_allowed_velocity_err,
			p_allowed_angular_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			tt->positions.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (tt->positions.size() == 2) {
		if ((tt->positions[0].value - tt->positions[1].value).length() <
			p_allowed_precision_error) {
			tt->positions.remove_at(1);
		}
	}
}

void Animation::_rotation_track_optimize(int p_idx, real_t p_allowed_velocity_err,
	real_t p_allowed_angular_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_ROTATION_3D);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	RotationTrack* rt = static_cast<RotationTrack*>(tracks[p_idx]);
	int i = 0;
	while (i < (int)rt->rotations.size() - 2) {
		TKey<Quaternion> t0 = rt->rotations[i];
		TKey<Quaternion> t1 = rt->rotations[i + 1];
		TKey<Quaternion> t2 = rt->rotations[i + 2];
		bool erase = _quaternion_track_optimize_key(t0, t1, t2, p_allowed_velocity_err,
			p_allowed_angular_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			rt->rotations.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (rt->rotations.size() == 2) {
		if ((rt->rotations[0].value - rt->rotations[1].value).length() <
			p_allowed_precision_error) {
			rt->rotations.remove_at(1);
		}
	}
}

void Animation::_scale_track_optimize(int p_idx, real_t p_allowed_velocity_err,
	real_t p_allowed_angular_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_SCALE_3D);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	ScaleTrack* st = static_cast<ScaleTrack*>(tracks[p_idx]);
	int i = 0;
	while (i < (int)st->scales.size() - 2) {
		TKey<Vector3> t0 = st->scales[i];
		TKey<Vector3> t1 = st->scales[i + 1];
		TKey<Vector3> t2 = st->scales[i + 2];
		bool erase = _vector3_track_optimize_key(t0, t1, t2, p_allowed_velocity_err,
			p_allowed_angular_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			st->scales.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (st->scales.size() == 2) {
		if ((st->scales[0].value - st->scales[1].value).length() < p_allowed_precision_error) {
			st->scales.remove_at(1);
		}
	}
}

void Animation::_blend_shape_track_optimize(
	int p_idx, real_t p_allowed_velocity_err, real_t p_allowed_precision_error)
{
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_idx, tracks.size());
	ERR_FAIL_COND(tracks[p_idx]->type != TYPE_BLEND_SHAPE);
	bool is_nearest = false;
	if (tracks[p_idx]->interpolation == INTERPOLATION_NEAREST) {
		is_nearest = true;
	}
	else if (tracks[p_idx]->interpolation != INTERPOLATION_LINEAR) {
		return;
	}
	BlendShapeTrack* bst = static_cast<BlendShapeTrack*>(tracks[p_idx]);
	int i = 0;
	while (i < (int)bst->blend_shapes.size() - 2) {
		TKey<float> t0 = bst->blend_shapes[i];
		TKey<float> t1 = bst->blend_shapes[i + 1];
		TKey<float> t2 = bst->blend_shapes[i + 2];

		bool erase = _float_track_optimize_key(
			t0, t1, t2, p_allowed_velocity_err, p_allowed_precision_error, is_nearest);
		if (erase) {
			bst->blend_shapes.remove_at(i + 1);
		}
		else {
			i++;
		}
	}

	if (bst->blend_shapes.size() == 2) {
		if (std::abs(bst->blend_shapes[0].value - bst->blend_shapes[1].value) <
			p_allowed_precision_error) {
			bst->blend_shapes.remove_at(1);
		}
	}
}

void Animation::optimize(
	real_t p_allowed_velocity_err, real_t p_allowed_angular_err, int p_precision)
{
	real_t precision = Math::pow(0.1, p_precision);
	for (uint32_t i = 0; i < tracks.size(); i++) {
		if (track_is_compressed(i)) {
			continue; // not possible to optimize compressed track
		}
		if (tracks[i]->type == TYPE_POSITION_3D) {
			_position_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
		else if (tracks[i]->type == TYPE_ROTATION_3D) {
			_rotation_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
		else if (tracks[i]->type == TYPE_SCALE_3D) {
			_scale_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
		else if (tracks[i]->type == TYPE_BLEND_SHAPE) {
			_blend_shape_track_optimize(i, p_allowed_velocity_err, precision);
		}
		else if (tracks[i]->type == TYPE_VALUE) {
			_value_track_optimize(i, p_allowed_velocity_err, p_allowed_angular_err, precision);
		}
	}
}

#define print_animc(m_str)

// #define print_animc(m_str) print_line(m_str);

struct AnimationCompressionDataState
{
	enum
	{
		MIN_OPTIMIZE_PACKETS = 5,
		MAX_PACKETS = 16
	};

	uint32_t components = 3;
	LocalVector<uint8_t> data; // Committed packets.

	struct PacketData
	{
		int32_t data[3] = {0, 0, 0};
		uint32_t frame = 0;
	};

	float split_tolerance = 1.5;

	LocalVector<PacketData> temp_packets;

	// used for rollback if the new frame does not fit
	int32_t validated_packet_count = -1;

	static int32_t _compute_delta16_signed(int32_t p_from, int32_t p_to)
	{
		int32_t delta = p_to - p_from;
		if (delta > 32767) {
			return delta - 65536; // use wrap around
		}
		else if (delta < -32768) {
			return 65536 + delta; // use wrap around
		}
		return delta;
	}

	static uint32_t _compute_shift_bits_signed(int32_t p_delta)
	{
		if (p_delta == 0) {
			return 0;
		}
		else if (p_delta < 0) {
			p_delta = Math::abs(p_delta) - 1;
			if (p_delta == 0) {
				return 1;
			}
		}
		return Math::nearest_shift((uint32_t)p_delta);
	}

	void _compute_max_shifts(
		uint32_t p_from, uint32_t p_to, uint32_t* max_shifts, uint32_t& max_frame_delta_shift) const
	{
		for (uint32_t j = 0; j < components; j++) {
			max_shifts[j] = 0;
		}
		max_frame_delta_shift = 0;

		for (uint32_t i = p_from + 1; i <= p_to; i++) {
			int32_t frame_delta = temp_packets[i].frame - temp_packets[i - 1].frame;
			max_frame_delta_shift =
				MAX(max_frame_delta_shift, Math::nearest_shift((uint32_t)frame_delta));
			for (uint32_t j = 0; j < components; j++) {
				int32_t diff =
					_compute_delta16_signed(temp_packets[i - 1].data[j], temp_packets[i].data[j]);
				uint32_t shift = _compute_shift_bits_signed(diff);
				max_shifts[j] = MAX(shift, max_shifts[j]);
			}
		}
	}

	bool insert_key(uint32_t p_frame, const Vector3i& p_key)
	{
		if (temp_packets.size() == MAX_PACKETS) {
			commit_temp_packets();
		}
		PacketData packet;
		packet.frame = p_frame;
		for (int i = 0; i < 3; i++) {
			ERR_FAIL_COND_V(p_key[i] > 65535, false); // Safety checks.
			packet.data[i] = p_key[i];
		}

		temp_packets.push_back(packet);

		if (temp_packets.size() >= MIN_OPTIMIZE_PACKETS) {
			uint32_t max_shifts[3] = {0, 0, 0}; // Base sizes, 16 bit
			uint32_t max_frame_delta_shift = 0;
			// Compute the average shift before the packet was added
			_compute_max_shifts(0, temp_packets.size() - 2, max_shifts, max_frame_delta_shift);

			float prev_packet_size_avg = 0;
			prev_packet_size_avg = float(1 << max_frame_delta_shift);
			for (uint32_t i = 0; i < components; i++) {
				prev_packet_size_avg += float(1 << max_shifts[i]);
			}
			prev_packet_size_avg /= float(1 + components);

			_compute_max_shifts(temp_packets.size() - 2, temp_packets.size() - 1, max_shifts,
				max_frame_delta_shift);

			float new_packet_size_avg = 0;
			new_packet_size_avg = float(1 << max_frame_delta_shift);
			for (uint32_t i = 0; i < components; i++) {
				new_packet_size_avg += float(1 << max_shifts[i]);
			}
			new_packet_size_avg /= float(1 + components);

			print_animc("packet count: " + rtos(temp_packets.size() - 1) + " size avg " +
						rtos(prev_packet_size_avg) + " new avg " + rtos(new_packet_size_avg));
			float ratio = (prev_packet_size_avg < new_packet_size_avg)
							  ? (new_packet_size_avg / prev_packet_size_avg)
							  : (prev_packet_size_avg / new_packet_size_avg);

			if (ratio > split_tolerance) {
				print_animc("split!");
				temp_packets.resize(temp_packets.size() - 1);
				commit_temp_packets();
				temp_packets.push_back(packet);
			}
		}

		return temp_packets.size() == 1; // First key
	}

	uint32_t get_temp_packet_size() const
	{
		if (temp_packets.is_empty()) {
			return 0;
		}
		else if (temp_packets.size() == 1) {
			return components == 1 ? 4 : 8; // 1 component packet is 16 bits and 16 bits unused. 3
											// component packets is 48 bits and 16 bits unused
		}
		uint32_t max_shifts[3] = {0, 0, 0}; // base sizes, 16 bit
		uint32_t max_frame_delta_shift = 0;

		_compute_max_shifts(0, temp_packets.size() - 1, max_shifts, max_frame_delta_shift);

		uint32_t size_bits = 16; // base value (all 4 bits of shift sizes for x,y,z,time)
		size_bits += max_frame_delta_shift * (temp_packets.size() - 1); // times
		for (uint32_t j = 0; j < components; j++) {
			size_bits += 16; // base value
			uint32_t shift = max_shifts[j];
			if (shift > 0) {
				shift += 1; // if not zero, add sign bit
			}
			size_bits += shift * (temp_packets.size() - 1);
		}
		if (size_bits % 8 != 0) { // wrap to 8 bits
			size_bits += 8 - (size_bits % 8);
		}
		uint32_t size_bytes = size_bits / 8; // wrap to words
		if (size_bytes % 4 != 0) {
			size_bytes += 4 - (size_bytes % 4);
		}
		return size_bytes;
	}

	static void _push_bits(LocalVector<uint8_t>& data, uint32_t& r_buffer, uint32_t& r_bits_used,
		uint32_t p_value, uint32_t p_bits)
	{
		r_buffer |= p_value << r_bits_used;
		r_bits_used += p_bits;
		while (r_bits_used >= 8) {
			uint8_t byte = r_buffer & 0xFF;
			data.push_back(byte);
			r_buffer >>= 8;
			r_bits_used -= 8;
		}
	}

	void commit_temp_packets()
	{
		if (temp_packets.is_empty()) {
			return; // Nothing to do.
		}
// #define DEBUG_PACKET_PUSH
#ifdef DEBUG_PACKET_PUSH
#ifndef _MSC_VER
#warning Debugging packet push, disable this code in production to gain a bit more import performance.
#endif
		uint32_t debug_packet_push = get_temp_packet_size();
		uint32_t debug_data_size = data.size();
#endif
		// Store header

		uint8_t header[8];
		uint32_t header_bytes = 0;
		for (uint32_t i = 0; i < components; i++) {
			encode_uint16(temp_packets[0].data[i], &header[header_bytes]);
			header_bytes += 2;
		}

		uint32_t max_shifts[3] = {0, 0, 0}; // base sizes, 16 bit
		uint32_t max_frame_delta_shift = 0;

		if (temp_packets.size() > 1) {
			_compute_max_shifts(0, temp_packets.size() - 1, max_shifts, max_frame_delta_shift);
			uint16_t shift_header = (max_frame_delta_shift - 1) << 12;
			for (uint32_t i = 0; i < components; i++) {
				shift_header |= max_shifts[i] << (4 * i);
			}

			encode_uint16(shift_header, &header[header_bytes]);
			header_bytes += 2;
		}

		while (header_bytes < 8 &&
			   header_bytes % 4 != 0) { // First cond needed to silence wrong GCC warning.
			header[header_bytes++] = 0;
		}

		for (uint32_t i = 0; i < header_bytes; i++) {
			data.push_back(header[i]);
		}

		if (temp_packets.size() == 1) {
			temp_packets.clear();
			validated_packet_count = 0;
			return; // only header stored, nothing else to do
		}

		uint32_t bit_buffer = 0;
		uint32_t bits_used = 0;

		for (uint32_t i = 1; i < temp_packets.size(); i++) {
			uint32_t frame_delta = temp_packets[i].frame - temp_packets[i - 1].frame;
			_push_bits(data, bit_buffer, bits_used, frame_delta, max_frame_delta_shift);

			for (uint32_t j = 0; j < components; j++) {
				if (max_shifts[j] == 0) {
					continue; // Zero delta, do not store
				}
				int32_t delta =
					_compute_delta16_signed(temp_packets[i - 1].data[j], temp_packets[i].data[j]);

				ERR_FAIL_COND(delta < -32768 || delta > 32767); // Safety check.

				uint16_t deltau;
				if (delta < 0) {
					deltau = (Math::abs(delta) - 1) | (1 << max_shifts[j]);
				}
				else {
					deltau = delta;
				}
				_push_bits(
					data, bit_buffer, bits_used, deltau, max_shifts[j] + 1); // Include sign bit
			}
		}
		if (bits_used != 0) {
			ERR_FAIL_COND(bit_buffer > 0xFF); // Safety check.
			data.push_back(bit_buffer);
		}

		while (data.size() % 4 != 0) {
			data.push_back(0); // pad to align with 4
		}

		temp_packets.clear();
		validated_packet_count = 0;

#ifdef DEBUG_PACKET_PUSH
		ERR_FAIL_COND((data.size() - debug_data_size) != debug_packet_push);
#endif
	}
};

struct AnimationCompressionTimeState
{
	struct Packet
	{
		uint32_t frame;
		uint32_t offset;
		uint32_t count;
	};

	LocalVector<Packet> packets;
	// used for rollback
	int32_t key_index = 0;
	int32_t validated_packet_count = 0;
	int32_t validated_key_index = -1;
	bool needs_start_frame = false;
};

Vector3i Animation::_compress_key(
	uint32_t p_track, const AABB& p_bounds, int32_t p_key, float p_time)
{
	Vector3i values;
	TrackType tt = track_get_type(p_track);
	switch (tt) {
	case TYPE_POSITION_3D: {
		Vector3 pos;
		if (p_key >= 0) {
			position_track_get_key(p_track, p_key, &pos);
		}
		else {
			try_position_track_interpolate(p_track, p_time, &pos);
		}
		pos = (pos - p_bounds.position) / p_bounds.size;
		for (int j = 0; j < 3; j++) {
			values[j] = CLAMP(int32_t(pos[j] * 65535.0), 0, 65535);
		}
	} break;
	case TYPE_ROTATION_3D: {
		Quaternion rot;
		if (p_key >= 0) {
			rotation_track_get_key(p_track, p_key, &rot);
		}
		else {
			try_rotation_track_interpolate(p_track, p_time, &rot);
		}
		Vector3 axis = rot.get_axis();
		float angle = rot.get_angle();
		angle = Math::fposmod(double(angle), double(Math::PI * 2.0));
		Vector2 oct = axis.octahedron_encode();
		Vector3 rot_norm(
			oct.x, oct.y, angle / (Math::PI * 2.0)); // high resolution rotation in 0-1 angle.

		for (int j = 0; j < 3; j++) {
			values[j] = CLAMP(int32_t(rot_norm[j] * 65535.0), 0, 65535);
		}
	} break;
	case TYPE_SCALE_3D: {
		Vector3 scale;
		if (p_key >= 0) {
			scale_track_get_key(p_track, p_key, &scale);
		}
		else {
			try_scale_track_interpolate(p_track, p_time, &scale);
		}
		scale = (scale - p_bounds.position) / p_bounds.size;
		for (int j = 0; j < 3; j++) {
			values[j] = CLAMP(int32_t(scale[j] * 65535.0), 0, 65535);
		}
	} break;
	case TYPE_BLEND_SHAPE: {
		float blend;
		if (p_key >= 0) {
			blend_shape_track_get_key(p_track, p_key, &blend);
		}
		else {
			try_blend_shape_track_interpolate(p_track, p_time, &blend);
		}

		blend = (blend / float(Compression::BLEND_SHAPE_RANGE)) * 0.5 + 0.5;
		values[0] = CLAMP(int32_t(blend * 65535.0), 0, 65535);
	} break;
	default: {
		ERR_FAIL_V(Vector3i()); // Safety check.
	} break;
	}

	return values;
}

struct AnimationCompressionBufferBitsRead
{
	uint32_t buffer = 0;
	uint32_t used = 0;
	const uint8_t* src_data = nullptr;

	_FORCE_INLINE_ uint32_t read(uint32_t p_bits)
	{
		uint32_t output = 0;
		uint32_t written = 0;
		while (p_bits > 0) {
			if (used == 0) {
				used = 8;
				buffer = *src_data;
				src_data++;
			}
			uint32_t to_write = MIN(used, p_bits);
			output |= (buffer & ((1 << to_write) - 1)) << written;
			buffer >>= to_write;
			used -= to_write;
			p_bits -= to_write;
			written += to_write;
		}
		return output;
	}
};

bool Animation::_rotation_interpolate_compressed(
	uint32_t p_compressed_track, double p_time, Quaternion& r_ret) const
{
	Vector3i current;
	Vector3i next;
	double time_current;
	double time_next;

	if (!_fetch_compressed<3>(p_compressed_track, p_time, current, time_current, next, time_next)) {
		return false; // some sort of problem
	}

	if (time_current >= p_time || time_current == time_next) {
		r_ret = _uncompress_quaternion(current);
	}
	else if (p_time >= time_next) {
		r_ret = _uncompress_quaternion(next);
	}
	else {
		double c = (p_time - time_current) / (time_next - time_current);
		Quaternion from = _uncompress_quaternion(current);
		Quaternion to = _uncompress_quaternion(next);
		r_ret = from.slerp(to, c);
	}

	return true;
}

bool Animation::_pos_scale_interpolate_compressed(
	uint32_t p_compressed_track, double p_time, Vector3& r_ret) const
{
	Vector3i current;
	Vector3i next;
	double time_current;
	double time_next;

	if (!_fetch_compressed<3>(p_compressed_track, p_time, current, time_current, next, time_next)) {
		return false; // some sort of problem
	}

	if (time_current >= p_time || time_current == time_next) {
		r_ret = _uncompress_pos_scale(p_compressed_track, current);
	}
	else if (p_time >= time_next) {
		r_ret = _uncompress_pos_scale(p_compressed_track, next);
	}
	else {
		double c = (p_time - time_current) / (time_next - time_current);
		Vector3 from = _uncompress_pos_scale(p_compressed_track, current);
		Vector3 to = _uncompress_pos_scale(p_compressed_track, next);
		r_ret = from.lerp(to, c);
	}

	return true;
}

bool Animation::_blend_shape_interpolate_compressed(
	uint32_t p_compressed_track, double p_time, float& r_ret) const
{
	Vector3i current;
	Vector3i next;
	double time_current;
	double time_next;

	if (!_fetch_compressed<1>(p_compressed_track, p_time, current, time_current, next, time_next)) {
		return false; // some sort of problem
	}

	if (time_current >= p_time || time_current == time_next) {
		r_ret = _uncompress_blend_shape(current);
	}
	else if (p_time >= time_next) {
		r_ret = _uncompress_blend_shape(next);
	}
	else {
		float c = (p_time - time_current) / (time_next - time_current);
		float from = _uncompress_blend_shape(current);
		float to = _uncompress_blend_shape(next);
		r_ret = Math::lerp(from, to, c);
	}

	return true;
}

template <uint32_t COMPONENTS>
bool Animation::_fetch_compressed(uint32_t p_compressed_track, double p_time,
	Vector3i& r_current_value, double& r_current_time, Vector3i& r_next_value, double& r_next_time,
	uint32_t* key_index) const
{
	ERR_FAIL_COND_V(!compression.enabled, false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_compressed_track, compression.bounds.size(), false);
	p_time = CLAMP(p_time, 0, length);
	if (key_index) {
		*key_index = 0;
	}

	double frame_to_sec = 1.0 / double(compression.fps);

	int32_t page_index = -1;
	for (uint32_t i = 0; i < compression.pages.size(); i++) {
		if (compression.pages[i].time_offset > p_time) {
			break;
		}
		page_index = i;
	}

	ERR_FAIL_COND_V(page_index == -1, false); // should not happen

	double page_base_time = compression.pages[page_index].time_offset;
	const uint8_t* page_data = compression.pages[page_index].data.ptr();
	// Little endian assumed. No major big endian hardware exists any longer, but in case it does it
	// will need to be supported.
	const uint32_t* indices = (const uint32_t*)page_data;
	const uint16_t* time_keys = (const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
	uint32_t time_key_count = indices[p_compressed_track * 3 + 1];

	int32_t packet_idx = 0;
	double packet_time = double(time_keys[0]) * frame_to_sec + page_base_time;
	uint32_t base_frame = time_keys[0];

	for (uint32_t i = 1; i < time_key_count; i++) {
		uint32_t f = time_keys[i * 2 + 0];
		double frame_time = double(f) * frame_to_sec + page_base_time;

		if (frame_time > p_time) {
			break;
		}

		if (key_index) {
			(*key_index) += (time_keys[(i - 1) * 2 + 1] >> 12) + 1;
		}

		packet_idx = i;
		packet_time = frame_time;
		base_frame = f;
	}

	const uint8_t* data_keys_base = (const uint8_t*)&page_data[indices[p_compressed_track * 3 + 2]];

	uint16_t time_key_data = time_keys[packet_idx * 2 + 1];
	uint32_t data_offset = (time_key_data & 0xFFF) * 4; // lower 12 bits
	uint32_t data_count = (time_key_data >> 12) + 1;

	const uint16_t* data_key = (const uint16_t*)(data_keys_base + data_offset);

	uint16_t decode[COMPONENTS];
	uint16_t decode_next[COMPONENTS];

	for (uint32_t i = 0; i < COMPONENTS; i++) {
		decode[i] = data_key[i];
		decode_next[i] = data_key[i];
	}

	double next_time = packet_time;

	if (p_time > packet_time) { // If its equal or less, then don't bother
		if (data_count > 1) {
			// decode forward
			uint32_t bit_width[COMPONENTS];
			for (uint32_t i = 0; i < COMPONENTS; i++) {
				bit_width[i] = (data_key[COMPONENTS] >> (i * 4)) & 0xF;
			}

			uint32_t frame_bit_width = (data_key[COMPONENTS] >> 12) + 1;

			AnimationCompressionBufferBitsRead buffer;

			buffer.src_data = (const uint8_t*)&data_key[COMPONENTS + 1];

			for (uint32_t i = 1; i < data_count; i++) {
				uint32_t frame_delta = buffer.read(frame_bit_width);
				base_frame += frame_delta;

				for (uint32_t j = 0; j < COMPONENTS; j++) {
					if (bit_width[j] == 0) {
						continue; // do none
					}
					uint32_t valueu = buffer.read(bit_width[j] + 1);
					bool sign = valueu & (1 << bit_width[j]);
					int16_t value = valueu & ((1 << bit_width[j]) - 1);
					if (sign) {
						value = -value - 1;
					}

					decode_next[j] += value;
				}

				next_time = double(base_frame) * frame_to_sec + page_base_time;
				if (p_time < next_time) {
					break;
				}

				packet_time = next_time;

				for (uint32_t j = 0; j < COMPONENTS; j++) {
					decode[j] = decode_next[j];
				}

				if (key_index) {
					(*key_index)++;
				}
			}
		}

		if (p_time > next_time) { // > instead of >= because if its equal, then it will be properly
								  // interpolated anyway
			// So, the last frame found still has a time that is less than the required frame,
			// will have to interpolate with the first frame of the next timekey.

			if ((uint32_t)packet_idx <
				time_key_count - 1) { // Safety check but should not matter much, otherwise current
									  // next packet is last packet.

				uint16_t time_key_data_next = time_keys[(packet_idx + 1) * 2 + 1];
				uint32_t data_offset_next = (time_key_data_next & 0xFFF) * 4; // Lower 12 bits

				const uint16_t* data_key_next =
					(const uint16_t*)(data_keys_base + data_offset_next);
				base_frame = time_keys[(packet_idx + 1) * 2 + 0];
				next_time = double(base_frame) * frame_to_sec + page_base_time;
				for (uint32_t i = 0; i < COMPONENTS; i++) {
					decode_next[i] = data_key_next[i];
				}
			}
		}
	}

	r_current_time = packet_time;
	r_next_time = next_time;

	for (uint32_t i = 0; i < COMPONENTS; i++) {
		r_current_value[i] = decode[i];
		r_next_value[i] = decode_next[i];
	}

	return true;
}

template <uint32_t COMPONENTS>
void Animation::_get_compressed_key_indices_in_range(
	uint32_t p_compressed_track, double p_time, double p_delta, LocalVector<int>* r_indices) const
{
	ERR_FAIL_COND(!compression.enabled);
	ERR_FAIL_UNSIGNED_INDEX(p_compressed_track, compression.bounds.size());

	double frame_to_sec = 1.0 / double(compression.fps);
	uint32_t key_index = 0;

	for (uint32_t p = 0; p < compression.pages.size(); p++) {
		if (compression.pages[p].time_offset >= p_time + p_delta) {
			// Page beyond range
			return;
		}

		// Page within range

		uint32_t page_index = p;

		double page_base_time = compression.pages[page_index].time_offset;
		const uint8_t* page_data = compression.pages[page_index].data.ptr();
		// Little endian assumed. No major big endian hardware exists any longer, but in case it
		// does it will need to be supported.
		const uint32_t* indices = (const uint32_t*)page_data;
		const uint16_t* time_keys =
			(const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
		uint32_t time_key_count = indices[p_compressed_track * 3 + 1];

		for (uint32_t i = 0; i < time_key_count; i++) {
			uint32_t f = time_keys[i * 2 + 0];
			double frame_time = f * frame_to_sec + page_base_time;
			if (frame_time >= p_time + p_delta) {
				return;
			}
			else if (frame_time >= p_time) {
				r_indices->push_back(key_index);
			}

			key_index++;

			const uint8_t* data_keys_base =
				(const uint8_t*)&page_data[indices[p_compressed_track * 3 + 2]];

			uint16_t time_key_data = time_keys[i * 2 + 1];
			uint32_t data_offset = (time_key_data & 0xFFF) * 4; // lower 12 bits
			uint32_t data_count = (time_key_data >> 12) + 1;

			const uint16_t* data_key = (const uint16_t*)(data_keys_base + data_offset);

			if (data_count > 1) {
				// decode forward
				uint32_t bit_width[COMPONENTS];
				for (uint32_t j = 0; j < COMPONENTS; j++) {
					bit_width[j] = (data_key[COMPONENTS] >> (j * 4)) & 0xF;
				}

				uint32_t frame_bit_width = (data_key[COMPONENTS] >> 12) + 1;

				AnimationCompressionBufferBitsRead buffer;

				buffer.src_data = (const uint8_t*)&data_key[COMPONENTS + 1];

				for (uint32_t j = 1; j < data_count; j++) {
					uint32_t frame_delta = buffer.read(frame_bit_width);
					f += frame_delta;

					frame_time = f * frame_to_sec + page_base_time;
					if (frame_time >= p_time + p_delta) {
						return;
					}
					else if (frame_time >= p_time) {
						r_indices->push_back(key_index);
					}

					for (uint32_t k = 0; k < COMPONENTS; k++) {
						if (bit_width[k] == 0) {
							continue; // do none
						}
						buffer.read(bit_width[k] + 1); // skip
					}

					key_index++;
				}
			}
		}
	}
}

int Animation::_get_compressed_key_count(uint32_t p_compressed_track) const
{
	ERR_FAIL_COND_V(!compression.enabled, -1);
	ERR_FAIL_UNSIGNED_INDEX_V(p_compressed_track, compression.bounds.size(), -1);

	int key_count = 0;

	for (const Compression::Page& page : compression.pages) {
		const uint8_t* page_data = page.data.ptr();
		// Little endian assumed. No major big endian hardware exists any longer, but in case it
		// does it will need to be supported.
		const uint32_t* indices = (const uint32_t*)page_data;
		const uint16_t* time_keys =
			(const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
		uint32_t time_key_count = indices[p_compressed_track * 3 + 1];

		for (uint32_t j = 0; j < time_key_count; j++) {
			key_count += (time_keys[j * 2 + 1] >> 12) + 1;
		}
	}

	return key_count;
}

Quaternion Animation::_uncompress_quaternion(const Vector3i& p_value) const
{
	Vector3 axis =
		Vector3::octahedron_decode(Vector2(float(p_value.x) / 65535.0, float(p_value.y) / 65535.0));
	float angle = (float(p_value.z) / 65535.0) * 2.0 * Math::PI;
	return Quaternion(axis, angle);
}

Vector3 Animation::_uncompress_pos_scale(uint32_t p_compressed_track, const Vector3i& p_value) const
{
	Vector3 pos_norm(
		float(p_value.x) / 65535.0, float(p_value.y) / 65535.0, float(p_value.z) / 65535.0);
	return compression.bounds[p_compressed_track].position +
		   pos_norm * compression.bounds[p_compressed_track].size;
}

float Animation::_uncompress_blend_shape(const Vector3i& p_value) const
{
	float bsn = float(p_value.x) / 65535.0;
	return (bsn * 2.0 - 1.0) * float(Compression::BLEND_SHAPE_RANGE);
}

template <uint32_t COMPONENTS>
bool Animation::_fetch_compressed_by_index(
	uint32_t p_compressed_track, int p_index, Vector3i& r_value, double& r_time) const
{
	ERR_FAIL_COND_V(!compression.enabled, false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_compressed_track, compression.bounds.size(), false);

	for (const Compression::Page& page : compression.pages) {
		const uint8_t* page_data = page.data.ptr();
		// Little endian assumed. No major big endian hardware exists any longer, but in case it
		// does it will need to be supported.
		const uint32_t* indices = (const uint32_t*)page_data;
		const uint16_t* time_keys =
			(const uint16_t*)&page_data[indices[p_compressed_track * 3 + 0]];
		uint32_t time_key_count = indices[p_compressed_track * 3 + 1];
		const uint8_t* data_keys_base =
			(const uint8_t*)&page_data[indices[p_compressed_track * 3 + 2]];

		for (uint32_t j = 0; j < time_key_count; j++) {
			uint32_t subkeys = (time_keys[j * 2 + 1] >> 12) + 1;
			if ((uint32_t)p_index < subkeys) {
				uint16_t data_offset = (time_keys[j * 2 + 1] & 0xFFF) * 4;

				const uint16_t* data_key = (const uint16_t*)(data_keys_base + data_offset);

				uint16_t frame = time_keys[j * 2 + 0];
				uint16_t decode[COMPONENTS];

				for (uint32_t k = 0; k < COMPONENTS; k++) {
					decode[k] = data_key[k];
				}

				if (p_index > 0) {
					uint32_t bit_width[COMPONENTS];
					for (uint32_t k = 0; k < COMPONENTS; k++) {
						bit_width[k] = (data_key[COMPONENTS] >> (k * 4)) & 0xF;
					}
					uint32_t frame_bit_width = (data_key[COMPONENTS] >> 12) + 1;

					AnimationCompressionBufferBitsRead buffer;
					buffer.src_data = (const uint8_t*)&data_key[COMPONENTS + 1];

					for (int k = 0; k < p_index; k++) {
						uint32_t frame_delta = buffer.read(frame_bit_width);
						frame += frame_delta;
						for (uint32_t l = 0; l < COMPONENTS; l++) {
							if (bit_width[l] == 0) {
								continue; // do none
							}
							uint32_t valueu = buffer.read(bit_width[l] + 1);
							bool sign = valueu & (1 << bit_width[l]);
							int16_t value = valueu & ((1 << bit_width[l]) - 1);
							if (sign) {
								value = -value - 1;
							}

							decode[l] += value;
						}
					}
				}

				r_time = page.time_offset + double(frame) / double(compression.fps);
				for (uint32_t l = 0; l < COMPONENTS; l++) {
					r_value[l] = decode[l];
				}

				return true;

			}
			else {
				p_index -= subkeys;
			}
		}
	}

	return false;
}

// Helper functions for Rotation.
double Animation::interpolate_via_rest(double p_from, double p_to, double p_weight, double p_rest)
{
	double rot_a = Math::fposmod(p_from, Math::TAU);
	double rot_b = Math::fposmod(p_to, Math::TAU);
	double rot_rest = Math::fposmod(p_rest, Math::TAU);
	if (rot_rest < Math::PI) {
		rot_a = rot_a > rot_rest + Math::PI ? rot_a - Math::TAU : rot_a;
		rot_b = rot_b > rot_rest + Math::PI ? rot_b - Math::TAU : rot_b;
	}
	else {
		rot_a = rot_a < rot_rest - Math::PI ? rot_a + Math::TAU : rot_a;
		rot_b = rot_b < rot_rest - Math::PI ? rot_b + Math::TAU : rot_b;
	}
	return Math::fposmod(rot_a + (rot_b - rot_rest) * p_weight, Math::TAU);
}

Quaternion Animation::interpolate_via_rest(
	const Quaternion& p_from, const Quaternion& p_to, real_t p_weight, const Quaternion& p_rest)
{
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V_MSG(
		!p_from.is_normalized(), Quaternion(), "The start quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(
		!p_to.is_normalized(), Quaternion(), "The end quaternion must be normalized.");
	ERR_FAIL_COND_V_MSG(
		!p_rest.is_normalized(), Quaternion(), "The rest quaternion must be normalized.");
#endif
	return (p_from * Quaternion().slerp(p_rest.inverse() * p_to, p_weight)).normalized();
}

bool Animation::inform_variant_array(int& r_min, int& r_max)
{
	if (r_min <= r_max) {
		return false;
	}
	SWAP(r_min, r_max);
	return true;
}

Animation::Animation() {}

Animation::~Animation()
{
	for (uint32_t i = 0; i < tracks.size(); i++) {
		memdelete(tracks[i]);
	}
}


