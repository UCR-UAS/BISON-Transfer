#!/bin/bash
SOURCE=$1
DESTINATION=$2

#Note: IP and login name must be changed for whatever it ends up being used for
#Finds all the differences between the two directories and stores in array
ssh cody@192.168.1.167 "ls $SOURCE" > one.txt
ls $DESTINATION > two.txt
myArr=($(diff one.txt two.txt | awk '{if($1~"<") print $2}'))

#creates dir for the files to be cp into
ssh cody@192.168.1.167 "mkdir $SOURCE/transfer"
for i in "${myArr[@]}"
do
   #copies all files into transfer recursively
   ssh cody@192.168.1.167 "cd $SOURCE && cp $i ./transfer && echo $i"
done

#tar and transfer directory
ssh cody@192.168.1.167 "cd $SOURCE && tar -zcvf transfer.tar.gz transfer"
ssh cody@192.168.1.167 "scp $SOURCE/transfer.tar.gz cody@192.168.1.32:$DESTINATION"

#untar directory and moves it into destination dir
tar -xf $DESTINATION/transfer.tar.gz
rm $DESTINATION/transfer.tar.gz
for i in "${myArr[@]}"
do
   mv $DESTINATION/transfer/$i $DESTINATION
done

#clean up
ssh cody@192.168.1.167 "cd $SOURCE; rm transfer.tar.gz; rm -r transfer"
rmdir $DESTINATION/transfer
rm one.txt
rm two.txt 
