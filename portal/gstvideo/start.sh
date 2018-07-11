#!/bin/bash
rm ~/.Xauthority*

if [[ "${GORDON}" ]]; then
  echo "Gordon"
  sudo /usr/bin/gst-launch-1.0 rpicamsrc preview=0 do-timestamp=0 ! image/jpeg,width=640,height=480,framerate=20/1 ! queue max-size-time=50000000 leaky=upstream ! jpegparse ! tee name=t t. ! queue ! rtpjpegpay ! udpsink host=192.168.3.21 port=9000 sync=false t. ! queue ! rtpjpegpay ! udpsink host=127.0.0.1    port=8999 sync=false t. ! queue ! jpegdec ! videorate ! video/x-raw,framerate=5/1 ! videoscale ! video/x-raw,width=400,height=240 ! videoflip method=3 ! jpegenc ! multifilesink location=/var/www/html/tmp/snapshot.jpg sync=false &
fi

if [[ "${CHELL}" ]]; then
  echo "Chell"
  sudo /usr/bin/gst-launch-1.0 rpicamsrc preview=0 do-timestamp=0 ! image/jpeg,width=640,height=480,framerate=20/1 ! queue max-size-time=50000000 leaky=upstream ! jpegparse ! tee name=t t. ! queue ! rtpjpegpay ! udpsink host=192.168.3.20 port=9000 sync=false t. ! queue ! rtpjpegpay ! udpsink host=127.0.0.1    port=8999 sync=false t. ! queue ! jpegdec ! videorate ! video/x-raw,framerate=5/1 ! videoscale ! video/x-raw,width=400,height=240 ! videoflip method=3 ! jpegenc ! multifilesink location=/var/www/html/tmp/snapshot.jpg sync=false &
fi


startx ~/portal/gstvideo/auto.sh
