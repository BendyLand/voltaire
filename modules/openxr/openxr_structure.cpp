/**************************************************************************/
/*  openxr_structure.cpp                                                  */
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

#include "openxr_structure.h"
#include "extensions/spatial_entities/openxr_spatial_anchor.h"

void OpenXRStructureBase::set_next(const Ref<OpenXRStructureBase> p_next) { next = p_next; }

Ref<OpenXRStructureBase> OpenXRStructureBase::get_next() const { return next; }

bool OpenXRStructureBase::has_valid_configuration() const
{
	if (persistence_contexts.is_empty()) {
		return false;
	}

	ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), false);

	if (!OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()) {
		return false;
	}

	return true;
}

void* OpenXRStructureBase::get_header(void* p_next)
{
	void* n = p_next;
	if (get_next().is_valid()) {
		n = get_next()->get_header(n);
	}

	if (has_valid_configuration()) {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), nullptr);

		context_handles.resize(persistence_contexts.size());

		XrSpatialPersistenceContextEXT* ptr = context_handles.ptrw();
		int i = 0;
		for (const RID& rid : persistence_contexts) {
			ptr[i++] = OpenXRSpatialAnchorCapability::get_persistence_context_handle(rid);
		}

		persistence_config.next = n;
		persistence_config.persistenceContextCount = (uint32_t)context_handles.size();
		persistence_config.persistenceContexts = context_handles.ptr();

		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&persistence_config;
	}

	return n;
}

XrStructureType OpenXRStructureBase::get_structure_type()
{
	if (!persistence_contexts.is_empty()) {
		return XR_TYPE_SPATIAL_CONTEXT_PERSISTENCE_CONFIG_EXT;
	}

	XrStructureType* header = (XrStructureType*)get_header();
	if (header == nullptr) {
		return XR_TYPE_UNKNOWN;
	}
	return *header;
}

void OpenXRStructureBase::add_persistence_context(RID p_persistence_context)
{
	ERR_FAIL_COND(persistence_contexts.has(p_persistence_context));

	persistence_contexts.push_back(p_persistence_context);
}

void OpenXRStructureBase::remove_persistence_context(RID p_persistence_context)
{
	ERR_FAIL_COND(!persistence_contexts.has(p_persistence_context));

	persistence_contexts.erase(p_persistence_context);
}

// Return structure type as uint64_t to GDScript
uint64_t OpenXRStructureBase::_get_structure_type() { return (uint64_t)get_structure_type(); }


