/**************************************************************************/
/*  packed_data_container.cpp                                             */
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

#include "core/io/marshalls.h"
#include "packed_data_container.h"

int PackedDataContainer::size() const { return _size(0); }

uint32_t PackedDataContainer::_type_at_ofs(uint32_t p_ofs) const
{
	ERR_FAIL_COND_V(p_ofs + 4 > (uint32_t)data.size(), 0);
	const uint8_t* rd = data.ptr();
	ERR_FAIL_NULL_V(rd, 0);
	const uint8_t* r = &rd[p_ofs];
	uint32_t type = decode_uint32(r);

	return type;
}

int PackedDataContainer::_size(uint32_t p_ofs) const
{
	ERR_FAIL_COND_V(p_ofs + 4 > (uint32_t)data.size(), 0);
	const uint8_t* rd = data.ptr();
	ERR_FAIL_NULL_V(rd, 0);
	const uint8_t* r = &rd[p_ofs];
	uint32_t type = decode_uint32(r);

	if (type == TYPE_ARRAY) {
		uint32_t len = decode_uint32(r + 4);
		return len;

	}
	else if (type == TYPE_DICT) {
		uint32_t len = decode_uint32(r + 4);
		return len;
	}

	return -1;
}

void PackedDataContainer::_set_data(const Vector<uint8_t>& p_data)
{
	data = p_data;
	datalen = data.size();
}

Vector<uint8_t> PackedDataContainer::_get_data() const { return data; }

void PackedDataContainer::_bind_methods() {}

//////////////////

void PackedDataContainerRef::_bind_methods() {}

int PackedDataContainerRef::size() const { return from->_size(offset); }


