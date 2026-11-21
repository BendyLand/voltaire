/**************************************************************************/
/*  http_client.h                                                         */
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

#include "core/crypto/crypto.h"
#include "core/io/stream_peer.h"

#ifndef WEB_ENABLED
#include "core/io/ip.h"

class StreamPeerSocket;
#endif

class HTTPClient
#ifndef WEB_ENABLED
	final
#endif
	: public RefCounted
{
public:
	enum ResponseCode
	{
		// 1xx informational
		RESPONSE_CONTINUE = 100,
		RESPONSE_SWITCHING_PROTOCOLS = 101,
		RESPONSE_PROCESSING = 102,

		// 2xx successful
		RESPONSE_OK = 200,
		RESPONSE_CREATED = 201,
		RESPONSE_ACCEPTED = 202,
		RESPONSE_NON_AUTHORITATIVE_INFORMATION = 203,
		RESPONSE_NO_CONTENT = 204,
		RESPONSE_RESET_CONTENT = 205,
		RESPONSE_PARTIAL_CONTENT = 206,
		RESPONSE_MULTI_STATUS = 207,
		RESPONSE_ALREADY_REPORTED = 208,
		RESPONSE_IM_USED = 226,

		// 3xx redirection
		RESPONSE_MULTIPLE_CHOICES = 300,
		RESPONSE_MOVED_PERMANENTLY = 301,
		RESPONSE_FOUND = 302,
		RESPONSE_SEE_OTHER = 303,
		RESPONSE_NOT_MODIFIED = 304,
		RESPONSE_USE_PROXY = 305,
		RESPONSE_SWITCH_PROXY = 306,
		RESPONSE_TEMPORARY_REDIRECT = 307,
		RESPONSE_PERMANENT_REDIRECT = 308,

		// 4xx client error
		RESPONSE_BAD_REQUEST = 400,
		RESPONSE_UNAUTHORIZED = 401,
		RESPONSE_PAYMENT_REQUIRED = 402,
		RESPONSE_FORBIDDEN = 403,
		RESPONSE_NOT_FOUND = 404,
		RESPONSE_METHOD_NOT_ALLOWED = 405,
		RESPONSE_NOT_ACCEPTABLE = 406,
		RESPONSE_PROXY_AUTHENTICATION_REQUIRED = 407,
		RESPONSE_REQUEST_TIMEOUT = 408,
		RESPONSE_CONFLICT = 409,
		RESPONSE_GONE = 410,
		RESPONSE_LENGTH_REQUIRED = 411,
		RESPONSE_PRECONDITION_FAILED = 412,
		RESPONSE_REQUEST_ENTITY_TOO_LARGE = 413,
		RESPONSE_REQUEST_URI_TOO_LONG = 414,
		RESPONSE_UNSUPPORTED_MEDIA_TYPE = 415,
		RESPONSE_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
		RESPONSE_EXPECTATION_FAILED = 417,
		RESPONSE_IM_A_TEAPOT = 418,
		RESPONSE_MISDIRECTED_REQUEST = 421,
		RESPONSE_UNPROCESSABLE_ENTITY = 422,
		RESPONSE_LOCKED = 423,
		RESPONSE_FAILED_DEPENDENCY = 424,
		RESPONSE_UPGRADE_REQUIRED = 426,
		RESPONSE_PRECONDITION_REQUIRED = 428,
		RESPONSE_TOO_MANY_REQUESTS = 429,
		RESPONSE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
		RESPONSE_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

		// 5xx server error
		RESPONSE_INTERNAL_SERVER_ERROR = 500,
		RESPONSE_NOT_IMPLEMENTED = 501,
		RESPONSE_BAD_GATEWAY = 502,
		RESPONSE_SERVICE_UNAVAILABLE = 503,
		RESPONSE_GATEWAY_TIMEOUT = 504,
		RESPONSE_HTTP_VERSION_NOT_SUPPORTED = 505,
		RESPONSE_VARIANT_ALSO_NEGOTIATES = 506,
		RESPONSE_INSUFFICIENT_STORAGE = 507,
		RESPONSE_LOOP_DETECTED = 508,
		RESPONSE_NOT_EXTENDED = 510,
		RESPONSE_NETWORK_AUTH_REQUIRED = 511,
	};

	enum Method
	{
		METHOD_GET,
		METHOD_HEAD,
		METHOD_POST,
		METHOD_PUT,
		METHOD_DELETE,
		METHOD_OPTIONS,
		METHOD_TRACE,
		METHOD_CONNECT,
		METHOD_PATCH,
		METHOD_MAX
	};

	enum Status
	{
		STATUS_DISCONNECTED,
		STATUS_RESOLVING, // Resolving hostname (if passed a hostname)
		STATUS_CANT_RESOLVE,
		STATUS_CONNECTING, // Connecting to IP
		STATUS_CANT_CONNECT,
		STATUS_CONNECTED,  // Connected, requests can be made
		STATUS_REQUESTING, // Request in progress
		STATUS_BODY,	   // Request resulted in body, which must be read
		STATUS_CONNECTION_ERROR,
		STATUS_TLS_HANDSHAKE_ERROR,
	};

protected:
	static const char* _methods[METHOD_MAX];
	static const int HOST_MIN_LEN = 4;

	enum Port
	{
		PORT_HTTP = 80,
		PORT_HTTPS = 443,
	};

	Vector<String> _get_response_headers();
	Error _request_raw(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const Vector<uint8_t>& p_body);
	Error _request(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const String& p_body = String());

#ifdef WEB_ENABLED
	static HTTPClient* (*_create)(bool p_notify_postinitialize);
#else
private:
	Status status = STATUS_DISCONNECTED;
	IP::ResolverID resolving = IP::RESOLVER_INVALID_ID;
	int conn_port = -1; // Server to make requests to.
	String conn_host;
	int server_port = -1; // Server to connect to (might be a proxy server).
	String server_host;
	int http_proxy_port = -1; // Proxy server for http requests.
	String http_proxy_host;
	int https_proxy_port = -1; // Proxy server for https requests.
	String https_proxy_host;
	bool blocking = false;
	bool handshaking = false;
	bool head_request = false;
	Ref<TLSOptions> tls_options;

	Vector<uint8_t> response_str;

	bool chunked = false;
	Vector<uint8_t> chunk;
	int chunk_left = 0;
	bool chunk_trailer_part = false;
	int64_t body_size = -1;
	int64_t body_left = 0;
	bool read_until_eof = false;

	Ref<StreamPeerBuffer> request_buffer;
	Ref<StreamPeerSocket> tcp_connection;
	Ref<StreamPeer> connection;
	Ref<HTTPClient> proxy_client; // Negotiate with proxy server.

	int response_num = 0;
	Vector<String> response_headers;
	// 64 KiB by default (favors fast download speeds at the cost of memory usage).
	int read_chunk_size = 65536;

	Error _get_http_data(uint8_t* p_buffer, int p_bytes, int& r_received);
#endif

public:
	static Ref<HTTPClient> create(bool p_notify_postinitialize = true);

	Error verify_headers(const Vector<String>& p_headers);

#ifdef WEB_ENABLED
	virtual Error request(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const uint8_t* p_body, int p_body_size) = 0;
	virtual Error connect_to_host(const String& p_host, int p_port = -1,
		Ref<TLSOptions> p_tls_options = Ref<TLSOptions>()) = 0;

	virtual void set_connection(const Ref<StreamPeer>& p_connection) = 0;
	virtual Ref<StreamPeer> get_connection() const = 0;

	virtual void close() = 0;

	virtual Status get_status() const = 0;

	virtual bool has_response() const = 0;
	virtual bool is_response_chunked() const = 0;
	virtual int get_response_code() const = 0;
	virtual Error get_response_headers(List<String>* r_response) = 0;
	virtual int64_t get_response_body_length() const = 0;

	virtual Vector<uint8_t> read_response_body_chunk() = 0;

	virtual void set_blocking_mode(bool p_enable) = 0;
	virtual bool is_blocking_mode_enabled() const = 0;

	virtual void set_read_chunk_size(int p_size) = 0;
	virtual int get_read_chunk_size() const = 0;

	virtual Error poll() = 0;

	// Use empty string or -1 to unset
	virtual void set_http_proxy(const String& p_host, int p_port);
	virtual void set_https_proxy(const String& p_host, int p_port);

	virtual ~HTTPClient() {}
#else
	Error request(Method p_method, const String& p_url, const Vector<String>& p_headers,
		const uint8_t* p_body, int p_body_size);
	Error connect_to_host(const String& p_host, int p_port = -1,
		Ref<TLSOptions> p_tls_options = Ref<TLSOptions>());

	void set_connection(const Ref<StreamPeer>& p_connection);
	Ref<StreamPeer> get_connection() const;

	void close();

	Status get_status() const;

	bool has_response() const;
	bool is_response_chunked() const;
	int get_response_code() const;
	Error get_response_headers(List<String>* r_response);
	int64_t get_response_body_length() const;

	Vector<uint8_t> read_response_body_chunk();

	void set_blocking_mode(bool p_enable);
	bool is_blocking_mode_enabled() const;

	void set_read_chunk_size(int p_size);
	int get_read_chunk_size() const;

	Error poll();

	// Use empty string or -1 to unset
	void set_http_proxy(const String& p_host, int p_port);
	void set_https_proxy(const String& p_host, int p_port);

	HTTPClient();
	virtual ~HTTPClient();
#endif
};
