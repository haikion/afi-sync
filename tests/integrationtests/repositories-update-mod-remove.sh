#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories2mods.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories1mod.json
SYNC_DIR=${TESTS_DIR}/files/sync
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

trash settings
cp -r ${TESTS_DIR}/files/settings settings
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

xvfb-run --auto-servernum --server-num=1 ./AFISync &
sleep 3

cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json

while ! grep "Removing http://localhost/afisync-tests/torrents/@afi_editor_enhancements_5.torrent from sync" afisync.log; do
   sleep 1
done
killall AFISync
# Verify that torrent settings were deleted
while [[ $(ls settings/sync | wc -w) != 4 ]]; do
    sleep 1
done

kill_and_wait

if [ -f core* ]; then
    echo "Core file detected"
    exit 1
fi
exit 0
