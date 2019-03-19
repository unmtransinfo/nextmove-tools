#!/bin/bash
#
PROG=$(basename $0)
#
if [ $# -lt 1 ]; then
	echo "Syntax: $PROG ZIPFILEDIR"
	exit
fi
#
ZIPDIR=$1
#
n_zip="0"
n_xml="0"
#
for fzip in $(ls $ZIPDIR/*.zip); do
	n_zip=$(($n_zip + 1))
	n_xml_this=$(zipinfo $fzip |grep '.xml$' |wc -l)
	printf "%d. %s: %d XMLs\n" $n_zip $(basename $fzip) $n_xml_this
	n_xml=$(($n_xml + $n_xml_this))
done
#
printf "%s: n_zip = %d\n" $PROG $n_zip
printf "%s: n_xml = %d\n" $PROG $n_xml
