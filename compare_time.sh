#!/bin/bash

# Spacing for printout
SPC1=34; SPC2=8

[ ! -z "$WINDIR" ] && EXT=".exe"
[ ! -z "$1" ] && FILE="$1"
[ -z "$FILE" ] && FILE=test.input
[ ! -e "$FILE" ] && echo "Bad file" && exit 1
[ ! -x ./lzjody.static ] && make -j

BEST=9999999999999
FSZ="$(stat -c '%s' "$FILE")"
[ "$FSZ" = "0" ] && echo "Zero-length file" && exit 1

echo -e "Source file: '$FILE'\n"
printf "%${SPC1}s: %${SPC2}s\n" "Source file size" "$FSZ"
for X in "./lzjody.static$EXT -c" "lzop -1c" "lzop -9c" "gzip -9c" "bzip2 -9c" "xz -ec"
	do
	P=""; printf "%${SPC1}s: " "$P$X"
	D1=$(date +%s%N)
	SZ="$($X < "$FILE" | wc -c)"
	D2=$(date +%s%N)
	T=$((D2 - D1)); T=$((T / 1000000))
	printf "%${SPC2}s ms\n" "$T"
	[ $T -lt $BEST ] && BP="$P$X" && BEST="$T"

	P="bpxfrm => "; printf "%${SPC1}s: " "$P$X"
	D1=$(date +%s%N)
	SZ="$(./bpxfrm$EXT f "$FILE" - | $X | wc -c)"
	D2=$(date +%s%N)
	T=$((D2 - D1)); T=$((T / 1000000))
	printf "%${SPC2}s ms\n" "$T"
	[ $T -lt $BEST ] && BP="$P$X" && BEST="$T"

	P="diffxfrm => "; printf "%${SPC1}s: " "$P$X"
	D1=$(date +%s%N)
	SZ="$(./diffxfrm$EXT -c < "$FILE" | $X | wc -c)"
	D2=$(date +%s%N)
	T=$((D2 - D1)); T=$((T / 1000000))
	printf "%${SPC2}s ms\n" "$T"
	[ $T -lt $BEST ] && BP="$P$X" && BEST="$T"

	P="xorxfrm => "; printf "%${SPC1}s: " "$P$X"
	D1=$(date +%s%N)
	SZ="$(./xorxfrm$EXT -c < "$FILE" | $X | wc -c)"
	D2=$(date +%s%N)
	T=$((D2 - D1)); T=$((T / 1000000))
	printf "%${SPC2}s ms\n" "$T"
	[ $T -lt $BEST ] && BP="$P$X" && BEST="$T"

#	P="bp+diff => "; printf "%${SPC1}s: " "$P$X"
#	SZ="$(./bpxfrm$EXT f "$FILE" - | ./diffxfrm$EXT -c - | $X | wc -c)"
#	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
#	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

#	P="bp+xor => "; printf "%${SPC1}s: " "$P$X"
#	SZ="$(./bpxfrm$EXT f "$FILE" - | ./xorxfrm$EXT -c - | $X | wc -c)"
#	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
#	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

#	P="xor+bp => "; printf "%${SPC1}s: " "$P$X"
#	SZ="$(./xorxfrm$EXT -c < "$FILE" | ./bpxfrm$EXT f - - | $X | wc -c)"
#	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
#	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

#	P="bp+d+x => "; printf "%${SPC1}s: " "$P$X"
#	SZ="$(./bpxfrm$EXT f "$FILE" - | ./diffxfrm$EXT -c | ./xorxfrm$EXT -c - | $X | wc -c)"
#	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
#	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

#	P="bp+x+d => "; printf "%${SPC1}s: " "$P$X"
#	SZ="$(./bpxfrm$EXT f "$FILE" - | ./xorxfrm$EXT -c | ./diffxfrm$EXT -c - | $X | wc -c)"
#	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
#	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

	echo "Best so far: $BP"
done

echo -e "\nBest algorithm: $BP with ratio of $(expr $BEST \* 100 / $FSZ)%"
