#!/bin/bash
./rebuild.sh -p firestreamer --libs='-ldl -l:libssl.a -l:libcrypto.a' -f '-Wno-missing-field-initializers'
