/* Differential coding - convert to/from a byte stream of differences
   Copyright (C) 2023 by Jody Bruchon <jody@jodybruchon.com>
   Released under The MIT License */


#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int differential_enc8(const char *in, char *out, int len)
{
	char c = 0;

	if (in == NULL || out == NULL) return -1;

	for (int i = 0; i < len; i++) {
		out[i] = in[i] - c;
		c = in[i];
	}
	return 0;
}


int differential_dec8(const char *in, char *out, int len)
{
	char c = 0;

	if (in == NULL || out == NULL) return -1;

	for (int i = 0; i < len; i++) {
		out[i] = in[i] + c;
		c = out[i];
	}
	return 0;
}


#define CHUNK_SIZE 32768
int main(int argc, char **argv)
{
	FILE *in = stdin, *out = stdout;
	uint64_t src[CHUNK_SIZE], dest[CHUNK_SIZE];
	int mode;

	if (argc < 2 || *argv[1] != '-') goto usage;
	switch (*(argv[1] + 1)) {
		case 'c':
			mode = 0;
			break;
		case 'd':
			mode = 1;
			break;
		default:
			goto usage;
	}
	while (1) {
		int bytes, i;

		errno = 0;
		bytes = fread((char *)src, 1, CHUNK_SIZE, in);
		if (ferror(in) != 0) {
			fprintf(stderr, "error reading data [%d] %s\n", errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (bytes > 0) {
			if (mode == 0) differential_enc8((char *)src, (char *)dest, bytes);
			else differential_dec8((char *)src, (char *)dest, bytes);
			i = fwrite(dest, 1, bytes, out);
			if (ferror(out) != 0 || i != bytes) {
				fprintf(stderr, "error writing data [%d] %s\n", errno, strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		if (feof(in) != 0) break;
	}
	exit(EXIT_SUCCESS);
usage:
	fprintf(stderr, "usage: input | %s -c|-d > output\n", argv[0]);
	exit(EXIT_FAILURE);
}
