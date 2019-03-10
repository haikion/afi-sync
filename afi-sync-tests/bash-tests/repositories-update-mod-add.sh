#!/bin/bash

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories1mod.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories2mods.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
mkdir settings
mkdir settings/AFISync
cp ${INI_FILE} settings/AFISync/AFISync.ini
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

./AFISync &
sleep 3

cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
while ! grep "Adding http://localhost/afisync-tests/torrents/@afi_editor_enhancements_5.torrent to sync" afisync.log; do
    sleep 1
done

killall AFISync
# Verify that torrent settings were added
while [[ $(ls settings/sync | wc -w) != 7 ]]; do
    sleep 1
done
exit 0
