#! /bin/sh

Xvfb :1 -screen 0 480x240x8 -shmem 2>&1 | ./xvsmfbg | python ./stream.py 921600

