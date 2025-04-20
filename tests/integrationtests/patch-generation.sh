#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories.json

cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json

trash new old out
mkdir new old out

cp -R ${TESTS_DIR}/files/ver4/mods/@cba_a3 old/
cp -R ${TESTS_DIR}/files/ver5/mods/@cba_a3 new/

trash *7z
touch afisync.log
./AFISync --old-path old/@cba_a3 --new-path new/@cba_a3 --output-path out &

while [ ! -f out/@cba_a3.0.051e0f09816d2d77492196534b722d4b.7z ]; do
   sleep 2
done

kill_and_wait
if [ -f core* ]; then
   echo -e "\e[31m$1Core file detected\e[0m"
   exit 1
fi

exit 0
