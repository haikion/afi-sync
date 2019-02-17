#!/bin/bash

WORKING_DIR=~/afisync-tests/work

./init.sh
cd $WORKING_DIR

trash *7z
touch afisync.log
./AFISync &
while [ ! -f *7z ]; do
   sleep 2
done
killall AFISync
exit 0
