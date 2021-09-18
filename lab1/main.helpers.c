#include <msp430.h>

void disableWatchdogTimer() {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
}

bool getIsS1Pressed() {
    return !(bool)(P1IN & BIT7);
}

bool getIsS2Pressed() {
    return !(bool)(P2IN & BIT2);
}

void toggleLED1(bool turnOn) {
    if(turnOn) {
        P1OUT |= BIT0;
    } else {
        P1OUT &= ~BIT0;
    }
}

void toggleLED2(bool turnOn) {
    if(turnOn) {
        P8OUT |= BIT1;
    } else {
        P8OUT &= ~BIT1;
    }
}