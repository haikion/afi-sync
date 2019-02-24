#!/bin/bash

export MODS_DIR=~/afisync-tests/mods
export WORKING_DIR=~/afisync-tests/work
export TESTS_DIR=$PWD

run_test () {
    timeout 15 ${TESTS_DIR}/$1 && echo -e "\e[32m$1 passed\e[0m" || echo -e "\e[31m$1 failed\e[0m"
    killall AFISync
}

run_negative_test () {
    timeout 15 ${TESTS_DIR}/$1 && echo -e "\e[31m$1 failed\e[0m"  || echo -e "\e[32m$1 passed\e[0m"
    killall AFISync
}

init () {
    sudo service apache2 start

    trash ${WORKING_DIR}
    mkdir ${WORKING_DIR}
    cp -R ../../* ${WORKING_DIR}/
    cd ${WORKING_DIR}
    qmake && make
}

init

run_test log-rotate.sh
run_test repositories-update.sh
run_negative_test repositories-update-corrupted.sh

sudo service apache2 stop
echo -e "\e[0mAll tests run"
