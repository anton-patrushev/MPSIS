#include "HAL_Dogs102x6.h"
#include "structure.h"

#include "CTS_Layer.h"
#include "structure.h"
#include <stdint.h>

#include <msp430.h>

#define FONT_COLUMNS 7

typedef struct {
    char name;
    uint8_t shape[7];
} Symbol;

const Symbol SYMBOLS[] = {

    {   //0000  0000  0010  1011  0010  0000  0000
        '+',
        { 0x00, 0x00, 0x04, 0x0E, 0x04, 0x00, 0x00 }
    },
    {   //0000  0000  0000  1011  0000  0000  0000
        '-',
        { 0x00, 0x00, 0x00, 0x0E, 0x00, 0x00, 0x00 }
    },
    {
        '0',
        { 0x0f, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0f }
    },
    {
        '1',
        { 0x01, 0x03, 0x05, 0x09, 0x01, 0x01, 0x01 }
    },
    {
        '2',
        { 0x0f, 0x01, 0x01, 0x0f, 0x08, 0x08, 0x0f }
    },
    {
        '3',
        { 0x0f, 0x01, 0x01, 0x07, 0x01, 0x01, 0x0f }
    },
    {
        '4',
        { 0x09, 0x09, 0x09, 0x0f, 0x01, 0x01, 0x01 }
    },
    {
        '5',
        { 0x0f, 0x08, 0x08, 0x0f, 0x01, 0x01, 0x0f }
    },
    {
        '6',
        { 0x0f, 0x08, 0x08, 0x0f, 0x09, 0x09, 0x0f }
    },
    {
        '7',
        { 0x0f, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04 }
    },
    {
        '8',
        { 0x0f, 0x09, 0x09, 0x0f, 0x09, 0x09, 0x0f }
    },
    {
        '9',
        { 0x0f, 0x09, 0x09, 0x0f, 0x01, 0x01, 0x0f }
    },
};
const unsigned int SYMBOLS_COUNT = sizeof(SYMBOLS) / sizeof(Symbol);
extern const Symbol SYMBOLS[];
extern const unsigned int SYMBOLS_COUNT;

#define FRAME_PAGES 8
#define FRAME_COLUMNS 102
// set column CA=[0, 131]
#define LCD_CMD_COL_LOW  0x00 // | CA[3..0]
#define LCD_CMD_COL_HIGH 0x10 // | CA[7..4]
#define LCD_CMD_PWR 0x28
#define LCD_CMD_PWR__AMP 0x01
#define LCD_CMD_PWR__REG 0x02
#define LCD_CMD_PWR__REP 0x04
#define LCD_CMD_SCROLL 0x40 // | SL[5..0]
#define LCD_CMD_PAGE 0xB0 // | PA[3..0]
#define LCD_CMD_BRIGHT 0x20 // | PC[5..3]
#define LCD_CMD_CONTRAST_1 0x81
#define LCD_CMD_CONTRAST_2 0x00 // | PM[5..0]
#define LCD_CMD_ALL_NO 0xA4
#define LCD_CMD_INV_NO 0xA6
#define LCD_CMD_EN_ON 0xAE | 0x01
#define LCD_CMD_COL_ORDER_NORMAL 0xA0
#define LCD_CMD_ROW_ORDER_NORMAL 0xC0
#define LCD_CMD_PWR_OFFSET_1_9 0xA2
#define LCD_CMD_EXT_1 0xFA
#define LCD_CMD_EXT_2 0x10
#define LCD_CMD_EXT_2__TEMP       0x80
#undef DIR
#undef OUT
#define __PORT(num, func) P ## num ## func
#define _PORT(num, func) __PORT(num, func)
#define PORT(name, func) _PORT(name ## _PORT, func)
#define _PIN(pos) (1 << pos)
#define PIN(name) _PIN(name ## _PIN)
#undef TOGGLE
#define GET(name, func)    PORT(name, func) &   PIN(name)
#define SET(name, func)    PORT(name, func) |=  PIN(name);
#define RESET(name, func)  PORT(name, func) &= ~PIN(name);
#define TOGGLE(name, func) PORT(name, func) ^=  PIN(name);

void led_init(void);
void led_enable(void);
void led_disable(void);
void timer_init(void);
void lcd_init(void);
void lcd_sync(int columns);
void lcd_clear(void);
void lcd_draw_symbol(char symbol, int position, int offset);
void lcd_draw_int(int value, int offset);
void pad_init(void);
void pad_loop(void);

static uint8_t frame[FRAME_PAGES][FRAME_COLUMNS];

static uint8_t initCmds[] = {
    LCD_CMD_SCROLL,
    LCD_CMD_COL_ORDER_NORMAL,
    LCD_CMD_ROW_ORDER_NORMAL,
    LCD_CMD_ALL_NO,
    LCD_CMD_INV_NO,
    LCD_CMD_PWR_OFFSET_1_9,
    LCD_CMD_PWR | LCD_CMD_PWR__AMP | LCD_CMD_PWR__REG | LCD_CMD_PWR__REP,
    LCD_CMD_BRIGHT | 0x07,
    LCD_CMD_CONTRAST_1,
    LCD_CMD_CONTRAST_2 | 0x11,
    LCD_CMD_EXT_1,
    LCD_CMD_EXT_2 | LCD_CMD_EXT_2__TEMP,
    LCD_CMD_EN_ON,
};
static const int initCmdsCount = sizeof(initCmds) / sizeof(uint8_t);

void led_init(void)
{
    P1DIR |= BIT1;
    P1DIR |= BIT2;
    P1DIR |= BIT3;
    P1DIR |= BIT4;
    P1DIR |= BIT5;
    led_disable();
}

void led_enable(void)
{
    P1OUT |= BIT1;
    P1OUT |= BIT2;
    P1OUT |= BIT3;
    P1OUT |= BIT4;
    P1OUT |= BIT5;
}

void led_disable(void)
{
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT2;
    P1OUT &= ~BIT3;
    P1OUT &= ~BIT4;
    P1OUT &= ~BIT5;
}

void lcd_init(void)
{
    P4SEL |= BIT1; //устанавливаем на периферию SIMO
    P4SEL |= BIT3; //устанавливаем на периферию CLK

    P5DIR |= BIT6; //устанавливаем направление пина на выход
    P5DIR |= BIT7;

    P7DIR |= BIT4;
    P7DIR |= BIT6;

    P5OUT |= BIT7; // чтобы не было сброса
    P5OUT &= ~BIT6; // для отправки команд
    P7OUT |= BIT4; // если 0, то устройство выбрано
    P7OUT |= BIT6; // включение подсветки

    // configure SPI
    UCB1CTL1 = UCSWRST // USCI Software Reset(разрешение программного сброса)
           | UCSSEL__SMCLK; // Clock Source: SMCLK
    UCB1CTL0 = UCSYNC // Sync-Mode  0:UART-Mode / 1:SPI-Mode Режим: синхронный - 1
            | UCMST //  Sync. Mode: Master Select Режим: Master
            | UCMSB  // Порядок передачи:- MSB(старший значащий бит)
            | UCMODE_0 // Sync. Mode: USCI Mode: 0 (Синхронный режим: 00 – 3pin SPI)
            | UCCKPL; // Sync. Mode: Clock Polarity
    UCB1BR0 = 0x01; // UCA0BR0 - младший байт делителя частоты
    UCB1BR1 = 0; // старший байт делителя частоты
    UCB1CTL1 &= ~UCSWRST; // USCI Software Reset(запрет программного сброса)

    // send init commands
    Dogs102x6_writeCommand(initCmds, initCmdsCount);

    lcd_clear();
    lcd_sync(0);
}

void lcd_clear(void)
{
    int page, column;
    for (page = 0; page < FRAME_PAGES; page++)
    {
        for (column = 0; column < FRAME_COLUMNS; column++)
        {
            frame[page][column] = 0;
        }
    }
}

static inline void set_page(uint8_t page) {
    uint8_t cmd[1];
    cmd[0] = LCD_CMD_PAGE | page;
    Dogs102x6_writeCommand(cmd, 1);
}

static inline void set_column(uint8_t column) {
    uint8_t cmd[2];
    cmd[0] = LCD_CMD_COL_LOW | (column & 0x0F);
    cmd[1] = LCD_CMD_COL_HIGH | ((column & 0xF0) >> 4);
    Dogs102x6_writeCommand(cmd, 2);
}

void lcd_sync(int columns)
{
    uint8_t page, column;
    const int offset = columns ? (FRAME_COLUMNS - FONT_COLUMNS * columns) : 0;
    for (page = 0; page < FRAME_PAGES; page++)
    {
        set_page(page);
        set_column(30+offset);

        for (column = 0+offset; column < FRAME_COLUMNS; column++)
        {
            Dogs102x6_writeData(&frame[page][column], 1);
        }
    }
}

#define DRAW_DOT(x, y) frame[y / 8][x] |= 1 << (y % 8)

char flip(char c){
    char fliped = 0;

    fliped = ((c & 1) ? 8 : 0) +
             ((c & 2) ? 4 : 0) +
             ((c & 4) ? 2 : 0) +
             ((c & 8) ? 1 : 0);

    return fliped;
}

Symbol reverse(Symbol sym){
    Symbol newOne;
    int i = 0;
    newOne.name = sym.name;
    for(i = 0; i < 7; i++){
        newOne.shape[i] = flip(sym.shape[i]);
    }
    return newOne;
}

void lcd_draw_symbol(char symbol, int position, int offset) {
    if (position >= FRAME_PAGES)
        return;

    uint8_t page = (uint8_t) position;

    int s;
    for (s = 0; s < SYMBOLS_COUNT; s++) {
        if (SYMBOLS[s].name == symbol) {
            break;
        }
    }

    if (s == SYMBOLS_COUNT) // not found
        return;

    const Symbol* S = &SYMBOLS[s];

    Symbol sym = reverse(*S);
    int i;
    //8659
    for (i = 0; i < sizeof(S->shape); i++) {
        frame[page][FRAME_COLUMNS-1-abs(i-6)-offset] |= sym.shape[i];
    }
}

void lcd_draw_int(int val, int offset)
{
    lcd_clear(); // чистим экран

        int sign = val >= 0; // выводим знак
        lcd_draw_symbol(sign ? '+' : '-', 0, FRAME_COLUMNS - 8);

        char digits[5];
        unsigned int positions = 0;

        short value = val;
        if (value < 0) value = -value;

        do {
            unsigned char offset = value % 10;
            digits[positions] = '0' + offset;
            value /= 10;
            positions++;
        } while (value);

        int i = 0;
        for (i = 0; i < positions; i++)
        {
            lcd_draw_symbol(digits[positions - 1 - i], i + 1, FRAME_COLUMNS - 8);
        }

        lcd_sync(0);
}

void timer_init(void) {
    TB0CTL |= TBSSEL__ACLK;
    TB0CTL |= ID__1;
    TB0CCR0 = 8000;
    TB0CCR1 = 7990;
    TA0R = 0;
    TB0CCTL1 |= OUTMOD_3;
    TB0CTL |= MC__UP;
}

void pad_init(void) {
    TI_CAPT_Init_Baseline(&keypad);
    TI_CAPT_Update_Baseline(&keypad, 5);
}

#define NUM_KEYS    5
#define LED4        BIT5
#define LED5        BIT4
#define LED6        BIT3
#define LED7        BIT2
#define LED8        BIT1

struct Element* keypressed;

const struct Element* address_list[NUM_KEYS] = {
        &PAD1,
        &PAD2,
        &PAD3,
        &PAD4,
        &PAD5
};

void SetupTimer()
{
    // doesn't affect sample/conversion!!!, only triggers the beginning

    // setup timer
    TA0CTL = TASSEL__SMCLK | MC__UP | ID__1 | TACLR;     // SMCLK, UP-mode
    long int second = 32768;
    long int period = second / 2;
    TA0CCR0 = second;
    TA0CCR1 = period;
    TA0CCTL1 = OUTMOD_3;
}

void SetVcoreUp(uint16_t level)
{
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

void pad_loop(void) {
    uint8_t i;

        /* Initialize IO */
        P1DIR = 0xFF;
        P2DIR = 0xFF;
        P8DIR = 0xFF;
        P1OUT = 0;
        P2OUT = 0;
        P8OUT = 0;

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

        /*  establish baseline */
        TI_CAPT_Init_Baseline(&keypad);
        TI_CAPT_Update_Baseline(&keypad, 5);
//
//        SetupControllButton();
//        // SetupLCD();
//        SetupADC();

          __bis_SR_register(GIE);

        while (1)
        {
          led_disable();
          keypressed = (struct Element *) TI_CAPT_Buttons(&keypad);
          //TI_CAPT_Custom(&keypad,dCnt);
          __no_operation();
          if (keypressed)
          {
              for (i = 0; i < NUM_KEYS; i++)
              {
                  if (keypressed == address_list[3])
                  {
                      led_enable();

                      if (!(ADC12CTL1 & ADC12BUSY)) // if there is no active operation
                            {
                                SetupTimer();
                                ADC12CTL0 |= ADC12ENC;
                            }
                  }
              }
          }
          /* 32ms delay. This delay can be replaced with other application tasks. */
          __delay_cycles(900000);
        } // while loop

}


/*
 * The element defines the input of the comparator mux, the maximum response,
 * and the threshold. The threshold and maxResponse values are based upon the gate time, defined
 * in the keypad sensor definition.  In this example the gate time is 1.5ms.
 */
const struct Element PAD1 = {
    .inputBits = CBIMSEL_0,
    .maxResponse = 100,
    .threshold = 50
};

const struct Element PAD2 = {
    .inputBits = CBIMSEL_1,
    .maxResponse = 390,
    .threshold = 195
};

const struct Element PAD3 = {
    .inputBits = CBIMSEL_2,
    .maxResponse = 340,
    .threshold = 170
};

const struct Element PAD4 = {
    .inputBits = CBIMSEL_3,
    .maxResponse = 500,
    .threshold = 230
};

const struct Element PAD5 = {
    .inputBits = CBIMSEL_4,
    .maxResponse = 400,
    .threshold = 200
};

/*
 *  The sensor defines the grouping of elements, the method to measure change in
 *  capacitance, and the measurement time of each element.
 */
const struct Sensor keypad = {
      .halDefinition = fRO_COMPB_TA1_TA01,
      .numElements = 5,
      .baseOffset = 0,
      .cbpdBits = 0x001F, //BIT0+BIT1+BIT2+...BITE+BITF
      // Pointer to elements
      .arrayPtr[0] = &PAD1,  // point to first element
      .arrayPtr[1] = &PAD2,
      .arrayPtr[2] = &PAD3,
      .arrayPtr[3] = &PAD4,
      .arrayPtr[4] = &PAD5,

      .cboutTAxDirRegister = (unsigned char *)&P1DIR,  // PxDIR
      .cboutTAxSelRegister = (unsigned char *)&P1SEL,  // PxSEL
      .cboutTAxBits = BIT6, // P1.6
      // Timer Information
      .measGateSource = TIMER_ACLK,      //  ACLK
      .sourceScale = TIMER_SOURCE_DIV_0, // ACLK/1
      //.measGateSource= GATE_WDT_ACLK,     //  0->SMCLK, 1-> ACLK
      .accumulationCycles = 50 //
};

#pragma vector = ADC12_VECTOR
__interrupt void handleADC(void) {
    // https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/452923?ADC-temperature-sensor
    ADC12CTL0 |= ADC12ENC + ADC12SC;

    int temp = ADC12MEM0;
    temp = ((temp - 1855) * 667) / 1024;
    lcd_clear();
    lcd_draw_int(temp, 0);
    lcd_sync(3);

    ADC12IFG &= ~ADC12IFG0;
}

void adc_init(void) {
    ADC12CTL0 = ADC12ON | // включение АЦП
             ADC12REFON | // включение опорного генератора
             ADC12SHT0_1; // длительность выборки
    ADC12CTL1 = ADC12SHP | //выбор источника сигнала (импульсный(по варианту))
             ADC12SSEL_1 | // выбор тактового сигнала
           ADC12CONSEQ_0 | // режим
           ADC12SHS_3;  // источник сигнала запуска
    ADC12CTL2 = ADC12RES_1 | //точность ADC12+ Resolution : 10 Bit
                   ADC12SR;  // буффер работает на частоте 50 выборок/c ADC12+ Sampling Rate
    ADC12MCTL0 = ADC12INCH_10 | // выбор входного сигнала Input Channel 10
                  ADC12SREF_1 | // выбор пары опорных напряжений  Select Reference 1
                  ADC12EOS; // маркер конца последовательностей
    ADC12IFG &= ~ADC12IFG0; // флаг запроса на прерывание
    ADC12IE |= ADC12IE0; // разрешить прерывание по соответствующему флагу

    REFCTL0 &= ~REFMSTR; // датчик температуры
    ADC12CTL0 |= ADC12ENC; // разрешение измерения Enable Conversion

    P6SEL |= BIT5;
    P6DIR &= ~BIT5;

    P8DIR |= BIT0;
    P8OUT |= BIT0;
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    __bis_SR_register(GIE);

    led_init();
    lcd_init();
    timer_init();
    pad_init();
    adc_init();

    pad_loop();
    __bis_SR_register(LPM0_bits);
}
