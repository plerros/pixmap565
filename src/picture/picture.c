// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_utils.h"
#include "picture.h"
#include "pixmap.h"

// all integer values are stored in little-endian format

struct picture
{
// Bitmap file header (bytes 0~13)
#define MAGIC_NUMBER ('B' + ('M' << CHAR_BIT)) // (header_field)
	udword_t file_bytes;
	uword_t  reserved_1;
	uword_t  reserved_2;
	udword_t pixel_array_offset;

// DIB HEADER: BITMAPINFOHEADER (bytes 14~53)
	udword_t DIB_bytes;
	dword_t  width;  // signed integer
	dword_t  height; // signed integer
#define COLOR_PLANES 1ul
#define BITS_PER_PIXEL 16u
#define COMPRESSION_METHOD 3ul // BI_BITFIELDS, required for RGB565
	udword_t image_size; // in bytes
	dword_t  horizontal_resolution; // pixels per meter, signed integer
	dword_t  vertical_resolution;   // pixels per meter, signed integer
	udword_t palette_colors;
	udword_t important_colors;

// Extra bit masks (bytes 54~65)
#define RED_BITMASK   0b1111100000000000ul
#define GREEN_BITMASK 0b0000011111100000ul
#define BLUE_BITMASK  0b0000000000011111ul

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
	new->image_size = 0;
	new->horizontal_resolution = 0;
	new->vertical_resolution = 0;
	new->palette_colors = 0;
	new->important_colors = 0;

// Extra bit masks
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

	pixmap_flip_y(matrix);

	ptr->matrix = matrix;
	ptr->width = pixmap_get_x(matrix);
	ptr->height = pixmap_get_y(matrix);

	ptr->image_size = dword_abs(ptr->width) * BYTES_PER_PIXEL;
	ptr->image_size += ptr->image_size % 4;
	ptr->image_size *= dword_abs(ptr->height);
	ptr->file_bytes = ptr->image_size + ptr->pixel_array_offset;
}

struct pixmap *picture_get_pixmap(struct picture *ptr)
{
	struct pixmap *ret = NULL;
	ret = ptr->matrix;
	ptr->matrix = NULL;

	// by default the image is stored upside down
	if (ptr->height > 0)
		pixmap_flip_y(ret);

	if (ptr->width < 0)
		pixmap_flip_x(ret);

	return ret;
}

bool is_pic(char *filename)
{
	bool ret = false;
	if (filename == NULL)
		goto out;

	size_t size = strlen(filename);
	if (size <= strlen(PICTURE_EXTENSION))
		goto out;

	char bmp_ext[] = PICTURE_EXTENSION;
	char tmp[] = PICTURE_EXTENSION; // ensure that we have enough characters
	strcpy(tmp, &(filename[size - strlen(PICTURE_EXTENSION)]));

	if (strcmp(tmp, bmp_ext) == 0)
		ret = true;
out:
	return ret;
}

static void conflicting_data(void)
{
	print_error();
	fprintf(stderr, "The input file has conflicting data.\n");
}

static void bad_data(const char *structure, const char *name)
{
	print_error();
	fprintf(stderr,
		"The input file has invalid or unsupported data.\n"
		"structure: %s\n"
		"field:     %s\n", structure, name
	);
}

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

int type(int item)
{
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

		skip, // gap1
		pixel,
		skip, // padding
		skip  // gap2
	};
	return (type[item]);
}

int picture_read(struct picture *ptr, FILE *fp)
{
	assert(ptr != NULL);
	assert(fp != NULL);

	pixmap_free(ptr->matrix);
	ptr->matrix = NULL;

	int rc = 0;

	udword_t offset = 0; // byte offset of the current item
	udword_t byte = 0;   // # of bytes from the file start

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
			fprintf(stderr, "The value of byte %llu is out of range [0, %u].\n", (unsigned long long)byte, UCHAR_MAX);
			rc = 1;
			goto out;
		}

		udword_t item_size = 0;
		switch (type(item)) {
		case uword:
			uw_value += ((uword_t)ch) << (byte - offset) * CHAR_BIT;
			item_size = 2;
			break;

		case dword:
			{
				udword_t u_value = ch;
				u_value <<= (CHAR_BIT * (byte-offset));
				dw_value |= u_value;
			}
			item_size = 4;
			break;

		case udword:
			udw_value += ((udword_t)ch) << (byte - offset) * CHAR_BIT;
			item_size = 4;
			break;

		case skip:
			item_size = skip_bytes;
			break;

		case pixel:
			uw_value += ((uword_t)ch) << ((byte - offset) % BYTES_PER_PIXEL) * CHAR_BIT;
			if ((byte - offset) % BYTES_PER_PIXEL == BYTES_PER_PIXEL - 1) {
				pixmap_add(ptr->matrix, uw_value);
				uw_value = 0;
			}
			item_size = dword_abs(ptr->width) * BYTES_PER_PIXEL;
			break;
		}
		byte++;

		if (byte - offset == item_size) {
			switch (item) {
			case magic_number:
				if (uw_value != MAGIC_NUMBER) {
					bad_data("Bitmap file header", "magic number");
					rc = 1;
				}
				break;

			case file_bytes:
				if (udw_value < 14 + 40 + 12) {
					bad_data("Bitmap file header", "filesize");
					fprintf(stderr, "expected:  >= %u\n", 14 + 40 + 12);
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
				} else if (udw_value < 40) {
					bad_data("DIB header", "DIB header size");
					fprintf(stderr, "expected:  >= %u\n", 40);
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
					pixmap_new(&(ptr->matrix), dword_abs(ptr->width));
				}
				break;

			case height:
				if (dw_value <= 0) {
					print_warning();
					fprintf(stderr, "negative height\n");
				}
				if (dword_abs(dw_value) > (ptr->file_bytes - ptr->pixel_array_offset)) {
					conflicting_data();
					fprintf(stderr, "height * %u > filesize - pixel_array_offset\n", BYTES_PER_PIXEL);
					fprintf(stderr, "(i.e., too many pixels or too little space)\n");
					rc = 1;
				} else if (dword_abs(dw_value) * BYTES_PER_PIXEL > ULONG_MAX /  dword_abs(ptr->width)) {
					conflicting_data();
					fprintf(stderr, "height * width * %u > %lu\n", BYTES_PER_PIXEL, ULONG_MAX);
					fprintf(stderr, "(i.e., too many pixels)\n");
					rc = 1;
				} else if (dword_abs(ptr->width) * dword_abs(dw_value) * BYTES_PER_PIXEL > ptr->file_bytes - ptr->pixel_array_offset) {
					conflicting_data();
					fprintf(stderr, "height * width * %u > filesize - pixel_array_offset\n", BYTES_PER_PIXEL);
					fprintf(stderr, "(i.e., too many pixels or too little space)\n");
					rc = 1;
				} else {
					ptr->height = dw_value;
				}
				break;

			case color_planes:
				if (uw_value != COLOR_PLANES) {
					bad_data("DIB header", "color planes");
					fprintf(stderr, "expected:  %lu\n", COLOR_PLANES);
					rc = 1;
				}
				break;

			case bits_per_pixel:
				if (uw_value != BITS_PER_PIXEL) {
					bad_data("DIB header", "bits per pixel");
					fprintf(stderr, "expected:  %u\n", BITS_PER_PIXEL);
					rc = 1;
				}
				break;

			case compression_method:
				if (udw_value != COMPRESSION_METHOD) {
					bad_data("DIB header", "compression method");
					fprintf(stderr, "expected:  %lu\n", COMPRESSION_METHOD);
					rc = 1;
				}
				break;

			case image_size:
				{
					udword_t tmp = dword_abs(ptr->width) * BYTES_PER_PIXEL;
					if (udw_value != (tmp + tmp % 4) * dword_abs(ptr->height)) {
						conflicting_data();
						fprintf(stderr, "image_size != (width + padding) * height * %u\n", BYTES_PER_PIXEL);
						fprintf(stderr, "(i.e., too many pixels or too little space)\n");
						rc = 1;
					} else {
						ptr->image_size = udw_value;
					}
				}
				break;

			//case horizontal_resolution:
			//case vertical_resolution:
			//case palette_colors:
			//case important_colors:

			case red_bitmask:
				if (udw_value != RED_BITMASK) {
					bad_data("Extra bit masks", "red bitmask");
					fprintf(stderr, "expected:  %lu\n", RED_BITMASK);
					rc = 1;
				}
				break;

			case green_bitmask:
				if (udw_value != GREEN_BITMASK) {
					bad_data("Extra bit masks", "green bitmask");
					fprintf(stderr, "expected:  %lu\n", GREEN_BITMASK);
					rc = 1;
				}
				break;

			case blue_bitmask:
				if (udw_value != BLUE_BITMASK) {
					bad_data("Extra bit masks", "blue bitmask");
					fprintf(stderr, "expected:  %lu\n", BLUE_BITMASK);
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
					if ((dword_abs(ptr->width) * BYTES_PER_PIXEL) % 4 != 0)
						skip_bytes = 4 - ((dword_abs(ptr->width) * BYTES_PER_PIXEL) % 4);
					else if (byte < ptr->pixel_array_offset + ptr->image_size)
						item = pixel_line;
				}
				if (item == gap2) {
					if (byte < ptr->pixel_array_offset + ptr->image_size)
						item = pixel_line;
					else
						skip_bytes = ptr->file_bytes - byte;
				}
			} while (type(item) == skip && skip_bytes == 0);

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

int picture_write(struct picture *ptr, FILE *fp)
{
	assert(fp != NULL);

	int rc = 0;
	udword_t byte = 0;

	int item = 0;
	while (rc == 0) {
		switch(item) {
		// Bitmap file header
		case magic_number:
			rc = fput_uword(MAGIC_NUMBER, fp);
			break;

		case file_bytes:
			if (ptr->file_bytes > 2 << 20) {
				print_warning();
				fprintf(stderr, "The output file is very large and could cause compatibility issues.\n");
			}
			rc = fput_udword(ptr->file_bytes, fp);
			break;

		case reserved_1:
			rc = fput_uword(ptr->reserved_1, fp);
			break;

		case reserved_2:
			rc = fput_uword(ptr->reserved_2, fp);
			break;

		case pixel_array_offset:
			rc = fput_udword(ptr->pixel_array_offset, fp);
			break;

		// DIB HEADER (bytes 14~54)
		case DIB_bytes:
			rc = fput_udword(ptr->DIB_bytes, fp);
			break;

		case width:
			rc = fput_dword(ptr->width, fp);
			break;

		case height:
			rc = fput_dword(ptr->height, fp);
			break;

		case color_planes:
			rc = fput_uword(COLOR_PLANES, fp);
			break;

		case bits_per_pixel:
			rc = fput_uword(BITS_PER_PIXEL, fp);
			break;

		case compression_method:
			rc = fput_udword(COMPRESSION_METHOD, fp);
			break;

		case image_size:
			rc = fput_udword(ptr->image_size, fp);
			break;

		case horizontal_resolution:
			rc = fput_dword(ptr->horizontal_resolution, fp);
			break;

		case vertical_resolution:
			rc = fput_dword(ptr->vertical_resolution, fp);
			break;

		case palette_colors:
			rc = fput_udword(ptr->palette_colors, fp);
			break;

		case important_colors:
			rc = fput_udword(ptr->important_colors, fp);
			break;

		// Extra bit masks
		case red_bitmask:
			rc = fput_udword(RED_BITMASK, fp);
			break;

		case green_bitmask:
			rc = fput_udword(GREEN_BITMASK, fp);
			break;

		case blue_bitmask:
			rc = fput_udword(BLUE_BITMASK, fp);
			break;

		case gap:
			for (; byte < ptr->pixel_array_offset && rc == 0; byte++)
				rc = (fputc(0, fp) != rc);
			break;

		case pixel_line:
			rc = pixmap_write(ptr->matrix, fp);
			break;

		case gap2:
			for (; byte < ptr->file_bytes && rc == 0; byte++)
				rc = (fputc(0, fp) != rc);
			goto out;
		}
		switch (type(item)) {
		case uword:
			byte += 2;
			break;

		case dword:
			byte += 4;
			break;

		case udword:
			byte += 4;
			break;

		case skip:
			// handled in the previous switch case
			break;

		case pixel:
			byte += ptr->image_size;
			break;
		}
		item++;
	}
out:
	if (rc)
		fprintf(stderr, "\n");

	return rc;
}
