/**************************************************************************/
/*  compression.cpp                                                       */
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

#include "compression.h"

#include "core/io/zip_io.h"

#include <thirdparty/misc/fastlz.h>

#include <zstd.h>

#ifdef BROTLI_ENABLED
#include <brotli/decode.h>
#endif

namespace {
struct ZstdDecompressorContext {
	ZSTD_DCtx *zstd_d_ctx = nullptr;
	bool zstd_long_distance_matching = false;
	int zstd_window_log_size = 0;

	~ZstdDecompressorContext() {
		if (zstd_d_ctx) {
			ZSTD_freeDCtx(zstd_d_ctx);
		}
	}

	void invalidate(bool p_zstd_long_distance_matching, int p_zstd_window_log_size) {
		if (!zstd_d_ctx || zstd_long_distance_matching != p_zstd_long_distance_matching || zstd_window_log_size != p_zstd_window_log_size) {
			if (zstd_d_ctx) {
				ZSTD_freeDCtx(zstd_d_ctx);
			}

			zstd_d_ctx = ZSTD_createDCtx();
			if (p_zstd_long_distance_matching) {
				ZSTD_DCtx_setParameter(zstd_d_ctx, ZSTD_d_windowLogMax, p_zstd_window_log_size);
			}
			zstd_long_distance_matching = p_zstd_long_distance_matching;
			zstd_window_log_size = p_zstd_window_log_size;
		}
	}
};
} //namespace

int64_t Compression::compress(uint8_t *p_dst, const uint8_t *p_src, int64_t p_src_size, Mode p_mode) {
	switch (p_mode) {
		case MODE_BROTLI: {
			ERR_FAIL_V_MSG(-1, "Only brotli decompression is supported.");
		} break;
		case MODE_FASTLZ: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a FastLZ/LZ77 file 2 GiB or larger. LZ77 supports larger files, but FastLZ's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			if (p_src_size < 16) {
				uint8_t src[16];
				memset(&src[p_src_size], 0, 16 - p_src_size);
				memcpy(src, p_src, p_src_size);
				return fastlz_compress(src, 16, p_dst);
			} else {
				return fastlz_compress(p_src, p_src_size, p_dst);
			}

		} break;
		case MODE_DEFLATE:
		case MODE_GZIP: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a Deflate or GZip file 2 GiB or larger. Deflate and GZip are both limited to 4 GiB, and ZLib's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

			z_stream strm;
			strm.zalloc = zipio_alloc;
			strm.zfree = zipio_free;
			strm.opaque = Z_NULL;
			int level = p_mode == MODE_DEFLATE ? zlib_level : gzip_level;
			int err = deflateInit2(&strm, level, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);
			if (err != Z_OK) {
				return -1;
			}

			strm.avail_in = p_src_size;
			int aout = deflateBound(&strm, p_src_size);
			strm.avail_out = aout;
			strm.next_in = (Bytef *)p_src;
			strm.next_out = p_dst;
			deflate(&strm, Z_FINISH);
			aout = aout - strm.avail_out;
			deflateEnd(&strm);
			return aout;

		} break;
		case MODE_ZSTD: {
			ZSTD_CCtx *cctx = ZSTD_createCCtx();
			ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, zstd_level);
			if (zstd_long_distance_matching) {
				ZSTD_CCtx_setParameter(cctx, ZSTD_c_enableLongDistanceMatching, 1);
				ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, zstd_window_log_size);
			}
			const int64_t max_dst_size = get_max_compressed_buffer_size(p_src_size, MODE_ZSTD);
			const size_t ret = ZSTD_compressCCtx(cctx, p_dst, max_dst_size, p_src, p_src_size, zstd_level);
			ZSTD_freeCCtx(cctx);
			return (int64_t)ret;
		} break;
	}

	ERR_FAIL_V(-1);
}

int64_t Compression::get_max_compressed_buffer_size(int64_t p_src_size, Mode p_mode) {
	switch (p_mode) {
		case MODE_BROTLI: {
			ERR_FAIL_V_MSG(-1, "Only brotli decompression is supported.");
		} break;
		case MODE_FASTLZ: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a FastLZ/LZ77 file 2 GiB or larger. LZ77 supports larger files, but FastLZ's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int ss = p_src_size + p_src_size * 6 / 100;
			if (ss < 66) {
				ss = 66;
			}
			return ss;

		} break;
		case MODE_DEFLATE:
		case MODE_GZIP: {
			ERR_FAIL_COND_V_MSG(p_src_size > INT32_MAX, -1, "Cannot compress a Deflate or GZip file 2 GiB or larger. Deflate and GZip are both limited to 4 GiB, and ZLib's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

			z_stream strm;
			strm.zalloc = zipio_alloc;
			strm.zfree = zipio_free;
			strm.opaque = Z_NULL;
			int err = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY);
			if (err != Z_OK) {
				return -1;
			}
			int aout = deflateBound(&strm, p_src_size);
			deflateEnd(&strm);
			return aout;
		} break;
		case MODE_ZSTD: {
			return ZSTD_compressBound(p_src_size);
		} break;
	}

	ERR_FAIL_V(-1);
}

int64_t Compression::decompress(uint8_t *p_dst, int64_t p_dst_max_size, const uint8_t *p_src, int64_t p_src_size, Mode p_mode) {
	switch (p_mode) {
		case MODE_BROTLI: {
#ifdef BROTLI_ENABLED
			size_t ret_size = p_dst_max_size;
			BrotliDecoderResult res = BrotliDecoderDecompress(p_src_size, p_src, &ret_size, p_dst);
			ERR_FAIL_COND_V(res != BROTLI_DECODER_RESULT_SUCCESS, -1);
			return ret_size;
#else
			ERR_FAIL_V_MSG(-1, "Godot was compiled without brotli support.");
#endif
		} break;
		case MODE_FASTLZ: {
			ERR_FAIL_COND_V_MSG(p_dst_max_size > INT32_MAX, -1, "Cannot decompress a FastLZ/LZ77 file 2 GiB or larger. LZ77 supports larger files, but FastLZ's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int ret_size = 0;

			if (p_dst_max_size < 16) {
				uint8_t dst[16];
				fastlz_decompress(p_src, p_src_size, dst, 16);
				memcpy(p_dst, dst, p_dst_max_size);
				ret_size = p_dst_max_size;
			} else {
				ret_size = fastlz_decompress(p_src, p_src_size, p_dst, p_dst_max_size);
			}
			return ret_size;
		} break;
		case MODE_DEFLATE:
		case MODE_GZIP: {
			ERR_FAIL_COND_V_MSG(p_dst_max_size > INT32_MAX, -1, "Cannot decompress a Deflate or GZip file 2 GiB or larger. Deflate and GZip are both limited to 4 GiB, and ZLib's implementation uses C++ `int` so is limited to 2 GiB. Consider using Zstd instead.");
			int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

			z_stream strm;
			strm.zalloc = zipio_alloc;
			strm.zfree = zipio_free;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;
			int err = inflateInit2(&strm, window_bits);
			ERR_FAIL_COND_V(err != Z_OK, -1);

			strm.avail_in = p_src_size;
			strm.avail_out = p_dst_max_size;
			strm.next_in = (Bytef *)p_src;
			strm.next_out = p_dst;

			err = inflate(&strm, Z_FINISH);
			int total = strm.total_out;
			inflateEnd(&strm);
			ERR_FAIL_COND_V(err != Z_STREAM_END, -1);
			return total;
		} break;
		case MODE_ZSTD: {
			thread_local ZstdDecompressorContext decompressor_ctx;
			decompressor_ctx.invalidate(zstd_long_distance_matching, zstd_window_log_size);

			size_t ret = ZSTD_decompressDCtx(decompressor_ctx.zstd_d_ctx, p_dst, p_dst_max_size, p_src, p_src_size);
			return (int64_t)ret;
		} break;
	}

	ERR_FAIL_V(-1);
}

/**
	This will handle both Gzip and Deflate streams. It will automatically allocate the output buffer into the provided p_dst_vect Vector.
	This is required for compressed data whose final uncompressed size is unknown, as is the case for HTTP response bodies.
	This is much slower however than using Compression::decompress because it may result in multiple full copies of the output buffer.
*/
int Compression::decompress_dynamic(Vector<uint8_t> *p_dst_vect, int64_t p_max_dst_size, const uint8_t *p_src, int64_t p_src_size, Mode p_mode) {
	uint8_t *dst = nullptr;
	int out_mark = 0;

	ERR_FAIL_COND_V(p_src_size <= 0, Z_DATA_ERROR);

	if (p_mode == MODE_BROTLI) {
#ifdef BROTLI_ENABLED
		BrotliDecoderResult ret;
		BrotliDecoderState *state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
		ERR_FAIL_NULL_V(state, Z_DATA_ERROR);

		// Setup the stream inputs.
		const uint8_t *next_in = p_src;
		size_t avail_in = p_src_size;
		uint8_t *next_out = nullptr;
		size_t avail_out = 0;
		size_t total_out = 0;

		// Ensure the destination buffer is empty.
		p_dst_vect->clear();

		// Decompress until stream ends or end of file.
		do {
			// Add another chunk size to the output buffer.
			// This forces a copy of the whole buffer.
			p_dst_vect->resize(p_dst_vect->size() + gzip_chunk);
			// Get pointer to the actual output buffer.
			dst = p_dst_vect->ptrw();

			// Set the stream to the new output stream.
			// Since it was copied, we need to reset the stream to the new buffer.
			next_out = &(dst[out_mark]);
			avail_out += gzip_chunk;

			ret = BrotliDecoderDecompressStream(state, &avail_in, &next_in, &avail_out, &next_out, &total_out);
			if (ret == BROTLI_DECODER_RESULT_ERROR) {
				WARN_PRINT(BrotliDecoderErrorString(BrotliDecoderGetErrorCode(state)));
				BrotliDecoderDestroyInstance(state);
				p_dst_vect->clear();
				return Z_DATA_ERROR;
			}

			out_mark += gzip_chunk - avail_out;

			// Enforce max output size.
			if (p_max_dst_size > -1 && total_out > (uint64_t)p_max_dst_size) {
				BrotliDecoderDestroyInstance(state);
				p_dst_vect->clear();
				return Z_BUF_ERROR;
			}
		} while (ret != BROTLI_DECODER_RESULT_SUCCESS);

		// If all done successfully, resize the output if it's larger than the actual output.
		if ((unsigned long)p_dst_vect->size() > total_out) {
			p_dst_vect->resize(total_out);
		}

		// Clean up and return.
		BrotliDecoderDestroyInstance(state);
		return Z_OK;
#else
		ERR_FAIL_V_MSG(Z_ERRNO, "Godot was compiled without brotli support.");
#endif
	} else {
		// This function only supports GZip and Deflate.
		ERR_FAIL_COND_V_MSG(p_mode != MODE_DEFLATE && p_mode != MODE_GZIP, Z_ERRNO, "Dynamic decompression is only supported with gzip, DEFLATE, and Brotli compression methods.");

		int ret;
		z_stream strm;
		int window_bits = p_mode == MODE_DEFLATE ? 15 : 15 + 16;

		// Initialize the stream.
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;

		int err = inflateInit2(&strm, window_bits);
		ERR_FAIL_COND_V(err != Z_OK, -1);

		// Setup the stream inputs.
		strm.next_in = (Bytef *)p_src;
		strm.avail_in = p_src_size;

		// Ensure the destination buffer is empty.
		p_dst_vect->clear();

		// Decompress until deflate stream ends or end of file.
		do {
			// Add another chunk size to the output buffer.
			// This forces a copy of the whole buffer.
			p_dst_vect->resize(p_dst_vect->size() + gzip_chunk);
			// Get pointer to the actual output buffer.
			dst = p_dst_vect->ptrw();

			// Set the stream to the new output stream.
			// Since it was copied, we need to reset the stream to the new buffer.
			strm.next_out = &(dst[out_mark]);
			strm.avail_out = gzip_chunk;

			// Run inflate() on input until output buffer is full and needs to be resized or input runs out.
			do {
				ret = inflate(&strm, Z_SYNC_FLUSH);

				switch (ret) {
					case Z_NEED_DICT:
						ret = Z_DATA_ERROR;
						[[fallthrough]];
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
					case Z_STREAM_ERROR:
					case Z_BUF_ERROR:
						if (strm.msg) {
							WARN_PRINT(strm.msg);
						}
						(void)inflateEnd(&strm);
						p_dst_vect->clear();
						return ret;
				}
			} while (strm.avail_out > 0 && strm.avail_in > 0);

			out_mark += gzip_chunk;

			// Enforce max output size.
			if (p_max_dst_size > -1 && strm.total_out > (uint64_t)p_max_dst_size) {
				(void)inflateEnd(&strm);
				p_dst_vect->clear();
				return Z_BUF_ERROR;
			}
		} while (ret != Z_STREAM_END);

		// If all done successfully, resize the output if it's larger than the actual output.
		if ((unsigned long)p_dst_vect->size() > strm.total_out) {
			p_dst_vect->resize(strm.total_out);
		}

		// Clean up and return.
		(void)inflateEnd(&strm);
		return Z_OK;
	}
}

// ==========================================================================
// delta_encoding.cpp
// ==========================================================================

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

#include <zstd.h>
#include "core/error/error_list.h"
#include "core/templates/vector.h"
#include "core/types.h"
#include "delta_encoding.h"

#define ERR_FAIL_ZSTD_V_MSG(m_result, m_retval, m_msg)                                             \
	ERR_FAIL_COND_V_MSG(ZSTD_isError(m_result), m_retval,                                          \
		vformat("%s Zstandard reported error code %d: \"%s\".", m_msg,                             \
			ZSTD_getErrorCode(m_result), ZSTD_getErrorString(ZSTD_getErrorCode(m_result))))

struct ZstdCompressionContext
{
	ZSTD_CCtx* context = ZSTD_createCCtx();

	~ZstdCompressionContext() { ZSTD_freeCCtx(context); }

	operator ZSTD_CCtx*() { return context; }
};

struct ZstdDecompressionContext
{
	ZSTD_DCtx* context = ZSTD_createDCtx();

	~ZstdDecompressionContext() { ZSTD_freeDCtx(context); }

	operator ZSTD_DCtx*() { return context; }
};

static constexpr uint8_t DELTA_MAGIC[4] = {'G', 'D', 'D', 'L'};
static constexpr uint8_t DELTA_VERSION_NUMBER = 1;
static constexpr size_t DELTA_HEADER_SIZE = 5;

Error DeltaEncoding::encode_delta(Span<uint8_t> p_old_data, Span<uint8_t> p_new_data,
	Vector<uint8_t>& r_delta, int p_compression_level)
{
	size_t zstd_result = ZSTD_compressBound(p_new_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Calculating compression bounds failed.");

	r_delta.reserve_exact(DELTA_HEADER_SIZE + zstd_result);
	r_delta.resize(DELTA_HEADER_SIZE + zstd_result);

	memcpy(r_delta.ptrw(), DELTA_MAGIC, 4);
	r_delta.write[4] = DELTA_VERSION_NUMBER;

	ZstdCompressionContext zstd_context;

	zstd_result =
		ZSTD_CCtx_setParameter(zstd_context, ZSTD_c_compressionLevel, p_compression_level);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting compression level failed.");

	zstd_result = ZSTD_CCtx_setPledgedSrcSize(zstd_context, p_new_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting pledged source size failed.");

	zstd_result = ZSTD_CCtx_refPrefix(zstd_context, p_old_data.ptr(), p_old_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting prefix dictionary failed.");

	zstd_result = ZSTD_CCtx_setParameter(zstd_context, ZSTD_c_contentSizeFlag, 1);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting content size flag failed.");

	zstd_result = ZSTD_CCtx_setParameter(zstd_context, ZSTD_c_checksumFlag, 1);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to encode delta. Setting checksum flag failed.");

	zstd_result = ZSTD_compress2(zstd_context, r_delta.ptrw() + DELTA_HEADER_SIZE,
		r_delta.size() - DELTA_HEADER_SIZE, p_new_data.ptr(), p_new_data.size());
	ERR_FAIL_ZSTD_V_MSG(zstd_result, FAILED, "Failed to encode delta. Compression failed.");

	r_delta.resize(DELTA_HEADER_SIZE + zstd_result);

	return OK;
}

Error DeltaEncoding::decode_delta(
	Span<uint8_t> p_old_data, Span<uint8_t> p_delta, Vector<uint8_t>& r_new_data)
{
	ERR_FAIL_COND_V_MSG(p_delta.size() < DELTA_HEADER_SIZE, ERR_INVALID_DATA,
		vformat("Failed to decode delta. File size (%d) is too small.", p_delta.size()));

	uint8_t magic[4];
	memcpy(magic, p_delta.ptr(), sizeof(magic));
	uint8_t version = p_delta[4];

	ERR_FAIL_COND_V_MSG(memcmp(magic, DELTA_MAGIC, 4) != 0, ERR_FILE_CORRUPT,
		"Failed to decode delta. Header is invalid.");
	ERR_FAIL_COND_V_MSG(version != DELTA_VERSION_NUMBER, ERR_FILE_UNRECOGNIZED,
		vformat("Failed to decode delta. Expected version %d but found %d.", DELTA_VERSION_NUMBER,
			version));

	size_t zstd_result = ZSTD_getFrameContentSize(
		p_delta.ptr() + DELTA_HEADER_SIZE, p_delta.size() - DELTA_HEADER_SIZE);
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to decode delta. Unable to find decompressed size.");

	r_new_data.reserve_exact(zstd_result);
	r_new_data.resize(zstd_result);

	ZstdDecompressionContext zstd_context;

	zstd_result = ZSTD_DCtx_refPrefix(zstd_context, p_old_data.ptr(), p_old_data.size());
	ERR_FAIL_ZSTD_V_MSG(
		zstd_result, FAILED, "Failed to decode delta. Setting prefix dictionary failed.");

	zstd_result = ZSTD_decompressDCtx(zstd_context, r_new_data.ptrw(), r_new_data.size(),
		p_delta.ptr() + DELTA_HEADER_SIZE, p_delta.size() - DELTA_HEADER_SIZE);
	ERR_FAIL_ZSTD_V_MSG(zstd_result, FAILED, "Failed to decode delta. Decompression failed.");
	ERR_FAIL_COND_V(zstd_result != (size_t)r_new_data.size(), ERR_FILE_CORRUPT);

	return OK;
}
#ifdef ERR_FAIL_ZSTD_V_MSG
#undef ERR_FAIL_ZSTD_V_MSG
#endif

// ==========================================================================
// zip_io.cpp
// ==========================================================================

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

#include "zip_io.h"

#include "core/templates/local_vector.h"

int godot_unzip_get_current_file_info(unzFile p_zip_file, unz_file_info64 &r_file_info, String &r_filepath) {
	const uLong short_file_path_buffer_size = 16384ul;
	char short_file_path_buffer[short_file_path_buffer_size];

	int err = unzGetCurrentFileInfo64(p_zip_file, &r_file_info, short_file_path_buffer, short_file_path_buffer_size, nullptr, 0, nullptr, 0);
	if (unlikely((err != UNZ_OK) || (r_file_info.size_filename > short_file_path_buffer_size))) {
		LocalVector<char> long_file_path_buffer;
		long_file_path_buffer.resize(r_file_info.size_filename);

		err = unzGetCurrentFileInfo64(p_zip_file, &r_file_info, long_file_path_buffer.ptr(), long_file_path_buffer.size(), nullptr, 0, nullptr, 0);
		if (err != UNZ_OK) {
			return err;
		}
		r_filepath = String::utf8(long_file_path_buffer.ptr(), r_file_info.size_filename);
	} else {
		r_filepath = String::utf8(short_file_path_buffer, r_file_info.size_filename);
	}

	return err;
}

int godot_unzip_locate_file(unzFile p_zip_file, const String &p_filepath, bool p_case_sensitive) {
	int err = unzGoToFirstFile(p_zip_file);
	while (err == UNZ_OK) {
		unz_file_info64 current_file_info;
		String current_filepath;
		err = godot_unzip_get_current_file_info(p_zip_file, current_file_info, current_filepath);
		if (err == UNZ_OK) {
			bool filepaths_are_equal = p_case_sensitive ? (p_filepath == current_filepath) : (p_filepath.nocasecmp_to(current_filepath) == 0);
			if (filepaths_are_equal) {
				return UNZ_OK;
			}
			err = unzGoToNextFile(p_zip_file);
		}
	}
	return err;
}

//

void *zipio_open(voidpf p_opaque, const char *p_fname, int p_mode) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, nullptr);

	String fname = String::utf8(p_fname);

	int file_access_mode = 0;
	if (p_mode & ZLIB_FILEFUNC_MODE_READ) {
		file_access_mode |= FileAccess::READ;
	}
	if (p_mode & ZLIB_FILEFUNC_MODE_WRITE) {
		file_access_mode |= FileAccess::WRITE;
	}
	if (p_mode & ZLIB_FILEFUNC_MODE_CREATE) {
		file_access_mode |= FileAccess::WRITE_READ;
	}

	(*fa) = FileAccess::open(fname, file_access_mode);
	if (fa->is_null()) {
		return nullptr;
	}

	return p_opaque;
}

uLong zipio_read(voidpf p_opaque, voidpf p_stream, void *p_buf, uLong p_size) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return (*fa)->get_buffer((uint8_t *)p_buf, p_size);
}

uLong zipio_write(voidpf p_opaque, voidpf p_stream, const void *p_buf, uLong p_size) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	bool fa_success = (*fa)->store_buffer((uint8_t *)p_buf, p_size);

	if (fa_success) {
		return p_size;
	}

	return 0;
}

long zipio_tell(voidpf p_opaque, voidpf p_stream) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return (*fa)->get_position();
}

long zipio_seek(voidpf p_opaque, voidpf p_stream, uLong p_offset, int p_origin) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	uint64_t pos = p_offset;
	switch (p_origin) {
		case ZLIB_FILEFUNC_SEEK_CUR:
			pos = (*fa)->get_position() + p_offset;
			break;
		case ZLIB_FILEFUNC_SEEK_END:
			pos = (*fa)->get_length() + p_offset;
			break;
		default:
			break;
	}

	(*fa)->seek(pos);
	return 0;
}

int zipio_close(voidpf p_opaque, voidpf p_stream) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 0);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	fa->unref();
	return 0;
}

int zipio_testerror(voidpf p_opaque, voidpf p_stream) {
	Ref<FileAccess> *fa = reinterpret_cast<Ref<FileAccess> *>(p_opaque);
	ERR_FAIL_NULL_V(fa, 1);
	ERR_FAIL_COND_V(fa->is_null(), 0);

	return (fa->is_valid() && (*fa)->get_error() != OK) ? 1 : 0;
}

voidpf zipio_alloc(voidpf p_opaque, uInt p_items, uInt p_size) {
	voidpf ptr = memalloc_zeroed((size_t)p_items * p_size);
	return ptr;
}

void zipio_free(voidpf p_opaque, voidpf r_address) {
	memfree(r_address);
}

zlib_filefunc_def zipio_create_io(Ref<FileAccess> *p_data) {
	zlib_filefunc_def io;
	io.opaque = (void *)p_data;
	io.zopen_file = zipio_open;
	io.zread_file = zipio_read;
	io.zwrite_file = zipio_write;
	io.ztell_file = zipio_tell;
	io.zseek_file = zipio_seek;
	io.zclose_file = zipio_close;
	io.zerror_file = zipio_testerror;
	io.alloc_mem = zipio_alloc;
	io.free_mem = zipio_free;
	return io;
}

// ==========================================================================
// pck_packer.cpp
// ==========================================================================

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

#include "core/crypto/crypto_core.h"
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/version.h"
#include "pck_packer.h"

static int _get_pad(int p_alignment, int p_n)
{
	int rest = p_n % p_alignment;
	int pad = 0;
	if (rest > 0) {
		pad = p_alignment - rest;
	}

	return pad;
}

Error PCKPacker::pck_start(
	const String& p_pck_path, int p_alignment, const String& p_key, bool p_encrypt_directory)
{
	ERR_FAIL_COND_V_MSG(
		(p_key.is_empty() || !p_key.is_valid_hex_number(false) || p_key.length() != 64),
		ERR_CANT_CREATE, "Invalid Encryption Key (must be 64 characters long).");
	ERR_FAIL_COND_V_MSG(
		p_alignment <= 0, ERR_CANT_CREATE, "Invalid alignment, must be greater then 0.");

	String _key = p_key.to_lower();
	key.resize(32);
	for (int i = 0; i < 32; i++) {
		int v = 0;
		if (i * 2 < _key.length()) {
			char32_t ct = _key[i * 2];
			if (is_digit(ct)) {
				ct = ct - '0';
			}
			else if (ct >= 'a' && ct <= 'f') {
				ct = 10 + ct - 'a';
			}
			v |= ct << 4;
		}

		if (i * 2 + 1 < _key.length()) {
			char32_t ct = _key[i * 2 + 1];
			if (is_digit(ct)) {
				ct = ct - '0';
			}
			else if (ct >= 'a' && ct <= 'f') {
				ct = 10 + ct - 'a';
			}
			v |= ct;
		}
		key.write[i] = v;
	}
	enc_dir = p_encrypt_directory;

	file = FileAccess::open(p_pck_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_CANT_CREATE,
		vformat("Can't open file to write: '%s'.", String(p_pck_path)));

	alignment = p_alignment;

	file->store_32(PACK_HEADER_MAGIC);
	file->store_32(PACK_FORMAT_VERSION);
	file->store_32(VLTR_VERSION_MAJOR);
	file->store_32(VLTR_VERSION_MINOR);
	file->store_32(VLTR_VERSION_PATCH);

	uint32_t pack_flags = PACK_REL_FILEBASE;
	if (enc_dir) {
		pack_flags |= PACK_DIR_ENCRYPTED;
	}
	file->store_32(pack_flags); // flags

	file_base_ofs = file->get_position();
	file->store_64(0); // Files base.

	dir_base_ofs = file->get_position();
	file->store_64(0); // Directory offset.

	for (int i = 0; i < 16; i++) {
		file->store_32(0); // Reserved.
	}

	// Align for first file.
	int pad = _get_pad(alignment, file->get_position());
	for (int i = 0; i < pad; i++) {
		file->store_8(0);
	}

	file_base = file->get_position();
	file->seek(file_base_ofs);
	file->store_64(file_base); // Update files base.
	file->seek(file_base);

	files.clear();

	return OK;
}

Error PCKPacker::add_file_removal(const String& p_target_path)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	File pf;
	// Simplify path here and on every 'files' access so that paths that have extra '/'
	// symbols or 'res://' in them still match the MD5 hash for the saved path.
	pf.path = p_target_path.simplify_path().trim_prefix("res://");
	pf.ofs = file->get_position();
	pf.size = 0;
	pf.removal = true;

	pf.md5.resize_initialized(16);

	files.push_back(pf);

	return OK;
}

Error PCKPacker::add_file(const String& p_target_path, const String& p_source_path, bool p_encrypt)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	Ref<FileAccess> f = FileAccess::open(p_source_path, FileAccess::READ);
	if (f.is_null()) {
		return ERR_FILE_CANT_OPEN;
	}
	Vector<uint8_t> data = FileAccess::get_file_as_bytes(p_source_path);

	return _add_file(p_target_path, p_source_path, data, p_encrypt);
}

Error PCKPacker::add_file_from_buffer(
	const String& p_target_path, const Vector<uint8_t>& p_data, bool p_encrypt)
{
	return _add_file(p_target_path, "<PackedByteArray>", p_data, p_encrypt);
}

Error PCKPacker::_add_file(const String& p_target_path, const String& p_source_path,
	const Vector<uint8_t>& p_data, bool p_encrypt)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	File pf;
	// Simplify path here and on every 'files' access so that paths that have extra '/'
	// symbols or 'res://' in them still match the MD5 hash for the saved path.
	pf.path = p_target_path.simplify_path().trim_prefix("res://");
	pf.src_path = p_source_path;
	pf.ofs = file->get_position();
	pf.size = p_data.size();

	{
		unsigned char hash[16];
		CryptoCore::md5(p_data.ptr(), p_data.size(), hash);
		pf.md5.resize(16);
		for (int i = 0; i < 16; i++) {
			pf.md5.write[i] = hash[i];
		}
	}
	pf.encrypted = p_encrypt;

	Ref<FileAccess> ftmp = file;

	Ref<FileAccessEncrypted> fae;
	if (p_encrypt) {
		fae.instantiate();
		ERR_FAIL_COND_V(fae.is_null(), ERR_CANT_CREATE);

		Error err = fae->open_and_parse(file, key, FileAccessEncrypted::MODE_WRITE_AES256, false);
		ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);
		ftmp = fae;
	}

	ftmp->store_buffer(p_data);

	if (fae.is_valid()) {
		ftmp.unref();
		fae.unref();
	}

	int pad = _get_pad(alignment, file->get_position());
	for (int j = 0; j < pad; j++) {
		file->store_8(0);
	}

	files.push_back(pf);

	return OK;
}

Error PCKPacker::flush(bool p_verbose)
{
	ERR_FAIL_COND_V_MSG(file.is_null(), ERR_INVALID_PARAMETER, "File must be opened before use.");

	int dir_padding = _get_pad(alignment, file->get_position());
	for (int i = 0; i < dir_padding; i++) {
		file->store_8(0);
	}

	// Write directory.
	uint64_t dir_offset = file->get_position();
	file->seek(dir_base_ofs);
	file->store_64(dir_offset);
	file->seek(dir_offset);

	file->store_32(uint32_t(files.size()));

	Ref<FileAccessEncrypted> fae;
	Ref<FileAccess> fhead = file;

	if (enc_dir) {
		fae.instantiate();
		ERR_FAIL_COND_V(fae.is_null(), ERR_CANT_CREATE);

		Error err = fae->open_and_parse(file, key, FileAccessEncrypted::MODE_WRITE_AES256, false);
		ERR_FAIL_COND_V(err != OK, ERR_CANT_CREATE);

		fhead = fae;
	}

	const int file_num = files.size();
	for (int i = 0; i < file_num; i++) {
		CharString utf8_string = files[i].path.utf8();
		int string_len = utf8_string.length();
		int pad = _get_pad(4, string_len);

		fhead->store_32(uint32_t(string_len + pad));
		fhead->store_buffer((const uint8_t*)utf8_string.get_data(), string_len);
		for (int j = 0; j < pad; j++) {
			fhead->store_8(0);
		}

		fhead->store_64(files[i].ofs - file_base);
		fhead->store_64(files[i].size);
		fhead->store_buffer(files[i].md5.ptr(), 16);

		uint32_t flags = 0;
		if (files[i].encrypted) {
			flags |= PACK_FILE_ENCRYPTED;
		}
		if (files[i].removal) {
			flags |= PACK_FILE_REMOVAL;
		}
		fhead->store_32(flags);

		if (p_verbose) {
			__print_line(vformat("[%d/%d - %d%%] PCKPacker flush: %s -> %s", i + 1, file_num,
				float(i + 1) / file_num * 100, files[i].src_path, files[i].path));
		}
	}

	if (fae.is_valid()) {
		fhead.unref();
		fae.unref();
	}

	file.unref();
	return OK;
}

PCKPacker::~PCKPacker()
{
	if (file.is_valid()) {
		flush();
	}
}

// ==========================================================================
// marshalls.cpp
// ==========================================================================

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

#include <climits>
#include <cstdio>
#include "core/io/resource_loader.h"
#include "marshalls.h"
#include "core/types.h"

#define ERR_FAIL_ADD_OF(a, b, err)                                                                 \
	ERR_FAIL_COND_V(                                                                               \
		((int32_t)(b)) < 0 || ((int32_t)(a)) < 0 || ((int32_t)(a)) > INT_MAX - ((int32_t)(b)),     \
		err)
#define ERR_FAIL_MUL_OF(a, b, err)                                                                 \
	ERR_FAIL_COND_V(                                                                               \
		((int32_t)(a)) < 0 || ((int32_t)(b)) <= 0 || ((int32_t)(a)) > INT_MAX / ((int32_t)(b)),    \
		err)

// Byte 0: `Variant::Type`, byte 1: unused, bytes 2 and 3: additional data.
#define HEADER_TYPE_MASK 0xFF

// For `Variant::INT`, `Variant::FLOAT` and other math types.
#define HEADER_DATA_FLAG_64 (1 << 16)

// For `Variant::OBJECT`.
#define HEADER_DATA_FLAG_OBJECT_AS_ID (1 << 16)

// For `Variant::ARRAY`.
// Occupies bits 16 and 17.
#define HEADER_DATA_FIELD_TYPED_ARRAY_MASK (0b11 << 16)
#define HEADER_DATA_FIELD_TYPED_ARRAY_SHIFT 16

// For `Variant::DICTIONARY`.
// Occupies bits 16 and 17.
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_MASK (0b11 << 16)
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_SHIFT 16
// Occupies bits 18 and 19.
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_MASK (0b11 << 18)
#define HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_SHIFT 18

enum ContainerTypeKind
{
	CONTAINER_TYPE_KIND_NONE = 0b00,
	CONTAINER_TYPE_KIND_BUILTIN = 0b01,
	CONTAINER_TYPE_KIND_CLASS_NAME = 0b10,
	CONTAINER_TYPE_KIND_SCRIPT = 0b11,
};

#define GET_CONTAINER_TYPE_KIND(m_header, m_field)                                                 \
	((ContainerTypeKind)(((m_header)&HEADER_DATA_FIELD_##m_field##_MASK) >>                        \
						 HEADER_DATA_FIELD_##m_field##_SHIFT))

static Error _decode_string(const uint8_t*& p_buffer, int& r_left, int* r_len, String& r_string)
{
	ERR_FAIL_COND_V(r_left < 4, ERR_INVALID_DATA);

	int32_t strlen = decode_uint32(p_buffer);
	int32_t pad = 0;

	// Handle padding.
	if (strlen % 4) {
		pad = 4 - strlen % 4;
	}

	p_buffer += 4;
	r_left -= 4;

	// Ensure buffer is big enough.
	ERR_FAIL_ADD_OF(strlen, pad, ERR_FILE_EOF);
	ERR_FAIL_COND_V(strlen < 0 || strlen + pad > r_left, ERR_FILE_EOF);

	String str;
	ERR_FAIL_COND_V(str.append_utf8((const char*)p_buffer, strlen) != OK, ERR_INVALID_DATA);
	r_string = str;

	// Add padding.
	strlen += pad;

	// Update buffer pos, left data count, and return size.
	p_buffer += strlen;
	r_left -= strlen;
	if (r_len) {
		(*r_len) += 4 + strlen;
	}

	return OK;
}

static void _encode_string(const String& p_string, uint8_t*& p_buffer, int& r_len)
{
	CharString utf8 = p_string.utf8();

	if (p_buffer) {
		encode_uint32(utf8.length(), p_buffer);
		p_buffer += 4;
		memcpy(p_buffer, utf8.get_data(), utf8.length());
		p_buffer += utf8.length();
	}

	r_len += 4 + utf8.length();
	while (r_len % 4) {
		r_len++; // Pad.
		if (p_buffer) {
			*(p_buffer++) = 0;
		}
	}
}

Vector<float> vector3_to_float32_array(const Vector3* p_vecs, size_t p_count)
{
	// We always allocate a new array, and we don't `memcpy()`.
	// We also don't consider returning a pointer to the passed vectors when `sizeof(real_t) == 4`.
	// One reason is that we could decide to put a 4th component in `Vector3` for SIMD/mobile
	// performance, which would cause trouble with these optimizations.
	Vector<float> floats;
	if (p_count == 0) {
		return floats;
	}
	floats.resize(p_count * 3);
	float* floats_w = floats.ptrw();
	for (size_t i = 0; i < p_count; ++i) {
		const Vector3 v = p_vecs[i];
		floats_w[0] = v.x;
		floats_w[1] = v.y;
		floats_w[2] = v.z;
		floats_w += 3;
	}
	return floats;
}
#ifdef ERR_FAIL_ADD_OF
#undef ERR_FAIL_ADD_OF
#endif
#ifdef ERR_FAIL_MUL_OF
#undef ERR_FAIL_MUL_OF
#endif
#ifdef HEADER_TYPE_MASK
#undef HEADER_TYPE_MASK
#endif
#ifdef HEADER_DATA_FLAG_64
#undef HEADER_DATA_FLAG_64
#endif
#ifdef HEADER_DATA_FLAG_OBJECT_AS_ID
#undef HEADER_DATA_FLAG_OBJECT_AS_ID
#endif
#ifdef HEADER_DATA_FIELD_TYPED_ARRAY_MASK
#undef HEADER_DATA_FIELD_TYPED_ARRAY_MASK
#endif
#ifdef HEADER_DATA_FIELD_TYPED_ARRAY_SHIFT
#undef HEADER_DATA_FIELD_TYPED_ARRAY_SHIFT
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_MASK
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_MASK
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_SHIFT
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_KEY_SHIFT
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_MASK
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_MASK
#endif
#ifdef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_SHIFT
#undef HEADER_DATA_FIELD_TYPED_DICTIONARY_VALUE_SHIFT
#endif
#ifdef GET_CONTAINER_TYPE_KIND
#undef GET_CONTAINER_TYPE_KIND
#endif

// ==========================================================================
// packed_data_container.cpp
// ==========================================================================

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

#include "core/io/marshalls.h"
#include "packed_data_container.h"

int PackedDataContainer::size() const { return _size(0); }

uint32_t PackedDataContainer::_type_at_ofs(uint32_t p_ofs) const
{
	ERR_FAIL_COND_V(p_ofs + 4 > (uint32_t)data.size(), 0);
	const uint8_t* rd = data.ptr();
	ERR_FAIL_NULL_V(rd, 0);
	const uint8_t* r = &rd[p_ofs];
	uint32_t type = decode_uint32(r);

	return type;
}

int PackedDataContainer::_size(uint32_t p_ofs) const
{
	ERR_FAIL_COND_V(p_ofs + 4 > (uint32_t)data.size(), 0);
	const uint8_t* rd = data.ptr();
	ERR_FAIL_NULL_V(rd, 0);
	const uint8_t* r = &rd[p_ofs];
	uint32_t type = decode_uint32(r);

	if (type == TYPE_ARRAY) {
		uint32_t len = decode_uint32(r + 4);
		return len;

	}
	else if (type == TYPE_DICT) {
		uint32_t len = decode_uint32(r + 4);
		return len;
	}

	return -1;
}

void PackedDataContainer::_set_data(const Vector<uint8_t>& p_data)
{
	data = p_data;
	datalen = data.size();
}

Vector<uint8_t> PackedDataContainer::_get_data() const { return data; }

int PackedDataContainerRef::size() const { return from->_size(offset); }
