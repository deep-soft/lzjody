/*
 * Lempel-Ziv-JodyBruchon compression library
 *
 * Copyright (C) 2014-2020 by Jody Bruchon <jody@jodybruchon.com>
 * See lzjody.c for license information.
 */

#ifndef LZJODY_H
#define LZJODY_H

#ifdef __cplusplus
extern "C" {
#endif

#define LZJODY_VER "0.4"
#define LZJODY_VERDATE "2023-08-09"

/* Maximum amount of data the algorithm can process at a time */
#define LZJODY_BSIZE 4096

/* Options for the compressor */
#define O_FAST_LZ   0x01	/* Stop at first LZ match (faster but not recommended) */
#define O_NO_LZ     0x02	/* Don't use the LZ compressor */
#define O_NO_SEQ    0x04	/* Don't use the sequence compressors */
#define O_NO_RLE    0x08	/* Don't use the RLE compressor */
#define O_NOPREFIX  0x40	/* Don't prefix lzjody_compress() data with the compressed length */
#define O_REALFLUSH 0x80	/* Make lzjody_flush_literals() flush without question */

/* Decompressor options (some copied from data block header) */
#define O_NOCOMPRESS 0x80	/* Incompressible block packing flag */

extern int lzjody_compress(const unsigned char * const, unsigned char * const,
		const unsigned int, const unsigned int);
extern int lzjody_decompress(const unsigned char * const, unsigned char * const,
		const unsigned int, const unsigned int);

#ifdef __cplusplus
}
#endif

#endif	/* LZJODY_H */
