#!/usr/bin/python3 -u

import time
import os
import stat

if stat.S_ISFIFO(os.stat("GSTVIDEO_IN_PIPE").st_mode):
	file = open("GSTVIDEO_IN_PIPE","w") 
	while True:
		filename = input('Enter Request: ')
		file.write(filename)
		file.write(" 0\n") 
		file.flush();
else:
	print("GSTVIDEO_IN_PIPE doesn't Exist!")
	

