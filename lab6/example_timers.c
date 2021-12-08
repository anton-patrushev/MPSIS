#include <msp430.h>
#include <stdbool.h>
#include <math.h>
#include "CTS_Layer.h"

#define START_COLUMN_ADDRESS_LOW_PART   0x00
#define START_COLUMN_ADDRESS_HIGH_PART  0x10
#define START_PAGE_ADDRESS              0xB0
#define SET_SEG_DIRECTION               0xA0
#define SET_COM_DIRECTION               0xC0
#define SET_POWER_CONTROL               0x2F
#define SET_SCROLL_LINE                 0x40
#define SET_VLCD_RESISTOR_RATIO         0x27
#define SET_ELECTRONIC_VOLUME_MSB       0x81
#define SET_ELECTRONIC_VOLUME_LSB       0x0F
#define SET_ALL_PIXEL_OFF               0xA4
#define SET_INVERSE_DISPLAY             0xA6
#define SET_DISPLAY_ENABLE              0xAF
#define SET_LCD_BIAS_RATIO              0xA2
#define SET_ADV_PROGRAM_CONTROL0_MSB    0xFA
#define SET_ADV_PROGRAM_CONTROL0_LSB    0x90

#define CALADC12_15V_30C *((unsigned int *)0x1A1A) // Temperature Sensor Calibration-30 C
#define CALADC12_15V_85C *((unsigned int *)0x1A1C) // Temperature Sensor Calibration-85 C

unsigned char Dogs102x6_initMacro[] = {
    SET_SCROLL_LINE,
    SET_SEG_DIRECTION,
    SET_COM_DIRECTION,
    SET_ALL_PIXEL_OFF,
    SET_INVERSE_DISPLAY,
    SET_LCD_BIAS_RATIO,
    SET_POWER_CONTROL,
    SET_VLCD_RESISTOR_RATIO,
    SET_ELECTRONIC_VOLUME_MSB,
    SET_ELECTRONIC_VOLUME_LSB,
    SET_ADV_PROGRAM_CONTROL0_MSB,
    SET_ADV_PROGRAM_CONTROL0_LSB,
    SET_DISPLAY_ENABLE,
    START_PAGE_ADDRESS,
    START_COLUMN_ADDRESS_HIGH_PART,
    START_COLUMN_ADDRESS_LOW_PART
};

unsigned char symbols[][8] = {
        {0, 0, 0, 0x10, 0x10, 0xFF, 0x10, 0x10}, // plus
        {0, 0, 0, 0, 0, 1, 0, 0}, // plus
        {0, 0, 0, 0x10, 0x10, 0x10, 0x10, 0x10}, // minus
        {0, 0, 0, 0xFF, 1, 1, 1, 0xFF}, // 0
        {0, 0, 0, 1, 1, 1, 1, 1}, // 0
        {0, 0, 0, 0xFF, 2, 4, 8, 0x10}, // 1
        {0, 0, 0, 1, 0, 0, 0, 0}, // 1
        {0, 0, 0, 0x1F, 0x21, 0x41, 0x81, 1}, // 2
        {0, 0, 0, 1, 1, 1, 1, 1}, // 2
        {0, 0, 0, 0xFF, 0x11, 0x11, 0x11, 0x11}, // 3
        {0, 0, 0, 1, 1, 1, 1, 1}, // 3
        {0, 0, 0, 0xFF, 0x10, 0x10, 0x10, 0x1F}, // 4
        {0, 0, 0, 1, 0, 0, 0, 0}, // 4
        {0, 0, 0, 0xF1, 0x11, 0x11, 0x11, 0x1F}, // 5
        {0, 0, 0, 1, 1, 1, 1, 1}, // 5
        {0, 0, 0, 0xF1, 0x11, 0x11, 0x11, 0x1F}, // 6
        {0, 0, 0, 1, 1, 1, 1, 1}, // 6
        {0, 0, 0, 0x1F, 0x21, 0x41, 0x81, 1}, // 7
        {0, 0, 0, 0, 0, 0, 0, 1}, // 7
        {0, 0, 0, 0xFF, 0x11, 0x11, 0x11, 0xFF}, // 8
        {0, 0, 0, 1, 1, 1, 1, 1}, // 8
        {0, 0, 0, 0xFF, 0x11, 0x11, 0x11, 0x1F}, // 9
        {0, 0, 0, 1, 1, 1, 1, 1} // 9
};

int equivalent_values[] = { 4571, 2286, 1142, 571, 286, 143, 71 };

unsigned char bits[] = { BIT6, BIT5, BIT4, BIT3, BIT2, BIT1, BIT0 };

void init();

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    init();

    P1DIR = 0xFF;
    P2DIR = 0xFF;
    P8DIR = 0xFF;
    P1OUT = 0;
    P2OUT = 0;
    P8OUT = 0;

    SetVcoreUp(0x01);
    SetVcoreUp(0x02);
    SetVcoreUp(0x03);

    UCSCTL3 = SELREF_2;
    UCSCTL4 |= SELA_2;

    __bis_SR_register(SCG0);
    UCSCTL0 = 0x0000;
    UCSCTL1 = DCORSEL_7;
    UCSCTL2 = FLLD_1 + 762;

    __bic_SR_register(SCG0);

    __delay_cycles(782000);


    TI_CAPT_Init_Baseline(&slider);
    TI_CAPT_Update_Baseline(&slider, 1);
    setup_ADC();

    if (!(ADC12CTL1 & ADC12BUSY))
    {
        TA0CTL = TASSEL__SMCLK | MC__UP | ID__1 | TACLR;
        long int second = 32768;
        long int period = second / 2;
        TA0CCR0 = second;
        TA0CCR1 = period;
        TA0CCTL1 = OUTMOD_3;
        ADC12CTL0 |= ADC12ENC;
    }

    while (true)
    {

      struct Element * keypressed = (struct Element *) TI_CAPT_Slider(&slider);
      if (keypressed < 65535)
      {
          P1OUT |= BIT3;
      }
      else
      {
          P1OUT &= ~BIT3;
      }
      __delay_cycles(900000);
    }


    __no_operation();
    return 0;
}



void SetVcoreUp(uint16_t level)
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



void init()
{
    P1DIR |= BIT3;
    P1OUT &= ~BIT3;
    Dogs102x6_init();
    Dogs102x6_clearScreen();
    __bis_SR_register(GIE);
}

void Dogs102x6_init()
{
    P5DIR |= BIT7;
    P5OUT = BIT7;

    P7DIR |= BIT4;

    P5DIR |= BIT6;
    P5OUT &= ~BIT6;

    P4SEL |= BIT1;
    P4DIR |= BIT1;

    P4SEL |= BIT3;
    P4DIR |= BIT3;

    P7DIR |= BIT6;
    P7OUT = BIT6;

    UCB1CTL1 = UCSSEL__SMCLK + UCSWRST;
    UCB1CTL0 = UCCKPH + UCMSB + UCMST + UCSYNC + UCMODE_0;
    UCB1BR0 = 3;
    UCB1BR1 = 0;
    UCB1CTL1 &= ~UCSWRST;
    UCB1IFG &= ~UCRXIFG;
    Dogs102x6_writeCommand(Dogs102x6_initMacro, 13);
}

void Dogs102x6_writeCommand(unsigned char *sCmd, unsigned char i)
{
    // CS Low
    P7OUT &= ~BIT4;

    // CD Low
    P5OUT &= ~BIT6;
    while (i)
    {
        // USCI_B1 TX buffer ready?
        while (!(UCB1IFG & UCTXIFG)) ;

        // Transmit data
        UCB1TXBUF = *sCmd;

        // Increment the pointer on the array
        sCmd++;

        // Decrement the Byte counter
        i--;
    }

    // Wait for all TX/RX to finish
    while (UCB1STAT & UCBUSY);

    // Dummy read to empty RX buffer and clear any overrun conditions
    UCB1RXBUF;

    // CS High
    P7OUT |= BIT4;
}

void Dogs102x6_writeData(unsigned char *sData, unsigned char i)
{
    // CS Low
    P7OUT &= ~BIT4;
    //CD High
    P5OUT |= BIT6;

    while (i)
    {
        // USCI_B1 TX buffer ready?
        while (!(UCB1IFG & UCTXIFG)) ;

        // Transmit data and increment pointer
        UCB1TXBUF = *sData++;

        // Decrement the Byte counter
        i--;
    }

    // Wait for all TX/RX to finish
    while (UCB1STAT & UCBUSY) ;

    // Dummy read to empty RX buffer and clear any overrun conditions
    UCB1RXBUF;

    // CS High
    P7OUT |= BIT4;
}

void Dogs102x6_clearScreen()
{
    unsigned char LcdData[] = { 0 };
    unsigned char p, c;

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

void Dogs102x6_setAddress(unsigned char pa, unsigned char ca)
{
    if (pa > 7)
    {
        pa %= 8;
    }

    if (ca > 101)
    {
        ca %= 102;
    }

    unsigned char page_address = START_PAGE_ADDRESS + pa;
    unsigned char H = 0;
    unsigned char L = 0;
    unsigned char column_address[] = { START_COLUMN_ADDRESS_HIGH_PART, START_COLUMN_ADDRESS_LOW_PART };

    L = (ca & 0x0F);
    H = ((ca & 0xF0) >> 4);

    column_address[0] = START_COLUMN_ADDRESS_LOW_PART + L;
    column_address[1] = START_COLUMN_ADDRESS_HIGH_PART + H;

    Dogs102x6_writeCommand(&page_address, 1);
    Dogs102x6_writeCommand(column_address, 2);
}

void Dogs102x6_printNumber(int num, int column_start_address)
{
    int nDigits = get_number_length(num);
    int temp = num;

    int i = 0;
    temp = abs(temp);

    for (i = 0; i < nDigits; i++) {
        int digit = temp % 10;

        Dogs102x6_setAddress(START_PAGE_ADDRESS, column_start_address + i * 8);
        Dogs102x6_writeData(symbols[digit * 2 + 3], 8);
        Dogs102x6_setAddress(START_PAGE_ADDRESS + 1, column_start_address + i * 8);
        Dogs102x6_writeData(symbols[digit * 2 + 4], 8);

        temp /= 10;
    }

    if (num > 0)
    {
        Dogs102x6_setAddress(START_PAGE_ADDRESS, column_start_address + nDigits * 8);
        Dogs102x6_writeData(symbols[0], 8);
        Dogs102x6_setAddress(START_PAGE_ADDRESS + 1, column_start_address + nDigits * 8);
        Dogs102x6_writeData(symbols[1], 8);
    }
    else if (num < 0)
    {
        Dogs102x6_setAddress(START_PAGE_ADDRESS, column_start_address + nDigits * 8);
        Dogs102x6_writeData(symbols[2], 8);
    }
}

int get_number_length(int num) {
    num = num < 0 ? num * -1 : num;
    int len = 0;
    while (num != 0)
    {
        num /= 10;
        len++;
    }
    return len;
}

void setup_ADC()
{
    P8DIR |= BIT1;
    P8OUT &= ~BIT1;

    REFCTL0 &= ~REFMSTR;

    ADC12CTL0 =
            ADC12SHT0_8
            + ADC12REFON
            + ADC12ON;


    ADC12CTL1 = ADC12CONSEQ_1 + ADC12SHS_1
            + ADC12SSEL_0;

    ADC12CTL1 &= ~ADC12SHP;

    ADC12MCTL0 =
            ADC12SREF_1
             + ADC12INCH_10;

    ADC12IE = ADC12IE0;

    __delay_cycles(100);

    ADC12CTL0 |= ADC12ENC;
}

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR() {

    TA0CTL &= ~MC__UP;
    TA0CTL |= MC__STOP | TACLR;
    TA0CCTL1 &= ~BIT2;

    unsigned short int result = ADC12MEM0 & 0x0FFF;
    ADC12MEM0 = 0;

    long int res = result;



    res = ((res - CALADC12_15V_30C )* (85 - 30))/ (CALADC12_15V_85C - CALADC12_15V_30C);

    Dogs102x6_clearScreen();
    Dogs102x6_printNumber(res + 30, 30);

    ADC12CTL0 &= ~ADC12ENC;
}
