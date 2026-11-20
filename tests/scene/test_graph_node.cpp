/**************************************************************************/
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

TEST_FORCE_LINK(test_graph_node)

#ifndef ADVANCED_GUI_DISABLED

#include "scene/gui/graph_node.h"

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
