#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-cba-1.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-cba-3.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

trash "${WORKING_DIR}/@cba_a3"
mkdir -p settings/AFISync
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
cp ${INI_FILE} settings/AFISync.ini
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

xvfb-run ./AFISync &

while ! grep "cba_a3_1.torrent Completed" afisync.log; do
    sleep 1
done
echo "First version of cba_a3 completed"

cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
while [ $(grep -i "Delta patching successful" afisync.log  | wc -l) -lt 2 ]; do
    sleep 1
done
kill_and_wait

if [ -f core ]; then
    echo -e "\e[31m$1Core file detected\e[0m"
    exit 1
fi
exit 0
