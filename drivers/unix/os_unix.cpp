/**************************************************************************/
/*  os_unix.cpp                                                           */
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

#include "os_unix.h"

#ifdef UNIX_ENABLED

#include "drivers/unix/dir_access_unix.h"
#include "drivers/unix/os_unix.h"
// --- Inlined from drivers/unix/file_access_unix_pipe.h ---
#if defined(UNIX_ENABLED)

#include "core/io/file_access.h"

class FileAccessUnixPipe : public FileAccess
{
	bool unlink_on_close = false;

	int fd[2] = {-1, -1};

	mutable Error last_error = OK;
	String path;
	String path_src;

	void _close();

public:
	Error open_existing(int p_rfd, int p_wfd, bool p_blocking);
	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file

	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String
	get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override {}

	virtual void seek_end(int64_t p_position = 0) override {}

	virtual uint64_t get_position() const override { return 0; }

	virtual uint64_t get_length() const override;

	virtual bool eof_reached() const override { return false; }

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override {}

	virtual bool store_buffer(
		const uint8_t* p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String& p_path) override { return false; }

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(
		const String& p_file) override
	{
		return 0;
	}

	virtual Error _set_unix_permissions(
		const String& p_file, uint32_t p_permissions) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual void close() override;

	FileAccessUnixPipe() {}

	virtual ~FileAccessUnixPipe();
};

#endif // UNIX_ENABLED



// --- End drivers/unix/file_access_unix_pipe.h ---
// --- Inlined from drivers/unix/thread_posix.h ---
void init_thread_posix();

// --- End drivers/unix/thread_posix.h ---

#ifndef UNIX_SOCKET_UNAVAILABLE
// --- Inlined from drivers/unix/ip_unix.h ---
#if defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE)

#include "core/io/ip.h"

class IPUnix final
{
public:
	static void _resolve_hostname(
		List<IPAddress>& r_addresses, const String& p_hostname, IP::Type p_type = IP::TYPE_ANY);
	static void get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces);

	static void make_default();

	IPUnix() = delete;
	IPUnix(const IPUnix&) = delete;
	IPUnix& operator=(const IPUnix&) = delete;
	~IPUnix() = delete;
};

#endif // UNIX_ENABLED



// --- End drivers/unix/ip_unix.h ---
#include "drivers/unix/os_unix.h"
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if defined(__FreeBSD__)
#include <kvm.h>
#endif

#if defined(__OpenBSD__)
#include <sys/swap.h>
#include <uvm/uvmexp.h>
#endif

#if defined(__NetBSD__)
#include <uvm/uvm_extern.h>
#endif

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <dlfcn.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef RTLD_DEEPBIND
#define RTLD_DEEPBIND 0
#endif

#ifndef ASAN_ENABLED
#define GODOT_DLOPEN_MODE RTLD_NOW | RTLD_DEEPBIND
#else
#define GODOT_DLOPEN_MODE RTLD_NOW
#endif

#if defined(MACOS_ENABLED) || (defined(__ANDROID_API__) && __ANDROID_API__ >= 28)
// Random location for getentropy. Fitting.
#include <sys/random.h>
#define UNIX_GET_ENTROPY
#elif defined(__FreeBSD__) || defined(__OpenBSD__) ||                                              \
	(defined(__GLIBC_MINOR__) && (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 26))
// In <unistd.h>.
// One day... (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 700)
// https://publications.opengroup.org/standards/unix/c211
#define UNIX_GET_ENTROPY
#endif

#if !defined(UNIX_GET_ENTROPY) && !defined(NO_URANDOM)
#include <fcntl.h>
#endif

/// Clock Setup function (used by get_ticks_usec)
static uint64_t _clock_start = 0;
#if defined(__APPLE__)
static double _clock_scale = 0;

static void _setup_clock()
{
	mach_timebase_info_data_t info;
	kern_return_t ret = mach_timebase_info(&info);
	ERR_FAIL_COND_MSG(ret != 0, "OS CLOCK IS NOT WORKING!");
	_clock_scale = ((double)info.numer / (double)info.denom) / 1000.0;
	_clock_start = mach_absolute_time() * _clock_scale;
}
#else
#if defined(CLOCK_MONOTONIC_RAW) && !defined(WEB_ENABLED) // This is a better clock on Linux.
#define GODOT_CLOCK CLOCK_MONOTONIC_RAW
#else
#define GODOT_CLOCK CLOCK_MONOTONIC
#endif
static void _setup_clock()
{
	struct timespec tv_now = {0, 0};
	ERR_FAIL_COND_MSG(clock_gettime(GODOT_CLOCK, &tv_now) != 0, "OS CLOCK IS NOT WORKING!");
	_clock_start = ((uint64_t)tv_now.tv_nsec / 1000L) + (uint64_t)tv_now.tv_sec * 1000000L;
}
#endif

struct sigaction old_action;

static void handle_interrupt(int sig)
{
	// Ensure we call the old action if it was configured.
	if (old_action.sa_handler && old_action.sa_handler != SIG_IGN &&
		old_action.sa_handler != SIG_DFL) {
		old_action.sa_handler(sig);
	}
}

void OS_Unix::initialize_debugging()
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = handle_interrupt;
	sigaction(SIGINT, &action, &old_action);
}

int OS_Unix::unix_initialize_audio(int p_audio_driver) { return 0; }

void OS_Unix::initialize_core()
{
#ifdef THREADS_ENABLED
	init_thread_posix();
#endif

	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
	FileAccess::make_default<FileAccessUnixPipe>(FileAccess::ACCESS_PIPE);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_FILESYSTEM);

#ifndef UNIX_SOCKET_UNAVAILABLE
	NetSocketUnix::make_default();
	IPUnix::make_default();
#endif
	process_map = memnew((HashMap<ProcessID, ProcessInfo>));

	_setup_clock();
}

void OS_Unix::finalize_core()
{
    if (process_map) {
        memdelete(process_map);
        process_map = nullptr;
    }
#ifndef UNIX_SOCKET_UNAVAILABLE
	IP::finalize();
	IP::reset_backend();
	NetSocketUnix::cleanup();
#endif
}

Vector<String> OS_Unix::get_video_adapter_driver_info() { return Vector<String>(); }

String OS_Unix::get_stdin_string(int64_t p_buffer_size)
{
	Vector<uint8_t> data;
	data.resize(p_buffer_size);
	if (fgets((char*)data.ptrw(), data.size(), stdin)) {
		return String::utf8((char*)data.ptr()).replace("\r\n", "\n").rstrip("\n");
	}
	return String();
}

PackedByteArray OS_Unix::get_stdin_buffer(int64_t p_buffer_size)
{
	Vector<uint8_t> data;
	data.resize(p_buffer_size);
	size_t sz = fread((void*)data.ptrw(), 1, data.size(), stdin);
	if (sz > 0) {
		data.resize(sz);
		return data;
	}
	return PackedByteArray();
}

OS::StdHandleType OS_Unix::get_stdin_type()
{
	int h = fileno(stdin);
	if (h == -1) {
		return OS::STD_HANDLE_INVALID;
	}

	if (isatty(h)) {
		return OS::STD_HANDLE_CONSOLE;
	}
	struct stat statbuf;
	if (fstat(h, &statbuf) < 0) {
		return OS::STD_HANDLE_UNKNOWN;
	}
	if (S_ISFIFO(statbuf.st_mode)) {
		return OS::STD_HANDLE_PIPE;
	}
	else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
		return OS::STD_HANDLE_FILE;
	}
	return OS::STD_HANDLE_UNKNOWN;
}

OS::StdHandleType OS_Unix::get_stdout_type()
{
	int h = fileno(stdout);
	if (h == -1) {
		return OS::STD_HANDLE_INVALID;
	}

	if (isatty(h)) {
		return OS::STD_HANDLE_CONSOLE;
	}
	struct stat statbuf;
	if (fstat(h, &statbuf) < 0) {
		return OS::STD_HANDLE_UNKNOWN;
	}
	if (S_ISFIFO(statbuf.st_mode)) {
		return OS::STD_HANDLE_PIPE;
	}
	else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
		return OS::STD_HANDLE_FILE;
	}
	return OS::STD_HANDLE_UNKNOWN;
}

OS::StdHandleType OS_Unix::get_stderr_type()
{
	int h = fileno(stderr);
	if (h == -1) {
		return OS::STD_HANDLE_INVALID;
	}

	if (isatty(h)) {
		return OS::STD_HANDLE_CONSOLE;
	}
	struct stat statbuf;
	if (fstat(h, &statbuf) < 0) {
		return OS::STD_HANDLE_UNKNOWN;
	}
	if (S_ISFIFO(statbuf.st_mode)) {
		return OS::STD_HANDLE_PIPE;
	}
	else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
		return OS::STD_HANDLE_FILE;
	}
	return OS::STD_HANDLE_UNKNOWN;
}

Error OS_Unix::get_entropy(uint8_t* r_buffer, int p_bytes)
{
#if defined(UNIX_GET_ENTROPY)
	int left = p_bytes;
	int ofs = 0;
	do {
		int chunk = MIN(left, 256);
		ERR_FAIL_COND_V(getentropy(r_buffer + ofs, chunk), FAILED);
		left -= chunk;
		ofs += chunk;
	} while (left > 0);
// Define this yourself if you don't want to fall back to /dev/urandom.
#elif !defined(NO_URANDOM)
	int r = open("/dev/urandom", O_RDONLY);
	ERR_FAIL_COND_V(r < 0, FAILED);
	int left = p_bytes;
	do {
		ssize_t ret = read(r, r_buffer, p_bytes);
		ERR_FAIL_COND_V(ret <= 0, FAILED);
		left -= ret;
	} while (left > 0);
#else
	return ERR_UNAVAILABLE;
#endif
	return OK;
}

String OS_Unix::get_name() { return "Unix"; }

String OS_Unix::get_distribution_name() { return ""; }

String OS_Unix::get_version() { return ""; }

String OS_Unix::get_temp_path() { return "/tmp"; }

double OS_Unix::get_unix_time()
{
	struct timeval tv_now;
	gettimeofday(&tv_now, nullptr);
	return (double)tv_now.tv_sec + double(tv_now.tv_usec) / 1000000;
}

OS::DateTime OS_Unix::get_datetime(bool p_utc)
{
	time_t t = time(nullptr);
	struct tm lt;
	if (p_utc) {
		gmtime_r(&t, &lt);
	}
	else {
		localtime_r(&t, &lt);
	}
	OS::DateTime ret;
	ret.year = 1900 + lt.tm_year;
	// Index starting at 1 to match OS_Unix::get_date
	//   and Windows SYSTEMTIME and tm_mon follows the typical structure
	//   of 0-11, noted here: http://www.cplusplus.com/reference/ctime/tm/
	ret.month = (Month)(lt.tm_mon + 1);
	ret.day = lt.tm_mday;
	ret.weekday = (Weekday)lt.tm_wday;
	ret.hour = lt.tm_hour;
	ret.minute = lt.tm_min;
	ret.second = lt.tm_sec;
	ret.dst = lt.tm_isdst;

	return ret;
}

OS::TimeZoneInfo OS_Unix::get_time_zone_info()
{
	time_t t = time(nullptr);
	struct tm lt;
	localtime_r(&t, &lt);
	char name[16];
	strftime(name, 16, "%Z", &lt);
	name[15] = 0;
	OS::TimeZoneInfo ret;
	ret.name = name;

	char bias_buf[16];
	strftime(bias_buf, 16, "%z", &lt);
	int bias;
	bias_buf[15] = 0;
	sscanf(bias_buf, "%d", &bias);

	// convert from ISO 8601 (1 minute=1, 1 hour=100) to minutes
	int hour = (int)bias / 100;
	int minutes = bias % 100;
	if (bias < 0) {
		ret.bias = hour * 60 - minutes;
	}
	else {
		ret.bias = hour * 60 + minutes;
	}

	return ret;
}

void OS_Unix::delay_usec(uint32_t p_usec)
{
	struct timespec requested = {
		static_cast<time_t>(p_usec / 1000000), (static_cast<long>(p_usec) % 1000000) * 1000};
	struct timespec remaining;
	while (nanosleep(&requested, &remaining) == -1 && errno == EINTR) {
		requested.tv_sec = remaining.tv_sec;
		requested.tv_nsec = remaining.tv_nsec;
	}
}

uint64_t OS_Unix::get_ticks_usec()
{
#if defined(__APPLE__)
	uint64_t longtime = mach_absolute_time() * _clock_scale;
#else
	// Unchecked return. Static analyzers might complain.
	// If _setup_clock() succeeded, we assume clock_gettime() works.
	struct timespec tv_now = {0, 0};
	clock_gettime(GODOT_CLOCK, &tv_now);
	uint64_t longtime = ((uint64_t)tv_now.tv_nsec / 1000L) + (uint64_t)tv_now.tv_sec * 1000000L;
#endif
	longtime -= _clock_start;

	return longtime;
}

#if !defined(__GLIBC__) && !defined(WEB_ENABLED)
void OS_Unix::_load_iconv()
{
#if defined(MACOS_ENABLED) || defined(IOS_ENABLED)
	String iconv_lib_aliases[] = {"/usr/lib/libiconv.2.dylib"};
	String iconv_func_aliases[] = {"iconv"};
	String charset_lib_aliases[] = {"/usr/lib/libcharset.1.dylib"};
#else
	String iconv_lib_aliases[] = {"", "libiconv.2.so", "libiconv.so"};
	String iconv_func_aliases[] = {"libiconv", "iconv", "bsd_iconv", "rpl_iconv"};
	String charset_lib_aliases[] = {"", "libcharset.1.so", "libcharset.so"};
#endif

	for (size_t i = 0; i < sizeof(iconv_lib_aliases) / sizeof(iconv_lib_aliases[0]); i++) {
		void* iconv_lib = iconv_lib_aliases[i].is_empty()
							  ? RTLD_NEXT
							  : dlopen(iconv_lib_aliases[i].utf8().get_data(), RTLD_NOW);
		if (iconv_lib) {
			for (size_t j = 0; j < sizeof(iconv_func_aliases) / sizeof(iconv_func_aliases[0]);
				 j++) {
				gd_iconv_open = (PIConvOpen)dlsym(
					iconv_lib, (iconv_func_aliases[j] + "_open").utf8().get_data());
				gd_iconv = (PIConv)dlsym(iconv_lib, (iconv_func_aliases[j]).utf8().get_data());
				gd_iconv_close = (PIConvClose)dlsym(
					iconv_lib, (iconv_func_aliases[j] + "_close").utf8().get_data());
				if (gd_iconv_open && gd_iconv && gd_iconv_close) {
					break;
				}
			}
			if (gd_iconv_open && gd_iconv && gd_iconv_close) {
				break;
			}
			if (!iconv_lib_aliases[i].is_empty()) {
				dlclose(iconv_lib);
			}
		}
	}

	for (size_t i = 0; i < sizeof(charset_lib_aliases) / sizeof(charset_lib_aliases[0]); i++) {
		void* cs_lib = charset_lib_aliases[i].is_empty()
						   ? RTLD_NEXT
						   : dlopen(charset_lib_aliases[i].utf8().get_data(), RTLD_NOW);
		if (cs_lib) {
			gd_locale_charset = (PIConvLocaleCharset)dlsym(cs_lib, "locale_charset");
			if (gd_locale_charset) {
				break;
			}
			if (!charset_lib_aliases[i].is_empty()) {
				dlclose(cs_lib);
			}
		}
	}
	_iconv_ok = gd_iconv_open && gd_iconv && gd_iconv_close && gd_locale_charset;
}
#endif

String OS_Unix::multibyte_to_string(const String& p_encoding, const PackedByteArray& p_array)
{
	ERR_FAIL_COND_V_MSG(!_iconv_ok, String(), "Conversion failed: Unable to load libiconv");

	LocalVector<char> chars;
#if defined(__GLIBC__) || defined(WEB_ENABLED)
	gd_iconv_t ctx = gd_iconv_open(
		"UTF-8", p_encoding.is_empty() ? nl_langinfo(CODESET) : p_encoding.utf8().get_data());
#else
	gd_iconv_t ctx = gd_iconv_open(
		"UTF-8", p_encoding.is_empty() ? gd_locale_charset() : p_encoding.utf8().get_data());
#endif
	ERR_FAIL_COND_V_MSG(ctx == (gd_iconv_t)(-1), String(), "Conversion failed: Unknown encoding");

	char* in_ptr = (char*)p_array.ptr();
	size_t in_size = p_array.size();

	chars.resize(in_size);
	char* out_ptr = (char*)chars.ptr();
	size_t out_size = chars.size();

	while (gd_iconv(ctx, &in_ptr, &in_size, &out_ptr, &out_size) == (size_t)-1) {
		if (errno != E2BIG) {
			gd_iconv_close(ctx);
			ERR_FAIL_V_MSG(String(), vformat("Conversion failed: %d - %s", errno, strerror(errno)));
		}
		int64_t rate = (chars.size()) / (p_array.size() - in_size);
		size_t oldpos = chars.size() - out_size;
		chars.resize(chars.size() + in_size * rate);
		out_ptr = (char*)chars.ptr() + oldpos;
		out_size = chars.size() - oldpos;
	}
	chars.resize(chars.size() - out_size);
	gd_iconv_close(ctx);

	return String::utf8((const char*)chars.ptr(), chars.size());
}

PackedByteArray OS_Unix::string_to_multibyte(const String& p_encoding, const String& p_string)
{
	ERR_FAIL_COND_V_MSG(
		!_iconv_ok, PackedByteArray(), "Conversion failed: Unable to load libiconv");

	CharString charstr = p_string.utf8();

	PackedByteArray ret;
#if defined(__GLIBC__) || defined(WEB_ENABLED)
	gd_iconv_t ctx = gd_iconv_open(
		p_encoding.is_empty() ? nl_langinfo(CODESET) : p_encoding.utf8().get_data(), "UTF-8");
#else
	gd_iconv_t ctx = gd_iconv_open(
		p_encoding.is_empty() ? gd_locale_charset() : p_encoding.utf8().get_data(), "UTF-8");
#endif
	ERR_FAIL_COND_V_MSG(
		ctx == (gd_iconv_t)(-1), PackedByteArray(), "Conversion failed: Unknown encoding");

	char* in_ptr = (char*)charstr.ptr();
	size_t in_size = charstr.size();

	ret.resize(in_size);
	char* out_ptr = (char*)ret.ptrw();
	size_t out_size = ret.size();

	while (gd_iconv(ctx, &in_ptr, &in_size, &out_ptr, &out_size) == (size_t)-1) {
		if (errno != E2BIG) {
			gd_iconv_close(ctx);
			ERR_FAIL_V_MSG(
				PackedByteArray(), vformat("Conversion failed: %d - %s", errno, strerror(errno)));
		}
		int64_t rate = (ret.size()) / (charstr.size() - in_size);
		size_t oldpos = ret.size() - out_size;
		ret.resize(ret.size() + in_size * rate);
		out_ptr = (char*)ret.ptrw() + oldpos;
		out_size = ret.size() - oldpos;
	}
	ret.resize(ret.size() - out_size);
	gd_iconv_close(ctx);

	return ret;
}

int OS_Unix::_wait_for_pid_completion(const pid_t p_pid, int* r_status, int p_options, pid_t* r_pid)
{
	while (true) {
		pid_t pid = waitpid(p_pid, r_status, p_options);
		if (pid != -1) {
			// When `p_options` has `WNOHANG`, 0 can be returned if the process is still running.
			if (r_pid) {
				*r_pid = pid;
			}
			return 0;
		}
		const int error = errno;
		if (error == EINTR) {
			// We're in a debugger, should call waitpid again.
			// See https://stackoverflow.com/a/45472920/730797.
			continue;
		}
		return error;
	}
}

bool OS_Unix::_check_pid_is_running(const pid_t p_pid, int* r_status)
{
	const ProcessInfo* pi = process_map->getptr(p_pid);

	if (pi && !pi->is_running) {
		// Can return cached value.
		if (r_status) {
			*r_status = pi->exit_code;
		}
		return false;
	}

	pid_t pid = -1;
	int status = 0;
	const int result = _wait_for_pid_completion(p_pid, &status, WNOHANG, &pid);
	if (result == 0 && pid == 0) {
		// Thread is still running.
		return true;
	}
	ERR_FAIL_COND_V_MSG(result != 0 && errno == ECHILD, false,
		vformat(
			"The process %d does not exist or is not a child of the calling process.", (int)p_pid));
	ERR_FAIL_COND_V_MSG(result != 0, false,
		vformat("waitpid for process %d failed with errno: %d", (int)p_pid, errno));

	// Thread exited normally.
	status = WIFEXITED(status) ? WEXITSTATUS(status) : status;

	if (pi) {
		pi->is_running = false;
		pi->exit_code = status;
	}

	if (r_status) {
		*r_status = status;
	}

	return false;
}

Error OS_Unix::execute(const String& p_path, const List<String>& p_arguments, String* r_pipe,
	int* r_exitcode, bool read_stderr, Mutex* p_pipe_mutex, bool p_open_console)
{
#ifdef __EMSCRIPTEN__
	// Don't compile this code at all to avoid undefined references.
	// Actual virtual call goes to OS_Web.
	ERR_FAIL_V(ERR_BUG);
#else
	if (r_pipe) {
		String command = "\"" + p_path + "\"";
		for (const String& arg : p_arguments) {
			command += String(" \"") + arg + "\"";
		}
		if (read_stderr) {
			command += " 2>&1"; // Include stderr
		}
		else {
			command += " 2>/dev/null"; // Silence stderr
		}

		FILE* f = popen(command.utf8().get_data(), "r");
		ERR_FAIL_NULL_V_MSG(f, ERR_CANT_OPEN, "Cannot create pipe from command: " + command + ".");
		char buf[65535];
		while (fgets(buf, 65535, f)) {
			if (p_pipe_mutex) {
				p_pipe_mutex->lock();
			}
			String pipe_out;
			if (pipe_out.append_utf8(buf) == OK) {
				(*r_pipe) += pipe_out;
			}
			else {
				(*r_pipe) += String(buf); // If not valid UTF-8 try decode as Latin-1
			}
			if (p_pipe_mutex) {
				p_pipe_mutex->unlock();
			}
		}
		int rv = pclose(f);

		if (r_exitcode) {
			*r_exitcode = WEXITSTATUS(rv);
		}
		return OK;
	}

	pid_t pid = fork();
	ERR_FAIL_COND_V(pid < 0, ERR_CANT_FORK);

	if (pid == 0) {
		// The child process
		Vector<CharString> cs;
		cs.push_back(p_path.utf8());
		for (const String& arg : p_arguments) {
			cs.push_back(arg.utf8());
		}

		Vector<char*> args;
		for (int i = 0; i < cs.size(); i++) {
			args.push_back((char*)cs[i].get_data());
		}
		args.push_back(0);

		execvp(p_path.utf8().get_data(), &args[0]);
		// The execvp() function only returns if an error occurs.
		fprintf(stderr, "Could not create child process: %s\n", p_path.utf8().get_data());
		raise(SIGKILL);
	}

	int status = 0;
	const int result = _wait_for_pid_completion(pid, &status, 0);
	if (r_exitcode) {
		*r_exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : status;
	}
	return result ? FAILED : OK;
#endif
}

Error OS_Unix::create_process(const String& p_path, const List<String>& p_arguments,
	ProcessID* r_child_id, bool p_open_console)
{
#ifdef __EMSCRIPTEN__
	// Don't compile this code at all to avoid undefined references.
	// Actual virtual call goes to OS_Web.
	ERR_FAIL_V(ERR_BUG);
#else
	pid_t pid = fork();
	ERR_FAIL_COND_V(pid < 0, ERR_CANT_FORK);

	if (pid == 0) {
		// The new process
		// Create a new session-ID so parent won't wait for it.
		// This ensures the process won't go zombie at the end.
		setsid();

		Vector<CharString> cs;
		cs.push_back(p_path.utf8());
		for (const String& arg : p_arguments) {
			cs.push_back(arg.utf8());
		}

		Vector<char*> args;
		for (int i = 0; i < cs.size(); i++) {
			args.push_back((char*)cs[i].get_data());
		}
		args.push_back(0);

		execvp(p_path.utf8().get_data(), &args[0]);
		// The execvp() function only returns if an error occurs.
		fprintf(stderr, "Could not create child process: %s\n", p_path.utf8().get_data());
		raise(SIGKILL);
	}

	ProcessInfo pi;
	process_map_mutex.lock();
	process_map->insert(pid, pi);
	process_map_mutex.unlock();

	if (r_child_id) {
		*r_child_id = pid;
	}
	return OK;
#endif
}

Error OS_Unix::kill(const ProcessID& p_pid)
{
	int ret = ::kill(p_pid, SIGKILL);
	if (!ret) {
		// avoid zombie process
		int st;
		_wait_for_pid_completion(p_pid, &st, 0);
	}
	return ret ? ERR_INVALID_PARAMETER : OK;
}

int OS_Unix::get_process_id() { return getpid(); }

bool OS_Unix::is_process_running(const ProcessID& p_pid)
{
	MutexLock lock(process_map_mutex);
	return _check_pid_is_running(p_pid, nullptr);
}

int OS_Unix::get_process_exit_code(const ProcessID& p_pid)
{
	MutexLock lock(process_map_mutex);

	int exit_code = 0;
	if (_check_pid_is_running(p_pid, &exit_code)) {
		// Thread is still running
		return -1;
	}

	return exit_code;
}

String OS_Unix::get_locale()
{
	if (!has_environment("LANG")) {
		return "en";
	}

	String locale = get_environment("LANG");
	int tp = locale.find_char('.');
	if (tp != -1) {
		locale = locale.substr(0, tp);
	}
	return locale;
}

Error OS_Unix::open_dynamic_library(
	const String& p_path, void*& p_library_handle, OS::GDExtensionData* p_data)
{
	String path = p_path;

	if (FileAccess::exists(path) && path.is_relative_path()) {
		// dlopen expects a slash, in this case a leading ./ for it to be interpreted as a relative
		// path,
		//  otherwise it will end up searching various system directories for the lib instead and
		//  finally failing.
		path = "./" + path;
	}

	if (!FileAccess::exists(path)) {
		// This code exists so GDExtension can load .so files from within the executable path.
		path = get_executable_path().get_base_dir().path_join(p_path.get_file());
	}

	if (!FileAccess::exists(path)) {
		// This code exists so GDExtension can load .so files from a standard unix location.
		path =
			get_executable_path().get_base_dir().path_join("../lib").path_join(p_path.get_file());
	}

	ERR_FAIL_COND_V_MSG(!FileAccess::exists(path), ERR_FILE_NOT_FOUND,
		vformat("Can't open dynamic library, file not found: '%s'.", p_path));

	p_library_handle = dlopen(path.utf8().get_data(), GODOT_DLOPEN_MODE);
	ERR_FAIL_NULL_V_MSG(p_library_handle, ERR_CANT_OPEN,
		vformat("Can't open dynamic library: %s. Error: %s.", p_path, dlerror()));

	if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
		*p_data->r_resolved_path = path;
	}

	return OK;
}

Error OS_Unix::close_dynamic_library(void* p_library_handle)
{
	if (dlclose(p_library_handle)) {
		return FAILED;
	}
	return OK;
}

Error OS_Unix::get_dynamic_library_symbol_handle(
	void* p_library_handle, const String& p_name, void*& p_symbol_handle, bool p_optional)
{
	const char* error;
	dlerror(); // Clear existing errors

	p_symbol_handle = dlsym(p_library_handle, p_name.utf8().get_data());

	error = dlerror();
	if (error != nullptr) {
		ERR_FAIL_COND_V_MSG(!p_optional, ERR_CANT_RESOLVE,
			"Can't resolve symbol " + p_name + ". Error: " + error + ".");

		return ERR_CANT_RESOLVE;
	}
	return OK;
}

Error OS_Unix::set_cwd(const String& p_cwd)
{
	if (chdir(p_cwd.utf8().get_data()) != 0) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

String OS_Unix::get_cwd()
{
	String dir;
	char real_current_dir_name[2048];
	ERR_FAIL_NULL_V(getcwd(real_current_dir_name, 2048), ".");
	if (dir.append_utf8(real_current_dir_name) != OK) {
		dir = real_current_dir_name;
	}
	return dir;
}

bool OS_Unix::has_environment(const String& p_var)
{
	return getenv(p_var.utf8().get_data()) != nullptr;
}

String OS_Unix::get_environment(const String& p_var)
{
	const char* val = getenv(p_var.utf8().get_data());
	if (val == nullptr) { // Not set; return empty string
		return "";
	}
	String s;
	if (s.append_utf8(val) == OK) {
		return s;
	}
	return String(val); // Not valid UTF-8, so return as-is
}

void OS_Unix::set_environment(const String& p_var, const String& p_value)
{
	ERR_FAIL_COND_MSG(p_var.is_empty() || p_var.contains_char('='),
		vformat("Invalid environment variable name '%s', cannot be empty or include '='.", p_var));
	int err = setenv(p_var.utf8().get_data(), p_value.utf8().get_data(), /* overwrite: */ 1);
	ERR_FAIL_COND_MSG(err != 0,
		vformat("Failed setting environment variable '%s', the system is out of memory.", p_var));
}

void OS_Unix::unset_environment(const String& p_var)
{
	ERR_FAIL_COND_MSG(p_var.is_empty() || p_var.contains_char('='),
		vformat("Invalid environment variable name '%s', cannot be empty or include '='.", p_var));
	unsetenv(p_var.utf8().get_data());
}

String OS_Unix::get_user_data_dir(const String& p_user_dir)
{
	return OS::get_data_path().path_join(p_user_dir);
}

String OS_Unix::get_executable_path()
{
#ifdef __linux__
	// fix for running from a symlink
	char buf[PATH_MAX];
	memset(buf, 0, PATH_MAX);
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf));
	String b;
	if (len > 0) {
		b.append_utf8(buf, len);
	}
	if (b.is_empty()) {
		WARN_PRINT("Couldn't get executable path from /proc/self/exe, using argv[0]");
		return OS::get_executable_path();
	}
	return b;
#elif defined(__OpenBSD__)
	char resolved_path[MAXPATHLEN];

	realpath(OS::get_executable_path().utf8().get_data(), resolved_path);

	return String(resolved_path);
#elif defined(__NetBSD__)
	int mib[4] = {CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME};
	char buf[MAXPATHLEN];
	size_t len = sizeof(buf);
	if (sysctl(mib, 4, buf, &len, nullptr, 0) != 0) {
		WARN_PRINT("Couldn't get executable path from sysctl");
		return OS::get_executable_path();
	}

	// NetBSD does not always return a normalized path. For example if argv[0] is "./a.out" then
	// executable path is "/home/netbsd/./a.out". Normalize with realpath:
	char resolved_path[MAXPATHLEN];

	realpath(buf, resolved_path);

	return String(resolved_path);
#elif defined(__FreeBSD__)
	int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
	char buf[MAXPATHLEN];
	size_t len = sizeof(buf);
	if (sysctl(mib, 4, buf, &len, nullptr, 0) != 0) {
		WARN_PRINT("Couldn't get executable path from sysctl");
		return OS::get_executable_path();
	}

	return String::utf8(buf);
#elif defined(__APPLE__)
	char temp_path[1];
	uint32_t buff_size = 1;
	_NSGetExecutablePath(temp_path, &buff_size);

	char* resolved_path = new char[buff_size + 1];

	if (_NSGetExecutablePath(resolved_path, &buff_size) == 1) {
		WARN_PRINT("MAXPATHLEN is too small");
	}

	String path = String::utf8(resolved_path);
	delete[] resolved_path;

	return path;
#else
	ERR_PRINT("Warning, don't know how to obtain executable path on this OS! Please override t
		his "
			  "function properly.");
	return OS::get_executable_path();
#endif
}

String OS_Unix::expand_path(const String& p_path)
{
	String path = p_path;

	if (path.begins_with("~/") || path == "~") {
		String home = get_environment("HOME");
		if (!home.is_empty()) {
			path = home + path.substr(1);
		}
	}

	return path;
}

UnixTerminalLogger::~UnixTerminalLogger() {}

void OS_Unix::initialize()
{
#if !defined(__GLIBC__) && !defined(WEB_ENABLED)
	_load_iconv();
#endif

	Vector<Logger*> loggers;
	loggers.push_back(memnew(UnixTerminalLogger));
	OS::_set_logger(memnew(CompositeLogger(loggers)));
}

#endif

#ifdef RTLD_DEEPBIND
#undef RTLD_DEEPBIND
#endif
#ifdef GODOT_DLOPEN_MODE
#undef GODOT_DLOPEN_MODE
#endif
#ifdef UNIX_GET_ENTROPY
#undef UNIX_GET_ENTROPY
#endif
#ifdef GODOT_CLOCK
#undef GODOT_CLOCK
#endif

// ============================================================================
// Merged from drivers/unix/dir_access_unix.cpp
// ============================================================================

#include "dir_access_unix.h"

#if defined(UNIX_ENABLED)

#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/templates/list.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/statfs.h>
#endif
#include <sys/statvfs.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#if __has_include(<mntent.h>)
#include <mntent.h>
#endif

String DirAccessUnix::fix_path(const String &p_path) const {
	return DirAccess::fix_path(p_path).simplify_path();
}

Error DirAccessUnix::list_dir_begin() {
	list_dir_end(); //close any previous dir opening!

	//char real_current_dir_name[2048]; //is this enough?!
	//getcwd(real_current_dir_name,2048);
	//chdir(current_path.utf8().get_data());
	dir_stream = opendir(current_dir.utf8().get_data());
	//chdir(real_current_dir_name);
	if (!dir_stream) {
		return ERR_CANT_OPEN; //error!
	}

	return OK;
}

bool DirAccessUnix::file_exists(String p_file) {
	GLOBAL_LOCK_FUNCTION

	if (p_file.is_relative_path()) {
		p_file = current_dir.path_join(p_file);
	}

	p_file = fix_path(p_file);

	struct stat flags = {};
	bool success = (stat(p_file.utf8().get_data(), &flags) == 0);

	if (success && S_ISDIR(flags.st_mode)) {
		success = false;
	}

	return success;
}

bool DirAccessUnix::dir_exists(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);

	struct stat flags = {};
	bool success = (stat(p_dir.utf8().get_data(), &flags) == 0);

	return (success && S_ISDIR(flags.st_mode));
}

bool DirAccessUnix::is_readable(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);
	return (access(p_dir.utf8().get_data(), R_OK) == 0);
}

bool DirAccessUnix::is_writable(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);
	return (access(p_dir.utf8().get_data(), W_OK) == 0);
}

uint64_t DirAccessUnix::get_modified_time(String p_file) {
	if (p_file.is_relative_path()) {
		p_file = current_dir.path_join(p_file);
	}

	p_file = fix_path(p_file);

	struct stat flags = {};
	bool success = (stat(p_file.utf8().get_data(), &flags) == 0);

	if (success) {
		return flags.st_mtime;
	} else {
		ERR_FAIL_V(0);
	}
	return 0;
}

String DirAccessUnix::get_next() {
	if (!dir_stream) {
		return "";
	}

	dirent *entry = readdir(dir_stream);

	if (entry == nullptr) {
		list_dir_end();
		return "";
	}

	String fname = fix_unicode_name(entry->d_name);

	// Look at d_type to determine if the entry is a directory, unless
	// its type is unknown (the file system does not support it) or if
	// the type is a link, in that case we want to resolve the link to
	// known if it points to a directory. stat() will resolve the link
	// for us.
	if (entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK) {
		String f = current_dir.path_join(fname);

		struct stat flags = {};
		if (stat(f.utf8().get_data(), &flags) == 0) {
			_cisdir = S_ISDIR(flags.st_mode);
		} else {
			_cisdir = false;
		}
	} else {
		_cisdir = (entry->d_type == DT_DIR);
	}

	_cishidden = is_hidden(fname);

	return fname;
}

bool DirAccessUnix::current_is_dir() const {
	return _cisdir;
}

bool DirAccessUnix::current_is_hidden() const {
	return _cishidden;
}

void DirAccessUnix::list_dir_end() {
	if (dir_stream) {
		closedir(dir_stream);
	}
	dir_stream = nullptr;
	_cisdir = false;
}

#if __has_include(<mntent.h>) && defined(LINUXBSD_ENABLED)
static bool _filter_drive(struct mntent *mnt) {
	// Ignore devices that don't point to /dev
	if (strncmp(mnt->mnt_fsname, "/dev", 4) != 0) {
		return false;
	}

	// Accept devices mounted at common locations
	if (strncmp(mnt->mnt_dir, "/media", 6) == 0 ||
			strncmp(mnt->mnt_dir, "/mnt", 4) == 0 ||
			strncmp(mnt->mnt_dir, "/home", 5) == 0 ||
			strncmp(mnt->mnt_dir, "/run/media", 10) == 0) {
		return true;
	}

	// Ignore everything else
	return false;
}
#endif

static void _get_drives(List<String> *list) {
	// Add root.
	list->push_back("/");

#if __has_include(<mntent.h>) && defined(LINUXBSD_ENABLED)
	// Check /etc/mtab for the list of mounted partitions.
	FILE *mtab = setmntent("/etc/mtab", "r");
	if (mtab) {
		struct mntent mnt;
		char strings[4096];

		while (getmntent_r(mtab, &mnt, strings, sizeof(strings))) {
			if (mnt.mnt_dir != nullptr && _filter_drive(&mnt)) {
				// Avoid duplicates
				String name = String::utf8(mnt.mnt_dir);
				if (!list->find(name)) {
					list->push_back(name);
				}
			}
		}

		endmntent(mtab);
	}
#endif

	// Add $HOME.
	const char *home = getenv("HOME");
	if (home) {
		// Only add if it's not a duplicate
		String home_name = String::utf8(home);
		if (!list->find(home_name)) {
			list->push_back(home_name);
		}

		// Check GTK+3 bookmarks for both XDG locations (Documents, Downloads, etc.)
		// and potential user-defined bookmarks.
		char path[1024];
		snprintf(path, 1024, "%s/.config/gtk-3.0/bookmarks", home);
		FILE *fd = fopen(path, "r");
		if (fd) {
			char string[1024];
			while (fgets(string, 1024, fd)) {
				// Parse only file:// links
				if (strncmp(string, "file://", 7) == 0) {
					// Strip any unwanted edges on the strings and push_back if it's not a duplicate.
					String fpath = String::utf8(string + 7).strip_edges().split_spaces()[0].uri_file_decode();
					if (!list->find(fpath)) {
						list->push_back(fpath);
					}
				}
			}

			fclose(fd);
		}

		// Add Desktop dir.
		String dpath = OS::get_system_dir(OS::SystemDir::SYSTEM_DIR_DESKTOP);
		if (dpath.length() > 0 && !list->find(dpath)) {
			list->push_back(dpath);
		}
	}

	list->sort();
}

int DirAccessUnix::get_drive_count() {
	List<String> list;
	_get_drives(&list);

	return list.size();
}

String DirAccessUnix::get_drive(int p_drive) {
	List<String> list;
	_get_drives(&list);

	ERR_FAIL_INDEX_V(p_drive, list.size(), "");

	return list.get(p_drive);
}

int DirAccessUnix::get_current_drive() {
	int drive = 0;
	int max_length = -1;
	const String path = get_current_dir().to_lower();
	for (int i = 0; i < get_drive_count(); i++) {
		const String d = get_drive(i).to_lower();
		if (max_length < d.length() && path.begins_with(d)) {
			max_length = d.length();
			drive = i;
		}
	}
	return drive;
}

bool DirAccessUnix::drives_are_shortcuts() {
	return true;
}

Error DirAccessUnix::make_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);

	bool success = (mkdir(p_dir.utf8().get_data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
	int err = errno;

	if (success) {
		return OK;
	}

	if (err == EEXIST) {
		return ERR_ALREADY_EXISTS;
	}

	return ERR_CANT_CREATE;
}

Error DirAccessUnix::change_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	p_dir = fix_path(p_dir);

	// prev_dir is the directory we are changing out of
	String prev_dir;
	char real_current_dir_name[2048];
	ERR_FAIL_NULL_V(getcwd(real_current_dir_name, 2048), ERR_BUG);
	if (prev_dir.append_utf8(real_current_dir_name) != OK) {
		prev_dir = real_current_dir_name; //no utf8, maybe latin?
	}

	// try_dir is the directory we are trying to change into
	String try_dir = "";
	if (p_dir.is_relative_path()) {
		String next_dir = current_dir.path_join(p_dir);
		next_dir = next_dir.simplify_path();
		try_dir = next_dir;
	} else {
		try_dir = p_dir;
	}

	bool worked = (chdir(try_dir.utf8().get_data()) == 0); // we can only give this utf8
	if (!worked) {
		return ERR_INVALID_PARAMETER;
	}

	String base = _get_root_path();
	if (!base.is_empty() && !try_dir.begins_with(base)) {
		ERR_FAIL_NULL_V(getcwd(real_current_dir_name, 2048), ERR_BUG);
		String new_dir;
		new_dir.append_utf8(real_current_dir_name);

		if (!new_dir.begins_with(base)) {
			try_dir = current_dir; //revert
		}
	}

	// the directory exists, so set current_dir to try_dir
	current_dir = try_dir;
	ERR_FAIL_COND_V(chdir(prev_dir.utf8().get_data()) != 0, ERR_BUG);
	return OK;
}

String DirAccessUnix::get_current_dir(bool p_include_drive) const {
	String base = _get_root_path();
	if (!base.is_empty()) {
		String bd = current_dir.replace_first(base, "");
		if (bd.begins_with("/")) {
			return _get_root_string() + bd.substr(1);
		} else {
			return _get_root_string() + bd;
		}
	}
	return current_dir;
}

Error DirAccessUnix::rename(String p_path, String p_new_path) {
	if (p_path.is_relative_path()) {
		p_path = get_current_dir().path_join(p_path);
	}

	p_path = fix_path(p_path);
	if (p_path.ends_with("/")) {
		p_path = p_path.left(-1);
	}

	if (p_new_path.is_relative_path()) {
		p_new_path = get_current_dir().path_join(p_new_path);
	}

	p_new_path = fix_path(p_new_path);
	if (p_new_path.ends_with("/")) {
		p_new_path = p_new_path.left(-1);
	}

	int res = ::rename(p_path.utf8().get_data(), p_new_path.utf8().get_data());
	if (res != 0 && errno == EXDEV) { // Cross-device move, use copy and remove.
		Error err = OK;
		err = copy(p_path, p_new_path);
		if (err != OK) {
			return err;
		}
		return remove(p_path);
	} else {
		return (res == 0) ? OK : FAILED;
	}
}

Error DirAccessUnix::remove(String p_path) {
	if (p_path.is_relative_path()) {
		p_path = get_current_dir().path_join(p_path);
	}

	p_path = fix_path(p_path);
	if (p_path.ends_with("/")) {
		p_path = p_path.left(-1);
	}

	struct stat flags = {};
	if (lstat(p_path.utf8().get_data(), &flags) != 0) {
		return FAILED;
	}

	int err;
	if (S_ISDIR(flags.st_mode) && !is_link(p_path)) {
		err = ::rmdir(p_path.utf8().get_data());
	} else {
		err = ::unlink(p_path.utf8().get_data());
	}
	if (err != 0) {
		return FAILED;
	}
	if (remove_notification_func != nullptr) {
		remove_notification_func(p_path);
	}
	return OK;
}

bool DirAccessUnix::is_link(String p_file) {
	if (p_file.is_relative_path()) {
		p_file = get_current_dir().path_join(p_file);
	}

	p_file = fix_path(p_file);
	if (p_file.ends_with("/")) {
		p_file = p_file.left(-1);
	}

	struct stat flags = {};
	if (lstat(p_file.utf8().get_data(), &flags) != 0) {
		return false;
	}

	return S_ISLNK(flags.st_mode);
}

String DirAccessUnix::read_link(String p_file) {
	if (p_file.is_relative_path()) {
		p_file = get_current_dir().path_join(p_file);
	}

	p_file = fix_path(p_file);
	if (p_file.ends_with("/")) {
		p_file = p_file.left(-1);
	}

	char buf[PATH_MAX];
	memset(buf, 0, PATH_MAX);
	ssize_t len = readlink(p_file.utf8().get_data(), buf, sizeof(buf));
	String link;
	if (len > 0) {
		link.append_utf8(buf, len);
	}
	return link;
}

Error DirAccessUnix::create_link(String p_source, String p_target) {
	if (p_target.is_relative_path()) {
		p_target = get_current_dir().path_join(p_target);
	}

	p_source = fix_path(p_source);
	p_target = fix_path(p_target);

	if (symlink(p_source.utf8().get_data(), p_target.utf8().get_data()) == 0) {
		return OK;
	} else {
		return FAILED;
	}
}

uint64_t DirAccessUnix::get_space_left() {
	struct statvfs vfs;
	if (statvfs(current_dir.utf8().get_data(), &vfs) != 0) {
		return 0;
	}

	return (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize;
}

String DirAccessUnix::get_filesystem_type() const {
#ifdef __linux__
	struct statfs fs;
	if (statfs(current_dir.utf8().get_data(), &fs) != 0) {
		return "";
	}
	switch (static_cast<unsigned int>(fs.f_type)) {
		case 0x0000adf5:
			return "ADFS";
		case 0x0000adff:
			return "AFFS";
		case 0x5346414f:
			return "AFS";
		case 0x00000187:
			return "AUTOFS";
		case 0x00c36400:
			return "CEPH";
		case 0x73757245:
			return "CODA";
		case 0x28cd3d45:
			return "CRAMFS";
		case 0x453dcd28:
			return "CRAMFS";
		case 0x64626720:
			return "DEBUGFS";
		case 0x73636673:
			return "SECURITYFS";
		case 0xf97cff8c:
			return "SELINUX";
		case 0x43415d53:
			return "SMACK";
		case 0x858458f6:
			return "RAMFS";
		case 0x01021994:
			return "TMPFS";
		case 0x958458f6:
			return "HUGETLBFS";
		case 0x73717368:
			return "SQUASHFS";
		case 0x0000f15f:
			return "ECRYPTFS";
		case 0x00414a53:
			return "EFS";
		case 0xe0f5e1e2:
			return "EROFS";
		case 0x0000ef53:
			return "EXTFS";
		case 0xabba1974:
			return "XENFS";
		case 0x9123683e:
			return "BTRFS";
		case 0x00003434:
			return "NILFS";
		case 0xf2f52010:
			return "F2FS";
		case 0xf995e849:
			return "HPFS";
		case 0x00009660:
			return "ISOFS";
		case 0x000072b6:
			return "JFFS2";
		case 0x58465342:
			return "XFS";
		case 0x6165676c:
			return "PSTOREFS";
		case 0xde5e81e4:
			return "EFIVARFS";
		case 0x00c0ffee:
			return "HOSTFS";
		case 0x794c7630:
			return "OVERLAYFS";
		case 0x65735546:
			return "FUSE";
		case 0xca451a4e:
			return "BCACHEFS";
		case 0x00004d44:
			return "FAT32";
		case 0x2011bab0:
			return "EXFAT";
		case 0x0000564c:
			return "NCP";
		case 0x00006969:
			return "NFS";
		case 0x7461636f:
			return "OCFS2";
		case 0x00009fa1:
			return "OPENPROM";
		case 0x0000002f:
			return "QNX4";
		case 0x68191122:
			return "QNX6";
		case 0x6b414653:
			return "AFS";
		case 0x52654973:
			return "REISERFS";
		case 0x0000517b:
			return "SMB";
		case 0xff534d42:
			return "CIFS";
		case 0x0027e0eb:
			return "CGROUP";
		case 0x63677270:
			return "CGROUP2";
		case 0x07655821:
			return "RDTGROUP";
		case 0x74726163:
			return "TRACEFS";
		case 0x01021997:
			return "V9FS";
		case 0x62646576:
			return "BDEVFS";
		case 0x64646178:
			return "DAXFS";
		case 0x42494e4d:
			return "BINFMTFS";
		case 0x00001cd1:
			return "DEVPTS";
		case 0x6c6f6f70:
			return "BINDERFS";
		case 0x0bad1dea:
			return "FUTEXFS";
		case 0x50495045:
			return "PIPEFS";
		case 0x00009fa0:
			return "PROC";
		case 0x534f434b:
			return "SOCKFS";
		case 0x62656572:
			return "SYSFS";
		case 0x00009fa2:
			return "USBDEVICE";
		case 0x11307854:
			return "MTD_INODE";
		case 0x09041934:
			return "ANON_INODE";
		case 0x73727279:
			return "BTRFS";
		case 0x6e736673:
			return "NSFS";
		case 0xcafe4a11:
			return "BPF_FS";
		case 0x5a3c69f0:
			return "AAFS";
		case 0x5a4f4653:
			return "ZONEFS";
		case 0x15013346:
			return "UDF";
		case 0x444d4142:
			return "DMA_BUF";
		case 0x454d444d:
			return "DEVMEM";
		case 0x5345434d:
			return "SECRETMEM";
		case 0x50494446:
			return "PID_FS";
		default:
			return "";
	}
#else
	return ""; //TODO this should be implemented
#endif
}

bool DirAccessUnix::is_hidden(const String &p_name) {
	return p_name != "." && p_name != ".." && p_name.begins_with(".");
}

bool DirAccessUnix::is_case_sensitive(const String &p_path) const {
#if defined(LINUXBSD_ENABLED)
	String f = p_path;
	if (!f.is_absolute_path()) {
		f = get_current_dir().path_join(f);
	}
	f = fix_path(f);

	int fd = ::open(f.utf8().get_data(), O_RDONLY | O_NONBLOCK);
	if (fd) {
		long flags = 0;
		if (ioctl(fd, _IOR('f', 1, long), &flags) >= 0) {
			::close(fd);
			return !(flags & 0x40000000 /* FS_CASEFOLD_FL */);
		}
		::close(fd);
	}
#endif
	return true;
}

bool DirAccessUnix::is_equivalent(const String &p_path_a, const String &p_path_b) const {
	String f1 = fix_path(p_path_a);
	struct stat st1 = {};
	int err = stat(f1.utf8().get_data(), &st1);
	if (err) {
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}

	String f2 = fix_path(p_path_b);
	struct stat st2 = {};
	err = stat(f2.utf8().get_data(), &st2);
	if (err) {
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}

	return (st1.st_dev == st2.st_dev) && (st1.st_ino == st2.st_ino);
}

DirAccessUnix::DirAccessUnix() {
	dir_stream = nullptr;
	_cisdir = false;

	/* determine drive count */

	// set current directory to an absolute path of the current directory
	char real_current_dir_name[2048];
	ERR_FAIL_NULL(getcwd(real_current_dir_name, 2048));
	current_dir.clear();
	if (current_dir.append_utf8(real_current_dir_name) != OK) {
		current_dir = real_current_dir_name;
	}

	change_dir(current_dir);
}

DirAccessUnix::RemoveNotificationFunc DirAccessUnix::remove_notification_func = nullptr;

DirAccessUnix::~DirAccessUnix() {
	list_dir_end();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/file_access_unix.cpp
// ============================================================================

#include "drivers/unix/os_unix.h"

#if defined(UNIX_ENABLED)

#include "core/string/ustring.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(WEB_ENABLED)
#include <sys/xattr.h>
#endif
#include <unistd.h>

#include <cerrno>
#include <cstdlib>

void FileAccessUnix::check_errors(bool p_write) const {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");

	last_error = OK;
	if (ferror(f)) {
		if (p_write) {
			last_error = ERR_FILE_CANT_WRITE;
		} else {
			last_error = ERR_FILE_CANT_READ;
		}
	}
	if (!p_write && feof(f)) {
		last_error = ERR_FILE_EOF;
	}
}

Error FileAccessUnix::open_internal(const String &p_path, int p_mode_flags) {
	_close();

	path_src = p_path;
	path = fix_path(p_path);
	//printf("opening %s, %i\n", path.utf8().get_data(), Memory::get_static_mem_usage());

	ERR_FAIL_COND_V_MSG(f, ERR_ALREADY_IN_USE, "File is already in use.");
	const char *mode_string;

	if (p_mode_flags == READ) {
		mode_string = "rb";
	} else if (p_mode_flags == WRITE) {
		mode_string = "wb";
	} else if (p_mode_flags == READ_WRITE) {
		mode_string = "rb+";
	} else if (p_mode_flags == WRITE_READ) {
		mode_string = "wb+";
	} else {
		return ERR_INVALID_PARAMETER;
	}

	/* pretty much every implementation that uses fopen as primary
	   backend (unix-compatible mostly) supports utf8 encoding */

	//printf("opening %s as %s\n", p_path.utf8().get_data(), path.utf8().get_data());
	struct stat st = {};
	int err = stat(path.utf8().get_data(), &st);
	if (!err) {
		switch (st.st_mode & S_IFMT) {
			case S_IFLNK:
			case S_IFREG:
				break;
			default:
				return ERR_FILE_CANT_OPEN;
		}
	}

#if defined(TOOLS_ENABLED)
	if (p_mode_flags & READ) {
		String real_path = get_real_path();
		if (real_path != path) {
			// Don't warn on symlinks, since they can be used to simply share addons on multiple projects.
			if (real_path.to_lower() == path.to_lower()) {
				// The File system is case insensitive, but other platforms can be sensitive to it
				// To ease cross-platform development, we issue a warning if users try to access
				// a file using the wrong case (which *works* on Windows and macOS, but won't on other
				// platforms).
				WARN_PRINT(vformat("Case mismatch opening requested file '%s', stored as '%s' in the filesystem. This file will not open when exported to other case-sensitive platforms.", path, real_path));
			}
		}
	}
#endif

	if (is_backup_save_enabled() && (p_mode_flags == WRITE)) {
		// Set save path to the symlink target, not the link itself.
		String link;
		bool is_link = false;
		{
			CharString cs = path.utf8();
			struct stat lst = {};
			if (lstat(cs.get_data(), &lst) == 0) {
				is_link = S_ISLNK(lst.st_mode);
			}
			if (is_link) {
				char buf[PATH_MAX];
				memset(buf, 0, PATH_MAX);
				ssize_t len = readlink(cs.get_data(), buf, sizeof(buf));
				if (len > 0) {
					link.append_utf8(buf, len);
				}
				if (!link.is_absolute_path()) {
					link = path.get_base_dir().path_join(link);
				}
			}
		}
		save_path = is_link ? link : path;

		// Create a temporary file in the same directory as the target file.
		path = path + "-XXXXXX";
		CharString cs = path.utf8();
		int fd = mkstemp(cs.ptrw());
		if (fd == -1) {
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}

		struct stat file_stat = {};
		int error = stat(save_path.utf8().get_data(), &file_stat);
		if (!error) {
			fchmod(fd, file_stat.st_mode & 0xFFF); // Mask to remove file type
		} else {
			fchmod(fd, 0644); // Fallback permissions
		}

		path = String::utf8(cs.ptr());

		f = fdopen(fd, mode_string);
		if (f == nullptr) {
			// Delete temp file and close descriptor if open failed.
			::unlink(cs.ptr());
			::close(fd);
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}
	} else {
		f = fopen(path.utf8().get_data(), mode_string);
	}

	if (f == nullptr) {
		switch (errno) {
			case ENOENT: {
				last_error = ERR_FILE_NOT_FOUND;
			} break;
			default: {
				last_error = ERR_FILE_CANT_OPEN;
			} break;
		}
		return last_error;
	}

	// Set close on exec to avoid leaking it to subprocesses.
	int fd = fileno(f);

	if (fd != -1) {
		int opts = fcntl(fd, F_GETFD);
		fcntl(fd, F_SETFD, opts | FD_CLOEXEC);
	}

	last_error = OK;
	flags = p_mode_flags;
	return OK;
}

void FileAccessUnix::_close() {
	if (!f) {
		return;
	}

	fclose(f);
	f = nullptr;

	if (close_notification_func) {
		close_notification_func(path, flags);
	}

	if (!save_path.is_empty()) {
		int rename_error = rename(path.utf8().get_data(), save_path.utf8().get_data());

		if (rename_error && close_fail_notify) {
			close_fail_notify(save_path);
		}

		save_path = "";
		ERR_FAIL_COND(rename_error != 0);
	}
}

bool FileAccessUnix::is_open() const {
	return (f != nullptr);
}

String FileAccessUnix::get_path() const {
	return path_src;
}

String FileAccessUnix::get_path_absolute() const {
	return path;
}

#if defined(TOOLS_ENABLED)
String FileAccessUnix::get_real_path() const {
	char *resolved_path = ::realpath(path.utf8().get_data(), nullptr);

	if (!resolved_path) {
		return path;
	}

	String result;
	Error parse_ok = result.append_utf8(resolved_path);
	::free(resolved_path);

	if (parse_ok != OK) {
		return path;
	}

	return result.simplify_path();
}
#endif

void FileAccessUnix::seek(uint64_t p_position) {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");

	if (fseeko(f, p_position, SEEK_SET)) {
		check_errors();
	}
}

void FileAccessUnix::seek_end(int64_t p_position) {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");

	if (fseeko(f, p_position, SEEK_END)) {
		check_errors();
	}
}

uint64_t FileAccessUnix::get_position() const {
	ERR_FAIL_NULL_V_MSG(f, 0, "File must be opened before use.");

	int64_t pos = ftello(f);
	if (pos < 0) {
		check_errors();
		ERR_FAIL_V(0);
	}
	return pos;
}

uint64_t FileAccessUnix::get_length() const {
	ERR_FAIL_NULL_V_MSG(f, 0, "File must be opened before use.");

	int64_t pos = ftello(f);
	ERR_FAIL_COND_V(pos < 0, 0);
	ERR_FAIL_COND_V(fseeko(f, 0, SEEK_END), 0);
	int64_t size = ftello(f);
	ERR_FAIL_COND_V(size < 0, 0);
	ERR_FAIL_COND_V(fseeko(f, pos, SEEK_SET), 0);

	return size;
}

bool FileAccessUnix::eof_reached() const {
	return feof(f);
}

uint64_t FileAccessUnix::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_NULL_V_MSG(f, -1, "File must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	uint64_t read = fread(p_dst, 1, p_length, f);
	check_errors();

	return read;
}

Error FileAccessUnix::get_error() const {
	return last_error;
}

Error FileAccessUnix::resize(int64_t p_length) {
	ERR_FAIL_NULL_V_MSG(f, FAILED, "File must be opened before use.");
	int res = ::ftruncate(fileno(f), p_length);
	switch (res) {
		case 0:
			return OK;
		case EBADF:
			return ERR_FILE_CANT_OPEN;
		case EFBIG:
			return ERR_OUT_OF_MEMORY;
		case EINVAL:
			return ERR_INVALID_PARAMETER;
		default:
			return FAILED;
	}
}

void FileAccessUnix::flush() {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");
	fflush(f);
}

bool FileAccessUnix::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_NULL_V_MSG(f, false, "File must be opened before use.");
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);
	bool res = fwrite(p_src, 1, p_length, f) == p_length;
	check_errors(true);
	return res;
}

bool FileAccessUnix::file_exists(const String &p_path) {
	struct stat st = {};
	const CharString filename_utf8 = fix_path(p_path).utf8();

	// Does the name exist at all?
	if (stat(filename_utf8.get_data(), &st)) {
		return false;
	}

	// See if we have access to the file
	if (access(filename_utf8.get_data(), F_OK)) {
		return false;
	}

	// See if this is a regular file
	switch (st.st_mode & S_IFMT) {
		case S_IFLNK:
		case S_IFREG:
			return true;
		default:
			return false;
	}
}

uint64_t FileAccessUnix::_get_modified_time(const String &p_file) {
	String file = fix_path(p_file);
	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);

	if (!err) {
		uint64_t modified_time = 0;
		if ((st.st_mode & S_IFMT) == S_IFLNK || (st.st_mode & S_IFMT) == S_IFREG || (st.st_mode & S_IFDIR) == S_IFDIR) {
			modified_time = st.st_mtime;
		}
#ifdef ANDROID_ENABLED
		// Workaround for GH-101007
		//FIXME: After saving, all timestamps (st_mtime, st_ctime, st_atime) are set to the same value.
		// After exporting or after some time, only 'modified_time' resets to a past timestamp.
		uint64_t created_time = st.st_ctime;
		if (modified_time < created_time) {
			modified_time = created_time;
		}
#endif
		return modified_time;
	} else {
		return 0;
	}
}

uint64_t FileAccessUnix::_get_access_time(const String &p_file) {
	String file = fix_path(p_file);
	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);

	if (!err) {
		if ((st.st_mode & S_IFMT) == S_IFLNK || (st.st_mode & S_IFMT) == S_IFREG || (st.st_mode & S_IFDIR) == S_IFDIR) {
			return st.st_atime;
		}
	}
	ERR_FAIL_V_MSG(0, "Failed to get access time for: " + p_file + "");
}

int64_t FileAccessUnix::_get_size(const String &p_file) {
	String file = fix_path(p_file);
	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);

	if (!err) {
		if ((st.st_mode & S_IFMT) == S_IFLNK || (st.st_mode & S_IFMT) == S_IFREG) {
			return st.st_size;
		}
	}
	ERR_FAIL_V_MSG(-1, "Failed to get size for: " + p_file + "");
}

uint32_t FileAccessUnix::_get_unix_permissions(const String &p_file) {
	String file = fix_path(p_file);
	struct stat status = {};
	int err = stat(file.utf8().get_data(), &status);

	if (!err) {
		return status.st_mode & 0xFFF; //only permissions
	} else {
		ERR_FAIL_V_MSG(0, "Failed to get unix permissions for: " + p_file + ".");
	}
}

Error FileAccessUnix::_set_unix_permissions(const String &p_file, uint32_t p_permissions) {
	String file = fix_path(p_file);

	int err = chmod(file.utf8().get_data(), p_permissions);
	if (!err) {
		return OK;
	}

	return FAILED;
}

bool FileAccessUnix::_get_hidden_attribute(const String &p_file) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, false, "Failed to get attributes for: " + p_file);

	return (st.st_flags & UF_HIDDEN);
#else
	return false;
#endif
}

Error FileAccessUnix::_set_hidden_attribute(const String &p_file, bool p_hidden) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to get attributes for: " + p_file);

	if (p_hidden) {
		err = chflags(file.utf8().get_data(), st.st_flags | UF_HIDDEN);
	} else {
		err = chflags(file.utf8().get_data(), st.st_flags & ~UF_HIDDEN);
	}
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to set attributes for: " + p_file);
	return OK;
#else
	return ERR_UNAVAILABLE;
#endif
}

bool FileAccessUnix::_get_read_only_attribute(const String &p_file) {
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, false, "Failed to get attributes for: " + p_file);

	return st.st_flags & UF_IMMUTABLE;
#else
	return false;
#endif
}

Error FileAccessUnix::_set_read_only_attribute(const String &p_file, bool p_ro) {
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to get attributes for: " + p_file);

	if (p_ro) {
		err = chflags(file.utf8().get_data(), st.st_flags | UF_IMMUTABLE);
	} else {
		err = chflags(file.utf8().get_data(), st.st_flags & ~UF_IMMUTABLE);
	}
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to set attributes for: " + p_file);
	return OK;
#else
	return ERR_UNAVAILABLE;
#endif
}

PackedByteArray FileAccessUnix::_get_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), PackedByteArray());

	PackedByteArray data;
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	CharString attr_name = p_attribute_name.utf8();
	ssize_t attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), nullptr, 0, 0, 0);
	if (attr_size <= 0) {
		return PackedByteArray();
	}

	data.resize(attr_size);
	attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), (void *)data.ptrw(), data.size(), 0, 0);
	ERR_FAIL_COND_V_MSG(attr_size != data.size(), PackedByteArray(), "Failed to set extended attributes for: " + p_file);
#else
	String file = fix_path(p_file);
	CharString attr_name = ("user." + p_attribute_name).utf8();
	ssize_t attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), nullptr, 0);
	if (attr_size <= 0) {
		return PackedByteArray();
	}

	data.resize(attr_size);
	attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), (void *)data.ptrw(), data.size());
	ERR_FAIL_COND_V_MSG(attr_size != data.size(), PackedByteArray(), "Failed to set extended attributes for: " + p_file);
#endif
	return data;
}

Error FileAccessUnix::_set_extended_attribute(const String &p_file, const String &p_attribute_name, const PackedByteArray &p_data) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	int err = setxattr(file.utf8().get_data(), p_attribute_name.utf8().get_data(), (const void *)p_data.ptr(), p_data.size(), 0, 0);
	if (err != 0) {
		return FAILED;
	}
#else
	String file = fix_path(p_file);
	int err = setxattr(file.utf8().get_data(), ("user." + p_attribute_name).utf8().get_data(), (const void *)p_data.ptr(), p_data.size(), 0);
	if (err != 0) {
		return FAILED;
	}
#endif
	return OK;
}

Error FileAccessUnix::_remove_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	int err = removexattr(file.utf8().get_data(), p_attribute_name.utf8().get_data(), 0);
	if (err != 0) {
		return FAILED;
	}
#else
	String file = fix_path(p_file);
	int err = removexattr(file.utf8().get_data(), ("user." + p_attribute_name).utf8().get_data());
	if (err != 0) {
		return FAILED;
	}
#endif
	return OK;
}

PackedStringArray FileAccessUnix::_get_extended_attributes_list(const String &p_file) {
	PackedStringArray ret;
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	size_t size = listxattr(file.utf8().get_data(), nullptr, 0, 0);
	if (size > 0) {
		PackedByteArray data;
		data.resize(size);
		listxattr(file.utf8().get_data(), (char *)data.ptrw(), data.size(), 0);
		int64_t start = 0;
		for (int64_t x = 0; x < data.size(); x++) {
			if (x != start && data[x] == 0) {
				ret.push_back(String::utf8((const char *)(data.ptr() + start), x - start));
				start = x + 1;
			}
		}
	}
#else
	String file = fix_path(p_file);
	size_t size = listxattr(file.utf8().get_data(), nullptr, 0);
	if (size > 0) {
		PackedByteArray data;
		data.resize(size);
		listxattr(file.utf8().get_data(), (char *)data.ptrw(), data.size());
		int64_t start = 0;
		for (int64_t x = 0; x < data.size(); x++) {
			if (x != start && data[x] == 0) {
				String name = String::utf8((const char *)(data.ptr() + start), x - start);
				if (name.begins_with("user.")) {
					ret.push_back(name.trim_prefix("user."));
				}
				start = x + 1;
			}
		}
	}
#endif
	return ret;
}

void FileAccessUnix::close() {
	_close();
}

FileAccessUnix::CloseNotificationFunc FileAccessUnix::close_notification_func = nullptr;

FileAccessUnix::~FileAccessUnix() {
	_close();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/file_access_unix_pipe.cpp
// ============================================================================


#if defined(UNIX_ENABLED)

#include "core/string/ustring.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>

#ifndef _GNU_SOURCE
typedef typeof(void(int)) *sighandler_t;
#endif

Error FileAccessUnixPipe::open_existing(int p_rfd, int p_wfd, bool p_blocking) {
	// Open pipe using handles created by pipe(fd) call in the OS.execute_with_pipe.
	_close();

	path_src = String();
	unlink_on_close = false;
	ERR_FAIL_COND_V_MSG(fd[0] >= 0 || fd[1] >= 0, ERR_ALREADY_IN_USE, "Pipe is already in use.");
	fd[0] = p_rfd;
	fd[1] = p_wfd;

	if (!p_blocking) {
		fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK);
		fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL) | O_NONBLOCK);
	}

	last_error = OK;
	return OK;
}

Error FileAccessUnixPipe::open_internal(const String &p_path, int p_mode_flags) {
	_close();

	path_src = p_path;
	ERR_FAIL_COND_V_MSG(fd[0] >= 0 || fd[1] >= 0, ERR_ALREADY_IN_USE, "Pipe is already in use.");

	path = String("/tmp/") + p_path.replace("pipe://", "").replace_char('/', '_');
	const CharString path_utf8 = path.utf8();

	struct stat st = {};
	int err = stat(path_utf8.get_data(), &st);
	if (err) {
		if (mkfifo(path_utf8.get_data(), 0600) != 0) {
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}
		unlink_on_close = true;
	} else {
		ERR_FAIL_COND_V_MSG(!S_ISFIFO(st.st_mode), ERR_ALREADY_IN_USE, "Pipe name is already used by file.");
	}

	int f = ::open(path_utf8.get_data(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (f < 0) {
		switch (errno) {
			case ENOENT: {
				last_error = ERR_FILE_NOT_FOUND;
			} break;
			default: {
				last_error = ERR_FILE_CANT_OPEN;
			} break;
		}
		return last_error;
	}

	// Set close on exec to avoid leaking it to subprocesses.
	fd[0] = f;
	fd[1] = f;

	last_error = OK;
	return OK;
}

void FileAccessUnixPipe::_close() {
	if (fd[0] < 0) {
		return;
	}

	if (fd[1] != fd[0]) {
		::close(fd[1]);
	}
	::close(fd[0]);
	fd[0] = -1;
	fd[1] = -1;

	if (unlink_on_close) {
		::unlink(path.utf8().ptr());
	}
	unlink_on_close = false;
}

bool FileAccessUnixPipe::is_open() const {
	return (fd[0] >= 0 || fd[1] >= 0);
}

String FileAccessUnixPipe::get_path() const {
	return path_src;
}

String FileAccessUnixPipe::get_path_absolute() const {
	return path_src;
}

uint64_t FileAccessUnixPipe::get_length() const {
	ERR_FAIL_COND_V_MSG(fd[0] < 0, 0, "Pipe must be opened before use.");

	int buf_rem = 0;
	ERR_FAIL_COND_V(ioctl(fd[0], FIONREAD, &buf_rem) != 0, 0);
	return buf_rem;
}

uint64_t FileAccessUnixPipe::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_COND_V_MSG(fd[0] < 0, -1, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	ssize_t read = ::read(fd[0], p_dst, p_length);
	if (read == -1) {
		last_error = ERR_FILE_CANT_READ;
		read = 0;
	} else if (read != (ssize_t)p_length) {
		last_error = ERR_FILE_CANT_READ;
	} else {
		last_error = OK;
	}
	return read;
}

Error FileAccessUnixPipe::get_error() const {
	return last_error;
}

bool FileAccessUnixPipe::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_COND_V_MSG(fd[1] < 0, false, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);

	sighandler_t sig_pipe = signal(SIGPIPE, SIG_IGN);
	ssize_t ret = ::write(fd[1], p_src, p_length);
	signal(SIGPIPE, sig_pipe);

	if (ret != (ssize_t)p_length) {
		last_error = ERR_FILE_CANT_WRITE;
		return false;
	} else {
		last_error = OK;
		return true;
	}
}

void FileAccessUnixPipe::close() {
	_close();
}

FileAccessUnixPipe::~FileAccessUnixPipe() {
	_close();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/ip_unix.cpp
// ============================================================================

#if defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE)


#include <netdb.h>

#ifdef ANDROID_ENABLED
// We could drop this file once we up our API level to 24,
// where the NDK's ifaddrs.h supports to needed getifaddrs.
#include <thirdparty/misc/ifaddrs-android.h>
#else
#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <ifaddrs.h>
#endif

#include <arpa/inet.h>
#include <sys/socket.h>

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#include <net/if.h> // Order is important on OpenBSD, leave as last.

static IPAddress _sockaddr2ip(struct sockaddr *p_addr) {
	IPAddress ip;

	if (p_addr->sa_family == AF_INET) {
		struct sockaddr_in *addr = (struct sockaddr_in *)p_addr;
		ip.set_ipv4((uint8_t *)&(addr->sin_addr));
	} else if (p_addr->sa_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		ip.set_ipv6(addr6->sin6_addr.s6_addr);
	}

	return ip;
}

void IPUnix::_resolve_hostname(
	List<IPAddress>& r_addresses, const String& p_hostname, IP::Type p_type)
{
	struct addrinfo hints;
	struct addrinfo* result = nullptr;

	memset(&hints, 0, sizeof(struct addrinfo));
	if (p_type == IP::TYPE_IPV4) {
		hints.ai_family = AF_INET;
	}
	else if (p_type == IP::TYPE_IPV6) {
		hints.ai_family = AF_INET6;
		hints.ai_flags = 0;
	}
	else {
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_ADDRCONFIG;
	}
	hints.ai_flags &= ~AI_NUMERICHOST;

	int s = getaddrinfo(p_hostname.utf8().get_data(), nullptr, &hints, &result);
	if (s != 0) {
		print_verbose("getaddrinfo failed! Cannot resolve hostname.");
		return;
	}

	if (result == nullptr || result->ai_addr == nullptr) {
		print_verbose("Invalid response from getaddrinfo.");
		if (result) {
			freeaddrinfo(result);
		}
		return;
	}

	struct addrinfo *next = result;

	do {
		if (next->ai_addr == nullptr) {
			next = next->ai_next;
			continue;
		}
		IPAddress ip = _sockaddr2ip(next->ai_addr);
		if (ip.is_valid() && !r_addresses.find(ip)) {
			r_addresses.push_back(ip);
		}
		next = next->ai_next;
	} while (next);

	freeaddrinfo(result);
}

void IPUnix::get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces)
{
	struct ifaddrs* ifAddrStruct = nullptr;
	struct ifaddrs* ifa = nullptr;
	int family;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if (family != AF_INET && family != AF_INET6) {
			continue;
		}

		HashMap<String, IP::Interface_Info>::Iterator E = r_interfaces->find(ifa->ifa_name);
		if (!E) {
			IP::Interface_Info info;
			info.name = ifa->ifa_name;
			info.name_friendly = ifa->ifa_name;
			info.index = String::num_uint64(if_nametoindex(ifa->ifa_name));
			E = r_interfaces->insert(ifa->ifa_name, info);
			ERR_CONTINUE(!E);
		}

		IP::Interface_Info& info = E->value;
		info.ip_addresses.push_front(_sockaddr2ip(ifa->ifa_addr));
	}

	if (ifAddrStruct != nullptr) {
		freeifaddrs(ifAddrStruct);
	}
}

void IPUnix::make_default()
{
	IP::bind<IPUnix>();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/net_socket_unix.cpp
// ============================================================================

// Some proprietary Unix-derived platforms don't expose Unix sockets
// so this allows skipping this file to reimplement this API differently.
#if defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE)

#include "drivers/unix/os_unix.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#ifdef WEB_ENABLED
#include <arpa/inet.h>
#endif

// BSD calls this flag IPV6_JOIN_GROUP
#if !defined(IPV6_ADD_MEMBERSHIP) && defined(IPV6_JOIN_GROUP)
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif
#if !defined(IPV6_DROP_MEMBERSHIP) && defined(IPV6_LEAVE_GROUP)
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

size_t NetSocketUnix::_set_addr_storage(struct sockaddr_storage *p_addr, const IPAddress &p_ip, uint16_t p_port, IP::Type p_ip_type) {
	memset(p_addr, 0, sizeof(struct sockaddr_storage));
	if (p_ip_type == IP::TYPE_IPV6 || p_ip_type == IP::TYPE_ANY) { // IPv6 socket.

		// IPv6 only socket with IPv4 address.
		ERR_FAIL_COND_V(!p_ip.is_wildcard() && p_ip_type == IP::TYPE_IPV6 && p_ip.is_ipv4(), 0);

		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(p_port);
		if (p_ip.is_valid()) {
			memcpy(&addr6->sin6_addr.s6_addr, p_ip.get_ipv6(), 16);
		} else {
			addr6->sin6_addr = in6addr_any;
		}
		return sizeof(sockaddr_in6);
	} else { // IPv4 socket.

		// IPv4 socket with IPv6 address.
		ERR_FAIL_COND_V(!p_ip.is_wildcard() && !p_ip.is_ipv4(), 0);

		struct sockaddr_in *addr4 = (struct sockaddr_in *)p_addr;
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(p_port); // Short, network byte order.

		if (p_ip.is_valid()) {
			memcpy(&addr4->sin_addr.s_addr, p_ip.get_ipv4(), 4);
		} else {
			addr4->sin_addr.s_addr = INADDR_ANY;
		}

		return sizeof(sockaddr_in);
	}
}

socklen_t NetSocketUnix::_unix_set_sockaddr(struct sockaddr_un *p_addr, const CharString &p_path) {
	memset(p_addr, 0, sizeof(struct sockaddr_un));
	p_addr->sun_family = AF_UNIX;

	// Path must not exceed maximum path length for Unix domain socket
	size_t path_len = p_path.length();
	ERR_FAIL_COND_V(path_len >= sizeof(p_addr->sun_path) - 1, 0);

	// Regular file system socket
	memcpy(p_addr->sun_path, p_path.get_data(), path_len);
	p_addr->sun_path[path_len] = '\0';
	return sizeof(struct sockaddr_un);
}

void NetSocketUnix::_set_ip_port(struct sockaddr_storage *p_addr, IPAddress *r_ip, uint16_t *r_port) {
	if (p_addr->ss_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)p_addr;
		if (r_ip) {
			r_ip->set_ipv4((uint8_t *)&(addr4->sin_addr.s_addr));
		}
		if (r_port) {
			*r_port = ntohs(addr4->sin_port);
		}
	} else if (p_addr->ss_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		if (r_ip) {
			r_ip->set_ipv6(addr6->sin6_addr.s6_addr);
		}
		if (r_port) {
			*r_port = ntohs(addr6->sin6_port);
		}
	}
}

Ref<NetSocket> NetSocketUnix::_create_func() {
	return memnew(NetSocketUnix);
}

void NetSocketUnix::make_default() {
	_create = _create_func;
}

void NetSocketUnix::cleanup() {
}

NetSocketUnix::NetSocketUnix() {
}

NetSocketUnix::~NetSocketUnix() {
	close();
}

// Silence a warning reported in GH-27594.
// EAGAIN and EWOULDBLOCK have the same value on most platforms, but it's not guaranteed.
VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wlogical-op")

NetSocketUnix::NetError NetSocketUnix::_get_socket_error() const {
	if (errno == EISCONN) {
		return ERR_NET_IS_CONNECTED;
	}
	if (errno == EINPROGRESS || errno == EALREADY) {
		return ERR_NET_IN_PROGRESS;
	}
	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return ERR_NET_WOULD_BLOCK;
	}
	if (errno == EADDRINUSE || errno == EINVAL || errno == EADDRNOTAVAIL) {
		return ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE;
	}
	if (errno == EACCES) {
		return ERR_NET_UNAUTHORIZED;
	}
	if (errno == ENOBUFS) {
		return ERR_NET_BUFFER_TOO_SMALL;
	}
	print_verbose("Socket error: " + itos(errno) + ".");
	return ERR_NET_OTHER;
}

VLTR_GCC_WARNING_POP

bool NetSocketUnix::_can_use_ip(const IPAddress &p_ip, const bool p_for_bind) const {
	if (p_for_bind && !(p_ip.is_valid() || p_ip.is_wildcard())) {
		return false;
	} else if (!p_for_bind && !p_ip.is_valid()) {
		return false;
	}
	// Check if socket support this IP type.
	IP::Type type = p_ip.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	return !(_ip_type != IP::TYPE_ANY && !p_ip.is_wildcard() && _ip_type != type);
}

bool NetSocketUnix::_can_use_path(const CharString &p_path) const {
	// Path must not exceed maximum path length for Unix domain socket
	return !p_path.is_empty() && (size_t)p_path.length() < sizeof(((sockaddr_un *)nullptr)->sun_path);
}

_FORCE_INLINE_ Error NetSocketUnix::_change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != Family::INET, ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!_can_use_ip(p_ip, false), ERR_INVALID_PARAMETER);

	// Need to force level and af_family to IP(v4) when using dual stacking and provided multicast group is IPv4.
	IP::Type type = _ip_type == IP::TYPE_ANY && p_ip.is_ipv4() ? IP::TYPE_IPV4 : _ip_type;
	// This needs to be the proper level for the multicast group, no matter if the socket is dual stacking.
	int level = type == IP::TYPE_IPV4 ? IPPROTO_IP : IPPROTO_IPV6;
	int ret = -1;

	IPAddress if_ip;
	uint32_t if_v6id = 0;
	HashMap<String, IP::Interface_Info> if_info;
	IP::get_local_interfaces(&if_info);
	for (KeyValue<String, IP::Interface_Info> &E : if_info) {
		IP::Interface_Info &c = E.value;
		if (c.name != p_if_name) {
			continue;
		}

		if_v6id = (uint32_t)c.index.to_int();
		if (type == IP::TYPE_IPV6) {
			break; // IPv6 uses index.
		}

		for (const IPAddress &F : c.ip_addresses) {
			if (!F.is_ipv4()) {
				continue; // Wrong IP type.
			}
			if_ip = F;
			break;
		}
		break;
	}

	if (level == IPPROTO_IP) {
		ERR_FAIL_COND_V(!if_ip.is_valid(), ERR_INVALID_PARAMETER);
		struct ip_mreq greq;
		int sock_opt = p_add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
		memcpy(&greq.imr_multiaddr, p_ip.get_ipv4(), 4);
		memcpy(&greq.imr_interface, if_ip.get_ipv4(), 4);
		ret = setsockopt(_sock, level, sock_opt, (const char *)&greq, sizeof(greq));
	} else {
		struct ipv6_mreq greq;
		int sock_opt = p_add ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
		memcpy(&greq.ipv6mr_multiaddr, p_ip.get_ipv6(), 16);
		greq.ipv6mr_interface = if_v6id;
		ret = setsockopt(_sock, level, sock_opt, (const char *)&greq, sizeof(greq));
	}
	ERR_FAIL_COND_V(ret != 0, FAILED);

	return OK;
}

void NetSocketUnix::_set_socket(int p_sock, IP::Type p_ip_type, bool p_is_stream) {
	_sock = p_sock;
	_ip_type = p_ip_type;
	_is_stream = p_is_stream;
	// Disable descriptor sharing with subprocesses.
	_set_close_exec_enabled(true);
}

void NetSocketUnix::_set_close_exec_enabled(bool p_enabled) {
	// Enable close on exec to avoid sharing with subprocesses. Off by default on Windows.
	int opts = fcntl(_sock, F_GETFD);
	fcntl(_sock, F_SETFD, opts | FD_CLOEXEC);
}

Error NetSocketUnix::_inet_open(Type p_sock_type, IP::Type &r_ip_type) {
	ERR_FAIL_COND_V(r_ip_type > IP::TYPE_ANY || r_ip_type < IP::TYPE_NONE, ERR_INVALID_PARAMETER);

#if defined(__OpenBSD__)
	// OpenBSD does not support dual stacking, fallback to IPv4 only.
	if (r_ip_type == IP::TYPE_ANY) {
		r_ip_type = IP::TYPE_IPV4;
	}
#endif

	int family = r_ip_type == IP::TYPE_IPV4 ? AF_INET : AF_INET6;
	int protocol = p_sock_type == TYPE_TCP ? IPPROTO_TCP : IPPROTO_UDP;
	int type = p_sock_type == TYPE_TCP ? SOCK_STREAM : SOCK_DGRAM;
	_sock = socket(family, type, protocol);

	if (_sock == -1 && r_ip_type == IP::TYPE_ANY) {
		// Careful here, changing the referenced parameter so the caller knows that we are using an IPv4 socket
		// in place of a dual stack one, and further calls to _set_sock_addr will work as expected.
		r_ip_type = IP::TYPE_IPV4;
		family = AF_INET;
		_sock = socket(family, type, protocol);
	}

	ERR_FAIL_COND_V(_sock == -1, FAILED);
	_ip_type = r_ip_type;
	_family = Family::INET;

	if (family == AF_INET6) {
		// Select IPv4 over IPv6 mapping.
		set_ipv6_only_enabled(r_ip_type != IP::TYPE_ANY);
	}

	if (protocol == IPPROTO_UDP) {
		// Make sure to disable broadcasting for UDP sockets.
		// Depending on the OS, this option might or might not be enabled by default. Let's normalize it.
		set_broadcasting_enabled(false);
	}

	_is_stream = p_sock_type == TYPE_TCP;

	// Disable descriptor sharing with subprocesses.
	_set_close_exec_enabled(true);

#if defined(SO_NOSIGPIPE)
	// Disable SIGPIPE (should only be relevant to stream sockets, but seems to affect UDP too on iOS).
	int par = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_NOSIGPIPE, &par, sizeof(int)) != 0) {
		print_verbose("Unable to turn off SIGPIPE on socket.");
	}
#endif
	return OK;
}

Error NetSocketUnix::_unix_open() {
	_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	ERR_FAIL_COND_V(_sock == -1, FAILED);

	_family = Family::UNIX;

	_set_close_exec_enabled(true);

#if defined(SO_NOSIGPIPE)
	// Disable SIGPIPE (should only be relevant to stream sockets, but seems to affect UDP too on iOS).
	int par = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_NOSIGPIPE, &par, sizeof(int)) != 0) {
		print_verbose("Unable to turn off SIGPIPE on socket.");
	}
#endif

	return OK;
}

Error NetSocketUnix::open(NetSocket::Family p_family, NetSocket::Type p_sock_type, IP::Type &r_ip_type) {
	ERR_FAIL_COND_V(is_open(), ERR_ALREADY_IN_USE);

	switch (p_family) {
		case Family::INET:
			return _inet_open(p_sock_type, r_ip_type);
		case Family::UNIX:
			return _unix_open();
		case Family::NONE:
		default:
			return ERR_INVALID_PARAMETER;
	}
}

void NetSocketUnix::close() {
	if (_sock != -1) {
		::close(_sock);

		if (_family == Family::UNIX) {
			if (_unlink_on_close) {
				::unlink(_unix_path.get_data());
				_unlink_on_close = false;
				_unix_path = CharString();
			}
		}
	}

	_sock = -1;
	_family = Family::NONE;
	_ip_type = IP::TYPE_NONE;
	_is_stream = false;
}

Error NetSocketUnix::_inet_bind(IPAddress p_addr, uint16_t p_port) {
	ERR_FAIL_COND_V(!_can_use_ip(p_addr, true), ERR_INVALID_PARAMETER);

	sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_addr, p_port, _ip_type);

	if (::bind(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		print_verbose("Failed to bind socket. Error: " + itos(err) + ".");
		close();
		return ERR_UNAVAILABLE;
	}

	return OK;
}

Error NetSocketUnix::_unix_bind(const CharString &p_path) {
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_INVALID_PARAMETER);

	struct sockaddr_un addr;
	socklen_t addr_size = _unix_set_sockaddr(&addr, p_path);
	ERR_FAIL_COND_V(addr_size == 0, ERR_INVALID_PARAMETER);

	// If the socket file exists, attempt to remove it.
	if (access(p_path.get_data(), F_OK) == 0) {
		// Check if it's a socket
		struct stat st;
		if (stat(p_path.get_data(), &st) == 0) {
			if (S_ISSOCK(st.st_mode)) {
				// It is a socket, try to remove it.
				if (unlink(p_path.get_data()) != 0) {
					// Failed to remove existing socket file.
					return FAILED;
				}
			} else {
				// It's not a socket, don't remove it.
				return ERR_ALREADY_EXISTS;
			}
		}
	}

	_unlink_on_close = true;

	if (::bind(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		print_verbose("Failed to bind socket. Error: " + itos(err) + ".");
		close();
		switch (err) {
			case ERR_NET_UNAUTHORIZED:
				return ERR_UNAUTHORIZED;
			default:
				return ERR_UNAVAILABLE;
		}
	}

	return OK;
}

Error NetSocketUnix::bind(NetSocket::Address p_addr) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != p_addr.get_family(), ERR_INVALID_PARAMETER);
	switch (p_addr.get_family()) {
		case Family::INET: {
			return _inet_bind(p_addr.ip(), p_addr.port());
		}
		case Family::UNIX: {
			_unix_path = p_addr.get_path();
			return _unix_bind(_unix_path);
		}
		case Family::NONE:
		default:
			return ERR_INVALID_PARAMETER;
	}
}

Error NetSocketUnix::listen(int p_max_pending) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	if (::listen(_sock, p_max_pending) != 0) {
		_get_socket_error();
		print_verbose("Failed to listen from socket.");
		close();
		return FAILED;
	}

	return OK;
}

Error NetSocketUnix::_inet_connect_to_host(IPAddress p_host, uint16_t p_port) {
	ERR_FAIL_COND_V(!_can_use_ip(p_host, false), ERR_INVALID_PARAMETER);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_host, p_port, _ip_type);

	if (::connect(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();

		switch (err) {
			// We are already connected.
			case ERR_NET_IS_CONNECTED:
				return OK;
			// Still waiting to connect, try again in a while.
			case ERR_NET_WOULD_BLOCK:
			case ERR_NET_IN_PROGRESS:
				return ERR_BUSY;
			default:
				print_verbose("Connection to remote host failed.");
				close();
				return FAILED;
		}
	}

	return OK;
}

Error NetSocketUnix::_unix_connect_to_host(const CharString &p_path) {
	ERR_FAIL_COND_V(!_can_use_path(p_path), ERR_INVALID_PARAMETER);

	struct sockaddr_un addr;
	socklen_t addr_size = _unix_set_sockaddr(&addr, p_path);
	ERR_FAIL_COND_V(addr_size == 0, ERR_INVALID_PARAMETER);

	if (::connect(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		switch (err) {
			case ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE:
				return ERR_INVALID_PARAMETER;
			// Still waiting to connect, try again in a while.
			case ERR_NET_WOULD_BLOCK:
			case ERR_NET_IN_PROGRESS:
				return ERR_BUSY;
			case ERR_NET_UNAUTHORIZED:
				return ERR_UNAUTHORIZED;
			default:
				print_verbose("Connection to host failed.");
				close();
				return FAILED;
		}
	}

	return OK;
}

Error NetSocketUnix::connect_to_host(NetSocket::Address p_addr) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != p_addr.get_family(), ERR_INVALID_PARAMETER);

	switch (p_addr.get_family()) {
		case Family::INET:
			return _inet_connect_to_host(p_addr.ip(), p_addr.port());
		case Family::UNIX:
			return _unix_connect_to_host(p_addr.get_path());
		case Family::NONE:
		default:
			return ERR_INVALID_PARAMETER;
	}
}

Error NetSocketUnix::poll(PollType p_type, int p_timeout) const {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	struct pollfd pfd;
	pfd.fd = _sock;
	pfd.events = POLLIN;
	pfd.revents = 0;

	switch (p_type) {
		case POLL_TYPE_IN:
			pfd.events = POLLIN;
			break;
		case POLL_TYPE_OUT:
			pfd.events = POLLOUT;
			break;
		case POLL_TYPE_IN_OUT:
			pfd.events = POLLOUT | POLLIN;
	}

	int ret = ::poll(&pfd, 1, p_timeout);

	if (ret < 0 || pfd.revents & POLLERR) {
		_get_socket_error();
		print_verbose("Error when polling socket.");
		return FAILED;
	}

	if (ret == 0) {
		return ERR_BUSY;
	}

	return OK;
}

Error NetSocketUnix::recv(uint8_t *p_buffer, int p_len, int &r_read) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	r_read = ::recv(_sock, p_buffer, p_len, 0);

	if (r_read < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}

		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketUnix::recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != Family::INET, ERR_UNAVAILABLE);

	struct sockaddr_storage from;
	socklen_t len = sizeof(struct sockaddr_storage);
	memset(&from, 0, len);

	r_read = ::recvfrom(_sock, p_buffer, p_len, p_peek ? MSG_PEEK : 0, (struct sockaddr *)&from, &len);

	if (r_read < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}

		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	if (from.ss_family == AF_INET) {
		struct sockaddr_in *sin_from = (struct sockaddr_in *)&from;
		r_ip.set_ipv4((uint8_t *)&sin_from->sin_addr);
		r_port = ntohs(sin_from->sin_port);
	} else if (from.ss_family == AF_INET6) {
		struct sockaddr_in6 *s6_from = (struct sockaddr_in6 *)&from;
		r_ip.set_ipv6((uint8_t *)&s6_from->sin6_addr);
		r_port = ntohs(s6_from->sin6_port);
	} else {
		// Unsupported socket family, should never happen.
		ERR_FAIL_V(FAILED);
	}

	return OK;
}

Error NetSocketUnix::send(const uint8_t *p_buffer, int p_len, int &r_sent) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	int flags = 0;
#ifdef MSG_NOSIGNAL
	if (_is_stream || _family == Family::UNIX) {
		flags = MSG_NOSIGNAL;
	}
#endif
	r_sent = ::send(_sock, p_buffer, p_len, flags);

	if (r_sent < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}
		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketUnix::sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != Family::INET, ERR_UNAVAILABLE);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_ip, p_port, _ip_type);
	r_sent = ::sendto(_sock, p_buffer, p_len, 0, (struct sockaddr *)&addr, addr_size);

	if (r_sent < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}
		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketUnix::set_broadcasting_enabled(bool p_enabled) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	// IPv6 has no broadcast support.
	if (_ip_type == IP::TYPE_IPV6) {
		return ERR_UNAVAILABLE;
	}

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, &par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change broadcast setting.");
		return FAILED;
	}
	return OK;
}

void NetSocketUnix::set_blocking_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	int ret = 0;
	int opts = fcntl(_sock, F_GETFL);
	if (p_enabled) {
		ret = fcntl(_sock, F_SETFL, opts & ~O_NONBLOCK);
	} else {
		ret = fcntl(_sock, F_SETFL, opts | O_NONBLOCK);
	}

	if (ret != 0) {
		WARN_PRINT("Unable to change non-block mode.");
	}
}

void NetSocketUnix::set_ipv6_only_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	// This option is only available in IPv6 sockets.
	ERR_FAIL_COND(_ip_type == IP::TYPE_IPV4);

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, &par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change IPv4 address mapping over IPv6 option.");
	}
}

void NetSocketUnix::set_tcp_no_delay_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	ERR_FAIL_COND(!_is_stream); // Not TCP.

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, &par, sizeof(int)) < 0) {
		WARN_PRINT("Unable to set TCP no delay option.");
	}
}

void NetSocketUnix::set_reuse_address_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &par, sizeof(int)) < 0) {
		WARN_PRINT("Unable to set socket REUSEADDR option.");
	}
}

bool NetSocketUnix::is_open() const {
	return _sock != -1;
}

int NetSocketUnix::get_available_bytes() const {
	ERR_FAIL_COND_V(!is_open(), -1);

	int len;
	int ret = ioctl(_sock, FIONREAD, &len);
	if (ret == -1) {
		_get_socket_error();
		print_verbose("Error when checking available bytes on socket.");
		return -1;
	}
	return len;
}

Error NetSocketUnix::_inet_get_socket_address(IPAddress *r_ip, uint16_t *r_port) const {
	struct sockaddr_storage saddr;
	socklen_t len = sizeof(saddr);
	if (getsockname(_sock, (struct sockaddr *)&saddr, &len) != 0) {
		_get_socket_error();
		print_verbose("Error when reading local socket address.");
		return FAILED;
	}
	_set_ip_port(&saddr, r_ip, r_port);
	return OK;
}

Error NetSocketUnix::get_socket_address(NetSocket::Address *r_addr) const {
	ERR_FAIL_COND_V(!is_open(), FAILED);
	switch (_family) {
		case Family::INET: {
			IPAddress ip;
			uint16_t port = 0;
			Error res = _inet_get_socket_address(&ip, &port);
			ERR_FAIL_COND_V(res != OK, res);
			if (r_addr) {
				Address addr(ip, port);
				*r_addr = addr;
			}
		} break;
		case Family::UNIX: {
			if (r_addr) {
				*r_addr = Address(_unix_path);
			}
		} break;
		case Family::NONE:
		default:
			return FAILED;
	}

	return OK;
}

Ref<NetSocket> NetSocketUnix::_inet_accept(IPAddress &r_ip, uint16_t &r_port) {
	struct sockaddr_storage their_addr;
	socklen_t size = sizeof(their_addr);
	int fd = ::accept(_sock, (struct sockaddr *)&their_addr, &size);
	if (fd == -1) {
		_get_socket_error();
		print_verbose("Error when accepting socket connection.");
		return Ref<NetSocket>();
	}

	_set_ip_port(&their_addr, &r_ip, &r_port);

	Ref<NetSocketUnix> ns;
	ns.instantiate();
	ns->_set_socket(fd, _ip_type, _is_stream);
	ns->set_blocking_enabled(false);
	return Ref<NetSocket>(ns);
}

Ref<NetSocket> NetSocketUnix::_unix_accept() {
	struct sockaddr_un addr;
	socklen_t addr_len = sizeof(addr);

	int fd = ::accept(_sock, (struct sockaddr *)&addr, &addr_len);
	if (fd == -1) {
		_get_socket_error();
		print_verbose("Error when accepting socket connection.");
		return Ref<NetSocket>();
	}

	Ref<NetSocketUnix> ret = memnew(NetSocketUnix);
	ret->_sock = fd;
	ret->_family = _family;
	ret->_unix_path = _unix_path;
	ret->set_blocking_enabled(false);
	return Ref<NetSocket>(ret);
}

Ref<NetSocket> NetSocketUnix::accept(NetSocket::Address &r_addr) {
	Ref<NetSocket> out;
	ERR_FAIL_COND_V(!is_open(), out);

	switch (_family) {
		case Family::INET: {
			IPAddress ip;
			uint16_t port;
			out = _inet_accept(ip, port);
			if (out.is_valid()) {
				r_addr = Address(ip, port);
			}
		} break;
		case Family::UNIX: {
			out = _unix_accept();
		} break;
		case Family::NONE:
		default:
			break;
	}
	return out;
}

Error NetSocketUnix::join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, true);
}

Error NetSocketUnix::leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, false);
}

#endif // UNIX_ENABLED && !UNIX_SOCKET_UNAVAILABLE

#ifdef IPV6_ADD_MEMBERSHIP
#undef IPV6_ADD_MEMBERSHIP
#endif
#ifdef IPV6_DROP_MEMBERSHIP
#undef IPV6_DROP_MEMBERSHIP
#endif

// ============================================================================
// Merged from drivers/unix/syslog_logger.cpp
// ============================================================================

#ifdef UNIX_ENABLED
// --- Inlined from drivers/unix/syslog_logger.h ---
#ifdef UNIX_ENABLED

#include "core/io/logger.h"

class SyslogLogger : public Logger {
public:
	virtual void logv(const char *p_format, va_list p_list, bool p_err) _PRINTF_FORMAT_ATTRIBUTE_2_0;
	virtual void print_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, ErrorType p_type);

	virtual ~SyslogLogger();
};

#endif // UNIX_ENABLED

// --- End drivers/unix/syslog_logger.h ---

#include <syslog.h>

void SyslogLogger::logv(const char *p_format, va_list p_list, bool p_err) {
	if (!should_log(p_err)) {
		return;
	}

	vsyslog(p_err ? LOG_ERR : LOG_INFO, p_format, p_list);
}

void SyslogLogger::print_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, ErrorType p_type) {
	if (!should_log(true)) {
		return;
	}

	const char *err_type = error_type_string(p_type);

	const char *err_details;
	if (p_rationale && *p_rationale) {
		err_details = p_rationale;
	} else {
		err_details = p_code;
	}

	syslog(p_type == ERR_WARNING ? LOG_WARNING : LOG_ERR, "**%s**: %s\n   At: %s:%i:%s() - %s", err_type, err_details, p_file, p_line, p_function, p_code);
}

SyslogLogger::~SyslogLogger() {
}

#endif

// ============================================================================
// Merged from drivers/unix/thread_posix.cpp
// ============================================================================

#if defined(UNIX_ENABLED)


#include "core/os/thread.h"
#include "core/string/ustring.h"

#if defined(PLATFORM_THREAD_OVERRIDE) && defined(__APPLE__)
void init_thread_posix() {
}
#else

#ifdef PTHREAD_BSD_SET_NAME
#include <pthread_np.h>
#endif

static Error set_name(const String &p_name) {
#ifdef PTHREAD_NO_RENAME
	return ERR_UNAVAILABLE;

#else

#ifdef PTHREAD_RENAME_SELF

	// check if thread is the same as caller
	int err = pthread_setname_np(p_name.utf8().get_data());

#else

	pthread_t running_thread = pthread_self();
#ifdef PTHREAD_BSD_SET_NAME
	pthread_set_name_np(running_thread, p_name.utf8().get_data());
	int err = 0; // Open/FreeBSD ignore errors in this function
#elif defined(PTHREAD_NETBSD_SET_NAME)
	int err = pthread_setname_np(running_thread, "%s", const_cast<char *>(p_name.utf8().get_data()));
#else
	int err = pthread_setname_np(running_thread, p_name.utf8().get_data());
#endif // PTHREAD_BSD_SET_NAME

#endif // PTHREAD_RENAME_SELF

	return err == 0 ? OK : ERR_INVALID_PARAMETER;

#endif // PTHREAD_NO_RENAME
}

void init_thread_posix() {
	Thread::_set_platform_functions({ .set_name = set_name });
}

#endif // PLATFORM_THREAD_OVERRIDE && __APPLE__

#endif // UNIX_ENABLED
