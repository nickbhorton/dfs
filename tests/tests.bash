#!/bin/bash
rm -f dfs1/*
cat tests/put.dat | nc 127.0.0.1 10001
rm -f dfs1/*
# -q 0 closes socket after done sending
cat tests/put.dat | nc -q 0 127.0.0.1 10001
rm -f dfs1/*
cat tests/put_incomplete.dat | nc -q 0 127.0.0.1 10001
