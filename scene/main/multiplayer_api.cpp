/**************************************************************************/
/*  multiplayer_api.cpp                                                   */
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
#include "multiplayer_api.h"

StringName MultiplayerAPI::default_interface;

void MultiplayerAPI::set_default_interface(const StringName& p_interface)
{
	default_interface = StringName(p_interface, true);
}

StringName MultiplayerAPI::get_default_interface() { return default_interface; }

Ref<MultiplayerAPI> MultiplayerAPI::create_default_interface()
{
	return Ref<MultiplayerAPI>(memnew(MultiplayerAPI));
}

// The variant is compressed and encoded; The first byte contains all the meta
// information and the format is:
// - The first LSB 6 bits are used for the variant type.
// - The next two bits are used to store the encoding mode.
// - Boolean values uses the encoding mode to store the value.
#define VARIANT_META_TYPE_MASK 0x3F
#define VARIANT_META_EMODE_MASK 0xC0
#define VARIANT_META_BOOL_MASK 0x80
#define ENCODE_8 0 << 6
#define ENCODE_16 1 << 6
#define ENCODE_32 2 << 6
#define ENCODE_64 3 << 6

int MultiplayerAPI::get_unique_id() { return 0; }

Vector<int> MultiplayerAPI::get_peer_ids() { return Vector<int>(); }

Error MultiplayerAPI::poll() { return OK; }

void MultiplayerAPI::set_multiplayer_peer(const Ref<MultiplayerPeer>& p_peer) {}

Ref<MultiplayerPeer> MultiplayerAPI::get_multiplayer_peer() { return Ref<MultiplayerPeer>(); }


