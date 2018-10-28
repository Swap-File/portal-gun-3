#!/usr/bin/python3 -u

import time
import os
import stat

if stat.S_ISFIFO(os.stat("/home/pi/GSTVIDEO_IN_PIPE").st_mode):
	file = open("/home/pi/GSTVIDEO_IN_PIPE","w") 
	while True:
		filename = input('Enter Request: ')
		file.write("8 ") 
		file.write(filename)
		file.write(" 0 0 0\n") 
		file.flush();
else:
	print("/home/pi/GSTVIDEO_IN_PIPE doesn't Exist!")
	

