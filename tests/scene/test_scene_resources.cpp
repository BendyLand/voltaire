
// === Merged from tests/scene/test_animation.cpp ===
/*  test_animation.cpp                                                    */
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

#include "tests/test_macros.h"



#include "scene/main/scene_tree.h"

namespace TestAnimation {

TEST_CASE("[Animation] Empty animation getters") {
	const Ref<Animation> animation = memnew(Animation);

	CHECK(animation->get_length() == doctest::Approx(real_t(1.0)));
	CHECK(animation->get_step() == doctest::Approx(real_t(1.0 / 30)));
}

TEST_CASE("[Animation] Create value track") {
	// This creates an animation that makes the node "Enemy" move to the right by
	// 100 pixels in 0.5 seconds.
	Ref<Animation> animation = memnew(Animation);
	const int track_index = animation->add_track(Animation::TYPE_VALUE);
	CHECK(track_index == 0);
	animation->track_set_path(track_index, NodePath("Enemy:position:x"));
	animation->track_insert_key(track_index, 0.0, 0);
	animation->track_insert_key(track_index, 0.5, 100);

	CHECK(animation->get_track_count() == 1);
	CHECK(!animation->track_is_compressed(0));
	CHECK(int(animation->track_get_key_value(0, 0)) == 0);
	CHECK(int(animation->track_get_key_value(0, 1)) == 100);

	CHECK(animation->value_track_interpolate(0, -0.2) == doctest::Approx(0.0));
	CHECK(animation->value_track_interpolate(0, 0.0) == doctest::Approx(0.0));
	CHECK(animation->value_track_interpolate(0, 0.2) == doctest::Approx(40.0));
	CHECK(animation->value_track_interpolate(0, 0.4) == doctest::Approx(80.0));
	CHECK(animation->value_track_interpolate(0, 0.5) == doctest::Approx(100.0));
	CHECK(animation->value_track_interpolate(0, 0.6) == doctest::Approx(100.0));

	CHECK(animation->track_get_key_transition(0, 0) == doctest::Approx(real_t(1.0)));
	CHECK(animation->track_get_key_transition(0, 1) == doctest::Approx(real_t(1.0)));

	ERR_PRINT_OFF;
	// Nonexistent keys.
	CHECK(animation->track_get_key_value(0, 2).is_null());
	CHECK(animation->track_get_key_value(0, -1).is_null());
	CHECK(animation->track_get_key_transition(0, 2) == doctest::Approx(real_t(-1.0)));
	// Nonexistent track (and keys).
	CHECK(animation->track_get_key_value(1, 0).is_null());
	CHECK(animation->track_get_key_value(1, 1).is_null());
	CHECK(animation->track_get_key_value(1, 2).is_null());
	CHECK(animation->track_get_key_value(1, -1).is_null());
	CHECK(animation->track_get_key_transition(1, 0) == doctest::Approx(real_t(-1.0)));

	// This is a value track, so the methods below should return errors.
	CHECK(animation->try_position_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_rotation_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_scale_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->bezier_track_interpolate(0, 0.0) == doctest::Approx(0.0));
	CHECK(animation->try_blend_shape_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
}

TEST_CASE("[Animation] Create 3D position track") {
	Ref<Animation> animation = memnew(Animation);
	const int track_index = animation->add_track(Animation::TYPE_POSITION_3D);
	animation->track_set_path(track_index, NodePath("Enemy:position"));
	animation->position_track_insert_key(track_index, 0.0, Vector3(0, 1, 2));
	animation->position_track_insert_key(track_index, 0.5, Vector3(3.5, 4, 5));

	CHECK(animation->get_track_count() == 1);
	CHECK(!animation->track_is_compressed(0));
	CHECK(Vector3(animation->track_get_key_value(0, 0)).is_equal_approx(Vector3(0, 1, 2)));
	CHECK(Vector3(animation->track_get_key_value(0, 1)).is_equal_approx(Vector3(3.5, 4, 5)));

	Vector3 r_interpolation;

	CHECK(animation->try_position_track_interpolate(0, -0.2, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(0, 1, 2)));

	CHECK(animation->try_position_track_interpolate(0, 0.0, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(0, 1, 2)));

	CHECK(animation->try_position_track_interpolate(0, 0.2, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(1.4, 2.2, 3.2)));

	CHECK(animation->try_position_track_interpolate(0, 0.4, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(2.8, 3.4, 4.4)));

	CHECK(animation->try_position_track_interpolate(0, 0.5, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(3.5, 4, 5)));

	CHECK(animation->try_position_track_interpolate(0, 0.6, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(3.5, 4, 5)));

	// 3D position tracks always use linear interpolation for performance reasons.
	CHECK(animation->track_get_key_transition(0, 0) == doctest::Approx(real_t(1.0)));
	CHECK(animation->track_get_key_transition(0, 1) == doctest::Approx(real_t(1.0)));

	// This is a 3D position track, so the methods below should return errors.
	ERR_PRINT_OFF;
	CHECK(animation->value_track_interpolate(0, 0.0).is_null());
	CHECK(animation->try_rotation_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_scale_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->bezier_track_interpolate(0, 0.0) == doctest::Approx(0.0));
	CHECK(animation->try_blend_shape_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
}

TEST_CASE("[Animation] Create 3D rotation track") {
	Ref<Animation> animation = memnew(Animation);
	const int track_index = animation->add_track(Animation::TYPE_ROTATION_3D);
	animation->track_set_path(track_index, NodePath("Enemy:rotation"));
	animation->rotation_track_insert_key(track_index, 0.0, Quaternion::from_euler(Vector3(0, 1, 2)));
	animation->rotation_track_insert_key(track_index, 0.5, Quaternion::from_euler(Vector3(3.5, 4, 5)));

	CHECK(animation->get_track_count() == 1);
	CHECK(!animation->track_is_compressed(0));
	CHECK(Quaternion(animation->track_get_key_value(0, 0)).is_equal_approx(Quaternion::from_euler(Vector3(0, 1, 2))));
	CHECK(Quaternion(animation->track_get_key_value(0, 1)).is_equal_approx(Quaternion::from_euler(Vector3(3.5, 4, 5))));

	Quaternion r_interpolation;

	CHECK(animation->try_rotation_track_interpolate(0, -0.2, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Quaternion(0.403423, 0.259035, 0.73846, 0.47416)));

	CHECK(animation->try_rotation_track_interpolate(0, 0.0, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Quaternion(0.403423, 0.259035, 0.73846, 0.47416)));

	CHECK(animation->try_rotation_track_interpolate(0, 0.2, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Quaternion(0.336182, 0.30704, 0.751515, 0.477425)));

	CHECK(animation->try_rotation_track_interpolate(0, 0.4, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Quaternion(0.266585, 0.352893, 0.759303, 0.477344)));

	CHECK(animation->try_rotation_track_interpolate(0, 0.5, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Quaternion(0.231055, 0.374912, 0.761204, 0.476048)));

	CHECK(animation->try_rotation_track_interpolate(0, 0.6, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Quaternion(0.231055, 0.374912, 0.761204, 0.476048)));

	// 3D rotation tracks always use linear interpolation for performance reasons.
	CHECK(animation->track_get_key_transition(0, 0) == doctest::Approx(real_t(1.0)));
	CHECK(animation->track_get_key_transition(0, 1) == doctest::Approx(real_t(1.0)));

	// This is a 3D rotation track, so the methods below should return errors.
	ERR_PRINT_OFF;
	CHECK(animation->value_track_interpolate(0, 0.0).is_null());
	CHECK(animation->try_position_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_scale_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->bezier_track_interpolate(0, 0.0) == doctest::Approx(real_t(0.0)));
	CHECK(animation->try_blend_shape_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
}

TEST_CASE("[Animation] Create 3D scale track") {
	Ref<Animation> animation = memnew(Animation);
	const int track_index = animation->add_track(Animation::TYPE_SCALE_3D);
	animation->track_set_path(track_index, NodePath("Enemy:scale"));
	animation->scale_track_insert_key(track_index, 0.0, Vector3(0, 1, 2));
	animation->scale_track_insert_key(track_index, 0.5, Vector3(3.5, 4, 5));

	CHECK(animation->get_track_count() == 1);
	CHECK(!animation->track_is_compressed(0));
	CHECK(Vector3(animation->track_get_key_value(0, 0)).is_equal_approx(Vector3(0, 1, 2)));
	CHECK(Vector3(animation->track_get_key_value(0, 1)).is_equal_approx(Vector3(3.5, 4, 5)));

	Vector3 r_interpolation;

	CHECK(animation->try_scale_track_interpolate(0, -0.2, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(0, 1, 2)));

	CHECK(animation->try_scale_track_interpolate(0, 0.0, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(0, 1, 2)));

	CHECK(animation->try_scale_track_interpolate(0, 0.2, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(1.4, 2.2, 3.2)));

	CHECK(animation->try_scale_track_interpolate(0, 0.4, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(2.8, 3.4, 4.4)));

	CHECK(animation->try_scale_track_interpolate(0, 0.5, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(3.5, 4, 5)));

	CHECK(animation->try_scale_track_interpolate(0, 0.6, &r_interpolation) == OK);
	CHECK(r_interpolation.is_equal_approx(Vector3(3.5, 4, 5)));

	// 3D scale tracks always use linear interpolation for performance reasons.
	CHECK(animation->track_get_key_transition(0, 0) == doctest::Approx(1.0));
	CHECK(animation->track_get_key_transition(0, 1) == doctest::Approx(1.0));

	// This is a 3D scale track, so the methods below should return errors.
	ERR_PRINT_OFF;
	CHECK(animation->value_track_interpolate(0, 0.0).is_null());
	CHECK(animation->try_position_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_rotation_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->bezier_track_interpolate(0, 0.0) == doctest::Approx(0.0));
	CHECK(animation->try_blend_shape_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
}

TEST_CASE("[Animation] Create blend shape track") {
	Ref<Animation> animation = memnew(Animation);
	const int track_index = animation->add_track(Animation::TYPE_BLEND_SHAPE);
	animation->track_set_path(track_index, NodePath("Enemy:scale"));
	// Negative values for blend shapes should work as expected.
	animation->blend_shape_track_insert_key(track_index, 0.0, -1.0);
	animation->blend_shape_track_insert_key(track_index, 0.5, 1.0);

	CHECK(animation->get_track_count() == 1);
	CHECK(!animation->track_is_compressed(0));

	float r_blend = 0.0f;

	CHECK(animation->blend_shape_track_get_key(0, 0, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(-1.0f));

	CHECK(animation->blend_shape_track_get_key(0, 1, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(1.0f));

	CHECK(animation->try_blend_shape_track_interpolate(0, -0.2, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(-1.0f));

	CHECK(animation->try_blend_shape_track_interpolate(0, 0.0, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(-1.0f));

	CHECK(animation->try_blend_shape_track_interpolate(0, 0.2, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(-0.2f));

	CHECK(animation->try_blend_shape_track_interpolate(0, 0.4, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(0.6f));

	CHECK(animation->try_blend_shape_track_interpolate(0, 0.5, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(1.0f));

	CHECK(animation->try_blend_shape_track_interpolate(0, 0.6, &r_blend) == OK);
	CHECK(r_blend == doctest::Approx(1.0f));

	// Blend shape tracks always use linear interpolation for performance reasons.
	CHECK(animation->track_get_key_transition(0, 0) == doctest::Approx(real_t(1.0)));
	CHECK(animation->track_get_key_transition(0, 1) == doctest::Approx(real_t(1.0)));

	// This is a blend shape track, so the methods below should return errors.
	ERR_PRINT_OFF;
	CHECK(animation->value_track_interpolate(0, 0.0).is_null());
	CHECK(animation->try_position_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_rotation_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_scale_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->bezier_track_interpolate(0, 0.0) == doctest::Approx(0.0));
	ERR_PRINT_ON;
}

TEST_CASE("[Animation] Create Bezier track") {
	Ref<Animation> animation = memnew(Animation);
	const int track_index = animation->add_track(Animation::TYPE_BEZIER);
	animation->track_set_path(track_index, NodePath("Enemy:scale"));
	animation->bezier_track_insert_key(track_index, 0.0, -1.0, Vector2(-1, -1), Vector2(1, 1));
	animation->bezier_track_insert_key(track_index, 0.5, 1.0, Vector2(0, 1), Vector2(1, 0.5));

	CHECK(animation->get_track_count() == 1);
	CHECK(!animation->track_is_compressed(0));

	CHECK(animation->bezier_track_get_key_value(0, 0) == doctest::Approx(real_t(-1.0)));
	CHECK(animation->bezier_track_get_key_value(0, 1) == doctest::Approx(real_t(1.0)));

	CHECK(animation->bezier_track_interpolate(0, -0.2) == doctest::Approx(real_t(-1.0)));
	CHECK(animation->bezier_track_interpolate(0, 0.0) == doctest::Approx(real_t(-1.0)));
	CHECK(animation->bezier_track_interpolate(0, 0.2) == doctest::Approx(real_t(-0.76057207584381)));
	CHECK(animation->bezier_track_interpolate(0, 0.4) == doctest::Approx(real_t(-0.39975279569626)));
	CHECK(animation->bezier_track_interpolate(0, 0.5) == doctest::Approx(real_t(1.0)));
	CHECK(animation->bezier_track_interpolate(0, 0.6) == doctest::Approx(real_t(1.0)));

	// This is a bezier track, so the methods below should return errors.
	ERR_PRINT_OFF;
	CHECK(animation->value_track_interpolate(0, 0.0).is_null());
	CHECK(animation->try_position_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_rotation_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_scale_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	CHECK(animation->try_blend_shape_track_interpolate(0, 0.0, nullptr) == ERR_INVALID_PARAMETER);
	ERR_PRINT_ON;
}

} // namespace TestAnimation

// === Merged from tests/scene/test_animation_blend_tree.cpp ===
/*  test_animation_blend_tree.cpp                                         */
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

#include "tests/test_macros.h"



#include "scene/main/scene_tree.h"

namespace TestAnimationBlendTree {

TEST_CASE("[SceneTree][AnimationBlendTree] Create AnimationBlendTree and add AnimationNode") {
	Ref<AnimationNodeBlendTree> blend_tree;
	blend_tree.instantiate();

	// Test initial state.
	CHECK(blend_tree->has_node("output"));
	CHECK_EQ(blend_tree->get_graph_offset(), Vector2(0, 0));
	CHECK_EQ(blend_tree->get_node_list().size(), 1);

	// Test adding animation node.
	Ref<AnimationNodeBlendNode> anim_node;
	anim_node.instantiate(AnimationNodeBlendNode::NODE_ANIMATION);
	anim_node->set_animation(String("test_animation"));
	Vector2 position(100, 100);
	blend_tree->add_node("test_node", anim_node, position);

	// Test node existence.
	CHECK(blend_tree->has_node("test_node"));
	CHECK_EQ(blend_tree->get_node("test_node"), anim_node);
	CHECK_EQ(blend_tree->get_node_position("test_node"), position);

	// Test node connection on port 0.
	CHECK_EQ(blend_tree->can_connect_node("output", 0, "test_node"), AnimationNodeBlendTree::CONNECTION_OK);
	blend_tree->connect_node("output", 0, "test_node");

	const LocalVector<String> *connections = blend_tree->get_node_connection_array("output");
	CHECK_EQ(connections->size(), 1);
	CHECK_EQ(connections->operator[](0), String("test_node"));

	// Test node rename.
	blend_tree->rename_node("test_node", "renamed_node");
	CHECK_FALSE(blend_tree->has_node("test_node"));
	CHECK(blend_tree->has_node("renamed_node"));

	connections = blend_tree->get_node_connection_array("output");
	CHECK_EQ(connections->operator[](0), String("renamed_node"));

	// Test node removal.
	blend_tree->remove_node("renamed_node");
	CHECK_FALSE(blend_tree->has_node("renamed_node"));

	connections = blend_tree->get_node_connection_array("output");
	CHECK_EQ(connections->operator[](0), String());
}

} // namespace TestAnimationBlendTree

// === Merged from tests/scene/test_animation_player.cpp ===
/*  test_animation_player.cpp                                             */
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

#include "tests/test_macros.h"



#include "scene/main/scene_tree.h"

namespace TestAnimationPlayer {

TEST_CASE("[AnimationPlayer] get & set default_blend_time") {
	AnimationPlayer *animation_player = memnew(AnimationPlayer);
	animation_player->set_default_blend_time(4.0);

	CHECK(animation_player->get_default_blend_time() == doctest::Approx(4.0f));
	memdelete(animation_player);
}

TEST_CASE("[AnimationPlayer] get & set blend_time") {
	String anim1 = "animation1";
	String anim2 = "animation2";
	const Ref<Animation> animation1 = memnew(Animation);
	const Ref<Animation> animation2 = memnew(Animation);
	const Ref<AnimationLibrary> animation_library = memnew(AnimationLibrary);
	animation_library->add_animation(anim1, animation1);
	animation_library->add_animation(anim2, animation2);

	AnimationPlayer *animation_player = memnew(AnimationPlayer);
	animation_player->add_animation_library("", animation_library);

	animation_player->set_blend_time(anim1, anim2, 4.0);
	CHECK(animation_player->get_blend_time(anim1, anim2) == doctest::Approx(4.0f));
	memdelete(animation_player);
}

} // namespace TestAnimationPlayer

// === Merged from tests/scene/test_audio_stream_wav.cpp ===
/*  test_audio_stream_wav.cpp                                             */
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

#include "tests/test_macros.h"



#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/typedefs.h"
#include "servers/display/display_server.h"
#include "tests/test_macros.h"

namespace TestAudioStreamWAV {

// Default wav rate for test cases.
constexpr float WAV_RATE = 44100;
/* Default wav count for test cases. 1 second of audio is used so that the file can be listened
to manually if needed. */
constexpr int WAV_COUNT = WAV_RATE;

float gen_wav(float frequency, float wav_rate, int wav_number) {
	// formula for generating a sin wave with given frequency.
	return Math::sin((Math::TAU * frequency / wav_rate) * wav_number);
}

/* Generates a 440Hz sin wave in channel 0 (mono channel or left stereo channel)
 * and a 261.63Hz wave in channel 1 (right stereo channel).
 * These waves correspond to the music notes A4 and C4 respectively.
 */
Vector<uint8_t> gen_pcm8_test(float wav_rate, int wav_count, bool stereo) {
	Vector<uint8_t> buffer;
	buffer.resize(stereo ? wav_count * 2 : wav_count);

	uint8_t *write_ptr = buffer.ptrw();
	for (int i = 0; i < buffer.size(); i++) {
		float wav;
		if (stereo) {
			if (i % 2 == 0) {
				wav = gen_wav(440, wav_rate, i / 2);
			} else {
				wav = gen_wav(261.63, wav_rate, i / 2);
			}
		} else {
			wav = gen_wav(440, wav_rate, i);
		}

		// Map sin wave to full range of 8-bit values.
		uint8_t wav_8bit = Math::fast_ftoi(((wav + 1) / 2) * UINT8_MAX);
		// Unlike the .wav format, AudioStreamWAV expects signed 8-bit wavs.
		uint8_t wav_8bit_signed = wav_8bit - (INT8_MAX + 1);
		write_ptr[i] = wav_8bit_signed;
	}

	return buffer;
}

// Same as gen_pcm8_test but with 16-bit wavs.
Vector<uint8_t> gen_pcm16_test(float wav_rate, int wav_count, bool stereo) {
	Vector<uint8_t> buffer;
	buffer.resize(stereo ? wav_count * 4 : wav_count * 2);

	uint8_t *write_ptr = buffer.ptrw();
	for (int i = 0; i < buffer.size() / 2; i++) {
		float wav;
		if (stereo) {
			if (i % 2 == 0) {
				wav = gen_wav(440, wav_rate, i / 2);
			} else {
				wav = gen_wav(261.63, wav_rate, i / 2);
			}
		} else {
			wav = gen_wav(440, wav_rate, i);
		}

		// Map sin wave to full range of 16-bit values.
		uint16_t wav_16bit = Math::fast_ftoi(((wav + 1) / 2) * UINT16_MAX);
		// The .wav format expects wavs larger than 8 bits to be signed.
		uint16_t wav_16bit_signed = wav_16bit - (INT16_MAX + 1);
		encode_uint16(wav_16bit_signed, write_ptr + (i * 2));
	}

	return buffer;
}

void run_test(String file_name, AudioStreamWAV::Format data_format, bool stereo, float wav_rate, float wav_count) {
	String save_path = TestUtils::get_temp_path(file_name);

	Vector<uint8_t> test_data;
	if (data_format == AudioStreamWAV::FORMAT_8_BITS) {
		test_data = gen_pcm8_test(wav_rate, wav_count, stereo);
	} else {
		test_data = gen_pcm16_test(wav_rate, wav_count, stereo);
	}

	Ref<AudioStreamWAV> stream = memnew(AudioStreamWAV);
	stream->set_mix_rate(wav_rate);
	CHECK(stream->get_mix_rate() == wav_rate);

	stream->set_format(data_format);
	CHECK(stream->get_format() == data_format);

	stream->set_stereo(stereo);
	CHECK(stream->is_stereo() == stereo);

	stream->set_data(test_data);
	CHECK(stream->get_data() == test_data);

	SUBCASE("Stream length is computed properly") {
		CHECK(stream->get_length() == doctest::Approx(double(wav_count / wav_rate)));
	}

	SUBCASE("Stream can be saved as .wav") {
		REQUIRE(stream->save_to_wav(save_path) == OK);

		Error error;
		Ref<FileAccess> wav_file = FileAccess::open(save_path, FileAccess::READ, &error);
		REQUIRE(error == OK);

		Dictionary options;
		Ref<AudioStreamWAV> loaded_stream = AudioStreamWAV::load_from_file(save_path, options);

		CHECK(loaded_stream->get_format() == stream->get_format());
		CHECK(loaded_stream->get_loop_mode() == stream->get_loop_mode());
		CHECK(loaded_stream->get_loop_begin() == stream->get_loop_begin());
		CHECK(loaded_stream->get_loop_end() == stream->get_loop_end());
		CHECK(loaded_stream->get_mix_rate() == stream->get_mix_rate());
		CHECK(loaded_stream->is_stereo() == stream->is_stereo());
		CHECK(loaded_stream->get_length() == stream->get_length());
		CHECK(loaded_stream->is_monophonic() == stream->is_monophonic());
		CHECK(loaded_stream->get_data() == stream->get_data());
	}
}

TEST_CASE("[Audio][AudioStreamWAV] Mono PCM8 format") {
	run_test("test_pcm8_mono.wav", AudioStreamWAV::FORMAT_8_BITS, false, WAV_RATE, WAV_COUNT);
}

TEST_CASE("[Audio][AudioStreamWAV] Mono PCM16 format") {
	run_test("test_pcm16_mono.wav", AudioStreamWAV::FORMAT_16_BITS, false, WAV_RATE, WAV_COUNT);
}

TEST_CASE("[Audio][AudioStreamWAV] Stereo PCM8 format") {
	run_test("test_pcm8_stereo.wav", AudioStreamWAV::FORMAT_8_BITS, true, WAV_RATE, WAV_COUNT);
}

TEST_CASE("[Audio][AudioStreamWAV] Stereo PCM16 format") {
	run_test("test_pcm16_stereo.wav", AudioStreamWAV::FORMAT_16_BITS, true, WAV_RATE, WAV_COUNT);
}

TEST_CASE("[Audio][AudioStreamWAV] Alternate mix rate") {
	run_test("test_pcm16_stereo_38000Hz.wav", AudioStreamWAV::FORMAT_16_BITS, true, 38000, 38000);
}

TEST_CASE("[Audio][AudioStreamWAV] save_to_wav() adds '.wav' file extension automatically") {
	String save_path = TestUtils::get_temp_path("test_wav_extension");
	Vector<uint8_t> test_data = gen_pcm8_test(WAV_RATE, WAV_COUNT, false);
	Ref<AudioStreamWAV> stream = memnew(AudioStreamWAV);
	stream->set_data(test_data);

	REQUIRE(stream->save_to_wav(save_path) == OK);
	Error error;
	Ref<FileAccess> wav_file = FileAccess::open(save_path + ".wav", FileAccess::READ, &error);
	CHECK(error == OK);
}

TEST_CASE("[Audio][AudioStreamWAV] Default values") {
	Ref<AudioStreamWAV> stream = memnew(AudioStreamWAV);
	CHECK(stream->get_format() == AudioStreamWAV::FORMAT_8_BITS);
	CHECK(stream->get_loop_mode() == AudioStreamWAV::LOOP_DISABLED);
	CHECK(stream->get_loop_begin() == 0);
	CHECK(stream->get_loop_end() == 0);
	CHECK(stream->get_mix_rate() == 44100);
	CHECK(stream->is_stereo() == false);
	CHECK(stream->get_length() == 0);
	CHECK(stream->is_monophonic() == false);
	CHECK(stream->get_data() == Vector<uint8_t>{});
}

TEST_CASE("[Audio][AudioStreamWAV] Save empty file") {
	run_test("test_empty.wav", AudioStreamWAV::FORMAT_8_BITS, false, WAV_RATE, 0);
}

TEST_CASE("[Audio][AudioStreamWAV] Saving IMA ADPCM is not supported") {
	String save_path = TestUtils::get_temp_path("test_adpcm.wav");
	Ref<AudioStreamWAV> stream = memnew(AudioStreamWAV);
	stream->set_format(AudioStreamWAV::FORMAT_IMA_ADPCM);
	ERR_PRINT_OFF;
	CHECK(stream->save_to_wav(save_path) == ERR_UNAVAILABLE);
	ERR_PRINT_ON;
}

} // namespace TestAudioStreamWAV

// === Merged from tests/scene/test_bit_map.cpp ===
/*  test_bit_map.cpp                                                      */
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

#include "tests/test_macros.h"



#include "servers/rendering/renderer.h"

namespace TestBitMap {

void reset_bit_map(BitMap &p_bm) {
	Size2i size = p_bm.get_size();
	p_bm.set_bit_rect(Rect2i(0, 0, size.width, size.height), false);
}

TEST_CASE("[BitMap] Create bit map") {
	Size2i dim{ 256, 512 };
	BitMap bit_map{};
	bit_map.create(dim);
	CHECK(bit_map.get_size() == Size2i(256, 512));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 0, "This will go through the entire bitmask inside of bitmap, thus hopefully checking if the bitmask was correctly set up.");

	ERR_PRINT_OFF

	dim = Size2i(0, 256);
	bit_map.create(dim);
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 512), "We should still have the same dimensions as before, because the new dimension is invalid.");

	dim = Size2i(512, 0);
	bit_map.create(dim);
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 512), "We should still have the same dimensions as before, because the new dimension is invalid.");

	dim = Size2i(46341, 46341);
	bit_map.create(dim);
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 512), "We should still have the same dimensions as before, because the new dimension is too large (46341*46341=2147488281).");

	ERR_PRINT_ON
}

TEST_CASE("[BitMap] Create bit map from image alpha") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};
	bit_map.create(dim);

	ERR_PRINT_OFF

	const Ref<Image> null_img = nullptr;
	bit_map.create_from_image_alpha(null_img);
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 256), "Bitmap should have its old values because bitmap creation from a nullptr should fail.");

	Ref<Image> empty_img;
	empty_img.instantiate();
	bit_map.create_from_image_alpha(empty_img);
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 256), "Bitmap should have its old values because bitmap creation from an empty image should fail.");

	Ref<Image> wrong_format_img = Image::create_empty(3, 3, false, Image::Format::FORMAT_DXT1);
	bit_map.create_from_image_alpha(wrong_format_img);
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 256), "Bitmap should have its old values because converting from a compressed image should fail.");

	ERR_PRINT_ON

	Ref<Image> img = Image::create_empty(3, 3, false, Image::Format::FORMAT_RGBA8);
	img->set_pixel(0, 0, Color(0, 0, 0, 0));
	img->set_pixel(0, 1, Color(0, 0, 0, 0.09f));
	img->set_pixel(0, 2, Color(0, 0, 0, 0.25f));
	img->set_pixel(1, 0, Color(0, 0, 0, 0.5f));
	img->set_pixel(1, 1, Color(0, 0, 0, 0.75f));
	img->set_pixel(1, 2, Color(0, 0, 0, 0.99f));
	img->set_pixel(2, 0, Color(0, 0, 0, 1.f));

	// Check different threshold values.
	bit_map.create_from_image_alpha(img);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 5, "There are 5 values in the image that are smaller than the default threshold of 0.1.");

	bit_map.create_from_image_alpha(img, 0.08f);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 6, "There are 6 values in the image that are smaller than the threshold of 0.08.");

	bit_map.create_from_image_alpha(img, 1);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 0, "There are no values in the image that are smaller than the threshold of 1, there is one value equal to 1, but we check for inequality only.");
}

TEST_CASE("[BitMap] Set bit") {
	Size2i dim{ 256, 256 };
	BitMap bit_map{};

	// Setting a point before a bit map is created should not crash, because there are checks to see if we are out of bounds.
	ERR_PRINT_OFF

	bit_map.set_bitv(Point2i(128, 128), true);

	bit_map.create(dim);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 0, "All values should be initialized to false.");
	bit_map.set_bitv(Point2i(128, 128), true);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 1, "One bit should be set to true.");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(128, 128)) == true, "The bit at (128,128) should be set to true");

	bit_map.set_bitv(Point2i(128, 128), false);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 0, "The bit should now be set to false again");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(128, 128)) == false, "The bit at (128,128) should now be set to false again");

	bit_map.create(dim);
	bit_map.set_bitv(Point2i(512, 512), true);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 0, "Nothing should change as we were trying to edit a bit outside of the correct range.");

	ERR_PRINT_ON
}

TEST_CASE("[BitMap] Get bit") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};

	ERR_PRINT_OFF

	CHECK_MESSAGE(bit_map.get_bitv(Point2i(128, 128)) == false, "Trying to access a bit outside of the BitMap's range should always return false");

	bit_map.create(dim);
	CHECK(bit_map.get_bitv(Point2i(128, 128)) == false);

	bit_map.set_bit_rect(Rect2i(-1, -1, 257, 257), true);

	// Checking that range is [0, 256).
	CHECK(bit_map.get_bitv(Point2i(-1, 0)) == false);
	CHECK(bit_map.get_bitv(Point2i(0, 0)) == true);
	CHECK(bit_map.get_bitv(Point2i(128, 128)) == true);
	CHECK(bit_map.get_bitv(Point2i(255, 255)) == true);
	CHECK(bit_map.get_bitv(Point2i(256, 256)) == false);
	CHECK(bit_map.get_bitv(Point2i(257, 257)) == false);

	ERR_PRINT_ON
}

TEST_CASE("[BitMap] Set bit rect") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};

	// Although we have not setup the BitMap yet, this should not crash because we get an empty intersection inside of the method.
	bit_map.set_bit_rect(Rect2i{ 0, 0, 128, 128 }, true);

	bit_map.create(dim);
	CHECK(bit_map.get_true_bit_count() == 0);

	bit_map.set_bit_rect(Rect2i{ 0, 0, 256, 256 }, true);
	CHECK(bit_map.get_true_bit_count() == 65536);

	reset_bit_map(bit_map);

	// Checking out of bounds handling.

	ERR_PRINT_OFF

	bit_map.set_bit_rect(Rect2i{ 128, 128, 256, 256 }, true);
	CHECK(bit_map.get_true_bit_count() == 16384);

	reset_bit_map(bit_map);

	bit_map.set_bit_rect(Rect2i{ -128, -128, 256, 256 }, true);
	CHECK(bit_map.get_true_bit_count() == 16384);

	reset_bit_map(bit_map);

	bit_map.set_bit_rect(Rect2i{ -128, -128, 512, 512 }, true);
	CHECK(bit_map.get_true_bit_count() == 65536);

	ERR_PRINT_ON
}

TEST_CASE("[BitMap] Get true bit count") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};

	CHECK(bit_map.get_true_bit_count() == 0);

	bit_map.create(dim);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 0, "Uninitialized bit map should have no true bits");
	bit_map.set_bit_rect(Rect2i{ 0, 0, 256, 256 }, true);
	CHECK(bit_map.get_true_bit_count() == 65536);
	bit_map.set_bitv(Point2i{ 0, 0 }, false);
	CHECK(bit_map.get_true_bit_count() == 65535);
	bit_map.set_bit_rect(Rect2i{ 0, 0, 256, 256 }, false);
	CHECK(bit_map.get_true_bit_count() == 0);
}

TEST_CASE("[BitMap] Get size") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};

	CHECK_MESSAGE(bit_map.get_size() == Size2i(0, 0), "Uninitialized bit map should have a size of 0x0");

	bit_map.create(dim);
	CHECK(bit_map.get_size() == Size2i(256, 256));

	ERR_PRINT_OFF
	bit_map.create(Size2i(-1, 0));
	ERR_PRINT_ON
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 256), "Invalid size should not be accepted by create");

	bit_map.create(Size2i(256, 128));
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 128), "Bitmap should have updated size");
}

TEST_CASE("[BitMap] Resize") {
	const Size2i dim{ 128, 128 };
	BitMap bit_map{};

	bit_map.resize(dim);
	CHECK(bit_map.get_size() == dim);

	bit_map.create(dim);
	bit_map.set_bit_rect(Rect2i(0, 0, 10, 10), true);
	bit_map.set_bit_rect(Rect2i(118, 118, 10, 10), true);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 200, "There should be 100 bits in the top left corner, and 100 bits in the bottom right corner");
	bit_map.resize(Size2i(64, 64));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 50, "There should be 25 bits in the top left corner, and 25 bits in the bottom right corner");

	bit_map.create(dim);
	ERR_PRINT_OFF
	bit_map.resize(Size2i(-1, 128));
	ERR_PRINT_ON
	CHECK_MESSAGE(bit_map.get_size() == Size2i(128, 128), "When an invalid size is given the bit map will keep its size");

	bit_map.create(dim);
	bit_map.set_bit_rect(Rect2i(0, 0, 10, 10), true);
	bit_map.set_bit_rect(Rect2i(118, 118, 10, 10), true);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 200, "There should be 100 bits in the top left corner, and 100 bits in the bottom right corner");
	bit_map.resize(Size2i(256, 256));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 800, "There should still be 100 bits in the bottom right corner, and all new bits should be initialized to false");
	CHECK_MESSAGE(bit_map.get_size() == Size2i(256, 256), "The bitmap should now be 256x256");
}

TEST_CASE("[BitMap] Grow and shrink mask") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};
	ERR_PRINT_OFF
	bit_map.grow_mask(100, Rect2i(0, 0, 128, 128)); // Check if method does not crash when working with an uninitialized bit map.
	ERR_PRINT_ON
	CHECK_MESSAGE(bit_map.get_size() == Size2i(0, 0), "Size should still be equal to 0x0");

	bit_map.create(dim);

	bit_map.set_bit_rect(Rect2i(96, 96, 64, 64), true);

	CHECK_MESSAGE(bit_map.get_true_bit_count() == 4096, "Creating a square of 64x64 should be 4096 bits");
	bit_map.grow_mask(0, Rect2i(0, 0, 256, 256));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 4096, "Growing with size of 0 should not change any bits");

	reset_bit_map(bit_map);

	bit_map.set_bit_rect(Rect2i(96, 96, 64, 64), true);

	CHECK_MESSAGE(bit_map.get_bitv(Point2i(95, 128)) == false, "Bits just outside of the square should not be set");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(160, 128)) == false, "Bits just outside of the square should not be set");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(128, 95)) == false, "Bits just outside of the square should not be set");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(128, 160)) == false, "Bits just outside of the square should not be set");
	bit_map.grow_mask(1, Rect2i(0, 0, 256, 256));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 4352, "We should have 4*64 (perimeter of square) more bits set to true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(95, 128)) == true, "Bits that were just outside of the square should now be set to true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(160, 128)) == true, "Bits that were just outside of the square should now be set to true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(128, 95)) == true, "Bits that were just outside of the square should now be set to true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(128, 160)) == true, "Bits that were just outside of the square should now be set to true");

	reset_bit_map(bit_map);

	bit_map.set_bit_rect(Rect2i(127, 127, 1, 1), true);

	CHECK(bit_map.get_true_bit_count() == 1);
	bit_map.grow_mask(32, Rect2i(0, 0, 256, 256));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 3209, "Creates a circle around the initial bit with a radius of 32 bits. Any bit that has a distance within this radius will be set to true");

	reset_bit_map(bit_map);

	bit_map.set_bit_rect(Rect2i(127, 127, 1, 1), true);
	for (int i = 0; i < 32; i++) {
		bit_map.grow_mask(1, Rect2i(0, 0, 256, 256));
	}
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 2113, "Creates a diamond around the initial bit with diagonals that are 65 bits long.");

	reset_bit_map(bit_map);

	bit_map.set_bit_rect(Rect2i(123, 123, 10, 10), true);

	CHECK(bit_map.get_true_bit_count() == 100);
	bit_map.grow_mask(-11, Rect2i(0, 0, 256, 256));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 0, "Shrinking by more than the width of the square should totally remove it.");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(96, 96, 64, 64), true);

	CHECK_MESSAGE(bit_map.get_bitv(Point2i(96, 129)) == true, "Bits on the edge of the square should be true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(159, 129)) == true, "Bits on the edge of the square should be true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(129, 96)) == true, "Bits on the edge of the square should be true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(129, 159)) == true, "Bits on the edge of the square should be true");
	bit_map.grow_mask(-1, Rect2i(0, 0, 256, 256));
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 3844, "Shrinking by 1 should set 4*63=252 bits to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(96, 129)) == false, "Bits that were on the edge of the square should now be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(159, 129)) == false, "Bits that were on the edge of the square should now be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(129, 96)) == false, "Bits that were on the edge of the square should now be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(129, 159)) == false, "Bits that were on the edge of the square should now be set to false");

	reset_bit_map(bit_map);

	bit_map.set_bit_rect(Rect2i(125, 125, 1, 6), true);
	bit_map.set_bit_rect(Rect2i(130, 125, 1, 6), true);
	bit_map.set_bit_rect(Rect2i(125, 130, 6, 1), true);

	CHECK(bit_map.get_true_bit_count() == 16);
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(125, 131)) == false, "Bits that are on the edge of the shape should be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(131, 131)) == false, "Bits that are on the edge of the shape should be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(125, 124)) == false, "Bits that are on the edge of the shape should be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(130, 124)) == false, "Bits that are on the edge of the shape should be set to false");
	bit_map.grow_mask(1, Rect2i(0, 0, 256, 256));
	CHECK(bit_map.get_true_bit_count() == 48);
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(125, 131)) == true, "Bits that were on the edge of the shape should now be set to true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(131, 130)) == true, "Bits that were on the edge of the shape should now be set to true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(125, 124)) == true, "Bits that were on the edge of the shape should now be set to true");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(130, 124)) == true, "Bits that were on the edge of the shape should now be set to true");

	CHECK_MESSAGE(bit_map.get_bitv(Point2i(124, 124)) == false, "Bits that are on the edge of the shape should be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(126, 124)) == false, "Bits that are on the edge of the shape should be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(124, 131)) == false, "Bits that are on the edge of the shape should be set to false");
	CHECK_MESSAGE(bit_map.get_bitv(Point2i(131, 131)) == false, "Bits that are on the edge of the shape should be set to false");
}

TEST_CASE("[BitMap] Blit") {
	Point2i blit_pos{ 128, 128 };
	Point2i bit_map_size{ 256, 256 };
	Point2i blit_size{ 32, 32 };

	BitMap bit_map{};
	Ref<BitMap> blit_bit_map{};

	// Testing null reference to blit bit map.
	ERR_PRINT_OFF
	bit_map.blit(blit_pos, blit_bit_map);
	ERR_PRINT_ON

	blit_bit_map.instantiate();

	// Testing if uninitialized blit bit map and uninitialized bit map does not crash
	bit_map.blit(blit_pos, blit_bit_map);

	// Testing if uninitialized bit map does not crash
	blit_bit_map->create(blit_size);
	bit_map.blit(blit_pos, blit_bit_map);

	// Testing if uninitialized bit map does not crash
	blit_bit_map.unref();
	blit_bit_map.instantiate();
	CHECK_MESSAGE(blit_bit_map->get_size() == Point2i(0, 0), "Size should be cleared by unref and instance calls.");
	bit_map.create(bit_map_size);
	bit_map.blit(Point2i(128, 128), blit_bit_map);

	// Testing if both initialized does not crash.
	blit_bit_map->create(blit_size);
	bit_map.blit(blit_pos, blit_bit_map);

	bit_map.set_bit_rect(Rect2i{ 127, 127, 3, 3 }, true);
	CHECK(bit_map.get_true_bit_count() == 9);
	bit_map.blit(Point2i(112, 112), blit_bit_map);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 9, "No bits should have been changed, as the blit bit map only contains falses");

	bit_map.create(bit_map_size);
	blit_bit_map->create(blit_size);
	blit_bit_map->set_bit_rect(Rect2i(15, 15, 3, 3), true);
	CHECK(blit_bit_map->get_true_bit_count() == 9);

	CHECK(bit_map.get_true_bit_count() == 0);
	bit_map.blit(Point2i(112, 112), blit_bit_map);
	CHECK_MESSAGE(bit_map.get_true_bit_count() == 9, "All true bits should have been moved to the bit map");
	for (int x = 127; x < 129; ++x) {
		for (int y = 127; y < 129; ++y) {
			CHECK_MESSAGE(bit_map.get_bitv(Point2i(x, y)) == true, "All true bits should have been moved to the bit map");
		}
	}
}

TEST_CASE("[BitMap] Convert to image") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};
	Ref<Image> img;

	ERR_PRINT_OFF
	img = bit_map.convert_to_image();
	ERR_PRINT_ON
	CHECK_MESSAGE(img.is_valid(), "We should receive a valid Image Object even if BitMap is not created yet");
	CHECK_MESSAGE(img->get_format() == Image::FORMAT_L8, "We should receive a valid Image Object even if BitMap is not created yet");
	CHECK_MESSAGE(img->get_size() == (Size2i(0, 0)), "Image should have no width or height, because BitMap has not yet been created");

	bit_map.create(dim);
	img = bit_map.convert_to_image();
	CHECK_MESSAGE(img->get_size() == dim, "Image should have the same dimensions as the BitMap");
	CHECK_MESSAGE(img->get_pixel(0, 0).is_equal_approx(Color(0, 0, 0)), "BitMap is initialized to all 0's, so Image should be all black");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(0, 0, 128, 128), true);
	img = bit_map.convert_to_image();
	CHECK_MESSAGE(img->get_pixel(0, 0).is_equal_approx(Color(1, 1, 1)), "BitMap's top-left quadrant is all 1's, so Image should be white");
	CHECK_MESSAGE(img->get_pixel(255, 255).is_equal_approx(Color(0, 0, 0)), "All other quadrants were 0's, so these should be black");
}

TEST_CASE("[BitMap] Clip to polygon") {
	const Size2i dim{ 256, 256 };
	BitMap bit_map{};
	Vector<Vector<Vector2>> polygons;

	ERR_PRINT_OFF
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 128, 128));
	ERR_PRINT_ON
	CHECK_MESSAGE(polygons.size() == 0, "We should have no polygons, because the BitMap was not initialized");

	bit_map.create(dim);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 128, 128));
	CHECK_MESSAGE(polygons.size() == 0, "We should have no polygons, because the BitMap was all 0's");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(0, 0, 64, 64), true);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 128, 128));
	CHECK_MESSAGE(polygons.size() == 1, "We should have exactly 1 polygon");
	CHECK_MESSAGE(polygons[0].size() == 4, "The polygon should have exactly 4 points");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(0, 0, 32, 32), true);
	bit_map.set_bit_rect(Rect2i(64, 64, 32, 32), true);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 128, 128));
	CHECK_MESSAGE(polygons.size() == 2, "We should have exactly 2 polygons");
	CHECK_MESSAGE(polygons[0].size() == 4, "The polygon should have exactly 4 points");
	CHECK_MESSAGE(polygons[1].size() == 4, "The polygon should have exactly 4 points");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(124, 112, 8, 32), true);
	bit_map.set_bit_rect(Rect2i(112, 124, 32, 8), true);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 256, 256));
	CHECK_MESSAGE(polygons.size() == 1, "We should have exactly 1 polygon");
	CHECK_MESSAGE(polygons[0].size() == 12, "The polygon should have exactly 12 points");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(124, 112, 8, 32), true);
	bit_map.set_bit_rect(Rect2i(112, 124, 32, 8), true);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 128, 128));
	CHECK_MESSAGE(polygons.size() == 1, "We should have exactly 1 polygon");
	CHECK_MESSAGE(polygons[0].size() == 6, "The polygon should have exactly 6 points");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(0, 0, 64, 64), true);
	bit_map.set_bit_rect(Rect2i(64, 64, 64, 64), true);
	bit_map.set_bit_rect(Rect2i(192, 128, 64, 64), true);
	bit_map.set_bit_rect(Rect2i(128, 192, 64, 64), true);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 256, 256));
	CHECK_MESSAGE(polygons.size() == 4, "We should have exactly 4 polygons");
	CHECK_MESSAGE(polygons[0].size() == 4, "The polygon should have exactly 4 points");
	CHECK_MESSAGE(polygons[1].size() == 4, "The polygon should have exactly 4 points");
	CHECK_MESSAGE(polygons[2].size() == 4, "The polygon should have exactly 4 points");
	CHECK_MESSAGE(polygons[3].size() == 4, "The polygon should have exactly 4 points");

	reset_bit_map(bit_map);
	bit_map.set_bit(0, 0, true);
	bit_map.set_bit(2, 0, true);
	bit_map.set_bit_rect(Rect2i(1, 1, 1, 2), true);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 3, 3));
	CHECK_MESSAGE(polygons.size() == 3, "We should have exactly 3 polygons");
	CHECK_MESSAGE(polygons[0].size() == 4, "The polygon should have exactly 4 points");
	CHECK_MESSAGE(polygons[1].size() == 4, "The polygon should have exactly 4 points");
	CHECK_MESSAGE(polygons[2].size() == 4, "The polygon should have exactly 4 points");

	reset_bit_map(bit_map);
	bit_map.set_bit_rect(Rect2i(0, 0, 2, 1), true);
	bit_map.set_bit_rect(Rect2i(0, 2, 3, 1), true);
	bit_map.set_bit(0, 1, true);
	bit_map.set_bit(2, 1, true);
	polygons = bit_map.clip_opaque_to_polygons(Rect2i(0, 0, 4, 4));
	CHECK_MESSAGE(polygons.size() == 1, "We should have exactly 1 polygon");
	CHECK_MESSAGE(polygons[0].size() == 6, "The polygon should have exactly 6 points");
}

} // namespace TestBitMap

// === Merged from tests/scene/test_curve.cpp ===
/*  test_curve.cpp                                                        */
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

#include "tests/test_macros.h"



#include "core/typedefs.h"
#include "servers/rendering/renderer.h"

namespace TestCurve {

TEST_CASE("[Curve] Default curve") {
	const Ref<Curve> curve = memnew(Curve);

	CHECK_MESSAGE(
			curve->get_point_count() == 0,
			"Default curve should contain the expected number of points.");
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample(0)),
			"Default curve should return the expected value at offset 0.0.");
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample(0.5)),
			"Default curve should return the expected value at offset 0.5.");
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample(1)),
			"Default curve should return the expected value at offset 1.0.");
}

TEST_CASE("[Curve] Custom unit curve with free tangents") {
	Ref<Curve> curve = memnew(Curve);
	// "Sawtooth" curve with an open ending towards the 1.0 offset.
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(0.25, 1));
	curve->add_point(Vector2(0.5, 0));
	curve->add_point(Vector2(0.75, 1));
	curve->set_bake_resolution(11);

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_left_tangent(0)),
			"get_point_left_tangent() should return the expected value for point index 0.");
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_right_tangent(0)),
			"get_point_right_tangent() should return the expected value for point index 0.");
	CHECK_MESSAGE(
			curve->get_point_left_mode(0) == Curve::TangentMode::TANGENT_FREE,
			"get_point_left_mode() should return the expected value for point index 0.");
	CHECK_MESSAGE(
			curve->get_point_right_mode(0) == Curve::TangentMode::TANGENT_FREE,
			"get_point_right_mode() should return the expected value for point index 0.");

	CHECK_MESSAGE(
			curve->get_point_count() == 4,
			"Custom free curve should contain the expected number of points.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample(-0.1)),
			"Custom free curve should return the expected value at offset -0.1.");
	CHECK_MESSAGE(
			curve->sample(0.1) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected value at offset 0.1.");
	CHECK_MESSAGE(
			curve->sample(0.4) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected value at offset 0.4.");
	CHECK_MESSAGE(
			curve->sample(0.7) == doctest::Approx((real_t)0.896),
			"Custom free curve should return the expected value at offset 0.7.");
	CHECK_MESSAGE(
			curve->sample(1) == doctest::Approx(1),
			"Custom free curve should return the expected value at offset 1.");
	CHECK_MESSAGE(
			curve->sample(2) == doctest::Approx(1),
			"Custom free curve should return the expected value at offset 2.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample_baked(-0.1)),
			"Custom free curve should return the expected baked value at offset -0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.1) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected baked value at offset 0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.4) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected baked value at offset 0.4.");
	CHECK_MESSAGE(
			curve->sample_baked(0.7) == doctest::Approx((real_t)0.896),
			"Custom free curve should return the expected baked value at offset 0.7.");
	CHECK_MESSAGE(
			curve->sample_baked(1) == doctest::Approx(1),
			"Custom free curve should return the expected baked value at offset 1.");
	CHECK_MESSAGE(
			curve->sample_baked(2) == doctest::Approx(1),
			"Custom free curve should return the expected baked value at offset 2.");

	curve->remove_point(1);
	CHECK_MESSAGE(
			curve->sample(0.1) == doctest::Approx(0),
			"Custom free curve should return the expected value at offset 0.1 after removing point at index 1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.1) == doctest::Approx(0),
			"Custom free curve should return the expected baked value at offset 0.1 after removing point at index 1.");

	curve->clear_points();
	CHECK_MESSAGE(
			curve->sample(0.6) == doctest::Approx(0),
			"Custom free curve should return the expected value at offset 0.6 after clearing all points.");
	CHECK_MESSAGE(
			curve->sample_baked(0.6) == doctest::Approx(0),
			"Custom free curve should return the expected baked value at offset 0.6 after clearing all points.");
}

TEST_CASE("[Curve] Custom non-unit curve with free tangents") {
	Ref<Curve> curve = memnew(Curve);
	curve->set_min_domain(-100.0);
	curve->set_max_domain(100.0);
	// "Sawtooth" curve with an open ending towards the 100 offset.
	curve->add_point(Vector2(-100, 0));
	curve->add_point(Vector2(-50, 1));
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(50, 1));
	curve->set_bake_resolution(11);

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_left_tangent(0)),
			"get_point_left_tangent() should return the expected value for point index 0.");
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_right_tangent(0)),
			"get_point_right_tangent() should return the expected value for point index 0.");
	CHECK_MESSAGE(
			curve->get_point_left_mode(0) == Curve::TangentMode::TANGENT_FREE,
			"get_point_left_mode() should return the expected value for point index 0.");
	CHECK_MESSAGE(
			curve->get_point_right_mode(0) == Curve::TangentMode::TANGENT_FREE,
			"get_point_right_mode() should return the expected value for point index 0.");

	CHECK_MESSAGE(
			curve->get_point_count() == 4,
			"Custom free curve should contain the expected number of points.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample(-200)),
			"Custom free curve should return the expected value at offset -200.");
	CHECK_MESSAGE(
			curve->sample(0.1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected value at offset equivalent to a unit curve's 0.1.");
	CHECK_MESSAGE(
			curve->sample(0.4 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected value at offset equivalent to a unit curve's 0.4.");
	CHECK_MESSAGE(
			curve->sample(0.7 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.896),
			"Custom free curve should return the expected value at offset equivalent to a unit curve's 0.7.");
	CHECK_MESSAGE(
			curve->sample(1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom free curve should return the expected value at offset equivalent to a unit curve's 1.");
	CHECK_MESSAGE(
			curve->sample(2 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom free curve should return the expected value at offset equivalent to a unit curve's 2.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample_baked(-200)),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's -0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's 0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.4 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.352),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's 0.4.");
	CHECK_MESSAGE(
			curve->sample_baked(0.7 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.896),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's 0.7.");
	CHECK_MESSAGE(
			curve->sample_baked(1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's 1.");
	CHECK_MESSAGE(
			curve->sample_baked(2 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's 2.");

	curve->remove_point(1);
	CHECK_MESSAGE(
			curve->sample(0.1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(0),
			"Custom free curve should return the expected value at offset equivalent to a unit curve's 0.1 after removing point at index 1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(0),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's 0.1 after removing point at index 1.");

	curve->clear_points();
	CHECK_MESSAGE(
			curve->sample(0.6 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(0),
			"Custom free curve should return the expected value at offset 0.6 after clearing all points.");
	CHECK_MESSAGE(
			curve->sample_baked(0.6 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(0),
			"Custom free curve should return the expected baked value at offset 0.6 after clearing all points.");
}

TEST_CASE("[Curve] Custom unit curve with linear tangents") {
	Ref<Curve> curve = memnew(Curve);
	// "Sawtooth" curve with an open ending towards the 1.0 offset.
	curve->add_point(Vector2(0, 0), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);
	curve->add_point(Vector2(0.25, 1), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);
	curve->add_point(Vector2(0.5, 0), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);
	curve->add_point(Vector2(0.75, 1), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);

	CHECK_MESSAGE(
			curve->get_point_left_tangent(3) == doctest::Approx(4),
			"get_point_left_tangent() should return the expected value for point index 3.");
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_right_tangent(3)),
			"get_point_right_tangent() should return the expected value for point index 3.");
	CHECK_MESSAGE(
			curve->get_point_left_mode(3) == Curve::TangentMode::TANGENT_LINEAR,
			"get_point_left_mode() should return the expected value for point index 3.");
	CHECK_MESSAGE(
			curve->get_point_right_mode(3) == Curve::TangentMode::TANGENT_LINEAR,
			"get_point_right_mode() should return the expected value for point index 3.");

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_right_tangent(300)),
			"get_point_right_tangent() should return the expected value for invalid point index 300.");
	CHECK_MESSAGE(
			curve->get_point_left_mode(-12345) == Curve::TangentMode::TANGENT_FREE,
			"get_point_left_mode() should return the expected value for invalid point index -12345.");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			curve->get_point_count() == 4,
			"Custom linear curve should contain the expected number of points.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample(-0.1)),
			"Custom linear curve should return the expected value at offset -0.1.");
	CHECK_MESSAGE(
			curve->sample(0.1) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected value at offset 0.1.");
	CHECK_MESSAGE(
			curve->sample(0.4) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected value at offset 0.4.");
	CHECK_MESSAGE(
			curve->sample(0.7) == doctest::Approx((real_t)0.8),
			"Custom linear curve should return the expected value at offset 0.7.");
	CHECK_MESSAGE(
			curve->sample(1) == doctest::Approx(1),
			"Custom linear curve should return the expected value at offset 1.0.");
	CHECK_MESSAGE(
			curve->sample(2) == doctest::Approx(1),
			"Custom linear curve should return the expected value at offset 2.0.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample_baked(-0.1)),
			"Custom linear curve should return the expected baked value at offset -0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.1) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected baked value at offset 0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.4) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected baked value at offset 0.4.");
	CHECK_MESSAGE(
			curve->sample_baked(0.7) == doctest::Approx((real_t)0.8),
			"Custom linear curve should return the expected baked value at offset 0.7.");
	CHECK_MESSAGE(
			curve->sample_baked(1) == doctest::Approx(1),
			"Custom linear curve should return the expected baked value at offset 1.0.");
	CHECK_MESSAGE(
			curve->sample_baked(2) == doctest::Approx(1),
			"Custom linear curve should return the expected baked value at offset 2.0.");

	ERR_PRINT_OFF;
	curve->remove_point(10);
	ERR_PRINT_ON;
	CHECK_MESSAGE(
			curve->sample(0.7) == doctest::Approx((real_t)0.8),
			"Custom free curve should return the expected value at offset 0.7 after removing point at invalid index 10.");
	CHECK_MESSAGE(
			curve->sample_baked(0.7) == doctest::Approx((real_t)0.8),
			"Custom free curve should return the expected baked value at offset 0.7 after removing point at invalid index 10.");
}

TEST_CASE("[Curve] Custom non-unit curve with linear tangents") {
	Ref<Curve> curve = memnew(Curve);
	curve->set_min_domain(-100.0);
	curve->set_max_domain(100.0);
	// "Sawtooth" curve with an open ending towards the 100 offset.
	curve->add_point(Vector2(-100, 0), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);
	curve->add_point(Vector2(-50, 1), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);
	curve->add_point(Vector2(0, 0), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);
	curve->add_point(Vector2(50, 1), 0, 0, Curve::TangentMode::TANGENT_LINEAR, Curve::TangentMode::TANGENT_LINEAR);

	CHECK_MESSAGE(
			curve->get_point_left_tangent(3) == doctest::Approx(1.f / 50),
			"get_point_left_tangent() should return the expected value for point index 3.");
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_right_tangent(3)),
			"get_point_right_tangent() should return the expected value for point index 3.");
	CHECK_MESSAGE(
			curve->get_point_left_mode(3) == Curve::TangentMode::TANGENT_LINEAR,
			"get_point_left_mode() should return the expected value for point index 3.");
	CHECK_MESSAGE(
			curve->get_point_right_mode(3) == Curve::TangentMode::TANGENT_LINEAR,
			"get_point_right_mode() should return the expected value for point index 3.");

	ERR_PRINT_OFF;
	CHECK_MESSAGE(
			Math::is_zero_approx(curve->get_point_right_tangent(300)),
			"get_point_right_tangent() should return the expected value for invalid point index 300.");
	CHECK_MESSAGE(
			curve->get_point_left_mode(-12345) == Curve::TangentMode::TANGENT_FREE,
			"get_point_left_mode() should return the expected value for invalid point index -12345.");
	ERR_PRINT_ON;

	CHECK_MESSAGE(
			curve->get_point_count() == 4,
			"Custom linear unit curve should contain the expected number of points.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample(-0.1 * curve->get_domain_range() + curve->get_min_domain())),
			"Custom linear curve should return the expected value at offset equivalent to a unit curve's -0.1.");
	CHECK_MESSAGE(
			curve->sample(0.1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected value at offset equivalent to a unit curve's 0.1.");
	CHECK_MESSAGE(
			curve->sample(0.4 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected value at offset equivalent to a unit curve's 0.4.");
	CHECK_MESSAGE(
			curve->sample(0.7 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.8),
			"Custom linear curve should return the expected value at offset equivalent to a unit curve's 0.7.");
	CHECK_MESSAGE(
			curve->sample(1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom linear curve should return the expected value at offset equivalent to a unit curve's 1.0.");
	CHECK_MESSAGE(
			curve->sample(2 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom linear curve should return the expected value at offset equivalent to a unit curve's 2.0.");

	CHECK_MESSAGE(
			Math::is_zero_approx(curve->sample_baked(-0.1 * curve->get_domain_range() + curve->get_min_domain())),
			"Custom linear curve should return the expected baked value at offset equivalent to a unit curve's -0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected baked value at offset equivalent to a unit curve's 0.1.");
	CHECK_MESSAGE(
			curve->sample_baked(0.4 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.4),
			"Custom linear curve should return the expected baked value at offset equivalent to a unit curve's 0.4.");
	CHECK_MESSAGE(
			curve->sample_baked(0.7 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.8),
			"Custom linear curve should return the expected baked value at offset equivalent to a unit curve's 0.7.");
	CHECK_MESSAGE(
			curve->sample_baked(1 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom linear curve should return the expected baked value at offset equivalent to a unit curve's 1.0.");
	CHECK_MESSAGE(
			curve->sample_baked(2 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx(1),
			"Custom linear curve should return the expected baked value at offset equivalent to a unit curve's 2.0.");

	ERR_PRINT_OFF;
	curve->remove_point(10);
	ERR_PRINT_ON;
	CHECK_MESSAGE(
			curve->sample(0.7 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.8),
			"Custom free curve should return the expected value at offset equivalent to a unit curve's 0.7 after removing point at invalid index 10.");
	CHECK_MESSAGE(
			curve->sample_baked(0.7 * curve->get_domain_range() + curve->get_min_domain()) == doctest::Approx((real_t)0.8),
			"Custom free curve should return the expected baked value at offset equivalent to a unit curve's 0.7 after removing point at invalid index 10.");
}

TEST_CASE("[Curve] Straight line offset test") {
	Ref<Curve> curve = memnew(Curve);
	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(1, 1));

	CHECK_MESSAGE(
			curve->sample_baked(1.0 - (0.5 / curve->get_bake_resolution())) != curve->sample_baked(1),
			"Straight line curve should return different baked values at offset 1 vs offset (1 - 0.5 / bake resolution) .");
}

TEST_CASE("[Curve2D] Linear sampling should return exact value") {
	Ref<Curve2D> curve = memnew(Curve2D);
	real_t len = 2048.0;

	curve->add_point(Vector2(0, 0));
	curve->add_point(Vector2(len, 0));

	real_t baked_length = curve->get_baked_length();
	CHECK(len == baked_length);

	for (int i = 0; i < len; i++) {
		Vector2 pos = curve->sample_baked(i);
		CHECK_MESSAGE(Math::is_equal_approx(pos.x, i), "sample_baked should return exact value");
	}
}

TEST_CASE("[Curve3D] Linear sampling should return exact value") {
	Ref<Curve3D> curve = memnew(Curve3D);
	real_t len = 2048.0;

	curve->add_point(Vector3(0, 0, 0));
	curve->add_point(Vector3(len, 0, 0));
	ERR_PRINT_OFF
	real_t baked_length = curve->get_baked_length();
	ERR_PRINT_ON
	CHECK(len == baked_length);

	for (int i = 0; i < len; i++) {
		Vector3 pos = curve->sample_baked(i);
		CHECK_MESSAGE(Math::is_equal_approx(pos.x, i), "sample_baked should return exact value");
	}
}

} // namespace TestCurve

// === Merged from tests/scene/test_curve_2d.cpp ===
/*  test_curve_2d.cpp                                                     */
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

#include "tests/test_macros.h"



#include "core/typedefs.h"
#include "servers/rendering/renderer.h"

namespace TestCurve2D {

void add_sample_curve_points(Ref<Curve2D> &curve) {
	Vector2 p0 = Vector2(0, 0);
	Vector2 p1 = Vector2(50, 0);
	Vector2 p2 = Vector2(50, 50);
	Vector2 p3 = Vector2(0, 50);

	Vector2 control0 = p1 - p0;
	Vector2 control1 = p3 - p2;

	curve->add_point(p0, Vector2(), control0);
	curve->add_point(p3, control1, Vector2());
}

TEST_CASE("[Curve2D] Default curve is empty") {
	const Ref<Curve2D> curve = memnew(Curve2D);
	CHECK(curve->get_point_count() == 0);
}

TEST_CASE("[Curve2D] Point management") {
	Ref<Curve2D> curve = memnew(Curve2D);

	SUBCASE("Functions for adding/removing points should behave as expected") {
		curve->set_point_count(2);
		CHECK(curve->get_point_count() == 2);

		curve->remove_point(0);
		CHECK(curve->get_point_count() == 1);

		curve->add_point(Vector2());
		CHECK(curve->get_point_count() == 2);

		curve->clear_points();
		CHECK(curve->get_point_count() == 0);
	}

	SUBCASE("Functions for changing single point properties should behave as expected") {
		Vector2 new_in = Vector2(1, 1);
		Vector2 new_out = Vector2(1, 1);
		Vector2 new_pos = Vector2(1, 1);

		curve->add_point(Vector2());

		CHECK(curve->get_point_in(0) != new_in);
		curve->set_point_in(0, new_in);
		CHECK(curve->get_point_in(0) == new_in);

		CHECK(curve->get_point_out(0) != new_out);
		curve->set_point_out(0, new_out);
		CHECK(curve->get_point_out(0) == new_out);

		CHECK(curve->get_point_position(0) != new_pos);
		curve->set_point_position(0, new_pos);
		CHECK(curve->get_point_position(0) == new_pos);
	}
}

TEST_CASE("[Curve2D] Baked") {
	Ref<Curve2D> curve = memnew(Curve2D);

	SUBCASE("Single Point") {
		curve->add_point(Vector2());

		CHECK(curve->get_baked_length() == 0);
		CHECK(curve->get_baked_points().size() == 1);
	}

	SUBCASE("Straight line") {
		curve->add_point(Vector2());
		curve->add_point(Vector2(0, 50));

		CHECK(Math::is_equal_approx(curve->get_baked_length(), 50));
		CHECK(curve->get_baked_points().size() == 15);
	}

	SUBCASE("Beziér Curve") {
		add_sample_curve_points(curve);

		real_t len = curve->get_baked_length();
		real_t n_points = curve->get_baked_points().size();
		// Curve length should be bigger than a straight between points
		CHECK(len > 50);

		SUBCASE("Increase bake interval") {
			curve->set_bake_interval(10.0);
			// Lower resolution should imply less points and smaller length
			CHECK(curve->get_baked_length() < len);
			CHECK(curve->get_baked_points().size() < n_points);
		}
	}
}

TEST_CASE("[Curve2D] Sampling") {
	// Sampling over a simple straight line to make assertions simpler
	Ref<Curve2D> curve = memnew(Curve2D);
	curve->add_point(Vector2());
	curve->add_point(Vector2(0, 50));

	SUBCASE("sample") {
		CHECK(curve->sample(0, 0) == Vector2(0, 0));
		CHECK(curve->sample(0, 0.5) == Vector2(0, 25));
		CHECK(curve->sample(0, 1) == Vector2(0, 50));
	}

	SUBCASE("samplef") {
		CHECK(curve->samplef(0) == Vector2(0, 0));
		CHECK(curve->samplef(0.5) == Vector2(0, 25));
		CHECK(curve->samplef(1) == Vector2(0, 50));
	}

	SUBCASE("sample_baked, cubic = false") {
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector2(0, 0))) == Vector2(0, 0));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector2(0, 25))) == Vector2(0, 25));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector2(0, 50))) == Vector2(0, 50));
	}

	SUBCASE("sample_baked, cubic = true") {
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector2(0, 0)), true) == Vector2(0, 0));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector2(0, 25)), true) == Vector2(0, 25));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector2(0, 50)), true) == Vector2(0, 50));
	}

	SUBCASE("sample_baked_with_rotation, cubic = false") {
		const real_t pi = 3.14159;
		const real_t half_pi = pi * 0.5;
		Ref<Curve2D> rot_curve = memnew(Curve2D);
		Transform2D t;

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(50, 0));
		t = rot_curve->sample_baked_with_rotation(25);
		CHECK(t.get_origin() == Vector2(25, 0));
		CHECK(Math::is_equal_approx(t.get_rotation(), 0));

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(0, 50));
		t = rot_curve->sample_baked_with_rotation(25);
		CHECK(t.get_origin() == Vector2(0, 25));
		CHECK(Math::is_equal_approx(t.get_rotation(), half_pi));

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(-50, 0));
		t = rot_curve->sample_baked_with_rotation(25);
		CHECK(t.get_origin() == Vector2(-25, 0));
		CHECK(Math::is_equal_approx(t.get_rotation(), pi));

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(0, -50));
		t = rot_curve->sample_baked_with_rotation(25);
		CHECK(t.get_origin() == Vector2(0, -25));
		CHECK(Math::is_equal_approx(t.get_rotation(), -half_pi));
	}

	SUBCASE("sample_baked_with_rotation, cubic = true") {
		const real_t pi = 3.14159;
		const real_t half_pi = pi * 0.5;
		Ref<Curve2D> rot_curve = memnew(Curve2D);
		Transform2D t;

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(50, 0));
		t = rot_curve->sample_baked_with_rotation(25, true);
		CHECK(t.get_origin() == Vector2(25, 0));
		CHECK(Math::is_equal_approx(t.get_rotation(), 0));

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(0, 50));
		t = rot_curve->sample_baked_with_rotation(25, true);
		CHECK(t.get_origin() == Vector2(0, 25));
		CHECK(Math::is_equal_approx(t.get_rotation(), half_pi));

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(-50, 0));
		t = rot_curve->sample_baked_with_rotation(25, true);
		CHECK(t.get_origin() == Vector2(-25, 0));
		CHECK(Math::is_equal_approx(t.get_rotation(), pi));

		rot_curve->clear_points();
		rot_curve->add_point(Vector2());
		rot_curve->add_point(Vector2(0, -50));
		t = rot_curve->sample_baked_with_rotation(25, true);
		CHECK(t.get_origin() == Vector2(0, -25));
		CHECK(Math::is_equal_approx(t.get_rotation(), -half_pi));
	}

	SUBCASE("get_closest_point") {
		CHECK(curve->get_closest_point(Vector2(0, 0)) == Vector2(0, 0));
		CHECK(curve->get_closest_point(Vector2(0, 25)) == Vector2(0, 25));
		CHECK(curve->get_closest_point(Vector2(50, 25)) == Vector2(0, 25));
		CHECK(curve->get_closest_point(Vector2(0, 50)) == Vector2(0, 50));
		CHECK(curve->get_closest_point(Vector2(50, 50)) == Vector2(0, 50));
		CHECK(curve->get_closest_point(Vector2(0, 100)) == Vector2(0, 50));
	}

	SUBCASE("sample_baked_with_rotation, linear curve with control1 = end and control2 = begin") {
		// Regression test for issue #88923
		// The Vector2s that aren't relevant to the issue have x = 2 or x = -2.
		// They're just set to make collisions with corner cases less likely
		// that involve zero-vector control points.
		Ref<Curve2D> cross_linear_curve = memnew(Curve2D);
		cross_linear_curve->set_bake_interval(0.5);
		cross_linear_curve->add_point(Vector2(), Vector2(-2, 0), Vector2(1, 0));
		cross_linear_curve->add_point(Vector2(1, 0), Vector2(-1, 0), Vector2(2, 0));
		CHECK(cross_linear_curve->get_baked_points().size() >= 3);
		CHECK(cross_linear_curve->sample_baked_with_rotation(cross_linear_curve->get_closest_offset(Vector2(0.5, 0))).is_equal_approx(Transform2D(Vector2(1, 0), Vector2(0, 1), Vector2(0.5, 0))));
	}
}

TEST_CASE("[Curve2D] Tessellation") {
	Ref<Curve2D> curve = memnew(Curve2D);
	add_sample_curve_points(curve);

	const int default_size = curve->tessellate().size();

	SUBCASE("Increase to max stages should increase num of points") {
		CHECK(curve->tessellate(6).size() > default_size);
	}

	SUBCASE("Decrease to max stages should decrease num of points") {
		CHECK(curve->tessellate(4).size() < default_size);
	}

	SUBCASE("Increase to tolerance should decrease num of points") {
		CHECK(curve->tessellate(5, 5).size() < default_size);
	}

	SUBCASE("Decrease to tolerance should increase num of points") {
		CHECK(curve->tessellate(5, 3).size() > default_size);
	}

	SUBCASE("Adding a straight segment should only add the last point to tessellate return array") {
		curve->add_point(Vector2(0, 100));
		PackedVector2Array tes = curve->tessellate();
		CHECK(tes.size() == default_size + 1);
		CHECK(tes[tes.size() - 1] == Vector2(0, 100));
		CHECK(tes[tes.size() - 2] == Vector2(0, 50));
	}
}

TEST_CASE("[Curve2D] Even length tessellation") {
	Ref<Curve2D> curve = memnew(Curve2D);
	add_sample_curve_points(curve);

	const int default_size = curve->tessellate_even_length().size();

	// Default tessellate_even_length tolerance_length is 20.0, by adding a 100 units
	// straight, we expect the total size to be increased by more than 5,
	// that is, the algo will pick a length < 20.0 and will divide the straight as
	// well as the curve as opposed to tessellate() which only adds the final point
	curve->add_point(Vector2(0, 150));
	CHECK(curve->tessellate_even_length().size() > default_size + 5);
}

} // namespace TestCurve2D

// === Merged from tests/scene/test_curve_3d.cpp ===
/*  test_curve_3d.cpp                                                     */
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

#include "tests/test_macros.h"



#include "core/typedefs.h"
#include "servers/rendering/renderer.h"

namespace TestCurve3D {

void add_sample_curve_points(Ref<Curve3D> &curve) {
	Vector3 p0 = Vector3(0, 0, 0);
	Vector3 p1 = Vector3(50, 0, 0);
	Vector3 p2 = Vector3(50, 50, 50);
	Vector3 p3 = Vector3(0, 50, 0);

	Vector3 control0 = p1 - p0;
	Vector3 control1 = p3 - p2;

	curve->add_point(p0, Vector3(), control0);
	curve->add_point(p3, control1, Vector3());
}

TEST_CASE("[Curve3D] Default curve is empty") {
	const Ref<Curve3D> curve = memnew(Curve3D);
	CHECK(curve->get_point_count() == 0);
}

TEST_CASE("[Curve3D] Point management") {
	Ref<Curve3D> curve = memnew(Curve3D);

	SUBCASE("Functions for adding/removing points should behave as expected") {
		curve->set_point_count(2);
		CHECK(curve->get_point_count() == 2);

		curve->remove_point(0);
		CHECK(curve->get_point_count() == 1);

		curve->add_point(Vector3());
		CHECK(curve->get_point_count() == 2);

		curve->clear_points();
		CHECK(curve->get_point_count() == 0);
	}

	SUBCASE("Functions for changing single point properties should behave as expected") {
		Vector3 new_in = Vector3(1, 1, 1);
		Vector3 new_out = Vector3(1, 1, 1);
		Vector3 new_pos = Vector3(1, 1, 1);
		real_t new_tilt = 1;

		curve->add_point(Vector3());

		CHECK(curve->get_point_in(0) != new_in);
		curve->set_point_in(0, new_in);
		CHECK(curve->get_point_in(0) == new_in);

		CHECK(curve->get_point_out(0) != new_out);
		curve->set_point_out(0, new_out);
		CHECK(curve->get_point_out(0) == new_out);

		CHECK(curve->get_point_position(0) != new_pos);
		curve->set_point_position(0, new_pos);
		CHECK(curve->get_point_position(0) == new_pos);

		CHECK(curve->get_point_tilt(0) != new_tilt);
		curve->set_point_tilt(0, new_tilt);
		CHECK(curve->get_point_tilt(0) == new_tilt);
	}
}

TEST_CASE("[Curve3D] Baked") {
	Ref<Curve3D> curve = memnew(Curve3D);

	SUBCASE("Single Point") {
		curve->add_point(Vector3());

		CHECK(curve->get_baked_length() == 0);
		CHECK(curve->get_baked_points().size() == 1);
		CHECK(curve->get_baked_tilts().size() == 1);
		CHECK(curve->get_baked_up_vectors().size() == 1);
	}

	SUBCASE("Straight line") {
		curve->add_point(Vector3());
		curve->add_point(Vector3(0, 50, 0));

		CHECK(Math::is_equal_approx(curve->get_baked_length(), 50));
		CHECK(curve->get_baked_points().size() == 369);
		CHECK(curve->get_baked_tilts().size() == 369);
		CHECK(curve->get_baked_up_vectors().size() == 369);
	}

	SUBCASE("Beziér Curve") {
		add_sample_curve_points(curve);

		real_t len = curve->get_baked_length();
		real_t n_points = curve->get_baked_points().size();
		// Curve length should be bigger than a straight line between points
		CHECK(len > 50);

		SUBCASE("Increase bake interval") {
			curve->set_bake_interval(10.0);
			CHECK(curve->get_bake_interval() == 10.0);
			// Lower resolution should imply less points and smaller length
			CHECK(curve->get_baked_length() < len);
			CHECK(curve->get_baked_points().size() < n_points);
			CHECK(curve->get_baked_tilts().size() < n_points);
			CHECK(curve->get_baked_up_vectors().size() < n_points);
		}

		SUBCASE("Disable up vectors") {
			curve->set_up_vector_enabled(false);
			CHECK(curve->is_up_vector_enabled() == false);
			CHECK(curve->get_baked_up_vectors().size() == 0);
		}
	}
}

TEST_CASE("[Curve3D] Sampling") {
	// Sampling over a simple straight line to make assertions simpler
	Ref<Curve3D> curve = memnew(Curve3D);
	curve->add_point(Vector3());
	curve->add_point(Vector3(0, 50, 0));

	SUBCASE("sample") {
		CHECK(curve->sample(0, 0) == Vector3(0, 0, 0));
		CHECK(curve->sample(0, 0.5) == Vector3(0, 25, 0));
		CHECK(curve->sample(0, 1) == Vector3(0, 50, 0));
	}

	SUBCASE("samplef") {
		CHECK(curve->samplef(0) == Vector3(0, 0, 0));
		CHECK(curve->samplef(0.5) == Vector3(0, 25, 0));
		CHECK(curve->samplef(1) == Vector3(0, 50, 0));
	}

	SUBCASE("sample_baked, cubic = false") {
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector3(0, 0, 0))) == Vector3(0, 0, 0));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector3(0, 25, 0))) == Vector3(0, 25, 0));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector3(0, 50, 0))) == Vector3(0, 50, 0));
	}

	SUBCASE("sample_baked, cubic = true") {
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector3(0, 0, 0)), true) == Vector3(0, 0, 0));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector3(0, 25, 0)), true) == Vector3(0, 25, 0));
		CHECK(curve->sample_baked(curve->get_closest_offset(Vector3(0, 50, 0)), true) == Vector3(0, 50, 0));
	}

	SUBCASE("sample_baked_with_rotation, cubic = false, p_apply_tilt = false") {
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 0, 0))) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 0, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 25, 0))) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 25, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 50, 0))) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 50, 0)));
	}

	SUBCASE("sample_baked_with_rotation, cubic = false, p_apply_tilt = true") {
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 0, 0)), false, true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 0, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 25, 0)), false, true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 25, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 50, 0)), false, true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 50, 0)));
	}

	SUBCASE("sample_baked_with_rotation, cubic = true, p_apply_tilt = false") {
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 0, 0)), true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 0, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 25, 0)), true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 25, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 50, 0)), true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 50, 0)));
	}

	SUBCASE("sample_baked_with_rotation, cubic = true, p_apply_tilt = true") {
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 0, 0)), true, true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 0, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 25, 0)), true, true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 25, 0)));
		CHECK(curve->sample_baked_with_rotation(curve->get_closest_offset(Vector3(0, 50, 0)), true, true) == Transform3D(Basis(Vector3(0, 0, -1), Vector3(1, 0, 0), Vector3(0, -1, 0)), Vector3(0, 50, 0)));
	}

	SUBCASE("sample_baked_tilt") {
		CHECK(curve->sample_baked_tilt(curve->get_closest_offset(Vector3(0, 0, 0))) == 0);
		CHECK(curve->sample_baked_tilt(curve->get_closest_offset(Vector3(0, 25, 0))) == 0);
		CHECK(curve->sample_baked_tilt(curve->get_closest_offset(Vector3(0, 50, 0))) == 0);
	}

	SUBCASE("sample_baked_up_vector, p_apply_tilt = false") {
		CHECK(curve->sample_baked_up_vector(curve->get_closest_offset(Vector3(0, 0, 0))) == Vector3(1, 0, 0));
		CHECK(curve->sample_baked_up_vector(curve->get_closest_offset(Vector3(0, 25, 0))) == Vector3(1, 0, 0));
		CHECK(curve->sample_baked_up_vector(curve->get_closest_offset(Vector3(0, 50, 0))) == Vector3(1, 0, 0));
	}

	SUBCASE("sample_baked_up_vector, p_apply_tilt = true") {
		CHECK(curve->sample_baked_up_vector(curve->get_closest_offset(Vector3(0, 0, 0)), true) == Vector3(1, 0, 0));
		CHECK(curve->sample_baked_up_vector(curve->get_closest_offset(Vector3(0, 25, 0)), true) == Vector3(1, 0, 0));
		CHECK(curve->sample_baked_up_vector(curve->get_closest_offset(Vector3(0, 50, 0)), true) == Vector3(1, 0, 0));
	}

	SUBCASE("get_closest_point") {
		CHECK(curve->get_closest_point(Vector3(0, 0, 0)) == Vector3(0, 0, 0));
		CHECK(curve->get_closest_point(Vector3(0, 25, 0)) == Vector3(0, 25, 0));
		CHECK(curve->get_closest_point(Vector3(50, 25, 0)) == Vector3(0, 25, 0));
		CHECK(curve->get_closest_point(Vector3(0, 50, 0)) == Vector3(0, 50, 0));
		CHECK(curve->get_closest_point(Vector3(50, 50, 0)) == Vector3(0, 50, 0));
		CHECK(curve->get_closest_point(Vector3(0, 100, 0)) == Vector3(0, 50, 0));
	}

	SUBCASE("sample_baked_up_vector, off-axis") {
		// Regression test for issue #81879
		Ref<Curve3D> c = memnew(Curve3D);
		c->add_point(Vector3());
		c->add_point(Vector3(0, .1, 1));
		CHECK_LT((c->sample_baked_up_vector(c->get_closest_offset(Vector3(0, 0, .9))) - Vector3(0, 0.995037, -0.099504)).length(), 0.01);
	}

	SUBCASE("sample_baked_with_rotation, linear curve with control1 = end and control2 = begin") {
		// Regression test for issue #88923
		// The Vector3s that aren't relevant to the issue have z = 2.
		// They're just set to make collisions with corner cases less likely
		// that involve zero-vector control points.
		Ref<Curve3D> cross_linear_curve = memnew(Curve3D);
		cross_linear_curve->add_point(Vector3(), Vector3(-1, 0, 2), Vector3(1, 0, 0));
		cross_linear_curve->add_point(Vector3(1, 0, 0), Vector3(-1, 0, 0), Vector3(1, 0, 2));
		CHECK(cross_linear_curve->get_baked_points().size() >= 3);
		CHECK(cross_linear_curve->sample_baked_with_rotation(cross_linear_curve->get_closest_offset(Vector3(0.5, 0, 0))).is_equal_approx(Transform3D(Basis(Vector3(0, 0, 1), Vector3(0, 1, 0), Vector3(-1, 0, 0)), Vector3(0.5, 0, 0))));
	}
}

TEST_CASE("[Curve3D] Tessellation") {
	Ref<Curve3D> curve = memnew(Curve3D);
	add_sample_curve_points(curve);

	const int default_size = curve->tessellate().size();

	SUBCASE("Increase to max stages should increase num of points") {
		CHECK(curve->tessellate(6).size() > default_size);
	}

	SUBCASE("Decrease to max stages should decrease num of points") {
		CHECK(curve->tessellate(4).size() < default_size);
	}

	SUBCASE("Increase to tolerance should decrease num of points") {
		CHECK(curve->tessellate(5, 5).size() < default_size);
	}

	SUBCASE("Decrease to tolerance should increase num of points") {
		CHECK(curve->tessellate(5, 3).size() > default_size);
	}

	SUBCASE("Adding a straight segment should only add the last point to tessellate return array") {
		curve->add_point(Vector3(0, 100, 0));
		PackedVector3Array tes = curve->tessellate();
		CHECK(tes.size() == default_size + 1);
		CHECK(tes[tes.size() - 1] == Vector3(0, 100, 0));
		CHECK(tes[tes.size() - 2] == Vector3(0, 50, 0));
	}
}

TEST_CASE("[Curve3D] Even length tessellation") {
	Ref<Curve3D> curve = memnew(Curve3D);
	add_sample_curve_points(curve);

	const int default_size = curve->tessellate_even_length().size();

	// Default tessellate_even_length tolerance_length is 20.0, by adding a 100 units
	// straight, we expect the total size to be increased by more than 5,
	// that is, the algo will pick a length < 20.0 and will divide the straight as
	// well as the curve as opposed to tessellate() which only adds the final point.
	curve->add_point(Vector3(0, 150, 0));
	CHECK(curve->tessellate_even_length().size() > default_size + 5);
}

} // namespace TestCurve3D

// === Merged from tests/scene/test_fontfile.cpp ===
/*  test_fontfile.cpp                                                     */
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

#include "tests/test_macros.h"



#include "scene/resources/theme.h"

#include "modules/modules_enabled.gen.h" // For freetype.

namespace TestFontFile {

TEST_CASE("[FontFile] Load Dynamic Font - getters") {
#ifdef MODULE_FREETYPE_ENABLED
	String test_dynamic_font = "thirdparty/fonts/NotoSansHebrew_Regular.woff2";
	Ref<FontFile> ff;
	ff.instantiate();
	CHECK(ff->load_dynamic_font(test_dynamic_font) == OK);

	// These properties come from the font file itself.
	CHECK(ff->get_font_name() == "Noto Sans Hebrew");
	CHECK(ff->get_font_style_name() == "Regular");
	CHECK(ff->get_font_weight() == 400);
	CHECK(ff->get_font_stretch() == 100);
	CHECK(ff->get_opentype_features() == Dictionary());

	Dictionary expected_ot_name_strings;
	Dictionary en_dict;
	en_dict["copyright"] = "Copyright 2024 The Noto Project Authors (https://github.com/notofonts/hebrew)";
	en_dict["family_name"] = "Noto Sans Hebrew";
	en_dict["subfamily_name"] = "Regular";
	en_dict["unique_identifier"] = "3.001;GOOG;NotoSansHebrew-Regular";
	en_dict["full_name"] = "Noto Sans Hebrew Regular";
	en_dict["version"] = "Version 3.001";
	en_dict["postscript_name"] = "NotoSansHebrew-Regular";
	en_dict["trademark"] = "Noto is a trademark of Google Inc.";
	en_dict["manufacturer"] = "Google LLC";
	en_dict["designer"] = "Ben Nathan";
	en_dict["description"] = "Designed by Ben Nathan";
	en_dict["vendor_url"] = "http://www.google.com/get/noto/";
	en_dict["designer_url"] = "https://hafontia.com/";
	en_dict["license"] = "This Font Software is licensed under the SIL Open Font License, Version 1.1. This Font Software is distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the SIL Open Font License for the specific language, permissions and limitations governing your use of this Font Software.";
	en_dict["license_url"] = "http://scripts.sil.org/OFL";
	expected_ot_name_strings["en"] = en_dict;
	CHECK(ff->get_ot_name_strings() == expected_ot_name_strings);

	// These are dependent on size and potentially other state. Act as regression tests based of arbitrary small size 10 and large size 100.
	CHECK(ff->get_height(10) == doctest::Approx((real_t)14));
	CHECK(ff->get_ascent(10) == doctest::Approx((real_t)11));
	CHECK(ff->get_descent(10) == doctest::Approx((real_t)3));
	CHECK(ff->get_underline_position(10) == doctest::Approx((real_t)1.25));
	CHECK(ff->get_underline_thickness(10) == doctest::Approx((real_t)0.5));
	CHECK(ff->get_height(100) == doctest::Approx((real_t)137));
	CHECK(ff->get_ascent(100) == doctest::Approx((real_t)107));
	CHECK(ff->get_descent(100) == doctest::Approx((real_t)30));
	CHECK(ff->get_underline_position(100) == doctest::Approx((real_t)12.5));
	CHECK(ff->get_underline_thickness(100) == doctest::Approx((real_t)5));
#endif
}

TEST_CASE("[FontFile] Create font file and check data") {
	// Create test instance.
	Ref<FontFile> font_file;
	font_file.instantiate();

#ifdef MODULE_FREETYPE_ENABLED
	// Try to load non-existent files.
	ERR_PRINT_OFF
	CHECK(font_file->load_dynamic_font("") == OK);
	CHECK_MESSAGE(font_file->get_data().is_empty() == true, "Invalid fontfile should not be loaded.");

	CHECK(font_file->load_dynamic_font("thirdparty/fonts/nofonthasthisname.woff2") == OK);
	CHECK_MESSAGE(font_file->get_data().is_empty() == true, "Invalid fontfile should not be loaded.");
	ERR_PRINT_ON

	// Load a valid file.
	CHECK(font_file->load_dynamic_font("thirdparty/fonts/Inter_Regular.woff2") == OK);

	// Check fontfile data.
	CHECK_MESSAGE(font_file->get_data().is_empty() == false, "Fontfile should have been loaded.");
	CHECK_MESSAGE(font_file->get_font_name() == "Inter", "Loaded correct font name.");
	CHECK_MESSAGE(font_file->get_font_style_name() == "Regular", "Loaded correct font style.");
	CHECK_MESSAGE(font_file->get_data().size() == 111268llu, "Whole fontfile was loaded.");

	// Valid glyphs.
	CHECK_MESSAGE(font_file->get_glyph_index(2, 'a', 0) != 0, "Glyph index for 'a' is valid.");
	CHECK_MESSAGE(font_file->get_glyph_index(2, 'b', 0) != 0, "Glyph index for 'b' is valid.");
	CHECK_MESSAGE(font_file->get_glyph_index(2, 0x0103, 0) != 0, "Glyph index for 'latin small letter a with breve' is valid.");
	CHECK_MESSAGE(font_file->get_glyph_index(2, 0x03a8, 0) != 0, "Glyph index for 'Greek psi' is valid.");
	CHECK_MESSAGE(font_file->get_glyph_index(2, 0x0416, 0) != 0, "Glyph index for 'Cyrillic zhe' is valid.");
	CHECK_MESSAGE(font_file->get_glyph_index(2, '&', 0) != 0, "Glyph index for '&' is valid.");

	// Invalid glyphs.
	CHECK_MESSAGE(font_file->get_glyph_index(2, 0x4416, 0) == 0, "Glyph index is invalid.");
	CHECK_MESSAGE(font_file->get_glyph_index(2, 0x5555, 0) == 0, "Glyph index is invalid.");
	CHECK_MESSAGE(font_file->get_glyph_index(2, 0x2901, 0) == 0, "Glyph index is invalid.");
#endif
}

} // namespace TestFontFile

// === Merged from tests/scene/test_gradient.cpp ===
/*  test_gradient.cpp                                                     */
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

#include "tests/test_macros.h"



#include "servers/rendering/renderer.h"

namespace TestGradient {

TEST_CASE("[Gradient] Default gradient") {
	// Black-white gradient.
	Ref<Gradient> gradient = memnew(Gradient);

	CHECK_MESSAGE(
			gradient->get_point_count() == 2,
			"Default gradient should contain the expected number of points.");

	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.0).is_equal_approx(Color(0, 0, 0)),
			"Default gradient should return the expected interpolated value at offset 0.0.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.4).is_equal_approx(Color(0.4, 0.4, 0.4)),
			"Default gradient should return the expected interpolated value at offset 0.4.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.8).is_equal_approx(Color(0.8, 0.8, 0.8)),
			"Default gradient should return the expected interpolated value at offset 0.8.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(1.0).is_equal_approx(Color(1, 1, 1)),
			"Default gradient should return the expected interpolated value at offset 1.0.");

	// Out of bounds checks.
	CHECK_MESSAGE(
			gradient->get_color_at_offset(-1.0).is_equal_approx(Color(0, 0, 0)),
			"Default gradient should return the expected interpolated value at offset -1.0.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(1234.0).is_equal_approx(Color(1, 1, 1)),
			"Default gradient should return the expected interpolated value at offset 1234.0.");
}

TEST_CASE("[Gradient] Custom gradient (points specified in order)") {
	// Red-yellow-green gradient (with overbright green).
	Ref<Gradient> gradient = memnew(Gradient);
	Vector<float> offsets = { 0.0, 0.5, 1.0 };
	Vector<Color> colors = { Color(1, 0, 0), Color(1, 1, 0), Color(0, 2, 0) };

	gradient->set_offsets(offsets);
	gradient->set_colors(colors);

	CHECK_MESSAGE(
			gradient->get_point_count() == 3,
			"Custom gradient should contain the expected number of points.");

	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.0).is_equal_approx(Color(1, 0, 0)),
			"Custom gradient should return the expected interpolated value at offset 0.0.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.25).is_equal_approx(Color(1, 0.5, 0)),
			"Custom gradient should return the expected interpolated value at offset 0.25.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.5).is_equal_approx(Color(1, 1, 0)),
			"Custom gradient should return the expected interpolated value at offset 0.5.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.75).is_equal_approx(Color(0.5, 1.5, 0)),
			"Custom gradient should return the expected interpolated value at offset 0.75.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(1.0).is_equal_approx(Color(0, 2, 0)),
			"Custom gradient should return the expected interpolated value at offset 1.0.");

	gradient->remove_point(1);
	CHECK_MESSAGE(
			gradient->get_point_count() == 2,
			"Custom gradient should contain the expected number of points after removing one point.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.5).is_equal_approx(Color(0.5, 1, 0)),
			"Custom gradient should return the expected interpolated value at offset 0.5 after removing point at index 1.");
}

TEST_CASE("[Gradient] Custom gradient (points specified out-of-order)") {
	// HSL rainbow with points specified out of order.
	// These should be sorted automatically when adding points.
	Ref<Gradient> gradient = memnew(Gradient);
	Vector<float> offsets = { 0.2, 0.0, 0.8, 0.4, 1.0, 0.6 };
	Vector<Color> colors = { Color(1, 0, 0), Color(1, 1, 0), Color(0, 1, 0), Color(0, 1, 1), Color(0, 0, 1), Color(1, 0, 1) };

	gradient->set_offsets(offsets);
	gradient->set_colors(colors);

	CHECK_MESSAGE(
			gradient->get_point_count() == 6,
			"Custom out-of-order gradient should contain the expected number of points.");

	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.0).is_equal_approx(Color(1, 1, 0)),
			"Custom out-of-order gradient should return the expected interpolated value at offset 0.0.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.3).is_equal_approx(Color(0.5, 0.5, 0.5)),
			"Custom out-of-order gradient should return the expected interpolated value at offset 0.3.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.6).is_equal_approx(Color(1, 0, 1)),
			"Custom out-of-order gradient should return the expected interpolated value at offset 0.6.");
	CHECK_MESSAGE(
			gradient->get_color_at_offset(1.0).is_equal_approx(Color(0, 0, 1)),
			"Custom out-of-order gradient should return the expected interpolated value at offset 1.0.");

	gradient->remove_point(0);
	CHECK_MESSAGE(
			gradient->get_point_count() == 5,
			"Custom out-of-order gradient should contain the expected number of points after removing one point.");
	// The color will be clamped to the nearest point (which is at offset 0.2).
	CHECK_MESSAGE(
			gradient->get_color_at_offset(0.1).is_equal_approx(Color(1, 0, 0)),
			"Custom out-of-order gradient should return the expected interpolated value at offset 0.1 after removing point at index 0.");
}

} // namespace TestGradient

// === Merged from tests/scene/test_gradient_texture.cpp ===
/*  test_gradient_texture.cpp                                             */
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

#include "tests/test_macros.h"



#include "servers/rendering/renderer.h"

namespace TestGradientTexture {

// [SceneTree] in a test case name enables initializing a mock render server,
// which ImageTexture is dependent on.
TEST_CASE("[SceneTree][GradientTexture1D] Create GradientTexture1D") {
	Ref<GradientTexture2D> gradient_texture = memnew(GradientTexture2D(true));
	CHECK(gradient_texture->get_width() == 256);
	CHECK(gradient_texture->get_height() == 1);

	Ref<Gradient> test_gradient = memnew(Gradient);
	gradient_texture->set_gradient(test_gradient);
	CHECK(gradient_texture->get_gradient() == test_gradient);

	gradient_texture->set_width(83);
	CHECK(gradient_texture->get_width() == 83);
	CHECK(gradient_texture->get_height() == 1);

	gradient_texture->set_use_hdr(true);
	CHECK(gradient_texture->is_using_hdr());
}

TEST_CASE("[SceneTree][GradientTexture2D] Create GradientTexture2D") {
	Ref<GradientTexture2D> gradient_texture = memnew(GradientTexture2D);

	Ref<Gradient> test_gradient = memnew(Gradient);
	gradient_texture->set_gradient(test_gradient);
	CHECK(gradient_texture->get_gradient() == test_gradient);

	gradient_texture->set_width(82);
	CHECK(gradient_texture->get_width() == 82);

	gradient_texture->set_height(81);
	CHECK(gradient_texture->get_height() == 81);

	gradient_texture->set_use_hdr(true);
	CHECK(gradient_texture->is_using_hdr());

	gradient_texture->set_fill(GradientTexture2D::Fill::FILL_SQUARE);
	CHECK(gradient_texture->get_fill() == GradientTexture2D::Fill::FILL_SQUARE);

	gradient_texture->set_fill_from(Vector2(0.2, 0.25));
	CHECK(gradient_texture->get_fill_from() == Vector2(0.2, 0.25));

	gradient_texture->set_fill_to(Vector2(0.35, 0.5));
	CHECK(gradient_texture->get_fill_to() == Vector2(0.35, 0.5));

	gradient_texture->set_repeat(GradientTexture2D::Repeat::REPEAT);
	CHECK(gradient_texture->get_repeat() == GradientTexture2D::Repeat::REPEAT);
}

} // namespace TestGradientTexture

// === Merged from tests/scene/test_image_texture.cpp ===
/*  test_image_texture.cpp                                                */
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

#include "tests/test_macros.h"



#include "core/io/resource_loader.h"
#include "servers/rendering/renderer.h"
#include "tests/test_macros.h"

namespace TestImageTexture {

// [SceneTree] in a test case name enables initializing a mock render server,
// which ImageTexture is dependent on.
TEST_CASE("[SceneTree][ImageTexture] constructor") {
	Ref<ImageTexture> image_texture = memnew(ImageTexture);
	CHECK(image_texture->get_width() == 0);
	CHECK(image_texture->get_height() == 0);
	CHECK(image_texture->get_format() == 0);
	CHECK(image_texture->has_alpha() == false);
	CHECK(image_texture->get_image() == Ref<Image>());
}

TEST_CASE("[SceneTree][ImageTexture] create_from_image") {
	Ref<Image> image = memnew(Image(16, 8, true, Image::FORMAT_RGBA8));
	Ref<ImageTexture> image_texture = ImageTexture::create_from_image(image);
	CHECK(image_texture->get_width() == 16);
	CHECK(image_texture->get_height() == 8);
	CHECK(image_texture->get_format() == Image::FORMAT_RGBA8);
	CHECK(image_texture->has_alpha() == true);
	CHECK(image_texture->get_rid().is_valid() == true);
}

TEST_CASE("[SceneTree][ImageTexture] set_image") {
	Ref<ImageTexture> image_texture = memnew(ImageTexture);
	Ref<Image> image = memnew(Image(8, 4, false, Image::FORMAT_RGB8));
	image_texture->set_image(image);
	CHECK(image_texture->get_width() == 8);
	CHECK(image_texture->get_height() == 4);
	CHECK(image_texture->get_format() == Image::FORMAT_RGB8);
	CHECK(image_texture->has_alpha() == false);
	CHECK(image_texture->get_width() == image_texture->get_image()->get_width());
	CHECK(image_texture->get_height() == image_texture->get_image()->get_height());
	CHECK(image_texture->get_format() == image_texture->get_image()->get_format());
}

TEST_CASE("[SceneTree][ImageTexture] set_size_override") {
	Ref<Image> image = memnew(Image(16, 8, false, Image::FORMAT_RGB8));
	Ref<ImageTexture> image_texture = ImageTexture::create_from_image(image);
	CHECK(image_texture->get_width() == 16);
	CHECK(image_texture->get_height() == 8);
	image_texture->set_size_override(Size2i(32, 16));
	CHECK(image_texture->get_width() == 32);
	CHECK(image_texture->get_height() == 16);
}

TEST_CASE("[SceneTree][ImageTexture] is_pixel_opaque") {
	Ref<Image> image = memnew(Image(8, 8, false, Image::FORMAT_RGBA8));
	image->set_pixel(0, 0, Color(0.0, 0.0, 0.0, 0.0)); // not opaque
	image->set_pixel(0, 1, Color(0.0, 0.0, 0.0, 0.1)); // not opaque
	image->set_pixel(0, 2, Color(0.0, 0.0, 0.0, 0.5)); // opaque
	image->set_pixel(0, 3, Color(0.0, 0.0, 0.0, 0.9)); // opaque
	image->set_pixel(0, 4, Color(0.0, 0.0, 0.0, 1.0)); // opaque

	Ref<ImageTexture> image_texture = ImageTexture::create_from_image(image);
	CHECK(image_texture->is_pixel_opaque(0, 0) == false);
	CHECK(image_texture->is_pixel_opaque(0, 1) == false);
	CHECK(image_texture->is_pixel_opaque(0, 2) == true);
	CHECK(image_texture->is_pixel_opaque(0, 3) == true);
	CHECK(image_texture->is_pixel_opaque(0, 4) == true);
}

TEST_CASE("[SceneTree][ImageTexture] set_path") {
	Ref<ImageTexture> image_texture = memnew(ImageTexture);
	String path = TestUtils::get_data_path("images/icon.png");
	image_texture->set_path(path, true);
	CHECK(image_texture->get_path() == path);
}

} // namespace TestImageTexture

// === Merged from tests/scene/test_image_texture_3d.cpp ===
/*  test_image_texture_3d.cpp                                             */
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

#include "tests/test_macros.h"



#include "core/io/resource_loader.h"
#include "servers/rendering/renderer.h"
#include "tests/test_macros.h"

namespace TestImageTexture3D {

// [SceneTree] in a test case name enables initializing a mock render server,
// which ImageTexture3D is dependent on.
TEST_CASE("[SceneTree][ImageTexture3D] Constructor") {
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	CHECK(image_texture_3d->get_format() == Image::FORMAT_L8);
	CHECK(image_texture_3d->get_width() == 1);
	CHECK(image_texture_3d->get_height() == 1);
	CHECK(image_texture_3d->get_depth() == 1);
	CHECK(image_texture_3d->has_mipmaps() == false);
}

TEST_CASE("[SceneTree][ImageTexture3D] get_format") {
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	CHECK(image_texture_3d->get_format() == Image::FORMAT_L8);
}

TEST_CASE("[SceneTree][ImageTexture3D] get_width") {
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	CHECK(image_texture_3d->get_width() == 1);
}

TEST_CASE("[SceneTree][ImageTexture3D] get_height") {
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	CHECK(image_texture_3d->get_height() == 1);
}

TEST_CASE("[SceneTree][ImageTexture3D] get_depth") {
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	CHECK(image_texture_3d->get_depth() == 1);
}

TEST_CASE("[SceneTree][ImageTexture3D] has_mipmaps") {
	const Vector<Ref<Image>> images = { memnew(Image(8, 8, false, Image::FORMAT_RGBA8)), memnew(Image(8, 8, false, Image::FORMAT_RGBA8)) };
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	CHECK(image_texture_3d->has_mipmaps() == false); // No mipmaps.
	image_texture_3d->create(Image::FORMAT_RGBA8, 2, 2, 2, true, images);
	CHECK(image_texture_3d->has_mipmaps() == true); // Mipmaps.
}

TEST_CASE("[SceneTree][ImageTexture3D] create") {
	const Vector<Ref<Image>> images = { memnew(Image(8, 8, false, Image::FORMAT_RGBA8)), memnew(Image(8, 8, false, Image::FORMAT_RGBA8)) };
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	CHECK(image_texture_3d->create(Image::FORMAT_RGBA8, 2, 2, 2, true, images) == OK); // Run create and check return value simultaneously.
	CHECK(image_texture_3d->get_format() == Image::FORMAT_RGBA8);
	CHECK(image_texture_3d->get_width() == 2);
	CHECK(image_texture_3d->get_height() == 2);
	CHECK(image_texture_3d->get_depth() == 2);
	CHECK(image_texture_3d->has_mipmaps() == true);
}

TEST_CASE("[SceneTree][ImageTexture3D] set_path") {
	Ref<ImageTexture3D> image_texture_3d = memnew(ImageTexture3D);
	String path = TestUtils::get_data_path("images/icon.png");
	image_texture_3d->set_path(path, true);
	CHECK(image_texture_3d->get_path() == path);
}

} // namespace TestImageTexture3D

// === Merged from tests/scene/test_packed_scene.cpp ===
/*  test_packed_scene.cpp                                                 */
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

#include "tests/test_macros.h"



#include "scene/main/node.h"

namespace TestPackedScene {

TEST_CASE("[PackedScene] Pack Scene and Retrieve State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	const Error err = packed_scene.pack(scene);
	CHECK(err == OK);

	// Retrieve the packed state.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state->get_node_count() == 1);
	CHECK(state->get_node_name(0) == "TestScene");

	memdelete(scene);
}

TEST_CASE("[PackedScene] Signals Preserved when Packing Scene") {
	// Create main scene
	// root
	// `- sub_node (local)
	// `- sub_scene (instance of another scene)
	//    `- sub_scene_node (owned by sub_scene)
	Node *main_scene_root = memnew(Node);
	Node *sub_node = memnew(Node);
	Node *sub_scene_root = memnew(Node);
	Node *sub_scene_node = memnew(Node);

	main_scene_root->add_child(sub_node);
	sub_node->set_owner(main_scene_root);

	sub_scene_root->add_child(sub_scene_node);
	sub_scene_node->set_owner(sub_scene_root);

	main_scene_root->add_child(sub_scene_root);
	sub_scene_root->set_owner(main_scene_root);

	SUBCASE("Signals that should be saved") {
		int main_flags = Object::CONNECT_PERSIST;
		// sub node to a node in main scene
		sub_node->connect("ready", callable_mp(main_scene_root, &Node::is_ready), main_flags);
		// subscene root to a node in main scene
		sub_scene_root->connect("ready", callable_mp(main_scene_root, &Node::is_ready), main_flags);
		//subscene root to subscene root (connected within main scene)
		sub_scene_root->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), main_flags);

		// Pack the scene.
		Ref<PackedScene> packed_scene;
		packed_scene.instantiate();
		const Error err = packed_scene->pack(main_scene_root);
		CHECK(err == OK);

		// Make sure the right connections are in packed scene.
		Ref<SceneState> state = packed_scene->get_state();
		CHECK_EQ(state->get_connection_count(), 3);
	}

	/*
	// FIXME: This subcase requires GH-48064 to be fixed.
	SUBCASE("Signals that should not be saved") {
		int subscene_flags = Object::CONNECT_PERSIST | Object::CONNECT_INHERITED;
		// subscene node to itself
		sub_scene_node->connect("ready", callable_mp(sub_scene_node, &Node::is_ready), subscene_flags);
		// subscene node to subscene root
		sub_scene_node->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), subscene_flags);
		//subscene root to subscene root (connected within sub scene)
		sub_scene_root->connect("ready", callable_mp(sub_scene_root, &Node::is_ready), subscene_flags);

		// Pack the scene.
		Ref<PackedScene> packed_scene;
		packed_scene.instantiate();
		const Error err = packed_scene->pack(main_scene_root);
		CHECK(err == OK);

		// Make sure the right connections are in packed scene.
		Ref<SceneState> state = packed_scene->get_state();
		CHECK_EQ(state->get_connection_count(), 0);
	}
	*/

	memdelete(main_scene_root);
}

TEST_CASE("[PackedScene] Clear Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Clear the packed scene.
	packed_scene.clear();

	// Check if it has been cleared.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK_FALSE(state->get_node_count() == 1);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Can Instantiate Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Check if the packed scene can be instantiated.
	const bool can_instantiate = packed_scene.can_instantiate();
	CHECK(can_instantiate == true);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Instantiate Packed Scene") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Instantiate the packed scene.
	Node *instance = packed_scene.instantiate();
	CHECK(instance != nullptr);
	CHECK(instance->get_name() == "TestScene");

	memdelete(scene);
	memdelete(instance);
}

TEST_CASE("[PackedScene] Instantiate Packed Scene With Children") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Add persisting child nodes to the scene.
	Node *child1 = memnew(Node);
	child1->set_name("Child1");
	scene->add_child(child1);
	child1->set_owner(scene);

	Node *child2 = memnew(Node);
	child2->set_name("Child2");
	scene->add_child(child2);
	child2->set_owner(scene);

	// Add non persisting child node to the scene.
	Node *child3 = memnew(Node);
	child3->set_name("Child3");
	scene->add_child(child3);

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Instantiate the packed scene.
	Node *instance = packed_scene.instantiate();
	CHECK(instance != nullptr);
	CHECK(instance->get_name() == "TestScene");

	// Validate the child nodes of the instantiated scene.
	CHECK(instance->get_child_count() == 2);
	CHECK(instance->get_child(0)->get_name() == "Child1");
	CHECK(instance->get_child(1)->get_name() == "Child2");
	CHECK(instance->get_child(0)->get_owner() == instance);
	CHECK(instance->get_child(1)->get_owner() == instance);

	memdelete(scene);
	memdelete(instance);
}

TEST_CASE("[PackedScene] Set Path") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Set a new path for the packed scene.
	const String new_path = "NewTestPath";
	packed_scene.set_path(new_path);

	// Check if the path has been set correctly.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state->get_path() == new_path);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Replace State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	PackedScene packed_scene;
	packed_scene.pack(scene);

	// Create another scene state to replace with.
	Ref<SceneState> new_state = memnew(SceneState);
	new_state->set_path("NewPath");

	// Replace the state.
	packed_scene.replace_state(new_state);

	// Check if the state has been replaced.
	Ref<SceneState> state = packed_scene.get_state();
	CHECK(state.is_valid());
	CHECK(state == new_state);

	memdelete(scene);
}

TEST_CASE("[PackedScene] Recreate State") {
	// Create a scene to pack.
	Node *scene = memnew(Node);
	scene->set_name("TestScene");

	// Pack the scene.
	Ref<PackedScene> packed_scene;
	packed_scene.instantiate();
	packed_scene->pack(scene);

	// Recreate the state.
	packed_scene->recreate_state();

	// Check if the state has been recreated.
	Ref<SceneState> state = packed_scene->get_state();
	CHECK(state.is_valid());
	CHECK(state->get_node_count() == 0); // Since the state was recreated, it should be empty.

	memdelete(scene);
}

} // namespace TestPackedScene

// === Merged from tests/scene/test_sky.cpp ===
/*  test_sky.cpp                                                          */
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

#include "tests/test_macros.h"



#ifndef _3D_DISABLED

#include "servers/rendering/renderer.h"

namespace TestSky {

TEST_CASE("[SceneTree][Sky] Constructor") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	CHECK(test_sky->get_process_mode() == Sky::PROCESS_MODE_AUTOMATIC);
	CHECK(test_sky->get_radiance_size() == Sky::RADIANCE_SIZE_256);
	CHECK(test_sky->get_material().is_null());
}

TEST_CASE("[SceneTree][Sky] Radiance size setter and getter") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	// Check default.
	CHECK(test_sky->get_radiance_size() == Sky::RADIANCE_SIZE_256);

	test_sky->set_radiance_size(Sky::RADIANCE_SIZE_1024);
	CHECK(test_sky->get_radiance_size() == Sky::RADIANCE_SIZE_1024);

	ERR_PRINT_OFF;
	// Check setting invalid radiance size.
	test_sky->set_radiance_size(Sky::RADIANCE_SIZE_MAX);
	ERR_PRINT_ON;

	CHECK(test_sky->get_radiance_size() == Sky::RADIANCE_SIZE_1024);
}

TEST_CASE("[SceneTree][Sky] Process mode setter and getter") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	// Check default.
	CHECK(test_sky->get_process_mode() == Sky::PROCESS_MODE_AUTOMATIC);

	test_sky->set_process_mode(Sky::PROCESS_MODE_INCREMENTAL);
	CHECK(test_sky->get_process_mode() == Sky::PROCESS_MODE_INCREMENTAL);
}

TEST_CASE("[SceneTree][Sky] Material setter and getter") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	Ref<Material> material;
	material.instantiate();

	SUBCASE("Material passed to the class should remain the same") {
		test_sky->set_material(material);
		CHECK(test_sky->get_material() == material);
	}
	SUBCASE("Material passed many times to the class should remain the same") {
		test_sky->set_material(material);
		test_sky->set_material(material);
		test_sky->set_material(material);
		CHECK(test_sky->get_material() == material);
	}
	SUBCASE("Material rewrite testing") {
		Ref<Material> material1;
		Ref<Material> material2;
		material1.instantiate();
		material2.instantiate();

		test_sky->set_material(material1);
		test_sky->set_material(material2);
		CHECK_MESSAGE(test_sky->get_material() != material1,
				"After rewrite, second material should be in class.");
		CHECK_MESSAGE(test_sky->get_material() == material2,
				"After rewrite, second material should be in class.");
	}

	SUBCASE("Assign same material to two skys") {
		Ref<Sky> sky2;
		sky2.instantiate();

		test_sky->set_material(material);
		sky2->set_material(material);
		CHECK_MESSAGE(test_sky->get_material() == sky2->get_material(),
				"Both skys should have the same material.");
	}

	SUBCASE("Swapping materials between two skys") {
		Ref<Sky> sky2;
		sky2.instantiate();
		Ref<Material> material1;
		Ref<Material> material2;
		material1.instantiate();
		material2.instantiate();

		test_sky->set_material(material1);
		sky2->set_material(material2);
		CHECK(test_sky->get_material() == material1);
		CHECK(sky2->get_material() == material2);

		// Do the swap.
		Ref<Material> temp = test_sky->get_material();
		test_sky->set_material(sky2->get_material());
		sky2->set_material(temp);

		CHECK(test_sky->get_material() == material2);
		CHECK(sky2->get_material() == material1);
	}
}

TEST_CASE("[SceneTree][Sky] Invalid radiance size handling") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	// Attempt to set an invalid radiance size.
	ERR_PRINT_OFF;
	test_sky->set_radiance_size(Sky::RADIANCE_SIZE_MAX);
	ERR_PRINT_ON;

	// Verify that the radiance size remains unchanged.
	CHECK(test_sky->get_radiance_size() == Sky::RADIANCE_SIZE_256);
}

TEST_CASE("[SceneTree][Sky] Process mode variations") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	// Test all process modes.
	const Sky::ProcessMode process_modes[] = {
		Sky::PROCESS_MODE_AUTOMATIC,
		Sky::PROCESS_MODE_QUALITY,
		Sky::PROCESS_MODE_INCREMENTAL,
		Sky::PROCESS_MODE_REALTIME
	};

	for (Sky::ProcessMode mode : process_modes) {
		test_sky->set_process_mode(mode);
		CHECK(test_sky->get_process_mode() == mode);
	}
}

TEST_CASE("[SceneTree][Sky] Radiance size variations") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	// Test all radiance sizes except MAX.
	const Sky::RadianceSize radiance_sizes[] = {
		Sky::RADIANCE_SIZE_32,
		Sky::RADIANCE_SIZE_64,
		Sky::RADIANCE_SIZE_128,
		Sky::RADIANCE_SIZE_256,
		Sky::RADIANCE_SIZE_512,
		Sky::RADIANCE_SIZE_1024,
		Sky::RADIANCE_SIZE_2048
	};

	for (Sky::RadianceSize size : radiance_sizes) {
		test_sky->set_radiance_size(size);
		CHECK(test_sky->get_radiance_size() == size);
	}
}

TEST_CASE("[SceneTree][Sky] Null material handling") {
	Ref<Sky> test_sky;
	test_sky.instantiate();

	SUBCASE("Setting null material") {
		test_sky->set_material(Ref<Material>());
		CHECK(test_sky->get_material().is_null());
	}

	SUBCASE("Overwriting existing material with null") {
		Ref<Material> material;
		material.instantiate();
		test_sky->set_material(material);
		test_sky->set_material(Ref<Material>());

		CHECK(test_sky->get_material().is_null());
	}
}

TEST_CASE("[SceneTree][Sky] RID generation") {
	Ref<Sky> test_sky;
	test_sky.instantiate();
	// Check validity.
	CHECK(!test_sky->get_rid().is_valid());
}

} // namespace TestSky

#endif // _3D_DISABLED

// === Merged from tests/scene/test_style_box_empty.cpp ===
/*  test_style_box_empty.cpp                                              */
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

#include "tests/test_macros.h"



#include "scene/resources/theme.h"

namespace TestStyleBoxEmpty {

TEST_CASE("[StyleBox] STYLE_EMPTY Constructor") {
	Ref<StyleBox> style_box_empty = memnew(StyleBox(StyleBox::STYLE_EMPTY));

	CHECK(style_box_empty->get_style_type() == StyleBox::STYLE_EMPTY);
	CHECK(style_box_empty->get_offset() == Point2(0, 0));
	CHECK(style_box_empty->get_current_item_drawn() == nullptr);
}

TEST_CASE("[StyleBox] STYLE_EMPTY set_content_margin, set_content_margin_all, set_content_margin_individual, get_content_margin, get_margin") {
	Ref<StyleBox> style_box_empty = memnew(StyleBox(StyleBox::STYLE_EMPTY));

	SUBCASE("set_content_margin, get_content_margin") {
		style_box_empty->set_content_margin(SIDE_LEFT, 2);
		style_box_empty->set_content_margin(SIDE_TOP, 3);
		style_box_empty->set_content_margin(SIDE_RIGHT, 4);
		style_box_empty->set_content_margin(SIDE_BOTTOM, 5);

		CHECK(style_box_empty->get_content_margin(SIDE_LEFT) == 2);
		CHECK(style_box_empty->get_content_margin(SIDE_TOP) == 3);
		CHECK(style_box_empty->get_content_margin(SIDE_RIGHT) == 4);
		CHECK(style_box_empty->get_content_margin(SIDE_BOTTOM) == 5);
	}

	SUBCASE("set_content_margin_all, get_content_margin") {
		style_box_empty->set_content_margin_all(10);

		CHECK(style_box_empty->get_content_margin(SIDE_LEFT) == 10);
		CHECK(style_box_empty->get_content_margin(SIDE_TOP) == 10);
		CHECK(style_box_empty->get_content_margin(SIDE_RIGHT) == 10);
		CHECK(style_box_empty->get_content_margin(SIDE_BOTTOM) == 10);
	}

	SUBCASE("set_content_margin, get_margin") {
		style_box_empty->set_content_margin(SIDE_LEFT, -1);
		style_box_empty->set_content_margin(SIDE_RIGHT, -2);
		style_box_empty->set_content_margin(SIDE_TOP, 3);
		style_box_empty->set_content_margin(SIDE_BOTTOM, 4);

		CHECK_MESSAGE(style_box_empty->get_margin(SIDE_LEFT) == 0,
				"Value is lesser than zero, so it returns 0");
		CHECK_MESSAGE(style_box_empty->get_margin(SIDE_RIGHT) == 0,
				"Value is lesser than zero, so it returns 0");
		CHECK_MESSAGE(style_box_empty->get_margin(SIDE_TOP) == 3,
				"Value is higher than zero, so it returns value");
		CHECK_MESSAGE(style_box_empty->get_margin(SIDE_BOTTOM) == 4,
				"Value is higher than zero, so it returns value");
	}

	SUBCASE("set_content_margin_individual, get_minimum_size") {
		style_box_empty->set_content_margin_individual(-1, 2, 5, 15);

		CHECK(style_box_empty->get_minimum_size() == Size2(5, 17));
	}

	SUBCASE("set_content_margin_individual, get_offset") {
		style_box_empty->set_content_margin_individual(-3, 5, 1, 2);

		CHECK_MESSAGE(style_box_empty->get_offset() == Point2(0, 5),
				"Returns Point2 with get_margin of SIDE_LEFT and SIDE_TOP");
	}
}

} // namespace TestStyleBoxEmpty

// === Merged from tests/scene/test_style_box_line.cpp ===
/*  test_style_box_line.cpp                                               */
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

#include "tests/test_macros.h"



#include "scene/resources/theme.h"

namespace TestStyleBoxLine {

TEST_CASE("[StyleBox] STYLE_LINE Constructor") {
	Ref<StyleBox> style_box_line = memnew(StyleBox(StyleBox::STYLE_LINE));

	CHECK(style_box_line->get_style_type() == StyleBox::STYLE_LINE);
	CHECK(style_box_line->get_color() == Color(0, 0, 0, 1));
	CHECK(style_box_line->get_thickness() == 1);
	CHECK(style_box_line->is_vertical() == false);
	CHECK(style_box_line->get_grow_begin() == 1.0);
	CHECK(style_box_line->get_grow_end() == 1.0);
}

TEST_CASE("[StyleBox] STYLE_LINE set_color, get_color") {
	Ref<StyleBox> style_box_line = memnew(StyleBox(StyleBox::STYLE_LINE));
	Color color = Color(0.1, 0.2, 0.3, 1.0);

	style_box_line->set_color(color);
	CHECK(style_box_line->get_color() == color);
}

TEST_CASE("[StyleBox] STYLE_LINE set_thickness, get_thickness") {
	Ref<StyleBox> style_box_line = memnew(StyleBox(StyleBox::STYLE_LINE));

	style_box_line->set_thickness(5);
	CHECK(style_box_line->get_thickness() == 5);
}

TEST_CASE("[StyleBox] STYLE_LINE set_vertical, is_vertical") {
	Ref<StyleBox> style_box_line = memnew(StyleBox(StyleBox::STYLE_LINE));

	style_box_line->set_vertical(true);
	CHECK(style_box_line->is_vertical() == true);
}

TEST_CASE("[StyleBox] STYLE_LINE set_grow_begin, get_grow_begin, set_grow_end, get_grow_end") {
	Ref<StyleBox> style_box_line = memnew(StyleBox(StyleBox::STYLE_LINE));
	float grow_value = 3.5;

	SUBCASE("set_grow_begin, get_grow_begin") {
		style_box_line->set_grow_begin(grow_value);
		CHECK(style_box_line->get_grow_begin() == grow_value);
	}

	SUBCASE("set_grow_end, get_grow_end") {
		style_box_line->set_grow_end(grow_value);
		CHECK(style_box_line->get_grow_end() == grow_value);
	}
}

} // namespace TestStyleBoxLine

// === Merged from tests/scene/test_style_box_texture.cpp ===
/*  test_style_box_texture.cpp                                            */
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

#include "tests/test_macros.h"



#include "scene/resources/theme.h"

namespace TestStyleBoxTexture {

TEST_CASE("[StyleBox] STYLE_TEXTURE Constructor") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	CHECK(style_box_texture->get_style_type() == StyleBox::STYLE_TEXTURE);
	CHECK(style_box_texture->get_h_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_STRETCH);
	CHECK(style_box_texture->get_v_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_STRETCH);
	CHECK(style_box_texture->is_draw_center_enabled() == true);

	CHECK(style_box_texture->get_expand_margin(SIDE_LEFT) == 0);
	CHECK(style_box_texture->get_expand_margin(SIDE_TOP) == 0);
	CHECK(style_box_texture->get_expand_margin(SIDE_RIGHT) == 0);
	CHECK(style_box_texture->get_expand_margin(SIDE_BOTTOM) == 0);

	CHECK(style_box_texture->get_modulate() == Color(1, 1, 1, 1));
	CHECK(style_box_texture->get_region_rect() == Rect2(0, 0, 0, 0));
	CHECK(style_box_texture->get_texture() == Ref<Texture2D>());

	CHECK(style_box_texture->get_texture_margin(SIDE_LEFT) == 0);
	CHECK(style_box_texture->get_texture_margin(SIDE_TOP) == 0);
	CHECK(style_box_texture->get_texture_margin(SIDE_RIGHT) == 0);
	CHECK(style_box_texture->get_texture_margin(SIDE_BOTTOM) == 0);
}

TEST_CASE("[StyleBox] STYLE_TEXTURE set_texture, get_texture") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));
	Ref<Texture2D> texture = memnew(Texture2D);

	style_box_texture->set_texture(texture);
	CHECK(style_box_texture->get_texture() == texture);
}

TEST_CASE("[StyleBox] STYLE_TEXTURE set_texture_margin, set_texture_margin_all, set_texture_margin_individual, get_texture_margin") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	SUBCASE("set_texture_margin, get_texture_margin") {
		style_box_texture->set_texture_margin(SIDE_LEFT, 1);
		style_box_texture->set_texture_margin(SIDE_TOP, 1);
		style_box_texture->set_texture_margin(SIDE_RIGHT, 1);
		style_box_texture->set_texture_margin(SIDE_BOTTOM, 1);

		CHECK(style_box_texture->get_texture_margin(SIDE_LEFT) == 1);
		CHECK(style_box_texture->get_texture_margin(SIDE_TOP) == 1);
		CHECK(style_box_texture->get_texture_margin(SIDE_RIGHT) == 1);
		CHECK(style_box_texture->get_texture_margin(SIDE_BOTTOM) == 1);
	}

	SUBCASE("set_texture_margin_all") {
		style_box_texture->set_texture_margin_all(2);

		CHECK(style_box_texture->get_texture_margin(SIDE_LEFT) == 2);
		CHECK(style_box_texture->get_texture_margin(SIDE_TOP) == 2);
		CHECK(style_box_texture->get_texture_margin(SIDE_RIGHT) == 2);
		CHECK(style_box_texture->get_texture_margin(SIDE_BOTTOM) == 2);
	}

	SUBCASE("set_texture_margin_individual") {
		style_box_texture->set_texture_margin_individual(3, 4, 5, 6);

		CHECK(style_box_texture->get_texture_margin(SIDE_LEFT) == 3);
		CHECK(style_box_texture->get_texture_margin(SIDE_TOP) == 4);
		CHECK(style_box_texture->get_texture_margin(SIDE_RIGHT) == 5);
		CHECK(style_box_texture->get_texture_margin(SIDE_BOTTOM) == 6);
	}
}

TEST_CASE("[StyleBox] STYLE_TEXTURE set_expand_margin, set_expand_margin_all, set_expand_margin_individual") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	SUBCASE("set_expand_margin, get_expand_margin") {
		style_box_texture->set_expand_margin(SIDE_LEFT, 1);
		style_box_texture->set_expand_margin(SIDE_TOP, 1);
		style_box_texture->set_expand_margin(SIDE_RIGHT, 1);
		style_box_texture->set_expand_margin(SIDE_BOTTOM, 1);

		CHECK(style_box_texture->get_expand_margin(SIDE_LEFT) == 1);
		CHECK(style_box_texture->get_expand_margin(SIDE_TOP) == 1);
		CHECK(style_box_texture->get_expand_margin(SIDE_RIGHT) == 1);
		CHECK(style_box_texture->get_expand_margin(SIDE_BOTTOM) == 1);
	}

	SUBCASE("set_expand_margin_all") {
		style_box_texture->set_expand_margin_all(2);

		CHECK(style_box_texture->get_expand_margin(SIDE_LEFT) == 2);
		CHECK(style_box_texture->get_expand_margin(SIDE_TOP) == 2);
		CHECK(style_box_texture->get_expand_margin(SIDE_RIGHT) == 2);
		CHECK(style_box_texture->get_expand_margin(SIDE_BOTTOM) == 2);
	}

	SUBCASE("set_expand_margin_individual") {
		style_box_texture->set_expand_margin_individual(3, 4, 5, 6);

		CHECK(style_box_texture->get_expand_margin(SIDE_LEFT) == 3);
		CHECK(style_box_texture->get_expand_margin(SIDE_TOP) == 4);
		CHECK(style_box_texture->get_expand_margin(SIDE_RIGHT) == 5);
		CHECK(style_box_texture->get_expand_margin(SIDE_BOTTOM) == 6);
	}
}

TEST_CASE("[StyleBox] STYLE_TEXTURE set_region_rect, get_region_rect") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	style_box_texture->set_region_rect(Rect2(1, 1, 1, 1));
	CHECK(style_box_texture->get_region_rect() == Rect2(1, 1, 1, 1));
}

TEST_CASE("[StyleBox] STYLE_TEXTURE set_draw_center, get_draw_center") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	style_box_texture->set_draw_center(false);
	CHECK(style_box_texture->is_draw_center_enabled() == false);
}

TEST_CASE("[StyleBox] STYLE_TEXTURE set_h_axis_stretch_mode, set_v_axis_stretch_mode, get_h_axis_stretch_mode, get_v_axis_stretch_mode") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	SUBCASE("set_h_axis_stretch_mode, get_h_axis_stretch_mode") {
		style_box_texture->set_h_axis_stretch_mode(StyleBox::AXIS_STRETCH_MODE_TILE);
		CHECK(style_box_texture->get_h_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_TILE);

		style_box_texture->set_h_axis_stretch_mode(StyleBox::AXIS_STRETCH_MODE_TILE_FIT);
		CHECK(style_box_texture->get_h_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_TILE_FIT);

		style_box_texture->set_h_axis_stretch_mode(StyleBox::AXIS_STRETCH_MODE_STRETCH);
		CHECK(style_box_texture->get_h_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_STRETCH);
	}

	SUBCASE("set_v_axis_stretch_mode, get_v_axis_stretch_mode") {
		style_box_texture->set_v_axis_stretch_mode(StyleBox::AXIS_STRETCH_MODE_TILE);
		CHECK(style_box_texture->get_v_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_TILE);

		style_box_texture->set_v_axis_stretch_mode(StyleBox::AXIS_STRETCH_MODE_TILE_FIT);
		CHECK(style_box_texture->get_v_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_TILE_FIT);

		style_box_texture->set_v_axis_stretch_mode(StyleBox::AXIS_STRETCH_MODE_STRETCH);
		CHECK(style_box_texture->get_v_axis_stretch_mode() == StyleBox::AXIS_STRETCH_MODE_STRETCH);
	}
}

TEST_CASE("[StyleBox] STYLE_TEXTURE set_modulate, get_modulate") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	style_box_texture->set_modulate(Color(0, 0, 0, 0));
	CHECK(style_box_texture->get_modulate() == Color(0, 0, 0, 0));
}

TEST_CASE("[StyleBox] STYLE_TEXTURE get_draw_rect") {
	Ref<StyleBox> style_box_texture = memnew(StyleBox(StyleBox::STYLE_TEXTURE));

	style_box_texture->set_expand_margin_all(5);
	CHECK(style_box_texture->get_draw_rect(Rect2(0, 0, 1, 1)) == Rect2(-5, -5, 11, 11));
}

} // namespace TestStyleBoxTexture

// === Merged from tests/scene/test_theme.cpp ===
/*  test_theme.cpp                                                        */
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

#include "tests/test_macros.h"



#include "servers/rendering/renderer.h"
#include "scene/resources/theme.h"
#include "tests/test_macros.h"

namespace TestTheme {

class Fixture {
public:
	struct DataEntry {
		Theme::DataType type;
		Variant value;
	} const valid_data[Theme::DATA_TYPE_MAX] = {
		{ Theme::DATA_TYPE_COLOR, Color() },
		{ Theme::DATA_TYPE_CONSTANT, 42 },
		{ Theme::DATA_TYPE_FONT, Ref<FontFile>(memnew(FontFile)) },
		{ Theme::DATA_TYPE_FONT_SIZE, 42 },
		{ Theme::DATA_TYPE_ICON, Ref<Texture2D>(memnew(ImageTexture)) },
		{ Theme::DATA_TYPE_STYLEBOX, Ref<StyleBox>(memnew(StyleBox(StyleBox::STYLE_FLAT))) },
	};

	const String valid_item_name = "valid_item_name";
	const String valid_type_name = "ValidTypeName";
};

TEST_CASE_FIXTURE(Fixture, "[Theme] Good theme type names") {
	String names[] = {
		"", // Empty name.
		"CapitalizedName",
		"snake_cased_name",
		"42",
		"_Underscore_",
	};

	SUBCASE("add_type") {
		for (const String &name : names) {
			Ref<Theme> theme = memnew(Theme);

			ErrorDetector ed;
			theme->add_type(name);
			CHECK_FALSE(ed.has_error);
		}
	}

	SUBCASE("set_theme_item") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);

				ErrorDetector ed;
				theme->set_theme_item(entry.type, valid_item_name, name, entry.value);
				CHECK_FALSE(ed.has_error);
			}
		}
	}

	SUBCASE("add_theme_item_type") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);

				ErrorDetector ed;
				theme->add_theme_item_type(entry.type, name);
				CHECK_FALSE(ed.has_error);
			}
		}
	}

	SUBCASE("set_type_variation") {
		for (const String &name : names) {
			if (name == String()) { // Skip empty here, not allowed.
				continue;
			}
			Ref<Theme> theme = memnew(Theme);

			ErrorDetector ed;
			theme->set_type_variation(valid_type_name, name);
			CHECK_FALSE(ed.has_error);
		}
		for (const String &name : names) {
			if (name == String()) { // Skip empty here, not allowed.
				continue;
			}
			Ref<Theme> theme = memnew(Theme);

			ErrorDetector ed;
			theme->set_type_variation(name, valid_type_name);
			CHECK_FALSE(ed.has_error);
		}
	}
}

TEST_CASE_FIXTURE(Fixture, "[Theme] Bad theme type names") {
	String names[] = {
		"With/Slash",
		"With Space",
		"With@various$symbols!",
		String::utf8("contains_汉字"),
	};

	ERR_PRINT_OFF; // All these rightfully print errors.

	SUBCASE("add_type") {
		for (const String &name : names) {
			Ref<Theme> theme = memnew(Theme);

			ErrorDetector ed;
			theme->add_type(name);
			CHECK(ed.has_error);
		}
	}

	SUBCASE("set_theme_item") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);

				ErrorDetector ed;
				theme->set_theme_item(entry.type, valid_item_name, name, entry.value);
				CHECK(ed.has_error);
			}
		}
	}

	SUBCASE("add_theme_item_type") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);

				ErrorDetector ed;
				theme->add_theme_item_type(entry.type, name);
				CHECK(ed.has_error);
			}
		}
	}

	SUBCASE("set_type_variation") {
		for (const String &name : names) {
			Ref<Theme> theme = memnew(Theme);

			ErrorDetector ed;
			theme->set_type_variation(valid_type_name, name);
			CHECK(ed.has_error);
		}
		for (const String &name : names) {
			Ref<Theme> theme = memnew(Theme);

			ErrorDetector ed;
			theme->set_type_variation(name, valid_type_name);
			CHECK(ed.has_error);
		}
	}

	ERR_PRINT_ON;
}

TEST_CASE_FIXTURE(Fixture, "[Theme] Good theme item names") {
	String names[] = {
		"CapitalizedName",
		"snake_cased_name",
		"42",
		"_Underscore_",
	};

	SUBCASE("set_theme_item") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);

				ErrorDetector ed;
				theme->set_theme_item(entry.type, name, valid_type_name, entry.value);
				CHECK_FALSE(ed.has_error);
				CHECK(theme->has_theme_item(entry.type, name, valid_type_name));
			}
		}
	}

	SUBCASE("rename_theme_item") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);
				theme->set_theme_item(entry.type, valid_item_name, valid_type_name, entry.value);

				ErrorDetector ed;
				theme->rename_theme_item(entry.type, valid_item_name, name, valid_type_name);
				CHECK_FALSE(ed.has_error);
				CHECK_FALSE(theme->has_theme_item(entry.type, valid_item_name, valid_type_name));
				CHECK(theme->has_theme_item(entry.type, name, valid_type_name));
			}
		}
	}
}

TEST_CASE_FIXTURE(Fixture, "[Theme] Bad theme item names") {
	String names[] = {
		"", // Empty name.
		"With/Slash",
		"With Space",
		"With@various$symbols!",
		String::utf8("contains_汉字"),
	};

	ERR_PRINT_OFF; // All these rightfully print errors.

	SUBCASE("set_theme_item") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);

				ErrorDetector ed;
				theme->set_theme_item(entry.type, name, valid_type_name, entry.value);
				CHECK(ed.has_error);
				CHECK_FALSE(theme->has_theme_item(entry.type, name, valid_type_name));
			}
		}
	}

	SUBCASE("rename_theme_item") {
		for (const String &name : names) {
			for (const DataEntry &entry : valid_data) {
				Ref<Theme> theme = memnew(Theme);
				theme->set_theme_item(entry.type, valid_item_name, valid_type_name, entry.value);

				ErrorDetector ed;
				theme->rename_theme_item(entry.type, valid_item_name, name, valid_type_name);
				CHECK(ed.has_error);
				CHECK(theme->has_theme_item(entry.type, valid_item_name, valid_type_name));
				CHECK_FALSE(theme->has_theme_item(entry.type, name, valid_type_name));
			}
		}
	}

	ERR_PRINT_ON;
}

} // namespace TestTheme

TEST_FORCE_LINK(test_scene_resources)

