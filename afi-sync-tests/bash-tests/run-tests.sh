#!/bin/bash

command_exists() {
    local cmd=$1
    if ! command -v "$cmd" > /dev/null; then
        echo "Missing executable: $cmd"
        return 1
    else
        return 0
    fi
}

command_exists apache2 || exit 1
command_exists trash || exit 1
command_exists xvfb-run || exit 1
command_exists 7za || exit 1

export WORKING_DIR=~/afisync-tests/work
export TESTS_DIR=$PWD
export MODS_DIR=${TESTS_DIR}/files/mods
export TORRENTS_DIR=${TESTS_DIR}/files/torrents
export APP_DIR=${TESTS_DIR}/../../app

source ${TESTS_DIR}/functions.sh

run_test () {
    if [ -f core* ]; then
        echo -e "\e[31mCore file detected\e[0m"
        exit 1
    fi

    # ${TESTS_DIR}/$1
    echo -n ${1} ...
    timeout 60 ${TESTS_DIR}/$1 &> ${1}_shell.log && echo -e "\e[32m passed\e[0m" || echo -e "\e[31m failed\e[0m"
    mv afisync.log ${1}_afisync.log 2>/dev/null
}

run_negative_test () {
    if [ -f core* ]; then
        echo -e "\e[31m$1Core file detected\e[0m"
        exit 1
    fi

    echo -n ${1} ...
    timeout 15 ${TESTS_DIR}/$1 &> ${1}_shell.log && echo -e "\e[31m failed\e[0m"  || echo -e "\e[32m passed\e[0m"
    mv afisync.log ${1}_afisync.log
    kill_and_wait >> {1}_shell.log
    if [ -f core* ]; then
        echo -e "\e[31m$1Core file detected\e[0m"
        exit 1
    fi
}

clean () {
    trash ${WORKING_DIR}
    mkdir -p ${WORKING_DIR}
    cp -R ../../app/* ${WORKING_DIR}/
}

compile () {
    # clean
    cd ${WORKING_DIR}
    CMAKE_PREFIX_PATH=~/Qt/6.8.3/gcc_64 cmake ~/code/afi-sync/app -DLIBTORRENT_PATH=~/code/libtorrent &> compile.log && make &>> compile.log && return 0
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
sudo systemctl start apache2

test -f ${WORKING_DIR}/settings && trash ${WORKING_DIR}/settings
mkdir -p ${WORKING_DIR}/settings/AFISync
rm -rf /var/www/html/afisync-tests/*
sudo mkdir -p /var/www/html/afisync-tests/torrents
user=$(whoami)
sudo chown -R $user /var/www/html/afisync-tests
cp -R ${TESTS_DIR}/files/torrents /var/www/html/afisync-tests/
cp -R ${TESTS_DIR}/files/mods /var/www/html/afisync-tests/
cp -R ${TESTS_DIR}/files/afisync_patches /var/www/html/afisync-tests/
cp -R ${TESTS_DIR}/files/ver2 /var/www/html/afisync-tests/
cp -R ${TESTS_DIR}/files/ver3 /var/www/html/afisync-tests/

compileWithMessages
cd ${WORKING_DIR}

run_test log-rotate.sh
run_test patch-generation.sh
run_test patch.sh
run_test ts-plugin-install.sh
run_test repositories-update.sh
run_test repositories-update-chain-patch.sh
run_test repositories-update-mod-add.sh
run_test repositories-update-mod-remove.sh
run_test repositories-update-patch.sh
run_test repositories-update-repo-remove.sh
run_test mirror.sh
run_test repositories-update-mirror.sh
run_test overflown-data.sh
run_negative_test repositories-update-corrupted.sh

sudo service apache2 stop
echo -e "\e[0mAll tests run"
