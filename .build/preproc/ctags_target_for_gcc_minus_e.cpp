# 1 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
/*
  Software serial  test
*/

# 6 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 2
# 7 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 2
# 8 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 2

const byte LED13 = 13; // set the output LED Port B Pin 5
const byte PIN12 = 6; // connected to DTR PortB Pin 4

// interrupt to be raised by the analog comparator
byte first = 1;
void changeStatus()
{
    if (first) {
        first = 0;
        return;
    }
    if (
# 20 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
       (*(volatile uint8_t *)((0x30) + 0x20)) 
# 20 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
            & 0x20) {
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


        // Do reset with bootloader
        // Works ONLY with optiboot!
        // cf
        // https://forum.arduino.cc/t/software-reset-with-bootloader/206946/11
        // https://github.com/Optiboot/optiboot/blob/0a6528d1fc7e129209e3cfabfed1699ac29e96ff/optiboot/examples/test_reset/test_reset.ino#L130
        
# 40 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
       (*(volatile uint8_t *)((0x34) + 0x20)) 
# 40 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
             = ~(1 << 
# 40 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
                      3
# 40 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
                          ); // allow us to disable WD
        wdt_disable();
        
# 42 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
       (*(volatile uint8_t *)((0x34) + 0x20)) 
# 42 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
             = 0;

        /* Figure out where the bootloader starts. */




        typedef void (*do_reboot_t)(void);
        const do_reboot_t do_reboot = (do_reboot_t)((
# 50 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
                                                    0x7FFF 
# 50 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
                                                             - 511) >> 1);

        
# 52 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
       __asm__ __volatile__ ("cli" ::: "memory")
# 52 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
            ;
        
# 53 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
       (*(volatile uint8_t *)((0x24) + 0x20)) 
# 53 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
              = 
# 53 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
                (*(volatile uint8_t *)(0x80)) 
# 53 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
                       = 
# 53 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino" 3
                         (*(volatile uint8_t *)(0xB0)) 
# 53 "/home/mgouget/Arduino/generated_examples/SoftwareSerialExample/SoftwareSerialExample.ino"
                                =
          0; // make sure interrupts are off and timers are reset.
        do_reboot();

        // while (true);

    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("A");


    pinMode(LED13, 0x1); // LED pin as output
    digitalWrite(LED13, 0x1);


    pinMode(LED13, 0x1); // LED pin as output
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
