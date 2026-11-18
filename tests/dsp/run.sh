#!/usr/bin/env bash
cd "$(dirname "$0")" && g++ -O2 -std=c++17 -I. dsp_test.cpp -o /tmp/dsp_test && /tmp/dsp_test
