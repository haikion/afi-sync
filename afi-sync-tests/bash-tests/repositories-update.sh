#!/bin/bash

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories.json
UPDATED_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-updated.json
UPDATED_HASH=$(cat ${UPDATED_REPOSITORIES_JSON})

cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json
cp ${UPDATED_REPOSITORIES_JSON} /var/www/html/afisync-tests/repositories.json

./AFISync &

hash=$(cat settings/repositories.json)
while [[ "$hash" != "$UPDATED_HASH" ]]; do
    hash=$(cat settings/repositories.json)
    sleep 2
done
killall AFISync
exit 0