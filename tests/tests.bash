#!/bin/bash

# setup
./dfs ./.test_dfs1/ 5000 1> .dfs_log.txt &
dfs_pid=$!

# tests
#
./testcli 5000 na LS > .temp
if [ -s ".temp" ]; then
    echo "list on empty failed"
fi
 
rv=$(./testcli 5000 tests/test_file1.txt TEST)
if [ "$rv" != "n" ]; then
    echo "test_file1.txt TEST before put failed"
fi

rv=$(./testcli 5000 tests/test_file2.jpg TEST)
if [ "$rv" != "n" ]; then
    echo "tests/test_file2.jpg TEST before put failed"
fi

./testcli 5000 tests/test_file1.txt PUT
rv=$(./testcli 5000 tests/test_file1.txt TEST)
if [ "$rv" != "y" ]; then
    echo "test_file1.txt TEST after put failed"
fi

./testcli 5000 tests/test_file2.jpg PUT
rv=$(./testcli 5000 tests/test_file2.jpg TEST)
if [ "$rv" != "y" ]; then
    echo "test_file2.jpg TEST after put failed"
fi

./testcli 5000 tests/test_file1.txt GET > .temp
if ! cmp -s .temp tests/test_file1.txt; then
    echo "test_file1.txt GET failed"
fi
rm .temp

./testcli 5000 tests/test_file2.jpg GET > .temp
if ! cmp -s .temp tests/test_file2.jpg; then
    echo "test_file2.jpg GET failed"
fi
rm .temp

./testcli 5000 tests/test_file2.jpg.dne GET > .temp1
echo "tests/test_file2.jpg.dne does not exist" > .temp2
if ! cmp -s .temp1 .temp2; then
    echo "test_file2.jpg.dne GET failed"
fi

./testcli 5000 na LS > .temp1
ls .test_dfs1 > .temp2
if ! cmp -s .temp1 .temp2; then
    echo "basic LS failed"
fi

rm -f .temp .temp1 .temp2

# kill the server
kill $dfs_pid
# print the log
echo "dfs log:"
cat .dfs_log.txt
rm .dfs_log.txt
rm -rf ./.test_dfs1/
