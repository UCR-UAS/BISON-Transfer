#!/bin/bash
SOURCE=$1
DESTINATION=$2
ssh cody@192.168.1.40 "tar -zcvf transfer.tar.gz /$SOURCE"
ssh cody@192.168.1.40 "scp /$SOURCE/transfer.tar.gz cody@192.168.1.32:/$DESTINATION"
