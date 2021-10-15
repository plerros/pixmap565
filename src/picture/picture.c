// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "picture.h"
#include "pixmap.h"

typedef unsigned short uword;
typedef long dword;
typedef unsigned long udword;
// all integer values are stored in little-endian format

struct picture
{
// Bitmap file header (bytes 0~13)
	uword  header_field;
	udword BMP_size_bytes;
	uword  reserved_1;
	uword  reserved_2;
	udword pixel_array_offset;

// DIB HEADER
// BITMAPINFOHEADER (bytes 14~53)
	udword DIB_header_size;
	dword  width;  // signed integer
	dword  height; // signed integer
	uword  color_planes;
	uword  bits_per_pixel;
	udword compression_method;
	udword image_size;
	dword  horizontal_resolution; // pixels per meter, signed integer
	dword  vertical_resolution;   // pixels per meter, signed integer
	udword palette_colors;
	udword important_colors;

// Extra bit masks (bytes 54~65)
	udword red_bitmask;
	udword green_bitmask;
	udword blue_bitmask;

// Color table
// Gap1
// Pixel array
	struct pixmap *matrix;

// Gap 2
// ICC color profile
};

void picture_new(struct picture **ptr)
{
	struct picture *new = malloc(sizeof(struct picture));
	if (new == NULL)
		abort();

// Bitmap file header
	new->header_field = 'B' + 'M' * (UCHAR_MAX + 1);
	new->BMP_size_bytes = 14; // (the size of this header)
	new->reserved_1 = 0;
	new->reserved_2 = 0;
	new->pixel_array_offset = 14;

// DIB header (BITMAPINFOHEADER)
	new->DIB_header_size = 40;
	new->BMP_size_bytes += new->DIB_header_size;
	new->pixel_array_offset += new->DIB_header_size;

	new->width = 0;
	new->height = 0;
	new->color_planes = 1;
	new->bits_per_pixel = 16;
	new->compression_method = 3; // BI_BITFIELDS, required for RGB565
	new->image_size = 0;
	new->horizontal_resolution = 0;
	new->vertical_resolution = 0;
	new->palette_colors = 0;
	new->important_colors = 0;

// Extra bit masks

/*
 * Red   1111100000000000
 * Green 0000011111100000
 * Blue  0000000000011111
 */
	new->red_bitmask = 0x1f << (6 + 5);
	new->green_bitmask = 0x3f << 5;
	new->blue_bitmask = 0x1f;

	new->BMP_size_bytes += 12;
	new->pixel_array_offset += 12;

// Pixel array
	new->matrix = NULL;

	*ptr = new;
}

void picture_free(struct picture *ptr)
{
	if (ptr == NULL)
		return;

	pixmap_free(ptr->matrix);
	free(ptr);
}

void picture_set_pixmap(struct picture *ptr, struct pixmap *matrix)
{
	assert(ptr != NULL);
	assert(ptr->matrix == NULL);

	ptr->matrix = matrix;
	ptr->width = pixmap_get_x(matrix);
	ptr->height = pixmap_get_y(matrix);

	ptr->image_size = ptr->width * ptr->height;
	ptr->BMP_size_bytes += ptr->image_size * 2;
}

struct pixmap *picture_get_pixmap(struct picture *ptr)
{
	struct pixmap *ret = NULL;
	ret = ptr->matrix;
	ptr->matrix = NULL;

	return ret;
}

void picture_read(struct picture *ptr, FILE *fp)
{

}

static void fput_uword(unsigned short value, FILE *fp)
{
	for (int i = 0; i < 2; i++) {
		int tmp = value % (UCHAR_MAX + 1); // & 0x0f would probably be just fine
		value = value / (UCHAR_MAX + 1);
		fputc(tmp, fp);
	}
}

static void fput_dword(long value, FILE *fp)
{
	for (int i = 0; i < 4; i++) {
		int tmp = value % (UCHAR_MAX + 1); // & 0x0f would probably be just fine
		value = value / (UCHAR_MAX + 1);
		fputc(tmp, fp);
	}
}

static void fput_udword(unsigned long value, FILE *fp)
{
	for (int i = 0; i < 4; i++) {
		int tmp = value % (UCHAR_MAX + 1); // & 0x0f would probably be just fine
		value = value / (UCHAR_MAX + 1);
		fputc(tmp, fp);
	}
}

void picture_write(struct picture *ptr, FILE *fp)
{
	assert(fp != NULL);

	unsigned long long byte = 0;
// Bitmap file header
	fput_uword(ptr->header_field, fp);
	fput_udword(ptr->BMP_size_bytes, fp);
	fput_uword(ptr->reserved_1, fp);
	fput_uword(ptr->reserved_2, fp);
	fput_udword(ptr->pixel_array_offset, fp);
	byte += 14;

// DIB HEADER (bytes 14~54)
	fput_udword(ptr->DIB_header_size, fp);
	fput_dword(ptr->width, fp);
	fput_dword(ptr->height, fp);
	fput_uword(ptr->color_planes, fp);
	fput_uword(ptr->bits_per_pixel, fp);
	fput_udword(ptr->compression_method, fp);
	fput_udword(ptr->image_size, fp);
	fput_dword(ptr->horizontal_resolution, fp);
	fput_dword(ptr->vertical_resolution, fp);
	fput_udword(ptr->palette_colors, fp);
	fput_udword(ptr->important_colors, fp);
	byte += 40;

// Extra bit masks
	fput_udword(ptr->red_bitmask, fp);
	fput_udword(ptr->green_bitmask, fp);
	fput_udword(ptr->blue_bitmask, fp);
	byte += 12;

	for (; byte != ptr->pixel_array_offset; byte++)
		fputc(0, fp);

// Pixel array
	pixmap_write(ptr->matrix, fp);
}
