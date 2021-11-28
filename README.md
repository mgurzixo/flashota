THE PROJECT

I wanted to be able to pilot and update OTA (Over The Air) an arduino PRO mini using bluetooth.
Doing so allows to update easily a card located behind a dashboard, or sealed in a waterproof container...
It seemed simple, but it turned out to be very difficult.

A first try was to follow one of the many tutos like [this one](https://create.arduino.cc/projecthub/PSoC_Rocks/washing-machine-timer-25d969) and write a small sketch which echoes received characters incremented (ie. an 'a' is echoed as 'b'), and test that using a communication program such as [CuteCom](http://cutecom.sourceforge.net/) (I am a Linux user)

For that I needed to setup bluetooth on Ubuntu, and create a serial communication. I installed a cheap dongle which was hopefully recognised, and wired the HC-05 to the arduino as explained.

Bluetooth serial communication under Linux is not so simple, but following [This tuto](https://gist.github.com/0/c73e2557d875446b9603) worked well.

After having reprogrammed the HC-05 by following [this tuto](https://www.buildlog.net/blog/2017/10/using-the-hc-05-bluetooth-module/) I was able get my characters echoed.

The problems started when I tried to upload a sketch OTA: it worked one time out of 30!

The reasons are multiple:

1. SENSE voltege: The module inside the HC-05 is alimented in 3.3V, but use internally 1.8V, according to an [old datasheet](https://zaguan.unizar.es/record/86110/files/TAZ-TFG-2017-1855_ANE.pdf), 


