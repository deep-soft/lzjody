/*
 * Lempel-Ziv-JodyBruchon compression library
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * Released under The MIT License
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "likely_unlikely.h"
#include "lzjody.h"
#include "lzjody_util.h"


/* Detect Windows and modify as needed */
#if defined _WIN32 || defined __CYGWIN__
 #define ON_WINDOWS 1
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <io.h>
#endif

#ifdef THREADED
#include <pthread.h>
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; /* lock for change */
pthread_cond_t thread_change;	/* pthreads change condition */
static int thread_error;	/* nonzero if any thread fails */
#endif

/* Debugging stuff */
#ifndef DLOG
 #ifdef DEBUG
  #define DLOG(...) fprintf(stderr, __VA_ARGS__)
 #else
  #define DLOG(...)
 #endif
#endif

struct files_t files;
struct thread_writes *writes;


/**** End definitions, start code ****/


#ifdef THREADED
static void *compress_thread(void *arg)
{
	struct thread_info * const thr = arg;
	unsigned char *input = thr->in;   /* Uncompressed input pointer */
	unsigned char *output = thr->out; /* Compressed output pointer */
	int i;
	int bsize;	/* Compressor block size */
	int remain = thr->in_length;	/* Remaining input bytes */

	thr->out_length = 0;
	while (remain) {
		bsize = LZJODY_BSIZE;
		if (remain < LZJODY_BSIZE) bsize = remain;
		i = lzjody_compress(input, output, thr->options, bsize);
		if (i < 0) {
			thread_error = 1;
			goto compress_exit;
		}
		input += bsize;
		output += i;
		thr->out_length += i;
		remain -= bsize;
	}
	pthread_mutex_lock(&mtx);
	thr->working = -1;
	pthread_mutex_unlock(&mtx);
compress_exit:
	pthread_cond_broadcast(&thread_change);
	pthread_exit(NULL);
	return 0;
}


/* Write data that arrives in non-sequential order */
int thread_write_and_free(struct thread_info *file)
{
	static int blocknum = 0;
	struct thread_writes *cur, *prev, *del;
	static int blockcnt = 0;

	if (file == NULL) {
//		fprintf(stderr, "Purge called: %d blocks remain\n", blockcnt - blocknum);
		if (blockcnt - blocknum == 0) return 0;
		goto write_only;
	}
//	else fprintf(stderr, "Write and free called: %d blocks remain\n", blockcnt - blocknum);

	blockcnt++;

#if 0
	if (file->block == blocknum) {
		errno = 0;
fprintf(stderr, "EARLY writing block %d length %d\n", blocknum, file->out_length);
		fwrite(file->out, file->out_length, 1, files.out);
		if (unlikely(errno != 0 || ferror(files.out))) return -1;
		free(file->out);
		blocknum++;
fprintf(stderr, "Write: %d blocks remain (%d - %d)\n", blockcnt - blocknum, blockcnt, blocknum);
		return blocknum;
	}
#endif // 0

	cur = (struct thread_writes *)malloc(sizeof(struct thread_writes));
	if (!cur) return -1;
	cur->data = file->out;
	cur->length = file->out_length;
	cur->block = file->block;
//fprintf(stderr, "thread_write: block %d, length %d\n", cur->block, cur->length);
	
	/* Add to write queue head in loose block order */
	if (writes == NULL) {
		writes = cur;
		cur->next = NULL;
	} else if (writes->block > cur->block) {
		cur->next = writes;
		writes = cur;
	} else {
		cur->next = writes->next;
		writes->next = cur;
	}

write_only:
	/* Write out as many blocks as possible */
	prev = NULL;
	cur = writes;
	while (1) {
//fprintf(stderr, "Write loop: %d blocks remain (%d - %d)\n", blockcnt - blocknum, blockcnt, blocknum);
//fprintf(stderr, "thread_write loop start: cur %p, writes %p, next ", (void *)cur, (void *)writes);
//if (cur != NULL) fprintf(stderr, "%p\n", (void *)cur->next);
//else fprintf(stderr, "NULL\n");
		if (cur == NULL) {
//fprintf(stderr, "cur is NULL\n");
			break;
		}
		if (cur->block == blocknum) {
			errno = 0;
//fprintf(stderr, "writing block %d length %d\n", blocknum, cur->length);
			fwrite(cur->data, cur->length, 1, files.out);
			if (unlikely(errno != 0 || ferror(files.out))) return -1;
			// Remove from list
			if (cur == writes) {
				// Head of list
//fprintf(stderr, "head of list: cur/writes %p", (void *)cur);
//fprintf(stderr, ", data %p", (void *)cur->data);
//fprintf(stderr, ", next %p\n", (void *)cur->next);
				writes = cur->next;
			} else if (cur->next == NULL) {
				// Tail of list
//fprintf(stderr, "tail of list\n");
				if (prev != NULL) prev->next = NULL;
			} else {
				// Not head or tail of list
//fprintf(stderr, "middle of list\n");
				if (prev != NULL) prev->next = cur->next;
			}
			del = cur;
			cur = cur->next;
			free(del->data); free(del);
//fprintf(stderr, "survived list\n");
			blocknum++;
		} else {
//fprintf(stderr, "wrong block: %d != %d\n", cur->block, blocknum);
			prev = cur;
			cur = cur->next;
		}
//fprintf(stderr, "thread_write loop end: cur %p, writes %p, next ", (void *)cur, (void *)writes);
//if (cur != NULL) fprintf(stderr, "%p\n", (void *)cur->next);
//else fprintf(stderr, "NULL\n");
		if (unlikely(file == NULL && cur == NULL && blocknum != blockcnt)) cur = writes;
	}
//fprintf(stderr, "Returning from thread_write\n");
	return blocknum;
}
#endif /* THREADED */


int main(int argc, char **argv)
{
	static unsigned char blk[LZJODY_BSIZE + 4];
	static unsigned char out[LZJODY_BSIZE + 4];
	int i;
	int length = 0;	/* Incoming data block length counter */
	int c_length;   /* Compressed block length temp variable */
	int blocknum = 0;	/* Current block number */
	unsigned char options = 0;	/* Compressor options */
#ifdef THREADED
	struct thread_info *thrs; /* Thread states */
	unsigned char *in_blks;	/* Thread data blocks */
	int nprocs = 1;		/* Number of processors */
	int t_open;	/* Number of available threads */
	int eof = 0;	/* End of file? */
#endif /* THREADED */

	if (argc < 2) goto usage;

	/* Windows requires that data streams be put into binary mode */
#ifdef ON_WINDOWS
	setmode(STDIN_FILENO, _O_BINARY);
	setmode(STDOUT_FILENO, _O_BINARY);
#endif /* ON_WINDOWS */

	files.in = stdin;
	files.out = stdout;

	if (!strncmp(argv[1], "-c", 2)) {
#ifndef THREADED
		/* Non-threaded compression */
		/* fprintf(stderr, "blk %p, blkend %p, files %p\n",
				blk, blk + LZJODY_BSIZE - 1, files); */
		errno = 0;
		while ((length = fread(blk, 1, LZJODY_BSIZE, files.in))) {
			if (ferror(files.in)) goto error_read;
			DLOG("\n--- Compressing block %d (%d bytes)\n", length, blocknum);
			i = lzjody_compress(blk, out, options, length);
			if (i < 0) goto error_compression;
			DLOG("c_size %d bytes\n", i);
			i = fwrite(out, i, 1, files.out);
			if (unlikely(!i)) goto error_write;
			blocknum++;
			errno = 0;
		}

#else /* Using POSIX threads */

 #ifdef _SC_NPROCESSORS_ONLN
		/* Get number of online processors for pthreads */
		nprocs = (int)sysconf(_SC_NPROCESSORS_ONLN);
		if (nprocs < 1) {
			fprintf(stderr, "warning: system returned bad number of processors: %d\n", nprocs);
			nprocs = 1;
		}
 #endif /* _SC_NPROCESSORS_ONLN */
		/* Run two threads per processor */
		nprocs <<= 1;
		fprintf(stderr, "lzjody: compressing with %d worker threads\n", nprocs);

		/* Allocate per-thread input/output memory and control blocks */
		thrs = (struct thread_info *)calloc(nprocs, sizeof(struct thread_info));
		if (thrs == NULL) goto oom;
		in_blks = (unsigned char *)malloc((LZJODY_BSIZE + 4) * nprocs);
		if (in_blks == NULL) goto oom;

		/* Assign compressor input blocks */
		for (i = 0; i < nprocs; i++) {
			(thrs + i)->in = in_blks + ((LZJODY_BSIZE + 4) * i);
		};

		thread_error = 0;
		while (1) {
			struct thread_info *cur;

			// Reap threads, get open thread count
			t_open = 0;
			pthread_mutex_lock(&mtx);
			for (i = 0; i < nprocs; i++) {
				cur = thrs + i;
				if (cur->working == 0) {
//fprintf(stderr, "Open thread %d\n", i);
					t_open++;
				}
				if (cur->working == -1) {
					// Reap thread
//fprintf(stderr, "  Reap thread %d blk %2d: %4d bytes\n", i, cur->block, cur->out_length);
					if (unlikely(thread_write_and_free(cur) < 0)) goto error_write;
//fprintf(stderr, "Detaching thread...\n");
					pthread_detach(cur->id);
					cur->working = 0;
					t_open++;
//fprintf(stderr, "Detach done\n");
				}
			}
//fprintf(stderr, "Open threads: %d\n", t_open);

			// If no threads are open and not EOF then wait for one
			// If EOF then wait for a thread too
			if (t_open == 0 || unlikely(t_open != nprocs && eof == 1)) {
				pthread_cond_wait(&thread_change, &mtx);
				pthread_mutex_unlock(&mtx);
				continue;
			}

			if (eof == 1 && t_open == nprocs) break;

			// Start threads
			if (eof == 0) for (i = 0; i < nprocs; i++) {
				cur = thrs + i;
				if (cur->working == 0) {
					cur->out = (unsigned char *)malloc(LZJODY_BSIZE + 8);
					errno = 0;
					cur->in_length = fread(cur->in, 1, LZJODY_BSIZE, files.in);
					if (unlikely(errno != 0 || ferror(files.in))) goto error_read;
					if (feof(files.in)) eof = 1;
					if (cur->in_length == 0) break;
//fprintf(stderr, "Create thread %d blk %2d: %4d bytes\n", i, blocknum, cur->in_length);
					cur->block = blocknum;
					blocknum++;
					cur->working = 1;
					pthread_create(&(cur->id), NULL, compress_thread, (void *)cur);
				}
				if (eof == 1) break;
			}
			pthread_mutex_unlock(&mtx);
		}
		pthread_mutex_unlock(&mtx);
		if (unlikely(thread_write_and_free(NULL) < 0)) goto error_write;
		free(thrs); free(in_blks);
//fprintf(stderr, "Normal exit\n");
#endif /* THREADED */
	}

	/* Decompress */
	if (!strncmp(argv[1], "-d", 2)) {
		errno = 0;
		while(fread(blk, 1, 2, files.in)) {
			/* Get block-level decompression options */
			options = *blk & 0xc0;

			/* Read the length of the compressed data */
			length = *(blk + 1);
			length |= ((*blk & 0x1f) << 8);
			if (length > (LZJODY_BSIZE + 4)) goto error_blocksize_d_prefix;

			i = fread(blk, 1, length, files.in);
			if (ferror(files.in)) goto error_read;
			if (i != length) goto error_shortread;

			if (options & O_NOCOMPRESS) {
				c_length = *(blk + 1);
				c_length |= ((*blk & 0x1f) << 8);
				DLOG("--- Writing uncompressed block %d (%d bytes)\n", blocknum, c_length);
				if (c_length > LZJODY_BSIZE) goto error_unc_length;
				i = fwrite((blk + 2), 1, c_length, files.out);
				if (i != c_length) {
					length = c_length;
					goto error_write;
				}
			} else {
				DLOG("--- Decompressing block %d\n", blocknum);
				length = lzjody_decompress(blk, out, i, options);
				if (length < 0) goto error_decompress;
				if (length > LZJODY_BSIZE) goto error_blocksize_decomp;
				i = fwrite(out, 1, length, files.out);
				if (i != length) goto error_write;
 /*			     DLOG("Wrote %d bytes\n", i); */
			}

			blocknum++;
			errno = 0;
		}
	}

	exit(EXIT_SUCCESS);

error_compression:
	fprintf(stderr, "Fatal error during compression, aborting.\n");
	exit(EXIT_FAILURE);
error_read:
	fprintf(stderr, "Error reading file '%s': %s\n", "stdin", strerror(errno));
	exit(EXIT_FAILURE);
error_write:
	fprintf(stderr, "Error writing file %s\n", "stdout");
	exit(EXIT_FAILURE);
error_shortread:
	fprintf(stderr, "Error: short read: %d < %d (eof %d, error %d)\n",
			i, length, feof(files.in), ferror(files.in));
	exit(EXIT_FAILURE);
error_unc_length:
	fprintf(stderr, "Error: uncompressed length too large (%d > %d)\n",
			c_length, LZJODY_BSIZE);
	exit(EXIT_FAILURE);
error_blocksize_d_prefix:
	fprintf(stderr, "Error: decompressor prefix too large (%d > %d)\n",
			length, (LZJODY_BSIZE + 4));
	exit(EXIT_FAILURE);
error_blocksize_decomp:
	fprintf(stderr, "Error: decompressor overflow (%d > %d)\n",
			length, LZJODY_BSIZE);
	exit(EXIT_FAILURE);
error_decompress:
	fprintf(stderr, "Error: cannot decompress block %d\n", blocknum);
	exit(EXIT_FAILURE);
#ifdef THREADED
oom:
	fprintf(stderr, "Error: out of memory\n");
	exit(EXIT_FAILURE);
#endif
usage:
	fprintf(stderr, "lzjody %s, a compression utility by Jody Bruchon (%s)%s\n",
			LZJODY_UTIL_VER, LZJODY_UTIL_VERDATE,
#ifdef THREADED
			" threading enabled"
#else
			""
#endif
			);
	fprintf(stderr, "\nlzjody -c   compress stdin to stdout\n");
	fprintf(stderr, "\nlzjody -d   decompress stdin to stdout\n");
	exit(EXIT_FAILURE);
}
