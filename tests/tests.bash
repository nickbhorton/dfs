#!/bin/bash
cat tests/put.dat | nc 127.0.0.1 10001
# -q 0 closes socket after done sending
cat tests/put_incomplete.dat | nc -q 0 127.0.0.1 10001 # should fail
cat tests/put.dat | nc 127.0.0.1 10001
cat tests/test.dat | nc 127.0.0.1 10001 | hexdump -X
cat tests/test_exists.dat | nc 127.0.0.1 10001 | hexdump -X
