#include <msp430.h> 
#include "interrupt.helpers.c"

void setupLEDs() {
    P1DIR |= BIT0; // make LED1 output
    P1OUT &= ~BIT0; // make LED1 off by default

    P8DIR |= BIT1; // make LED2 output
    P8DIR &= ~BIT1; // make LED2 off by default
}

void setupButtons() {
    P1DIR &= ~BIT7; // make S1 input
    P1REN |= BIT7; // enable pull up/down resistor for S1
    P1OUT |= BIT7; // select pull up resistor (not pressed - high, pressed - low state)
    P1IE |= BIT7; // enable interrupts for S1
    // TODO: verify it's fine
    P1IES |= BIT7; // interrupts generated at failing edge (from high to low)
    P1IFG &= ~BIT7; // clear interrupt flag

    P2DIR &= ~BIT2; // make S1 input
    P2REN |= BIT2; // enable pull up/down resistor for S1
    P2OUT |= BIT2; // select pull up resistor (not pressed - high, pressed - low state)
    P2IE |= BIT2; // enable interrupts for S2
    // TODO: verify it's fine
    P2IES |= BIT2; // interrupts generated at failing edge (from high to low)
    P2IFG &= ~BIT2; // clear interrupt flag
}

void enableInterruptions() {
    __bis_SR_register(GIE); // set GIE flag to 1 (Global Interruptions Enabled)
    __enable_interrupt(); // enable Maskable IRQs
}


void setup() {
    disableWatchdogTimer();
    enableGlobalInterruptions();

    setupLEDs();
    setupButtons();
}

void runApp() {
    // TODO: use this or line below
    __no_operation(); // 1 cycle delay

    // TODO: decide to use this or line above
    while(true) {}
}

/**
 * main.c
 */
int main(void) {
	setup();

	runApp();
	
	return 0;
}

// control flags
bool isS1Pressed = false;
bool isS2Pressed = false;


#pragma vector = PORT1_VECTOR
__interrupt void handleButton1InterruptRoutine() {
    // TODO: ensure it's a right way to check IRQ register source
    bool didS1InterruptRequested = isS1IRQ();

    if(!didS1InterruptRequested) return endS1IRQ();

    isS1Pressed = true;




    endS1IRQ()
}

#pragma vector = PORT2_VECTOR
__interrupt void handleButton2InterruptRoutine() {
    // TODO: ensure it's a right way to check IRQ register source
    bool didS2InterruptRequested = isS2IRQ();

    if(!didS2InterruptRequested) return endS2IRQ();

    isS2Pressed = true;


    


    endS2IRQ()
}