#include <msp430.h>

#include "helpers.h"

void setup() {
    disableWatchdogTimer();
    enableInterruptions();

    setupButtons();
    setupOscillographOutput();

    setupDCOCLK();
    setMCLKDefaultConfiguration();
}

void runApp() {
    __no_operation(); // 1 cycle delay
}

int main(void) {
	setup();

	runApp();

	return 0;
}

// control flags
int isLPMEnabled = FALSE;
int isMCLKFrequencyModeChanged = FALSE;

#pragma vector = PORT1_VECTOR
__interrupt void handleS1InterruptRoutine() {
	int didS1InterruptRequested = isS1IRQ();

	if(!didS1InterruptRequested) {
		return;
	}

	delayInterruptRoutineExecution(1488);

    if(isLPMEnabled) {
    	__bic_SR_register_on_exit(LPM4_bits); // exit LPM
    	isLPMEnabled = FALSE;

    } else {
    	isLPMEnabled = TRUE;
      __bis_SR_register_on_exit(LPM4_bits); // enter LPM
    }

    endS1IRQ();
}

#pragma vector = PORT2_VECTOR
__interrupt void handleS2InterruptRoutine() {
    int didS2InterruptRequested = isS2IRQ();

    if(!didS2InterruptRequested) {
    	return;
    }

//	delayInterruptRoutineExecution(1488);

    if(isMCLKFrequencyModeChanged) {
    	setMCLKDefaultConfiguration();
    } else {
    	setMCLKDividedFrequencyConfiguration();
    }

    isMCLKFrequencyModeChanged = !isMCLKFrequencyModeChanged;

    endS2IRQ();
}
