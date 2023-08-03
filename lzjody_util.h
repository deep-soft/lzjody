/*
 * Lempel-Ziv-JodyBruchon compression library
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 */

#ifndef LZJODY_UTIL_H
#define LZJODY_UTIL_H

#include <lzjody.h>

#define LZJODY_UTIL_VER "0.2"
#define LZJODY_UTIL_VERDATE "2023-07-03"

/* Debugging stuff */
#ifndef DLOG
 #ifdef DEBUG
  #define DLOG(...) fprintf(stderr, __VA_ARGS__)
 #else
  #define DLOG(...)
 #endif
#endif

/* Use POSIX threads in compression utility  (define this in Makefile) */
/* #define THREADED 1 */

struct files_t {
	FILE *in;
	FILE *out;
};

/* Number of LZJODY_BSIZE blocks to process per thread */
#define CHUNK 1

#ifdef THREADED
/* Per-thread working state */
struct thread_info {
	unsigned char in[LZJODY_BSIZE + 4];	/* Thread input blocks */
	unsigned char out[LZJODY_BSIZE + 4];	/* Thread output blocks */
	char options;	/* Compressor options */
	pthread_t id;	/* Thread ID */
	int block;	/* What block is thread working on? */
	int in_length;	/* Input size */
	int out_length;	/* Output size */
	int working;	/* 0 = idle, 1 = working, -1 = completed */
};
#endif /* THREADED */

#endif	/* LZJODY_UTIL_H */
