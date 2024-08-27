#!/bin/bash
./rebuild.sh -p firestreamer --libs='-ldl -l:libssl.a -l:libcrypto.a -lv4l2' -f '-Wno-missing-field-initializers'
