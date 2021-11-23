#include <Arduino.h>
#line 1 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
/*
  Software serial  test
*/
#define __AVR_ATmega328P__
#include <avr/io.h>
#include <avr/wdt.h>
#include <analogComp.h>

const byte LED13 = 13; // set the output LED Port B Pin 5
const byte PIN12 = 6;  // connected to DTR PortB Pin 4

// interrupt to be raised by the analog comparator
byte first = 1;
#line 14 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
void changeStatus();
#line 62 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
void setup();
#line 83 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
void loop();
#line 14 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
void changeStatus()
{
    if (first) {
        first = 0;
        return;
    }
    if (ACSR & 0x20) {
        // Disconnect
        // PORTB |= 0x10; // PIN12 High
        // digitalWrite(PIN12, HIGH);
        analogWrite(PIN12, 255);
        // digitalWrite(LED13, LOW);
    }
    else {
        // Connection
        // PORTB &= ~0x10; // PIN12 -> Low
        // digitalWrite(PIN12, LOW);
        analogWrite(PIN12, 0);
        // digitalWrite(LED13, HIGH);

#if 1
        // Do reset with bootloader
        // Works ONLY with optiboot!
        // cf
        // https://forum.arduino.cc/t/software-reset-with-bootloader/206946/11
        // https://github.com/Optiboot/optiboot/blob/0a6528d1fc7e129209e3cfabfed1699ac29e96ff/optiboot/examples/test_reset/test_reset.ino#L130
        MCUSR = ~(1 << WDRF); // allow us to disable WD
        wdt_disable();
        MCUSR = 0;

        /* Figure out where the bootloader starts. */
#if FLASHEND > 140000
        Serial.println(
          F("Jump to bootloader not supported on chips with >128k memory"));
#else
        typedef void (*do_reboot_t)(void);
        const do_reboot_t do_reboot = (do_reboot_t)((FLASHEND - 511) >> 1);

        cli();
        TCCR0A = TCCR1A = TCCR2A =
          0; // make sure interrupts are off and timers are reset.
        do_reboot();
#endif
        // while (true);
#endif // 0
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("A");

#if 1
    pinMode(LED13, OUTPUT); // LED pin as output
    digitalWrite(LED13, HIGH);
#endif

    pinMode(LED13, OUTPUT); // LED pin as output
    // digitalWrite(PIN12, HIGH);

    // analogComparator.setOn(
    //   INTERNAL_REFERENCE,
    //   AIN1); // we instruct the lib to use voltages on the pins
    // analogComparator.enableInterrupt(
    //   changeStatus,
    //   CHANGE); // we set the interrupt and when it has to be raised
}

void loop()
{ // run over and over
    int c;
    if (Serial.available()) {
        c = Serial.read();
        if (c >= 0x20) c += 1;
        Serial.write(c);
    }
}

