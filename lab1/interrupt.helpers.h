#include <msp430.h>

#define TRUE 1
#define FALSE 0

void disableWatchdogTimer() {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
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

void endS1IRQ() {
    P1IFG &= ~BIT7; // end interrupt routine
}

void endS2IRQ() {
    P2IFG &= ~BIT2; // end interrupt routine
}

int isS1IRQ() {
    return (P1IFG & BIT7) == BIT7 ? TRUE : FALSE;
}

int isS2IRQ() {
    return (P2IFG & BIT2) == BIT2 ? TRUE : FALSE;
}
