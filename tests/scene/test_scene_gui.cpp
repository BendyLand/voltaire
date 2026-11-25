
// === Merged from tests/scene/test_aspect_ratio_container.cpp ===
/*  test_aspect_ratio_container.cpp                                       */
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



#include "scene/gui/aspect_ratio_container.h"
#include "scene/main/scene_tree.h"

namespace TestAspectRatioContainer {

TEST_CASE("[SceneTree][AspectRatioContainer] Default Properties") {
	AspectRatioContainer *aspect_ratio_container = memnew(AspectRatioContainer);
	Window *root = SceneTree::get_root();
	root->add_child(aspect_ratio_container);
	Control *child_control = memnew(Control);
	aspect_ratio_container->add_child(child_control);

	CHECK_MESSAGE(
			aspect_ratio_container->get_stretch_mode() == AspectRatioContainer::STRETCH_FIT,
			"AspectRatioContainer stretch mode is set to STRETCH_FIT by default.");

	CHECK_MESSAGE(
			Math::is_equal_approx(aspect_ratio_container->get_ratio(), 1.0f),
			"AspectRatioContainer ratio is set to 1.0 by default.");

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(0, 0)),
			"Child control is positioned at the top-left corner of the AspectRatioContainer by default.");

	CHECK_MESSAGE(
			child_control->get_h_size_flags() == Control::SIZE_FILL,
			"Child control horizontal size flags are set to SIZE_FILL by default.");

	CHECK_MESSAGE(
			child_control->get_v_size_flags() == Control::SIZE_FILL,
			"Child control vertical size flags are set to SIZE_FILL by default.");

	memdelete(child_control);
	memdelete(aspect_ratio_container);
}

TEST_CASE("[SceneTree][AspectRatioContainer] Aspect Ratio") {
	AspectRatioContainer *aspect_ratio_container = memnew(AspectRatioContainer);
	Window *root = SceneTree::get_root();
	root->add_child(aspect_ratio_container);
	Control *child_control = memnew(Control);
	aspect_ratio_container->add_child(child_control);

	aspect_ratio_container->set_size(Size2(200, 100));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 100)),
			"Child control is resized to maintain aspect ratio when AspectRatioContainer is resized.");

	aspect_ratio_container->set_size(Size2(100, 200));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 100)),
			"Child control is resized to maintain aspect ratio when AspectRatioContainer is resized.");

	aspect_ratio_container->set_ratio(2.0f);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 50)),
			"Child control is resized to maintain aspect ratio when AspectRatioContainer ratio is changed.");

	aspect_ratio_container->set_ratio(0.5f);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 200)),
			"Child control is resized to maintain aspect ratio when AspectRatioContainer ratio is changed.");

	aspect_ratio_container->set_size(Size2(200, 100));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(50, 100)),
			"Child control is resized to maintain aspect ratio when AspectRatioContainer is resized after ratio change.");

	memdelete(child_control);
	memdelete(aspect_ratio_container);
}

TEST_CASE("[SceneTree][AspectRatioContainer] Stretch Modes") {
	AspectRatioContainer *aspect_ratio_container = memnew(AspectRatioContainer);
	Window *root = SceneTree::get_root();
	root->add_child(aspect_ratio_container);
	Control *child_control = memnew(Control);
	aspect_ratio_container->add_child(child_control);

	aspect_ratio_container->set_size(Size2(200, 100));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 100)),
			"Child control is resized to maintain aspect ratio in STRETCH_FIT mode.");

	aspect_ratio_container->set_stretch_mode(AspectRatioContainer::STRETCH_WIDTH_CONTROLS_HEIGHT);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(200, 200)),
			"Child control height is adjusted to maintain aspect ratio in STRETCH_WIDTH_CONTROLS_HEIGHT mode.");

	aspect_ratio_container->set_stretch_mode(AspectRatioContainer::STRETCH_HEIGHT_CONTROLS_WIDTH);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 100)),
			"Child control width is adjusted to maintain aspect ratio in STRETCH_HEIGHT_CONTROLS_WIDTH mode.");

	aspect_ratio_container->set_stretch_mode(AspectRatioContainer::STRETCH_COVER);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(200, 200)),
			"Child control is resized to cover the entire AspectRatioContainer in STRETCH_COVER mode.");

	aspect_ratio_container->set_size(Size2(100, 200));
	aspect_ratio_container->set_stretch_mode(AspectRatioContainer::STRETCH_FIT);
	SceneTree::process(0);

	CHECK_MESSAGE(
			aspect_ratio_container->get_stretch_mode() == AspectRatioContainer::STRETCH_FIT,
			"AspectRatioContainer stretch mode is set to STRETCH_FIT by default.");

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 100)),
			"Child control is resized to maintain aspect ratio in STRETCH_FIT mode.");

	aspect_ratio_container->set_stretch_mode(AspectRatioContainer::STRETCH_WIDTH_CONTROLS_HEIGHT);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 100)),
			"Child control height is adjusted to maintain aspect ratio in STRETCH_WIDTH_CONTROLS_HEIGHT mode.");

	aspect_ratio_container->set_stretch_mode(AspectRatioContainer::STRETCH_HEIGHT_CONTROLS_WIDTH);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(200, 200)),
			"Child control width is adjusted to maintain aspect ratio in STRETCH_HEIGHT_CONTROLS_WIDTH mode.");

	aspect_ratio_container->set_stretch_mode(AspectRatioContainer::STRETCH_COVER);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(200, 200)),
			"Child control is resized to cover the entire AspectRatioContainer in STRETCH_COVER mode.");

	memdelete(child_control);
	memdelete(aspect_ratio_container);
}

TEST_CASE("[SceneTree][AspectRatioContainer] Alignment modes") {
	AspectRatioContainer *aspect_ratio_container = memnew(AspectRatioContainer);
	Window *root = SceneTree::get_root();
	root->add_child(aspect_ratio_container);
	Control *child_control = memnew(Control);
	aspect_ratio_container->add_child(child_control);

	SUBCASE("Horizontal Alignment") {
		aspect_ratio_container->set_size(Size2(200, 100));
		SceneTree::process(0);

		CHECK_MESSAGE(
				aspect_ratio_container->get_alignment_horizontal() == AspectRatioContainer::ALIGNMENT_CENTER,
				"AspectRatioContainer horizontal alignment is set to ALIGNMENT_CENTER by default.");

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is aligned to the center in ALIGNMENT_CENTER horizontal alignment mode.");

		aspect_ratio_container->set_alignment_horizontal(AspectRatioContainer::ALIGNMENT_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left in ALIGNMENT_BEGIN horizontal alignment mode.");

		aspect_ratio_container->set_alignment_horizontal(AspectRatioContainer::ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(100, 0)),
				"Child control is aligned to the right in ALIGNMENT_END horizontal alignment mode.");
	}

	SUBCASE("Vertical Alignment") {
		aspect_ratio_container->set_size(Size2(100, 200));
		SceneTree::process(0);

		CHECK_MESSAGE(
				aspect_ratio_container->get_alignment_vertical() == AspectRatioContainer::ALIGNMENT_CENTER,
				"AspectRatioContainer vertical alignment is set to ALIGNMENT_CENTER by default.");

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 50)),
				"Child control is aligned to the center in ALIGNMENT_CENTER vertical alignment mode.");

		aspect_ratio_container->set_alignment_vertical(AspectRatioContainer::ALIGNMENT_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top in ALIGNMENT_BEGIN vertical alignment mode.");

		aspect_ratio_container->set_alignment_vertical(AspectRatioContainer::ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 100)),
				"Child control is aligned to the bottom in ALIGNMENT_END vertical alignment mode.");
	}

	memdelete(child_control);
	memdelete(aspect_ratio_container);
}

TEST_CASE("[SceneTree][AspectRatioContainer] Container Sizing Flags") {
	AspectRatioContainer *aspect_ratio_container = memnew(AspectRatioContainer);
	Window *root = SceneTree::get_root();
	root->add_child(aspect_ratio_container);
	Control *child_control = memnew(Control);
	aspect_ratio_container->add_child(child_control);

	SUBCASE("Horizontal Flags") {
		aspect_ratio_container->set_size(Size2(200, 100));
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is vertically centered by default with SIZE_FILL.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is aligned to the left of its would-be area with SIZE_FILL in SIZE_SHRINK_BEGIN horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(100, 0)),
				"Child control is centered in its would-be area with SIZE_FILL in SIZE_SHRINK_CENTER horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(150, 0)),
				"Child control is aligned to the right of its would-be area with SIZE_FILL in SIZE_SHRINK_END horizontal size flag mode.");
	}

	SUBCASE("Horizontal Flags RTL") {
		aspect_ratio_container->set_size(Size2(200, 100));
		aspect_ratio_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is vertically centered by default with SIZE_FILL.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(150, 0)),
				"Child control is aligned to the right of its would-be area with SIZE_FILL in RTL SIZE_SHRINK_BEGIN horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(100, 0)),
				"Child control is centered in its would-be area with SIZE_FILL in SIZE_SHRINK_CENTER horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is aligned to the left of its would-be area with SIZE_FILL in RTL SIZE_SHRINK_END horizontal size flag mode.");
	}

	SUBCASE("Vertical Flags") {
		aspect_ratio_container->set_size(Size2(100, 200));
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 50)),
				"Child control is horizontally centered by default with SIZE_FILL.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 50)),
				"Child control is aligned to the top of its would-be area with SIZE_FILL in SIZE_SHRINK_BEGIN vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 100)),
				"Child control is centered in its would-be area with SIZE_FILL in SIZE_SHRINK_CENTER vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 150)),
				"Child control is aligned to the bottom of its would-be area with SIZE_FILL in SIZE_SHRINK_END vertical size flag mode.");
	}

	memdelete(child_control);
	memdelete(aspect_ratio_container);
}

} // namespace TestAspectRatioContainer

// === Merged from tests/scene/test_box_container.cpp ===
/*  test_box_container.cpp                                                */
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



#include "scene/gui/box_container.h"
#include "scene/main/scene_tree.h"

namespace TestBoxContainer {

TEST_CASE("[SceneTree][BoxContainer] HBoxContainer") {
	HBoxContainer *hbox_container = memnew(HBoxContainer);
	Window *root = SceneTree::get_root();
	root->add_child(hbox_container);

	hbox_container->set_size(Size2(100, 100));
	SceneTree::process(0);

	SUBCASE("Default behavior") {
		Control *child_control = memnew(Control);
		hbox_container->add_child(child_control);
		SceneTree::process(0);

		CHECK_MESSAGE(
				hbox_container->get_alignment() == BoxContainer::ALIGNMENT_BEGIN,
				"HBoxContainer alignment is set to ALIGNMENT_BEGIN by default.");

		CHECK_MESSAGE(
				!hbox_container->is_vertical(),
				"HBoxContainer orientation is horizontal.");

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left in ALIGNMENT_BEGIN mode.");

		CHECK_MESSAGE(
				child_control->get_h_size_flags() == Control::SIZE_FILL,
				"Child control horizontal size flags are set to SIZE_FILL by default.");

		CHECK_MESSAGE(
				child_control->get_v_size_flags() == Control::SIZE_FILL,
				"Child control vertical size flags are set to SIZE_FILL by default.");

		CHECK_MESSAGE(
				child_control->get_size().is_equal_approx(Size2(0, 100)),
				"Child control takes up minimum horizontal space and matches container height by default.");

		memdelete(child_control);
	}

	SUBCASE("Child Horizontal Size Flags") {
		Control *child_control = memnew(Control);
		hbox_container->add_child(child_control);
		child_control->set_custom_minimum_size(Size2(20, 20));
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left by default with SIZE_FILL.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left of in SIZE_SHRINK_BEGIN horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left of in SIZE_SHRINK_CENTER horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left of in SIZE_SHRINK_END horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_EXPAND | Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left in SIZE_EXPAND | SIZE_SHRINK_BEGIN horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_EXPAND | Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(40, 0)),
				"Child control is aligned to the center in SIZE_EXPAND | SIZE_SHRINK_CENTER horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_EXPAND | Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(80, 0)),
				"Child control is aligned to the right in SIZE_EXPAND | SIZE_SHRINK_END horizontal size flag mode.");

		memdelete(child_control);
	}

	SUBCASE("Child Vertical Size Flags") {
		Control *child_control = memnew(Control);
		hbox_container->add_child(child_control);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top by default with SIZE_FILL.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top of in SIZE_SHRINK_BEGIN vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 50)),
				"Child control is aligned to the center of in SIZE_SHRINK_CENTER vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 100)),
				"Child control is aligned to the bottom of in SIZE_SHRINK_END vertical size flag mode.");

		memdelete(child_control);
	}

	SUBCASE("Child Alignment") {
		Control *child_control = memnew(Control);
		hbox_container->add_child(child_control);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left in ALIGNMENT_BEGIN mode by default.");

		hbox_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is aligned to the center in ALIGNMENT_CENTER mode.");

		hbox_container->set_alignment(BoxContainer::ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(100, 0)),
				"Child control is aligned to the right in ALIGNMENT_END mode.");

		memdelete(child_control);
	}

	SUBCASE("Child Alignment RTL") {
		Control *child_control = memnew(Control);
		hbox_container->add_child(child_control);
		hbox_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(100, 0)),
				"Child control is aligned to the right in RTL ALIGNMENT_BEGIN mode by default.");

		hbox_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is aligned to the center in RTL ALIGNMENT_CENTER mode.");

		hbox_container->set_alignment(BoxContainer::ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the right in RTL ALIGNMENT_END mode.");

		memdelete(child_control);
	}

	SUBCASE("Child Separation") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		hbox_container->add_child(child_control_1);
		hbox_container->add_child(child_control_2);

		hbox_container->add_theme_constant_override("separation", 10);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(10, 0)),
				"Child controls are separated by the specified separation value.");

		memdelete(child_control_1);
		memdelete(child_control_2);
	}

	SUBCASE("Spacers") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		hbox_container->add_child(child_control_1);
		hbox_container->add_spacer();
		hbox_container->add_child(child_control_2);
		hbox_container->add_theme_constant_override("separation", 0);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(100, 0)),
				"Spacer pushes child controls apart.");

		memdelete(child_control_1);
		memdelete(child_control_2);
	}

	SUBCASE("Expanding children") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		hbox_container->add_child(child_control_1);
		hbox_container->add_child(child_control_2);
		hbox_container->add_theme_constant_override("separation", 0);

		child_control_1->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(100, 100)),
				"One child control expands to fill available space.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(0, 100)),
				"The non-expanding child control remains at minimum size.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"Expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(100, 0)),
				"Non-expanding child control is positioned after the expanding child control.");

		child_control_2->set_custom_minimum_size(Size2(20, 0));
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(80, 100)),
				"Expanding child control shrinks to accommodate the custom minimum size of the other child.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(20, 100)),
				"The non-expanding child control respects its custom minimum size.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"Expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(80, 0)),
				"Non-expanding child control is positioned after the expanding child control.");

		child_control_2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(50, 100)),
				"Both child controls expand equally to fill available space.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(50, 100)),
				"Both child controls expand equally to fill available space.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(50, 0)),
				"Second expanding child control is positioned after the first expanding child control.");

		child_control_2->set_stretch_ratio(3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(25, 100)),
				"Child control with lower stretch ratio takes less space.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(75, 100)),
				"Child control with higher stretch ratio takes more space.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(25, 0)),
				"Second expanding child control is positioned after the first expanding child control.");

		memdelete(child_control_1);
		memdelete(child_control_2);
	}

	memdelete(hbox_container);
}

TEST_CASE("[SceneTree][BoxContainer] VBoxContainer") {
	VBoxContainer *vbox_container = memnew(VBoxContainer);
	Window *root = SceneTree::get_root();
	root->add_child(vbox_container);

	vbox_container->set_size(Size2(100, 100));
	SceneTree::process(0);

	SUBCASE("Default behavior") {
		Control *child_control = memnew(Control);
		vbox_container->add_child(child_control);
		SceneTree::process(0);

		CHECK_MESSAGE(
				vbox_container->get_alignment() == BoxContainer::ALIGNMENT_BEGIN,
				"HBoxContainer alignment is set to ALIGNMENT_BEGIN by default.");

		CHECK_MESSAGE(
				vbox_container->is_vertical(),
				"VBoxContainer orientation is vertical.");

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left in ALIGNMENT_BEGIN mode.");

		CHECK_MESSAGE(
				child_control->get_h_size_flags() == Control::SIZE_FILL,
				"Child control horizontal size flags are set to SIZE_FILL by default.");

		CHECK_MESSAGE(
				child_control->get_v_size_flags() == Control::SIZE_FILL,
				"Child control vertical size flags are set to SIZE_FILL by default.");

		CHECK_MESSAGE(
				child_control->get_size().is_equal_approx(Size2(100, 0)),
				"Child control takes up minimum vertical space and matches container width by default.");

		memdelete(child_control);
	}

	SUBCASE("Child Size Flags") {
		Control *child_control = memnew(Control);
		vbox_container->add_child(child_control);
		child_control->set_custom_minimum_size(Size2(20, 20));
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top by default with SIZE_FILL.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top of in SIZE_SHRINK_BEGIN vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top of in SIZE_SHRINK_CENTER vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top of in SIZE_SHRINK_END vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_EXPAND | Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top in SIZE_EXPAND | SIZE_SHRINK_BEGIN vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_EXPAND | Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 40)),
				"Child control is aligned to the center in SIZE_EXPAND | SIZE_SHRINK_CENTER vertical size flag mode.");

		child_control->set_v_size_flags(Control::SIZE_EXPAND | Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 80)),
				"Child control is aligned to the bottom in SIZE_EXPAND | SIZE_SHRINK_END vertical size flag mode.");

		memdelete(child_control);
	}

	SUBCASE("Child Horizontal Size Flags") {
		Control *child_control = memnew(Control);
		vbox_container->add_child(child_control);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left by default with SIZE_FILL.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_BEGIN);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the left of in SIZE_SHRINK_BEGIN horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(50, 0)),
				"Child control is aligned to the center of in SIZE_SHRINK_CENTER horizontal size flag mode.");

		child_control->set_h_size_flags(Control::SIZE_SHRINK_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(100, 0)),
				"Child control is aligned to the right of in SIZE_SHRINK_END horizontal size flag mode.");

		memdelete(child_control);
	}

	SUBCASE("Child Alignment") {
		Control *child_control = memnew(Control);
		vbox_container->add_child(child_control);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 0)),
				"Child control is aligned to the top in ALIGNMENT_BEGIN mode by default.");

		vbox_container->set_alignment(BoxContainer::ALIGNMENT_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 50)),
				"Child control is aligned to the center in ALIGNMENT_CENTER mode.");

		vbox_container->set_alignment(BoxContainer::ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control->get_position().is_equal_approx(Point2(0, 100)),
				"Child control is aligned to the bottom in ALIGNMENT_END mode.");

		memdelete(child_control);
	}

	SUBCASE("Child Separation") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		vbox_container->add_child(child_control_1);
		vbox_container->add_child(child_control_2);

		vbox_container->add_theme_constant_override("separation", 10);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 10)),
				"Child controls are separated by the specified separation value.");

		memdelete(child_control_1);
		memdelete(child_control_2);
	}

	SUBCASE("Spacers") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		vbox_container->add_child(child_control_1);
		vbox_container->add_spacer();
		vbox_container->add_child(child_control_2);
		vbox_container->add_theme_constant_override("separation", 0);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 100)),
				"Spacer pushes child controls apart.");

		memdelete(child_control_1);
		memdelete(child_control_2);
	}

	SUBCASE("Expanding children") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		vbox_container->add_child(child_control_1);
		vbox_container->add_child(child_control_2);
		vbox_container->add_theme_constant_override("separation", 0);

		child_control_1->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(100, 100)),
				"One child control expands to fill available space.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(100, 0)),
				"The non-expanding child control remains at minimum size.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"Expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 100)),
				"Non-expanding child control is positioned after the expanding child control.");

		child_control_2->set_custom_minimum_size(Size2(0, 20));
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(100, 80)),
				"Expanding child control shrinks to accommodate the custom minimum size of the other child.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(100, 20)),
				"The non-expanding child control respects its custom minimum size.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"Expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 80)),
				"Non-expanding child control is positioned after the expanding child control.");

		child_control_2->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(100, 50)),
				"Both child controls expand equally to fill available space.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(100, 50)),
				"Both child controls expand equally to fill available space.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 50)),
				"Second expanding child control is positioned after the first expanding child control.");

		child_control_2->set_stretch_ratio(3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(100, 25)),
				"Child control with lower stretch ratio takes less space.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(100, 75)),
				"Child control with higher stretch ratio takes more space.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First expanding child control is aligned to the left.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 25)),
				"Second expanding child control is positioned after the first expanding child control.");

		memdelete(child_control_1);
		memdelete(child_control_2);
	}

	memdelete(vbox_container);
}

} // namespace TestBoxContainer

// === Merged from tests/scene/test_button.cpp ===
/*  test_button.cpp                                                       */
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
#include "tests/test_macros.h"

namespace TestButton {

TEST_CASE("[SceneTree][Button] is_hovered()") {
	// Create new button instance.
	Button *button = memnew(Button);
	CHECK(button != nullptr);
	Window *root = SceneTree::get_root();
	root->add_child(button);

	// Set up button's size and position.
	button->set_size(Size2i(50, 50));
	button->set_position(Size2i(10, 10));

	// Button should initially be not hovered.
	CHECK(button->is_hovered() == false);

	// Simulate mouse entering the button.
	SEND_GUI_MOUSE_MOTION_EVENT(Point2i(25, 25), MouseButtonMask::NONE, Key::NONE);
	CHECK(button->is_hovered() == true);

	// Simulate mouse exiting the button.
	SEND_GUI_MOUSE_MOTION_EVENT(Point2i(150, 150), MouseButtonMask::NONE, Key::NONE);
	CHECK(button->is_hovered() == false);

	memdelete(button);
}

} // namespace TestButton

// === Merged from tests/scene/test_center_container.cpp ===
/*  test_center_container.cpp                                             */
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



#include "scene/gui/center_container.h"
#include "scene/main/scene_tree.h"

namespace TestCenterContainer {

TEST_CASE("[SceneTree][CenterContainer] Standard behavior") {
	CenterContainer *center_container = memnew(CenterContainer);
	Window *root = SceneTree::get_root();
	root->add_child(center_container);
	Control *child_control = memnew(Control);
	center_container->add_child(child_control);

	center_container->set_size(Size2(100, 100));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(50, 50)),
			"Child control is centered within the CenterContainer by default.");

	child_control->set_custom_minimum_size(Size2(20, 20));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(40, 40)),
			"Child control remains centered when custom minimum size is set.");

	child_control->set_custom_minimum_size(Size2(200, 200));
	SceneTree::process(0);

	CHECK_MESSAGE(
			center_container->get_size().is_equal_approx(Size2(200, 200)),
			"CenterContainer expands to accommodate the minimum size of its child control.");

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(0, 0)),
			"Child control is positioned at the top-left corner when its minimum size equals the container size.");

	child_control->set_custom_minimum_size(Size2());
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(50, 50)),
			"Child control re-centers when custom minimum size is reset.");

	CHECK_MESSAGE(
			center_container->get_size().is_equal_approx(Size2(100, 100)),
			"CenterContainer returns to original size when child control's custom minimum size is reset.");

	center_container->set_use_top_left(true);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(0, 0)),
			"Child control is aligned to the top-left corner when use_top_left is enabled.");

	center_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(100, 0)),
			"Child control is aligned to the top-right corner in RTL layout direction when use_top_left is enabled.");

	memdelete(child_control);
	memdelete(center_container);
}

TEST_CASE("[SceneTree][CenterContainer] Multiple children") {
	CenterContainer *center_container = memnew(CenterContainer);
	Window *root = SceneTree::get_root();
	root->add_child(center_container);

	center_container->set_size(Size2(100, 100));

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	center_container->add_child(child_control_1);
	center_container->add_child(child_control_2);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control_1->get_position().is_equal_approx(Point2(50, 50)),
			"First child control is centered within the CenterContainer.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(50, 50)),
			"Second child control is also centered and overlaps the first child control by default.");

	child_control_1->set_custom_minimum_size(Size2(20, 20));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control_1->get_position().is_equal_approx(Point2(40, 40)),
			"First child control remains centered when custom minimum size is set.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(50, 50)),
			"Second child control remains centered and overlaps the first child control even when the first child has a custom minimum size.");

	child_control_1->set_custom_minimum_size(Size2(200, 0));
	SceneTree::process(0);

	CHECK_MESSAGE(
			center_container->get_size().is_equal_approx(Size2(200, 100)),
			"CenterContainer expands to accommodate the minimum size of the first child control.");

	CHECK_MESSAGE(
			child_control_1->get_position().is_equal_approx(Point2(0, 50)),
			"First child control is aligned to the left edge of the CenterContainer when its minimum width equals the container width.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(100, 50)),
			"Second child control is centered on the new container center.");

	child_control_2->set_custom_minimum_size(Size2(0, 200));
	SceneTree::process(0);

	CHECK_MESSAGE(
			center_container->get_size().is_equal_approx(Size2(200, 200)),
			"CenterContainer expands to accommodate the minimum size of the second child control.");

	CHECK_MESSAGE(
			child_control_1->get_position().is_equal_approx(Point2(0, 100)),
			"First child control is aligned to the left edge and centered vertically when its minimum width equals the container width but its minimum height is smaller than the container height.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(100, 0)),
			"Second child control is aligned to the top edge and centered horizontally when its minimum height equals the container height but its minimum width is smaller than the container width.");

	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(center_container);
}

} // namespace TestCenterContainer

// === Merged from tests/scene/test_code_edit.cpp ===
/*  test_code_edit.cpp                                                    */
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



#ifndef ADVANCED_GUI_DISABLED

#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestCodeEdit {

TEST_CASE("[SceneTree][CodeEdit] line gutters") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	SUBCASE("[CodeEdit] breakpoints") {
		SIGNAL_WATCH(code_edit, "breakpoint_toggled");

		SUBCASE("[CodeEdit] draw breakpoints gutter") {
			code_edit->set_draw_breakpoints_gutter(false);
			CHECK_FALSE(code_edit->is_drawing_breakpoints_gutter());

			code_edit->set_draw_breakpoints_gutter(true);
			CHECK(code_edit->is_drawing_breakpoints_gutter());
		}

		SUBCASE("[CodeEdit] set line as breakpoint") {
			/* Out of bounds. */
			ERR_PRINT_OFF;

			code_edit->set_line_as_breakpoint(-1, true);
			CHECK_FALSE(code_edit->is_line_breakpointed(-1));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			code_edit->set_line_as_breakpoint(1, true);
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			ERR_PRINT_ON;

			Array args = { { 0 } };

			code_edit->set_line_as_breakpoint(0, true);
			CHECK(code_edit->is_line_breakpointed(0));
			CHECK(code_edit->get_breakpointed_lines()[0] == 0);
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->set_line_as_breakpoint(0, false);
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] clear breakpointed lines") {
			code_edit->clear_breakpointed_lines();
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			Array args = { { 0 } };

			code_edit->set_line_as_breakpoint(0, true);
			CHECK(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->clear_breakpointed_lines();
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and set text") {
			Array args = { { 0 } };

			code_edit->set_text("test\nline");
			code_edit->set_line_as_breakpoint(0, true);
			CHECK(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			/* breakpoint on lines that still exist are kept. */
			code_edit->set_text("");
			CHECK(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			/* breakpoint on lines that are removed should also be removed. */
			code_edit->clear_breakpointed_lines();
			SIGNAL_DISCARD("breakpoint_toggled")

			args = { { 1 } };
			code_edit->set_text("test\nline");
			code_edit->set_line_as_breakpoint(1, true);
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->set_text("");
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			ERR_PRINT_ON;
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and clear") {
			Array args = { { 0 } };

			code_edit->set_text("test\nline");
			code_edit->set_line_as_breakpoint(0, true);
			CHECK(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			/* breakpoint on lines that still exist are removed. */
			code_edit->clear();
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			/* breakpoint on lines that are removed should also be removed. */
			code_edit->clear_breakpointed_lines();
			SIGNAL_DISCARD("breakpoint_toggled")

			args = { { 1 } };
			code_edit->set_text("test\nline");
			code_edit->set_line_as_breakpoint(1, true);
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->clear();
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			ERR_PRINT_ON;
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and new lines no text") {
			Array args = { { 0 } };

			/* No text moves breakpoint. */
			code_edit->set_line_as_breakpoint(0, true);
			CHECK(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			// Normal.
			args = { { 0 }, { 1 } };

			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line_count() == 2);
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);

			// Non-Breaking.
			args = { { 1 }, { 2 } };
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line_count() == 3);
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			CHECK(code_edit->is_line_breakpointed(2));
			SIGNAL_CHECK("breakpoint_toggled", args);

			// Above.
			args = { { 2 }, { 3 } };
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line_count() == 4);
			CHECK_FALSE(code_edit->is_line_breakpointed(2));
			CHECK(code_edit->is_line_breakpointed(3));
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and new lines with text") {
			Array args = { { 0 } };

			/* Having text does not move breakpoint. */
			code_edit->insert_text_at_caret("text");
			code_edit->set_line_as_breakpoint(0, true);
			CHECK(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			/* Normal. */
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line_count() == 2);
			CHECK(code_edit->is_line_breakpointed(0));
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			/* Non-Breaking. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line_count() == 3);
			CHECK(code_edit->is_line_breakpointed(0));
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			// Above does move.
			args = { { 0 }, { 1 } };

			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line_count() == 4);
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and backspace") {
			Array args = { { 1 } };

			code_edit->set_text("\n\n");
			code_edit->set_line_as_breakpoint(1, true);
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->set_caret_line(2);

			/* backspace onto line does not remove breakpoint */
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			/* backspace on breakpointed line removes it */
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			ERR_PRINT_ON;
			SIGNAL_CHECK("breakpoint_toggled", args);

			// Backspace above breakpointed line moves it.
			args = { { 2 } };

			code_edit->set_text("\n\n");
			code_edit->set_line_as_breakpoint(2, true);
			CHECK(code_edit->is_line_breakpointed(2));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->set_caret_line(1);

			args = { { 2 }, { 1 } };
			SEND_GUI_ACTION("ui_text_backspace");
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_breakpointed(2));
			ERR_PRINT_ON;
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and delete") {
			Array args = { { 1 } };

			code_edit->set_text("\n\n");
			code_edit->set_line_as_breakpoint(1, true);
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);
			code_edit->set_caret_line(1);

			/* Delete onto breakpointed lines does not remove it. */
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(code_edit->get_line_count() == 2);
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");

			/* Delete moving breakpointed line up removes it. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(code_edit->get_line_count() == 1);
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			ERR_PRINT_ON;
			SIGNAL_CHECK("breakpoint_toggled", args);

			// Delete above breakpointed line moves it.
			args = { { 2 } };

			code_edit->set_text("\n\n");
			code_edit->set_line_as_breakpoint(2, true);
			CHECK(code_edit->is_line_breakpointed(2));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->set_caret_line(0);

			args = { { 2 }, { 1 } };
			SEND_GUI_ACTION("ui_text_delete");
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_breakpointed(2));
			ERR_PRINT_ON;
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and delete selection") {
			Array args = { { 1 } };

			code_edit->set_text("\n\n");
			code_edit->set_line_as_breakpoint(1, true);
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->select(0, 0, 2, 0);
			code_edit->delete_selection();
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			// Should handle breakpoint move when deleting selection by adding less text then removed.
			args = { { 9 } };

			code_edit->set_text("\n\n\n\n\n\n\n\n\n");
			code_edit->set_line_as_breakpoint(9, true);
			CHECK(code_edit->is_line_breakpointed(9));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->select(0, 0, 6, 0);

			args = { { 9 }, { 4 } };
			SEND_GUI_ACTION("ui_text_newline");
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_breakpointed(9));
			ERR_PRINT_ON;
			CHECK(code_edit->is_line_breakpointed(4));
			SIGNAL_CHECK("breakpoint_toggled", args);

			// Should handle breakpoint move when deleting selection by adding more text then removed.
			args = { { 9 }, { 14 } };

			code_edit->insert_text_at_caret("\n\n\n\n\n");
			SIGNAL_DISCARD("breakpoint_toggled")
			CHECK(code_edit->is_line_breakpointed(9));

			code_edit->select(0, 0, 6, 0);
			code_edit->insert_text_at_caret("\n\n\n\n\n\n\n\n\n\n\n");
			CHECK(code_edit->is_line_breakpointed(14));
			SIGNAL_CHECK("breakpoint_toggled", args);
		}

		SUBCASE("[CodeEdit] breakpoints and undo") {
			Array args = { { 1 } };

			code_edit->set_text("\n\n");
			code_edit->set_line_as_breakpoint(1, true);
			CHECK(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK("breakpoint_toggled", args);

			code_edit->select(0, 0, 2, 0);
			code_edit->delete_selection();
			CHECK_FALSE(code_edit->is_line_breakpointed(0));
			SIGNAL_CHECK("breakpoint_toggled", args);

			/* Undo does not restore breakpoint. */
			code_edit->undo();
			CHECK_FALSE(code_edit->is_line_breakpointed(1));
			SIGNAL_CHECK_FALSE("breakpoint_toggled");
		}

		SIGNAL_UNWATCH(code_edit, "breakpoint_toggled");
	}

	SUBCASE("[CodeEdit] bookmarks") {
		SUBCASE("[CodeEdit] draw bookmarks gutter") {
			code_edit->set_draw_bookmarks_gutter(false);
			CHECK_FALSE(code_edit->is_drawing_bookmarks_gutter());

			code_edit->set_draw_bookmarks_gutter(true);
			CHECK(code_edit->is_drawing_bookmarks_gutter());
		}

		SUBCASE("[CodeEdit] set line as bookmarks") {
			/* Out of bounds. */
			ERR_PRINT_OFF;

			code_edit->set_line_as_bookmarked(-1, true);
			CHECK_FALSE(code_edit->is_line_bookmarked(-1));

			code_edit->set_line_as_bookmarked(1, true);
			CHECK_FALSE(code_edit->is_line_bookmarked(1));

			ERR_PRINT_ON;

			code_edit->set_line_as_bookmarked(0, true);
			CHECK(code_edit->get_bookmarked_lines()[0] == 0);
			CHECK(code_edit->is_line_bookmarked(0));

			code_edit->set_line_as_bookmarked(0, false);
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
		}

		SUBCASE("[CodeEdit] clear bookmarked lines") {
			code_edit->clear_bookmarked_lines();

			code_edit->set_line_as_bookmarked(0, true);
			CHECK(code_edit->is_line_bookmarked(0));

			code_edit->clear_bookmarked_lines();
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
		}

		SUBCASE("[CodeEdit] bookmarks and set text") {
			code_edit->set_text("test\nline");
			code_edit->set_line_as_bookmarked(0, true);
			CHECK(code_edit->is_line_bookmarked(0));

			/* bookmarks on lines that still exist are kept. */
			code_edit->set_text("");
			CHECK(code_edit->is_line_bookmarked(0));

			/* bookmarks on lines that are removed should also be removed. */
			code_edit->clear_bookmarked_lines();

			code_edit->set_text("test\nline");
			code_edit->set_line_as_bookmarked(1, true);
			CHECK(code_edit->is_line_bookmarked(1));

			code_edit->set_text("");
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_bookmarked(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] bookmarks and clear") {
			code_edit->set_text("test\nline");
			code_edit->set_line_as_bookmarked(0, true);
			CHECK(code_edit->is_line_bookmarked(0));

			/* bookmarks on lines that still exist are removed. */
			code_edit->clear();
			CHECK_FALSE(code_edit->is_line_bookmarked(0));

			/* bookmarks on lines that are removed should also be removed. */
			code_edit->clear_bookmarked_lines();

			code_edit->set_text("test\nline");
			code_edit->set_line_as_bookmarked(1, true);
			CHECK(code_edit->is_line_bookmarked(1));

			code_edit->clear();
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_bookmarked(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] bookmarks and new lines no text") {
			/* No text moves bookmarks. */
			code_edit->set_line_as_bookmarked(0, true);
			CHECK(code_edit->is_line_bookmarked(0));

			/* Normal. */
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line_count() == 2);
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
			CHECK(code_edit->is_line_bookmarked(1));

			/* Non-Breaking. */
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line_count() == 3);
			CHECK_FALSE(code_edit->is_line_bookmarked(1));
			CHECK(code_edit->is_line_bookmarked(2));

			/* Above. */
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line_count() == 4);
			CHECK_FALSE(code_edit->is_line_bookmarked(2));
			CHECK(code_edit->is_line_bookmarked(3));
		}

		SUBCASE("[CodeEdit] bookmarks and new lines with text") {
			/* Having text does not move bookmark. */
			code_edit->insert_text_at_caret("text");
			code_edit->set_line_as_bookmarked(0, true);
			CHECK(code_edit->is_line_bookmarked(0));

			/* Normal. */
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line_count() == 2);
			CHECK(code_edit->is_line_bookmarked(0));
			CHECK_FALSE(code_edit->is_line_bookmarked(1));

			/* Non-Breaking. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line_count() == 3);
			CHECK(code_edit->is_line_bookmarked(0));
			CHECK_FALSE(code_edit->is_line_bookmarked(1));

			/* Above does move. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line_count() == 4);
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
			CHECK(code_edit->is_line_bookmarked(1));
		}

		SUBCASE("[CodeEdit] bookmarks and backspace") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_bookmarked(1, true);
			CHECK(code_edit->is_line_bookmarked(1));

			code_edit->set_caret_line(2);

			/* backspace onto line does not remove bookmark */
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(code_edit->is_line_bookmarked(1));

			/* backspace on bookmarked line removes it */
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_bookmarked(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] bookmarks and delete") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_bookmarked(1, true);
			CHECK(code_edit->is_line_bookmarked(1));
			code_edit->set_caret_line(1);

			/* Delete onto bookmarked lines does not remove it. */
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(code_edit->get_line_count() == 2);
			CHECK(code_edit->is_line_bookmarked(1));

			/* Delete moving bookmarked line up removes it. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(code_edit->get_line_count() == 1);
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_bookmarked(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] bookmarks and delete selection") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_bookmarked(1, true);
			CHECK(code_edit->is_line_bookmarked(1));

			code_edit->select(0, 0, 2, 0);
			code_edit->delete_selection();
			CHECK_FALSE(code_edit->is_line_bookmarked(0));
		}

		SUBCASE("[CodeEdit] bookmarks and undo") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_bookmarked(1, true);
			CHECK(code_edit->is_line_bookmarked(1));

			code_edit->select(0, 0, 2, 0);
			code_edit->delete_selection();
			CHECK_FALSE(code_edit->is_line_bookmarked(0));

			/* Undo does not restore bookmark. */
			code_edit->undo();
			CHECK_FALSE(code_edit->is_line_bookmarked(1));
		}
	}

	SUBCASE("[CodeEdit] executing lines") {
		SUBCASE("[CodeEdit] draw executing lines gutter") {
			code_edit->set_draw_executing_lines_gutter(false);
			CHECK_FALSE(code_edit->is_drawing_executing_lines_gutter());

			code_edit->set_draw_executing_lines_gutter(true);
			CHECK(code_edit->is_drawing_executing_lines_gutter());
		}

		SUBCASE("[CodeEdit] set line as executing lines") {
			/* Out of bounds. */
			ERR_PRINT_OFF;

			code_edit->set_line_as_executing(-1, true);
			CHECK_FALSE(code_edit->is_line_executing(-1));

			code_edit->set_line_as_executing(1, true);
			CHECK_FALSE(code_edit->is_line_executing(1));

			ERR_PRINT_ON;

			code_edit->set_line_as_executing(0, true);
			CHECK(code_edit->get_executing_lines()[0] == 0);
			CHECK(code_edit->is_line_executing(0));

			code_edit->set_line_as_executing(0, false);
			CHECK_FALSE(code_edit->is_line_executing(0));
		}

		SUBCASE("[CodeEdit] clear executing lines lines") {
			code_edit->clear_executing_lines();

			code_edit->set_line_as_executing(0, true);
			CHECK(code_edit->is_line_executing(0));

			code_edit->clear_executing_lines();
			CHECK_FALSE(code_edit->is_line_executing(0));
		}

		SUBCASE("[CodeEdit] executing lines and set text") {
			code_edit->set_text("test\nline");
			code_edit->set_line_as_executing(0, true);
			CHECK(code_edit->is_line_executing(0));

			/* executing on lines that still exist are kept. */
			code_edit->set_text("");
			CHECK(code_edit->is_line_executing(0));

			/* executing on lines that are removed should also be removed. */
			code_edit->clear_executing_lines();

			code_edit->set_text("test\nline");
			code_edit->set_line_as_executing(1, true);
			CHECK(code_edit->is_line_executing(1));

			code_edit->set_text("");
			CHECK_FALSE(code_edit->is_line_executing(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_executing(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] executing lines and clear") {
			code_edit->set_text("test\nline");
			code_edit->set_line_as_executing(0, true);
			CHECK(code_edit->is_line_executing(0));

			/* executing on lines that still exist are removed. */
			code_edit->clear();
			CHECK_FALSE(code_edit->is_line_executing(0));

			/* executing on lines that are removed should also be removed. */
			code_edit->clear_executing_lines();

			code_edit->set_text("test\nline");
			code_edit->set_line_as_executing(1, true);
			CHECK(code_edit->is_line_executing(1));

			code_edit->clear();
			CHECK_FALSE(code_edit->is_line_executing(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_executing(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] executing lines and new lines no text") {
			/* No text moves executing lines. */
			code_edit->set_line_as_executing(0, true);
			CHECK(code_edit->is_line_executing(0));

			/* Normal. */
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line_count() == 2);
			CHECK_FALSE(code_edit->is_line_executing(0));
			CHECK(code_edit->is_line_executing(1));

			/* Non-Breaking. */
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line_count() == 3);
			CHECK_FALSE(code_edit->is_line_executing(1));
			CHECK(code_edit->is_line_executing(2));

			/* Above. */
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line_count() == 4);
			CHECK_FALSE(code_edit->is_line_executing(2));
			CHECK(code_edit->is_line_executing(3));
		}

		SUBCASE("[CodeEdit] executing lines and new lines with text") {
			/* Having text does not move executing lines. */
			code_edit->insert_text_at_caret("text");
			code_edit->set_line_as_executing(0, true);
			CHECK(code_edit->is_line_executing(0));

			/* Normal. */
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line_count() == 2);
			CHECK(code_edit->is_line_executing(0));
			CHECK_FALSE(code_edit->is_line_executing(1));

			/* Non-Breaking. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line_count() == 3);
			CHECK(code_edit->is_line_executing(0));
			CHECK_FALSE(code_edit->is_line_executing(1));

			/* Above does move. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line_count() == 4);
			CHECK_FALSE(code_edit->is_line_executing(0));
			CHECK(code_edit->is_line_executing(1));
		}

		SUBCASE("[CodeEdit] executing lines and backspace") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_executing(1, true);
			CHECK(code_edit->is_line_executing(1));

			code_edit->set_caret_line(2);

			/* backspace onto line does not remove executing lines. */
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(code_edit->is_line_executing(1));

			/* backspace on executing line removes it */
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK_FALSE(code_edit->is_line_executing(0));
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_executing(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] executing lines and delete") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_executing(1, true);
			CHECK(code_edit->is_line_executing(1));
			code_edit->set_caret_line(1);

			/* Delete onto executing lines does not remove it. */
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(code_edit->get_line_count() == 2);
			CHECK(code_edit->is_line_executing(1));

			/* Delete moving executing line up removes it. */
			code_edit->set_caret_line(0);
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(code_edit->get_line_count() == 1);
			ERR_PRINT_OFF;
			CHECK_FALSE(code_edit->is_line_executing(1));
			ERR_PRINT_ON;
		}

		SUBCASE("[CodeEdit] executing lines and delete selection") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_executing(1, true);
			CHECK(code_edit->is_line_executing(1));

			code_edit->select(0, 0, 2, 0);
			code_edit->delete_selection();
			CHECK_FALSE(code_edit->is_line_executing(0));
		}

		SUBCASE("[CodeEdit] executing lines and undo") {
			code_edit->set_text("\n\n");
			code_edit->set_line_as_executing(1, true);
			CHECK(code_edit->is_line_executing(1));

			code_edit->select(0, 0, 2, 0);
			code_edit->delete_selection();
			CHECK_FALSE(code_edit->is_line_executing(0));

			/* Undo does not restore executing lines. */
			code_edit->undo();
			CHECK_FALSE(code_edit->is_line_executing(1));
		}
	}

	SUBCASE("[CodeEdit] line numbers") {
		SUBCASE("[CodeEdit] draw line numbers gutter and padding") {
			code_edit->set_draw_line_numbers(false);
			CHECK_FALSE(code_edit->is_draw_line_numbers_enabled());

			code_edit->set_draw_line_numbers(true);
			CHECK(code_edit->is_draw_line_numbers_enabled());

			code_edit->set_line_numbers_zero_padded(false);
			CHECK_FALSE(code_edit->is_line_numbers_zero_padded());

			code_edit->set_line_numbers_zero_padded(true);
			CHECK(code_edit->is_line_numbers_zero_padded());

			code_edit->set_line_numbers_zero_padded(false);
			CHECK_FALSE(code_edit->is_line_numbers_zero_padded());

			code_edit->set_draw_line_numbers(false);
			CHECK_FALSE(code_edit->is_draw_line_numbers_enabled());

			code_edit->set_line_numbers_zero_padded(true);
			CHECK(code_edit->is_line_numbers_zero_padded());
		}
	}

	SUBCASE("[CodeEdit] line folding") {
		SUBCASE("[CodeEdit] draw line folding gutter") {
			code_edit->set_draw_fold_gutter(false);
			CHECK_FALSE(code_edit->is_drawing_fold_gutter());

			code_edit->set_draw_fold_gutter(true);
			CHECK(code_edit->is_drawing_fold_gutter());
		}
	}

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] delimiters") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	const Point2 OUTSIDE_DELIMITER = Point2(-1, -1);

	code_edit->clear_string_delimiters();
	code_edit->clear_comment_delimiters();

	SUBCASE("[CodeEdit] add and remove delimiters") {
		SUBCASE("[CodeEdit] add and remove string delimiters") {
			/* Add a delimiter.*/
			code_edit->add_string_delimiter("\"", "\"", false);
			CHECK(code_edit->has_string_delimiter("\""));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			ERR_PRINT_OFF;

			/* Adding a duplicate start key is not allowed. */
			code_edit->add_string_delimiter("\"", "\'", false);
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* Adding a duplicate end key is allowed. */
			code_edit->add_string_delimiter("'", "\"", false);
			CHECK(code_edit->has_string_delimiter("'"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			/* Both start and end keys have to be symbols. */
			code_edit->add_string_delimiter("f", "\"", false);
			CHECK_FALSE(code_edit->has_string_delimiter("f"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			code_edit->add_string_delimiter("f", "\"", false);
			CHECK_FALSE(code_edit->has_string_delimiter("f"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			code_edit->add_string_delimiter("@", "f", false);
			CHECK_FALSE(code_edit->has_string_delimiter("@"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			code_edit->add_string_delimiter("f", "f", false);
			CHECK_FALSE(code_edit->has_string_delimiter("f"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			/* Blank start keys are not allowed */
			code_edit->add_string_delimiter("", "#", false);
			CHECK_FALSE(code_edit->has_string_delimiter("#"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			ERR_PRINT_ON;

			/* Blank end keys are allowed. */
			code_edit->add_string_delimiter("#", "", false);
			CHECK(code_edit->has_string_delimiter("#"));
			CHECK(code_edit->get_string_delimiters().size() == 3);

			/* Remove a delimiter. */
			code_edit->remove_string_delimiter("#");
			CHECK_FALSE(code_edit->has_string_delimiter("#"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			/* Set should override existing, and test multiline */
			Vector<String> delimiters = { "^^ ^^" };

			code_edit->set_string_delimiters(delimiters);
			CHECK_FALSE(code_edit->has_string_delimiter("\""));
			CHECK(code_edit->has_string_delimiter("^^"));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* clear should remove all. */
			code_edit->clear_string_delimiters();
			CHECK_FALSE(code_edit->has_string_delimiter("^^"));
			CHECK(code_edit->get_string_delimiters().size() == 0);
		}

		SUBCASE("[CodeEdit] add and remove comment delimiters") {
			/* Add a delimiter.*/
			code_edit->add_comment_delimiter("\"", "\"", false);
			CHECK(code_edit->has_comment_delimiter("\""));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			ERR_PRINT_OFF;

			/* Adding a duplicate start key is not allowed. */
			code_edit->add_comment_delimiter("\"", "\'", false);
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* Adding a duplicate end key is allowed. */
			code_edit->add_comment_delimiter("'", "\"", false);
			CHECK(code_edit->has_comment_delimiter("'"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			/* Both start and end keys have to be symbols. */
			code_edit->add_comment_delimiter("f", "\"", false);
			CHECK_FALSE(code_edit->has_comment_delimiter("f"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			code_edit->add_comment_delimiter("f", "\"", false);
			CHECK_FALSE(code_edit->has_comment_delimiter("f"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			code_edit->add_comment_delimiter("@", "f", false);
			CHECK_FALSE(code_edit->has_comment_delimiter("@"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			code_edit->add_comment_delimiter("f", "f", false);
			CHECK_FALSE(code_edit->has_comment_delimiter("f"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			/* Blank start keys are not allowed. */
			code_edit->add_comment_delimiter("", "#", false);
			CHECK_FALSE(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			ERR_PRINT_ON;

			/* Blank end keys are allowed. */
			code_edit->add_comment_delimiter("#", "", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 3);

			/* Remove a delimiter. */
			code_edit->remove_comment_delimiter("#");
			CHECK_FALSE(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			/* Set should override existing, and test multiline. */
			Vector<String> delimiters = { "^^ ^^" };

			code_edit->set_comment_delimiters(delimiters);
			CHECK_FALSE(code_edit->has_comment_delimiter("\""));
			CHECK(code_edit->has_comment_delimiter("^^"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* clear should remove all. */
			code_edit->clear_comment_delimiters();
			CHECK_FALSE(code_edit->has_comment_delimiter("^^"));
			CHECK(code_edit->get_comment_delimiters().size() == 0);
		}

		SUBCASE("[CodeEdit] add and remove mixed delimiters") {
			code_edit->add_comment_delimiter("#", "", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			ERR_PRINT_OFF;

			/* Disallow adding a string with the same start key as comment. */
			code_edit->add_string_delimiter("#", "", false);
			CHECK_FALSE(code_edit->has_string_delimiter("#"));
			CHECK(code_edit->get_string_delimiters().size() == 0);

			code_edit->add_string_delimiter("\"", "\"", false);
			CHECK(code_edit->has_string_delimiter("\""));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* Disallow adding a comment with the same start key as string. */
			code_edit->add_comment_delimiter("\"", "", false);
			CHECK_FALSE(code_edit->has_comment_delimiter("\""));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			ERR_PRINT_ON;

			/* Cannot remove string with remove comment. */
			code_edit->remove_comment_delimiter("\"");
			CHECK(code_edit->has_string_delimiter("\""));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* Cannot remove comment with remove string. */
			code_edit->remove_string_delimiter("#");
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* Clear comments leave strings. */
			code_edit->clear_comment_delimiters();
			CHECK(code_edit->has_string_delimiter("\""));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* Clear string leave comments. */
			code_edit->add_comment_delimiter("#", "", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			code_edit->clear_string_delimiters();
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);
		}
	}

	SUBCASE("[CodeEdit] single line delimiters") {
		SUBCASE("[CodeEdit] single line string delimiters") {
			/* Blank end key should set lineonly to true. */
			code_edit->add_string_delimiter("#", "", false);
			CHECK(code_edit->has_string_delimiter("#"));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* Insert line above, line with string then line below. */
			code_edit->insert_text_at_caret(" \n#\n ");

			/* Check line above is not in string. */
			CHECK(code_edit->is_in_string(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in string. */
			CHECK(code_edit->is_in_string(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column after start key is in string and start / end positions are correct. */
			CHECK(code_edit->is_in_string(1, 1) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 1) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 1) == Point2(2, 1));

			/* Check line after is not in string. */
			CHECK(code_edit->is_in_string(2, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(2, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(2, 1) == OUTSIDE_DELIMITER);

			/* Check region metadata. */
			int idx = code_edit->is_in_string(1, 1);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "");

			/* Check nested strings are handled correctly. */
			code_edit->set_text(" \n#  # \n ");

			/* Check line above is not in string. */
			CHECK(code_edit->is_in_string(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before first start key is not in string. */
			CHECK(code_edit->is_in_string(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column after the first start key is in string and start / end positions are correct. */
			CHECK(code_edit->is_in_string(1, 1) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 1) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 1) == Point2(6, 1));

			/* Check column after the second start key returns data for the first. */
			CHECK(code_edit->is_in_string(1, 5) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 5) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 5) == Point2(6, 1));

			/* Check line after is not in string. */
			CHECK(code_edit->is_in_string(2, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(2, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(2, 1) == OUTSIDE_DELIMITER);

			/* Check is in string with no column returns true if entire line is comment excluding whitespace. */
			code_edit->set_text(" \n  #  # \n ");
			CHECK(code_edit->is_in_string(1) != -1);

			code_edit->set_text(" \n  text #  # \n ");
			CHECK(code_edit->is_in_string(1) == -1);

			/* Removing delimiter should update. */
			code_edit->set_text(" \n  #  # \n ");

			code_edit->remove_string_delimiter("#");
			CHECK_FALSE(code_edit->has_string_delimiter("$"));
			CHECK(code_edit->get_string_delimiters().size() == 0);

			CHECK(code_edit->is_in_string(1) == -1);

			/* Adding and clear should update. */
			code_edit->add_string_delimiter("#", "", false);
			CHECK(code_edit->has_string_delimiter("#"));
			CHECK(code_edit->get_string_delimiters().size() == 1);
			CHECK(code_edit->is_in_string(1) != -1);

			code_edit->clear_string_delimiters();
			CHECK_FALSE(code_edit->has_string_delimiter("$"));
			CHECK(code_edit->get_string_delimiters().size() == 0);

			CHECK(code_edit->is_in_string(1) == -1);
		}

		SUBCASE("[CodeEdit] single line comment delimiters") {
			/* Blank end key should set lineonly to true. */
			code_edit->add_comment_delimiter("#", "", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* Insert line above, line with comment then line below. */
			code_edit->insert_text_at_caret(" \n#\n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column after start key is in comment and start / end positions are correct. */
			CHECK(code_edit->is_in_comment(1, 1) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 1) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 1) == Point2(2, 1));

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(2, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(2, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(2, 1) == OUTSIDE_DELIMITER);

			/* Check region metadata. */
			int idx = code_edit->is_in_comment(1, 1);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "");

			/* Check nested comments are handled correctly. */
			code_edit->set_text(" \n#  # \n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before first start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column after the first start key is in comment and start / end positions are correct. */
			CHECK(code_edit->is_in_comment(1, 1) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 1) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 1) == Point2(6, 1));

			/* Check column after the second start key returns data for the first. */
			CHECK(code_edit->is_in_comment(1, 5) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 5) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 5) == Point2(6, 1));

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(2, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(2, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(2, 1) == OUTSIDE_DELIMITER);

			/* Check is in comment with no column returns true if entire line is comment excluding whitespace. */
			code_edit->set_text(" \n  #  # \n ");
			CHECK(code_edit->is_in_comment(1) != -1);

			code_edit->set_text(" \n  text #  # \n ");
			CHECK(code_edit->is_in_comment(1) == -1);

			/* Removing delimiter should update. */
			code_edit->set_text(" \n  #  # \n ");

			code_edit->remove_comment_delimiter("#");
			CHECK_FALSE(code_edit->has_comment_delimiter("$"));
			CHECK(code_edit->get_comment_delimiters().size() == 0);

			CHECK(code_edit->is_in_comment(1) == -1);

			/* Adding and clear should update. */
			code_edit->add_comment_delimiter("#", "", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);
			CHECK(code_edit->is_in_comment(1) != -1);

			code_edit->clear_comment_delimiters();
			CHECK_FALSE(code_edit->has_comment_delimiter("$"));
			CHECK(code_edit->get_comment_delimiters().size() == 0);

			CHECK(code_edit->is_in_comment(1) == -1);
		}

		SUBCASE("[CodeEdit] single line mixed delimiters") {
			/* Blank end key should set lineonly to true. */
			/* Add string delimiter. */
			code_edit->add_string_delimiter("&", "", false);
			CHECK(code_edit->has_string_delimiter("&"));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* Add comment delimiter. */
			code_edit->add_comment_delimiter("#", "", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* Nest a string delimiter inside a comment. */
			code_edit->set_text(" \n#  & \n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before first start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column after the first start key is in comment and start / end positions are correct. */
			CHECK(code_edit->is_in_comment(1, 1) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 1) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 1) == Point2(6, 1));

			/* Check column after the second start key returns data for the first, and does not state string. */
			CHECK(code_edit->is_in_comment(1, 5) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 5) == Point2(1, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 5) == Point2(6, 1));
			CHECK(code_edit->is_in_string(1, 5) == -1);

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(2, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(2, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(2, 1) == OUTSIDE_DELIMITER);

			/* Remove the comment delimiter. */
			code_edit->remove_comment_delimiter("#");
			CHECK_FALSE(code_edit->has_comment_delimiter("$"));
			CHECK(code_edit->get_comment_delimiters().size() == 0);

			/* The "first" comment region is no longer valid. */
			CHECK(code_edit->is_in_comment(1, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 1) == OUTSIDE_DELIMITER);

			/* The "second" region as string is now valid. */
			CHECK(code_edit->is_in_string(1, 5) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 5) == Point2(4, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 5) == Point2(6, 1));
		}
	}

	SUBCASE("[CodeEdit] multiline delimiters") {
		SUBCASE("[CodeEdit] multiline string delimiters") {
			code_edit->clear_string_delimiters();
			code_edit->clear_comment_delimiters();

			/* Add string delimiter. */
			code_edit->add_string_delimiter("#", "#", false);
			CHECK(code_edit->has_string_delimiter("#"));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* First test over a single line. */
			code_edit->set_text(" \n #  # \n ");

			/* Check line above is not in string. */
			CHECK(code_edit->is_in_string(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in string. */
			CHECK(code_edit->is_in_string(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column before closing delimiter is in string. */
			CHECK(code_edit->is_in_string(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(5, 1));

			/* Check column after end key is not in string. */
			CHECK(code_edit->is_in_string(1, 6) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 6) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 6) == OUTSIDE_DELIMITER);

			/* Check line after is not in string. */
			CHECK(code_edit->is_in_string(2, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(2, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(2, 1) == OUTSIDE_DELIMITER);

			/* Check the region metadata. */
			int idx = code_edit->is_in_string(1, 2);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Next test over a multiple blank lines. */
			code_edit->set_text(" \n # \n\n # \n ");

			/* Check line above is not in string. */
			CHECK(code_edit->is_in_string(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in string. */
			CHECK(code_edit->is_in_string(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column just after start key is in string. */
			CHECK(code_edit->is_in_string(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(2, 3));

			/* Check blank middle line. */
			CHECK(code_edit->is_in_string(2, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(2, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(2, 0) == Point2(2, 3));

			/* Check column just before end key is in string. */
			CHECK(code_edit->is_in_string(3, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(3, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(3, 0) == Point2(2, 3));

			/* Check column after end key is not in string. */
			CHECK(code_edit->is_in_string(3, 3) == -1);
			CHECK(code_edit->get_delimiter_start_position(3, 3) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(3, 3) == OUTSIDE_DELIMITER);

			/* Check line after is not in string. */
			CHECK(code_edit->is_in_string(4, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(4, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(4, 1) == OUTSIDE_DELIMITER);

			/* Next test over a multiple non-blank lines. */
			code_edit->set_text(" \n # \n \n # \n ");

			/* Check line above is not in string. */
			CHECK(code_edit->is_in_string(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in string. */
			CHECK(code_edit->is_in_string(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column just after start key is in string. */
			CHECK(code_edit->is_in_string(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(2, 3));

			/* Check middle line. */
			CHECK(code_edit->is_in_string(2, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(2, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(2, 0) == Point2(2, 3));

			/* Check column just before end key is in string. */
			CHECK(code_edit->is_in_string(3, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(3, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(3, 0) == Point2(2, 3));

			/* Check column after end key is not in string. */
			CHECK(code_edit->is_in_string(3, 3) == -1);
			CHECK(code_edit->get_delimiter_start_position(3, 3) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(3, 3) == OUTSIDE_DELIMITER);

			/* Check line after is not in string. */
			CHECK(code_edit->is_in_string(4, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(4, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(4, 1) == OUTSIDE_DELIMITER);

			/* check the region metadata. */
			idx = code_edit->is_in_string(1, 2);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Next test nested strings. */
			code_edit->add_string_delimiter("^", "^", false);
			CHECK(code_edit->has_string_delimiter("^"));
			CHECK(code_edit->get_string_delimiters().size() == 2);

			code_edit->set_text(" \n # ^\n \n^ # \n ");

			/* Check line above is not in string. */
			CHECK(code_edit->is_in_string(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in string. */
			CHECK(code_edit->is_in_string(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column just after start key is in string. */
			CHECK(code_edit->is_in_string(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(3, 3));

			/* Check middle line. */
			CHECK(code_edit->is_in_string(2, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(2, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(2, 0) == Point2(3, 3));

			/* Check column just before end key is in string. */
			CHECK(code_edit->is_in_string(3, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(3, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(3, 0) == Point2(3, 3));

			/* Check column after end key is not in string. */
			CHECK(code_edit->is_in_string(3, 3) == -1);
			CHECK(code_edit->get_delimiter_start_position(3, 3) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(3, 3) == OUTSIDE_DELIMITER);

			/* Check line after is not in string. */
			CHECK(code_edit->is_in_string(4, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(4, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(4, 1) == OUTSIDE_DELIMITER);

			/* check the region metadata. */
			idx = code_edit->is_in_string(1, 2);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Next test no end key. */
			code_edit->set_text(" \n # \n ");

			/* check the region metadata. */
			idx = code_edit->is_in_string(1, 2);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(-1, -1));
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Check is in string with no column returns true if entire line is string excluding whitespace. */
			code_edit->set_text(" \n # \n\n #\n ");
			CHECK(code_edit->is_in_string(1) != -1);
			CHECK(code_edit->is_in_string(2) != -1);
			CHECK(code_edit->is_in_string(3) != -1);

			code_edit->set_text(" \n test # \n\n # test \n ");
			CHECK(code_edit->is_in_string(1) == -1);
			CHECK(code_edit->is_in_string(2) != -1);
			CHECK(code_edit->is_in_string(3) == -1);

			/* Next check updating the delimiter cache while typing. */
			code_edit->set_text("\n\n");
			code_edit->set_caret_line(0);
			code_edit->set_caret_column(0);
			CHECK(code_edit->is_in_string(0) == -1);
			CHECK(code_edit->is_in_string(1) == -1);
			code_edit->insert_text_at_caret("#");
			CHECK(code_edit->is_in_string(0) != -1);
			CHECK(code_edit->is_in_string(1) != -1);
			code_edit->insert_text_at_caret("#");
			CHECK(code_edit->is_in_string(0) != -1);
			CHECK(code_edit->is_in_string(1) == -1);
		}

		SUBCASE("[CodeEdit] multiline comment delimiters") {
			/* Add comment delimiter. */
			code_edit->add_comment_delimiter("#", "#", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* First test over a single line. */
			code_edit->set_text(" \n #  # \n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column before closing delimiter is in comment. */
			CHECK(code_edit->is_in_comment(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(5, 1));

			/* Check column after end key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 6) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 6) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 6) == OUTSIDE_DELIMITER);

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(2, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(2, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(2, 1) == OUTSIDE_DELIMITER);

			/* Check the region metadata. */
			int idx = code_edit->is_in_comment(1, 2);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Next test over a multiple blank lines. */
			code_edit->set_text(" \n # \n\n # \n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column just after start key is in comment. */
			CHECK(code_edit->is_in_comment(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(2, 3));

			/* Check blank middle line. */
			CHECK(code_edit->is_in_comment(2, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(2, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(2, 0) == Point2(2, 3));

			/* Check column just before end key is in comment. */
			CHECK(code_edit->is_in_comment(3, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(3, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(3, 0) == Point2(2, 3));

			/* Check column after end key is not in comment. */
			CHECK(code_edit->is_in_comment(3, 3) == -1);
			CHECK(code_edit->get_delimiter_start_position(3, 3) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(3, 3) == OUTSIDE_DELIMITER);

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(4, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(4, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(4, 1) == OUTSIDE_DELIMITER);

			/* Next test over a multiple non-blank lines. */
			code_edit->set_text(" \n # \n \n # \n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column just after start key is in comment. */
			CHECK(code_edit->is_in_comment(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(2, 3));

			/* Check middle line. */
			CHECK(code_edit->is_in_comment(2, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(2, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(2, 0) == Point2(2, 3));

			/* Check column just before end key is in comment. */
			CHECK(code_edit->is_in_comment(3, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(3, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(3, 0) == Point2(2, 3));

			/* Check column after end key is not in comment. */
			CHECK(code_edit->is_in_comment(3, 3) == -1);
			CHECK(code_edit->get_delimiter_start_position(3, 3) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(3, 3) == OUTSIDE_DELIMITER);

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(4, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(4, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(4, 1) == OUTSIDE_DELIMITER);

			/* check the region metadata. */
			idx = code_edit->is_in_comment(1, 2);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Next test nested comments. */
			code_edit->add_comment_delimiter("^", "^", false);
			CHECK(code_edit->has_comment_delimiter("^"));
			CHECK(code_edit->get_comment_delimiters().size() == 2);

			code_edit->set_text(" \n # ^\n \n^ # \n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column just after start key is in comment. */
			CHECK(code_edit->is_in_comment(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(3, 3));

			/* Check middle line. */
			CHECK(code_edit->is_in_comment(2, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(2, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(2, 0) == Point2(3, 3));

			/* Check column just before end key is in comment. */
			CHECK(code_edit->is_in_comment(3, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(3, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(3, 0) == Point2(3, 3));

			/* Check column after end key is not in comment. */
			CHECK(code_edit->is_in_comment(3, 3) == -1);
			CHECK(code_edit->get_delimiter_start_position(3, 3) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(3, 3) == OUTSIDE_DELIMITER);

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(4, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(4, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(4, 1) == OUTSIDE_DELIMITER);

			/* check the region metadata. */
			idx = code_edit->is_in_comment(1, 2);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Next test no end key. */
			code_edit->set_text(" \n # \n ");

			/* check the region metadata. */
			idx = code_edit->is_in_comment(1, 2);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(-1, -1));
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Check is in comment with no column returns true if entire line is comment excluding whitespace. */
			code_edit->set_text(" \n # \n\n #\n ");
			CHECK(code_edit->is_in_comment(1) != -1);
			CHECK(code_edit->is_in_comment(2) != -1);
			CHECK(code_edit->is_in_comment(3) != -1);

			code_edit->set_text(" \n test # \n\n # test \n ");
			CHECK(code_edit->is_in_comment(1) == -1);
			CHECK(code_edit->is_in_comment(2) != -1);
			CHECK(code_edit->is_in_comment(3) == -1);

			/* Next check updating the delimiter cache while typing. */
			code_edit->set_text("\n\n");
			code_edit->set_caret_line(0);
			code_edit->set_caret_column(0);
			CHECK(code_edit->is_in_comment(0) == -1);
			CHECK(code_edit->is_in_comment(1) == -1);
			code_edit->insert_text_at_caret("#");
			CHECK(code_edit->is_in_comment(0) != -1);
			CHECK(code_edit->is_in_comment(1) != -1);
			code_edit->insert_text_at_caret("#");
			CHECK(code_edit->is_in_comment(0) != -1);
			CHECK(code_edit->is_in_comment(1) == -1);
		}

		SUBCASE("[CodeEdit] multiline mixed delimiters") {
			/* Add comment delimiter. */
			code_edit->add_comment_delimiter("#", "#", false);
			CHECK(code_edit->has_comment_delimiter("#"));
			CHECK(code_edit->get_comment_delimiters().size() == 1);

			/* Add string delimiter. */
			code_edit->add_string_delimiter("^", "^", false);
			CHECK(code_edit->has_string_delimiter("^"));
			CHECK(code_edit->get_string_delimiters().size() == 1);

			/* Nest a string inside a comment. */
			code_edit->set_text(" \n # ^\n \n^ # \n ");

			/* Check line above is not in comment. */
			CHECK(code_edit->is_in_comment(0, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(0, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(0, 1) == OUTSIDE_DELIMITER);

			/* Check column before start key is not in comment. */
			CHECK(code_edit->is_in_comment(1, 0) == -1);
			CHECK(code_edit->get_delimiter_start_position(1, 0) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(1, 0) == OUTSIDE_DELIMITER);

			/* Check column just after start key is in comment. */
			CHECK(code_edit->is_in_comment(1, 2) != -1);
			CHECK(code_edit->get_delimiter_start_position(1, 2) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(1, 2) == Point2(3, 3));

			/* Check middle line. */
			CHECK(code_edit->is_in_comment(2, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(2, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(2, 0) == Point2(3, 3));

			/* Check column just before end key is in comment. */
			CHECK(code_edit->is_in_comment(3, 0) != -1);
			CHECK(code_edit->get_delimiter_start_position(3, 0) == Point2(2, 1));
			CHECK(code_edit->get_delimiter_end_position(3, 0) == Point2(3, 3));

			/* Check column after end key is not in comment. */
			CHECK(code_edit->is_in_comment(3, 3) == -1);
			CHECK(code_edit->get_delimiter_start_position(3, 3) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(3, 3) == OUTSIDE_DELIMITER);

			/* Check line after is not in comment. */
			CHECK(code_edit->is_in_comment(4, 1) == -1);
			CHECK(code_edit->get_delimiter_start_position(4, 1) == OUTSIDE_DELIMITER);
			CHECK(code_edit->get_delimiter_end_position(4, 1) == OUTSIDE_DELIMITER);

			/* check the region metadata. */
			int idx = code_edit->is_in_comment(1, 2);
			CHECK(code_edit->get_delimiter_start_key(idx) == "#");
			CHECK(code_edit->get_delimiter_end_key(idx) == "#");

			/* Check is in comment with no column returns true as inner delimiter should not be counted. */
			CHECK(code_edit->is_in_comment(1) != -1);
			CHECK(code_edit->is_in_comment(2) != -1);
			CHECK(code_edit->is_in_comment(3) != -1);
		}
	}

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] indent") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	SUBCASE("[CodeEdit] indent settings") {
		code_edit->set_indent_size(10);
		CHECK(code_edit->get_indent_size() == 10);
		CHECK(code_edit->get_tab_size() == 10);

		code_edit->set_auto_indent_enabled(false);
		CHECK_FALSE(code_edit->is_auto_indent_enabled());

		code_edit->set_auto_indent_enabled(true);
		CHECK(code_edit->is_auto_indent_enabled());

		code_edit->set_indent_using_spaces(false);
		CHECK_FALSE(code_edit->is_indent_using_spaces());

		code_edit->set_indent_using_spaces(true);
		CHECK(code_edit->is_indent_using_spaces());

		/* Only the first char is registered. */
		Vector<String> auto_indent_prefixes = { "::", "s", "1" };
		code_edit->set_auto_indent_prefixes(auto_indent_prefixes);

		auto_indent_prefixes = code_edit->get_auto_indent_prefixes();
		CHECK(auto_indent_prefixes.has(":"));
		CHECK(auto_indent_prefixes.has("s"));
		CHECK(auto_indent_prefixes.has("1"));
	}

	SUBCASE("[CodeEdit] indent tabs") {
		code_edit->set_indent_size(4);
		code_edit->set_auto_indent_enabled(true);
		code_edit->set_indent_using_spaces(false);

		/* Do nothing if not editable. */
		code_edit->set_editable(false);

		code_edit->do_indent();
		CHECK(code_edit->get_line(0).is_empty());

		code_edit->indent_lines();
		CHECK(code_edit->get_line(0).is_empty());

		code_edit->set_editable(true);

		/* Simple indent. */
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\t");

		/* Check input action. */
		SEND_GUI_ACTION("ui_text_indent");
		CHECK(code_edit->get_line(0) == "\t\t");

		/* Insert in place. */
		code_edit->set_text("");
		code_edit->insert_text_at_caret("test");
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "test\t");

		// Insert in place with multiple carets.
		code_edit->set_text("test text");
		code_edit->set_caret_column(5);
		code_edit->add_caret(0, 7);
		code_edit->add_caret(0, 2);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "te\tst \tte\txt");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK(code_edit->get_caret_column(0) == 7);
		CHECK(code_edit->get_caret_column(1) == 10);
		CHECK(code_edit->get_caret_column(2) == 3);
		code_edit->remove_secondary_carets();

		// Indent lines does entire line and works without selection.
		code_edit->set_text("");
		code_edit->insert_text_at_caret("test");
		code_edit->indent_lines();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_caret_column() == 5);

		/* Selection does entire line. */
		code_edit->set_text("test");
		code_edit->select_all();
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 5);

		// Selection does entire line, right to left selection.
		code_edit->set_text("test");
		code_edit->select(0, 4, 0, 0);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 5);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		/* Handles multiple lines. */
		code_edit->set_text("test\ntext");
		code_edit->select_all();
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_line(1) == "\ttext");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 5);

		/* Do not indent line if last col is zero. */
		code_edit->set_text("test\ntext");
		code_edit->select(0, 0, 1, 0);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		/* Indent even if last column of first line. */
		code_edit->set_text("test\ntext");
		code_edit->select(0, 4, 1, 0);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 5);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Indent even if last column of first line, reversed.
		code_edit->set_text("test\ntext");
		code_edit->select(1, 0, 0, 4);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 5);

		/* Check selection is adjusted. */
		code_edit->set_text("test");
		code_edit->select(0, 1, 0, 2);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 3);

		// Indent once with multiple selections.
		code_edit->set_text("test");
		code_edit->select(0, 1, 0, 2);
		code_edit->add_caret(0, 4);
		code_edit->select(0, 4, 0, 3, 1);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 0);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 3);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 0);
		CHECK(code_edit->get_selection_origin_column(1) == 5);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 4);
	}

	SUBCASE("[CodeEdit] indent spaces") {
		code_edit->set_indent_size(4);
		code_edit->set_auto_indent_enabled(true);
		code_edit->set_indent_using_spaces(true);

		/* Do nothing if not editable. */
		code_edit->set_editable(false);

		code_edit->do_indent();
		CHECK(code_edit->get_line(0).is_empty());

		code_edit->indent_lines();
		CHECK(code_edit->get_line(0).is_empty());

		code_edit->set_editable(true);

		/* Simple indent. */
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    ");

		/* Check input action. */
		SEND_GUI_ACTION("ui_text_indent");
		CHECK(code_edit->get_line(0) == "        ");

		/* Insert in place. */
		code_edit->set_text("");
		code_edit->insert_text_at_caret("test");
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "test    ");

		// Insert in place with multiple carets.
		code_edit->set_text("test text");
		code_edit->set_caret_column(5);
		code_edit->add_caret(0, 7);
		code_edit->add_caret(0, 2);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "te  st    te  xt");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK(code_edit->get_caret_column(0) == 10);
		CHECK(code_edit->get_caret_column(1) == 14);
		CHECK(code_edit->get_caret_column(2) == 4);
		code_edit->remove_secondary_carets();

		// Indent lines does entire line and works without selection.
		code_edit->set_text("");
		code_edit->insert_text_at_caret("test");
		code_edit->indent_lines();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_caret_column() == 8);

		/* Selection does entire line. */
		code_edit->set_text("test");
		code_edit->select_all();
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 8);

		// Selection does entire line, right to left selection.
		code_edit->set_text("test");
		code_edit->select(0, 4, 0, 0);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		/* single indent only add required spaces. */
		code_edit->set_text(" test");
		code_edit->select_all();
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 8);

		/* Handles multiple lines. */
		code_edit->set_text("test\ntext");
		code_edit->select_all();
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_line(1) == "    text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 8);

		/* Do not indent line if last col is zero. */
		code_edit->set_text("test\ntext");
		code_edit->select(0, 0, 1, 0);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		/* Indent even if last column of first line. */
		code_edit->set_text("test\ntext");
		code_edit->select(0, 4, 1, 0);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Indent even if last column of first line, right to left selection.
		code_edit->set_text("test\ntext");
		code_edit->select(1, 0, 0, 4);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 8);

		/* Check selection is adjusted. */
		code_edit->set_text("test");
		code_edit->select(0, 1, 0, 2);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 5);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 6);

		// Indent once with multiple selections.
		code_edit->set_text("test");
		code_edit->select(0, 1, 0, 2);
		code_edit->add_caret(0, 4);
		code_edit->select(0, 4, 0, 3, 1);
		code_edit->do_indent();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 0);
		CHECK(code_edit->get_selection_origin_column(0) == 5);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 6);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 0);
		CHECK(code_edit->get_selection_origin_column(1) == 8);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 7);
	}

	SUBCASE("[CodeEdit] unindent tabs") {
		code_edit->set_indent_size(4);
		code_edit->set_auto_indent_enabled(true);
		code_edit->set_indent_using_spaces(false);

		/* Do nothing if not editable. */
		code_edit->set_text("\t");

		code_edit->set_editable(false);

		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\t");

		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\t");

		code_edit->set_editable(true);

		/* Simple unindent. */
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "");

		/* Backspace does a simple unindent. */
		code_edit->set_text("");
		code_edit->insert_text_at_caret("\t");
		code_edit->backspace();
		CHECK(code_edit->get_line(0) == "");

		/* Unindent lines does entire line and works without selection. */
		code_edit->set_text("");
		code_edit->insert_text_at_caret("\ttest");
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_caret_column() == 4);

		// Unindent lines once with multiple carets.
		code_edit->set_text("\t\ttest");
		code_edit->set_caret_column(1);
		code_edit->add_caret(0, 3);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 2);
		code_edit->remove_secondary_carets();

		/* Caret on col zero unindent line. */
		code_edit->set_text("\t\ttest");
		code_edit->set_caret_column(0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_caret_column() == 0);

		/* Check input action. */
		code_edit->set_text("\t\ttest");
		SEND_GUI_ACTION("ui_text_dedent");
		CHECK(code_edit->get_line(0) == "\ttest");

		/* Selection does entire line. */
		code_edit->set_text("\t\ttest");
		code_edit->select_all();
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 5);

		// Selection does entire line, right to left selection.
		code_edit->set_text("\t\ttest");
		code_edit->select(0, 6, 0, 0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 5);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Handles multiple lines.
		code_edit->set_text("\t\ttest\n\t\ttext");
		code_edit->select_all();
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_line(1) == "\ttext");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 5);

		/* Do not unindent line if last col is zero. */
		code_edit->set_text("\ttest\n\ttext");
		code_edit->select(0, 0, 1, 0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_line(1) == "\ttext");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Do not unindent line if last col is zero, right to left selection.
		code_edit->set_text("\ttest\n\ttext");
		code_edit->select(1, 0, 0, 0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_line(1) == "\ttext");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		/* Unindent even if last column of first line. */
		code_edit->set_text("\ttest\n\ttext");
		code_edit->select(0, 5, 1, 1);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 4);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		/* Check selection is adjusted. */
		code_edit->set_text("\ttest");
		code_edit->select(0, 1, 0, 2);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 1);

		// Deselect if only the tab was selected.
		code_edit->set_text("\ttest");
		code_edit->select(0, 0, 0, 1);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Unindent once with multiple selections.
		code_edit->set_text("\t\ttest");
		code_edit->select(0, 1, 0, 2);
		code_edit->add_caret(0, 4);
		code_edit->select(0, 4, 0, 3, 1);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 0);
		CHECK(code_edit->get_selection_origin_column(0) == 0);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 1);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 0);
		CHECK(code_edit->get_selection_origin_column(1) == 3);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 2);
	}

	SUBCASE("[CodeEdit] unindent spaces") {
		code_edit->set_indent_size(4);
		code_edit->set_auto_indent_enabled(true);
		code_edit->set_indent_using_spaces(true);

		/* Do nothing if not editable. */
		code_edit->set_text("    ");

		code_edit->set_editable(false);

		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    ");

		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    ");

		code_edit->set_editable(true);

		/* Simple unindent. */
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "");

		/* Backspace does a simple unindent. */
		code_edit->set_text("");
		code_edit->insert_text_at_caret("    ");
		code_edit->backspace();
		CHECK(code_edit->get_line(0) == "");

		/* Backspace with letter. */
		code_edit->set_text("");
		code_edit->insert_text_at_caret("    a");
		code_edit->backspace();
		CHECK(code_edit->get_line(0) == "    ");

		/* Unindent lines does entire line and works without selection. */
		code_edit->set_text("");
		code_edit->insert_text_at_caret("    test");
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_caret_column() == 4);

		// Unindent lines once with multiple carets.
		code_edit->set_text("        test");
		code_edit->set_caret_column(1);
		code_edit->add_caret(0, 9);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 5);
		code_edit->remove_secondary_carets();

		/* Caret on col zero unindent line. */
		code_edit->set_text("        test");
		code_edit->set_caret_column(0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_caret_column() == 0);

		/* Only as far as needed */
		code_edit->set_text("       test");
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    test");

		/* Check input action. */
		code_edit->set_text("        test");
		SEND_GUI_ACTION("ui_text_dedent");
		CHECK(code_edit->get_line(0) == "    test");

		/* Selection does entire line. */
		code_edit->set_text("        test");
		code_edit->select_all();
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 8);

		// Selection does entire line, right to left selection.
		code_edit->set_text("        test");
		code_edit->select(0, 12, 0, 0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Handles multiple lines.
		code_edit->set_text("        test\n        text");
		code_edit->select_all();
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_line(1) == "    text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 8);

		/* Do not unindent line if last col is zero. */
		code_edit->set_text("    test\n    text");
		code_edit->select(0, 0, 1, 0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_line(1) == "    text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Do not unindent line if last col is zero, right to left selection.
		code_edit->set_text("    test\n    text");
		code_edit->select(1, 0, 0, 0);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_line(1) == "    text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		/* Unindent even if last column of first line. */
		code_edit->set_text("    test\n    text");
		code_edit->select(0, 5, 1, 1);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->get_line(1) == "text");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 1);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		/* Check selection is adjusted. */
		code_edit->set_text("    test");
		code_edit->select(0, 4, 0, 5);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 1);

		// Deselect if only the tab was selected.
		code_edit->set_text("    test");
		code_edit->select(0, 0, 0, 4);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "test");
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Unindent once with multiple selections.
		code_edit->set_text("        test");
		code_edit->select(0, 1, 0, 2);
		code_edit->add_caret(0, 4);
		code_edit->select(0, 12, 0, 10, 1);
		code_edit->unindent_lines();
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK_FALSE(code_edit->has_selection(0));
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 0);
		CHECK(code_edit->get_selection_origin_column(1) == 8);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 6);
	}

	SUBCASE("[CodeEdit] auto indent") {
		SUBCASE("[CodeEdit] auto indent tabs") {
			code_edit->set_indent_size(4);
			code_edit->set_auto_indent_enabled(true);
			code_edit->set_indent_using_spaces(false);

			/* Simple indent on new line. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test:");
			CHECK(code_edit->get_line(1) == "\t");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 1);

			/* new blank line should still indent. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:");
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line(0) == "test:");
			CHECK(code_edit->get_line(1) == "\t");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 1);

			/* new line above should not indent. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:");
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line(0) == "");
			CHECK(code_edit->get_line(1) == "test:");
			CHECK(code_edit->get_caret_line() == 0);
			CHECK(code_edit->get_caret_column() == 0);

			/* Whitespace between symbol and caret is okay. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:  ");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test:  ");
			CHECK(code_edit->get_line(1) == "\t");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 1);

			/* Comment between symbol and caret is okay. */
			code_edit->add_comment_delimiter("#", "");
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test: # comment");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test: # comment");
			CHECK(code_edit->get_line(1) == "\t");
			code_edit->remove_comment_delimiter("#");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 1);

			/* Strings between symbol and caret are not okay. */
			code_edit->add_string_delimiter("#", "");
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test: # string");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test: # string");
			CHECK(code_edit->get_line(1) == "");
			code_edit->remove_string_delimiter("#");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);

			/* Non-whitespace prevents auto-indentation. */
			code_edit->add_comment_delimiter("#", "");
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test := 0 # comment");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test := 0 # comment");
			CHECK(code_edit->get_line(1) == "");
			code_edit->remove_comment_delimiter("#");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);

			/* Even when there's no comments. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test := 0");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test := 0");
			CHECK(code_edit->get_line(1) == "");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);

			// Preserve current indentation.
			code_edit->set_text("\ttest");
			code_edit->set_caret_column(3);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "\tte");
			CHECK(code_edit->get_line(1) == "\tst");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 1);

			// Preserve current indentation blank.
			code_edit->set_text("\ttest");
			code_edit->set_caret_column(3);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line(0) == "\ttest");
			CHECK(code_edit->get_line(1) == "\t");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 1);

			// Preserve current indentation above.
			code_edit->set_text("\ttest");
			code_edit->set_caret_column(3);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line(0) == "\t");
			CHECK(code_edit->get_line(1) == "\ttest");
			CHECK(code_edit->get_caret_line() == 0);
			CHECK(code_edit->get_caret_column() == 1);

			// Increase existing indentation.
			code_edit->set_text("\ttest:");
			code_edit->set_caret_column(6);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "\ttest:");
			CHECK(code_edit->get_line(1) == "\t\t");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 2);

			// Increase existing indentation blank.
			code_edit->set_text("\ttest:");
			code_edit->set_caret_column(3);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line(0) == "\ttest:");
			CHECK(code_edit->get_line(1) == "\t\t");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 2);

			/* If between brace pairs an extra line is added. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test{}");
			code_edit->set_caret_column(5);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test{");
			CHECK(code_edit->get_line(1) == "\t");
			CHECK(code_edit->get_line(2) == "}");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 1);

			/* Except when we are going above. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test{}");
			code_edit->set_caret_column(5);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line(0) == "");
			CHECK(code_edit->get_line(1) == "test{}");
			CHECK(code_edit->get_caret_line() == 0);
			CHECK(code_edit->get_caret_column() == 0);

			/* or below. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test{}");
			code_edit->set_caret_column(5);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line(0) == "test{}");
			CHECK(code_edit->get_line(1) == "");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);
		}

		SUBCASE("[CodeEdit] auto indent spaces") {
			code_edit->set_indent_size(4);
			code_edit->set_auto_indent_enabled(true);
			code_edit->set_indent_using_spaces(true);

			/* Simple indent on new line. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test:");
			CHECK(code_edit->get_line(1) == "    ");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 4);

			/* new blank line should still indent. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:");
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line(0) == "test:");
			CHECK(code_edit->get_line(1) == "    ");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 4);

			/* new line above should not indent. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:");
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line(0) == "");
			CHECK(code_edit->get_line(1) == "test:");
			CHECK(code_edit->get_caret_line() == 0);
			CHECK(code_edit->get_caret_column() == 0);

			/* Whitespace between symbol and caret is okay. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:  ");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test:  ");
			CHECK(code_edit->get_line(1) == "    ");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 4);

			/* Comment between symbol and caret is okay. */
			code_edit->add_comment_delimiter("#", "");
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test: # comment");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test: # comment");
			CHECK(code_edit->get_line(1) == "    ");
			code_edit->remove_comment_delimiter("#");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 4);

			/* Strings between symbol and caret are not okay. */
			code_edit->add_string_delimiter("#", "");
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test: # string");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test: # string");
			CHECK(code_edit->get_line(1) == "");
			code_edit->remove_string_delimiter("#");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);

			/* Non-whitespace prevents auto-indentation. */
			code_edit->add_comment_delimiter("#", "");
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test := 0 # comment");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test := 0 # comment");
			CHECK(code_edit->get_line(1) == "");
			code_edit->remove_comment_delimiter("#");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);

			/* Even when there's no comments. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test := 0");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test := 0");
			CHECK(code_edit->get_line(1) == "");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);

			// Preserve current indentation.
			code_edit->set_text("    test");
			code_edit->set_caret_column(6);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "    te");
			CHECK(code_edit->get_line(1) == "    st");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 4);

			// Preserve current indentation blank.
			code_edit->set_text("    test");
			code_edit->set_caret_column(6);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line(0) == "    test");
			CHECK(code_edit->get_line(1) == "    ");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 4);

			// Preserve current indentation above.
			code_edit->set_text("    test");
			code_edit->set_caret_column(6);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line(0) == "    ");
			CHECK(code_edit->get_line(1) == "    test");
			CHECK(code_edit->get_caret_line() == 0);
			CHECK(code_edit->get_caret_column() == 4);

			// Increase existing indentation.
			code_edit->set_text("    test:");
			code_edit->set_caret_column(9);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "    test:");
			CHECK(code_edit->get_line(1) == "        ");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 8);

			// Increase existing indentation blank.
			code_edit->set_text("    test:");
			code_edit->set_caret_column(9);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "    test:");
			CHECK(code_edit->get_line(1) == "        ");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 8);

			/* If between brace pairs an extra line is added. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test{}");
			code_edit->set_caret_column(5);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test{");
			CHECK(code_edit->get_line(1) == "    ");
			CHECK(code_edit->get_line(2) == "}");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 4);

			/* Except when we are going above. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test{}");
			code_edit->set_caret_column(5);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(code_edit->get_line(0) == "");
			CHECK(code_edit->get_line(1) == "test{}");
			CHECK(code_edit->get_caret_line() == 0);
			CHECK(code_edit->get_caret_column() == 0);

			/* or below. */
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test{}");
			code_edit->set_caret_column(5);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(code_edit->get_line(0) == "test{}");
			CHECK(code_edit->get_line(1) == "");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);

			/* If there is something after a colon
			and there is a colon in the comment it
			should not indent. */
			code_edit->add_comment_delimiter("#", "");
			code_edit->set_text("");
			code_edit->insert_text_at_caret("test:test#:");
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(code_edit->get_line(0) == "test:test#:");
			CHECK(code_edit->get_line(1) == "");
			code_edit->remove_comment_delimiter("#");
			CHECK(code_edit->get_caret_line() == 1);
			CHECK(code_edit->get_caret_column() == 0);
		}
	}

	SUBCASE("[CodeEdit] convert indent to tabs") {
		code_edit->set_indent_size(4);
		code_edit->set_indent_using_spaces(false);

		// Only line.
		code_edit->set_text("        test");
		code_edit->select(0, 9, 0, 8);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(0) == "\t\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 3);
		CHECK(code_edit->get_caret_column() == 2);

		// First line.
		code_edit->set_text("        test\n");
		code_edit->select(0, 8, 0, 9);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(0) == "\t\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_column() == 3);

		// Middle line.
		code_edit->set_text("\n        test\n");
		code_edit->select(1, 8, 1, 9);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(1) == "\t\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_column() == 3);

		// End line.
		code_edit->set_text("\n        test");
		code_edit->select(1, 8, 1, 9);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(1) == "\t\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_column() == 3);

		// Within provided range.
		code_edit->set_text("    test\n        test\n");
		code_edit->select(1, 8, 1, 9);
		code_edit->convert_indent(1, 1);
		CHECK(code_edit->get_line(0) == "    test");
		CHECK(code_edit->get_line(1) == "\t\ttest");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_column() == 3);

		// Multiline blocks.
		code_edit->set_text("'''\n    test\n        test\n'''");
		code_edit->select(1, 0, 1, 8);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(1) == "    test");
		CHECK(code_edit->get_line(2) == "        test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_column() == 8);
	}

	SUBCASE("[CodeEdit] convert indent to spaces") {
		code_edit->set_indent_size(4);
		code_edit->set_indent_using_spaces(true);

		// Only line.
		code_edit->set_text("\t\ttest");
		code_edit->select(0, 3, 0, 2);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(0) == "        test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 9);
		CHECK(code_edit->get_caret_column() == 8);

		// First line.
		code_edit->set_text("\t\ttest\n");
		code_edit->select(0, 2, 0, 3);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(0) == "        test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_column() == 9);

		// Middle line.
		code_edit->set_text("\n\t\ttest\n");
		code_edit->select(1, 2, 1, 3);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(1) == "        test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_column() == 9);

		// End line.
		code_edit->set_text("\n\t\ttest");
		code_edit->select(1, 2, 1, 3);
		code_edit->convert_indent();
		CHECK(code_edit->get_line(1) == "        test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_column() == 9);

		// Within provided range.
		code_edit->set_text("\ttest\n\t\ttest\n");
		code_edit->select(1, 2, 1, 3);
		code_edit->convert_indent(1, 1);
		CHECK(code_edit->get_line(0) == "\ttest");
		CHECK(code_edit->get_line(1) == "        test");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_column() == 9);

		// Outside of range.
		ERR_PRINT_OFF;
		code_edit->convert_indent(0, 4);
		code_edit->convert_indent(4, 5);
		code_edit->convert_indent(4, 1);
		ERR_PRINT_ON;
	}

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] folding") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();
	code_edit->set_line_folding_enabled(true);

	SUBCASE("[CodeEdit] folding settings") {
		code_edit->set_line_folding_enabled(true);
		CHECK(code_edit->is_line_folding_enabled());

		code_edit->set_line_folding_enabled(false);
		CHECK_FALSE(code_edit->is_line_folding_enabled());
	}

	SUBCASE("[CodeEdit] folding") {
		// No indent.
		code_edit->set_text("line1\nline2\nline3");
		for (int i = 0; i < 2; i++) {
			CHECK_FALSE(code_edit->can_fold_line(i));
			code_edit->fold_line(i);
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Indented lines.
		code_edit->set_text("\tline1\n\tline2\n\tline3");
		for (int i = 0; i < 2; i++) {
			CHECK_FALSE(code_edit->can_fold_line(i));
			code_edit->fold_line(i);
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Indent.
		code_edit->set_text("line1\n\tline2\nline3");
		CHECK(code_edit->can_fold_line(0));
		for (int i = 1; i < 2; i++) {
			CHECK_FALSE(code_edit->can_fold_line(i));
			code_edit->fold_line(i);
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK_FALSE(code_edit->is_line_folded(2));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 2);

		// Indent with blank lines.
		code_edit->set_text("line1\n\tline2\n\n\nline3");
		CHECK(code_edit->can_fold_line(0));
		for (int i = 1; i < 2; i++) {
			CHECK_FALSE(code_edit->can_fold_line(i));
			code_edit->fold_line(i);
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK_FALSE(code_edit->is_line_folded(2));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 2);

		// Nested indents.
		code_edit->set_text("line1\n\tline2\n\t\tline3\nline4");
		CHECK(code_edit->can_fold_line(0));
		CHECK(code_edit->can_fold_line(1));
		for (int i = 2; i < 3; i++) {
			CHECK_FALSE(code_edit->can_fold_line(i));
			code_edit->fold_line(i);
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK(code_edit->is_line_folded(1));
		CHECK_FALSE(code_edit->is_line_folded(2));
		CHECK_FALSE(code_edit->is_line_folded(3));
		CHECK(code_edit->get_next_visible_line_offset_from(2, 1) == 2);

		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK_FALSE(code_edit->is_line_folded(2));
		CHECK_FALSE(code_edit->is_line_folded(3));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 3);

		// Check metadata.
		CHECK(code_edit->get_folded_lines().size() == 1);
		CHECK(code_edit->get_folded_lines_bind().size() == 1);
		CHECK((int)code_edit->get_folded_lines()[0] == 0);
		CHECK((int)code_edit->get_folded_lines_bind()[0] == 0);

		// Cannot unfold nested.
		code_edit->unfold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// (un)Fold all / toggle.
		code_edit->unfold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Check metadata.
		CHECK(code_edit->get_folded_lines().size() == 0);
		CHECK(code_edit->get_folded_lines_bind().size() == 0);

		code_edit->fold_all_lines();
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 3);

		code_edit->unfold_all_lines();
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		code_edit->toggle_foldable_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 3);

		// Can also unfold from hidden line.
		code_edit->unfold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Blank lines.
		code_edit->set_text("line1\n\tline2\n\n\n\ttest\n\nline3");
		CHECK(code_edit->can_fold_line(0));
		for (int i = 1; i < code_edit->get_line_count(); i++) {
			CHECK_FALSE(code_edit->can_fold_line(i));
			code_edit->fold_line(i);
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		for (int i = 1; i < code_edit->get_line_count(); i++) {
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 5);

		// End of file.
		code_edit->set_text("line1\n\tline2");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Comment & string blocks.
		// Single line block
		code_edit->add_comment_delimiter("#", "", true);
		code_edit->set_text("#line1\n#\tline2");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Has to be full line.
		code_edit->set_text("test #line1\n#\tline2");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		code_edit->set_text("#line1\ntest #\tline2");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// String.
		code_edit->add_string_delimiter("^", "", true);
		code_edit->set_text("^line1\n^\tline2");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Has to be full line.
		code_edit->set_text("test ^line1\n^\tline2");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		code_edit->set_text("^line1\ntest ^\tline2");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Multiline blocks.
		code_edit->add_comment_delimiter("&", "&", false);
		code_edit->set_text("&line1\n\tline2&\nline3");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 2);

		// Multiline comment before last line.
		code_edit->set_text("&line1\nline2&\ntest");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(2));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 2);

		// Has to be full line.
		code_edit->set_text("test &line1\n\tline2&");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		code_edit->set_text("&line1\n\tline2& test");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Strings.
		code_edit->add_string_delimiter("$", "$", false);
		code_edit->set_text("$line1\n\tline2$");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Has to be full line.
		code_edit->set_text("test $line1\n\tline2$");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		code_edit->set_text("$line1\n\tline2$ test");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 1);

		// Non-indented comments/strings.
		// Single line
		code_edit->set_text("test\n\tline1\n#line1\n#line2\n\ttest");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 4);

		code_edit->set_text("test\n\tline1\n^line1\n^line2\n\ttest");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 4);

		// Indent level 0->1, comment after lines
		code_edit->set_text("line1\n\tline2\n#test");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 2);

		// Indent level 0->1, comment between lines
		code_edit->set_text("line1\n#test\n\tline2\nline3");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(2));
		code_edit->fold_line(2);
		CHECK_FALSE(code_edit->is_line_folded(2));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(2));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 3);

		// Indent level 1->2, comment after lines
		code_edit->set_text("\tline1\n\t\tline2\n#test");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 2);

		// Indent level 1->2, comment between lines
		code_edit->set_text("\tline1\n#test\n\t\tline2\nline3");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(2));
		code_edit->fold_line(2);
		CHECK_FALSE(code_edit->is_line_folded(2));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(2));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 3);

		// Multiline
		code_edit->set_text("test\n\tline1\n&line1\nline2&\n\ttest");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 4);

		code_edit->set_text("test\n\tline1\n$line1\nline2$\n\ttest");
		CHECK(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(1));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 4);
	}

	SUBCASE("[CodeEdit] folding comments including and/or adjacent to code regions") {
		code_edit->add_comment_delimiter("#", "", true);

		// Single line comment directly above a code region tag is not foldable.
		code_edit->set_text("#line0\n#region a\nnothing\n#line3\n#endregion");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK_FALSE(code_edit->can_fold_line(3));

		// Comment blocks.
		// Foldable even when directly below a code region start tag.
		code_edit->set_text("#line0\n#line1\n#region a\n#line3\n#line4\nnothing\n#endregion");
		CHECK(code_edit->can_fold_line(3));

		// Doesn't fold beyond region start tag.
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_EQ(code_edit->get_visible_line_count_in_range(0, 1), 1);
		CHECK_EQ(code_edit->get_visible_line_count_in_range(2, 2), 1);

		// Foldable even when directly below a code region end tag.
		code_edit->set_text("#region a\nnothing\n#line2\n#line3\n#endregion\n#line5\n#line6");
		CHECK(code_edit->can_fold_line(5));

		// Doesn't fold beyond region end tag.
		code_edit->fold_line(2);
		CHECK(code_edit->is_line_folded(2));
		CHECK_EQ(code_edit->get_visible_line_count_in_range(2, 3), 1);
		CHECK_EQ(code_edit->get_visible_line_count_in_range(4, 4), 1);

		code_edit->add_comment_delimiter("/*", "*/", false);

		// Multiline comments.
		// Folds a region tag inside it.
		code_edit->set_text("/*\nnothing\n#region a\n*/\n#endregion");
		CHECK(code_edit->can_fold_line(0));
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_EQ(code_edit->get_visible_line_count_in_range(0, 3), 1);
		CHECK_EQ(code_edit->get_visible_line_count_in_range(4, 4), 1);
	}

	SUBCASE("[CodeEdit] folding carets") {
		// Folding a line moves all carets that would be hidden.
		code_edit->set_text("test\n\tline1\n\t\tline 2\n");
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(0);
		code_edit->add_caret(1, 3);
		code_edit->add_caret(2, 8);
		code_edit->add_caret(2, 1);
		code_edit->select(2, 0, 2, 1, 3);

		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK(code_edit->get_caret_count() == 1);
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 4);

		// Undoing an action that puts the caret on a folded line unfolds it.
		code_edit->set_text("test\n\tline1");
		code_edit->select(1, 1, 1, 2);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "test\n\tlline1");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 3);
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 2);
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 4);

		code_edit->undo();
		CHECK(code_edit->get_text() == "test\n\tline1");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 2);
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 1);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));

		// Redoing doesn't refold.
		code_edit->redo();
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 3);
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
	}

	SUBCASE("[CodeEdit] actions unfold") {
		// add_selection_for_next_occurrence unfolds.
		code_edit->set_text("test\n\tline1 test\n\t\tline 2\ntest2");
		code_edit->select(0, 0, 0, 4);
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		code_edit->add_selection_for_next_occurrence();

		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_caret_column() == 4);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_selection_origin_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 11);
		CHECK(code_edit->get_selection_origin_column(1) == 7);
		CHECK_FALSE(code_edit->is_line_folded(0));
		code_edit->remove_secondary_carets();

		// skip_selection_for_next_occurrence unfolds.
		code_edit->select(0, 0, 0, 4);
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		code_edit->skip_selection_for_next_occurrence();

		CHECK(code_edit->get_caret_count() == 1);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_caret_column() == 11);
		CHECK(code_edit->get_selection_origin_column() == 7);
		CHECK_FALSE(code_edit->is_line_folded(0));
		code_edit->remove_secondary_carets();
		code_edit->deselect();
	}

	SUBCASE("[CodeEdit] toggle folding carets") {
		code_edit->set_text("test\n\tline1\ntest2\n\tline2");

		// Fold lines with carets on them.
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(1);
		code_edit->toggle_foldable_lines_at_carets();
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(2));

		// Toggle fold on lines with carets.
		code_edit->add_caret(2, 0);
		code_edit->toggle_foldable_lines_at_carets();
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK(code_edit->is_line_folded(2));
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 1);
		CHECK(code_edit->get_caret_line(1) == 2);
		CHECK(code_edit->get_caret_column(1) == 0);

		// Multiple carets as part of one fold.
		code_edit->unfold_all_lines();
		code_edit->remove_secondary_carets();
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(1);
		code_edit->add_caret(0, 4);
		code_edit->add_caret(1, 2);
		code_edit->toggle_foldable_lines_at_carets();
		CHECK(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(2));
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 1);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 4);
	}

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] region folding") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	SUBCASE("[CodeEdit] region tags") {
		code_edit->set_line_folding_enabled(true);

		// Region tag detection.
		code_edit->set_text("#region region_name\nline2\n#endregion");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		CHECK(code_edit->is_line_code_region_start(0));
		CHECK_FALSE(code_edit->is_line_code_region_start(1));
		CHECK_FALSE(code_edit->is_line_code_region_start(2));
		CHECK_FALSE(code_edit->is_line_code_region_end(0));
		CHECK_FALSE(code_edit->is_line_code_region_end(1));
		CHECK(code_edit->is_line_code_region_end(2));

		// Region tag customization.
		code_edit->set_text("#region region_name\nline2\n#endregion\n#open region_name\nline2\n#close");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		CHECK(code_edit->is_line_code_region_start(0));
		CHECK(code_edit->is_line_code_region_end(2));
		CHECK_FALSE(code_edit->is_line_code_region_start(3));
		CHECK_FALSE(code_edit->is_line_code_region_end(5));
		code_edit->set_code_region_tags("open", "close");
		CHECK_FALSE(code_edit->is_line_code_region_start(0));
		CHECK_FALSE(code_edit->is_line_code_region_end(2));
		CHECK(code_edit->is_line_code_region_start(3));
		CHECK(code_edit->is_line_code_region_end(5));
		code_edit->set_code_region_tags("region", "endregion");

		// Setting identical start and end region tags should fail.
		CHECK(code_edit->get_code_region_start_tag() == "region");
		CHECK(code_edit->get_code_region_end_tag() == "endregion");
		ERR_PRINT_OFF;
		code_edit->set_code_region_tags("same_tag", "same_tag");
		ERR_PRINT_ON;
		CHECK(code_edit->get_code_region_start_tag() == "region");
		CHECK(code_edit->get_code_region_end_tag() == "endregion");
	}

	SUBCASE("[CodeEdit] create code region") {
		code_edit->set_line_folding_enabled(true);

		// Region creation with selection adds start and close region lines. Region name is selected and the region is folded.
		code_edit->set_text("line1\nline2\nline3");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->select(1, 0, 1, 4);
		code_edit->create_code_region();
		CHECK(code_edit->is_line_code_region_start(1));
		CHECK(code_edit->is_line_code_region_end(3));
		CHECK(code_edit->get_text() == "line1\n#region New Code Region\nline2\n#endregion\nline3");
		CHECK(code_edit->get_caret_count() == 1);
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selected_text() == "New Code Region");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 23);
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->is_line_folded(1));

		// Undo region creation. Line get unfolded.
		code_edit->undo();
		CHECK(code_edit->get_text() == "line1\nline2\nline3");
		CHECK(code_edit->get_caret_count() == 1);
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 4);
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK_FALSE(code_edit->is_line_folded(1));

		// Redo region creation.
		code_edit->redo();
		CHECK(code_edit->get_text() == "line1\n#region New Code Region\nline2\n#endregion\nline3");
		CHECK(code_edit->get_caret_count() == 1);
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selected_text() == "New Code Region");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 23);
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK_FALSE(code_edit->is_line_folded(1));

		// Region creation without any selection has no effect.
		code_edit->set_text("line1\nline2\nline3");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->create_code_region();
		CHECK(code_edit->get_text() == "line1\nline2\nline3");

		// Region creation with multiple selections. Secondary carets are removed and the first region name is selected.
		code_edit->set_text("line1\nline2\nline3");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->select(0, 0, 0, 4, 0);
		code_edit->add_caret(2, 5);
		code_edit->select(2, 0, 2, 5, 1);
		code_edit->create_code_region();
		CHECK(code_edit->get_text() == "#region New Code Region\nline1\n#endregion\nline2\n#region New Code Region\nline3\n#endregion");
		CHECK(code_edit->get_caret_count() == 1);
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selected_text() == "New Code Region");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 23);
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 8);

		// Region creation with mixed selection and non-selection carets. Regular carets are ignored.
		code_edit->set_text("line1\nline2\nline3");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->select(0, 0, 0, 4, 0);
		code_edit->add_caret(2, 5);
		code_edit->create_code_region();
		CHECK(code_edit->get_text() == "#region New Code Region\nline1\n#endregion\nline2\nline3");
		CHECK(code_edit->get_caret_count() == 1);
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selected_text() == "New Code Region");

		// Two selections on the same line create only one region.
		code_edit->set_text("test line1\ntest line2\ntest line3");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->select(0, 0, 1, 2, 0);
		code_edit->add_caret(1, 4);
		code_edit->select(1, 4, 2, 5, 1);
		code_edit->create_code_region();
		CHECK(code_edit->get_text() == "#region New Code Region\ntest line1\ntest line2\ntest line3\n#endregion");

		// Region tag with // comment delimiter.
		code_edit->set_text("//region region_name\nline2\n//endregion");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("//", "");
		CHECK(code_edit->is_line_code_region_start(0));
		CHECK(code_edit->is_line_code_region_end(2));

		// Creating region with no valid one line comment delimiter has no effect.
		code_edit->set_text("line1\nline2\nline3");
		code_edit->clear_comment_delimiters();
		code_edit->create_code_region();
		CHECK(code_edit->get_text() == "line1\nline2\nline3");
		code_edit->add_comment_delimiter("/*", "*/");
		code_edit->create_code_region();
		CHECK(code_edit->get_text() == "line1\nline2\nline3");
	}

	SUBCASE("[CodeEdit] region comment delimiters") {
		code_edit->set_line_folding_enabled(true);

		// Choose one line comment delimiter.
		code_edit->set_text("//region region_name\nline2\n//endregion");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("/*", "*/");
		code_edit->add_comment_delimiter("//", "");
		CHECK(code_edit->is_line_code_region_start(0));
		CHECK(code_edit->is_line_code_region_end(2));

		// Update code region delimiter when removing comment delimiter.
		code_edit->set_text("#region region_name\nline2\n#endregion\n//region region_name\nline2\n//endregion");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("//", "");
		code_edit->add_comment_delimiter("#", ""); // A shorter delimiter has higher priority.
		CHECK(code_edit->is_line_code_region_start(0));
		CHECK(code_edit->is_line_code_region_end(2));
		CHECK_FALSE(code_edit->is_line_code_region_start(3));
		CHECK_FALSE(code_edit->is_line_code_region_end(5));
		code_edit->remove_comment_delimiter("#");
		CHECK_FALSE(code_edit->is_line_code_region_start(0));
		CHECK_FALSE(code_edit->is_line_code_region_end(2));
		CHECK(code_edit->is_line_code_region_start(3));
		CHECK(code_edit->is_line_code_region_end(5));

		// Update code region delimiter when clearing comment delimiters.
		code_edit->set_text("//region region_name\nline2\n//endregion");
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("//", "");
		CHECK(code_edit->is_line_code_region_start(0));
		CHECK(code_edit->is_line_code_region_end(2));
		code_edit->clear_comment_delimiters();
		CHECK_FALSE(code_edit->is_line_code_region_start(0));
		CHECK_FALSE(code_edit->is_line_code_region_end(2));
	}

	SUBCASE("[CodeEdit] fold region") {
		code_edit->set_line_folding_enabled(true);

		// Fold region.
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->set_text("#region region_name\nline2\nline3\n#endregion\nvisible line");
		CHECK(code_edit->can_fold_line(0));
		for (int i = 1; i < 5; i++) {
			CHECK_FALSE(code_edit->can_fold_line(i));
		}
		for (int i = 0; i < 5; i++) {
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 4);

		// Region with no end can't be folded.
		ERR_PRINT_OFF;
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->set_text("#region region_name\nline2\nline3\n#bad_end_tag\nvisible line");
		CHECK_FALSE(code_edit->can_fold_line(0));
		ERR_PRINT_ON;

		// Bad nested region can't be folded.
		ERR_PRINT_OFF;
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->set_text("#region without end\n#region region2\nline3\n#endregion\n#no_end");
		CHECK_FALSE(code_edit->can_fold_line(0));
		CHECK(code_edit->can_fold_line(1));
		ERR_PRINT_ON;

		// Nested region folding.
		ERR_PRINT_OFF;
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->set_text("#region region1\n#region region2\nline3\n#endregion\n#endregion");
		CHECK(code_edit->can_fold_line(0));
		CHECK(code_edit->can_fold_line(1));
		code_edit->fold_line(1);
		CHECK(code_edit->get_next_visible_line_offset_from(2, 1) == 3);
		code_edit->fold_line(0);
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 4);
		ERR_PRINT_ON;

		// Unfolding a line inside a region unfold whole region.
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->set_text("#region region\ninside\nline3\n#endregion\nvisible");
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 4);
		code_edit->unfold_line(1);
		CHECK_FALSE(code_edit->is_line_folded(0));

		// Region start and end tags are ignored if in a string and at the start of the line.
		code_edit->clear_comment_delimiters();
		code_edit->add_comment_delimiter("#", "");
		code_edit->clear_string_delimiters();
		code_edit->add_string_delimiter("\"", "\"");
		code_edit->set_text("#region region_name1\nline2\n\"\n#region region_name2\n#endregion\n\"\n#endregion\nvisible");
		CHECK(code_edit->is_line_code_region_start(0));
		CHECK(code_edit->is_line_code_region_end(6));
		CHECK(code_edit->can_fold_line(0));
		for (int i = 1; i < 7; i++) {
			if (i == 2) {
				continue;
			}
			CHECK_FALSE(code_edit->can_fold_line(i));
		}
		for (int i = 0; i < 7; i++) {
			CHECK_FALSE(code_edit->is_line_folded(i));
		}
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		CHECK(code_edit->get_next_visible_line_offset_from(1, 1) == 7);
	}

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] completion") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	SUBCASE("[CodeEdit] auto brace completion") {
		code_edit->set_auto_brace_completion_enabled(true);
		CHECK(code_edit->is_auto_brace_completion_enabled());

		code_edit->set_highlight_matching_braces_enabled(true);
		CHECK(code_edit->is_highlight_matching_braces_enabled());

		/* Try setters, any length. */
		Dictionary auto_brace_completion_pairs;
		auto_brace_completion_pairs["["] = "]";
		auto_brace_completion_pairs["'"] = "'";
		auto_brace_completion_pairs[";"] = "'";
		auto_brace_completion_pairs["'''"] = "'''";
		code_edit->set_auto_brace_completion_pairs(auto_brace_completion_pairs);
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);
		CHECK(code_edit->get_auto_brace_completion_pairs()["["] == "]");
		CHECK(code_edit->get_auto_brace_completion_pairs()["'"] == "'");
		CHECK(code_edit->get_auto_brace_completion_pairs()[";"] == "'");
		CHECK(code_edit->get_auto_brace_completion_pairs()["'''"] == "'''");

		ERR_PRINT_OFF;

		/* No duplicate start keys. */
		code_edit->add_auto_brace_completion_pair("[", "]");
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);

		/* No empty keys. */
		code_edit->add_auto_brace_completion_pair("[", "");
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);

		code_edit->add_auto_brace_completion_pair("", "]");
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);

		code_edit->add_auto_brace_completion_pair("", "");
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);

		/* Must be a symbol. */
		code_edit->add_auto_brace_completion_pair("a", "]");
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);

		code_edit->add_auto_brace_completion_pair("[", "a");
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);

		code_edit->add_auto_brace_completion_pair("a", "a");
		CHECK(code_edit->get_auto_brace_completion_pairs().size() == 4);

		ERR_PRINT_ON;

		/* Check metadata. */
		CHECK(code_edit->has_auto_brace_completion_open_key("["));
		CHECK(code_edit->has_auto_brace_completion_open_key("'"));
		CHECK(code_edit->has_auto_brace_completion_open_key(";"));
		CHECK(code_edit->has_auto_brace_completion_open_key("'''"));
		CHECK_FALSE(code_edit->has_auto_brace_completion_open_key("("));

		CHECK(code_edit->has_auto_brace_completion_close_key("]"));
		CHECK(code_edit->has_auto_brace_completion_close_key("'"));
		CHECK(code_edit->has_auto_brace_completion_close_key("'''"));
		CHECK_FALSE(code_edit->has_auto_brace_completion_close_key(")"));

		CHECK(code_edit->get_auto_brace_completion_close_key("[") == "]");
		CHECK(code_edit->get_auto_brace_completion_close_key("'") == "'");
		CHECK(code_edit->get_auto_brace_completion_close_key(";") == "'");
		CHECK(code_edit->get_auto_brace_completion_close_key("'''") == "'''");
		CHECK(code_edit->get_auto_brace_completion_close_key("(").is_empty());

		/* Check typing inserts closing pair. */
		code_edit->clear();
		SEND_GUI_KEY_EVENT(Key::BRACKETLEFT);
		CHECK(code_edit->get_line(0) == "[]");

		/* Should first match and insert smaller key. */
		code_edit->clear();
		SEND_GUI_KEY_EVENT(Key::APOSTROPHE);
		CHECK(code_edit->get_line(0) == "''");
		CHECK(code_edit->get_caret_column() == 1);

		/* Move out from center, Should match and insert larger key. */
		SEND_GUI_ACTION("ui_text_caret_right");
		SEND_GUI_KEY_EVENT(Key::APOSTROPHE);
		CHECK(code_edit->get_line(0) == "''''''");
		CHECK(code_edit->get_caret_column() == 3);

		/* Backspace should remove all. */
		SEND_GUI_ACTION("ui_text_backspace");
		CHECK(code_edit->get_line(0).is_empty());

		/* If in between and typing close key should "skip". */
		SEND_GUI_KEY_EVENT(Key::BRACKETLEFT);
		CHECK(code_edit->get_line(0) == "[]");
		CHECK(code_edit->get_caret_column() == 1);
		SEND_GUI_KEY_EVENT(Key::BRACKETRIGHT);
		CHECK(code_edit->get_line(0) == "[]");
		CHECK(code_edit->get_caret_column() == 2);

		/* If current is char and inserting a string, do not autocomplete. */
		code_edit->clear();
		SEND_GUI_KEY_EVENT(Key::A);
		SEND_GUI_KEY_EVENT(Key::APOSTROPHE);
		CHECK(code_edit->get_line(0) == "A'");

		/* If in comment, do not complete. */
		code_edit->add_comment_delimiter("#", "");
		code_edit->clear();
		SEND_GUI_KEY_EVENT(Key::NUMBERSIGN);
		SEND_GUI_KEY_EVENT(Key::APOSTROPHE);
		CHECK(code_edit->get_line(0) == "#'");

		/* If in string, and inserting string do not complete. */
		code_edit->clear();
		SEND_GUI_KEY_EVENT(Key::APOSTROPHE);
		SEND_GUI_KEY_EVENT(Key::QUOTEDBL);
		CHECK(code_edit->get_line(0) == "'\"'");

		/* Wrap single line selection with brackets */
		code_edit->clear();
		code_edit->insert_text_at_caret("abc");
		code_edit->select_all();
		SEND_GUI_KEY_EVENT(Key::BRACKETLEFT);
		CHECK(code_edit->get_line(0) == "[abc]");

		/* Caret should be after the last character of the single line selection */
		CHECK(code_edit->get_caret_column() == 4);

		/* Wrap multi line selection with brackets */
		code_edit->clear();
		code_edit->insert_text_at_caret("abc\nabc");
		code_edit->select_all();
		SEND_GUI_KEY_EVENT(Key::BRACKETLEFT);
		CHECK(code_edit->get_text() == "[abc\nabc]");

		/* Caret should be after the last character of the multi line selection */
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 3);

		/* If inserted character is not a auto brace completion open key, replace selected text with the inserted character */
		code_edit->clear();
		code_edit->insert_text_at_caret("abc");
		code_edit->select_all();
		SEND_GUI_KEY_EVENT(Key::KEY_1);
		CHECK(code_edit->get_text() == "1");

		/* If potential multichar and single brace completion is matched, it should wrap the single.  */
		code_edit->clear();
		code_edit->insert_text_at_caret("\'\'abc");
		code_edit->select(0, 2, 0, 5);
		SEND_GUI_KEY_EVENT(Key::APOSTROPHE);
		CHECK(code_edit->get_text() == "\'\'\'abc\'");

		/* If only the potential multichar brace completion is matched, it does not wrap or complete. */
		auto_brace_completion_pairs.erase("\'");
		code_edit->set_auto_brace_completion_pairs(auto_brace_completion_pairs);
		CHECK_FALSE(code_edit->has_auto_brace_completion_open_key("\'"));

		code_edit->clear();
		code_edit->insert_text_at_caret("\'\'abc");
		code_edit->select(0, 2, 0, 5);
		SEND_GUI_KEY_EVENT(Key::APOSTROPHE);
		CHECK(code_edit->get_text() == "\'\'\'");
	}

	SUBCASE("[CodeEdit] autocomplete with brace completion") {
		code_edit->set_auto_brace_completion_enabled(true);
		CHECK(code_edit->is_auto_brace_completion_enabled());

		code_edit->insert_text_at_caret("(te)");
		code_edit->set_caret_column(3);

		// Full completion.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_FUNCTION, "test()", "test()");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "(test())");
		CHECK(code_edit->get_caret_column() == 7);
		code_edit->undo();

		// With "arg".
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_FUNCTION, "test(", "test(");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "(test())");
		CHECK(code_edit->get_caret_column() == 6);
		code_edit->undo();

		// brace completion disabled
		code_edit->set_auto_brace_completion_enabled(false);

		// Full completion.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_FUNCTION, "test()", "test()");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "(test())");
		CHECK(code_edit->get_caret_column() == 7);
		code_edit->undo();

		// With "arg".
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_FUNCTION, "test(", "test(");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "(test()");
		CHECK(code_edit->get_caret_column() == 6);

		// String
		code_edit->set_auto_brace_completion_enabled(true);
		code_edit->clear();
		code_edit->insert_text_at_caret("\"\"");
		code_edit->set_caret_column(1);

		// Full completion.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_NODE_PATH, "\"test\"", "\"test\"");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "\"test\"");
		CHECK(code_edit->get_caret_column() == 6);
		code_edit->undo();

		// With "arg".
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_NODE_PATH, "\"test", "\"test");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "\"\"test\"");
		CHECK(code_edit->get_caret_column() == 7);
		code_edit->undo();

		// brace completion disabled
		code_edit->set_auto_brace_completion_enabled(false);

		// Full completion.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_NODE_PATH, "\"test\"", "\"test\"");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "\"test\"");
		CHECK(code_edit->get_caret_column() == 6);
		code_edit->undo();

		// With "arg".
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_NODE_PATH, "\"test", "\"test");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "\"\"test\"");
		CHECK(code_edit->get_caret_column() == 7);
		code_edit->undo();
	}

	SUBCASE("[CodeEdit] autocomplete") {
		code_edit->set_code_completion_enabled(true);
		CHECK(code_edit->is_code_completion_enabled());

		/* Set prefixes, single char only, disallow empty. */
		Vector<String> completion_prefixes = { "", ".", ".", ",," };

		ERR_PRINT_OFF;
		code_edit->set_code_completion_prefixes(completion_prefixes);
		ERR_PRINT_ON;
		completion_prefixes = code_edit->get_code_completion_prefixes();
		CHECK(completion_prefixes.size() == 2);
		CHECK(completion_prefixes.has("."));
		CHECK(completion_prefixes.has(","));

		code_edit->set_text("test\ntest");
		CHECK(code_edit->get_text_for_code_completion() == String::chr(0xFFFF) + "test\ntest");
	}

	SUBCASE("[CodeEdit] autocomplete request") {
		SIGNAL_WATCH(code_edit, "code_completion_requested");
		code_edit->set_code_completion_enabled(true);

		Array signal_args = { {} };

		/* Force request. */
		code_edit->request_code_completion();
		SIGNAL_CHECK_FALSE("code_completion_requested");
		code_edit->request_code_completion(true);
		SIGNAL_CHECK("code_completion_requested", signal_args);

		/* Manual request should force. */
		SEND_GUI_ACTION("ui_text_completion_query");
		SIGNAL_CHECK("code_completion_requested", signal_args);

		/* Insert prefix. */
		Vector<String> completion_prefixes = { "." };
		code_edit->set_code_completion_prefixes(completion_prefixes);

		code_edit->insert_text_at_caret(".");
		code_edit->request_code_completion();
		SIGNAL_CHECK("code_completion_requested", signal_args);

		/* Should work with space too. */
		code_edit->insert_text_at_caret(" ");
		code_edit->request_code_completion();
		SIGNAL_CHECK("code_completion_requested", signal_args);

		/* Should work when complete ends with prefix. */
		code_edit->clear();
		code_edit->insert_text_at_caret("t");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "test.", "test.");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "test.");
		SIGNAL_CHECK("code_completion_requested", signal_args);

		SIGNAL_UNWATCH(code_edit, "code_completion_requested");
	}

	SUBCASE("[CodeEdit] autocomplete completion") {
		if (TS->has_feature(TextServer::FEATURE_FONT_DYNAMIC) && TS->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
			CHECK(code_edit->get_code_completion_selected_index() == -1);
			code_edit->set_code_completion_enabled(true);
			CHECK(code_edit->get_code_completion_selected_index() == -1);

			code_edit->update_code_completion_options();
			code_edit->set_code_completion_selected_index(1);
			CHECK(code_edit->get_code_completion_selected_index() == -1);
			CHECK(code_edit->get_code_completion_option(0).size() == 0);
			CHECK(code_edit->get_code_completion_options().size() == 0);

			/* Adding does not update the list. */
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_0.", "item_0");

			code_edit->set_code_completion_selected_index(1);
			CHECK(code_edit->get_code_completion_selected_index() == -1);
			CHECK(code_edit->get_code_completion_option(0).size() == 0);
			CHECK(code_edit->get_code_completion_options().size() == 0);

			/* After update, pending add should not be counted, */
			/* also does not work on col 0                      */
			int before_text_caret_column = code_edit->get_caret_column();
			code_edit->insert_text_at_caret("i");

			code_edit->update_code_completion_options();
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0", Color(1, 0, 0), Ref<Resource>(), Color(1, 0, 0));
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_1.", "item_1");
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_2.", "item_2");

			ERR_PRINT_OFF;
			code_edit->set_code_completion_selected_index(1);
			ERR_PRINT_ON;
			CHECK(code_edit->get_code_completion_selected_index() == 0);
			CHECK(code_edit->get_code_completion_option(0).size() == 7);
			CHECK(code_edit->get_code_completion_options().size() == 1);

			/* Check cancel closes completion. */
			SEND_GUI_ACTION("ui_cancel");
			CHECK(code_edit->get_code_completion_selected_index() == -1);

			code_edit->update_code_completion_options();
			CHECK(code_edit->get_code_completion_selected_index() == 0);
			code_edit->set_code_completion_selected_index(1);
			CHECK(code_edit->get_code_completion_selected_index() == 1);
			CHECK(code_edit->get_code_completion_option(0).size() == 7);
			CHECK(code_edit->get_code_completion_options().size() == 3);

			/* Check data. */
			Dictionary option = code_edit->get_code_completion_option(0);
			CHECK((int)option["kind"] == (int)TextEdit::CodeCompletionKind::KIND_CLASS);
			CHECK(option["display_text"] == "item_0.");
			CHECK(option["insert_text"] == "item_0");
			CHECK(option["font_color"] == Color(1, 0, 0));
			CHECK(option["icon"] == Ref<Resource>());
			CHECK(option["default_value"] == Color(1, 0, 0));

			/* Set size for mouse input. */
			code_edit->set_size(Size2(100, 100));

			/* Test home and end keys close the completion and move the caret */
			/* => ui_text_caret_line_start */
			code_edit->set_caret_column(before_text_caret_column + 1);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0", Color(1, 0, 0), Ref<Resource>(), Color(1, 0, 0));
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_1.", "item_1");
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_2.", "item_2");

			code_edit->update_code_completion_options();

			SEND_GUI_ACTION("ui_text_caret_line_start");
			code_edit->update_code_completion_options();
			CHECK(code_edit->get_code_completion_selected_index() == -1);
			CHECK(code_edit->get_caret_column() == 0);

			/* => ui_text_caret_line_end */
			code_edit->set_caret_column(before_text_caret_column + 1);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0", Color(1, 0, 0), Ref<Resource>(), Color(1, 0, 0));
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_1.", "item_1");
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_2.", "item_2");

			code_edit->update_code_completion_options();

			SEND_GUI_ACTION("ui_text_caret_line_end");
			code_edit->update_code_completion_options();
			CHECK(code_edit->get_code_completion_selected_index() == -1);
			CHECK(code_edit->get_caret_column() == before_text_caret_column + 1);

			/* Check input. */
			code_edit->set_caret_column(before_text_caret_column + 1);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0", Color(1, 0, 0), Ref<Resource>(), Color(1, 0, 0));
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_1.", "item_1");
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "item_2.", "item_2");

			code_edit->update_code_completion_options();

			SEND_GUI_ACTION("ui_page_down");
			CHECK(code_edit->get_code_completion_selected_index() == 2);

			SEND_GUI_ACTION("ui_page_up");
			CHECK(code_edit->get_code_completion_selected_index() == 0);

			SEND_GUI_ACTION("ui_up");
			CHECK(code_edit->get_code_completion_selected_index() == 2);

			SEND_GUI_ACTION("ui_down");
			CHECK(code_edit->get_code_completion_selected_index() == 0);

			SEND_GUI_KEY_EVENT(Key::T);
			CHECK(code_edit->get_code_completion_selected_index() == 0);

			SEND_GUI_ACTION("ui_left");
			CHECK(code_edit->get_code_completion_selected_index() == 0);

			SEND_GUI_ACTION("ui_right");
			CHECK(code_edit->get_code_completion_selected_index() == 0);

			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(code_edit->get_code_completion_selected_index() == 0);

			Point2 caret_pos = code_edit->get_caret_draw_pos();
			caret_pos.y += code_edit->get_line_height();
			SEND_GUI_MOUSE_BUTTON_EVENT(caret_pos, MouseButton::WHEEL_DOWN, MouseButtonMask::NONE, Key::NONE);
			CHECK(code_edit->get_code_completion_selected_index() == 1);

			SEND_GUI_MOUSE_BUTTON_EVENT(caret_pos, MouseButton::WHEEL_UP, MouseButtonMask::NONE, Key::NONE);
			CHECK(code_edit->get_code_completion_selected_index() == 0);

			/* Single click selects. */
			caret_pos.y += code_edit->get_line_height();
			SEND_GUI_MOUSE_BUTTON_EVENT(caret_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(caret_pos, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK(code_edit->get_code_completion_selected_index() == 1);

			/* Double click inserts. */
			SEND_GUI_DOUBLE_CLICK(caret_pos, Key::NONE);
			CHECK(code_edit->get_code_completion_selected_index() == -1);
			CHECK(code_edit->get_line(0) == "item_1");

			code_edit->set_auto_brace_completion_enabled(false);

			/* Does nothing in readonly. */
			code_edit->undo();
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0");
			code_edit->update_code_completion_options();
			code_edit->set_editable(false);
			code_edit->confirm_code_completion();
			code_edit->set_editable(true);
			CHECK(code_edit->get_line(0) == "i");

			/* Replace */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1 test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0 test");

			/* Replace string. */
			code_edit->clear();
			code_edit->insert_text_at_caret("\"item_1 test\"");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "\"item_0\"");

			/* Normal replace if no end is given. */
			code_edit->clear();
			code_edit->insert_text_at_caret("\"item_1 test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "\"item_0\" test");

			/* Insert at completion. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1 test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_accept");
			CHECK(code_edit->get_line(0) == "item_01 test");

			/* Insert at completion with string should have same output. */
			code_edit->clear();
			code_edit->insert_text_at_caret("\"item_1 test\"");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0.", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_accept");
			CHECK(code_edit->get_line(0) == "\"item_0\"1 test\"");

			/* Merge symbol at end on insert text. */
			/* End on completion entry. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1 test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0(", "item_0(");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0( test");
			CHECK(code_edit->get_caret_column() == 7);

			/* End of text*/
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1( test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0( test");
			CHECK(code_edit->get_caret_column() == 6);

			/* End of both. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1( test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0(", "item_0(");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0( test");
			CHECK(code_edit->get_caret_column() == 7);

			/* Full set. */
			/* End on completion entry. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1 test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0()", "item_0()");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0() test");
			CHECK(code_edit->get_caret_column() == 8);

			/* End of text*/
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1() test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0() test");
			CHECK(code_edit->get_caret_column() == 6);

			/* End of both. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1() test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0()", "item_0()");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0() test");
			CHECK(code_edit->get_caret_column() == 8);

			/* Autobrace completion. */
			code_edit->set_auto_brace_completion_enabled(true);

			/* End on completion entry. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1 test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0(", "item_0(");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0() test");
			CHECK(code_edit->get_caret_column() == 7);

			/* End of text*/
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1( test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0( test");
			CHECK(code_edit->get_caret_column() == 6);

			/* End of both. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1( test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0(", "item_0(");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0( test");
			CHECK(code_edit->get_caret_column() == 7);

			/* Full set. */
			/* End on completion entry. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1 test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0()", "item_0()");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0() test");
			CHECK(code_edit->get_caret_column() == 8);

			/* End of text*/
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1() test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0", "item_0");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0() test");
			CHECK(code_edit->get_caret_column() == 6);

			/* End of both. */
			code_edit->clear();
			code_edit->insert_text_at_caret("item_1() test");
			code_edit->set_caret_column(2);
			code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_CLASS, "item_0()", "item_0()");
			code_edit->update_code_completion_options();
			SEND_GUI_ACTION("ui_text_completion_replace");
			CHECK(code_edit->get_line(0) == "item_0() test");
			CHECK(code_edit->get_caret_column() == 8);
		}
	}

	SUBCASE("[CodeEdit] autocomplete suggestion order") {
		/* Prefer less fragmented suggestion. */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "test", "test");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "tset", "tset");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "test");

		/* Prefer suggestion starting with the string to complete (matching start). */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "test", "test");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "stest", "stest");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "test");

		/* Prefer less fragment over matching start. */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "tset", "tset");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "stest", "stest");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "stest");

		/* Prefer good capitalization. */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "test", "test");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "Test", "Test");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "test");

		/* Prefer matching start over good capitalization. */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "Test", "Test");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "stest_bis", "test_bis");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "Test");

		/* Prefer closer location. */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "test", "test");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "test_bis", "test_bis", Color(1, 1, 1), Ref<Resource>(), Variant::NIL, TextEdit::LOCATION_LOCAL);
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "test_bis");

		/* Prefer good capitalization over location. */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "test", "test");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "Test", "Test", Color(1, 1, 1), Ref<Resource>(), Variant::NIL, TextEdit::LOCATION_LOCAL);
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "test");

		/* Prefer the start of the string to complete being closest to the start of the suggestion (closest to start). */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "stest", "stest");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "sstest", "sstest");
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "stest");

		/* Prefer location over closest to start. */
		code_edit->clear();
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "stest", "stest");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "sstest", "sstest", Color(1, 1, 1), Ref<Resource>(), Variant::NIL, TextEdit::LOCATION_LOCAL);
		code_edit->update_code_completion_options();
		code_edit->confirm_code_completion();
		CHECK(code_edit->get_line(0) == "sstest");
	}

	SUBCASE("[CodeEdit] autocomplete currently selected option") {
		code_edit->set_code_completion_enabled(true);
		REQUIRE(code_edit->is_code_completion_enabled());

		// Initially select item 0.
		code_edit->insert_text_at_caret("te");
		code_edit->set_caret_column(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te1", "te1");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te3", "te3");
		code_edit->update_code_completion_options();
		CHECK_MESSAGE(code_edit->get_code_completion_selected_index() == 0, "Initially selected item should be 0.");

		// After adding later options shouldn't update selection.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te1", "te1");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te3", "te3");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te4", "te4"); // Added te4.
		code_edit->update_code_completion_options();
		CHECK_MESSAGE(code_edit->get_code_completion_selected_index() == 0, "Adding later options shouldn't update selection.");

		code_edit->set_code_completion_selected_index(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te1", "te1");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te3", "te3");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te4", "te4");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te5", "te5"); // Added te5.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te6", "te6"); // Added te6.
		code_edit->update_code_completion_options();
		CHECK_MESSAGE(code_edit->get_code_completion_selected_index() == 2, "Adding later options shouldn't update selection.");

		// Removing elements after selected element shouldn't update selection.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te1", "te1");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te3", "te3");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te4", "te4");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te5", "te5"); // Removed te6.
		code_edit->update_code_completion_options();
		CHECK_MESSAGE(code_edit->get_code_completion_selected_index() == 2, "Removing elements after selected element shouldn't update selection.");

		// Changing elements after selected element shouldn't update selection.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te1", "te1");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te3", "te3");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te4", "te4");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te6", "te6"); // Changed te5->te6.
		code_edit->update_code_completion_options();
		CHECK_MESSAGE(code_edit->get_code_completion_selected_index() == 2, "Changing elements after selected element shouldn't update selection.");

		// Changing elements before selected element should reset selection.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te2", "te2"); // Changed te1->te2.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te3", "te3");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te4", "te4");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te6", "te6");
		code_edit->update_code_completion_options();
		CHECK_MESSAGE(code_edit->get_code_completion_selected_index() == 0, "Changing elements before selected element should reset selection.");

		// Removing elements before selected element should reset selection.
		code_edit->set_code_completion_selected_index(2);
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te3", "te3"); // Removed te2.
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te4", "te4");
		code_edit->add_code_completion_option(TextEdit::CodeCompletionKind::KIND_VARIABLE, "te6", "te6");
		code_edit->update_code_completion_options();
		CHECK_MESSAGE(code_edit->get_code_completion_selected_index() == 0, "Removing elements before selected element should reset selection.");
	}

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] symbol lookup") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	code_edit->set_symbol_lookup_on_click_enabled(true);
	CHECK(code_edit->is_symbol_lookup_on_click_enabled());

	if (TS->has_feature(TextServer::FEATURE_FONT_DYNAMIC) && TS->has_feature(TextServer::FEATURE_SIMPLE_LAYOUT)) {
		/* Set size for mouse input. */
		code_edit->set_size(Size2(100, 100));

		code_edit->set_text("this is some text");

		Point2 caret_pos = code_edit->get_caret_draw_pos();
		caret_pos.x += 60;

		SEND_GUI_MOUSE_MOTION_EVENT(caret_pos, MouseButtonMask::NONE, Key::NONE);
		CHECK(code_edit->get_text_for_symbol_lookup() == "this is s" + String::chr(0xFFFF) + "ome text");

		SIGNAL_WATCH(code_edit, "symbol_validate");

#ifdef MACOS_ENABLED
		SEND_GUI_KEY_EVENT(Key::META);
#else
		SEND_GUI_KEY_EVENT(Key::CTRL);
#endif

		Array signal_args = { { "some" } };
		SIGNAL_CHECK("symbol_validate", signal_args);

		SIGNAL_UNWATCH(code_edit, "symbol_validate");
	}

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] line length guidelines") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	Vector<int> guide_lines;

	code_edit->set_line_length_guidelines(guide_lines);
	CHECK(code_edit->get_line_length_guidelines().size() == 0);

	guide_lines.push_back(80);
	guide_lines.push_back(120);

	/* Order should be preserved. */
	code_edit->set_line_length_guidelines(guide_lines);
	CHECK((int)code_edit->get_line_length_guidelines()[0] == 80);
	CHECK((int)code_edit->get_line_length_guidelines()[1] == 120);

	memdelete(code_edit);
}

TEST_CASE("[SceneTree][CodeEdit] text manipulation") {
	TextEdit *code_edit = memnew(TextEdit(true));
	SceneTree::get_root()->add_child(code_edit);
	code_edit->grab_focus();

	SUBCASE("[SceneTree][CodeEdit] backspace") {
		// Backspace with selection on first line.
		code_edit->set_text("test backspace");
		code_edit->select(0, 0, 0, 5);
		code_edit->backspace();
		CHECK(code_edit->get_line(0) == "backspace");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Backspace with selection on first line and caret at the beginning of file.
		code_edit->set_text("test backspace");
		code_edit->select(0, 5, 0, 0);
		code_edit->backspace();
		CHECK(code_edit->get_line(0) == "backspace");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Move caret up to the previous line on backspace if caret is at the first column.
		code_edit->set_text("line 1\nline 2");
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(0);
		code_edit->backspace();
		CHECK(code_edit->get_line(0) == "line 1line 2");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 6);

		// Multiple carets with a caret at the first column.
		code_edit->set_text("line 1\nline 2");
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(2);
		code_edit->add_caret(1, 0);
		code_edit->add_caret(1, 5);
		code_edit->backspace();
		CHECK(code_edit->get_text() == "line 1lne2");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 7);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 6);
		CHECK(code_edit->get_caret_line(2) == 0);
		CHECK(code_edit->get_caret_column(2) == 9);
		code_edit->remove_secondary_carets();

		// Multiple carets close together.
		code_edit->set_text("line 1\nline 2");
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(2);
		code_edit->add_caret(1, 1);
		code_edit->backspace();
		CHECK(code_edit->get_text() == "line 1\nne 2");
		CHECK(code_edit->get_caret_count() == 1);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Backspace delete all text if all text is selected.
		code_edit->set_text("line 1\nline 2\nline 3");
		code_edit->select_all();
		code_edit->backspace();
		CHECK(code_edit->get_text().is_empty());
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Backspace at the beginning without selection has no effect.
		code_edit->set_text("line 1\nline 2\nline 3");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(0);
		code_edit->backspace();
		CHECK(code_edit->get_text() == "line 1\nline 2\nline 3");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Unfold previous folded line on backspace if the caret is at the first column.
		code_edit->set_line_folding_enabled(true);
		code_edit->set_text("line 1\n\tline 2\nline 3");
		code_edit->set_caret_line(2);
		code_edit->set_caret_column(0);
		code_edit->fold_line(0);
		code_edit->backspace();
		CHECK_FALSE(code_edit->is_line_folded(0));
		code_edit->set_line_folding_enabled(false);

		// Do not unfold previous line on backspace if the caret is not at the first column.
		code_edit->set_line_folding_enabled(true);
		code_edit->set_text("line 1\n\tline 2\nline 3");
		code_edit->set_caret_line(2);
		code_edit->set_caret_column(4);
		code_edit->fold_line(0);
		code_edit->backspace();
		CHECK(code_edit->is_line_folded(0));
		code_edit->set_line_folding_enabled(false);
	}

	SUBCASE("[TextEdit] cut") {
		DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();
		code_edit->set_line_folding_enabled(true);

		// Cut without a selection removes the entire line.
		code_edit->set_text("this is\nsome\n");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(6);

		code_edit->cut();
		CHECK(DS->clipboard_get() == "this is\n");
		CHECK(code_edit->get_text() == "some\n");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 3); // In the default font, this is the same position.

		// Undo restores the cut text.
		code_edit->undo();
		CHECK(DS->clipboard_get() == "this is\n");
		CHECK(code_edit->get_text() == "this is\nsome\n");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 6);

		// Redo.
		code_edit->redo();
		CHECK(DS->clipboard_get() == "this is\n");
		CHECK(code_edit->get_text() == "some\n");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 3);

		// Cut unfolds the line.
		code_edit->set_text("this is\n\tsome\n");
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));

		code_edit->cut();
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK(DS->clipboard_get() == "this is\n");
		CHECK(code_edit->get_text() == "\tsome\n");
		CHECK(code_edit->get_caret_line() == 0);

		// Cut with a selection removes just the selection.
		code_edit->set_text("this is\nsome\n");
		code_edit->select(0, 5, 0, 7);

		SEND_GUI_ACTION("ui_cut");
		CHECK(code_edit->get_viewport()->is_input_handled());
		CHECK(DS->clipboard_get() == "is");
		CHECK(code_edit->get_text() == "this \nsome\n");
		CHECK_FALSE(code_edit->get_caret_line());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 5);

		// Cut does not change the text if not editable. Text is still added to clipboard.
		code_edit->set_text("this is\nsome\n");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(5);

		code_edit->set_editable(false);
		code_edit->cut();
		code_edit->set_editable(true);
		CHECK(DS->clipboard_get() == "this is\n");
		CHECK(code_edit->get_text() == "this is\nsome\n");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 5);

		// Cut line with multiple carets.
		code_edit->set_text("this is\nsome\n");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(3);
		code_edit->add_caret(0, 2);
		code_edit->add_caret(0, 4);
		code_edit->add_caret(2, 0);

		code_edit->cut();
		CHECK(DS->clipboard_get() == "this is\n\n");
		CHECK(code_edit->get_text() == "some");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK_FALSE(code_edit->has_selection(0));
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 2); // In the default font, this is the same position.
		// The previous caret at index 1 was merged.
		CHECK_FALSE(code_edit->has_selection(1));
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 3); // In the default font, this is the same position.
		CHECK_FALSE(code_edit->has_selection(2));
		CHECK(code_edit->get_caret_line(2) == 0);
		CHECK(code_edit->get_caret_column(2) == 4);
		code_edit->remove_secondary_carets();

		// Cut on the only line removes the contents.
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(2);

		code_edit->cut();
		CHECK(DS->clipboard_get() == "some\n");
		CHECK(code_edit->get_text() == "");
		CHECK(code_edit->get_line_count() == 1);
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Cut empty line.
		code_edit->cut();
		CHECK(DS->clipboard_get() == "\n");
		CHECK(code_edit->get_text() == "");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Cut multiple lines, in order.
		code_edit->set_text("this is\nsome\ntext to\nbe\n\ncut");
		code_edit->set_caret_line(2);
		code_edit->set_caret_column(7);
		code_edit->add_caret(3, 0);
		code_edit->add_caret(0, 2);

		code_edit->cut();
		CHECK(DS->clipboard_get() == "this is\ntext to\nbe\n");
		CHECK(code_edit->get_text() == "some\n\ncut");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 1);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 2);
		code_edit->remove_secondary_carets();

		// Cut multiple selections, in order. Ignores regular carets.
		code_edit->set_text("this is\nsome\ntext to\nbe\n\ncut");
		code_edit->add_caret(3, 0);
		code_edit->add_caret(0, 2);
		code_edit->add_caret(2, 0);
		code_edit->select(1, 0, 1, 2, 0);
		code_edit->select(3, 0, 4, 0, 1);
		code_edit->select(0, 5, 0, 3, 2);

		code_edit->cut();
		CHECK(DS->clipboard_get() == "s \nso\nbe\n");
		CHECK(code_edit->get_text() == "thiis\nme\ntext to\n\ncut");
		CHECK(code_edit->get_caret_count() == 4);
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line(0) == 1);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 3);
		CHECK(code_edit->get_caret_column(1) == 0);
		CHECK(code_edit->get_caret_line(2) == 0);
		CHECK(code_edit->get_caret_column(2) == 3);
		CHECK(code_edit->get_caret_line(3) == 2);
		CHECK(code_edit->get_caret_column(3) == 0);
	}

	SUBCASE("[SceneTree][CodeEdit] cut when empty selection clipboard disabled") {
		DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();
		code_edit->set_empty_selection_clipboard_enabled(false);
		DS->clipboard_set("");

		code_edit->set_text("this is\nsome\n");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(6);
		SIGNAL_DISCARD("text_set");
		SIGNAL_DISCARD("text_changed");
		SIGNAL_DISCARD("lines_edited_from");
		SIGNAL_DISCARD("caret_changed");

		code_edit->cut();
		CHECK(DS->clipboard_get() == "");
		CHECK(code_edit->get_text() == "this is\nsome\n");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 6);
		SIGNAL_CHECK_FALSE("caret_changed");
		SIGNAL_CHECK_FALSE("text_changed");
		SIGNAL_CHECK_FALSE("lines_edited_from");
	}

	SUBCASE("[SceneTree][CodeEdit] new line") {
		// Add a new line.
		code_edit->set_text("test new line");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(13);
		SEND_GUI_ACTION("ui_text_newline");
		CHECK(code_edit->get_line(0) == "test new line");
		CHECK(code_edit->get_line(1) == "");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Split line with new line.
		code_edit->set_text("test new line");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(5);
		SEND_GUI_ACTION("ui_text_newline");
		CHECK(code_edit->get_line(0) == "test ");
		CHECK(code_edit->get_line(1) == "new line");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Delete selection and split with new line.
		code_edit->set_text("test new line");
		code_edit->select(0, 0, 0, 5);
		SEND_GUI_ACTION("ui_text_newline");
		CHECK(code_edit->get_line(0) == "");
		CHECK(code_edit->get_line(1) == "new line");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Blank new line below with selection should not split.
		code_edit->set_text("test new line");
		code_edit->select(0, 0, 0, 5);
		SEND_GUI_ACTION("ui_text_newline_blank");
		CHECK(code_edit->get_line(0) == "test new line");
		CHECK(code_edit->get_line(1) == "");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Blank new line above with selection should not split.
		code_edit->set_text("test new line");
		code_edit->select(0, 0, 0, 5);
		SEND_GUI_ACTION("ui_text_newline_above");
		CHECK(code_edit->get_line(0) == "");
		CHECK(code_edit->get_line(1) == "test new line");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Multiple new lines with multiple carets.
		code_edit->set_text("test new line");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(5);
		code_edit->add_caret(0, 8);
		SEND_GUI_ACTION("ui_text_newline");
		CHECK(code_edit->get_line(0) == "test ");
		CHECK(code_edit->get_line(1) == "new");
		CHECK(code_edit->get_line(2) == " line");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 1);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 2);
		CHECK(code_edit->get_caret_column(1) == 0);

		// Multiple blank new lines with multiple carets.
		code_edit->set_text("test new line");
		code_edit->remove_secondary_carets();
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(5);
		code_edit->add_caret(0, 8);
		SEND_GUI_ACTION("ui_text_newline_blank");
		CHECK(code_edit->get_line(0) == "test new line");
		CHECK(code_edit->get_line(1) == "");
		CHECK(code_edit->get_line(2) == "");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 2);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 0);

		// Multiple new lines above with multiple carets.
		code_edit->set_text("test new line");
		code_edit->remove_secondary_carets();
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(5);
		code_edit->add_caret(0, 8);
		SEND_GUI_ACTION("ui_text_newline_above");
		CHECK(code_edit->get_line(0) == "");
		CHECK(code_edit->get_line(1) == "");
		CHECK(code_edit->get_line(2) == "test new line");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 0);

		// See '[CodeEdit] auto indent' tests for tests about new line with indentation.
	}

	SUBCASE("[SceneTree][CodeEdit] move lines up") {
		code_edit->set_text("test\nlines\nto\n\nmove\naround");

		// Move line up with caret on it.
		code_edit->set_caret_line(2);
		code_edit->set_caret_column(1);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nto\nlines\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 1);

		// Undo.
		code_edit->undo();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 1);

		// Redo.
		code_edit->redo();
		CHECK(code_edit->get_text() == "test\nto\nlines\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 1);

		// Does nothing at the first line.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(1);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 1);

		// Does nothing at the first line when selection ends at column 0.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(0, 0, 1, 0);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\nmove\naround");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Works on empty line.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->set_caret_line(3);
		code_edit->set_caret_column(0);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nlines\n\nto\nmove\naround");
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 0);

		// Move multiple lines up with selection.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(4, 0, 5, 1);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nlines\nto\nmove\naround\n");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 3);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 4);
		CHECK(code_edit->get_caret_column() == 1);

		// Does not affect line with selection end at column 0.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(4, 0, 5, 0);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nlines\nto\nmove\n\naround");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 3);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 4);
		CHECK(code_edit->get_caret_column() == 0);

		// Move multiple lines up with selection, right to left selection.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(5, 2, 4, 1);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nlines\nto\nmove\naround\n");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 4);
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_line() == 3);
		CHECK(code_edit->get_caret_column() == 1);

		// Move multiple lines with multiple carets. A line with multiple carets is only moved once.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(5, 2, 5, 4);
		code_edit->add_caret(4, 0);
		code_edit->add_caret(4, 4);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "test\nlines\nto\nmove\naround\n");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 4);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 4);
		CHECK(code_edit->get_caret_column(0) == 4);
		CHECK_FALSE(code_edit->has_selection(1));
		CHECK(code_edit->get_caret_line(1) == 3);
		CHECK(code_edit->get_caret_column(1) == 0);
		CHECK_FALSE(code_edit->has_selection(2));
		CHECK(code_edit->get_caret_line(2) == 3);
		CHECK(code_edit->get_caret_column(2) == 4);
		code_edit->remove_secondary_carets();

		// Move multiple separate lines with multiple selections.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(2, 2, 1, 4);
		code_edit->add_caret(5, 0);
		code_edit->select(5, 0, 5, 1, 1);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "lines\nto\ntest\n\naround\nmove");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 1);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 4);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 4);
		CHECK(code_edit->get_selection_origin_column(1) == 0);
		CHECK(code_edit->get_caret_line(1) == 4);
		CHECK(code_edit->get_caret_column(1) == 1);
		code_edit->remove_secondary_carets();

		// Move lines with adjacent selections that end at column 0.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(1, 2, 2, 0);
		code_edit->add_caret(2, 2);
		code_edit->select(2, 2, 3, 0, 1);
		code_edit->move_lines_up();
		CHECK(code_edit->get_text() == "lines\nto\ntest\n\nmove\naround");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 0);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 1);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 1);
		CHECK(code_edit->get_selection_origin_column(1) == 2);
		CHECK(code_edit->get_caret_line(1) == 2);
		CHECK(code_edit->get_caret_column(1) == 0);
		code_edit->remove_secondary_carets();
		code_edit->deselect();

		code_edit->set_line_folding_enabled(true);

		// Move line up into a folded region unfolds it.
		code_edit->set_text("test\n\tline1 test\n\t\tline 2\ntest2");
		code_edit->set_caret_line(3);
		code_edit->set_caret_column(0);
		code_edit->fold_line(0);
		CHECK(code_edit->is_line_folded(0));
		code_edit->move_lines_up();
		CHECK(code_edit->get_caret_count() == 1);
		CHECK_FALSE(code_edit->has_selection(0));
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 0);
		CHECK(code_edit->get_text() == "test\n\tline1 test\ntest2\n\t\tline 2");
		CHECK_FALSE(code_edit->is_line_folded(0));
	}

	SUBCASE("[SceneTree][CodeEdit] move lines down") {
		code_edit->set_text("test\nlines\nto\n\nmove\naround");

		// Move line down with caret on it.
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\nto\nlines\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 1);

		// Undo.
		code_edit->undo();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 1);

		// Redo.
		code_edit->redo();
		CHECK(code_edit->get_text() == "test\nto\nlines\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 1);

		// Does nothing at the last line.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->set_caret_line(5);
		code_edit->set_caret_column(1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\nmove\naround");
		CHECK(code_edit->get_caret_line() == 5);
		CHECK(code_edit->get_caret_column() == 1);

		// Does nothing at the last line when selection ends at column 0.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(4, 0, 5, 0);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\nmove\naround");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 4);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 5);
		CHECK(code_edit->get_caret_column() == 0);

		// Works on empty line.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->set_caret_line(3);
		code_edit->set_caret_column(0);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\nlines\nto\nmove\n\naround");
		CHECK(code_edit->get_caret_line() == 4);
		CHECK(code_edit->get_caret_column() == 0);

		// Move multiple lines down with selection.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(1, 0, 2, 1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\n\nlines\nto\nmove\naround");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 2);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 3);
		CHECK(code_edit->get_caret_column() == 1);

		// Does not affect line with selection end at column 0.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(1, 0, 2, 0);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\nto\nlines\n\nmove\naround");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 2);
		CHECK(code_edit->get_selection_origin_column() == 0);
		CHECK(code_edit->get_caret_line() == 3);
		CHECK(code_edit->get_caret_column() == 0);

		// Move multiple lines down with selection, right to left selection.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(2, 2, 1, 1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\n\nlines\nto\nmove\naround");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 3);
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 1);

		// Move multiple lines with multiple carets. A line with multiple carets is only moved once.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(1, 2, 1, 4);
		code_edit->add_caret(0, 0);
		code_edit->add_caret(0, 1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "to\ntest\nlines\n\nmove\naround");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 2);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 2);
		CHECK(code_edit->get_caret_column(0) == 4);
		CHECK_FALSE(code_edit->has_selection(1));
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 0);
		CHECK_FALSE(code_edit->has_selection(2));
		CHECK(code_edit->get_caret_line(2) == 1);
		CHECK(code_edit->get_caret_column(2) == 1);

		// Move multiple separate lines with multiple selections.
		code_edit->remove_secondary_carets();
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(0, 2, 1, 4);
		code_edit->add_caret(4, 0);
		code_edit->select(4, 0, 4, 2, 1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "to\ntest\nlines\n\naround\nmove");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 1);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 2);
		CHECK(code_edit->get_caret_column(0) == 4);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 5);
		CHECK(code_edit->get_selection_origin_column(1) == 0);
		CHECK(code_edit->get_caret_line(1) == 5);
		CHECK(code_edit->get_caret_column(1) == 2);

		// Move lines with adjacent selections that end at column 0.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(1, 2, 2, 0);
		code_edit->add_caret(2, 2);
		code_edit->select(2, 2, 3, 0, 1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "test\n\nlines\nto\nmove\naround");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 2);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 3);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 3);
		CHECK(code_edit->get_selection_origin_column(1) == 2);
		CHECK(code_edit->get_caret_line(1) == 4);
		CHECK(code_edit->get_caret_column(1) == 0);
		code_edit->remove_secondary_carets();

		// Move lines with disconnected adjacent selections that end at column 0.
		code_edit->set_text("test\nlines\nto\n\nmove\naround");
		code_edit->select(0, 2, 1, 0);
		code_edit->add_caret(2, 2);
		code_edit->select(2, 0, 3, 0, 1);
		code_edit->move_lines_down();
		CHECK(code_edit->get_text() == "lines\ntest\n\nto\nmove\naround");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 1);
		CHECK(code_edit->get_selection_origin_column(0) == 2);
		CHECK(code_edit->get_caret_line(0) == 2);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 3);
		CHECK(code_edit->get_selection_origin_column(1) == 0);
		CHECK(code_edit->get_caret_line(1) == 4);
		CHECK(code_edit->get_caret_column(1) == 0);
		code_edit->remove_secondary_carets();
		code_edit->deselect();

		code_edit->set_line_folding_enabled(true);

		// Move line down into a folded region unfolds it.
		code_edit->set_text("test\ntest2\n\tline1 test\n\t\tline 2\ntest2");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(0);
		code_edit->fold_line(1);
		CHECK(code_edit->is_line_folded(1));
		code_edit->move_lines_down();
		CHECK(code_edit->get_caret_count() == 1);
		CHECK_FALSE(code_edit->has_selection(0));
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);
		CHECK(code_edit->get_text() == "test2\ntest\n\tline1 test\n\t\tline 2\ntest2");
		CHECK_FALSE(code_edit->is_line_folded(0));
		CHECK_FALSE(code_edit->is_line_folded(1));
	}

	SUBCASE("[SceneTree][CodeEdit] delete lines") {
		code_edit->set_text("test\nlines\nto\n\ndelete");

		// Delete line with caret on it.
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(1);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "test\nto\n\ndelete");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 1);

		// Undo.
		code_edit->undo();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\ndelete");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 1);

		// Redo.
		code_edit->redo();
		CHECK(code_edit->get_text() == "test\nto\n\ndelete");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 1);

		// Delete empty line.
		code_edit->set_caret_line(2);
		code_edit->set_caret_column(0);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "test\nto\ndelete");
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 0);

		// Deletes only one line when there are multiple carets on it. Carets move down and the column gets clamped.
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(0);
		code_edit->add_caret(0, 1);
		code_edit->add_caret(0, 4);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "to\ndelete");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 0);
		CHECK(code_edit->get_caret_column(1) == 1);
		CHECK(code_edit->get_caret_line(2) == 0);
		CHECK(code_edit->get_caret_column(2) == 2);

		// Delete multiple lines with selection.
		code_edit->remove_secondary_carets();
		code_edit->set_text("test\nlines\nto\n\ndelete");
		code_edit->select(0, 1, 2, 1);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "\ndelete");
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Does not affect line with selection end at column 0.
		code_edit->set_text("test\nlines\nto\n\ndelete");
		code_edit->select(0, 1, 1, 0);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "lines\nto\n\ndelete");
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);

		// Delete multiple lines with multiple carets.
		code_edit->set_text("test\nlines\nto\n\ndelete");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(2);
		code_edit->add_caret(1, 0);
		code_edit->add_caret(4, 5);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "to\n");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 0);
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 0);

		// Delete multiple separate lines with multiple selections.
		code_edit->remove_secondary_carets();
		code_edit->set_text("test\nlines\nto\n\ndelete");
		code_edit->add_caret(4, 5);
		code_edit->select(0, 1, 1, 1);
		code_edit->select(5, 5, 4, 0, 1);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "to\n");
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->get_caret_line(0) == 0);
		CHECK(code_edit->get_caret_column(0) == 1);
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 0);

		// Deletes contents when there is only one line.
		code_edit->remove_secondary_carets();
		code_edit->set_text("test");
		code_edit->set_caret_line(0);
		code_edit->set_caret_column(4);
		code_edit->delete_lines();
		CHECK(code_edit->get_text() == "");
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 0);
		CHECK(code_edit->get_caret_column() == 0);
	}

	SUBCASE("[SceneTree][CodeEdit] duplicate selection") {
		code_edit->set_text("test\nlines\nto\n\nduplicate");

		// Duplicate selected text.
		code_edit->select(0, 1, 1, 2);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "test\nliest\nlines\nto\n\nduplicate");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 2);

		// Undo.
		code_edit->undo();
		CHECK(code_edit->get_text() == "test\nlines\nto\n\nduplicate");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 0);
		CHECK(code_edit->get_selection_origin_column() == 1);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 2);

		// Redo.
		code_edit->redo();
		CHECK(code_edit->get_text() == "test\nliest\nlines\nto\n\nduplicate");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 1);
		CHECK(code_edit->get_selection_origin_column() == 2);
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 2);

		// Duplicate selected text, right to left selection.
		code_edit->set_text("test\nlines\nto\n\nduplicate");
		code_edit->select(1, 1, 0, 2);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "test\nlst\nlines\nto\n\nduplicate");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 2);
		CHECK(code_edit->get_selection_origin_column() == 1);
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 1);

		// Duplicate line if there is no selection.
		code_edit->deselect();
		code_edit->set_text("test\nlines\nto\n\nduplicate");
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(2);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "test\nlines\nlines\nto\n\nduplicate");
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line() == 2);
		CHECK(code_edit->get_caret_column() == 2);

		// Duplicate multiple lines.
		code_edit->deselect();
		code_edit->set_text("test\nlines\nto\n\nduplicate");
		code_edit->set_caret_line(1);
		code_edit->set_caret_column(2);
		code_edit->add_caret(5, 0);
		code_edit->add_caret(0, 4);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "test\ntest\nlines\nlines\nto\n\nduplicate\nduplicate");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK_FALSE(code_edit->has_selection());
		CHECK(code_edit->get_caret_line(0) == 3);
		CHECK(code_edit->get_caret_column(0) == 2);
		CHECK(code_edit->get_caret_line(1) == 7);
		CHECK(code_edit->get_caret_column(1) == 0);
		CHECK(code_edit->get_caret_line(2) == 1);
		CHECK(code_edit->get_caret_column(2) == 4);

		// Duplicate multiple separate selections.
		code_edit->remove_secondary_carets();
		code_edit->set_text("test\nlines\nto\n\nduplicate");
		code_edit->add_caret(4, 4);
		code_edit->add_caret(0, 1);
		code_edit->add_caret(0, 4);
		code_edit->select(2, 0, 2, 1, 0);
		code_edit->select(3, 0, 4, 4, 1);
		code_edit->select(0, 1, 0, 0, 2);
		code_edit->select(0, 2, 0, 4, 3);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "ttestst\nlines\ntto\n\ndupl\nduplicate");
		CHECK(code_edit->get_caret_count() == 4);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 2);
		CHECK(code_edit->get_selection_origin_column(0) == 1);
		CHECK(code_edit->get_caret_line(0) == 2);
		CHECK(code_edit->get_caret_column(0) == 2);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 4);
		CHECK(code_edit->get_selection_origin_column(1) == 4);
		CHECK(code_edit->get_caret_line(1) == 5);
		CHECK(code_edit->get_caret_column(1) == 4);
		CHECK(code_edit->has_selection(2));
		CHECK(code_edit->get_selection_origin_line(2) == 0);
		CHECK(code_edit->get_selection_origin_column(2) == 2);
		CHECK(code_edit->get_caret_line(2) == 0);
		CHECK(code_edit->get_caret_column(2) == 1);
		CHECK(code_edit->has_selection(3));
		CHECK(code_edit->get_selection_origin_line(3) == 0);
		CHECK(code_edit->get_selection_origin_column(3) == 5);
		CHECK(code_edit->get_caret_line(3) == 0);
		CHECK(code_edit->get_caret_column(3) == 7);

		// Duplicate adjacent selections.
		code_edit->remove_secondary_carets();
		code_edit->set_text("test\nlines\nto\n\nduplicate");
		code_edit->add_caret(1, 2);
		code_edit->select(1, 0, 1, 1, 0);
		code_edit->select(1, 1, 1, 4, 1);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "test\nllineines\nto\n\nduplicate");
		CHECK(code_edit->get_caret_count() == 2);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 1);
		CHECK(code_edit->get_selection_origin_column(0) == 1);
		CHECK(code_edit->get_caret_line(0) == 1);
		CHECK(code_edit->get_caret_column(0) == 2);
		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 1);
		CHECK(code_edit->get_selection_origin_column(1) == 5);
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 8);

		// Duplicate lines then duplicate selections when there are both selections and non-selections.
		code_edit->remove_secondary_carets();
		code_edit->set_text("test duplicate");
		code_edit->select(0, 14, 0, 13, 0);
		code_edit->add_caret(0, 8);
		code_edit->add_caret(0, 4);
		code_edit->select(0, 2, 0, 4, 2);
		code_edit->duplicate_selection();
		CHECK(code_edit->get_text() == "test duplicate\ntestst duplicatee");
		CHECK(code_edit->get_caret_count() == 3);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 1);
		CHECK(code_edit->get_selection_origin_column(0) == 17);
		CHECK(code_edit->get_caret_line(0) == 1);
		CHECK(code_edit->get_caret_column(0) == 16);
		CHECK_FALSE(code_edit->has_selection(1));
		CHECK(code_edit->get_caret_line(1) == 1);
		CHECK(code_edit->get_caret_column(1) == 10);
		CHECK(code_edit->has_selection(2));
		CHECK(code_edit->get_selection_origin_line(2) == 1);
		CHECK(code_edit->get_selection_origin_column(2) == 4);
		CHECK(code_edit->get_caret_line(2) == 1);
		CHECK(code_edit->get_caret_column(2) == 6);
	}

	SUBCASE("[SceneTree][CodeEdit] duplicate lines") {
		String reset_text = R"(extends Node

func _ready():
	var a := len(OS.get_cmdline_args())
	var b := get_child_count()
	var c := a + b
	for i in range(c):
		print("This is the solution: ", sin(i))
	var pos = get_index() - 1
	print("Make sure this exits: %b" % pos)
)";

		code_edit->set_text(reset_text);

		// Duplicate a single line without selection.
		code_edit->set_caret_line(0);
		code_edit->duplicate_lines();
		CHECK(code_edit->get_line(0) == "extends Node");
		CHECK(code_edit->get_line(1) == "extends Node");
		CHECK(code_edit->get_line(2) == "");
		CHECK(code_edit->get_caret_line() == 1);
		CHECK(code_edit->get_caret_column() == 0);

		// Duplicate multiple lines with selection.
		code_edit->set_text(reset_text);
		code_edit->select(4, 8, 6, 15);
		code_edit->duplicate_lines();
		CHECK(code_edit->get_text() == R"(extends Node

func _ready():
	var a := len(OS.get_cmdline_args())
	var b := get_child_count()
	var c := a + b
	for i in range(c):
	var b := get_child_count()
	var c := a + b
	for i in range(c):
		print("This is the solution: ", sin(i))
	var pos = get_index() - 1
	print("Make sure this exits: %b" % pos)
)");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 7);
		CHECK(code_edit->get_selection_origin_column() == 8);
		CHECK(code_edit->get_caret_line() == 9);
		CHECK(code_edit->get_caret_column() == 15);

		// Duplicate multiple lines with right to left selection.
		code_edit->set_text(reset_text);
		code_edit->select(6, 15, 4, 8);
		code_edit->duplicate_lines();
		CHECK(code_edit->get_text() == R"(extends Node

func _ready():
	var a := len(OS.get_cmdline_args())
	var b := get_child_count()
	var c := a + b
	for i in range(c):
	var b := get_child_count()
	var c := a + b
	for i in range(c):
		print("This is the solution: ", sin(i))
	var pos = get_index() - 1
	print("Make sure this exits: %b" % pos)
)");
		CHECK(code_edit->has_selection());
		CHECK(code_edit->get_selection_origin_line() == 9);
		CHECK(code_edit->get_selection_origin_column() == 15);
		CHECK(code_edit->get_caret_line() == 7);
		CHECK(code_edit->get_caret_column() == 8);

		// Duplicate single lines with multiple carets. Multiple carets on a single line only duplicate once.
		code_edit->remove_secondary_carets();
		code_edit->deselect();
		code_edit->set_text(reset_text);
		code_edit->set_caret_line(3);
		code_edit->set_caret_column(1);
		code_edit->add_caret(5, 1);
		code_edit->add_caret(5, 5);
		code_edit->add_caret(4, 2);
		code_edit->duplicate_lines();
		CHECK(code_edit->get_text() == R"(extends Node

func _ready():
	var a := len(OS.get_cmdline_args())
	var a := len(OS.get_cmdline_args())
	var b := get_child_count()
	var b := get_child_count()
	var c := a + b
	var c := a + b
	for i in range(c):
		print("This is the solution: ", sin(i))
	var pos = get_index() - 1
	print("Make sure this exits: %b" % pos)
)");
		CHECK(code_edit->get_caret_count() == 4);
		CHECK_FALSE(code_edit->has_selection(0));
		CHECK(code_edit->get_caret_line(0) == 4);
		CHECK(code_edit->get_caret_column(0) == 1);
		CHECK_FALSE(code_edit->has_selection(1));
		CHECK(code_edit->get_caret_line(1) == 8);
		CHECK(code_edit->get_caret_column(1) == 1);
		CHECK_FALSE(code_edit->has_selection(2));
		CHECK(code_edit->get_caret_line(2) == 8);
		CHECK(code_edit->get_caret_column(2) == 5);
		CHECK_FALSE(code_edit->has_selection(3));
		CHECK(code_edit->get_caret_line(3) == 6);
		CHECK(code_edit->get_caret_column(3) == 2);

		// Duplicate multiple lines with multiple selections.
		code_edit->remove_secondary_carets();
		code_edit->set_text(reset_text);
		code_edit->add_caret(4, 2);
		code_edit->add_caret(6, 0);
		code_edit->add_caret(7, 8);
		code_edit->select(0, 0, 2, 5, 0);
		code_edit->select(3, 0, 4, 2, 1);
		code_edit->select(7, 1, 6, 0, 2);
		code_edit->select(7, 3, 7, 8, 3);
		code_edit->duplicate_lines();
		CHECK(code_edit->get_text() == R"(extends Node

func _ready():
extends Node

func _ready():
	var a := len(OS.get_cmdline_args())
	var b := get_child_count()
	var a := len(OS.get_cmdline_args())
	var b := get_child_count()
	var c := a + b
	for i in range(c):
		print("This is the solution: ", sin(i))
	for i in range(c):
		print("This is the solution: ", sin(i))
	var pos = get_index() - 1
	print("Make sure this exits: %b" % pos)
)");
		CHECK(code_edit->get_caret_count() == 4);
		CHECK(code_edit->has_selection(0));
		CHECK(code_edit->get_selection_origin_line(0) == 3);
		CHECK(code_edit->get_selection_origin_column(0) == 0);
		CHECK(code_edit->get_caret_line(0) == 5);
		CHECK(code_edit->get_caret_column(0) == 5);

		CHECK(code_edit->has_selection(1));
		CHECK(code_edit->get_selection_origin_line(1) == 8);
		CHECK(code_edit->get_selection_origin_column(1) == 0);
		CHECK(code_edit->get_caret_line(1) == 9);
		CHECK(code_edit->get_caret_column(1) == 2);

		CHECK(code_edit->has_selection(2));
		CHECK(code_edit->get_selection_origin_line(2) == 14);
		CHECK(code_edit->get_selection_origin_column(2) == 1);
		CHECK(code_edit->get_caret_line(2) == 13);
		CHECK(code_edit->get_caret_column(2) == 0);

		CHECK(code_edit->has_selection(3));
		CHECK(code_edit->get_selection_origin_line(3) == 14);
		CHECK(code_edit->get_selection_origin_column(3) == 3);
		CHECK(code_edit->get_caret_line(3) == 14);
		CHECK(code_edit->get_caret_column(3) == 8);
	}

	memdelete(code_edit);
}

} // namespace TestCodeEdit

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_color_picker.cpp ===
/*  test_color_picker.cpp                                                 */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestColorPicker {

TEST_CASE("[SceneTree][ColorPicker]") {
	ColorPicker *cp = memnew(ColorPicker);
	Window *root = SceneTree::get_root();
	root->add_child(cp);

	SUBCASE("[COLOR_PICKER] Mouse movement after Slider release") {
		Point2i pos_left = Point2i(50, 340); // On the left side of the red slider.
		Point2i pos_right = Point2i(200, 340); // On the right side of the red slider.
		SEND_GUI_MOUSE_MOTION_EVENT(pos_left, MouseButtonMask::NONE, Key::NONE);
		SEND_GUI_MOUSE_BUTTON_EVENT(pos_left, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(cp->get_pick_color().r < 0.5);
		SEND_GUI_MOUSE_MOTION_EVENT(pos_right, MouseButtonMask::LEFT, Key::NONE);
		CHECK(cp->get_pick_color().r > 0.5);
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(pos_right, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(pos_left, MouseButtonMask::NONE, Key::NONE);
		CHECK(cp->get_pick_color().r > 0.5); // Issue GH-77773.
	}
}

} // namespace TestColorPicker

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_control.cpp ===
/*  test_control.cpp                                                      */
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



#include "core/os/os.h" // IWYU pragma: keep // Used by `SEND_GUI_ACTION` macro.
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestControl {

TEST_CASE("[SceneTree][Control] Transforms") {
	SUBCASE("[Control][Global Transform] Global Transform should be accessible while not in SceneTree.") { // GH-79453
		Control *test_node = memnew(Control);
		Control *test_child = memnew(Control);
		test_node->add_child(test_child);

		test_node->set_global_position(Point2(1, 1));
		CHECK_EQ(test_node->get_global_position(), Point2(1, 1));
		CHECK_EQ(test_child->get_global_position(), Point2(1, 1));
		test_node->set_global_position(Point2(2, 2));
		CHECK_EQ(test_node->get_global_position(), Point2(2, 2));
		test_node->set_scale(Vector2(4, 4));
		CHECK_EQ(test_node->get_global_transform(), Transform2D(0, Size2(4, 4), 0, Vector2(2, 2)));
		test_node->set_scale(Vector2(1, 1));
		test_node->set_rotation_degrees(90);
		CHECK_EQ(test_node->get_global_transform(), Transform2D(Math::PI / 2, Vector2(2, 2)));
		test_node->set_pivot_offset(Vector2(1, 0));
		CHECK_EQ(test_node->get_global_transform(), Transform2D(Math::PI / 2, Vector2(3, 1)));

		memdelete(test_child);
		memdelete(test_node);
	}
}

TEST_CASE("[SceneTree][Control] Focus") {
	Control *ctrl = memnew(Control);
	SceneTree::get_root()->add_child(ctrl);

	SUBCASE("[SceneTree][Control] Default focus") {
		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Can't grab focus with default focus mode") {
		ERR_PRINT_OFF
		ctrl->grab_focus();
		ERR_PRINT_ON

		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Can grab focus") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();

		CHECK_UNARY(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Can release focus") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());

		ctrl->release_focus();
		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Only one can grab focus at the same time") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());

		Control *other_ctrl = memnew(Control);
		SceneTree::get_root()->add_child(other_ctrl);
		other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		other_ctrl->grab_focus();

		CHECK_UNARY(other_ctrl->has_focus());
		CHECK_UNARY_FALSE(ctrl->has_focus());

		memdelete(other_ctrl);
	}

	SUBCASE("[SceneTree][Control] Hide control will cause the focus to be released") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());

		ctrl->hide();
		CHECK_UNARY_FALSE(ctrl->has_focus());

		ctrl->show();
		CHECK_UNARY_FALSE(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] The parent node is hidden causing the focus to be released") {
		Control *child_ctrl = memnew(Control);
		ctrl->add_child(child_ctrl);

		child_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		child_ctrl->grab_focus();
		CHECK_UNARY(child_ctrl->has_focus());

		ctrl->hide();
		CHECK_UNARY_FALSE(child_ctrl->has_focus());

		ctrl->show();
		CHECK_UNARY_FALSE(child_ctrl->has_focus());

		memdelete(child_ctrl);
	}

	SUBCASE("[SceneTree][Control] Grab focus with focus behavior recursive") {
		CHECK_UNARY_FALSE(ctrl->has_focus());

		// Cannot grab focus if focus behavior is disabled.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_DISABLED);

		ERR_PRINT_OFF
		ctrl->grab_focus();
		ERR_PRINT_ON
		CHECK_UNARY_FALSE(ctrl->has_focus());

		// Cannot grab focus if focus behavior is enabled but focus mode is none.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_NONE);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_ENABLED);

		ERR_PRINT_OFF
		ctrl->grab_focus();
		ERR_PRINT_ON
		CHECK_UNARY_FALSE(ctrl->has_focus());

		// Can grab focus if focus behavior is enabled and focus mode is all.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_ENABLED);

		ctrl->grab_focus();
		CHECK_UNARY(ctrl->has_focus());
	}

	SUBCASE("[SceneTree][Control] Children focus with focus behavior recursive") {
		Control *child_control = memnew(Control);
		ctrl->add_child(child_control);

		// Can grab focus on child if parent focus behavior is inherit.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_INHERITED);
		child_control->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		child_control->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_INHERITED);

		child_control->grab_focus();
		CHECK_UNARY(child_control->has_focus());

		// Cannot grab focus on child if parent focus behavior is disabled.
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		ctrl->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_DISABLED);
		child_control->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		child_control->set_focus_behavior_recursive(Control::FOCUS_BEHAVIOR_INHERITED);

		ERR_PRINT_OFF
		child_control->grab_focus();
		ERR_PRINT_ON
		CHECK_UNARY_FALSE(child_control->has_focus());

		memdelete(child_control);
	}

	memdelete(ctrl);
}

TEST_CASE("[SceneTree][Control] Find next/prev valid focus") {
	Node *intermediate = memnew(Node);
	Control *ctrl = memnew(Control);
	intermediate->add_child(ctrl);
	SceneTree::get_root()->add_child(intermediate);

	SUBCASE("[SceneTree][Control] In FOCUS_CLICK mode") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
		ctrl->grab_focus();
		REQUIRE_UNARY(ctrl->has_focus());

		SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
			SEND_GUI_ACTION("ui_focus_next");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
			SEND_GUI_ACTION("ui_focus_prev");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Has a sibling control and the parent is a window") {
			Control *ctrl1 = memnew(Control);
			Control *ctrl2 = memnew(Control);
			Control *ctrl3 = memnew(Control);
			Window *win = SceneTree::get_root();

			ctrl1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			ctrl2->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			ctrl3->set_focus_mode(Control::FocusMode::FOCUS_ALL);

			ctrl2->add_child(ctrl3);
			win->add_child(ctrl1);
			win->add_child(ctrl2);

			SUBCASE("[SceneTree][Control] Focus Next") {
				ctrl1->grab_focus();
				CHECK_UNARY(ctrl1->has_focus());

				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl2->has_focus());

				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl3->has_focus());

				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Focus Prev") {
				ctrl1->grab_focus();
				CHECK_UNARY(ctrl1->has_focus());

				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl3->has_focus());

				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl2->has_focus());

				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl1->has_focus());
			}

			memdelete(ctrl3);
			memdelete(ctrl1);
			memdelete(ctrl2);
		}

		SUBCASE("[SceneTree][Control] Has a sibling control but the parent node is not a control or window") {
			Control *other_ctrl = memnew(Control);
			intermediate->add_child(other_ctrl);

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_ALL") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_CLICK") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_NONE") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_NONE);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}
			}

			memdelete(other_ctrl);
		}
	}

	SUBCASE("[SceneTree][Control] In FOCUS_ALL mode") {
		ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
		REQUIRE_EQ(ctrl->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

		ctrl->grab_focus();
		REQUIRE_UNARY(ctrl->has_focus());

		SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
			SEND_GUI_ACTION("ui_focus_next");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
			SEND_GUI_ACTION("ui_focus_prev");
			CHECK_UNARY(ctrl->has_focus());
		}

		SUBCASE("[SceneTree][Control] Has a sibling control but the parent node is not a control") {
			Control *other_ctrl = memnew(Control);
			SceneTree::get_root()->add_child(other_ctrl);

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_ALL") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next is hidden") {
						other_ctrl->hide();
						REQUIRE_UNARY_FALSE(other_ctrl->is_visible());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
							CHECK_UNARY_FALSE(other_ctrl->has_focus());
						}
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_CLICK") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY_FALSE(ctrl->has_focus());
						CHECK_UNARY(other_ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Has a sibling control with FOCUS_NONE") {
				other_ctrl->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(other_ctrl->get_focus_mode(), Control::FocusMode::FOCUS_NONE);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl->has_focus());
					CHECK_UNARY_FALSE(other_ctrl->has_focus());
				}

				SUBCASE("[SceneTree][Control] Manually specify focus next") {
					ctrl->set_focus_next(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Manually specify focus prev") {
					ctrl->set_focus_previous(ctrl->get_path_to(other_ctrl));

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
						CHECK_UNARY_FALSE(other_ctrl->has_focus());
					}
				}
			}

			memdelete(other_ctrl);
		}

		SUBCASE("[SceneTree][Control] Simple control tree") {
			Control *ctrl_0 = memnew(Control);
			Control *ctrl_1 = memnew(Control);
			Node2D *node_2d_2 = memnew(Node2D);

			ctrl->add_child(ctrl_0);
			ctrl->add_child(ctrl_1);
			ctrl->add_child(node_2d_2);

			ctrl_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			ctrl_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
			REQUIRE_EQ(ctrl_0->get_focus_mode(), Control::FocusMode::FOCUS_ALL);
			REQUIRE_EQ(ctrl_1->get_focus_mode(), Control::FocusMode::FOCUS_ALL);

			SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY(ctrl_0->has_focus());

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl_1->has_focus());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY(ctrl_1->has_focus());

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl_0->has_focus());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
					}
				}
			}

			SUBCASE("[SceneTree][Control] Skip next hidden control") {
				ctrl_0->hide();
				REQUIRE_UNARY_FALSE(ctrl_0->is_visible());
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip next control with FOCUS_NONE") {
				ctrl_0->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(ctrl_0->get_focus_mode(), Control::FocusMode::FOCUS_NONE);
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip next control with FOCUS_CLICK") {
				ctrl_0->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(ctrl_0->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip next top level control") {
				ctrl_0->set_as_top_level(true);
				REQUIRE_UNARY(ctrl_0->is_set_as_top_level());
				SEND_GUI_ACTION("ui_focus_next");
				CHECK_UNARY_FALSE(ctrl_0->has_focus());
				CHECK_UNARY(ctrl_1->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev hidden control") {
				ctrl_1->hide();
				REQUIRE_UNARY_FALSE(ctrl_1->is_visible());
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev control with FOCUS_NONE") {
				ctrl_1->set_focus_mode(Control::FocusMode::FOCUS_NONE);
				REQUIRE_EQ(ctrl_1->get_focus_mode(), Control::FocusMode::FOCUS_NONE);
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev control with FOCUS_CLICK") {
				ctrl_1->set_focus_mode(Control::FocusMode::FOCUS_CLICK);
				REQUIRE_EQ(ctrl_1->get_focus_mode(), Control::FocusMode::FOCUS_CLICK);
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Skip prev top level control") {
				ctrl_1->set_as_top_level(true);
				REQUIRE_UNARY(ctrl_1->is_set_as_top_level());
				SEND_GUI_ACTION("ui_focus_prev");
				CHECK_UNARY_FALSE(ctrl_1->has_focus());
				CHECK_UNARY(ctrl_0->has_focus());
			}

			SUBCASE("[SceneTree][Control] Add more node controls") {
				Control *ctrl_0_0 = memnew(Control);
				Control *ctrl_0_1 = memnew(Control);
				Control *ctrl_0_2 = memnew(Control);
				ctrl_0->add_child(ctrl_0_0);
				ctrl_0->add_child(ctrl_0_1);
				ctrl_0->add_child(ctrl_0_2);
				ctrl_0_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_0_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_0_2->set_focus_mode(Control::FocusMode::FOCUS_ALL);

				Control *ctrl_1_0 = memnew(Control);
				Control *ctrl_1_1 = memnew(Control);
				Control *ctrl_1_2 = memnew(Control);
				ctrl_1->add_child(ctrl_1_0);
				ctrl_1->add_child(ctrl_1_1);
				ctrl_1->add_child(ctrl_1_2);
				ctrl_1_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_1_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_1_2->set_focus_mode(Control::FocusMode::FOCUS_ALL);

				Control *ctrl_2_0 = memnew(Control);
				Control *ctrl_2_1 = memnew(Control);
				Control *ctrl_2_2 = memnew(Control);
				node_2d_2->add_child(ctrl_2_0);
				node_2d_2->add_child(ctrl_2_1);
				node_2d_2->add_child(ctrl_2_2);
				ctrl_2_0->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_2_1->set_focus_mode(Control::FocusMode::FOCUS_ALL);
				ctrl_2_2->set_focus_mode(Control::FocusMode::FOCUS_ALL);

				SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
					SEND_GUI_ACTION("ui_focus_next");
					CHECK_UNARY(ctrl_0->has_focus());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl_0_0->has_focus());
					}

					SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
						SEND_GUI_ACTION("ui_focus_prev");
						CHECK_UNARY(ctrl->has_focus());
					}
				}

				SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
					SEND_GUI_ACTION("ui_focus_prev");
					CHECK_UNARY(ctrl_1_2->has_focus());
				}

				SUBCASE("[SceneTree][Control] Exist top level tree") {
					ctrl_0->set_as_top_level(true);
					REQUIRE_UNARY(ctrl_0->is_set_as_top_level());

					SUBCASE("[SceneTree][Control] Outside top level tree") {
						ctrl->grab_focus();
						REQUIRE_UNARY(ctrl->has_focus());
						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl_1->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl->has_focus());
							}
						}
					}

					SUBCASE("[SceneTree][Control] Inside top level tree") {
						ctrl_0->grab_focus();
						REQUIRE_UNARY(ctrl_0->has_focus());
						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl_0_0->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl_0->has_focus());
							}
						}
						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl_0_2->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY(ctrl_0->has_focus());
							}
						}
					}

					SUBCASE("[SceneTree][Control] Manually specified focus next") {
						ctrl->set_focus_next(ctrl->get_path_to(ctrl_2_1));
						ctrl_2_1->set_focus_next(ctrl_2_1->get_path_to(ctrl_1_0));
						ctrl_1_0->set_focus_next(ctrl_1_0->get_path_to(ctrl_0));
						ctrl_0->set_focus_next(ctrl_0->get_path_to(ctrl));

						SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
							SEND_GUI_ACTION("ui_focus_next");
							CHECK_UNARY(ctrl_2_1->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY(ctrl_1_0->has_focus());

								SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
									SEND_GUI_ACTION("ui_focus_next");
									CHECK_UNARY(ctrl_0->has_focus());

									SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
										SEND_GUI_ACTION("ui_focus_next");
										CHECK_UNARY(ctrl->has_focus());
									}

									SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
										SEND_GUI_ACTION("ui_focus_prev");
										CHECK_UNARY(ctrl_0_2->has_focus());
									}
								}

								SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
									SEND_GUI_ACTION("ui_focus_prev");
									CHECK_UNARY(ctrl_1->has_focus());
								}
							}

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl_2_1->has_focus());
							}
						}

						SUBCASE("[SceneTree][Control] The parent node is not visible") {
							node_2d_2->hide();
							REQUIRE_UNARY(ctrl_2_1->is_visible());
							REQUIRE_UNARY_FALSE(ctrl_2_1->is_visible_in_tree());
							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY_FALSE(ctrl->has_focus());
								CHECK_UNARY_FALSE(ctrl_2_1->has_focus());
								CHECK_UNARY_FALSE(ctrl_0->has_focus());
								CHECK_UNARY(ctrl_1->has_focus());
							}
						}
					}

					SUBCASE("[SceneTree][Control] Manually specified focus prev") {
						ctrl->set_focus_previous(ctrl->get_path_to(ctrl_0_2));
						ctrl_0_2->set_focus_previous(ctrl_0_2->get_path_to(ctrl_1_1));
						ctrl_1_1->set_focus_previous(ctrl_1_1->get_path_to(ctrl_2_0));
						ctrl_2_0->set_focus_previous(ctrl_2_0->get_path_to(ctrl));

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl_0_2->has_focus());

							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY(ctrl_1_1->has_focus());

								SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
									SEND_GUI_ACTION("ui_focus_prev");
									CHECK_UNARY(ctrl_2_0->has_focus());

									SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
										SEND_GUI_ACTION("ui_focus_prev");
										CHECK_UNARY(ctrl->has_focus());
									}

									SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
										SEND_GUI_ACTION("ui_focus_next");
										CHECK_UNARY(ctrl_2_0->has_focus());
									}
								}

								SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
									SEND_GUI_ACTION("ui_focus_next");
									CHECK_UNARY(ctrl_1_2->has_focus());
								}
							}

							SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
								SEND_GUI_ACTION("ui_focus_next");
								CHECK_UNARY(ctrl_0->has_focus());
							}
						}

						SUBCASE("[SceneTree][Control] The parent node is not visible") {
							ctrl_0->hide();
							REQUIRE_UNARY(ctrl_0_2->is_visible());
							REQUIRE_UNARY_FALSE(ctrl_0_2->is_visible_in_tree());
							SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
								SEND_GUI_ACTION("ui_focus_prev");
								CHECK_UNARY_FALSE(ctrl->has_focus());
								CHECK_UNARY_FALSE(ctrl_0_2->has_focus());
								CHECK_UNARY(ctrl_1_2->has_focus());
							}
						}
					}
				}

				SUBCASE("[SceneTree][Control] Exist hidden control tree") {
					ctrl_0->hide();
					REQUIRE_UNARY_FALSE(ctrl_0->is_visible());

					SUBCASE("[SceneTree][Control] Simulate ui_focus_next action") {
						SEND_GUI_ACTION("ui_focus_next");
						CHECK_UNARY(ctrl_1->has_focus());

						SUBCASE("[SceneTree][Control] Simulate ui_focus_prev action") {
							SEND_GUI_ACTION("ui_focus_prev");
							CHECK_UNARY(ctrl->has_focus());
						}
					}
				}

				memdelete(ctrl_2_2);
				memdelete(ctrl_2_1);
				memdelete(ctrl_2_0);
				memdelete(ctrl_1_2);
				memdelete(ctrl_1_1);
				memdelete(ctrl_1_0);
				memdelete(ctrl_0_2);
				memdelete(ctrl_0_1);
				memdelete(ctrl_0_0);
			}

			memdelete(node_2d_2);
			memdelete(ctrl_1);
			memdelete(ctrl_0);
		}
	}

	memdelete(ctrl);
	memdelete(intermediate);
}

TEST_CASE("[SceneTree][Control] Anchoring") {
	Control *test_control = memnew(Control);
	Control *test_child = memnew(Control);
	test_control->add_child(test_child);
	test_control->set_size(Size2(2, 2));
	Window *root = SceneTree::get_root();
	root->add_child(test_control);

	SUBCASE("Anchoring without offsets") {
		test_child->set_anchor(SIDE_RIGHT, 0.75);
		test_child->set_anchor(SIDE_BOTTOM, 0.1);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(1.5, 0.2)),
				"With no LEFT or TOP anchors, positive RIGHT and BOTTOM anchors should be proportional to the size.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0, 0)),
				"With positive RIGHT and BOTTOM anchors set and no LEFT or TOP anchors, the position should not change.");

		test_child->set_anchor(SIDE_LEFT, 0.5);
		test_child->set_anchor(SIDE_TOP, 0.01);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.5, 0.18)),
				"With all anchors set, the size should fit between all four anchors.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(1, 0.02)),
				"With all anchors set, the LEFT and TOP anchors should proportional to the position.");
	}

	SUBCASE("Anchoring with offsets") {
		test_child->set_offset(SIDE_RIGHT, 0.33);
		test_child->set_offset(SIDE_BOTTOM, 0.2);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.33, 0.2)),
				"With no anchors or LEFT or TOP offsets set, the RIGHT and BOTTOM offsets should be equal to size.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0, 0)),
				"With only positive RIGHT and BOTTOM offsets set, the position should not change.");

		test_child->set_offset(SIDE_LEFT, 0.1);
		test_child->set_offset(SIDE_TOP, 0.05);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.23, 0.15)),
				"With no anchors set, the size should fit between all four offsets.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0.1, 0.05)),
				"With no anchors set, the LEFT and TOP offsets should be equal to the position.");

		test_child->set_anchor(SIDE_RIGHT, 0.5);
		test_child->set_anchor(SIDE_BOTTOM, 0.3);
		test_child->set_anchor(SIDE_LEFT, 0.2);
		test_child->set_anchor(SIDE_TOP, 0.1);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.83, 0.55)),
				"Anchors adjust size first then it is affected by offsets.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0.5, 0.25)),
				"Anchors adjust positions first then it is affected by offsets.");

		test_child->set_offset(SIDE_RIGHT, -0.1);
		test_child->set_offset(SIDE_BOTTOM, -0.01);
		test_child->set_offset(SIDE_LEFT, -0.33);
		test_child->set_offset(SIDE_TOP, -0.16);
		CHECK_MESSAGE(
				test_child->get_size().is_equal_approx(Vector2(0.83, 0.55)),
				"Keeping offset distance equal when changing offsets, keeps size equal.");
		CHECK_MESSAGE(
				test_child->get_position().is_equal_approx(Vector2(0.07, 0.04)),
				"Negative offsets move position in top left direction.");
	}

	SUBCASE("Anchoring is preserved on parent size changed") {
		test_child->set_offset(SIDE_RIGHT, -0.05);
		test_child->set_offset(SIDE_BOTTOM, 0.1);
		test_child->set_offset(SIDE_LEFT, 0.05);
		test_child->set_offset(SIDE_TOP, 0.1);
		test_child->set_anchor(SIDE_RIGHT, 0.3);
		test_child->set_anchor(SIDE_BOTTOM, 0.85);
		test_child->set_anchor(SIDE_LEFT, 0.2);
		test_child->set_anchor(SIDE_TOP, 0.55);
		CHECK(test_child->get_rect().is_equal_approx(
				Rect2(Vector2(0.45, 1.2), Size2(0.1, 0.6))));

		test_control->set_size(Size2(4, 1));
		CHECK(test_child->get_rect().is_equal_approx(
				Rect2(Vector2(0.85, 0.65), Size2(0.3, 0.3))));
	}

	memdelete(test_child);
	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Custom maximum size") {
	Control *test_control = memnew(Control);
	test_control->set_custom_maximum_size(Size2(4, 2));
	Window *root = SceneTree::get_root();
	root->add_child(test_control);
	test_control->set_size(Size2(4, 2));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size is allowed to increase to match custom maximum size.");

	test_control->set_size(Size2(3, 1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(3, 1)),
			"Size does not change if below custom maximum size.");

	test_control->set_size(Size2(5, 4));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size is limited to custom maximum size.");

	test_control->set_size(Size2(5, 1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 1)),
			"Adjust only x axis if x is above custom maximum size.");

	test_control->set_size(Size2(3, 3));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(3, 2)),
			"Adjust only y axis if y is above custom maximum size.");

	test_control->set_custom_minimum_size(Size2(5, 3));
	SceneTree::process(0);
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Custom maximum size overrides custom minimum size.");

	Control *test_child = memnew(Control);
	test_control->add_child(test_child);

	CHECK_MESSAGE(
			test_child->get_combined_maximum_size().is_equal_approx(Vector2(-1, -1)),
			"Child combined maximum size does not factor in parent's custom maximum size if not propagating.");

	test_child->set_size(Size2(5, 3));
	CHECK_MESSAGE(
			test_child->get_size().is_equal_approx(Vector2(5, 3)),
			"Child size is not constrained by parent's custom maximum size if not propagating.");

	test_control->set_propagate_maximum_size(true);

	CHECK_MESSAGE(
			test_child->get_combined_maximum_size().is_equal_approx(Vector2(4, 2)),
			"Child combined maximum size factors in parent's custom maximum size if propagating.");

	test_child->set_size(Size2(5, 3));
	CHECK_MESSAGE(
			test_child->get_size().is_equal_approx(Vector2(4, 2)),
			"Child size is constrained by parent's custom maximum size if propagating.");

	test_child->set_size(Size2(3, 1));
	CHECK_MESSAGE(
			test_child->get_size().is_equal_approx(Vector2(3, 1)),
			"Child size can be smaller than parent's custom maximum size.");

	memdelete(test_child);
	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Custom minimum size") {
	Control *test_control = memnew(Control);
	test_control->set_custom_minimum_size(Size2(4, 2));
	Window *root = SceneTree::get_root();
	root->add_child(test_control);
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size increases to match custom minimum size.");

	test_control->set_size(Size2(5, 4));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(5, 4)),
			"Size does not change if above custom minimum size.");

	test_control->set_size(Size2(1, 1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 2)),
			"Size matches minimum size if set below custom minimum size.");

	test_control->set_size(Size2(3, 3));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(4, 3)),
			"Adjusts only x axis size if x is below custom minimum size.");

	test_control->set_size(Size2(10, 0.1));
	CHECK_MESSAGE(
			test_control->get_size().is_equal_approx(Vector2(10, 2)),
			"Adjusts only y axis size if y is below custom minimum size.");

	memdelete(test_control);
}

TEST_CASE("[SceneTree][Control] Grow direction") {
	Control *test_control = memnew(Control);
	test_control->set_size(Size2(1, 1));
	Window *root = SceneTree::get_root();
	root->add_child(test_control);

	SUBCASE("Defaults") {
		CHECK(test_control->get_h_grow_direction() == Control::GROW_DIRECTION_END);
		CHECK(test_control->get_v_grow_direction() == Control::GROW_DIRECTION_END);
	}

	SIGNAL_WATCH(test_control, SNAME("minimum_size_changed"))
	Array signal_args = { {} };

	SUBCASE("Horizontal grow direction begin") {
		test_control->set_h_grow_direction(Control::GROW_DIRECTION_BEGIN);
		test_control->set_custom_minimum_size(Size2(2, 2));
		SceneTree::process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args)
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(-1, 0), Size2(2, 2))),
				"Expand leftwards.");
	}

	SUBCASE("Vertical grow direction begin") {
		test_control->set_v_grow_direction(Control::GROW_DIRECTION_BEGIN);
		test_control->set_custom_minimum_size(Size2(4, 3));
		SceneTree::process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, -2), Size2(4, 3))),
				"Expand upwards.");
	}

	SUBCASE("Horizontal grow direction end") {
		test_control->set_h_grow_direction(Control::GROW_DIRECTION_END);
		test_control->set_custom_minimum_size(Size2(5, 3));
		SceneTree::process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, 0), Size2(5, 3))),
				"Expand rightwards.");
	}

	SUBCASE("Vertical grow direction end") {
		test_control->set_v_grow_direction(Control::GROW_DIRECTION_END);
		test_control->set_custom_minimum_size(Size2(4, 4));
		SceneTree::process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, 0), Size2(4, 4))),
				"Expand downwards.");
		;
	}

	SUBCASE("Horizontal grow direction both") {
		test_control->set_h_grow_direction(Control::GROW_DIRECTION_BOTH);
		test_control->set_custom_minimum_size(Size2(2, 4));
		SceneTree::process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(-0.5, 0), Size2(2, 4))),
				"Expand equally leftwards and rightwards.");
	}

	SUBCASE("Vertical grow direction both") {
		test_control->set_v_grow_direction(Control::GROW_DIRECTION_BOTH);
		test_control->set_custom_minimum_size(Size2(6, 3));
		SceneTree::process(0);
		SIGNAL_CHECK("minimum_size_changed", signal_args);
		CHECK_MESSAGE(
				test_control->get_rect().is_equal_approx(
						Rect2(Vector2(0, -1), Size2(6, 3))),
				"Expand equally upwards and downwards.");
	}

	memdelete(test_control);
}

} // namespace TestControl

// === Merged from tests/scene/test_flow_container.cpp ===
/*  test_flow_container.cpp                                               */
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
#include "scene/gui/flow_container.h"
#include "scene/main/scene_tree.h"

namespace TestFlowContainer {

TEST_CASE("[SceneTree][FlowContainer] HFlowContainer") {
	HFlowContainer *hflow_container = memnew(HFlowContainer);
	Window *root = SceneTree::get_root();
	root->add_child(hflow_container);

	hflow_container->add_theme_constant_override("h_separation", 0);
	hflow_container->add_theme_constant_override("v_separation", 0);
	hflow_container->set_size(Size2(100, 100));
	SceneTree::process(0);

	SUBCASE("Default behavior") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		Control *child_control_3 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 10));
		child_control_2->set_custom_minimum_size(Size2(20, 10));
		child_control_3->set_custom_minimum_size(Size2(20, 10));
		hflow_container->add_child(child_control_1);
		hflow_container->add_child(child_control_2);
		hflow_container->add_child(child_control_3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				hflow_container->get_alignment() == FlowContainer::ALIGNMENT_BEGIN,
				"HFlowContainer alignment is set to ALIGNMENT_BEGIN by default.");

		CHECK_MESSAGE(
				hflow_container->get_last_wrap_alignment() == FlowContainer::LAST_WRAP_ALIGNMENT_INHERIT,
				"HFlowContainer last wrap alignment is set to LAST_WRAP_ALIGNMENT_INHERIT by default.");

		CHECK_MESSAGE(
				!hflow_container->is_vertical(),
				"HFlowContainer is horizontal.");

		CHECK_MESSAGE(
				!hflow_container->is_reverse_fill(),
				"HFlowContainer reverse fill is disabled by default.");

		CHECK_MESSAGE(
				hflow_container->get_line_count() == 1,
				"HFlowContainer keeps all children in one line when there is enough horizontal space.");

		CHECK_MESSAGE(
				hflow_container->get_line_max_child_count() == 3,
				"HFlowContainer reports the child count of the first line.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First child control is positioned at the beginning of the line.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(20, 0)),
				"Second child control is placed after the first child control.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(40, 0)),
				"Third child control is placed after the second child control.");

		memdelete(child_control_3);
		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Wrapping and last wrap alignment") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		Control *child_control_3 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 10));
		child_control_2->set_custom_minimum_size(Size2(20, 10));
		child_control_3->set_custom_minimum_size(Size2(20, 10));
		hflow_container->set_size(Size2(50, 100));
		hflow_container->add_child(child_control_1);
		hflow_container->add_child(child_control_2);
		hflow_container->add_child(child_control_3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				hflow_container->get_line_count() == 2,
				"HFlowContainer wraps children to a second line when they no longer fit.");

		CHECK_MESSAGE(
				hflow_container->get_line_max_child_count() == 2,
				"HFlowContainer reports the first line child count as the line max child count.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First child control starts at the first line origin.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(20, 0)),
				"Second child control remains on the first line.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(0, 10)),
				"Third child control wraps to the second line.");

		hflow_container->set_last_wrap_alignment(FlowContainer::LAST_WRAP_ALIGNMENT_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(10, 10)),
				"Last wrapped line is centered relative to the previous line when LAST_WRAP_ALIGNMENT_CENTER is used.");

		hflow_container->set_last_wrap_alignment(FlowContainer::LAST_WRAP_ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(20, 10)),
				"Last wrapped line is aligned to the end relative to the previous line when LAST_WRAP_ALIGNMENT_END is used.");

		memdelete(child_control_3);
		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Alignment and RTL") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 10));
		child_control_2->set_custom_minimum_size(Size2(20, 10));
		hflow_container->set_size(Size2(100, 100));
		hflow_container->add_child(child_control_1);
		hflow_container->add_child(child_control_2);
		SceneTree::process(0);

		hflow_container->set_alignment(FlowContainer::ALIGNMENT_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(30, 0)),
				"First child control is centered when ALIGNMENT_CENTER is used.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(50, 0)),
				"Second child control follows centered alignment when ALIGNMENT_CENTER is used.");

		hflow_container->set_alignment(FlowContainer::ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(60, 0)),
				"First child control is aligned to the end when ALIGNMENT_END is used.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(80, 0)),
				"Second child control is aligned to the end when ALIGNMENT_END is used.");

		hflow_container->set_alignment(FlowContainer::ALIGNMENT_BEGIN);
		hflow_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(80, 0)),
				"First child control starts at the right edge in RTL mode with ALIGNMENT_BEGIN.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(60, 0)),
				"Second child control follows right-to-left ordering in RTL mode with ALIGNMENT_BEGIN.");

		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Expanding children and stretch ratio") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 10));
		child_control_2->set_custom_minimum_size(Size2(20, 10));
		child_control_1->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		child_control_2->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		hflow_container->set_size(Size2(100, 100));
		hflow_container->add_child(child_control_1);
		hflow_container->add_child(child_control_2);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(50, 10)),
				"First child control expands to half of the available width when both children have equal stretch ratio.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(50, 10)),
				"Second child control expands to half of the available width when both children have equal stretch ratio.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(50, 0)),
				"Second child control is placed immediately after the first expanded child control.");

		child_control_2->set_stretch_ratio(3.0f);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(35, 10)),
				"First child control receives less width when its stretch ratio is lower.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(65, 10)),
				"Second child control receives more width when its stretch ratio is higher.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(35, 0)),
				"Second child control starts after the first child control's stretched width.");

		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Reverse fill") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		Control *child_control_3 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 10));
		child_control_2->set_custom_minimum_size(Size2(20, 10));
		child_control_3->set_custom_minimum_size(Size2(20, 10));
		hflow_container->set_size(Size2(50, 100));
		hflow_container->add_child(child_control_1);
		hflow_container->add_child(child_control_2);
		hflow_container->add_child(child_control_3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First child control starts at the top row by default.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(0, 10)),
				"Third child control wraps to the next row by default.");

		hflow_container->set_reverse_fill(true);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 90)),
				"First child control moves to the bottom row when reverse fill is enabled.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(20, 90)),
				"Second child control remains in the same row as the first child control when reverse fill is enabled.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(0, 80)),
				"Wrapped row order is inverted from bottom to top when reverse fill is enabled.");

		hflow_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(30, 90)),
				"RTL mirrors horizontal placement while reverse fill keeps rows bottom to top.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(10, 90)),
				"Second child control follows mirrored RTL order while reverse fill remains enabled.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(30, 80)),
				"Wrapped child control is mirrored in RTL while keeping reverse-filled row order.");

		memdelete(child_control_3);
		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	memdelete(hflow_container);
}

TEST_CASE("[SceneTree][FlowContainer] VFlowContainer") {
	VFlowContainer *vflow_container = memnew(VFlowContainer);
	Window *root = SceneTree::get_root();
	root->add_child(vflow_container);

	vflow_container->add_theme_constant_override("h_separation", 0);
	vflow_container->add_theme_constant_override("v_separation", 0);
	vflow_container->set_size(Size2(100, 100));
	SceneTree::process(0);

	SUBCASE("Default behavior") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		Control *child_control_3 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 10));
		child_control_2->set_custom_minimum_size(Size2(20, 10));
		child_control_3->set_custom_minimum_size(Size2(20, 10));
		vflow_container->add_child(child_control_1);
		vflow_container->add_child(child_control_2);
		vflow_container->add_child(child_control_3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				vflow_container->get_alignment() == FlowContainer::ALIGNMENT_BEGIN,
				"VFlowContainer alignment is set to ALIGNMENT_BEGIN by default.");

		CHECK_MESSAGE(
				vflow_container->get_last_wrap_alignment() == FlowContainer::LAST_WRAP_ALIGNMENT_INHERIT,
				"VFlowContainer last wrap alignment is set to LAST_WRAP_ALIGNMENT_INHERIT by default.");

		CHECK_MESSAGE(
				vflow_container->is_vertical(),
				"VFlowContainer is vertical.");

		CHECK_MESSAGE(
				!vflow_container->is_reverse_fill(),
				"VFlowContainer reverse fill is disabled by default.");

		CHECK_MESSAGE(
				vflow_container->get_line_count() == 1,
				"VFlowContainer keeps all children in one column when there is enough vertical space.");

		CHECK_MESSAGE(
				vflow_container->get_line_max_child_count() == 3,
				"VFlowContainer reports the child count of the first column.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First child control is positioned at the top of the first column.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 10)),
				"Second child control is placed below the first child control.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(0, 20)),
				"Third child control is placed below the second child control.");

		memdelete(child_control_3);
		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Wrapping and last wrap alignment") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		Control *child_control_3 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 20));
		child_control_2->set_custom_minimum_size(Size2(20, 20));
		child_control_3->set_custom_minimum_size(Size2(20, 20));
		vflow_container->set_size(Size2(100, 50));
		vflow_container->add_child(child_control_1);
		vflow_container->add_child(child_control_2);
		vflow_container->add_child(child_control_3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				vflow_container->get_line_count() == 2,
				"VFlowContainer wraps children to a second column when they no longer fit vertically.");

		CHECK_MESSAGE(
				vflow_container->get_line_max_child_count() == 2,
				"VFlowContainer reports the first column child count as the line max child count.");

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First child control starts at the first column origin.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 20)),
				"Second child control remains in the first column.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(20, 0)),
				"Third child control wraps to the second column.");

		vflow_container->set_last_wrap_alignment(FlowContainer::LAST_WRAP_ALIGNMENT_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(20, 10)),
				"Last wrapped column is centered relative to the previous column when LAST_WRAP_ALIGNMENT_CENTER is used.");

		vflow_container->set_last_wrap_alignment(FlowContainer::LAST_WRAP_ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(20, 20)),
				"Last wrapped column is aligned to the end relative to the previous column when LAST_WRAP_ALIGNMENT_END is used.");

		memdelete(child_control_3);
		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Alignment and RTL") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 20));
		child_control_2->set_custom_minimum_size(Size2(20, 20));
		vflow_container->set_size(Size2(100, 100));
		vflow_container->add_child(child_control_1);
		vflow_container->add_child(child_control_2);
		SceneTree::process(0);

		vflow_container->set_alignment(FlowContainer::ALIGNMENT_CENTER);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 30)),
				"First child control is vertically centered when ALIGNMENT_CENTER is used.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 50)),
				"Second child control follows centered vertical alignment when ALIGNMENT_CENTER is used.");

		vflow_container->set_alignment(FlowContainer::ALIGNMENT_END);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 60)),
				"First child control is aligned to the bottom when ALIGNMENT_END is used.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 80)),
				"Second child control is aligned to the bottom when ALIGNMENT_END is used.");

		vflow_container->set_alignment(FlowContainer::ALIGNMENT_BEGIN);
		vflow_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(80, 0)),
				"First child control moves to the right edge in RTL mode with vertical flow.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(80, 20)),
				"Second child control follows RTL horizontal mirroring in vertical flow.");

		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Expanding children and stretch ratio") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 20));
		child_control_2->set_custom_minimum_size(Size2(20, 20));
		child_control_1->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		child_control_2->set_v_size_flags(Control::SIZE_EXPAND_FILL);
		vflow_container->set_size(Size2(100, 100));
		vflow_container->add_child(child_control_1);
		vflow_container->add_child(child_control_2);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(20, 50)),
				"First child control expands to half of the available height when both children have equal stretch ratio.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(20, 50)),
				"Second child control expands to half of the available height when both children have equal stretch ratio.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 50)),
				"Second child control is placed immediately after the first expanded child control.");

		child_control_2->set_stretch_ratio(3.0f);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_size().is_equal_approx(Size2(20, 35)),
				"First child control receives less height when its stretch ratio is lower.");

		CHECK_MESSAGE(
				child_control_2->get_size().is_equal_approx(Size2(20, 65)),
				"Second child control receives more height when its stretch ratio is higher.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(0, 35)),
				"Second child control starts after the first child control's stretched height.");

		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	SUBCASE("Reverse fill") {
		Control *child_control_1 = memnew(Control);
		Control *child_control_2 = memnew(Control);
		Control *child_control_3 = memnew(Control);
		child_control_1->set_custom_minimum_size(Size2(20, 20));
		child_control_2->set_custom_minimum_size(Size2(20, 20));
		child_control_3->set_custom_minimum_size(Size2(20, 20));
		vflow_container->set_size(Size2(100, 30));
		vflow_container->add_child(child_control_1);
		vflow_container->add_child(child_control_2);
		vflow_container->add_child(child_control_3);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"First child control starts in the first column by default.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(20, 0)),
				"Second child control starts in the second column by default.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(40, 0)),
				"Third child control starts in the third column by default.");

		vflow_container->set_reverse_fill(true);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(80, 0)),
				"First child control starts from the right when reverse fill is enabled in vertical mode.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(60, 0)),
				"Second child control follows right-to-left column filling when reverse fill is enabled.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(40, 0)),
				"Third child control continues right-to-left column filling when reverse fill is enabled.");

		vflow_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
		SceneTree::process(0);

		CHECK_MESSAGE(
				child_control_1->get_position().is_equal_approx(Point2(0, 0)),
				"Vertical flow with RTL and reverse fill fills columns from left to right.");

		CHECK_MESSAGE(
				child_control_2->get_position().is_equal_approx(Point2(20, 0)),
				"Second child control follows left-to-right order in vertical mode with RTL and reverse fill.");

		CHECK_MESSAGE(
				child_control_3->get_position().is_equal_approx(Point2(40, 0)),
				"Third child control follows left-to-right order in vertical mode with RTL and reverse fill.");

		memdelete(child_control_3);
		memdelete(child_control_2);
		memdelete(child_control_1);
	}

	memdelete(vflow_container);
}

} // namespace TestFlowContainer

// === Merged from tests/scene/test_foldable_container.cpp ===
/*  test_foldable_container.cpp                                           */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"

namespace TestFoldableContainer {

TEST_CASE("[SceneTree][FoldableContainer] Default properties") {
	FoldableContainer *foldable_container = memnew(FoldableContainer);
	Window *root = SceneTree::get_root();
	root->add_child(foldable_container);
	SceneTree::process(0);

	CHECK_MESSAGE(
			!foldable_container->is_folded(),
			"FoldableContainer is expanded by default.");

	CHECK_MESSAGE(
			foldable_container->get_title().is_empty(),
			"FoldableContainer title is empty by default.");

	CHECK_MESSAGE(
			foldable_container->get_title_alignment() == HORIZONTAL_ALIGNMENT_LEFT,
			"FoldableContainer title alignment is set to left by default.");

	CHECK_MESSAGE(
			foldable_container->get_title_position() == FoldableContainer::POSITION_TOP,
			"FoldableContainer title position is set to top by default.");

	CHECK_MESSAGE(
			foldable_container->get_title_text_direction() == Control::TEXT_DIRECTION_AUTO,
			"FoldableContainer title text direction is set to auto by default.");

	CHECK_MESSAGE(
			foldable_container->get_title_text_overrun_behavior() == TextServer::OVERRUN_NO_TRIMMING,
			"FoldableContainer title overrun behavior is set to no trimming by default.");

	CHECK_MESSAGE(
			foldable_container->get_language().is_empty(),
			"FoldableContainer language is empty by default.");

	CHECK_MESSAGE(
			foldable_container->get_focus_mode() == Control::FOCUS_ALL,
			"FoldableContainer focus mode is set to FOCUS_ALL by default.");

	CHECK_MESSAGE(
			foldable_container->get_mouse_filter() == Control::MOUSE_FILTER_STOP,
			"FoldableContainer mouse filter is set to MOUSE_FILTER_STOP by default.");

	CHECK_MESSAGE(
			foldable_container->get_foldable_group().is_null(),
			"FoldableContainer has no foldable group by default.");

	memdelete(foldable_container);
}

TEST_CASE("[SceneTree][FoldableContainer] Fold and expand behavior") {
	FoldableContainer *foldable_container = memnew(FoldableContainer("Section"));
	Window *root = SceneTree::get_root();
	root->add_child(foldable_container);

	Control *child_control = memnew(Control);
	child_control->set_custom_minimum_size(Size2(120, 80));
	foldable_container->add_child(child_control);
	foldable_container->set_size(Size2(200, 200));
	SceneTree::process(0);

	Size2 expanded_minimum_size = foldable_container->get_minimum_size();

	CHECK_MESSAGE(
			child_control->is_visible(),
			"Child control is visible when FoldableContainer is expanded.");

	foldable_container->fold();
	SceneTree::process(0);

	CHECK_MESSAGE(
			foldable_container->is_folded(),
			"FoldableContainer is folded after calling fold().");

	CHECK_MESSAGE(
			!child_control->is_visible(),
			"Child control is hidden when FoldableContainer is folded.");

	CHECK_MESSAGE(
			foldable_container->get_minimum_size().y < expanded_minimum_size.y,
			"Folded minimum size is smaller than expanded minimum size when content is present.");

	foldable_container->expand();
	SceneTree::process(0);

	CHECK_MESSAGE(
			!foldable_container->is_folded(),
			"FoldableContainer is expanded after calling expand().");

	CHECK_MESSAGE(
			child_control->is_visible(),
			"Child control is visible again after FoldableContainer is expanded.");

	foldable_container->set_folded(true);
	SceneTree::process(0);

	CHECK_MESSAGE(
			foldable_container->is_folded(),
			"set_folded(true) folds the FoldableContainer.");

	foldable_container->set_folded(false);
	SceneTree::process(0);

	CHECK_MESSAGE(
			!foldable_container->is_folded(),
			"set_folded(false) expands the FoldableContainer.");

	memdelete(child_control);
	memdelete(foldable_container);
}

TEST_CASE("[SceneTree][FoldableContainer] Title position and title bar controls") {
	FoldableContainer *foldable_container = memnew(FoldableContainer("Section"));
	Window *root = SceneTree::get_root();
	root->add_child(foldable_container);

	Control *content_control = memnew(Control);
	content_control->set_custom_minimum_size(Size2(40, 40));
	foldable_container->add_child(content_control);

	Control *title_control = memnew(Control);
	title_control->set_custom_minimum_size(Size2(30, 20));
	foldable_container->add_title_bar_control(title_control);

	foldable_container->set_size(Size2(200, 140));
	SceneTree::process(0);

	CHECK_MESSAGE(
			title_control->get_parent() == foldable_container,
			"Title bar control is reparented to the FoldableContainer when added.");

	real_t top_title_control_y = title_control->get_position().y;
	real_t top_content_y = content_control->get_position().y;

	foldable_container->set_title_position(FoldableContainer::POSITION_BOTTOM);
	SceneTree::process(0);

	real_t bottom_title_control_y = title_control->get_position().y;
	real_t bottom_content_y = content_control->get_position().y;

	CHECK_MESSAGE(
			bottom_title_control_y > top_title_control_y,
			"Title bar controls move to the bottom when title position is set to POSITION_BOTTOM.");

	CHECK_MESSAGE(
			bottom_content_y < top_content_y,
			"Content area shifts upward when title position is changed from top to bottom.");

	foldable_container->remove_title_bar_control(title_control);

	CHECK_MESSAGE(
			title_control->get_parent() == nullptr,
			"Title bar control is detached from FoldableContainer when removed.");

	memdelete(title_control);
	memdelete(content_control);
	memdelete(foldable_container);
}

TEST_CASE("[SceneTree][FoldableContainer] FoldableGroup behavior") {
	Ref<FoldableGroup> foldable_group;
	foldable_group.instantiate();

	FoldableContainer *container_a = memnew(FoldableContainer("A"));
	FoldableContainer *container_b = memnew(FoldableContainer("B"));
	Window *root = SceneTree::get_root();
	root->add_child(container_a);
	root->add_child(container_b);

	container_a->set_folded(true);
	container_b->set_folded(true);
	container_a->set_foldable_group(foldable_group);
	container_b->set_foldable_group(foldable_group);
	SceneTree::process(0);

	CHECK_MESSAGE(
			!container_a->is_folded(),
			"First grouped FoldableContainer auto-expands when allow_folding_all is false and no container is expanded.");

	CHECK_MESSAGE(
			container_b->is_folded(),
			"Second grouped FoldableContainer remains folded when another container in the group is expanded.");

	CHECK_MESSAGE(
			foldable_group->get_expanded_container() == container_a,
			"FoldableGroup tracks the currently expanded container.");

	container_b->set_folded(false);
	SceneTree::process(0);

	CHECK_MESSAGE(
			container_a->is_folded(),
			"Expanding one grouped FoldableContainer folds the previously expanded container.");

	CHECK_MESSAGE(
			!container_b->is_folded(),
			"Expanded grouped FoldableContainer remains unfolded.");

	CHECK_MESSAGE(
			foldable_group->get_expanded_container() == container_b,
			"FoldableGroup updates expanded container after a different container is expanded.");

	container_b->set_folded(true);
	SceneTree::process(0);

	CHECK_MESSAGE(
			!container_b->is_folded(),
			"Expanded grouped FoldableContainer cannot be folded when allow_folding_all is false.");

	foldable_group->set_allow_folding_all(true);
	container_b->set_folded(true);
	SceneTree::process(0);

	CHECK_MESSAGE(
			container_b->is_folded(),
			"Grouped FoldableContainer can be folded when allow_folding_all is true.");

	CHECK_MESSAGE(
			foldable_group->get_expanded_container() == nullptr,
			"FoldableGroup has no expanded container when all grouped containers are folded.");

	memdelete(container_b);
	memdelete(container_a);
}

} // namespace TestFoldableContainer

#endif

// === Merged from tests/scene/test_graph_node.cpp ===
/*  test_graph_node.cpp                                                   */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"

namespace TestGraphNode {

TEST_CASE("[GraphNode][SceneTree]") {
	SUBCASE("[GraphNode] Graph Node only child on delete should not cause error.") {
		// Setup.
		GraphNode *test_node = memnew(GraphNode);
		test_node->set_name("Graph Node");
		CHECK(test_node->get_mode() == GraphNode::MODE_NODE);
		CHECK(!test_node->is_frame());

		Control *test_child = memnew(Control);
		test_child->set_name("child");
		test_node->add_child(test_child);

		// Test.
		test_node->remove_child(test_child);
		CHECK(test_node->get_child_count(false) == 0);

		memdelete(test_child);
		memdelete(test_node);
	}

	SUBCASE("[GraphNode] Frame and Element modes.") {
		GraphNode *frame_node = memnew(GraphNode(GraphNode::MODE_FRAME));
		CHECK(frame_node->get_mode() == GraphNode::MODE_FRAME);
		CHECK(frame_node->is_frame());
		CHECK(frame_node->is_autoshrink_enabled());
		frame_node->set_autoshrink_enabled(false);
		CHECK(!frame_node->is_autoshrink_enabled());
		frame_node->set_tint_color_enabled(true);
		CHECK(frame_node->is_tint_color_enabled());
		memdelete(frame_node);

		GraphNode *elem_node = memnew(GraphNode(GraphNode::MODE_ELEMENT));
		CHECK(elem_node->get_mode() == GraphNode::MODE_ELEMENT);
		CHECK(elem_node->is_element_only());
		CHECK(elem_node->get_titlebar_hbox() == nullptr);
		memdelete(elem_node);
	}
}

} // namespace TestGraphNode

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_grid_container.cpp ===
/*  test_grid_container.cpp                                               */
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
#include "scene/gui/grid_container.h"
#include "scene/main/scene_tree.h"

namespace TestGridContainer {

TEST_CASE("[SceneTree][GridContainer] Default properties") {
	GridContainer *grid_container = memnew(GridContainer);
	Window *root = SceneTree::get_root();
	root->add_child(grid_container);

	grid_container->set_size(Size2(200, 200));
	grid_container->add_theme_constant_override("h_separation", 0);
	grid_container->add_theme_constant_override("v_separation", 0);

	Control *child_control = memnew(Control);
	child_control->set_custom_minimum_size(Size2(20, 10));
	grid_container->add_child(child_control);
	SceneTree::process(0);

	CHECK_MESSAGE(
			grid_container->get_columns() == 1,
			"GridContainer columns are set to 1 by default.");

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(0, 0)),
			"Single child control starts at the top-left corner.");

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(20, 10)),
			"Single child control keeps its minimum size when no expansion is requested.");

	memdelete(child_control);
	memdelete(grid_container);
}

TEST_CASE("[SceneTree][GridContainer] Columns and placement order") {
	GridContainer *grid_container = memnew(GridContainer);
	Window *root = SceneTree::get_root();
	root->add_child(grid_container);

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	Control *child_control_3 = memnew(Control);
	Control *child_control_4 = memnew(Control);
	child_control_1->set_custom_minimum_size(Size2(10, 10));
	child_control_2->set_custom_minimum_size(Size2(10, 10));
	child_control_3->set_custom_minimum_size(Size2(10, 10));
	child_control_4->set_custom_minimum_size(Size2(10, 10));

	grid_container->set_columns(2);
	grid_container->set_size(Size2(200, 200));
	grid_container->add_theme_constant_override("h_separation", 0);
	grid_container->add_theme_constant_override("v_separation", 0);
	grid_container->add_child(child_control_1);
	grid_container->add_child(child_control_2);
	grid_container->add_child(child_control_3);
	grid_container->add_child(child_control_4);
	SceneTree::process(0);

	CHECK_MESSAGE(
			grid_container->get_columns() == 2,
			"GridContainer uses the configured number of columns.");

	CHECK_MESSAGE(
			child_control_1->get_position().is_equal_approx(Point2(0, 0)),
			"First child control is placed at row 0, column 0.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(10, 0)),
			"Second child control is placed at row 0, column 1.");

	CHECK_MESSAGE(
			child_control_3->get_position().is_equal_approx(Point2(0, 10)),
			"Third child control is placed at row 1, column 0.");

	CHECK_MESSAGE(
			child_control_4->get_position().is_equal_approx(Point2(10, 10)),
			"Fourth child control is placed at row 1, column 1.");

	memdelete(child_control_4);
	memdelete(child_control_3);
	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(grid_container);
}

TEST_CASE("[SceneTree][GridContainer] Separation and minimum size") {
	GridContainer *grid_container = memnew(GridContainer);
	Window *root = SceneTree::get_root();
	root->add_child(grid_container);

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	Control *child_control_3 = memnew(Control);
	child_control_1->set_custom_minimum_size(Size2(10, 20));
	child_control_2->set_custom_minimum_size(Size2(30, 10));
	child_control_3->set_custom_minimum_size(Size2(15, 40));

	grid_container->set_columns(2);
	grid_container->add_theme_constant_override("h_separation", 5);
	grid_container->add_theme_constant_override("v_separation", 7);
	grid_container->add_child(child_control_1);
	grid_container->add_child(child_control_2);
	grid_container->add_child(child_control_3);
	SceneTree::process(0);

	CHECK_MESSAGE(
			grid_container->get_h_separation() == 5,
			"GridContainer reports overridden horizontal separation.");

	CHECK_MESSAGE(
			grid_container->get_minimum_size().is_equal_approx(Size2(50, 67)),
			"GridContainer minimum size matches per-column and per-row maxima plus separations.");

	grid_container->set_size(Size2(50, 67));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control_1->get_position().is_equal_approx(Point2(0, 0)),
			"First child control is placed at row 0, column 0 with configured separations.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(20, 0)),
			"Second child control is offset by column width and horizontal separation.");

	CHECK_MESSAGE(
			child_control_3->get_position().is_equal_approx(Point2(0, 27)),
			"Third child control is offset by row height and vertical separation.");

	CHECK_MESSAGE(
			child_control_1->get_size().is_equal_approx(Size2(15, 20)),
			"First child control fills the first grid cell dimensions.");

	CHECK_MESSAGE(
			child_control_2->get_size().is_equal_approx(Size2(30, 20)),
			"Second child control fills the second grid cell dimensions.");

	CHECK_MESSAGE(
			child_control_3->get_size().is_equal_approx(Size2(15, 40)),
			"Third child control fills the second-row first-column cell dimensions.");

	memdelete(child_control_3);
	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(grid_container);
}

TEST_CASE("[SceneTree][GridContainer] Expanding rows and columns") {
	GridContainer *grid_container = memnew(GridContainer);
	Window *root = SceneTree::get_root();
	root->add_child(grid_container);

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	Control *child_control_3 = memnew(Control);
	child_control_1->set_custom_minimum_size(Size2(20, 20));
	child_control_2->set_custom_minimum_size(Size2(20, 20));
	child_control_3->set_custom_minimum_size(Size2(20, 20));
	child_control_1->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	child_control_1->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	grid_container->set_columns(2);
	grid_container->add_theme_constant_override("h_separation", 0);
	grid_container->add_theme_constant_override("v_separation", 0);
	grid_container->set_size(Size2(100, 100));
	grid_container->add_child(child_control_1);
	grid_container->add_child(child_control_2);
	grid_container->add_child(child_control_3);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control_1->get_size().is_equal_approx(Size2(80, 80)),
			"Expanded child control grows with its expanded column and row.");

	CHECK_MESSAGE(
			child_control_2->get_size().is_equal_approx(Size2(20, 80)),
			"Non-expanded column still receives expanded row height.");

	CHECK_MESSAGE(
			child_control_3->get_size().is_equal_approx(Size2(80, 20)),
			"Non-expanded row still receives expanded column width.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(80, 0)),
			"Second child control is positioned after expanded first column width.");

	CHECK_MESSAGE(
			child_control_3->get_position().is_equal_approx(Point2(0, 80)),
			"Third child control is positioned after expanded first row height.");

	memdelete(child_control_3);
	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(grid_container);
}

TEST_CASE("[SceneTree][GridContainer] RTL placement") {
	GridContainer *grid_container = memnew(GridContainer);
	Window *root = SceneTree::get_root();
	root->add_child(grid_container);

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	child_control_1->set_custom_minimum_size(Size2(20, 20));
	child_control_2->set_custom_minimum_size(Size2(20, 20));

	grid_container->set_columns(2);
	grid_container->add_theme_constant_override("h_separation", 0);
	grid_container->add_theme_constant_override("v_separation", 0);
	grid_container->set_size(Size2(40, 20));
	grid_container->add_child(child_control_1);
	grid_container->add_child(child_control_2);
	grid_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control_1->get_position().is_equal_approx(Point2(20, 0)),
			"First child control starts in the right column in RTL mode.");

	CHECK_MESSAGE(
			child_control_2->get_position().is_equal_approx(Point2(0, 0)),
			"Second child control is placed in the left column in RTL mode.");

	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(grid_container);
}

} // namespace TestGridContainer

// === Merged from tests/scene/test_label.cpp ===
/*  test_label.cpp                                                        */
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



#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. Needed for MODULE_TEXT_SERVER_FB_ENABLED and MODULE_TEXT_SERVER_ADV_ENABLED definitions.

#if defined(MODULE_TEXT_SERVER_FB_ENABLED) || defined(MODULE_TEXT_SERVER_ADV_ENABLED)

#include "scene/main/scene_tree.h"

namespace TestLabel {

TEST_CASE("[SceneTree][Label] Custom minimum size") {
	// This is an anti-regression test case introduced in GH-116640. When the old minimum size behavior is removed, this test case should be removed too.
	Label *test_label = memnew(Label);
	Window *root = SceneTree::get_root();
	root->add_child(test_label);

	real_t min_width = 50;

	test_label->set_custom_minimum_size(Size2(min_width, 0));
	test_label->set_text("This is a long text that should be wrapped and exceeds minimum size.");
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase to fit the text with AUTOWRAP_OFF.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_ARBITRARY);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_ARBITRARY.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_WORD.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_WORD_SMART.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	test_label->set_clip_text(true);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with clip_text.");

	test_label->set_clip_text(false);
	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_CHAR);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_CHAR.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_WORD.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS_FORCE);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_ELLIPSIS_FORCE.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS_FORCE);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS_FORCE.");

	memdelete(test_label);
}

TEST_CASE("[SceneTree][Label] Sizing") {
	Label *test_label = memnew(Label);
	Window *root = SceneTree::get_root();
	root->add_child(test_label);

	real_t max_width = 50;
	real_t min_width = 25;

	test_label->set_custom_maximum_size(Size2(max_width, -1));
	test_label->set_custom_minimum_size(Size2(min_width, 0));
	test_label->set_text("This is a long text that should be wrapped and exceeds minimum size.");
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_OFF.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_OFF.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_ARBITRARY);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_ARBITRARY.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_ARBITRARY.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_WORD.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_WORD.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_WORD_SMART.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_WORD_SMART.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	test_label->set_clip_text(true);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with clip_text.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with clip_text.");

	test_label->set_clip_text(false);
	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_CHAR);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_CHAR.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_CHAR.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_WORD.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_WORD.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_ELLIPSIS.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_WORD_ELLIPSIS.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_ELLIPSIS_FORCE);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_ELLIPSIS_FORCE.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_ELLIPSIS_FORCE.");

	test_label->set_text_overrun_behavior(TextServer::OVERRUN_TRIM_WORD_ELLIPSIS_FORCE);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with OVERRUN_TRIM_WORD_ELLIPSIS_FORCE.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with OVERRUN_TRIM_WORD_ELLIPSIS_FORCE.");

	memdelete(test_label);
}

} // namespace TestLabel

#endif // MODULE_TEXT_SERVER_FB_ENABLED || MODULE_TEXT_SERVER_ADV_ENABLED

// === Merged from tests/scene/test_margin_container.cpp ===
/*  test_margin_container.cpp                                             */
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
#include "scene/gui/margin_container.h"
#include "scene/main/scene_tree.h"

namespace TestMarginContainer {

TEST_CASE("[SceneTree][MarginContainer] Default margins and layout") {
	MarginContainer *margin_container = memnew(MarginContainer);
	Window *root = SceneTree::get_root();
	root->add_child(margin_container);

	Control *child_control = memnew(Control);
	child_control->set_custom_minimum_size(Size2(20, 10));
	margin_container->add_child(child_control);

	margin_container->set_size(Size2(100, 80));
	SceneTree::process(0);

	CHECK_MESSAGE(
			margin_container->get_margin_size(SIDE_LEFT) == 0,
			"Left margin is 0 by default.");

	CHECK_MESSAGE(
			margin_container->get_margin_size(SIDE_TOP) == 0,
			"Top margin is 0 by default.");

	CHECK_MESSAGE(
			margin_container->get_margin_size(SIDE_RIGHT) == 0,
			"Right margin is 0 by default.");

	CHECK_MESSAGE(
			margin_container->get_margin_size(SIDE_BOTTOM) == 0,
			"Bottom margin is 0 by default.");

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(0, 0)),
			"Child control starts at the top-left corner when all margins are 0.");

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(100, 80)),
			"Child control fills the container area when all margins are 0.");

	CHECK_MESSAGE(
			margin_container->get_minimum_size().is_equal_approx(Size2(20, 10)),
			"Minimum size equals the child minimum size when all margins are 0.");

	memdelete(child_control);
	memdelete(margin_container);
}

TEST_CASE("[SceneTree][MarginContainer] Custom margins affect layout and minimum size") {
	MarginContainer *margin_container = memnew(MarginContainer);
	Window *root = SceneTree::get_root();
	root->add_child(margin_container);

	margin_container->add_theme_constant_override("margin_left", 10);
	margin_container->add_theme_constant_override("margin_top", 20);
	margin_container->add_theme_constant_override("margin_right", 30);
	margin_container->add_theme_constant_override("margin_bottom", 40);

	Control *child_control = memnew(Control);
	child_control->set_custom_minimum_size(Size2(20, 10));
	margin_container->add_child(child_control);

	margin_container->set_size(Size2(200, 150));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(10, 20)),
			"Child control is offset by left and top margins.");

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(160, 90)),
			"Child control size is reduced by left/right and top/bottom margins.");

	CHECK_MESSAGE(
			margin_container->get_minimum_size().is_equal_approx(Size2(60, 70)),
			"Minimum size equals child minimum size plus all configured margins.");

	memdelete(child_control);
	memdelete(margin_container);
}

TEST_CASE("[SceneTree][MarginContainer] Multiple children use maximum minimum size") {
	MarginContainer *margin_container = memnew(MarginContainer);
	Window *root = SceneTree::get_root();
	root->add_child(margin_container);

	margin_container->add_theme_constant_override("margin_left", 1);
	margin_container->add_theme_constant_override("margin_top", 2);
	margin_container->add_theme_constant_override("margin_right", 3);
	margin_container->add_theme_constant_override("margin_bottom", 4);

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	child_control_1->set_custom_minimum_size(Size2(40, 10));
	child_control_2->set_custom_minimum_size(Size2(20, 50));
	margin_container->add_child(child_control_1);
	margin_container->add_child(child_control_2);
	SceneTree::process(0);

	CHECK_MESSAGE(
			margin_container->get_minimum_size().is_equal_approx(Size2(44, 56)),
			"Minimum size uses maximum child minimum width/height plus margins.");

	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(margin_container);
}

} // namespace TestMarginContainer

// === Merged from tests/scene/test_nine_patch_rect.cpp ===
/*  test_nine_patch_rect.cpp                                              */
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
#include "servers/display/display_server.h"
#include "tests/test_macros.h"

namespace TestNinePatchRect {

TEST_CASE("[SceneTree][NinePatchRect] Default properties") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);

	CHECK(nine_patch->get_texture().is_null());

	CHECK(nine_patch->get_patch_margin(SIDE_LEFT) == 0);
	CHECK(nine_patch->get_patch_margin(SIDE_TOP) == 0);
	CHECK(nine_patch->get_patch_margin(SIDE_RIGHT) == 0);
	CHECK(nine_patch->get_patch_margin(SIDE_BOTTOM) == 0);

	CHECK(nine_patch->get_region_rect() == Rect2(0, 0, 0, 0));
	CHECK(nine_patch->is_draw_center_enabled());

	CHECK(nine_patch->get_h_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_STRETCH);
	CHECK(nine_patch->get_v_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_STRETCH);

	// Set in the NinePatchRect constructor, unlike the Control default.
	CHECK(nine_patch->get_mouse_filter() == Control::MOUSE_FILTER_IGNORE);

	CHECK(nine_patch->get_combined_minimum_size() == Size2(0, 0));

	memdelete(nine_patch);
}

TEST_CASE("[SceneTree][NinePatchRect] Set/get texture") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);

	Ref<Texture2D> texture;
	texture.instantiate();

	nine_patch->set_texture(texture);
	CHECK(nine_patch->get_texture() == texture);

	nine_patch->set_texture(Ref<Texture2D>());
	CHECK(nine_patch->get_texture().is_null());

	memdelete(nine_patch);
}

TEST_CASE("[SceneTree][NinePatchRect] texture_changed signal") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);

	Ref<Texture2D> texture_a;
	texture_a.instantiate();
	Ref<Texture2D> texture_b;
	texture_b.instantiate();

	SIGNAL_WATCH(nine_patch, "texture_changed");
	Array empty_signal_args = { {} };

	SUBCASE("Setting a new texture emits the signal") {
		nine_patch->set_texture(texture_a);
		SIGNAL_CHECK("texture_changed", empty_signal_args);
	}

	SUBCASE("Setting the same texture does not re-emit") {
		nine_patch->set_texture(texture_a);
		SIGNAL_DISCARD("texture_changed");

		nine_patch->set_texture(texture_a);
		SIGNAL_CHECK_FALSE("texture_changed");
	}

	SUBCASE("Replacing one texture with another emits the signal") {
		nine_patch->set_texture(texture_a);
		SIGNAL_DISCARD("texture_changed");

		nine_patch->set_texture(texture_b);
		SIGNAL_CHECK("texture_changed", empty_signal_args);
	}

	SUBCASE("Clearing the texture emits the signal") {
		nine_patch->set_texture(texture_a);
		SIGNAL_DISCARD("texture_changed");

		nine_patch->set_texture(Ref<Texture2D>());
		SIGNAL_CHECK("texture_changed", empty_signal_args);
	}

	SIGNAL_UNWATCH(nine_patch, "texture_changed");
	memdelete(nine_patch);
}

TEST_CASE("[SceneTree][NinePatchRect] Patch margins") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);

	SUBCASE("Each side can be set and read independently") {
		nine_patch->set_patch_margin(SIDE_LEFT, 5);
		nine_patch->set_patch_margin(SIDE_TOP, 10);
		nine_patch->set_patch_margin(SIDE_RIGHT, 15);
		nine_patch->set_patch_margin(SIDE_BOTTOM, 20);

		CHECK(nine_patch->get_patch_margin(SIDE_LEFT) == 5);
		CHECK(nine_patch->get_patch_margin(SIDE_TOP) == 10);
		CHECK(nine_patch->get_patch_margin(SIDE_RIGHT) == 15);
		CHECK(nine_patch->get_patch_margin(SIDE_BOTTOM) == 20);
	}

	SUBCASE("Updating one side does not affect the others") {
		nine_patch->set_patch_margin(SIDE_LEFT, 7);
		nine_patch->set_patch_margin(SIDE_RIGHT, 9);

		nine_patch->set_patch_margin(SIDE_LEFT, 42);

		CHECK(nine_patch->get_patch_margin(SIDE_LEFT) == 42);
		CHECK(nine_patch->get_patch_margin(SIDE_TOP) == 0);
		CHECK(nine_patch->get_patch_margin(SIDE_RIGHT) == 9);
		CHECK(nine_patch->get_patch_margin(SIDE_BOTTOM) == 0);
	}

	memdelete(nine_patch);
}

TEST_CASE("[SceneTree][NinePatchRect] Patch margins drive minimum size") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);
	Window *root = SceneTree::get_root();
	root->add_child(nine_patch);

	nine_patch->set_patch_margin(SIDE_LEFT, 10);
	nine_patch->set_patch_margin(SIDE_TOP, 20);
	nine_patch->set_patch_margin(SIDE_RIGHT, 30);
	nine_patch->set_patch_margin(SIDE_BOTTOM, 40);
	SceneTree::process(0);

	// Minimum width is left + right; minimum height is top + bottom.
	CHECK(nine_patch->get_combined_minimum_size() == Size2(40, 60));

	nine_patch->set_patch_margin(SIDE_RIGHT, 0);
	SceneTree::process(0);
	CHECK(nine_patch->get_combined_minimum_size() == Size2(10, 60));

	memdelete(nine_patch);
}

TEST_CASE("[SceneTree][NinePatchRect] Patch margin out-of-range index is guarded") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);

	nine_patch->set_patch_margin(SIDE_LEFT, 11);
	nine_patch->set_patch_margin(SIDE_TOP, 22);
	nine_patch->set_patch_margin(SIDE_RIGHT, 33);
	nine_patch->set_patch_margin(SIDE_BOTTOM, 44);

	ERR_PRINT_OFF;
	nine_patch->set_patch_margin((Side)4, 999);
	const int out_of_range_value = nine_patch->get_patch_margin((Side)4);
	ERR_PRINT_ON;

	CHECK(out_of_range_value == 0);
	CHECK(nine_patch->get_patch_margin(SIDE_LEFT) == 11);
	CHECK(nine_patch->get_patch_margin(SIDE_TOP) == 22);
	CHECK(nine_patch->get_patch_margin(SIDE_RIGHT) == 33);
	CHECK(nine_patch->get_patch_margin(SIDE_BOTTOM) == 44);

	memdelete(nine_patch);
}

TEST_CASE("[SceneTree][NinePatchRect] Region rect and draw center") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);

	SUBCASE("Region rect is set and retrieved") {
		const Rect2 region(8, 16, 32, 64);
		nine_patch->set_region_rect(region);
		CHECK(nine_patch->get_region_rect() == region);
	}

	SUBCASE("Draw center can be toggled") {
		nine_patch->set_draw_center(false);
		CHECK_FALSE(nine_patch->is_draw_center_enabled());

		nine_patch->set_draw_center(true);
		CHECK(nine_patch->is_draw_center_enabled());
	}

	memdelete(nine_patch);
}

TEST_CASE("[SceneTree][NinePatchRect] Axis stretch modes") {
	NinePatchRect *nine_patch = memnew(NinePatchRect);

	SUBCASE("Horizontal stretch mode accepts each value") {
		nine_patch->set_h_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_TILE);
		CHECK(nine_patch->get_h_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_TILE);

		nine_patch->set_h_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_TILE_FIT);
		CHECK(nine_patch->get_h_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_TILE_FIT);

		nine_patch->set_h_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_STRETCH);
		CHECK(nine_patch->get_h_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_STRETCH);
	}

	SUBCASE("Vertical stretch mode accepts each value") {
		nine_patch->set_v_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_TILE);
		CHECK(nine_patch->get_v_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_TILE);

		nine_patch->set_v_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_TILE_FIT);
		CHECK(nine_patch->get_v_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_TILE_FIT);

		nine_patch->set_v_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_STRETCH);
		CHECK(nine_patch->get_v_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_STRETCH);
	}

	SUBCASE("Horizontal and vertical stretch modes are independent") {
		nine_patch->set_h_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_TILE);
		nine_patch->set_v_axis_stretch_mode(NinePatchRect::AXIS_STRETCH_MODE_TILE_FIT);

		CHECK(nine_patch->get_h_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_TILE);
		CHECK(nine_patch->get_v_axis_stretch_mode() == NinePatchRect::AXIS_STRETCH_MODE_TILE_FIT);
	}

	memdelete(nine_patch);
}

} // namespace TestNinePatchRect

// === Merged from tests/scene/test_option_button.cpp ===
/*  test_option_button.cpp                                                */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"

namespace TestOptionButton {

TEST_CASE("[SceneTree][OptionButton] Initialization") {
	OptionButton *test_opt = memnew(OptionButton);

	SUBCASE("There should be no options right after initialization") {
		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 0);
	}

	memdelete(test_opt);
}

TEST_CASE("[SceneTree][OptionButton] Single item") {
	OptionButton *test_opt = memnew(OptionButton);

	SUBCASE("There should a single item after after adding one") {
		test_opt->add_item("single", 1013);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 1);
		CHECK(test_opt->get_item_index(1013) == 0);
		CHECK(test_opt->get_item_id(0) == 1013);

		test_opt->remove_item(0);

		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 0);
	}

	SUBCASE("There should a single item after after adding an icon") {
		Ref<Texture2D> test_icon = memnew(Texture2D);
		test_opt->add_icon_item(test_icon, "icon", 345);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 1);
		CHECK(test_opt->get_item_index(345) == 0);
		CHECK(test_opt->get_item_id(0) == 345);

		test_opt->remove_item(0);

		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 0);
	}

	memdelete(test_opt);
}

TEST_CASE("[SceneTree][OptionButton] Many items") {
	OptionButton *test_opt = memnew(OptionButton);

	SUBCASE("Creating a complex structure and checking getters") {
		// Regular item at index 0.
		Ref<Texture2D> test_icon1 = memnew(Texture2D);
		Ref<Texture2D> test_icon2 = memnew(Texture2D);
		// Regular item at index 3.
		Ref<Texture2D> test_icon4 = memnew(Texture2D);

		test_opt->add_item("first", 100);
		test_opt->add_icon_item(test_icon1, "second_icon", 101);
		test_opt->add_icon_item(test_icon2, "third_icon", 102);
		test_opt->add_item("fourth", 104);
		test_opt->add_icon_item(test_icon4, "fifth_icon", 104);

		// Disable test_icon4.
		test_opt->set_item_disabled(4, true);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 5);

		// Check for test_icon2.
		CHECK(test_opt->get_item_index(102) == 2);
		CHECK(test_opt->get_item_id(2) == 102);

		// Remove the two regular items.
		test_opt->remove_item(3);
		test_opt->remove_item(0);

		CHECK(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 3);

		// Check test_icon4.
		CHECK(test_opt->get_item_index(104) == 2);
		CHECK(test_opt->get_item_id(2) == 104);

		// Remove the two non-disabled icon items.
		test_opt->remove_item(1);
		test_opt->remove_item(0);

		CHECK_FALSE(test_opt->has_selectable_items());
		CHECK(test_opt->get_item_count() == 1);
	}

	SUBCASE("Getters and setters not related to structure") {
		test_opt->add_item("regular", 2019);

		Ref<Texture2D> test_icon = memnew(Texture2D);
		test_opt->add_icon_item(test_icon, "icon", 3092);

		// item_text.
		test_opt->set_item_text(0, "example text");
		CHECK(test_opt->get_item_text(0) == "example text");

		// item_metadata.
		Dictionary m;
		m["bool"] = true;
		m["String"] = "yes";
		test_opt->set_item_metadata(1, m);
		CHECK(test_opt->get_item_metadata(1) == m);

		// item_tooltip.
		test_opt->set_item_tooltip(0, "tooltip guide");
		CHECK(test_opt->get_item_tooltip(0) == "tooltip guide");

		test_opt->remove_item(1);
		test_opt->remove_item(0);
	}

	memdelete(test_opt);
}

} // namespace TestOptionButton

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_panel_container.cpp ===
/*  test_panel_container.cpp                                              */
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
#include "scene/gui/panel_container.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/theme.h"

namespace TestPanelContainer {

TEST_CASE("[SceneTree][PanelContainer] Default properties") {
	PanelContainer *panel_container = memnew(PanelContainer);
	Window *root = SceneTree::get_root();
	root->add_child(panel_container);
	SceneTree::process(0);

	CHECK_MESSAGE(
			panel_container->get_mouse_filter() == Control::MOUSE_FILTER_STOP,
			"PanelContainer mouse filter is set to MOUSE_FILTER_STOP by default.");

	memdelete(panel_container);
}

TEST_CASE("[SceneTree][PanelContainer] StyleBox affects layout and minimum size") {
	PanelContainer *panel_container = memnew(PanelContainer);
	Window *root = SceneTree::get_root();
	root->add_child(panel_container);

	Ref<StyleBox> panel_style = memnew(StyleBox(StyleBox::STYLE_EMPTY));
	panel_style->set_content_margin_individual(5, 6, 7, 8);
	panel_container->add_theme_style_override("panel", panel_style);

	Control *child_control = memnew(Control);
	child_control->set_custom_minimum_size(Size2(20, 10));
	panel_container->add_child(child_control);

	panel_container->set_size(Size2(100, 80));
	SceneTree::process(0);

	CHECK_MESSAGE(
			child_control->get_position().is_equal_approx(Point2(5, 6)),
			"Child control is offset by the panel StyleBox margins.");

	CHECK_MESSAGE(
			child_control->get_size().is_equal_approx(Size2(88, 66)),
			"Child control size is reduced by the panel StyleBox minimum size.");

	CHECK_MESSAGE(
			panel_container->get_minimum_size().is_equal_approx(Size2(32, 24)),
			"Minimum size equals child minimum size plus panel StyleBox minimum size.");

	memdelete(child_control);
	memdelete(panel_container);
}

TEST_CASE("[SceneTree][PanelContainer] Multiple children use maximum minimum size") {
	PanelContainer *panel_container = memnew(PanelContainer);
	Window *root = SceneTree::get_root();
	root->add_child(panel_container);

	Ref<StyleBox> panel_style = memnew(StyleBox(StyleBox::STYLE_EMPTY));
	panel_style->set_content_margin_individual(2, 3, 4, 5);
	panel_container->add_theme_style_override("panel", panel_style);

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	child_control_1->set_custom_minimum_size(Size2(40, 10));
	child_control_2->set_custom_minimum_size(Size2(20, 50));
	panel_container->add_child(child_control_1);
	panel_container->add_child(child_control_2);
	SceneTree::process(0);

	CHECK_MESSAGE(
			panel_container->get_minimum_size().is_equal_approx(Size2(46, 58)),
			"Minimum size uses maximum child minimum width/height plus panel StyleBox minimum size.");

	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(panel_container);
}

} // namespace TestPanelContainer

// === Merged from tests/scene/test_rich_text_label.cpp ===
/*  test_rich_text_label.cpp                                              */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"

namespace TestRichTextLabel {

TEST_CASE("[SceneTree][RichTextLabel] Custom minimum size with fit content") {
	// This is an anti-regression test case introduced in GH-116640. When the old minimum size behavior is removed, this test case should be removed too.
	RichTextLabel *test_label = memnew(RichTextLabel);
	Window *root = SceneTree::get_root();
	root->add_child(test_label);

	real_t min_width = 50;

	test_label->set_fit_content(true);
	test_label->set_custom_minimum_size(Size2(min_width, 0));
	test_label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	test_label->set_text("This is a long text that should be wrapped and exceeds minimum size.");
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase to fit the text with AUTOWRAP_OFF.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_ARBITRARY);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_ARBITRARY.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_WORD.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	SceneTree::process(0);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_label->get_size().width, min_width),
			"Label width will be equal to custom minimum width with AUTOWRAP_WORD_SMART.");

	memdelete(test_label);
}

TEST_CASE("[SceneTree][RichTextLabel] Sizing with fit content") {
	RichTextLabel *test_label = memnew(RichTextLabel);
	Window *root = SceneTree::get_root();
	root->add_child(test_label);

	real_t max_width = 50;
	real_t min_width = 25;

	test_label->set_fit_content(true);
	test_label->set_custom_maximum_size(Size2(max_width, 0));
	test_label->set_custom_minimum_size(Size2(min_width, 0));
	test_label->set_autowrap_mode(TextServer::AUTOWRAP_OFF);
	test_label->set_text("This is a long text that should be wrapped and exceeds minimum size.");
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_OFF.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_OFF.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_ARBITRARY);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_ARBITRARY.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_ARBITRARY.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_WORD.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_WORD.");

	test_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_label->get_size().width <= max_width,
			"Label width will increase up to the custom maximum width with AUTOWRAP_WORD_SMART.");
	CHECK_MESSAGE(
			test_label->get_size().width > min_width,
			"Label width will increase beyond the custom minimum width with AUTOWRAP_WORD_SMART.");

	memdelete(test_label);
}

} // namespace TestRichTextLabel

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_scroll_container.cpp ===
/*  test_scroll_container.cpp                                             */
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

namespace TestScrollContainer {

TEST_CASE("[SceneTree][ScrollContainer] Default behavior") {
	ScrollContainer *test_container = memnew(ScrollContainer);
	Window *root = SceneTree::get_root();
	root->add_child(test_container);

	Control *test_control = memnew(Control);
	test_control->set_custom_minimum_size(Size2(20, 20));
	test_container->add_child(test_control);

	test_container->set_size(Size2(100, 100));
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_container->get_horizontal_scroll_mode() == ScrollContainer::SCROLL_MODE_AUTO,
			"Horizontal scroll mode is SCROLL_MODE_AUTO by default.");

	CHECK_MESSAGE(
			test_container->get_vertical_scroll_mode() == ScrollContainer::SCROLL_MODE_AUTO,
			"Vertical scroll mode is SCROLL_MODE_AUTO by default.");

	CHECK_MESSAGE(
			test_container->get_scroll_hint_mode() == ScrollContainer::SCROLL_HINT_MODE_DISABLED,
			"Scroll hint mode is SCROLL_HINT_MODE_DISABLED by default.");

	CHECK_MESSAGE(
			!test_container->is_scroll_hint_tiled(),
			"tile_scroll_hint is disabled by default.");

	CHECK_MESSAGE(
			!test_container->is_following_focus(),
			"follow_focus is disabled by default.");

	CHECK_MESSAGE(
			!test_container->is_scroll_horizontal_by_default(),
			"scroll_horizontal_by_default is disabled by default.");

	CHECK_MESSAGE(
			!test_container->get_draw_focus_border(),
			"draw_focus_border is disabled by default.");

	CHECK_MESSAGE(
			test_container->is_clipping_contents(),
			"clip_contents is enabled by default.");

	CHECK_MESSAGE(
			test_container->get_h_scroll() == 0,
			"Horizontal scroll starts at 0 by default.");

	CHECK_MESSAGE(
			test_container->get_v_scroll() == 0,
			"Vertical scroll starts at 0 by default.");

	CHECK_MESSAGE(
			test_container->get_h_scroll_bar() != nullptr,
			"Horizontal scrollbar is created by default.");

	CHECK_MESSAGE(
			test_container->get_v_scroll_bar() != nullptr,
			"Vertical scrollbar is created by default.");

	CHECK_MESSAGE(
			!test_container->get_h_scroll_bar()->is_visible(),
			"Horizontal scrollbar is hidden in auto mode when content fits.");

	CHECK_MESSAGE(
			!test_container->get_v_scroll_bar()->is_visible(),
			"Vertical scrollbar is hidden in auto mode when content fits.");

	memdelete(test_control);
	memdelete(test_container);
}

TEST_CASE("[SceneTree][ScrollContainer] Scroll modes") {
	ScrollContainer *test_container = memnew(ScrollContainer);
	Window *root = SceneTree::get_root();
	root->add_child(test_container);
	Control *test_control = memnew(Control);
	test_container->add_child(test_control);
	test_control->set_custom_minimum_size(Size2(100, 100));
	Control *target_control = memnew(Control);
	target_control->set_custom_minimum_size(Size2(20, 20));
	target_control->set_position(Point2(80, 80));
	target_control->set_size(Size2(20, 20));
	test_control->add_child(target_control);

	test_container->set_size(Size2(50, 50));
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar displays when ScrollContainer width is smaller than contents width with SCROLL_MODE_AUTO.");

	CHECK_MESSAGE(
			test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar displays when ScrollContainer height is smaller than contents height with SCROLL_MODE_AUTO.");

	test_container->set_size(Size2(150, 150));
	SceneTree::process(0);

	CHECK_MESSAGE(
			!test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar does not display when ScrollContainer width is larger than contents width with SCROLL_MODE_AUTO.");

	CHECK_MESSAGE(
			!test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar does not display when ScrollContainer height is larger than contents height with SCROLL_MODE_AUTO.");

	test_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	test_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);

	test_container->set_size(Size2(50, 50));
	SceneTree::process(0);

	CHECK_MESSAGE(
			!test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar does not display when ScrollContainer width is smaller than contents width with SCROLL_MODE_DISABLED.");

	CHECK_MESSAGE(
			!test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar does not display when ScrollContainer height is smaller than contents height with SCROLL_MODE_DISABLED.");

	test_container->set_h_scroll(0);
	test_container->set_v_scroll(0);
	int disabled_h_scroll_before = test_container->get_h_scroll();
	int disabled_v_scroll_before = test_container->get_v_scroll();
	test_container->ensure_control_visible(target_control);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_container->get_h_scroll() == disabled_h_scroll_before,
			"SCROLL_MODE_DISABLED does not change horizontal scroll when ensure_control_visible() targets an out-of-view descendant.");

	CHECK_MESSAGE(
			test_container->get_v_scroll() == disabled_v_scroll_before,
			"SCROLL_MODE_DISABLED does not change vertical scroll when ensure_control_visible() targets an out-of-view descendant.");

	test_container->set_h_scroll(0);
	test_container->set_v_scroll(0);

	test_container->set_size(Size2(150, 150));
	SceneTree::process(0);

	CHECK_MESSAGE(
			!test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar does not display when ScrollContainer width is larger than contents width with SCROLL_MODE_DISABLED.");

	CHECK_MESSAGE(
			!test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar does not display when ScrollContainer height is larger than contents height with SCROLL_MODE_DISABLED.");

	test_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_ALWAYS);
	test_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_ALWAYS);

	test_container->set_size(Size2(50, 50));
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar displays when ScrollContainer width is smaller than contents width with SCROLL_MODE_SHOW_ALWAYS.");

	CHECK_MESSAGE(
			test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar displays when ScrollContainer height is smaller than contents height with SCROLL_MODE_SHOW_ALWAYS.");

	test_container->set_size(Size2(150, 150));
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar displays when ScrollContainer width is larger than contents width with SCROLL_MODE_SHOW_ALWAYS.");

	CHECK_MESSAGE(
			test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar displays when ScrollContainer height is larger than contents height with SCROLL_MODE_SHOW_ALWAYS.");

	test_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_NEVER);
	test_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_NEVER);

	test_container->set_size(Size2(50, 50));
	SceneTree::process(0);

	CHECK_MESSAGE(
			!test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar does not display when ScrollContainer width is smaller than contents width with SCROLL_MODE_SHOW_NEVER.");

	CHECK_MESSAGE(
			!test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar does not display when ScrollContainer height is smaller than contents height with SCROLL_MODE_SHOW_NEVER.");

	test_container->set_h_scroll(0);
	test_container->set_v_scroll(0);
	int show_never_h_scroll_before = test_container->get_h_scroll();
	int show_never_v_scroll_before = test_container->get_v_scroll();
	test_container->ensure_control_visible(target_control);
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_container->get_h_scroll() > show_never_h_scroll_before,
			"SCROLL_MODE_SHOW_NEVER keeps horizontal scrollbar hidden while still allowing programmatic scrolling.");

	CHECK_MESSAGE(
			test_container->get_v_scroll() > show_never_v_scroll_before,
			"SCROLL_MODE_SHOW_NEVER keeps vertical scrollbar hidden while still allowing programmatic scrolling.");

	test_container->set_h_scroll(0);
	test_container->set_v_scroll(0);

	test_container->set_size(Size2(150, 150));
	SceneTree::process(0);

	CHECK_MESSAGE(
			!test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar does not display when ScrollContainer width is larger than contents width with SCROLL_MODE_SHOW_NEVER.");

	CHECK_MESSAGE(
			!test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar does not display when ScrollContainer height is larger than contents height with SCROLL_MODE_SHOW_NEVER.");

	test_container->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_RESERVE);
	test_container->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_RESERVE);

	test_container->set_size(Size2(50, 50));
	SceneTree::process(0);

	CHECK_MESSAGE(
			test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar displays when ScrollContainer width is smaller than contents width with SCROLL_MODE_RESERVE.");

	CHECK_MESSAGE(
			test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar displays when ScrollContainer height is smaller than contents height with SCROLL_MODE_RESERVE.");

	test_container->set_size(Size2(150, 150));
	SceneTree::process(0);

	CHECK_MESSAGE(
			!test_container->get_h_scroll_bar()->is_visible(),
			"H Scrollbar does not display when ScrollContainer width is larger than contents width with SCROLL_MODE_RESERVE.");

	CHECK_MESSAGE(
			!test_container->get_v_scroll_bar()->is_visible(),
			"V Scrollbar does not display when ScrollContainer height is larger than contents height with SCROLL_MODE_RESERVE.");

	memdelete(test_control);
	memdelete(test_container);
}

TEST_CASE("[SceneTree][ScrollContainer] Scroll values and property setters") {
	ScrollContainer *test_container = memnew(ScrollContainer);
	Window *root = SceneTree::get_root();
	root->add_child(test_container);

	Control *test_control = memnew(Control);
	test_control->set_custom_minimum_size(Size2(300, 300));
	test_container->add_child(test_control);

	test_container->set_size(Size2(100, 100));
	SceneTree::process(0);

	test_container->set_h_scroll(25);
	test_container->set_v_scroll(35);

	CHECK_MESSAGE(
			test_container->get_h_scroll() == 25,
			"Horizontal scroll value is updated by set_h_scroll().");

	CHECK_MESSAGE(
			test_container->get_v_scroll() == 35,
			"Vertical scroll value is updated by set_v_scroll().");

	test_container->set_horizontal_custom_step(7.0f);
	test_container->set_vertical_custom_step(9.0f);

	CHECK_MESSAGE(
			Math::is_equal_approx(test_container->get_horizontal_custom_step(), 7.0f),
			"Horizontal custom step is updated correctly.");

	CHECK_MESSAGE(
			Math::is_equal_approx(test_container->get_vertical_custom_step(), 9.0f),
			"Vertical custom step is updated correctly.");

	test_container->set_deadzone(13);
	CHECK_MESSAGE(
			test_container->get_deadzone() == 13,
			"Deadzone value is updated correctly.");

	test_container->set_scroll_horizontal_by_default(true);
	CHECK_MESSAGE(
			test_container->is_scroll_horizontal_by_default(),
			"scroll_horizontal_by_default is enabled correctly.");

	test_container->set_follow_focus(true);
	CHECK_MESSAGE(
			test_container->is_following_focus(),
			"follow_focus is enabled correctly.");

	test_container->set_scroll_hint_mode(ScrollContainer::SCROLL_HINT_MODE_ALL);
	CHECK_MESSAGE(
			test_container->get_scroll_hint_mode() == ScrollContainer::SCROLL_HINT_MODE_ALL,
			"Scroll hint mode is updated correctly.");

	test_container->set_tile_scroll_hint(true);
	CHECK_MESSAGE(
			test_container->is_scroll_hint_tiled(),
			"Tiled scroll hint mode is enabled correctly.");

	test_container->set_draw_focus_border(true);
	CHECK_MESSAGE(
			test_container->get_draw_focus_border(),
			"draw_focus_border is enabled correctly.");

	memdelete(test_control);
	memdelete(test_container);
}

TEST_CASE("[SceneTree][ScrollContainer] ensure_control_visible and follow_focus") {
	const Size2 container_size = Size2(100, 100);
	ScrollContainer *test_container = memnew(ScrollContainer);
	test_container->set_size(container_size);
	Window *root = SceneTree::get_root();
	root->add_child(test_container);

	Control *content_control = memnew(Control);
	content_control->set_custom_minimum_size(Size2(1000, 1000));
	test_container->add_child(content_control);

	Control *target_control = memnew(Control);
	target_control->set_focus_mode(Control::FOCUS_ALL);

	content_control->add_child(target_control);

	REQUIRE_EQ(test_container->get_h_scroll(), 0);
	REQUIRE_EQ(test_container->get_v_scroll(), 0);

	SceneTree::process(0);

	const Point2 initial_scroll = Point2(450, 450);
	test_container->set_h_scroll(initial_scroll.x);
	test_container->set_v_scroll(initial_scroll.y);

	SceneTree::process(0);
	REQUIRE_EQ(test_container->get_h_scroll(), initial_scroll.x);
	REQUIRE_EQ(test_container->get_v_scroll(), initial_scroll.y);
	REQUIRE_UNARY(test_container->get_h_scroll_bar()->is_visible_in_tree());
	REQUIRE_UNARY(test_container->get_v_scroll_bar()->is_visible_in_tree());
	REQUIRE_UNARY_FALSE(test_container->is_following_focus());
	REQUIRE_UNARY_FALSE(target_control->has_focus());

	SUBCASE("No scrolling when focused by default") {
		target_control->grab_focus();
		REQUIRE_UNARY(target_control->has_focus());
		CHECK_EQ(test_container->get_h_scroll(), initial_scroll.x);
		CHECK_EQ(test_container->get_v_scroll(), initial_scroll.y);
	}

	SUBCASE("Scrolling when ensure control visible by default") {
		test_container->ensure_control_visible(target_control);
		CHECK_EQ(test_container->get_h_scroll(), 0);
		CHECK_EQ(test_container->get_v_scroll(), 0);
	}

	test_container->set_follow_focus(true);
	REQUIRE_UNARY(test_container->is_following_focus());

	const char *size_cases[2] = {
		"The target control size is 50% of the container size",
		"The target control size is 150% of the container size",
	};

	const char *location_cases[9] = {
		"Target control is in the upper left corner",
		"Target control is directly above",
		"Target control is in the upper right corner",
		"Target control is on the left",
		"Target control is in the center",
		"Target control is on the right",
		"Target control is in the lower left corner",
		"Target control is directly below",
		"Target control is in the lower right corner",
	};
	const Size2 scroll_bar_space = Size2(test_container->get_v_scroll_bar()->get_size().x, test_container->get_h_scroll_bar()->get_size().y);

	for (int size_case_idx = 0; size_case_idx < 2; size_case_idx++) {
		SUBCASE(size_cases[size_case_idx]) {
			Size2 control_size = container_size / 2 + size_case_idx * container_size;
			target_control->set_size(control_size);
			REQUIRE_EQ(target_control->get_size(), control_size);
			int size_flag = 2 * size_case_idx - 1; // Map 0~1 to -1~1.

			for (int location_case_idx = 0; location_case_idx < 9; location_case_idx++) {
				Point2i location_flag = Point2i(location_case_idx % 3 - 1, location_case_idx / 3 - 1);
				// Ensure that the numeric type and the get_*_scroll value are of the same type.
				Point2i control_position;
				Point2i expected_scroll;

				for (int axis = 0; axis < 2; axis++) {
					real_t size_offset = (size_flag == -1) ? control_size[axis] / 2 : -control_size[axis] / 4; // The effect of size on offset.
					control_position[axis] = initial_scroll[axis] + size_offset + location_flag[axis] * container_size[axis] * 3;
					// The situation is reversed when the size is large versus when it is small.
					const int com_flag = size_flag * location_flag[axis];
					if (com_flag == 1) {
						expected_scroll[axis] = control_position[axis];
					} else if (com_flag == 0) {
						expected_scroll[axis] = initial_scroll[axis];
					} else if (com_flag == -1) {
						expected_scroll[axis] = control_position[axis] + control_size[axis] - container_size[axis] + scroll_bar_space[axis];
					}
				}
				SUBCASE(location_cases[location_case_idx]) {
					target_control->set_position(control_position);
					REQUIRE_EQ(target_control->get_position(), control_position);

					SUBCASE("Scrolling when grab focus") {
						REQUIRE_EQ(test_container->get_h_scroll(), initial_scroll.x);
						REQUIRE_EQ(test_container->get_v_scroll(), initial_scroll.y);
						REQUIRE_UNARY_FALSE(target_control->has_focus());
						target_control->grab_focus();
						REQUIRE_UNARY(target_control->has_focus());
						CHECK_EQ(test_container->get_h_scroll(), expected_scroll.x);
						CHECK_EQ(test_container->get_v_scroll(), expected_scroll.y);
					}

					SUBCASE("Scrolling when ensure control visible") {
						REQUIRE_EQ(test_container->get_h_scroll(), initial_scroll.x);
						REQUIRE_EQ(test_container->get_v_scroll(), initial_scroll.y);
						test_container->ensure_control_visible(target_control);
						CHECK_EQ(test_container->get_h_scroll(), expected_scroll.x);
						CHECK_EQ(test_container->get_v_scroll(), expected_scroll.y);
					}
				}
			}
		}
	}

	memdelete(target_control);
	memdelete(content_control);
	memdelete(test_container);
}

TEST_CASE("[SceneTree][ScrollContainer] Configuration warnings") {
	ScrollContainer *test_container = memnew(ScrollContainer);
	Window *root = SceneTree::get_root();
	root->add_child(test_container);

	CHECK_MESSAGE(
			test_container->get_configuration_warnings().size() == 1,
			"ScrollContainer reports one configuration warning when it has no content child.");

	Control *child_control_1 = memnew(Control);
	Control *child_control_2 = memnew(Control);
	test_container->add_child(child_control_1);

	CHECK_MESSAGE(
			test_container->get_configuration_warnings().is_empty(),
			"ScrollContainer reports no configuration warning when it has exactly one content child.");

	test_container->add_child(child_control_2);

	CHECK_MESSAGE(
			test_container->get_configuration_warnings().size() == 1,
			"ScrollContainer reports one configuration warning when it has more than one content child.");

	memdelete(child_control_2);
	memdelete(child_control_1);
	memdelete(test_container);
}

} // namespace TestScrollContainer

// === Merged from tests/scene/test_spin_box.cpp ===
/*  test_spin_box.cpp                                                     */
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



#ifndef ADVANCED_GUI_DISABLED

#include "core/typedefs.h"
#include "scene/main/scene_tree.h"

namespace TestSpinBox {

TEST_CASE("[SceneTree][SpinBox] Line edit and range defaults") {
	SpinBox *spin = memnew(SpinBox);
	LineEdit *le = spin->get_line_edit();

	CHECK(le != nullptr);
	CHECK(spin->get_value() == doctest::Approx(0.0));
	CHECK(spin->get_min() == doctest::Approx(0.0));
	CHECK(spin->get_max() == doctest::Approx(100.0));
	CHECK(spin->get_step() == doctest::Approx(1.0));
	CHECK_FALSE(spin->is_greater_allowed());
	CHECK_FALSE(spin->is_lesser_allowed());

	memdelete(spin);
}

TEST_CASE("[SceneTree][SpinBox] Range clamping and allow flags") {
	SpinBox *spin = memnew(SpinBox);
	spin->set_max(10.0);
	spin->set_value(20.0);
	CHECK(spin->get_value() == doctest::Approx(10.0));

	spin->set_allow_greater(true);
	spin->set_value(20.0);
	CHECK(spin->get_value() == doctest::Approx(20.0));

	spin->set_min(0.0);
	spin->set_allow_lesser(false);
	spin->set_value(-5.0);
	CHECK(spin->get_value() == doctest::Approx(0.0));

	spin->set_allow_lesser(true);
	spin->set_value(-3.0);
	CHECK(spin->get_value() == doctest::Approx(-3.0));

	memdelete(spin);
}

TEST_CASE("[SceneTree][SpinBox] Prefix and suffix in line edit text") {
	SpinBox *spin = memnew(SpinBox);
	spin->set_prefix("px");
	spin->set_suffix("deg");
	spin->set_value(42.0);

	Window *root = SceneTree::get_root();
	root->add_child(spin);
	SceneTree::process(0);

	LineEdit *le = spin->get_line_edit();
	const String t = le->get_text();
	CHECK(t.contains("px"));
	CHECK(t.contains("deg"));
	CHECK_FALSE(t.is_empty());
	CHECK(spin->get_value() == doctest::Approx(42.0));

	memdelete(spin);
}

TEST_CASE("[SceneTree][SpinBox] Delegation to inner line edit") {
	SpinBox *spin = memnew(SpinBox);

	spin->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_RIGHT);
	CHECK(spin->get_horizontal_alignment() == HORIZONTAL_ALIGNMENT_RIGHT);

	spin->set_editable(false);
	CHECK_FALSE(spin->is_editable());
	spin->set_editable(true);
	CHECK(spin->is_editable());

	spin->set_select_all_on_focus(true);
	CHECK(spin->is_select_all_on_focus());

	memdelete(spin);
}

TEST_CASE("[SceneTree][SpinBox] update_on_text_changed toggle") {
	SpinBox *spin = memnew(SpinBox);

	CHECK_FALSE(spin->get_update_on_text_changed());
	spin->set_update_on_text_changed(true);
	CHECK(spin->get_update_on_text_changed());
	SceneTree::process(0);
	spin->set_update_on_text_changed(false);
	CHECK_FALSE(spin->get_update_on_text_changed());

	memdelete(spin);
}

TEST_CASE("[SceneTree][SpinBox] apply parses expression") {
	SpinBox *spin = memnew(SpinBox);
	spin->set_max(1000.0);
	LineEdit *le = spin->get_line_edit();

	le->set_text("10+5");
	spin->apply();
	CHECK(spin->get_value() == doctest::Approx(15.0));

	const double before_invalid = spin->get_value();
	le->set_text("___not_an_expression___");
	spin->apply();
	CHECK(spin->get_value() == doctest::Approx(before_invalid));

	memdelete(spin);
}

TEST_CASE("[SceneTree][SpinBox] Custom arrow step and round getters") {
	SpinBox *spin = memnew(SpinBox);

	spin->set_custom_arrow_step(2.5);
	CHECK(spin->get_custom_arrow_step() == doctest::Approx(2.5));

	spin->set_custom_arrow_round(true);
	CHECK(spin->is_custom_arrow_rounding());
	spin->set_custom_arrow_round(false);
	CHECK_FALSE(spin->is_custom_arrow_rounding());

	memdelete(spin);
}

TEST_CASE("[SceneTree][SpinBox] Combined minimum size in tree") {
	SpinBox *spin = memnew(SpinBox);
	Window *root = SceneTree::get_root();
	root->add_child(spin);
	SceneTree::process(0);

	const Size2 cms = spin->get_combined_minimum_size();
	CHECK(cms.width > 0);
	CHECK(cms.height > 0);

	memdelete(spin);
}

} // namespace TestSpinBox

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_split_container.cpp ===
/*  test_split_container.cpp                                              */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestSplitContainer {

#define CHECK_RECTS(m_rects, m_child_rects) \
	CHECK(m_rects.size() == m_child_rects.size()); \
	for (int i = 0; i < (int)m_child_rects.size(); i++) { \
		CHECK_MESSAGE(m_child_rects[i] == m_rects[i], vformat("Child %s is the wrong size. Child rect: %s, expected: %s.", i, m_child_rects[i], m_rects[i])); \
	}

static inline Vector<Rect2> get_rects_multi(SplitContainer *p_sc, const Vector<int> &p_positions, int p_sep, bool p_horizontal = true) {
	// p_positions is the top/left side of the dragger.
	Vector<Rect2> rects;
	int last_pos = 0;
	for (int i = 0; i < (int)p_positions.size() + 1; i++) {
		const int pos = i >= (int)p_positions.size() ? p_sc->get_size()[p_horizontal ? 0 : 1] : p_positions[i];
		Rect2 rect;
		if (p_horizontal) {
			rect.size.y = p_sc->get_size().y;
			rect.position.x = last_pos;
			rect.size.x = pos - last_pos;
		} else {
			rect.size.x = p_sc->get_size().x;
			rect.position.y = last_pos;
			rect.size.y = pos - last_pos;
		}
		rects.push_back(rect);
		last_pos = pos + p_sep;
	}
	return rects;
}

static inline Vector<Rect2> get_rects(SplitContainer *p_sc, int p_position, int p_sep, bool p_horizontal = true) {
	return get_rects_multi(p_sc, Vector<int>({ p_position }), p_sep, p_horizontal);
}

static inline Vector<Rect2> get_rects_multi_rtl(SplitContainer *p_sc, const Vector<int> &p_positions, int p_sep) {
	Vector<Rect2> rects;
	int last_pos = p_sc->get_size().x;
	for (int i = 0; i < (int)p_positions.size() + 1; i++) {
		const int pos = i >= (int)p_positions.size() ? 0 : p_sc->get_size().x - p_positions[i];
		Rect2 rect;
		rect.size.y = p_sc->get_size().y;
		rect.position.x = pos;
		rect.size.x = last_pos - pos;
		rects.push_back(rect);
		last_pos = pos - p_sep;
	}
	return rects;
}

static inline Vector<Rect2> get_rects_rtl(SplitContainer *p_sc, int p_position, int p_sep) {
	return get_rects_multi_rtl(p_sc, Vector<int>({ p_position }), p_sep);
}

static inline Vector<Rect2> get_child_rects(SplitContainer *p_sc) {
	Vector<Rect2> rects;
	for (int i = 0; i < p_sc->get_child_count(false); i++) {
		Control *c = Object::cast_to<Control>(p_sc->get_child(i, false));
		if (!c || !c->is_visible()) {
			continue;
		}
		rects.push_back(c->get_rect());
	}
	return rects;
}

static inline void set_size_flags(SplitContainer *p_sc, Vector<float> p_expand_ratios, bool p_horizontal = true) {
	for (int i = 0; i < p_sc->get_child_count(false); i++) {
		Control *c = Object::cast_to<Control>(p_sc->get_child(i, false));
		if (!c || !c->is_visible()) {
			continue;
		}
		float ratio = p_expand_ratios[i];
		if (p_horizontal) {
			c->set_h_size_flags(ratio > 0 ? Control::SIZE_EXPAND_FILL : Control::SIZE_FILL);
		} else {
			c->set_v_size_flags(ratio > 0 ? Control::SIZE_EXPAND_FILL : Control::SIZE_FILL);
		}
		if (ratio > 0) {
			c->set_stretch_ratio(ratio);
		}
	}
}

TEST_CASE("[SceneTree][SplitContainer] Add and remove children") {
	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_root()->add_child(split_container);

	SUBCASE("[SplitContainer] One child") {
		Control *child_a = memnew(Control);
		split_container->add_child(child_a);

		// One child will fill the entire area.
		CHECK(child_a->get_rect() == split_container->get_rect());

		split_container->set_vertical(true);
		CHECK(child_a->get_rect() == split_container->get_rect());

		memdelete(child_a);
	}

	SUBCASE("[SplitContainer] Preserve split offset") {
		// The split offset is preserved through adding, removing, and changing visibility of children.
		split_container->set_split_offset(100);
		CHECK(split_container->get_split_offset() == 100);

		Control *child_a = memnew(Control);
		split_container->add_child(child_a);
		CHECK(split_container->get_split_offset() == 100);

		Control *child_b = memnew(Control);
		split_container->add_child(child_b);
		CHECK(split_container->get_split_offset() == 100);

		child_a->hide();
		CHECK(split_container->get_split_offset() == 100);

		child_b->hide();
		CHECK(split_container->get_split_offset() == 100);

		child_b->show();
		CHECK(split_container->get_split_offset() == 100);

		child_a->show();
		CHECK(split_container->get_split_offset() == 100);

		split_container->remove_child(child_a);
		CHECK(split_container->get_split_offset() == 100);

		split_container->add_child(child_a);
		CHECK(split_container->get_split_offset() == 100);

		memdelete(child_a);
		memdelete(child_b);
	}

	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] Dragger visibility") {
	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	Control *child_b = memnew(Control);
	split_container->add_child(child_a);
	split_container->add_child(child_b);
	SplitContainerDragger *dragger = Object::cast_to<SplitContainerDragger>(split_container->get_child(2, true));

	split_container->add_theme_constant_override("autohide", 0);

	const int sep_constant = split_container->get_theme_constant("separation");
	const Size2i sep = Size2i(MAX(sep_constant, split_container->get_theme_icon("h_grabber")->get_width()), MAX(sep_constant, split_container->get_theme_icon("v_grabber")->get_height()));

	SUBCASE("[SplitContainer] Visibility based on child count") {
		split_container->remove_child(child_a);
		split_container->remove_child(child_b);

		// No children, not visible.
		CHECK_FALSE(dragger->is_visible());

		// Add one child, not visible.
		split_container->add_child(child_a);
		CHECK_FALSE(dragger->is_visible());

		// Two children, visible.
		split_container->add_child(child_b);
		CHECK(dragger->is_visible());

		// Remove a child, not visible.
		split_container->remove_child(child_b);
		CHECK_FALSE(dragger->is_visible());
	}

	SUBCASE("[SplitContainer] Set dragger visibility") {
		split_container->set_dragger_visibility(SplitContainer::DRAGGER_HIDDEN);
		CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
		// Can't check the visibility since it happens in draw.

		split_container->set_dragger_visibility(SplitContainer::DRAGGER_HIDDEN_COLLAPSED);
		CHECK_RECTS(get_rects(split_container, 0, 0), get_child_rects(split_container));

		split_container->set_dragger_visibility(SplitContainer::DRAGGER_VISIBLE);
		CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
	}

	SUBCASE("[SplitContainer] Not visible when collapsed") {
		split_container->set_collapsed(true);
		CHECK_FALSE(dragger->is_visible());

		split_container->set_collapsed(false);
		CHECK(dragger->is_visible());
	}

	memdelete(child_a);
	memdelete(child_b);
	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] Collapsed") {
	DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();

	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	split_container->add_child(child_a);
	Control *child_b = memnew(Control);
	split_container->add_child(child_b);

	const int sep_constant = split_container->get_theme_constant("separation");
	const Size2i sep = Size2i(MAX(sep_constant, split_container->get_theme_icon("h_grabber")->get_width()), MAX(sep_constant, split_container->get_theme_icon("v_grabber")->get_height()));

	SUBCASE("[SplitContainer] Dragging and cursor") {
		split_container->set_collapsed(true);

		// Cursor is default.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(1, 1), MouseButtonMask::NONE, Key::NONE);
		CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

		// Dragger is disabled, cannot drag.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(1, 1), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offset() == 0);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(10, 1), MouseButtonMask::LEFT, Key::NONE);
		CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offset() == 0);
	}

	SUBCASE("[SplitContainer] No expand flags") {
		int def_pos = 0;

		split_container->set_split_offset(10);
		CHECK_RECTS(get_rects(split_container, def_pos + 10, sep.x), get_child_rects(split_container));

		split_container->set_collapsed(true);

		// The split offset is treated as 0 when collapsed.
		CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offset() == 10);
	}

	SUBCASE("[SplitContainer] First child expanded") {
		int def_pos = split_container->get_size().x - sep.x;
		child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		split_container->set_split_offset(-10);

		CHECK_RECTS(get_rects(split_container, def_pos - 10, sep.x), get_child_rects(split_container));

		split_container->set_collapsed(true);

		// The split offset is treated as 0 when collapsed.
		CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offset() == -10);
	}

	SUBCASE("[SplitContainer] Second child expanded") {
		int def_pos = 0;
		child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		split_container->set_split_offset(10);

		CHECK_RECTS(get_rects(split_container, def_pos + 10, sep.x), get_child_rects(split_container));

		split_container->set_collapsed(true);

		// The split offset is treated as 0 when collapsed.
		CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offset() == 10);
	}

	SUBCASE("[SplitContainer] Both children expanded") {
		int def_pos = (split_container->get_size().y - sep.y) / 2;
		child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		split_container->set_split_offset(10);

		CHECK_RECTS(get_rects(split_container, def_pos + 10, sep.x), get_child_rects(split_container));

		split_container->set_collapsed(true);

		// The split offset is treated as 0 when collapsed.
		CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offset() == 10);
	}

	memdelete(child_a);
	memdelete(child_b);
	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] Cursor shape") {
	DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();

	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	split_container->add_child(child_a);
	Control *child_b = memnew(Control);
	split_container->add_child(child_b);

	Point2 on_dragger = Point2(1, 1);
	Point2 not_on_dragger = Point2(50, 50);

	// Default cursor shape.
	SEND_GUI_MOUSE_MOTION_EVENT(not_on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

	// Horizontal cursor shape.
	SEND_GUI_MOUSE_MOTION_EVENT(on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_HSPLIT);

	// Vertical cursor shape.
	split_container->set_vertical(true);
	SEND_GUI_MOUSE_MOTION_EVENT(on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_VSPLIT);

	// Move off, default cursor shape.
	SEND_GUI_MOUSE_MOTION_EVENT(not_on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

	memdelete(child_a);
	memdelete(child_b);
	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] Two children") {
	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	Control *child_b = memnew(Control);
	split_container->add_child(child_a);
	split_container->add_child(child_b);

	const int sep_constant = split_container->get_theme_constant("separation");
	const Size2i sep = Size2i(MAX(sep_constant, split_container->get_theme_icon("h_grabber")->get_width()), MAX(sep_constant, split_container->get_theme_icon("v_grabber")->get_height()));

	SUBCASE("[SplitContainer] Minimum size") {
		// Minimum size is the sum of both children's minimum sizes and the separator depending on the vertical axis.
		child_a->set_custom_minimum_size(Size2(100, 200));
		child_b->set_custom_minimum_size(Size2(100, 200));

		Size2 min_size = split_container->get_minimum_size();
		CHECK(min_size.x == 200 + sep.x);
		CHECK(min_size.y == 200);

		split_container->set_vertical(true);
		min_size = split_container->get_minimum_size();
		CHECK(min_size.x == 100);
		CHECK(min_size.y == 400 + sep.y);
	}

	SUBCASE("[SplitContainer] Default position") {
		SUBCASE("[SplitContainer] Vertical") {
			// Make sure clamping the split offset doesn't change it or the position.
			split_container->set_vertical(true);

			// No expand flags set.
			int def_pos = 0;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));

			// First expand flags set.
			child_a->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_v_size_flags(Control::SIZE_FILL);
			def_pos = split_container->get_size().y - sep.y;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));

			// Second expand flags set.
			child_a->set_v_size_flags(Control::SIZE_FILL);
			child_b->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			def_pos = 0;
			CHECK_RECTS(get_rects(split_container, 0, sep.y, false), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));

			// Both expand flags set.
			child_a->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			def_pos = (split_container->get_size().y - sep.y) / 2;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));

			// Unequal stretch ratios.
			child_a->set_stretch_ratio(2.0);
			def_pos = (split_container->get_size().y * 2 / 3) - sep.y / 2;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.y, false), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Right to left") {
			split_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
			split_container->set_position(Point2(0, 0));

			// No expand flags set.
			int def_pos = 0;
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));

			// First expand flags set.
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_FILL);
			def_pos = split_container->get_size().y - sep.y;
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));

			// Second expand flags set.
			child_a->set_h_size_flags(Control::SIZE_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			def_pos = 0;
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));

			// Both expand flags set.
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			def_pos = (split_container->get_size().y - sep.y) / 2;
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));

			// Unequal stretch ratios.
			child_a->set_stretch_ratio(2.0);
			def_pos = (split_container->get_size().y * 2 / 3) - sep.y / 2;
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] No expand flags") {
			int def_pos = 0;

			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			def_pos = 400;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == def_pos);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			def_pos = split_container->get_size().x - 400 - sep.x;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == def_pos);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			def_pos = 200;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == def_pos);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] First child expanded") {
			const int def_pos = split_container->get_size().x - sep.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			int pos = split_container->get_size().x - 400 - sep.x;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			pos = 200;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			def_pos = 400;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == def_pos);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			def_pos = 500 - 400 - sep.x;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == def_pos);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			def_pos = 200;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == def_pos);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Both children expanded") {
			const int def_pos = (split_container->get_size().x - sep.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			int pos = 400;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			pos = split_container->get_size().x - 400 - sep.x;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			pos = 200;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Unequal stretch ratios") {
			const int def_pos = (split_container->get_size().x * 2 / 3) - sep.x / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_a->set_stretch_ratio(2.0);

			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == 0);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			int pos = 400;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			pos = split_container->get_size().x - 400 - sep.x;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			pos = 200;
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			CHECK_RECTS(get_rects(split_container, pos, sep.x), get_child_rects(split_container));
		}
	}

	SUBCASE("[SplitContainer] Set split offset") {
		SUBCASE("[SplitContainer] Right to left") {
			split_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
			split_container->set_position(Point2(0, 0));
			int def_pos = 0;
			// Positive.
			split_container->set_split_offset(10);
			CHECK(split_container->get_split_offset() == 10);
			CHECK_RECTS(get_rects_rtl(split_container, def_pos + 10, sep.y), get_child_rects(split_container));

			// Negative.
			split_container->set_split_offset(-10);
			CHECK(split_container->get_split_offset() == -10);
			CHECK_RECTS(get_rects_rtl(split_container, def_pos, sep.y), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] No expand flags") {
			int def_pos = 0;

			// Positive.
			split_container->set_split_offset(10);
			CHECK(split_container->get_split_offset() == 10);
			CHECK_RECTS(get_rects(split_container, def_pos + 10, sep.x), get_child_rects(split_container));

			// Negative.
			split_container->set_split_offset(-10);
			CHECK(split_container->get_split_offset() == -10);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Clamped.
			split_container->set_split_offset(1000);
			CHECK(split_container->get_split_offset() == 1000);
			CHECK_RECTS(get_rects(split_container, split_container->get_size().x - sep.x, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] First child expanded") {
			int def_pos = split_container->get_size().x - sep.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			// Positive.
			split_container->set_split_offset(10);
			CHECK(split_container->get_split_offset() == 10);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Negative.
			split_container->set_split_offset(-10);
			CHECK(split_container->get_split_offset() == -10);
			CHECK_RECTS(get_rects(split_container, def_pos - 10, sep.x), get_child_rects(split_container));

			// Clamped.
			split_container->set_split_offset(-1000);
			CHECK(split_container->get_split_offset() == -1000);
			CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			// Positive.
			split_container->set_split_offset(10);
			CHECK(split_container->get_split_offset() == 10);
			CHECK_RECTS(get_rects(split_container, def_pos + 10, sep.x), get_child_rects(split_container));

			// Negative.
			split_container->set_split_offset(-10);
			CHECK(split_container->get_split_offset() == -10);
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Clamped.
			split_container->set_split_offset(1000);
			CHECK(split_container->get_split_offset() == 1000);
			CHECK_RECTS(get_rects(split_container, split_container->get_size().x - sep.x, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Both children expanded") {
			int def_pos = (split_container->get_size().x - sep.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			// Positive.
			split_container->set_split_offset(10);
			CHECK(split_container->get_split_offset() == 10);
			CHECK_RECTS(get_rects(split_container, def_pos + 10, sep.x), get_child_rects(split_container));

			// Negative.
			split_container->set_split_offset(-10);
			CHECK(split_container->get_split_offset() == -10);
			CHECK_RECTS(get_rects(split_container, def_pos - 10, sep.x), get_child_rects(split_container));

			// Clamped positive.
			split_container->set_split_offset(1000);
			CHECK(split_container->get_split_offset() == 1000);
			CHECK_RECTS(get_rects(split_container, split_container->get_size().x - sep.x, sep.x), get_child_rects(split_container));

			// Clamped negative.
			split_container->set_split_offset(-1000);
			CHECK(split_container->get_split_offset() == -1000);
			CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Unequal stretch ratios") {
			int def_pos = (split_container->get_size().x * 2 / 3) - sep.x / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_a->set_stretch_ratio(2.0);

			// Positive.
			split_container->set_split_offset(10);
			CHECK(split_container->get_split_offset() == 10);
			CHECK_RECTS(get_rects(split_container, def_pos + 10, sep.x), get_child_rects(split_container));

			// Negative.
			split_container->set_split_offset(-10);
			CHECK(split_container->get_split_offset() == -10);
			CHECK_RECTS(get_rects(split_container, def_pos - 10, sep.x), get_child_rects(split_container));

			// Clamped positive.
			split_container->set_split_offset(1000);
			CHECK(split_container->get_split_offset() == 1000);
			CHECK_RECTS(get_rects(split_container, split_container->get_size().x - sep.x, sep.x), get_child_rects(split_container));

			// Clamped negative.
			split_container->set_split_offset(-1000);
			CHECK(split_container->get_split_offset() == -1000);
			CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
		}
	}

	SUBCASE("[SplitContainer] Keep split offset when changing minimum size") {
		SUBCASE("[SplitContainer] No expand flags") {
			int def_pos = 0;

			split_container->set_split_offset(100);
			child_a->set_custom_minimum_size(Size2(10, 0));
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));

			child_a->set_custom_minimum_size(Size2(50, 0));
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] First child expanded") {
			int def_pos = split_container->get_size().x - sep.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			split_container->set_split_offset(-100);
			child_b->set_custom_minimum_size(Size2(10, 0));
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects(split_container, def_pos - 100, sep.x), get_child_rects(split_container));

			child_b->set_custom_minimum_size(Size2(50, 0));
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects(split_container, def_pos - 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			split_container->set_split_offset(100);
			child_a->set_custom_minimum_size(Size2(10, 0));
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));

			child_a->set_custom_minimum_size(Size2(50, 0));
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Both children expanded") {
			int def_pos = (split_container->get_size().x - sep.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			split_container->set_split_offset(20);
			child_a->set_custom_minimum_size(Size2(10, 0));
			child_b->set_custom_minimum_size(Size2(10, 0));
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));

			child_a->set_custom_minimum_size(Size2(50, 0));
			child_b->set_custom_minimum_size(Size2(50, 0));
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));
		}
	}

	SUBCASE("[SplitContainer] Keep split offset when changing visibility") {
		SUBCASE("[SplitContainer] No expand flags") {
			int def_pos = 0;
			split_container->set_split_offset(100);

			child_a->hide();
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->hide();
			CHECK(split_container->get_split_offset() == 100);

			child_a->show();
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->show();
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] First child expanded") {
			int def_pos = split_container->get_size().x - sep.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			split_container->set_split_offset(-100);

			child_a->hide();
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->hide();
			CHECK(split_container->get_split_offset() == -100);

			child_a->show();
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->show();
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects(split_container, def_pos - 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			split_container->set_split_offset(100);

			child_a->hide();
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->hide();
			CHECK(split_container->get_split_offset() == 100);

			child_a->show();
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->show();
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Both children expanded") {
			int def_pos = (split_container->get_size().x - sep.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			split_container->set_split_offset(20);

			child_a->hide();
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->hide();
			CHECK(split_container->get_split_offset() == 20);

			child_a->show();
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			child_b->show();
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));
		}
	}

	SUBCASE("[SplitContainer] Keep split offset when removing children") {
		SUBCASE("[SplitContainer] No expand flags") {
			int def_pos = 0;
			split_container->set_split_offset(100);

			split_container->remove_child(child_a);
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->remove_child(child_b);
			CHECK(split_container->get_split_offset() == 100);

			split_container->add_child(child_a);
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->add_child(child_b);
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] First child expanded") {
			int def_pos = split_container->get_size().x - sep.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			split_container->set_split_offset(-100);

			split_container->remove_child(child_a);
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->remove_child(child_b);
			CHECK(split_container->get_split_offset() == -100);

			split_container->add_child(child_a);
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->add_child(child_b);
			CHECK(split_container->get_split_offset() == -100);
			CHECK_RECTS(get_rects(split_container, def_pos - 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			split_container->set_split_offset(100);

			split_container->remove_child(child_a);
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->remove_child(child_b);
			CHECK(split_container->get_split_offset() == 100);

			split_container->add_child(child_a);
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->add_child(child_b);
			CHECK(split_container->get_split_offset() == 100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] Both children expanded") {
			int def_pos = (split_container->get_size().x - sep.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			split_container->set_split_offset(20);

			split_container->remove_child(child_a);
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->remove_child(child_b);
			CHECK(split_container->get_split_offset() == 20);

			split_container->add_child(child_a);
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects_multi(split_container, Vector<int>(), sep.x), get_child_rects(split_container));

			split_container->add_child(child_b);
			CHECK(split_container->get_split_offset() == 20);
			CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));
		}
	}

	SUBCASE("[SplitContainer] Keep split offset when changing expand flags") {
		int def_pos = 0;
		split_container->set_split_offset(20);
		CHECK(split_container->get_split_offset() == 20);
		CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));

		child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		def_pos = split_container->get_size().x - sep.x;
		CHECK(split_container->get_split_offset() == 20);
		CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

		child_a->set_h_size_flags(Control::SIZE_FILL);
		child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		def_pos = 0;
		CHECK(split_container->get_split_offset() == 20);
		CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));

		child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		def_pos = (split_container->get_size().x - sep.x) / 2;
		CHECK(split_container->get_split_offset() == 20);
		CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));

		child_a->set_h_size_flags(Control::SIZE_FILL);
		child_b->set_h_size_flags(Control::SIZE_FILL);
		def_pos = 0;
		CHECK(split_container->get_split_offset() == 20);
		CHECK_RECTS(get_rects(split_container, def_pos + 20, sep.x), get_child_rects(split_container));
	}

	SUBCASE("[SplitContainer] Keep split offset when moving children") {
		int def_pos = 0;
		split_container->set_split_offset(100);

		split_container->move_child(child_a, 1);
		CHECK(split_container->get_split_offset() == 100);
		CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
	}

	SUBCASE("[SplitContainer] Resize split container") {
		SUBCASE("[SplitContainer] No expand flags") {
			int def_pos = 0;
			// Increase the size.
			split_container->set_size(Size2(600, 500));
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offset(100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));

			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the first child changes size.
			split_container->set_size(Size2(80, 500));
			CHECK_RECTS(get_rects(split_container, 80 - sep.x, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);
		}

		SUBCASE("[SplitContainer] First child expanded") {
			int def_pos = split_container->get_size().x - sep.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			def_pos = split_container->get_size().x - sep.x;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			def_pos = split_container->get_size().x - sep.x;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offset(-100);
			CHECK_RECTS(get_rects(split_container, def_pos - 100, sep.x), get_child_rects(split_container));

			split_container->set_size(Size2(500, 500));
			def_pos = split_container->get_size().x - sep.x;
			CHECK_RECTS(get_rects(split_container, def_pos - 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == -100);

			// Change size so that the second child changes size.
			split_container->set_size(Size2(80, 500));
			CHECK_RECTS(get_rects(split_container, 0, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == -100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects(split_container, def_pos - 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == -100);
		}

		SUBCASE("[SplitContainer] Second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offset(100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));

			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the first child changes size.
			split_container->set_size(Size2(80, 500));
			CHECK_RECTS(get_rects(split_container, 80 - sep.x, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);
		}

		SUBCASE("[SplitContainer] Both children expanded") {
			int def_pos = (split_container->get_size().x - sep.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			def_pos = (split_container->get_size().x - sep.x) / 2;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			def_pos = (split_container->get_size().x - sep.x) / 2;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offset(100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));

			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x - sep.x) / 2;
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the second child is minimized.
			split_container->set_size(Size2(80, 500));
			CHECK_RECTS(get_rects(split_container, 80 - sep.x, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x - sep.x) / 2;
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);
		}

		SUBCASE("[SplitContainer] Unequal stretch ratios") {
			int def_pos = (split_container->get_size().x * 2 / 3) - sep.x / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_a->set_stretch_ratio(2.0);

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - sep.x / 2;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - sep.x / 2;
			CHECK_RECTS(get_rects(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offset(100);
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));

			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - sep.x / 2;
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the second child is minimized.
			split_container->set_size(Size2(80, 500));
			CHECK_RECTS(get_rects(split_container, 80 - sep.x, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - sep.x / 2;
			CHECK_RECTS(get_rects(split_container, def_pos + 100, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == 100);
		}
	}

	SUBCASE("[SplitContainer] Drag") {
		SUBCASE("[SplitContainer] Vertical, no expand flags") {
			SIGNAL_WATCH(split_container, "dragged");
			Array signal_args = { { 0 } };

			split_container->set_vertical(true);
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = 0;
			int split_dragger_ofs = 0;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.y, false), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			SIGNAL_CHECK_FALSE("dragged");

			// Move the dragger.
			dragger_pos = 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.y, false), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Move down.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.y, false), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().y - sep.y;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, 1000), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.y, false), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Move up.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.y, false), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.y, false), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			SIGNAL_CHECK_FALSE("dragged");

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, 200), MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.y, false), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			SIGNAL_CHECK_FALSE("dragged");

			SIGNAL_UNWATCH(split_container, "dragged");
		}

		SUBCASE("[SplitContainer] No expand flags") {
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = 0;
			int split_dragger_ofs = 0;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos = 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - sep.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}

		SUBCASE("[SplitContainer] First child expanded") {
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = split_container->get_size().x - sep.x;
			int split_dragger_ofs = -dragger_pos;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos -= 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - sep.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}

		SUBCASE("[SplitContainer] Second child expanded") {
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = 0;
			int split_dragger_ofs = 0;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos = 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - sep.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}

		SUBCASE("[SplitContainer] Both children expanded") {
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = (split_container->get_size().x - sep.x) / 2;
			int split_dragger_ofs = -dragger_pos;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos += 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - sep.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			CHECK_RECTS(get_rects(split_container, dragger_pos, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}
	}

	memdelete(child_b);
	memdelete(child_a);
	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] More children") {
	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	Control *child_b = memnew(Control);
	Control *child_c = memnew(Control);
	split_container->add_child(child_a);
	split_container->add_child(child_b);
	split_container->add_child(child_c);
	Size2i min_size = Size2i(10, 10);
	child_a->set_custom_minimum_size(min_size);
	child_b->set_custom_minimum_size(min_size);
	child_c->set_custom_minimum_size(min_size);

	const int sep_constant = split_container->get_theme_constant("separation");
	const Size2i sep = Size2i(MAX(sep_constant, split_container->get_theme_icon("h_grabber")->get_width()), MAX(sep_constant, split_container->get_theme_icon("v_grabber")->get_height()));

	SUBCASE("[SplitContainer] Duplicate") {
		// Make sure dynamically added internal draggers duplicate properly.
		SplitContainer *duplicate = (SplitContainer *)(Node *)split_container->duplicate();
		SceneTree::get_root()->add_child(duplicate);
		CHECK(duplicate->get_child_count(false) == split_container->get_child_count(false));
		CHECK(duplicate->get_child_count(true) == split_container->get_child_count(true));
		memdelete(duplicate);
	}

	SUBCASE("[SplitContainer] Default position") {
		CHECK(split_container->get_split_offsets() == Vector<int>({ 0, 0 }));

		set_size_flags(split_container, { -1, -1, -1 }); // None expanded.
		Vector<int> def_pos = { min_size.x, min_size.x * 2 + sep.x };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ min_size.x, min_size.x * 2 + sep.x }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { 1, -1, -1 }); // First expanded.
		def_pos = { (int)split_container->get_size().x - sep.x * 2 - min_size.x * 2, (int)split_container->get_size().x - sep.x - min_size.x };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ -min_size.x * 2 - sep.x, -min_size.x }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { -1, 1, -1 }); // Second expanded.
		def_pos = { min_size.x, (int)split_container->get_size().x - min_size.x - sep.x };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ min_size.x, -min_size.x }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { -1, -1, 1 }); // Third expanded.
		def_pos = { min_size.x, min_size.x * 2 + sep.x };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ min_size.x, min_size.x * 2 + sep.x }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { 1, 1, -1 }); // First and second expanded.
		int child_2_expanded_size = ((int)split_container->get_size().x - min_size.x) / 2 - sep.x;
		def_pos = { child_2_expanded_size, (int)split_container->get_size().x - min_size.x - sep.x };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ 0, -min_size.x }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { 1, -1, 1 }); // First and third expanded.
		def_pos = { child_2_expanded_size, child_2_expanded_size + min_size.x + sep.x };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ 0, 0 }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { -1, 1, 1 }); // Second and third expanded.
		def_pos = { min_size.x, min_size.x + child_2_expanded_size + sep.x };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ min_size.x, 0 }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { 1, 1, 1 }); // All expanded.
		int child_3_expanded_size = (split_container->get_size().x - sep.x * 2) / 3;
		// Add 1 due to pixel error accumulation.
		def_pos = { child_3_expanded_size, child_3_expanded_size * 2 + sep.x + 1 };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ 0, 0 }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });

		set_size_flags(split_container, { 1, 2, 3 }); // All expanded, different ratios.
		int child_6_expanded_size = (split_container->get_size().x - sep.x * 2) / 6;
		def_pos = { child_6_expanded_size, child_6_expanded_size * 3 + sep.x + 1 };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->clamp_split_offset();
		CHECK(split_container->get_split_offsets() == Vector<int>({ 0, 0 }));
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		split_container->set_split_offsets({ 0, 0 });
	}

	SUBCASE("[SplitContainer] Set split offset") {
		const int expanded_single_size = (int)split_container->get_size().x - min_size.x * 2 - sep.x * 2;

		SUBCASE("[SplitContainer] No expand flags") {
			set_size_flags(split_container, { -1, -1, -1 }); // None expanded.

			// First is positive.
			split_container->set_split_offsets({ 50, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { 50, 50 + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// Second is positive.
			split_container->set_split_offsets({ 0, 50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, 50 });
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, 50 }, sep.x), get_child_rects(split_container));

			// Both are positive and equal, the first will override since they both start at 0.
			split_container->set_split_offsets({ 50, 50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 50 });
			CHECK_RECTS(get_rects_multi(split_container, { 50, 50 + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// Both are negative and clamped.
			split_container->set_split_offsets({ -50, -50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ -50, -50 });
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, min_size.x * 2 + sep.x }, sep.x), get_child_rects(split_container));

			// First positive, second negative. First takes priority.
			split_container->set_split_offsets({ 50, -50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, -50 });
			CHECK_RECTS(get_rects_multi(split_container, { 50, 50 + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// First is clamped and pushes second to the end.
			split_container->set_split_offsets({ 1000, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 1000, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { expanded_single_size, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// Second is clamped.
			split_container->set_split_offsets({ 0, 1000 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, 1000 });
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// Both are clamped positively, first one takes priority.
			split_container->set_split_offsets({ 1000, 1000 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 1000, 1000 });
			CHECK_RECTS(get_rects_multi(split_container, { expanded_single_size, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] First child expanded") {
			set_size_flags(split_container, { 1, -1, -1 }); // First expanded.

			// First is positive and clamped.
			split_container->set_split_offsets({ 50, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { expanded_single_size, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// Second is positive and clamped.
			split_container->set_split_offsets({ 0, 50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, 50 });
			CHECK_RECTS(get_rects_multi(split_container, { expanded_single_size, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// First is negative and moves left.
			split_container->set_split_offsets({ -50, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ -50, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { (int)split_container->get_size().x - 50 - sep.x, (int)split_container->get_size().x - min_size.x - sep.x }, sep.x), get_child_rects(split_container));

			// Second is negative, but first has priority so it doesn't move.
			split_container->set_split_offsets({ 0, -50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, -50 });
			CHECK_RECTS(get_rects_multi(split_container, { expanded_single_size, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// Both are negative and equal, they move left but the second doesn't move as much as wanted.
			split_container->set_split_offsets({ -50, -50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ -50, -50 });
			CHECK_RECTS(get_rects_multi(split_container, { (int)split_container->get_size().x - 50 - sep.x, (int)split_container->get_size().x - 50 + min_size.x }, sep.x), get_child_rects(split_container));

			// Both are negative with space and move left.
			split_container->set_split_offsets({ -100, -50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ -100, -50 });
			CHECK_RECTS(get_rects_multi(split_container, { (int)split_container->get_size().x - 100 - sep.x, (int)split_container->get_size().x - 50 - sep.x }, sep.x), get_child_rects(split_container));

			// First moves all the way left.
			split_container->set_split_offsets({ -1000, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ -1000, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// Second cannot move all the way left since first takes priority.
			split_container->set_split_offsets({ 0, -1000 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, -1000 });
			CHECK_RECTS(get_rects_multi(split_container, { expanded_single_size, expanded_single_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));

			// First and second move all the way left.
			split_container->set_split_offsets({ -1000, -1000 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ -1000, -1000 });
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, min_size.x * 2 + sep.x }, sep.x), get_child_rects(split_container));
		}

		SUBCASE("[SplitContainer] All children expanded") {
			set_size_flags(split_container, { 1, 1, 1 }); // All expanded.
			const int child_3_expanded_size = (split_container->get_size().x - sep.x * 2) / 3;

			// First is moved positive, does not affect second.
			split_container->set_split_offsets({ 50, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { child_3_expanded_size + 50, child_3_expanded_size * 2 + sep.x + 1 }, sep.x), get_child_rects(split_container));

			// First is moved negative, does not affect second.
			split_container->set_split_offsets({ -50, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ -50, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { child_3_expanded_size - 50, child_3_expanded_size * 2 + sep.x + 1 }, sep.x), get_child_rects(split_container));

			// Second is moved positive, does not affect first.
			split_container->set_split_offsets({ 0, 50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, 50 });
			CHECK_RECTS(get_rects_multi(split_container, { child_3_expanded_size, child_3_expanded_size * 2 + 50 + sep.x + 1 }, sep.x), get_child_rects(split_container));

			// Second is moved negative, does not affect first.
			split_container->set_split_offsets({ 0, -50 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, -50 });
			CHECK_RECTS(get_rects_multi(split_container, { child_3_expanded_size, child_3_expanded_size * 2 - 50 + sep.x + 1 }, sep.x), get_child_rects(split_container));

			// First is moved positive enough to affect second.
			split_container->set_split_offsets({ 200, 0 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 200, 0 });
			CHECK_RECTS(get_rects_multi(split_container, { child_3_expanded_size + 200, child_3_expanded_size + 200 + sep.x + min_size.x }, sep.x), get_child_rects(split_container));

			// Second is moved enough to pass the first, but the first has priority.
			split_container->set_split_offsets({ 0, -200 });
			CHECK(split_container->get_split_offsets() == Vector<int>{ 0, -200 });
			CHECK_RECTS(get_rects_multi(split_container, { child_3_expanded_size, child_3_expanded_size + min_size.x + sep.x }, sep.x), get_child_rects(split_container));
		}
	}

	SUBCASE("[SplitContainer] Resize") {
		SUBCASE("[SplitContainer] No expand flags") {
			Vector<int> def_pos = { min_size.x, min_size.x * 2 + sep.x };
			// Increase the size.
			split_container->set_size(Size2(600, 500));
			CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offsets({ 50, 100 });
			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects_multi(split_container, { 50, 100 }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 100 });

			// Change size so that the second child gets clamped and changes size.
			split_container->set_size(Size2(100, 500));
			CHECK_RECTS(get_rects_multi(split_container, { 50, 100 - sep.x - min_size.x }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 100 });

			// Change size so that the first child changes size.
			split_container->set_size(Size2(60, 500));
			CHECK_RECTS(get_rects_multi(split_container, { 60 - min_size.x * 2 - sep.x * 2, 60 - sep.x - min_size.x }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 100 });

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects_multi(split_container, { 50, 100 }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ 50, 100 });
		}

		SUBCASE("[SplitContainer] First child expanded") {
			set_size_flags(split_container, { 1, -1, -1 });
			Vector<int> def_pos = { (int)split_container->get_size().x - sep.x * 2 - min_size.x, (int)split_container->get_size().x - sep.x };
			// Increase the size.
			split_container->set_size(Size2(600, 500));
			def_pos = { (int)split_container->get_size().x - sep.x * 2 - min_size.x * 2, (int)split_container->get_size().x - sep.x - min_size.x };
			CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			def_pos = { (int)split_container->get_size().x - sep.x * 2 - min_size.x * 2, (int)split_container->get_size().x - sep.x - min_size.x };
			CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offsets({ -100, -50 });
			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects_multi(split_container, { (int)split_container->get_size().x - 100 - sep.x, (int)split_container->get_size().x - 50 - sep.x }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ -100, -50 });

			// Change size so that the first child gets clamped and changes size.
			split_container->set_size(Size2(100, 500));
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, (int)split_container->get_size().x - 50 - sep.x }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ -100, -50 });

			// Change size so that the second child changes size.
			split_container->set_size(Size2(50, 500));
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, min_size.x * 2 + sep.x }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ -100, -50 });

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			CHECK_RECTS(get_rects_multi(split_container, { (int)split_container->get_size().x - 100 - sep.x, (int)split_container->get_size().x - 50 - sep.x }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ -100, -50 });
		}

		SUBCASE("[SplitContainer] All children expanded") {
			set_size_flags(split_container, { 1, 1, 1 });
			// Increase the size.
			split_container->set_size(Size2(600, 500));
			int child_3_expanded_size = (split_container->get_size().x - sep.x * 2) / 3;
			Vector<int> def_pos = { child_3_expanded_size, child_3_expanded_size * 2 + sep.x };
			CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			child_3_expanded_size = (split_container->get_size().x - sep.x * 2) / 3;
			def_pos = { child_3_expanded_size, child_3_expanded_size * 2 + sep.x };
			CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

			// Change size with a split offset.
			split_container->set_split_offsets({ -50, 50 });
			split_container->set_size(Size2(500, 500));
			child_3_expanded_size = (split_container->get_size().x - sep.x * 2) / 3;
			def_pos = { child_3_expanded_size, child_3_expanded_size * 2 + sep.x + 1 };
			CHECK_RECTS(get_rects_multi(split_container, { def_pos[0] - 50, def_pos[1] + 50 }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ -50, 50 });

			// Change size so that the children get clamped and change sizes.
			split_container->set_size(Size2(100, 500));
			CHECK_RECTS(get_rects_multi(split_container, { min_size.x, (int)split_container->get_size().x - sep.x - min_size.x }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ -50, 50 });

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			child_3_expanded_size = (split_container->get_size().x - sep.x * 2) / 3;
			def_pos = { child_3_expanded_size, child_3_expanded_size * 2 + sep.x + 1 };
			CHECK_RECTS(get_rects_multi(split_container, { def_pos[0] - 50, def_pos[1] + 50 }, sep.x), get_child_rects(split_container));
			CHECK(split_container->get_split_offsets() == Vector<int>{ -50, 50 });
		}
	}

	SUBCASE("[SplitContainer] Visibility changes") {
		set_size_flags(split_container, { -1, -1, -1 }); // None expanded.
		split_container->set_split_offsets({ 50, 122 });
		Vector<int> def_pos = { 50, 122 };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

		// Hide and show the first child.
		child_a->set_visible(false);
		CHECK(split_container->get_split_offsets() == Vector<int>({ 60 }));
		CHECK_RECTS(get_rects_multi(split_container, { 60 }, sep.x), get_child_rects(split_container));

		child_a->set_visible(true);
		CHECK(split_container->get_split_offsets() == def_pos);
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

		// Hide and show the second child.
		child_b->set_visible(false);
		CHECK(split_container->get_split_offsets() == Vector<int>({ 50 }));
		CHECK_RECTS(get_rects_multi(split_container, { 50 }, sep.x), get_child_rects(split_container));
		child_b->set_visible(true);
		CHECK(split_container->get_split_offsets() == def_pos);
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

		// Hide and show the last child.
		child_c->set_visible(false);
		CHECK(split_container->get_split_offsets() == Vector<int>({ 50 }));
		CHECK_RECTS(get_rects_multi(split_container, { 50 }, sep.x), get_child_rects(split_container));
		child_c->set_visible(true);
		CHECK(split_container->get_split_offsets() == def_pos);
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

		set_size_flags(split_container, { 1, 1, 1 }); // All expanded.
		split_container->set_split_offsets({ 50, 60 });
		int child_3_expanded_size = (split_container->get_size().x - sep.x * 2) / 3;
		def_pos = { child_3_expanded_size + 50, child_3_expanded_size * 2 + sep.x + 1 + 60 };
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));

		// Hide and show the first child.
		int child_2_expanded_size = (split_container->get_size().x - sep.x) / 2;
		child_a->set_visible(false);
		int half_point = (split_container->get_size().x - def_pos[0]) / 2 - sep.x;
		int so = child_3_expanded_size + 11 - half_point; // 11 is from 60 - 50 + 1 to get the second child's size.
		CHECK_RECTS(get_rects_multi(split_container, { child_2_expanded_size + so }, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offsets() == Vector<int>({ so }));
		child_a->set_visible(true);
		CHECK_RECTS(get_rects_multi(split_container, def_pos, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offsets() == Vector<int>({ 50, 60 }));

		// Hide and show the second child.
		child_b->set_visible(false);
		half_point = (split_container->get_size().x - (def_pos[1] - def_pos[0] - sep.x)) / 2 - sep.x + 1;
		so = def_pos[0] - half_point;
		CHECK_RECTS(get_rects_multi(split_container, { child_2_expanded_size + so }, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offsets() == Vector<int>({ so }));
		child_b->set_visible(true);
		// There is lost precision due to SplitContainer using ints, so this is off by one.
		CHECK_RECTS(get_rects_multi(split_container, { def_pos[0] - 1, def_pos[1] - 1 }, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offsets() == Vector<int>({ 49, 59 }));

		// Hide and show the last child.
		split_container->set_split_offsets({ 50, 60 });
		child_c->set_visible(false);
		half_point = (def_pos[1] - sep.x) / 2 + 1;
		so = def_pos[0] - half_point;
		CHECK_RECTS(get_rects_multi(split_container, { child_2_expanded_size + so }, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offsets() == Vector<int>({ so }));
		child_c->set_visible(true);
		CHECK_RECTS(get_rects_multi(split_container, { def_pos[0] - 1, def_pos[1] - 1 }, sep.x), get_child_rects(split_container));
		CHECK(split_container->get_split_offsets() == Vector<int>({ 49, 59 }));
	}

	SUBCASE("[SplitContainer] Adjust split offset when moving children") {
		split_container->set_split_offsets({ 50, 80 });
		split_container->move_child(child_a, 1);
		Vector<int> pos = { 30 - sep.x, 80 }; // 30 = 80 - 50.
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		// Move last child to first.
		split_container->set_split_offsets({ 50, 80 });
		split_container->move_child(child_c, 0);
		pos = { (int)split_container->get_size().x - 80 - sep.x, (int)split_container->get_size().x - 30 };
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		// Move it back.
		split_container->move_child(child_c, 2);
		pos = { 50, 80 };
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));
	}

	SUBCASE("[SplitContainer] Showing child with not enough space shrinks the largest child first") {
		set_size_flags(split_container, { -1, -1, -1 }); // None expanded.

		// Second child is largest.
		child_a->set_visible(false);
		Vector<int> pos = { 360 };
		split_container->set_split_offsets(pos);

		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		child_a->set_size(Vector2(100, 100));
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		child_a->set_visible(true);
		pos = { 100, 360 };
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		// Last child is largest.
		child_a->set_visible(false);
		pos = { 60 };
		split_container->set_split_offsets(pos);

		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		child_a->set_size(Vector2(100, 100));
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		child_a->set_visible(true);
		pos = { 100, 160 + sep.x };
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		// Both visible children are the same size.
		child_a->set_visible(false);
		pos = { (int)split_container->get_size().x / 2 - sep.x / 2 };
		split_container->set_split_offsets(pos);

		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));
		CHECK(child_b->get_size().x == child_c->get_size().x);

		child_a->set_size(Vector2(100, 100));
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		child_a->set_visible(true);
		pos = { 100, (int)split_container->get_size().x / 2 + 50 };
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));
		CHECK(child_b->get_size().x == child_c->get_size().x);

		// Second child is slightly larger than the last child.
		child_a->set_visible(false);
		pos = { (int)split_container->get_size().x / 2 - sep.x / 2 + 20 };
		split_container->set_split_offsets(pos);

		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		child_a->set_size(Vector2(100, 100));
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));

		child_a->set_visible(true);
		pos = { 100, (int)split_container->get_size().x / 2 + 50 };
		CHECK(split_container->get_split_offsets() == pos);
		CHECK_RECTS(get_rects_multi(split_container, pos, sep.x), get_child_rects(split_container));
		CHECK(child_b->get_size().x == child_c->get_size().x);
	}

	memdelete(split_container);
}

} // namespace TestSplitContainer

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_tab_bar.cpp ===
/*  test_tab_bar.cpp                                                      */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestTabBar {

TEST_CASE("[SceneTree][TabBar] tab operations") {
	TabBar *tab_bar = memnew(TabBar);
	SceneTree::get_root()->add_child(tab_bar);
	tab_bar->set_clip_tabs(false);

	SIGNAL_WATCH(tab_bar, "tab_selected");
	SIGNAL_WATCH(tab_bar, "tab_changed");

	SUBCASE("[TabBar] no tabs") {
		CHECK(tab_bar->get_tab_count() == 0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
	}

	SUBCASE("[TabBar] add tabs disallowing deselection") {
		tab_bar->set_deselect_enabled(false);
		tab_bar->add_tab("tab0");
		CHECK(tab_bar->get_tab_count() == 1);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK("tab_changed", { { 0 } });

		tab_bar->add_tab("tab1");
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_bar->add_tab("tab2");
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		CHECK(tab_bar->get_tab_title(0) == "tab0");
		CHECK(tab_bar->get_tab_tooltip(0) == "");
		CHECK(tab_bar->get_tab_text_direction(0) == Control::TEXT_DIRECTION_INHERITED);
		CHECK_FALSE(tab_bar->is_tab_disabled(0));
		CHECK_FALSE(tab_bar->is_tab_hidden(0));

		CHECK(tab_bar->get_tab_title(1) == "tab1");
		CHECK(tab_bar->get_tab_tooltip(1) == "");
		CHECK(tab_bar->get_tab_text_direction(1) == Control::TEXT_DIRECTION_INHERITED);
		CHECK_FALSE(tab_bar->is_tab_disabled(1));
		CHECK_FALSE(tab_bar->is_tab_hidden(1));

		CHECK(tab_bar->get_tab_title(2) == "tab2");
		CHECK(tab_bar->get_tab_tooltip(2) == "");
		CHECK(tab_bar->get_tab_text_direction(2) == Control::TEXT_DIRECTION_INHERITED);
		CHECK_FALSE(tab_bar->is_tab_disabled(2));
		CHECK_FALSE(tab_bar->is_tab_hidden(2));
	}

	SUBCASE("[TabBar] add tabs allowing deselection") {
		tab_bar->set_deselect_enabled(true);
		tab_bar->add_tab("tab0");
		CHECK(tab_bar->get_tab_count() == 1);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_bar->add_tab("tab1");
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabBar] set tab count") {
		// Adds multiple tabs at once.
		tab_bar->set_tab_count(3);
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		CHECK(tab_bar->get_tab_title(0) == "");
		CHECK(tab_bar->get_tab_tooltip(0) == "");
		CHECK(tab_bar->get_tab_text_direction(0) == Control::TEXT_DIRECTION_INHERITED);
		CHECK_FALSE(tab_bar->is_tab_disabled(0));
		CHECK_FALSE(tab_bar->is_tab_hidden(0));

		CHECK(tab_bar->get_tab_title(1) == "");
		CHECK(tab_bar->get_tab_tooltip(1) == "");
		CHECK(tab_bar->get_tab_text_direction(1) == Control::TEXT_DIRECTION_INHERITED);
		CHECK_FALSE(tab_bar->is_tab_disabled(1));
		CHECK_FALSE(tab_bar->is_tab_hidden(1));

		CHECK(tab_bar->get_tab_title(2) == "");
		CHECK(tab_bar->get_tab_tooltip(2) == "");
		CHECK(tab_bar->get_tab_text_direction(2) == Control::TEXT_DIRECTION_INHERITED);
		CHECK_FALSE(tab_bar->is_tab_disabled(2));
		CHECK_FALSE(tab_bar->is_tab_hidden(2));

		// Setting to less tabs than there are removes from the end.
		tab_bar->set_tab_title(0, "tab0");
		tab_bar->set_tab_title(1, "tab1");
		tab_bar->set_tab_title(2, "tab2");

		tab_bar->set_tab_count(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		CHECK(tab_bar->get_tab_title(0) == "tab0");
		CHECK(tab_bar->get_tab_title(1) == "tab1");

		// Remove all tabs.
		tab_bar->set_tab_count(0);
		CHECK(tab_bar->get_tab_count() == 0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
	}

	SUBCASE("[TabBar] clear tabs") {
		CHECK(tab_bar->get_tab_count() == 0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);

		tab_bar->set_tab_count(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		tab_bar->clear_tabs();
		CHECK(tab_bar->get_tab_count() == 0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabBar] remove tabs") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Remove first tab.
		tab_bar->remove_tab(0);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_tab_title(0) == "tab1");
		CHECK(tab_bar->get_tab_title(1) == "tab2");
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Remove last tab.
		tab_bar->remove_tab(1);
		CHECK(tab_bar->get_tab_count() == 1);
		CHECK(tab_bar->get_tab_title(0) == "tab1");
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Remove only tab.
		tab_bar->remove_tab(0);
		CHECK(tab_bar->get_tab_count() == 0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK("tab_changed", { { -1 } });

		// Remove current tab when there are other tabs.
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		tab_bar->set_current_tab(1);
		tab_bar->set_current_tab(2);
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(tab_bar->get_current_tab() == 2);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		tab_bar->remove_tab(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK("tab_changed", { { 1 } });
	}

	SUBCASE("[TabBar] move tabs") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Don't move if index is the same.
		tab_bar->move_tab(0, 0);
		CHECK(tab_bar->get_tab_title(0) == "tab0");
		CHECK(tab_bar->get_tab_title(1) == "tab1");
		CHECK(tab_bar->get_tab_title(2) == "tab2");
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Move the first tab to the end.
		tab_bar->move_tab(0, 2);
		CHECK(tab_bar->get_tab_title(0) == "tab1");
		CHECK(tab_bar->get_tab_title(1) == "tab2");
		CHECK(tab_bar->get_tab_title(2) == "tab0");
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 2);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Move the second tab to the front.
		tab_bar->move_tab(1, 0);
		CHECK(tab_bar->get_tab_title(0) == "tab2");
		CHECK(tab_bar->get_tab_title(1) == "tab1");
		CHECK(tab_bar->get_tab_title(2) == "tab0");
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 2);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabBar] set current tab") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK("tab_changed", { { 0 } });

		// Set the current tab.
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });

		// Set to same tab.
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK_FALSE("tab_changed");

		// Out of bounds.
		ERR_PRINT_OFF;
		tab_bar->set_current_tab(-5);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_bar->set_current_tab(5);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		ERR_PRINT_ON;
	}

	SUBCASE("[TabBar] deselection enabled") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Setting deselect enabled doesn't change current tab.
		tab_bar->set_deselect_enabled(true);
		CHECK(tab_bar->get_deselect_enabled());
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Can deselect all tabs by setting current to -1.
		tab_bar->set_current_tab(-1);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { -1 } });
		SIGNAL_CHECK("tab_changed", { { -1 } });

		// Adding first tab will NOT change the current tab. (stays deselected)
		tab_bar->clear_tabs();
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);

		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_bar->set_current_tab(-1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Disabling while at -1 will select the first available tab.
		tab_bar->set_deselect_enabled(false);
		CHECK_FALSE(tab_bar->get_deselect_enabled());
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK("tab_changed", { { 0 } });

		// Cannot set to -1 if disabled.
		ERR_PRINT_OFF;
		tab_bar->set_current_tab(-1);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		ERR_PRINT_ON;

		// Disabling while at -1 skips any disabled or hidden tabs.
		tab_bar->set_deselect_enabled(true);
		tab_bar->set_tab_disabled(0, true);
		tab_bar->set_tab_hidden(1, true);
		tab_bar->set_current_tab(-1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");
		tab_bar->set_deselect_enabled(false);
		CHECK(tab_bar->get_current_tab() == 2);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK("tab_selected", { { 2 } });
		SIGNAL_CHECK("tab_changed", { { 2 } });
	}

	SUBCASE("[TabBar] hidden tabs") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		CHECK_FALSE(tab_bar->is_tab_hidden(1));
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		Vector<Rect2> tab_rects = {
			tab_bar->get_tab_rect(0),
			tab_bar->get_tab_rect(1),
			tab_bar->get_tab_rect(2)
		};

		// Hiding a tab does not affect current tab.
		tab_bar->set_tab_hidden(1, true);
		CHECK(tab_bar->is_tab_hidden(1));
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// The tabs after are moved over.
		CHECK(tab_bar->get_tab_rect(0) == tab_rects[0]);
		CHECK(tab_bar->get_tab_rect(2) == tab_rects[1]);

		// Unhiding a tab does not affect current tab.
		tab_bar->set_tab_hidden(1, false);
		CHECK_FALSE(tab_bar->is_tab_hidden(1));
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// The tabs are back where they were.
		CHECK(tab_bar->get_tab_rect(0) == tab_rects[0]);
		CHECK(tab_bar->get_tab_rect(1) == tab_rects[1]);
		CHECK(tab_bar->get_tab_rect(2) == tab_rects[2]);
	}

	SUBCASE("[TabBar] disabled tabs") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		CHECK_FALSE(tab_bar->is_tab_disabled(1));
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		CHECK_FALSE(tab_bar->is_tab_hidden(1));
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Disabling a tab does not affect current tab.
		tab_bar->set_tab_disabled(1, true);
		CHECK(tab_bar->is_tab_disabled(1));
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Enabling a tab does not affect current tab.
		tab_bar->set_tab_disabled(1, false);
		CHECK_FALSE(tab_bar->is_tab_disabled(1));
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabBar] select next available") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		tab_bar->add_tab("tab3");
		tab_bar->add_tab("tab4");
		tab_bar->set_tab_disabled(2, true);
		tab_bar->set_tab_hidden(3, true);
		tab_bar->set_current_tab(0);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Selects the next tab.
		CHECK(tab_bar->select_next_available());
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });

		// Skips over disabled and hidden tabs.
		CHECK(tab_bar->select_next_available());
		CHECK(tab_bar->get_current_tab() == 4);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK("tab_selected", { { 4 } });
		SIGNAL_CHECK("tab_changed", { { 4 } });

		// Does not wrap around.
		CHECK_FALSE(tab_bar->select_next_available());
		CHECK(tab_bar->get_current_tab() == 4);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Fails if there is only one valid tab.
		tab_bar->remove_tab(0);
		tab_bar->remove_tab(3);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		CHECK_FALSE(tab_bar->select_next_available());
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Fails if there are no valid tabs.
		tab_bar->remove_tab(0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		CHECK_FALSE(tab_bar->select_next_available());
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Fails if there are no tabs.
		tab_bar->clear_tabs();
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		CHECK_FALSE(tab_bar->select_next_available());
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabBar] select previous available") {
		tab_bar->add_tab("tab0");
		tab_bar->add_tab("tab1");
		tab_bar->add_tab("tab2");
		tab_bar->add_tab("tab3");
		tab_bar->add_tab("tab4");
		tab_bar->set_tab_disabled(1, true);
		tab_bar->set_tab_hidden(2, true);
		tab_bar->set_current_tab(4);
		CHECK(tab_bar->get_current_tab() == 4);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Selects the previous tab.
		CHECK(tab_bar->select_previous_available());
		CHECK(tab_bar->get_current_tab() == 3);
		CHECK(tab_bar->get_previous_tab() == 4);
		SIGNAL_CHECK("tab_selected", { { 3 } });
		SIGNAL_CHECK("tab_changed", { { 3 } });

		// Skips over disabled and hidden tabs.
		CHECK(tab_bar->select_previous_available());
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 3);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK("tab_changed", { { 0 } });

		// Does not wrap around.
		CHECK_FALSE(tab_bar->select_previous_available());
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 3);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Fails if there is only one valid tab.
		tab_bar->remove_tab(4);
		tab_bar->remove_tab(3);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 2);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		CHECK_FALSE(tab_bar->select_previous_available());
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == 2);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Fails if there are no valid tabs.
		tab_bar->remove_tab(0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		CHECK_FALSE(tab_bar->select_previous_available());
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Fails if there are no tabs.
		tab_bar->clear_tabs();
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		CHECK_FALSE(tab_bar->select_previous_available());
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SIGNAL_UNWATCH(tab_bar, "tab_selected");
	SIGNAL_UNWATCH(tab_bar, "tab_changed");

	memdelete(tab_bar);
}

TEST_CASE("[SceneTree][TabBar] initialization") {
	TabBar *tab_bar = memnew(TabBar);

	SIGNAL_WATCH(tab_bar, "tab_selected");
	SIGNAL_WATCH(tab_bar, "tab_changed");

	SUBCASE("[TabBar] current tab can be set before tabs are set") {
		// This queues the current tab to update on when tabs are set.
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_bar->set_tab_count(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });

		// Does not work again.
		ERR_PRINT_OFF;
		tab_bar->set_current_tab(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_bar->set_tab_count(3);
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		ERR_PRINT_ON;
	}

	SUBCASE("[TabBar] setting tabs works normally if no current tab was set") {
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);

		tab_bar->set_tab_count(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabBar] cannot set current tab to an invalid value before tabs are set") {
		tab_bar->set_current_tab(100);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// This will print an error message as if `set_current_tab` was called after.
		ERR_PRINT_OFF;
		tab_bar->set_tab_count(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		ERR_PRINT_ON;
	}

	SUBCASE("[TabBar] setting the current tab before tabs only works when out of tree") {
		tab_bar->set_current_tab(1);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		SceneTree::get_root()->add_child(tab_bar);
		CHECK(tab_bar->get_tab_count() == 0);
		CHECK(tab_bar->get_current_tab() == -1);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Works normally.
		tab_bar->set_tab_count(2);
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SIGNAL_UNWATCH(tab_bar, "tab_selected");
	SIGNAL_UNWATCH(tab_bar, "tab_changed");

	memdelete(tab_bar);
}

TEST_CASE("[SceneTree][TabBar] layout and offset") {
	TabBar *tab_bar = memnew(TabBar);
	SceneTree::get_root()->add_child(tab_bar);

	tab_bar->set_clip_tabs(false);
	tab_bar->add_tab("tab0");
	tab_bar->add_tab("tab1 ");
	tab_bar->add_tab("tab2    ");
	Size2 all_tabs_size = tab_bar->get_size();

	Vector<Rect2> tab_rects = {
		tab_bar->get_tab_rect(0),
		tab_bar->get_tab_rect(1),
		tab_bar->get_tab_rect(2)
	};

	SUBCASE("[TabBar] tabs are arranged next to each other") {
		// Horizontal positions are next to each other.
		CHECK(tab_rects[0].position.x == 0);
		CHECK(tab_rects[1].position.x == tab_rects[0].size.x);
		CHECK(tab_rects[2].position.x == tab_rects[1].position.x + tab_rects[1].size.x);

		// Fills the entire width.
		CHECK(tab_rects[2].position.x + tab_rects[2].size.x == all_tabs_size.x);

		// Horizontal sizes are positive.
		CHECK(tab_rects[0].size.x > 0);
		CHECK(tab_rects[1].size.x > 0);
		CHECK(tab_rects[2].size.x > 0);

		// Vertical positions are at 0.
		CHECK(tab_rects[0].position.y == 0);
		CHECK(tab_rects[1].position.y == 0);
		CHECK(tab_rects[2].position.y == 0);

		// Vertical sizes are the same.
		CHECK(tab_rects[0].size.y == tab_rects[1].size.y);
		CHECK(tab_rects[1].size.y == tab_rects[2].size.y);
	}

	SUBCASE("[TabBar] tab alignment") {
		// Add extra space so the alignment can be seen.
		tab_bar->set_size(Size2(all_tabs_size.x + 100, all_tabs_size.y));

		// Left alignment.
		tab_bar->set_tab_alignment(TabBar::ALIGNMENT_LEFT);
		tab_rects = {
			tab_bar->get_tab_rect(0),
			tab_bar->get_tab_rect(1),
			tab_bar->get_tab_rect(2)
		};
		CHECK(tab_bar->get_tab_alignment() == TabBar::ALIGNMENT_LEFT);
		CHECK(tab_rects[0].position.x == 0);
		CHECK(tab_rects[1].position.x == tab_rects[0].size.x);
		CHECK(tab_rects[2].position.x == tab_rects[1].position.x + tab_rects[1].size.x);

		// Right alignment.
		tab_bar->set_tab_alignment(TabBar::ALIGNMENT_RIGHT);
		tab_rects = {
			tab_bar->get_tab_rect(0),
			tab_bar->get_tab_rect(1),
			tab_bar->get_tab_rect(2)
		};
		CHECK(tab_bar->get_tab_alignment() == TabBar::ALIGNMENT_RIGHT);
		CHECK(tab_rects[2].position.x == tab_bar->get_size().x - tab_rects[2].size.x);
		CHECK(tab_rects[1].position.x == tab_rects[2].position.x - tab_rects[1].size.x);
		CHECK(tab_rects[0].position.x == tab_rects[1].position.x - tab_rects[0].size.x);

		// Center alignment.
		tab_bar->set_tab_alignment(TabBar::ALIGNMENT_CENTER);
		tab_rects = {
			tab_bar->get_tab_rect(0),
			tab_bar->get_tab_rect(1),
			tab_bar->get_tab_rect(2)
		};
		CHECK(tab_bar->get_tab_alignment() == TabBar::ALIGNMENT_CENTER);
		float center_pos = tab_bar->get_size().x / 2;
		CHECK(tab_rects[0].position.x == center_pos - all_tabs_size.x / 2);
		CHECK(tab_rects[1].position.x == tab_rects[0].position.x + tab_rects[0].size.x);
		CHECK(tab_rects[2].position.x == tab_rects[1].position.x + tab_rects[1].size.x);
	}

	SUBCASE("[TabBar] clip tabs") {
		// Clip tabs disabled means all tabs are visible and the minimum size holds all of them.
		tab_bar->set_clip_tabs(false);
		CHECK_FALSE(tab_bar->get_clip_tabs());
		CHECK(tab_bar->get_tab_offset() == 0);
		CHECK(tab_bar->get_minimum_size() == tab_bar->get_size());
		CHECK(tab_bar->get_size().x == tab_rects[0].size.x + tab_rects[1].size.x + tab_rects[2].size.x);
		CHECK(tab_bar->get_size().y == MAX(tab_rects[0].size.y, MAX(tab_rects[1].size.y, tab_rects[2].size.y)));

		tab_bar->set_clip_tabs(true);
		CHECK(tab_bar->get_clip_tabs());
		CHECK(tab_bar->get_tab_offset() == 0);

		// Horizontal size and minimum size get set to the widest tab plus arrow icons.
		const float offset_button_size = tab_bar->get_theme_icon("decrement_icon")->get_width() + tab_bar->get_theme_icon("increment_icon")->get_width();
		CHECK(tab_bar->get_minimum_size().x == offset_button_size + MAX(tab_rects[0].size.x, MAX(tab_rects[1].size.x, tab_rects[2].size.x)));
		CHECK(tab_bar->get_minimum_size().y == all_tabs_size.y);
		CHECK(tab_bar->get_size().x == offset_button_size + MAX(tab_rects[0].size.x, MAX(tab_rects[1].size.x, tab_rects[2].size.x)));
		CHECK(tab_bar->get_size().y == all_tabs_size.y);
	}

	SUBCASE("[TabBar] ensure tab visible") {
		tab_bar->set_scroll_to_selected(false);
		tab_bar->set_clip_tabs(true);

		// Resize tab bar to only be able to fit 2 tabs.
		const float offset_button_size = tab_bar->get_theme_icon("decrement_icon")->get_width() + tab_bar->get_theme_icon("increment_icon")->get_width();
		tab_bar->set_size(Size2(tab_rects[2].size.x + tab_rects[1].size.x + offset_button_size, all_tabs_size.y));
		CHECK(tab_bar->get_tab_offset() == 0);
		CHECK(tab_bar->get_offset_buttons_visible());

		// Scroll right to a tab that is not visible.
		tab_bar->ensure_tab_visible(2);
		CHECK(tab_bar->get_tab_offset() == 1);
		CHECK(tab_bar->get_tab_rect(1).position.x == 0);
		CHECK(tab_bar->get_tab_rect(2).position.x == tab_rects[1].size.x);

		tab_bar->set_tab_offset(2);
		CHECK(tab_bar->get_tab_offset() == 2);
		CHECK(tab_bar->get_tab_rect(2).position.x == 0);

		// Scroll left to a previous tab.
		tab_bar->ensure_tab_visible(1);
		CHECK(tab_bar->get_tab_offset() == 1);
		CHECK(tab_bar->get_tab_rect(1).position.x == 0);
		CHECK(tab_bar->get_tab_rect(2).position.x == tab_rects[1].size.x);

		// Will not scroll if the tab is already visible.
		tab_bar->ensure_tab_visible(2);
		CHECK(tab_bar->get_tab_offset() == 1);
		CHECK(tab_bar->get_tab_rect(1).position.x == 0);
		CHECK(tab_bar->get_tab_rect(2).position.x == tab_rects[1].size.x);
	}

	memdelete(tab_bar);
}

TEST_CASE("[SceneTree][TabBar] Mouse interaction") {
	TabBar *tab_bar = memnew(TabBar);
	SceneTree::get_root()->add_child(tab_bar);

	tab_bar->set_clip_tabs(false);
	tab_bar->add_tab("tab0");
	tab_bar->add_tab("tab1 ");
	tab_bar->add_tab("tab2    ");

	Vector<Rect2> tab_rects = {
		tab_bar->get_tab_rect(0),
		tab_bar->get_tab_rect(1),
		tab_bar->get_tab_rect(2)
	};

	SIGNAL_WATCH(tab_bar, "active_tab_rearranged");
	SIGNAL_WATCH(tab_bar, "tab_button_pressed");
	SIGNAL_WATCH(tab_bar, "tab_changed");
	SIGNAL_WATCH(tab_bar, "tab_clicked");
	SIGNAL_WATCH(tab_bar, "tab_close_pressed");
	SIGNAL_WATCH(tab_bar, "tab_hovered");
	SIGNAL_WATCH(tab_bar, "tab_selected");

	SUBCASE("[TabBar] Hover over tabs") {
		// Default is -1.
		CHECK(tab_bar->get_hovered_tab() == -1);

		// Hover over the first tab.
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position, MouseButtonMask::NONE, Key::NONE);
		SIGNAL_CHECK("tab_hovered", { { 0 } });
		CHECK(tab_bar->get_hovered_tab() == 0);

		// Hover over same tab won't send signal again.
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position + Point2(2, 2), MouseButtonMask::NONE, Key::NONE);
		SIGNAL_CHECK_FALSE("tab_hovered");
		CHECK(tab_bar->get_hovered_tab() == 0);

		// Hover over different tab.
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[1].position, MouseButtonMask::NONE, Key::NONE);
		SIGNAL_CHECK("tab_hovered", { { 1 } });
		CHECK(tab_bar->get_hovered_tab() == 1);

		// Exit area.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(-1, -1), MouseButtonMask::NONE, Key::NONE);
		SIGNAL_CHECK_FALSE("tab_hovered");
		CHECK(tab_bar->get_hovered_tab() == -1);
	}

	SUBCASE("[TabBar] Click to change current") {
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(tab_bar->get_previous_tab() == -1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Click to set the current tab.
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[1].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		SIGNAL_CHECK("tab_clicked", { { 1 } });

		// Click on the same tab.
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[1].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(tab_bar->get_current_tab() == 1);
		CHECK(tab_bar->get_previous_tab() == 1);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		SIGNAL_CHECK("tab_clicked", { { 1 } });
	}

	SUBCASE("[TabBar] Click on close button") {
		const Size2 close_button_size = tab_bar->get_theme_icon("close_icon")->get_size();
		int h_separation = tab_bar->get_theme_constant("h_separation");
		float margin = tab_bar->get_theme_stylebox("tab_hovered_style")->get_margin(SIDE_RIGHT);

		tab_bar->set_tab_close_display_policy(TabBar::CLOSE_BUTTON_SHOW_ALWAYS);
		tab_rects = { tab_bar->get_tab_rect(0), tab_bar->get_tab_rect(1), tab_bar->get_tab_rect(2) };

		Point2 cb_pos = Size2(tab_rects[0].get_end().x - close_button_size.x - h_separation - margin + 1, tab_rects[0].position.y + (tab_rects[0].size.y - close_button_size.y) / 2 + 1);
		SEND_GUI_MOUSE_MOTION_EVENT(cb_pos, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_BUTTON_EVENT(cb_pos, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(cb_pos, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		SIGNAL_CHECK("tab_close_pressed", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_clicked");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		SIGNAL_CHECK_FALSE("tab_button_pressed");
		// It does not remove the tab.
		CHECK(tab_bar->get_tab_count() == 3);
	}

	SUBCASE("[TabBar] Drag and drop internally") {
		// Cannot drag if not enabled.
		CHECK_FALSE(tab_bar->get_drag_to_rearrange_enabled());
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[1].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(tab_rects[1].position, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_title(0) == "tab0");
		CHECK(tab_bar->get_tab_title(1) == "tab1 ");
		CHECK(tab_bar->get_tab_title(2) == "tab2    ");
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_bar->set_drag_to_rearrange_enabled(true);
		CHECK(tab_bar->get_drag_to_rearrange_enabled());

		// Release over the same tab to not move.
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[1].position, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_title(0) == "tab0");
		CHECK(tab_bar->get_tab_title(1) == "tab1 ");
		CHECK(tab_bar->get_tab_title(2) == "tab2    ");
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Move the first tab after the second.
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[1].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(tab_rects[1].position + Point2(tab_rects[1].size.x / 2 + 1, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_title(0) == "tab1 ");
		CHECK(tab_bar->get_tab_title(1) == "tab0");
		CHECK(tab_bar->get_tab_title(2) == "tab2    ");
		CHECK(tab_bar->get_current_tab() == 1);
		SIGNAL_CHECK("active_tab_rearranged", { { 1 } });
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_rects = { tab_bar->get_tab_rect(0), tab_bar->get_tab_rect(1), tab_bar->get_tab_rect(2) };

		// Move the last tab to be the first.
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[2].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 2 } });
		SIGNAL_CHECK("tab_changed", { { 2 } });
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_title(0) == "tab2    ");
		CHECK(tab_bar->get_tab_title(1) == "tab1 ");
		CHECK(tab_bar->get_tab_title(2) == "tab0");
		CHECK(tab_bar->get_current_tab() == 0);
		SIGNAL_CHECK("active_tab_rearranged", { { 0 } });
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabBar] Drag and drop to different TabBar") {
		TabBar *target_tab_bar = memnew(TabBar);
		SceneTree::get_root()->add_child(target_tab_bar);

		target_tab_bar->set_clip_tabs(false);
		target_tab_bar->add_tab("other_tab0");
		target_tab_bar->set_position(tab_bar->get_size());

		Vector<Rect2> target_tab_rects = { target_tab_bar->get_tab_rect(0) };
		tab_bar->set_drag_to_rearrange_enabled(true);
		tab_bar->set_tabs_rearrange_group(1);

		Point2 target_tab_after_first = target_tab_bar->get_position() + target_tab_rects[0].position + Point2(target_tab_rects[0].size.x / 2 + 1, 0);

		// Cannot drag to another TabBar that does not have drag to rearrange enabled.
		CHECK_FALSE(target_tab_bar->get_drag_to_rearrange_enabled());
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(target_tab_bar->get_tab_count() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Cannot drag to another TabBar that has a tabs rearrange group of -1.
		target_tab_bar->set_drag_to_rearrange_enabled(true);
		tab_bar->set_tabs_rearrange_group(-1);
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(target_tab_bar->get_tab_count() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Cannot drag to another TabBar that has a different tabs rearrange group.
		tab_bar->set_tabs_rearrange_group(1);
		target_tab_bar->set_tabs_rearrange_group(2);
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_count() == 3);
		CHECK(target_tab_bar->get_tab_count() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Drag to target container.
		target_tab_bar->set_tabs_rearrange_group(1);
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_count() == 2);
		CHECK(target_tab_bar->get_tab_count() == 2);
		CHECK(tab_bar->get_tab_title(0) == "tab1 ");
		CHECK(tab_bar->get_tab_title(1) == "tab2    ");
		CHECK(target_tab_bar->get_tab_title(0) == "other_tab0");
		CHECK(target_tab_bar->get_tab_title(1) == "tab0");
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(target_tab_bar->get_current_tab() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected"); // Does not send since tab was removed.
		SIGNAL_CHECK("tab_changed", { { 0 } });

		Point2 target_tab = target_tab_bar->get_position();

		// Drag to target container at first index.
		target_tab_bar->set_tabs_rearrange_group(1);
		SEND_GUI_MOUSE_BUTTON_EVENT(tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_bar->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_bar->get_viewport()->gui_is_dragging());
		CHECK(tab_bar->get_tab_count() == 1);
		CHECK(target_tab_bar->get_tab_count() == 3);
		CHECK(tab_bar->get_tab_title(0) == "tab2    ");
		CHECK(target_tab_bar->get_tab_title(0) == "tab1 ");
		CHECK(target_tab_bar->get_tab_title(1) == "other_tab0");
		CHECK(target_tab_bar->get_tab_title(2) == "tab0");
		CHECK(tab_bar->get_current_tab() == 0);
		CHECK(target_tab_bar->get_current_tab() == 0);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected"); // Does not send since tab was removed.
		SIGNAL_CHECK("tab_changed", { { 0 } });

		memdelete(target_tab_bar);
	}

	SIGNAL_UNWATCH(tab_bar, "active_tab_rearranged");
	SIGNAL_UNWATCH(tab_bar, "tab_button_pressed");
	SIGNAL_UNWATCH(tab_bar, "tab_changed");
	SIGNAL_UNWATCH(tab_bar, "tab_clicked");
	SIGNAL_UNWATCH(tab_bar, "tab_close_pressed");
	SIGNAL_UNWATCH(tab_bar, "tab_hovered");
	SIGNAL_UNWATCH(tab_bar, "tab_selected");

	memdelete(tab_bar);
}

// FIXME: Add tests for keyboard navigation and other methods.

} // namespace TestTabBar

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_tab_container.cpp ===
/*  test_tab_container.cpp                                                */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/gui/box_container.h"
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestTabContainer {

TEST_CASE("[SceneTree][TabContainer] tab operations") {
	TabContainer *tab_container = memnew(TabContainer);
	SceneTree::get_root()->add_child(tab_container);
	SIGNAL_WATCH(tab_container, "tab_selected");
	SIGNAL_WATCH(tab_container, "tab_changed");

	Control *tab0 = memnew(Control);
	tab0->set_name("tab0");
	Control *tab1 = memnew(Control);
	tab1->set_name("tab1");
	Control *tab2 = memnew(Control);
	tab2->set_name("tab2");

	SUBCASE("[TabContainer] add tabs by adding children") {
		CHECK(tab_container->get_tab_count() == 0);
		CHECK(tab_container->get_current_tab() == -1);
		CHECK(tab_container->get_previous_tab() == -1);

		// Add first tab child.
		tab_container->add_child(tab0);
		//		CHECK(tab_container->get_tab_count() == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK("tab_changed", { { 0 } });

		// Add second tab child.
		tab_container->add_child(tab1);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Check default values, the title is the name of the child.
		CHECK(tab_container->get_tab_control(0) == tab0);
		CHECK(tab_container->get_tab_idx_from_control(tab0) == 0);
		CHECK(tab_container->get_tab_title(0) == "tab0");
		CHECK(tab_container->get_tab_tooltip(0) == "");
		CHECK_FALSE(tab_container->is_tab_disabled(0));
		CHECK_FALSE(tab_container->is_tab_hidden(0));

		CHECK(tab_container->get_tab_control(1) == tab1);
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 1);
		CHECK(tab_container->get_tab_title(1) == "tab1");
		CHECK(tab_container->get_tab_tooltip(1) == "");
		CHECK_FALSE(tab_container->is_tab_disabled(1));
		CHECK_FALSE(tab_container->is_tab_hidden(1));
	}

	SUBCASE("[TabContainer] remove tabs by removing children") {
		tab_container->add_child(tab0);
		tab_container->add_child(tab1);
		tab_container->add_child(tab2);
		tab_container->set_current_tab(1);
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Remove first tab.
		tab_container->remove_child(tab0);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_tab_title(0) == "tab1");
		CHECK(tab_container->get_tab_title(1) == "tab2");
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 0);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Remove last tab.
		tab_container->remove_child(tab2);
		CHECK(tab_container->get_tab_count() == 1);
		CHECK(tab_container->get_tab_title(0) == "tab1");
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 0);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Remove only tab.
		tab_container->remove_child(tab1);
		CHECK(tab_container->get_tab_count() == 0);
		CHECK(tab_container->get_current_tab() == -1);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK("tab_changed", { { -1 } });

		// Remove current tab when there are other tabs.
		tab_container->add_child(tab0);
		tab_container->add_child(tab1);
		tab_container->add_child(tab2);
		tab_container->set_current_tab(1);
		tab_container->set_current_tab(2);
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(tab_container->get_current_tab() == 2);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		tab_container->remove_child(tab2);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK("tab_changed", { { 1 } });
	}

	SUBCASE("[TabContainer] move tabs by moving children") {
		tab_container->add_child(tab0);
		tab_container->add_child(tab1);
		tab_container->add_child(tab2);
		tab_container->set_current_tab(1);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Move the first tab to the end.
		tab_container->move_child(tab0, 2);
		CHECK(tab_container->get_tab_idx_from_control(tab0) == 2);
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 0);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == 2);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Move the second tab to the front.
		tab_container->move_child(tab2, 0);
		CHECK(tab_container->get_tab_idx_from_control(tab0) == 2);
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 1);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 0);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 2);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabContainer] set current tab") {
		tab_container->add_child(tab0);
		tab_container->add_child(tab1);
		tab_container->add_child(tab2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK("tab_changed", { { 0 } });
		CHECK(tab0->is_visible());
		CHECK_FALSE(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());

		// Set the current tab.
		tab_container->set_current_tab(1);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());

		// Set to same tab.
		tab_container->set_current_tab(1);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());

		// Out of bounds.
		ERR_PRINT_OFF;
		tab_container->set_current_tab(-5);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());

		tab_container->set_current_tab(5);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());
		ERR_PRINT_ON;
	}

	SUBCASE("[TabContainer] change current tab by changing visibility of children") {
		tab_container->add_child(tab0);
		tab_container->add_child(tab1);
		tab_container->add_child(tab2);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");
		CHECK(tab0->is_visible());
		CHECK_FALSE(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());

		// Show a child to make it the current tab.
		tab1->show();
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());

		// Hide the visible child to select the next tab.
		tab1->hide();
		CHECK(tab_container->get_current_tab() == 2);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_CHECK("tab_selected", { { 2 } });
		SIGNAL_CHECK("tab_changed", { { 2 } });
		CHECK_FALSE(tab0->is_visible());
		CHECK_FALSE(tab1->is_visible());
		CHECK(tab2->is_visible());

		// Hide the visible child to select the previous tab if there is no next.
		tab2->hide();
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 2);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());

		// Cannot hide if there is only one valid child since deselection is not enabled.
		tab_container->remove_child(tab1);
		tab_container->remove_child(tab2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");
		CHECK(tab0->is_visible());

		tab0->hide();
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab0->is_visible());

		// Can hide the last tab if deselection is enabled.
		tab_container->set_deselect_enabled(true);
		tab0->hide();
		CHECK(tab_container->get_current_tab() == -1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { -1 } });
		SIGNAL_CHECK("tab_changed", { { -1 } });
		CHECK_FALSE(tab0->is_visible());
	}

	SIGNAL_UNWATCH(tab_container, "tab_selected");
	SIGNAL_UNWATCH(tab_container, "tab_changed");

	memdelete(tab2);
	memdelete(tab1);
	memdelete(tab0);
	memdelete(tab_container);
}

TEST_CASE("[SceneTree][TabContainer] initialization") {
	TabContainer *tab_container = memnew(TabContainer);

	Control *tab0 = memnew(Control);
	tab0->set_name("tab0");
	Control *tab1 = memnew(Control);
	tab1->set_name("tab1 ");
	Control *tab2 = memnew(Control);
	tab2->set_name("tab2    ");

	SIGNAL_WATCH(tab_container, "tab_selected");
	SIGNAL_WATCH(tab_container, "tab_changed");

	SUBCASE("[TabContainer] add children before entering tree") {
		CHECK(tab_container->get_current_tab() == -1);
		CHECK(tab_container->get_previous_tab() == -1);

		tab_container->add_child(tab0);
		CHECK(tab_container->get_tab_count() == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);

		tab_container->add_child(tab1);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);

		SceneTree::get_root()->add_child(tab_container);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab0->is_visible());
		CHECK_FALSE(tab1->is_visible());
	}

	SUBCASE("[TabContainer] current tab can be set before children are added") {
		// Set the current tab before there are any tabs.
		// This queues the current tab to update on entering the tree.
		tab_container->set_current_tab(1);
		CHECK(tab_container->get_current_tab() == -1);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_container->add_child(tab0);
		CHECK(tab_container->get_tab_count() == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);

		tab_container->add_child(tab1);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);

		tab_container->add_child(tab2);
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Current tab is set when entering the tree.
		SceneTree::get_root()->add_child(tab_container);
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());
	}

	SUBCASE("[TabContainer] cannot set current tab to an invalid value before tabs are set") {
		tab_container->set_current_tab(100);
		CHECK(tab_container->get_current_tab() == -1);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_container->add_child(tab0);
		CHECK(tab_container->get_tab_count() == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_container->add_child(tab1);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// This will print an error message as if `set_current_tab` was called after.
		ERR_PRINT_OFF;
		SceneTree::get_root()->add_child(tab_container);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		ERR_PRINT_ON;
	}

	SUBCASE("[TabContainer] children visibility before entering tree") {
		CHECK(tab_container->get_current_tab() == -1);
		CHECK(tab_container->get_previous_tab() == -1);

		// Adding a hidden child first will change visibility because it is the current tab.
		tab0->hide();
		tab_container->add_child(tab0);
		CHECK(tab_container->get_tab_count() == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		CHECK(tab0->is_visible());

		// Adding a visible child after will hide it because it is not the current tab.
		tab_container->add_child(tab1);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		CHECK(tab0->is_visible());
		CHECK_FALSE(tab1->is_visible());

		// Can change current by showing child now after children have been added.
		// This queues the current tab to update on entering the tree.
		tab1->show();
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab0->is_visible());
		CHECK(tab1->is_visible());

		SceneTree::get_root()->add_child(tab_container);
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
	}

	SUBCASE("[TabContainer] setting current tab and changing child visibility after adding") {
		tab_container->add_child(tab0);
		tab_container->add_child(tab1);
		tab_container->add_child(tab2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);

		tab2->show();
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab0->is_visible());
		CHECK_FALSE(tab1->is_visible());
		CHECK(tab2->is_visible());

		// Whichever happens last will have priority.
		tab_container->set_current_tab(1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);

		// Current tab is set when entering the tree.
		SceneTree::get_root()->add_child(tab_container);
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		CHECK_FALSE(tab0->is_visible());
		CHECK(tab1->is_visible());
		CHECK_FALSE(tab2->is_visible());
	}

	SIGNAL_UNWATCH(tab_container, "tab_selected");
	SIGNAL_UNWATCH(tab_container, "tab_changed");

	memdelete(tab2);
	memdelete(tab1);
	memdelete(tab0);
	memdelete(tab_container);
}

TEST_CASE("[SceneTree][TabContainer] layout and offset") {
	TabContainer *tab_container = memnew(TabContainer);
	SceneTree::get_root()->add_child(tab_container);
	tab_container->set_clip_tabs(false);

	Control *tab0 = memnew(Control);
	tab0->set_name("tab0");
	Control *tab1 = memnew(Control);
	tab1->set_name("tab1 ");
	Control *tab2 = memnew(Control);
	tab2->set_name("tab2    ");

	tab_container->add_child(tab0);
	tab_container->add_child(tab1);
	tab_container->add_child(tab2);


	Size2 all_tabs_size = tab_container->get_size();
	const float side_margin = tab_container->get_theme_constant("side_margin");

	HBoxContainer *internal_container = tab_container->get_internal_container();
	TabBar *tab_bar = tab_container->get_tab_bar();

	Vector<Rect2> tab_rects = {
		tab_bar->get_tab_rect(0),
		tab_bar->get_tab_rect(1),
		tab_bar->get_tab_rect(2)
	};

	SUBCASE("[TabContainer] tabs are arranged next to each other") {
		// Horizontal positions are next to each other.
		CHECK(tab_rects[0].position.x == 0);
		CHECK(tab_rects[1].position.x == tab_rects[0].size.x);
		CHECK(tab_rects[2].position.x == tab_rects[1].position.x + tab_rects[1].size.x);

		// Fills the entire width.
		CHECK(tab_rects[2].position.x + tab_rects[2].size.x == all_tabs_size.x - side_margin);

		// Horizontal sizes are positive.
		CHECK(tab_rects[0].size.x > 0);
		CHECK(tab_rects[1].size.x > 0);
		CHECK(tab_rects[2].size.x > 0);

		// Vertical positions are at 0.
		CHECK(tab_rects[0].position.y == 0);
		CHECK(tab_rects[1].position.y == 0);
		CHECK(tab_rects[2].position.y == 0);

		// Vertical sizes are the same.
		CHECK(tab_rects[0].size.y == tab_rects[1].size.y);
		CHECK(tab_rects[1].size.y == tab_rects[2].size.y);
	}

	SUBCASE("[TabContainer] tab position") {
		float tab_height = tab_rects[0].size.y;
		Ref<StyleBox> panel_style = tab_container->get_theme_stylebox("panel_style");

		// Initial position, same as top position.
		// Tab bar is at the top.
		CHECK(internal_container->get_anchor(SIDE_TOP) == 0);
		CHECK(internal_container->get_anchor(SIDE_BOTTOM) == 0);
		CHECK(internal_container->get_anchor(SIDE_LEFT) == 0);
		CHECK(internal_container->get_anchor(SIDE_RIGHT) == 1);
		CHECK(internal_container->get_offset(SIDE_TOP) == 0);
		CHECK(internal_container->get_offset(SIDE_BOTTOM) == tab_height);
		CHECK(internal_container->get_offset(SIDE_LEFT) == side_margin);
		CHECK(internal_container->get_offset(SIDE_RIGHT) == 0);

		// Child is expanded and below the tab bar.
		CHECK(tab0->get_anchor(SIDE_TOP) == 0);
		CHECK(tab0->get_anchor(SIDE_BOTTOM) == 1);
		CHECK(tab0->get_anchor(SIDE_LEFT) == 0);
		CHECK(tab0->get_anchor(SIDE_RIGHT) == 1);
		CHECK(tab0->get_offset(SIDE_TOP) == tab_height);
		CHECK(tab0->get_offset(SIDE_BOTTOM) == 0);
		CHECK(tab0->get_offset(SIDE_LEFT) == 0);
		CHECK(tab0->get_offset(SIDE_RIGHT) == 0);

		// Bottom position.
		tab_container->set_tabs_position(TabContainer::POSITION_BOTTOM);
		CHECK(tab_container->get_tabs_position() == TabContainer::POSITION_BOTTOM);

		// Tab bar is at the bottom.
		CHECK(internal_container->get_anchor(SIDE_TOP) == 1);
		CHECK(internal_container->get_anchor(SIDE_BOTTOM) == 1);
		CHECK(internal_container->get_anchor(SIDE_LEFT) == 0);
		CHECK(internal_container->get_anchor(SIDE_RIGHT) == 1);
		CHECK(internal_container->get_offset(SIDE_TOP) == -tab_height);
		CHECK(internal_container->get_offset(SIDE_BOTTOM) == 0);
		CHECK(internal_container->get_offset(SIDE_LEFT) == side_margin);
		CHECK(internal_container->get_offset(SIDE_RIGHT) == 0);

		// Child is expanded and above the tab bar.
		CHECK(tab0->get_anchor(SIDE_TOP) == 0);
		CHECK(tab0->get_anchor(SIDE_BOTTOM) == 1);
		CHECK(tab0->get_anchor(SIDE_LEFT) == 0);
		CHECK(tab0->get_anchor(SIDE_RIGHT) == 1);
		CHECK(tab0->get_offset(SIDE_TOP) == 0);
		CHECK(tab0->get_offset(SIDE_BOTTOM) == -tab_height);
		CHECK(tab0->get_offset(SIDE_LEFT) == 0);
		CHECK(tab0->get_offset(SIDE_RIGHT) == 0);

		// Top position.
		tab_container->set_tabs_position(TabContainer::POSITION_TOP);
		CHECK(tab_container->get_tabs_position() == TabContainer::POSITION_TOP);

		// Tab bar is at the top.
		CHECK(internal_container->get_anchor(SIDE_TOP) == 0);
		CHECK(internal_container->get_anchor(SIDE_BOTTOM) == 0);
		CHECK(internal_container->get_anchor(SIDE_LEFT) == 0);
		CHECK(internal_container->get_anchor(SIDE_RIGHT) == 1);
		CHECK(internal_container->get_offset(SIDE_TOP) == 0);
		CHECK(internal_container->get_offset(SIDE_BOTTOM) == tab_height);
		CHECK(internal_container->get_offset(SIDE_LEFT) == side_margin);
		CHECK(internal_container->get_offset(SIDE_RIGHT) == 0);

		// Child is expanded and below the tab bar.
		CHECK(tab0->get_anchor(SIDE_TOP) == 0);
		CHECK(tab0->get_anchor(SIDE_BOTTOM) == 1);
		CHECK(tab0->get_anchor(SIDE_LEFT) == 0);
		CHECK(tab0->get_anchor(SIDE_RIGHT) == 1);
		CHECK(tab0->get_offset(SIDE_TOP) == tab_height);
		CHECK(tab0->get_offset(SIDE_BOTTOM) == 0);
		CHECK(tab0->get_offset(SIDE_LEFT) == 0);
		CHECK(tab0->get_offset(SIDE_RIGHT) == 0);
	}

	memdelete(tab_container);
}

TEST_CASE("[SceneTree][TabContainer] Mouse interaction") {
	TabContainer *tab_container = memnew(TabContainer);
	SceneTree::get_root()->add_child(tab_container);

	tab_container->set_clip_tabs(false);
	Control *tab0 = memnew(Control);
	tab0->set_name("tab0");
	Control *tab1 = memnew(Control);
	tab1->set_name("tab1 ");
	Control *tab2 = memnew(Control);
	tab2->set_name("tab2    ");

	tab_container->add_child(tab0);
	tab_container->add_child(tab1);
	tab_container->add_child(tab2);


	const float side_margin = tab_container->get_theme_constant("side_margin");

	TabBar *tab_bar = tab_container->get_tab_bar();
	Vector<Rect2> tab_rects = {
		tab_bar->get_tab_rect(0),
		tab_bar->get_tab_rect(1),
		tab_bar->get_tab_rect(2)
	};

	SIGNAL_WATCH(tab_container, "active_tab_rearranged");
	SIGNAL_WATCH(tab_container, "tab_changed");
	SIGNAL_WATCH(tab_container, "tab_clicked");
	SIGNAL_WATCH(tab_container, "tab_selected");

	SUBCASE("[TabContainer] Click to change current") {
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(tab_container->get_previous_tab() == -1);
		SIGNAL_DISCARD("tab_selected");
		SIGNAL_DISCARD("tab_changed");

		// Click to set the current tab.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[1].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 0);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK("tab_changed", { { 1 } });
		SIGNAL_CHECK("tab_clicked", { { 1 } });

		// Click on the same tab.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[1].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		SIGNAL_CHECK("tab_clicked", { { 1 } });

		// Click outside of tabs.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(0, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(tab_container->get_current_tab() == 1);
		CHECK(tab_container->get_previous_tab() == 1);
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");
		SIGNAL_CHECK_FALSE("tab_clicked");
	}

	SUBCASE("[TabContainer] Drag and drop internally") {
		// Cannot drag if not enabled.
		CHECK_FALSE(tab_container->get_drag_to_rearrange_enabled());
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[1].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(side_margin, 0) + tab_rects[1].position, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_idx_from_control(tab0) == 0);
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 1);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 2);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_container->set_drag_to_rearrange_enabled(true);
		CHECK(tab_container->get_drag_to_rearrange_enabled());

		// Release over the same tab to not move.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[1].position, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_idx_from_control(tab0) == 0);
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 1);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 2);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Move the first tab after the second.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[1].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(side_margin, 0) + tab_rects[1].position + Point2(tab_rects[1].size.x / 2 + 1, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 0);
		CHECK(tab_container->get_tab_idx_from_control(tab0) == 1);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 2);
		SIGNAL_CHECK("active_tab_rearranged", { { 1 } });
		SIGNAL_CHECK("tab_selected", { { 1 } });
		SIGNAL_CHECK_FALSE("tab_changed");

		tab_rects = { tab_bar->get_tab_rect(0), tab_bar->get_tab_rect(1), tab_bar->get_tab_rect(2) };

		// Move the last tab to be the first.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[2].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 2 } });
		SIGNAL_CHECK("tab_changed", { { 2 } });
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 0);
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 1);
		CHECK(tab_container->get_tab_idx_from_control(tab0) == 2);
		SIGNAL_CHECK("active_tab_rearranged", { { 0 } });
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
	}

	SUBCASE("[TabContainer] Drag and drop to different TabContainer") {
		TabContainer *target_tab_container = memnew(TabContainer);
		SceneTree::get_root()->add_child(target_tab_container);

		target_tab_container->set_clip_tabs(false);
		Control *other_tab0 = memnew(Control);
		other_tab0->set_name("other_tab0");
		target_tab_container->add_child(other_tab0);

		target_tab_container->set_position(tab_container->get_size());

		Vector<Rect2> target_tab_rects = {
			target_tab_container->get_tab_bar()->get_tab_rect(0)
		};
		tab_container->set_drag_to_rearrange_enabled(true);
		tab_container->set_tabs_rearrange_group(1);

		Point2 target_tab_after_first = Point2(side_margin, 0) + target_tab_container->get_position() + target_tab_rects[0].position + Point2(target_tab_rects[0].size.x / 2 + 1, 0);

		// Cannot drag to another TabContainer that does not have drag to rearrange enabled.
		CHECK_FALSE(target_tab_container->get_drag_to_rearrange_enabled());
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(target_tab_container->get_tab_count() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Cannot drag to another TabContainer that has a tabs rearrange group of -1.
		target_tab_container->set_drag_to_rearrange_enabled(true);
		tab_container->set_tabs_rearrange_group(-1);
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(target_tab_container->get_tab_count() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Cannot drag to another TabContainer that has a different tabs rearrange group.
		tab_container->set_tabs_rearrange_group(1);
		target_tab_container->set_tabs_rearrange_group(2);
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_count() == 3);
		CHECK(target_tab_container->get_tab_count() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected");
		SIGNAL_CHECK_FALSE("tab_changed");

		// Drag to target container.
		target_tab_container->set_tabs_rearrange_group(1);
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab_after_first, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab_after_first, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_count() == 2);
		CHECK(target_tab_container->get_tab_count() == 2);
		CHECK(tab_container->get_child_count(false) == 2);
		CHECK(target_tab_container->get_child_count(false) == 2);
		CHECK(tab_container->get_tab_idx_from_control(tab1) == 0);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 1);
		CHECK(target_tab_container->get_tab_idx_from_control(other_tab0) == 0);
		CHECK(target_tab_container->get_tab_idx_from_control(tab0) == 1);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(target_tab_container->get_current_tab() == 1);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected"); // Does not send since tab was removed.
		SIGNAL_CHECK("tab_changed", { { 0 } });

		Point2 target_tab = Point2(side_margin, 0) + target_tab_container->get_position();

		// Drag to target container at first index.
		target_tab_container->set_tabs_rearrange_group(1);
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(side_margin, 0) + tab_rects[0].position, MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(side_margin, 0) + tab_rects[0].position + Point2(20, 0), MouseButtonMask::LEFT, Key::NONE);
		SEND_GUI_MOUSE_MOTION_EVENT(target_tab, MouseButtonMask::LEFT, Key::NONE);
		SIGNAL_CHECK("tab_selected", { { 0 } });
		SIGNAL_CHECK_FALSE("tab_changed");
		CHECK(tab_container->get_viewport()->gui_is_dragging());
		SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_tab, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
		CHECK_FALSE(tab_container->get_viewport()->gui_is_dragging());
		CHECK(tab_container->get_tab_count() == 1);
		CHECK(target_tab_container->get_tab_count() == 3);
		CHECK(tab_container->get_tab_idx_from_control(tab2) == 0);
		CHECK(target_tab_container->get_tab_idx_from_control(tab1) == 0);
		CHECK(target_tab_container->get_tab_idx_from_control(other_tab0) == 1);
		CHECK(target_tab_container->get_tab_idx_from_control(tab0) == 2);
		CHECK(tab_container->get_current_tab() == 0);
		CHECK(target_tab_container->get_current_tab() == 0);
		SIGNAL_CHECK_FALSE("active_tab_rearranged");
		SIGNAL_CHECK_FALSE("tab_selected"); // Does not send since tab was removed.
		SIGNAL_CHECK("tab_changed", { { 0 } });

		memdelete(target_tab_container);
	}

	SIGNAL_UNWATCH(tab_container, "active_tab_rearranged");
	SIGNAL_UNWATCH(tab_container, "tab_changed");
	SIGNAL_UNWATCH(tab_container, "tab_clicked");
	SIGNAL_UNWATCH(tab_container, "tab_selected");

	memdelete(tab_container);
}

// FIXME: Add tests for keyboard navigation and other methods.

} // namespace TestTabContainer

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_text_edit.cpp ===
/*  test_text_edit.cpp                                                    */
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



#ifndef ADVANCED_GUI_DISABLED

#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "tests/test_macros.h"

namespace TestTextEdit {

static inline Array reverse_nested(Array array) {
	Array reversed_array = array.duplicate(true);
	reversed_array.reverse();
	for (int i = 0; i < reversed_array.size(); i++) {
		((Array)reversed_array[i]).reverse();
	}
	return reversed_array;
}

TEST_CASE("[SceneTree][TextEdit] text entry") {
#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
	SceneTree::get_root()->set_physics_object_picking(false);
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);
	text_edit->grab_focus();

	Array empty_signal_args = { {} };

	SUBCASE("[TextEdit] text entry") {
		SIGNAL_WATCH(text_edit, "text_set");
		SIGNAL_WATCH(text_edit, "text_changed");
		SIGNAL_WATCH(text_edit, "lines_edited_from");
		SIGNAL_WATCH(text_edit, "caret_changed");

		Array lines_edited_args = { { 0, 0 }, { 0, 0 } };

		SUBCASE("[TextEdit] clear and set text") {
			// "text_changed" should not be emitted on clear / set.
			text_edit->clear();
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_line_count() == 1);
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->set_text("test text");
			CHECK(text_edit->get_text() == "test text");
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_line_count() == 1);
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->clear();
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");

			// Can undo / redo words when editable.
			text_edit->undo();
			CHECK(text_edit->get_text() == "test text");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_set");

			text_edit->redo();
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_set");

			// Cannot undo when not-editable but should still clear.
			text_edit->undo();
			CHECK(text_edit->get_text() == "test text");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_set");

			// Can clear even if not editable.
			text_edit->set_editable(false);

			Array lines_edited_clear_args = { { 1, 0 } };

			text_edit->clear();
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_clear_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->set_editable(true);

			text_edit->undo();
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK_FALSE("text_set");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("caret_changed");

			// Can still undo set_text.
			text_edit->set_editable(false);

			text_edit->set_text("test text");
			CHECK(text_edit->get_text() == "test text");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->set_editable(true);

			text_edit->undo();
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_set");

			// Any selections are removed.
			text_edit->set_text("test text");
			text_edit->select_all();
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			CHECK(text_edit->get_text() == "test text");
			CHECK(text_edit->get_caret_column() == 9);
			CHECK(text_edit->has_selection());
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->set_text("test");
			CHECK(text_edit->get_text() == "test");
			CHECK(text_edit->get_caret_column() == 0);
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->select_all();
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			CHECK(text_edit->has_selection());

			text_edit->clear();
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_column() == 0);
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
		}

		SUBCASE("[TextEdit] insert text") {
			// insert_text is 0 indexed.
			ERR_PRINT_OFF;
			text_edit->insert_text("test", 1, 0);
			ERR_PRINT_ON;
			CHECK(text_edit->get_text() == "");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_set");

			// Insert text when there is no text.
			lines_edited_args = { { 0, 0 } };

			text_edit->insert_text("tes", 0, 0);
			CHECK(text_edit->get_text() == "tes");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 3);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Insert multiple lines.
			lines_edited_args = { { 0, 1 } };

			text_edit->insert_text("t\ninserting text", 0, 3);
			CHECK(text_edit->get_text() == "test\ninserting text");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 14);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Can insert even if not editable.
			lines_edited_args = { { 1, 1 } };

			text_edit->set_editable(false);
			text_edit->insert_text("mid", 1, 2);
			CHECK(text_edit->get_text() == "test\ninmidserting text");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 17);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->set_editable(true);

			// Undo insert.
			text_edit->undo();
			CHECK(text_edit->get_text() == "test\ninserting text");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 14);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Redo insert.
			text_edit->redo();
			CHECK(text_edit->get_text() == "test\ninmidserting text");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 17);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Insert offsets carets after the edit.
			text_edit->add_caret(1, 1);
			text_edit->add_caret(1, 4);
			text_edit->select(1, 4, 1, 6, 2);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 2 } };

			text_edit->insert_text("\n ", 1, 2);
			CHECK(text_edit->get_text() == "test\nin\n midserting text");
			CHECK(text_edit->get_caret_count() == 3);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 2);
			CHECK(text_edit->get_caret_column(0) == 16);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 1);
			CHECK(text_edit->has_selection(2));
			CHECK(text_edit->get_caret_line(2) == 2);
			CHECK(text_edit->get_caret_column(2) == 5);
			CHECK(text_edit->get_selection_origin_line(2) == 2);
			CHECK(text_edit->get_selection_origin_column(2) == 3);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->remove_secondary_carets();
			text_edit->deselect();

			// Insert text outside of selections.
			text_edit->set_text("test text");
			text_edit->add_caret(0, 8);
			text_edit->select(0, 1, 0, 4, 0);
			text_edit->select(0, 4, 0, 8, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			text_edit->insert_text("a", 0, 4, true, false);
			CHECK(text_edit->get_text() == "testa text");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 1);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 9);
			CHECK(text_edit->get_selection_origin_line(1) == 0);
			CHECK(text_edit->get_selection_origin_column(1) == 5);

			// Insert text to beginning of selections.
			text_edit->set_text("test text");
			text_edit->add_caret(0, 8);
			text_edit->select(0, 1, 0, 4, 0);
			text_edit->select(0, 4, 0, 8, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			text_edit->insert_text("a", 0, 4, false, false);
			CHECK(text_edit->get_text() == "testa text");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 1);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 9);
			CHECK(text_edit->get_selection_origin_line(1) == 0);
			CHECK(text_edit->get_selection_origin_column(1) == 4);

			// Insert text to end of selections.
			text_edit->set_text("test text");
			text_edit->add_caret(0, 8);
			text_edit->select(0, 1, 0, 4, 0);
			text_edit->select(0, 4, 0, 8, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			text_edit->insert_text("a", 0, 4, true, true);
			CHECK(text_edit->get_text() == "testa text");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 5);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 1);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 9);
			CHECK(text_edit->get_selection_origin_line(1) == 0);
			CHECK(text_edit->get_selection_origin_column(1) == 5);

			// Insert text inside of selections.
			text_edit->set_text("test text");
			text_edit->add_caret(0, 8);
			text_edit->select(0, 1, 0, 4, 0);
			text_edit->select(0, 4, 0, 8, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			text_edit->insert_text("a", 0, 4, false, true);
			CHECK(text_edit->get_text() == "testa text");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 9);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 1);
		}

		SUBCASE("[TextEdit] remove text") {
			lines_edited_args = { { 0, 0 }, { 0, 2 } };

			text_edit->set_text("test\nremoveing text\nthird line");
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			// remove_text is 0 indexed.
			ERR_PRINT_OFF;
			text_edit->remove_text(3, 0, 3, 4);
			ERR_PRINT_ON;
			CHECK(text_edit->get_text() == "test\nremoveing text\nthird line");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_set");

			// Remove multiple lines.
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(10);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 2, 1 } };

			text_edit->remove_text(1, 9, 2, 2);
			CHECK(text_edit->get_text() == "test\nremoveingird line");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 17);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Can remove even if not editable.
			lines_edited_args = { { 1, 1 } };

			text_edit->set_editable(false);
			text_edit->remove_text(1, 5, 1, 6);
			CHECK(text_edit->get_text() == "test\nremovingird line");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 16);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->set_editable(true);

			// Undo remove.
			text_edit->undo();
			CHECK(text_edit->get_text() == "test\nremoveingird line");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 17);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Redo remove.
			text_edit->redo();
			CHECK(text_edit->get_text() == "test\nremovingird line");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 16);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Remove collapses carets and offsets carets after the edit.
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(9);
			text_edit->add_caret(1, 10);
			text_edit->select(1, 10, 1, 13, 1);
			text_edit->add_caret(1, 14);
			text_edit->add_caret(1, 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			text_edit->remove_text(1, 8, 1, 11);
			CHECK(text_edit->get_text() == "test\nremoving line");
			// Caret 0 was merged into the selection.
			CHECK(text_edit->get_caret_count() == 3);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 10);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 8);
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 11);
			CHECK(text_edit->get_caret_line(2) == 1);
			CHECK(text_edit->get_caret_column(2) == 2);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->remove_secondary_carets();
		}

		SUBCASE("[TextEdit] set and get line") {
			// Set / Get line is 0 indexed.
			text_edit->set_line(1, "test");
			CHECK(text_edit->get_text() == "");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("text_set");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("caret_changed");

			text_edit->set_line(0, "test");
			CHECK(text_edit->get_text() == "test");
			CHECK(text_edit->get_line(0) == "test");
			CHECK(text_edit->get_line(1) == "");
			CHECK(text_edit->get_line_count() == 1);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			SIGNAL_CHECK_FALSE("caret_changed");

			// Setting to a longer line, caret and selections should be preserved.
			text_edit->select_all();
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_line(0, "test text");
			CHECK(text_edit->get_line(0) == "test text");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "test");
			CHECK(text_edit->get_selection_origin_column() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_set");

			// Setting to a shorter line, selection and caret should be adjusted. Also works if not editable.
			text_edit->set_editable(false);
			text_edit->set_line(0, "te");
			CHECK(text_edit->get_line(0) == "te");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "te");
			CHECK(text_edit->get_caret_column() == 2);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->set_editable(true);

			// Undo / redo should work.
			text_edit->undo();
			CHECK(text_edit->get_line(0) == "test text");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "test");
			CHECK(text_edit->get_caret_column() == 4);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			text_edit->redo();
			CHECK(text_edit->get_line(0) == "te");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_column() == 2);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Out of range.
			ERR_PRINT_OFF;
			text_edit->set_line(-1, "test");
			CHECK(text_edit->get_line(0) == "te");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("text_set");

			text_edit->set_line(1, "test");
			CHECK(text_edit->get_line(0) == "te");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("text_set");
			ERR_PRINT_ON;

			// Both ends of selection are adjusted and deselects.
			text_edit->set_text("test text");
			text_edit->select(0, 8, 0, 6);
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_line(0, "test");
			CHECK(text_edit->get_line(0) == "test");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_column() == 4);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Multiple carets adjust to keep visual position.
			text_edit->set_text("test text");
			text_edit->set_caret_column(2);
			text_edit->add_caret(0, 0);
			text_edit->add_caret(0, 1);
			text_edit->add_caret(0, 6);
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_line(0, "\tset line");
			CHECK(text_edit->get_line(0) == "\tset line");
			CHECK(text_edit->get_caret_count() == 3);
			CHECK_FALSE(text_edit->has_selection());
			// In the default font, these are the same positions.
			CHECK(text_edit->get_caret_column(0) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			// The previous caret at index 2 was merged.
			CHECK(text_edit->get_caret_column(2) == 4);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->remove_secondary_carets();

			// Insert multiple lines.
			text_edit->set_text("test text\nsecond line");
			text_edit->set_caret_column(5);
			text_edit->add_caret(1, 6);
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 }, { 0, 1 } };

			text_edit->set_line(0, "multiple\nlines");
			CHECK(text_edit->get_text() == "multiple\nlines\nsecond line");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 3); // In the default font, this is the same position.
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 6);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->remove_secondary_carets();
		}

		SUBCASE("[TextEdit] swap lines") {
			lines_edited_args = { { 0, 0 }, { 0, 1 } };

			text_edit->set_text("testing\nswap");
			CHECK(text_edit->get_text() == "testing\nswap");
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->set_caret_column(text_edit->get_line(0).length());
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			// Emitted twice for each line.
			lines_edited_args = { { 0, 0 }, { 0, 0 }, { 1, 1 }, { 1, 1 } };

			// Order does not matter. Works when not editable.
			text_edit->set_editable(false);
			text_edit->swap_lines(1, 0);
			CHECK(text_edit->get_text() == "swap\ntesting");
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->set_editable(true);

			// Single undo/redo action.
			text_edit->undo();
			CHECK(text_edit->get_text() == "testing\nswap");
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			text_edit->redo();
			CHECK(text_edit->get_text() == "swap\ntesting");
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Out of range.
			ERR_PRINT_OFF;
			text_edit->swap_lines(-1, 0);
			CHECK(text_edit->get_text() == "swap\ntesting");
			text_edit->swap_lines(0, -1);
			CHECK(text_edit->get_text() == "swap\ntesting");
			text_edit->swap_lines(2, 0);
			CHECK(text_edit->get_text() == "swap\ntesting");
			text_edit->swap_lines(0, 2);
			CHECK(text_edit->get_text() == "swap\ntesting");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("text_set");
			ERR_PRINT_ON;

			// Carets are also swapped.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(2);
			text_edit->select(0, 0, 0, 2);
			text_edit->add_caret(1, 6);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 1 }, { 1, 1 }, { 0, 0 }, { 0, 0 } };

			text_edit->swap_lines(0, 1);
			CHECK(text_edit->get_text() == "testing\nswap");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 2);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 6);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->remove_secondary_carets();

			// Swap non adjacent lines.
			text_edit->insert_line_at(1, "new line");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(5);
			CHECK(text_edit->get_text() == "testing\nnew line\nswap");
			SIGNAL_DISCARD("caret_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("text_changed");
			lines_edited_args = { { 2, 2 }, { 2, 2 }, { 0, 0 }, { 0, 0 } };

			text_edit->swap_lines(0, 2);
			CHECK(text_edit->get_text() == "swap\nnew line\ntesting");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 5);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
		}

		SUBCASE("[TextEdit] insert line at") {
			lines_edited_args = { { 0, 0 }, { 0, 1 } };

			text_edit->set_text("testing\nswap");
			CHECK(text_edit->get_text() == "testing\nswap");
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			text_edit->select_all();
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selection_from_line() == 0);
			CHECK(text_edit->get_selection_to_line() == 1);
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			// Insert line at inserts a line before and moves caret and selection. Works when not editable.
			text_edit->set_editable(false);
			lines_edited_args = { { 0, 1 } };
			text_edit->insert_line_at(0, "new");
			CHECK(text_edit->get_text() == "new\ntesting\nswap");
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == text_edit->get_line(2).size() - 1);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selection_from_line() == 1);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 2);
			CHECK(text_edit->get_selection_to_column() == 4);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->set_editable(true);

			// Can undo/redo as single action.
			text_edit->undo();
			CHECK(text_edit->get_text() == "testing\nswap");
			CHECK(text_edit->has_selection());
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			text_edit->redo();
			CHECK(text_edit->get_text() == "new\ntesting\nswap");
			CHECK(text_edit->has_selection());
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Adding inside selection extends selection.
			text_edit->select_all();
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selection_from_line() == 0);
			CHECK(text_edit->get_selection_to_line() == 2);
			SIGNAL_CHECK_FALSE("caret_changed");
			lines_edited_args = { { 2, 3 } };

			text_edit->insert_line_at(2, "after");
			CHECK(text_edit->get_text() == "new\ntesting\nafter\nswap");
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == text_edit->get_line(3).size() - 1);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selection_from_line() == 0);
			CHECK(text_edit->get_selection_to_line() == 3);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Out of range.
			ERR_PRINT_OFF;
			text_edit->insert_line_at(-1, "after");
			CHECK(text_edit->get_text() == "new\ntesting\nafter\nswap");
			text_edit->insert_line_at(4, "after");
			CHECK(text_edit->get_text() == "new\ntesting\nafter\nswap");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("text_set");
			ERR_PRINT_ON;

			// Can insert multiple lines.
			text_edit->select(0, 1, 2, 2);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 2, 4 } };

			text_edit->insert_line_at(2, "multiple\nlines");
			CHECK(text_edit->get_text() == "new\ntesting\nmultiple\nlines\nafter\nswap");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 4);
			CHECK(text_edit->get_caret_column() == 2);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 1);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
		}

		SUBCASE("[TextEdit] remove line at") {
			lines_edited_args = { { 0, 0 }, { 0, 5 } };
			text_edit->set_text("testing\nremove line at\n\tremove\nlines\n\ntest");
			CHECK(text_edit->get_text() == "testing\nremove line at\n\tremove\nlines\n\ntest");
			SIGNAL_CHECK("text_set", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");

			// Remove line handles multiple carets.
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(0);
			text_edit->add_caret(2, 7);
			text_edit->select(2, 1, 2, 7, 1);
			text_edit->add_caret(3, 1);
			text_edit->add_caret(4, 5);
			text_edit->add_caret(1, 5);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 2 } };

			text_edit->remove_line_at(2, true);
			CHECK(text_edit->get_text() == "testing\nremove line at\nlines\n\ntest");
			CHECK(text_edit->get_caret_count() == 5);
			CHECK_FALSE(text_edit->has_selection(0)); // Same line.
			CHECK(text_edit->get_caret_line(0) == 2);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->has_selection(1)); // Same line, clamped.
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 5);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 3); // In the default font, this is the same position.
			CHECK_FALSE(text_edit->has_selection(2)); // Moved up.
			CHECK(text_edit->get_caret_line(2) == 2);
			CHECK(text_edit->get_caret_column(2) == 1);
			CHECK_FALSE(text_edit->has_selection(3)); // Moved up.
			CHECK(text_edit->get_caret_line(3) == 3);
			CHECK(text_edit->get_caret_column(3) == 0);
			CHECK_FALSE(text_edit->has_selection(4)); // Didn't move.
			CHECK(text_edit->get_caret_line(4) == 1);
			CHECK(text_edit->get_caret_column(4) == 5);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->remove_secondary_carets();

			// Remove first line.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(5);
			text_edit->add_caret(4, 4);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 } };

			text_edit->remove_line_at(0, false);
			CHECK(text_edit->get_text() == "remove line at\nlines\n\ntest");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->remove_secondary_carets();

			// Remove empty line.
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(0);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 2 } };

			text_edit->remove_line_at(2, false);
			CHECK(text_edit->get_text() == "remove line at\nlines\ntest");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Remove last line.
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 2, 1 } };

			text_edit->remove_line_at(2, true);
			CHECK(text_edit->get_text() == "remove line at\nlines");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 5);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Out of bounds.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("caret_changed");

			ERR_PRINT_OFF
			text_edit->remove_line_at(2, true);
			ERR_PRINT_ON
			CHECK(text_edit->get_text() == "remove line at\nlines");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 2);
			SIGNAL_CHECK_FALSE("lines_edited_from");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("text_set");

			// Remove regular line with move caret up and not editable.
			text_edit->set_editable(false);
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 } };

			text_edit->remove_line_at(1, false);
			CHECK(text_edit->get_text() == "remove line at");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 1); // In the default font, this is the same position.
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->set_editable(true);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "remove line at\nlines");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 2);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "remove line at");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 1);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Remove only line removes line content.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(10);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			text_edit->remove_line_at(0);
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_line_count() == 1);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
		}

		SUBCASE("[TextEdit] insert text at caret") {
			lines_edited_args = { { 0, 1 } };

			// Insert text at caret can insert multiple lines.
			text_edit->insert_text_at_caret("testing\nswap");
			CHECK(text_edit->get_text() == "testing\nswap");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == text_edit->get_line(1).size() - 1);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Text is inserted at caret.
			text_edit->set_caret_line(0, false);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("caret_changed");

			lines_edited_args = { { 0, 0 } };
			text_edit->insert_text_at_caret("mid");
			CHECK(text_edit->get_text() == "temidsting\nswap");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 5);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			// Selections are deleted then text is inserted. It also works even if not editable.
			text_edit->select(0, 0, 0, text_edit->get_line(0).length());
			CHECK(text_edit->has_selection());
			lines_edited_args = { { 0, 0 }, { 0, 0 } };

			text_edit->set_editable(false);
			text_edit->insert_text_at_caret("new line");
			CHECK(text_edit->get_text() == "new line\nswap");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == text_edit->get_line(0).size() - 1);
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
			text_edit->set_editable(true);

			// Undo restores text and selection.
			text_edit->undo();
			CHECK(text_edit->get_text() == "temidsting\nswap");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == text_edit->get_line(0).length());
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");

			text_edit->redo();
			CHECK(text_edit->get_text() == "new line\nswap");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_set");
		}

		SIGNAL_UNWATCH(text_edit, "text_set");
		SIGNAL_UNWATCH(text_edit, "text_changed");
		SIGNAL_UNWATCH(text_edit, "lines_edited_from");
		SIGNAL_UNWATCH(text_edit, "caret_changed");
	}

	SUBCASE("[TextEdit] indent level") {
		CHECK(text_edit->get_indent_level(0) == 0);
		CHECK(text_edit->get_first_non_whitespace_column(0) == 0);

		text_edit->set_line(0, "a");
		CHECK(text_edit->get_indent_level(0) == 0);
		CHECK(text_edit->get_first_non_whitespace_column(0) == 0);

		text_edit->set_line(0, "\t");
		CHECK(text_edit->get_indent_level(0) == 4);
		CHECK(text_edit->get_first_non_whitespace_column(0) == 1);

		text_edit->set_tab_size(8);
		CHECK(text_edit->get_indent_level(0) == 8);

		text_edit->set_line(0, "\t a");
		CHECK(text_edit->get_first_non_whitespace_column(0) == 2);
		CHECK(text_edit->get_indent_level(0) == 9);
	}

	SUBCASE("[TextEdit] selection") {
		SIGNAL_WATCH(text_edit, "text_set");
		SIGNAL_WATCH(text_edit, "text_changed");
		SIGNAL_WATCH(text_edit, "lines_edited_from");
		SIGNAL_WATCH(text_edit, "caret_changed");

		SUBCASE("[TextEdit] select all") {
			// Select when there is no text does not select.
			text_edit->select_all();
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selection_from_line() == 0);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 0);
			CHECK(text_edit->get_selection_to_column() == 0);
			CHECK(text_edit->get_selected_text() == "");

			// Select all selects all text.
			text_edit->set_text("test\nselection");
			SEND_GUI_ACTION("ui_text_select_all");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_selected_text() == "test\nselection");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selection_from_line() == 0);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 1);
			CHECK(text_edit->get_selection_to_column() == 9);
			CHECK(text_edit->get_selection_mode() == TextEdit::SelectionMode::SELECTION_MODE_SHIFT);
			CHECK(text_edit->is_caret_after_selection_origin());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 9);
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			// Cannot select when disabled.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);
			text_edit->set_selecting_enabled(false);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);

			text_edit->select_all();
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
		}

		SUBCASE("[TextEdit] select word under caret") {
			text_edit->set_text("\ntest   test\ntest   test");

			text_edit->set_caret_column(0);
			text_edit->set_caret_line(1);

			text_edit->add_caret(2, 0);
			text_edit->add_caret(2, 2);
			CHECK(text_edit->get_caret_count() == 3);


			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Select word under caret with multiple carets.
			text_edit->select_word_under_caret();
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 1);
			CHECK(text_edit->get_selection_from_column(0) == 0);
			CHECK(text_edit->get_selection_to_line(0) == 1);
			CHECK(text_edit->get_selection_to_column(0) == 4);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "test");
			CHECK(text_edit->get_selection_from_line(1) == 2);
			CHECK(text_edit->get_selection_from_column(1) == 0);
			CHECK(text_edit->get_selection_to_line(1) == 2);
			CHECK(text_edit->get_selection_to_column(1) == 4);
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 4);

			CHECK(text_edit->get_caret_count() == 2);

			// Select word under caret disables selection if there is already a selection.
			text_edit->select_word_under_caret();
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");

			SEND_GUI_ACTION("ui_text_select_word_under_caret");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 1);
			CHECK(text_edit->get_selection_from_column(0) == 0);
			CHECK(text_edit->get_selection_to_line(0) == 1);
			CHECK(text_edit->get_selection_to_column(0) == 4);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "test");
			CHECK(text_edit->get_selection_from_line(1) == 2);
			CHECK(text_edit->get_selection_from_column(1) == 0);
			CHECK(text_edit->get_selection_to_line(1) == 2);
			CHECK(text_edit->get_selection_to_column(1) == 4);
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 4);

			CHECK(text_edit->get_selected_text() == "test\ntest");
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			// Cannot select when disabled.
			text_edit->set_selecting_enabled(false);
			text_edit->select_word_under_caret();
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK_FALSE("caret_changed");
			text_edit->set_selecting_enabled(true);

			// Select word under caret when there is no word does not select.
			text_edit->set_caret_line(1, false, true, -1, 0);
			text_edit->set_caret_column(5, false, 0);
			text_edit->set_caret_line(2, false, true, -1, 1);
			text_edit->set_caret_column(5, false, 1);

			text_edit->select_word_under_caret();
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");

			text_edit->select_word_under_caret();
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 5);
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 5);
			SIGNAL_CHECK_FALSE("caret_changed");
		}

		SUBCASE("[TextEdit] add selection for next occurrence") {
			text_edit->set_text("\ntest   other_test\nrandom   test\nword test word nonrandom");
			text_edit->set_caret_column(0);
			text_edit->set_caret_line(1);

			// First selection made by the implicit select_word_under_caret call.
			text_edit->add_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 1);
			CHECK(text_edit->get_selection_from_column(0) == 0);
			CHECK(text_edit->get_selection_to_line(0) == 1);
			CHECK(text_edit->get_selection_to_column(0) == 4);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);

			text_edit->add_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->get_selected_text(1) == "test");
			CHECK(text_edit->get_selection_from_line(1) == 1);
			CHECK(text_edit->get_selection_from_column(1) == 13);
			CHECK(text_edit->get_selection_to_line(1) == 1);
			CHECK(text_edit->get_selection_to_column(1) == 17);
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 17);

			text_edit->add_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 3);
			CHECK(text_edit->get_selected_text(2) == "test");
			CHECK(text_edit->get_selection_from_line(2) == 2);
			CHECK(text_edit->get_selection_from_column(2) == 9);
			CHECK(text_edit->get_selection_to_line(2) == 2);
			CHECK(text_edit->get_selection_to_column(2) == 13);
			CHECK(text_edit->get_caret_line(2) == 2);
			CHECK(text_edit->get_caret_column(2) == 13);

			text_edit->add_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 4);
			CHECK(text_edit->get_selected_text(3) == "test");
			CHECK(text_edit->get_selection_from_line(3) == 3);
			CHECK(text_edit->get_selection_from_column(3) == 5);
			CHECK(text_edit->get_selection_to_line(3) == 3);
			CHECK(text_edit->get_selection_to_column(3) == 9);
			CHECK(text_edit->get_caret_line(3) == 3);
			CHECK(text_edit->get_caret_column(3) == 9);

			// A different word with a new manually added caret.
			text_edit->add_caret(2, 1);
			text_edit->select(2, 0, 2, 4, 4);
			CHECK(text_edit->get_selected_text(4) == "rand");

			text_edit->add_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 6);
			CHECK(text_edit->get_selected_text(5) == "rand");
			CHECK(text_edit->get_selection_from_line(5) == 3);
			CHECK(text_edit->get_selection_from_column(5) == 18);
			CHECK(text_edit->get_selection_to_line(5) == 3);
			CHECK(text_edit->get_selection_to_column(5) == 22);
			CHECK(text_edit->get_caret_line(5) == 3);
			CHECK(text_edit->get_caret_column(5) == 22);

			// Make sure the previous selections are still active.
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selected_text(1) == "test");
			CHECK(text_edit->get_selected_text(2) == "test");
			CHECK(text_edit->get_selected_text(3) == "test");
		}

		SUBCASE("[TextEdit] skip selection for next occurrence") {
			text_edit->set_text("\ntest   other_test\nrandom   test\nword test word nonrandom");
			text_edit->set_caret_column(0);
			text_edit->set_caret_line(1);

			// Without selection on the current caret, the caret as 'jumped' to the next occurrence of the word under the caret.
			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 13);

			// Repeating previous action.
			// This time caret is in 'other_test' (other_|test)
			// so the searched term will be 'other_test' or not just 'test'
			// => no occurrence, as a side effect, the caret will move to start of the term.
			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 7);

			// Repeating action again should do nothing now
			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 7);

			// Moving back to the first 'test' occurrence.
			text_edit->set_caret_column(0);
			text_edit->set_caret_line(1);

			// But this time, create a selection of it.
			text_edit->add_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 1);
			CHECK(text_edit->get_selection_from_column(0) == 0);
			CHECK(text_edit->get_selection_to_line(0) == 1);
			CHECK(text_edit->get_selection_to_column(0) == 4);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);

			// Then, skipping it, but this time, selection has been made on the next occurrence.
			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 1);
			CHECK(text_edit->get_selection_from_column(0) == 13);
			CHECK(text_edit->get_selection_to_line(0) == 1);
			CHECK(text_edit->get_selection_to_column(0) == 17);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 17);

			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 2);
			CHECK(text_edit->get_selection_from_column(0) == 9);
			CHECK(text_edit->get_selection_to_line(0) == 2);
			CHECK(text_edit->get_selection_to_column(0) == 13);
			CHECK(text_edit->get_caret_line(0) == 2);
			CHECK(text_edit->get_caret_column(0) == 13);

			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 3);
			CHECK(text_edit->get_selection_from_column(0) == 5);
			CHECK(text_edit->get_selection_to_line(0) == 3);
			CHECK(text_edit->get_selection_to_column(0) == 9);
			CHECK(text_edit->get_caret_line(0) == 3);
			CHECK(text_edit->get_caret_column(0) == 9);

			// Last skip, we are back to the first occurrence.
			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 1);
			CHECK(text_edit->get_selection_from_column(0) == 0);
			CHECK(text_edit->get_selection_to_line(0) == 1);
			CHECK(text_edit->get_selection_to_column(0) == 4);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);

			// Adding first occurrence to selections/carets list
			// and select occurrence on 'other_test'.
			text_edit->add_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "test");
			CHECK(text_edit->get_selection_from_line(1) == 1);
			CHECK(text_edit->get_selection_from_column(1) == 13);
			CHECK(text_edit->get_selection_to_line(1) == 1);
			CHECK(text_edit->get_selection_to_column(1) == 17);
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 17);

			// We don't want this occurrence.
			// Let's skip it.
			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");

			CHECK(text_edit->get_selected_text(1) == "test");
			CHECK(text_edit->get_selection_from_line(1) == 2);
			CHECK(text_edit->get_selection_from_column(1) == 9);
			CHECK(text_edit->get_selection_to_line(1) == 2);
			CHECK(text_edit->get_selection_to_column(1) == 13);
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 13);

			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");

			CHECK(text_edit->get_selected_text(1) == "test");
			CHECK(text_edit->get_selection_from_line(1) == 3);
			CHECK(text_edit->get_selection_from_column(1) == 5);
			CHECK(text_edit->get_selection_to_line(1) == 3);
			CHECK(text_edit->get_selection_to_column(1) == 9);
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 9);

			// We are back the first occurrence.
			text_edit->skip_selection_for_next_occurrence();
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_selection_from_line(0) == 1);
			CHECK(text_edit->get_selection_from_column(0) == 0);
			CHECK(text_edit->get_selection_to_line(0) == 1);
			CHECK(text_edit->get_selection_to_column(0) == 4);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);
		}

		SUBCASE("[TextEdit] deselect on focus loss") {
			text_edit->set_text("test");

			text_edit->set_deselect_on_focus_loss_enabled(true);
			CHECK(text_edit->is_deselect_on_focus_loss_enabled());

			text_edit->grab_focus();
			text_edit->select_all();
			CHECK(text_edit->has_focus());
			CHECK(text_edit->has_selection());

			text_edit->release_focus();
			CHECK_FALSE(text_edit->has_focus());
			CHECK_FALSE(text_edit->has_selection());

			text_edit->set_deselect_on_focus_loss_enabled(false);
			CHECK_FALSE(text_edit->is_deselect_on_focus_loss_enabled());

			text_edit->grab_focus();
			text_edit->select_all();
			CHECK(text_edit->has_focus());
			CHECK(text_edit->has_selection());

			text_edit->release_focus();
			CHECK_FALSE(text_edit->has_focus());
			CHECK(text_edit->has_selection());

			text_edit->set_deselect_on_focus_loss_enabled(true);
			CHECK_FALSE(text_edit->has_selection());
		}

		SUBCASE("[TextEdit] key select") {
			text_edit->set_text("test");

			text_edit->grab_focus();
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT)
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "t");
			CHECK(text_edit->is_caret_after_selection_origin());

#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT | KeyModifierMask::ALT)
#else
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL)
#endif
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "test");
			CHECK(text_edit->is_caret_after_selection_origin());

			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT)
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "tes");
			CHECK(text_edit->is_caret_after_selection_origin());

#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT | KeyModifierMask::ALT)
#else
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT | KeyModifierMask::CMD_OR_CTRL)
#endif
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");

			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT)
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "t");

			SEND_GUI_KEY_EVENT(Key::RIGHT)
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");

			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT)
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "t");
			CHECK_FALSE(text_edit->is_caret_after_selection_origin());

			SEND_GUI_KEY_EVENT(Key::LEFT)
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");

			// Cannot select when disabled.
			text_edit->set_selecting_enabled(false);
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT)
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "");
			text_edit->set_selecting_enabled(true);
		}

		SUBCASE("[TextEdit] mouse drag select") {
			// Set size for mouse input.
			text_edit->set_size(Size2(200, 200));

			text_edit->set_text("this is some text\nfor selection");
			text_edit->grab_focus();

			// Click and drag to make a selection.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			// Add (2,0) to bring it past the center point of the grapheme and account for integer division flooring.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for s");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 0);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 5);
			CHECK(text_edit->is_caret_after_selection_origin());
			CHECK(text_edit->is_dragging_cursor());

			// Releasing finishes.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for s");
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 9).get_center() + Point2i(2, 0), MouseButtonMask::NONE, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for s");
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 0);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 5);
			CHECK(text_edit->is_caret_after_selection_origin());

			// Clicking clears selection.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 7).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 7);

			// Cannot select when disabled, but caret still moves.
			text_edit->set_selecting_enabled(false);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 5);
			text_edit->set_selecting_enabled(true);

			// Only last caret is moved when adding a selection.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(4);
			text_edit->add_caret(0, 15);
			text_edit->select(0, 11, 0, 15, 1);

			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::ALT);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 0).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_caret_count() == 3);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selection_origin_line(1) == 0);
			CHECK(text_edit->get_selection_origin_column(1) == 11);
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 15);

			CHECK(text_edit->has_selection(2));
			CHECK(text_edit->get_selected_text(2) == "for s");
			CHECK(text_edit->get_selection_origin_line(2) == 1);
			CHECK(text_edit->get_selection_origin_column(2) == 5);
			CHECK(text_edit->get_caret_line(2) == 1);
			CHECK(text_edit->get_caret_column(2) == 0);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(2));

			// Overlapping carets and selections merges them.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 3).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "s is some text\nfor s");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 5);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 3);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin());

			// Entering text stops selecting.
			text_edit->insert_text_at_caret("a");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "thiaelection");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 10).get_center() + Point2i(2, 0), MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);

			// Wrapped lines.
			text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
			text_edit->set_text("this is some text\nfor selection");
			text_edit->set_size(Size2(110, 100));

			// Line 0 wraps: 'this is ', 'some text'.
			// Line 1 wraps: 'for ', 'selection'.
			CHECK(text_edit->is_line_wrapped(0));

			// Select to the first character of a wrapped line.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 11).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 8).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "so");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 10);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK(text_edit->is_dragging_cursor());
		}

		SUBCASE("[TextEdit] mouse word select") {
			// Set size for mouse input.
			text_edit->set_size(Size2(200, 200));

			text_edit->set_text("this is some text\nfor selection\n");
			SIGNAL_DISCARD("caret_changed");

			// Double click to select word.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(1, 2).get_center() + Point2i(2, 0), Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->get_selection_from_line() == 1);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 1);
			CHECK(text_edit->get_selection_to_column() == 3);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 3);
			CHECK(text_edit->is_caret_after_selection_origin());
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			// Moving mouse selects entire words at a time.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 6).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for selection");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->get_selection_from_line() == 1);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 1);
			CHECK(text_edit->get_selection_to_column() == 13);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 13);
			CHECK(text_edit->is_caret_after_selection_origin());
			CHECK(text_edit->is_dragging_cursor());
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			// Moving to a word before the initial selected word reverses selection direction and keeps the initial word selected.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 10).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some text\nfor");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->get_selection_from_line() == 0);
			CHECK(text_edit->get_selection_from_column() == 8);
			CHECK(text_edit->get_selection_to_line() == 1);
			CHECK(text_edit->get_selection_to_column() == 3);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin());
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			// Releasing finishes.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 10).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some text\nfor");
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 2).get_center(), MouseButtonMask::NONE, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some text\nfor");
			text_edit->deselect();

			// Can start word select mode on an empty line.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(2, 0).get_center() + Point2i(2, 0), Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 0);

			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 9).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "selection\n");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 4);
			CHECK(text_edit->get_selection_origin_line() == 2);
			CHECK(text_edit->get_selection_origin_column() == 0);

			// Clicking clears selection.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);

			// Can select word from left endpoint.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(0, 8).get_center() + Point2i(2, 0), Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 8);

			// Can select word from right endpoint.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(0, 12).get_center() + Point2i(2, 0), Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 8);

			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 9).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some text\nfor selection");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 13);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 8);

			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 10).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 8);

			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 15).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);

			// Add a new selection without affecting the old one.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::ALT);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 8).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::ALT);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "some");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 12);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 8);

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "ele");
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 8);
			CHECK(text_edit->get_selection_origin_line(1) == 1);
			CHECK(text_edit->get_selection_origin_column(1) == 5);

			// Shift + double click to extend selection and start word select mode.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 8).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			text_edit->remove_secondary_carets();
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "some text\nfor selection");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 13);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 8);

			// Cannot select when disabled, but caret still moves to end of word.
			text_edit->set_selecting_enabled(false);
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(1, 1).get_center() + Point2i(2, 0), Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 3);
			text_edit->set_selecting_enabled(true);

			// Can start word select mode when not on a word.
			text_edit->set_text("this is  some text\nwith an extra space\n");
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(0, 8).get_center() + Point2i(2, 0), Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);

			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 13).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == " some text\nwith an extra");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 13);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 8);

			// Can reverse selection direction without retaining previous selection.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 0).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "this is ");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 8);

			// Can deselect by moving to initial selection point.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 8).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_WORD);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);
		}

		SUBCASE("[TextEdit] mouse line select") {
			// Set size for mouse input.
			text_edit->set_size(Size2(200, 200));

			text_edit->set_text("this is some text\nfor selection\nwith 3 lines");

			// Triple click to select line.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(1, 2).get_center(), Key::NONE);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 2).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for selection\n");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->get_selection_from_line() == 1);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 2);
			CHECK(text_edit->get_selection_to_column() == 0);
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->is_caret_after_selection_origin());

			// Moving mouse selects entire lines at a time. Selecting above reverses the selection direction.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 10).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "this is some text\nfor selection");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->get_selection_from_line() == 0);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 1);
			CHECK(text_edit->get_selection_to_column() == 13);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin());
			CHECK(text_edit->is_dragging_cursor());

			// Selecting to the last line puts the caret at end of the line.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(2, 10).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for selection\nwith 3 lines");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->get_selection_from_line() == 1);
			CHECK(text_edit->get_selection_from_column() == 0);
			CHECK(text_edit->get_selection_to_line() == 2);
			CHECK(text_edit->get_selection_to_column() == 12);
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK(text_edit->is_caret_after_selection_origin());

			// Releasing finishes.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(2, 10).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for selection\nwith 3 lines");
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 2).get_center(), MouseButtonMask::NONE, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for selection\nwith 3 lines");

			// Clicking clears selection.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);

			// Can start line select mode on an empty line.
			text_edit->set_text("this is some text\n\nfor selection\nwith 4 lines");
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(1, 0).get_center() + Point2i(2, 0), Key::NONE);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "\n");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 0);

			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(2, 9).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "\nfor selection\n");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 0);

			// Add a new selection without affecting the old one.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 3).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::ALT);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 4).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::ALT);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "\nfor selection\n");
			CHECK(text_edit->get_caret_line(0) == 3);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 0);

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "is");
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 4);
			CHECK(text_edit->get_selection_origin_line(1) == 0);
			CHECK(text_edit->get_selection_origin_column(1) == 2);
			text_edit->remove_secondary_carets();
			text_edit->deselect();

			// Selecting the last line puts caret at the end.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(3, 3).get_center(), Key::NONE);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(3, 3).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "with 4 lines");
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK(text_edit->get_selection_origin_line() == 3);
			CHECK(text_edit->get_selection_origin_column() == 0);

			// Selecting above reverses direction.
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(2, 10).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "for selection\nwith 4 lines");
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_selection_origin_line() == 3);
			CHECK(text_edit->get_selection_origin_column() == 12);

			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(2, 10).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);

			// Shift + triple click to extend selection and restart line select mode.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(0, 9).get_center() + Point2i(2, 0), Key::NONE | KeyModifierMask::SHIFT);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 9).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_LINE);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "this is some text\n\nfor selection\nwith 4 lines");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_selection_origin_line() == 3);
			CHECK(text_edit->get_selection_origin_column() == 12);

			// Cannot select when disabled, but caret still moves to the start of the next line.
			text_edit->set_selecting_enabled(false);
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(0, 2).get_center(), Key::NONE);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 2).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			text_edit->set_selecting_enabled(true);
		}

		SUBCASE("[TextEdit] mouse shift click select") {
			// Set size for mouse input.
			text_edit->set_size(Size2(200, 200));

			text_edit->set_text("this is some text\nfor selection");

			// Shift click to make a selection from the previous caret position.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 1).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 1).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "or s");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 1);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 5);
			CHECK(text_edit->is_caret_after_selection_origin());
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);

			// Shift click above to switch selection direction. Uses original selection position.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 6).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "s some text\nf");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 1);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 6);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin());
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 6).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);

			// Clicking clears selection.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 7);

			// Cannot select when disabled, but caret still moves.
			text_edit->set_selecting_enabled(false);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 5);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			text_edit->set_selecting_enabled(true);

			// Shift click to make a selection from caret.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			text_edit->set_caret_column(5, false);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 13).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "is some ");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 5);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 13);
			CHECK(text_edit->is_caret_after_selection_origin());
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 13).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			text_edit->deselect();

			// Shift click with an existing selection.
			text_edit->select(1, 2, 1, 5);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 8).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "r sele");
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 2);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK(text_edit->is_caret_after_selection_origin());
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 8).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			text_edit->deselect();

			// Shift selecting after word selection mode keeps the selection on the word.
			SEND_GUI_DOUBLE_CLICK(text_edit->get_rect_at_line_column(0, 6).get_center() + Point2i(2, 0), Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 6).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "is");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 5);

			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 10).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "is so");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 10);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 5);

			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 3).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_selection_mode() == TextEdit::SELECTION_MODE_POINTER);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "is is");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 2);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 7);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 3).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE | KeyModifierMask::SHIFT);
		}

		SUBCASE("[TextEdit] select and deselect") {
			text_edit->set_text("this is some text\nfor selection");
			SIGNAL_DISCARD("caret_changed");

			// Select clamps input to full text.
			text_edit->select(-1, -1, 500, 500);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "this is some text\nfor selection");
			CHECK(text_edit->is_caret_after_selection_origin(0));
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 0);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 13);
			CHECK(text_edit->get_selection_from_line(0) == text_edit->get_selection_origin_line(0));
			CHECK(text_edit->get_selection_from_column(0) == text_edit->get_selection_origin_column(0));
			CHECK(text_edit->get_selection_to_line(0) == text_edit->get_caret_line(0));
			CHECK(text_edit->get_selection_to_column(0) == text_edit->get_caret_column(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			text_edit->deselect();
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK_FALSE("caret_changed");

			// Select works in the other direction.
			text_edit->select(500, 500, -1, -1);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "this is some text\nfor selection");
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(0));
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 13);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->get_selection_from_line(0) == text_edit->get_caret_line(0));
			CHECK(text_edit->get_selection_from_column(0) == text_edit->get_caret_column(0));
			CHECK(text_edit->get_selection_to_line(0) == text_edit->get_selection_origin_line(0));
			CHECK(text_edit->get_selection_to_column(0) == text_edit->get_selection_origin_column(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			text_edit->deselect();
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK_FALSE("caret_changed");

			// Select part of a line.
			text_edit->select(0, 4, 0, 8);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == " is ");
			CHECK(text_edit->is_caret_after_selection_origin(0));
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 4);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 8);
			CHECK(text_edit->get_selection_from_line(0) == text_edit->get_selection_origin_line(0));
			CHECK(text_edit->get_selection_from_column(0) == text_edit->get_selection_origin_column(0));
			CHECK(text_edit->get_selection_to_line(0) == text_edit->get_caret_line(0));
			CHECK(text_edit->get_selection_to_column(0) == text_edit->get_caret_column(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			text_edit->deselect();
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK_FALSE("caret_changed");

			// Select part of a line in the other direction.
			text_edit->select(0, 8, 0, 4);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == " is ");
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(0));
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 8);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_from_line(0) == text_edit->get_caret_line(0));
			CHECK(text_edit->get_selection_from_column(0) == text_edit->get_caret_column(0));
			CHECK(text_edit->get_selection_to_line(0) == text_edit->get_selection_origin_line(0));
			CHECK(text_edit->get_selection_to_column(0) == text_edit->get_selection_origin_column(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);

			// Cannot select when disabled.
			text_edit->set_selecting_enabled(false);
			CHECK_FALSE(text_edit->has_selection());
			text_edit->select(0, 8, 0, 4);
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK_FALSE("caret_changed");
			text_edit->set_selecting_enabled(true);
		}

		SUBCASE("[TextEdit] delete selection") {
			text_edit->set_text("this is some text\nfor selection");

			// Delete selection does nothing if there is no selection.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(8);
			CHECK_FALSE(text_edit->has_selection());

			text_edit->delete_selection();
			CHECK(text_edit->get_text() == "this is some text\nfor selection");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);

			// Backspace removes selection.
			text_edit->select(0, 4, 0, 8);
			CHECK(text_edit->has_selection());
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "thissome text\nfor selection");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);

			// Undo restores previous selection.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is some text\nfor selection");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 4);

			// Redo restores caret.
			text_edit->redo();
			CHECK(text_edit->get_text() == "thissome text\nfor selection");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);

			text_edit->undo();
			CHECK(text_edit->get_text() == "this is some text\nfor selection");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);

			text_edit->select(0, 4, 0, 8);
			CHECK(text_edit->has_selection());

			// Delete selection removes text, deselects, and moves caret.
			text_edit->delete_selection();
			CHECK(text_edit->get_text() == "thissome text\nfor selection");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);

			// Undo delete works.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is some text\nfor selection");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 4);

			// Redo delete works.
			text_edit->redo();
			CHECK(text_edit->get_text() == "thissome text\nfor selection");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);

			text_edit->undo();
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_text() == "this is some text\nfor selection");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 8);

			// Can still delete if not editable.
			text_edit->set_editable(false);
			text_edit->delete_selection();
			text_edit->set_editable(false);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "thissome text\nfor selection");

			// Cannot undo since it was not editable.
			text_edit->undo();
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "thissome text\nfor selection");

			// Delete multiple adjacent selections on the same line.
			text_edit->select(0, 0, 0, 5);
			text_edit->add_caret(0, 8);
			text_edit->select(0, 5, 0, 8, 1);
			CHECK(text_edit->get_caret_count() == 2);
			text_edit->delete_selection();
			CHECK(text_edit->get_text() == " text\nfor selection");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);

			// Delete mulitline selection. Ignore non selections.
			text_edit->remove_secondary_carets();
			text_edit->select(1, 3, 0, 2);
			text_edit->add_caret(1, 7);
			text_edit->delete_selection();
			CHECK(text_edit->get_text() == " t selection");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 2);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 6);
		}

		SUBCASE("[TextEdit] text drag") {
			text_edit->set_size(Size2(200, 200));
			text_edit->set_text("drag test\ndrop here ''");
			text_edit->grab_click_focus();

			// Drag and drop selected text to mouse position.
			text_edit->select(0, 0, 0, 4);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 7).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "drag");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 11).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 11).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == " test\ndrop here 'drag'");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 15);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 11);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "drag test\ndrop here ''");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == " test\ndrop here 'drag'");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 15);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 11);

			// Hold control when dropping to not delete selected text.
			text_edit->select(1, 10, 1, 16);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 12).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "'drag'");
			CHECK(text_edit->has_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 0).get_center(), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_KEY_EVENT(Key::CMD_OR_CTRL);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(0, 0).get_center(), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			SEND_GUI_KEY_UP_EVENT(Key::CMD_OR_CTRL);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "'drag' test\ndrop here 'drag'");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 6);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);

			// Multiple caret drags entire selection.
			text_edit->select(0, 11, 0, 7, 0);
			text_edit->add_caret(1, 2);
			text_edit->select(1, 2, 1, 4, 1);
			text_edit->add_caret(1, 12);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 3).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection(true, 1));
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 12).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "test\nop");
			// Carets aren't removed from dragging, only dropping.
			CHECK(text_edit->get_caret_count() == 3);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 7);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 11);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 4);
			CHECK(text_edit->get_selection_origin_line(1) == 1);
			CHECK(text_edit->get_selection_origin_column(1) == 2);
			CHECK_FALSE(text_edit->has_selection(2));
			CHECK(text_edit->get_caret_line(2) == 1);
			CHECK(text_edit->get_caret_column(2) == 12);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 9).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 9).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "'drag' \ndr heretest\nop 'drag'");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 2);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 7);

			// Drop onto same selection should do effectively nothing.
			text_edit->select(1, 3, 1, 7);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(1, 6).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "here");
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "'drag' \ndr heretest\nop 'drag'");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK(text_edit->get_selection_origin_line() == 1);
			CHECK(text_edit->get_selection_origin_column() == 3);

			// Cannot drag when drag and drop selection is disabled. It becomes regular drag to select.
			text_edit->set_drag_and_drop_selection_enabled(false);
			text_edit->select(0, 1, 0, 5);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "'drag' \ndr heretest\nop 'drag'");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 2);
			text_edit->set_drag_and_drop_selection_enabled(true);

			// Cancel drag and drop from Escape key.
			text_edit->select(0, 1, 0, 5);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 3).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "drag");
			SEND_GUI_KEY_EVENT(Key::ESCAPE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "'drag' \ndr heretest\nop 'drag'");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 5);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 1);

			// Cancel drag and drop from caret move key input.
			text_edit->select(0, 1, 0, 5);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 3).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "drag");
			SEND_GUI_KEY_EVENT(Key::RIGHT);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "'drag' \ndr heretest\nop 'drag'");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 5);

			// Cancel drag and drop from text key input.
			text_edit->select(0, 1, 0, 5);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 3).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "drag");
			SEND_GUI_KEY_EVENT(Key::A);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "'A' \ndr heretest\nop 'drag'");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 2);
		}

		SUBCASE("[TextEdit] text drag to another text edit") {
			TextEdit *target_text_edit = memnew(TextEdit);
			SceneTree::get_root()->add_child(target_text_edit);

			target_text_edit->set_size(Size2(200, 200));
			target_text_edit->set_position(Point2(400, 0));

			text_edit->set_size(Size2(200, 200));

			CHECK_FALSE(text_edit->is_mouse_over_selection());
			text_edit->set_text("drag me");
			text_edit->select_all();
			text_edit->grab_click_focus();
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);

			// Drag text between text edits.
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 0).get_center(), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 7).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "drag me");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);

			Point2i target_line0 = target_text_edit->get_position() + Point2i(1, target_text_edit->get_line_height() / 2);
			SEND_GUI_MOUSE_MOTION_EVENT(target_line0, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 0);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_line0, MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(target_text_edit->get_text() == "drag me");
			CHECK(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 7);
			CHECK(target_text_edit->get_selection_origin_line() == 0);
			CHECK(target_text_edit->get_selection_origin_column() == 0);

			// Undo is separate per TextEdit.
			text_edit->undo();
			CHECK(text_edit->get_text() == "drag me");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);
			CHECK(target_text_edit->get_text() == "drag me");
			CHECK(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 7);
			CHECK(target_text_edit->get_selection_origin_line() == 0);
			CHECK(target_text_edit->get_selection_origin_column() == 0);

			target_text_edit->undo();
			CHECK(text_edit->get_text() == "drag me");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);
			CHECK(target_text_edit->get_text() == "");
			CHECK_FALSE(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 0);

			// Redo is also separate.
			text_edit->redo();
			CHECK(text_edit->get_text() == "");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(target_text_edit->get_text() == "");
			CHECK_FALSE(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 0);

			target_text_edit->redo();
			CHECK(text_edit->get_text() == "");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(target_text_edit->get_text() == "drag me");
			CHECK(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 7);
			CHECK(target_text_edit->get_selection_origin_line() == 0);
			CHECK(target_text_edit->get_selection_origin_column() == 0);

			// Hold control to not remove selected text.
			text_edit->set_text("drag test\ndrop test");
			target_text_edit->select(0, 0, 0, 3, 0);
			target_text_edit->add_caret(0, 5);
			text_edit->select(0, 5, 0, 7, 0);
			text_edit->add_caret(0, 1);
			text_edit->select(0, 1, 0, 0, 1);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 5).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection(true, 0));
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 6).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "d\nte");
			CHECK(text_edit->has_selection());
			SEND_GUI_KEY_EVENT(Key::CMD_OR_CTRL);
			SEND_GUI_MOUSE_MOTION_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 6).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 6).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			SEND_GUI_KEY_UP_EVENT(Key::CMD_OR_CTRL);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "drag test\ndrop test");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 7);
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 0);
			CHECK(target_text_edit->get_text() == "drag md\ntee");
			CHECK(target_text_edit->get_caret_count() == 1);
			CHECK(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 1);
			CHECK(target_text_edit->get_caret_column() == 2);
			CHECK(target_text_edit->get_selection_origin_line() == 0);
			CHECK(target_text_edit->get_selection_origin_column() == 6);

			// Drop onto selected text deletes the selected text first.
			text_edit->set_deselect_on_focus_loss_enabled(false);
			target_text_edit->set_deselect_on_focus_loss_enabled(false);
			text_edit->remove_secondary_carets();
			text_edit->select(0, 5, 0, 9);
			target_text_edit->select(0, 6, 0, 8);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 6).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection(true, 0));
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center(), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "test");
			CHECK(text_edit->has_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 7).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 7).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "drag \ndrop test");
			CHECK(target_text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 5);
			CHECK(target_text_edit->get_text() == "drag mdtest\ntee");
			CHECK(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 11);
			CHECK(target_text_edit->get_selection_origin_line() == 0);
			CHECK(target_text_edit->get_selection_origin_column() == 7);
			text_edit->set_deselect_on_focus_loss_enabled(true);
			target_text_edit->set_deselect_on_focus_loss_enabled(true);

			// Can drop even when drag and drop selection is disabled.
			target_text_edit->set_drag_and_drop_selection_enabled(false);
			text_edit->select(0, 4, 0, 5);
			target_text_edit->deselect();
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 4).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == " ");
			CHECK(text_edit->has_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 7).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "drag\ndrop test");
			CHECK(target_text_edit->get_text() == "drag md test\ntee");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			CHECK(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 8);
			CHECK(target_text_edit->get_selection_origin_line() == 0);
			CHECK(target_text_edit->get_selection_origin_column() == 7);
			target_text_edit->set_drag_and_drop_selection_enabled(true);

			// Cannot drop when not editable.
			target_text_edit->set_editable(false);
			text_edit->select(0, 1, 0, 4);
			target_text_edit->deselect();
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "rag");
			CHECK(text_edit->has_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "drag\ndrop test");
			CHECK(target_text_edit->get_text() == "drag md test\ntee");
			CHECK(text_edit->has_selection());
			CHECK_FALSE(target_text_edit->has_selection());
			target_text_edit->set_editable(true);

			// Can drag when not editable, but text will not be removed.
			text_edit->set_editable(false);
			text_edit->select(0, 0, 0, 4);
			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->is_mouse_over_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 7).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			CHECK(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_viewport()->gui_get_drag_data() == "drag");
			CHECK(text_edit->has_selection());
			SEND_GUI_MOUSE_MOTION_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 4).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(target_text_edit->get_position() + target_text_edit->get_rect_at_line_column(0, 4).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			CHECK_FALSE(text_edit->get_viewport()->gui_is_dragging());
			CHECK(text_edit->get_text() == "drag\ndrop test");
			CHECK(target_text_edit->get_text() == "dragdrag md test\ntee");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			CHECK(target_text_edit->has_selection());
			CHECK(target_text_edit->get_caret_line() == 0);
			CHECK(target_text_edit->get_caret_column() == 8);
			CHECK(target_text_edit->get_selection_origin_line() == 0);
			CHECK(target_text_edit->get_selection_origin_column() == 4);
			text_edit->set_editable(true);

			memdelete(target_text_edit);
		}

		SIGNAL_UNWATCH(text_edit, "text_set");
		SIGNAL_UNWATCH(text_edit, "text_changed");
		SIGNAL_UNWATCH(text_edit, "lines_edited_from");
		SIGNAL_UNWATCH(text_edit, "caret_changed");
	}

	SUBCASE("[TextEdit] overridable actions") {
		DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();

		SIGNAL_WATCH(text_edit, "text_set");
		SIGNAL_WATCH(text_edit, "text_changed");
		SIGNAL_WATCH(text_edit, "lines_edited_from");
		SIGNAL_WATCH(text_edit, "caret_changed");

		Array lines_edited_args = { { 0, 0 } };

		SUBCASE("[TextEdit] backspace") {
			text_edit->set_text("this is\nsome\n");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Cannot backspace at start of text.
			text_edit->backspace();
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Backspace at start of line removes the line.
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(0);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 2, 1 } };

			text_edit->backspace();
			CHECK(text_edit->get_text() == "this is\nsome");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Backspace removes a character.
			lines_edited_args = { { 1, 1 } };
			text_edit->backspace();
			CHECK(text_edit->get_text() == "this is\nsom");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 3);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Backspace when text is selected removes the selection.
			text_edit->end_complex_operation();
			text_edit->select(1, 0, 1, 3);
			text_edit->backspace();
			CHECK(text_edit->get_text() == "this is\n");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Cannot backspace if not editable.
			text_edit->set_editable(false);
			text_edit->backspace();
			text_edit->set_editable(true);
			CHECK(text_edit->get_text() == "this is\n");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Undo restores text to the previous end of complex operation.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is\nsom");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 3);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "this is\n");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// See ui_text_backspace for more backspace tests.
		}

		SUBCASE("[TextEdit] cut") {
			// Cut without a selection removes the entire line.
			text_edit->set_text("this is\nsome\n");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(6);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 } };

			text_edit->cut();
			CHECK(DS->clipboard_get() == "this is\n");
			CHECK(text_edit->get_text() == "some\n");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 3); // In the default font, this is the same position.
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo restores the cut text.
			text_edit->undo();
			CHECK(DS->clipboard_get() == "this is\n");
			CHECK(text_edit->get_text() == "this is\nsome\n");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 6);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo.
			text_edit->redo();
			CHECK(DS->clipboard_get() == "this is\n");
			CHECK(text_edit->get_text() == "some\n");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 3);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Cut with a selection removes just the selection.
			text_edit->set_text("this is\nsome\n");
			text_edit->select(0, 5, 0, 7);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			SEND_GUI_ACTION("ui_cut");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(DS->clipboard_get() == "is");
			CHECK(text_edit->get_text() == "this \nsome\n");
			CHECK_FALSE(text_edit->get_caret_line());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 5);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Cut does not change the text if not editable. Text is still added to clipboard.
			text_edit->set_text("this is\nsome\n");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(5);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			text_edit->cut();
			text_edit->set_editable(true);
			CHECK(DS->clipboard_get() == "this is\n");
			CHECK(text_edit->get_text() == "this is\nsome\n");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 5);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Cut line with multiple carets.
			text_edit->set_text("this is\nsome\n");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(3);
			text_edit->add_caret(0, 2);
			text_edit->add_caret(0, 4);
			text_edit->add_caret(2, 0);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 }, { 1, 0 } };

			text_edit->cut();
			CHECK(DS->clipboard_get() == "this is\n\n");
			CHECK(text_edit->get_text() == "some");
			CHECK(text_edit->get_caret_count() == 3);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 2); // In the default font, this is the same position.
			// The previous caret at index 1 was merged.
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 3); // In the default font, this is the same position.
			CHECK_FALSE(text_edit->has_selection(2));
			CHECK(text_edit->get_caret_line(2) == 0);
			CHECK(text_edit->get_caret_column(2) == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			text_edit->remove_secondary_carets();

			// Cut on the only line removes the contents.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			text_edit->cut();
			CHECK(DS->clipboard_get() == "some\n");
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_line_count() == 1);
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Cut empty line.
			text_edit->cut();
			CHECK(DS->clipboard_get() == "\n");
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			// These signals are emitted even if there is no change.
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Cut multiple lines, in order.
			text_edit->set_text("this is\nsome\ntext to\nbe\n\ncut");
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(7);
			text_edit->add_caret(3, 0);
			text_edit->add_caret(0, 2);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 }, { 3, 2 }, { 2, 1 } };

			text_edit->cut();
			CHECK(DS->clipboard_get() == "this is\ntext to\nbe\n");
			CHECK(text_edit->get_text() == "some\n\ncut");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			text_edit->remove_secondary_carets();

			// Cut multiple selections, in order. Ignores regular carets.
			text_edit->set_text("this is\nsome\ntext to\nbe\n\ncut");
			text_edit->add_caret(3, 0);
			text_edit->add_caret(0, 2);
			text_edit->add_caret(2, 0);
			text_edit->select(1, 0, 1, 2, 0);
			text_edit->select(3, 0, 4, 0, 1);
			text_edit->select(0, 5, 0, 3, 2);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 1 }, { 4, 3 }, { 0, 0 } };

			text_edit->cut();
			CHECK(DS->clipboard_get() == "s \nso\nbe\n");
			CHECK(text_edit->get_text() == "thiis\nme\ntext to\n\ncut");
			CHECK(text_edit->get_caret_count() == 4);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			CHECK(text_edit->get_caret_line(2) == 0);
			CHECK(text_edit->get_caret_column(2) == 3);
			CHECK(text_edit->get_caret_line(3) == 2);
			CHECK(text_edit->get_caret_column(3) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
		}

		SUBCASE("[TextEdit] copy") {
			text_edit->set_text("this is\nsome\ntest\n\ntext");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Copy selected text.
			text_edit->select(0, 0, 1, 2, 0);
			SIGNAL_DISCARD("caret_changed");
			DS->clipboard_set_primary("");

			text_edit->copy();
			CHECK(DS->clipboard_get() == "this is\nso");
			CHECK(DS->clipboard_get_primary() == "");
			CHECK(text_edit->get_text() == "this is\nsome\ntest\n\ntext");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 0);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 2);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Copy with GUI action.
			text_edit->select(0, 0, 0, 2, 0);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_copy");
			CHECK(DS->clipboard_get() == "th");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Can copy even if not editable.
			text_edit->select(2, 4, 1, 2, 0);
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			text_edit->copy();
			text_edit->set_editable(true);
			CHECK(DS->clipboard_get() == "me\ntest");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->deselect();

			// Copy full line when there is no selection.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("caret_changed");

			text_edit->copy();
			CHECK(DS->clipboard_get() == "this is\n");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Copy empty line.
			text_edit->set_caret_line(3);
			text_edit->set_caret_column(0);
			SIGNAL_DISCARD("caret_changed");

			text_edit->copy();
			CHECK(DS->clipboard_get() == "\n");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->deselect();

			// Copy full line with multiple carets on that line only copies once.
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(2);
			text_edit->add_caret(1, 0);
			text_edit->add_caret(1, 4);
			SIGNAL_DISCARD("caret_changed");

			text_edit->copy();
			CHECK(DS->clipboard_get() == "some\n");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->remove_secondary_carets();

			// Copy selected text from all selections with `\n` in between, in order. Ignore regular carets.
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(4);
			text_edit->add_caret(4, 0);
			text_edit->add_caret(0, 4);
			text_edit->add_caret(1, 0);
			text_edit->select(1, 3, 2, 4, 0);
			text_edit->select(4, 4, 4, 0, 1);
			text_edit->select(0, 5, 0, 4, 2);
			SIGNAL_DISCARD("caret_changed");

			text_edit->copy();
			CHECK(DS->clipboard_get() == " \ne\ntest\ntext");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->remove_secondary_carets();
			text_edit->deselect();

			// Copy multiple lines with multiple carets, in order.
			text_edit->set_caret_line(3);
			text_edit->set_caret_column(0);
			text_edit->add_caret(4, 2);
			text_edit->add_caret(0, 4);
			SIGNAL_DISCARD("caret_changed");

			text_edit->copy();
			CHECK(DS->clipboard_get() == "this is\n\ntext\n");
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] paste") {
			// Paste text from clipboard at caret.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 1 } };
			DS->clipboard_set("paste");

			text_edit->paste();
			CHECK(DS->clipboard_get() == "paste");
			CHECK(text_edit->get_text() == "this is\nsopasteme\n\ntext");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 7);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			text_edit->undo();
			CHECK(DS->clipboard_get() == "paste");
			CHECK(text_edit->get_text() == "this is\nsome\n\ntext");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Redo.
			text_edit->redo();
			CHECK(DS->clipboard_get() == "paste");
			CHECK(text_edit->get_text() == "this is\nsopasteme\n\ntext");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 7);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Paste on empty line. Use GUI action.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(2);
			text_edit->set_caret_column(0);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 2, 2 } };
			DS->clipboard_set("paste2");

			SEND_GUI_ACTION("ui_paste");
			CHECK(DS->clipboard_get() == "paste2");
			CHECK(text_edit->get_text() == "this is\nsome\npaste2\ntext");
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 6);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Paste removes selection before pasting.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->select(0, 5, 1, 3);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 }, { 0, 0 } };
			DS->clipboard_set("paste");

			text_edit->paste();
			CHECK(DS->clipboard_get() == "paste");
			CHECK(text_edit->get_text() == "this pastee\n\ntext");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 10);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Paste multiple lines.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 3 } };
			DS->clipboard_set("multi\n\nline\npaste");

			text_edit->paste();
			CHECK(DS->clipboard_get() == "multi\n\nline\npaste");
			CHECK(text_edit->get_text() == "tmulti\n\nline\npastehis is\nsome\n\ntext");
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 5);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Paste full line after copying it.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(2);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 2 } };
			DS->clipboard_set("");
			text_edit->copy();
			text_edit->set_caret_column(3);
			CHECK(DS->clipboard_get() == "some\n");
			SIGNAL_DISCARD("caret_changed");

			text_edit->paste();
			CHECK(DS->clipboard_get() == "some\n");
			CHECK(text_edit->get_text() == "this is\nsome\nsome\n\ntext");
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 3);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Do not paste as line since it wasn't copied.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(4);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 1 } };
			DS->clipboard_set("paste\n");

			text_edit->paste();
			CHECK(DS->clipboard_get() == "paste\n");
			CHECK(text_edit->get_text() == "thispaste\n is\nsome\n\ntext");
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Paste text at each caret.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(2);
			text_edit->add_caret(3, 4);
			text_edit->add_caret(0, 4);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 1 }, { 2, 3 }, { 5, 6 } };
			DS->clipboard_set("paste\ntest");

			text_edit->paste();
			CHECK(DS->clipboard_get() == "paste\ntest");
			CHECK(text_edit->get_text() == "thispaste\ntest is\nsopaste\ntestme\n\ntextpaste\ntest");
			CHECK(text_edit->get_caret_count() == 3);
			CHECK(text_edit->get_caret_line(0) == 3);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_caret_line(1) == 6);
			CHECK(text_edit->get_caret_column(1) == 4);
			CHECK(text_edit->get_caret_line(2) == 1);
			CHECK(text_edit->get_caret_column(2) == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			text_edit->remove_secondary_carets();

			// Paste line per caret when the amount of lines is equal to the number of carets.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(2);
			text_edit->add_caret(3, 4);
			text_edit->add_caret(0, 4);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 }, { 1, 1 }, { 3, 3 } };
			DS->clipboard_set("paste\ntest\n1");

			text_edit->paste();
			CHECK(DS->clipboard_get() == "paste\ntest\n1");
			CHECK(text_edit->get_text() == "thispaste is\nsotestme\n\ntext1");
			CHECK(text_edit->get_caret_count() == 3);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 6);
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 5);
			CHECK(text_edit->get_caret_line(2) == 0);
			CHECK(text_edit->get_caret_column(2) == 9);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			text_edit->remove_secondary_carets();

			// Cannot paste when not editable.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(4);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			DS->clipboard_set("no paste");

			text_edit->set_editable(false);
			text_edit->paste();
			text_edit->set_editable(true);
			CHECK(DS->clipboard_get() == "no paste");
			CHECK(text_edit->get_text() == "this is\nsome\n\ntext");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] paste primary") {
			// Set size for mouse input.
			text_edit->set_size(Size2(200, 200));

			text_edit->grab_focus();
			DS->clipboard_set("");
			DS->clipboard_set_primary("");
			CHECK(DS->clipboard_get_primary() == "");

			// Select text with mouse to put into primary clipboard.
			text_edit->set_text("this is\nsome\n\ntext");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(0, 2).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 3).get_center() + Point2i(2, 0), MouseButtonMask::LEFT, Key::NONE);
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(text_edit->get_rect_at_line_column(1, 3).get_center() + Point2i(2, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			CHECK(DS->clipboard_get() == "");
			CHECK(DS->clipboard_get_primary() == "is is\nsom");
			CHECK(text_edit->get_text() == "this is\nsome\n\ntext");
			CHECK(text_edit->has_selection());
			CHECK(text_edit->get_selected_text() == "is is\nsom");
			CHECK(text_edit->get_selection_origin_line() == 0);
			CHECK(text_edit->get_selection_origin_column() == 2);
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 3);
			SIGNAL_CHECK_FALSE("text_set");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Middle click to paste at mouse.
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 4 } };

			SEND_GUI_MOUSE_BUTTON_EVENT(text_edit->get_rect_at_line_column(3, 2).get_center() + Point2i(2, 0), MouseButton::MIDDLE, MouseButtonMask::MIDDLE, Key::NONE);
			CHECK(DS->clipboard_get_primary() == "is is\nsom");
			CHECK(text_edit->get_text() == "this is\nsome\n\nteis is\nsomxt");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 4);
			CHECK(text_edit->get_caret_column() == 3);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Paste at mouse position if there is only one caret.
			text_edit->set_text("this is\nsome\n\ntext");
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButtonMask::NONE, Key::NONE);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			DS->clipboard_set_primary("paste");
			lines_edited_args = { { 0, 0 } };

			text_edit->paste_primary_clipboard();
			CHECK(DS->clipboard_get_primary() == "paste");
			CHECK(text_edit->get_text() == "tpastehis is\nsome\n\ntext");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 6);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Paste at all carets if there are multiple carets.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(0);
			text_edit->add_caret(2, 0);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(0, 1).get_center() + Point2i(2, 0), MouseButtonMask::NONE, Key::NONE);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			DS->clipboard_set_primary("paste");
			lines_edited_args = { { 1, 1 }, { 2, 2 } };

			text_edit->paste_primary_clipboard();
			CHECK(DS->clipboard_get_primary() == "paste");
			CHECK(text_edit->get_text() == "this is\npastesome\npaste\ntext");
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 5);
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 5);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Cannot paste if not editable.
			text_edit->set_text("this is\nsome\n\ntext");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(4);
			SEND_GUI_MOUSE_MOTION_EVENT(text_edit->get_rect_at_line_column(1, 3).get_center() + Point2i(2, 0), MouseButtonMask::NONE, Key::NONE);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			DS->clipboard_set("no paste");

			text_edit->set_editable(false);
			text_edit->paste_primary_clipboard();
			text_edit->set_editable(true);
			CHECK(DS->clipboard_get() == "no paste");
			CHECK(text_edit->get_text() == "this is\nsome\n\ntext");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 4);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] cut when empty selection clipboard disabled") {
			text_edit->set_empty_selection_clipboard_enabled(false);
			DS->clipboard_set("");

			text_edit->set_text("this is\nsome\n");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(6);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			text_edit->cut();
			CHECK(DS->clipboard_get() == "");
			CHECK(text_edit->get_text() == "this is\nsome\n");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 6);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] copy when empty selection clipboard disabled") {
			text_edit->set_empty_selection_clipboard_enabled(false);
			DS->clipboard_set("");

			text_edit->set_text("this is\nsome\n");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(6);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			text_edit->copy();
			CHECK(DS->clipboard_get() == "");
			CHECK(text_edit->get_text() == "this is\nsome\n");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 6);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SIGNAL_UNWATCH(text_edit, "text_set");
		SIGNAL_UNWATCH(text_edit, "text_changed");
		SIGNAL_UNWATCH(text_edit, "lines_edited_from");
		SIGNAL_UNWATCH(text_edit, "caret_changed");
	}

	SUBCASE("[TextEdit] input") {
		SIGNAL_WATCH(text_edit, "text_set");
		SIGNAL_WATCH(text_edit, "text_changed");
		SIGNAL_WATCH(text_edit, "lines_edited_from");
		SIGNAL_WATCH(text_edit, "caret_changed");

		Array lines_edited_args = { { 0, 0 } };

		SUBCASE("[TextEdit] ui_text_newline_above") {
			text_edit->set_text("this is some test text.\nthis is some test text.");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Insert new line above.
			text_edit->select(0, 0, 0, 4);
			text_edit->add_caret(1, 4);
			CHECK(text_edit->get_caret_count() == 2);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 1 }, { 2, 3 } };

			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\nthis is some test text.\n\nthis is some test text.");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is some test text.\nthis is some test text.");
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "\nthis is some test text.\n\nthis is some test text.");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(4);
			text_edit->set_caret_line(3, false, true, -1, 1);
			text_edit->set_caret_column(4, false, 1);
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\nthis is some test text.\n\nthis is some test text.");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// Works on first line, empty lines, and only happens at caret for selections.
			text_edit->select(1, 10, 0, 0);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 1 }, { 4, 5 } };

			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n\nthis is some test text.\n\n\nthis is some test text.");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 4);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Insert multiple new lines above from one line.
			text_edit->set_text("test");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(1);
			text_edit->add_caret(0, 3);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 1 }, { 1, 2 } };

			SEND_GUI_ACTION("ui_text_newline_above");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n\ntest");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
		}

		SUBCASE("[TextEdit] ui_text_newline_blank") {
			text_edit->set_text("this is some test text.\nthis is some test text.");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");

			// Insert new line below.
			text_edit->select(0, 0, 0, 4);
			text_edit->add_caret(1, 4);
			CHECK(text_edit->get_caret_count() == 2);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 1 }, { 2, 3 } };

			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is some test text.\n\nthis is some test text.\n");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is some test text.\nthis is some test text.");
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "this is some test text.\n\nthis is some test text.\n");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is some test text.\n\nthis is some test text.\n");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// Insert multiple new lines below from one line.
			text_edit->set_text("test");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(1);
			text_edit->add_caret(0, 3);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 1 }, { 0, 1 } };

			SEND_GUI_ACTION("ui_text_newline_blank");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "test\n\n");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 2);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
		}

		SUBCASE("[TextEdit] ui_text_newline") {
			text_edit->set_text("this is some test text.\nthis is some test text.");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Insert new line at caret.
			text_edit->select(0, 0, 0, 4);
			text_edit->add_caret(1, 4);
			CHECK(text_edit->get_caret_count() == 2);
			SIGNAL_DISCARD("caret_changed");
			// Lines edited: deletion, insert line, insert line.
			lines_edited_args = { { 0, 0 }, { 0, 1 }, { 2, 3 } };

			SEND_GUI_ACTION("ui_text_newline");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n is some test text.\nthis\n is some test text.");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is some test text.\nthis is some test text.");
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "\n is some test text.\nthis\n is some test text.");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_newline");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n is some test text.\nthis\n is some test text.");
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);
		}

		SUBCASE("[TextEdit] ui_text_backspace_all_to_left") {
			Ref<InputEvent> tmpevent = InputEventKey::create_reference(Key::BACKSPACE | KeyModifierMask::ALT | KeyModifierMask::CMD_OR_CTRL);
			InputMap::action_add_event("ui_text_backspace_all_to_left", tmpevent);

			text_edit->set_text("\nthis is some test text.\n\nthis is some test text.");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Remove all text to the left.
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(5);
			text_edit->add_caret(1, 2);
			text_edit->add_caret(1, 8);
			lines_edited_args = { { 1, 1 } };
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_backspace_all_to_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\nsome test text.\n\nthis is some test text.");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "\nthis is some test text.\n\nthis is some test text.");
			CHECK(text_edit->get_caret_count() == 3);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 5);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 2);
			CHECK_FALSE(text_edit->has_selection(2));
			CHECK(text_edit->get_caret_line(2) == 1);
			CHECK(text_edit->get_caret_column(2) == 8);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "\nsome test text.\n\nthis is some test text.");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Acts as a normal backspace with selections.
			text_edit->select(1, 5, 1, 9, 0);
			text_edit->add_caret(3, 4);
			text_edit->select(3, 7, 3, 4, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 3 }, { 1, 1 } };

			SEND_GUI_ACTION("ui_text_backspace_all_to_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\nsome  text.\n\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 5);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Acts as a normal backspace when at the start of a line.
			text_edit->set_caret_column(0);
			text_edit->set_caret_column(0, false, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 2 }, { 1, 0 } };

			SEND_GUI_ACTION("ui_text_backspace_all_to_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "some  text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_caret_column(text_edit->get_line(0).length());
			text_edit->set_caret_column(text_edit->get_line(1).length(), false, 1);
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_backspace_all_to_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "some  text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == text_edit->get_line(0).length());
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == text_edit->get_line(1).length());
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// Remove entire line content when at the end of the line.
			lines_edited_args = { { 1, 1 }, { 0, 0 } };

			SEND_GUI_ACTION("ui_text_backspace_all_to_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			text_edit->remove_secondary_carets();

			// Removing newline effectively happens after removing text.
			text_edit->set_text("test\nlines");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(0);
			text_edit->add_caret(1, 4);

			SEND_GUI_ACTION("ui_text_backspace_all_to_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "tests");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			text_edit->remove_secondary_carets();

			// Removing newline effectively happens after removing text, reverse caret order.
			text_edit->set_text("test\nlines");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(4);
			text_edit->add_caret(1, 0);

			SEND_GUI_ACTION("ui_text_backspace_all_to_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "tests");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			text_edit->remove_secondary_carets();

			InputMap::action_erase_event("ui_text_backspace_all_to_left", tmpevent);
		}

		SUBCASE("[TextEdit] ui_text_backspace_word") {
			text_edit->set_text("\nthis is some test text.\n\nthis is some test text.");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");

			// Acts as a normal backspace with selections.
			text_edit->select(1, 8, 1, 15);
			text_edit->add_caret(3, 6);
			text_edit->select(3, 10, 3, 6, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 3 }, { 1, 1 } };

			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\nthis is st text.\n\nthis ime test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 8);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 6);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			text_edit->end_complex_operation();

			lines_edited_args = { { 3, 2 }, { 1, 0 } };

			// Start of line should also be a normal backspace.
			text_edit->set_caret_column(0);
			text_edit->set_caret_column(0, false, 1);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is st text.\nthis ime test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is st text.\nthis ime test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// FIXME: Remove after GH-77101 is fixed.
			text_edit->start_action(TextEdit::ACTION_NONE);

			// Remove text to the start of the word to the left of the caret.
			text_edit->set_caret_column(text_edit->get_line(0).length());
			text_edit->set_caret_column(12, false, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 1 }, { 0, 0 } };

			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is st \nthis ime t text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 11);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 9);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is st text.\nthis ime test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 16);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 12);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "this is st \nthis ime t text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 11);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 9);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Removing newline effectively happens after removing text.
			text_edit->set_text("test\nlines");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(0);
			text_edit->add_caret(1, 4);

			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "tests");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			text_edit->remove_secondary_carets();

			// Removing newline effectively happens after removing text, reverse caret order.
			text_edit->set_text("test\nlines");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(4);
			text_edit->add_caret(1, 0);

			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "tests");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			text_edit->remove_secondary_carets();

			// Remove when there are no words, only symbols.
			text_edit->set_text("#{}");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(3);

			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
		}

		SUBCASE("[TextEdit] ui_text_backspace_word same line") {
			text_edit->set_text("test longwordtest test");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Multiple carets on the same line is handled.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(4);
			text_edit->add_caret(0, 11);
			text_edit->add_caret(0, 15);
			text_edit->add_caret(0, 9);
			SIGNAL_DISCARD("caret_changed");

			lines_edited_args = { { 0, 0 }, { 0, 0 } };

			SEND_GUI_ACTION("ui_text_backspace_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " st test");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 1);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			text_edit->undo();
			CHECK(text_edit->get_text() == "test longwordtest test");
			CHECK(text_edit->get_caret_count() == 4);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 11);
			CHECK_FALSE(text_edit->has_selection(2));
			CHECK(text_edit->get_caret_line(2) == 0);
			CHECK(text_edit->get_caret_column(2) == 15);
			CHECK_FALSE(text_edit->has_selection(3));
			CHECK(text_edit->get_caret_line(3) == 0);
			CHECK(text_edit->get_caret_column(3) == 9);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			text_edit->redo();
			CHECK(text_edit->get_text() == " st test");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 1);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
		}

		SUBCASE("[TextEdit] ui_text_backspace") {
			text_edit->set_text("\nthis is some test text.\n\nthis is some test text.");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");

			// Remove selected text when there are selections.
			text_edit->select(1, 0, 1, 4);
			text_edit->add_caret(3, 4);
			text_edit->select(3, 5, 3, 2, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 3 }, { 1, 1 } };

			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n is some test text.\n\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo remove selection.
			text_edit->undo();
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->get_text() == "\nthis is some test text.\n\nthis is some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 0);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 2);
			CHECK(text_edit->get_selection_origin_line(1) == 3);
			CHECK(text_edit->get_selection_origin_column(1) == 5);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo remove selection.
			text_edit->redo();
			CHECK(text_edit->get_text() == "\n is some test text.\n\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Remove the newline when at start of line.
			text_edit->set_caret_column(0, false, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 3, 2 }, { 1, 0 } };

			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo remove newline.
			text_edit->undo();
			CHECK(text_edit->get_text() == "\n is some test text.\n\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 3);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo remove newline.
			text_edit->redo();
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_caret_column(text_edit->get_line(0).length());
			text_edit->set_caret_column(15, false, 1);
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == text_edit->get_line(0).length());
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 15);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// FIXME: Remove after GH-77101 is fixed.
			text_edit->start_action(TextEdit::ACTION_NONE);

			// Backspace removes character to the left.
			lines_edited_args = { { 1, 1 }, { 0, 0 } };

			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some test text\nthis some testtext.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 18);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 14);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Backspace another character without changing caret.
			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some test tex\nthis some testext.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 17);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 13);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo both backspaces.
			lines_edited_args = { { 1, 1 }, { 0, 0 }, { 1, 1 }, { 0, 0 } };

			text_edit->undo();
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 19);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 15);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo both backspaces.
			text_edit->redo();
			CHECK(text_edit->get_text() == " is some test tex\nthis some testext.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 17);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 13);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Backspace with multiple carets that will overlap.
			text_edit->remove_secondary_carets();
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(8);
			text_edit->add_caret(0, 7);
			text_edit->add_caret(0, 9);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 }, { 0, 0 }, { 0, 0 } };

			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is sotest tex\nthis some testext.");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 6);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Select each line of text, from right to left. Remove selection to column 0.
			text_edit->select(0, text_edit->get_line(0).length(), 0, 0);
			text_edit->add_caret(1, 0);
			text_edit->select(1, text_edit->get_line(1).length(), 1, 0, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 1 }, { 0, 0 } };

			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_text() == "\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Backspace at start of first line does nothing.
			text_edit->remove_secondary_carets();
			text_edit->deselect();
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_backspace");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] ui_text_delete_all_to_right") {
			Ref<InputEvent> tmpevent = InputEventKey::create_reference(Key::BACKSPACE | KeyModifierMask::ALT | KeyModifierMask::CMD_OR_CTRL);
			InputMap::action_add_event("ui_text_delete_all_to_right", tmpevent);

			text_edit->set_text("this is some test text.\nthis is some test text.\n");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");

			// Remove all text to right of caret.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(18);
			text_edit->add_caret(0, 16);
			text_edit->add_caret(0, 20);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 } };

			SEND_GUI_ACTION("ui_text_delete_all_to_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is some tes\nthis is some test text.\n");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 16);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			lines_edited_args = { { 0, 0 } };

			text_edit->undo();
			CHECK(text_edit->get_text() == "this is some test text.\nthis is some test text.\n");
			CHECK(text_edit->get_caret_count() == 3);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 18);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 16);
			CHECK_FALSE(text_edit->has_selection(2));
			CHECK(text_edit->get_caret_line(2) == 0);
			CHECK(text_edit->get_caret_column(2) == 20);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "this is some tes\nthis is some test text.\n");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 16);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Acts as a normal delete with selections.
			text_edit->select(0, 0, 0, 4);
			text_edit->add_caret(1, 4);
			text_edit->select(1, 8, 1, 4, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 }, { 1, 1 } };

			SEND_GUI_ACTION("ui_text_delete_all_to_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some tes\nthissome test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 4);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does nothing when caret is at end of line.
			text_edit->set_caret_column(text_edit->get_line(0).length());
			text_edit->set_caret_column(text_edit->get_line(1).length(), false, 1);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_delete_all_to_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some tes\nthissome test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == text_edit->get_line(0).length());
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == text_edit->get_line(1).length());
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Does not work if not editable.
			text_edit->set_caret_column(0);
			text_edit->set_caret_column(0, false, 1);
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_delete_all_to_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some tes\nthissome test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// Delete entire line.
			SEND_GUI_ACTION("ui_text_delete_all_to_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "\n\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			InputMap::action_erase_event("ui_text_delete_all_to_right", tmpevent);
		}

		SUBCASE("[TextEdit] ui_text_delete_word") {
			text_edit->set_caret_mid_grapheme_enabled(true);
			CHECK(text_edit->is_caret_mid_grapheme_enabled());

			text_edit->set_text("this is some test text.\n\nthis is some test text.\n");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Acts as a normal delete with selections.
			text_edit->select(0, 8, 0, 15);
			text_edit->add_caret(2, 6);
			text_edit->select(2, 10, 2, 6, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 }, { 2, 2 } };

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is st text.\n\nthis ime test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 8);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 6);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Removes newlines when at end of line.
			text_edit->set_caret_column(text_edit->get_line(0).length());
			text_edit->set_caret_column(text_edit->get_line(2).length(), false, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 }, { 2, 1 } };

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is st text.\nthis ime test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == text_edit->get_line(0).length());
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == text_edit->get_line(1).length());
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_caret_column(0);
			text_edit->set_caret_column(10, false, 1);
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is st text.\nthis ime test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 10);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// FIXME: Remove after GH-77101 is fixed.
			text_edit->start_action(TextEdit::ACTION_NONE);

			// Delete to the end of the word right of the caret.
			lines_edited_args = { { 0, 0 }, { 1, 1 } };

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is st text.\nthis ime t text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 10);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo.
			text_edit->undo();
			CHECK(text_edit->get_text() == "this is st text.\nthis ime test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 10);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == " is st text.\nthis ime t text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 10);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Delete one word with multiple carets.
			text_edit->remove_secondary_carets();
			text_edit->set_text("onelongword test");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(6);
			text_edit->add_caret(0, 9);
			text_edit->add_caret(0, 3);
			lines_edited_args = { { 0, 0 } };
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "one test");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 3);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Removing newline effectively happens after removing text.
			text_edit->set_text("test\nlines");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(2);
			text_edit->add_caret(0, 4);

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "telines");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 2);
			text_edit->remove_secondary_carets();

			// Removing newline effectively happens after removing text, reverse caret order.
			text_edit->set_text("test\nlines");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(4);
			text_edit->add_caret(0, 2);

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "telines");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 2);
			text_edit->remove_secondary_carets();

			// Remove when there are no words, only symbols.
			text_edit->set_text("#{}");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK_FALSE(text_edit->has_selection());
			CHECK(text_edit->get_text() == "");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
		}

		SUBCASE("[TextEdit] ui_text_delete_word same line") {
			text_edit->set_text("test longwordtest test");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Multiple carets on the same line is handled.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);
			text_edit->add_caret(0, 11);
			text_edit->add_caret(0, 15);
			text_edit->add_caret(0, 9);
			SIGNAL_DISCARD("caret_changed");

			lines_edited_args = { { 0, 0 }, { 0, 0 } };

			SEND_GUI_ACTION("ui_text_delete_word");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " long test");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 5);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			lines_edited_args = { { 0, 0 }, { 0, 0 } };

			text_edit->undo();
			CHECK(text_edit->get_text() == "test longwordtest test");
			CHECK(text_edit->get_caret_count() == 4);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 11);
			CHECK_FALSE(text_edit->has_selection(2));
			CHECK(text_edit->get_caret_line(2) == 0);
			CHECK(text_edit->get_caret_column(2) == 15);
			CHECK_FALSE(text_edit->has_selection(3));
			CHECK(text_edit->get_caret_line(3) == 0);
			CHECK(text_edit->get_caret_column(3) == 9);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			text_edit->redo();
			CHECK(text_edit->get_text() == " long test");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 0);
			CHECK(text_edit->get_caret_column(1) == 5);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
		}

		SUBCASE("[TextEdit] ui_text_delete") {
			text_edit->set_caret_mid_grapheme_enabled(true);
			CHECK(text_edit->is_caret_mid_grapheme_enabled());

			text_edit->set_text("this is some test text.\n\nthis is some test text.\n");
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Remove selected text when there are selections.
			text_edit->select(0, 0, 0, 4);
			text_edit->add_caret(2, 2);
			text_edit->select(2, 5, 2, 2, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 0, 0 }, { 2, 2 } };

			SEND_GUI_ACTION("ui_text_delete");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some test text.\n\nthis some test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo remove selection.
			text_edit->undo();
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->get_text() == "this is some test text.\n\nthis is some test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 4);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 0);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 5);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo remove selection.
			text_edit->redo();
			CHECK(text_edit->get_text() == " is some test text.\n\nthis some test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Remove newline when at end of line.
			text_edit->set_caret_column(text_edit->get_line(0).length());
			text_edit->set_caret_column(text_edit->get_line(2).length(), false, 1);
			SIGNAL_DISCARD("caret_changed");
			lines_edited_args = { { 1, 0 }, { 2, 1 } };

			SEND_GUI_ACTION("ui_text_delete");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 19);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 20);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo remove newline.
			text_edit->undo();
			CHECK(text_edit->get_text() == " is some test text.\n\nthis some test text.\n");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 19);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 20);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo remove newline.
			text_edit->redo();
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 19);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 20);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_caret_column(0);
			text_edit->set_caret_column(15, false, 1);
			SIGNAL_DISCARD("caret_changed");

			text_edit->set_editable(false);
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 15);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			// FIXME: Remove after GH-77101 is fixed.
			text_edit->start_action(TextEdit::EditAction::ACTION_NONE);

			// Delete removes character to the right.
			lines_edited_args = { { 0, 0 }, { 1, 1 } };

			SEND_GUI_ACTION("ui_text_delete");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "is some test text.\nthis some test ext.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 15);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Delete another character without changing caret.
			SEND_GUI_ACTION("ui_text_delete");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "s some test text.\nthis some test xt.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 15);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo both deletes.
			lines_edited_args = { { 0, 0 }, { 1, 1 }, { 0, 0 }, { 1, 1 } };

			text_edit->undo();
			CHECK(text_edit->get_text() == " is some test text.\nthis some test text.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 15);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo both deletes.
			text_edit->redo();
			CHECK(text_edit->get_text() == "s some test text.\nthis some test xt.");
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 15);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Delete at end of last line does nothing.
			text_edit->remove_secondary_carets();
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(18);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_delete");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "s some test text.\nthis some test xt.");
			CHECK(text_edit->get_caret_count() == 1);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 18);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] ui_text_caret_word_left") {
			text_edit->set_text("\nthis is some test text.\nthis is some test text.");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(15);
			text_edit->add_caret(2, 10);
			text_edit->select(1, 10, 1, 15);
			text_edit->select(2, 15, 2, 10, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Deselect to start of previous word when selection is right to left.
			// Select to start of next word when selection is left to right.
#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::ALT | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);

			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "me ");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 13);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 10);
			CHECK(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "some te");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 8);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 15);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Select to start of word with shift.
			text_edit->deselect();
			text_edit->set_caret_column(7);
			text_edit->set_caret_column(16, false, 1);
			SIGNAL_DISCARD("caret_changed");

#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::ALT | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);

			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "is");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 5);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 7);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "tes");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 13);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 16);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Deselect and move caret to start of next word without shift.
			SEND_GUI_ACTION("ui_text_caret_word_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 8);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Moves to end of previous line when at start of line. Does nothing at start of text.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);
			text_edit->set_caret_column(0, false, 1);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_caret_word_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 23);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Move when there are no words, only symbols.
			text_edit->set_text("#{}");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(3);

			SEND_GUI_ACTION("ui_text_caret_word_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
		}

		SUBCASE("[TextEdit] ui_text_caret_left") {
			text_edit->set_text("\nthis is some test text.\nthis is some test text.");
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(7);
			text_edit->select(1, 3, 1, 7);
			text_edit->add_caret(2, 3);
			text_edit->select(2, 7, 2, 3, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Remove one character from selection when selection is left to right.
			// Add one character to selection when selection is right to left.
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "s i");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 6);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 3);
			CHECK(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "is is");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 7);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Deselect and put caret at selection start without shift.
			SEND_GUI_ACTION("ui_text_caret_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 3);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Move caret one character to the left.
			SEND_GUI_ACTION("ui_text_caret_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 2);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 1);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Select one character to the left with shift and no existing selection.
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "h");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 1);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 2);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "t");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 0);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 1);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Moves to end of previous line when at start of line. Does nothing at start of text.
			text_edit->deselect();
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);
			text_edit->set_caret_column(0, false, 1);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_caret_left");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 23);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Selects to end of previous line when at start of line.
			text_edit->remove_secondary_carets();
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(0);
			text_edit->select(1, 1, 1, 0);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "\nt");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 1);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Merge selections when they overlap.
			text_edit->set_caret_line(1);
			text_edit->set_caret_column(4);
			text_edit->select(1, 6, 1, 4);
			text_edit->add_caret(1, 8);
			text_edit->select(1, 8, 1, 6, 1);
			SIGNAL_DISCARD("caret_changed");
			CHECK(text_edit->get_caret_count() == 2);

			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "s is ");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 3);
			CHECK(text_edit->get_selection_origin_line(0) == 1);
			CHECK(text_edit->get_selection_origin_column(0) == 8);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] ui_text_caret_word_right") {
			text_edit->set_text("this is some test text\n\nthis is some test text");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(15);
			text_edit->add_caret(2, 10);
			text_edit->select(0, 10, 0, 15);
			text_edit->select(2, 15, 2, 10, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Select to end of next word when selection is right to left.
			// Deselect to end of previous word when selection is left to right.
#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::ALT | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "me test");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 17);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 10);
			CHECK(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == " te");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 12);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 15);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Select to end of word with shift.
			text_edit->deselect();
			text_edit->set_caret_column(13);
			text_edit->set_caret_column(15, false, 1);
			SIGNAL_DISCARD("caret_changed");
#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::ALT | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "test");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 17);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 13);
			CHECK(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "st");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 17);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 15);
			CHECK(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Deselect and move caret to end of next word without shift.
			SEND_GUI_ACTION("ui_text_caret_word_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 22);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 22);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Moves to start of next line when at end of line. Does nothing at end of text.
			SEND_GUI_ACTION("ui_text_caret_word_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 22);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Move when there are no words, only symbols.
			text_edit->set_text("#{}");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);

			SEND_GUI_ACTION("ui_text_caret_word_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 3);
		}

		SUBCASE("[TextEdit] ui_text_caret_right") {
			text_edit->set_text("this is some test text\n\nthis is some test text");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(19);
			text_edit->select(0, 15, 0, 19);
			text_edit->add_caret(2, 15);
			text_edit->select(2, 19, 2, 15, 1);
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Remove one character from selection when selection is right to left.
			// Add one character to selection when selection is left to right.
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "st te");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 20);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 15);
			CHECK(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "t t");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 16);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 19);
			CHECK_FALSE(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Deselect and put caret at selection end without shift.
			SEND_GUI_ACTION("ui_text_caret_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 20);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 19);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Move caret one character to the right.
			SEND_GUI_ACTION("ui_text_caret_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 21);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 20);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Select one character to the right with shift and no existing selection.
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "t");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 22);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 21);
			CHECK(text_edit->is_caret_after_selection_origin(0));

			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "x");
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 21);
			CHECK(text_edit->get_selection_origin_line(1) == 2);
			CHECK(text_edit->get_selection_origin_column(1) == 20);
			CHECK(text_edit->is_caret_after_selection_origin(1));

			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Moves to start of next line when at end of line. Does nothing at end of text.
			text_edit->deselect();
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(22);
			text_edit->set_caret_column(22, false, 1);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_ACTION("ui_text_caret_right");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 2);
			CHECK_FALSE(text_edit->has_selection(0));
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			CHECK(text_edit->get_caret_line(1) == 2);
			CHECK(text_edit->get_caret_column(1) == 22);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Selects to start of next line when at end of line.
			text_edit->remove_secondary_carets();
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(22);
			text_edit->select(0, 21, 0, 22);
			SIGNAL_DISCARD("caret_changed");

			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "t\n");
			CHECK(text_edit->get_caret_line(0) == 1);
			CHECK(text_edit->get_caret_column(0) == 0);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 21);
			CHECK(text_edit->is_caret_after_selection_origin(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Merge selections when they overlap.
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(4);
			text_edit->select(0, 4, 0, 6);
			text_edit->add_caret(0, 8);
			text_edit->select(0, 6, 0, 8, 1);
			SIGNAL_DISCARD("caret_changed");
			CHECK(text_edit->get_caret_count() == 2);

			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_count() == 1);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == " is s");
			CHECK(text_edit->get_caret_line(0) == 0);
			CHECK(text_edit->get_caret_column(0) == 9);
			CHECK(text_edit->get_selection_origin_line(0) == 0);
			CHECK(text_edit->get_selection_origin_column(0) == 4);
			CHECK(text_edit->is_caret_after_selection_origin(0));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] ui_text_caret_up") {
			text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);

			text_edit->set_size(Size2(110, 100));
			text_edit->set_text("this is some\nother test\nlines\ngo here\nthis is some\nother test\nlines\ngo here");
			text_edit->set_caret_line(3);
			text_edit->set_caret_column(7);

			text_edit->add_caret(7, 7);
			CHECK(text_edit->get_caret_count() == 2);

			// Lines 0 and 4 are wrapped into 2 parts: 'this is ' and 'some'.
			CHECK(text_edit->is_line_wrapped(0));
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Select + up should select everything to the left on that line.
			SEND_GUI_KEY_EVENT(Key::UP | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 5);
			CHECK(text_edit->get_selected_text(0) == "\ngo here");
			CHECK(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 6);
			CHECK(text_edit->get_caret_column(1) == 5);
			CHECK(text_edit->get_selected_text(1) == "\ngo here");
			CHECK(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Should deselect and move up.
			SEND_GUI_ACTION("ui_text_caret_up");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 5);
			CHECK(text_edit->get_caret_column(1) == 8);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Normal up over wrapped line.
			SEND_GUI_ACTION("ui_text_caret_up");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 4);
			CHECK(text_edit->get_caret_column(1) == 12);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Normal up over wrapped line to line 0.
			text_edit->set_caret_column(12, false);
			SEND_GUI_ACTION("ui_text_caret_up");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 4);
			CHECK(text_edit->get_caret_column(1) == 7);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Normal up from column 0 to a wrapped line.
			text_edit->remove_secondary_carets();
			text_edit->set_caret_line(5);
			text_edit->set_caret_column(0);
			SEND_GUI_ACTION("ui_text_caret_up");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 4);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK_FALSE(text_edit->has_selection(0));

			// Normal up to column 0 of a wrapped line.
			SEND_GUI_ACTION("ui_text_caret_up");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 4);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK_FALSE(text_edit->has_selection(0));
		}

		SUBCASE("[TextEdit] ui_text_caret_down") {
			text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);

			text_edit->set_size(Size2(110, 100));
			text_edit->set_text("go here\nlines\nother test\nthis is some\ngo here\nlines\nother test\nthis is some");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(7);

			text_edit->add_caret(4, 7);
			CHECK(text_edit->get_caret_count() == 2);


			// Lines 3 and 7 are wrapped into 2 parts: 'this is ' and 'some'.
			CHECK(text_edit->is_line_wrapped(3));
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			// Select + down should select everything to the right on that line.
			SEND_GUI_KEY_EVENT(Key::DOWN | KeyModifierMask::SHIFT);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 1);
			CHECK(text_edit->get_caret_column() == 5);
			CHECK(text_edit->get_selected_text(0) == "\nlines");
			CHECK(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 5);
			CHECK(text_edit->get_caret_column(1) == 5);
			CHECK(text_edit->get_selected_text(1) == "\nlines");
			CHECK(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Should deselect and move down.
			SEND_GUI_ACTION("ui_text_caret_down");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 2);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 6);
			CHECK(text_edit->get_caret_column(1) == 8);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Normal down over wrapped line.
			SEND_GUI_ACTION("ui_text_caret_down");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 7);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 7);
			CHECK(text_edit->get_caret_column(1) == 7);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Normal down over wrapped line to last wrapped line.
			text_edit->set_caret_column(7, false);
			SEND_GUI_ACTION("ui_text_caret_down");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 7);
			CHECK(text_edit->get_caret_column(1) == 12);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			// Normal down to column 0 of a wrapped line.
			text_edit->remove_secondary_carets();
			text_edit->set_caret_line(3);
			text_edit->set_caret_column(0);
			SEND_GUI_ACTION("ui_text_caret_down");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 8);
			CHECK_FALSE(text_edit->has_selection(0));

			// Normal down out of visual column 0 of a wrapped line moves to start of next line.
			SEND_GUI_ACTION("ui_text_caret_down");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 4);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK_FALSE(text_edit->has_selection(0));
		}

		SUBCASE("[TextEdit] ui_text_caret_document_start") {
			text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);

			text_edit->set_size(Size2(110, 100));
			text_edit->set_text("this is some\nother test\nlines\ngo here");
			text_edit->set_caret_line(4);
			text_edit->set_caret_column(7);

			text_edit->add_caret(3, 2);
			CHECK(text_edit->get_caret_count() == 2);


			CHECK(text_edit->is_line_wrapped(0));
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::UP | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::HOME | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is some\nother test\nlines\ngo here");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK(text_edit->get_selected_text() == "this is some\nother test\nlines\ngo here");
			CHECK(text_edit->has_selection());
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			CHECK(text_edit->get_caret_count() == 1);

			SEND_GUI_ACTION("ui_text_caret_document_start");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "this is some\nother test\nlines\ngo here");
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] ui_text_caret_document_end") {
			text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);

			text_edit->set_size(Size2(110, 100));
			text_edit->set_text("go here\nlines\nother test\nthis is some");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);

			text_edit->add_caret(1, 0);
			CHECK(text_edit->get_caret_count() == 2);

			CHECK(text_edit->is_line_wrapped(3));
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::DOWN | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::END | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "go here\nlines\nother test\nthis is some");
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK(text_edit->get_selected_text() == "go here\nlines\nother test\nthis is some");
			CHECK(text_edit->has_selection());
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			CHECK(text_edit->get_caret_count() == 1);

			SEND_GUI_ACTION("ui_text_caret_document_end");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "go here\nlines\nother test\nthis is some");
			CHECK(text_edit->get_caret_line() == 3);
			CHECK(text_edit->get_caret_column() == 12);
			CHECK_FALSE(text_edit->has_selection());
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] ui_text_caret_line_start") {
			text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);

			text_edit->set_size(Size2(110, 100));
			text_edit->set_text("  this is some\n  this is some");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(text_edit->get_line(0).length());

			text_edit->add_caret(1, text_edit->get_line(1).length());
			CHECK(text_edit->get_caret_count() == 2);

			CHECK(text_edit->is_line_wrapped(0));
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::LEFT | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::HOME | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 10);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "some");

			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 10);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "some");
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			SEND_GUI_ACTION("ui_text_caret_line_start");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 2);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 2);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			SEND_GUI_ACTION("ui_text_caret_line_start");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 0);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 0);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			SEND_GUI_ACTION("ui_text_caret_line_start");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 2);
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 2);
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] ui_text_caret_line_end") {
			text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);

			text_edit->set_size(Size2(110, 100));
			text_edit->set_text("  this is some\n  this is some");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);

			text_edit->add_caret(1, 0);
			CHECK(text_edit->get_caret_count() == 2);

			CHECK(text_edit->is_line_wrapped(0));
			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

#ifdef MACOS_ENABLED
			SEND_GUI_KEY_EVENT(Key::RIGHT | KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT);
#else
			SEND_GUI_KEY_EVENT(Key::END | KeyModifierMask::SHIFT);
#endif
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == 9);
			CHECK(text_edit->has_selection(0));
			CHECK(text_edit->get_selected_text(0) == "  this is");

			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == 9);
			CHECK(text_edit->has_selection(1));
			CHECK(text_edit->get_selected_text(1) == "  this is");
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");

			SEND_GUI_ACTION("ui_text_caret_line_end");
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line() == 0);
			CHECK(text_edit->get_caret_column() == text_edit->get_line(0).length());
			CHECK_FALSE(text_edit->has_selection(0));

			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_caret_line(1) == 1);
			CHECK(text_edit->get_caret_column(1) == text_edit->get_line(1).length());
			CHECK_FALSE(text_edit->has_selection(1));
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
		}

		SUBCASE("[TextEdit] unicode") {
			text_edit->set_text("\n");
			text_edit->set_caret_line(0);
			text_edit->set_caret_column(0);

			text_edit->add_caret(1, 0);
			CHECK(text_edit->get_caret_count() == 2);
			text_edit->insert_text_at_caret("a");

			SIGNAL_DISCARD("text_set");
			SIGNAL_DISCARD("text_changed");
			SIGNAL_DISCARD("lines_edited_from");
			SIGNAL_DISCARD("caret_changed");

			lines_edited_args = { { 0, 0 }, { 1, 1 } };

			SEND_GUI_KEY_EVENT(Key::A);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "aA\naA");
			CHECK(text_edit->get_caret_column() == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Undo reverts both carets.
			text_edit->undo();
			CHECK(text_edit->get_text() == "a\na");
			CHECK(text_edit->get_caret_column() == 1);
			CHECK(text_edit->get_caret_column(1) == 1);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", reverse_nested(lines_edited_args));

			// Redo.
			text_edit->redo();
			CHECK(text_edit->get_text() == "aA\naA");
			CHECK(text_edit->get_caret_column() == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			// Does not work if not editable.
			text_edit->set_editable(false);
			SEND_GUI_KEY_EVENT(Key::A);
			CHECK_FALSE(text_edit->get_viewport()->is_input_handled()); // Should this be handled?
			CHECK(text_edit->get_text() == "aA\naA");
			CHECK(text_edit->get_caret_column() == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK_FALSE("caret_changed");
			SIGNAL_CHECK_FALSE("text_changed");
			SIGNAL_CHECK_FALSE("lines_edited_from");
			text_edit->set_editable(true);

			lines_edited_args = { { 0, 0 }, { 0, 0 }, { 1, 1 }, { 1, 1 } };

			text_edit->select(0, 0, 0, 1);
			text_edit->select(1, 0, 1, 1, 1);
			SEND_GUI_KEY_EVENT(Key::B);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "BA\nBA");
			CHECK(text_edit->get_caret_column() == 1);
			CHECK(text_edit->get_caret_column(1) == 1);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			SEND_GUI_ACTION("ui_text_toggle_insert_mode");
			CHECK(text_edit->is_overtype_mode_enabled());

			SEND_GUI_KEY_EVENT(Key::B);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "BB\nBB");
			CHECK(text_edit->get_caret_column() == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);

			text_edit->select(0, 0, 0, 1);
			text_edit->select(1, 0, 1, 1, 1);
			SEND_GUI_KEY_EVENT(Key::A);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "AB\nAB");
			CHECK(text_edit->get_caret_column() == 1);
			CHECK(text_edit->get_caret_column(1) == 1);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
			text_edit->set_overtype_mode_enabled(false);
			CHECK_FALSE(text_edit->is_overtype_mode_enabled());

			lines_edited_args = { { 0, 0 }, { 1, 1 } };

			SEND_GUI_KEY_EVENT(Key::TAB);
			CHECK(text_edit->get_viewport()->is_input_handled());
			CHECK(text_edit->get_text() == "A\tB\nA\tB");
			CHECK(text_edit->get_caret_column() == 2);
			CHECK(text_edit->get_caret_column(1) == 2);
			SIGNAL_CHECK("caret_changed", empty_signal_args);
			SIGNAL_CHECK("text_changed", empty_signal_args);
			SIGNAL_CHECK("lines_edited_from", lines_edited_args);
		}

		SIGNAL_UNWATCH(text_edit, "text_set");
		SIGNAL_UNWATCH(text_edit, "text_changed");
		SIGNAL_UNWATCH(text_edit, "lines_edited_from");
		SIGNAL_UNWATCH(text_edit, "caret_changed");
	}

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] context menu") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	text_edit->set_size(Size2(800, 200));
	text_edit->set_line(0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vasius mattis leo, sed porta ex lacinia bibendum. Nunc bibendum pellentesque.");

	text_edit->set_context_menu_enabled(false);
	CHECK_FALSE(text_edit->is_context_menu_enabled());

	CHECK_FALSE(text_edit->is_menu_visible());
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(600, 10), MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
	CHECK_FALSE(text_edit->is_menu_visible());

	text_edit->set_context_menu_enabled(true);
	CHECK(text_edit->is_context_menu_enabled());

	CHECK_FALSE(text_edit->is_menu_visible());
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(700, 10), MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
	CHECK(text_edit->is_menu_visible());

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] versioning") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	// Action undo / redo states are tested in the action test e.g selection_delete.
	CHECK_FALSE(text_edit->has_undo());
	CHECK_FALSE(text_edit->has_redo());
	CHECK(text_edit->get_version() == 0);
	CHECK(text_edit->get_saved_version() == 0);

	text_edit->begin_complex_operation();
	text_edit->begin_complex_operation();
	text_edit->begin_complex_operation();

	text_edit->insert_text_at_caret("test");
	CHECK(text_edit->get_version() == 1);
	CHECK(text_edit->get_saved_version() == 0);
	CHECK(text_edit->has_undo());
	CHECK_FALSE(text_edit->has_redo());

	text_edit->end_complex_operation();

	// Can undo and redo mid op.
	text_edit->insert_text_at_caret(" nested");
	CHECK(text_edit->get_version() == 2);
	CHECK(text_edit->get_saved_version() == 0);
	CHECK(text_edit->has_undo());
	CHECK_FALSE(text_edit->has_redo());
	text_edit->undo();

	CHECK(text_edit->has_redo());
	text_edit->redo();

	text_edit->end_complex_operation();

	text_edit->insert_text_at_caret(" ops");
	CHECK(text_edit->get_version() == 3);
	CHECK(text_edit->get_saved_version() == 0);
	CHECK(text_edit->has_undo());
	CHECK_FALSE(text_edit->has_redo());

	text_edit->end_complex_operation();

	text_edit->tag_saved_version();
	CHECK(text_edit->get_saved_version() == 3);

	text_edit->undo();
	CHECK(text_edit->get_line(0) == "");
	CHECK(text_edit->get_version() == 0);
	CHECK(text_edit->get_saved_version() == 3);
	CHECK_FALSE(text_edit->has_undo());
	CHECK(text_edit->has_redo());

	text_edit->redo();
	CHECK(text_edit->get_line(0) == "test ops nested");
	CHECK(text_edit->get_version() == 3);
	CHECK(text_edit->get_saved_version() == 3);
	CHECK(text_edit->has_undo());
	CHECK_FALSE(text_edit->has_redo());

	text_edit->clear_undo_history();
	CHECK_FALSE(text_edit->has_undo());
	CHECK_FALSE(text_edit->has_redo());
	CHECK(text_edit->get_version() == 3); // Should this be cleared?
	CHECK(text_edit->get_saved_version() == 0);

	SUBCASE("[TextEdit] versioning selection") {
		text_edit->set_text("Godot Engine\nWaiting for Godot\nTest Text for multi carat\nLine 4 Text");
		text_edit->set_multiple_carets_enabled(true);

		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(0);

		CHECK(text_edit->get_caret_count() == 1);

		Array caret_index = { 0 };

		for (int i = 1; i < 4; i++) {
			caret_index.push_back(text_edit->add_caret(i, 0));
			CHECK((int)caret_index.back() >= 0);
		}

		CHECK(text_edit->get_caret_count() == 4);

		for (int i = 0; i < 4; i++) {
			text_edit->select(i, 0, i, 5, caret_index[i]);
		}

		CHECK(text_edit->get_caret_count() == 4);
		for (int i = 0; i < 4; i++) {
			CHECK(text_edit->has_selection(caret_index[i]));
			CHECK(text_edit->get_selection_from_line(caret_index[i]) == i);
			CHECK(text_edit->get_selection_from_column(caret_index[i]) == 0);
			CHECK(text_edit->get_selection_to_line(caret_index[i]) == i);
			CHECK(text_edit->get_selection_to_column(caret_index[i]) == 5);
		}
		text_edit->begin_complex_operation();
		text_edit->deselect();
		text_edit->set_text("New Line Text");
		text_edit->select(0, 0, 0, 7, 0);
		text_edit->end_complex_operation();

		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->get_selected_text(0) == "New Lin");

		text_edit->undo();

		CHECK(text_edit->get_caret_count() == 4);
		for (int i = 0; i < 4; i++) {
			CHECK(text_edit->has_selection(caret_index[i]));
			CHECK(text_edit->get_selection_from_line(caret_index[i]) == i);
			CHECK(text_edit->get_selection_from_column(caret_index[i]) == 0);
			CHECK(text_edit->get_selection_to_line(caret_index[i]) == i);
			CHECK(text_edit->get_selection_to_column(caret_index[i]) == 5);
		}
	}

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] search") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	text_edit->set_text("hay needle, hay\nHAY NEEDLE, HAY\nwordword.word.word");
	int length = text_edit->get_line(1).length();

	CHECK(text_edit->search("test", 0, 0, 0) == Point2i(-1, -1));
	CHECK(text_edit->search("test", TextEdit::SEARCH_MATCH_CASE, 0, 0) == Point2i(-1, -1));
	CHECK(text_edit->search("test", TextEdit::SEARCH_WHOLE_WORDS, 0, 0) == Point2i(-1, -1));
	CHECK(text_edit->search("test", TextEdit::SEARCH_BACKWARDS, 0, 0) == Point2i(-1, -1));

	CHECK(text_edit->search("test", 0, 1, length) == Point2i(-1, -1));
	CHECK(text_edit->search("test", TextEdit::SEARCH_MATCH_CASE, 1, length) == Point2i(-1, -1));
	CHECK(text_edit->search("test", TextEdit::SEARCH_WHOLE_WORDS, 1, length) == Point2i(-1, -1));
	CHECK(text_edit->search("test", TextEdit::SEARCH_BACKWARDS, 1, length) == Point2i(-1, -1));

	CHECK(text_edit->search("needle", 0, 0, 0) == Point2i(4, 0));
	CHECK(text_edit->search("needle", 0, 1, length) == Point2i(4, 0));
	CHECK(text_edit->search("needle", 0, 0, 5) == Point2i(4, 1));
	CHECK(text_edit->search("needle", TextEdit::SEARCH_BACKWARDS, 0, 0) == Point2i(4, 1));
	CHECK(text_edit->search("needle", TextEdit::SEARCH_BACKWARDS, 1, 5) == Point2i(4, 1));
	CHECK(text_edit->search("needle", TextEdit::SEARCH_BACKWARDS, 1, 3) == Point2i(4, 0));

	CHECK(text_edit->search("needle", TextEdit::SEARCH_MATCH_CASE, 0, 0) == Point2i(4, 0));
	CHECK(text_edit->search("needle", TextEdit::SEARCH_MATCH_CASE | TextEdit::SEARCH_BACKWARDS, 0, 0) == Point2i(4, 0));

	CHECK(text_edit->search("needle", TextEdit::SEARCH_WHOLE_WORDS | TextEdit::SEARCH_MATCH_CASE, 0, 0) == Point2i(4, 0));
	CHECK(text_edit->search("needle", TextEdit::SEARCH_WHOLE_WORDS | TextEdit::SEARCH_MATCH_CASE | TextEdit::SEARCH_BACKWARDS, 0, 0) == Point2i(4, 0));

	CHECK(text_edit->search("need", TextEdit::SEARCH_MATCH_CASE, 0, 0) == Point2i(4, 0));
	CHECK(text_edit->search("need", TextEdit::SEARCH_MATCH_CASE | TextEdit::SEARCH_BACKWARDS, 0, 0) == Point2i(4, 0));

	CHECK(text_edit->search("need", TextEdit::SEARCH_WHOLE_WORDS | TextEdit::SEARCH_MATCH_CASE, 0, 0) == Point2i(-1, -1));
	CHECK(text_edit->search("need", TextEdit::SEARCH_WHOLE_WORDS | TextEdit::SEARCH_MATCH_CASE | TextEdit::SEARCH_BACKWARDS, 0, 0) == Point2i(-1, -1));

	CHECK(text_edit->search("word", TextEdit::SEARCH_WHOLE_WORDS, 2, 0) == Point2i(9, 2));
	CHECK(text_edit->search("word", TextEdit::SEARCH_WHOLE_WORDS, 2, 10) == Point2i(14, 2));
	CHECK(text_edit->search(".word", TextEdit::SEARCH_WHOLE_WORDS, 2, 0) == Point2i(8, 2));
	CHECK(text_edit->search("word.", TextEdit::SEARCH_WHOLE_WORDS, 2, 0) == Point2i(9, 2));

	ERR_PRINT_OFF;
	CHECK(text_edit->search("", 0, 0, 0) == Point2i(-1, -1));
	CHECK(text_edit->search("needle", 0, -1, 0) == Point2i(-1, -1));
	CHECK(text_edit->search("needle", 0, 0, -1) == Point2i(-1, -1));
	CHECK(text_edit->search("needle", 0, 100, 0) == Point2i(-1, -1));
	CHECK(text_edit->search("needle", 0, 0, 100) == Point2i(-1, -1));
	ERR_PRINT_ON;

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] mouse") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	text_edit->set_size(Size2(800, 200));

	CHECK(text_edit->get_rect_at_line_column(0, 0).get_position() == Point2i(0, 0));

	text_edit->set_line(0, "A");
	CHECK(text_edit->get_rect_at_line_column(0, 1).get_position().x > 0);

	text_edit->clear(); // Necessary, otherwise the following test cases fail.

	text_edit->set_line(0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vasius mattis leo, sed porta ex lacinia bibendum. Nunc bibendum pellentesque.");

	CHECK(text_edit->get_word_at_pos(text_edit->get_pos_at_line_column(0, 1)) == "Lorem");
	CHECK(text_edit->get_word_at_pos(text_edit->get_pos_at_line_column(0, 9)) == "ipsum");

	ERR_PRINT_OFF;
	CHECK(text_edit->get_pos_at_line_column(0, -1) == Point2i(-1, -1));
	CHECK(text_edit->get_pos_at_line_column(-1, 0) == Point2i(-1, -1));
	CHECK(text_edit->get_pos_at_line_column(-1, -1) == Point2i(-1, -1));

	CHECK(text_edit->get_pos_at_line_column(0, 500) == Point2i(-1, -1));
	CHECK(text_edit->get_pos_at_line_column(2, 0) == Point2i(-1, -1));
	CHECK(text_edit->get_pos_at_line_column(2, 500) == Point2i(-1, -1));

	// Out of view.
	CHECK(text_edit->get_pos_at_line_column(0, text_edit->get_line(0).length() - 1) == Point2i(-1, -1));
	ERR_PRINT_ON;

	// Add method to get drawn column count?
	Point2i start_pos = text_edit->get_pos_at_line_column(0, 0);
	Point2i end_pos = text_edit->get_pos_at_line_column(0, 104);

	CHECK(text_edit->get_line_column_at_pos(Point2i(start_pos.x, start_pos.y)) == Point2i(0, 0));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x, end_pos.y)) == Point2i(103, 0));

	// Should this return Point2i(-1, -1) if its also < 0 not just > vis_lines.
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x - 100, end_pos.y), false) == Point2i(88, 0));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x, end_pos.y + 100), false) == Point2i(-1, -1));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x - 100, end_pos.y + 100), false) == Point2i(-1, -1));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x, end_pos.y - 100), false) == Point2i(103, 0));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x - 100, end_pos.y - 100), false) == Point2i(88, 0));

	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x - 100, end_pos.y)) == Point2i(88, 0));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x, end_pos.y + 100)) == Point2i(140, 0));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x - 100, end_pos.y + 100)) == Point2i(140, 0));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x, end_pos.y - 100)) == Point2i(103, 0));
	CHECK(text_edit->get_line_column_at_pos(Point2i(end_pos.x - 100, end_pos.y - 100)) == Point2i(88, 0));

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] caret") {
	TextEdit *text_edit = memnew(TextEdit);
	text_edit->set_context_menu_enabled(false); // Prohibit sending InputEvents to the context menu.
	SceneTree::get_root()->add_child(text_edit);

	text_edit->set_size(Size2(800, 200));
	text_edit->grab_focus();
	text_edit->set_line(0, "ffi");

	text_edit->set_caret_mid_grapheme_enabled(true);
	CHECK(text_edit->is_caret_mid_grapheme_enabled());

	SEND_GUI_ACTION("ui_text_caret_right");
	CHECK(text_edit->get_caret_column() == 1);

	SEND_GUI_ACTION("ui_text_caret_right");
	CHECK(text_edit->get_caret_column() == 2);

	SEND_GUI_ACTION("ui_text_caret_right");
	CHECK(text_edit->get_caret_column() == 3);

	SEND_GUI_ACTION("ui_text_caret_left");
	CHECK(text_edit->get_caret_column() == 2);

	text_edit->set_line(0, "Lorem  ipsum dolor sit amet, consectetur adipiscing elit. Donec vasius mattis leo, sed porta ex lacinia bibendum. Nunc bibendum pellentesque.");
	for (int i = 0; i < 3; i++) {
		text_edit->insert_line_at(0, "Lorem  ipsum dolor sit amet, consectetur adipiscing elit. Donec vasius mattis leo, sed porta ex lacinia bibendum. Nunc bibendum pellentesque.");
	}

	text_edit->set_caret_blink_enabled(false);
	CHECK_FALSE(text_edit->is_caret_blink_enabled());

	text_edit->set_caret_blink_enabled(true);
	CHECK(text_edit->is_caret_blink_enabled());

	text_edit->set_caret_blink_interval(10);
	CHECK(text_edit->get_caret_blink_interval() == 10);

	ERR_PRINT_OFF;
	text_edit->set_caret_blink_interval(-1);
	CHECK(text_edit->get_caret_blink_interval() == 10);

	text_edit->set_caret_blink_interval(0);
	CHECK(text_edit->get_caret_blink_interval() == 10);
	ERR_PRINT_ON;

	text_edit->set_caret_type(TextEdit::CaretType::CARET_TYPE_LINE);
	CHECK(text_edit->get_caret_type() == TextEdit::CaretType::CARET_TYPE_LINE);

	text_edit->set_caret_type(TextEdit::CaretType::CARET_TYPE_BLOCK);
	CHECK(text_edit->get_caret_type() == TextEdit::CaretType::CARET_TYPE_BLOCK);

	text_edit->set_caret_type(TextEdit::CaretType::CARET_TYPE_LINE);
	CHECK(text_edit->get_caret_type() == TextEdit::CaretType::CARET_TYPE_LINE);

	int caret_col = text_edit->get_caret_column();
	text_edit->set_move_caret_on_right_click_enabled(false);
	CHECK_FALSE(text_edit->is_move_caret_on_right_click_enabled());

	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(100, 1), MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
	CHECK(text_edit->get_caret_column() == caret_col);

	text_edit->set_move_caret_on_right_click_enabled(true);
	CHECK(text_edit->is_move_caret_on_right_click_enabled());

	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(100, 1), MouseButton::RIGHT, MouseButtonMask::RIGHT, Key::NONE);
	CHECK(text_edit->get_caret_column() != caret_col);

	text_edit->set_move_caret_on_right_click_enabled(false);
	CHECK_FALSE(text_edit->is_move_caret_on_right_click_enabled());

	text_edit->set_caret_column(0);
	CHECK(text_edit->get_word_under_caret() == "Lorem");

	text_edit->set_caret_column(4);
	CHECK(text_edit->get_word_under_caret() == "Lorem");

	text_edit->set_caret_column(1);
	text_edit->add_caret(0, 15);
	CHECK(text_edit->get_word_under_caret() == "Lorem\ndolor");
	text_edit->remove_secondary_carets();

	// Should this work?
	text_edit->set_caret_column(5);
	CHECK(text_edit->get_word_under_caret() == "Lorem");

	text_edit->set_caret_column(6);
	CHECK(text_edit->get_word_under_caret() == "");

	text_edit->set_caret_line(1);
	CHECK(text_edit->get_caret_line() == 1);

	text_edit->set_caret_line(-1);
	CHECK(text_edit->get_caret_line() == 0);
	text_edit->set_caret_line(100);
	CHECK(text_edit->get_caret_line() == 3);

	text_edit->set_caret_column(-1);
	CHECK(text_edit->get_caret_column() == 0);
	text_edit->set_caret_column(10000000);
	CHECK(text_edit->get_caret_column() == 141);

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] multicaret") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);
	text_edit->set_multiple_carets_enabled(true);

	Array empty_signal_args = { {} };

	SIGNAL_WATCH(text_edit, "caret_changed");

	text_edit->set_text("this is\nsome test\ntext");
	text_edit->set_caret_line(0);
	text_edit->set_caret_column(0);
	SIGNAL_DISCARD("caret_changed");

	SUBCASE("[TextEdit] add remove caret") {
		// Overlapping.
		CHECK(text_edit->add_caret(0, 0) == -1);
		SIGNAL_CHECK_FALSE("caret_changed");

		// Select.
		text_edit->select(2, 4, 0, 0);

		// Cannot add in selection.
		CHECK(text_edit->add_caret(0, 0) == -1);
		CHECK(text_edit->add_caret(2, 4) == -1);
		CHECK(text_edit->add_caret(1, 2) == -1);

		// Cannot add when out of bounds.
		CHECK(text_edit->add_caret(-1, 0) == -1);
		CHECK(text_edit->add_caret(5, 0) == -1);
		CHECK(text_edit->add_caret(0, 100) == -1);

		SIGNAL_CHECK_FALSE("caret_changed");

		CHECK(text_edit->get_caret_count() == 1);

		text_edit->deselect();
		SIGNAL_CHECK_FALSE("caret_changed");

		CHECK(text_edit->add_caret(0, 1) == 1);
		SIGNAL_CHECK("caret_changed", empty_signal_args);
		CHECK(text_edit->get_caret_count() == 2);

		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 0);

		CHECK(text_edit->get_caret_line(1) == 0);
		CHECK(text_edit->get_caret_column(1) == 1);

		ERR_PRINT_OFF;
		text_edit->remove_caret(-1);
		text_edit->remove_caret(5);
		ERR_PRINT_ON;
		CHECK(text_edit->get_caret_count() == 2);
		SIGNAL_CHECK_FALSE("caret_changed");

		text_edit->remove_caret(0);
		SIGNAL_CHECK_FALSE("caret_changed");
		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 1);

		ERR_PRINT_OFF;
		text_edit->remove_caret(0);
		CHECK(text_edit->get_caret_count() == 1);
		ERR_PRINT_ON;
	}

	SUBCASE("[TextEdit] sort carets") {
		Vector<int> sorted_carets = { 0, 1, 2 };

		// Ascending order.
		text_edit->remove_secondary_carets();
		text_edit->add_caret(0, 1);
		text_edit->add_caret(1, 0);
		CHECK(text_edit->get_sorted_carets() == sorted_carets);

		// Descending order.
		sorted_carets = { 2, 1, 0 };
		text_edit->remove_secondary_carets();
		text_edit->set_caret_line(1);
		text_edit->add_caret(0, 1);
		text_edit->add_caret(0, 0);
		CHECK(text_edit->get_sorted_carets() == sorted_carets);

		// Mixed order.
		sorted_carets = { 0, 2, 1, 3 };
		text_edit->remove_secondary_carets();
		text_edit->set_caret_line(0);
		text_edit->add_caret(1, 0);
		text_edit->add_caret(0, 1);
		text_edit->add_caret(1, 1);
		CHECK(text_edit->get_sorted_carets() == sorted_carets);

		// Overlapping carets.
		sorted_carets = { 0, 1, 3, 2 };
		text_edit->remove_secondary_carets();
		text_edit->add_caret(0, 1);
		text_edit->add_caret(1, 2);
		text_edit->add_caret(0, 2);
		text_edit->set_caret_column(1, false, 3);
		CHECK(text_edit->get_sorted_carets() == sorted_carets);

		// Sorted by selection start.
		sorted_carets = { 1, 0 };
		text_edit->remove_secondary_carets();
		text_edit->select(1, 3, 1, 5);
		text_edit->add_caret(2, 0);
		text_edit->select(1, 0, 2, 0, 1);
		CHECK(text_edit->get_sorted_carets() == sorted_carets);
	}

	SUBCASE("[TextEdit] merge carets") {
		text_edit->set_text("this is some text\nfor selection");

		// Don't merge carets that are not overlapping.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(4);
		text_edit->add_caret(0, 6);
		text_edit->add_caret(1, 6);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->get_caret_count() == 3);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->get_caret_line(1) == 0);
		CHECK(text_edit->get_caret_column(1) == 6);
		CHECK(text_edit->get_caret_line(2) == 1);
		CHECK(text_edit->get_caret_column(2) == 6);
		text_edit->remove_secondary_carets();

		// Don't merge when in a multicaret edit.
		text_edit->begin_multicaret_edit();
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(4);
		text_edit->add_caret(0, 4);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->is_in_mulitcaret_edit());
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->get_caret_line(1) == 0);
		CHECK(text_edit->get_caret_column(1) == 4);

		// Merge overlapping carets. Merge at the end of the multicaret edit.
		text_edit->end_multicaret_edit();
		CHECK_FALSE(text_edit->is_in_mulitcaret_edit());
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);

		// Don't merge selections that are not overlapping.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(4);
		text_edit->add_caret(0, 2);
		text_edit->add_caret(1, 4);
		text_edit->select(0, 4, 1, 2, 0);
		text_edit->select(0, 2, 0, 3, 1);
		text_edit->select(1, 4, 1, 8, 2);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->has_selection(1));
		CHECK(text_edit->has_selection(2));
		text_edit->remove_secondary_carets();
		text_edit->deselect();

		// Don't merge selections that are only touching.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(4);
		text_edit->add_caret(1, 2);
		text_edit->select(0, 4, 1, 2, 0);
		text_edit->select(1, 2, 1, 5, 1);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->has_selection(1));
		text_edit->remove_secondary_carets();
		text_edit->deselect();

		// Merge carets into selection.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(3);
		text_edit->add_caret(0, 2);
		text_edit->add_caret(1, 4);
		text_edit->add_caret(1, 8);
		text_edit->add_caret(1, 10);
		text_edit->select(0, 2, 1, 8, 0);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_from_line(0) == 0);
		CHECK(text_edit->get_selection_from_column(0) == 2);
		CHECK(text_edit->get_selection_to_line(0) == 1);
		CHECK(text_edit->get_selection_to_column(0) == 8);
		CHECK(text_edit->is_caret_after_selection_origin(0));
		CHECK_FALSE(text_edit->has_selection(1));
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 10);
		text_edit->remove_secondary_carets();
		text_edit->deselect();

		// Merge partially overlapping selections.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(1);
		text_edit->add_caret(0, 2);
		text_edit->add_caret(0, 3);
		text_edit->select(0, 2, 0, 6, 0);
		text_edit->select(0, 4, 1, 3, 1);
		text_edit->select(1, 0, 1, 5, 2);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_from_line(0) == 0);
		CHECK(text_edit->get_selection_from_column(0) == 2);
		CHECK(text_edit->get_selection_to_line(0) == 1);
		CHECK(text_edit->get_selection_to_column(0) == 5);
		CHECK(text_edit->is_caret_after_selection_origin(0));
		text_edit->remove_secondary_carets();
		text_edit->deselect();

		// Merge smaller overlapping selection into a bigger one.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(1);
		text_edit->add_caret(0, 2);
		text_edit->add_caret(0, 3);
		text_edit->select(0, 2, 0, 6, 0);
		text_edit->select(0, 8, 1, 3, 1);
		text_edit->select(0, 2, 1, 5, 2);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_from_line(0) == 0);
		CHECK(text_edit->get_selection_from_column(0) == 2);
		CHECK(text_edit->get_selection_to_line(0) == 1);
		CHECK(text_edit->get_selection_to_column(0) == 5);
		CHECK(text_edit->is_caret_after_selection_origin(0));
		text_edit->remove_secondary_carets();
		text_edit->deselect();

		// Merge equal overlapping selections.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(1);
		text_edit->add_caret(0, 2);
		text_edit->select(0, 2, 1, 6, 0);
		text_edit->select(0, 2, 1, 6, 1);
		text_edit->merge_overlapping_carets();
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_from_line(0) == 0);
		CHECK(text_edit->get_selection_from_column(0) == 2);
		CHECK(text_edit->get_selection_to_line(0) == 1);
		CHECK(text_edit->get_selection_to_column(0) == 6);
		CHECK(text_edit->is_caret_after_selection_origin(0));
	}

	SUBCASE("[TextEdit] collapse carets") {
		text_edit->set_text("this is some text\nfor selection");

		// Collapse carets in range, dont affect other carets.
		text_edit->add_caret(0, 9);
		text_edit->add_caret(1, 0);
		text_edit->add_caret(1, 2);
		text_edit->add_caret(1, 6);
		text_edit->begin_multicaret_edit();

		text_edit->collapse_carets(0, 8, 1, 2);
		CHECK(text_edit->get_caret_count() == 5);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 0);
		CHECK(text_edit->get_caret_line(1) == 0);
		CHECK(text_edit->get_caret_column(1) == 8);
		CHECK(text_edit->get_caret_line(2) == 0);
		CHECK(text_edit->get_caret_column(2) == 8);
		CHECK(text_edit->get_caret_line(3) == 1);
		CHECK(text_edit->get_caret_column(3) == 2);
		CHECK(text_edit->get_caret_line(4) == 1);
		CHECK(text_edit->get_caret_column(4) == 6);
		CHECK_FALSE(text_edit->multicaret_edit_ignore_caret(0));
		CHECK(text_edit->multicaret_edit_ignore_caret(1));
		CHECK(text_edit->multicaret_edit_ignore_caret(2));
		CHECK_FALSE(text_edit->multicaret_edit_ignore_caret(3));
		CHECK_FALSE(text_edit->multicaret_edit_ignore_caret(4));

		// Collapsed carets get merged at the end of the edit.
		text_edit->end_multicaret_edit();
		CHECK(text_edit->get_caret_count() == 4);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 0);
		CHECK(text_edit->get_caret_line(1) == 0);
		CHECK(text_edit->get_caret_column(1) == 8);
		CHECK(text_edit->get_caret_line(2) == 1);
		CHECK(text_edit->get_caret_column(2) == 2);
		CHECK(text_edit->get_caret_line(3) == 1);
		CHECK(text_edit->get_caret_column(3) == 6);
		text_edit->remove_secondary_carets();

		// Collapse inclusive.
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(3);
		text_edit->add_caret(1, 2);
		text_edit->collapse_carets(0, 3, 1, 2, true);
		CHECK(text_edit->get_caret_count() == 1);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line() == 0);
		CHECK(text_edit->get_caret_column() == 3);
		text_edit->remove_secondary_carets();

		// Deselect if selection was encompassed.
		text_edit->select(0, 5, 0, 7);
		text_edit->collapse_carets(0, 3, 1, 2);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line() == 0);
		CHECK(text_edit->get_caret_column() == 3);

		// Clamp only caret end of selection.
		text_edit->select(0, 1, 0, 7);
		text_edit->collapse_carets(0, 3, 1, 2);
		CHECK(text_edit->has_selection());
		CHECK(text_edit->get_caret_line() == 0);
		CHECK(text_edit->get_caret_column() == 3);
		CHECK(text_edit->get_selection_origin_line() == 0);
		CHECK(text_edit->get_selection_origin_column() == 1);
		text_edit->deselect();

		// Clamp only selection origin end of selection.
		text_edit->select(0, 7, 0, 1);
		text_edit->collapse_carets(0, 3, 1, 2);
		CHECK(text_edit->has_selection());
		CHECK(text_edit->get_caret_line() == 0);
		CHECK(text_edit->get_caret_column() == 1);
		CHECK(text_edit->get_selection_origin_line() == 0);
		CHECK(text_edit->get_selection_origin_column() == 3);
		text_edit->deselect();
	}

	SUBCASE("[TextEdit] add caret at carets") {
		text_edit->remove_secondary_carets();
		text_edit->set_caret_line(1);
		text_edit->set_caret_column(9);

		// Add caret below. Column will clamp.
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 2);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 9);
		CHECK(text_edit->get_caret_line(1) == 2);
		CHECK(text_edit->get_caret_column(1) == 4);

		// Cannot add below when at last line.
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 2);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 9);
		CHECK(text_edit->get_caret_line(1) == 2);
		CHECK(text_edit->get_caret_column(1) == 4);

		// Add caret above. Column will clamp.
		text_edit->add_caret_at_carets(false);
		CHECK(text_edit->get_caret_count() == 3);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 9);
		CHECK(text_edit->get_caret_line(1) == 2);
		CHECK(text_edit->get_caret_column(1) == 4);
		CHECK(text_edit->get_caret_line(2) == 0);
		CHECK(text_edit->get_caret_column(2) == 7);

		// Cannot add above when at first line.
		text_edit->add_caret_at_carets(false);
		CHECK(text_edit->get_caret_count() == 3);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 9);
		CHECK(text_edit->get_caret_line(1) == 2);
		CHECK(text_edit->get_caret_column(1) == 4);
		CHECK(text_edit->get_caret_line(2) == 0);
		CHECK(text_edit->get_caret_column(2) == 7);

		// Cannot add below when at the last line for selection.
		text_edit->remove_secondary_carets();
		text_edit->select(2, 1, 2, 4);
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 2);
		CHECK(text_edit->get_selection_origin_column(0) == 1);
		CHECK(text_edit->get_caret_line(0) == 2);
		CHECK(text_edit->get_caret_column(0) == 4);

		// Cannot add above when at the first line for selection.
		text_edit->select(0, 1, 0, 4);
		text_edit->add_caret_at_carets(false);
		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 0);
		CHECK(text_edit->get_selection_origin_column(0) == 1);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);

		// Add selection below.
		text_edit->select(0, 0, 0, 4);
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 0);
		CHECK(text_edit->get_selection_origin_column(0) == 0);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->has_selection(1));
		CHECK(text_edit->get_selection_origin_line(1) == 1);
		CHECK(text_edit->get_selection_origin_column(1) == 0);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 3); // In the default font, this is the same position.

		// Add selection below again.
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 0);
		CHECK(text_edit->get_selection_origin_column(0) == 0);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->has_selection(1));
		CHECK(text_edit->get_selection_origin_line(1) == 1);
		CHECK(text_edit->get_selection_origin_column(1) == 0);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 3);
		CHECK(text_edit->has_selection(2));
		CHECK(text_edit->get_selection_origin_line(2) == 2);
		CHECK(text_edit->get_selection_origin_column(2) == 0);
		CHECK(text_edit->get_caret_line(2) == 2);
		CHECK(text_edit->get_caret_column(2) == 4);

		text_edit->set_text("\tthis is\nsome\n\ttest text");

		// Last fit x is preserved when adding below.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(6);
		text_edit->add_caret_at_carets(true);
		text_edit->add_caret_at_carets(true);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 6);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 4);
		CHECK(text_edit->get_caret_line(2) == 2);
		CHECK(text_edit->get_caret_column(2) == 6);

		// Last fit x is preserved when adding above.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(2);
		text_edit->set_caret_column(9);
		text_edit->add_caret_at_carets(false);
		text_edit->add_caret_at_carets(false);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->get_caret_line(0) == 2);
		CHECK(text_edit->get_caret_column(0) == 9);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 4);
		CHECK(text_edit->get_caret_line(2) == 0);
		CHECK(text_edit->get_caret_column(2) == 8);

		// Last fit x is preserved when selection adding below.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->select(0, 8, 0, 5);
		text_edit->add_caret_at_carets(true);
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 0);
		CHECK(text_edit->get_selection_origin_column(0) == 8);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 5);
		CHECK_FALSE(text_edit->has_selection(1));
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 4);
		CHECK(text_edit->has_selection(2));
		CHECK(text_edit->get_selection_origin_line(2) == 2);
		CHECK(text_edit->get_selection_origin_column(2) == 7);
		CHECK(text_edit->get_caret_line(2) == 2);
		CHECK(text_edit->get_caret_column(2) == 5);

		// Last fit x is preserved when selection adding above.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->select(2, 9, 2, 5);
		text_edit->add_caret_at_carets(false);
		text_edit->add_caret_at_carets(false);
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 2);
		CHECK(text_edit->get_selection_origin_column(0) == 9);
		CHECK(text_edit->get_caret_line(0) == 2);
		CHECK(text_edit->get_caret_column(0) == 5);
		CHECK_FALSE(text_edit->has_selection(1));
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 4);
		CHECK(text_edit->has_selection(2));
		CHECK(text_edit->get_selection_origin_line(2) == 0);
		CHECK(text_edit->get_selection_origin_column(2) == 8);
		CHECK(text_edit->get_caret_line(2) == 0);
		CHECK(text_edit->get_caret_column(2) == 5);

		// Selections are merged when they overlap.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->select(0, 1, 0, 5);
		text_edit->add_caret(1, 0);
		text_edit->select(1, 1, 1, 3, 1);
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 0);
		CHECK(text_edit->get_selection_origin_column(0) == 1);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 5);
		CHECK(text_edit->has_selection(1));
		CHECK(text_edit->get_selection_origin_line(1) == 1);
		CHECK(text_edit->get_selection_origin_column(1) == 1);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 4);
		CHECK(text_edit->has_selection(2));
		CHECK(text_edit->get_selection_origin_line(2) == 2);
		CHECK(text_edit->get_selection_origin_column(2) == 0);
		CHECK(text_edit->get_caret_line(2) == 2);
		CHECK(text_edit->get_caret_column(2) == 3);

		// Multiline selection.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(1);
		text_edit->select(0, 3, 1, 1);
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->has_selection(0));
		CHECK(text_edit->get_selection_origin_line(0) == 0);
		CHECK(text_edit->get_selection_origin_column(0) == 3);
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 1);
		CHECK(text_edit->has_selection(1));
		CHECK(text_edit->get_selection_origin_line(1) == 1);
		CHECK(text_edit->get_selection_origin_column(1) == 3);
		CHECK(text_edit->get_caret_line(1) == 2);
		CHECK(text_edit->get_caret_column(1) == 0);

		text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
		text_edit->set_size(Size2(50, 100));
		// Line wraps: `\t,this, is\nso,me\n\t,test, ,text`.
		CHECK(text_edit->is_line_wrapped(0));

		// Add caret below on next line wrap.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(4);
		text_edit->add_caret_at_carets(true);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->get_caret_line(1) == 0);
		CHECK(text_edit->get_caret_column(1) == 8);

		// Add caret below from end of line wrap.
		text_edit->add_caret_at_carets(true);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->get_caret_line(1) == 0);
		CHECK(text_edit->get_caret_column(1) == 8);
		CHECK(text_edit->get_caret_line(2) == 1);
		CHECK(text_edit->get_caret_column(2) == 1);

		// Add caret below from last line and not last line wrap.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(2);
		text_edit->set_caret_column(5);
		text_edit->add_caret_at_carets(true);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->get_caret_line(0) == 2);
		CHECK(text_edit->get_caret_column(0) == 5);
		CHECK(text_edit->get_caret_line(1) == 2);
		CHECK(text_edit->get_caret_column(1) == 6);

		// Cannot add caret below from last line last line wrap.
		text_edit->add_caret_at_carets(true);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->get_caret_line(0) == 2);
		CHECK(text_edit->get_caret_column(0) == 5);
		CHECK(text_edit->get_caret_line(1) == 2);
		CHECK(text_edit->get_caret_column(1) == 6);

		// Add caret above from not first line wrap.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(1);
		text_edit->set_caret_column(4);
		text_edit->add_caret_at_carets(false);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 2);
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 1);

		// Add caret above from first line wrap.
		text_edit->add_caret_at_carets(false);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 3);
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 1);
		CHECK(text_edit->get_caret_line(2) == 0);
		CHECK(text_edit->get_caret_column(2) == 8);

		// Add caret above from first line and not first line wrap.
		text_edit->add_caret_at_carets(false);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 4);
		CHECK(text_edit->get_caret_line(0) == 1);
		CHECK(text_edit->get_caret_column(0) == 4);
		CHECK(text_edit->get_caret_line(1) == 1);
		CHECK(text_edit->get_caret_column(1) == 1);
		CHECK(text_edit->get_caret_line(2) == 0);
		CHECK(text_edit->get_caret_column(2) == 8);
		CHECK(text_edit->get_caret_line(3) == 0);
		CHECK(text_edit->get_caret_column(3) == 4);

		// Cannot add caret above from first line first line wrap.
		text_edit->remove_secondary_carets();
		text_edit->deselect();
		text_edit->set_caret_line(0);
		text_edit->set_caret_column(0);
		text_edit->add_caret_at_carets(false);
		CHECK_FALSE(text_edit->has_selection());
		CHECK(text_edit->get_caret_count() == 1);
		CHECK(text_edit->get_caret_line(0) == 0);
		CHECK(text_edit->get_caret_column(0) == 0);

		// Does nothing if multiple carets are disabled.
		text_edit->set_multiple_carets_enabled(false);
		text_edit->add_caret_at_carets(true);
		CHECK(text_edit->get_caret_count() == 1);
	}

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] line wrapping") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);
	text_edit->grab_focus();

	// Set size for boundary.
	text_edit->set_size(Size2(800, 200));
	text_edit->set_line(0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vasius mattis leo, sed porta ex lacinia bibendum. Nunc bibendum pellentesque.");
	CHECK_FALSE(text_edit->is_line_wrapped(0));
	CHECK(text_edit->get_line_wrap_count(0) == 0);
	CHECK(text_edit->get_line_wrap_index_at_column(0, 130) == 0);
	CHECK(text_edit->get_line_wrapped_text(0).size() == 1);

	SIGNAL_WATCH(text_edit, "text_set");
	SIGNAL_WATCH(text_edit, "text_changed");
	SIGNAL_WATCH(text_edit, "lines_edited_from");
	SIGNAL_WATCH(text_edit, "caret_changed");

	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
	SIGNAL_CHECK_FALSE("text_set");
	SIGNAL_CHECK_FALSE("text_changed");
	SIGNAL_CHECK_FALSE("lines_edited_from");
	SIGNAL_CHECK_FALSE("caret_changed");

	CHECK(text_edit->is_line_wrapped(0));
	CHECK(text_edit->get_line_wrap_count(0) == 1);
	CHECK(text_edit->get_line_wrap_index_at_column(0, 130) == 1);
	CHECK(text_edit->get_line_wrapped_text(0).size() == 2);

	SIGNAL_UNWATCH(text_edit, "text_set");
	SIGNAL_UNWATCH(text_edit, "text_changed");
	SIGNAL_UNWATCH(text_edit, "lines_edited_from");
	SIGNAL_UNWATCH(text_edit, "caret_changed");

	ERR_PRINT_OFF;
	CHECK_FALSE(text_edit->is_line_wrapped(-1));
	CHECK_FALSE(text_edit->is_line_wrapped(1));
	CHECK(text_edit->get_line_wrap_count(-1) == 0);
	CHECK(text_edit->get_line_wrap_count(1) == 0);
	CHECK(text_edit->get_line_wrap_index_at_column(-1, 0) == 0);
	CHECK(text_edit->get_line_wrap_index_at_column(0, -1) == 0);
	CHECK(text_edit->get_line_wrap_index_at_column(1, 0) == 0);
	CHECK(text_edit->get_line_wrap_index_at_column(0, 10000) == 0);
	CHECK(text_edit->get_line_wrapped_text(-1).size() == 0);
	CHECK(text_edit->get_line_wrapped_text(1).size() == 0);
	ERR_PRINT_ON;

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] viewport") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	// No subcases here for performance.
	text_edit->set_size(Size2(800, 600));
	for (int i = 0; i < 50; i++) {
		text_edit->insert_line_at(0, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Donec vasius mattis leo, sed porta ex lacinia bibendum. Nunc bibendum pellentesque.");
	}

	const int visible_lines = text_edit->get_visible_line_count();
	const int total_visible_lines = text_edit->get_total_visible_line_count();
	CHECK(total_visible_lines == 51);

	// First visible line.
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	text_edit->set_line_as_first_visible(visible_lines);
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(visible_lines)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	ERR_PRINT_OFF;
	text_edit->set_line_as_first_visible(-1);
	text_edit->set_line_as_first_visible(500);
	text_edit->set_line_as_first_visible(0, -1);
	text_edit->set_line_as_first_visible(0, 500);
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	ERR_PRINT_ON;

	// Wrap.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
	CHECK(text_edit->get_total_visible_line_count() > total_visible_lines);

	text_edit->set_line_as_first_visible(5, 1);
	CHECK(text_edit->get_first_visible_line() == 5);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(11)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 6);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 1);

	// Reset.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);
	CHECK(text_edit->get_total_visible_line_count() == total_visible_lines);
	text_edit->set_line_as_first_visible(0);
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Last visible line.
	text_edit->set_line_as_last_visible(visible_lines * 2);
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(visible_lines)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	ERR_PRINT_OFF;
	text_edit->set_line_as_last_visible(-1);
	text_edit->set_line_as_last_visible(500);
	text_edit->set_line_as_last_visible(0, -1);
	text_edit->set_line_as_last_visible(0, 500);
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	ERR_PRINT_ON;

	// Wrap.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
	CHECK(text_edit->get_total_visible_line_count() > total_visible_lines);

	text_edit->set_line_as_last_visible(visible_lines + 5, 1);
	CHECK(text_edit->get_first_visible_line() == 16);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(32.0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines + 5);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Reset.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);
	CHECK(text_edit->get_total_visible_line_count() == total_visible_lines);
	text_edit->set_line_as_first_visible(0);
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Center.
	text_edit->set_line_as_center_visible(visible_lines + (visible_lines / 2));
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(visible_lines)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	ERR_PRINT_OFF;
	text_edit->set_line_as_last_visible(-1);
	text_edit->set_line_as_last_visible(500);
	text_edit->set_line_as_last_visible(0, -1);
	text_edit->set_line_as_last_visible(0, 500);
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	ERR_PRINT_ON;

	// Wrap.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
	CHECK(text_edit->get_total_visible_line_count() > total_visible_lines);

	text_edit->set_line_as_center_visible(visible_lines + (visible_lines / 2) + 5, 1);
	CHECK(text_edit->get_first_visible_line() == visible_lines + (visible_lines / 2));
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double((visible_lines * 3))));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 1);

	// Scroll past eof.
	int line_count = text_edit->get_line_count();
	text_edit->set_scroll_past_end_of_file_enabled(true);
	text_edit->set_line_as_center_visible(line_count - 1);

	CHECK(text_edit->get_first_visible_line() == (visible_lines * 2) + 3);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double((visible_lines * 4))) + 6);
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) + 8);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	text_edit->set_scroll_past_end_of_file_enabled(false);
	CHECK(text_edit->get_first_visible_line() == (visible_lines * 2) + 3);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double((visible_lines * 4))) - 4);
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) + 8);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Reset.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);
	CHECK(text_edit->get_total_visible_line_count() == total_visible_lines);
	text_edit->set_line_as_first_visible(0);
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Auto adjust - todo: horizontal scroll.
	// Below.
	CHECK_FALSE(text_edit->is_caret_visible());
	text_edit->set_caret_line(visible_lines + 5, false);
	CHECK_FALSE(text_edit->is_caret_visible());
	text_edit->adjust_viewport_to_caret();
	CHECK(text_edit->is_caret_visible());
	CHECK(text_edit->get_first_visible_line() == 5);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(5)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines - 1) + 5);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	text_edit->center_viewport_to_caret();
	CHECK(text_edit->get_first_visible_line() == visible_lines - 5);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(visible_lines - 5)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 6);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Caret visible, do nothing.
	text_edit->adjust_viewport_to_caret();
	CHECK(text_edit->get_first_visible_line() == visible_lines - 5);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(visible_lines - 5)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 6);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Above.
	text_edit->set_caret_line(1, false);
	text_edit->adjust_viewport_to_caret();
	CHECK(text_edit->is_caret_visible());
	CHECK(text_edit->get_first_visible_line() == 1);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(1)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	text_edit->set_line_as_first_visible(0);
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	text_edit->adjust_viewport_to_caret();
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Wrap.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_BOUNDARY);
	CHECK(text_edit->get_total_visible_line_count() > total_visible_lines);

	text_edit->set_caret_line(visible_lines + 5, false, true, 1);
	text_edit->adjust_viewport_to_caret();

	CHECK(text_edit->get_first_visible_line() == (visible_lines / 2) + 6);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double((visible_lines + (visible_lines / 2)))) + 1);
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines) + 5);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 1);

	text_edit->center_viewport_to_caret();
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double((visible_lines * 2))) + 1);
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 11);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 1);

	// Caret visible, do nothing.
	text_edit->adjust_viewport_to_caret();
	CHECK(text_edit->get_first_visible_line() == visible_lines);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double((visible_lines * 2))) + 1);
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 11);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 1);

	// Above.
	text_edit->set_caret_line(1, false, true, 1);
	text_edit->adjust_viewport_to_caret();
	CHECK(text_edit->is_caret_visible());
	CHECK(text_edit->get_first_visible_line() == 1);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(3)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines / 2) + 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 1);
	CHECK(text_edit->get_caret_wrap_index() == 1);

	text_edit->set_line_as_first_visible(0);
	CHECK(text_edit->is_caret_visible());
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 11);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	text_edit->adjust_viewport_to_caret();
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 11);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	// Reset.
	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);
	CHECK(text_edit->get_total_visible_line_count() == total_visible_lines);
	text_edit->set_line_as_first_visible(0);
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	// Smooth scroll.
	text_edit->set_v_scroll_speed(10);
	CHECK(text_edit->get_v_scroll_speed() == 10);
	ERR_PRINT_OFF;
	text_edit->set_v_scroll_speed(-1);
	CHECK(text_edit->get_v_scroll_speed() == 10);

	text_edit->set_v_scroll_speed(0);
	CHECK(text_edit->get_v_scroll_speed() == 10);

	text_edit->set_v_scroll_speed(1);
	CHECK(text_edit->get_v_scroll_speed() == 1);
	ERR_PRINT_ON;

	// Scroll.
	int v_scroll = text_edit->get_v_scroll();
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(10, 10), MouseButton::WHEEL_DOWN, MouseButtonMask::NONE, Key::NONE);
	CHECK(text_edit->get_v_scroll() > v_scroll);
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(10, 10), MouseButton::WHEEL_UP, MouseButtonMask::NONE, Key::NONE);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(v_scroll)));

	// smooth scroll speed.
	text_edit->set_smooth_scroll_enabled(true);

	v_scroll = text_edit->get_v_scroll();
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(10, 10), MouseButton::WHEEL_DOWN, MouseButtonMask::NONE, Key::NONE);
	text_edit->notification(TextEdit::NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
	CHECK(text_edit->get_v_scroll() >= v_scroll);
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(10, 10), MouseButton::WHEEL_UP, MouseButtonMask::NONE, Key::NONE);
	text_edit->notification(TextEdit::NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(v_scroll)));

	v_scroll = text_edit->get_v_scroll();
	text_edit->set_v_scroll_speed(10000);
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(10, 10), MouseButton::WHEEL_DOWN, MouseButtonMask::NONE, Key::NONE);
	text_edit->notification(TextEdit::NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
	CHECK(text_edit->get_v_scroll() >= v_scroll);
	SEND_GUI_MOUSE_BUTTON_EVENT(Point2i(10, 10), MouseButton::WHEEL_UP, MouseButtonMask::NONE, Key::NONE);
	text_edit->notification(TextEdit::NOTIFICATION_INTERNAL_PHYSICS_PROCESS);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(v_scroll)));

	ERR_PRINT_OFF;
	CHECK(text_edit->get_scroll_pos_for_line(-1) == 0);
	CHECK(text_edit->get_scroll_pos_for_line(1000) == 0);
	CHECK(text_edit->get_scroll_pos_for_line(1, -1) == 0);
	CHECK(text_edit->get_scroll_pos_for_line(1, 100) == 0);
	ERR_PRINT_ON;

	text_edit->set_h_scroll(-100);
	CHECK(text_edit->get_h_scroll() == 0);

	text_edit->set_h_scroll(10000000);
	CHECK(text_edit->get_h_scroll() == 307);
	CHECK(text_edit->get_h_scroll_bar()->get_bound_minimum_size().x == 8);

	text_edit->set_h_scroll(-100);
	CHECK(text_edit->get_h_scroll() == 0);

	text_edit->set_smooth_scroll_enabled(false);

	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);

	text_edit->grab_focus();
	SEND_GUI_ACTION("ui_text_scroll_down");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 1);
	CHECK(text_edit->get_first_visible_line() == 1);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(1)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_scroll_up");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 1);
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	// Page down, similar to VSCode, to end of page then scroll.
	SEND_GUI_ACTION("ui_text_caret_page_down");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 21);
	CHECK(text_edit->get_first_visible_line() == 0);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(0)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_caret_page_down");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 41);
	CHECK(text_edit->get_first_visible_line() == 20);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(20)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines - 1) * 2);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_caret_page_up");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 21);
	CHECK(text_edit->get_first_visible_line() == 20);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(20)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines - 1) * 2);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_caret_page_up");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 1);
	CHECK(text_edit->get_first_visible_line() == 1);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(1)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);

	text_edit->grab_focus();
	SEND_GUI_ACTION("ui_text_scroll_down");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 2);
	CHECK(text_edit->get_first_visible_line() == 2);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(2)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines + 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_scroll_up");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 2);
	CHECK(text_edit->get_first_visible_line() == 1);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(1)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	// Page down, similar to VSCode, to end of page then scroll.
	SEND_GUI_ACTION("ui_text_caret_page_down");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 22);
	CHECK(text_edit->get_first_visible_line() == 1);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(1)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_caret_page_down");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 42);
	CHECK(text_edit->get_first_visible_line() == 21);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(21)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_caret_page_up");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 22);
	CHECK(text_edit->get_first_visible_line() == 21);
	CHECK(Math::is_equal_approx(Math::floor(text_edit->get_v_scroll()), double(21)));
	CHECK(text_edit->get_last_full_visible_line() == (visible_lines * 2) - 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	SEND_GUI_ACTION("ui_text_caret_page_up");
	CHECK(text_edit->get_viewport()->is_input_handled());
	CHECK(text_edit->get_caret_line() == 2);
	CHECK(text_edit->get_first_visible_line() == 2);
	CHECK(Math::is_equal_approx(text_edit->get_v_scroll(), double(2)));
	CHECK(text_edit->get_last_full_visible_line() == visible_lines + 1);
	CHECK(text_edit->get_last_full_visible_line_wrap_index() == 0);
	CHECK(text_edit->get_caret_wrap_index() == 0);

	// Typing and undo / redo should adjust viewport.
	text_edit->set_caret_line(0);
	text_edit->set_caret_column(0);
	text_edit->set_line_as_first_visible(5);
	CHECK(text_edit->get_first_visible_line() == 5);

	SEND_GUI_KEY_EVENT(Key::A);
	CHECK(text_edit->get_first_visible_line() == 0);

	text_edit->set_line_as_first_visible(5);
	CHECK(text_edit->get_first_visible_line() == 5);

	text_edit->undo();
	CHECK(text_edit->get_first_visible_line() == 0);

	text_edit->set_line_as_first_visible(5);
	CHECK(text_edit->get_first_visible_line() == 5);

	text_edit->redo();
	CHECK(text_edit->get_first_visible_line() == 0);

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] small height value") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	text_edit->set_size(Size2(800, 32));
	text_edit->set_text("0\n1\n2");

	text_edit->set_v_scroll(100);
	CHECK(text_edit->get_v_scroll() < 3);

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] fit content scrollbar behavior") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	text_edit->set_line_wrapping_mode(TextEdit::LineWrappingMode::LINE_WRAPPING_NONE);
	text_edit->set_fit_content_width_enabled(true);
	text_edit->set_fit_content_height_enabled(true);

	const String long_line = "0123456789012345678901234567890123456789012345678901234567890123456789";
	String content;
	for (int i = 0; i < 40; i++) {
		if (i > 0) {
			content += "\n";
		}
		content += long_line;
	}
	text_edit->set_text(content);

	SUBCASE("[TextEdit] fit content without maximum size") {
		text_edit->set_custom_maximum_size(Size2(-1, -1));
		text_edit->set_size(text_edit->get_combined_minimum_size());

		CHECK_FALSE(text_edit->get_h_scroll_bar()->is_visible());
		CHECK_FALSE(text_edit->get_v_scroll_bar()->is_visible());
	}

	SUBCASE("[TextEdit] fit content with maximum size") {
		text_edit->set_custom_maximum_size(Size2(180, 120));
		text_edit->set_size(text_edit->get_combined_maximum_size());

		CHECK(text_edit->get_h_scroll_bar()->is_visible());
		CHECK(text_edit->get_v_scroll_bar()->is_visible());
	}

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] setter getters") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	SUBCASE("[TextEdit] set and get placeholder") {
		text_edit->set_placeholder("test\nplaceholder");
		CHECK(text_edit->get_placeholder() == "test\nplaceholder");

		CHECK(text_edit->get_text() == "");
		CHECK(text_edit->get_line_count() == 1);
		CHECK(text_edit->get_last_full_visible_line() == 0);
	}

	SUBCASE("[TextEdit] highlight current line") {
		text_edit->set_highlight_current_line(true);
		CHECK(text_edit->is_highlight_current_line_enabled());
		text_edit->set_highlight_current_line(false);
		CHECK_FALSE(text_edit->is_highlight_current_line_enabled());
	}

	SUBCASE("[TextEdit] highlight all occurrences") {
		text_edit->set_highlight_all_occurrences(true);
		CHECK(text_edit->is_highlight_all_occurrences_enabled());
		text_edit->set_highlight_all_occurrences(false);
		CHECK_FALSE(text_edit->is_highlight_all_occurrences_enabled());
	}

	SUBCASE("[TextEdit] draw control chars") {
		text_edit->set_draw_control_chars(true);
		CHECK(text_edit->get_draw_control_chars());
		text_edit->set_draw_control_chars(false);
		CHECK_FALSE(text_edit->get_draw_control_chars());
	}

	SUBCASE("[TextEdit] draw tabs") {
		text_edit->set_draw_tabs(true);
		CHECK(text_edit->is_drawing_tabs());
		text_edit->set_draw_tabs(false);
		CHECK_FALSE(text_edit->is_drawing_tabs());
	}

	SUBCASE("[TextEdit] draw spaces") {
		text_edit->set_draw_spaces(true);
		CHECK(text_edit->is_drawing_spaces());
		text_edit->set_draw_spaces(false);
		CHECK_FALSE(text_edit->is_drawing_spaces());
	}

	SUBCASE("[TextEdit] draw minimap") {
		text_edit->set_draw_minimap(true);
		CHECK(text_edit->is_drawing_minimap());
		text_edit->set_draw_minimap(false);
		CHECK_FALSE(text_edit->is_drawing_minimap());
	}

	SUBCASE("[TextEdit] minimap width") {
		text_edit->set_minimap_width(-1);
		CHECK(text_edit->get_minimap_width() == -1);
		text_edit->set_minimap_width(1000);
		CHECK(text_edit->get_minimap_width() == 1000);
	}

	SUBCASE("[TextEdit] line color background") {
		ERR_PRINT_OFF;
		text_edit->set_line_background_color(-1, Color("#ff0000"));
		text_edit->set_line_background_color(0, Color("#00ff00"));
		text_edit->set_line_background_color(1, Color("#0000ff"));

		CHECK(text_edit->get_line_background_color(-1) == Color());
		CHECK(text_edit->get_line_background_color(0) == Color("#00ff00"));
		CHECK(text_edit->get_line_background_color(1) == Color());
		ERR_PRINT_ON;

		text_edit->set_line_background_color(0, Color("#ffff00"));
		CHECK(text_edit->get_line_background_color(0) == Color("#ffff00"));
	}

	memdelete(text_edit);
}

TEST_CASE("[SceneTree][TextEdit] gutters") {
	TextEdit *text_edit = memnew(TextEdit);
	SceneTree::get_root()->add_child(text_edit);

	Array empty_signal_args = { {} };

	SIGNAL_WATCH(text_edit, "gutter_clicked");
	SIGNAL_WATCH(text_edit, "gutter_added");
	SIGNAL_WATCH(text_edit, "gutter_removed");

	SUBCASE("[TextEdit] gutter add and remove") {
		text_edit->set_text("test1\ntest2\ntest3\ntest4");

		text_edit->add_gutter();
		CHECK(text_edit->get_gutter_count() == 1);
		CHECK(text_edit->get_gutter_width(0) == 24);
		CHECK(text_edit->get_total_gutter_width() == 24 + 2);
		SIGNAL_CHECK("gutter_added", empty_signal_args);

		text_edit->set_gutter_name(0, "test_gutter");
		CHECK(text_edit->get_gutter_name(0) == "test_gutter");

		text_edit->set_gutter_width(0, 10);
		CHECK(text_edit->get_gutter_width(0) == 10);
		CHECK(text_edit->get_total_gutter_width() == 10 + 2);

		text_edit->add_gutter(-100);
		text_edit->set_gutter_width(1, 10);
		CHECK(text_edit->get_gutter_width(1) == 10);
		CHECK(text_edit->get_total_gutter_width() == 20 + 2);
		CHECK(text_edit->get_gutter_count() == 2);
		CHECK(text_edit->get_gutter_name(0) == "test_gutter");
		SIGNAL_CHECK("gutter_added", empty_signal_args);

		text_edit->set_gutter_draw(1, false);
		CHECK(text_edit->get_total_gutter_width() == 10 + 2);

		text_edit->add_gutter(100);
		CHECK(text_edit->get_gutter_count() == 3);
		CHECK(text_edit->get_gutter_width(2) == 24);
		CHECK(text_edit->get_total_gutter_width() == 34 + 2);
		CHECK(text_edit->get_gutter_name(0) == "test_gutter");
		SIGNAL_CHECK("gutter_added", empty_signal_args);

		text_edit->add_gutter(0);
		CHECK(text_edit->get_gutter_count() == 4);
		CHECK(text_edit->get_gutter_width(0) == 24);
		CHECK(text_edit->get_total_gutter_width() == 58 + 2);
		CHECK(text_edit->get_gutter_name(1) == "test_gutter");
		SIGNAL_CHECK("gutter_added", empty_signal_args);

		text_edit->remove_gutter(2);
		CHECK(text_edit->get_gutter_name(1) == "test_gutter");
		CHECK(text_edit->get_gutter_count() == 3);
		CHECK(text_edit->get_total_gutter_width() == 58 + 2);
		SIGNAL_CHECK("gutter_removed", empty_signal_args);

		text_edit->remove_gutter(0);
		CHECK(text_edit->get_gutter_name(0) == "test_gutter");
		CHECK(text_edit->get_gutter_count() == 2);
		CHECK(text_edit->get_total_gutter_width() == 34 + 2);
		SIGNAL_CHECK("gutter_removed", empty_signal_args);

		ERR_PRINT_OFF;
		text_edit->remove_gutter(-1);
		SIGNAL_CHECK_FALSE("gutter_removed");

		text_edit->remove_gutter(100);
		SIGNAL_CHECK_FALSE("gutter_removed");

		CHECK(text_edit->get_gutter_name(-1) == "");
		CHECK(text_edit->get_gutter_name(100) == "");
		ERR_PRINT_ON;
	}

	SUBCASE("[TextEdit] gutter data") {
		text_edit->add_gutter();
		CHECK(text_edit->get_gutter_count() == 1);
		SIGNAL_CHECK("gutter_added", empty_signal_args);

		text_edit->set_gutter_name(0, "test_gutter");
		CHECK(text_edit->get_gutter_name(0) == "test_gutter");

		text_edit->set_gutter_width(0, 10);
		CHECK(text_edit->get_gutter_width(0) == 10);

		text_edit->set_gutter_clickable(0, true);
		CHECK(text_edit->is_gutter_clickable(0));

		text_edit->set_gutter_overwritable(0, true);
		CHECK(text_edit->is_gutter_overwritable(0));

		text_edit->set_gutter_type(0, TextEdit::GutterType::GUTTER_TYPE_CUSTOM);
		CHECK(text_edit->get_gutter_type(0) == TextEdit::GutterType::GUTTER_TYPE_CUSTOM);

		text_edit->set_text("test\ntext");

		ERR_PRINT_OFF;
		text_edit->set_line_gutter_metadata(1, 0, "test");
		text_edit->set_line_gutter_metadata(0, -1, "test");
		text_edit->set_line_gutter_metadata(0, 2, "test");
		text_edit->set_line_gutter_metadata(2, 0, "test");
		text_edit->set_line_gutter_metadata(-1, 0, "test");

		CHECK(text_edit->get_line_gutter_metadata(1, 0) == "test");
		CHECK(text_edit->get_line_gutter_metadata(0, -1) == "");
		CHECK(text_edit->get_line_gutter_metadata(0, 2) == "");
		CHECK(text_edit->get_line_gutter_metadata(2, 0) == "");
		CHECK(text_edit->get_line_gutter_metadata(-1, 0) == "");

		text_edit->set_line_gutter_text(1, 0, "test");
		text_edit->set_line_gutter_text(0, -1, "test");
		text_edit->set_line_gutter_text(0, 2, "test");
		text_edit->set_line_gutter_text(2, 0, "test");
		text_edit->set_line_gutter_text(-1, 0, "test");

		CHECK(text_edit->get_line_gutter_text(1, 0) == "test");
		CHECK(text_edit->get_line_gutter_text(0, -1) == "");
		CHECK(text_edit->get_line_gutter_text(0, 2) == "");
		CHECK(text_edit->get_line_gutter_text(2, 0) == "");
		CHECK(text_edit->get_line_gutter_text(-1, 0) == "");

		text_edit->set_line_gutter_item_color(1, 0, Color(1, 0, 0));
		text_edit->set_line_gutter_item_color(0, -1, Color(1, 0, 0));
		text_edit->set_line_gutter_item_color(0, 2, Color(1, 0, 0));
		text_edit->set_line_gutter_item_color(2, 0, Color(1, 0, 0));
		text_edit->set_line_gutter_item_color(-1, 0, Color(1, 0, 0));

		CHECK(text_edit->get_line_gutter_item_color(1, 0) == Color(1, 0, 0));
		CHECK(text_edit->get_line_gutter_item_color(0, -1) == Color());
		CHECK(text_edit->get_line_gutter_item_color(0, 2) == Color());
		CHECK(text_edit->get_line_gutter_item_color(2, 0) == Color());
		CHECK(text_edit->get_line_gutter_item_color(-1, 0) == Color());

		text_edit->set_line_gutter_clickable(1, 0, true);
		text_edit->set_line_gutter_clickable(0, -1, true);
		text_edit->set_line_gutter_clickable(0, 2, true);
		text_edit->set_line_gutter_clickable(2, 0, true);
		text_edit->set_line_gutter_clickable(-1, 0, true);

		CHECK(text_edit->is_line_gutter_clickable(1, 0) == true);
		CHECK(text_edit->is_line_gutter_clickable(0, -1) == false);
		CHECK(text_edit->is_line_gutter_clickable(0, 2) == false);
		CHECK(text_edit->is_line_gutter_clickable(2, 0) == false);
		CHECK(text_edit->is_line_gutter_clickable(-1, 0) == false);
		ERR_PRINT_ON;

		// Merging tested via TextEdit code mode gutters.
	}

	SUBCASE("[TextEdit] gutter mouse") {
		DisplayServerMock *DS = DisplayServer::driver_as<DisplayServerMock>();
		// Set size for mouse input.
		text_edit->set_size(Size2(200, 200));

		text_edit->set_text("test1\ntest2\ntest3\ntest4");
		text_edit->grab_focus();

		text_edit->add_gutter();
		text_edit->set_gutter_name(0, "test_gutter");
		text_edit->set_gutter_width(0, 10);
		text_edit->set_gutter_clickable(0, true);

		text_edit->add_gutter();
		text_edit->set_gutter_name(1, "test_gutter_not_clickable");
		text_edit->set_gutter_width(1, 10);
		text_edit->set_gutter_clickable(1, false);

		text_edit->add_gutter();
		CHECK(text_edit->get_gutter_count() == 3);
		text_edit->set_gutter_name(2, "test_gutter_3");
		text_edit->set_gutter_width(2, 10);
		text_edit->set_gutter_clickable(2, true);

		const int line_height = text_edit->get_line_height();

		// Defaults to none.
		CHECK(text_edit->get_hovered_gutter() == Vector2i(-1, -1));
		CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

		// Hover over gutter.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(5, line_height + line_height / 2), MouseButtonMask::NONE, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(0, 1));
		SIGNAL_CHECK_FALSE("gutter_clicked");
		CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_POINTING_HAND);

		// Click on gutter.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(5, line_height / 2), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(0, 0));
		SIGNAL_CHECK("gutter_clicked", Array({ { 0, 0 } }));

		// Click on gutter on another line.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(5, line_height * 3 + line_height / 2), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(0, 3));
		SIGNAL_CHECK("gutter_clicked", Array({ { 3, 0 } }));

		// Unclickable gutter can be hovered.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(15, line_height + line_height / 2), MouseButtonMask::NONE, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(1, 1));
		SIGNAL_CHECK_FALSE("gutter_clicked");
		CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

		// Unclickable gutter can be clicked.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(15, line_height * 2 + line_height / 2), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(1, 2));
		SIGNAL_CHECK("gutter_clicked", Array({ { 2, 1 } }));
		CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

		// Hover past last line.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(5, line_height * 5), MouseButtonMask::NONE, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(-1, -1));
		SIGNAL_CHECK_FALSE("gutter_clicked");
		CHECK(DS->get_cursor_shape() == DisplayServerEnums::CURSOR_ARROW);

		// Click on gutter past last line.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(5, line_height * 5), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(-1, -1));
		SIGNAL_CHECK_FALSE("gutter_clicked");

		// Mouse exit resets hover.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(5, line_height + line_height / 2), MouseButtonMask::NONE, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(0, 1));
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(-1, -1), MouseButtonMask::NONE, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(-1, -1));

		// Removing gutter updates hover.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(25, line_height + line_height / 2), MouseButtonMask::NONE, Key::NONE);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(2, 1));
		text_edit->remove_gutter(2);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(-1, -1));

		// Updating size updates hover.
		text_edit->set_gutter_width(1, 20);
		CHECK(text_edit->get_hovered_gutter() == Vector2i(1, 1));
	}

	SIGNAL_UNWATCH(text_edit, "gutter_clicked");
	SIGNAL_UNWATCH(text_edit, "gutter_added");
	SIGNAL_UNWATCH(text_edit, "gutter_removed");
	memdelete(text_edit);
}

} // namespace TestTextEdit

#endif // ADVANCED_GUI_DISABLED

// === Merged from tests/scene/test_texture_button.cpp ===
/*  test_texture_button.cpp                                               */
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
#include "scene/gui/texture_button.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

namespace TestTextureButton {

static Ref<ImageTexture> make_test_texture(int p_w, int p_h) {
	Ref<Image> image = memnew(Image(p_w, p_h, false, Image::FORMAT_RGB8));
	return ImageTexture::create_from_image(image);
}

TEST_CASE("[SceneTree][TextureButton] Default properties") {
	TextureButton *texture_button = memnew(TextureButton);

	CHECK(texture_button->get_texture_normal().is_null());
	CHECK(texture_button->get_texture_pressed().is_null());
	CHECK(texture_button->get_texture_hover().is_null());
	CHECK(texture_button->get_texture_disabled().is_null());
	CHECK(texture_button->get_texture_focused().is_null());
	CHECK(texture_button->get_click_mask().is_null());

	CHECK_FALSE(texture_button->get_ignore_texture_size());
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_KEEP);
	CHECK_FALSE(texture_button->is_flipped_h());
	CHECK_FALSE(texture_button->is_flipped_v());

	CHECK(texture_button->get_mouse_filter() == Control::MOUSE_FILTER_STOP);

	CHECK(texture_button->get_combined_minimum_size() == Size2(0, 0));

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] Texture setters and getters") {
	TextureButton *texture_button = memnew(TextureButton);

	Ref<ImageTexture> tex_a = make_test_texture(8, 8);
	Ref<ImageTexture> tex_b = make_test_texture(12, 10);

	SUBCASE("texture_normal") {
		texture_button->set_texture_normal(tex_a);
		CHECK(texture_button->get_texture_normal() == tex_a);
		texture_button->set_texture_normal(Ref<Texture2D>());
		CHECK(texture_button->get_texture_normal().is_null());
	}

	SUBCASE("texture_pressed") {
		texture_button->set_texture_pressed(tex_a);
		CHECK(texture_button->get_texture_pressed() == tex_a);
		texture_button->set_texture_pressed(Ref<Texture2D>());
		CHECK(texture_button->get_texture_pressed().is_null());
	}

	SUBCASE("texture_hover") {
		texture_button->set_texture_hover(tex_a);
		CHECK(texture_button->get_texture_hover() == tex_a);
		texture_button->set_texture_hover(Ref<Texture2D>());
		CHECK(texture_button->get_texture_hover().is_null());
	}

	SUBCASE("texture_disabled") {
		texture_button->set_texture_disabled(tex_a);
		CHECK(texture_button->get_texture_disabled() == tex_a);
		texture_button->set_texture_disabled(Ref<Texture2D>());
		CHECK(texture_button->get_texture_disabled().is_null());
	}

	SUBCASE("texture_focused") {
		texture_button->set_texture_focused(tex_a);
		CHECK(texture_button->get_texture_focused() == tex_a);
		texture_button->set_texture_focused(Ref<Texture2D>());
		CHECK(texture_button->get_texture_focused().is_null());
	}

	SUBCASE("Independent texture slots") {
		texture_button->set_texture_normal(tex_a);
		texture_button->set_texture_pressed(tex_b);
		CHECK(texture_button->get_texture_normal() == tex_a);
		CHECK(texture_button->get_texture_pressed() == tex_b);
	}

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] Click mask set and get") {
	TextureButton *texture_button = memnew(TextureButton);

	Ref<BitMap> mask;
	mask.instantiate();
	mask->create(Size2i(4, 3));

	texture_button->set_click_mask(mask);
	CHECK(texture_button->get_click_mask() == mask);

	texture_button->set_click_mask(Ref<BitMap>());
	CHECK(texture_button->get_click_mask().is_null());

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] Combined minimum size priority") {
	TextureButton *texture_button = memnew(TextureButton);
	Window *root = SceneTree::get_root();
	root->add_child(texture_button);

	Ref<ImageTexture> tex_n = make_test_texture(16, 8);
	Ref<ImageTexture> tex_p = make_test_texture(10, 20);
	Ref<ImageTexture> tex_h = make_test_texture(4, 4);

	SUBCASE("texture_normal wins when present") {
		texture_button->set_texture_normal(tex_n);
		texture_button->set_texture_pressed(tex_p);
		texture_button->set_texture_hover(tex_h);
		SceneTree::process(0);
		CHECK(texture_button->get_combined_minimum_size() == Size2(16, 8));
	}

	SUBCASE("texture_pressed when normal is empty") {
		texture_button->set_texture_pressed(tex_p);
		texture_button->set_texture_hover(tex_h);
		SceneTree::process(0);
		CHECK(texture_button->get_combined_minimum_size() == Size2(10, 20));
	}

	SUBCASE("texture_hover when normal and pressed are empty") {
		texture_button->set_texture_hover(tex_h);
		SceneTree::process(0);
		CHECK(texture_button->get_combined_minimum_size() == Size2(4, 4));
	}

	SUBCASE("click_mask size when no textures") {
		Ref<BitMap> mask;
		mask.instantiate();
		mask->create(Size2i(7, 5));
		texture_button->set_click_mask(mask);
		SceneTree::process(0);
		CHECK(texture_button->get_combined_minimum_size() == Size2(7, 5));
	}

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] ignore_texture_size") {
	TextureButton *texture_button = memnew(TextureButton);
	Window *root = SceneTree::get_root();
	root->add_child(texture_button);

	Ref<ImageTexture> tex = make_test_texture(32, 16);
	texture_button->set_texture_normal(tex);
	texture_button->set_ignore_texture_size(true);
	SceneTree::process(0);

	CHECK(texture_button->get_combined_minimum_size() == Size2(0, 0));

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] Stretch mode") {
	TextureButton *texture_button = memnew(TextureButton);

	texture_button->set_stretch_mode(TextureButton::STRETCH_SCALE);
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_SCALE);

	texture_button->set_stretch_mode(TextureButton::STRETCH_TILE);
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_TILE);

	texture_button->set_stretch_mode(TextureButton::STRETCH_KEEP);
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_KEEP);

	texture_button->set_stretch_mode(TextureButton::STRETCH_KEEP_CENTERED);
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_KEEP_CENTERED);

	texture_button->set_stretch_mode(TextureButton::STRETCH_KEEP_ASPECT);
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_KEEP_ASPECT);

	texture_button->set_stretch_mode(TextureButton::STRETCH_KEEP_ASPECT_CENTERED);
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_KEEP_ASPECT_CENTERED);

	texture_button->set_stretch_mode(TextureButton::STRETCH_KEEP_ASPECT_COVERED);
	CHECK(texture_button->get_stretch_mode() == TextureButton::STRETCH_KEEP_ASPECT_COVERED);

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] Flip flags") {
	TextureButton *texture_button = memnew(TextureButton);

	SUBCASE("flip_h can be toggled") {
		texture_button->set_flip_h(true);
		CHECK(texture_button->is_flipped_h());
		texture_button->set_flip_h(false);
		CHECK_FALSE(texture_button->is_flipped_h());
	}

	SUBCASE("flip_v can be toggled") {
		texture_button->set_flip_v(true);
		CHECK(texture_button->is_flipped_v());
		texture_button->set_flip_v(false);
		CHECK_FALSE(texture_button->is_flipped_v());
	}

	SUBCASE("flip_h and flip_v are independent") {
		texture_button->set_flip_h(true);
		texture_button->set_flip_v(false);
		CHECK(texture_button->is_flipped_h());
		CHECK_FALSE(texture_button->is_flipped_v());
	}

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] has_point without click mask") {
	TextureButton *texture_button = memnew(TextureButton);
	Control *control = texture_button;
	texture_button->set_size(Size2(20, 15));

	CHECK(control->has_point(Point2(10, 7)));
	CHECK_FALSE(control->has_point(Point2(25, 7)));

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] has_point with click mask (keep stretch)") {
	TextureButton *texture_button = memnew(TextureButton);

	Ref<BitMap> mask;
	mask.instantiate();
	mask->create(Size2i(4, 4));
	mask->set_bit_rect(Rect2i(0, 0, 4, 4), false);
	mask->set_bit(2, 2, true);

	texture_button->set_click_mask(mask);
	texture_button->set_size(Size2(4, 4));

	// `has_point` uses draw-time bookkeeping; refresh it without requiring a full render pass.
	texture_button->notification(Control::NOTIFICATION_DRAW);

	Control *control = texture_button;
	CHECK_FALSE(control->has_point(Point2(0.5f, 0.5f)));
	CHECK(control->has_point(Point2(2.5f, 2.5f)));

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] has_point with click mask and stretch scale") {
	TextureButton *texture_button = memnew(TextureButton);

	Ref<ImageTexture> tex = make_test_texture(10, 10);
	texture_button->set_texture_normal(tex);

	Ref<BitMap> mask;
	mask.instantiate();
	mask->create(Size2i(10, 10));
	mask->set_bit_rect(Rect2i(0, 0, 10, 10), true);

	texture_button->set_click_mask(mask);
	texture_button->set_stretch_mode(TextureButton::STRETCH_SCALE);
	texture_button->set_size(Size2(40, 40));
	texture_button->notification(Control::NOTIFICATION_DRAW);

	Control *control = texture_button;
	CHECK(control->has_point(Point2(20.0f, 20.0f)));

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] has_point with click mask and stretch tile") {
	TextureButton *texture_button = memnew(TextureButton);

	Ref<ImageTexture> tex = make_test_texture(4, 4);
	texture_button->set_texture_normal(tex);

	Ref<BitMap> mask;
	mask.instantiate();
	mask->create(Size2i(4, 4));
	mask->set_bit_rect(Rect2i(0, 0, 4, 4), false);
	mask->set_bit(2, 2, true);

	texture_button->set_click_mask(mask);
	texture_button->set_stretch_mode(TextureButton::STRETCH_TILE);
	texture_button->set_size(Size2(12, 12));
	texture_button->notification(Control::NOTIFICATION_DRAW);

	Control *control = texture_button;
	CHECK_FALSE(control->has_point(Point2(1.0f, 1.0f)));
	CHECK(control->has_point(Point2(2.5f, 2.5f)));
	// Second horizontal tile: local x becomes 2 after subtracting one mask width.
	CHECK(control->has_point(Point2(6.5f, 2.5f)));

	memdelete(texture_button);
}

TEST_CASE("[SceneTree][TextureButton] has_point with click mask and stretch keep aspect covered") {
	TextureButton *texture_button = memnew(TextureButton);

	Ref<ImageTexture> tex = make_test_texture(10, 10);
	texture_button->set_texture_normal(tex);

	Ref<BitMap> mask;
	mask.instantiate();
	mask->create(Size2i(10, 10));
	mask->set_bit_rect(Rect2i(0, 0, 10, 10), true);

	texture_button->set_click_mask(mask);
	texture_button->set_stretch_mode(TextureButton::STRETCH_KEEP_ASPECT_COVERED);
	texture_button->set_size(Size2(20, 20));
	texture_button->notification(Control::NOTIFICATION_DRAW);

	Control *control = texture_button;
	CHECK(control->has_point(Point2(10.0f, 10.0f)));

	memdelete(texture_button);
}

} // namespace TestTextureButton

// === Merged from tests/scene/test_texture_progress_bar.cpp ===
/*  test_texture_progress_bar.cpp                                         */
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

namespace TestTextureProgressBar {

TEST_CASE("[SceneTree][TextureProgressBar]") {
	TextureProgressBar *texture_progress_bar = memnew(TextureProgressBar);

	SUBCASE("[TextureProgressBar] set_radial_initial_angle() should wrap angle between 0 and 360 degrees (inclusive).") {
		texture_progress_bar->set_radial_initial_angle(0.0);
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)0.0));

		texture_progress_bar->set_radial_initial_angle(360.0);
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)360.0));

		texture_progress_bar->set_radial_initial_angle(30.5);
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)30.5));

		texture_progress_bar->set_radial_initial_angle(-30.5);
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)(360 - 30.5)));

		texture_progress_bar->set_radial_initial_angle(36000 + 30.5);
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)30.5));

		texture_progress_bar->set_radial_initial_angle(-(36000 + 30.5));
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)(360 - 30.5)));
	}

	SUBCASE("[TextureProgressBar] set_radial_initial_angle() should not set non-finite values.") {
		texture_progress_bar->set_radial_initial_angle(30.5);

		ERR_PRINT_OFF;
		texture_progress_bar->set_radial_initial_angle(Math::INF);
		ERR_PRINT_ON;
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)30.5));

		ERR_PRINT_OFF;
		texture_progress_bar->set_radial_initial_angle(-Math::INF);
		ERR_PRINT_ON;
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)30.5));

		ERR_PRINT_OFF;
		texture_progress_bar->set_radial_initial_angle(Math::NaN);
		ERR_PRINT_ON;
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)30.5));

		ERR_PRINT_OFF;
		texture_progress_bar->set_radial_initial_angle(-Math::NaN);
		ERR_PRINT_ON;
		CHECK(Math::is_equal_approx(texture_progress_bar->get_radial_initial_angle(), (float)30.5));
	}

	memdelete(texture_progress_bar);
}

} // namespace TestTextureProgressBar

// === Merged from tests/scene/test_texture_rect.cpp ===
/*  test_texture_rect.cpp                                                 */
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
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

namespace TestTextureRect {

TEST_CASE("[SceneTree][TextureRect] Default properties") {
	TextureRect *texture_rect = memnew(TextureRect);

	CHECK(texture_rect->get_texture().is_null());
	CHECK(texture_rect->get_expand_mode() == TextureRect::EXPAND_KEEP_SIZE);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_SCALE);
	CHECK_FALSE(texture_rect->is_flipped_h());
	CHECK_FALSE(texture_rect->is_flipped_v());

	// Set in the TextureRect constructor, unlike the Control default.
	CHECK(texture_rect->get_mouse_filter() == Control::MOUSE_FILTER_PASS);

	CHECK(texture_rect->get_combined_minimum_size() == Size2(0, 0));

	memdelete(texture_rect);
}

TEST_CASE("[SceneTree][TextureRect] Set/get texture") {
	TextureRect *texture_rect = memnew(TextureRect);

	Ref<Image> image = memnew(Image(8, 8, false, Image::FORMAT_RGB8));
	Ref<ImageTexture> tex = ImageTexture::create_from_image(image);

	texture_rect->set_texture(tex);
	CHECK(texture_rect->get_texture() == tex);

	texture_rect->set_texture(Ref<Texture2D>());
	CHECK(texture_rect->get_texture().is_null());

	memdelete(texture_rect);
}

TEST_CASE("[SceneTree][TextureRect] Expand mode") {
	TextureRect *texture_rect = memnew(TextureRect);

	texture_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	CHECK(texture_rect->get_expand_mode() == TextureRect::EXPAND_IGNORE_SIZE);

	texture_rect->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH);
	CHECK(texture_rect->get_expand_mode() == TextureRect::EXPAND_FIT_WIDTH);

	texture_rect->set_expand_mode(TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);
	CHECK(texture_rect->get_expand_mode() == TextureRect::EXPAND_FIT_WIDTH_PROPORTIONAL);

	texture_rect->set_expand_mode(TextureRect::EXPAND_FIT_HEIGHT);
	CHECK(texture_rect->get_expand_mode() == TextureRect::EXPAND_FIT_HEIGHT);

	texture_rect->set_expand_mode(TextureRect::EXPAND_FIT_HEIGHT_PROPORTIONAL);
	CHECK(texture_rect->get_expand_mode() == TextureRect::EXPAND_FIT_HEIGHT_PROPORTIONAL);

	texture_rect->set_expand_mode(TextureRect::EXPAND_KEEP_SIZE);
	CHECK(texture_rect->get_expand_mode() == TextureRect::EXPAND_KEEP_SIZE);

	memdelete(texture_rect);
}

TEST_CASE("[SceneTree][TextureRect] Stretch mode") {
	TextureRect *texture_rect = memnew(TextureRect);

	texture_rect->set_stretch_mode(TextureRect::STRETCH_TILE);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_TILE);

	texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_KEEP);

	texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_KEEP_CENTERED);

	texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_KEEP_ASPECT);

	texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_CENTERED);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_KEEP_ASPECT_CENTERED);

	texture_rect->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_COVERED);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_KEEP_ASPECT_COVERED);

	texture_rect->set_stretch_mode(TextureRect::STRETCH_SCALE);
	CHECK(texture_rect->get_stretch_mode() == TextureRect::STRETCH_SCALE);

	memdelete(texture_rect);
}

TEST_CASE("[SceneTree][TextureRect] Flip flags") {
	TextureRect *texture_rect = memnew(TextureRect);

	SUBCASE("flip_h can be toggled") {
		texture_rect->set_flip_h(true);
		CHECK(texture_rect->is_flipped_h());

		texture_rect->set_flip_h(false);
		CHECK_FALSE(texture_rect->is_flipped_h());
	}

	SUBCASE("flip_v can be toggled") {
		texture_rect->set_flip_v(true);
		CHECK(texture_rect->is_flipped_v());

		texture_rect->set_flip_v(false);
		CHECK_FALSE(texture_rect->is_flipped_v());
	}

	SUBCASE("flip_h and flip_v are independent") {
		texture_rect->set_flip_h(true);
		texture_rect->set_flip_v(false);

		CHECK(texture_rect->is_flipped_h());
		CHECK_FALSE(texture_rect->is_flipped_v());
	}

	memdelete(texture_rect);
}

TEST_CASE("[SceneTree][TextureRect] Minimum size") {
	TextureRect *texture_rect = memnew(TextureRect);
	Window *root = SceneTree::get_root();
	root->add_child(texture_rect);

	Ref<Image> image = memnew(Image(64, 32, false, Image::FORMAT_RGB8));
	Ref<ImageTexture> tex = ImageTexture::create_from_image(image);

	SUBCASE("EXPAND_KEEP_SIZE reports the texture's pixel dimensions") {
		texture_rect->set_texture(tex);
		SceneTree::process(0);

		CHECK(texture_rect->get_combined_minimum_size() == Size2(64, 32));
	}

	SUBCASE("EXPAND_IGNORE_SIZE reports zero even with a texture") {
		texture_rect->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		texture_rect->set_texture(tex);
		SceneTree::process(0);

		CHECK(texture_rect->get_combined_minimum_size() == Size2(0, 0));
	}

	SUBCASE("Clearing the texture resets minimum size to zero") {
		texture_rect->set_texture(tex);
		SceneTree::process(0);

		texture_rect->set_texture(Ref<Texture2D>());
		SceneTree::process(0);

		CHECK(texture_rect->get_combined_minimum_size() == Size2(0, 0));
	}

	memdelete(texture_rect);
}

TEST_CASE("[SceneTree][TextureRect] Configuration warnings") {
	TextureRect *texture_rect = memnew(TextureRect);

	SUBCASE("No warnings by default") {
		CHECK(texture_rect->get_configuration_warnings().is_empty());
	}

	SUBCASE("STRETCH_TILE with a plain texture produces no warning") {
		Ref<Image> image = memnew(Image(4, 4, false, Image::FORMAT_RGB8));
		texture_rect->set_texture(ImageTexture::create_from_image(image));
		texture_rect->set_stretch_mode(TextureRect::STRETCH_TILE);

		CHECK(texture_rect->get_configuration_warnings().is_empty());
	}

	SUBCASE("STRETCH_TILE with an AtlasTexture with non-zero margin produces a warning") {
		Ref<AtlasTexture> at;
		at.instantiate();
		at->set_margin(Rect2(1, 0, 0, 0));

		texture_rect->set_texture(at);
		texture_rect->set_stretch_mode(TextureRect::STRETCH_TILE);

		CHECK(texture_rect->get_configuration_warnings().size() == 1);
	}

	memdelete(texture_rect);
}

} // namespace TestTextureRect

// === Merged from tests/scene/test_tree.cpp ===
/*  test_tree.cpp                                                         */
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



#ifndef ADVANCED_GUI_DISABLED

#include "scene/main/scene_tree.h"

namespace TestTree {

TEST_CASE("[SceneTree][Tree]") {
	SUBCASE("[Tree] Create and remove items.") {
		Tree *tree = memnew(Tree);
		TreeItem *root = tree->create_item();

		TreeItem *child1 = tree->create_item();
		CHECK_EQ(root->get_child_count(), 1);

		TreeItem *child2 = tree->create_item(root);
		CHECK_EQ(root->get_child_count(), 2);

		TreeItem *child3 = tree->create_item(root, 0);
		CHECK_EQ(root->get_child_count(), 3);

		CHECK_EQ(root->get_child(0), child3);
		CHECK_EQ(root->get_child(1), child1);
		CHECK_EQ(root->get_child(2), child2);

		root->remove_child(child3);
		CHECK_EQ(root->get_child_count(), 2);

		root->add_child(child3);
		CHECK_EQ(root->get_child_count(), 3);

		TreeItem *child4 = root->create_child();
		CHECK_EQ(root->get_child_count(), 4);

		CHECK_EQ(root->get_child(0), child1);
		CHECK_EQ(root->get_child(1), child2);
		CHECK_EQ(root->get_child(2), child3);
		CHECK_EQ(root->get_child(3), child4);

		memdelete(tree);
	}

	SUBCASE("[Tree] Clear items.") {
		Tree *tree = memnew(Tree);
		TreeItem *root = tree->create_item();

		for (int i = 0; i < 10; i++) {
			tree->create_item();
		}
		CHECK_EQ(root->get_child_count(), 10);

		root->clear_children();
		CHECK_EQ(root->get_child_count(), 0);

		memdelete(tree);
	}

	SUBCASE("[Tree] Get last item.") {
		Tree *tree = memnew(Tree);
		TreeItem *root = tree->create_item();

		TreeItem *last;
		for (int i = 0; i < 10; i++) {
			last = tree->create_item();
		}
		CHECK_EQ(root->get_child_count(), 10);
		CHECK_EQ(tree->get_last_item(), last);

		// Check nested.
		TreeItem *old_last = last;
		for (int i = 0; i < 10; i++) {
			last = tree->create_item(old_last);
		}
		CHECK_EQ(tree->get_last_item(), last);

		memdelete(tree);
	}

	// https://github.com/godotengine/godot/issues/96205
	SUBCASE("[Tree] Get last item after removal.") {
		Tree *tree = memnew(Tree);
		TreeItem *root = tree->create_item();

		TreeItem *child1 = tree->create_item(root);
		TreeItem *child2 = tree->create_item(root);

		CHECK_EQ(root->get_child_count(), 2);
		CHECK_EQ(tree->get_last_item(), child2);

		root->remove_child(child2);

		CHECK_EQ(root->get_child_count(), 1);
		CHECK_EQ(tree->get_last_item(), child1);

		root->add_child(child2);

		CHECK_EQ(root->get_child_count(), 2);
		CHECK_EQ(tree->get_last_item(), child2);

		memdelete(tree);
	}

	SUBCASE("[Tree] Previous and Next items.") {
		Tree *tree = memnew(Tree);
		TreeItem *root = tree->create_item();

		TreeItem *child1 = tree->create_item();
		TreeItem *child2 = tree->create_item();
		TreeItem *child3 = tree->create_item();
		CHECK_EQ(root->get_next(), nullptr);
		CHECK_EQ(root->get_next_visible(), child1);
		CHECK_EQ(root->get_next_in_tree(), child1);
		CHECK_EQ(child1->get_next(), child2);
		CHECK_EQ(child1->get_next_visible(), child2);
		CHECK_EQ(child1->get_next_in_tree(), child2);
		CHECK_EQ(child2->get_next(), child3);
		CHECK_EQ(child2->get_next_visible(), child3);
		CHECK_EQ(child2->get_next_in_tree(), child3);
		CHECK_EQ(child3->get_next(), nullptr);
		CHECK_EQ(child3->get_next_visible(), nullptr);
		CHECK_EQ(child3->get_next_in_tree(), nullptr);

		CHECK_EQ(root->get_prev(), nullptr);
		CHECK_EQ(root->get_prev_visible(), nullptr);
		CHECK_EQ(root->get_prev_in_tree(), nullptr);
		CHECK_EQ(child1->get_prev(), nullptr);
		CHECK_EQ(child1->get_prev_visible(), root);
		CHECK_EQ(child1->get_prev_in_tree(), root);
		CHECK_EQ(child2->get_prev(), child1);
		CHECK_EQ(child2->get_prev_visible(), child1);
		CHECK_EQ(child2->get_prev_in_tree(), child1);
		CHECK_EQ(child3->get_prev(), child2);
		CHECK_EQ(child3->get_prev_visible(), child2);
		CHECK_EQ(child3->get_prev_in_tree(), child2);

		TreeItem *nested1 = tree->create_item(child2);
		TreeItem *nested2 = tree->create_item(child2);
		TreeItem *nested3 = tree->create_item(child2);

		CHECK_EQ(child1->get_next(), child2);
		CHECK_EQ(child1->get_next_visible(), child2);
		CHECK_EQ(child1->get_next_in_tree(), child2);
		CHECK_EQ(child2->get_next(), child3);
		CHECK_EQ(child2->get_next_visible(), nested1);
		CHECK_EQ(child2->get_next_in_tree(), nested1);
		CHECK_EQ(child3->get_prev(), child2);
		CHECK_EQ(child3->get_prev_visible(), nested3);
		CHECK_EQ(child3->get_prev_in_tree(), nested3);
		CHECK_EQ(nested1->get_prev_in_tree(), child2);
		CHECK_EQ(nested1->get_next_in_tree(), nested2);
		CHECK_EQ(nested3->get_next_in_tree(), child3);

		memdelete(tree);
	}

	SUBCASE("[Tree] Previous and Next items with hide root.") {
		Tree *tree = memnew(Tree);
		tree->set_hide_root(true);
		TreeItem *root = tree->create_item();

		TreeItem *child1 = tree->create_item();
		TreeItem *child2 = tree->create_item();
		TreeItem *child3 = tree->create_item();
		CHECK_EQ(root->get_next(), nullptr);
		CHECK_EQ(root->get_next_visible(), child1);
		CHECK_EQ(root->get_next_in_tree(), child1);
		CHECK_EQ(child1->get_next(), child2);
		CHECK_EQ(child1->get_next_visible(), child2);
		CHECK_EQ(child1->get_next_in_tree(), child2);
		CHECK_EQ(child2->get_next(), child3);
		CHECK_EQ(child2->get_next_visible(), child3);
		CHECK_EQ(child2->get_next_in_tree(), child3);
		CHECK_EQ(child3->get_next(), nullptr);
		CHECK_EQ(child3->get_next_visible(), nullptr);
		CHECK_EQ(child3->get_next_in_tree(), nullptr);

		CHECK_EQ(root->get_prev(), nullptr);
		CHECK_EQ(root->get_prev_visible(), nullptr);
		CHECK_EQ(root->get_prev_in_tree(), nullptr);
		CHECK_EQ(child1->get_prev(), nullptr);
		CHECK_EQ(child1->get_prev_visible(), nullptr);
		CHECK_EQ(child1->get_prev_in_tree(), nullptr);
		CHECK_EQ(child2->get_prev(), child1);
		CHECK_EQ(child2->get_prev_visible(), child1);
		CHECK_EQ(child2->get_prev_in_tree(), child1);
		CHECK_EQ(child3->get_prev(), child2);
		CHECK_EQ(child3->get_prev_visible(), child2);
		CHECK_EQ(child3->get_prev_in_tree(), child2);

		memdelete(tree);
	}

	SUBCASE("[Tree] Previous and Next items wrapping.") {
		Tree *tree = memnew(Tree);
		TreeItem *root = tree->create_item();

		TreeItem *child1 = tree->create_item();
		TreeItem *child2 = tree->create_item();
		TreeItem *child3 = tree->create_item();
		CHECK_EQ(root->get_next_visible(true), child1);
		CHECK_EQ(root->get_next_in_tree(true), child1);
		CHECK_EQ(child1->get_next_visible(true), child2);
		CHECK_EQ(child1->get_next_in_tree(true), child2);
		CHECK_EQ(child2->get_next_visible(true), child3);
		CHECK_EQ(child2->get_next_in_tree(true), child3);
		CHECK_EQ(child3->get_next_visible(true), root);
		CHECK_EQ(child3->get_next_in_tree(true), root);

		CHECK_EQ(root->get_prev_visible(true), child3);
		CHECK_EQ(root->get_prev_in_tree(true), child3);
		CHECK_EQ(child1->get_prev_visible(true), root);
		CHECK_EQ(child1->get_prev_in_tree(true), root);
		CHECK_EQ(child2->get_prev_visible(true), child1);
		CHECK_EQ(child2->get_prev_in_tree(true), child1);
		CHECK_EQ(child3->get_prev_visible(true), child2);
		CHECK_EQ(child3->get_prev_in_tree(true), child2);

		TreeItem *nested1 = tree->create_item(child2);
		TreeItem *nested2 = tree->create_item(child2);
		TreeItem *nested3 = tree->create_item(child2);

		CHECK_EQ(child1->get_next_visible(true), child2);
		CHECK_EQ(child1->get_next_in_tree(true), child2);
		CHECK_EQ(child2->get_next_visible(true), nested1);
		CHECK_EQ(child2->get_next_in_tree(true), nested1);
		CHECK_EQ(nested3->get_next_visible(true), child3);
		CHECK_EQ(nested3->get_next_in_tree(true), child3);
		CHECK_EQ(child3->get_prev_visible(true), nested3);
		CHECK_EQ(child3->get_prev_in_tree(true), nested3);
		CHECK_EQ(nested1->get_prev_in_tree(true), child2);
		CHECK_EQ(nested1->get_next_in_tree(true), nested2);
		CHECK_EQ(nested3->get_next_in_tree(true), child3);

		memdelete(tree);
	}

	SUBCASE("[Tree] Previous and Next items wrapping with hide root.") {
		Tree *tree = memnew(Tree);
		tree->set_hide_root(true);
		TreeItem *root = tree->create_item();

		TreeItem *child1 = tree->create_item();
		TreeItem *child2 = tree->create_item();
		TreeItem *child3 = tree->create_item();
		CHECK_EQ(root->get_next_visible(true), child1);
		CHECK_EQ(root->get_next_in_tree(true), child1);
		CHECK_EQ(child1->get_next_visible(true), child2);
		CHECK_EQ(child1->get_next_in_tree(true), child2);
		CHECK_EQ(child2->get_next_visible(true), child3);
		CHECK_EQ(child2->get_next_in_tree(true), child3);
		CHECK_EQ(child3->get_next_visible(true), root);
		CHECK_EQ(child3->get_next_in_tree(true), root);

		CHECK_EQ(root->get_prev_visible(true), child3);
		CHECK_EQ(root->get_prev_in_tree(true), child3);
		CHECK_EQ(child1->get_prev_visible(true), child3);
		CHECK_EQ(child1->get_prev_in_tree(true), child3);
		CHECK_EQ(child2->get_prev_visible(true), child1);
		CHECK_EQ(child2->get_prev_in_tree(true), child1);
		CHECK_EQ(child3->get_prev_visible(true), child2);
		CHECK_EQ(child3->get_prev_in_tree(true), child2);

		memdelete(tree);
	}
}

} // namespace TestTree

#endif // ADVANCED_GUI_DISABLED

TEST_FORCE_LINK(test_scene_gui)

