#!/bin/bash

source "${TESTS_DIR}/functions.sh"

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-cba-2.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

mkdir -p settings/AFISync
cp "${CURRENT_REPOSITORIES_JSON}" settings/repositories.json
cp "${INI_FILE}" settings/AFISync.ini
cp "${CURRENT_REPOSITORIES_JSON}" /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

./AFISync --mirror ${WORKING_DIR} &
sleep 1

counter=0
while [ ! -f "afisync_patches/@dummy.0.5c668c74b841d239fb6418e978a07fa5.7z" ] && ps -A | grep AFISync ; do
    sleep 1
    ((counter++))
    if [ $counter -ge 5 ]; then
        echo "Timeout while waiting for the patch to download"
        exit 1
    fi
done

kill_and_wait

if [ -f core* ]; then
    echo "Core file detected"
    exit 1
fi
exit 0
