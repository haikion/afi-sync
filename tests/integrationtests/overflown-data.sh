#!/bin/bash

source ${TESTS_DIR}/functions.sh

trash settings

cp -R ${TESTS_DIR}/files/settings .
cp ${TESTS_DIR}/files/repositories-cba-1.json settings/repositories.json
cp ${TESTS_DIR}/files/repositories-cba-1.json /var/www/html/afisync-tests/repositories.json

trash @cba_a3
cp -R ${TESTS_DIR}/files/mods/@cba_a3 .

echo "CORRUPTED_DATA" >> @cba_a3/addons/cba_accessory.pbo

killall xvfb-run
xvfb-run --auto-servernum --server-num=1 ./AFISync &

echo "Waiting for PBO file to be repaired..."
max_attempts=10
attempt=0
while [ "$current_checksum" != "073ed40b3d93c4d94f735e95baeceaa5" ]; do
  attempt=$((attempt + 1))
  current_checksum=$(md5sum @cba_a3/addons/cba_accessory.pbo | awk '{print $1}')
  if [ $attempt -gt $max_attempts ]; then
    echo "Timeout"
    exit 1
    break
  fi
  echo "Current checksum: $current_checksum attempt: $attempt/$max_attempts"
  sleep 1
done

kill_and_wait
if [ -f core* ]; then
   echo "Core file detected"
   exit 1
fi
exit 0
