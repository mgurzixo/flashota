/*
  Software serial test
*/
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <analogComp.h>

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

const byte LED13 = 13; // set the output LED Port B Pin 5
byte doReboot = false;

// interrupt to be raised by the analog comparator
byte first = 1;
void changeStatus()
{
    // if (first) {
    //     first = 0;
    //     return;
    // }
    byte isConnected = (ACSR & 0x20) ? true : false; // FTDI
    // byte isConnected = (ACSR & 0x20) ? false : true; // Inverted

    if (isConnected) {
        digitalWrite(LED13, LOW);
        doReboot = true;
    }
    // else    digitalWrite(LED13, LOW);
}

void printBoot()
{
    /* Figure out where the bootloader starts. */
    typedef void (*reboot_t)(void);
    reboot_t reboot = (reboot_t)((FLASHEND - 511) >> 1);

    Serial.print("bootstart = ");
    Serial.println((unsigned int)reboot, HEX);

    Serial.flush();
}

void setup()
{
    MCUSR = ~(1 << WDRF); // allow us to disable WD
    wdt_disable();

    Serial.begin(115200);
    Serial.println(F("Reset flag test"));
    printReset("Actual MCUSR content: 0x", MCUSR);
    printReset("Passed in R2: 0x", resetFlag);
    printBoot();
    Serial.println(F(""));

#if 1
    pinMode(LED13, OUTPUT); // LED pin as output
    digitalWrite(LED13, HIGH);
    pinMode(12, OUTPUT); // green led

#endif

    analogComparator.setOn(
      INTERNAL_REFERENCE,
      AIN1); // we instruct the lib to use voltages on the pins
    analogComparator.enableInterrupt(
      changeStatus,
      CHANGE); // we set the interrupt and when it has to be raised
}

void loop()
{ // run over and over
    int c;
    if (Serial.available()) {
        c = Serial.read();
        if (c >= 0x20) c += 1;
        Serial.write(c);
    }
    // Reboot
#if 1
    if (doReboot) {
        // Do reset with bootloader
        // Works ONLY with optiboot!
        // cf
        // https://forum.arduino.cc/t/software-reset-with-bootloader/206946/11
        // https://github.com/Optiboot/optiboot/blob/0a6528d1fc7e129209e3cfabfed1699ac29e96ff/optiboot/examples/test_reset/test_reset.ino#L130
        MCUSR = ~(1 << WDRF); // allow us to disable WD
        wdt_disable();
        MCUSR = 0;

        /* Figure out where the bootloader starts. */
        typedef void (*reboot_t)(void);
        reboot_t reboot = (reboot_t)((FLASHEND - 511) >> 1);
        cli();
        // make sure interrupts are off and timers are reset.
        TCCR0A = TCCR1A = TCCR2A = 0;
        reboot();
    }
#endif // 0
}
