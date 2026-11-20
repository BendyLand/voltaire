/**************************************************************************/
/*  openxr_structure.h                                                    */
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

#include <openxr/openxr.h>
#include "core/types.h"

// Base class for XrStructureType based headers (with persistence config folded in)
class OpenXRStructureBase : public RefCounted
{
public:
	bool has_valid_configuration() const;
	void* get_header(void* p_next = nullptr);
	XrStructureType get_structure_type();

	void set_next(const Ref<OpenXRStructureBase> p_next);
	Ref<OpenXRStructureBase> get_next() const;

	void add_persistence_context(RID p_persistence_context);
	void remove_persistence_context(RID p_persistence_context);

private:
	Ref<OpenXRStructureBase> next;
	Vector<RID> persistence_contexts;
	Vector<XrSpatialPersistenceContextEXT> context_handles;
	XrSpatialContextPersistenceConfigEXT persistence_config = {
		XR_TYPE_SPATIAL_CONTEXT_PERSISTENCE_CONFIG_EXT, nullptr, 0, nullptr};

	uint64_t _get_structure_type();
};


