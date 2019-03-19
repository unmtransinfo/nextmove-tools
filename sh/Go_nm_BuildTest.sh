#!/bin/sh
#############################################################################
### - Compile LeadMine dictionary from MeSH disease terms.
### - Create LeadMine configuration (CFG) file.
### - Run LeadMine on drug indications from DailyMed via DrugCentral.
#############################################################################
#set -x
#
cwd=$(pwd)
#
NM_ROOT="/home/app/nextmove"
#
COMPILE_JAR=${NM_ROOT}/leadmine-3.12/DictionaryBuilding/CompileCfx/compilecfx-3.12.jar
#
DATADIR="$cwd/data"
#
SRCDICT="/home2/jjyang/projects/drugcentral/data/mesh_disease_lower.txt"
DICTNAME="meshdisease"
#
printf "Source dictionary: %s\n" $(basename $SRCDICT)
#
CFXFILE="$DATADIR/${DICTNAME}.cfx"
#
printf "%s terms: %d\n" $(basename $SRCDICT) $(cat $SRCDICT |wc -l)
#
echo "Compiling LeadMine dictionary..."
#
java -jar $COMPILE_JAR -i $SRCDICT $CFXFILE
#
###
#
LEADMINE_JAR="${NM_ROOT}/leadmine-3.12/LeadMine/leadmine-3.12.jar"
#
CORPUSNAME="indications"
#
cfgfile="${DATADIR}/${DICTNAME}_config.cfg"
(cat <<__EOF__
#Disease terms
[dictionary]
  location  $CFXFILE
  entityType  disease
  useSpellingCorrection  true
  minimumCorrectedEntityLength 7
  maxCorrectionDistance 1
__EOF__
) \
	>$cfgfile
#
csv_utils.py \
	--i ~/projects/drugcentral/data/label_indications.csv \
	--coltag "TEXT" --extractcol \
	>$DATADIR/label_indications.txt
#
###
#
nthreads="4"
#
echo "LeadMining..."
#
java -jar $LEADMINE_JAR \
	-c $cfgfile -t $nthreads \
	-tsv \
	$DATADIR/label_indications.txt \
	>data/${DICTNAME}-${CORPUSNAME}_test.tsv
#
#
