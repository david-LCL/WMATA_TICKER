/*
 * UART TX demonstration program for MSP430G2231, using
 * SparkFun Serial LCD module.
 *
 * Hook up the RX line on the SparkFun Serial LCD backpack to
 * P1.1 (TXD) on the LaunchPad board, power everything appropriately,
 * and you're all set to go.
 *
 * Matthew R. Wilson <mwilson@mattwilson.org>
 * http://mattwilson.org/msp430/
 * August 16, 2010
 *
 * The code to deal with serial data transmission in the following
 * functions is copied and modified from the sample program that ships
 * with the TI MSP430 LaunchPad:
 *   ConfigureTimerUart()
 *   Transmit()
 *   Timer_A()
 *
 * Release for demonstration and education purposes only, I make no
 * promises that this code will even work properly, let alone do anything
 * useful.
 */

#include "msp430g2231.h"

// Pins on the MSP430 we're using
#define     TXD                   BIT1     // TXD on P1.1
#define     BUTTON                BIT3     // Button on P1.3

//   Conditions for 9600 Baud SW UART, SMCLK = 1MHz
#define     Bitime                13       // 1,000,000 / 8 / 9600 = ~13

// variables for serial communication
unsigned char BitCnt;
unsigned int  TXByte;

// How many times have we pushed the button?
unsigned int  buttonPresses = 0;

// function prototypes
void TXString(char *string);
void ConfigureTimerUart(void);
void Transmit();
void brag(void);
void itoa(unsigned int val, char *str, unsigned int limit);

void main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  // Set clock to 1MHz, and SMCLK to 125kHz
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ;
  BCSCTL2 &= ~(DIVS_3);         //SMCLK is DCO/8

  // Set up the timer to simulate a UART
  ConfigureTimerUart();

  // Set up button (P1.3)
  P1DIR &= ~BUTTON;
  P1OUT |= BUTTON;
  P1REN |= BUTTON;
  P1IES |= BUTTON;
  P1IFG &= ~BUTTON;
  P1IE |= BUTTON;

  // Turn on the LED
  P1DIR |= (BIT0|BIT6);
  P1OUT |= BIT0;
  P1OUT &= ~BIT6;

  // wait for LCD display to initialize
  __delay_cycles(1000000);

  __enable_interrupt();

  // Set brightness full
  //TXByte = 0x7c; Transmit();
  //TXByte = 157; Transmit();

  brag();

  // Display the current button count, then go into LPM4. The button
  // interrupt handler will turn the CPU back on which will bring us back
  // to the beginning of the while() loop.
  while(1)
  {
    brag();
    __bis_SR_register(LPM4_bits + GIE);
  }
}

// base-10 itoa for positive numbers. Populates str with a null-terminated string.
// limit lets you overflow back to 1 when you hit the limit+1, 2*limit+1, ...
// make sure *str is an array that can hold the number of digits in your limit + 1.
void itoa(unsigned int val, char *str, unsigned int limit)
{
  int temploc = 0;
  int digit = 0;
  int strloc = 0;
  char tempstr[5]; //16-bit number can be at most 5 ASCII digits;

  if(val>limit)
    val %= limit;

  do
  {
    digit = val % 10;
    tempstr[temploc++] = digit + '0';
    val /= 10;
  } while (val > 0);

  // reverse the digits back into the output string
  while(temploc>0)
    str[strloc++] = tempstr[--temploc];

  str[strloc]=0;
}

// Output the number of button presses to LCD
void brag(void)
{
  // Clear LCD
  TXByte = 0xFE; Transmit();
  TXByte = 0x01; Transmit();

  TXString("You pressed the button ");
  char times[4];
  itoa(buttonPresses, times, 999);
  TXString(times);
  if(buttonPresses!=1)
    TXString(" times");
  else
    TXString(" time");
}

// Transmit a string via serial UART by looping through it and transmitting
// each character one at a time.
void TXString(char *string)
{
  while(*string != 0)
  {
    TXByte = *string; Transmit();
    string++;
  }
}

// Set up timer for simulated UART. Copied from MSP430 LaunchPad sample code.
void ConfigureTimerUart(void)
{
  TACCTL0 = OUT;                             // TXD Idle as Mark
  TACTL = TASSEL_2 + MC_2 + ID_3;            // SMCLK/8, continuous mode
  P1SEL |= TXD;                              //
  P1DIR |= TXD;                              //
}

// Function Transmits Character from TXByte
// Copied from MSP430 LaunchPad sample code
void Transmit()
{
  BitCnt = 0xA;                             // Load Bit counter, 8data + ST/SP
  while (TACCR0 != TAR)                       // Prevent async capture
    TACCR0 = TAR;                             // Current state of TA counter
  TACCR0 += Bitime;                     // Some time till first bit
  TXByte |= 0x100;                        // Add mark stop bit to TXByte
  TXByte = TXByte << 1;                 // Add space start bit
  TACCTL0 =  CCIS0 + OUTMOD0 + CCIE;          // TXD = mark = idle
  while ( TACCTL0 & CCIE );                   // Wait for TX completion
}

// Timer A0 interrupt service routine
// Copied from MSP430 LaunchPad sample code
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
  TACCR0 += Bitime;                           // Add Offset to CCR0
  if (TACCTL0 & CCIS0)                        // TX on CCI0B?
  {
    if ( BitCnt == 0)
      TACCTL0 &= ~ CCIE;                     // All bits TXed, disable interrupt
    else
    {
      TACCTL0 |=  OUTMOD2;                    // TX Space
      if (TXByte & 0x01)
      TACCTL0 &= ~ OUTMOD2;                   // TX Mark
      TXByte = TXByte >> 1;
      BitCnt --;
    }
  }
}

// Update the button press count, and toggle LEDs, when the button is pressed
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
  buttonPresses++;
  P1OUT ^= BIT0;
  P1OUT ^= BIT6;
  P1IFG = 0; // clear interrupt
  __bic_SR_register_on_exit(LPM4_bits);
}
