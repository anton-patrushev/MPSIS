#include <msp430.h> 

void disableWatchdogTimer() {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
}

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


void runApp() {
    bool isAppRunning = true;

    bool didS1Pressed = false;
    bool didS2Pressed = false;

    while(isAppRunning) {

        bool isS1Pressed = getIsS1Pressed();
        bool isS2Pressed = getIsS2Pressed();

        bool wasS2Released = didS2Pressed && !isS2Pressed;
        bool wasS1Pressed = !didS1Pressed && isS1Pressed;

        if(wasS2Released) {
            toggleLED2(false);
        }

        if(wasS1Pressed) {
            toggleLED2(true);
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
