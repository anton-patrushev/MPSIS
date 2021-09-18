#include <msp430.h> 
#include "main.helpers.c"

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
    bool isAppRunning = true;

    bool didS1Pressed = false;
    bool didS2Pressed = false;

    bool isLED1Enabled = false;

    while(isAppRunning) {

        bool isS1Pressed = getIsS1Pressed();
        bool isS2Pressed = getIsS2Pressed();

        bool wasS2Released = didS2Pressed && !isS2Pressed;
        bool wasS1Pressed = !didS1Pressed && isS1Pressed;

        if(wasS2Released && !isS1Pressed) {
            toggleLED2(false);
        }

        if(wasS1Pressed) {
            toggleLED2(true);
        }

        // LED1 handling
        // TODO: ensure it works (may require debug and fix)
        if(wasS2Released && isS1Pressed) {
            if(isLED1Enabled) {
                isLED1Enabled = false;
                toggleLED1(false);
            } else {
                isLED1Enabled = true;
                toggleLED1(true);
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
