/**************************************************************************/
/*  register_core_types.cpp                                               */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/core_bind.h"
#include "core/crypto/aes_context.h"
#include "core/crypto/crypto.h"
#include "core/crypto/crypto_resource_format.h"
#include "core/crypto/hashing_context.h"
#include "core/debugger/engine_profiler.h"
#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/input/shortcut.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/dtls_server.h"
#include "core/io/http_client.h"
#include "core/io/image_loader.h"
#include "core/io/image_resource_format.h"
#include "core/io/json.h"
#include "core/io/json_resource_format.h"
#include "core/io/marshalls.h"
#include "core/io/missing_resource.h"
#include "core/io/packet_peer.h"
#include "core/io/packet_peer_dtls.h"
#include "core/io/packet_peer_udp.h"
#include "core/io/pck_packer.h"
#include "core/io/resource_format_binary.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/io/resource_uid.h"
#include "core/io/stream_peer_gzip.h"
#include "core/io/stream_peer_tls.h"
#include "core/io/tcp_server.h"
#include "core/io/translation_loader_po.h"
#include "core/io/udp_server.h"
#include "core/io/uds_server.h"
#include "core/io/xml_parser.h"
#include "core/math/a_star.h"
#include "core/math/a_star_grid_2d.h"
#include "core/math/expression.h"
#include "core/math/random_number_generator.h"
#include "core/math/triangle_mesh.h"
#include "core/object/class_db.h"
#include "core/object/script_backtrace.h"
#include "core/object/undo_redo.h"
#include "core/object/worker_thread_pool.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/os/time.h"
#include "core/string/fuzzy_search.h"
#include "core/string/optimized_translation.h"
#include "core/string/translation.h"
#include "core/string/translation_server.h"
#include "register_core_types.h"
#ifndef DISABLE_DEPRECATED
#include "core/io/packed_data_container.h"
#endif

static Ref<ResourceFormatSaverBinary> resource_saver_binary;
static Ref<ResourceFormatLoaderBinary> resource_loader_binary;
static Ref<ResourceFormatImporter> resource_format_importer;
static Ref<ResourceFormatImporterSaver> resource_format_importer_saver;
static Ref<ResourceFormatLoaderImage> resource_format_image;
static Ref<TranslationLoaderPO> resource_format_po;
static Ref<ResourceFormatSaverCrypto> resource_format_saver_crypto;
static Ref<ResourceFormatLoaderCrypto> resource_format_loader_crypto;
static Ref<ResourceFormatSaverJSON> resource_saver_json;
static Ref<ResourceFormatLoaderJSON> resource_loader_json;

static CoreBind::ResourceLoader* _resource_loader = nullptr;
static CoreBind::ResourceSaver* _resource_saver = nullptr;
static CoreBind::OS* _os = nullptr;
static CoreBind::Engine* _engine = nullptr;
static CoreBind::Marshalls* _marshalls = nullptr;
static CoreBind::EngineDebugger* _engine_debugger = nullptr;

static IP* ip = nullptr;
static Time* _time = nullptr;

static CoreBind::Geometry2D* _geometry_2d = nullptr;
static CoreBind::Geometry3D* _geometry_3d = nullptr;

static WorkerThreadPool* worker_thread_pool = nullptr;

extern Mutex _global_mutex;

extern void register_global_constants();
extern void unregister_global_constants();

static ResourceUID* resource_uid = nullptr;

static bool _is_core_extensions_registered = false;

void register_core_types()
{
	OS::get_singleton()->benchmark_begin_measure("Core", "Register Types");

	// consistency check
	static_assert(sizeof(Callable) <= 16);

	CryptoCore::initialize();

	ObjectDB::setup();
	StringName::setup();
	register_global_constants();
	CoreStringNames::create();

	_time = memnew(Time);
	ResourceLoader::initialize();

	Variant::register_types();

	resource_format_po.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_po);

	resource_saver_binary.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_binary);
	resource_loader_binary.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_binary);

	resource_format_importer.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_importer);

	resource_format_importer_saver.instantiate();
	ResourceSaver::add_resource_format_saver(resource_format_importer_saver);

	resource_format_image.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_image);

	resource_format_saver_crypto.instantiate();
	ResourceSaver::add_resource_format_saver(resource_format_saver_crypto);

	resource_format_loader_crypto.instantiate();
	ResourceLoader::add_resource_format_loader(resource_format_loader_crypto);

	resource_saver_json.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_json);

	resource_loader_json.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_json);

#ifndef DISABLE_DEPRECATED
#endif

	resource_uid = memnew(ResourceUID);

	ip = IP::create();

	_geometry_2d = memnew(CoreBind::Geometry2D);
	_geometry_3d = memnew(CoreBind::Geometry3D);

	_resource_loader = memnew(CoreBind::ResourceLoader);
	_resource_saver = memnew(CoreBind::ResourceSaver);
	_os = memnew(CoreBind::OS);
	_engine = memnew(CoreBind::Engine);
	_marshalls = memnew(CoreBind::Marshalls);
	_engine_debugger = memnew(CoreBind::EngineDebugger);

	worker_thread_pool = memnew(WorkerThreadPool);

	OS::get_singleton()->benchmark_end_measure("Core", "Register Types");
}

void register_core_settings()
{
	// Since in register core types, globals may not be present.
	GLOBAL_DEF(PropertyInfo(Variant::INT, "network/limits/tcp/connect_timeout_seconds",
				   PROPERTY_HINT_RANGE, "1,1800,1"),
		(30));
	GLOBAL_DEF(PropertyInfo(Variant::INT, "network/limits/unix/connect_timeout_seconds",
				   PROPERTY_HINT_RANGE, "1,1800,1"),
		(30));
	GLOBAL_DEF_RST(PropertyInfo(Variant::INT, "network/limits/packet_peer_stream/max_buffer_po2",
					   PROPERTY_HINT_RANGE, "8,64,1,or_greater"),
		(16));
	GLOBAL_DEF(PropertyInfo(Variant::STRING, "network/tls/certificate_bundle_override",
				   PROPERTY_HINT_FILE, "*.crt"),
		"");

	GLOBAL_DEF("threading/worker_pool/max_threads", -1);
	GLOBAL_DEF("threading/worker_pool/low_priority_thread_ratio", 0.3);
}

void register_early_core_singletons()
{
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("Engine", CoreBind::Engine::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("ProjectSettings", ProjectSettings::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("OS", CoreBind::OS::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Time", Time::get_singleton()));
}

void register_core_singletons()
{
	OS::get_singleton()->benchmark_begin_measure("Core", "Register Singletons");

	Engine::get_singleton()->add_singleton(Engine::Singleton("IP", IP::get_singleton(), "IP"));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("Geometry2D", CoreBind::Geometry2D::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("Geometry3D", CoreBind::Geometry3D::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("ResourceLoader", CoreBind::ResourceLoader::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("ResourceSaver", CoreBind::ResourceSaver::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("Marshalls", CoreBind::Marshalls::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("TranslationServer", TranslationServer::get_singleton()->obj.get()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("Input", Input::get_singleton()->obj.get()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("InputMap", InputMap::get_singleton()->obj.get()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("EngineDebugger", CoreBind::EngineDebugger::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("ResourceUID", ResourceUID::get_singleton()));
	Engine::get_singleton()->add_singleton(
		Engine::Singleton("WorkerThreadPool", worker_thread_pool));

	OS::get_singleton()->benchmark_end_measure("Core", "Register Singletons");
}

void register_core_extensions()
{
	OS::get_singleton()->benchmark_begin_measure("Core", "Register Extensions");

	// Hardcoded for now.
	_is_core_extensions_registered = true;

	OS::get_singleton()->benchmark_end_measure("Core", "Register Extensions");
}

void unregister_core_extensions()
{
	OS::get_singleton()->benchmark_begin_measure("Core", "Unregister Extensions");
	OS::get_singleton()->benchmark_end_measure("Core", "Unregister Extensions");
}

void unregister_core_types()
{
	OS::get_singleton()->benchmark_begin_measure("Core", "Unregister Types");

	// Destroy singletons in reverse order to ensure dependencies are not broken.

	memdelete(worker_thread_pool);

	memdelete(_engine_debugger);
	memdelete(_marshalls);
	memdelete(_engine);
	memdelete(_os);
	memdelete(_resource_saver);
	memdelete(_resource_loader);

	memdelete(_geometry_3d);
	memdelete(_geometry_2d);

	memdelete(resource_uid);

	memdelete(ip);

	ResourceLoader::remove_resource_format_loader(resource_format_image);
	resource_format_image.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_binary);
	resource_saver_binary.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_binary);
	resource_loader_binary.unref();

	ResourceLoader::remove_resource_format_loader(resource_format_importer);
	resource_format_importer.unref();

	ResourceSaver::remove_resource_format_saver(resource_format_importer_saver);
	resource_format_importer_saver.unref();

	ResourceLoader::remove_resource_format_loader(resource_format_po);
	resource_format_po.unref();

	ResourceSaver::remove_resource_format_saver(resource_format_saver_crypto);
	resource_format_saver_crypto.unref();

	ResourceLoader::remove_resource_format_loader(resource_format_loader_crypto);
	resource_format_loader_crypto.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_json);
	resource_saver_json.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_json);
	resource_loader_json.unref();

	ResourceLoader::finalize();

	memdelete(_time);
	ObjectDB::cleanup();

	CryptoCore::finalize();

	Variant::unregister_types();

	unregister_global_constants();

	ResourceCache::clear();
	CoreStringNames::free();
	StringName::cleanup();

	OS::get_singleton()->benchmark_end_measure("Core", "Unregister Types");
}


