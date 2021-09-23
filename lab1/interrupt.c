#include <msp430.h>
#include "interrupt.helpers.h"

void setupLEDs() {
    P1DIR |= BIT0; // make LED1 output
    P1OUT &= ~BIT0; // make LED1 off by default

    P8DIR |= BIT1; // make LED2 output
    P8OUT &= ~BIT1; // make LED2 off by default
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

void enableInterruptions() {
    __bis_SR_register(GIE); // set GIE flag to 1 (Global Interruptions Enabled)
    __enable_interrupt(); // enable Maskable IRQs
}


void setup() {
    disableWatchdogTimer();
    enableInterruptions();

    setupLEDs();
    setupButtons();
}

void runApp() {
    __no_operation(); // 1 cycle delay
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
int isS1Pressed = FALSE;
int isS2Pressed = FALSE;
int isLED1Enabled = FALSE

// will be called when button fired
#pragma vector = PORT1_VECTOR
__interrupt void handleButton1InterruptRoutine() {
    int didS1InterruptRequested = isS1IRQ();

    if(didS1InterruptRequested) {
    	isS1Pressed = !isS1Pressed;

        if(isS1Pressed) {
            toggleLED2(TRUE);
        }

        toggleS1InterruptMode(!isS1Pressed); // make next interrupt routine to be called when button will be released
    }

    endS1IRQ();
}

// will be called when button fired
#pragma vector = PORT2_VECTOR
__interrupt void handleButton2InterruptRoutine() {
    int didS2InterruptRequested = isS2IRQ();

    if(didS2InterruptRequested) {
    	isS2Pressed = !isS2Pressed;

        if(!isS2Pressed && !isS1Pressed) {
            toggleLED2(FALSE);
        }

        if (isS1Pressed && !isS1Pressed) {
            isLED1Enabled = !isLED1Enabled;
            toggleLED1(isLED1Enabled);
        }

        toggleS2InterruptMode(!isS2Pressed); // make next interrupt routine to be called when button will be released
    }

    endS2IRQ();
}
