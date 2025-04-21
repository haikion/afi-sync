#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories1mod.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories2mods.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

mkdir -p settings/AFISync
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
cp ${INI_FILE} settings/AFISync/AFISync.ini
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

xvfb-run --auto-servernum --server-num=1 ./AFISync &
sleep 3

cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
counter=0
while ! grep "Adding http://localhost/afisync-tests/torrents/@afi_editor_enhancements_5.torrent to sync" afisync.log; do
    sleep 1
    ((counter++))
    if [ $counter -ge 50 ]; then
        echo "Timeout while waiting for mod addition"
        exit 1
    fi
done

kill_and_wait

if [ -f core* ]; then
    echo "Core file detected"
    exit 1
fi
if [ $counter -ge 50 ]; then
    echo "Timeout waiting for mod addition"
    exit 1
fi

exit 0
