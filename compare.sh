#!/bin/bash

# Spacing for printout
SPC1=30; SPC2=10

[ ! -z "$1" ] && FILE="$1"
[ -z "$FILE" ] && FILE=test.input
[ ! -e "$FILE" ] && echo "Bad file" && exit 1
[ ! -x ./lzjody.static ] && make -j

BEST=99999999999
FSZ="$(stat -c '%s' "$FILE")"

echo -e "Source file: '$FILE'\n"
printf "%${SPC1}s: %${SPC2}s\n" "Source file size" "$FSZ"
for X in './lzjody.static -c' 'gzip -9c' 'lzop -9c' 'xz -ec' 'bzip2 -9c'
	do
	P=""; printf "%${SPC1}s: " "$P$X"
	SZ="$($X < $FILE | wc -c)"
	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

	P="bpxfrm => "; printf "%${SPC1}s: " "$P$X"
	SZ="$(./bpxfrm f $FILE - | $X | wc -c)"
	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

	P="diffxfrm => "; printf "%${SPC1}s: " "$P$X"
	SZ="$(./diffxfrm -c < $FILE | $X | wc -c)"
	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"

	P="bp+diff => "; printf "%${SPC1}s: " "$P$X"
	SZ="$(./bpxfrm f $FILE - | ./diffxfrm -c - | $X | wc -c)"
	printf "%${SPC2}s%4s%%\n" "$SZ" "$(expr $SZ \* 100 / $FSZ)"
	[ $SZ -lt $BEST ] && BP="$P$X" && BEST="$SZ"
done

echo -e "\nBest algorithm: $P$X with ratio of $(expr $SZ \* 100 / $FSZ)%"
