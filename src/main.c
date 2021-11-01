// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "picture.h"
#include "pixmap.h"

static void help(void)
{
	printf(
		"Usage: pixmap565 [options] -i infile%s -o outfile\n"
		"   or: pixmap565 [options] -w width -i infile -o outfile%s\n"
		"Convert between %s image and RGB565 pixmap.\n\n"
		"  -w [width]   set the width (height is derived from filesize/width)\n"
		"\nOptions:\n"
		"     --help    display this help and exit\n",
		picture_extension(),
		picture_extension(),
		picture_type()
	);
}

static void strnewcpy(char **new, const char *old)
{
	assert(old != NULL);
	free(*new);
	*new = malloc(sizeof(char) * (strlen(old) + 1));
	if (new == NULL)
		abort();

	strcpy(*new, old);
}

static bool willoverflow(udword_t x, unsigned digit)
{
	assert(digit < 10);
	if (x > UDWORD_MAX / 10)
		return true;
	if (x == UDWORD_MAX / 10 && digit > UDWORD_MAX % 10)
		return true;
	return false;
}

static int strto_ul(const char *str, udword_t *number) // string to unsigned long
{
	assert(str != NULL);
	assert(number != NULL);
	int err = 0;
	udword_t tmp = 0;
	for (size_t i = 0; isgraph(str[i]); i++) {
		if (!isdigit(str[i])) {
			err = 1;
			goto out;
		}
		unsigned digit = str[i] - '0';
		if (willoverflow(tmp, digit)) {
			err = 1;
			goto out;
		}
		tmp = 10 * tmp + digit;
	}
	*number = tmp;
out:
	if (err)
		fprintf(stderr, "Input out of range: [0, %llu]\n", (unsigned long long) UDWORD_MAX);
	return err;
}

int main(int argc, char *argv[])
{
	int rc = 0;

	char *inname = NULL;
	char *outname = NULL;
	udword_t width = 0;

	struct picture *pic = NULL;
	struct pixmap *pix = NULL;
	FILE *infile = NULL;
	FILE *outfile = NULL;

	{
		static int help_flag = 0;
		bool infile_is_set = false;
		bool outfile_is_set = false;
		bool width_is_set = false;
		int c;
		while (1) {
			static struct option long_options[] = {
				{"help", no_argument, &help_flag, true},
				{"infile", required_argument, NULL, 'i'},
				{"outfile", required_argument, NULL, 'o'},
				{"width", required_argument, NULL, 'w'},
				{NULL, 0, NULL, 0}
			};
			int option_index = 0;
			c = getopt_long(argc, argv, "i:o:w:", long_options, &option_index);

			// Detect end of the options
			if (c == -1)
				break;

			if (help_flag) {
				help();
				goto out;
			}

			switch (c) {
			case 0:
				break;

			case 'i':
				if (infile_is_set) {
					help();
					goto out;
				}
				strnewcpy(&inname, optarg);
				infile_is_set = true;
				break;

			case 'o':
				if (outfile_is_set) {
					help();
					goto out;
				}
				strnewcpy(&outname, optarg);
				outfile_is_set = true;
				break;

			case 'w':
				if (width_is_set) {
					help();
					goto out;
				}
				rc = strto_ul(optarg, &width);
				width_is_set = true;
				break;

			case '?':
				help();
				goto out;

			default:
				abort();
			}
		}
		if (!infile_is_set || !outfile_is_set) {
			help();
			goto out;
		}
		if (!(is_pic(inname) || is_pic(outname))) {
			help();
			goto out;
		}
		if (!is_pic(inname) && is_pic(outname) && !width_is_set) {
			help();
			goto out;
		}
	}

	infile = fopen(inname, "r");
	if (infile == NULL) {
		printf("Cannot open file '%s'\n", inname);
		rc = 1;
		goto out;
	}

	picture_new(&pic);

	if (is_pic(inname)) {
		rc = picture_read(pic, infile);
		if (rc)
			goto out;
		pix = picture_get_pixmap(pic);
	} else {
		pixmap_new(&pix, width);
		rc = pixmap_read(pix, infile);
		if (rc)
			goto out;
	}

	if (access(outname, F_OK) == 0) {
		printf("File '%s' already exists.\n", outname);
		rc = 1;
		goto out;
	}
	outfile = fopen(outname, "w+");
	if (outfile == NULL) {
		printf("Cannot open file '%s'\n", outname);
		rc = 1;
		goto out;
	}

	if (is_pic(outname)) {
		picture_set_pixmap(pic, pix);
		pix = NULL;
		rc = picture_write(pic, outfile);
		if (rc)
			goto out;
	} else {
		rc = pixmap_write(pix, outfile);
		if (rc)
			goto out;
	}

out:
	picture_free(pic);
	pixmap_free(pix);

	if (infile != NULL)
		fclose(infile);
	if (outfile != NULL)
		fclose(outfile);

	free(inname);
	free(outname);
	return rc;
}
