/**************************************************************************/
/*  java_class_wrapper.cpp                                                */
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

#include "api/java_class_wrapper.h"

#include "platform/android/os_android.h"
#include "platform/android/os_android.h"

bool JavaClass::_call_method(JavaObject *p_instance, const String &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error, Variant &ret) {
	HashMap<String, List<MethodInfo>>::Iterator M = methods.find(p_method);
	if (!M) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		return false;
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, false);

	env->PushLocalFrame(p_argcount);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return false;
	}

	MethodInfo *method = nullptr;
	for (MethodInfo &E : M->value) {
		if (!p_instance && !E._static && !E._constructor) {
			r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
			continue;
		}

		int pc = E.param_types.size();
		if (p_argcount < pc) {
			r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.expected = pc;
			continue;
		}
		if (p_argcount > pc) {
			r_error.error = Callable::CallError::CALL_ERROR_TOO_MANY_ARGUMENTS;
			r_error.expected = pc;
			continue;
		}
		uint32_t *ptypes = E.param_types.ptrw();
		bool valid = true;

		for (int i = 0; i < pc; i++) {
			Variant::Type arg_expected = Variant::NIL;
			switch (ptypes[i]) {
				case ARG_TYPE_VOID: {
					//bug?
				} break;
				case ARG_TYPE_BOOLEAN: {
					if (p_args[i]->get_type() != Variant::BOOL) {
						arg_expected = Variant::BOOL;
					}
				} break;
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_BYTE:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_CHAR:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_SHORT:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_INT:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_LONG:
				case ARG_TYPE_BYTE:
				case ARG_TYPE_CHAR:
				case ARG_TYPE_SHORT:
				case ARG_TYPE_INT:
				case ARG_TYPE_LONG: {
					if (!p_args[i]->is_num()) {
						arg_expected = Variant::INT;
					}
				} break;
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_FLOAT:
				case ARG_NUMBER_CLASS_BIT | ARG_TYPE_DOUBLE:
				case ARG_TYPE_FLOAT:
				case ARG_TYPE_DOUBLE: {
					if (!p_args[i]->is_num()) {
						arg_expected = Variant::FLOAT;
					}
				} break;
				case ARG_TYPE_STRING:
				case ARG_TYPE_CHARSEQUENCE: {
					if (!p_args[i]->is_string()) {
						arg_expected = Variant::STRING;
					}
				} break;
				case ARG_TYPE_CALLABLE: {
					if (p_args[i]->get_type() != Variant::CALLABLE) {
						arg_expected = Variant::CALLABLE;
					}
				} break;
				case ARG_TYPE_CLASS: {
					String cn = E.param_sigs[i].string();
					if (cn.begins_with("L") && cn.ends_with(";")) {
						cn = cn.substr(1, cn.length() - 2);
					}
					if (cn == "org/godotengine/godot/Dictionary") {
						if (p_args[i]->get_type() != Variant::DICTIONARY) {
							arg_expected = Variant::DICTIONARY;
						}
					} else if (p_args[i]->get_type() != Variant::OBJECT && p_args[i]->get_type() != Variant::NIL) {
						arg_expected = Variant::OBJECT;
					} else {
						Ref<RefCounted> ref = *p_args[i];
						if (ref.is_valid()) {
							if (Object::cast_to<JavaObject>(ref.ptr())) {
								Ref<JavaObject> jo = ref;
								jclass c = jni_find_class(env, cn.utf8().get_data());
								if (!c || !env->IsInstanceOf(jo->instance, c)) {
									arg_expected = Variant::OBJECT;
								} else {
									//ok
								}
							} else {
								arg_expected = Variant::OBJECT;
							}
						}
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::BOOL) {
							arg_expected = Variant::ARRAY;
						}
					} else {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_BYTE:
				case ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
					if (p_args[i]->get_type() != Variant::PACKED_BYTE_ARRAY) {
						arg_expected = Variant::PACKED_BYTE_ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_SHORT:
				case ARG_ARRAY_BIT | ARG_TYPE_INT: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::INT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_INT32_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_LONG: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::INT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_INT64_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::FLOAT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_FLOAT32_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::FLOAT) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_FLOAT64_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_STRING:
				case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && (arr.get_typed_builtin() != Variant::STRING && arr.get_typed_builtin() != Variant::STRING_NAME)) {
							arg_expected = Variant::ARRAY;
						}
					} else if (p_args[i]->get_type() != Variant::PACKED_STRING_ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::CALLABLE) {
							arg_expected = Variant::ARRAY;
						}
					} else {
						arg_expected = Variant::ARRAY;
					}
				} break;
				case ARG_ARRAY_BIT | ARG_TYPE_CLASS: {
					if (p_args[i]->get_type() == Variant::ARRAY) {
						Array arr = *p_args[i];
						if (arr.is_typed() && arr.get_typed_builtin() != Variant::OBJECT) {
							arg_expected = Variant::ARRAY;
						} else {
							String cn = E.param_sigs[i].string();
							if (cn.begins_with("[L") && cn.ends_with(";")) {
								cn = cn.substr(2, cn.length() - 3);
							}
							jclass c = jni_find_class(env, cn.utf8().get_data());
							if (c) {
								for (int j = 0; j < arr.size(); j++) {
									Ref<JavaObject> jo = arr[j];
									if (jo.is_valid()) {
										if (!env->IsInstanceOf(jo->instance, c)) {
											arg_expected = Variant::ARRAY;
											break;
										}
									} else {
										arg_expected = Variant::ARRAY;
										break;
									}
								}
							} else {
								arg_expected = Variant::ARRAY;
							}
						}
					} else {
						arg_expected = Variant::ARRAY;
					}
				} break;
				default: {
					if (p_args[i]->get_type() != Variant::ARRAY) {
						arg_expected = Variant::ARRAY;
					}
				} break;
			}

			if (arg_expected != Variant::NIL) {
				r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
				r_error.argument = i;
				r_error.expected = arg_expected;
				valid = false;
				break;
			}
		}
		if (!valid) {
			continue;
		}

		method = &E;
		break;
	}

	if (!method) {
		if (r_error.error == Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL) {
			ERR_PRINT(vformat(R"(Cannot call static function "%s" on Java class "%s" directly. Make an instance instead.)", p_method, java_class_name));
		}
		return true; //no version convinces
	}

	r_error.error = Callable::CallError::CALL_OK;

	jvalue *argv = nullptr;

	if (method->param_types.size()) {
		argv = (jvalue *)alloca(sizeof(jvalue) * method->param_types.size());
	}

	for (int i = 0; i < method->param_types.size(); i++) {
		switch (method->param_types[i]) {
			case ARG_TYPE_VOID: {
				//can't happen
				argv[i].l = nullptr; //I hope this works
			} break;

			case ARG_TYPE_BOOLEAN: {
				argv[i].z = *p_args[i];
			} break;
			case ARG_TYPE_BYTE: {
				argv[i].b = *p_args[i];
			} break;
			case ARG_TYPE_CHAR: {
				argv[i].c = *p_args[i];
			} break;
			case ARG_TYPE_SHORT: {
				argv[i].s = *p_args[i];
			} break;
			case ARG_TYPE_INT: {
				argv[i].i = *p_args[i];
			} break;
			case ARG_TYPE_LONG: {
				argv[i].j = (int64_t)*p_args[i];
			} break;
			case ARG_TYPE_FLOAT: {
				argv[i].f = *p_args[i];
			} break;
			case ARG_TYPE_DOUBLE: {
				argv[i].d = *p_args[i];
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_BOOLEAN: {
				jclass bclass = jni_find_class(env, "java/lang/Boolean");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(Z)V");
				jvalue val;
				val.z = (bool)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_BYTE: {
				jclass bclass = jni_find_class(env, "java/lang/Byte");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(B)V");
				jvalue val;
				val.b = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_CHAR: {
				jclass bclass = jni_find_class(env, "java/lang/Character");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(C)V");
				jvalue val;
				val.c = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_SHORT: {
				jclass bclass = jni_find_class(env, "java/lang/Short");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(S)V");
				jvalue val;
				val.s = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_INT: {
				jclass bclass = jni_find_class(env, "java/lang/Integer");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(I)V");
				jvalue val;
				val.i = (int)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_LONG: {
				jclass bclass = jni_find_class(env, "java/lang/Long");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(J)V");
				jvalue val;
				val.j = (int64_t)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_FLOAT: {
				jclass bclass = jni_find_class(env, "java/lang/Float");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(F)V");
				jvalue val;
				val.f = (float)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_NUMBER_CLASS_BIT | ARG_TYPE_DOUBLE: {
				jclass bclass = jni_find_class(env, "java/lang/Double");
				jmethodID ctor = env->GetMethodID(bclass, "<init>", "(D)V");
				jvalue val;
				val.d = (double)(*p_args[i]);
				jobject obj = env->NewObjectA(bclass, ctor, &val);
				argv[i].l = obj;
			} break;
			case ARG_TYPE_STRING:
			case ARG_TYPE_CHARSEQUENCE: {
				String s = *p_args[i];
				jstring jStr = env->NewStringUTF(s.utf8().get_data());
				argv[i].l = jStr;
			} break;
			case ARG_TYPE_CALLABLE: {
				jobject jcallable = callable_to_jcallable(env, *p_args[i]);
				argv[i].l = jcallable;
			} break;
			case ARG_TYPE_CLASS: {
				if (p_args[i]->get_type() == Variant::DICTIONARY) {
					argv[i].l = _variant_to_jobject(env, Variant::DICTIONARY, p_args[i]);
				} else {
					Ref<JavaObject> jo = *p_args[i];
					if (jo.is_valid()) {
						argv[i].l = jo->instance;
					} else {
						argv[i].l = nullptr; //I hope this works
					}
				}
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
				Array arr = *p_args[i];
				jbooleanArray a = env->NewBooleanArray(arr.size());
				for (int j = 0; j < arr.size(); j++) {
					jboolean val = arr[j];
					env->SetBooleanArrayRegion(a, j, 1, &val);
				}
				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_BYTE: {
				jbyteArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewByteArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jbyte val = arr[j];
						env->SetByteArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_BYTE_ARRAY) {
					PackedByteArray arr = *p_args[i];
					a = env->NewByteArray(arr.size());
					env->SetByteArrayRegion(a, 0, arr.size(), (const jbyte *)arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
				jcharArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewCharArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jchar val = arr[j];
						env->SetCharArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_BYTE_ARRAY) {
					PackedByteArray arr = *p_args[i];
					// The data is expected to be UTF-16 encoded, so the length is half the size of the byte array.
					int size = arr.size() / 2;
					a = env->NewCharArray(size);
					env->SetCharArrayRegion(a, 0, size, (const jchar *)arr.ptr());
				}

				argv[i].l = a;

			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_SHORT: {
				jshortArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewShortArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jshort val = arr[j];
						env->SetShortArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_INT32_ARRAY) {
					PackedInt32Array arr = *p_args[i];
					a = env->NewShortArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jshort val = arr[j];
						env->SetShortArrayRegion(a, j, 1, &val);
					}
				}

				argv[i].l = a;

			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_INT: {
				jintArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewIntArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jint val = arr[j];
						env->SetIntArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_INT32_ARRAY) {
					PackedInt32Array arr = *p_args[i];
					a = env->NewIntArray(arr.size());
					env->SetIntArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_LONG: {
				jlongArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewLongArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jlong val = (int64_t)arr[j];
						env->SetLongArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_INT64_ARRAY) {
					PackedInt64Array arr = *p_args[i];
					a = env->NewLongArray(arr.size());
					env->SetLongArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
				jfloatArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewFloatArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jfloat val = arr[j];
						env->SetFloatArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_FLOAT32_ARRAY) {
					PackedFloat32Array arr = *p_args[i];
					a = env->NewFloatArray(arr.size());
					env->SetFloatArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
				jdoubleArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewDoubleArray(arr.size());
					for (int j = 0; j < arr.size(); j++) {
						jdouble val = arr[j];
						env->SetDoubleArrayRegion(a, j, 1, &val);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_FLOAT64_ARRAY) {
					PackedFloat64Array arr = *p_args[i];
					a = env->NewDoubleArray(arr.size());
					env->SetDoubleArrayRegion(a, 0, arr.size(), arr.ptr());
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_STRING:
			case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE: {
				jobjectArray a = nullptr;

				if (p_args[i]->get_type() == Variant::ARRAY) {
					Array arr = *p_args[i];
					a = env->NewObjectArray(arr.size(), jni_find_class(env, "java/lang/String"), nullptr);
					for (int j = 0; j < arr.size(); j++) {
						String s = arr[j];
						jstring jStr = env->NewStringUTF(s.utf8().get_data());
						env->SetObjectArrayElement(a, j, jStr);
					}
				} else if (p_args[i]->get_type() == Variant::PACKED_STRING_ARRAY) {
					PackedStringArray arr = *p_args[i];
					a = env->NewObjectArray(arr.size(), jni_find_class(env, "java/lang/String"), nullptr);
					for (int j = 0; j < arr.size(); j++) {
						String s = arr[j];
						jstring jStr = env->NewStringUTF(s.utf8().get_data());
						env->SetObjectArrayElement(a, j, jStr);
					}
				}

				argv[i].l = a;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE: {
				Array arr = *p_args[i];
				jobjectArray jarr = env->NewObjectArray(arr.size(), jni_find_class(env, "org/godotengine/godot/variant/Callable"), nullptr);
				for (int j = 0; j < arr.size(); j++) {
					Variant callable = arr[j];
					jobject jcallable = callable_to_jcallable(env, callable);
					env->SetObjectArrayElement(jarr, j, jcallable);
				}

				argv[i].l = jarr;
			} break;
			case ARG_ARRAY_BIT | ARG_TYPE_CLASS: {
				String cn = method->param_sigs[i].string();
				if (cn.begins_with("[L") && cn.ends_with(";")) {
					cn = cn.substr(2, cn.length() - 3);
				}
				jclass c = jni_find_class(env, cn.utf8().get_data());
				if (c) {
					Array arr = *p_args[i];
					jobjectArray jarr = env->NewObjectArray(arr.size(), c, nullptr);
					for (int j = 0; j < arr.size(); j++) {
						Ref<JavaObject> jo = arr[j];
						env->SetObjectArrayElement(jarr, j, jo->instance);
					}

					argv[i].l = jarr;
				}
			} break;
		}
	}

	r_error.error = Callable::CallError::CALL_OK;
	bool success = true;

	switch (method->return_type) {
		case ARG_TYPE_VOID: {
			if (method->_static) {
				env->CallStaticVoidMethodA(_class, method->method, argv);
			} else {
				env->CallVoidMethodA(p_instance->instance, method->method, argv);
			}
			ret = Variant();

		} break;
		case ARG_TYPE_BOOLEAN: {
			if (method->_static) {
				ret = env->CallStaticBooleanMethodA(_class, method->method, argv);
			} else {
				ret = env->CallBooleanMethodA(p_instance->instance, method->method, argv);
			}
		} break;
		case ARG_TYPE_BYTE: {
			if (method->_static) {
				ret = env->CallStaticByteMethodA(_class, method->method, argv);
			} else {
				ret = env->CallByteMethodA(p_instance->instance, method->method, argv);
			}
		} break;
		case ARG_TYPE_CHAR: {
			if (method->_static) {
				ret = env->CallStaticCharMethodA(_class, method->method, argv);
			} else {
				ret = env->CallCharMethodA(p_instance->instance, method->method, argv);
			}
		} break;
		case ARG_TYPE_SHORT: {
			if (method->_static) {
				ret = env->CallStaticShortMethodA(_class, method->method, argv);
			} else {
				ret = env->CallShortMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_INT: {
			if (method->_static) {
				ret = env->CallStaticIntMethodA(_class, method->method, argv);
			} else {
				ret = env->CallIntMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_LONG: {
			if (method->_static) {
				ret = (int64_t)env->CallStaticLongMethodA(_class, method->method, argv);
			} else {
				ret = (int64_t)env->CallLongMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_FLOAT: {
			if (method->_static) {
				ret = env->CallStaticFloatMethodA(_class, method->method, argv);
			} else {
				ret = env->CallFloatMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		case ARG_TYPE_DOUBLE: {
			if (method->_static) {
				ret = env->CallStaticDoubleMethodA(_class, method->method, argv);
			} else {
				ret = env->CallDoubleMethodA(p_instance->instance, method->method, argv);
			}

		} break;
		default: {
			jobject obj;
			if (method->_constructor) {
				obj = env->NewObjectA(_class, method->method, argv);
			} else if (method->_static) {
				obj = env->CallStaticObjectMethodA(_class, method->method, argv);
			} else {
				obj = env->CallObjectMethodA(p_instance->instance, method->method, argv);
			}

			if (!obj) {
				ret = Variant();
			} else {
				if (!_convert_object_to_variant(env, obj, ret, method->return_type)) {
					ret = Variant();
					r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
					success = false;
				}
			}

		} break;
	}

	jobject exception = env->ExceptionOccurred();
	if (exception) {
		env->ExceptionClear();

		jclass java_class = env->GetObjectClass(exception);
		Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class);

		JavaClassWrapper::data->exception.instantiate(java_class_wrapped, exception);
	} else {
		JavaClassWrapper::data->exception.unref();
	}

	env->PopLocalFrame(nullptr);

	return success;
}

bool JavaClass::_get(const String &p_name, Variant &r_ret) const {
	if (constant_map.has(p_name)) {
		r_ret = constant_map[p_name];
		return true;
	}

	return false;
}

Variant JavaClass::callp(const String &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	// Godot methods take precedence.
	Variant ret = RefCounted::callp(p_method, p_args, p_argcount, r_error);
	if (r_error.error == Callable::CallError::CALL_OK) {
		return ret;
	}

	String method = (p_method == java_constructor_name) ? "<init>" : p_method;
	bool found = _call_method(nullptr, method, p_args, p_argcount, r_error, ret);
	if (found) {
		return ret;
	}

	return Variant();
}

String JavaClass::get_java_class_name() const {
	return java_class_name;
}

TypedArray<Dictionary> JavaClass::get_java_method_list() const {
	TypedArray<Dictionary> method_list;

	for (const KeyValue<String, List<MethodInfo>> &item : methods) {
		for (const MethodInfo &mi : item.value) {
			Dictionary method;

			method["name"] = mi._constructor ? java_constructor_name : String(item.key);
			method["id"] = (uint64_t)mi.method;
			method["default_args"] = Array();
			method["flags"] = METHOD_FLAGS_DEFAULT & (mi._static || mi._constructor ? METHOD_FLAG_STATIC : METHOD_FLAG_NORMAL);

			{
				Array a;

				for (uint32_t argtype : mi.param_types) {
					Dictionary d;

					Variant::Type t = Variant::NIL;
					float likelihood = 0.0;
					_convert_to_variant_type(argtype, t, likelihood);
					d["type"] = t;
					if (t == Variant::OBJECT) {
						d["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
						d["hint_string"] = "JavaObject";
					} else {
						d["hint"] = 0;
						d["hint_string"] = "";
					}

					a.push_back(d);
				}

				method["args"] = a;
			}

			{
				Dictionary d;

				if (mi._constructor) {
					d["type"] = Variant::OBJECT;
					d["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
					d["hint_string"] = "JavaObject";
				} else {
					Variant::Type t = Variant::NIL;
					float likelihood = 0.0;
					_convert_to_variant_type(mi.return_type, t, likelihood);
					d["type"] = t;
					if (t == Variant::OBJECT) {
						d["hint"] = PROPERTY_HINT_RESOURCE_TYPE;
						d["hint_string"] = "JavaObject";
					} else {
						d["hint"] = 0;
						d["hint_string"] = "";
					}
				}

				method["return_type"] = d;
			}

			method_list.push_back(method);
		}
	}

	return method_list;
}

Ref<JavaClass> JavaClass::get_java_parent_class() const {
	ERR_FAIL_NULL_V(_class, Ref<JavaClass>());

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaClass>());

	jclass superclass = (jclass)env->CallObjectMethod(_class, JavaClassWrapper::data->Class_getSuperclass);
	if (!superclass) {
		return Ref<JavaClass>();
	}

	Ref<JavaClass> ret = JavaClassWrapper::wrap_jclass(superclass);
	env->DeleteLocalRef(superclass);
	return ret;
}

String JavaClass::_to_string() {
	return "<JavaClass:" + java_class_name + ">";
}

bool JavaClass::has_java_method(const String &p_method) const {
	String method = (p_method == java_constructor_name) ? "<init>" : p_method;
	return methods.has(method);
}

JavaClass::JavaClass() {
}

JavaClass::~JavaClass() {
	if (_class) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->DeleteGlobalRef(_class);
	}
}

/////////////////////

Variant JavaObject::callp(const String &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	// Godot methods take precedence.
	Variant ret = RefCounted::callp(p_method, p_args, p_argcount, r_error);
	if (r_error.error == Callable::CallError::CALL_OK) {
		return ret;
	}

	if (instance) {
		Ref<JavaClass> c = base_class;
		while (c.is_valid()) {
			bool found = c->_call_method(this, p_method, p_args, p_argcount, r_error, ret);
			if (found) {
				return ret;
			}
			c = c->get_java_parent_class();
		}
	}

	return Variant();
}

Ref<JavaClass> JavaObject::get_java_class() const {
	return base_class;
}

String JavaObject::_to_string() {
	if (base_class.is_valid() && instance) {
		return "<JavaObject:" + base_class->java_class_name + " \"" + (String)call("toString") + "\">";
	}
	return RefCounted::_to_string();
}

bool JavaObject::has_java_method(const String &p_method) const {
	if (instance) {
		Ref<JavaClass> c = base_class;
		while (c.is_valid()) {
			if (c->has_java_method(p_method)) {
				return true;
			}
			c = c->get_java_parent_class();
		}
	}
	return false;
}

JavaObject::JavaObject() {
}

JavaObject::JavaObject(const Ref<JavaClass> &p_base, jobject p_instance) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	base_class = p_base;
	instance = env->NewGlobalRef(p_instance);
}

JavaObject::~JavaObject() {
	if (instance) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->DeleteGlobalRef(instance);
	}
}

////////////////////

bool JavaClassWrapper::_get_type_sig(JNIEnv *env, jobject obj, uint32_t &sig, String &strsig) {
	jstring name2 = (jstring)env->CallObjectMethod(obj, data->Class_getName);
	String str_type = jstring_to_string(name2, env);
	env->DeleteLocalRef(name2);
	uint32_t t = 0;

	if (str_type.begins_with("[")) {
		t = JavaClass::ARG_ARRAY_BIT;
		strsig = "[";
		str_type = str_type.substr(1);
		if (str_type.begins_with("[")) {
			print_line("Nested arrays not supported for type: " + str_type);
			return false;
		}
		if (str_type.begins_with("L")) {
			str_type = str_type.substr(1, str_type.length() - 2); //ok it's a class
		}
	}

	if (str_type == "void" || str_type == "V") {
		t |= JavaClass::ARG_TYPE_VOID;
		strsig += "V";
	} else if (str_type == "boolean" || str_type == "Z") {
		t |= JavaClass::ARG_TYPE_BOOLEAN;
		strsig += "Z";
	} else if (str_type == "byte" || str_type == "B") {
		t |= JavaClass::ARG_TYPE_BYTE;
		strsig += "B";
	} else if (str_type == "char" || str_type == "C") {
		t |= JavaClass::ARG_TYPE_CHAR;
		strsig += "C";
	} else if (str_type == "short" || str_type == "S") {
		t |= JavaClass::ARG_TYPE_SHORT;
		strsig += "S";
	} else if (str_type == "int" || str_type == "I") {
		t |= JavaClass::ARG_TYPE_INT;
		strsig += "I";
	} else if (str_type == "long" || str_type == "J") {
		t |= JavaClass::ARG_TYPE_LONG;
		strsig += "J";
	} else if (str_type == "float" || str_type == "F") {
		t |= JavaClass::ARG_TYPE_FLOAT;
		strsig += "F";
	} else if (str_type == "double" || str_type == "D") {
		t |= JavaClass::ARG_TYPE_DOUBLE;
		strsig += "D";
	} else if (str_type == "java.lang.String") {
		t |= JavaClass::ARG_TYPE_STRING;
		strsig += "Ljava/lang/String;";
	} else if (str_type == "java.lang.CharSequence") {
		t |= JavaClass::ARG_TYPE_CHARSEQUENCE;
		strsig += "Ljava/lang/CharSequence;";
	} else if (str_type == "org.godotengine.godot.variant.Callable") {
		t |= JavaClass::ARG_TYPE_CALLABLE;
		strsig += "Lorg/godotengine/godot/variant/Callable;";
	} else if (str_type == "java.lang.Boolean") {
		t |= JavaClass::ARG_TYPE_BOOLEAN | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Boolean;";
	} else if (str_type == "java.lang.Byte") {
		t |= JavaClass::ARG_TYPE_BYTE | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Byte;";
	} else if (str_type == "java.lang.Character") {
		t |= JavaClass::ARG_TYPE_CHAR | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Character;";
	} else if (str_type == "java.lang.Short") {
		t |= JavaClass::ARG_TYPE_SHORT | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Short;";
	} else if (str_type == "java.lang.Integer") {
		t |= JavaClass::ARG_TYPE_INT | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Integer;";
	} else if (str_type == "java.lang.Long") {
		t |= JavaClass::ARG_TYPE_LONG | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Long;";
	} else if (str_type == "java.lang.Float") {
		t |= JavaClass::ARG_TYPE_FLOAT | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Float;";
	} else if (str_type == "java.lang.Double") {
		t |= JavaClass::ARG_TYPE_DOUBLE | JavaClass::ARG_NUMBER_CLASS_BIT;
		strsig += "Ljava/lang/Double;";
	} else {
		//a class likely
		strsig += "L" + str_type.replace_char('.', '/') + ";";
		t |= JavaClass::ARG_TYPE_CLASS;
	}

	sig = t;

	return true;
}

bool JavaClass::_convert_object_to_variant(JNIEnv *env, jobject obj, Variant &var, uint32_t p_sig) {
	if (!obj) {
		var = Variant(); //seems null is just null...
		return true;
	}

	switch (p_sig) {
		case ARG_TYPE_VOID: {
			return Variant();
		} break;
		case ARG_TYPE_BOOLEAN | ARG_NUMBER_CLASS_BIT: {
			var = env->CallBooleanMethod(obj, JavaClassWrapper::data->Boolean_booleanValue);
			return true;
		} break;
		case ARG_TYPE_BYTE | ARG_NUMBER_CLASS_BIT: {
			var = env->CallByteMethod(obj, JavaClassWrapper::data->Byte_byteValue);
			return true;

		} break;
		case ARG_TYPE_CHAR | ARG_NUMBER_CLASS_BIT: {
			var = env->CallCharMethod(obj, JavaClassWrapper::data->Character_characterValue);
			return true;

		} break;
		case ARG_TYPE_SHORT | ARG_NUMBER_CLASS_BIT: {
			var = env->CallShortMethod(obj, JavaClassWrapper::data->Short_shortValue);
			return true;

		} break;
		case ARG_TYPE_INT | ARG_NUMBER_CLASS_BIT: {
			var = env->CallIntMethod(obj, JavaClassWrapper::data->Integer_integerValue);
			return true;

		} break;
		case ARG_TYPE_LONG | ARG_NUMBER_CLASS_BIT: {
			var = (int64_t)env->CallLongMethod(obj, JavaClassWrapper::data->Long_longValue);
			return true;

		} break;
		case ARG_TYPE_FLOAT | ARG_NUMBER_CLASS_BIT: {
			var = env->CallFloatMethod(obj, JavaClassWrapper::data->Float_floatValue);
			return true;

		} break;
		case ARG_TYPE_DOUBLE | ARG_NUMBER_CLASS_BIT: {
			var = env->CallDoubleMethod(obj, JavaClassWrapper::data->Double_doubleValue);
			return true;
		} break;
		case ARG_TYPE_STRING: {
			var = jstring_to_string((jstring)obj, env);
			return true;
		} break;
		case ARG_TYPE_CHARSEQUENCE: {
			var = charsequence_to_string(env, obj);
			return true;
		} break;
		case ARG_TYPE_CALLABLE: {
			var = jcallable_to_callable(env, obj);
			return true;
		} break;
		case ARG_TYPE_CLASS: {
			jclass java_class = env->GetObjectClass(obj);
			Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class);
			env->DeleteLocalRef(java_class);

			if (java_class_wrapped.is_valid()) {
				String cn = java_class_wrapped->get_java_class_name();
				if (cn == "org.godotengine.godot.Dictionary") {
					var = _jobject_to_variant(env, obj);
				} else {
					Ref<JavaObject> ret = Ref<JavaObject>(memnew(JavaObject(java_class_wrapped, obj)));
					var = ret;
				}
				return true;
			}

			return false;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_VOID: {
			var = Array(); // ?
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
			Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			for (int i = 0; i < count; i++) {
				jboolean val;
				env->GetBooleanArrayRegion((jbooleanArray)arr, i, 1, &val);
				ret[i] = (bool)val;
			}

			var = ret;
			return true;

		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_BYTE: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetByteArrayRegion((jbyteArray)arr, 0, count, (int8_t *)ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			// Char arrays are UTF-16 encoded, so it's double the length.
			ret.resize(count * 2);
			env->GetCharArrayRegion((jcharArray)arr, 0, count, (jchar *)ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_SHORT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int32_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jshort val;
				env->GetShortArrayRegion((jshortArray)arr, i, 1, &val);
				ptr[i] = val;
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_INT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetIntArrayRegion((jintArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_LONG: {
			PackedInt64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetLongArrayRegion((jlongArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
			PackedFloat32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetFloatArrayRegion((jfloatArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
			PackedFloat64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);
			env->GetDoubleArrayRegion((jdoubleArray)arr, 0, count, ret.ptrw());

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN: {
			Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (o) {
					bool val = env->CallBooleanMethod(o, JavaClassWrapper::data->Boolean_booleanValue);
					ret[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;

		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_BYTE: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			uint8_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int val = env->CallByteMethod(o, JavaClassWrapper::data->Byte_byteValue);
					ptr[i] = (uint8_t)val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_CHAR: {
			PackedByteArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			// Char arrays are UTF-16 encoded, so it's double the length.
			ret.resize(count * 2);

			jchar *ptr = (jchar *)ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					count = i;
					break;
				} else {
					int val = env->CallCharMethod(o, JavaClassWrapper::data->Character_characterValue);
					ptr[i] = (jchar)val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_SHORT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int32_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int val = env->CallShortMethod(o, JavaClassWrapper::data->Short_shortValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_INT: {
			PackedInt32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int32_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int val = env->CallIntMethod(o, JavaClassWrapper::data->Integer_integerValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_LONG: {
			PackedInt64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			int64_t *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0;
				} else {
					int64_t val = env->CallLongMethod(o, JavaClassWrapper::data->Long_longValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_FLOAT: {
			PackedFloat32Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			float *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0.0;
				} else {
					float val = env->CallFloatMethod(o, JavaClassWrapper::data->Float_floatValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_NUMBER_CLASS_BIT | ARG_ARRAY_BIT | ARG_TYPE_DOUBLE: {
			PackedFloat64Array ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			double *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (!o) {
					ptr[i] = 0.0;
				} else {
					double val = env->CallDoubleMethod(o, JavaClassWrapper::data->Double_doubleValue);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;

		case ARG_ARRAY_BIT | ARG_TYPE_STRING: {
			PackedStringArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			String *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (o) {
					String val = jstring_to_string((jstring)o, env);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE: {
			PackedStringArray ret;
			jobjectArray arr = (jobjectArray)obj;

			int count = env->GetArrayLength(arr);
			ret.resize(count);

			String *ptr = ret.ptrw();
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(arr, i);
				if (o) {
					String val = charsequence_to_string(env, o);
					ptr[i] = val;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE: {
			Array ret;
			jobjectArray jarr = (jobjectArray)obj;
			int count = env->GetArrayLength(jarr);
			ret.resize(count);
			for (int i = 0; i < count; i++) {
				jobject o = env->GetObjectArrayElement(jarr, i);
				if (o) {
					Callable callable = jcallable_to_callable(env, o);
					ret[i] = callable;
				}
				env->DeleteLocalRef(o);
			}

			var = ret;
			return true;
		} break;
		case ARG_ARRAY_BIT | ARG_TYPE_CLASS: {
			Array ret;
			jobjectArray jarr = (jobjectArray)obj;
			int count = env->GetArrayLength(jarr);
			ret.resize(count);
			for (int i = 0; i < count; i++) {
				jobject obj = env->GetObjectArrayElement(jarr, i);
				if (obj) {
					jclass java_class = env->GetObjectClass(obj);
					Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class);
					env->DeleteLocalRef(java_class);

					if (java_class_wrapped.is_valid()) {
						String cn = java_class_wrapped->get_java_class_name();
						if (cn == "org.godotengine.godot.Dictionary") {
							ret[i] = _jobject_to_variant(env, obj);
						} else {
							Ref<JavaObject> java_obj_wrapped = Ref<JavaObject>(memnew(JavaObject(java_class_wrapped, obj)));
							ret[i] = java_obj_wrapped;
						}
					}
				}
				env->DeleteLocalRef(obj);
			}

			var = ret;
			return true;
		} break;
	}

	return false;
}

bool JavaClassWrapper::_wrap_class_components(JNIEnv *p_env, const Ref<JavaClass> &p_java_class, jclass p_class, bool p_allow_non_public_methods_access) {
	ERR_FAIL_NULL_V(p_class, false);

	jobjectArray constructors = (jobjectArray)p_env->CallObjectMethod(p_class, data->Class_getConstructors);
	if (p_env->ExceptionCheck()) {
		p_env->ExceptionDescribe();
		p_env->ExceptionClear();
	}
	ERR_FAIL_NULL_V(constructors, false);

	jobjectArray methods = (jobjectArray)p_env->CallObjectMethod(p_class, data->Class_getDeclaredMethods);
	if (p_env->ExceptionCheck()) {
		p_env->ExceptionDescribe();
		p_env->ExceptionClear();
	}
	ERR_FAIL_NULL_V(methods, false);

	int constructor_count = p_env->GetArrayLength(constructors);
	int method_count = p_env->GetArrayLength(methods);

	int methods_and_constructors_count = method_count + constructor_count;
	for (int i = 0; i < methods_and_constructors_count; i++) {
		bool is_constructor = i < constructor_count;
		jobject obj = is_constructor
				? p_env->GetObjectArrayElement(constructors, i)
				: p_env->GetObjectArrayElement(methods, i - constructor_count);
		ERR_CONTINUE(!obj);

		String str_method;
		if (is_constructor) {
			str_method = "<init>";
		} else {
			jstring name = (jstring)p_env->CallObjectMethod(obj, data->Method_getName);
			str_method = jstring_to_string(name, p_env);
			p_env->DeleteLocalRef(name);
		}

		jint mods = p_env->CallIntMethod(obj, is_constructor ? data->Constructor_getModifiers : data->Method_getModifiers);
		bool is_public = (mods & 0x0001) != 0; // java.lang.reflect.Modifier.PUBLIC

		if (!is_public && (is_constructor || !p_allow_non_public_methods_access)) {
			p_env->DeleteLocalRef(obj);
			continue; //not public bye
		}

		jobjectArray param_types = (jobjectArray)p_env->CallObjectMethod(obj, is_constructor ? data->Constructor_getParameterTypes : data->Method_getParameterTypes);
		int count = p_env->GetArrayLength(param_types);

		if (!p_java_class->methods.has(str_method)) {
			p_java_class->methods[str_method] = List<JavaClass::MethodInfo>();
		}

		JavaClass::MethodInfo mi;
		mi._public = is_public;
		mi._static = (mods & 0x8) != 0;
		mi._constructor = is_constructor;
		bool valid = true;
		String signature = "(";

		for (int j = 0; j < count; j++) {
			jobject obj2 = p_env->GetObjectArrayElement(param_types, j);
			String strsig;
			uint32_t sig = 0;
			if (!_get_type_sig(p_env, obj2, sig, strsig)) {
				valid = false;
				p_env->DeleteLocalRef(obj2);
				break;
			}
			signature += strsig;
			mi.param_types.push_back(sig);
			mi.param_sigs.push_back(strsig);
			p_env->DeleteLocalRef(obj2);
		}

		if (!valid) {
			print_line("Method can't be bound (unsupported arguments): " + p_java_class->java_class_name + "::" + str_method);
			p_env->DeleteLocalRef(obj);
			p_env->DeleteLocalRef(param_types);
			continue;
		}

		signature += ")";

		if (is_constructor) {
			signature += "V";
			mi.return_type = JavaClass::ARG_TYPE_CLASS;
		} else {
			jobject return_type = (jobject)p_env->CallObjectMethod(obj, data->Method_getReturnType);

			String strsig;
			uint32_t sig = 0;
			if (!_get_type_sig(p_env, return_type, sig, strsig)) {
				print_line("Method can't be bound (unsupported return type): " + p_java_class->java_class_name + "::" + str_method);
				p_env->DeleteLocalRef(obj);
				p_env->DeleteLocalRef(param_types);
				p_env->DeleteLocalRef(return_type);
				continue;
			}

			signature += strsig;
			mi.return_type = sig;

			p_env->DeleteLocalRef(return_type);
		}

		bool discard = false;

		for (List<JavaClass::MethodInfo>::Element *E = p_java_class->methods[str_method].front(); E; E = E->next()) {
			float new_likeliness = 0;
			float existing_likeliness = 0;

			if (E->get().param_types.size() != mi.param_types.size()) {
				continue;
			}
			bool this_valid = true;
			for (int j = 0; j < E->get().param_types.size(); j++) {
				Variant::Type _new;
				float new_l;
				Variant::Type existing;
				float existing_l;
				JavaClass::_convert_to_variant_type(E->get().param_types[j], existing, existing_l);
				JavaClass::_convert_to_variant_type(mi.param_types[j], _new, new_l);
				if (_new != existing) {
					this_valid = false;
					break;
				} else if ((_new == Variant::OBJECT || _new == Variant::ARRAY) && E->get().param_sigs[j] != mi.param_sigs[j]) {
					this_valid = false;
					break;
				}
				new_likeliness += new_l;
				existing_likeliness += existing_l;
			}

			if (!this_valid) {
				continue;
			}

			if (new_likeliness > existing_likeliness) {
				p_java_class->methods[str_method].erase(E);
				break;
			} else {
				discard = true;
			}
		}

		if (!discard) {
			if (mi._static) {
				mi.method = p_env->GetStaticMethodID(p_class, str_method.utf8().get_data(), signature.utf8().get_data());
			} else {
				mi.method = p_env->GetMethodID(p_class, str_method.utf8().get_data(), signature.utf8().get_data());
			}

			if (p_env->ExceptionCheck()) {
				// Exceptions may be thrown when trying to access hidden methods; write the exception to the logs and continue.
				p_env->ExceptionDescribe();
				p_env->ExceptionClear();
				continue;
			}

			if (mi.method) {
				p_java_class->methods[str_method].push_back(mi);
			}
		}

		p_env->DeleteLocalRef(obj);
		p_env->DeleteLocalRef(param_types);
	}

	p_env->DeleteLocalRef(constructors);
	p_env->DeleteLocalRef(methods);

	jobjectArray fields = (jobjectArray)p_env->CallObjectMethod(p_class, data->Class_getFields);

	int count = p_env->GetArrayLength(fields);

	for (int i = 0; i < count; i++) {
		jobject obj = p_env->GetObjectArrayElement(fields, i);
		ERR_CONTINUE(!obj);

		jstring name = (jstring)p_env->CallObjectMethod(obj, data->Field_getName);
		String str_field = jstring_to_string(name, p_env);
		p_env->DeleteLocalRef(name);
		int mods = p_env->CallIntMethod(obj, data->Field_getModifiers);
		if ((mods & 0x8) && (mods & 0x1)) { //static public!

			jobject objc = p_env->CallObjectMethod(obj, data->Field_get, nullptr);
			if (objc) {
				uint32_t sig;
				String strsig;
				jclass cl = p_env->GetObjectClass(objc);
				if (JavaClassWrapper::_get_type_sig(p_env, cl, sig, strsig)) {
					if ((sig & JavaClass::ARG_TYPE_MASK) <= JavaClass::ARG_TYPE_STRING) {
						Variant value;
						if (JavaClass::_convert_object_to_variant(p_env, objc, value, sig)) {
							p_java_class->constant_map[str_field] = value;
						}
					}
				}

				p_env->DeleteLocalRef(cl);
			}

			p_env->DeleteLocalRef(objc);
		}
		p_env->DeleteLocalRef(obj);
	}

	p_env->DeleteLocalRef(fields);
	return true;
}{
					if ((sig & JavaClass::ARG_TYPE_MASK) <= JavaClass::ARG_TYPE_STRING) {
						Variant value;
						if (JavaClass::_convert_object_to_variant(p_env, objc, value, sig)) {
							p_java_class->constant_map[str_field] = value;
						}
					}
				}

				p_env->DeleteLocalRef(cl);
			}

			p_env->DeleteLocalRef(objc);
		}
		p_env->DeleteLocalRef(obj);
	}

	p_env->DeleteLocalRef(fields);
	return true;
}

Ref<JavaClass> JavaClassWrapper::_wrap(const String &p_class, bool p_allow_non_public_methods_access) {
	String class_name_dots = p_class.replace_char('/', '.');
	if (data->class_cache.has(class_name_dots)) {
		return data->class_cache[class_name_dots];
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaClass>());

	jclass bclass = jni_find_class(env, class_name_dots.replace_char('.', '/').utf8().get_data());
	ERR_FAIL_NULL_V_MSG(bclass, Ref<JavaClass>(), vformat("Java class '%s' not found.", p_class));

	Ref<JavaClass> java_class;
	java_class.instantiate();
	java_class->java_class_name = class_name_dots;
	Vector<String> class_name_parts = class_name_dots.split(".");
	java_class->java_constructor_name = class_name_parts[class_name_parts.size() - 1];
	java_class->_class = (jclass)env->NewGlobalRef(bclass);
	java_class->is_interface = env->CallBooleanMethod(bclass, data->Class_isInterface);

	bool class_components_configured;
	if (_is_proxy_class(env, bclass)) {
		// Proxy class components must be setup using the interfaces they implement.
		jobjectArray interfaces = (jobjectArray)env->CallObjectMethod(bclass, data->Class_getInterfaces);
		if (env->ExceptionCheck()) {
			env->ExceptionDescribe();
			env->ExceptionClear();
		}
		int interfaces_count = interfaces == nullptr ? 0 : env->GetArrayLength(interfaces);
		for (int i = 0; i < interfaces_count; i++) {
			jclass interface = (jclass)env->GetObjectArrayElement(interfaces, i);
			ERR_CONTINUE(!interface);

			jstring j_interface_name = (jstring)env->CallObjectMethod(interface, data->Class_getName);
			String interface_name = jstring_to_string(j_interface_name, env);
			env->DeleteLocalRef(j_interface_name);
			if (!_wrap_class_components(env, java_class, interface, p_allow_non_public_methods_access)) {
				ERR_PRINT(vformat("Unable to set up components for proxy class interface %s.", interface_name));
				continue;
			}

			class_components_configured = true;
		}
	} else {
		class_components_configured = _wrap_class_components(env, java_class, bclass, p_allow_non_public_methods_access);
	}
	env->DeleteLocalRef(bclass);

	if (!class_components_configured) {
		java_class.unref();
		return Ref<JavaClass>();
	}

	// Cache the initialized JavaClass instance.
	data->class_cache[class_name_dots] = java_class;

	return java_class;
}

Ref<JavaObject> JavaClassWrapper::create_sam_callback(const String &p_sam_interface, const Callable &p_callable) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->android_runtime_class, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->ARP_create_proxy_from_godot_callable, Ref<JavaObject>());

	Ref<JavaClass> interface_class = wrap(p_sam_interface);
	if (interface_class.is_null()) {
		ERR_PRINT(vformat("Invalid java class: %s.", p_sam_interface));
		return Ref<JavaObject>();
	}
	if (!interface_class->is_interface) {
		ERR_PRINT(vformat("Java class %s must be an interface.", p_sam_interface));
		return Ref<JavaObject>();
	}

	if (interface_class->methods.size() != 1) {
		ERR_PRINT(vformat("%s must be a Single Abstract Method (SAM) interface.", p_sam_interface));
		return Ref<JavaObject>();
	}

	jobject j_callable = callable_to_jcallable(env, p_callable);
	jstring j_interface = env->NewStringUTF(p_sam_interface.utf8().get_data());

	jobject proxy = env->CallStaticObjectMethod(data->android_runtime_class, data->ARP_create_proxy_from_godot_callable, j_interface, j_callable);
	Ref<JavaObject> result = _jobject_to_variant(env, proxy);

	env->DeleteLocalRef(j_callable);
	env->DeleteLocalRef(j_interface);
	env->DeleteLocalRef(proxy);

	return result;
}

Ref<JavaObject> JavaClassWrapper::create_proxy(const Object *p_object, const PackedStringArray &p_interfaces) {
	ERR_FAIL_NULL_V(p_object, Ref<JavaObject>());
	ERR_FAIL_COND_V(p_interfaces.is_empty(), Ref<JavaObject>());

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->android_runtime_class, Ref<JavaObject>());
	ERR_FAIL_NULL_V(data->ARP_create_proxy_from_godot_callable, Ref<JavaObject>());

	for (const String &interface_name : p_interfaces) {
		Ref<JavaClass> interface_class = wrap(interface_name);
		if (interface_class.is_null()) {
			ERR_PRINT(vformat("Invalid java class: %s.", interface_name));
			return Ref<JavaObject>();
		}

		if (!interface_class->is_interface) {
			ERR_PRINT(vformat("Java class %s must be an interface.", interface_name));
			return Ref<JavaObject>();
		}
	}

	jlong object_id = p_object->get_instance_id();
	jobjectArray j_interfaces = env->NewObjectArray(p_interfaces.size(), jni_find_class(env, "java/lang/String"), nullptr);
	for (int i = 0; i < p_interfaces.size(); i++) {
		jstring j_interface = env->NewStringUTF(p_interfaces[i].utf8().get_data());
		env->SetObjectArrayElement(j_interfaces, i, j_interface);
		env->DeleteLocalRef(j_interface);
	}

	jobject proxy = env->CallStaticObjectMethod(data->android_runtime_class, data->ARP_create_proxy_from_godot_object_id, object_id, j_interfaces);
	Ref<JavaObject> result = _jobject_to_variant(env, proxy);

	env->DeleteLocalRef(j_interfaces);
	env->DeleteLocalRef(proxy);

	return result;
}

Ref<JavaClass> JavaClassWrapper::wrap_jclass(jclass p_class, bool p_allow_non_public_methods_access) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, Ref<JavaClass>());

	jstring class_name = (jstring)env->CallObjectMethod(p_class, data->Class_getName);
	String class_name_string = jstring_to_string(class_name, env);
	env->DeleteLocalRef(class_name);

	return _wrap(class_name_string, p_allow_non_public_methods_access);
}

bool JavaClassWrapper::_is_proxy_class(JNIEnv *env, jclass p_class) {
	ERR_FAIL_NULL_V(data->proxy_class, false);
	ERR_FAIL_NULL_V(data->Proxy_isProxyClass, false);
	ERR_FAIL_NULL_V(env, false);

	return env->CallStaticBooleanMethod(data->proxy_class, data->Proxy_isProxyClass, p_class);
}

void JavaClassWrapper::finalize() {
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void JavaClassWrapper::initialize() {
	if (data) {
		return;
	}
	data = memnew(Data);

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	data->proxy_class = jni_find_class(env, "java/lang/reflect/Proxy");
	if (data->proxy_class) {
		data->proxy_class = (jclass)env->NewGlobalRef(data->proxy_class);
		data->Proxy_isProxyClass = env->GetStaticMethodID(data->proxy_class, "isProxyClass", "(Ljava/lang/Class;)Z");
	}

	data->android_runtime_class = jni_find_class(env, "org/godotengine/godot/plugin/AndroidRuntimePlugin");
	if (data->android_runtime_class) {
		data->android_runtime_class = (jclass)env->NewGlobalRef(data->android_runtime_class);
		data->ARP_create_proxy_from_godot_callable = env->GetStaticMethodID(data->android_runtime_class, "createProxyFromGodotCallable", "(Ljava/lang/String;Lorg/godotengine/godot/variant/Callable;)Ljava/lang/Object;");
		data->ARP_create_proxy_from_godot_object_id = env->GetStaticMethodID(data->android_runtime_class, "createProxyFromGodotObjectID", "(J[Ljava/lang/String;)Ljava/lang/Object;");
	}

	jclass bclass = jni_find_class(env, "java/lang/Class");
	data->Class_getConstructors = env->GetMethodID(bclass, "getConstructors", "()[Ljava/lang/reflect/Constructor;");
	data->Class_getDeclaredMethods = env->GetMethodID(bclass, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
	data->Class_getFields = env->GetMethodID(bclass, "getFields", "()[Ljava/lang/reflect/Field;");
	data->Class_getInterfaces = env->GetMethodID(bclass, "getInterfaces", "()[Ljava/lang/Class;");
	data->Class_getName = env->GetMethodID(bclass, "getName", "()Ljava/lang/String;");
	data->Class_getSuperclass = env->GetMethodID(bclass, "getSuperclass", "()Ljava/lang/Class;");
	data->Class_isInterface = env->GetMethodID(bclass, "isInterface", "()Z");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/reflect/Constructor");
	data->Constructor_getParameterTypes = env->GetMethodID(bclass, "getParameterTypes", "()[Ljava/lang/Class;");
	data->Constructor_getModifiers = env->GetMethodID(bclass, "getModifiers", "()I");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/reflect/Method");
	data->Method_getParameterTypes = env->GetMethodID(bclass, "getParameterTypes", "()[Ljava/lang/Class;");
	data->Method_getReturnType = env->GetMethodID(bclass, "getReturnType", "()Ljava/lang/Class;");
	data->Method_getName = env->GetMethodID(bclass, "getName", "()Ljava/lang/String;");
	data->Method_getModifiers = env->GetMethodID(bclass, "getModifiers", "()I");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/reflect/Field");
	data->Field_getName = env->GetMethodID(bclass, "getName", "()Ljava/lang/String;");
	data->Field_getModifiers = env->GetMethodID(bclass, "getModifiers", "()I");
	data->Field_get = env->GetMethodID(bclass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Boolean");
	data->Boolean_booleanValue = env->GetMethodID(bclass, "booleanValue", "()Z");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Byte");
	data->Byte_byteValue = env->GetMethodID(bclass, "byteValue", "()B");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Character");
	data->Character_characterValue = env->GetMethodID(bclass, "charValue", "()C");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Short");
	data->Short_shortValue = env->GetMethodID(bclass, "shortValue", "()S");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Integer");
	data->Integer_integerValue = env->GetMethodID(bclass, "intValue", "()I");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Long");
	data->Long_longValue = env->GetMethodID(bclass, "longValue", "()J");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Float");
	data->Float_floatValue = env->GetMethodID(bclass, "floatValue", "()F");
	env->DeleteLocalRef(bclass);

	bclass = jni_find_class(env, "java/lang/Double");
	data->Double_doubleValue = env->GetMethodID(bclass, "doubleValue", "()D");
	env->DeleteLocalRef(bclass);
}


// --- Merged from platform/android/java_godot_lib_jni.cpp ---
// --- Inlined from platform/android/java_godot_lib_jni.h ---
#include <android/log.h>
#include <jni.h>

// These functions can be called from within JAVA and are the means by which our JAVA implementation calls back into our C++ code.
// See java/src/org/godotengine/godot/GodotLib.java for the JAVA side of this (yes that's why we have the long names)
extern "C" {
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_initialize(JNIEnv *env, jclass clazz, jobject p_godot_native_bridge, jobject p_asset_manager, jobject p_godot_io, jobject p_net_utils, jobject p_directory_access_handler, jobject p_file_access_handler, jboolean p_use_apk_expansion);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ondestroy(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_setup(JNIEnv *env, jclass clazz, jobjectArray p_cmdline, jobject p_godot_tts);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_resize(JNIEnv *env, jclass clazz, jobject p_surface, jint p_width, jint p_height);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_newcontext(JNIEnv *env, jclass clazz, jobject p_surface);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_step(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ttsCallback(JNIEnv *env, jclass clazz, jint event, jlong id, jint pos);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_back(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchMouseEvent(JNIEnv *env, jclass clazz, jint p_event_type, jint p_button_mask, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y, jboolean p_double_click, jboolean p_source_mouse_relative, jfloat p_pressure, jfloat p_tilt_x, jfloat p_tilt_y);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchTouchEvent(JNIEnv *env, jclass clazz, jint ev, jint pointer, jint pointer_count, jfloatArray positions, jboolean p_double_tap);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnify(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_factor);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_pan(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_key(JNIEnv *env, jclass clazz, jint p_physical_keycode, jint p_unicode, jint p_key_label, jboolean p_pressed, jboolean p_echo);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joybutton(JNIEnv *env, jclass clazz, jint p_device, jint p_button, jboolean p_pressed);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyaxis(JNIEnv *env, jclass clazz, jint p_device, jint p_axis, jfloat p_value);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyhat(JNIEnv *env, jclass clazz, jint p_device, jint p_hat_x, jint p_hat_y);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyconnectionchanged(JNIEnv *env, jclass clazz, jint p_device, jboolean p_connected, jstring p_name);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_accelerometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gravity(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnetometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gyroscope(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusin(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusout(JNIEnv *env, jclass clazz);
JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getGlobal(JNIEnv *env, jclass clazz, jstring path);
JNIEXPORT jobjectArray JNICALL Java_org_godotengine_godot_GodotLib_getRendererInfo(JNIEnv *env, jclass clazz, jboolean p_vulkan_requirements_met);
JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getEditorSetting(JNIEnv *env, jclass clazz, jstring p_setting_key);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorSetting(JNIEnv *env, jclass clazz, jstring p_key, jobject p_data);
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_GodotLib_getEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_default_value);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_data);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setVirtualKeyboardHeight(JNIEnv *env, jclass clazz, jint p_height);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_requestPermissionResult(JNIEnv *env, jclass clazz, jstring p_permission, jboolean p_result);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onNightModeChanged(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_hardwareKeyboardConnected(JNIEnv *env, jclass clazz, jboolean p_connected);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_filePickerCallback(JNIEnv *env, jclass clazz, jboolean p_ok, jobjectArray p_selected_paths);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererResumed(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererPaused(JNIEnv *env, jclass clazz);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onOrientationChange(JNIEnv *env, jclass clazz, jint p_orientation);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_shouldDispatchInputToRenderThread(JNIEnv *env, jclass clazz);
JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getProjectResourceDir(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isEditorHint(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isProjectManagerHint(JNIEnv *env, jclass clazz);
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_hasFeature(JNIEnv *env, jclass clazz, jstring p_feature);
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onPictureInPictureModeChanged(JNIEnv *env, jclass clazz, jboolean p_is_in_picture_in_picture_mode);
}

// --- End platform/android/java_godot_lib_jni.h ---

#include "platform/android/os_android.h"
#include "platform/android/api/java_class_wrapper.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/os/thread.h"
#include "core/os/thread_safe.h"
#include "main/main.h"
#include "servers/camera/camera_server.h"
#include "servers/rendering/renderer.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif

#include <android/asset_manager_jni.h>
#include <android/input.h>
#include <android/native_window_jni.h>
#include <unistd.h>

static AndroidInputHandler *input_handler = nullptr;
static GodotJavaWrapper *godot_java = nullptr;
static GodotIOJavaWrapper *godot_io_java = nullptr;

enum StartupStep {
	STEP_TERMINATED = -1,
	STEP_SETUP,
	STEP_SHOW_LOGO,
	STEP_STARTED
};

static SafeNumeric<int> step; // Shared between UI and render threads

static Vector3 accelerometer;
static Vector3 gravity;
static Vector3 magnetometer;
static Vector3 gyroscope;

static void _terminate(JNIEnv *env, bool p_restart = false) {
	if (step.get() == STEP_TERMINATED) {
		return;
	}

	step.set(STEP_TERMINATED); // Ensure no further steps are attempted and no further events are sent

	// lets cleanup
	// Unregister android plugins
	unregister_plugins_singletons();

	JavaClassWrapper::finalize();
	if (input_handler) {
		delete input_handler;
	}
	// Whether restarting is handled by 'Main::cleanup()'
	bool restart_on_cleanup = false;
	if (OS_Android::is_initialized()) {
		restart_on_cleanup = OS::is_restart_on_exit_set();
		OS_Android::main_loop_end();
		Main::cleanup();
		OS::finalize();
		OS::finalize_core();
	}
	if (godot_io_java) {
		delete godot_io_java;
	}

	TTS_Android::terminate();
	FileAccessAndroid::terminate();
	DirAccessJAndroid::terminate();
	FileAccessFilesystemJAndroid::terminate();
	NetSocketAndroid::terminate();

	cleanup_android_class_loader();
	godot_cleanup_profiler();

	if (godot_java) {
		godot_java->on_godot_terminating(env);
		if (!restart_on_cleanup) {
			if (p_restart) {
				godot_java->restart(env);
			} else {
				godot_java->force_quit(env);
			}
		}
		delete godot_java;
	}
}

static String rendering_source_to_string(OS::RenderingSource p_source) {
	switch (p_source) {
		case OS::RENDERING_SOURCE_DEFAULT:
			return "Default";
		case OS::RENDERING_SOURCE_PROJECT_SETTING:
			return "ProjectSettings";
		case OS::RENDERING_SOURCE_COMMANDLINE:
			return "Commandline";
		case OS::RENDERING_SOURCE_FALLBACK:
			return "Fallback";
		default:
			return "Unknown";
	}
}

extern "C" {

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setVirtualKeyboardHeight(JNIEnv *env, jclass clazz, jint p_height) {
	if (godot_io_java) {
		godot_io_java->set_vk_height(p_height);
	}
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_initialize(JNIEnv *env, jclass clazz, jobject p_godot_native_bridge, jobject p_asset_manager, jobject p_godot_io, jobject p_net_utils, jobject p_directory_access_handler, jobject p_file_access_handler, jboolean p_use_apk_expansion) {
	godot_init_profiler();

	JavaVM *jvm;
	env->GetJavaVM(&jvm);

	init_thread_jandroid(jvm, env);
	setup_android_class_loader();

	// create our wrapper classes
	godot_java = new GodotJavaWrapper(env, p_godot_native_bridge);
	godot_io_java = new GodotIOJavaWrapper(env, p_godot_io);

	FileAccessAndroid::setup(p_asset_manager);
	DirAccessJAndroid::setup(p_directory_access_handler);
	FileAccessFilesystemJAndroid::setup(p_file_access_handler);
	NetSocketAndroid::setup(p_net_utils);

	OS_Android::setup(godot_java, godot_io_java, p_use_apk_expansion);
	OS::initialize_core();
	OS::initialize();

	return true;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ondestroy(JNIEnv *env, jclass clazz) {
	_terminate(env, false);
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_setup(JNIEnv *env, jclass clazz, jobjectArray p_cmdline, jobject p_godot_tts) {
	setup_android_thread();

	const char **cmdline = nullptr;
	jstring *j_cmdline = nullptr;
	int cmdlen = 0;
	if (p_cmdline) {
		cmdlen = env->GetArrayLength(p_cmdline);
		if (cmdlen) {
			cmdline = (const char **)memalloc((cmdlen + 1) * sizeof(const char *));
			ERR_FAIL_NULL_V_MSG(cmdline, false, "Out of memory.");
			cmdline[cmdlen] = nullptr;
			j_cmdline = (jstring *)memalloc(cmdlen * sizeof(jstring));
			ERR_FAIL_NULL_V_MSG(j_cmdline, false, "Out of memory.");

			for (int i = 0; i < cmdlen; i++) {
				jstring string = (jstring)env->GetObjectArrayElement(p_cmdline, i);
				const char *rawString = env->GetStringUTFChars(string, nullptr);

				cmdline[i] = rawString;
				j_cmdline[i] = string;
			}
		}
	}

	Error err = Main::setup(OS_Android::ANDROID_EXEC_PATH, cmdlen, (char **)cmdline, false);
	if (cmdline) {
		if (j_cmdline) {
			for (int i = 0; i < cmdlen; ++i) {
				env->ReleaseStringUTFChars(j_cmdline[i], cmdline[i]);
			}
			memfree(j_cmdline);
		}
		memfree(cmdline);
	}

	// Note: --help and --version return ERR_HELP, but this should be translated to 0 if exit codes are propagated.
	if (err != OK) {
		return false;
	}

	Thread::release_main_thread(); // setup2 will be called from another thread.
	set_current_thread_safe_for_nodes(false);

	TTS_Android::setup(p_godot_tts);

	JavaClassWrapper::initialize();
	return true;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_resize(JNIEnv *env, jclass clazz, jobject p_surface, jint p_width, jint p_height) {
	if (OS_Android::is_initialized()) {
		OS_Android::set_display_size(Size2i(p_width, p_height));

		// No need to reset the surface during startup
		if (step.get() > STEP_SETUP) {
			if (p_surface) {
				ANativeWindow *native_window = ANativeWindow_fromSurface(env, p_surface);
				OS_Android::set_native_window(native_window);
			}
			DisplayServer::driver_as<DisplayServerAndroid>()->reset_window();
			DisplayServer::driver_as<DisplayServerAndroid>()->notify_surface_changed(p_width, p_height);
		}
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_newcontext(JNIEnv *env, jclass clazz, jobject p_surface) {
	if (OS_Android::is_initialized()) {
		if (step.get() == STEP_SETUP) {
			// During startup
			if (p_surface) {
				ANativeWindow *native_window = ANativeWindow_fromSurface(env, p_surface);
				OS_Android::set_native_window(native_window);
			}
		} else {
			// Rendering context recreated because it was lost; restart app to let it reload everything
			_terminate(env, true);
		}
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_back(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (DisplayServerAndroid *dsa = DisplayServer::driver_as<DisplayServerAndroid>()) {
		dsa->send_window_event(DisplayServerEnums::WINDOW_EVENT_GO_BACK_REQUEST);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_ttsCallback(JNIEnv *env, jclass clazz, jint event, jlong id, jint pos) {
	TTS_Android::_java_utterance_callback(event, id, pos);
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_step(JNIEnv *env, jclass clazz) {
	if (step.get() == STEP_TERMINATED) {
		return true;
	}

	if (step.get() == STEP_SETUP) {
		// Since Godot is initialized on the UI thread, main_thread_id was set to that thread's id,
		// but for Godot purposes, the main thread is the one running the game loop
		Main::setup2(false); // The logo is shown in the next frame otherwise we run into rendering issues
		input_handler = new AndroidInputHandler();
		step.increment();
		return true;
	}

	if (step.get() == STEP_SHOW_LOGO) {
		bool xr_enabled = false;
#ifndef XR_DISABLED
		// Unlike PCVR, there's no additional 2D screen onto which to render the boot logo,
		// so we skip this step if xr is enabled.
		if (XRServer::get_xr_mode() == XRServer::XRMODE_DEFAULT) {
			xr_enabled = GLOBAL_GET_CACHED(bool, "xr/shaders/enabled");
		} else {
			xr_enabled = XRServer::get_xr_mode() == XRServer::XRMODE_ON;
		}
#endif // XR_DISABLED
		if (!xr_enabled) {
			Main::setup_boot_logo();
		}

		step.increment();
		return true;
	}

	if (step.get() == STEP_STARTED) {
		if (Main::start() != EXIT_SUCCESS) {
			return true; // should exit instead and print the error
		}

		godot_java->on_godot_setup_completed(env);
		OS_Android::main_loop_begin();
		godot_java->on_godot_main_loop_started(env);
		step.increment();
	}

	DisplayServer::driver_as<DisplayServerAndroid>()->process_accelerometer(accelerometer);
	DisplayServer::driver_as<DisplayServerAndroid>()->process_gravity(gravity);
	DisplayServer::driver_as<DisplayServerAndroid>()->process_magnetometer(magnetometer);
	DisplayServer::driver_as<DisplayServerAndroid>()->process_gyroscope(gyroscope);

	bool should_swap_buffers = false;
	if (OS_Android::main_loop_iterate(&should_swap_buffers)) {
		_terminate(env, false);
	}

	return should_swap_buffers;
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchMouseEvent(JNIEnv *env, jclass clazz, jint p_event_type, jint p_button_mask, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y, jboolean p_double_click, jboolean p_source_mouse_relative, jfloat p_pressure, jfloat p_tilt_x, jfloat p_tilt_y) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	input_handler->process_mouse_event(p_event_type, p_button_mask, Point2(p_x, p_y), Vector2(p_delta_x, p_delta_y), p_double_click, p_source_mouse_relative, p_pressure, Vector2(p_tilt_x, p_tilt_y));
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_dispatchTouchEvent(JNIEnv *env, jclass clazz, jint ev, jint pointer, jint pointer_count, jfloatArray position, jboolean p_double_tap) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	Vector<AndroidInputHandler::TouchPos> points;
	for (int i = 0; i < pointer_count; i++) {
		jfloat p[6];
		env->GetFloatArrayRegion(position, i * 6, 6, p);
		AndroidInputHandler::TouchPos tp;
		tp.id = (int)p[0];
		tp.pos = Point2(p[1], p[2]);
		tp.pressure = p[3];
		tp.tilt = Vector2(p[4], p[5]);
		tp.double_tap = p_double_tap;
		points.push_back(tp);
	}

	input_handler->process_touch_event(ev, pointer, points);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnify(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_factor) {
	if (step.get() <= STEP_SETUP) {
		return;
	}
	input_handler->process_magnify(Point2(p_x, p_y), p_factor);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_pan(JNIEnv *env, jclass clazz, jfloat p_x, jfloat p_y, jfloat p_delta_x, jfloat p_delta_y) {
	if (step.get() <= STEP_SETUP) {
		return;
	}
	input_handler->process_pan(Point2(p_x, p_y), Vector2(p_delta_x, p_delta_y));
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joybutton(JNIEnv *env, jclass clazz, jint p_device, jint p_button, jboolean p_pressed) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	AndroidInputHandler::JoypadEvent jevent;
	jevent.device = p_device;
	jevent.type = AndroidInputHandler::JOY_EVENT_BUTTON;
	jevent.index = p_button;
	jevent.pressed = p_pressed;

	input_handler->process_joy_event(jevent);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyaxis(JNIEnv *env, jclass clazz, jint p_device, jint p_axis, jfloat p_value) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	AndroidInputHandler::JoypadEvent jevent;
	jevent.device = p_device;
	jevent.type = AndroidInputHandler::JOY_EVENT_AXIS;
	jevent.index = p_axis;
	jevent.value = p_value;

	input_handler->process_joy_event(jevent);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyhat(JNIEnv *env, jclass clazz, jint p_device, jint p_hat_x, jint p_hat_y) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	AndroidInputHandler::JoypadEvent jevent;
	jevent.device = p_device;
	jevent.type = AndroidInputHandler::JOY_EVENT_HAT;
	uint32_t hat = HatMask::CENTER;
	if (p_hat_x != 0) {
		if (p_hat_x < 0) {
			hat.set_flag(HatMask::LEFT);
		} else {
			hat.set_flag(HatMask::RIGHT);
		}
	}
	if (p_hat_y != 0) {
		if (p_hat_y < 0) {
			hat.set_flag(HatMask::UP);
		} else {
			hat.set_flag(HatMask::DOWN);
		}
	}
	jevent.hat = hat;

	input_handler->process_joy_event(jevent);
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_joyconnectionchanged(JNIEnv *env, jclass clazz, jint p_device, jboolean p_connected, jstring p_name) {
	if (OS_Android::is_initialized()) {
		String name = jstring_to_string(p_name, env);
		(void)name;
	}
}

// Called on the UI thread
JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_key(JNIEnv *env, jclass clazz, jint p_physical_keycode, jint p_unicode, jint p_key_label, jboolean p_pressed, jboolean p_echo) {
	if (step.get() <= STEP_SETUP) {
		return;
	}
	input_handler->process_key_event(p_physical_keycode, p_unicode, p_key_label, p_pressed, p_echo);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_accelerometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	accelerometer = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gravity(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	gravity = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_magnetometer(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	magnetometer = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_gyroscope(JNIEnv *env, jclass clazz, jfloat x, jfloat y, jfloat z) {
	gyroscope = Vector3(x, y, z);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusin(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	OS_Android::main_loop_focusin();
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_focusout(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	OS_Android::main_loop_focusout();
}

JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getGlobal(JNIEnv *env, jclass clazz, jstring path) {
	String js = jstring_to_string(path, env);

	Variant setting_with_override = GLOBAL_GET(js);
	String setting_value = (setting_with_override.get_type() == Variant::NIL) ? "" : setting_with_override;
	return env->NewStringUTF(setting_value.utf8().get_data());
}

JNIEXPORT jobjectArray JNICALL Java_org_godotengine_godot_GodotLib_getRendererInfo(JNIEnv *env, jclass clazz, jboolean p_vulkan_requirements_met) {
	String rendering_driver_original = Renderer::get_current_rendering_driver_name();
	String rendering_driver_chosen = rendering_driver_original;
	String rendering_method = Renderer::get_current_rendering_method();

#ifdef VULKAN_ENABLED
	if (rendering_driver_original == "vulkan" && !DisplayServerAndroid::check_vulkan_global_context(p_vulkan_requirements_met)) {
		// The Android display server only gets created after this step, so a static check must be used to check for Vulkan
		// availability instead. If the check fails, it'll fall back to OpenGL3 accordingly if the relevant project setting
		// is enabled. The Vulkan context created by this check will be reused by the DisplayServer afterwards.
		rendering_driver_chosen = Renderer::get_current_rendering_driver_name();
		rendering_method = Renderer::get_current_rendering_method();
	}
#ifndef XR_DISABLED
	// When running in XR mode, vulkan initialization must be done by the XR module, so we ensure that the vulkan
	// global context is reset.
	// Note: This is temporary workaround to address https://github.com/godotengine/godot/issues/115924
	// A proper fix involves updating the Android init flow so that DisplayServerAndroid can update the Android surface
	// type (vulkan or opengl) after it's initialized.
	bool xr_enabled = false;
	if (XRServer::get_xr_mode() == XRServer::XRMODE_DEFAULT) {
		xr_enabled = GLOBAL_GET_CACHED(bool, "xr/shaders/enabled");
	} else {
		xr_enabled = XRServer::get_xr_mode() == XRServer::XRMODE_ON;
	}
	if (xr_enabled) {
		DisplayServerAndroid::free_vulkan_global_context();
	}
#endif // XR_DISABLED
#endif

	String rendering_driver_source = rendering_source_to_string(OS::get_current_rendering_driver_name_source());
	String rendering_method_source = rendering_source_to_string(OS::get_current_rendering_method_source());

	jobjectArray result = env->NewObjectArray(5, jni_find_class(env, "java/lang/String"), nullptr);
	env->SetObjectArrayElement(result, 0, env->NewStringUTF(rendering_driver_chosen.utf8().get_data()));
	env->SetObjectArrayElement(result, 1, env->NewStringUTF(rendering_driver_original.utf8().get_data()));
	env->SetObjectArrayElement(result, 2, env->NewStringUTF(rendering_method.utf8().get_data()));
	env->SetObjectArrayElement(result, 3, env->NewStringUTF(rendering_driver_source.utf8().get_data()));
	env->SetObjectArrayElement(result, 4, env->NewStringUTF(rendering_method_source.utf8().get_data()));

	return result;
}

JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getEditorSetting(JNIEnv *env, jclass clazz, jstring p_setting_key) {
	String editor_setting_value = "";
#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String godot_setting_key = jstring_to_string(p_setting_key, env);
		Variant editor_setting = EDITOR_GET(godot_setting_key);
		editor_setting_value = (editor_setting.get_type() == Variant::NIL) ? "" : editor_setting;
	}
#else
	WARN_PRINT("Access to the Editor Settings in only available on Editor builds");
#endif

	return env->NewStringUTF(editor_setting_value.utf8().get_data());
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorSetting(JNIEnv *env, jclass clazz, jstring p_key, jobject p_data) {
#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String key = jstring_to_string(p_key, env);
		Variant data = _jobject_to_variant(env, p_data);
		(void)key;
		(void)data;
	}
#else
	WARN_PRINT("Access to the Editor Settings in only available on Editor builds");
#endif
}

JNIEXPORT jobject JNICALL Java_org_godotengine_godot_GodotLib_getEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_default_value) {
	jvalue result;

#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String section = jstring_to_string(p_section, env);
		String key = jstring_to_string(p_key, env);
		Variant data = _jobject_to_variant(env, p_default_value);
		(void)section;
		(void)key;
		result.l = _variant_to_jobject(env, data.get_type(), &data);
	}
#else
	WARN_PRINT("Access to the Editor Settings Project Metadata is only available on Editor builds");
#endif

	return result.l;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_setEditorProjectMetadata(JNIEnv *env, jclass clazz, jstring p_section, jstring p_key, jobject p_data) {
#ifdef TOOLS_ENABLED
	if (EditorSettings::is_initialized()) {
		String section = jstring_to_string(p_section, env);
		String key = jstring_to_string(p_key, env);
		Variant data = _jobject_to_variant(env, p_data);
		(void)section;
		(void)key;
		(void)data;
	}
#else
	WARN_PRINT("Access to the Editor Settings Project Metadata is only available on Editor builds");
#endif
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onNightModeChanged(JNIEnv *env, jclass clazz) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		ds->emit_system_theme_changed();
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_hardwareKeyboardConnected(JNIEnv *env, jclass clazz, jboolean p_connected) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		ds->emit_hardware_keyboard_connection_changed(p_connected);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_filePickerCallback(JNIEnv *env, jclass clazz, jboolean p_ok, jobjectArray p_selected_paths) {
	DisplayServerAndroid *ds = DisplayServer::driver_as<DisplayServerAndroid>();
	if (ds) {
		Vector<String> selected_paths;

		jint length = env->GetArrayLength(p_selected_paths);
		for (jint i = 0; i < length; ++i) {
			jstring java_string = (jstring)env->GetObjectArrayElement(p_selected_paths, i);
			String path = jstring_to_string(java_string, env);
			selected_paths.push_back(path);
			env->DeleteLocalRef(java_string);
		}
		ds->emit_file_picker_callback(p_ok, selected_paths);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_requestPermissionResult(JNIEnv *env, jclass clazz, jstring p_permission, jboolean p_result) {
	String permission = jstring_to_string(p_permission, env);
	if (permission == "android.permission.RECORD_AUDIO" && p_result) {
		AudioDriverManager::get_active_driver()->input_start();
	}

}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererResumed(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	// We force redraw to ensure we render at least once when resuming the app.
	Main::force_redraw();
	if (CameraServer::is_initialized()) {
		CameraServer::handle_application_resume();
	}
	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_RESUMED);
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onRendererPaused(JNIEnv *env, jclass clazz) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (CameraServer::is_initialized()) {
		CameraServer::handle_application_pause();
	}

	if (MainLoop::is_active()) {
		MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_PAUSED);
	}

	if (DisplayServerAndroid *dsa = DisplayServer::driver_as<DisplayServerAndroid>()) {
		dsa->notify_application_paused();
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onOrientationChange(JNIEnv *env, jclass clazz, jint p_orientation) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (CameraServer::is_initialized()) {
		CameraServer::handle_display_rotation_change(p_orientation);
	}
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_shouldDispatchInputToRenderThread(JNIEnv *env, jclass clazz) {
	if (Input::is_initialized()) {
		return !Input::is_agile_input_event_flushing();
	}
	return false;
}

JNIEXPORT jstring JNICALL Java_org_godotengine_godot_GodotLib_getProjectResourceDir(JNIEnv *env, jclass clazz) {
	const String resource_dir = OS::get_resource_dir();
	return env->NewStringUTF(resource_dir.utf8().get_data());
}
JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isEditorHint(JNIEnv *env, jclass clazz) {
	return Engine::is_editor_hint();
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_isProjectManagerHint(JNIEnv *env, jclass clazz) {
	return Engine::is_project_manager_hint();
}

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_GodotLib_hasFeature(JNIEnv *env, jclass clazz, jstring p_feature) {
	if (OS::is_initialized()) {
		String feature = jstring_to_string(p_feature, env);
		return OS::has_feature(feature);
	}
	return false;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_GodotLib_onPictureInPictureModeChanged(JNIEnv *env, jclass clazz, jboolean p_is_in_picture_in_picture_mode) {
	if (step.get() <= STEP_SETUP) {
		return;
	}

	if (MainLoop::is_active()) {
		if (p_is_in_picture_in_picture_mode) {
			MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_ENTERED);
		} else {
			MainLoop::notification(MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_EXITED);
		}
	}
}
}


// --- Merged from platform/android/java_godot_wrapper.cpp ---
#include "platform/android/os_android.h"

#include "platform/android/os_android.h"

// JNIEnv is only valid within the thread it belongs to, in a multi threading environment
// we can't cache it.
// For Godot we call most access methods from our thread and we thus get a valid JNIEnv
// from get_jni_env(). For one or two we expect to pass the environment

// TODO we could probably create a base class for this...

GodotJavaWrapper::GodotJavaWrapper(JNIEnv *p_env, jobject p_godot_native_bridge) {
	godot_native_bridge = p_env->NewGlobalRef(p_godot_native_bridge);

	// get info about our Godot class so we can get pointers and stuff...
	godot_native_bridge_class = jni_find_class(p_env, "org/godotengine/godot/nativeapi/GodotNativeBridge");
	if (godot_native_bridge_class) {
		godot_native_bridge_class = (jclass)p_env->NewGlobalRef(godot_native_bridge_class);
	} else {
		// this is a pretty serious fail.. bail... pointers will stay 0
		return;
	}

	// get some Godot method pointers...
	_restart = p_env->GetMethodID(godot_native_bridge_class, "restart", "()V");
	_finish = p_env->GetMethodID(godot_native_bridge_class, "forceQuit", "(I)Z");
	_set_keep_screen_on = p_env->GetMethodID(godot_native_bridge_class, "setKeepScreenOn", "(Z)V");
	_alert = p_env->GetMethodID(godot_native_bridge_class, "alert", "(Ljava/lang/String;Ljava/lang/String;)V");
	_is_dark_mode_supported = p_env->GetMethodID(godot_native_bridge_class, "isDarkModeSupported", "()Z");
	_is_dark_mode = p_env->GetMethodID(godot_native_bridge_class, "isDarkMode", "()Z");
	_get_accent_color = p_env->GetMethodID(godot_native_bridge_class, "getAccentColor", "()I");
	_get_base_color = p_env->GetMethodID(godot_native_bridge_class, "getBaseColor", "()I");
	_get_clipboard = p_env->GetMethodID(godot_native_bridge_class, "getClipboard", "()Ljava/lang/String;");
	_set_clipboard = p_env->GetMethodID(godot_native_bridge_class, "setClipboard", "(Ljava/lang/String;)V");
	_has_clipboard = p_env->GetMethodID(godot_native_bridge_class, "hasClipboard", "()Z");
	_show_dialog = p_env->GetMethodID(godot_native_bridge_class, "showDialog", "(Ljava/lang/String;Ljava/lang/String;[Ljava/lang/String;)V");
	_show_input_dialog = p_env->GetMethodID(godot_native_bridge_class, "showInputDialog", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
	_show_file_picker = p_env->GetMethodID(godot_native_bridge_class, "showFilePicker", "(Ljava/lang/String;Ljava/lang/String;I[Ljava/lang/String;)V");
	_request_permission = p_env->GetMethodID(godot_native_bridge_class, "requestPermission", "(Ljava/lang/String;)Z");
	_request_permissions = p_env->GetMethodID(godot_native_bridge_class, "requestPermissions", "()Z");
	_get_granted_permissions = p_env->GetMethodID(godot_native_bridge_class, "getGrantedPermissions", "()[Ljava/lang/String;");
	_get_ca_certificates = p_env->GetMethodID(godot_native_bridge_class, "getCACertificates", "()Ljava/lang/String;");
	_init_input_devices = p_env->GetMethodID(godot_native_bridge_class, "initInputDevices", "()V");
	_vibrate = p_env->GetMethodID(godot_native_bridge_class, "vibrate", "(II)V");
	_get_input_fallback_mapping = p_env->GetMethodID(godot_native_bridge_class, "getInputFallbackMapping", "()Ljava/lang/String;");
	_on_godot_setup_completed = p_env->GetMethodID(godot_native_bridge_class, "onGodotSetupCompleted", "()V");
	_on_godot_main_loop_started = p_env->GetMethodID(godot_native_bridge_class, "onGodotMainLoopStarted", "()V");
	_on_godot_terminating = p_env->GetMethodID(godot_native_bridge_class, "onGodotTerminating", "()V");
	_create_new_godot_instance = p_env->GetMethodID(godot_native_bridge_class, "createNewGodotInstance", "([Ljava/lang/String;)I");
	_get_render_view = p_env->GetMethodID(godot_native_bridge_class, "getRenderView", "()Lorg/godotengine/godot/GodotRenderView;");
	_begin_benchmark_measure = p_env->GetMethodID(godot_native_bridge_class, "nativeBeginBenchmarkMeasure", "(Ljava/lang/String;Ljava/lang/String;)V");
	_end_benchmark_measure = p_env->GetMethodID(godot_native_bridge_class, "nativeEndBenchmarkMeasure", "(Ljava/lang/String;Ljava/lang/String;)V");
	_dump_benchmark = p_env->GetMethodID(godot_native_bridge_class, "nativeDumpBenchmark", "(Ljava/lang/String;)V");
	_get_gdextension_list_config_file = p_env->GetMethodID(godot_native_bridge_class, "getGDExtensionConfigFiles", "()[Ljava/lang/String;");
	_check_internal_feature_support = p_env->GetMethodID(godot_native_bridge_class, "checkInternalFeatureSupport", "(Ljava/lang/String;)Z");
	_sign_apk = p_env->GetMethodID(godot_native_bridge_class, "nativeSignApk", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I");
	_verify_apk = p_env->GetMethodID(godot_native_bridge_class, "nativeVerifyApk", "(Ljava/lang/String;)I");
	_enable_immersive_mode = p_env->GetMethodID(godot_native_bridge_class, "nativeEnableImmersiveMode", "(Z)V");
	_is_in_immersive_mode = p_env->GetMethodID(godot_native_bridge_class, "isInImmersiveMode", "()Z");
	_set_window_color = p_env->GetMethodID(godot_native_bridge_class, "setWindowColor", "(Ljava/lang/String;)V");
	_on_editor_workspace_selected = p_env->GetMethodID(godot_native_bridge_class, "nativeOnEditorWorkspaceSelected", "(Ljava/lang/String;)V");
	_on_distraction_free_mode_changed = p_env->GetMethodID(godot_native_bridge_class, "nativeOnDistractionFreeModeChanged", "(Z)V");
	_get_activity = p_env->GetMethodID(godot_native_bridge_class, "getActivity", "()Landroid/app/Activity;");
	_build_env_connect = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvConnect", "(Lorg/godotengine/godot/variant/Callable;)Z");
	_build_env_disconnect = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvDisconnect", "()V");
	_build_env_execute = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvExecute", "(Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lorg/godotengine/godot/variant/Callable;Lorg/godotengine/godot/variant/Callable;)I");
	_build_env_cancel = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvCancel", "(I)V");
	_build_env_clean_project = p_env->GetMethodID(godot_native_bridge_class, "nativeBuildEnvCleanProject", "(Ljava/lang/String;Ljava/lang/String;Lorg/godotengine/godot/variant/Callable;)V");

	// PiP mode method ids.
	_is_pip_mode_supported = p_env->GetMethodID(godot_native_bridge_class, "nativeIsPiPModeSupported", "()Z");
	_is_in_pip_mode = p_env->GetMethodID(godot_native_bridge_class, "nativeIsInPiPMode", "()Z");
	_enter_pip_mode = p_env->GetMethodID(godot_native_bridge_class, "nativeEnterPiPMode", "()V");
	_set_pip_mode_aspect_ratio = p_env->GetMethodID(godot_native_bridge_class, "nativeSetPiPModeAspectRatio", "(II)V");
	_set_auto_enter_pip_mode_on_background = p_env->GetMethodID(godot_native_bridge_class, "nativeSetAutoEnterPiPModeOnBackground", "(Z)V");
}

GodotJavaWrapper::~GodotJavaWrapper() {
	if (godot_view) {
		delete godot_view;
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);
	env->DeleteGlobalRef(godot_native_bridge);
	env->DeleteGlobalRef(godot_native_bridge_class);
}

jobject GodotJavaWrapper::get_activity() {
	if (_get_activity) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, nullptr);
		jobject activity = env->CallObjectMethod(godot_native_bridge, _get_activity);
		return activity;
	}
	return nullptr;
}

GodotJavaViewWrapper *GodotJavaWrapper::get_godot_view() {
	if (godot_view != nullptr) {
		return godot_view;
	}
	if (_get_render_view) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, nullptr);
		jobject godot_render_view = env->CallObjectMethod(godot_native_bridge, _get_render_view);
		if (!env->IsSameObject(godot_render_view, nullptr)) {
			godot_view = new GodotJavaViewWrapper(godot_render_view);
		}
	}
	return godot_view;
}

void GodotJavaWrapper::on_godot_setup_completed(JNIEnv *p_env) {
	if (_on_godot_setup_completed) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		p_env->CallVoidMethod(godot_native_bridge, _on_godot_setup_completed);
	}
}

void GodotJavaWrapper::on_godot_main_loop_started(JNIEnv *p_env) {
	if (_on_godot_main_loop_started) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL(p_env);
		p_env->CallVoidMethod(godot_native_bridge, _on_godot_main_loop_started);
	}
}

void GodotJavaWrapper::on_godot_terminating(JNIEnv *p_env) {
	if (_on_godot_terminating) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL(p_env);
		p_env->CallVoidMethod(godot_native_bridge, _on_godot_terminating);
	}
}

void GodotJavaWrapper::restart(JNIEnv *p_env) {
	if (_restart) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL(p_env);
		p_env->CallVoidMethod(godot_native_bridge, _restart);
	}
}

bool GodotJavaWrapper::force_quit(JNIEnv *p_env, int p_instance_id) {
	if (_finish) {
		if (p_env == nullptr) {
			p_env = get_jni_env();
		}
		ERR_FAIL_NULL_V(p_env, false);
		return p_env->CallBooleanMethod(godot_native_bridge, _finish, p_instance_id);
	}
	return false;
}

void GodotJavaWrapper::set_keep_screen_on(bool p_enabled) {
	if (_set_keep_screen_on) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _set_keep_screen_on, p_enabled);
	}
}

void GodotJavaWrapper::alert(const String &p_message, const String &p_title) {
	if (_alert) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring jStrMessage = env->NewStringUTF(p_message.utf8().get_data());
		jstring jStrTitle = env->NewStringUTF(p_title.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _alert, jStrMessage, jStrTitle);
		env->DeleteLocalRef(jStrMessage);
		env->DeleteLocalRef(jStrTitle);
	}
}

bool GodotJavaWrapper::is_dark_mode_supported() {
	if (_is_dark_mode_supported) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_dark_mode_supported);
	} else {
		return false;
	}
}

bool GodotJavaWrapper::is_dark_mode() {
	if (_is_dark_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_dark_mode);
	} else {
		return false;
	}
}

// Convert ARGB to RGBA.
static Color _argb_to_rgba(int p_color) {
	int alpha = (p_color >> 24) & 0xFF;
	int red = (p_color >> 16) & 0xFF;
	int green = (p_color >> 8) & 0xFF;
	int blue = p_color & 0xFF;
	return Color(red / 255.0f, green / 255.0f, blue / 255.0f, alpha / 255.0f);
}

Color GodotJavaWrapper::get_accent_color() {
	if (_get_accent_color) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, Color(0, 0, 0, 0));
		int accent_color = env->CallIntMethod(godot_native_bridge, _get_accent_color);
		return _argb_to_rgba(accent_color);
	} else {
		return Color(0, 0, 0, 0);
	}
}

Color GodotJavaWrapper::get_base_color() {
	if (_get_base_color) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, Color(0, 0, 0, 0));
		int base_color = env->CallIntMethod(godot_native_bridge, _get_base_color);
		return _argb_to_rgba(base_color);
	} else {
		return Color(0, 0, 0, 0);
	}
}

bool GodotJavaWrapper::has_get_clipboard() {
	return _get_clipboard != nullptr;
}

String GodotJavaWrapper::get_clipboard() {
	String clipboard;
	if (_get_clipboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_native_bridge, _get_clipboard);
		clipboard = jstring_to_string(s, env);
		env->DeleteLocalRef(s);
	}
	return clipboard;
}

String GodotJavaWrapper::get_input_fallback_mapping() {
	String input_fallback_mapping;
	if (_get_input_fallback_mapping) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring fallback_mapping = (jstring)env->CallObjectMethod(godot_native_bridge, _get_input_fallback_mapping);
		input_fallback_mapping = jstring_to_string(fallback_mapping, env);
		env->DeleteLocalRef(fallback_mapping);
	}
	return input_fallback_mapping;
}

bool GodotJavaWrapper::has_set_clipboard() {
	return _set_clipboard != nullptr;
}

void GodotJavaWrapper::set_clipboard(const String &p_text) {
	if (_set_clipboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring jStr = env->NewStringUTF(p_text.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _set_clipboard, jStr);
		env->DeleteLocalRef(jStr);
	}
}

bool GodotJavaWrapper::has_has_clipboard() {
	return _has_clipboard != nullptr;
}

bool GodotJavaWrapper::has_clipboard() {
	if (_has_clipboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _has_clipboard);
	} else {
		return false;
	}
}

Error GodotJavaWrapper::show_dialog(const String &p_title, const String &p_description, const Vector<String> &p_buttons) {
	if (_show_input_dialog) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);
		jstring j_title = env->NewStringUTF(p_title.utf8().get_data());
		jstring j_description = env->NewStringUTF(p_description.utf8().get_data());
		jobjectArray j_buttons = env->NewObjectArray(p_buttons.size(), jni_find_class(env, "java/lang/String"), nullptr);
		for (int i = 0; i < p_buttons.size(); ++i) {
			jstring j_button = env->NewStringUTF(p_buttons[i].utf8().get_data());
			env->SetObjectArrayElement(j_buttons, i, j_button);
			env->DeleteLocalRef(j_button);
		}
		env->CallVoidMethod(godot_native_bridge, _show_dialog, j_title, j_description, j_buttons);
		env->DeleteLocalRef(j_title);
		env->DeleteLocalRef(j_description);
		env->DeleteLocalRef(j_buttons);
		return OK;
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error GodotJavaWrapper::show_input_dialog(const String &p_title, const String &p_message, const String &p_existing_text) {
	if (_show_input_dialog) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);
		jstring jStrTitle = env->NewStringUTF(p_title.utf8().get_data());
		jstring jStrMessage = env->NewStringUTF(p_message.utf8().get_data());
		jstring jStrExistingText = env->NewStringUTF(p_existing_text.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _show_input_dialog, jStrTitle, jStrMessage, jStrExistingText);
		env->DeleteLocalRef(jStrTitle);
		env->DeleteLocalRef(jStrMessage);
		env->DeleteLocalRef(jStrExistingText);
		return OK;
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error GodotJavaWrapper::show_file_picker(const String &p_current_directory, const String &p_filename, int p_mode, const Vector<String> &p_filters) {
	if (_show_file_picker) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);
		jstring j_current_directory = env->NewStringUTF(p_current_directory.utf8().get_data());
		jstring j_filename = env->NewStringUTF(p_filename.utf8().get_data());
		jint j_mode = p_mode;
		Vector<String> filters;
		for (const String &E : p_filters) {
			filters.append_array(E.get_slicec(';', 0).split(",")); // Add extensions.
			filters.append_array(E.get_slicec(';', 2).split(",")); // Add MIME types.
		}
		jobjectArray j_filters = env->NewObjectArray(filters.size(), jni_find_class(env, "java/lang/String"), nullptr);
		for (int i = 0; i < filters.size(); ++i) {
			jstring j_filter = env->NewStringUTF(filters[i].utf8().get_data());
			env->SetObjectArrayElement(j_filters, i, j_filter);
			env->DeleteLocalRef(j_filter);
		}
		env->CallVoidMethod(godot_native_bridge, _show_file_picker, j_current_directory, j_filename, j_mode, j_filters);
		env->DeleteLocalRef(j_current_directory);
		env->DeleteLocalRef(j_filename);
		env->DeleteLocalRef(j_filters);
		return OK;
	} else {
		return ERR_UNCONFIGURED;
	}
}

bool GodotJavaWrapper::request_permission(const String &p_name) {
	if (_request_permission) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		jstring jStrName = env->NewStringUTF(p_name.utf8().get_data());
		bool result = env->CallBooleanMethod(godot_native_bridge, _request_permission, jStrName);
		env->DeleteLocalRef(jStrName);
		return result;
	} else {
		return false;
	}
}

bool GodotJavaWrapper::request_permissions() {
	if (_request_permissions) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _request_permissions);
	} else {
		return false;
	}
}

Vector<String> GodotJavaWrapper::get_granted_permissions() const {
	Vector<String> permissions_list;
	if (_get_granted_permissions) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, permissions_list);
		jobject permissions_object = env->CallObjectMethod(godot_native_bridge, _get_granted_permissions);
		jobjectArray *arr = reinterpret_cast<jobjectArray *>(&permissions_object);

		jsize len = env->GetArrayLength(*arr);
		for (int i = 0; i < len; i++) {
			jstring jstr = (jstring)env->GetObjectArrayElement(*arr, i);
			String str = jstring_to_string(jstr, env);
			permissions_list.push_back(str);
			env->DeleteLocalRef(jstr);
		}
	}
	return permissions_list;
}

Vector<String> GodotJavaWrapper::get_gdextension_list_config_file() const {
	Vector<String> config_file_list;
	if (_get_gdextension_list_config_file) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, config_file_list);
		jobject config_file_list_object = env->CallObjectMethod(godot_native_bridge, _get_gdextension_list_config_file);
		jobjectArray *arr = reinterpret_cast<jobjectArray *>(&config_file_list_object);

		jsize len = env->GetArrayLength(*arr);
		for (int i = 0; i < len; i++) {
			jstring j_config_file = (jstring)env->GetObjectArrayElement(*arr, i);
			String config_file = jstring_to_string(j_config_file, env);
			config_file_list.push_back(config_file);
			env->DeleteLocalRef(j_config_file);
		}
	}
	return config_file_list;
}

String GodotJavaWrapper::get_ca_certificates() const {
	String ca_certificates;
	if (_get_ca_certificates) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_native_bridge, _get_ca_certificates);
		ca_certificates = jstring_to_string(s, env);
		env->DeleteLocalRef(s);
	}
	return ca_certificates;
}

void GodotJavaWrapper::init_input_devices() {
	if (_init_input_devices) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _init_input_devices);
	}
}

void GodotJavaWrapper::vibrate(int p_duration_ms, float p_amplitude) {
	if (_vibrate) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		int j_amplitude = -1.0;

		if (p_amplitude != -1.0) {
			j_amplitude = CLAMP(int(p_amplitude * 255), 1, 255);
		}

		env->CallVoidMethod(godot_native_bridge, _vibrate, p_duration_ms, j_amplitude);
	}
}

int GodotJavaWrapper::create_new_godot_instance(const List<String> &args) {
	if (_create_new_godot_instance) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		jobjectArray jargs = env->NewObjectArray(args.size(), jni_find_class(env, "java/lang/String"), env->NewStringUTF(""));
		int i = 0;
		for (List<String>::ConstIterator itr = args.begin(); itr != args.end(); ++itr, ++i) {
			jstring j_arg = env->NewStringUTF(itr->utf8().get_data());
			env->SetObjectArrayElement(jargs, i, j_arg);
			env->DeleteLocalRef(j_arg);
		}
		return env->CallIntMethod(godot_native_bridge, _create_new_godot_instance, jargs);
	} else {
		return 0;
	}
}

void GodotJavaWrapper::begin_benchmark_measure(const String &p_context, const String &p_label) {
	if (_begin_benchmark_measure) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring j_context = env->NewStringUTF(p_context.utf8().get_data());
		jstring j_label = env->NewStringUTF(p_label.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _begin_benchmark_measure, j_context, j_label);
		env->DeleteLocalRef(j_context);
		env->DeleteLocalRef(j_label);
	}
}

void GodotJavaWrapper::end_benchmark_measure(const String &p_context, const String &p_label) {
	if (_end_benchmark_measure) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring j_context = env->NewStringUTF(p_context.utf8().get_data());
		jstring j_label = env->NewStringUTF(p_label.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _end_benchmark_measure, j_context, j_label);
		env->DeleteLocalRef(j_context);
		env->DeleteLocalRef(j_label);
	}
}

void GodotJavaWrapper::dump_benchmark(const String &benchmark_file) {
	if (_dump_benchmark) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring j_benchmark_file = env->NewStringUTF(benchmark_file.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _dump_benchmark, j_benchmark_file);
		env->DeleteLocalRef(j_benchmark_file);
	}
}

bool GodotJavaWrapper::check_internal_feature_support(const String &p_feature) const {
	if (_check_internal_feature_support) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		jstring j_feature = env->NewStringUTF(p_feature.utf8().get_data());
		bool result = env->CallBooleanMethod(godot_native_bridge, _check_internal_feature_support, j_feature);
		env->DeleteLocalRef(j_feature);
		return result;
	} else {
		return false;
	}
}

Error GodotJavaWrapper::sign_apk(const String &p_input_path, const String &p_output_path, const String &p_keystore_path, const String &p_keystore_user, const String &p_keystore_password) {
	if (_sign_apk) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		jstring j_input_path = env->NewStringUTF(p_input_path.utf8().get_data());
		jstring j_output_path = env->NewStringUTF(p_output_path.utf8().get_data());
		jstring j_keystore_path = env->NewStringUTF(p_keystore_path.utf8().get_data());
		jstring j_keystore_user = env->NewStringUTF(p_keystore_user.utf8().get_data());
		jstring j_keystore_password = env->NewStringUTF(p_keystore_password.utf8().get_data());

		int result = env->CallIntMethod(godot_native_bridge, _sign_apk, j_input_path, j_output_path, j_keystore_path, j_keystore_user, j_keystore_password);

		env->DeleteLocalRef(j_input_path);
		env->DeleteLocalRef(j_output_path);
		env->DeleteLocalRef(j_keystore_path);
		env->DeleteLocalRef(j_keystore_user);
		env->DeleteLocalRef(j_keystore_password);

		return static_cast<Error>(result);
	} else {
		return ERR_UNCONFIGURED;
	}
}

Error GodotJavaWrapper::verify_apk(const String &p_apk_path) {
	if (_verify_apk) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNCONFIGURED);

		jstring j_apk_path = env->NewStringUTF(p_apk_path.utf8().get_data());
		int result = env->CallIntMethod(godot_native_bridge, _verify_apk, j_apk_path);
		env->DeleteLocalRef(j_apk_path);
		return static_cast<Error>(result);
	} else {
		return ERR_UNCONFIGURED;
	}
}

void GodotJavaWrapper::enable_immersive_mode(bool p_enabled) {
	if (_enable_immersive_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _enable_immersive_mode, p_enabled);
	}
}

bool GodotJavaWrapper::is_in_immersive_mode() {
	if (_is_in_immersive_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_in_immersive_mode);
	} else {
		return false;
	}
}

void GodotJavaWrapper::set_window_color(const Color &p_color) {
	if (_set_window_color) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		String color = "#" + p_color.to_html(false);
		jstring jStrColor = env->NewStringUTF(color.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _set_window_color, jStrColor);
	}
}

void GodotJavaWrapper::on_editor_workspace_selected(const String &p_workspace) {
	if (_on_editor_workspace_selected) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		jstring j_workspace = env->NewStringUTF(p_workspace.utf8().get_data());
		env->CallVoidMethod(godot_native_bridge, _on_editor_workspace_selected, j_workspace);
	}
}

void GodotJavaWrapper::on_distraction_free_mode_changed(bool p_enabled) {
	if (_on_distraction_free_mode_changed) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(godot_native_bridge, _on_distraction_free_mode_changed, p_enabled);
	}
}

bool GodotJavaWrapper::build_env_connect(const Callable &p_callback) {
	if (_build_env_connect) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		jobject j_callback = callable_to_jcallable(env, p_callback);
		jboolean result = env->CallBooleanMethod(godot_native_bridge, _build_env_connect, j_callback);
		env->DeleteLocalRef(j_callback);

		return result;
	}

	return false;
}

void GodotJavaWrapper::build_env_disconnect() {
	if (_build_env_disconnect) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(godot_native_bridge, _build_env_disconnect);
	}
}

int GodotJavaWrapper::build_env_execute(const String &p_build_tool, const List<String> &p_arguments, const String &p_project_path, const String &p_gradle_build_directory, const Callable &p_output_callback, const Callable &p_result_callback) {
	if (_build_env_execute) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, -1);

		jstring j_build_tool = env->NewStringUTF(p_build_tool.utf8().get_data());
		jobjectArray j_args = env->NewObjectArray(p_arguments.size(), env->FindClass("java/lang/String"), nullptr);
		for (int i = 0; i < p_arguments.size(); i++) {
			jstring j_arg = env->NewStringUTF(p_arguments.get(i).utf8().get_data());
			env->SetObjectArrayElement(j_args, i, j_arg);
			env->DeleteLocalRef(j_arg);
		}
		jstring j_project_path = env->NewStringUTF(p_project_path.utf8().get_data());
		jstring j_gradle_build_directory = env->NewStringUTF(p_gradle_build_directory.utf8().get_data());
		jobject j_output_callback = callable_to_jcallable(env, p_output_callback);
		jobject j_result_callback = callable_to_jcallable(env, p_result_callback);

		jint result = env->CallIntMethod(godot_native_bridge, _build_env_execute, j_build_tool, j_args, j_project_path, j_gradle_build_directory, j_output_callback, j_result_callback);

		env->DeleteLocalRef(j_build_tool);
		env->DeleteLocalRef(j_args);
		env->DeleteLocalRef(j_project_path);
		env->DeleteLocalRef(j_gradle_build_directory);
		env->DeleteLocalRef(j_output_callback);
		env->DeleteLocalRef(j_result_callback);

		return result;
	}

	return -1;
}

void GodotJavaWrapper::build_env_cancel(int p_job_id) {
	if (_build_env_cancel) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _build_env_cancel, p_job_id);
	}
}

void GodotJavaWrapper::build_env_clean_project(const String &p_project_path, const String &p_gradle_build_directory, const Callable &p_callback) {
	if (_build_env_clean_project) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		jstring j_project_path = env->NewStringUTF(p_project_path.utf8().get_data());
		jstring j_gradle_build_directory = env->NewStringUTF(p_gradle_build_directory.utf8().get_data());
		jobject j_callback = callable_to_jcallable(env, p_callback);

		env->CallVoidMethod(godot_native_bridge, _build_env_clean_project, j_project_path, j_gradle_build_directory, j_callback);

		env->DeleteLocalRef(j_project_path);
		env->DeleteLocalRef(j_gradle_build_directory);
		env->DeleteLocalRef(j_callback);
	}
}

bool GodotJavaWrapper::is_pip_mode_supported() {
	if (_is_pip_mode_supported) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_pip_mode_supported);
	} else {
		return false;
	}
}

bool GodotJavaWrapper::is_in_pip_mode() {
	if (_is_in_pip_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_native_bridge, _is_in_pip_mode);
	} else {
		return false;
	}
}

void GodotJavaWrapper::enter_pip_mode() {
	if (_enter_pip_mode) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _enter_pip_mode);
	}
}

void GodotJavaWrapper::set_pip_mode_aspect_ratio(int p_numerator, int p_denominator) {
	if (_set_pip_mode_aspect_ratio) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _set_pip_mode_aspect_ratio, p_numerator, p_denominator);
	}
}

void GodotJavaWrapper::set_auto_enter_pip_mode_on_background(bool p_auto_enter_on_background) {
	if (_set_auto_enter_pip_mode_on_background) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_native_bridge, _set_auto_enter_pip_mode_on_background, p_auto_enter_on_background);
	}
}


// --- Merged from platform/android/java_godot_view_wrapper.cpp ---
#include "platform/android/os_android.h"

#include "platform/android/os_android.h"

#include "core/string/ustring.h"

GodotJavaViewWrapper::GodotJavaViewWrapper(jobject godot_view) {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	_godot_view = env->NewGlobalRef(godot_view);

	_cls = (jclass)env->NewGlobalRef(env->GetObjectClass(godot_view));

	_configure_pointer_icon = env->GetMethodID(_cls, "configurePointerIcon", "(ILjava/lang/String;FF)V");
	_set_pointer_icon = env->GetMethodID(_cls, "setPointerIcon", "(I)V");

	int android_device_api_level = android_get_device_api_level();
	if (android_device_api_level >= __ANDROID_API_O__) {
		_request_pointer_capture = env->GetMethodID(_cls, "requestPointerCapture", "()V");
		_release_pointer_capture = env->GetMethodID(_cls, "releasePointerCapture", "()V");
	}

	_can_capture_pointer = env->GetMethodID(_cls, "canCapturePointer", "()Z");
}

bool GodotJavaViewWrapper::can_update_pointer_icon() const {
	return _configure_pointer_icon != nullptr && _set_pointer_icon != nullptr;
}

bool GodotJavaViewWrapper::can_capture_pointer() const {
	// We can capture the pointer if the other jni capture method ids are initialized,
	// and GodotView#canCapturePointer() returns true.
	if (_request_pointer_capture != nullptr && _release_pointer_capture != nullptr && _can_capture_pointer != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);

		return env->CallBooleanMethod(_godot_view, _can_capture_pointer);
	}

	return false;
}

void GodotJavaViewWrapper::request_pointer_capture() {
	if (_request_pointer_capture != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(_godot_view, _request_pointer_capture);
	}
}

void GodotJavaViewWrapper::release_pointer_capture() {
	if (_release_pointer_capture != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(_godot_view, _release_pointer_capture);
	}
}

void GodotJavaViewWrapper::configure_pointer_icon(int pointer_type, const String &image_path, const Vector2 &p_hotspot) {
	if (_configure_pointer_icon != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		jstring jImagePath = env->NewStringUTF(image_path.utf8().get_data());
		env->CallVoidMethod(_godot_view, _configure_pointer_icon, pointer_type, jImagePath, p_hotspot.x, p_hotspot.y);
		env->DeleteLocalRef(jImagePath);
	}
}

void GodotJavaViewWrapper::set_pointer_icon(int pointer_type) {
	if (_set_pointer_icon != nullptr) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);

		env->CallVoidMethod(_godot_view, _set_pointer_icon, pointer_type);
	}
}

GodotJavaViewWrapper::~GodotJavaViewWrapper() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(_godot_view);
	env->DeleteGlobalRef(_cls);
}


// --- Merged from platform/android/java_godot_io_wrapper.cpp ---
#include "platform/android/os_android.h"

#include "platform/android/os_android.h"
#include "platform/android/os_android.h"

#include "core/math/rect2.h"

// JNIEnv is only valid within the thread it belongs to, in a multi threading environment
// we can't cache it.
// For GodotIO we call all access methods from our thread and we thus get a valid JNIEnv
// from get_jni_env().

GodotIOJavaWrapper::GodotIOJavaWrapper(JNIEnv *p_env, jobject p_godot_io_instance) {
	godot_io_instance = p_env->NewGlobalRef(p_godot_io_instance);
	if (godot_io_instance) {
		cls = p_env->GetObjectClass(godot_io_instance);
		if (cls) {
			cls = (jclass)p_env->NewGlobalRef(cls);
		} else {
			// this is a pretty serious fail.. bail... pointers will stay 0
			return;
		}

		_open_URI = p_env->GetMethodID(cls, "openURI", "(Ljava/lang/String;)I");
		_get_cache_dir = p_env->GetMethodID(cls, "getCacheDir", "()Ljava/lang/String;");
		_get_temp_dir = p_env->GetMethodID(cls, "getTempDir", "()Ljava/lang/String;");
		_get_data_dir = p_env->GetMethodID(cls, "getDataDir", "()Ljava/lang/String;");
		_get_display_cutouts = p_env->GetMethodID(cls, "getDisplayCutouts", "()[I"),
		_get_display_safe_area = p_env->GetMethodID(cls, "getDisplaySafeArea", "()[I"),
		_get_locale = p_env->GetMethodID(cls, "getLocale", "()Ljava/lang/String;");
		_get_model = p_env->GetMethodID(cls, "getModel", "()Ljava/lang/String;");
		_get_screen_DPI = p_env->GetMethodID(cls, "getScreenDPI", "()I");
		_get_scaled_density = p_env->GetMethodID(cls, "getScaledDensity", "()F");
		_get_screen_refresh_rate = p_env->GetMethodID(cls, "getScreenRefreshRate", "(D)D");
		_get_unique_id = p_env->GetMethodID(cls, "getUniqueID", "()Ljava/lang/String;");
		_show_keyboard = p_env->GetMethodID(cls, "showKeyboard", "(Ljava/lang/String;IIII)V");
		_hide_keyboard = p_env->GetMethodID(cls, "hideKeyboard", "()V");
		_has_hardware_keyboard = p_env->GetMethodID(cls, "hasHardwareKeyboard", "()Z");
		_set_screen_orientation = p_env->GetMethodID(cls, "setScreenOrientation", "(I)V");
		_get_screen_orientation = p_env->GetMethodID(cls, "getScreenOrientation", "()I");
		_get_system_dir = p_env->GetMethodID(cls, "getSystemDir", "(IZ)Ljava/lang/String;");
		_get_display_rotation = p_env->GetMethodID(cls, "getDisplayRotation", "()I");
	}
}

GodotIOJavaWrapper::~GodotIOJavaWrapper() {
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	env->DeleteGlobalRef(cls);
	env->DeleteGlobalRef(godot_io_instance);
}

jobject GodotIOJavaWrapper::get_instance() {
	return godot_io_instance;
}

Error GodotIOJavaWrapper::open_uri(const String &p_uri) {
	if (_open_URI) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, ERR_UNAVAILABLE);
		jstring jStr = env->NewStringUTF(p_uri.utf8().get_data());
		Error result = env->CallIntMethod(godot_io_instance, _open_URI, jStr) ? ERR_CANT_OPEN : OK;
		env->DeleteLocalRef(jStr);
		return result;
	} else {
		return ERR_UNAVAILABLE;
	}
}

String GodotIOJavaWrapper::get_cache_dir() {
	if (_get_cache_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_cache_dir);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_temp_dir() {
	if (_get_temp_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_temp_dir);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_user_data_dir(const String &p_user_dir) {
	if (_get_data_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_data_dir);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_locale() {
	if (_get_locale) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_locale);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

String GodotIOJavaWrapper::get_model() {
	if (_get_model) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_model);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

int GodotIOJavaWrapper::get_screen_dpi() {
	if (_get_screen_DPI) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 160);
		return env->CallIntMethod(godot_io_instance, _get_screen_DPI);
	} else {
		return 160;
	}
}

float GodotIOJavaWrapper::get_scaled_density() {
	if (_get_scaled_density) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 1.0f);
		return env->CallFloatMethod(godot_io_instance, _get_scaled_density);
	} else {
		return 1.0f;
	}
}

float GodotIOJavaWrapper::get_screen_refresh_rate(float fallback) {
	if (_get_screen_refresh_rate) {
		JNIEnv *env = get_jni_env();
		if (env == nullptr) {
			ERR_PRINT("An error occurred while trying to get screen refresh rate.");
			return fallback;
		}
		return (float)env->CallDoubleMethod(godot_io_instance, _get_screen_refresh_rate, (double)fallback);
	}
	ERR_PRINT("An error occurred while trying to get the screen refresh rate.");
	return fallback;
}

TypedArray<Rect2> GodotIOJavaWrapper::get_display_cutouts() {
	TypedArray<Rect2> result;
	ERR_FAIL_NULL_V(_get_display_cutouts, result);
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, result);
	jintArray returnArray = (jintArray)env->CallObjectMethod(godot_io_instance, _get_display_cutouts);
	jint arrayLength = env->GetArrayLength(returnArray);
	jint *arrayBody = env->GetIntArrayElements(returnArray, JNI_FALSE);
	int cutouts = arrayLength / 4;
	for (int i = 0; i < cutouts; i++) {
		int x = arrayBody[i * 4];
		int y = arrayBody[i * 4 + 1];
		int width = arrayBody[i * 4 + 2];
		int height = arrayBody[i * 4 + 3];
		Rect2 cutout(x, y, width, height);
		result.append(cutout);
	}
	env->ReleaseIntArrayElements(returnArray, arrayBody, 0);
	return result;
}

Rect2i GodotIOJavaWrapper::get_display_safe_area() {
	Rect2i result;
	ERR_FAIL_NULL_V(_get_display_safe_area, result);
	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL_V(env, result);
	jintArray returnArray = (jintArray)env->CallObjectMethod(godot_io_instance, _get_display_safe_area);
	ERR_FAIL_COND_V(env->GetArrayLength(returnArray) != 4, result);
	jint *arrayBody = env->GetIntArrayElements(returnArray, JNI_FALSE);
	result = Rect2i(arrayBody[0], arrayBody[1], arrayBody[2], arrayBody[3]);
	env->ReleaseIntArrayElements(returnArray, arrayBody, 0);
	return result;
}

String GodotIOJavaWrapper::get_unique_id() {
	if (_get_unique_id) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String());
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_unique_id);
		return jstring_to_string(s, env);
	} else {
		return String();
	}
}

bool GodotIOJavaWrapper::has_vk() {
	return (_show_keyboard != nullptr) && (_hide_keyboard != nullptr);
}

bool GodotIOJavaWrapper::has_hardware_keyboard() {
	if (_has_hardware_keyboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, false);
		return env->CallBooleanMethod(godot_io_instance, _has_hardware_keyboard);
	} else {
		return false;
	}
}

void GodotIOJavaWrapper::show_vk(const String &p_existing, int p_type, int p_max_input_length, int p_cursor_start, int p_cursor_end) {
	if (_show_keyboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		jstring jStr = env->NewStringUTF(p_existing.utf8().get_data());
		env->CallVoidMethod(godot_io_instance, _show_keyboard, jStr, p_type, p_max_input_length, p_cursor_start, p_cursor_end);
		env->DeleteLocalRef(jStr);
	}
}

void GodotIOJavaWrapper::hide_vk() {
	if (_hide_keyboard) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_io_instance, _hide_keyboard);
	}
}

void GodotIOJavaWrapper::set_screen_orientation(int p_orient) {
	if (_set_screen_orientation) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL(env);
		env->CallVoidMethod(godot_io_instance, _set_screen_orientation, p_orient);
	}
}

int GodotIOJavaWrapper::get_screen_orientation() {
	if (_get_screen_orientation) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		return env->CallIntMethod(godot_io_instance, _get_screen_orientation);
	} else {
		return 0;
	}
}

String GodotIOJavaWrapper::get_system_dir(int p_dir, bool p_shared_storage) {
	if (_get_system_dir) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, String("."));
		jstring s = (jstring)env->CallObjectMethod(godot_io_instance, _get_system_dir, p_dir, p_shared_storage);
		return jstring_to_string(s, env);
	} else {
		return String(".");
	}
}

int GodotIOJavaWrapper::get_display_rotation() {
	if (_get_display_rotation) {
		JNIEnv *env = get_jni_env();
		ERR_FAIL_NULL_V(env, 0);
		return env->CallIntMethod(godot_io_instance, _get_display_rotation);
	} else {
		return 0;
	}
}

// SafeNumeric because it can be changed from non-main thread and we need to
// ensure the change is immediately visible to other threads.
static SafeNumeric<int> virtual_keyboard_height;

int GodotIOJavaWrapper::get_vk_height() {
	return virtual_keyboard_height.get();
}

void GodotIOJavaWrapper::set_vk_height(int p_height) {
	virtual_keyboard_height.set(p_height);
}


// --- Merged from platform/android/jni_utils.cpp ---
#include "platform/android/os_android.h"

#include "platform/android/api/java_class_wrapper.h"

// Maximum recursion depth allowed when converting variants to jobjects or vice-versa.
#define RECURSION_DEPTH_LIMIT 64

static jobject android_class_loader = nullptr;
static jmethodID load_class_method = nullptr;

jobject callable_to_jcallable(JNIEnv *p_env, const Variant &p_callable) {
	ERR_FAIL_NULL_V(p_env, nullptr);
	if (p_callable.get_type() != Variant::CALLABLE) {
		return nullptr;
	}

	Variant *callable_jcopy = memnew(Variant(p_callable));

	jclass bclass = jni_find_class(p_env, "org/godotengine/godot/variant/Callable");
	jmethodID ctor = p_env->GetMethodID(bclass, "<init>", "(J)V");
	jobject jcallable = p_env->NewObject(bclass, ctor, reinterpret_cast<int64_t>(callable_jcopy));
	p_env->DeleteLocalRef(bclass);

	return jcallable;
}

Callable jcallable_to_callable(JNIEnv *p_env, jobject p_jcallable_obj) {
	ERR_FAIL_NULL_V(p_env, Callable());

	const Variant *callable_variant = nullptr;
	if (p_jcallable_obj) {
		jclass callable_class = jni_find_class(p_env, "org/godotengine/godot/variant/Callable");
		if (callable_class && p_env->IsInstanceOf(p_jcallable_obj, callable_class)) {
			jmethodID get_native_pointer = p_env->GetMethodID(callable_class, "getNativePointer", "()J");
			jlong native_callable = p_env->CallLongMethod(p_jcallable_obj, get_native_pointer);

			callable_variant = reinterpret_cast<const Variant *>(native_callable);
		}

		p_env->DeleteLocalRef(callable_class);
	}

	ERR_FAIL_NULL_V(callable_variant, Callable());
	return *callable_variant;
}

String charsequence_to_string(JNIEnv *p_env, jobject p_charsequence) {
	ERR_FAIL_NULL_V(p_env, String());

	String result;
	if (p_charsequence) {
		jclass bclass = jni_find_class(p_env, "java/lang/CharSequence");
		if (bclass && p_env->IsInstanceOf(p_charsequence, bclass)) {
			jmethodID to_string = p_env->GetMethodID(bclass, "toString", "()Ljava/lang/String;");
			jstring obj_string = (jstring)p_env->CallObjectMethod(p_charsequence, to_string);

			result = jstring_to_string(obj_string, p_env);
			p_env->DeleteLocalRef(obj_string);
		}

		p_env->DeleteLocalRef(bclass);
	}
	return result;
}

jobject _variant_to_jobject(JNIEnv *env, Variant::Type p_type, const Variant *p_arg, int p_depth) {
	jobject ret = nullptr;

	if (p_depth > RECURSION_DEPTH_LIMIT) {
		ERR_PRINT("Variant is too deep! Bailing.");
		return ret;
	}

	env->PushLocalFrame(2);
	if (env->ExceptionCheck()) {
		env->ExceptionDescribe();
		env->ExceptionClear();
		return ret;
	}

	switch (p_type) {
		case Variant::BOOL: {
			jclass bclass = jni_find_class(env, "java/lang/Boolean");
			jmethodID ctor = env->GetMethodID(bclass, "<init>", "(Z)V");
			jvalue val;
			val.z = (bool)(*p_arg);
			ret = env->NewObjectA(bclass, ctor, &val);
			env->DeleteLocalRef(bclass);
		} break;
		case Variant::INT: {
			jclass bclass = jni_find_class(env, "java/lang/Long");
			jmethodID ctor = env->GetMethodID(bclass, "<init>", "(J)V");
			jvalue val;
			val.j = (jlong)(*p_arg);
			ret = env->NewObjectA(bclass, ctor, &val);
			env->DeleteLocalRef(bclass);
		} break;
		case Variant::FLOAT: {
			jclass bclass = jni_find_class(env, "java/lang/Double");
			jmethodID ctor = env->GetMethodID(bclass, "<init>", "(D)V");
			jvalue val;
			val.d = (double)(*p_arg);
			ret = env->NewObjectA(bclass, ctor, &val);
			env->DeleteLocalRef(bclass);
		} break;
		case Variant::STRING: {
			String s = *p_arg;
			jstring jStr = env->NewStringUTF(s.utf8().get_data());
			ret = jStr;
		} break;
		case Variant::PACKED_STRING_ARRAY: {
			Vector<String> sarray = *p_arg;
			jobjectArray arr = env->NewObjectArray(sarray.size(), jni_find_class(env, "java/lang/String"), env->NewStringUTF(""));

			for (int j = 0; j < sarray.size(); j++) {
				jstring str = env->NewStringUTF(sarray[j].utf8().get_data());
				env->SetObjectArrayElement(arr, j, str);
				env->DeleteLocalRef(str);
			}
			ret = arr;
		} break;

		case Variant::CALLABLE: {
			jobject jcallable = callable_to_jcallable(env, *p_arg);
			ret = jcallable;
		} break;

		case Variant::DICTIONARY: {
			Dictionary dict = *p_arg;
			jclass dclass = jni_find_class(env, "org/godotengine/godot/Dictionary");
			jmethodID ctor = env->GetMethodID(dclass, "<init>", "()V");
			jobject jdict = env->NewObject(dclass, ctor);

			Array keys = dict.keys();

			jobjectArray jkeys = env->NewObjectArray(keys.size(), jni_find_class(env, "java/lang/String"), env->NewStringUTF(""));
			for (int j = 0; j < keys.size(); j++) {
				jstring str = env->NewStringUTF(String(keys[j]).utf8().get_data());
				env->SetObjectArrayElement(jkeys, j, str);
				env->DeleteLocalRef(str);
			}

			jmethodID set_keys = env->GetMethodID(dclass, "set_keys", "([Ljava/lang/String;)V");
			jvalue val;
			val.l = jkeys;
			env->CallVoidMethodA(jdict, set_keys, &val);
			env->DeleteLocalRef(jkeys);

			jobjectArray jvalues = env->NewObjectArray(keys.size(), jni_find_class(env, "java/lang/Object"), nullptr);

			for (int j = 0; j < keys.size(); j++) {
				Variant var = dict[keys[j]];
				jobject jvar = _variant_to_jobject(env, var.get_type(), &var, p_depth + 1);
				env->SetObjectArrayElement(jvalues, j, jvar);
				if (jvar) {
					env->DeleteLocalRef(jvar);
				}
			}

			jmethodID set_values = env->GetMethodID(dclass, "set_values", "([Ljava/lang/Object;)V");
			val.l = jvalues;
			env->CallVoidMethodA(jdict, set_values, &val);
			env->DeleteLocalRef(jvalues);
			env->DeleteLocalRef(dclass);

			ret = jdict;
		} break;

		case Variant::ARRAY: {
			Array array = *p_arg;
			jobjectArray arr = env->NewObjectArray(array.size(), jni_find_class(env, "java/lang/Object"), nullptr);

			for (int j = 0; j < array.size(); j++) {
				Variant var = array[j];
				jobject jvar = _variant_to_jobject(env, var.get_type(), &var, p_depth + 1);
				env->SetObjectArrayElement(arr, j, jvar);
				if (jvar) {
					env->DeleteLocalRef(jvar);
				}
			}
			ret = arr;
		} break;

		case Variant::PACKED_INT32_ARRAY: {
			Vector<int> array = *p_arg;
			jintArray arr = env->NewIntArray(array.size());
			const int *r = array.ptr();
			env->SetIntArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::PACKED_INT64_ARRAY: {
			Vector<int64_t> array = *p_arg;
			jlongArray arr = env->NewLongArray(array.size());
			const int64_t *r = array.ptr();
			env->SetLongArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::PACKED_BYTE_ARRAY: {
			Vector<uint8_t> array = *p_arg;
			jbyteArray arr = env->NewByteArray(array.size());
			const uint8_t *r = array.ptr();
			env->SetByteArrayRegion(arr, 0, array.size(), reinterpret_cast<const signed char *>(r));
			ret = arr;
		} break;
		case Variant::PACKED_FLOAT32_ARRAY: {
			Vector<float> array = *p_arg;
			jfloatArray arr = env->NewFloatArray(array.size());
			const float *r = array.ptr();
			env->SetFloatArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::PACKED_FLOAT64_ARRAY: {
			Vector<double> array = *p_arg;
			jdoubleArray arr = env->NewDoubleArray(array.size());
			const double *r = array.ptr();
			env->SetDoubleArrayRegion(arr, 0, array.size(), r);
			ret = arr;
		} break;
		case Variant::OBJECT: {
			Ref<JavaObject> generic_object = *p_arg;
			if (generic_object.is_valid()) {
				jobject obj = env->NewLocalRef(generic_object->get_instance());
				ret = obj;
			}
		} break;

		// Add default to prevent compiler warning about not handling all types.
		default:
			break;
	}

	return env->PopLocalFrame(ret);
}

String _get_class_name(JNIEnv *env, jclass cls, bool *array) {
	jclass cclass = jni_find_class(env, "java/lang/Class");
	jmethodID getName = env->GetMethodID(cclass, "getName", "()Ljava/lang/String;");
	jstring clsName = (jstring)env->CallObjectMethod(cls, getName);

	if (array) {
		jmethodID isArray = env->GetMethodID(cclass, "isArray", "()Z");
		jboolean isarr = env->CallBooleanMethod(cls, isArray);
		(*array) = isarr != 0;
	}
	String name = jstring_to_string(clsName, env);
	env->DeleteLocalRef(clsName);
	env->DeleteLocalRef(cclass);

	return name;
}

Variant _jobject_to_variant(JNIEnv *env, jobject obj, int p_depth) {
	ERR_FAIL_COND_V_MSG(p_depth > RECURSION_DEPTH_LIMIT, Variant(), "Variant is too deep! Bailing.");

	if (obj == nullptr) {
		return Variant();
	}

	jclass c = env->GetObjectClass(obj);
	bool array;
	String name = _get_class_name(env, c, &array);

	if (name == "java.lang.String") {
		return jstring_to_string((jstring)obj, env);
	}

	if (name == "java.lang.CharSequence") {
		return charsequence_to_string(env, obj);
	}

	if (name == "[Ljava.lang.String;") {
		jobjectArray arr = (jobjectArray)obj;
		int stringCount = env->GetArrayLength(arr);
		Vector<String> sarr;

		for (int i = 0; i < stringCount; i++) {
			jstring string = (jstring)env->GetObjectArrayElement(arr, i);
			sarr.push_back(jstring_to_string(string, env));
			env->DeleteLocalRef(string);
		}

		return sarr;
	}

	if (name == "[Ljava.lang.CharSequence;") {
		jobjectArray arr = (jobjectArray)obj;
		int stringCount = env->GetArrayLength(arr);
		Vector<String> sarr;

		for (int i = 0; i < stringCount; i++) {
			jobject charsequence = env->GetObjectArrayElement(arr, i);
			sarr.push_back(charsequence_to_string(env, charsequence));
			env->DeleteLocalRef(charsequence);
		}

		return sarr;
	}

	if (name == "java.lang.Boolean") {
		jmethodID boolValue = env->GetMethodID(c, "booleanValue", "()Z");
		bool ret = env->CallBooleanMethod(obj, boolValue);
		return ret;
	}

	if (name == "java.lang.Integer" || name == "java.lang.Long") {
		jclass nclass = jni_find_class(env, "java/lang/Number");
		jmethodID longValue = env->GetMethodID(nclass, "longValue", "()J");
		jlong ret = env->CallLongMethod(obj, longValue);
		env->DeleteLocalRef(nclass);
		return ret;
	}

	if (name == "[I") {
		jintArray arr = (jintArray)obj;
		int fCount = env->GetArrayLength(arr);
		Vector<int> sarr;
		sarr.resize(fCount);

		int *w = sarr.ptrw();
		env->GetIntArrayRegion(arr, 0, fCount, w);
		return sarr;
	}

	if (name == "[J") {
		jlongArray arr = (jlongArray)obj;
		int fCount = env->GetArrayLength(arr);
		Vector<int64_t> sarr;
		sarr.resize(fCount);

		int64_t *w = sarr.ptrw();
		env->GetLongArrayRegion(arr, 0, fCount, w);
		return sarr;
	}

	if (name == "[B") {
		jbyteArray arr = (jbyteArray)obj;
		int fCount = env->GetArrayLength(arr);
		Vector<uint8_t> sarr;
		sarr.resize(fCount);

		uint8_t *w = sarr.ptrw();
		env->GetByteArrayRegion(arr, 0, fCount, reinterpret_cast<signed char *>(w));
		return sarr;
	}

	if (name == "java.lang.Float" || name == "java.lang.Double") {
		jclass nclass = jni_find_class(env, "java/lang/Number");
		jmethodID doubleValue = env->GetMethodID(nclass, "doubleValue", "()D");
		double ret = env->CallDoubleMethod(obj, doubleValue);
		env->DeleteLocalRef(nclass);
		return ret;
	}

	if (name == "[D") {
		jdoubleArray arr = (jdoubleArray)obj;
		int fCount = env->GetArrayLength(arr);
		PackedFloat64Array packed_array;
		packed_array.resize(fCount);

		double *w = packed_array.ptrw();

		for (int i = 0; i < fCount; i++) {
			double n;
			env->GetDoubleArrayRegion(arr, i, 1, &n);
			w[i] = n;
		}
		return packed_array;
	}

	if (name == "[F") {
		jfloatArray arr = (jfloatArray)obj;
		int fCount = env->GetArrayLength(arr);
		PackedFloat32Array packed_array;
		packed_array.resize(fCount);

		float *w = packed_array.ptrw();

		for (int i = 0; i < fCount; i++) {
			float n;
			env->GetFloatArrayRegion(arr, i, 1, &n);
			w[i] = n;
		}
		return packed_array;
	}

	if (name == "[Ljava.lang.Object;") {
		jobjectArray arr = (jobjectArray)obj;
		int objCount = env->GetArrayLength(arr);
		Array varr;

		for (int i = 0; i < objCount; i++) {
			jobject jobj = env->GetObjectArrayElement(arr, i);
			Variant v = _jobject_to_variant(env, jobj, p_depth + 1);
			varr.push_back(v);
			env->DeleteLocalRef(jobj);
		}

		return varr;
	}

	if (name == "org.godotengine.godot.Dictionary") {
		Dictionary ret;
		jclass oclass = c;

		jmethodID entrySet = env->GetMethodID(oclass, "entrySet", "()Ljava/util/Set;");
		jobject entrySetObj = env->CallObjectMethod(obj, entrySet);

		jclass setClass = env->GetObjectClass(entrySetObj);
		jmethodID iterator = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
		jobject iteratorObj = env->CallObjectMethod(entrySetObj, iterator);

		jclass iteratorClass = env->GetObjectClass(iteratorObj);
		jmethodID hasNext = env->GetMethodID(iteratorClass, "hasNext", "()Z");
		jmethodID next = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

		jclass entryClass = jni_find_class(env, "java/util/Map$Entry");
		jmethodID getKey = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
		jmethodID getValue = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");

		while (env->CallBooleanMethod(iteratorObj, hasNext)) {
			jobject entry = env->CallObjectMethod(iteratorObj, next);

			jobject jkey = env->CallObjectMethod(entry, getKey);
			jobject jvalue = env->CallObjectMethod(entry, getValue);

			Variant key = _jobject_to_variant(env, jkey, p_depth + 1);
			Variant value = _jobject_to_variant(env, jvalue, p_depth + 1);
			ret[key] = value;

			env->DeleteLocalRef(entry);
			env->DeleteLocalRef(jkey);
			env->DeleteLocalRef(jvalue);
		}

		env->DeleteLocalRef(entrySetObj);
		env->DeleteLocalRef(iteratorObj);
		env->DeleteLocalRef(setClass);
		env->DeleteLocalRef(iteratorClass);
		env->DeleteLocalRef(entryClass);

		return ret;
	}

	if (name == "org.godotengine.godot.variant.Callable") {
		return jcallable_to_callable(env, obj);
	}

	Ref<JavaObject> generic_object(memnew(JavaObject(JavaClassWrapper::wrap(name), obj)));

	env->DeleteLocalRef(c);

	return generic_object;
}

Variant::Type get_jni_type(const String &p_type) {
	static struct {
		const char *name;
		Variant::Type type;
	} _type_to_vtype[] = {
		{ "void", Variant::NIL },
		{ "boolean", Variant::BOOL },
		{ "int", Variant::INT },
		{ "long", Variant::INT },
		{ "float", Variant::FLOAT },
		{ "double", Variant::FLOAT },
		{ "java.lang.String", Variant::STRING },
		{ "java.lang.CharSequence", Variant::STRING },
		{ "[I", Variant::PACKED_INT32_ARRAY },
		{ "[J", Variant::PACKED_INT64_ARRAY },
		{ "[B", Variant::PACKED_BYTE_ARRAY },
		{ "[F", Variant::PACKED_FLOAT32_ARRAY },
		{ "[D", Variant::PACKED_FLOAT64_ARRAY },
		{ "[Ljava.lang.String;", Variant::PACKED_STRING_ARRAY },
		{ "[Ljava.lang.CharSequence;", Variant::PACKED_STRING_ARRAY },
		{ "org.godotengine.godot.Dictionary", Variant::DICTIONARY },
		{ "org.godotengine.godot.variant.Callable", Variant::CALLABLE },
		{ nullptr, Variant::NIL }
	};

	int idx = 0;

	while (_type_to_vtype[idx].name) {
		if (p_type == _type_to_vtype[idx].name) {
			return _type_to_vtype[idx].type;
		}

		idx++;
	}

	return Variant::OBJECT;
}

void setup_android_class_loader() {
	// Find a known class defined in the Godot package and obtain its ClassLoader.
	// This ClassLoader will be used by jni_find_class() to locate classes at runtime
	// in a thread-safe manner, avoiding issues with FindClass in non-main threads.

	if (android_class_loader) {
		cleanup_android_class_loader();
	}

	JNIEnv *env = get_jni_env();
	ERR_FAIL_NULL(env);

	jclass known_class = env->FindClass("org/godotengine/godot/Godot");
	ERR_FAIL_NULL(known_class);

	jclass class_class = env->FindClass("java/lang/Class");
	ERR_FAIL_NULL(class_class);

	jmethodID get_class_loader_method = env->GetMethodID(class_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
	ERR_FAIL_NULL(get_class_loader_method);

	jobject class_loader = env->CallObjectMethod(known_class, get_class_loader_method);
	ERR_FAIL_NULL(class_loader);

	// NOTE: Make global ref so it can be used later.
	android_class_loader = env->NewGlobalRef(class_loader);
	ERR_FAIL_NULL(android_class_loader);

	jclass class_loader_class = env->FindClass("java/lang/ClassLoader");
	ERR_FAIL_NULL(class_loader_class);

	load_class_method = env->GetMethodID(class_loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	if (!load_class_method) {
		env->DeleteGlobalRef(android_class_loader);
		android_class_loader = nullptr;
		ERR_FAIL_MSG("Failed to find method ID for ClassLoader::loadClass.");
	}

	env->DeleteLocalRef(class_loader_class);
	env->DeleteLocalRef(class_loader);
	env->DeleteLocalRef(class_class);
	env->DeleteLocalRef(known_class);
}

void cleanup_android_class_loader() {
	if (android_class_loader != nullptr) {
		JNIEnv *env = get_jni_env();
		if (env) {
			env->DeleteGlobalRef(android_class_loader);
		} else {
			ERR_PRINT("Failed to release Android ClassLoader - JNIEnv is not available.");
		}
		android_class_loader = nullptr;
		load_class_method = nullptr;
	}
}

jclass jni_find_class(JNIEnv *p_env, const char *p_class_name) {
	ERR_FAIL_NULL_V(p_env, nullptr);
	ERR_FAIL_NULL_V(p_class_name, nullptr);

	jobject class_object = nullptr;
	if (!android_class_loader || !load_class_method) {
		ERR_PRINT("Android ClassLoader is not initialized. Falling back to FindClass.");
		class_object = p_env->FindClass(p_class_name);
	} else {
		jstring java_class_name = p_env->NewStringUTF(p_class_name);
		class_object = p_env->CallObjectMethod(
				android_class_loader,
				load_class_method,
				java_class_name);
		p_env->DeleteLocalRef(java_class_name);
	}
	if (p_env->ExceptionCheck()) {
		p_env->ExceptionDescribe();
		p_env->ExceptionClear();
	}
	ERR_FAIL_NULL_V_MSG(class_object, nullptr, vformat("Failed to find Java class: \"%s\".", p_class_name));
	return static_cast<jclass>(class_object);
}


// --- Merged from platform/android/plugin/godot_plugin_jni.cpp ---
#include "platform/android/os_android.h"

#include "api/java_class_wrapper.h"
#include "platform/android/os_android.h"
#include "platform/android/os_android.h"

#include "core/config/engine.h"
#include "core/error/error_macros.h"

static HashMap<String, JNISingleton *> jni_singletons;

void unregister_plugins_singletons() {
	for (const KeyValue<String, JNISingleton *> &E : jni_singletons) {
		Engine::remove_singleton(E.key);

		memdelete(E.value);
	}
	jni_singletons.clear();
}

extern "C" {

JNIEXPORT jboolean JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterSingleton(JNIEnv *env, jclass clazz, jstring name, jobject obj) {
	String singname = jstring_to_string(name, env);

	ERR_FAIL_COND_V(jni_singletons.has(singname), false);

	jclass java_class = env->GetObjectClass(obj);
	Ref<JavaClass> java_class_wrapped = JavaClassWrapper::wrap_jclass(java_class, true);
	env->DeleteLocalRef(java_class);

	Ref<JavaObject> plugin_object = memnew(JavaObject(java_class_wrapped, obj));
	JNISingleton *plugin_singleton = memnew(JNISingleton(plugin_object));
	jni_singletons[singname] = plugin_singleton;

	Engine::add_singleton(Engine::Singleton(singname, plugin_singleton));
	return true;
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterMethod(JNIEnv *env, jclass clazz, jstring sname, jstring name, jstring ret, jobjectArray args) {
	String singname = jstring_to_string(sname, env);

	ERR_FAIL_COND(!jni_singletons.has(singname));

	JNISingleton *s = jni_singletons.get(singname);

	String mname = jstring_to_string(name, env);
	String retval = jstring_to_string(ret, env);
	Vector<Variant::Type> types;

	int stringCount = env->GetArrayLength(args);

	for (int i = 0; i < stringCount; i++) {
		jstring string = (jstring)env->GetObjectArrayElement(args, i);
		const String rawString = jstring_to_string(string, env);
		types.push_back(get_jni_type(rawString));
	}

	s->add_method(mname, types, get_jni_type(retval));
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeRegisterSignal(JNIEnv *env, jclass clazz, jstring j_plugin_name, jstring j_signal_name, jobjectArray j_signal_param_types) {
	String singleton_name = jstring_to_string(j_plugin_name, env);

	ERR_FAIL_COND(!jni_singletons.has(singleton_name));

	JNISingleton *singleton = jni_singletons.get(singleton_name);

	String signal_name = jstring_to_string(j_signal_name, env);
	Vector<Variant::Type> types;

	int stringCount = env->GetArrayLength(j_signal_param_types);

	for (int i = 0; i < stringCount; i++) {
		jstring j_signal_param_type = (jstring)env->GetObjectArrayElement(j_signal_param_types, i);
		const String signal_param_type = jstring_to_string(j_signal_param_type, env);
		types.push_back(get_jni_type(signal_param_type));
	}

	singleton->add_signal(signal_name, types);
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_plugin_GodotPlugin_nativeEmitSignal(JNIEnv *env, jclass clazz, jstring j_plugin_name, jstring j_signal_name, jobjectArray j_signal_params) {
	String singleton_name = jstring_to_string(j_plugin_name, env);

	ERR_FAIL_COND(!jni_singletons.has(singleton_name));

	JNISingleton *singleton = jni_singletons.get(singleton_name);

	String signal_name = jstring_to_string(j_signal_name, env);

	int count = env->GetArrayLength(j_signal_params);

	Variant *variant_params = (Variant *)alloca(sizeof(Variant) * count);
	const Variant **args = (const Variant **)alloca(sizeof(Variant *) * count);

	for (int i = 0; i < count; i++) {
		jobject j_param = env->GetObjectArrayElement(j_signal_params, i);
		memnew_placement(&variant_params[i], Variant(_jobject_to_variant(env, j_param)));
		args[i] = &variant_params[i];
		env->DeleteLocalRef(j_param);
	}

	singleton->emit_signalp(String(signal_name), args, count);

	// Manually invoke the destructor to decrease the reference counts for the variant arguments.
	for (int i = 0; i < count; i++) {
		variant_params[i].~Variant();
	}
}
}


// --- Merged from platform/android/variant/callable_jni.cpp ---
// --- Inlined from platform/android/variant/callable_jni.h ---
#include <jni.h>

extern "C" {
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCall(JNIEnv *p_env, jclass p_clazz, jlong p_native_callable, jobjectArray p_parameters);
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObject(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters);
JNIEXPORT void JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObjectDeferred(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters);
JNIEXPORT void JNICALL Java_org_godotengine_godot_variant_Callable_releaseNativePointer(JNIEnv *p_env, jclass clazz, jlong p_native_pointer);
}

// --- End platform/android/variant/callable_jni.h ---

#include "platform/android/os_android.h"

#include "core/error/error_macros.h"

static Callable _generate_callable(JNIEnv *p_env, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters) {
	Object *obj = ObjectDB::get_instance(ObjectID(p_object_id));
	ERR_FAIL_NULL_V(obj, Callable());

	String str_method = jstring_to_string(p_method_name, p_env);

	int count = p_env->GetArrayLength(p_parameters);

	Variant *args = (Variant *)alloca(sizeof(Variant) * count);
	const Variant **argptrs = (const Variant **)alloca(sizeof(Variant *) * count);

	for (int i = 0; i < count; i++) {
		jobject jobj = p_env->GetObjectArrayElement(p_parameters, i);
		ERR_FAIL_NULL_V(jobj, Callable());
		memnew_placement(&args[i], Variant(_jobject_to_variant(p_env, jobj)));
		argptrs[i] = &args[i];
		p_env->DeleteLocalRef(jobj);
	}

	Callable ret = Callable(obj, str_method).bindp(argptrs, count);

	// Manually invoke the destructor to decrease the reference counts for the variant arguments.
	for (int i = 0; i < count; i++) {
		args[i].~Variant();
	}

	return ret;
}

extern "C" {
JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCall(JNIEnv *p_env, jclass p_clazz, jlong p_native_callable, jobjectArray p_parameters) {
	const Variant *callable_variant = reinterpret_cast<const Variant *>(p_native_callable);
	ERR_FAIL_NULL_V(callable_variant, nullptr);
	if (callable_variant->get_type() != Variant::CALLABLE) {
		return nullptr;
	}

	int count = p_env->GetArrayLength(p_parameters);

	Variant *args = (Variant *)alloca(sizeof(Variant) * count);
	const Variant **argptrs = (const Variant **)alloca(sizeof(Variant *) * count);

	for (int i = 0; i < count; i++) {
		jobject jobj = p_env->GetObjectArrayElement(p_parameters, i);
		ERR_FAIL_NULL_V(jobj, nullptr);
		memnew_placement(&args[i], Variant(_jobject_to_variant(p_env, jobj)));
		argptrs[i] = &args[i];
		p_env->DeleteLocalRef(jobj);
	}

	Callable callable = *callable_variant;
	jobject ret = nullptr;
	if (callable.is_valid()) {
		Callable::CallError err;
		Variant result;
		callable.callp(argptrs, count, result, err);
		ret = _variant_to_jobject(p_env, result.get_type(), &result);
	}

	// Manually invoke the destructor to decrease the reference counts for the variant arguments.
	for (int i = 0; i < count; i++) {
		args[i].~Variant();
	}

	return ret;
}

JNIEXPORT jobject JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObject(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters) {
	Callable callable = _generate_callable(p_env, p_object_id, p_method_name, p_parameters);
	if (callable.is_valid()) {
		Variant result = callable.call();
		return _variant_to_jobject(p_env, result.get_type(), &result);
	} else {
		return nullptr;
	}
}

JNIEXPORT void JNICALL Java_org_godotengine_godot_variant_Callable_nativeCallObjectDeferred(JNIEnv *p_env, jclass p_clazz, jlong p_object_id, jstring p_method_name, jobjectArray p_parameters) {
	Callable callable = _generate_callable(p_env, p_object_id, p_method_name, p_parameters);
	if (callable.is_valid()) {
		callable.call_deferred();
	}
}

JNIEXPORT void JNICALL
Java_org_godotengine_godot_variant_Callable_releaseNativePointer(JNIEnv *p_env, jclass clazz, jlong p_native_pointer) {
	Variant *variant = reinterpret_cast<Variant *>(p_native_pointer);
	ERR_FAIL_NULL(variant);
	memdelete(variant);
}
}
