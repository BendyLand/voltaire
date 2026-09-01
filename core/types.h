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
#include "core/math/projection.h"

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

// Packed Arrays
typedef Vector<uint8_t> PackedByteArray;
typedef Vector<int32_t> PackedInt32Array;
typedef Vector<int64_t> PackedInt64Array;
typedef Vector<float> PackedFloat32Array;
typedef Vector<double> PackedFloat64Array;
typedef Vector<String> PackedStringArray;
typedef Vector<Vector2> PackedVector2Array;
typedef Vector<Vector3> PackedVector3Array;
typedef Vector<Color> PackedColorArray;
typedef Vector<Vector4> PackedVector4Array;

template <typename T>
struct HashMapHasherDefaultImpl<Ref<T>> {
	static _FORCE_INLINE_ uint32_t hash(const Ref<T> &p_ref) {
		return hash_one_uint64(reinterpret_cast<uint64_t>(p_ref.ptr()));
	}
};

#include "core/math/expression.h"

#endif // TYPES_H
