#!/bin/bash

export MODS_DIR=~/afisync-tests/mods
export WORKING_DIR=~/afisync-tests/work
export TESTS_DIR=$PWD
export MODS_DIR=${TESTS_DIR}/files/mods

run_test () {
    if [ -f core ]; then
        echo -e "\e[31mCore file detected\e[0m"
        exit 1
    fi

    timeout 60 ${TESTS_DIR}/$1 && echo -e "\e[32m$1 passed\e[0m" || echo -e "\e[31m$1 failed\e[0m"
    mv afisync.log ${1}.log
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
    mkdir ${WORKING_DIR}
    cp -R ../../app/* ${WORKING_DIR}/
    cd ${WORKING_DIR}
    qmake CONFIG+=debug && make -j8
}

ulimit -c unlimited
sudo service apache2 start
compile
cd ${WORKING_DIR}

run_test log-rotate.sh
run_test repositories-update.sh
run_test repositories-update-mod-add.sh
run_test repositories-update-mod-remove.sh
run_test repositories-update-repo-remove.sh
run_negative_test repositories-update-corrupted.sh

sudo service apache2 stop
echo -e "\e[0mAll tests run"
