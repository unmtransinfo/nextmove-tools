#!/bin/bash
#############################################################################
# https://nextmovesoftware.com/blog/2018/04/09/textmining-pubmed-abstracts-with-leadmine/
# Updates include revised and deleted citations which should overwrite
# baseline records via PMID. Probably this should be done at the PMID.xml
# level but could be done post-NER.
#############################################################################
#
wd=$(pwd)
#
XMLGZDIR_BASELINE="/home/data/PubMed/data/ftp.ncbi.nlm.nih.gov/pubmed/baseline"
printf "PubMed BASELINE XMLGZ files: %d\n" $(ls -1 $XMLGZDIR_BASELINE/*.xml.gz |wc -l)
#
XMLGZDIR_UPDATE="/home/data/PubMed/data/ftp.ncbi.nlm.nih.gov/pubmed/updatefiles"
printf "PubMed UPDATE XMLGZ files: %d\n" $(ls -1 $XMLGZDIR_UPDATE/*.xml.gz |wc -l)
#
#
DATADIR_BASELINE="/home/data/PubMed/data/nextmove_baseline"
DATADIR_UPDATE="/home/data/PubMed/data/nextmove_update"
#
#
### REFORMAT CORPUS:
#
NPROC=8
#
${wd}/python/pubmed_split_abstracts.py $XMLGZDIR_BASELINE $DATADIR_BASELINE $NPROC
${wd}/python/pubmed_split_abstracts.py $XMLGZDIR_UPDATE $DATADIR_UPDATE $NPROC
#
### CORPUS COUNTS:
#
sh/pubmed_zip_counts.sh $DATADIR_BASELINE
sh/pubmed_zip_counts.sh $DATADIR_UPDATE
#
