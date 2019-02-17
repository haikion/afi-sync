run_test () {
    timeout 15 ./$1 && echo -e "\e[32m$1 passed\e[0m" || echo -e "\e[31m$1 failed\e[0m"
    killall AFISync
}

run_test log-rotate.sh
run_test repositories-update.sh
echo -e "\e[0mAll tests run"
