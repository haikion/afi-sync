#!/usr/bin/env bash

kill_and_wait () {
    killall AFISync
    while ps -A | grep AFISync; do
        sleep 1;
    done
}
