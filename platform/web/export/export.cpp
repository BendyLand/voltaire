/**************************************************************************/
/*  export.cpp                                                            */
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

#include "editor/export/editor_export.h"
#include "editor/settings/editor_settings.h"
#include "export.h"
#include "export_plugin.h"

void register_web_exporter_types() {}





// ============================================================================
// Merged from platform/web/export/editor_http_server.cpp
// ============================================================================

#include "editor_http_server.h"

#include "core/io/file_access.h"
#include "core/os/os.h"
#include "editor/file_system/editor_paths.h"

void EditorHTTPServer::_server_thread_poll(void *data) {
	EditorHTTPServer *web_server = static_cast<EditorHTTPServer *>(data);
	while (!web_server->server_quit.is_set()) {
		OS::delay_usec(6900);
		{
			MutexLock lock(web_server->server_lock);
			web_server->_poll();
		}
	}
}

void EditorHTTPServer::_clear_client() {
	peer = Ref<StreamPeer>();
	tls = Ref<StreamPeerTLS>();
	tcp = Ref<StreamPeerSocket>();
	memset(req_buf, 0, sizeof(req_buf));
	time = 0;
	req_pos = 0;
}

void EditorHTTPServer::_set_internal_certs(Ref<Crypto> p_crypto) {
	const String cache_path = EditorPaths::get_cache_dir();
	const String key_path = cache_path.path_join("html5_server.key");
	const String crt_path = cache_path.path_join("html5_server.crt");
	bool regen = !FileAccess::exists(key_path) || !FileAccess::exists(crt_path);
	if (!regen) {
		key = Ref<CryptoKey>(CryptoKey::create());
		cert = Ref<X509Certificate>(X509Certificate::create());
		if (key->load(key_path) != OK || cert->load(crt_path) != OK) {
			regen = true;
		}
	}
	if (regen) {
		key = p_crypto->generate_rsa(2048);
		key->save(key_path);
		cert = p_crypto->generate_self_signed_certificate(key, "CN=godot-debug.local,O=A Game Dev,C=XXA", "20140101000000", "20340101000000");
		cert->save(crt_path);
	}
}

void EditorHTTPServer::_send_response() {
	Vector<String> psa = String((char *)req_buf).split("\r\n");
	int len = psa.size();
	ERR_FAIL_COND_MSG(len < 4, "Not enough response headers, got: " + itos(len) + ", expected >= 4.");

	Vector<String> req = psa[0].split(" ", false);
	ERR_FAIL_COND_MSG(req.size() < 2, "Invalid protocol or status code.");

	// Wrong protocol
	ERR_FAIL_COND_MSG(req[0] != "GET" || req[2] != "HTTP/1.1", "Invalid method or HTTP version.");

	const int query_index = req[1].find_char('?');
	const String path = (query_index == -1) ? req[1] : req[1].substr(0, query_index);

	const String req_file = path.get_file();
	const String req_ext = path.get_extension();
	const String cache_path = EditorPaths::get_temp_dir().path_join("web");
	const String filepath = cache_path.path_join(req_file);

	if (!mimes.has(req_ext) || !FileAccess::exists(filepath)) {
		String s = "HTTP/1.1 404 Not Found\r\n";
		s += "Connection: Close\r\n";
		s += "\r\n";
		CharString cs = s.utf8();
		peer->put_data((const uint8_t *)cs.get_data(), cs.size() - 1);
		return;
	}
	const String ctype = mimes[req_ext];

	Ref<FileAccess> f = FileAccess::open(filepath, FileAccess::READ);
	ERR_FAIL_COND(f.is_null());
	String s = "HTTP/1.1 200 OK\r\n";
	s += "Connection: Close\r\n";
	s += "Content-Type: " + ctype + "\r\n";
	s += "Access-Control-Allow-Origin: *\r\n";
	s += "Cross-Origin-Opener-Policy: same-origin\r\n";
	s += "Cross-Origin-Embedder-Policy: require-corp\r\n";
	s += "Cache-Control: no-store, max-age=0\r\n";
	s += "\r\n";
	CharString cs = s.utf8();
	Error err = peer->put_data((const uint8_t *)cs.get_data(), cs.size() - 1);
	if (err != OK) {
		ERR_FAIL();
	}

	while (true) {
		uint8_t bytes[4096];
		uint64_t read = f->get_buffer(bytes, 4096);
		if (read == 0) {
			break;
		}
		err = peer->put_data(bytes, read);
		if (err != OK) {
			ERR_FAIL();
		}
	}
}

void EditorHTTPServer::_poll() {
	if (!server->is_listening()) {
		return;
	}
	if (tcp.is_null()) {
		if (!server->is_connection_available()) {
			return;
		}
		tcp = server->take_connection();
		peer = tcp;
		time = OS::get_ticks_usec();
	}
	if (OS::get_ticks_usec() - time > 1000000) {
		_clear_client();
		return;
	}
	if (tcp->get_status() != StreamPeerSocket::STATUS_CONNECTED) {
		return;
	}

	if (use_tls) {
		if (tls.is_null()) {
			tls = Ref<StreamPeerTLS>(StreamPeerTLS::create());
			peer = tls;
			if (tls->accept_stream(tcp, TLSOptions::server(key, cert)) != OK) {
				_clear_client();
				return;
			}
		}
		tls->poll();
		if (tls->get_status() == StreamPeerTLS::STATUS_HANDSHAKING) {
			// Still handshaking, keep waiting.
			return;
		}
		if (tls->get_status() != StreamPeerTLS::STATUS_CONNECTED) {
			_clear_client();
			return;
		}
	}

	while (true) {
		char *r = (char *)req_buf;
		int l = req_pos - 1;
		if (l > 3 && r[l] == '\n' && r[l - 1] == '\r' && r[l - 2] == '\n' && r[l - 3] == '\r') {
			_send_response();
			_clear_client();
			return;
		}

		int read = 0;
		ERR_FAIL_COND(req_pos >= 4096);
		Error err = peer->get_partial_data(&req_buf[req_pos], 1, read);
		if (err != OK) {
			// Got an error
			_clear_client();
			return;
		} else if (read != 1) {
			// Busy, wait next poll
			return;
		}
		req_pos += read;
	}
}

void EditorHTTPServer::stop() {
	server_quit.set();
	if (server_thread.is_started()) {
		server_thread.wait_to_finish();
	}
	if (server.is_valid()) {
		server->stop();
	}
	_clear_client();
}

Error EditorHTTPServer::listen(int p_port, IPAddress p_address, bool p_use_tls, String p_tls_key, String p_tls_cert) {
	MutexLock lock(server_lock);
	if (server->is_listening()) {
		return ERR_ALREADY_IN_USE;
	}
	use_tls = p_use_tls;
	if (use_tls) {
		Ref<Crypto> crypto = Crypto::create();
		if (crypto.is_null()) {
			return ERR_UNAVAILABLE;
		}
		if (!p_tls_key.is_empty() && !p_tls_cert.is_empty()) {
			key = Ref<CryptoKey>(CryptoKey::create());
			Error err = key->load(p_tls_key);
			ERR_FAIL_COND_V(err != OK, err);
			cert = Ref<X509Certificate>(X509Certificate::create());
			err = cert->load(p_tls_cert);
			ERR_FAIL_COND_V(err != OK, err);
		} else {
			_set_internal_certs(crypto);
		}
	}
	Error err = server->listen(p_port, p_address);
	if (err == OK) {
		server_quit.clear();
		server_thread.start(_server_thread_poll, this);
	}
	return err;
}

bool EditorHTTPServer::is_listening() const {
	MutexLock lock(server_lock);
	return server->is_listening();
}

EditorHTTPServer::EditorHTTPServer() {
	mimes["html"] = "text/html";
	mimes["js"] = "application/javascript";
	mimes["json"] = "application/json";
	mimes["pck"] = "application/octet-stream";
	mimes["png"] = "image/png";
	mimes["svg"] = "image/svg";
	mimes["wasm"] = "application/wasm";
	server.instantiate();
	stop();
}

EditorHTTPServer::~EditorHTTPServer() {
	stop();
}

// ============================================================================
// Merged from platform/web/export/export_plugin.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/zip_io.h"
#include "core/os/os.h"
#include "editor/editor_string_names.h"
#include "editor/export/editor_export.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "export_plugin.h"
#include "logo_svg.gen.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#include "modules/svg/image_loader_svg.h"
#include "run_icon_svg.gen.h"
#include "scene/resources/image_texture.h"

Error EditorExportPlatformWeb::_extract_template(
	const String& p_template, const String& p_dir, const String& p_name, bool pwa)
{
	Ref<FileAccess> io_fa;
	zlib_filefunc_def io = zipio_create_io(&io_fa);
	unzFile pkg = unzOpen2(p_template.utf8().get_data(), &io);

	if (!pkg) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"),
			vformat(TTR("Could not open template for export: \"%s\"."), p_template));
		return ERR_FILE_NOT_FOUND;
	}

	if (unzGoToFirstFile(pkg) != UNZ_OK) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"),
			vformat(TTR("Invalid export template: \"%s\"."), p_template));
		unzClose(pkg);
		return ERR_FILE_CORRUPT;
	}

	do {
		// get filename
		unz_file_info info;
		char fname[16384];
		unzGetCurrentFileInfo(pkg, &info, fname, 16384, nullptr, 0, nullptr, 0);

		String file = String::utf8(fname);

		// Skip folders.
		if (file.ends_with("/")) {
			continue;
		}

		// Skip service worker and offline page if not exporting pwa.
		if (!pwa && (file == "godot.service.worker.js" || file == "godot.offline.html")) {
			continue;
		}
		Vector<uint8_t> data;
		data.resize(info.uncompressed_size);

		// read
		unzOpenCurrentFile(pkg);
		unzReadCurrentFile(pkg, data.ptrw(), data.size());
		unzCloseCurrentFile(pkg);

		// write
		String dst = p_dir.path_join(file.replace("voltaire", p_name));
		Ref<FileAccess> f = FileAccess::open(dst, FileAccess::WRITE);
		if (f.is_null()) {
			add_message(EXPORT_MESSAGE_ERROR, TTR("Prepare Templates"),
				vformat(TTR("Could not write file: \"%s\"."), dst));
			unzClose(pkg);
			return ERR_FILE_CANT_WRITE;
		}
		f->store_buffer(data.ptr(), data.size());

	} while (unzGoToNextFile(pkg) == UNZ_OK);
	unzClose(pkg);
	return OK;
}

Error EditorExportPlatformWeb::_write_or_error(const uint8_t* p_content, int p_size, String p_path)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.is_null()) {
		add_message(EXPORT_MESSAGE_ERROR, TTR("Export"),
			vformat(TTR("Could not write file: \"%s\"."), p_path));
		return ERR_FILE_CANT_WRITE;
	}
	f->store_buffer(p_content, p_size);
	return OK;
}

void EditorExportPlatformWeb::_replace_strings(
	const HashMap<String, String>& p_replaces, Vector<uint8_t>& r_template)
{
	String str_template =
		String::utf8(reinterpret_cast<const char*>(r_template.ptr()), r_template.size());
	String out;
	Vector<String> lines = str_template.split("\n");
	for (int i = 0; i < lines.size(); i++) {
		String current_line = lines[i];
		for (const KeyValue<String, String>& E : p_replaces) {
			current_line = current_line.replace(E.key, E.value);
		}
		out += current_line + "\n";
	}
	CharString cs = out.utf8();
	r_template.resize(cs.length());
	for (int i = 0; i < cs.length(); i++) {
		r_template.write[i] = cs[i];
	}
}

String EditorExportPlatformWeb::get_name() const { return "Web"; }

String EditorExportPlatformWeb::get_os_name() const { return "Web"; }

Ref<Texture2D> EditorExportPlatformWeb::get_logo() const { return logo; }

List<String> EditorExportPlatformWeb::get_binary_extensions(
	const Ref<EditorExportPreset>& p_preset) const
{
	List<String> list;
	list.push_back("html");
	return list;
}

bool EditorExportPlatformWeb::poll_export()
{
	Ref<EditorExportPreset> preset =
		EditorExport::get_runnable_preset_for_platform(this);

	RemoteDebugState prev_remote_debug_state = remote_debug_state;
	remote_debug_state = REMOTE_DEBUG_STATE_UNAVAILABLE;

	if (preset.is_valid()) {
		const bool debug = true;
		// Throwaway variables to pass to validation functions.
		String err;
		bool missing_templates;

		bool valid = has_valid_export_configuration(preset, err, missing_templates, debug) &&
					 has_valid_project_configuration(preset, err);

		if (valid) {
			if (server->is_listening()) {
				remote_debug_state = REMOTE_DEBUG_STATE_SERVING;
			}
			else {
				remote_debug_state = REMOTE_DEBUG_STATE_AVAILABLE;
			}
		}
	}

	if (remote_debug_state != REMOTE_DEBUG_STATE_SERVING && server->is_listening()) {
		server->stop();
	}

	return remote_debug_state != prev_remote_debug_state;
}

int EditorExportPlatformWeb::get_options_count() const
{
	switch (remote_debug_state) {
	case REMOTE_DEBUG_STATE_UNAVAILABLE: {
		return 0;
	} break;

	case REMOTE_DEBUG_STATE_AVAILABLE: {
		return 2;
	} break;

	case REMOTE_DEBUG_STATE_SERVING: {
		return 3;
	} break;
	}

	return 0;
}

String EditorExportPlatformWeb::get_option_label(int p_index) const
{
	String run_in_browser = TTR("Run in Browser");
	String start_http_server = TTR("Start HTTP Server");
	String reexport_project = TTR("Re-export Project");
	String stop_http_server = TTR("Stop HTTP Server");

	switch (remote_debug_state) {
	case REMOTE_DEBUG_STATE_UNAVAILABLE:
		return "";

	case REMOTE_DEBUG_STATE_AVAILABLE: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return start_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;

	case REMOTE_DEBUG_STATE_SERVING: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return reexport_project;
		case 2:
			return stop_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;
	}

	return "";
}

String EditorExportPlatformWeb::get_option_tooltip(int p_index) const
{
	String run_in_browser = TTR("Run exported HTML in the system's default browser.");
	String start_http_server = TTR("Start the HTTP server.");
	String reexport_project = TTR("Export project again to account for updates.");
	String stop_http_server = TTR("Stop the HTTP server.");

	switch (remote_debug_state) {
	case REMOTE_DEBUG_STATE_UNAVAILABLE:
		return "";

	case REMOTE_DEBUG_STATE_AVAILABLE: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return start_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;

	case REMOTE_DEBUG_STATE_SERVING: {
		switch (p_index) {
		case 0:
			return run_in_browser;
		case 1:
			return reexport_project;
		case 2:
			return stop_http_server;
		default:
			ERR_FAIL_V("");
		}
	} break;
	}

	return "";
}

Error EditorExportPlatformWeb::_export_project(
	const Ref<EditorExportPreset>& p_preset, int p_debug_flags)
{
	const String dest = EditorPaths::get_temp_dir().path_join("web");
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (!da->dir_exists(dest)) {
		Error err = da->make_dir_recursive(dest);
		if (err != OK) {
			add_message(EXPORT_MESSAGE_ERROR, TTR("Run"),
				vformat(TTR("Could not create HTTP server directory: %s."), dest));
			return err;
		}
	}

	const String basepath = dest.path_join("tmp_js_export");
	Error err = export_project(p_preset, true, basepath + ".html", p_debug_flags);
	if (err != OK) {
		// Export generates several files, clean them up on failure.
		DirAccess::remove_file_or_error(basepath + ".html");
		DirAccess::remove_file_or_error(basepath + ".offline.html");
		DirAccess::remove_file_or_error(basepath + ".js");
		DirAccess::remove_file_or_error(basepath + ".audio.worklet.js");
		DirAccess::remove_file_or_error(basepath + ".audio.position.worklet.js");
		DirAccess::remove_file_or_error(basepath + ".service.worker.js");
		DirAccess::remove_file_or_error(basepath + ".pck");
		DirAccess::remove_file_or_error(basepath + ".png");
		DirAccess::remove_file_or_error(basepath + ".side.wasm");
		DirAccess::remove_file_or_error(basepath + ".wasm");
		DirAccess::remove_file_or_error(basepath + ".icon.png");
		DirAccess::remove_file_or_error(basepath + ".apple-touch-icon.png");
	}
	return err;
}

Error EditorExportPlatformWeb::_launch_browser(
	const String& p_bind_host, const uint16_t p_bind_port, const bool p_use_tls)
{
	OS::shell_open(String((p_use_tls ? "https://" : "http://") + p_bind_host +
										   ":" + itos(p_bind_port) + "/tmp_js_export.html"));
	// FIXME: Find out how to clean up export files after running the successfully
	// exported game. Might not be trivial.
	return OK;
}

Error EditorExportPlatformWeb::_stop_server()
{
	server->stop();
	return OK;
}

Ref<Texture2D> EditorExportPlatformWeb::get_run_icon() const { return run_icon; }

void register_web_exporter() {}


