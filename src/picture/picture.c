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

typedef unsigned short uword_t;
typedef long dword_t;
typedef unsigned long udword_t;
// all integer values are stored in little-endian format

struct picture
{
// Bitmap file header (bytes 0~13)
	uword_t  magic_number; // (header_field)
	udword_t file_bytes;
	uword_t  reserved_1;
	uword_t  reserved_2;
	udword_t pixel_array_offset;

// DIB HEADER
	// BITMAPINFOHEADER (bytes 14~53)
	udword_t DIB_bytes;
	dword_t  width;  // signed integer
	dword_t  height; // signed integer
	uword_t  color_planes;
	uword_t  bits_per_pixel;
	udword_t compression_method;
	udword_t image_size; // in bytes
	dword_t  horizontal_resolution; // pixels per meter, signed integer
	dword_t  vertical_resolution;   // pixels per meter, signed integer
	udword_t palette_colors;
	udword_t important_colors;

// Extra bit masks (bytes 54~65)
	udword_t red_bitmask;
	udword_t green_bitmask;
	udword_t blue_bitmask;

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
	new->magic_number = 'B' + 'M' * (UCHAR_MAX + 1);
	new->file_bytes = 14; // (14 is the size of this header)
	new->reserved_1 = 0;
	new->reserved_2 = 0;
	new->pixel_array_offset = 14;

// DIB header (BITMAPINFOHEADER)
	new->DIB_bytes = 40;
	new->file_bytes += new->DIB_bytes;
	new->pixel_array_offset += new->DIB_bytes;

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

	new->file_bytes += 12;
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
	ptr->file_bytes += ptr->image_size * 2;
}

struct pixmap *picture_get_pixmap(struct picture *ptr)
{
	struct pixmap *ret = NULL;
	ret = ptr->matrix;
	ptr->matrix = NULL;

	return ret;
}

int picture_read(struct picture *ptr, FILE *fp)
{
	int rc = 0;

	enum item_ids{
		// bitmap header
		magic_number,
		file_bytes,
		reserved_1,
		reserved_2,
		pixel_array_offset,

		// DIB header
		DIB_bytes,
		width,
		height,
		color_planes,
		bits_per_pixel,
		compression_method,
		image_size,
		horizontal_resolution,
		vertical_resolution,
		palette_colors,
		important_colors,

		// extra bitmasks
		red_bitmask,
		green_bitmask,
		blue_bitmask,

		gap,        // space gap
		pixel_line, // pixel(s)
		padding     // padding
	};

	enum item_types {
		uword,
		dword,
		udword,
		zero,
		pixel
	};

	int type[] = {
		// bitmap header
		uword,
		udword,
		uword,
		uword,
		udword,

		// DIB header
		udword,
		dword, // width
		dword, // height
		uword,
		uword,
		udword,
		udword,
		dword,
		dword,
		udword,
		udword,

		// extra bitmasks
		udword,
		udword,
		udword,

		zero, // size will be filled in during execution
		pixel,
		zero
	};

	unsigned long offset = 0; // byte offset of the current item
	unsigned long byte = 0;   // # of bytes from the file start

	uword_t uw_value = 0;
	dword_t dw_value = 0;
	udword_t udw_value = 0;
	int item = 0;

	int zero_size = 0;

	while (1) {
		int ch = fgetc(fp);

		if (feof(fp)) {
			break;
		}
		if (ferror(fp)) {

		}

		assert(ch <= UCHAR_MAX);
		assert(byte >= offset);

		unsigned long item_size = 0;

		switch (type[item]) {
			case uword:
				{
					unsigned short tmp = ch;
					for (int i = 0; i < byte - offset; i++)
						tmp *= (UCHAR_MAX + 1);
					uw_value += tmp;
				}
				item_size = 2;
				break;

			case dword:
				{
					long tmp = ch;
					for (int i = 0; i < byte - offset; i++)
						tmp *= (UCHAR_MAX + 1);
					dw_value += tmp;

					if (byte - offset == 3) {
						if (ch & ((signed char) -1)) {
							// is negative?
							dw_value *= -1;
						}
					}
				}
				item_size = 4;
				break;

			case udword:
				{
					unsigned long tmp = ch;
					for (int i = 0; i < byte - offset; i++)
						tmp *= (UCHAR_MAX + 1);
					udw_value += tmp;
				}
				item_size = 4;
				break;

			case zero:
				item_size = zero_size;
				break;

			case pixel:
				{
					unsigned short tmp = ch;
					for (int i = 0; i < byte - offset; i++)
						tmp *= (UCHAR_MAX + 1);
					uw_value += tmp;
				}
				if ((byte - offset) % 2 == 1) {
					if (ptr->matrix == NULL) {
						pixmap_new(&(ptr->matrix), ptr->width);
					}
					pixmap_add(ptr->matrix, uw_value);
					uw_value = 0;
				}

				item_size = ptr->width;
				break;
		}
		byte++;

		if (byte - offset == item_size) {
			switch (item) {
				case magic_number:
					assert(ch != ptr->magic_number);
					// check BM
					break;

				case file_bytes:
					break;

				case reserved_1:
					// ignore
					break;

				case reserved_2:
					// ignore
					break;

				case pixel_array_offset:
					// pixel_array_offset < file_bytes
					ptr->pixel_array_offset = udw_value;
					break;

				case DIB_bytes:
					// DIB_bytes + 12 <= pixel_array_offset
					// DIB_bytes >= 40
					// set gap = pixel_array_offset -14 -40 -12
					break;

				case width:
					// THIS IS SIGNED!
					// width * 2 <= file_bytes - pixel_array_offset
					// set pixel line
					// set padding
					ptr->width = dw_value;
					break;

				case height:
					// THIS IS SIGNED!
					// width * height * 2 <= file_bytes - pixel_array_offset
					break;

				case color_planes:
					// = 1
					break;

				case bits_per_pixel:
					// = 16
					break;

				case compression_method:
					// = 3
					break;

				case image_size:
					// image_size = width * height (in bytes)
					break;

				case horizontal_resolution:
					// ignore
					break;

				case vertical_resolution:
					// ignore
					break;

				case palette_colors:
					break;

				case important_colors:
					break;

				case red_bitmask:
					break;

				case green_bitmask:
					break;

				case blue_bitmask:
					break;

				case gap:

					break;

				case pixel_line:
					break;

				case padding:
					break;
				default:
					break;
			}

			if (item == padding)
				item = pixel_line;
			else
				item++;

			if (item == gap) {
				assert(ptr->pixel_array_offset >= byte);
				zero_size = ptr->pixel_array_offset - byte;

			}

			if (item == padding) {
				if (ptr->height % 4 != 0)
					zero_size = 4 - (ptr->height % 4);
				else
					item = pixel_line;
			}

			uw_value = 0;
			dw_value = 0;
			udw_value = 0;
			offset = byte;

		}
	}
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
	fput_uword(ptr->magic_number, fp);
	fput_udword(ptr->file_bytes, fp);
	fput_uword(ptr->reserved_1, fp);
	fput_uword(ptr->reserved_2, fp);
	fput_udword(ptr->pixel_array_offset, fp);
	byte += 14;

// DIB HEADER (bytes 14~54)
	fput_udword(ptr->DIB_bytes, fp);
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
