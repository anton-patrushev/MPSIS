#include <msp430.h>

#define TRUE 1
#define FALSE 0

/********************************************* Control Flags ***************************************** */

volatile int interruptsCount = 0;

#define period 1400 // 700 * 2
#define period_x2 2800 // 700 * 2 * 2
#define period_x3 4200 // 700 * 2 * 3

int isTimerA1Selected = TRUE;

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
    interruptsCount = 0;

    SFRIE1 &= ~WDTIE; // disable watchdog interval mode
    WDTCTL = WDTPW | WDTHOLD;
}


void enableWDTIntervalMode() {
    SFRIE1 |= WDTIE; // enable watchdog interval mode
    // WDTTMSEL - enables mode selection
    // WDTCNTCL - clears WTD counter

    // WTDTIS - interval select;

    // SMCLK is choosen as a source
    // from the header file - (WDTPW+WDTTMSEL+WDTCNTCL+WDTIS2+WDTIS1) ~ 0.5ms
    WDTCTL = WDT_MDLY_0_5;
}


void disableTA1() {
    TA1CCTL0 &= ~CCIE; // disable interrupts
    TA1CTL = MC__STOP; // stop timer
}

void enableTA1() {
    // TASSEL__ACLK - choose ACLK as the source signal
    // MC__UP - UP mode
    // TACLR - clear timer
    // ID__1 - clear timer
    TA1CTL = TASSEL__ACLK | MC__UP | ID__1 | TACLR;
    TA1CCTL0 = CCIE; // enable interrupts for TA1 capture register 0=
    TA1CCR0 = 22400; // period = 0.7 sec, (ACLK * 0.7) = 32k * 0.7 = 22400
}


void enableLED1() { P1OUT |= BIT0; }
void disableLED1() { P1OUT &= ~BIT0; }

void enableLED2() { P8OUT |= BIT1; }
void disableLED2() { P8OUT &= ~BIT1; }

void enableLED3() { P8OUT |= BIT2; }
void disableLED3() { P8OUT &= ~BIT2; }

void setupLEDs() {
  P1DIR |= BIT0; // make LED1 output
  disableLED1(); // make LED1 off by default

  P8DIR |= BIT1; // make LED2 output
  disableLED2(); // make LED2 off by default

  P8DIR |= BIT2; // make LED2 output
  disableLED3(); // make LED2 off by default
}


void endS1IRQ() { P1IFG &= ~BIT7; }
void endS2IRQ() { P2IFG &= ~BIT2; }

int isS1IRQ() { return (P1IFG & BIT7) == BIT7 ? TRUE : FALSE; }
int isS2IRQ() { return (P2IFG & BIT2) == BIT2 ? TRUE : FALSE; }

int isS1FallingMode() { return (int)(P1IES & BIT7) != 0 ? TRUE : FALSE; }

int toggleS1InterruptMode() {
//  int isFallingModeEnabled = (int)(P1IES & BIT7) != 0 ? FALSE : TRUE;

  if (isS1FallingMode()) {
    P1IES &= ~BIT7; // interrupts generated at raising edge (from low to high)
  } else {
    P1IES |= BIT7; // interrupts generated at falling edge (from high to low)
  }
}

int isS2FallingMode() { return (int)(P2IES & BIT2) == 0 ? TRUE : FALSE; }

// TODO: is it required?
int toggleS2InterruptMode() {
//  int isFallingModeEnabled = (int)(P2IES & BIT2) == 0 ? FALSE : TRUE;

  if (isS2FallingMode()) {
    P2IES &= ~BIT2; // interrupts generated at raising edge (from low to high)
  } else {
    P2IES |= BIT2; // interrupts generated at falling edge (from high to low)
  }
}


void toggleSelectedTimer() {
    if (isTimerA1Selected) {
        isTimerA1Selected = FALSE;

        disableTA1();
        enableWDTIntervalMode();
    } else {
        isTimerA1Selected = TRUE;

        disableWatchDogTimer();
        enableTA1();
    }
}

void disableSelectedTimer() {
    if (isTimerA1Selected) {
        disableTA1();
    } else {
        disableWatchDogTimer();
    }
}

void enableSelectedTimer() {
    if (isTimerA1Selected) {
        enableTA1();
    } else {
        enableWDTIntervalMode();
    }
}

void setupLED7() {
    P1DIR |= BIT4; // make LED_CTP_4 output
	  P1OUT &= ~BIT4;
    P1SEL |= BIT4; // 0 - I/0; 1 - Peripheral
}

void setupTA0() {
  setupLED7();

	TA0CCR0 = 24000; // ~ 24 000 counts = 1.5 sec / 2 = 0.75
	TA0CCR3 = 16000; // ~ 16 000 counts = 0.75 - (1.5 sec / 6) = 0.5
	TA0CCTL3 = OUTMOD_7; // (SET / RESET)

	// /8 * /4 = /32 - SMCLK / 32 = ACLK

	TA0EX0 = TAIDEX_7; // /8
	TA0CTL = TASSEL__SMCLK | ID__4 | MC__UPDOWN | TACLR;
}

/********************************************* End Utils ***************************************** */

// ISR for CCIFG flag - from 0 to TACCR0 value
#pragma vector = TIMER1_A0_VECTOR
__interrupt void TA1_CCR0_ISR(void) {
    interruptsCount += period;

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

// Unused
 // ISR for TAIFG flag - from TACCR0 value to 0
// #pragma vector = TIMER1_A1_VECTOR
// __interrupt void WDT_ISR(void) {
//
// }


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

  if (isS1FallingMode()) {
    disableSelectedTimer();
    
    disableLED1();
    disableLED2();
    disableLED3();
  } else {
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
  interruptsCount = 0;

  // not sure we need to perform this
  // but just in case
  // requirements are not clear for this
  disableLED1();
  disableLED2();
  disableLED3();

  endS2IRQ();
}


int runApp() {
    __no_operation();
}


// * SMCLK ~ 1 MHz by default
// * ACLK ~ 32 kHz by default
// * according to this
// https://e2e.ti.com/support/microcontrollers/msp-low-power-microcontrollers-group/msp430/f/msp-low-power-microcontroller-forum/265568/default-frequency-of-msp430-without-connecting-any-crystal
int main(void) {
    disableWatchDogTimer();

    setupTA0();

    setupLEDs();
    setupButtons();

    enableSelectedTimer();

    enableInterruptions();

    runApp();

    return 0;
}
