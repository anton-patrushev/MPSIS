#include <msp430.h>

#define TRUE 1
#define FALSE 0

void disableWatchdogTimer() {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
}

void enableInterruptions() {
    __bis_SR_register(GIE); // set GIE flag to 1 (Global Interruptions Enabled)
    __enable_interrupt(); // enable Maskable IRQs
}

void endS1IRQ() {
    P1IFG &= ~BIT7; // end interrupt routine for S1
}

void endS2IRQ() {
    P2IFG &= ~BIT2; // end interrupt routine for S2
}

int isS1IRQ() {
    return (P1IFG & BIT7) == BIT7 ? TRUE : FALSE;
}

int isS2IRQ() {
    return (P2IFG & BIT2) == BIT2 ? TRUE : FALSE;
}

void setupButtons() {
    P1DIR &= ~BIT7; // make S1 input
    P1REN |= BIT7; // enable pull up/down resistor for S1
    P1OUT |= BIT7; // select pull up resistor (not pressed - high, pressed - low state)
    P1IE |= BIT7; // enable interrupts for S1
    P1IES |= BIT7; // interrupts generated at falling edge (from high to low)
    P1IFG &= ~BIT7; // clear interrupt flag

    P2DIR &= ~BIT2; // make S1 input
    P2REN |= BIT2; // enable pull up/down resistor for S1
    P2OUT |= BIT2; // select pull up resistor (not pressed - high, pressed - low state)
    P2IE |= BIT2; // enable interrupts for S2
    P2IES |= BIT2; // interrupts generated at falling edge (from high to low)
    P2IFG &= ~BIT2; // clear interrupt flag
}

void setupOscillographOutput() {
    P7DIR |= BIT7; // make P7.7 output pin
    P7SEL |= BIT7; // make P7.7 pin set up for peripheral module
}

void enterLPM4() {
    __bis_SR_register(LPM4_bits); // enter LPM
}

void exitLPM4() {
    __bic_SR_register(LPM4_bits); // enter LPM
}

void setupDCOCLK() {
    UCSCTL3 |= SELREF__REFOCLK;
    UCSCTL1 = DCORSEL_0;
    UCSCTL2 = FLLN4 | FLLN3 | FLLN2; //28
    UCSCTL2 |= FLLD__1;
    UCSCTL3 |= FLLREFDIV__1;

    // Explanation for above code
    // DCOCLK = REFOCLK / FLLDIV * (FLLN + 1) * FLLD
    // DCOCLK = REFOCLK / 1 * (28 + 1) *1
}

void setMCLKDefaultConfiguration() {
    UCSCTL4 = SELM__DCOCLK;
    UCSCTL5 |= DIVM__32;
}

void setMCLKDividedFrequencyConfiguration() {
    UCSCTL4 = SELM__REFOCLK;
    UCSCTL5 |= DIVM__4;
}