## THE PROJECT

I wanted to be able to pilot and update OTA (Over The Air) an arduino PRO mini using bluetooth and a cheap HC-05 clone.
Doing so allows to update easily a card located behind a dashboard, or sealed in a waterproof container, and use a phone as the user interface...

It seemed simple, but it turned out to be very difficult.

WARNING: This project is not for beginners; it involves electronics, Linux shell commands, soldering, interrupts and MCU programming at low level, recompiling a bootloader and patching the arduino GUI. It is NOT packaged and is more a proof of concept and a collection of notes than a finished product. Do NOT try it if you do not understand any of those words!!!

A first try was to follow one of the many tutos like [this one](https://create.arduino.cc/projecthub/PSoC_Rocks/washing-machine-timer-25d969) and write a small sketch which echoes received characters incremented (ie. an 'a' is echoed as 'b'), and test using a communication program such as the excellent [CuteCom](http://cutecom.sourceforge.net/) (I am a Linux user)

For that I needed to setup bluetooth on Ubuntu, and create a serial communication. I installed a cheap bluetooth dongle which was hopefully recognised, and wired the HC-05 to the arduino as explained.

Bluetooth serial communication under Linux is not so simple, but following [this tuto](https://gist.github.com/0/c73e2557d875446b9603) worked well. It involves discovering and pairing using `bluetoothctl`, then creating a device using `rfcomm`.

I reprogrammed the HC-05 by following [this tuto](https://www.buildlog.net/blog/2017/10/using-the-hc-05-bluetooth-module/) (Do not forget to use CR+LF for line ends, and use UPPERCASE commands!) So that STATE mimicks the DTR line by going low when the connections is established. Here is the [list of AT commands](https://wiki.iteadstudio.com/Serial_Port_Bluetooth_Module_(Master/Slave)_:_HC-05).

I then connected the arduino to Linux using a serial dongle, flashed my simple sketch and validated that, when connecting to the card using the serial dongle and CuteCom on /dev/ttyUSBx, I got my characters echoed with 1 added (ie: 'aaa' -> 'bbb')

Then I connected the arduino to the hc-05, and fired CuteCom onto /dev/rfcomm0. It worked and I was able get my characters echoed OTA :).

The real problems started when I tried to upload a sketch OTA: it simply did not work!

The first reason was that the STATE pin of the HC-05 was NOT connected! 
Very often, the HC05 chinese clones are crappy, and the PIO9 pin (when looking at the module with the antenna on top, it is the 4th one from the top left of the piggyback module) of the tiny piggyback module is not soldered to the PCB;you have to cut the plastic wrapping and add a blob of solder to connect it to the PCB. PIO9 is pin4, as shown here:

![IMG_20211130_124516](https://user-images.githubusercontent.com/87617071/144050426-891664b6-02a1-4b95-a742-3ed679280a3a.jpg)

After that, the STATE pin went low when the connection was established; but it did NOT reset the MCU !

The reason is the STATE voltage: The module inside the HC-05 is alimented in 3.3V, but uses internally 1.8V, according to an [old datasheet](https://zaguan.unizar.es/record/86110/files/TAZ-TFG-2017-1855_ANE.pdf). That was effectively the case, and, when looking with a scope at the RESET signal, it went barely below 3.5V, which is out of specification for 5V logic. OK, so let's amplify the signal.

I found [This schematic](https://forum.arduino.cc/t/solved-hc-05-wireless-programming-disabling-auto-reset/397319/4) which I found complicated, and ended up using a very simple schema: 
![image](https://user-images.githubusercontent.com/87617071/143788934-6118e41b-82a5-4c6e-9d0f-ca460f91be4c.png)

The collector is connected to the DTR of the arduino, saving a capacitor ;).

Obviously,this is an inverting amplifier, so I had to reprogram the HC-05 to its normal mode (AT+POLAR=0,0) and the MCU did reset reliabily each time the connection was established.

Unforunately OTA uploading worked only one time out of ten, on average!

When looking at TX and RX on the scope, triggered by RESET, I found that the handshake for uploading was not correct most of the time, and the arduino bootloader exited while AVRDUDE was still sending sync chars.

I suspect that this is caused by some line noise or an HC-05 "feature"...

Arduino uses the [STK500 protocol](http://ww1.microchip.com/downloads/en/AppNotes/doc2525.pdf) for uploading. I needed more delay in the bootloader, so time to flash another bootloader!

[Optiboot](https://github.com/Optiboot/optiboot) is a GREAT program, but the Arduino GUI is very complicated internally. [This post](https://tttapa.github.io/Pages/Arduino/Bootloaders/ATmega328P-custom-frequency.html) was a great help for changing the timeout from 1 second to 8 seconds. As added benefits, I gained more programming space (optiboot is only 512 bytes), and increased upload speed from 57600 to 115200 bauds.

Finally, upload worked reliably, but the price to pay was a startup time of 8 seconds in the normal case (no flashing) and some electronics.

Then I remembered that the [ATMEGA328P](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7810-Automotive-Microcontrollers-ATmega328P_Datasheet.pdf) has a little known feature called the analog comparator, and an internal Vref of about 1.1V, which sits nicely between the 2 values (1.8V and 0V) of STATE. In addition, Optiboot allows for [activating the bootloader by software](https://forum.arduino.cc/t/software-reset-with-bootloader/206946/11). There is even a [demo program](https://github.com/Optiboot/optiboot/blob/0a6528d1fc7e129209e3cfabfed1699ac29e96ff/optiboot/examples/test_reset/test_reset.ino#L130) for testing this.

So I removed the electronics, connected directly the STATE pin to the AIN1 (pin  7) of the Arduino PRO, downloaded a [nice library](https://www.arduino.cc/reference/en/libraries/analogcomp/), added a couple of lines (`analogComparator.setOn()` + `analogComparator.enableInterrupt()`) and an ISR setting a flag when the connection was established, and it worked!

CAVEAT: WDT must be cleared in `setup()`, otherwise we have an infinite reboot loop.

Problem: the MCU did reset correctly, but did not entered into bootloading most of the time. I recompiled Optiboot, setting the timer to 8 seconds, and OTA bootloading worked reliably!

Time to have a look to Optiboot source code!

When calling reset, and if MCUSR == 0, we arrive directly at [line 845](https://github.com/Optiboot/optiboot/blob/master/optiboot/bootloaders/optiboot/optiboot.c), then after some initialisations and setting the watchdog to the timer value, we enter the bootloader main loop at line 923.

There, the function `verifySpace()` is called  from lots of places, and, if something is even slightly wrong, resets the MCU and starts the normal program!

This behaviour is even [documented](https://github.com/Optiboot/optiboot/wiki/HowOptibootWorks#implemented-commands):

> All other commands will result in a success response if they are immediate followed by CRC_EOP (0x20, ' ') (ie they are ignored), or a WDT reset (start applicatin) otherwise.

That's what is called a [feature](https://www.wired.com/story/its-not-a-bug-its-a-feature/)!!!

So I put back timer to 1 second and rewrote verifySpace() according to the [STK500 protocol](http://ww1.microchip.com/downloads/en/AppNotes/doc2525.pdf#G1184161) to tell `avrdude` that we have not understood the command:

```
void verifySpace() {
  if (getch() != CRC_EOP) {
#ifdef CLASSIC_VERIFY_SPACE
    watchdogConfig(WATCHDOG_16MS);    // shorten WD timeout
    while (1)            // and busy-loop so that WD causes
      ;              //  a reset and app start.
#else
    putch(STK_NOSYNC);
#endif
  }
  else {
    putch(STK_INSYNC);
  }
}
```
et voila! it worked perfectly, quickly and reliably.

Now, time to integrate this in the Arduino GUI.

The simplest way to do it is to install the superb [MiniCore](https://github.com/MCUdude/MiniCore) package from [MCUdude](https://github.com/MCUdude) and patch it.

For doing that, patch the above function in `~/.arduino15/packages/MiniCore/hardware/avr/2.1.3/bootloaders/optiboot_flash/optiboot_flash.c`, and recompile the 8977 !!! bootloaders using the makefile provided in that directory.

As a courtesy and if you want to quickly experiment, I have copied in `patched_bootloaders` my patched MiniCore bootloaders directory.

I finally removed the `analogComp` library and programmed the analog comparator by hand (function `setupAC()` and `ISR(ANALOG_COMP_vect)` in order to have a smaller sketch.

The result is the `flashota` sketch.

This approach have some other PROs/CONs:
- It is now possible to enable/disable in software the flash update (could be password protected)
- The sketch is aware of the connection status and can act accordingly.
- CON: Never forget to put the setupAC()/ISR, otherwise it will be impossible next time to reflash OTA !!!

CAVEAT: Do not forget to load this sketch the first time using a direct serial line ;)

Have fun!
