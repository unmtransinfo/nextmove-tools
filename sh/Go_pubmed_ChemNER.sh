#!/bin/bash
#############################################################################
# https://nextmovesoftware.com/blog/2018/04/09/textmining-pubmed-abstracts-with-leadmine/
#############################################################################
#
wd=$(pwd)
#
date
#
DATADIR_BASELINE="/home/data/PubMed/data/nextmove_baseline"
DATADIR_UPDATE="/home/data/PubMed/data/nextmove_update"
#
###
### CHEMICAL NER:
#
CFGFILE="/home/app/nextmove/leadmine-dictionaries-20180205/default.cfg"
#
NM_ROOT="/home/app/nextmove"
#
LEADMINE_JAR="${NM_ROOT}/leadmine-3.13/bin/leadmine.jar"
#
nthreads="8"
#
echo "LeadMining..."
#
java -jar $LEADMINE_JAR \
	-c $CFGFILE -t $nthreads \
	-tsv -R $DATADIR_BASELINE \
	>$DATADIR_BASELINE/pubmed_baseline_leadmine.tsv
#
date
#
java -jar $LEADMINE_JAR \
	-c $CFGFILE -t $nthreads \
	-tsv -R $DATADIR_UPDATE \
	>$DATADIR_UPDATE/pubmed_update_leadmine.tsv
#
date
#
