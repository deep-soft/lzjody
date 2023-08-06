#!/bin/bash

clean_exit () {
	rm -f "$TF"
	exit $1
}

TF="$(mktemp)"
COMP=testdata/out.compressed
OUT=testdata/out.final

if [ -z "$LZJODY" ]
	then
	[ ! -z "$WINDIR" ] && EXT=".exe"
	LZJODY=./lzjody$EXT
	test -x lzjody.static$EXT && LZJODY=./lzjody.static$EXT
fi

test ! -x $LZJODY && echo "Compile the program first." && clean_exit 1

# For running e.g. Valgrind
test -z "$1" || LZJODY="$@ $LZJODY"


### Main compression tests

# Stadard data test
CFAIL=0; DFAIL=0
IN=testdata/standard
echo "$LZJODY -c < $IN > $COMP 2>testdata/log.compress1 || CFAIL=1"
$LZJODY -c < $IN > $COMP 2>testdata/log.compress1 || CFAIL=1
[ $CFAIL -eq 0 ] && $LZJODY -d < $COMP > $OUT 2>testdata/log.decompress1 || DFAIL=1
[ $CFAIL -eq 1 ] && echo -e "\nCompressor block test FAILED\n" && clean_exit 1
[ $DFAIL -eq 1 ] && echo -e "\nDecompressor block test FAILED\n" && clean_exit 1
S1="$(sha1sum $IN | cut -d' ' -f1)"; S2="$(sha1sum $OUT | cut -d' ' -f1)"
test "$S1" != "$S2" && echo -e "\nCompressor/decompressor tests FAILED: mismatched hashes\n" && clean_exit 1
echo "Block tests PASSED"

# Incompressible data test
CFAIL=0; DFAIL=0
IN=testdata/cantcompress
$LZJODY -c < $IN > $COMP 2>testdata/log.compress2 || CFAIL=1
[ $CFAIL -eq 0 ] && $LZJODY -d < $COMP > $OUT 2>testdata/log.decompress2 || DFAIL=1
[ $CFAIL -eq 1 ] && echo -e "\nCompressor oversize test FAILED\n" && clean_exit 1
[ $DFAIL -eq 1 ] && echo -e "\nDecompressor oversize test FAILED\n" && clean_exit 1
S1="$(sha1sum $IN | cut -d' ' -f1)"; S2="$(sha1sum $OUT | cut -d' ' -f1)"
test "$S1" != "$S2" && echo -e "\nCompressor/decompressor oversize tests FAILED: mismatched hashes\n" && clean_exit 1
echo "Oversize tests PASSED"


### Decompressor error tests

# Out-of-bounds length tests

: > testdata/log.invalid
echo -n "Testing invalid (large) lengths ";
echo "Large length test:" >> testdata/log.invalid
echo -en '\x10\x05\xaf\xff' > $TF
dd if=/dev/zero bs=4095 count=1 2>/dev/null >> $TF
$LZJODY -d < $TF 2>> testdata/log.invalid && echo "FAILED" && clean_exit 1
echo "PASSED"

echo -n "Testing invalid (zero) lengths ";
echo "Zero length test:" >> testdata/log.invalid
echo -en '\x00\x00\x00' | \
$LZJODY -d 2>> testdata/log.invalid && echo "FAILED" && clean_exit 1
echo "PASSED"

### All tests passed!
echo -e "\nAll compressor/decompressor tests PASSED.\n"
clean_exit
