#!/bin/bash

WORKING_DIR=~/afisync-tests/work
CURRENT_REPOSITORIES_JSON=$(pwd)/files/repositories.json
UPDATED_REPOSITORIES_JSON=$(pwd)/files/repositories-updated.json
UPDATED_HASH=$(cat $UPDATED_REPOSITORIES_JSON)

./init.sh
cd $WORKING_DIR
cp $CURRENT_REPOSITORIES_JSON settings/repositories.json
cp $UPDATED_REPOSITORIES_JSON /var/www/html/afisync-tests/repositories.json

./AFISync &

hash=$(cat settings/repositories.json)
while [ "$hash" != "$UPDATED_HASH" ]; do
    hash=$(cat settings/repositories.json)
    sleep 2
done
killall AFISync
exit 0
