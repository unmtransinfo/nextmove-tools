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
COMPILE_JAR=${NM_ROOT}/leadmine-4.2/bin/compilecfx.jar
#
DATADIR="$cwd/data"
#
SRCDICT="$DATADIR/mesh_disease.txt"
DICTNAME="meshdisease"
if [ ! -e $SRCDICT ]; then
	cat $DATADIR/mesh_disease.tsv \
		|sed '1d' \
		|awk -F '\t' '{print $3}' \
		|tr "[:upper:]" "[:lower:]" \
		>$SRCDICT
fi
#
printf "Source dictionary: %s\n" $(basename $SRCDICT)
#
CFXFILE="$DATADIR/${DICTNAME}.cfx"
#
printf "%s terms: %d\n" $(basename $SRCDICT) $(cat $SRCDICT |wc -l)
#
echo "Compiling LeadMine dictionary..."
#
java -jar $COMPILE_JAR -i $SRCDICT -o $CFXFILE
#
###
#
LEADMINE_JAR="${NM_ROOT}/leadmine-4.2/bin/leadmine.jar"
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
psql -P pager=off -qAF $'\t' -h unmtid-dbs.net -p 5433 -d drugcentral -U drugman \
		-c "SELECT DISTINCT concept_name FROM omop_relationship WHERE omop_relationship.relationship_name = 'indication'" \
	|sed -e '1d' \
	|grep -v '^([0-9]* rows)$' \
	|grep -v '^\s*$' \
	>$DATADIR/drugcentral_label_indications.txt
#
###
#
nthreads="4"
#
echo "LeadMining..."
#
java -jar $LEADMINE_JAR \
	-en -c $cfgfile -t $nthreads -tsv \
	$DATADIR/drugcentral_label_indications.txt \
	>$DATADIR/${DICTNAME}-${CORPUSNAME}_test.tsv
#
#
