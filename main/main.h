/**************************************************************************/
/*  main.h                                                                */
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

#include "core/os/thread.h"
#include "core/typedefs.h"

template <typename T>
class Vector;

class Main {
	enum CLIOptionAvailability {
		CLI_OPTION_AVAILABILITY_EDITOR,
		CLI_OPTION_AVAILABILITY_TEMPLATE_DEBUG,
		CLI_OPTION_AVAILABILITY_TEMPLATE_RELEASE,
		CLI_OPTION_AVAILABILITY_TEMPLATE_UNSAFE,
		CLI_OPTION_AVAILABILITY_HIDDEN,
	};

	static void print_header(bool p_rich);
	static void print_help_copyright(const char *p_notice);
	static void print_help_title(const char *p_title);
	static void print_help_option(const char *p_option, const char *p_description, CLIOptionAvailability p_availability = CLI_OPTION_AVAILABILITY_TEMPLATE_RELEASE);
	static String format_help_option(const char *p_option);
	static void print_help(const char *p_binary);
	static uint64_t last_ticks;
	static uint32_t hide_print_fps_attempts;
	static uint32_t frames;
	static uint32_t frame;
	static bool force_redraw_requested;
	static int iterating;

public:
	static bool is_cmdline_tool();
#ifdef TOOLS_ENABLED
	enum CLIScope {
		CLI_SCOPE_TOOL, // Editor and project manager.
		CLI_SCOPE_PROJECT,
	};
	static const Vector<String> &get_forwardable_cli_arguments(CLIScope p_scope);
#endif

	static int test_entrypoint(int argc, char *argv[], bool &tests_need_run);
	static Error setup(const char *execpath, int argc, char *argv[], bool p_second_phase = true);
	static Error setup2(bool p_show_boot_logo = true); // The thread calling setup2() will effectively become the main thread.
	static String get_locale_override();
	static void setup_boot_logo();
	static Error test_setup();
	static void test_cleanup();
	static int start();

	static bool iteration();
	static void force_redraw();

	static bool is_iterating();

	static void cleanup(bool p_force = false);
};

// Test main override is for the testing behavior.
#define TEST_MAIN_OVERRIDE \
	bool run_test = false; \
	int return_code = Main::test_entrypoint(argc, argv, run_test); \
	if (run_test) { \
		godot_cleanup_profiler(); \
		return return_code; \
	}

#define TEST_MAIN_PARAM_OVERRIDE(argc, argv) \
	bool run_test = false; \
	int return_code = Main::test_entrypoint(argc, argv, run_test); \
	if (run_test) { \
		godot_cleanup_profiler(); \
		return return_code; \
	}

// --- Folded from core/register_core_types.h ---
void register_core_types();
void register_core_settings();
void register_core_extensions();
void register_early_core_singletons();
void register_core_singletons();
void unregister_core_types();
void unregister_core_extensions();

// --- Folded from drivers/register_driver_types.h ---
void register_core_driver_types();
void unregister_core_driver_types();

void register_driver_types();
void unregister_driver_types();

// --- Folded from scene/register_scene_types.h ---
void register_scene_types();
void unregister_scene_types();
void register_scene_singletons();

// --- Folded from servers/register_server_types.h ---
void register_server_types();
void unregister_server_types();

// --- Folded from servers/movie_writer/movie_writer.h ---
#include "core/io/image.h"
#include "core/templates/local_vector.h"
#include "servers/display/display_server.h"

class MovieWriter
{
	uint64_t fps = 0;
	uint64_t mix_rate = 0;
	uint32_t audio_channels = 0;

	// The output resolution, which can differ from the window size.
	// Used as a base for resizing all subsequent frames if their resolution differs.
	Vector2i movie_size;

	float cpu_time = 0.0f;
	float gpu_time = 0.0f;
	uint64_t encoding_time_usec = 0;

	String project_name;

	LocalVector<int32_t> audio_mix_buffer;

	enum
	{
		MAX_WRITERS = 8
	};

	static MovieWriter* writers[];
	static uint32_t writer_count;

protected:
	virtual uint32_t get_audio_mix_rate() const;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const;

	virtual Error write_begin(
		const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path);
	virtual Error write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data);
	virtual void write_end();


public:
	virtual bool handles_file(const String& p_path) const;
	virtual void get_supported_extensions(List<String>* r_extensions) const;

	static void add_writer(MovieWriter* p_writer);
	static MovieWriter* find_writer_for_file(const String& p_file);

	void begin(const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path);
	void add_frame();

	static void set_extensions_hint();

	void end();
};

// --- Folded from servers/movie_writer/movie_writer_pngwav.h ---


class MovieWriterPNGWAV : public MovieWriter {

	enum {
		MAX_TRAILING_ZEROS = 8 // more than 10 days at 60fps, no hard drive can put up with this anyway :)
	};

	uint32_t mix_rate = 48000;
	AudioServer::SpeakerMode speaker_mode = AudioServer::SPEAKER_MODE_STEREO;
	String base_path;
	uint32_t frame_count = 0;
	uint32_t fps = 0;

	uint32_t audio_block_size = 0;

	Ref<FileAccess> f_wav;
	uint32_t wav_data_size_pos = 0;

	String zeros_str(uint32_t p_index);

protected:
	virtual uint32_t get_audio_mix_rate() const override;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const override;
	virtual void get_supported_extensions(List<String> *r_extensions) const override;

	virtual Error write_begin(const Size2i &p_movie_size, uint32_t p_fps, const String &p_base_path) override;
	virtual Error write_frame(const Ref<Image> &p_image, const int32_t *p_audio_data) override;
	virtual void write_end() override;

	virtual bool handles_file(const String &p_path) const override;

public:
	MovieWriterPNGWAV();
};

// --- End servers/movie_writer/movie_writer_pngwav.h ---

// --- Folded from platform/register_platform_apis.h ---
void register_platform_apis();
void unregister_platform_apis();

// --- Folded from platform/android/api/api.h ---
void register_android_api();
void unregister_android_api();
// --- End platform/android/api/api.h ---

// --- Folded from platform/ios/api/api.h ---
#if defined(IOS_ENABLED)
extern void godot_apple_embedded_plugins_initialize();
extern void godot_apple_embedded_plugins_deinitialize();
#endif

void register_ios_api();
void unregister_ios_api();
// --- End platform/ios/api/api.h ---

// --- Folded from platform/visionos/api/api.h ---
#if defined(VISIONOS_ENABLED)
extern void godot_apple_embedded_plugins_initialize();
extern void godot_apple_embedded_plugins_deinitialize();
#endif

void register_visionos_api();
void unregister_visionos_api();
// --- End platform/visionos/api/api.h ---

// --- Folded from platform/web/api/api.h ---
void register_web_api();
void unregister_web_api();
// --- End platform/web/api/api.h ---
// --- End platform/register_platform_apis.h ---

// --- Folded from platform/android/api/java_class_wrapper.h ---
#include "core/types.h"

#ifdef ANDROID_ENABLED
#include <android/log.h>
#include <jni.h>
#include "core/templates/hash_map.h"
#endif

#ifdef ANDROID_ENABLED
class JavaObject;
#endif

class JavaClass : public RefCounted
{
#ifdef ANDROID_ENABLED
	enum ArgumentType
	{
		ARG_TYPE_VOID,
		ARG_TYPE_BOOLEAN,
		ARG_TYPE_BYTE,
		ARG_TYPE_CHAR,
		ARG_TYPE_SHORT,
		ARG_TYPE_INT,
		ARG_TYPE_LONG,
		ARG_TYPE_FLOAT,
		ARG_TYPE_DOUBLE,
		ARG_TYPE_STRING, // special case
		ARG_TYPE_CHARSEQUENCE,
		ARG_TYPE_CALLABLE,
		ARG_TYPE_CLASS,
		ARG_ARRAY_BIT = 1 << 16,
		ARG_NUMBER_CLASS_BIT = 1 << 17,
		ARG_TYPE_MASK = (1 << 16) - 1
	};

	RBMap<String, Variant> constant_map;

	struct MethodInfo
	{
		bool _public = false;
		bool _static = false;
		bool _constructor = false;
		Vector<uint32_t> param_types;
		Vector<String> param_sigs;
		uint32_t return_type = 0;
		jmethodID method;
	};

	_FORCE_INLINE_ static void _convert_to_variant_type(
		int p_sig, Variant::Type& r_type, float& likelihood)
	{
		likelihood = 1.0;
		r_type = Variant::NIL;

		switch (p_sig) {
		case ARG_TYPE_VOID:
			r_type = Variant::NIL;
			break;
		case ARG_TYPE_BOOLEAN | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_BOOLEAN:
			r_type = Variant::BOOL;
			break;
		case ARG_TYPE_BYTE | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_BYTE:
			r_type = Variant::INT;
			likelihood = 0.1;
			break;
		case ARG_TYPE_CHAR | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_CHAR:
			r_type = Variant::INT;
			likelihood = 0.2;
			break;
		case ARG_TYPE_SHORT | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_SHORT:
			r_type = Variant::INT;
			likelihood = 0.3;
			break;
		case ARG_TYPE_INT | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_INT:
			r_type = Variant::INT;
			likelihood = 1.0;
			break;
		case ARG_TYPE_LONG | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_LONG:
			r_type = Variant::INT;
			likelihood = 0.5;
			break;
		case ARG_TYPE_FLOAT | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_FLOAT:
			r_type = Variant::FLOAT;
			likelihood = 1.0;
			break;
		case ARG_TYPE_DOUBLE | ARG_NUMBER_CLASS_BIT:
		case ARG_TYPE_DOUBLE:
			r_type = Variant::FLOAT;
			likelihood = 0.5;
			break;
		case ARG_TYPE_STRING:
		case ARG_TYPE_CHARSEQUENCE:
			r_type = Variant::STRING;
			break;
		case ARG_TYPE_CALLABLE:
			r_type = Variant::CALLABLE;
			break;
		case ARG_TYPE_CLASS:
			r_type = Variant::OBJECT;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_VOID:
			r_type = Variant::NIL;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_BOOLEAN:
			r_type = Variant::ARRAY;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_BYTE:
			r_type = Variant::PACKED_BYTE_ARRAY;
			likelihood = 1.0;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_CHAR:
			r_type = Variant::PACKED_BYTE_ARRAY;
			likelihood = 0.5;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_SHORT:
			r_type = Variant::PACKED_INT32_ARRAY;
			likelihood = 0.3;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_INT:
			r_type = Variant::PACKED_INT32_ARRAY;
			likelihood = 1.0;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_LONG:
			r_type = Variant::PACKED_INT32_ARRAY;
			likelihood = 0.5;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_FLOAT:
			r_type = Variant::PACKED_FLOAT32_ARRAY;
			likelihood = 1.0;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_DOUBLE:
			r_type = Variant::PACKED_FLOAT32_ARRAY;
			likelihood = 0.5;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_STRING:
		case ARG_ARRAY_BIT | ARG_TYPE_CHARSEQUENCE:
			r_type = Variant::PACKED_STRING_ARRAY;
			break;
		case ARG_ARRAY_BIT | ARG_TYPE_CLASS:
		case ARG_ARRAY_BIT | ARG_TYPE_CALLABLE:
			r_type = Variant::ARRAY;
			break;
		}
	}

	_FORCE_INLINE_ static bool _convert_object_to_variant(
		JNIEnv* env, jobject obj, Variant& var, uint32_t p_sig);

	bool _call_method(JavaObject* p_instance, const String& p_method, const Variant** p_args,
		int p_argcount, Callable::CallError& r_error, Variant& ret);

	friend class JavaClassWrapper;
	friend class JavaObject;
	String java_class_name;
	String java_constructor_name;
	HashMap<String, List<MethodInfo>> methods;
	jclass _class;
	bool is_interface;
#endif

public:

	String get_java_class_name() const;
	Ref<JavaClass> get_java_parent_class() const;
	bool has_java_method(const String& p_method) const;

#ifdef ANDROID_ENABLED
	virtual String _to_string() override;
#endif

	JavaClass();
	~JavaClass();
};

class JavaObject : public RefCounted
{
#ifdef ANDROID_ENABLED
	Ref<JavaClass> base_class;
	friend class JavaClass;

	jobject instance = nullptr;
#endif

protected:

public:

	Ref<JavaClass> get_java_class() const;
	bool has_java_method(const String& p_method) const;

#ifdef ANDROID_ENABLED
	virtual String _to_string() override;

	jobject get_instance() { return instance; }

	JavaObject();
	JavaObject(const Ref<JavaClass>& p_base, jobject p_instance);
	~JavaObject();
#endif
};

class JavaClassWrapper final
{
	friend class JavaClass;

public:
	struct Data
	{
#ifdef ANDROID_ENABLED
		RBMap<String, Ref<JavaClass>> class_cache;
		jmethodID Class_getConstructors = nullptr;
		jmethodID Class_getDeclaredMethods = nullptr;
		jmethodID Class_getFields = nullptr;
		jmethodID Class_getInterfaces = nullptr;
		jmethodID Class_getName = nullptr;
		jmethodID Class_getSuperclass = nullptr;
		jmethodID Class_isInterface = nullptr;
		jmethodID Constructor_getParameterTypes = nullptr;
		jmethodID Constructor_getModifiers = nullptr;
		jmethodID Method_getParameterTypes = nullptr;
		jmethodID Method_getReturnType = nullptr;
		jmethodID Method_getModifiers = nullptr;
		jmethodID Method_getName = nullptr;
		jmethodID Field_getName = nullptr;
		jmethodID Field_getModifiers = nullptr;
		jmethodID Field_get = nullptr;
		jmethodID Boolean_booleanValue = nullptr;
		jmethodID Byte_byteValue = nullptr;
		jmethodID Character_characterValue = nullptr;
		jmethodID Short_shortValue = nullptr;
		jmethodID Integer_integerValue = nullptr;
		jmethodID Long_longValue = nullptr;
		jmethodID Float_floatValue = nullptr;
		jmethodID Double_doubleValue = nullptr;

		jclass proxy_class = nullptr;
		jmethodID Proxy_isProxyClass = nullptr;

		jclass android_runtime_class = nullptr;
		jmethodID ARP_create_proxy_from_godot_callable = nullptr;
		jmethodID ARP_create_proxy_from_godot_object_id = nullptr;
#endif
		Ref<JavaObject> exception;
	};

	static inline Data* data = nullptr;

private:
#ifdef ANDROID_ENABLED
	static bool _is_proxy_class(JNIEnv* env, jclass p_class);
	static bool _get_type_sig(JNIEnv* env, jobject obj, uint32_t& sig, String& strsig);
	static bool _wrap_class_components(JNIEnv* p_env, const Ref<JavaClass>& p_java_class,
		jclass p_class, bool p_allow_non_public_methods_access);
#endif

	static Ref<JavaClass> _wrap(
		const String& p_class, bool p_allow_non_public_methods_access = false);

public:
	static void initialize();
	static void finalize();
	static _FORCE_INLINE_ bool is_initialized() { return data != nullptr; }

	static Ref<JavaClass> wrap(const String& p_class) { return _wrap(p_class, false); }

	static Ref<JavaObject> get_exception() { return data ? data->exception : Ref<JavaObject>(); }

#ifdef ANDROID_ENABLED
	static Ref<JavaClass> wrap_jclass(
		jclass p_class, bool p_allow_non_public_methods_access = false);
	static Ref<JavaObject> create_sam_callback(
		const String& p_sam_interface, const Callable& p_callable);
	static Ref<JavaObject> create_proxy(
		const Object* p_object, const PackedStringArray& p_interfaces);
#endif

	JavaClassWrapper() = delete;
	JavaClassWrapper(const JavaClassWrapper&) = delete;
	JavaClassWrapper& operator=(const JavaClassWrapper&) = delete;
	~JavaClassWrapper() = delete;
};
// --- End platform/android/api/java_class_wrapper.h ---

// --- Folded from modules/register_module_types.h ---
enum ModuleInitializationLevel {
	MODULE_INITIALIZATION_LEVEL_CORE = 0,
	MODULE_INITIALIZATION_LEVEL_SERVERS = 1,
	MODULE_INITIALIZATION_LEVEL_SCENE = 2,
	MODULE_INITIALIZATION_LEVEL_EDITOR = 3
};

void initialize_modules(ModuleInitializationLevel p_level);
void uninitialize_modules(ModuleInitializationLevel p_level);

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_OPENXR_ENABLED
#define MODULE_OPENXR_HAS_PREREGISTER
#endif
#ifdef MODULE_GLSLANG_ENABLED
#define MODULE_GLSLANG_HAS_PREREGISTER
#endif

inline void initialize_freetype_module(ModuleInitializationLevel p_level) {}
inline void uninitialize_freetype_module(ModuleInitializationLevel p_level) {}
inline void initialize_msdfgen_module(ModuleInitializationLevel p_level) {}
inline void uninitialize_msdfgen_module(ModuleInitializationLevel p_level) {}

void initialize_text_server_adv_module(ModuleInitializationLevel p_level);
void uninitialize_text_server_adv_module(ModuleInitializationLevel p_level);
// --- End modules/register_module_types.h ---

// --- Folded from tests/test_main.h ---
int test_main(int argc, char *argv[]);
// --- End tests/test_main.h ---
