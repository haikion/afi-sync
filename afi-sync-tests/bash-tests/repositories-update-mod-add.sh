#!/bin/bash

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories1mod.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories2mods.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
mkdir settings
mkdir settings/AFISync
cp ${INI_FILE} settings/AFISync/AFISync.ini
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json

./AFISync &
sleep 5

cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
sleep 40

retVal=1
grep "Adding http://armafinland.fi/afisync/torrents/@afi_128.torrent to sync" afisync.log && retVal=0
killall AFISync
echo ${retVal}
exit ${retVal}
