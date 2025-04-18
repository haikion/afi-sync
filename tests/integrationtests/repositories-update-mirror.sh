#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories1mod.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-cba-2.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

mkdir -p settings/AFISync
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
cp ${INI_FILE} settings/AFISync/AFISync.ini
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

echo "Running mirror ptr"
./AFISync --mirror ${WORKING_DIR} > afisync.log 2>&1 &
sleep 3

echo "Waiting for all mods to be mirrored ..."
while ! grep "All mods downloaded. Mirroring delta patches" afisync.log; do
    sleep 1
done
echo "All mods mirrored, updating repositories.json ..."
cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
while [ ! -f "afisync_patches/@dummy.0.5c668c74b841d239fb6418e978a07fa5.7z" ] && ps -A | grep AFISync ; do
    sleep 1
done
echo "repositories.json updated"

kill_and_wait

if [ -f core* ]; then
    echo -e "\e[31m$1Core file detected\e[0m"
    exit 1
fi
exit 0
