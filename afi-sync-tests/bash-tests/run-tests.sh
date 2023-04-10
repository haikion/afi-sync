#!/bin/bash

command -v apache2 > /dev/null  || (echo "No apache2" && exit 1)
command -v trash > /dev/null || (echo "No trash" && exit 1)

export MODS_DIR=~/afisync-tests/mods
export WORKING_DIR=~/afisync-tests/work
export TESTS_DIR=$PWD
export MODS_DIR=${TESTS_DIR}/files/mods
export TORRENTS_DIR=${TESTS_DIR}/files/torrents

run_test () {
    if [ -f core ]; then
        echo -e "\e[31mCore file detected\e[0m"
        exit 1
    fi

    timeout 60 ${TESTS_DIR}/$1 &> ${1}_shell.log && echo -e "\e[32m$1 passed\e[0m" || echo -e "\e[31m$1 failed\e[0m"
    mv afisync.log ${1}_afisync.log
}

run_negative_test () {
    if [ -f core ]; then
        echo -e "\e[31m$1Core file detected\e[0m"
        exit 1
    fi

    timeout 15 ${TESTS_DIR}/$1 && echo -e "\e[31m$1 failed\e[0m"  || echo -e "\e[32m$1 passed\e[0m"
}

compile () {
    trash ${WORKING_DIR}
    mkdir -p ${WORKING_DIR}
    cp -R ../../app/* ${WORKING_DIR}/
    cd ${WORKING_DIR}
    qmake CONFIG+=debug &> compile.log && make -j8 &>> compile.log && return 0
    return 1
}

compileWithMessages() {
    echo -n "Compiling... "
    if compile ; then
        echo -e "${GREEN}success${NC}"
    else
        echo -e "${RED}fail!${NC}"
        echo "See ${WORKING_DIR}/compile.log for details"
        exit 1
    fi
}

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

ulimit -c unlimited
sudo service apache2 start

trash ${WORKING_DIR}/settings
mkdir -p ${WORKING_DIR}/settings/AFISync
rm -rf /var/www/html/afisync-tests/*
sudo mkdir -p /var/www/html/afisync-tests/torrents
user=$(whoami)
sudo chown -R $user /var/www/html/afisync-tests
cp -R ${TESTS_DIR}/files/torrents /var/www/html/afisync-tests/
cp -R ${TESTS_DIR}/files/mods /var/www/html/afisync-tests/
cp -R ${TESTS_DIR}/files/ver2 /var/www/html/afisync-tests/

compileWithMessages
cd ${WORKING_DIR}

run_test log-rotate.sh
run_test repositories-update.sh
run_test repositories-update-mod-add.sh
run_test repositories-update-mod-remove.sh
run_test repositories-update-patch.sh
run_test repositories-update-repo-remove.sh
run_negative_test repositories-update-corrupted.sh

sudo service apache2 stop
echo -e "\e[0mAll tests run"
