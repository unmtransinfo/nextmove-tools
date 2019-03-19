#!/bin/bash
#############################################################################
# https://nextmovesoftware.com/blog/2018/04/09/textmining-pubmed-abstracts-with-leadmine/
#
# https://www.nlm.nih.gov/databases/download/pubmed_medline.html
# ANNUAL BASELINE (e.g. through 2017):
# ftp://ftp.ncbi.nlm.nih.gov/pubmed/baseline/
# DAILY UPDATE FILES:
# ftp://ftp.ncbi.nlm.nih.gov/pubmed/updatefiles/
#############################################################################
#https://www.nlm.nih.gov/databases/download/pubmed_medline_documentation.html
# "Daily Update Files
# Subsequent to the annual baseline release, NLM produces daily update files. These
# files include new, revised and deleted citations. If you are incorporating these
# files into a local database, load the baseline files first, then load the daily
# update files in numerical order. Revised or deleted citations should replace
# existing citations in your local database. More than one update file may become
# available on the same day."
#############################################################################
#
wd=$(pwd)
#
### DOWNLOAD PUBMED:
cd /home/data/PubMed/data
wget --mirror --accept "*.xml.gz" ftp://ftp.ncbi.nlm.nih.gov/pubmed/baseline/
wget --mirror --accept "*.xml.gz" ftp://ftp.ncbi.nlm.nih.gov/pubmed/updatefiles/
cd ${wd}
###
#
XMLGZDIR_BASELINE="/home/data/PubMed/data/ftp.ncbi.nlm.nih.gov/pubmed/baseline"
n_files=$(ls -1 $XMLGZDIR_BASELINE/*.xml.gz |wc -l)
printf "PubMed BASELINE XMLGZ files: %d\n" "$n_files"
#
XMLGZDIR_UPDATE="/home/data/PubMed/data/ftp.ncbi.nlm.nih.gov/pubmed/updatefiles"
n_files=$(ls -1 $XMLGZDIR_UPDATE/*.xml.gz |wc -l)
printf "PubMed UPDATE XMLGZ files: %d\n" "$n_files"
#
#
#
