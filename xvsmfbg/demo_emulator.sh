#! /bin/sh

Xvfb :1 -screen 0 480x240x8 -shmem 2>&1 | ./xvsmfbg | emulator/tc_emulator

