/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "spi.h"
#include "lt7680.h"
#include "main.h"

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/

/*
Original Display Conversion of 3457A for reference only (courtesy of users "xi" & "xyphro"):

3457A reverse engineering and implementation courtesy Users XI and XYPHRO:
https://www.eevblog.com/forum/projects/led-display-for-hp-3457a-multimeter-i-did-it-)/
https://www.eevblog.com/forum/projects/led-display-for-hp-3457a-multimeter-i-did-it-)/?action=dlattach;attach=415104
https://www.eevblog.com/forum/projects/led-display-for-hp-3457a-multimeter-i-did-it-)/?action=dlattach;attach=415101
https://github.com/xyphro/HP3457-OLED-display (by xyphro off the back of xi's effort)

I/O:
PD0(RXD)    = INA ^ ISA (data) (Actually it would not be required to do a HW XOR, but here it helped to save
                                money for a 8 line isolator IC. In case you don't want galvanic isolation, you
                                could optimize the XOR IC away).
PD3(INT1)   = SYNC (high = command, low = data)
PD4(XCK/T0) = O2 (clock)
PD2(INT0)   = PWO (power up)

//**********************************************************************************************************************************************************

TFT LCD Conversion (IanJ):
INA     - Pin 25 J612 - data                        - STM32 Pin B15
ISA     - Pin 19 J612 - data (instruction)          - STM32 Pin B14
SYNC    - Pin 21 J612 - sync (HI=command, LO=data)  - STM32 Pin B11
02      - Pin 27 J612 - clock                       - STM32 Pin B13
PWO     - Pin 31 J612 - power on                    - STM32 Pin B12
01      - Pin 29 J612 - n/a

LCD display board conn 16-way  -  J612 34-way conn on main board:
Pin ##  -  Pin 17 - n/c
Pin ##  -  Pin 18 - V3V  (LCD supply)
Pin ##  -  Pin 19 - ISA  (instruction)
Pin ##  -  Pin 20 - V2V  (LCD supply)
Pin ##  -  Pin 21 - SYNC (1 = read data on ISA / 0 = read data on INA)
Pin ##  -  Pin 22 - V1V  (LCD supply)
Pin ##  -  Pin 23 - n/c
Pin ##  -  Pin 24 - 0Vdc
Pin ##  -  Pin 25 - INA (data)
Pin ##  -  Pin 26 - OS1 (internal clock oscillator for the LCD display -connected to a 470pF capacitor on the main board)
Pin ##  -  Pin 27 - 02  (2nd clock oscillator - small delay)
Pin ##  -  Pin 28 - n/c
Pin ##  -  Pin 29 - 01  (clock)
Pin ##  -  Pin 30 - n/c
Pin ##  -  Pin 31 - PWO (global chip select)
Pin ##  -  Pin 32 - +5Vdc supply (via series resistor on main board)
Pin ##  -  Pin 33 - n/c
Pin ##  -  Pin 34 - n/c

Serial synchronous transmission, and for us there are 5 interesting lines: PWO, 02 clk, SYNC, ISA and INA.
With these lines we are able to completely decode the protocol of the LCD display.
Note: O1 and O2 clocks are exactly the same, but the O2 line is 6µs delayed compared to O1.
We will use rising edge of O2 clock to sample the data, because it has the best timing (middle of each data bit).

//**********************************************************************************************************************************************************
Original reverse engineering info from "xi" on EEVBlog forum:

Principle:
The HP3457 sends datagrams that are command + data payload based.
The SYNC line identifies if the data is a command (SYNC = HIGH) or data (SYNC = LOW).
A command has a bitlength of 10 bits, the data payload is variable in length as function of the received command

D3 is programmed to generate an interrupt on a rising edge (=start of a command).
  Once the interrupt arrives, To will be setup to generate an interrupt for every rising edge of the O2=clock line.
This timer interrupt does receive bits and store them for later processing.

The actual display content will be flagged as valid so that it can be updated if no command is received within 10 ms

//**********************************************************************************************************************************************************

Description of each line:
PWO: this is the global chip select, when at level "1", the LCD display is selected and must read the data.
CLK 02: this is the clock for the data received on ISA and INA lines. Max clock frequency is 55kHz (may be less, I guess it depends on the processor load).
SYNC: this line is at level "1" when the data must be read on ISA line / at level "0" when the data must be read on INA line. On the above screenshot,
the SYNC line goes "high" 7 times, meaning there are 7 instructions sent on ISA line
ISA: this line sends the instructions to the LCD display, I have found 7 types of instructions:
 - 0x3F0: select the LCD display (1 byte)
 - 0x2E0: ?
 - 0x320: toggle display ON/OFF
 - 0x2F0: annunciators ON/OFF ("SMPL", "REM", "SRQ", "ADRS", "AC+DC", "4W", "AZOFF", "MRNG", "MATH", "REAR", "ERR", "SHIFT") (2 bytes)
 - 0x028: write A registers (6 bytes)
 - 0x068: write B registers (6 bytes)
 - 0x0A8: write C registers (6 bytes)
INA: this line sends the data to the LCD display (eg the values to display on the digits)

Chip Select: is not a line from the motherboard, it is generated by my board with a bit of logic (we will see why later)

Examining the first instruction: 0x3F0 = Select the display
General info about the transmission:
- Each bit are transmitted reverse, ie the bit 0 (= Least Significant Bit) of a register is sent first ...
- The same apply for the digits: digit 12 (the last one) is transmitted first ...

The doc I found says this instruction is to select the display, and the display is selected when the data received on INA line is 0xFD. Let's verify!

Let's check the instruction between C1 and C2 markers first: remember that instructions are sent on ISA line when SYNC line is at level "1". On the above screenshot, we can see that we received these bits:
0000111111 (10 bits).
Also remember that the bits are sent in reverse order, so once reordered, the data on ISA line is:
1111110000 = 0x3F0 = select the LCD display command :-+

Now we are going to check the data between A1 and A2 markers: remember that data are sent on INA line when SYNC line is at level "0". On the above screenshot, we can see that we received these bits:
10111111 (8 bits).
Also remember that the bits are sent in reverse order, so once reordered, the data on INA line is:
11111101 = 0xFD = select the LCD display value :-+

What about the bits received between C2 and A1 markers ? (two clock pulses)
There are always these two bits, whatever the line (ISA or INA), and they are always 0.
The useful data always starts after these 2 bits, and are then sent on a multiple of 8 bits.
In other words, each transmission of data starts with a 10 bits set, followed by n 8 bits sets.
But since the first 2 bits are always 0, we can ignore them and consider that the data are only composed of 8 bits sets.
Ignoring these 2 bits is not an easy task for the microcontroller, because it is not designed to receive a variable length of data on the SPI bus ;)
For this reason, I have added a bit of logic (two D flip-flop to delay the SYNC signal and an XOR gate to generate my "Chip Select").
With this logic, I am able to decode directly the data from the HP 3457A using the hardware SPI module from the Atmel microcontroller
(the "Chip Select" line directly drives the CS input of the Atmel: the CS input is only enabled when at logic level "0" :)).

Now let's decode the interesting part of the frame: the data sent to the digits 8)
We will ignore the useless (for the mod) instructions (0x2F0, ...) and we will decode the 0x028 / 0x068 / 0x0A8 instructions.
There are 3 registers of 6 bytes each = 18 bytes in total to describe the 12 chars of the display + the punctuation.
Each register is set by one instruction received on ISA line: 0x028 for register A, 0x068 for register B and 0x0A8 for register C.

The principle for decoding this instruction is exactly the same as for the "First instruction: 0x3F0", so refer to this section to understand it ;)
The data set received for register A is (hexadecimal): 1F 99 99 D9 50 25
Note that there are 6 more bytes transmitted after these 6 bytes, but they are always 0, so we don't bother to retrieve and decode them (I don't know the purpose of these bytes).
The exact same principle applies to decode it, and the data set received is: 31 37 33 23 0D 00

Similarly, the register C (0xA8 command) will decode as: 00 00 00 00 00 00

Ok, so "BEEP,-99999.1_" is displayed on the screen, and we have 3 registers, containing the following values ... how to match them?
 reg A = 1F 99 99 D9 50 25
 reg B = 31 37 33 23 0D 00
 reg C = 00 00 00 00 00 00

//**********************************************************************************************************************************************************

Decode the Digits:
After much research for "HP LCD charset" and other similar things, I ended-up finding this little treasure: the complete doc of a ROM for the HP41C!
This is a calculator that contains a LCD display with a similar protocol as the one used on the DMM ; thanks HP for the good consistency between the products :-+
This is here: http://www.series80.org/Misc/ZenROM.pdf

The most interesting info are on page 115 and following (section 8.2 "Display handling"):
- description of the format of the frames
- description of the instructions (some are missing, like the 0x2E0, but it's enough to understand the protocol)
- and the most interesting: the HP character set for the LCD display! 8) Below is a reproduction of this charset, with the value for each char (eg, char 'X' has value "0x18")

Each byte of register A contains 4 LSB bits for each digit (meaning there are the LSB part of 2 digits in each byte)
Similarly, each byte of register B contains 2 MSB bits for 2 digits and 2 bits for the punctuation of 2 digits
And, each byte of register C contains the Extended bit for 2 digits.
----------------------------------------------------------------------------------------------------------------------
|Byte nr received |        1        |        2       |       3       |       4       |       5       |       6       |
----------------------------------------------------------------------------------------------------------------------
|Digit number     |   11   |   12   |   9   |   10   |   7   |   8   |   5   |   6   |   3   |   4   |   1   |   2   |
----------------------------------------------------------------------------------------------------------------------

Char value is given by retrieving the following bits from register A, B & C:
(The first line represents the 7 bits of the char value ; this value directly matches the HP charset showed above)
(The second and third line show which register and bit needs to be read to retrieve the char value. Eg, "RegB3_bit1" means Register B, byte 3, bit 1)
------------------------------------------------------------------------------------------------------------------------------
| bit number for one char         |     6      |     5      |     4      |     3      |     2      |     1      |     0      |
------------------------------------------------------------------------------------------------------------------------------
| pos in the reg. for even digits | RegCn_bit0 | RegBn_bit1 | RegBn_bit0 | RegAn_bit3 | RegAn_bit2 | RegAn_bit1 | RegAn_bit0 |
------------------------------------------------------------------------------------------------------------------------------
| pos in the reg. for odd digits  | RegCn_bit4 | RegBn_bit5 | RegBn_bit4 | RegAn_bit7 | RegAn_bit6 | RegAn_bit5 | RegAn_bit4 |
------------------------------------------------------------------------------------------------------------------------------

Lets apply this table to our example. Remember we got these values in the registers:
 reg A = 1F 99 99 D9 50 25
 reg B = 31 37 33 23 0D 00
 reg C = 00 00 00 00 00 00

So if we want to decode digit 4 for example, using the first table, we know that digit 4 data are stored in byte number 5:
Digit 4 data are 50 (reg A), 0D (reg B), 00 (reg C). Written in binary, this is:
01010000 (reg A), 00001101 (reg B), 00000000 (reg C)
Now we use the second table to decode the char ; digit 4 is an even number, so decoding is done using these bits:
-------------------------------------------------------------------------------------
| RegC_bit0 | RegB_bit1 | RegB_bit0 | RegA_bit3 | RegA_bit2 | RegA_bit1 | RegA_bit0 |
|     0     |     0     |     1     |     0     |     0     |     0     |     0     |
-------------------------------------------------------------------------------------
= 0010000 = 0x10
Last, we use the HP charset to see that char 0x10 is the letter 'P', eg what was expected ;D (forth digit of "BEEP,-99999.1_" text)

//**********************************************************************************************************************************************************

Decode the Punctuation:
The punctuation is transmitted in the register B, using the exact same principle as above.
-------------------------------------------------------------
| bit number for the punctuation  |     1      |     0      |
-------------------------------------------------------------
| pos in the reg. for even digits | RegBn_bit3 | RegBn_bit2 |
-------------------------------------------------------------
| pos in the reg. for odd digits  | RegBn_bit7 | RegBn_bit6 |
-------------------------------------------------------------

And the charset for the punctuation is this one:
--------------------
|Value | Sign      |
--------------------
| 0    |   (none)  |
| 1    | . (point) |
| 2    | : (colon) |
| 3    | , (comma) |
--------------------
(note: the annunciators are transmitted separately in another instruction)

Lets apply this table to our example. Remember we got these values in the registers:
 reg A = 1F 99 99 D9 50 25
 reg B = 31 37 33 23 0D 00
 reg C = 00 00 00 00 00 00

So if we want to decode punctuation for digit 10 for example, using the first table, we know that digit 10 data are stored in byte number 2:
So, digit 10 data is 0x37 (reg B) = 00110111 (reg B)
Now we use the punctuation decoding table ; digit 10 is an even number, so decoding is done using these bits:
-------------------------
| RegB_bit3 | RegB_bit2 |
|     0     |     1     |
-------------------------
= 0b01 = 1
Last, we use the charset for the punctuation to see that value 1 corresponds to punctuation '.' (point),
I.E. what we expected (punctuation for 10th digit of "BEEP,-99999.1_" text)

//**********************************************************************************************************************************************************

Decode the Annunciators:
Remember, the annunciators are the functions "SMPL", "REM", "SRQ", "ADRS", "AC+DC", "4W", "AZOFF", "MRNG", "MATH", "REAR", "ERR", "SHIFT" below the display.

As always, the instruction is between C1 & C2 markers. Decoding the instruction gives 0x2F0, which indicates that the annunciators will follow on INA line.

Decoding the annunciator is really simple: they are sent on INA line between markers D1 & D2, and there is one bit per annunciator.
The only difficulty is that they are written reverse (as always), so the first transmitted bit is the last annunciator.
---------------------------------------------------------------------------------------------------
| annunciator  | SMPL | REM | SRQ | ADRS | AC+DC | 4Wo | AZOFF | MRNG | MATH | REAR | ERR | SHIFT |
---------------------------------------------------------------------------------------------------
| bit position |  12  | 11  | 10  |  9   |   8   |  7  |   6   |  5   |  4   |  3   |  2  |   1   |
---------------------------------------------------------------------------------------------------

Applied to our example, we have bit's positions 4 and 12 which are at level "1", meaning that "MATH" indicator and "SMPL" indicators are ON

*/


void MX_GPIO_Init(void) {

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    __HAL_RCC_TIM3_CLK_ENABLE();  // Enable clock for TIM3
    __HAL_RCC_AFIO_CLK_ENABLE();  // Enable clock for Alternate Function IO
	

    // LED - Test output pin
    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(TEST_OUT_GPIO_Port, TEST_OUT_Pin, GPIO_PIN_RESET);
    /* Configure GPIO pin : TEST_OUT_Pin */
    GPIO_InitStruct.Pin = TEST_OUT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TEST_OUT_GPIO_Port, &GPIO_InitStruct);


    // LT7680A-R Reset
    /* Configure GPIO pin for LT7680 RESET */
    GPIO_InitStruct.Pin = RESET_PIN;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RESET_PORT, &GPIO_InitStruct);


    // HP3457A I/O pins (serial interface decode)
    // These are 5Vdc tolerant pins on the Blue Pill so can interface directly with the 3457A 5V logic levels
    /* Configure GPIO pin : DMM_SYNC_Pin */
    GPIO_InitStruct.Pin = DMM_SYNC_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;   // rising edge = start of command
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DMM_SYNC_GPIO_Port, &GPIO_InitStruct);

    /* Configure GPIO pin : DMM_O2_Pin */
    GPIO_InitStruct.Pin = DMM_O2_Pin;
    //GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DMM_O2_GPIO_Port, &GPIO_InitStruct);

    /* Configure GPIO pin : DMM_INA_Pin */
    GPIO_InitStruct.Pin = DMM_INA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DMM_INA_GPIO_Port, &GPIO_InitStruct);

    /* Configure GPIO pin : DMM_PWO_Pin */
    GPIO_InitStruct.Pin = DMM_PWO_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DMM_PWO_GPIO_Port, &GPIO_InitStruct);

    /* Configure GPIO pin : DMM_ISA_Pin */
    GPIO_InitStruct.Pin = DMM_ISA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DMM_ISA_GPIO_Port, &GPIO_InitStruct);
	
	
	// ST7701S LCD direct SPI
	// Configure bit bang SPI pins for direct LCD SPI
	GPIO_InitStruct.Pin = LCD_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(LCD_CS_Port, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = LCD_SCK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(LCD_SCK_Port, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = LCD_SDI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(LCD_SDI_Port, &GPIO_InitStruct);


    // Selection header LK1
    // Configure GPIO pin B0
    GPIO_InitStruct.Pin = GPIO_PIN_0;       // Select pin B0
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // Set as input
    GPIO_InitStruct.Pull = GPIO_PULLUP;   // Enable pull-up resistor
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Low speed is sufficient for input
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


    // Selection header LK2
    // Configure GPIO pin B1
    //GPIO_InitStruct.Pin = GPIO_PIN_1;       // Select pin B1
    //GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // Set as input
    //GPIO_InitStruct.Pull = GPIO_PULLUP;   // Enable pull-up resistor
    //GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Low speed is sufficient for input
    //HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


    /* EXTI interrupt init */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);  // High priority for SYNC
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);       // slightly lower than SYNC
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

}

