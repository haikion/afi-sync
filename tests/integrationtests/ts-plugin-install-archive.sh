#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-tfar-2.json

trash @tfar_beta
trash config/plugins
trash config/addons.ini
trash settings

mkdir -p ${WORKING_DIR}/config
cp ${TESTS_DIR}/files/addons.ini config/
cp -R ${TESTS_DIR}/files/ver2/mods/\@task_force_radio .
cp -R ${TESTS_DIR}/files/settings .
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json

killall xvfb-run
xvfb-run --auto-servernum --server-num=1 ./AFISync &
counter=0
while [ ! -f config/plugins/TFAR_win64.dll ] && [ $counter -lt 5 ]; do
   sleep 2
   counter=$((counter+1))
done
grep -RIi remote_start.wav config/addons.ini || exit 1

kill_and_wait
if [ -f core* ]; then
   echo -e "\e[31m$1Core file detected\e[0m"
   exit 1
fi
exit 0
