#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-corrupted.json
UPDATED_HASH=$(cat ${UPDATED_REPOSITORIES_JSON})

cd ${WORKING_DIR}
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json

xvfb-run ./AFISync &

hash=$(cat settings/repositories.json)
while [[ "$hash" != "$UPDATED_HASH" ]]; do
    hash=$(cat settings/repositories.json)
    sleep 2
done
kill_and_wait
exit 1 #Fail
