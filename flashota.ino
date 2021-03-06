/*
  Test of atmega328p reset & OTA upload with HC05
  using AVR analog comparator


  Pin Sense of hc05 connected to AIN1 of arduino (pin 7 of pro mini)
  uses MiniCore by MCUdude: https://github.com/MCUdude/MiniCore

  For ensuring a reliable OTA upload, the function verifySpace() of
optiboot_flash.c (located here:
~/.arduino15/packages/MiniCore/hardware/avr/2.1.3/bootloaders/optiboot_flash.c )
must be modified as follows:

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

and then the bootloaders must be regenerated by going to
~/.arduino15/packages/MiniCore/hardware/avr/2.1.3/bootloaders/optiboot_flash
and running make

The hc05 must be reprogrammed for inverting the SENSE line and setting the
correct speed:
- Inverted polarity: AT+POLAR=1,0
- speed: AT+UART=115200,0,0 for 8MHz parts and AT+UART=57600,0,0 for 8MHz parts


More explanations are available here (great posts!):
https://www.buildlog.net/blog/2017/10/using-the-hc-05-bluetooth-module/
or here:
http://www.martyncurrey.com/arduino-with-hc-05-bluetooth-module-at-mode/

Very often, the HC05 chinese clones are crappy, and the PIO9 pin (when looking
at the module with the antenna on top, it is the 4th one from the top left of
the piggyback module) of the tiny piggyback module is not soldered to the PCB;
you have to cut the plastic and add a blob of solder to connect it to the PCB.

I found a specification of an old version of the module here:
https://zaguan.unizar.es/record/86110/files/TAZ-TFG-2017-1855_ANE.pdf
PIO9 is 2 pins below RX (pin4)

*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

// Test reset code borrowed from:
// https://github.com/Optiboot/optiboot/blob/0a6528d1fc7e129209e3cfabfed1699ac29e96ff/optiboot/examples/test_reset/test_reset.ino#L130
uint8_t resetFlag __attribute__((section(".noinit")));

void resetFlagsInit(void) __attribute__((naked)) __attribute__((used))
__attribute__((section(".init0")));
void resetFlagsInit(void)
{
    /*
       save the reset flags passed from the bootloader
       This is a "simple" matter of storing (STS) r2 in the special variable
       that we have created.  We use assembler to access the right variable.
    */
    __asm__ __volatile__("sts %0, r2\n" : "=m"(resetFlag) :);
}

void printReset(const char *label, uint8_t resetFlags)
{
    Serial.print(label);
    Serial.print(resetFlags, HEX);
    /*
       check for the usual bits.  Note that the symnbols defined in wdt.h are
       bit numbers, so they have to be shifted before comparison.
    */
    if (resetFlags & (1 << WDRF)) {
        Serial.print(F(" Watchdog "));
        resetFlags &= ~(1 << WDRF);
    }
    if (resetFlags & (1 << BORF)) {
        Serial.print(F(" Brownout "));
        resetFlags &= ~(1 << BORF);
    }
    if (resetFlags & (1 << EXTRF)) {
        Serial.print(F(" External "));
        resetFlags &= ~(1 << EXTRF);
    }
    if (resetFlags & (1 << PORF)) {
        Serial.print(F(" PowerOn "));
        resetFlags &= ~(1 << PORF);
    }
    if (resetFlags != 0x00) {
        // It should never enter here
        Serial.print(" Unknown");
    }
    Serial.println("");
}

void printBoot()
{
    /* Figure out where the bootloader starts.  */
    typedef void (*reboot_t)(void);
    reboot_t reboot = (reboot_t)((FLASHEND - 511) >> 1);
    Serial.print("bootstart = ");
    Serial.println((unsigned int)reboot, HEX);
    Serial.flush();
}

const byte LED13 = 13; // set the output LED Port B Pin 5

#define USE_ANALOG_COMP_LIB 0

volatile byte doReboot = false; // Flag raised by onACInterrupt()

#ifndef USE_ANALOG_COMP_LIB
#include <analogComp.h>
// interrupt to be raised by the analog comparator
void onACInterrupt()
{
    byte isConnected = (ACSR & 0x20) ? true : false;

    if (isConnected) { doReboot = true; }
}

void setupAC()
{
    analogComparator.setOn(
      INTERNAL_REFERENCE,
      AIN1); // we instruct the lib to use voltages on the pins
    analogComparator.enableInterrupt(
      onACInterrupt,
      CHANGE); // we set the interrupt and when it has to be raised
}

#else

#include <avr/interrupt.h>
// interrupt to be raised by the analog comparator
ISR(ANALOG_COMP_vect)
{
    // true for FTDI, and HC05 reprogrammed with "AT+POLAR=1,0"
    byte isConnected = (ACSR & 0x20) ? true : false;
    if (isConnected) { doReboot = true; }
}

void setupAC()
{
    // initialize the analog comparator (AC)
    ACSR &= ~(1 << ACIE);   // Bit 3: disable interrupts on AC
    ACSR &= ~(1 << ACD);    // Bit 7: switch on the AC
    ACSR |= (1 << ACBG);    // Bit 6: Use Internal Voltage Reference (1V1) on +
    ACSR &= ~(1 << ACI);    // Bit 4: Clear Interrupt flag
    ACSR &= ~(1 << ACIC);   // Bit 2: Disable Input capture on AC output
    DIDR1 |= (1 << AIN1D);  // disable digital input on AIN1
    ADCSRB &= ~(1 << ACME); // Use AIN1 (pin 7) on - input
    ACSR &= ~((1 << ACIS1) | (1 << ACIS0)); // interrupt on toggle event
    ACSR |= (1 << ACIE);                    // Bit 3: enable AC interrupts
}
#endif

// Do reset with bootloader
// Works ONLY with optiboot!
// cf last post of:
// https://forum.arduino.cc/t/software-reset-with-bootloader/206946/11
// NOTA: reliable OTA flashing with a HC-05 is only possible with a
// modified Optiboot. Cf:

void reboot()
{
    MCUSR = ~(1 << WDRF); // allow us to disable WD
    wdt_disable();
    MCUSR = 0; // Force bootloader on optiboot

    /* Figure out where the bootloader starts. */
    typedef void (*reboot_t)(void);
    reboot_t optibootReboot = (reboot_t)((FLASHEND - 511) >> 1);
    // make sure interrupts are off and timers are reset.
    cli();
    TCCR0A = TCCR1A = TCCR2A = 0;
    optibootReboot();
}

void setup()
{
    // Disable watchdog, mandatory when using optiboot's reboot()
    MCUSR = ~(1 << WDRF); // allow to disable WD
    wdt_disable();

    Serial.begin(115200);
    Serial.println(F("Reset flag test"));
    printReset("Actual MCUSR content: 0x", MCUSR);
    printReset("Passed in R2: 0x", resetFlag);
    printBoot();
    Serial.println(F(""));
    pinMode(LED13, OUTPUT); // LED pin as output
    digitalWrite(LED13, HIGH);

    setupAC(); // Enable reset from analog comparator
}

void loop()
{
    int c;
    if (Serial.available()) {
        c = Serial.read();
        if (c >= 0x20) c += 1;
        Serial.write(c);
    }
    // Reboot
    if (doReboot) {
        digitalWrite(LED13, LOW);
        reboot();
        doReboot = false;
    }
}
