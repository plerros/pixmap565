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

#define BYTES_PER_PIXEL (8 / CHAR_BIT + 1)

#if (CHAR_BIT % 8) != 0
#error unsupported architecture
#endif

typedef unsigned short uword_t;
typedef long dword_t;
typedef unsigned long udword_t;
// all integer values are stored in little-endian format

static udword_t dword_abs(dword_t value)
{
	if (value < 0)
		value *= -1;
	
	return (value);
}

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

	ptr->image_size = ptr->width * ptr->height * BYTES_PER_PIXEL;
	ptr->file_bytes += ptr->image_size;
}

struct pixmap *picture_get_pixmap(struct picture *ptr)
{
	struct pixmap *ret = NULL;
	ret = ptr->matrix;
	ptr->matrix = NULL;

	return ret;
}
void print_warning()
{
	fprintf(stderr, "\n[WARNING]: ");
}

void print_error()
{
	fprintf(stderr, "\n[ERROR]: ");
}

void conflicting_data()
{
	print_error();
	fprintf(stderr, "The input file has conflicting data.\n");
}

void bad_data(const char *structure, const char *name)
{
	print_error();
	fprintf(stderr,
		"The input file has invalid or unsupported data.\n"
		"structure: %s\n"
		"field:     %s\n", structure, name
	);
}

int picture_read(struct picture *ptr, FILE *fp)
{
	assert(ptr != NULL);
	assert(fp != NULL);

	pixmap_free(ptr->matrix);
	ptr->matrix = NULL;

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
		padding,     // padding
		gap2
	};

	enum item_types {
		uword,  // 2 bytes
		dword,  // 4 bytes
		udword, // 4 bytes
		skip,   // skip_bytes
		pixel   // width*height*bytesperpixel
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

		skip,
		pixel,
		skip,
		skip
	};

	unsigned long offset = 0; // byte offset of the current item
	unsigned long byte = 0;   // # of bytes from the file start

	uword_t uw_value = 0;
	dword_t dw_value = 0;
	udword_t udw_value = 0;
	int item = 0;

	int skip_bytes = 0;

	while (rc == 0) {
		int ch = fgetc(fp);

		if (feof(fp)) {
			print_error();
			fprintf(stderr, "Unexpected end of file.\n");
			rc = 1;
			goto out;
		}
		if (ferror(fp)) {
			print_error();
			fprintf(stderr, "Unexpected end of file, caused by I/O error.\n");
			rc = 1;
			goto out;
		}

		if (ch > UCHAR_MAX) {
			print_error();
			fprintf(stderr, "The value of byte %lu is out of range [0, %lu].\n", byte, UCHAR_MAX);
			rc = 1;
			goto out;
		}

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

			case skip:
				item_size = skip_bytes;
				break;

			case pixel:
				{
					unsigned short tmp = ch;
					for (int i = 0; i < byte - offset; i++)
						tmp *= (UCHAR_MAX + 1);
					uw_value += tmp;
				}
				if ((byte - offset) % BYTES_PER_PIXEL == BYTES_PER_PIXEL - 1) {
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
					if (uw_value != ptr->magic_number) {
						bad_data("Bitmap file header", "magic number");
						rc = 1;
					}
					break;

				case file_bytes:
					if (udw_value < 14 + 40 + 12) {
						bad_data("Bitmap file header", "filesize");
						fprintf(stderr, "expected:  >= %lu\n", 14 + 40 + 12);
						rc = 1;
					} else {
						ptr->file_bytes = udw_value;
					}
					break;

				//case reserved_1:
				//case reserved_2:

				case pixel_array_offset:
					if (udw_value > ptr->file_bytes) {
						conflicting_data();
						fprintf(stderr, "pixel_array_offset > filesize.\n");
						rc = 1;
					} else {
						ptr->pixel_array_offset = udw_value;
					}
					break;

				case DIB_bytes:
					if (udw_value > ptr->pixel_array_offset - 14) {
						conflicting_data();
						fprintf(stderr, "DIB_header_size > pixel_array_offset - BMP_header_size\n");
						fprintf(stderr, "(i.e., the DIB header and pixel array overlap)\n");
						rc = 1;
					}
					else if (udw_value < 40) {
						bad_data("DIB header", "DIB header size");
						fprintf(stderr, "expected:  >= %lu\n", 40);
						rc = 1;
					} else {
						ptr->DIB_bytes = udw_value;
					}
					break;

				case width:
					if (dw_value <= 0) {
						print_warning();
						fprintf(stderr, "negative width\n");
					}
					/*
					 * Because we are converting from signed to unsigned, and BYTES_PER_PIXEL <= 2:
					 * abs(dw_value) <= ULONG_MAX / BYTES_PER_PIXEL
					 */
					if (dword_abs(dw_value) * BYTES_PER_PIXEL > (ptr->file_bytes - ptr->pixel_array_offset)) {
						conflicting_data();
						fprintf(stderr, "width * %u > filesize - pixel_array_offset\n", BYTES_PER_PIXEL);
						fprintf(stderr, "(i.e., too many pixels or too little space)\n");
						rc = 1;
					} else {
						ptr->width = dw_value;
						assert(ptr->matrix == NULL);
						pixmap_new(&(ptr->matrix), ptr->width);
					}
					break;

				case height:
					if (dw_value <= 0) {
						print_warning();
						fprintf(stderr, "negative height\n");
					}
					else if (dword_abs(dw_value) > (ptr->file_bytes - ptr->pixel_array_offset)) {
						conflicting_data();
						fprintf(stderr, "height * %u > filesize - pixel_array_offset\n", BYTES_PER_PIXEL);
						fprintf(stderr, "(i.e., too many pixels or too little space)\n");
						rc = 1;
					}
					else if (dword_abs(dw_value) * BYTES_PER_PIXEL > ULONG_MAX /  dword_abs(ptr->width)) {
						conflicting_data();
						fprintf(stderr, "height * width * %u > %lu\n", BYTES_PER_PIXEL, ULONG_MAX);
						fprintf(stderr, "(i.e., too many pixels)\n");
						rc = 1;
					}
					else if (dword_abs(ptr->width) * dword_abs(dw_value) * BYTES_PER_PIXEL > ptr->file_bytes - ptr->pixel_array_offset) {
						conflicting_data();
						fprintf(stderr, "height * width * %u > filesize - pixel_array_offset\n", BYTES_PER_PIXEL);
						fprintf(stderr, "(i.e., too many pixels or too little space)\n");
						rc = 1;
					} else {
						ptr->height = dw_value;
					}
					break;

				case color_planes:
					if (uw_value != 1) {
						bad_data("DIB header", "color planes");
						fprintf(stderr, "expected:  %lu\n", 1);
						rc = 1;
					}
					break;

				case bits_per_pixel:
					if (uw_value != 16) {
						bad_data("DIB header", "bits per pixel");
						fprintf(stderr, "expected:  %lu\n", 16);
						rc = 1;
					}
					break;

				case compression_method:
					if (udw_value != 3) {
						bad_data("DIB header", "compression method");
						fprintf(stderr, "expected:  %lu\n", 3);
						rc = 1;
					}
					break;

				case image_size:
					if (udw_value == dword_abs(ptr->width) * BYTES_PER_PIXEL * dword_abs(ptr->height) * BYTES_PER_PIXEL) {
						conflicting_data();
						fprintf(stderr, "image_size != height * %u * width * %u\n", BYTES_PER_PIXEL, BYTES_PER_PIXEL);
						fprintf(stderr, "(i.e., too many pixels or too little space)\n");
						rc = 1;
					} else {
						ptr->image_size = udw_value;
					}
					break;

				//case horizontal_resolution:
				//case vertical_resolution:
				//case palette_colors:
				//case important_colors:

				case red_bitmask:
					if (udw_value != ptr->red_bitmask) {
						bad_data("Extra bit masks", "red bitmask");
						fprintf(stderr, "expected:  %lu\n", ptr->red_bitmask);
						rc = 1;
					}
					break;

				case green_bitmask:
					if (udw_value != ptr->green_bitmask) {
						bad_data("Extra bit masks", "green bitmask");
						fprintf(stderr, "expected:  %lu\n", ptr->green_bitmask);
						rc = 1;
					}
					break;

				case blue_bitmask:
					if (udw_value != ptr->blue_bitmask) {
						bad_data("Extra bit masks", "blue bitmask");
						fprintf(stderr, "expected:  %lu\n", ptr->blue_bitmask);
						rc = 1;
					}
					break;

				//case gap:
				//case pixel_line:
				//case padding:

				case gap2:
					goto out;

				default:
					break;
			}
			if (rc)
				break;

			if (byte == ptr->file_bytes)
				goto out;

			skip_bytes = 0;
			do {
				item++; // we assume (item < gap2)
				if (item == gap) {
					assert(byte <= ptr->pixel_array_offset);
					skip_bytes = ptr->pixel_array_offset - byte;
				}
				if (item == padding) {
					if (ptr->height % 4 != 0)
						skip_bytes = 4 - (ptr->height % 4);
					else if (byte < ptr->pixel_array_offset + ptr->image_size)
						item = pixel_line;
				}
				if (item == gap2) {
					assert(byte < ptr->file_bytes);
					skip_bytes = ptr->file_bytes - byte;
				}
			} while (type[item] == skip && skip_bytes == 0);

			uw_value = 0;
			dw_value = 0;
			udw_value = 0;
			offset = byte;
		}
	}
out:
	if (rc)
		fprintf(stderr, "\n");

	return rc;
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
