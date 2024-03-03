#!/bin/bash

source ${TESTS_DIR}/functions.sh

CURRENT_REPOSITORIES_JSON=${TESTS_DIR}/files/repositories-tfar-1.json

trash config/plugins
trash config/addons.ini
trash settings

mkdir -p ${WORKING_DIR}/config
cp ${TESTS_DIR}/files/addons.ini config/
cp -R ${TESTS_DIR}/files/mods/\@tfar_beta .
cp -R ${TESTS_DIR}/files/settings .
cp ${CURRENT_REPOSITORIES_JSON} settings/repositories.json

killall xvfb-run
xvfb-run --auto-servernum --server-num=1 ./AFISync &
while [ ! -f config/plugins/TFAR_win64.dll ]; do
   sleep 2
done
# TODO: Test addons.ini

kill_and_wait
if [ -f core* ]; then
   echo -e "\e[31m$1Core file detected\e[0m"
   exit 1
fi
exit 0
