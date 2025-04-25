#!/bin/bash

# only works for $1 < 10
set_config() {
    rm -f ~/dfc.conf
    touch ~/dfc.conf
    for i in $(seq 1 $1); do
        echo "server dfs$i 127.0.0.1:1000$i" >> ~/dfc.conf
    done
}

boot_dfs() {
    for i in $(seq 1 $1); do
        mkdir -p dfs$i
        ./dfs dfs$i 1000$i &
    done
}

kill_dfs() {
    pid=$(ps aux | grep "./dfs dfs$1 1000$1" | head -n 1 | awk '{ print $2 }')
    kill "$pid"
}

set_config 4
boot_dfs 4

kill_dfs 1
kill_dfs 2
kill_dfs 3
kill_dfs 4
