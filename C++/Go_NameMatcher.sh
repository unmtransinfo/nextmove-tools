#!/bin/sh
#
NEXTMOVE_ROOT=/home/app/nextmove
CAFFIX_COMPILE=$NEXTMOVE_ROOT/CaffeineFix/utils/compile
CAFFIX_DECOMPILE=$NEXTMOVE_ROOT/CaffeineFix/utils/decompile
#
#
cat data/drug_synonyms.tab \
	|sed -e '1d' \
	|awk '{print $2}' \
	|perl -pe 's/[^[:ascii:]]//g;' \
	>data/drug_synonyms.txt
#
# Compile name list into FSMType data structure:
$CAFFIX_COMPILE \
	-l data/drug_synonyms.txt \
	data/drug_synonyms.cfx
#
$CAFFIX_DECOMPILE \
	data/drug_synonyms.cfx \
	data/drug_synonyms.h
#
make clean
make fuzzymatch_molnames
#
#
./fuzzymatch_molnames
#
./fuzzymatch_molnames \
	-in data/small_molecule.sdf \
	-out data/z.sdf \
	-sdtags 'SYNONYMS,BRANDS' \
	-vv \
	>& fuzzymatch_molnames.log
#
