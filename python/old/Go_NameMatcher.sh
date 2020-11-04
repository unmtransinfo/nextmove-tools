#!/bin/sh
#
NEXTMOVE_ROOT=/home/app/nextmove
CAFFIX_COMPILE=$NEXTMOVE_ROOT/CaffeineFix/utils/compile
CAFFIX_DECOMPILE=$NEXTMOVE_ROOT/CaffeineFix/utils/decompile
#
set -x
#
cat data/drug_synonyms.tab \
	|sed -e '1d' \
	|awk '{print $2}' \
	|perl -pe 's/[^[:ascii:]]//g' \
	>data/drug_synonyms.txt
#
# Compile name list into FSMType data structure:
$CAFFIX_COMPILE \
	-l data/drug_synonyms.txt \
	data/drug_synonyms.cfx
#
$CAFFIX_DECOMPILE \
	-p \
	data/drug_synonyms.cfx \
	drug_synonyms.py
#
echo "Testing..."
#
./Test_DrugSynonym_Match.py
