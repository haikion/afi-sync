#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories2repos.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
cp ${INI_FILE} settings/AFISync/AFISync.ini
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

xvfb-run --auto-servernum --server-num=1 ./AFISync &
sleep 3

cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json

while ! grep "~Repository" afisync.log; do
    sleep 1
done

kill_and_wait

if [ -f core ]; then
    echo -e "\e[31m$1Core file detected\e[0m"
    exit 1
fi
exit 0
