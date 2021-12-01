#include <msp430.h>
#include <math.h>

typedef unsigned char uchar;

#define SCROL_CTL					0x40  //Scroll image up by SL rows (SL = last 5 bits), range:0-63

#define SET_MIRROR_COL				0xA0  //Normal mirror SEG (column) mapping (set bit0 to mirror columns)

#define SET_MIRROR_ROW				0xC8  //Normal mirror COM (row) mapping (set bit3 to mirror rows)

#define ALL_PIXEL_ON				0xA4  //Disable all pixel on (last bit 1 to turn on all pixels - does not affect memory)

#define LCD_INVERSE					0xA6  //Inverse display off (last bit 1 to invert display - does not affect memory)

#define BIAS_RATIO_VCC				0xA2  //Set voltage bias ratio (BR = bit0)

#define POW_CTL						0x2F  //Set Power control - booster, regulator, and follower on

#define SET_CONTRAST_RESISTOR		0x27  //Set internal resistor ratio Rb/Ra to adjust contrast
#define MSB_SELECT_VOLUME			0x81  //Set Electronic Volume "PM" to adjust contrast
#define LSB_SELECT_VOLUME			0x10  //Set Electronic Volume "PM" to adjust contrast (PM = last 5 bits)

#define ADV_CTL_MSB					0xFA  //Set temp. compensation curve to -0.11%/C
#define ADV_CTL_LSB					0x90

#define COLUMN_ADR_MSB				0x10  //Set SRAM col. addr. before write, last 4 bits = ca4-ca7
#define COLUMN_ADR_LSB				0x00  //Set SRAM col. addr. before write, last 4 bits = ca0-ca3
#define PAGE_ADR					0xB0  //Set SRAM page addr (pa = last 4 bits), range:0-8

#define LCD_EN						0xAF  //Enable display (exit sleep mode & restore power)


#define NONE						0
#define READ_X_AXIS_DATA 			0x18
#define READ_Y_AXIS_DATA 			0x1C
#define READ_Z_AXIS_DATA 			0x20

#define ROWS 7
#define COLUMNS 13


void delay(long int value);

void setupAccelerometer();
void setupLCD();

int power(int number, int power);

void __LCD_SetAddress(uchar page, uchar column);
void Dogs102x6_writeData(uchar *sData, uchar i);
void Dogs102x6_writeCommand(uchar *sCmd, uchar i);

uchar cma3000_SPI(uchar byte_one, uchar byte_two);
int get_milli_g_from_byte(uchar projection_byte);

void clearScreen();
void showNumber(long int number);


unsigned char plus[COLUMNS]  = {0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x7F, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00};
unsigned char minus[COLUMNS] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char digits[10][COLUMNS] = {
  {0x7F, 0x7F, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7F, 0x7F}, // digit 0
  {0x7F, 0x7F, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x7C, 0x3C, 0x1C, 0x0C}, // digit 1
  {0x7F, 0x7F, 0x60, 0x60, 0x30, 0x18, 0x0C, 0x0C, 0x06, 0x03, 0x67, 0x7F, 0x3E}, // digit 2
  {0x7f, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x7f, 0x7f, 0x03, 0x03, 0x03, 0x7f, 0x7f}, // digit 3
  {0x03, 0x03, 0x03, 0x03, 0x03, 0x7f, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63}, // digit 4
  {0x7f, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x7f, 0x7f, 0x60, 0x60, 0x60, 0x7f, 0x7f}, // digit 5
  {0x7f, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x7f, 0x7f, 0x60, 0x60, 0x60, 0x7f, 0x7f}, // digit 6
  {0x60, 0x60, 0x30, 0x10, 0x18, 0x08, 0x0c, 0x04, 0x06, 0x03, 0x01, 0x7f, 0x7f}, // digit 7
  {0x7f, 0x7f, 0x63, 0x63, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x63, 0x7f, 0x7f}, // digit 8
  {0x7f, 0x7f, 0x03, 0x03, 0x03, 0x03, 0x7f, 0x7f, 0x63, 0x63, 0x63, 0x7f, 0x7f}  // digit 9
};

int column_offset = 0;

uchar LCD_INIT_COMMANDS_PART_1[7] = {
	SCROL_CTL,
	SET_MIRROR_COL,
	SET_MIRROR_ROW,
	ALL_PIXEL_ON,
	LCD_INVERSE,
	BIAS_RATIO_VCC,
	POW_CTL
};
uchar LCD_INIT_COMMANDS_PART_2[6] = {
	SET_CONTRAST_RESISTOR,
	MSB_SELECT_VOLUME,
	LSB_SELECT_VOLUME,
	ADV_CTL_MSB,
	ADV_CTL_LSB,
	LCD_EN,
};
int AMOUNT_OF_COMMANDS_1 = 7;
int AMOUNT_OF_COMMANDS_2 = 6;

int main(void)
{
  WDTCTL = WDTPW | WDTHOLD;

    P8DIR |= BIT1;
    P8OUT &= ~BIT1;

  setupLCD();
  setupAccelerometer();

  __bis_SR_register(GIE + LPM0_bits);
  __no_operation();

  return 0;
}

void setupLCD()
{
	// Reset LCD
	P5DIR |= BIT7;	// port init for LCD operations
	P5OUT &= ~BIT7;	// set RST (active low)
	P5OUT |= BIT7;	// reset RST (inactive is high)

	// Delay for at least 5ms
	__delay_cycles(550);

	// Choosing slave
	P7DIR |= BIT4;	// select LCD for chip
	P7OUT &= ~BIT4;	// CS is active low

	// Setting up LCD_D/C
	P5DIR |= BIT6;	// Command/Data for LCD
	P5OUT &= ~BIT6;	// CD low for command

	// Set up P4.1 -- SIMO, P4.3 -- SCLK (select PM_UCB1CLK)
	P4SEL |= BIT1 | BIT3;
	P4DIR |= BIT1 | BIT3;

	// Set up back light
	P7DIR |= BIT6;	// initializing
	P7OUT |= BIT6;	// back light
	P7SEL &= ~BIT6; // USE PWM to control brightness

	// Deselect slave
	P7OUT |= BIT4;	// CS = 1 (Deselect LCD) (stop setting it up)

	UCB1CTL1 |= UCSWRST;	// set UCSWRST bit to disable USCI and change its control registers

	UCB1CTL0 = (
		UCCKPH 	&	// UCCKPH - 1: change out on second signal change, capture input on first one)
		~UCCKPL |	// UCCKPL - 0: active level is 1
		UCMSB 	|	// MSB comes first, LSB is next
		UCMST 	|	// Master mode
		UCSYNC 	|	// Synchronous mode
		UCMODE_0	// 3 pin SPI mode
	);

	// set SMCLK as source and keep RESET
	UCB1CTL1 = UCSSEL_2 | UCSWRST;

	// set frequency divider
	UCB1BR0 = 0x01;	// LSB to 1
	UCB1BR1 = 0;	// MSB to 0

	UCB1CTL1 &= ~UCSWRST;	// enable USCI
	UCB1IFG &= ~UCRXIFG;	// reset int flag (which is set after input shift register gets data)
	Dogs102x6_writeCommand(LCD_INIT_COMMANDS_PART_1, AMOUNT_OF_COMMANDS_1);

	// delay to wait at least 120 ms
	__delay_cycles(12500);

	Dogs102x6_writeCommand(LCD_INIT_COMMANDS_PART_2, AMOUNT_OF_COMMANDS_2);

	clearScreen();
}

void setupAccelerometer()
{
    P2DIR  &= ~BIT5;	// mode: input
    P2REN  |=  BIT5;	// enable pull up resistor
    P2IE   |=  BIT5;	// interrupt enable
    P2IES  &= ~BIT5;	// process on interrupt's front
    P2IFG  &= ~BIT5;	// clear interrupt flag

    // set up cma3000 (CBS - Chip Select (active - 0))
    P3DIR  |=  BIT5;	// mode: output
    P3OUT  |=  BIT5;

    // set up ACCEL_SCK (SCK - Serial Clock)
    P2DIR  |=  BIT7;
    P2SEL  |=  BIT7;

    // Setup SPI communication
    P3DIR  |= (BIT3 | BIT6);	// Set MOSI and PWM pins to output mode
    P3DIR  &= ~BIT4;			// Set MISO to input mode
    P3SEL  |= (BIT3 | BIT4);	// Set mode : P3.3 - UCA0SIMO , P3.4 - UCA0SOMI
    P3OUT  |= BIT6;				// Power cma3000

    UCA0CTL1 |= UCSWRST;		// set UCSWRST bit to disable USCI and change its control registers

    UCA0CTL0 = (
		UCCKPH 	&	// UCCKPH - 1: change out on second signal change, capture input on first one)
		~UCCKPL |	// UCCKPL - 0: active level is 1
		UCMSB 	|	// MSB comes first, LSB is next
		UCMST 	|	// Master mode
		UCSYNC 	|	// Synchronous mode
		UCMODE_0	// 3 pin SPI mode
	);

	// set SMCLK as source and keep RESET
	UCA0CTL1 = UCSSEL_2 | UCSWRST;

	UCA0BR0 = 0x50;	// LSB to 48
	UCA0BR1 = 0x0;	// MSB to 0

	UCA0CTL1 &= ~UCSWRST;	// enable USCI

	cma3000_SPI(0x04, NONE);
	__delay_cycles(550);

	cma3000_SPI(
		0x0A,
		BIT4 |
		BIT1 |
		BIT7
	);
	__delay_cycles(10500);
}

uchar cma3000_SPI(uchar byte_one, uchar byte_two)
{
    char indata;

    P3OUT &= ~BIT5;

    indata = UCA0RXBUF;

    while(!(UCA0IFG & UCTXIFG));

    UCA0TXBUF = byte_one;

    while(!(UCA0IFG & UCRXIFG));

    indata = UCA0RXBUF;

    while(!(UCA0IFG & UCTXIFG));

    UCA0TXBUF = byte_two;

    while(!(UCA0IFG & UCRXIFG));

    indata = UCA0RXBUF;

    while(UCA0STAT & UCBUSY);

    P3OUT |= BIT5;

    return indata;
}

void __LCD_SetAddress(uchar page, uchar column)
{
	uchar cmd[1];

	if (page > 7)
	{
		page = 7;
	}

	cmd[0] = PAGE_ADR + (7 - page);
	uchar command_high = 0x00;
	uchar command_low = 0x00;
	uchar column_address[] = { COLUMN_ADR_MSB, COLUMN_ADR_LSB };

	command_low = (column & 0x0F);
	command_high = (column & 0xF0);
	command_high = (command_high >> 4);

	column_address[0] = COLUMN_ADR_LSB + command_low;
	column_address[1] = COLUMN_ADR_MSB + command_high;

	Dogs102x6_writeCommand(cmd, 1);
	Dogs102x6_writeCommand(column_address, 2);
}

void Dogs102x6_writeCommand(uchar *sCmd, uchar i)
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
	// Dummy read to empty RX buffer and Clear any overrun conditions
	UCB1RXBUF;
	// CS High
	P7OUT |= BIT4;
}

void Dogs102x6_writeData(uchar *sData, uchar i)
{
	// CS Low
	P7OUT &= ~BIT4;
	//CD High
	P5OUT |= BIT6;

	while (i)
	{
		// USCI_B1 TX buffer ready?
		while (!(UCB1IFG & UCTXIFG));

		// Transmit data and increment pointer
		UCB1TXBUF = *sData--;

		// Decrement the Byte counter
		i--;
	}

	// Wait for all TX/RX to finish
	while (UCB1STAT & UCBUSY);

	// Dummy read to empty RX buffer and Clear any overrun conditions
	UCB1RXBUF;

	// CS High
	P7OUT |= BIT4;
}


void showNumber(long int number)
{
	if (number == 0) {
		__LCD_SetAddress(0, 132 - COLUMNS + column_offset);
		Dogs102x6_writeData(digits[0] + COLUMNS - 1, COLUMNS);
		return;
	}
    if (number < 0) {
        __LCD_SetAddress(0, 132 - COLUMNS + column_offset);
        Dogs102x6_writeData(minus + COLUMNS - 1, COLUMNS);

    } else {
        __LCD_SetAddress(0, 132 - COLUMNS + column_offset);
        Dogs102x6_writeData(plus + COLUMNS - 1, COLUMNS);
    }

     int tempNumber = number < 0 ? number * (-1) : number;
     int length = 0;
     while (tempNumber != 0) {

         tempNumber /= 10;
         length++;
     }

     int divider = power(10, length - 1);
     int i = 0;
     int absNumber = number < 0 ? number * (-1) : number;
     for (i = 0; i < length; i++) {

         int numberToPrint = (int)(absNumber / divider);
         absNumber = absNumber % divider;

         __LCD_SetAddress(i + 1, 132 - COLUMNS + column_offset);
         Dogs102x6_writeData(digits[numberToPrint] + COLUMNS - 1, COLUMNS);

         divider /= 10;
     }
}

int power(int number, int power) {

    int result = 1;
    while (power) {

        result *= number;
        power--;
    }
    return result;
}

void clearScreen()
{
	uchar lcd_data[] = {0x00};
	uchar page, column;

	for (page = 0; page < 8; page++)
	{
		__LCD_SetAddress(page, 0);
		for (column = 0; column < 132; column++)
		{
			Dogs102x6_writeData(lcd_data, 1);
		}
	}
}

int get_mili_g_from_byte(uchar projection_byte)
{
	uchar is_negative = projection_byte & BIT7;
	volatile int value_bits = 7;
	uchar bits[] = { BIT6, BIT5, BIT4, BIT3, BIT2, BIT1, BIT0 };
	int mapping[] = { 1142, 571, 286, 143, 71, 36, 18 };

	int i = 0;
	int projection = 0;
	for (; i < value_bits; i++)
	{
		if (!is_negative)
		{
			projection += (bits[i] & projection_byte) ? mapping[i] : 0;
		}
		else
		{
			projection += (bits[i] & projection_byte) ? 0 : mapping[i];
		}
	}
	projection = is_negative ? projection * (-1) : projection;

	return projection;
}

# define M_PI           3.14159265358979323846  /* pi */

double RADIAN_TO_ANGLE_COEFICIENT = 180 / M_PI;

double convertRadianToAngle(double radian) {
	return radian * RADIAN_TO_ANGLE_COEFICIENT;
}

int get_angle(int xProjection, int yProjection)
{
//	double precised_projection = projection;
//
//	// from milli g to g
//	double ratio = precised_projection / 1000;
//
//	ratio = ratio > 1 ? 1 : ratio < -1 ? -1 : ratio;
//	double angle = asin(ratio);
//
//	// convert rad to deg
//	angle =;
//
//	return (long int)angle;
	double radianAngle = atan((double)xProjection / (double)yProjection);

	double angle = convertRadianToAngle(radianAngle);

	return angle;
}

#pragma vector = PORT2_VECTOR
__interrupt void __Accelerometer_ISR(void)
{
	delay(300);
	uchar z_projection_byte = cma3000_SPI(READ_Z_AXIS_DATA, NONE);
	delay(300);
	uchar x_projection_byte = cma3000_SPI(READ_X_AXIS_DATA, NONE);
	delay(300);
	uchar y_projection_byte = cma3000_SPI(READ_Y_AXIS_DATA, NONE);

	int x_projection = get_mili_g_from_byte(x_projection_byte);
	int z_projection = get_mili_g_from_byte(z_projection_byte);
	int y_projection = get_mili_g_from_byte(y_projection_byte);

	int angle = get_angle(x_projection, z_projection);


//	// 1
//	if(x_projection >= 0 && z_projection >= 0) {
//		angle = 1;
//	} else
//		// 2
//		if(x_projection >= 0 && z_projection < 0) {
//		angle = 2;
//	} else
//		// 3
//		if(x_projection < 0 && z_projection >= 0) {
//		angle = 3;
//	}
//		// 4
//		else { // (x_projection < 0 && z_projection < 0)
//		angle = 4;
//	}

//	if (x_projection >= 0) {
//		angle += 90;
//	} else {
//		angle += 270;
//	}

		// 1
		if(x_projection >= 0 && z_projection >= 0) {
			angle *= -1;
		} else
			// 2
			if(x_projection >= 0 && z_projection < 0) {
			angle = -180 - angle;
		} else
			// 3
			if(x_projection < 0 && z_projection >= 0) {
			angle *= -1;
		}
			// 4
			else { // (x_projection < 0 && z_projection < 0)
			angle = 180 - angle;
		}


//	angle *= z_projection < 0 ?  -1 : 1;

//	x_projection = (double)x_projection / 1000 * 9.81 * 1.093;

    clearScreen();
    showNumber(angle);

	if (angle >= -135 && angle <= -45) {
		P8OUT |= BIT1;
	}
	else {
		P8OUT &= ~BIT1;
	}
}

void delay(long int value)
{
	volatile long int i = 0;
	volatile long int temp = 0;
	for (; i < value; i++)
	{
		temp++;
	}
}
