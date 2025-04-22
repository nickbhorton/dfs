#!/bin/bash

rm -rf .dir1 .dir2 .dir3 .dir4
mkdir .dir1 .dir2 .dir3 .dir4

./dfs .dir1 10001 1> .dir1/1.txt 2> .dir1/2.txt &
./dfs .dir2 10002 1> .dir2/1.txt 2> .dir2/2.txt &
./dfs .dir3 10003 1> .dir3/1.txt 2> .dir3/2.txt &
./dfs .dir4 10004 1> .dir4/1.txt 2> .dir4/2.txt &
