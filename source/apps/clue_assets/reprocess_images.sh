#!/bin/sh

for x in *.png
do
	output=`echo $x | sed -e 's/[.]png/.h/' -e 's/[-]/_/g'`
	../../../tools/png-to-badge-asset 8 $x > $output
done
