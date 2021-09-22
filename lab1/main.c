#include <msp430.h> 
#include "main.helpers.h"

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

    P2DIR &= ~BIT2; // make S1 input
    P2REN |= BIT2; // enable pull up/down resistor for S1
    P2OUT |= BIT2; // select pull up resistor (not pressed - high, pressed - low state)
}


void setup() {
    disableWatchdogTimer();
    setupLEDs();
    setupButtons();
}


void runApp() {
    int isAppRunning = TRUE;

    int didS1Pressed = FALSE;
    int didS2Pressed = FALSE;

    int isLED1Enabled = FALSE;

    while(isAppRunning) {

    	int isS1Pressed = getIsS1Pressed();
    	int isS2Pressed = getIsS2Pressed();

    	int wasS2Released = didS2Pressed && !isS2Pressed;
    	int wasS1Pressed = !didS1Pressed && isS1Pressed;

        if(wasS2Released && !isS1Pressed) {
            toggleLED2(FALSE);
        }

        if(wasS1Pressed) {
            toggleLED2(TRUE);
        }

        // LED1 handling
        // TODO: ensure it works (may require debug and fix)
        if(wasS2Released && isS1Pressed) {
            if(isLED1Enabled) {
                isLED1Enabled = FALSE;
                toggleLED1(FALSE);
            } else {
                isLED1Enabled = TRUE;
                toggleLED1(TRUE);
            }
        }

        didS1Pressed = isS1Pressed;
        didS2Pressed = isS2Pressed;
    }
}

/**
 * main.c
 */
int main(void) {
	setup();

	runApp();
	
	return 0;
}
