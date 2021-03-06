#include <msp430.h>
#include "CTS_Layer.h"
#include <math.h>


typedef unsigned char uchar;

#define SET_COLUMN_ADDRESS_LSB        0x00
#define SET_COLUMN_ADDRESS_MSB        0x10
#define SET_PAGE_ADDRESS              0xB0

#define SET_SEG_DIRECTION             0xA0
#define SET_COM_DIRECTION             0xC0

#define SET_POWER_CONTROL             0x2F // Управление питанием. PC[0] – усилитель, PC[1] — регулятор, PC[2] — повторитель. 0 — отключено, 1 — включено
#define SET_SCROLL_LINE               0x40 // Установка начальной линии скроллинга SL=0..63
#define SET_VLCD_RESISTOR_RATIO       0x27 // Установка уровня внутреннего резисторного делителя PC = [0..7].Используется для управления контрастом.
#define SET_ELECTRONIC_VOLUME_MSB     0x81 // Регулировка контраста. Двухбайтная команда. PM[5..0] PM = 0..63.
#define SET_ELECTRONIC_VOLUME_LSB     0x0F
#define SET_ALL_PIXEL_ON              0xA4 // Включение всех пикселей. 0 – отображение содержимого памяти, 1 – все пиксели включены (содержимое памяти сохраняется).
#define SET_INVERSE_DISPLAY           0xA6 // Включение инверсного режима. 0 — нормальное отображение содержимого памяти, 1 — инверсное.
#define SET_DISPLAY_ENABLE            0xAF // Отключение экрана. 0 — экран отключен, 1 — включен.
#define SET_LCD_BIAS_RATIO            0xA2 // Смещение напряжения делителя: 0 – 1/9, 1 – 1/7.
#define SET_ADV_PROGRAM_CONTROL0_MSB  0xFA // Расширенное управление. ТС — температурная компенсация 0 = -0.05, 1 = -0.11 % / °С;
#define SET_ADV_PROGRAM_CONTROL0_LSB  0x90 // WC – циклический сдвиг столбцов 0 = нет, 1 = есть; WP –циклический сдвиг страниц 0 = нет, 1 = есть.

// CD: 0 - display choosen, 1 - controller choosen
#define CD              BIT6 // CD - choose device mode (BIT6 -> P5.6)
#define CS              BIT4 // CS - choose slave device (BIT4 -> P7.4)

uchar Dogs102x6_initMacro[] = {
	SET_SCROLL_LINE,
	SET_SEG_DIRECTION,
	SET_COM_DIRECTION,
	SET_ALL_PIXEL_ON,
	SET_INVERSE_DISPLAY,
	SET_LCD_BIAS_RATIO,
	SET_POWER_CONTROL,
	SET_VLCD_RESISTOR_RATIO,
	SET_ELECTRONIC_VOLUME_MSB,
	SET_ELECTRONIC_VOLUME_LSB,
	SET_ADV_PROGRAM_CONTROL0_MSB,
	SET_ADV_PROGRAM_CONTROL0_LSB,
	SET_DISPLAY_ENABLE,
	// SET_PAGE_ADDRESS,
	// SET_COLUMN_ADDRESS_MSB,
	// SET_COLUMN_ADDRESS_LSB
};

int COLUMN_START_ADDRESS = 30; // 0 - default (30), 1 - mirror horizontal (0)

uchar symbols[12][11] = {
	{0x00, 0x00, 0x20, 0x20, 0x20, 0xF8, 0x20, 0x20, 0x20, 0x00, 0x00}, // plus
	{0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00}, // minus
	{0xF8, 0xF8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xF8, 0xF8}, // num0
	{0xF8, 0xF8, 0x30, 0x30, 0x30, 0x30, 0xF0, 0xF0, 0x70, 0x70, 0x30}, // num1
	{0xF8, 0xF8, 0xC0, 0xC0, 0xC0, 0xF8, 0xF8, 0x18, 0x18, 0xF8, 0xF8}, // num2
	{0xF8, 0xF8, 0x18, 0x18, 0x18, 0xF8, 0xF8, 0x18, 0x18, 0xF8, 0xF8}, // num3
	{0x18, 0x18, 0x18, 0x18, 0xF8, 0xF8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8}, // num4
	{0xF8, 0xF8, 0x18, 0x18, 0x18, 0xF8, 0xF8, 0xC0, 0xC0, 0xF8, 0xF8}, // num5
	{0xF8, 0xF8, 0xD8, 0xD8, 0xD8, 0xF8, 0xF8, 0xC0, 0xC0, 0xF8, 0xF8}, // num6
	{0xC0, 0xC0, 0xC0, 0xC0, 0xE0, 0x70, 0x38, 0x18, 0x18, 0xF8, 0xF8}, // num7
	{0xF8, 0xF8, 0xD8, 0xD8, 0xD8, 0xF8, 0xD8, 0xD8, 0xD8, 0xF8, 0xF8}, // num8
	{0xF8, 0xF8, 0x18, 0x18, 0xF8, 0xF8, 0xD8, 0xD8, 0xD8, 0xF8, 0xF8}  // num9
};



#define NUM_KEYS    5
#define LED4        BIT5
#define LED5        BIT4
#define LED6        BIT3
#define LED7        BIT2
#define LED8        BIT1

struct Element* keypressed;

const struct Element* address_list[NUM_KEYS] = {&PAD1, &PAD2, &PAD3, &PAD4, &PAD5};
const uint8_t ledMask[NUM_KEYS] = {LED4, LED5, LED6, LED7, LED8};

int lenHelper(int number);
int abs(int number);
int _pow(int base, int exponent);
void printNumber(int number);

void Dogs102x6_clearScreen(void);
void Dogs102x6_setAddress(uchar pa, uchar ca);
void Dogs102x6_writeData(uchar* sData, uchar i);
void Dogs102x6_writeCommand(uchar* sCmd, uchar i);
void Dogs102x6_backlightInit(void);
void Dogs102x6_init(void);

void delay(long int value)
{
	volatile long int i = 0;
	volatile long int temp = 0;
	for (; i < value; i++)
	{
		temp++;
	}
}

void setupResetSignal() {
	P5DIR |= BIT7; // output direction
	P5OUT &= ~BIT7; // RST = 0
	P5OUT |= BIT7; // RST = 1
}


// just according to docs
void initPotentiometer() {
    P6DIR &= ~BIT5;
    P6SEL |= BIT5;
    P8DIR |= BIT0;
    P8SEL &= ~BIT0;
    P8OUT |= BIT0;
}


void initADC()
{
    ADC12CTL0 = ADC12ON;
    ADC12CTL1 = ADC12CSTARTADD_0 | // start writable memory address
            ADC12SHS_3 | // SHS - capture (start) signal source
            ADC12SSEL_0 | // clock signal
            ADC12CONSEQ_0; // consequence mode

    ADC12MCTL0 = ADC12EOS | ADC12INCH_5; // EOS - end of stream marker, 5ft input channel (potentiometer)
    ADC12IE = ADC12IE0; // enable interrupt
}


#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR() {
	Dogs102x6_clearScreen();
    __delay_cycles(1000000);
	// 1.5V - Reference Voltage
	// NADC = 4096 * (VIN – VR-) / (VR+ – VR- )
    printNumber(ADC12MEM0 * (1.5 / 4096) * 1000);
    ADC12CTL0 &= ~ADC12ENC; // disable measurements
}

void timer_init(void) {
    TB0CTL |= TBSSEL__ACLK;
    TB0CTL |= ID__1; // input clk divider /1

	//! review which to remove
    TB0CCR0 = 8000;
    TB0CCR1 = 7990;

    TA0R = 0; //! cadidate to remove (or replace with TB0R = 0)

    TB0CCTL1 |= OUTMOD_3;
    TB0CTL |= MC__UP;
}

// Taken from UCS_10
void SetVcoreUp(uint16_t level) {
    // Open PMM registers for write
    PMMCTL0_H = PMMPW_H;
    // Set SVS/SVM high side new level
    SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
    // Set SVM low side to new level
    SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
    // Wait till SVM is settled
    while ((PMMIFG & SVSMLDLYIFG) == 0)
        ;
    // Clear already set flags
    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
    // Set VCore to new level
    PMMCTL0_L = PMMCOREV0 * level;
    // Wait till new level reached
    if ((PMMIFG & SVMLIFG))
        while ((PMMIFG & SVMLVLRIFG) == 0)
            ;
    // Set SVS/SVM low side to new level
    SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
    // Lock PMM registers for write access
    PMMCTL0_H = 0x00;
}

void sensorsSetup() {
	    // uint8_t i;

        // /* Initialize IO */
        // P1DIR = 0xFF;
        // P2DIR = 0xFF;
        // P8DIR = 0xFF;
        // P1OUT = 0;
        // P2OUT = 0;
        // P8OUT = 0;

        /*
        *  Set DCO to 25Mhz and SMCLK to DCO. Taken from MSP430F55xx_UCS_10.c code
        *  example.
        */
        // Increase Vcore setting to level3 to support fsystem=25MHz
        // NOTE: Change core voltage one level at a time..
        SetVcoreUp(0x01);
        SetVcoreUp(0x02);
        SetVcoreUp(0x03);

        UCSCTL3 = SELREF_2;                       // Set DCO FLL reference = REFO
        UCSCTL4 |= SELA_2;                        // Set ACLK = REFO

        __bis_SR_register(SCG0);                  // Disable the FLL control loop
        UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
        UCSCTL1 = DCORSEL_7;
        // Select DCO range 50MHz operation
        UCSCTL2 = FLLD_1 + 762;                   // Set DCO Multiplier for 25MHz
                                                // (N + 1) * FLLRef = Fdco
                                                // (762 + 1) * 32768 = 25MHz
                                                // Set FLL Div = fDCOCLK/2
        __bic_SR_register(SCG0);                  // Enable the FLL control loop

        // Worst-case settling time for the DCO when the DCO range bits have been
        // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
        // UG for optimization.
        // 32 x 32 x 25 MHz / 32,768 Hz ~ 780k MCLK cycles for DCO to settle
        __delay_cycles(782000);
        // Loop until XT1,XT2 & DCO stabilizes - In this case only DCO has to stabilize
        do
        {
          UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
          // Clear XT2,XT1,DCO fault flags
          SFRIFG1 &= ~OFIFG;                      // Clear fault flags
        }
        while (SFRIFG1 & OFIFG);                   // Test oscillator fault flag

		TI_CAPT_Init_Baseline(&keypad);
   		TI_CAPT_Update_Baseline(&keypad, 5);
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

	Dogs102x6_init();
	Dogs102x6_backlightInit();
	Dogs102x6_clearScreen();

    P1DIR = 0xFF; // enable output mode for all P1 pins (done for LEDs disabling)
    P1OUT = 0; // set all P1 pins to zero values

    // UCSCTL3 = SELREF_2; // Set DCO FLL reference = REFO
    // UCSCTL4 |= SELA_2;

    // __bis_SR_register(SCG0);                  // Disable the FLL control loop
    // UCSCTL0 = 0x0000;
    // UCSCTL1 = DCORSEL_7;
    // UCSCTL2 = FLLD_1 + 762;

    // __bic_SR_register(SCG0);

	sensorsSetup();

    initPotentiometer();
    initADC();
    timer_init();

    __bis_SR_register(GIE);

    while (1) {
      P1OUT &= ~(LED4 + LED5 + LED6 + LED7 + LED8);
      keypressed = (struct Element *) TI_CAPT_Buttons(&keypad);
      if (keypressed == address_list[0]) {
              P1OUT |= BIT1;

              if (!(ADC12CTL1 & ADC12BUSY)) // if there is no active operation
				{
					ADC12CTL0 |= ADC12ENC;
				}
          }
      __delay_cycles(1000000);
    }


}

void printNumber(int number) {
	int nDigits = lenHelper(number);

	Dogs102x6_setAddress(0, COLUMN_START_ADDRESS);
	Dogs102x6_writeData(number > 0 ? symbols[0] : symbols[1], 11);

	int i = 0;
	int divider = _pow(10, nDigits - 1);

	number = abs(number);

	for (i = 1; i <= nDigits; i++) {
		int digit = number / divider;

		Dogs102x6_setAddress(i, COLUMN_START_ADDRESS);
		Dogs102x6_writeData(symbols[digit + 2], 11);

		number = number % divider;
		divider /= 10;
	}
}

int lenHelper(int number) {
	number = abs(number);

	if (number >= 100000) return 6;
	if (number >= 10000) return 5;
	if (number >= 1000) return 4;
	if (number >= 100) return 3;
	if (number >= 10) return 2;

	return 1;
}

int abs(int number) {
	return number > 0 ? number : number * (-1);
}

int _pow(int base, int exponent) {
	int i = 0;
	int result = base;

	for (i = 0; i < exponent - 1; i++) {
		result *= base;
	}

	return result;
}

void Dogs102x6_clearScreen(void)
{
	uchar LcdData[] = { 0x00 };
	uchar p, c;

	// 8 total pages in LCD controller memory
	for (p = 0; p < 8; p++)
	{
		Dogs102x6_setAddress(p, 0);
		// 132 total columns in LCD controller memory
		for (c = 0; c < 132; c++)
		{
			Dogs102x6_writeData(LcdData, 1);
		}
	}
}

void Dogs102x6_setAddress(uchar pa, uchar ca)
{
	uchar cmd[1];

	// 8 pages allowed
	if (pa > 7) {
		pa = 7;
	}

	// actual screen size smaller than controller
	if (ca > 101) {
		ca = 101;
	}

	cmd[0] = SET_PAGE_ADDRESS + (7 - pa); // (7 - pa) - inverse pages
	uchar H = 0x00;
	uchar L = 0x00;
	uchar ColumnAddress[2];

	L = (ca & 0x0F);
	H = (ca & 0xF0);
	H = (H >> 4);

	ColumnAddress[0] = SET_COLUMN_ADDRESS_LSB + L;
	ColumnAddress[1] = SET_COLUMN_ADDRESS_MSB + H;

	Dogs102x6_writeCommand(cmd, 1);
	Dogs102x6_writeCommand(ColumnAddress, 2);
}

void Dogs102x6_writeData(uchar* sData, uchar i)
{
	P7OUT &= ~CS;
	P5OUT |= CD; // 1 - data mode

	while (i){
		while (!(UCB1IFG & UCTXIFG)); // wait for controller interruption (1 - Buffer is available for writing)

		UCB1TXBUF = *sData;

		sData++;
		i--;
	}

	while (UCB1STAT & UCBUSY);
	// Dummy read to empty RX buffer and clear any overrun conditions
	UCB1RXBUF;

	P7OUT |= CS;
}

void Dogs102x6_writeCommand(uchar* sCmd, uchar i)
{
	P7OUT &= ~CS; // choose display
	P5OUT &= ~CD; // enter command mode

	while (i) {
		while (!(UCB1IFG & UCTXIFG)); // wait for controller interruption (1 - Buffer is available for writing)

		UCB1TXBUF = *sCmd;

		sCmd++;
		i--;
	}

	while (UCB1STAT & UCBUSY); // waiting for all data transmitted
	// Dummy read to empty RX buffer and clear any overrun conditions (? clear UCRXIFG)
	UCB1RXBUF;

	P7OUT |= CS; // choose controller
}

void Dogs102x6_backlightInit(void)
{
	P7DIR |= BIT6;
	P7OUT |= BIT6;
	P7SEL &= ~BIT6;
}

void Dogs102x6_init(void)
{
	setupResetSignal();

	P7DIR |= CS; // output direction

	P5DIR |= CD; // output direction
	P5OUT &= ~CD; // 0 - command mode

	P4SEL |= BIT1; // data transmission - LCD_SIMO (SIMO - Slave In, Master Out)
	P4DIR |= BIT1; // ?? - should be ignored

	P4SEL |= BIT3; // clock signal SCLK
	P4DIR |= BIT3; // ?? - should be ignored

	// UCSSEL__SMCLK - Select SMCLK as signal source
	// UCSWRST - enable software reset
	UCB1CTL1 = UCSSEL__SMCLK + UCSWRST;

	//3-pin, 8-bit SPI master
	UCB1CTL0 = UCCKPH + UCMSB + UCMST + UCMODE_0 + UCSYNC;

	// Frequency delimiters
	UCB1BR0 = 0x02;
	UCB1BR1 = 0;

	UCB1CTL1 &= ~UCSWRST; // disable software reset (used to change some CTL registers)
	UCB1IFG &= ~UCRXIFG;

	Dogs102x6_writeCommand(Dogs102x6_initMacro, 13);
}
