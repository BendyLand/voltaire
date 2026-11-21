/**************************************************************************/
/*  apple_export_utils.cpp                                                */
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

#include "apple_export_utils.h"

#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/plist.h"
#include "editor/file_system/editor_paths.h"
#include "modules/regex/regex.h"

#include <ctime>


/*************************************************************************/
/* MachO                                                                 */
/*************************************************************************/

uint32_t MachO::seg_align(uint64_t p_vmaddr, uint32_t p_min, uint32_t p_max) {
	uint32_t salign = p_max;
	if (p_vmaddr != 0) {
		uint64_t seg_align = 1;
		salign = 0;
		while ((seg_align & p_vmaddr) == 0) {
			seg_align = seg_align << 1;
			salign++;
		}
		salign = CLAMP(salign, p_min, p_max);
	}
	return salign;
}

bool MachO::alloc_signature(uint64_t p_size) {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "MachO: File not opened.");
	if (signature_offset != 0) {
		// Nothing to do, already have signature load command.
		return true;
	}
	if (lc_limit == 0 || lc_limit + 16 > exe_base) {
		ERR_FAIL_V_MSG(false, "MachO: Can't allocate signature load command, please use \"codesign_allocate\" utility first.");
	} else {
		// Add signature load command.
		signature_offset = lc_limit;

		fa->seek(lc_limit);
		LoadCommandHeader lc;
		lc.cmd = LC_CODE_SIGNATURE;
		lc.cmdsize = 16;
		if (swap) {
			lc.cmdsize = BSWAP32(lc.cmdsize);
		}
		fa->store_buffer((const uint8_t *)&lc, sizeof(LoadCommandHeader));

		uint32_t lc_offset = fa->get_length() + PAD(fa->get_length(), 16);
		uint32_t lc_size = 0;
		if (swap) {
			lc_offset = BSWAP32(lc_offset);
			lc_size = BSWAP32(lc_size);
		}
		fa->store_32(lc_offset);
		fa->store_32(lc_size);

		// Write new command number.
		fa->seek(0x10);
		uint32_t ncmds = fa->get_32();
		uint32_t cmdssize = fa->get_32();
		if (swap) {
			ncmds = BSWAP32(ncmds);
			cmdssize = BSWAP32(cmdssize);
		}
		ncmds += 1;
		cmdssize += 16;
		if (swap) {
			ncmds = BSWAP32(ncmds);
			cmdssize = BSWAP32(cmdssize);
		}
		fa->seek(0x10);
		fa->store_32(ncmds);
		fa->store_32(cmdssize);

		lc_limit = lc_limit + sizeof(LoadCommandHeader) + 8;

		return true;
	}
}

bool MachO::is_macho(const String &p_path) {
	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("MachO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fb->get_32();
	return (magic == 0xcefaedfe || magic == 0xfeedface || magic == 0xcffaedfe || magic == 0xfeedfacf);
}

uint32_t MachO::get_filetype(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, vformat("MachO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fa->get_32();
	MachHeader mach_header;

	// Read MachO header.
	if (magic == 0xcefaedfe || magic == 0xfeedface) {
		// Thin 32-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
	} else if (magic == 0xcffaedfe || magic == 0xfeedfacf) {
		// Thin 64-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
		fa->get_32(); // Skip extra reserved field.
	} else {
		ERR_FAIL_V_MSG(0, vformat("MachO: File is not a valid MachO binary: \"%s\".", p_path));
	}
	return mach_header.filetype;
}

bool MachO::open_file(const String &p_path) {
	fa = FileAccess::open(p_path, FileAccess::READ_WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("MachO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fa->get_32();
	MachHeader mach_header;

	// Read MachO header.
	swap = (magic == 0xcffaedfe || magic == 0xcefaedfe);
	if (magic == 0xcefaedfe || magic == 0xfeedface) {
		// Thin 32-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
	} else if (magic == 0xcffaedfe || magic == 0xfeedfacf) {
		// Thin 64-bit binary.
		fa->get_buffer((uint8_t *)&mach_header, sizeof(MachHeader));
		fa->get_32(); // Skip extra reserved field.
	} else {
		ERR_FAIL_V_MSG(false, vformat("MachO: File is not a valid MachO binary: \"%s\".", p_path));
	}

	if (swap) {
		mach_header.ncmds = BSWAP32(mach_header.ncmds);
		mach_header.cpusubtype = BSWAP32(mach_header.cpusubtype);
		mach_header.cputype = BSWAP32(mach_header.cputype);
	}
	cpusubtype = mach_header.cpusubtype;
	cputype = mach_header.cputype;
	align = 0;
	exe_base = std::numeric_limits<uint64_t>::max();
	exe_limit = 0;
	lc_limit = 0;
	link_edit_offset = 0;
	signature_offset = 0;

	// Read load commands.
	for (uint32_t i = 0; i < mach_header.ncmds; i++) {
		LoadCommandHeader lc;
		fa->get_buffer((uint8_t *)&lc, sizeof(LoadCommandHeader));
		if (swap) {
			lc.cmd = BSWAP32(lc.cmd);
			lc.cmdsize = BSWAP32(lc.cmdsize);
		}
		uint64_t ps = fa->get_position();
		switch (lc.cmd) {
			case LC_SEGMENT: {
				LoadCommandSegment lc_seg;
				fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment));
				if (swap) {
					lc_seg.nsects = BSWAP32(lc_seg.nsects);
					lc_seg.vmaddr = BSWAP32(lc_seg.vmaddr);
					lc_seg.vmsize = BSWAP32(lc_seg.vmsize);
				}
				align = MAX(align, seg_align(lc_seg.vmaddr, 2, 15));
				if (String(lc_seg.segname) == "__TEXT") {
					exe_limit = MAX(exe_limit, lc_seg.vmsize);
					for (uint32_t j = 0; j < lc_seg.nsects; j++) {
						Section lc_sect;
						fa->get_buffer((uint8_t *)&lc_sect, sizeof(Section));
						if (String(lc_sect.sectname) == "__text") {
							if (swap) {
								exe_base = MIN(exe_base, BSWAP32(lc_sect.offset));
							} else {
								exe_base = MIN(exe_base, lc_sect.offset);
							}
						}
						if (swap) {
							align = MAX(align, BSWAP32(lc_sect.align));
						} else {
							align = MAX(align, lc_sect.align);
						}
					}
				} else if (String(lc_seg.segname) == "__LINKEDIT") {
					link_edit_offset = ps - 8;
				}
			} break;
			case LC_SEGMENT_64: {
				LoadCommandSegment64 lc_seg;
				fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment64));
				if (swap) {
					lc_seg.nsects = BSWAP32(lc_seg.nsects);
					lc_seg.vmaddr = BSWAP64(lc_seg.vmaddr);
					lc_seg.vmsize = BSWAP64(lc_seg.vmsize);
				}
				align = MAX(align, seg_align(lc_seg.vmaddr, 3, 15));
				if (String(lc_seg.segname) == "__TEXT") {
					exe_limit = MAX(exe_limit, lc_seg.vmsize);
					for (uint32_t j = 0; j < lc_seg.nsects; j++) {
						Section64 lc_sect;
						fa->get_buffer((uint8_t *)&lc_sect, sizeof(Section64));
						if (String(lc_sect.sectname) == "__text") {
							if (swap) {
								exe_base = MIN(exe_base, BSWAP32(lc_sect.offset));
							} else {
								exe_base = MIN(exe_base, lc_sect.offset);
							}
							if (swap) {
								align = MAX(align, BSWAP32(lc_sect.align));
							} else {
								align = MAX(align, lc_sect.align);
							}
						}
					}
				} else if (String(lc_seg.segname) == "__LINKEDIT") {
					link_edit_offset = ps - 8;
				}
			} break;
			case LC_CODE_SIGNATURE: {
				signature_offset = ps - 8;
			} break;
			default: {
			} break;
		}
		fa->seek(ps + lc.cmdsize - 8);
		lc_limit = ps + lc.cmdsize - 8;
	}

	if (exe_limit == 0 || lc_limit == 0) {
		ERR_FAIL_V_MSG(false, vformat("MachO: No load commands or executable code found: \"%s\".", p_path));
	}

	return true;
}

uint64_t MachO::get_exe_base() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return exe_base;
}

uint64_t MachO::get_exe_limit() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return exe_limit;
}

int32_t MachO::get_align() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return align;
}

uint32_t MachO::get_cputype() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return cputype;
}

uint32_t MachO::get_cpusubtype() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return cpusubtype;
}

uint64_t MachO::get_size() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	return fa->get_length();
}

uint64_t MachO::get_signature_offset() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	ERR_FAIL_COND_V_MSG(signature_offset == 0, 0, "MachO: No signature load command.");

	fa->seek(signature_offset + 8);
	if (swap) {
		return BSWAP32(fa->get_32());
	} else {
		return fa->get_32();
	}
}

uint64_t MachO::get_code_limit() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");

	if (signature_offset == 0) {
		return fa->get_length() + PAD(fa->get_length(), 16);
	} else {
		return get_signature_offset();
	}
}

uint64_t MachO::get_signature_size() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "MachO: File not opened.");
	ERR_FAIL_COND_V_MSG(signature_offset == 0, 0, "MachO: No signature load command.");

	fa->seek(signature_offset + 12);
	if (swap) {
		return BSWAP32(fa->get_32());
	} else {
		return fa->get_32();
	}
}

bool MachO::is_signed() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "MachO: File not opened.");
	if (signature_offset == 0) {
		return false;
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return false; // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		uint32_t index_type = BSWAP32(fa->get_32());
		uint32_t offset = BSWAP32(fa->get_32());
		if (index_type == 0x00000000) { // CodeDirectory index type.
			fa->seek(get_signature_offset() + offset + 12);
			uint32_t flags = BSWAP32(fa->get_32());
			if (flags & 0x20000) {
				return false; // Found CD, linker-signed.
			} else {
				return true; // Found CD, not linker-signed.
			}
		}
	}
	return false; // No CD found.
}

PackedByteArray MachO::get_cdhash_sha1() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "MachO: File not opened.");
	if (signature_offset == 0) {
		return PackedByteArray();
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return PackedByteArray(); // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		fa->get_32(); // Index type, skip.
		uint32_t offset = BSWAP32(fa->get_32());
		uint64_t pos = fa->get_position();

		fa->seek(get_signature_offset() + offset);
		uint32_t cdmagic = BSWAP32(fa->get_32());
		uint32_t cdsize = BSWAP32(fa->get_32());
		if (cdmagic == 0xfade0c02) { // CodeDirectory.
			fa->seek(get_signature_offset() + offset + 36);
			uint8_t hash_size = fa->get_8();
			uint8_t hash_type = fa->get_8();
			if (hash_size == 0x14 && hash_type == 0x01) { /* SHA-1 */
				PackedByteArray hash;
				hash.resize(0x14);

				fa->seek(get_signature_offset() + offset);
				PackedByteArray blob;
				blob.resize(cdsize);
				fa->get_buffer(blob.ptrw(), cdsize);

				CryptoCore::SHA1Context ctx;
				ctx.start();
				ctx.update(blob.ptr(), blob.size());
				ctx.finish(hash.ptrw());

				return hash;
			}
		}
		fa->seek(pos);
	}
	return PackedByteArray();
}

PackedByteArray MachO::get_cdhash_sha256() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "MachO: File not opened.");
	if (signature_offset == 0) {
		return PackedByteArray();
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return PackedByteArray(); // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		fa->get_32(); // Index type, skip.
		uint32_t offset = BSWAP32(fa->get_32());
		uint64_t pos = fa->get_position();

		fa->seek(get_signature_offset() + offset);
		uint32_t cdmagic = BSWAP32(fa->get_32());
		uint32_t cdsize = BSWAP32(fa->get_32());
		if (cdmagic == 0xfade0c02) { // CodeDirectory.
			fa->seek(get_signature_offset() + offset + 36);
			uint8_t hash_size = fa->get_8();
			uint8_t hash_type = fa->get_8();
			if (hash_size == 0x20 && hash_type == 0x02) { /* SHA-256 */
				PackedByteArray hash;
				hash.resize(0x20);

				fa->seek(get_signature_offset() + offset);
				PackedByteArray blob;
				blob.resize(cdsize);
				fa->get_buffer(blob.ptrw(), cdsize);

				CryptoCore::SHA256Context ctx;
				ctx.start();
				ctx.update(blob.ptr(), blob.size());
				ctx.finish(hash.ptrw());

				return hash;
			}
		}
		fa->seek(pos);
	}
	return PackedByteArray();
}

PackedByteArray MachO::get_requirements() {
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "MachO: File not opened.");
	if (signature_offset == 0) {
		return PackedByteArray();
	}

	fa->seek(get_signature_offset());
	uint32_t magic = BSWAP32(fa->get_32());
	if (magic != 0xfade0cc0) {
		return PackedByteArray(); // No SuperBlob found.
	}
	fa->get_32(); // Skip size field, unused.
	uint32_t count = BSWAP32(fa->get_32());
	for (uint32_t i = 0; i < count; i++) {
		fa->get_32(); // Index type, skip.
		uint32_t offset = BSWAP32(fa->get_32());
		uint64_t pos = fa->get_position();

		fa->seek(get_signature_offset() + offset);
		uint32_t rqmagic = BSWAP32(fa->get_32());
		uint32_t rqsize = BSWAP32(fa->get_32());
		if (rqmagic == 0xfade0c01) { // Requirements.
			PackedByteArray blob;
			fa->seek(get_signature_offset() + offset);
			blob.resize(rqsize);
			fa->get_buffer(blob.ptrw(), rqsize);
			return blob;
		}
		fa->seek(pos);
	}
	return PackedByteArray();
}

const Ref<FileAccess> MachO::get_file() const {
	return fa;
}

Ref<FileAccess> MachO::get_file() {
	return fa;
}

bool MachO::set_signature_size(uint64_t p_size) {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "MachO: File not opened.");

	// Ensure signature load command exists.
	ERR_FAIL_COND_V_MSG(link_edit_offset == 0, false, "MachO: No __LINKEDIT segment found.");
	ERR_FAIL_COND_V_MSG(!alloc_signature(p_size), false, "MachO: Can't allocate signature load command.");

	// Update signature load command.
	uint64_t old_size = get_signature_size();
	uint64_t new_size = p_size + PAD(p_size, 16384);

	if (new_size <= old_size) {
		fa->seek(get_signature_offset());
		for (uint64_t i = 0; i < old_size; i++) {
			fa->store_8(0x00);
		}
		return true;
	}

	fa->seek(signature_offset + 12);
	if (swap) {
		fa->store_32(BSWAP32(new_size));
	} else {
		fa->store_32(new_size);
	}

	uint64_t end = get_signature_offset() + new_size;

	// Update "__LINKEDIT" segment.
	LoadCommandHeader lc;
	fa->seek(link_edit_offset);
	fa->get_buffer((uint8_t *)&lc, sizeof(LoadCommandHeader));
	if (swap) {
		lc.cmd = BSWAP32(lc.cmd);
		lc.cmdsize = BSWAP32(lc.cmdsize);
	}
	switch (lc.cmd) {
		case LC_SEGMENT: {
			LoadCommandSegment lc_seg;
			fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment));
			if (swap) {
				lc_seg.vmsize = BSWAP32(lc_seg.vmsize);
				lc_seg.filesize = BSWAP32(lc_seg.filesize);
				lc_seg.fileoff = BSWAP32(lc_seg.fileoff);
			}

			lc_seg.vmsize = end - lc_seg.fileoff;
			lc_seg.vmsize += PAD(lc_seg.vmsize, 4096);
			lc_seg.filesize = end - lc_seg.fileoff;

			if (swap) {
				lc_seg.vmsize = BSWAP32(lc_seg.vmsize);
				lc_seg.filesize = BSWAP32(lc_seg.filesize);
			}
			fa->seek(link_edit_offset + 8);
			fa->store_buffer((const uint8_t *)&lc_seg, sizeof(LoadCommandSegment));
		} break;
		case LC_SEGMENT_64: {
			LoadCommandSegment64 lc_seg;
			fa->get_buffer((uint8_t *)&lc_seg, sizeof(LoadCommandSegment64));
			if (swap) {
				lc_seg.vmsize = BSWAP64(lc_seg.vmsize);
				lc_seg.filesize = BSWAP64(lc_seg.filesize);
				lc_seg.fileoff = BSWAP64(lc_seg.fileoff);
			}
			lc_seg.vmsize = end - lc_seg.fileoff;
			lc_seg.vmsize += PAD(lc_seg.vmsize, 4096);
			lc_seg.filesize = end - lc_seg.fileoff;
			if (swap) {
				lc_seg.vmsize = BSWAP64(lc_seg.vmsize);
				lc_seg.filesize = BSWAP64(lc_seg.filesize);
			}
			fa->seek(link_edit_offset + 8);
			fa->store_buffer((const uint8_t *)&lc_seg, sizeof(LoadCommandSegment64));
		} break;
		default: {
			ERR_FAIL_V_MSG(false, "MachO: Invalid __LINKEDIT segment type.");
		} break;
	}
	fa->seek(get_signature_offset());
	for (uint64_t i = 0; i < new_size; i++) {
		fa->store_8(0x00);
	}
	return true;
}

/*************************************************************************/
/* LipO                                                                  */
/*************************************************************************/

bool LipO::is_lipo(const String &p_path) {
	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_path));
	uint32_t magic = fb->get_32();
	return (magic == 0xbebafeca || magic == 0xcafebabe || magic == 0xbfbafeca || magic == 0xcafebabf);
}

bool LipO::create_file(const String &p_output_path, const Vector<String> &p_files) {
	close();

	fa = FileAccess::open(p_output_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_output_path));

	uint64_t max_size = 0;
	for (int i = 0; i < p_files.size(); i++) {
		{
			MachO mh;
			if (!mh.open_file(p_files[i])) {
				ERR_FAIL_V_MSG(false, vformat("LipO: Invalid MachO file: \"%s\".", p_files[i]));
			}

			FatArch arch;
			arch.cputype = mh.get_cputype();
			arch.cpusubtype = mh.get_cpusubtype();
			arch.offset = 0;
			arch.size = mh.get_size();
			arch.align = mh.get_align();
			max_size += arch.size;

			archs.push_back(arch);
		}

		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}
	}

	// Write header.
	bool is_64 = (max_size >= std::numeric_limits<uint32_t>::max());
	if (is_64) {
		fa->store_32(0xbfbafeca);
	} else {
		fa->store_32(0xbebafeca);
	}
	fa->store_32(BSWAP32(archs.size()));
	uint64_t offset = archs.size() * (is_64 ? 32 : 20) + 8;
	for (int i = 0; i < archs.size(); i++) {
		archs.write[i].offset = offset + PAD(offset, uint64_t(1) << archs[i].align);
		if (is_64) {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_64(BSWAP64(archs[i].offset));
			fa->store_64(BSWAP64(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
			fa->store_32(0);
		} else {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_32(BSWAP32(archs[i].offset));
			fa->store_32(BSWAP32(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
		}
		offset = archs[i].offset + archs[i].size;
	}

	// Write files and padding.
	for (int i = 0; i < archs.size(); i++) {
		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}
		uint64_t cur = fa->get_position();
		for (uint64_t j = cur; j < archs[i].offset; j++) {
			fa->store_8(0);
		}
		int pages = archs[i].size / 4096;
		int remain = archs[i].size % 4096;
		unsigned char step[4096];
		for (int j = 0; j < pages; j++) {
			uint64_t br = fb->get_buffer(step, 4096);
			if (br > 0) {
				fa->store_buffer(step, br);
			}
		}
		uint64_t br = fb->get_buffer(step, remain);
		if (br > 0) {
			fa->store_buffer(step, br);
		}
	}
	return true;
}

bool LipO::create_file(const String &p_output_path, const Vector<String> &p_files, const Vector<Vector2i> &p_cputypes) {
	close();

	fa = FileAccess::open(p_output_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_output_path));
	ERR_FAIL_COND_V(p_files.size() != p_cputypes.size(), false);

	uint64_t max_size = 0;
	for (int i = 0; i < p_files.size(); i++) {
		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}

		{
			FatArch arch;
			MachO mh;
			if (MachO::is_macho(p_files[i]) && mh.open_file(p_files[i])) {
				arch.cputype = mh.get_cputype();
				arch.cpusubtype = mh.get_cpusubtype();
				arch.offset = 0;
				arch.size = mh.get_size();
				arch.align = mh.get_align();
				ERR_FAIL_V_MSG(arch.cputype != (uint32_t)p_cputypes[i].x || arch.cpusubtype != (uint32_t)p_cputypes[i].y, vformat("Mismatching MachO architecture: \"%s\".", p_files[i]));
			} else {
				arch.cputype = (uint32_t)p_cputypes[i].x;
				arch.cpusubtype = (uint32_t)p_cputypes[i].y;
				arch.offset = 0;
				arch.size = fb->get_length();
				arch.align = 0x03;
			}
			max_size += arch.size;

			archs.push_back(arch);
		}
	}

	// Write header.
	bool is_64 = (max_size >= std::numeric_limits<uint32_t>::max());
	if (is_64) {
		fa->store_32(0xbfbafeca);
	} else {
		fa->store_32(0xbebafeca);
	}
	fa->store_32(BSWAP32(archs.size()));
	uint64_t offset = archs.size() * (is_64 ? 32 : 20) + 8;
	for (int i = 0; i < archs.size(); i++) {
		archs.write[i].offset = offset + PAD(offset, uint64_t(1) << archs[i].align);
		if (is_64) {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_64(BSWAP64(archs[i].offset));
			fa->store_64(BSWAP64(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
			fa->store_32(0);
		} else {
			fa->store_32(BSWAP32(archs[i].cputype));
			fa->store_32(BSWAP32(archs[i].cpusubtype));
			fa->store_32(BSWAP32(archs[i].offset));
			fa->store_32(BSWAP32(archs[i].size));
			fa->store_32(BSWAP32(archs[i].align));
		}
		offset = archs[i].offset + archs[i].size;
	}

	// Write files and padding.
	for (int i = 0; i < archs.size(); i++) {
		Ref<FileAccess> fb = FileAccess::open(p_files[i], FileAccess::READ);
		if (fb.is_null()) {
			close();
			ERR_FAIL_V_MSG(false, vformat("LipO: Can't open file: \"%s\".", p_files[i]));
		}
		uint64_t cur = fa->get_position();
		for (uint64_t j = cur; j < archs[i].offset; j++) {
			fa->store_8(0);
		}
		int pages = archs[i].size / 4096;
		int remain = archs[i].size % 4096;
		unsigned char step[4096];
		for (int j = 0; j < pages; j++) {
			uint64_t br = fb->get_buffer(step, 4096);
			if (br > 0) {
				fa->store_buffer(step, br);
			}
		}
		uint64_t br = fb->get_buffer(step, remain);
		if (br > 0) {
			fa->store_buffer(step, br);
		}
	}
	return true;
}

bool LipO::open_file(const String &p_path) {
	close();

	fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_path));

	uint32_t magic = fa->get_32();
	if (magic == 0xbebafeca) {
		// 32-bit fat binary, bswap.
		uint32_t nfat_arch = BSWAP32(fa->get_32());
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = BSWAP32(fa->get_32());
			arch.cpusubtype = BSWAP32(fa->get_32());
			arch.offset = BSWAP32(fa->get_32());
			arch.size = BSWAP32(fa->get_32());
			arch.align = BSWAP32(fa->get_32());

			archs.push_back(arch);
		}
	} else if (magic == 0xcafebabe) {
		// 32-bit fat binary.
		uint32_t nfat_arch = fa->get_32();
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = fa->get_32();
			arch.cpusubtype = fa->get_32();
			arch.offset = fa->get_32();
			arch.size = fa->get_32();
			arch.align = fa->get_32();

			archs.push_back(arch);
		}
	} else if (magic == 0xbfbafeca) {
		// 64-bit fat binary, bswap.
		uint32_t nfat_arch = BSWAP32(fa->get_32());
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = BSWAP32(fa->get_32());
			arch.cpusubtype = BSWAP32(fa->get_32());
			arch.offset = BSWAP64(fa->get_64());
			arch.size = BSWAP64(fa->get_64());
			arch.align = BSWAP32(fa->get_32());
			fa->get_32(); // Skip, reserved.

			archs.push_back(arch);
		}
	} else if (magic == 0xcafebabf) {
		// 64-bit fat binary.
		uint32_t nfat_arch = fa->get_32();
		for (uint32_t i = 0; i < nfat_arch; i++) {
			FatArch arch;
			arch.cputype = fa->get_32();
			arch.cpusubtype = fa->get_32();
			arch.offset = fa->get_64();
			arch.size = fa->get_64();
			arch.align = fa->get_32();
			fa->get_32(); // Skip, reserved.

			archs.push_back(arch);
		}
	} else {
		close();
		ERR_FAIL_V_MSG(false, vformat("LipO: Invalid fat binary: \"%s\".", p_path));
	}
	return true;
}

int LipO::get_arch_count() const {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "LipO: File not opened.");
	return archs.size();
}

uint32_t LipO::get_arch_cputype(int p_index) const {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "LipO: File not opened.");
	ERR_FAIL_INDEX_V(p_index, archs.size(), 0);
	return archs[p_index].cputype;
}

uint32_t LipO::get_arch_cpusubtype(int p_index) const {
	ERR_FAIL_COND_V_MSG(fa.is_null(), 0, "LipO: File not opened.");
	ERR_FAIL_INDEX_V(p_index, archs.size(), 0);
	return archs[p_index].cpusubtype;
}

bool LipO::extract_arch(int p_index, const String &p_path) {
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "LipO: File not opened.");
	ERR_FAIL_INDEX_V(p_index, archs.size(), false);

	Ref<FileAccess> fb = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fb.is_null(), false, vformat("LipO: Can't open file: \"%s\".", p_path));

	fa->seek(archs[p_index].offset);

	int pages = archs[p_index].size / 4096;
	int remain = archs[p_index].size % 4096;
	unsigned char step[4096];
	for (int i = 0; i < pages; i++) {
		uint64_t br = fa->get_buffer(step, 4096);
		if (br > 0) {
			fb->store_buffer(step, br);
		}
	}
	uint64_t br = fa->get_buffer(step, remain);
	if (br > 0) {
		fb->store_buffer(step, br);
	}
	return true;
}

void LipO::close() {
	archs.clear();
}

LipO::~LipO() {
	close();
}

/*************************************************************************/
/* CodeSignCodeResources                                                 */
/*************************************************************************/

String CodeSignCodeResources::hash_sha1_base64(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), String(), vformat("CodeSign/CodeResources: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA1Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = fa->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	unsigned char hash[0x14];
	ctx.finish(hash);

	return CryptoCore::b64_encode_str(hash, 0x14);
}

String CodeSignCodeResources::hash_sha256_base64(const String &p_path) {
	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(fa.is_null(), String(), vformat("CodeSign/CodeResources: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA256Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = fa->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	unsigned char hash[0x20];
	ctx.finish(hash);

	return CryptoCore::b64_encode_str(hash, 0x20);
}

void CodeSignCodeResources::add_rule1(const String &p_rule, const String &p_key, int p_weight, bool p_store) {
	rules1.push_back(CRRule(p_rule, p_key, p_weight, p_store));
}

void CodeSignCodeResources::add_rule2(const String &p_rule, const String &p_key, int p_weight, bool p_store) {
	rules2.push_back(CRRule(p_rule, p_key, p_weight, p_store));
}

CodeSignCodeResources::CRMatch CodeSignCodeResources::match_rules1(const String &p_path) const {
	CRMatch found = CRMatch::CR_MATCH_NO;
	int weight = 0;
	for (int i = 0; i < rules1.size(); i++) {
		RegEx regex = RegEx(rules1[i].file_pattern);
		if (regex.search(p_path).is_valid()) {
			if (rules1[i].key == "omit") {
				return CRMatch::CR_MATCH_NO;
			} else if (rules1[i].key == "nested") {
				if (weight <= rules1[i].weight) {
					found = CRMatch::CR_MATCH_NESTED;
					weight = rules1[i].weight;
				}
			} else if (rules1[i].key == "optional") {
				if (weight <= rules1[i].weight) {
					found = CRMatch::CR_MATCH_OPTIONAL;
					weight = rules1[i].weight;
				}
			} else {
				if (weight <= rules1[i].weight) {
					found = CRMatch::CR_MATCH_YES;
					weight = rules1[i].weight;
				}
			}
		}
	}
	return found;
}

CodeSignCodeResources::CRMatch CodeSignCodeResources::match_rules2(const String &p_path) const {
	CRMatch found = CRMatch::CR_MATCH_NO;
	int weight = 0;
	for (int i = 0; i < rules2.size(); i++) {
		RegEx regex = RegEx(rules2[i].file_pattern);
		if (regex.search(p_path).is_valid()) {
			if (rules2[i].key == "omit") {
				return CRMatch::CR_MATCH_NO;
			} else if (rules2[i].key == "nested") {
				if (weight <= rules2[i].weight) {
					found = CRMatch::CR_MATCH_NESTED;
					weight = rules2[i].weight;
				}
			} else if (rules2[i].key == "optional") {
				if (weight <= rules2[i].weight) {
					found = CRMatch::CR_MATCH_OPTIONAL;
					weight = rules2[i].weight;
				}
			} else {
				if (weight <= rules2[i].weight) {
					found = CRMatch::CR_MATCH_YES;
					weight = rules2[i].weight;
				}
			}
		}
	}
	return found;
}

bool CodeSignCodeResources::add_file1(const String &p_root, const String &p_path) {
	CRMatch found = match_rules1(p_path);
	if (found != CRMatch::CR_MATCH_YES && found != CRMatch::CR_MATCH_OPTIONAL) {
		return true; // No match.
	}

	CRFile f;
	f.name = p_path;
	f.optional = (found == CRMatch::CR_MATCH_OPTIONAL);
	f.nested = false;
	f.hash = hash_sha1_base64(p_root.path_join(p_path));
	print_verbose(vformat("CodeSign/CodeResources: File(V1) %s hash1:%s", f.name, f.hash));

	files1.push_back(f);
	return true;
}

bool CodeSignCodeResources::add_file2(const String &p_root, const String &p_path) {
	CRMatch found = match_rules2(p_path);
	if (found == CRMatch::CR_MATCH_NESTED) {
		return add_nested_file(p_root, p_path, p_root.path_join(p_path));
	}
	if (found != CRMatch::CR_MATCH_YES && found != CRMatch::CR_MATCH_OPTIONAL) {
		return true; // No match.
	}

	CRFile f;
	f.name = p_path;
	f.optional = (found == CRMatch::CR_MATCH_OPTIONAL);
	f.nested = false;
	f.hash = hash_sha1_base64(p_root.path_join(p_path));
	f.hash2 = hash_sha256_base64(p_root.path_join(p_path));

	print_verbose(vformat("CodeSign/CodeResources: File(V2) %s hash1:%s hash2:%s", f.name, f.hash, f.hash2));

	files2.push_back(f);
	return true;
}

bool CodeSignCodeResources::add_nested_file(const String &p_root, const String &p_path, const String &p_exepath) {
#define CLEANUP() \
	if (files_to_add.size() > 1) { \
		for (int j = 0; j < files_to_add.size(); j++) { \
			da->remove(files_to_add[j]); \
		} \
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	ERR_FAIL_COND_V(da.is_null(), false);

	Vector<String> files_to_add;
	if (LipO::is_lipo(p_exepath)) {
		String tmp_path_name = EditorPaths::get_temp_dir().path_join("_lipo");
		Error err = da->make_dir_recursive(tmp_path_name);
		ERR_FAIL_COND_V_MSG(err != OK, false, vformat("CodeSign/CodeResources: Failed to create \"%s\" subfolder.", tmp_path_name));
		LipO lip;
		if (lip.open_file(p_exepath)) {
			for (int i = 0; i < lip.get_arch_count(); i++) {
				if (!lip.extract_arch(i, tmp_path_name.path_join("_rqexe_" + itos(i)))) {
					CLEANUP();
					ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Failed to extract thin binary.");
				}
				files_to_add.push_back(tmp_path_name.path_join("_rqexe_" + itos(i)));
			}
		}
	} else if (MachO::is_macho(p_exepath)) {
		files_to_add.push_back(p_exepath);
	}

	CRFile f;
	f.name = p_path;
	f.optional = false;
	f.nested = true;
	for (int i = 0; i < files_to_add.size(); i++) {
		MachO mh;
		if (!mh.open_file(files_to_add[i])) {
			CLEANUP();
			ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Invalid executable file.");
		}
		PackedByteArray hash = mh.get_cdhash_sha256(); // Use SHA-256 variant, if available.
		if (hash.size() != 0x20) {
			hash = mh.get_cdhash_sha1(); // Use SHA-1 instead.
			if (hash.size() != 0x14) {
				CLEANUP();
				ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Unsigned nested executable file.");
			}
		}
		hash.resize(0x14); // Always clamp to 0x14 size.
		f.hash = CryptoCore::b64_encode_str(hash.ptr(), hash.size());

		PackedByteArray rq_blob = mh.get_requirements();
		String req_string;
		if (rq_blob.size() > 8) {
			CodeSignBlob rq = CodeSignBlob::make_requirements(rq_blob);
			Vector<String> rqs = rq.parse_requirements();
			for (int j = 0; j < rqs.size(); j++) {
				if (rqs[j].begins_with("designated => ")) {
					req_string = rqs[j].replace("designated => ", "");
				}
			}
		}
		if (req_string.is_empty()) {
			req_string = "cdhash H\"" + String::hex_encode_buffer(hash.ptr(), hash.size()) + "\"";
		}
		print_verbose(vformat("CodeSign/CodeResources: Nested object %s (cputype: %d) cdhash:%s designated rq:%s", f.name, mh.get_cputype(), f.hash, req_string));
		if (f.requirements != req_string) {
			if (i != 0) {
				f.requirements += " or ";
			}
			f.requirements += req_string;
		}
	}
	files2.push_back(f);

	CLEANUP();
	return true;

#undef CLEANUP
}

bool CodeSignCodeResources::add_folder_recursive(const String &p_root, const String &p_path, const String &p_main_exe_path) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	ERR_FAIL_COND_V(da.is_null(), false);
	Error err = da->change_dir(p_root.path_join(p_path));
	ERR_FAIL_COND_V(err != OK, false);

	bool ret = true;
	da->list_dir_begin();
	String n = da->get_next();
	while (n != String()) {
		if (n != "." && n != "..") {
			String path = p_root.path_join(p_path).path_join(n);
			if (path == p_main_exe_path) {
				n = da->get_next();
				continue; // Skip main executable.
			}
			if (da->current_is_dir()) {
				CRMatch found = match_rules2(p_path.path_join(n));
				String fmw_ver = "Current"; // Framework version (default).
				String info_path;
				String main_exe;
				bool bundle = false;
				if (da->file_exists(path.path_join("Contents/Info.plist"))) {
					info_path = path.path_join("Contents/Info.plist");
					main_exe = path.path_join("Contents/MacOS");
					bundle = true;
				} else if (da->file_exists(path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver)))) {
					info_path = path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver));
					main_exe = path.path_join(vformat("Versions/%s", fmw_ver));
					bundle = true;
				} else if (da->file_exists(path.path_join("Resources/Info.plist"))) {
					info_path = path.path_join("Resources/Info.plist");
					main_exe = path;
					bundle = true;
				} else if (da->file_exists(path.path_join("Info.plist"))) {
					info_path = path.path_join("Info.plist");
					main_exe = path;
					bundle = true;
				}
				if (bundle && found == CRMatch::CR_MATCH_NESTED && !info_path.is_empty()) {
					// Read Info.plist.
					PList info_plist;
					if (info_plist.load_file(info_path)) {
						if (info_plist.get_root()->data_type == PList::PLNodeType::PL_NODE_TYPE_DICT && info_plist.get_root()->data_dict.has("CFBundleExecutable")) {
							main_exe = main_exe.path_join(String::utf8(info_plist.get_root()->data_dict["CFBundleExecutable"]->data_string.get_data()));
						} else {
							ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Invalid Info.plist, no exe name.");
						}
					} else {
						ERR_FAIL_V_MSG(false, "CodeSign/CodeResources: Invalid Info.plist, can't load.");
					}
					ret = ret && add_nested_file(p_root, p_path.path_join(n), main_exe);
				} else {
					ret = ret && add_folder_recursive(p_root, p_path.path_join(n), p_main_exe_path);
				}
			} else {
				ret = ret && add_file1(p_root, p_path.path_join(n));
				ret = ret && add_file2(p_root, p_path.path_join(n));
			}
		}

		n = da->get_next();
	}

	da->list_dir_end();
	return ret;
}

bool CodeSignCodeResources::save_to_file(const String &p_path) {
	PList pl;

	print_verbose(vformat("CodeSign/CodeResources: Writing to file: %s", p_path));

	// Write version 1 hashes.
	Ref<PListNode> files1_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(files1_dict, "files");
	for (int i = 0; i < files1.size(); i++) {
		if (files1[i].optional) {
			Ref<PListNode> file_dict = PListNode::new_dict();
			files1_dict->push_subnode(file_dict, files1[i].name);

			file_dict->push_subnode(PListNode::new_data(files1[i].hash), "hash");
			file_dict->push_subnode(PListNode::new_bool(true), "optional");
		} else {
			files1_dict->push_subnode(PListNode::new_data(files1[i].hash), files1[i].name);
		}
	}

	// Write version 2 hashes.
	Ref<PListNode> files2_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(files2_dict, "files2");
	for (int i = 0; i < files2.size(); i++) {
		Ref<PListNode> file_dict = PListNode::new_dict();
		files2_dict->push_subnode(file_dict, files2[i].name);

		if (files2[i].nested) {
			file_dict->push_subnode(PListNode::new_data(files2[i].hash), "cdhash");
			file_dict->push_subnode(PListNode::new_string(files2[i].requirements), "requirement");
		} else {
			file_dict->push_subnode(PListNode::new_data(files2[i].hash), "hash");
			file_dict->push_subnode(PListNode::new_data(files2[i].hash2), "hash2");
			if (files2[i].optional) {
				file_dict->push_subnode(PListNode::new_bool(true), "optional");
			}
		}
	}

	// Write version 1 rules.
	Ref<PListNode> rules1_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(rules1_dict, "rules");
	for (int i = 0; i < rules1.size(); i++) {
		if (rules1[i].store) {
			if (rules1[i].key.is_empty() && rules1[i].weight <= 0) {
				rules1_dict->push_subnode(PListNode::new_bool(true), rules1[i].file_pattern);
			} else {
				Ref<PListNode> rule_dict = PListNode::new_dict();
				rules1_dict->push_subnode(rule_dict, rules1[i].file_pattern);
				if (!rules1[i].key.is_empty()) {
					rule_dict->push_subnode(PListNode::new_bool(true), rules1[i].key);
				}
				if (rules1[i].weight != 1) {
					rule_dict->push_subnode(PListNode::new_real(rules1[i].weight), "weight");
				}
			}
		}
	}

	// Write version 2 rules.
	Ref<PListNode> rules2_dict = PListNode::new_dict();
	pl.get_root()->push_subnode(rules2_dict, "rules2");
	for (int i = 0; i < rules2.size(); i++) {
		if (rules2[i].store) {
			if (rules2[i].key.is_empty() && rules2[i].weight <= 0) {
				rules2_dict->push_subnode(PListNode::new_bool(true), rules2[i].file_pattern);
			} else {
				Ref<PListNode> rule_dict = PListNode::new_dict();
				rules2_dict->push_subnode(rule_dict, rules2[i].file_pattern);
				if (!rules2[i].key.is_empty()) {
					rule_dict->push_subnode(PListNode::new_bool(true), rules2[i].key);
				}
				if (rules2[i].weight != 1) {
					rule_dict->push_subnode(PListNode::new_real(rules2[i].weight), "weight");
				}
			}
		}
	}
	String text = pl.save_text();
	ERR_FAIL_COND_V_MSG(text.is_empty(), false, "CodeSign/CodeResources: Generating resources PList failed.");

	Ref<FileAccess> fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, vformat("CodeSign/CodeResources: Can't open file: \"%s\".", p_path));

	CharString cs = text.utf8();
	fa->store_buffer((const uint8_t *)cs.ptr(), cs.length());
	return true;
}



/*************************************************************************/
/* CodeSignBlob                                                          */
/*************************************************************************/

CodeSignBlob CodeSignBlob::make_requirements() {
	CodeSignBlob b;
	b.index_type = INDEX_REQUIREMENTS;
	b.blob.append_array({ 0xFA, 0xDE, 0x0C, 0x01 }); // Requirement set magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x0C }); // Length of requirements set (12 bytes).
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x00 }); // Empty.
	return b;
}

CodeSignBlob CodeSignBlob::make_requirements(const PackedByteArray &p_data) {
	CodeSignBlob b;
	b.index_type = INDEX_REQUIREMENTS;
	b.blob = p_data;
	return b;
}

_FORCE_INLINE_ void CodeSignBlob::_parse_certificate_slot(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_out += "certificate ";
	uint32_t tag_slot = _R(r_pos);
	if (tag_slot == 0x00000000) {
		r_out += "leaf";
	} else if (tag_slot == 0xffffffff) {
		r_out += "root";
	} else {
		r_out += itos((int32_t)tag_slot);
	}
	r_pos += 4;
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_key(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	const uint32_t key_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + key_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_pos += 4 + key_size + PAD(key_size, 4);
	r_out += "[" + String::utf8((const char *)blob.ptr() + r_pos + 4, key_size) + "]";
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_oid_key(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	uint32_t key_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + key_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_out += "[field.";
	r_out += itos(blob[r_pos + 4] / 40) + ".";
	r_out += itos(blob[r_pos + 4] % 40);
	uint32_t spos = r_pos + 5;
	while (spos < r_pos + 4 + key_size) {
		r_out += ".";
		if (blob[spos] <= 127) {
			r_out += itos(blob[spos]);
			spos += 1;
		} else {
			uint32_t x = (0x7F & blob[spos]) << 7;
			spos += 1;
			while (blob[spos] > 127) {
				x = (x + (0x7F & blob[spos])) << 7;
				spos += 1;
			}
			x = (x + (0x7F & blob[spos]));
			r_out += itos(x);
			spos += 1;
		}
	}
	r_out += "]";
	r_pos += 4 + key_size + PAD(key_size, 4);
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_hash_string(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	uint32_t tag_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + tag_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_out += "H\"" + String::hex_encode_buffer(blob.ptr() + r_pos + 4, tag_size) + "\"";
	r_pos += 4 + tag_size + PAD(tag_size, 4);
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_value(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	const uint32_t key_size = _R(r_pos);
	ERR_FAIL_COND_MSG(r_pos + key_size > p_rq_size, "CodeSign/Requirements: Out of bounds.");
	r_pos += 4 + key_size + PAD(key_size, 4);
	r_out += "\"" + String::utf8((const char *)blob.ptr() + r_pos + 4, key_size) + "\"";
#undef _R
}

_FORCE_INLINE_ void CodeSignBlob::_parse_date(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_MSG(r_pos >= p_rq_size, "CodeSign/Requirements: Out of bounds.");
	uint32_t date = _R(r_pos);
	time_t t = 978307200 + date;
	struct tm lt;
#ifdef WINDOWS_ENABLED
	gmtime_s(&lt, &t);
#else
	gmtime_r(&t, &lt);
#endif
	r_out += vformat("<%04d-%02d-%02d ", (int)(1900 + lt.tm_year), (int)(lt.tm_mon + 1), (int)(lt.tm_mday)) + vformat("%02d:%02d:%02d +0000>", (int)(lt.tm_hour), (int)(lt.tm_min), (int)(lt.tm_sec));
#undef _R
}

_FORCE_INLINE_ bool CodeSignBlob::_parse_match(uint32_t &r_pos, String &r_out, uint32_t p_rq_size) const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	ERR_FAIL_COND_V_MSG(r_pos >= p_rq_size, false, "CodeSign/Requirements: Out of bounds.");
	uint32_t match = _R(r_pos);
	r_pos += 4;
	switch (match) {
		case 0x00000000: {
			r_out += "exists";
		} break;
		case 0x00000001: {
			r_out += "= ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000002: {
			r_out += "~ ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000003: {
			r_out += "= *";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000004: {
			r_out += "= ";
			_parse_value(r_pos, r_out, p_rq_size);
			r_out += "*";
		} break;
		case 0x00000005: {
			r_out += "< ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000006: {
			r_out += "> ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000007: {
			r_out += "<= ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000008: {
			r_out += ">= ";
			_parse_value(r_pos, r_out, p_rq_size);
		} break;
		case 0x00000009: {
			r_out += "= ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000A: {
			r_out += "< ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000B: {
			r_out += "> ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000C: {
			r_out += "<= ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000D: {
			r_out += ">= ";
			_parse_date(r_pos, r_out, p_rq_size);
		} break;
		case 0x0000000E: {
			r_out += "absent";
		} break;
		default: {
			return false;
		}
	}
	return true;
#undef _R
}

Vector<String> CodeSignBlob::parse_requirements() const {
#define _R(x) BSWAP32(*(uint32_t *)(blob.ptr() + x))
	Vector<String> list;

	// Read requirements set header.
	ERR_FAIL_COND_V_MSG(blob.size() < 12, list, "CodeSign/Requirements: Blob is too small.");
	uint32_t magic = _R(0);
	ERR_FAIL_COND_V_MSG(magic != 0xfade0c01, list, "CodeSign/Requirements: Invalid set magic.");
	uint32_t size = _R(4);
	ERR_FAIL_COND_V_MSG(size != (uint32_t)blob.size(), list, "CodeSign/Requirements: Invalid set size.");
	uint32_t count = _R(8);

	for (uint32_t i = 0; i < count; i++) {
		String out;

		// Read requirement header.
		uint32_t rq_type = _R(12 + i * 8);
		uint32_t rq_offset = _R(12 + i * 8 + 4);
		ERR_FAIL_COND_V_MSG(rq_offset + 12 >= (uint32_t)blob.size(), list, "CodeSign/Requirements: Invalid requirement offset.");
		switch (rq_type) {
			case 0x00000001: {
				out += "host => ";
			} break;
			case 0x00000002: {
				out += "guest => ";
			} break;
			case 0x00000003: {
				out += "designated => ";
			} break;
			case 0x00000004: {
				out += "library => ";
			} break;
			case 0x00000005: {
				out += "plugin => ";
			} break;
			default: {
				ERR_FAIL_V_MSG(list, "CodeSign/Requirements: Invalid requirement type.");
			}
		}
		uint32_t rq_magic = _R(rq_offset);
		uint32_t rq_size = _R(rq_offset + 4);
		uint32_t rq_ver = _R(rq_offset + 8);
		uint32_t pos = rq_offset + 12;
		ERR_FAIL_COND_V_MSG(rq_magic != 0xfade0c00, list, "CodeSign/Requirements: Invalid requirement magic.");
		ERR_FAIL_COND_V_MSG(rq_ver != 0x00000001, list, "CodeSign/Requirements: Invalid requirement version.");

		// Read requirement tokens.
		List<String> tokens;
		while (pos < rq_offset + rq_size) {
			uint32_t rq_tag = _R(pos);
			pos += 4;
			String token;
			switch (rq_tag) {
				case 0x00000000: {
					token = "false";
				} break;
				case 0x00000001: {
					token = "true";
				} break;
				case 0x00000002: {
					token = "identifier ";
					_parse_value(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000003: {
					token = "anchor apple";
				} break;
				case 0x00000004: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					token += " ";
					_parse_hash_string(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000005: {
					token = "info";
					_parse_key(pos, token, rq_offset + rq_size);
					token += " = ";
					_parse_value(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000006: {
					token = "and";
				} break;
				case 0x00000007: {
					token = "or";
				} break;
				case 0x00000008: {
					token = "cdhash ";
					_parse_hash_string(pos, token, rq_offset + rq_size);
				} break;
				case 0x00000009: {
					token = "!";
				} break;
				case 0x0000000A: {
					token = "info";
					_parse_key(pos, token, rq_offset + rq_size);
					token += " ";
					ERR_FAIL_COND_V_MSG(!_parse_match(pos, token, rq_offset + rq_size), list, "CodeSign/Requirements: Unsupported match suffix.");
				} break;
				case 0x0000000B: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					_parse_key(pos, token, rq_offset + rq_size);
					token += " ";
					ERR_FAIL_COND_V_MSG(!_parse_match(pos, token, rq_offset + rq_size), list, "CodeSign/Requirements: Unsupported match suffix.");
				} break;
				case 0x0000000C: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					token += " trusted";
				} break;
				case 0x0000000D: {
					token = "anchor trusted";
				} break;
				case 0x0000000E: {
					_parse_certificate_slot(pos, token, rq_offset + rq_size);
					_parse_oid_key(pos, token, rq_offset + rq_size);
					token += " ";
					ERR_FAIL_COND_V_MSG(!_parse_match(pos, token, rq_offset + rq_size), list, "CodeSign/Requirements: Unsupported match suffix.");
				} break;
				case 0x0000000F: {
					token = "anchor apple generic";
				} break;
				default: {
					ERR_FAIL_V_MSG(list, "CodeSign/Requirements: Invalid requirement token.");
				} break;
			}
			tokens.push_back(token);
		}

		// Polish to infix notation (w/o bracket optimization).
		for (List<String>::Element *E = tokens.back(); E; E = E->prev()) {
			if (E->get() == "and") {
				ERR_FAIL_COND_V_MSG(!E->next() || !E->next()->next(), list, "CodeSign/Requirements: Invalid token sequence.");
				String token = "(" + E->next()->get() + " and " + E->next()->next()->get() + ")";
				tokens.erase(E->next()->next());
				tokens.erase(E->next());
				E->get() = token;
			} else if (E->get() == "or") {
				ERR_FAIL_COND_V_MSG(!E->next() || !E->next()->next(), list, "CodeSign/Requirements: Invalid token sequence.");
				String token = "(" + E->next()->get() + " or " + E->next()->next()->get() + ")";
				tokens.erase(E->next()->next());
				tokens.erase(E->next());
				E->get() = token;
			}
		}

		if (tokens.size() == 1) {
			list.push_back(out + tokens.front()->get());
		} else {
			ERR_FAIL_V_MSG(list, "CodeSign/Requirements: Invalid token sequence.");
		}
	}

	return list;
#undef _R
}

CodeSignBlob CodeSignBlob::make_entitlements_text() {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_TEXT;
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x71 }); // Text Entitlements set magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x08 }); // Length (8 bytes).
	return b;
}

CodeSignBlob CodeSignBlob::make_entitlements_text(const String &p_string) {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_TEXT;
	CharString utf8 = p_string.utf8();
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x71 }); // Text Entitlements set magic.
	for (int i = 3; i >= 0; i--) {
		uint8_t x = ((utf8.length() + 8) >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	for (int i = 0; i < utf8.length(); i++) { // Write data.
		b.blob.push_back(utf8[i]);
	}
	return b;
}

CodeSignBlob CodeSignBlob::make_entitlements_binary() {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_BINARY;
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x72 }); // Binary Entitlements magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x08 }); // Length (8 bytes).
	return b;
}

CodeSignBlob CodeSignBlob::make_entitlements_binary(const String &p_string) {
	CodeSignBlob b;
	b.index_type = INDEX_ENTITLEMENTS_BINARY;
	PList pl = PList(p_string);

	PackedByteArray asn1 = pl.save_asn1();
	b.blob.append_array({ 0xFA, 0xDE, 0x71, 0x72 }); // Binary Entitlements magic.
	uint32_t size = asn1.size() + 8;
	for (int i = 3; i >= 0; i--) {
		uint8_t x = (size >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	b.blob.append_array(asn1); // Write data.
	return b;
}

CodeSignBlob CodeSignBlob::make_code_directory() {
	CodeSignBlob b;
	b.index_type = INDEX_CODE_DIRECTORY;
	b.blob.append_array({ 0xFA, 0xDE, 0x0C, 0x02 }); // Code Directory magic.
	b.blob.append_array({ 0x00, 0x00, 0x00, 0x00 }); // Size (8 bytes).
	return b;
}

CodeSignBlob CodeSignBlob::make_code_directory(uint8_t p_hash_size, uint8_t p_hash_type, bool p_main, const CharString &p_id, const CharString &p_team_id, uint32_t p_page_size, uint64_t p_exe_limit, uint64_t p_code_limit) {
	CodeSignBlob b;
	b.index_type = INDEX_CODE_DIRECTORY;
	b.pages = p_code_limit / (uint64_t(1) << p_page_size);
	b.remain = p_code_limit % (uint64_t(1) << p_page_size);
	b.code_slots = b.pages + (b.remain > 0 ? 1 : 0);
	b.special_slots = 7;

	int cd_size = 8 + sizeof(CodeDirectoryHeader) + (b.code_slots + b.special_slots) * p_hash_size + p_id.size() + p_team_id.size();
	int cd_off = 8 + sizeof(CodeDirectoryHeader);
	b.blob.append_array({ 0xFA, 0xDE, 0x0C, 0x02 }); // Code Directory magic.
	for (int i = 3; i >= 0; i--) {
		uint8_t x = (cd_size >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	b.blob.resize(cd_size);
	memset(b.blob.ptrw() + 8, 0x00, cd_size - 8);
	CodeDirectoryHeader *cd = reinterpret_cast<CodeDirectoryHeader *>(b.blob.ptrw() + 8);

	bool is_64_cl = (p_code_limit >= std::numeric_limits<uint32_t>::max());

	// Version and options.
	cd->version = BSWAP32(0x20500);
	cd->flags = BSWAP32(SIGNATURE_ADHOC | SIGNATURE_RUNTIME);
	cd->special_slots = BSWAP32(b.special_slots);
	cd->code_slots = BSWAP32(b.code_slots);
	if (is_64_cl) {
		cd->code_limit_64 = BSWAP64(p_code_limit);
	} else {
		cd->code_limit = BSWAP32(p_code_limit);
	}
	cd->hash_size = p_hash_size;
	cd->hash_type = p_hash_type;
	cd->page_size = p_page_size;
	cd->exec_seg_base = 0x00;
	cd->exec_seg_limit = BSWAP64(p_exe_limit);
	cd->exec_seg_flags = 0;
	if (p_main) {
		cd->exec_seg_flags |= EXECSEG_MAIN_BINARY;
	}
	cd->exec_seg_flags = BSWAP64(cd->exec_seg_flags);
	uint32_t version = (11 << 16) + (3 << 8) + 0; // Version 11.3.0
	cd->runtime = BSWAP32(version);

	// Copy ID.
	cd->ident_offset = BSWAP32(cd_off);
	memcpy(b.blob.ptrw() + cd_off, p_id.get_data(), p_id.size());
	cd_off += p_id.size();

	// Copy Team ID.
	if (p_team_id.length() > 0) {
		cd->team_offset = BSWAP32(cd_off);
		memcpy(b.blob.ptrw() + cd_off, p_team_id.get_data(), p_team_id.size());
		cd_off += p_team_id.size();
	} else {
		cd->team_offset = 0;
	}

	// Scatter vector.
	cd->scatter_vector_offset = 0; // Not used.

	// Executable hashes offset.
	cd->hash_offset = BSWAP32(cd_off + b.special_slots * cd->hash_size);
	return b;
}

bool CodeSignBlob::set_hash_in_slot(const PackedByteArray &p_hash, int p_slot) {
	ERR_FAIL_COND_V_MSG((p_slot < -special_slots) || (p_slot >= code_slots), false, vformat("CodeSign/CodeDirectory: Invalid hash slot index: %d.", p_slot));
	CodeDirectoryHeader *cd = reinterpret_cast<CodeDirectoryHeader *>(blob.ptrw() + 8);
	for (int i = 0; i < cd->hash_size; i++) {
		blob.write[BSWAP32(cd->hash_offset) + p_slot * cd->hash_size + i] = p_hash[i];
	}
	return true;
}

CodeSignBlob CodeSignBlob::make_signature() {
	CodeSignBlob b;
	b.index_type = INDEX_SIGNATURE;
	b.blob.append_array({ 0xFA, 0xDE, 0x0B, 0x01 }); // Signature magic.
	uint32_t sign_size = 8; // Ad-hoc signature is empty.
	for (int i = 3; i >= 0; i--) {
		uint8_t x = (sign_size >> i * 8) & 0xFF; // Size.
		b.blob.push_back(x);
	}
	return b;
}

PackedByteArray CodeSignBlob::get_hash_sha1() const {
	PackedByteArray hash;
	hash.resize(0x14);

	CryptoCore::SHA1Context ctx;
	ctx.start();
	ctx.update(blob.ptr(), blob.size());
	ctx.finish(hash.ptrw());

	return hash;
}

PackedByteArray CodeSignBlob::get_hash_sha256() const {
	PackedByteArray hash;
	hash.resize(0x20);

	CryptoCore::SHA256Context ctx;
	ctx.start();
	ctx.update(blob.ptr(), blob.size());
	ctx.finish(hash.ptrw());

	return hash;
}

void CodeSignBlob::write_to_file(Ref<FileAccess> p_file) const {
	ERR_FAIL_COND_MSG(p_file.is_null(), "CodeSignBlob: Invalid file handle.");
	p_file->store_buffer(blob.ptr(), blob.size());
}

/*************************************************************************/
/* CodeSignSuperBlob                                                     */
/*************************************************************************/

bool CodeSignSuperBlob::add_blob(const CodeSignBlob &p_blob) {
	if (!p_blob.is_empty()) {
		blobs.push_back(p_blob);
		return true;
	} else {
		return false;
	}
}

int CodeSignSuperBlob::get_size() const {
	int size = 12 + blobs.size() * 8;
	for (int i = 0; i < blobs.size(); i++) {
		if (blobs[i].is_empty()) {
			return 0;
		}
		size += blobs[i].get_size();
	}
	return size;
}

void CodeSignSuperBlob::write_to_file(Ref<FileAccess> p_file) const {
	ERR_FAIL_COND_MSG(p_file.is_null(), "CodeSign/SuperBlob: Invalid file handle.");
	uint32_t size = get_size();
	uint32_t data_offset = 12 + blobs.size() * 8;

	// Write header.
	p_file->store_32(BSWAP32(0xfade0cc0));
	p_file->store_32(BSWAP32(size));
	p_file->store_32(BSWAP32(blobs.size()));

	// Write index.
	for (int i = 0; i < blobs.size(); i++) {
		if (blobs[i].is_empty()) {
			return;
		}
		p_file->store_32(BSWAP32(blobs[i].get_index_type()));
		p_file->store_32(BSWAP32(data_offset));
		data_offset += blobs[i].get_size();
	}

	// Write blobs.
	for (int i = 0; i < blobs.size(); i++) {
		blobs[i].write_to_file(p_file);
	}
}


/*************************************************************************/
/* CodeSign                                                              */
/*************************************************************************/

PackedByteArray CodeSign::file_hash_sha1(const String &p_path) {
	PackedByteArray file_hash;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), PackedByteArray(), vformat("CodeSign: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA1Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = f->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	file_hash.resize(0x14);
	ctx.finish(file_hash.ptrw());
	return file_hash;
}

PackedByteArray CodeSign::file_hash_sha256(const String &p_path) {
	PackedByteArray file_hash;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), PackedByteArray(), vformat("CodeSign: Can't open file: \"%s\".", p_path));

	CryptoCore::SHA256Context ctx;
	ctx.start();

	unsigned char step[4096];
	while (true) {
		uint64_t br = f->get_buffer(step, 4096);
		if (br > 0) {
			ctx.update(step, br);
		}
		if (br < 4096) {
			break;
		}
	}

	file_hash.resize(0x20);
	ctx.finish(file_hash.ptrw());
	return file_hash;
}

Error CodeSign::_codesign_file(bool p_use_hardened_runtime, bool p_force, const String &p_info, const String &p_exe_path, const String &p_bundle_path, const String &p_ent_path, bool p_ios_bundle, String &r_error_msg) {
#define CLEANUP() \
	if (files_to_sign.size() > 1) { \
		for (int j = 0; j < files_to_sign.size(); j++) { \
			da->remove(files_to_sign[j]); \
		} \
	}

	print_verbose(vformat("CodeSign: Signing executable: %s, bundle: %s with entitlements %s", p_exe_path, p_bundle_path, p_ent_path));

	PackedByteArray info_hash1, info_hash2;
	PackedByteArray res_hash1, res_hash2;
	String id;
	String main_exe = p_exe_path;

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_null()) {
		r_error_msg = TTR("Can't get filesystem access.");
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "CodeSign: Can't get filesystem access.");
	}

	// Read Info.plist.
	if (!p_info.is_empty()) {
		print_verbose("CodeSign: Reading bundle info...");
		PList info_plist;
		if (info_plist.load_file(p_info)) {
			info_hash1 = file_hash_sha1(p_info);
			info_hash2 = file_hash_sha256(p_info);
			if (info_hash1.is_empty() || info_hash2.is_empty()) {
				r_error_msg = TTR("Failed to get Info.plist hash.");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to get Info.plist hash.");
			}

			if (info_plist.get_root()->data_type == PList::PLNodeType::PL_NODE_TYPE_DICT && info_plist.get_root()->data_dict.has("CFBundleExecutable")) {
				main_exe = p_exe_path.path_join(String::utf8(info_plist.get_root()->data_dict["CFBundleExecutable"]->data_string.get_data()));
			} else {
				r_error_msg = TTR("Invalid Info.plist, no exe name.");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid Info.plist, no exe name.");
			}

			if (info_plist.get_root()->data_type == PList::PLNodeType::PL_NODE_TYPE_DICT && info_plist.get_root()->data_dict.has("CFBundleIdentifier")) {
				id = info_plist.get_root()->data_dict["CFBundleIdentifier"]->data_string.get_data();
			} else {
				r_error_msg = TTR("Invalid Info.plist, no bundle id.");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid Info.plist, no bundle id.");
			}
		} else {
			r_error_msg = TTR("Invalid Info.plist, can't load.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid Info.plist, can't load.");
		}
	}

	// Extract fat binary.
	Vector<String> files_to_sign;
	if (LipO::is_lipo(main_exe)) {
		print_verbose(vformat("CodeSign: Executable is fat, extracting..."));
		String tmp_path_name = EditorPaths::get_temp_dir().path_join("_lipo");
		Error err = da->make_dir_recursive(tmp_path_name);
		if (err != OK) {
			r_error_msg = vformat(TTR("Failed to create \"%s\" subfolder."), tmp_path_name);
			ERR_FAIL_V_MSG(FAILED, vformat("CodeSign: Failed to create \"%s\" subfolder.", tmp_path_name));
		}
		LipO lip;
		if (lip.open_file(main_exe)) {
			for (int i = 0; i < lip.get_arch_count(); i++) {
				if (!lip.extract_arch(i, tmp_path_name.path_join("_exe_" + itos(i)))) {
					CLEANUP();
					r_error_msg = TTR("Failed to extract thin binary.");
					ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to extract thin binary.");
				}
				files_to_sign.push_back(tmp_path_name.path_join("_exe_" + itos(i)));
			}
		}
	} else if (MachO::is_macho(main_exe)) {
		print_verbose("CodeSign: Executable is thin...");
		files_to_sign.push_back(main_exe);
	} else {
		r_error_msg = TTR("Invalid binary format.");
		ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid binary format.");
	}

	// Check if it's already signed.
	if (!p_force) {
		for (int i = 0; i < files_to_sign.size(); i++) {
			MachO mh;
			mh.open_file(files_to_sign[i]);
			if (mh.is_signed()) {
				CLEANUP();
				r_error_msg = TTR("Already signed!");
				ERR_FAIL_V_MSG(FAILED, "CodeSign: Already signed!");
			}
		}
	}

	// Generate core resources.
	if (!p_bundle_path.is_empty()) {
		print_verbose("CodeSign: Generating bundle CodeResources...");
		CodeSignCodeResources cr;

		if (p_ios_bundle) {
			cr.add_rule1("^.*");
			cr.add_rule1("^.*\\.lproj/", "optional", 100);
			cr.add_rule1("^.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule1("^Base\\.lproj/", "", 1010);
			cr.add_rule1("^version.plist$");

			cr.add_rule2(".*\\.dSYM($|/)", "", 11);
			cr.add_rule2("^(.*/)?\\.DS_Store$", "omit", 2000);
			cr.add_rule2("^.*");
			cr.add_rule2("^.*\\.lproj/", "optional", 1000);
			cr.add_rule2("^.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule2("^Base\\.lproj/", "", 1010);
			cr.add_rule2("^Info\\.plist$", "omit", 20);
			cr.add_rule2("^PkgInfo$", "omit", 20);
			cr.add_rule2("^embedded\\.provisionprofile$", "", 10);
			cr.add_rule2("^version\\.plist$", "", 20);

			cr.add_rule2("^_MASReceipt", "omit", 2000, false);
			cr.add_rule2("^_CodeSignature", "omit", 2000, false);
			cr.add_rule2("^CodeResources", "omit", 2000, false);
		} else {
			cr.add_rule1("^Resources($|/)");
			cr.add_rule1("^Resources/.*\\.lproj/", "optional", 1000);
			cr.add_rule1("^Resources/.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule1("^Resources/Base\\.lproj/", "", 1010);
			cr.add_rule1("^version.plist$");

			cr.add_rule2(".*\\.dSYM($|/)", "", 11);
			cr.add_rule2("^(.*/)?\\.DS_Store$", "omit", 2000);
			cr.add_rule2("^(Frameworks|SharedFrameworks|PlugIns|Plug-ins|XPCServices|Helpers|MacOS|Library/(Automator|Spotlight|LoginItems))/", "nested", 10);
			cr.add_rule2("^.*");
			cr.add_rule2("^Info\\.plist$", "omit", 20);
			cr.add_rule2("^PkgInfo$", "omit", 20);
			cr.add_rule2("^Resources($|/)", "", 20);
			cr.add_rule2("^Resources/.*\\.lproj/", "optional", 1000);
			cr.add_rule2("^Resources/.*\\.lproj/locversion.plist$", "omit", 1100);
			cr.add_rule2("^Resources/Base\\.lproj/", "", 1010);
			cr.add_rule2("^[^/]+$", "nested", 10);
			cr.add_rule2("^embedded\\.provisionprofile$", "", 10);
			cr.add_rule2("^version\\.plist$", "", 20);
			cr.add_rule2("^_MASReceipt", "omit", 2000, false);
			cr.add_rule2("^_CodeSignature", "omit", 2000, false);
			cr.add_rule2("^CodeResources", "omit", 2000, false);
		}

		if (!cr.add_folder_recursive(p_bundle_path, "", main_exe)) {
			CLEANUP();
			r_error_msg = TTR("Failed to process nested resources.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to process nested resources.");
		}
		Error err = da->make_dir_recursive(p_bundle_path.path_join("_CodeSignature"));
		if (err != OK) {
			CLEANUP();
			r_error_msg = TTR("Failed to create _CodeSignature subfolder.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to create _CodeSignature subfolder.");
		}
		cr.save_to_file(p_bundle_path.path_join("_CodeSignature").path_join("CodeResources"));
		res_hash1 = file_hash_sha1(p_bundle_path.path_join("_CodeSignature").path_join("CodeResources"));
		res_hash2 = file_hash_sha256(p_bundle_path.path_join("_CodeSignature").path_join("CodeResources"));
		if (res_hash1.is_empty() || res_hash2.is_empty()) {
			CLEANUP();
			r_error_msg = TTR("Failed to get CodeResources hash.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to get CodeResources hash.");
		}
	}

	// Generate common signature structures.
	if (id.is_empty()) {
		uint8_t uuid[16];
		Error err = CryptoCore::generate_random(uuid, 16);
		ERR_FAIL_COND_V_MSG(err, err, "Failed to generate UUID.");
		id = (String("a-55554944") /*a-UUID*/ + String::hex_encode_buffer(uuid, 16));
	}
	CharString uuid_str = id.utf8();
	print_verbose(vformat("CodeSign: Used bundle ID: %s", id));

	print_verbose("CodeSign: Processing entitlements...");

	CodeSignBlob cet;
	CodeSignBlob ceb;
	if (!p_ent_path.is_empty()) {
		String entitlements = FileAccess::get_file_as_string(p_ent_path);
		if (entitlements.is_empty()) {
			CLEANUP();
			r_error_msg = TTR("Invalid entitlements file.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid entitlements file.");
		}
		cet = CodeSignBlob::make_entitlements_text(entitlements);
		ceb = CodeSignBlob::make_entitlements_binary(entitlements);
	}

	print_verbose("CodeSign: Generating requirements...");
	String team_id = "";
	CodeSignBlob rq = CodeSignBlob::make_requirements();

	// Sign executables.
	for (int i = 0; i < files_to_sign.size(); i++) {
		MachO mh;
		if (!mh.open_file(files_to_sign[i])) {
			CLEANUP();
			r_error_msg = TTR("Invalid executable file.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Invalid executable file.");
		}
		print_verbose(vformat("CodeSign: Signing executable for cputype: %d ...", mh.get_cputype()));

		print_verbose("CodeSign: Generating CodeDirectory...");
		CodeSignBlob cd1 = CodeSignBlob::make_code_directory(0x14, 0x01, true, uuid_str, team_id.utf8(), 12, mh.get_exe_limit(), mh.get_code_limit());
		CodeSignBlob cd2 = CodeSignBlob::make_code_directory(0x20, 0x02, true, uuid_str, team_id.utf8(), 12, mh.get_exe_limit(), mh.get_code_limit());
		print_verbose("CodeSign: Calculating special slot hashes...");
		if (info_hash2.size() == 0x20) {
			cd2.set_hash_in_slot(info_hash2, CodeSignBlob::SLOT_INFO_PLIST);
		}
		if (info_hash1.size() == 0x14) {
			cd1.set_hash_in_slot(info_hash1, CodeSignBlob::SLOT_INFO_PLIST);
		}
		cd1.set_hash_in_slot(rq.get_hash_sha1(), CodeSignBlob::SLOT_REQUIREMENTS);
		cd2.set_hash_in_slot(rq.get_hash_sha256(), CodeSignBlob::SLOT_REQUIREMENTS);
		if (res_hash2.size() == 0x20) {
			cd2.set_hash_in_slot(res_hash2, CodeSignBlob::SLOT_RESOURCES);
		}
		if (res_hash1.size() == 0x14) {
			cd1.set_hash_in_slot(res_hash1, CodeSignBlob::SLOT_RESOURCES);
		}
		if (!cet.is_empty()) {
			cd1.set_hash_in_slot(cet.get_hash_sha1(), CodeSignBlob::SLOT_ENTITLEMENTS); //Text variant.
			cd2.set_hash_in_slot(cet.get_hash_sha256(), CodeSignBlob::SLOT_ENTITLEMENTS);
		}
		if (!ceb.is_empty()) {
			cd1.set_hash_in_slot(ceb.get_hash_sha1(), CodeSignBlob::SLOT_DER_ENTITLEMENTS); //ASN.1 variant.
			cd2.set_hash_in_slot(ceb.get_hash_sha256(), CodeSignBlob::SLOT_DER_ENTITLEMENTS);
		}

		// Calculate signature size.
		int sign_size = 12; // SuperBlob header.
		sign_size += cd1.get_size() + 8;
		sign_size += cd2.get_size() + 8;
		sign_size += rq.get_size() + 8;
		if (!cet.is_empty()) {
			sign_size += cet.get_size() + 8;
		}
		if (!ceb.is_empty()) {
			sign_size += ceb.get_size() + 8;
		}
		sign_size += 16; // Empty signature size.

		// Alloc/resize signature load command.
		print_verbose(vformat("CodeSign: Reallocating space for the signature superblob (%d)...", sign_size));
		if (!mh.set_signature_size(sign_size)) {
			CLEANUP();
			r_error_msg = TTR("Can't resize signature load command.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Can't resize signature load command.");
		}

		print_verbose("CodeSign: Calculating executable code hashes...");
		// Calculate executable code hashes.
		PackedByteArray buffer;
		PackedByteArray hash1, hash2;
		hash1.resize(0x14);
		hash2.resize(0x20);
		buffer.resize(1 << 12);
		mh.get_file()->seek(0);
		for (int32_t j = 0; j < cd2.get_page_count(); j++) {
			mh.get_file()->get_buffer(buffer.ptrw(), (1 << 12));
			CryptoCore::SHA256Context ctx2;
			ctx2.start();
			ctx2.update(buffer.ptr(), (1 << 12));
			ctx2.finish(hash2.ptrw());
			cd2.set_hash_in_slot(hash2, j);

			CryptoCore::SHA1Context ctx1;
			ctx1.start();
			ctx1.update(buffer.ptr(), (1 << 12));
			ctx1.finish(hash1.ptrw());
			cd1.set_hash_in_slot(hash1, j);
		}
		if (cd2.get_page_remainder() > 0) {
			mh.get_file()->get_buffer(buffer.ptrw(), cd2.get_page_remainder());
			CryptoCore::SHA256Context ctx2;
			ctx2.start();
			ctx2.update(buffer.ptr(), cd2.get_page_remainder());
			ctx2.finish(hash2.ptrw());
			cd2.set_hash_in_slot(hash2, cd2.get_page_count());

			CryptoCore::SHA1Context ctx1;
			ctx1.start();
			ctx1.update(buffer.ptr(), cd1.get_page_remainder());
			ctx1.finish(hash1.ptrw());
			cd1.set_hash_in_slot(hash1, cd1.get_page_count());
		}

		print_verbose("CodeSign: Generating signature...");
		CodeSignBlob cs = CodeSignBlob::make_signature();

		print_verbose("CodeSign: Writing signature superblob...");
		// Write signature data to the executable.
		CodeSignSuperBlob sb = CodeSignSuperBlob();
		sb.add_blob(cd2);
		sb.add_blob(cd1);
		sb.add_blob(rq);
		if (!cet.is_empty()) {
			sb.add_blob(cet);
		}
		if (!ceb.is_empty()) {
			sb.add_blob(ceb);
		}
		sb.add_blob(cs);
		mh.get_file()->seek(mh.get_signature_offset());
		sb.write_to_file(mh.get_file());
	}
	if (files_to_sign.size() > 1) {
		print_verbose("CodeSign: Rebuilding fat executable...");
		LipO lip;
		if (!lip.create_file(main_exe, files_to_sign)) {
			CLEANUP();
			r_error_msg = TTR("Failed to create fat binary.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Failed to create fat binary.");
		}
		CLEANUP();
	}
	FileAccess::set_unix_permissions(main_exe, 0755); // Restore unix permissions.
	return OK;
#undef CLEANUP
}

Error CodeSign::codesign(bool p_use_hardened_runtime, bool p_force, const String &p_path, const String &p_ent_path, String &r_error_msg) {
	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
	if (da.is_null()) {
		r_error_msg = TTR("Can't get filesystem access.");
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "CodeSign: Can't get filesystem access.");
	}

	if (da->dir_exists(p_path)) {
		String fmw_ver = "Current"; // Framework version (default).
		String info_path;
		String main_exe;
		String bundle_path;
		bool bundle = false;
		bool ios_bundle = false;
		if (da->file_exists(p_path.path_join("Contents/Info.plist"))) {
			info_path = p_path.path_join("Contents/Info.plist");
			main_exe = p_path.path_join("Contents/MacOS");
			bundle_path = p_path.path_join("Contents");
			bundle = true;
		} else if (da->file_exists(p_path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver)))) {
			info_path = p_path.path_join(vformat("Versions/%s/Resources/Info.plist", fmw_ver));
			main_exe = p_path.path_join(vformat("Versions/%s", fmw_ver));
			bundle_path = p_path.path_join(vformat("Versions/%s", fmw_ver));
			bundle = true;
		} else if (da->file_exists(p_path.path_join("Resources/Info.plist"))) {
			info_path = p_path.path_join("Resources/Info.plist");
			main_exe = p_path;
			bundle_path = p_path;
			bundle = true;
		} else if (da->file_exists(p_path.path_join("Info.plist"))) {
			info_path = p_path.path_join("Info.plist");
			main_exe = p_path;
			bundle_path = p_path;
			bundle = true;
			ios_bundle = true;
		}
		if (bundle) {
			return _codesign_file(p_use_hardened_runtime, p_force, info_path, main_exe, bundle_path, p_ent_path, ios_bundle, r_error_msg);
		} else {
			r_error_msg = TTR("Unknown bundle type.");
			ERR_FAIL_V_MSG(FAILED, "CodeSign: Unknown bundle type.");
		}
	} else if (da->file_exists(p_path)) {
		return _codesign_file(p_use_hardened_runtime, p_force, "", p_path, "", p_ent_path, false, r_error_msg);
	} else {
		r_error_msg = TTR("Unknown object type.");
		ERR_FAIL_V_MSG(FAILED, "CodeSign: Unknown object type.");
	}
}

/*************************************************************************/
/* PluginConfigAppleEmbedded                                             */
/*************************************************************************/

String PluginConfigAppleEmbedded::resolve_local_dependency_path(
	String plugin_config_dir, String dependency_path)
{
	String absolute_path;

	if (dependency_path.is_empty()) {
		return absolute_path;
	}

	if (dependency_path.is_absolute_path()) {
		return dependency_path;
	}

	String res_path = ProjectSettings::globalize_path("res://");
	absolute_path = plugin_config_dir.path_join(dependency_path);

	return absolute_path.replace(res_path, "res://");
}

String PluginConfigAppleEmbedded::resolve_system_dependency_path(String dependency_path)
{
	String absolute_path;

	if (dependency_path.is_empty()) {
		return absolute_path;
	}

	if (dependency_path.is_absolute_path()) {
		return dependency_path;
	}

	String system_path = "/System/Library/Frameworks";

	return system_path.path_join(dependency_path);
}

Vector<String> PluginConfigAppleEmbedded::resolve_local_dependencies(
	String plugin_config_dir, Vector<String> p_paths)
{
	Vector<String> paths;

	for (int i = 0; i < p_paths.size(); i++) {
		String path = resolve_local_dependency_path(plugin_config_dir, p_paths[i]);

		if (path.is_empty()) {
			continue;
		}

		paths.push_back(path);
	}

	return paths;
}

Vector<String> PluginConfigAppleEmbedded::resolve_system_dependencies(Vector<String> p_paths)
{
	Vector<String> paths;

	for (int i = 0; i < p_paths.size(); i++) {
		String path = resolve_system_dependency_path(p_paths[i]);

		if (path.is_empty()) {
			continue;
		}

		paths.push_back(path);
	}

	return paths;
}

bool PluginConfigAppleEmbedded::validate_plugin(PluginConfigAppleEmbedded& plugin_config)
{
	bool valid_name = !plugin_config.name.is_empty();
	bool valid_binary_name = !plugin_config.binary.is_empty();
	bool valid_initialize = !plugin_config.initialization_method.is_empty();
	bool valid_deinitialize = !plugin_config.deinitialization_method.is_empty();

	bool fields_value = valid_name && valid_binary_name && valid_initialize && valid_deinitialize;

	if (!fields_value) {
		return false;
	}

	String plugin_extension = plugin_config.binary.get_extension().to_lower();

	if ((plugin_extension == "a" && FileAccess::exists(plugin_config.binary)) ||
		(plugin_extension == "xcframework" && DirAccess::exists(plugin_config.binary))) {
		plugin_config.valid_config = true;
		plugin_config.supports_targets = false;
	}
	else {
		String file_path = plugin_config.binary.get_base_dir();
		String file_name = plugin_config.binary.get_basename().get_file();
		String file_extension = plugin_config.binary.get_extension();
		String release_file_name = file_path.path_join(file_name + ".release." + file_extension);
		String debug_file_name = file_path.path_join(file_name + ".debug." + file_extension);

		if ((plugin_extension == "a" && FileAccess::exists(release_file_name) &&
				FileAccess::exists(debug_file_name)) ||
			(plugin_extension == "xcframework" && DirAccess::exists(release_file_name) &&
				DirAccess::exists(debug_file_name))) {
			plugin_config.valid_config = true;
			plugin_config.supports_targets = true;
		}
	}

	return plugin_config.valid_config;
}

String PluginConfigAppleEmbedded::get_plugin_main_binary(
	PluginConfigAppleEmbedded& plugin_config, bool p_debug)
{
	if (!plugin_config.supports_targets) {
		return plugin_config.binary;
	}

	String plugin_binary_dir = plugin_config.binary.get_base_dir();
	String plugin_name_prefix = plugin_config.binary.get_basename().get_file();
	String plugin_extension = plugin_config.binary.get_extension();
	String plugin_file =
		plugin_name_prefix + "." + (p_debug ? "debug" : "release") + "." + plugin_extension;

	return plugin_binary_dir.path_join(plugin_file);
}

uint64_t PluginConfigAppleEmbedded::get_plugin_modification_time(
	const PluginConfigAppleEmbedded& plugin_config, const String& config_path)
{
	uint64_t last_updated = FileAccess::get_modified_time(config_path);

	if (!plugin_config.supports_targets) {
		last_updated = MAX(last_updated, FileAccess::get_modified_time(plugin_config.binary));
	}
	else {
		String file_path = plugin_config.binary.get_base_dir();
		String file_name = plugin_config.binary.get_basename().get_file();
		String plugin_extension = plugin_config.binary.get_extension();
		String release_file_name = file_path.path_join(file_name + ".release." + plugin_extension);
		String debug_file_name = file_path.path_join(file_name + ".debug." + plugin_extension);

		last_updated = MAX(last_updated, FileAccess::get_modified_time(release_file_name));
		last_updated = MAX(last_updated, FileAccess::get_modified_time(debug_file_name));
	}

	return last_updated;
}


