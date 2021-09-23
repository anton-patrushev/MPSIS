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

int toggleS1InterruptMode(int fallingMode) {
    if (fallingMode) {
        P1IES |= BIT7; // interrupts generated at failing edge (from high to low)
    } else {
        P1IES &= ~BIT7; // interrupts generated at raising edge (from low to high)
    }
}

int toggleS2InterruptMode(int fallingMode) {
    if (fallingMode) {
        P2IES |= BIT2; // interrupts generated at failing edge (from high to low)
    } else {
        P2IES &= ~BIT2; // interrupts generated at raising edge (from low to high)
    }
}
