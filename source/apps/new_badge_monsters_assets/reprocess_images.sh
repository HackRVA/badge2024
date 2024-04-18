#!/bin/sh

process_image()
{
	echo ../../../tools/png-to-badge-asset 4 "$1" \> "$2"
	../../../tools/png-to-badge-asset --nostatic 4 "$1" > "$2"
}

output_filename()
{
	# change .png to .h
	echo "$1" | sed -e 's/[.]png$/.h/'
}

# Get a list of all the png files excluding BadgeMonstersTemplate.png
input_files=$(ls -1 *.png | grep -v 'BadgeMonstersTemplate[.]png')

for input in $input_files
do
	output=$(output_filename $input)
	process_image $input $output
done
