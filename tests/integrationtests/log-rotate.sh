#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories.json

cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json

trash *7z
touch afisync.log
killall xvfb-run
xvfb-run --auto-servernum --server-num=1 ./AFISync &
while [ ! -f *7z ]; do
   sleep 2
done

kill_and_wait
if [ -f core* ]; then
   echo "Core file detected"
   exit 1
fi
exit 0
