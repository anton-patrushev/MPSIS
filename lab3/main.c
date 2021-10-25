#include <msp430.h>

#define TRUE 1
#define FALSE 0

/********************************************* Control Flags ***************************************** */

int interruptsCount = 0;

int period = 700 * 2;
int period_x2 = period * 2;
int period_x3 = period * 3;

int isTimerA1Selected = FALSE;

int isLEDsBlinkingEnabled = TRUE;

/********************************************* End Control Flags ***************************************** */

/********************************************* Utils ***************************************** */
void enableInterruptions() {
  __bis_SR_register(GIE); // set GIE flag to 1 (Global Interruptions Enabled)
  __enable_interrupt(); // enable Maskable IRQs
}

void setupButtons() {
  P1DIR &= ~BIT7; // make S1 input
  P1REN |= BIT7; // enable pull up/down resistor for S1
  P1OUT |= BIT7; // select pull up resistor (not pressed - high, pressed - low state)
  P1IE |= BIT7; // enable interrupts for S1
  P1IES |= BIT7; // interrupts generated at falling edge (from high to low)
  P1IFG &= ~BIT7; // clear interrupt flag

  P2DIR &= ~BIT2; // make S2 input
  P2REN |= BIT2; // enable pull up/down resistor for S2
  P2OUT |= BIT2; // select pull up resistor (not pressed - high, pressed - low state)
  P2IE |= BIT2; // enable interrupts for S2
  P2IES |= BIT2; // interrupts generated at falling edge (from high to low)
  P2IFG &= ~BIT2; // clear interrupt flag
}

void disableWatchDogTimer() {
	SFRIE1 &= ~WDTIE; // disable watchdog interval mode
	WDTCTL = WDTPW | WDTHOLD;
}


void enableWDTIntervalMode() {
	SFRIE1 |= WDTIE; // enable watchdog interval mode
	// WDTTMSEL - enables mode selection
	// WDTCNTCL - clears WTD counter

	// WTDTIS - interval select; 
	// WDTSSEL_0 - source = ACLK

	// from the header file - (WDTPW+WDTTMSEL+WDTCNTCL+WDTIS2+WDTIS1) ~ 0.5ms
	WDTCTL = WDT_MDLY_0_5;
}


// TODO: implement
void disableTA1() {}

// TODO: implement
void enableTA1() {}


void enableLED1(int turnOn) { P1OUT |= BIT0; }
void disableLED1(int turnOn) { P1OUT &= ~BIT0; }

void enableLED2(int turnOn) { P8OUT |= BIT1; }
void disableLED2(int turnOn) { P8OUT &= ~BIT1; }

void enableLED3(int turnOn) { P8OUT |= BIT3; }
void disableLED3(int turnOn) { P8OUT &= ~BIT3; }

void setupLEDs() {
  P1DIR |= BIT0; // make LED1 output
  disableLED1(); // make LED1 off by default

  P8DIR |= BIT1; // make LED2 output
  disableLED2(); // make LED2 off by default

	P8DIR |= BIT3; // make LED2 output
  disableLED3(); // make LED2 off by default
}


void endS1IRQ() { P1IFG &= ~BIT7; }
void endS2IRQ() { P2IFG &= ~BIT2; }

int isS1IRQ() { return (P1IFG & BIT7) == BIT7 ? TRUE : FALSE; }
int isS2IRQ() { return (P2IFG & BIT2) == BIT2 ? TRUE : FALSE; }

int toggleS1InterruptMode() {
	int isFallingModeEnabled = (int)(PIES & BIT7) != 0 ? FALSE : TRUE;

  if (isFallingModeEnabled) {
    P1IES &= ~BIT7; // interrupts generated at raising edge (from low to high)
  } else {
    P1IES |= BIT7; // interrupts generated at falling edge (from high to low)
  }
}

// TODO: is it required?
int toggleS2InterruptMode() {
	int isFallingModeEnabled = (int)(P2IES & BIT2) == 0 ? FALSE : TRUE;

  if (isFallingModeEnabled) {
    P2IES &= ~BIT2; // interrupts generated at raising edge (from low to high)
  } else {
    P2IES |= BIT2; // interrupts generated at falling edge (from high to low)
  }
}


void toggleSelectedTimer() {
	if (isTimerA1Selected) {
		isTimerA1Selected = FALSE;
		// TODO: support TA1
		disableTA1();
		enableWDTIntervalMode();
	} else {
		isTimerA1Selected = TRUE;
		disableWatchDogTimer();
		// TODO: support TA1
 		enableTA1();
	}
}

void disableSelectedTimer() { 
	if (isTimerA1Selected) {
		disableTA1()
	} else {
		disableWatchDogTimer();
	}
}

void enableSelectedTimer() { 
	if (isTimerA1Selected) {
		enableSelectedTimer()
	} else {
		enableWDTIntervalMode();
	}
}

void makeTA03Peripheral() {
	P1SEL |= BIT4; // 0 - I/0; 1 - Peripheral
}

/********************************************* End Utils ***************************************** */


#pragma vector = WDT_VECTOR
__interrupt void WDT_ISR(void) {
	interruptsCount++;

	switch (interruptsCount) {
	case period:
		enableLED1();
		disableLED2();
		break;
	case period_x2:
		enableLED2();
		disableLED3();
		break;
	case period_x3:
		enableLED3();
		disableLED1();

		interruptsCount = 0;
		break;
	}
}


#pragma vector = PORT1_VECTOR
__interrupt void S1ISR() {
  int didS1InterruptRequested = isS1IRQ();
	
  if (!didS1InterruptRequested) { return; }


	if (isLEDsBlinkingEnabled) {
		isLEDsBlinkingEnabled = FALSE;
		disableSelectedTimer();

		disableLED1();
		disableLED2();
		disableLED3();
	} else {
		isLEDsBlinkingEnabled = TRUE;
		enableSelectedTimer();
	}

  toggleS1InterruptMode();

  endS1IRQ();
}

#pragma vector = PORT2_VECTOR
__interrupt void S2ISR() {
  int didS2InterruptRequested = isS2IRQ();

  if (!didS2InterruptRequested) { return; }

  toggleSelectedTimer();

  endS2IRQ();
}


int runApp() {
	__no_operation();
}


int main(void) {
	disableWatchDogTimer();

	// TODO: configure and init TA1

	setupLEDs();
	setupButtons();

	makeTA03Peripheral();

	enableInterruptions();

	runApp();

	return 0;
}