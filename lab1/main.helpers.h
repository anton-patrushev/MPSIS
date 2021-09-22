#include <msp430.h>

#define TRUE 1
#define FALSE 0

void disableWatchdogTimer() {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
}

int getIsS1Pressed() {
    return (P1IN & BIT7) == BIT7 ? FALSE : TRUE;
}

int getIsS2Pressed() {
    return (P2IN & BIT2) == BIT2 ? FALSE : TRUE;
}

void toggleLED1(int turnOn) {
    if(turnOn) {
        P1OUT |= BIT0;
    } else {
        P1OUT &= ~BIT0;
    }
}

void toggleLED2(int turnOn) {
    if(turnOn) {
        P8OUT |= BIT1;
    } else {
        P8OUT &= ~BIT1;
    }
}
