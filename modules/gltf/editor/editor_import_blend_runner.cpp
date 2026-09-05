/**************************************************************************/
/*  editor_import_blend_runner.cpp                                        */
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

#include "core/io/http_client.h"
#include "core/io/xml_parser.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"
#include "editor_import_blend_runner.h"

static constexpr char PYTHON_SCRIPT_RPC[] = R"(
import bpy, sys, threading
from xmlrpc.server import SimpleXMLRPCServer
req = threading.Condition()
res = threading.Condition()
info = None
export_err = None
def xmlrpc_server():
  server = SimpleXMLRPCServer(('127.0.0.1', %d))
  server.register_function(export_gltf)
  server.serve_forever()
def export_gltf(opts):
  with req:
    global info
    info = ('export_gltf', opts)
    req.notify()
  with res:
    res.wait()
  if export_err:
    raise export_err
  # Important to return a value to prevent the error 'cannot marshal None unless allow_none is enabled'.
  return 'BLENDER_GODOT_EXPORT_SUCCESSFUL'
if bpy.app.version < (3, 0, 0):
  print('Blender 3.0 or higher is required.', file=sys.stderr)
threading.Thread(target=xmlrpc_server).start()
while True:
  with req:
    while info is None:
      req.wait()
  method, opts = info
  if method == 'export_gltf':
    try:
      export_err = None
      bpy.ops.wm.open_mainfile(filepath=opts['path'])
      if opts['unpack_all']:
        bpy.ops.file.unpack_all(method='USE_LOCAL')
      bpy.ops.export_scene.gltf(**opts['gltf_options'])
    except Exception as e:
      export_err = e
  info = None
  with res:
    res.notify()
)";

static constexpr char PYTHON_SCRIPT_DIRECT[] = R"(
import bpy, sys
opts = %s
if bpy.app.version < (3, 0, 0):
  print('Blender 3.0 or higher is required.', file=sys.stderr)
bpy.ops.wm.open_mainfile(filepath=opts['path'])
if opts['unpack_all']:
  bpy.ops.file.unpack_all(method='USE_LOCAL')
bpy.ops.export_scene.gltf(**opts['gltf_options'])
)";

bool EditorImportBlendRunner::is_running()
{
	return blender_pid != 0 && OS::get_singleton()->is_process_running(blender_pid);
}

HTTPClient::Status EditorImportBlendRunner::connect_blender_rpc(
	const Ref<HTTPClient>& p_client, int p_timeout_usecs)
{
	p_client->connect_to_host("127.0.0.1", rpc_port);
	HTTPClient::Status status = p_client->get_status();

	int attempts = 1;
	int wait_usecs = 1000;

	bool done = false;
	while (!done) {
		OS::get_singleton()->delay_usec(wait_usecs);
		attempts++;
		status = p_client->get_status();
		switch (status) {
		case HTTPClient::STATUS_RESOLVING:
		case HTTPClient::STATUS_CONNECTING: {
			p_client->poll();
			break;
		}
		case HTTPClient::STATUS_CONNECTED: {
			done = true;
			break;
		}
		default: {
			if (attempts * wait_usecs < p_timeout_usecs) {
				p_client->connect_to_host("127.0.0.1", rpc_port);
			}
			else {
				return status;
			}
		}
		}
	}

	return status;
}

bool EditorImportBlendRunner::_extract_error_message_xml(
	const Vector<uint8_t>& p_response_data, String& r_error_message)
{
	// Based on RPC Xml spec from: https://xmlrpc.com/spec.md
	Ref<XMLParser> parser = memnew(XMLParser);
	Error err = parser->open_buffer(p_response_data);
	if (err) {
		return false;
	}

	r_error_message = String();
	while (parser->read() == OK) {
		if (parser->get_node_type() == XMLParser::NODE_TEXT) {
			if (parser->get_node_data().size()) {
				if (r_error_message.size()) {
					r_error_message += " ";
				}
				r_error_message += parser->get_node_data().trim_suffix("\n");
			}
		}
	}

	return r_error_message.size();
}

void EditorImportBlendRunner::_resources_reimported(const PackedStringArray& p_files)
{
	if (is_running()) {
		// After a batch of imports is done, wait a few seconds before trying to kill blender,
		// in case of having multiple imports trigger in quick succession.
		kill_timer->start();
	}
}

void EditorImportBlendRunner::_kill_blender()
{
	kill_timer->stop();
	if (is_running()) {
		OS::get_singleton()->kill(blender_pid);
	}
	blender_pid = 0;
}

EditorImportBlendRunner* EditorImportBlendRunner::singleton = nullptr;


