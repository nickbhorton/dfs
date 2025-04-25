#!/bin/bash
# script operates on silence is success

# setup
mkdir -p dfs1 dfs2 dfs3 dfs4

./dfs dfs1 10001 2>&1 1> .dfs1_log.txt &
dfs1_pid=$!

./dfs dfs2 10002 2>&1 1> .dfs2_log.txt &
dfs2_pid=$!

./dfs dfs3 10003 2>&1 1> .dfs3_log.txt &
dfs3_pid=$!

./dfs dfs4 10004 2>&1 1> .dfs4_log.txt &
dfs4_pid=$!

# basic ls with 1 file
./dfc put tests/test_file1x.txt
ls1=$(./dfc ls)
if [ ! "$ls1" = "test_file1x.txt" ]; then
    echo "failed basic ls"
fi

# ls with 1 file and 1 broken server
rm dfs1/*
ls1=$(./dfc ls)
if [ ! "$ls1" = "test_file1x.txt" ]; then
    echo "failed ls with 1 broken server"
fi

# ls with 1 file and 2 broken servers
rm dfs2/*
ls1=$(./dfc ls)
if [ ! "$ls1" = "test_file1x.txt [incomplete]" ]; then
    echo "failed broken ls with 2 broken server"
fi

rm dfs3/*
rm dfs4/*

# ls with 2 file
./dfc put tests/test_file1x.txt tests/test_file2xxxx.jpg
./dfc ls > .temp1
echo "test_file1x.txt" > .temp2
echo "test_file2xxxx.jpg" >> .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed 2 file ls"
fi
rm .temp1 .temp2

# ls with 2 file and 1 broken server
rm dfs1/*
./dfc ls > .temp1
echo "test_file1x.txt" > .temp2
echo "test_file2xxxx.jpg" >> .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed 2 file ls with 1 broken server"
fi
rm .temp1 .temp2

# ls with 2 file and 2 broken servers
rm dfs2/*
./dfc ls > .temp1
echo "test_file1x.txt [incomplete]" > .temp2
echo "test_file2xxxx.jpg [incomplete]" >> .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed 2 file ls with 2 broken server"
fi
rm .temp1 .temp2

rm dfs3/*
rm dfs4/*

# some file can survive 2 broken servers
./dfc put tests/test_file1x.txt
rm dfs2/*
rm dfs4/*
ls1=$(./dfc ls)
if [ ! "$ls1" = "test_file1x.txt" ]; then
    echo "failed 1 file ls with 2 broken server"
fi
rm dfs1/*
rm dfs3/*

# puting 2 files of same name
./dfc put tests/test_file2xxxx.jpg
./dfc put tests/test_file2xxxx.jpg
ls1=$(./dfc ls)
if [ ! "$ls1" = "test_file2xxxx.jpg" ]; then
    echo "failed 2 same file ls"
fi

# if we really kill a server
kill $dfs1_pid
# then put something
./dfc put tests/test_file1x.txt
./dfc ls > .temp1
# this should be seemless
echo "test_file1x.txt" > .temp2
echo "test_file2xxxx.jpg" >> .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed 2 file ls with 1 killed server"
fi
rm .temp1 .temp2


# if we really kill a second server this should destroy file2 but not file1
kill $dfs2_pid
./dfc ls > .temp1
echo "test_file1x.txt" > .temp2
echo "test_file2xxxx.jpg [incomplete]" >> .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed 2 file ls with 2 killed server"
fi
rm .temp1 .temp2

# adding another file should work
./dfc put tests/test_file3xxxxxx.vox.png
./dfc ls > .temp1
echo "test_file1x.txt" > .temp2
echo "test_file2xxxx.jpg [incomplete]" >> .temp2
echo "test_file3xxxxxx.vox.png" >> .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed 3 file ls with 2 killed server"
fi
rm .temp1 .temp2

# put servers back up but remove there files
rm dfs1/*
rm dfs2/*
./dfs dfs1 10001 2>&1 1>> .dfs1_log.txt &
dfs1_pid=$!

./dfs dfs2 10002 2>&1 1>> .dfs2_log.txt &
dfs2_pid=$!

# put another file
./dfc put tests/test_file4xxxxxxx.txt
./dfc ls > .temp1
echo "test_file1x.txt" > .temp2
echo "test_file2xxxx.jpg [incomplete]" >> .temp2
echo "test_file3xxxxxx.vox.png" >> .temp2
echo "test_file4xxxxxxx.txt" >> .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed 4 file ls with 2 reuped servers"
fi
rm .temp1 .temp2

# setup for get tests
rm dfs1/*
rm dfs2/*
rm dfs3/*
rm dfs4/*

./dfc put tests/test_file1x.txt tests/test_file1x.txt tests/test_file2xxxx.jpg

./dfc get test_file1x.txt
mv test_file1x.txt .temp1
cat tests/test_file1x.txt > .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed basic get"
fi
rm .temp1 .temp2

./dfc get test_file2xxxx.jpg
mv test_file2xxxx.jpg .temp1
cat tests/test_file2xxxx.jpg > .temp2
if ! cmp -s .temp1 .temp2; then
    echo "failed basic get snd file"
fi
rm .temp1 .temp2

kill $dfs1_pid
kill $dfs2_pid
kill $dfs3_pid
kill $dfs4_pid

if [ "$1" = "d" ]; then
echo "dfs1 log"
cat .dfs1_log.txt
echo "dfs2 log"
cat .dfs2_log.txt
echo "dfs3 log"
cat .dfs3_log.txt
echo "dfs4 log"
cat .dfs4_log.txt
fi

rm -rf dfs1 dfs2 dfs3 dfs4
rm .dfs*_log.txt
