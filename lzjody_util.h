/*
 * Lempel-Ziv-JodyBruchon compression library
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 */

#ifndef LZJODY_UTIL_H
#define LZJODY_UTIL_H

#include <lzjody.h>

#define LZJODY_UTIL_VER "0.4"
#define LZJODY_UTIL_VERDATE "2023-08-09"

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

/* File read chunk size */
#ifndef UTIL_BSIZE
 #define UTIL_BSIZE 1048576
#endif
#define MIN_BSIZE 4096
#define IDEAL_BSIZE 1048576

#ifdef THREADED
 #include <pthread.h>
/* Per-thread working state */
struct thread_info {
	unsigned char *in;	/* Thread input blocks */
	unsigned char *out;	/* Thread output blocks */
	pthread_t id;	/* Thread ID */
	int block;	/* What block is thread working on? */
	int in_length;	/* Input size */
	int out_length;	/* Output size */
	int working;	/* 0 = idle, 1 = working, -1 = completed */
	char options;	/* Compressor options */
};

/* List of blocks to write */
struct thread_writes {
	struct thread_writes *next;
	unsigned char *data;
	int length;
	int block;
};
#endif /* THREADED */

#endif	/* LZJODY_UTIL_H */
