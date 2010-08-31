#! /usr/bin/python

import sys,time,serial

port   = "/dev/ttyUSB0"
speed  = int(sys.argv[1])
target = 0

ser = serial.Serial(port,speed)
#ser.setDTR()

while True:
	c = sys.stdin.read(1)
	ser.write(c)

