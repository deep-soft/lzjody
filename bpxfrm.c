/*
 * Byte Plane Transform Utility
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 */

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "byteplane_xfrm.h"

/* Detect Windows and modify as needed */
#if defined _WIN32 || defined __CYGWIN__
 #define ON_WINDOWS 1
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <io.h>
#endif

/* Block size to work on - must be divisible by BYTEPLANES
 * WARNING: this must be the same for a transform to be reversed! */
#define BSIZE 4096
#define BYTEPLANES 4

int main(int argc, char **argv)
{
	static unsigned char blk[BSIZE];
	static unsigned char xfrm[BSIZE];
	int i, length, d = BYTEPLANES;
	long total = 0;
	FILE *in, *out;

	if (argc != 4) goto usage;

	switch (*argv[1]) {
		case 'f':
			break;
		case 'r':
			d = -d;
			break;
		default:
			goto usage;
			break;
	}

	if (strcmp(argv[2], "-") == 0) {
		in = stdin;
	} else {
		in = fopen(argv[2], "rb");
		if (!in) goto error_open_input;
	}
	if (strcmp(argv[3], "-") == 0) {
		out = stdout;
	} else {
		out = fopen(argv[3], "wb");
		if (!out) goto error_open_output;
	}

	errno = 0;
	while ((length = fread(blk, 1, BSIZE, in))) {
		if (ferror(in) != 0) goto error_read;
		total += length;
		i = byteplane_transform(blk, xfrm, length, d);
		if (i != 0) goto error_xfrm;
		i = fwrite(xfrm, 1, length, out);
		if (ferror(out) != 0 || i != length) goto error_write;
	}
//	fprintf(stderr, "Success: %dx%d transformed %ld bytes\n", BYTEPLANES, BSIZE, total);
	exit(EXIT_SUCCESS);

error_open_input:
	fprintf(stderr, "Error opening input file\n");
	exit(EXIT_FAILURE);
error_open_output:
	fprintf(stderr, "Error opening output file\n");
	exit(EXIT_FAILURE);
error_read:
	fprintf(stderr, "Error reading input file\n");
	exit(EXIT_FAILURE);
error_write:
	fprintf(stderr, "Error writing output file (%d of %d written): [%d] %s\n", i, length, errno, strerror(errno));
	exit(EXIT_FAILURE);
error_xfrm:
	fprintf(stderr, "Error: byte plane transform returned failure\n");
	exit(EXIT_FAILURE);
usage:
	fprintf(stderr, "Byte plane transform utility\n");
	fprintf(stderr, "Usage: bpxfrm f|r infile outfile\n");
	fprintf(stderr, "f = forward transform, r = reverse transform\n");
	exit(EXIT_FAILURE);
}
