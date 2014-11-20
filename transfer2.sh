#!/bin/bash
SOURCE=$1
DESTINATION=$2

ssh cody@192.168.1.167 "ls $SOURCE" > one.txt
ls $DESTINATION > two.txt
myArr=($(diff one.txt two.txt | awk '{if($1~"<") print $2}'))
ssh cody@192.168.1.167 "mkdir $SOURCE/transfer"
for i in "${myArr[@]}"
do
   ssh cody@192.168.1.167 "cd $SOURCE && cp $i ./transfer && echo $i"
done
ssh cody@192.168.1.167 "cd $SOURCE && tar -zcvf transfer.tar.gz transfer"
ssh cody@192.168.1.167 "scp $SOURCE/transfer.tar.gz cody@192.168.1.32:$DESTINATION"
tar -xf $DESTINATION/transfer.tar.gz
ssh cody@192.168.1.167 "cd $SOURCE; rm transfer.tar.gz; rm -r transfer"
rm $DESTINATION/transfer.tar.gz
for i in "${myArr[@]}"
do
   mv $DESTINATION/transfer/$i $DESTINATION
done
rmdir $DESTINATION/transfer
rm one.txt
rm two.txt 
