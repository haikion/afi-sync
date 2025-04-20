#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-tfar-2.json

trash @tfar_beta 2>/dev/null
trash config/plugins 2>/dev/null
trash config/addons.ini 2>/dev/null
trash settings 2>/dev/null

mkdir -p ${WORKING_DIR}/config
cp ${TESTS_DIR}/files/addons.ini config/
cp -R ${TESTS_DIR}/files/ver2/mods/\@task_force_radio .
cp -R ${TESTS_DIR}/files/settings .
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json

killall xvfb-run
xvfb-run --auto-servernum --server-num=1 ./AFISync &
counter=0
while [ ! -f config/plugins/task_force_radio_win64.dll ]; do
   sleep 1
   counter=$((counter+1))
   if [ $counter -ge 5 ]; then
      echo "Timeout while waiting for TFAR plugin to install"
      exit 1
   fi
done
grep -RIi remote_start.wav config/addons.ini || exit 1

kill_and_wait
if [ -f core* ]; then
   echo -e "\e[31m$1Core file detected\e[0m"
   exit 1
fi
exit 0
