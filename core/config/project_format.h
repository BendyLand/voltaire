/**************************************************************************/
/*  project_format.h                                                      */
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

#include <cstddef>
#include <cstdint>
#include <cstring>

#define VLTR_PROJECT_MAGIC_0 'V'
#define VLTR_PROJECT_MAGIC_1 'L'
#define VLTR_PROJECT_MAGIC_2 'T'
#define VLTR_PROJECT_MAGIC_3 'R'
#define VLTR_PROJECT_FORMAT_VERSION 1u

enum VoltaireProjectFlags
{
	PROJECT_FLAG_NONE = 0,
	PROJECT_FLAG_USE_3D = 1u << 0,
	PROJECT_FLAG_VSYNC = 1u << 1,
	PROJECT_FLAG_BORDERLESS = 1u << 2,
	PROJECT_FLAG_RESIZABLE = 1u << 3,
};

/* 24 bytes: exact byte-for-byte layout of the start of the file */
typedef struct VoltaireProjectHeader
{
	char magic[4];
	uint32_t format_version;
	uint32_t header_size;
	uint32_t config_size;
	uint32_t flags;
	uint32_t reserved;
} VoltaireProjectHeader;

/* 672 bytes: exact byte-for-byte layout of the project settings payload */
typedef struct VoltaireProjectConfig
{
	char project_name[128];
	char main_scene[256];
	char icon_path[256];

	uint32_t window_width;
	uint32_t window_height;
	uint32_t physics_ticks_per_second;
	uint32_t max_fps;

	float clear_color[4];
} VoltaireProjectConfig;

/* 696 bytes: the entire file on disk is literally just these two structs back-to-back */
typedef struct VoltaireProjectFile
{
	VoltaireProjectHeader header;
	VoltaireProjectConfig config;
} VoltaireProjectFile;

/* Compile-time layout locks so compiler padding never silently shifts offsets */
static_assert(sizeof(VoltaireProjectHeader) == 24, "Header must be 24 bytes");
static_assert(sizeof(VoltaireProjectConfig) == 672, "Config must be 672 bytes");
static_assert(sizeof(VoltaireProjectFile) == 696, "File must be 696 bytes");
static_assert(
	offsetof(VoltaireProjectFile, config) == 24, "Config must immediately follow header");

static inline void vltr_copy_cstr(char* p_dst, size_t p_dst_cap, const char* p_src)
{
	if (!p_dst || p_dst_cap == 0) {
		return;
	}
	memset(p_dst, 0, p_dst_cap);
	if (!p_src) {
		return;
	}
	size_t len = strlen(p_src);
	size_t copy_len = len < (p_dst_cap - 1) ? len : (p_dst_cap - 1);
	memcpy(p_dst, p_src, copy_len);
}

static inline void vltr_project_init(
	VoltaireProjectFile* p_file, const char* p_name, const char* p_main_scene)
{
	memset(p_file, 0, sizeof(*p_file));

	p_file->header.magic[0] = VLTR_PROJECT_MAGIC_0;
	p_file->header.magic[1] = VLTR_PROJECT_MAGIC_1;
	p_file->header.magic[2] = VLTR_PROJECT_MAGIC_2;
	p_file->header.magic[3] = VLTR_PROJECT_MAGIC_3;
	p_file->header.format_version = VLTR_PROJECT_FORMAT_VERSION;
	p_file->header.header_size = sizeof(VoltaireProjectHeader);
	p_file->header.config_size = sizeof(VoltaireProjectConfig);
	p_file->header.flags = PROJECT_FLAG_VSYNC | PROJECT_FLAG_RESIZABLE;

	vltr_copy_cstr(p_file->config.project_name, sizeof(p_file->config.project_name), p_name);
	vltr_copy_cstr(p_file->config.main_scene, sizeof(p_file->config.main_scene), p_main_scene);

	p_file->config.window_width = 1152;
	p_file->config.window_height = 648;
	p_file->config.physics_ticks_per_second = 60;
	p_file->config.clear_color[0] = 0.12f;
	p_file->config.clear_color[1] = 0.14f;
	p_file->config.clear_color[2] = 0.17f;
	p_file->config.clear_color[3] = 1.00f;
}

static inline bool vltr_header_valid(const VoltaireProjectHeader* p_hdr)
{
	return p_hdr->magic[0] == VLTR_PROJECT_MAGIC_0 && p_hdr->magic[1] == VLTR_PROJECT_MAGIC_1 &&
		   p_hdr->magic[2] == VLTR_PROJECT_MAGIC_2 && p_hdr->magic[3] == VLTR_PROJECT_MAGIC_3 &&
		   p_hdr->format_version == VLTR_PROJECT_FORMAT_VERSION &&
		   p_hdr->header_size == sizeof(VoltaireProjectHeader) &&
		   p_hdr->config_size == sizeof(VoltaireProjectConfig);
}

static inline bool vltr_project_valid(const VoltaireProjectFile* p_file)
{
	return vltr_header_valid(&p_file->header) &&
		   p_file->config.project_name[sizeof(p_file->config.project_name) - 1] == '\0' &&
		   p_file->config.main_scene[sizeof(p_file->config.main_scene) - 1] == '\0' &&
		   p_file->config.icon_path[sizeof(p_file->config.icon_path) - 1] == '\0';
}

