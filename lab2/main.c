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

    if(didS1InterruptRequested) {
        if(isLPMEnabled) {
            exitLPM4();
        } else {
            enterLPM4();
        }

        isLPMEnabled = !isLPMEnabled;
        //__delay_cycles(160000);
    }

    endS1IRQ();
}


#pragma vector = PORT2_VECTOR
__interrupt void handleS2InterruptRoutine() {
    int didS2InterruptRequested = isS2IRQ();

    if(didS2InterruptRequested) {
        if(isMCLKFrequencyModeChanged) {
            setMCLKDefaultConfiguration();
        } else {
            setMCLKDividedFrequencyConfiguration();
        }

        //__delay_cycles(160000);
        isMCLKFrequencyModeChanged = !isMCLKFrequencyModeChanged;
    }

    endS2IRQ();
}