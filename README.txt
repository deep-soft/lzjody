                    LEMPEL-ZIV/JODY BRUCHON COMPRESSION
                    ===================================

     If you have any questions, comments, or patches, send me an email:
                             jody@jodybruchon.com

       This software is released under the terms of The MIT License


GENERAL INFORMATION
-------------------

This code compresses and decompresses a data stream using a combination of
compression techniques that are optimized for compressing disk image data.

Compression methods used by this program include:

* Run-length encoding (RLE), packing long repetitions of a single byte value
  into a short value:length pair

* Lempel-Ziv (dictionary-based) compression with optimized searching

* Sequential increment compression, where 8-, 16-, and 32-bit values that
  are incremented by 1 are converted to a pair consisting of an  inital value
  and a count

* Byte plane transformation, putting bytes at specific intervals together to
  allow compression of some forms of otherwise incompressible data. This is
  performed on otherwise incompressible data to see if it can be arranged
  differently to produce a compressible pattern.

The included compression utility can use POSIX threads; to use threaded mode,
build like this:

make THREADED=1

You can also use DEBUG=1 to turn on some very annoying debugging messages.

The lzjody library accepts blocks for compression up to 4096 bytes in size and
is designed to guarantee no more than four bytes of data expansion for a
block that is 100% incompressible. The compress/decompress functions return
the number of bytes that are output by the function. This return value can
be examined by the calling application and used to determine if it will be
better to store the data uncompressed with an "out-of-band" indicator that
the block is stored raw instead of in the lzjody compressed format.


COMPRESSED DATA FORMAT
----------------------

All 12-bit lengths are stored in big-endian format. This allows the packing
of the most common compression flags into the upper 4 bits when a 12-bit
length or offset is stored.

The first 2 bytes of a compressed block are the 12-bit length of all of the
compressed data. All data following these bytes are sub-blocks of data
prefixed with a compression command, a command-dependent set of bytes of
metadata, and the compressed data to be processed by the decompressor.

The first byte of every sub-block always contains a compression command
Bit 0x80 is a flag that indicates whether the command is stored in a short
form. Bit 0x10 indicates a long LZ match that requires an additional byte
of storage. Bits 0x20 and 0x40 select between LZ, RLE, and sequential
increment compression. If 0x40, 0x20, and 0x10 are all zero, the lower 4
bits are an "extended" command; the short form flag 0x80 can apply to
extended commands. Refer to lzjody.h for the exact mapping of bits to
algorithms and modifiers.

For "standard" commands, the short forms store a 4-bit length or offset in
the lower 4 bits of the command byte. "Normal" (not short) forms store a
12-bit value instead, with the top 4 bits packed into the bottom 4 bits of
the command byte. If the "LZ long" flag is set, the LZ compressor stored
more than 255 bytes and the length value is an extra byte in length to
accommodate the 12-bit length.

For "extended" commands, the short forms have a slightly different meaning.
Since the lower 4 bits of the command byte contain the extended command,
the short form of an extended command indicates a one-byte offset instead
of a two-byte (12-bit) offset.


LEMPEL-ZIV COMPRESSION
----------------------

LZ compression uses a "dictionary" of previously encountered data to reduce
later occurrences of the same data to (offset,length) references. In this
implementation the dictionary scanning performance is greatly increased by
using a "jump list" of offsets for each byte. The data block is scanned by
an indexer before LZ compression starts to make these lists and the scanner
uses the byte value itself to find the correct list. If a particular byte
results in a list that is very long (the exact threshold for which is in the
lzjody.h header and was chosen through performance profiling) then the LZ
compressor will fall back to the byte-by-byte linear scanner. This is done
because following the jump list entries is more expensive than scanning all
bytes one by one when too many bytes are of the value being scanned for.

The LZ algorithm also performs "fast rejection" checks that prevent entry
into a full LZ scan loop if the last byte of the minimum match length does
not match. This check results in a significant increase in performance.


RUN-LENGTH ENCODING
-------------------

RLE detects long runs of identical bytes and replaces them with a simple
(value,length) pair. There is very little to say about this algorithm; it
is very simple in both concept and implementation.


INCREMENTAL SEQUENCE COMPRESSION
--------------------------------

Incremental sequences are runs of values that increase by 1 for every
successive value. lzjody scans for sequences of 8, 16, and 32 bit widths and
reduces them to (start,count) pairs. For example, Seq(16) compression on the
following 8-byte data stream:

0x2201, 0x2202, 0x2203, 0x2204 -> (Start at 0x2201, output 4 values)

The 8 bytes would be reduced to 4 bytes: the compression command, a byte-wide
value count, and the initial 16-bit value.


BYTE PLANE TRANSFORMATION
-------------------------

While developing this compression algorithm and examining a data block
which could not be compressed, it was observed in the hex editor that the
data contained easily compressed patterns in the _columns_ even though the
rows could not be compressed. The data could be easily compressed if the
existing algorithms could somehow read the data "vertically." This is how the
idea for "byte plane transformation" was conceived. The name comes from the
way that VGA controllers store video data, where four "planes" of bits are
combined to obtain the 4-bit color value for a pixel.

The best way to see how this works is with some diagrams. Take this data
stream, for example:

                 Input stream position
 00 01 02 03   04 05 06 07   08 09 0a 0b   0c 0d 0e 0f
+--+--+--+--+ +--+--+--+--+ +--+--+--+--+ +--+--+--+--+
|0a|ff|80|c9| |0b|ff|80|c9| |0c|ff|80|c9| |0d|ff|80|c9|
+--+--+--+--+ +--+--+--+--+ +--+--+--+--+ +--+--+--+--+

After a 4-byte plane transformation, the data stream consists of the data as
if it was arranged "vertically." Look at the patterns that emerge this way:

        Plane
      00 01 02 03
  --++--+--+--+--+
D 00||0a|ff|80|c9|
A 01||0b|ff|80|c9|
T 02||0c|ff|80|c9|
A 03||0d|ff|80|c9|
  --++--+--+--+--+
       |  |  |  \--- RLE (4)
       |  |  |
       |  |  \------ RLE (4)
       |  |
       |  \--------- RLE (4)
       |
       \------------ Seq(8) (4)

The output from the 4-byte plane transform on this data would be this stream:

                 Output stream position
 00 01 02 03   04 05 06 07   08 09 0a 0b   0c 0d 0e 0f
+--+--+--+--+ +--+--+--+--+ +--+--+--+--+ +--+--+--+--+
|0a|0b|0c|0d| |ff|ff|ff|ff| |80|80|80|80| |c9|c9|c9|c9|
+--+--+--+--+ +--+--+--+--+ +--+--+--+--+ +--+--+--+--+

The result is a data stream that is now compressible for minimal extra cost.


A NOTE OF CAUTION
-----------------

This code has ONLY been tested on Intel i386 and x86_64 Linux platforms.

Endianness or unaligned access issues are likely to be present if run on
other platforms. Please submit patches to me if you have such a platform and
can help fix the bugs without otherwise changing the behavior of the program.


MISCELLANY
----------

lzjody was initially called "lzjb" and the name was changed to avoid confusion
with an existing compression algorithm by the same name that is used in the
ZFS filesystem.
