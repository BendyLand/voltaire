/**************************************************************************/
/*  shader_types.h                                                        */
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

#pragma once

#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/shader_language.h"

class ShaderTypes
{
public:
    ShaderTypes() = delete;
    ~ShaderTypes() = delete;

    struct Type
    {
        HashMap<StringName, ShaderLanguage::FunctionInfo> functions;
        Vector<ShaderLanguage::ModeInfo> modes;
        Vector<ShaderLanguage::ModeInfo> stencil_modes;
    };

    static inline HashMap<RSE::ShaderMode, Type> shader_modes;
    static inline HashSet<String> shader_types;
    static inline List<String> shader_types_list;

    static const HashMap<StringName, ShaderLanguage::FunctionInfo>& get_functions(
        RSE::ShaderMode p_mode);
    static const Vector<ShaderLanguage::ModeInfo>& get_modes(RSE::ShaderMode p_mode);
    static const Vector<ShaderLanguage::ModeInfo>& get_stencil_modes(RSE::ShaderMode p_mode);
    static const HashSet<String>& get_types();
    static const List<String>& get_types_list();

    static void initialize();
    static void finalize();
};

