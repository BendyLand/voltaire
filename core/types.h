#ifndef TYPES_H
#define TYPES_H

// Core definitions & memory management
#include "core/os/memory.h"
#include "core/typedefs.h"
#include "core/os/mutex.h"
#include "core/templates/ref.h"

// Math primitives
#include "core/math/basis.h"
#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "core/math/math_funcs.h"
#include "core/math/quaternion.h"
#include "core/math/rect2.h"
#include "core/math/rect2i.h"
#include "core/math/transform_2d.h"
#include "core/math/transform_3d.h"
#include "core/math/vector2.h"
#include "core/math/vector2i.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/math/vector4.h"
#include "core/math/vector4i.h"

// Strings & Paths
#include "core/string/node_path.h"
#include "core/string/string_name.h"
#include "core/string/ustring.h"

// Core Identifiers & Handles
#include "core/templates/rid.h"

// Native Container Templates
#include "core/templates/cowdata.h"
#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/ring_buffer.h"
#include "core/templates/vector.h"

#endif // TYPES_H
