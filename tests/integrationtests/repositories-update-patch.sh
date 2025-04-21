#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-cba-1.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-cba-2.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

trash "${WORKING_DIR}/@cba_a3"
trash settings
trash afisync.log
mkdir -p settings/AFISync
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
cp ${INI_FILE} settings/AFISync.ini
cp ${CURRENT_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
cp -R ${MODS_DIR}/* .

xvfb-run --auto-servernum --server-num=1 ./AFISync &

echo "Waiting for first version of cba_a3 to be synced ..."
while ! grep "cba_a3 synced" afisync.log; do
    sleep 1
done
echo "First version of cba_a3 completed"

cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json
while ! grep "Delta patching successful" afisync.log; do
    sleep 1
done
kill_and_wait

if [ -f core* ]; then
    echo "Core file detected"
    exit 1
fi
exit 0
