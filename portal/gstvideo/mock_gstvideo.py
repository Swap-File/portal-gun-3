#!/usr/bin/python3 -u

import time
import os
import stat

if stat.S_ISFIFO(os.stat("GSTVIDEO_IN_PIPE").st_mode):
	file = open("GSTVIDEO_IN_PIPE","w") 
	while True:
		print("Writing 1")
		file.write("1\n") 
		file.flush();
		time.sleep(60)

else:
	print("GSTVIDEO_IN_PIPE doesn't Exist!")
	

