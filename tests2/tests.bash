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
    rm -f .log.txt
    touch .log.txt
    for i in $(seq 1 $1); do
        ./dfs dfs$i 1000$i 2>&1 1>>.log.txt &
    done
}

kill_single_dfs() {
    pid=$(ps aux | grep "./dfs dfs$1 1000$1" | head -n 1 | awk '{ print $2 }')
    kill "$pid" 2> /dev/null
}

kill_dfs() {
    for i in $(seq 1 $1); do
        kill_single_dfs $i
    done
}

mk_dfs_dirs() {
    for i in $(seq 1 $1); do
        mkdir -p dfs$i
    done
}

rm_dfs_dirs() {
    for i in $(seq 1 $1); do
        rm -rf dfs$i
    done
    rm -f ~/dfc.conf
}

clear_dfs_dirs() {
    for i in $(seq 1 $1); do
        rm -f dfs$i/*
    done
}

server_count=4
set_config $server_count
boot_dfs $server_count
mk_dfs_dirs $server_count

file_name="1.jpg"
file_path="tests2/img/$file_name"

./dfc put $file_path

kill_single_dfs 1

ls_str=$(./dfc ls)
if [ ! "$ls_str" = "$file_name" ]; then
    echo "failed basic ls with single file: $file_path"
fi

./dfc get $file_name
mv $file_name .temp1
if ! cmp -s .temp1 $file_path; then
    echo "failed basic get with single file: $file_path"
fi
rm .temp1

clear_dfs_dirs $server_count
rm_dfs_dirs $server_count
kill_dfs $server_count
rm .log.txt
