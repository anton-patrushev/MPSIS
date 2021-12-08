#include <msp430.h>
#include "CTS_Layer.h"
#include <math.h>


typedef unsigned char uint8_t;

#define CD              BIT6
#define CS              BIT4

#define SET_COLUMN_ADDRESS_LSB        0x00
#define SET_COLUMN_ADDRESS_MSB        0x10
#define SET_PAGE_ADDRESS              0xB0

int COLUMN_START_ADDRESS = 0;

void initADC();
void Dogs102x6_setAddress(uint8_t pa, uint8_t ca);
void Dogs102x6_writeData(uint8_t* sData, uint8_t i);
void Dogs102x6_writeCommand(uint8_t* sCmd, uint8_t i);
void setUp(uint16_t level);
void clearDisplay();
void initDisplay();


int getNumberLength(long int number);
void printNumber(long int angle);

uint8_t initCommands[] = {0x40, 0xA1, 0xC0, 0xA4, 0xA6, 0xA2, 0x2F, 0x27, 0x81, 0x10, 0xFA, 0x90, 0xAF};

uint8_t symbols[12][13] = {
		{0x20, 0x20, 0x20, 0x20, 0x20, 0xF8, 0xF8, 0xF8, 0x20, 0x20, 0x20, 0x20, 0x20}, // plus
		{0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00}, // minus
		{0x70, 0xF8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xF8, 0x70}, // num0
		{0x18, 0x38, 0x78, 0xD8, 0x98, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, // num1 +
		{0x70, 0xF8, 0xD8, 0xD8, 0x18, 0x18, 0x38, 0x70, 0xE0, 0xC0, 0xC0, 0xF8, 0xF8}, // num2 +
		{0x70, 0xF8, 0xD8, 0x18, 0x18, 0xF8, 0xF8, 0x18, 0x18, 0x18, 0xD8, 0xF8, 0x70}, // num3 +
		{0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xD8, 0xF8, 0xF8, 0x18, 0x18, 0x18, 0x18, 0x18}, // num4 +
		{0xF8, 0xF8, 0xC0, 0xC0, 0xC0, 0xF0, 0xF8, 0xD8, 0x18, 0x18, 0xD8, 0xF8, 0x70}, // num5 +
		{0x70, 0xF8, 0xD8, 0xC0, 0xC0, 0xF0, 0xF8, 0xD8, 0xD8, 0xD8, 0xD8, 0xF8, 0x70}, // num6 +
		{0xF8, 0xF8, 0x18, 0x18, 0x18, 0x38, 0x70, 0xE0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0}, // num7 +
		{0x70, 0xF8, 0xD8, 0xD8, 0xD8, 0xF8, 0x70, 0xD8, 0xD8, 0xD8, 0xD8, 0xF8, 0x70}, // num8 +
		{0x70, 0xF8, 0xD8, 0xD8, 0xD8, 0xF8, 0x78, 0x18, 0x18, 0x18, 0x38, 0x70, 0xE0} 	// num9 +
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

void Dogs102x6_setAddress(uint8_t pa, uint8_t ca)
{
	uint8_t cmd[1];

	if (pa > 7)
	{
		pa = 7;
	}

	if (ca > 101)
	{
		ca = 101;
	}

	cmd[0] = SET_PAGE_ADDRESS + (7 - pa);
	uint8_t H = 0x00;
	uint8_t L = 0x00;
	uint8_t ColumnAddress[] = { SET_COLUMN_ADDRESS_MSB, SET_COLUMN_ADDRESS_LSB };

	L = (ca & 0x0F);
	H = (ca & 0xF0);
	H = (H >> 4);

	ColumnAddress[0] = SET_COLUMN_ADDRESS_LSB + L;
	ColumnAddress[1] = SET_COLUMN_ADDRESS_MSB + H;

	Dogs102x6_writeCommand(cmd, 1);
	Dogs102x6_writeCommand(ColumnAddress, 2);
}

int getNumberLength(long int number) {
	int length = 0;
	number = abs(number);

	if(number == 0) {
		return 1;
	}

	while(number) {
		number /= 10;
		length++;
	}

	return length;
}

void printNumber(long int number) {
	int nDigits = getNumberLength(number);

	Dogs102x6_setAddress(0, COLUMN_START_ADDRESS);
	Dogs102x6_writeData(number > 0 ? symbols[0] : symbols[1], 13);

	int i = 0;
	long int divider = pow(10, nDigits - 1);

	number = abs(number);

	for (i = 1; i <= nDigits; i++) {
		int digit = number / divider;

		Dogs102x6_setAddress(i, COLUMN_START_ADDRESS);
		Dogs102x6_writeData(symbols[digit + 2], 13);

		number = number % divider;
		divider /= 10;
	}
}

void initPotentiometer() {
    P6DIR &= ~BIT5; //����� �5 �����������
    P6SEL |= BIT5;
    P8DIR |= BIT0;// ����� �� �������� �������� �����������
    P8SEL &= ~BIT0;
    P8OUT |= BIT0;
}


void initADC()
{
    ADC12CTL0 = ADC12ON;
    ADC12CTL1 = ADC12CSTARTADD_0 | //��������� ����� ������ �����������
            ADC12SHS_0 | // �������� - ������������
            ADC12SSEL_0 |	// ������������ ADC12OSC
            ADC12CONSEQ_2; //���������������� �����

    ADC12MCTL0 = ADC12EOS | ADC12INCH_5;
    ADC12IE = ADC12IE0;//���������� ���������� �� ���������������� �����
}


#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR() {
    clearDisplay();
    __delay_cycles(1000000);
    printNumber(ADC12MEM0 * (1.5 / 4096) * 1000);
    ADC12CTL0 &= ~ADC12ENC;
}


int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    initDisplay();

    P1DIR = 0xFF;
    P1OUT = 0;

    UCSCTL3 = SELREF_2;
    UCSCTL4 |= SELA_2;

    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;
    UCSCTL1 = DCORSEL_7;
    UCSCTL2 = FLLD_1 + 762;

    __bic_SR_register(SCG0);

    TI_CAPT_Init_Baseline(&keypad);
    TI_CAPT_Update_Baseline(&keypad, 5);

    initPotentiometer();
    initADC();

    __bis_SR_register(GIE);

    while (1) {
      P1OUT &= ~(LED4 + LED5 + LED6 + LED7 + LED8);
      keypressed = (struct Element *) TI_CAPT_Buttons(&keypad);
      if (keypressed == address_list[4]) {
              P1OUT |= BIT5;

              if (!(ADC12CTL1 & ADC12BUSY)) {
                    ADC12CTL0 |= ADC12ENC; //���������� ���������
                    ADC12CTL0 |= ADC12SC;
                    __delay_cycles(1000);
                    ADC12CTL0 &= ~ADC12SC;
                }
          }
      __delay_cycles(1000000);
    }
}


void clearDisplay()
{
    uint8_t pageCommand;
    uint8_t colCommands[2];
    uint8_t colLSB = 0;
    uint8_t colMSB = 0;

    uint8_t data = 0x00;

    int p, c;
    for(p = 0; p < 8; p++) {
        pageCommand = 0b10110000 + p;
        Dogs102x6_writeCommand(&pageCommand, 1);

        colCommands[0] = 0b00000000 + colLSB;
        colCommands[1] = 0b00010000 + colMSB;
        Dogs102x6_writeCommand(colCommands, 2);

        for(c = 0; c < 132; c++) {
            Dogs102x6_writeData(&data, 1);
        }
    }
}

void initDisplay() {
    P5DIR |= BIT7;
    P5OUT &= ~BIT7;
    P5OUT |= BIT7;

    __delay_cycles(550);

    P7DIR |= BIT4;
    P7OUT &= ~BIT4;
    P5DIR |= BIT6;
    P5OUT &= ~BIT6;
    P4SEL |= BIT1 | BIT3;
    P4DIR |= BIT1 | BIT3;
    P7DIR |= BIT6;
    P7OUT |= BIT6;
    P7SEL &= ~BIT6;

    P7OUT |= BIT4;

    UCB1CTL1 |= UCSWRST;

    UCB1CTL0 = (UCCKPH  & ~UCCKPL | UCMSB | UCMST | UCSYNC | UCMODE_0);

    UCB1CTL1 = UCSSEL_2 | UCSWRST;

    UCB1BR0 = 0x01;
    UCB1BR1 = 0;

    UCB1CTL1 &= ~UCSWRST;
    UCB1IFG &= ~UCRXIFG;
    Dogs102x6_writeCommand(initCommands, 13);
    setUp(0x01); setUp(0x02); setUp(0x03);

    clearDisplay();
}


void Dogs102x6_writeData(uint8_t* sData, uint8_t i)
{
    P7OUT &= ~CS;
    P5OUT |= CD;

    while (i)
    {
        while (!(UCB1IFG & UCTXIFG));

        UCB1TXBUF = *sData;

        sData++;
        i--;
    }

    while (UCB1STAT & UCBUSY);

    UCB1RXBUF;

    P7OUT |= CS;
}

void Dogs102x6_writeCommand(uint8_t* sCmd, uint8_t i)
{
    P7OUT &= ~CS;
    P5OUT &= ~CD;

    while (i)
    {
        while (!(UCB1IFG & UCTXIFG));

        UCB1TXBUF = *sCmd;

        sCmd++;
        i--;
    }

    while (UCB1STAT & UCBUSY);

    UCB1RXBUF;

    P7OUT |= CS;
}


void setUp(uint16_t level)
{
    PMMCTL0_H = PMMPW_H;

    SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
    SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;

    while ((PMMIFG & SVSMLDLYIFG) == 0);

    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
    PMMCTL0_L = PMMCOREV0 * level;

    if ((PMMIFG & SVMLIFG))
        while ((PMMIFG & SVMLVLRIFG) == 0);
    SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
    PMMCTL0_H = 0x00;
}
