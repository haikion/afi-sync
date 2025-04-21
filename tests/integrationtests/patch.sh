#!/bin/bash

source "${TESTS_DIR}/functions.sh"

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-cba-2.json
INI_FILE=${TESTS_DIR}/files/AFISync-primary-enabled.ini

mkdir -p settings/AFISync
cp "${CURRENT_REPOSITORIES_JSON}" settings/repositories.json
cp "${INI_FILE}" settings/AFISync.ini
cp "${CURRENT_REPOSITORIES_JSON}" /var/www/html/afisync-tests/repositories.json
trash @cba_a3
cp -R ${MODS_DIR}/* .

# echo "Run AFISync now!"
# read
xvfb-run --auto-servernum --server-num=1 ./AFISync &

while ! grep "Delta patching successful" afisync.log; do
    sleep 1
done
kill_and_wait

if [ -f core* ]; then
    echo "Core file detected"
    exit 1
fi
exit 0
