#!/bin/sh
#############################################################################
# This is the demo by Noel O'Boyle from the blog post:
# https://nextmovesoftware.com/blog/2018/04/09/textmining-pubmed-abstracts-with-leadmine/
#############################################################################
#
NM_ROOT="/home/app/nextmove"
LEADMINE_HOME="${NM_ROOT}/leadmine-3.12"
LEADMINE_DICT_HOME="${NM_ROOT}/leadmine-dictionaries-20180205"
LEADMINE_CFG_FILE="data/disease_trial.cfg"
#
cwd=$(pwd)
#
#
echo 'NCT:? ?\d{8,9}' >${cwd}/data/nct.txt
#
java \
	-jar ${LEADMINE_HOME}/DictionaryBuilding/CompileCfx/compilecfx-3.12.jar \
	-r ${cwd}/data/nct.txt \
	${cwd}/data/nct.cfx
#
###
#
(cat <<__EOF__
[dictionary]
  location  ${LEADMINE_DICT_HOME}/Dictionaries/CFDictDiseaseCs.cfx
  entityType  Disease
  caseSensitive  true
  useSpellingCorrection  false

[dictionary]
  location  ${LEADMINE_DICT_HOME}/Dictionaries/CFDictDisease.cfx
  entityType  Disease
  caseSensitive  false
  useSpellingCorrection  false

[dictionary]
  location  ${cwd}/data/nct.cfx
  entityType  Trial
  caseSensitive  true
  useSpellingCorrection  false

[resolver]
  location ${LEADMINE_DICT_HOME}/Resolvers/diseaseCs.casesensitivedict
  entityType  Disease
  mmap true
  caseSensitive  true

[resolver]
  location ${LEADMINE_DICT_HOME}/Resolvers/disease.dict
  entityType  Disease
  mmap true
  caseSensitive  false
__EOF__
) \
	>${LEADMINE_CFG_FILE}
#
###
OFILE="${cwd}/data/disease_nct_out.txt"
#
#
time java \
	-jar ${LEADMINE_HOME}/LeadMine/leadmine-3.12.jar \
	-c ${LEADMINE_CFG_FILE} \
	-tsv \
	-t 12 \
	-R ${cwd}/data/zipfiles \
	>${OFILE}
#
###
#
csv_utils.py \
	--tsv \
	--i $OFILE \
	--col 5 --colvalcounts
#
