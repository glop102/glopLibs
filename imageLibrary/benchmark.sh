#!/bin/bash

location="/media/RAID/Documents/Pictures/Wallpapers"
files=$(find /media/RAID/Documents/Pictures/Wallpapers | grep -e ".jpg" -e ".png")
#ls $location | wc -l

IFS=$'\n' # make bash only split things on newlines
for x in $files
do
	./main "$x" 2>/dev/null
	true # just some command so i can comment out the line above and not have the syntax break
done;
