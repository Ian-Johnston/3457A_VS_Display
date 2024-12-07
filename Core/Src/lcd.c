/**
  ******************************************************************************
  * @file    lcd.c
  * @brief   This file provides code for the configuration
  *          of the lcd
  ******************************************************************************
  * ST7701S supports two kinds of RGB interface, DE mode (mode 1) and HV mode
  * (mode 2), and 16bit/18bit and 24 bit data format. When DE mode is selected
  * and the VSYNC, HSYNC, DOTCLK, DE, D23:0 pins can be used; when HV mode is
  * selected and the VSYNC, HSYNC, DOTCLK, D23:0 pins can be used. When using
  * RGB interface, only serial interface can be selected.
*/

/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "main.h"
#include "lcd.h"
#include "lt7680.h"


//************************************************************************************************************************************************************

// Bit bang SPI to LCD (9bit)
void LCD_SPI_Write(uint16_t data, uint8_t bits) {
	for (int i = bits - 1; i >= 0; i--) {  // Loop through each bit (MSB first)
		// Set SDA based on the current bit
		if (data & (1 << i)) {
			HAL_GPIO_WritePin(LCD_SDI_Port, LCD_SDI_Pin, GPIO_PIN_SET);  // SDA = 1
		}
		else {
			HAL_GPIO_WritePin(LCD_SDI_Port, LCD_SDI_Pin, GPIO_PIN_RESET); // SDA = 0
		}

		// Toggle the clock signal
		HAL_GPIO_WritePin(LCD_SCK_Port, LCD_SCK_Pin, GPIO_PIN_SET); // CLK high
		DelayMicroseconds(5);                                   // Hold high
		HAL_GPIO_WritePin(LCD_SCK_Port, LCD_SCK_Pin, GPIO_PIN_RESET); // CLK low
		DelayMicroseconds(5);                                   // Hold low
	}
}


void LCDWriteRegister(uint8_t reg) {
	HAL_GPIO_WritePin(LCD_CS_Port, LCD_CS_Pin, GPIO_PIN_RESET); // Pull CS low
	DelayMicroseconds(10);

	// Use LCD_SPI_Write for 9-bit SPI communication
	LCD_SPI_Write((0 << 8) | reg, 9); // D/CX = 0, reg[7:0]

	HAL_GPIO_WritePin(LCD_CS_Port, LCD_CS_Pin, GPIO_PIN_SET);   // Pull CS high
	DelayMicroseconds(10);
}



void LCDWriteData(uint8_t data) {
	HAL_GPIO_WritePin(LCD_CS_Port, LCD_CS_Pin, GPIO_PIN_RESET); // Pull CS low
	DelayMicroseconds(10);

	// Use LCD_SPI_Write for 9-bit SPI communication
	LCD_SPI_Write((1 << 8) | data, 9); // D/CX = 1, data[7:0]

	HAL_GPIO_WritePin(LCD_CS_Port, LCD_CS_Pin, GPIO_PIN_SET);   // Pull CS high
	DelayMicroseconds(10);
}



// delay used by the bit bang SPI	
void DelayMicroseconds(uint16_t us) {
	uint32_t start = SysTick->VAL; // Get current SysTick value
	uint32_t ticks = (HAL_RCC_GetHCLKFreq() / 1000000) * us; // Ticks for desired delay
	uint32_t reload = SysTick->LOAD + 1;

	while (((start - SysTick->VAL) & 0xFFFFFF) < ticks) {
		if (SysTick->VAL > reload) { // Handle SysTick counter wrap-around
			start -= reload;
		}
	}
}


//**************************************************************************************************
// Commands to LCD


void LCD_Clear(uint16_t color) {
	// Set the active area to the full screen
	// Black : 0x0000
	// White : 0xFFFF
	// Red   : 0xF800
	// Green : 0x07E0
	// Blue  : 0x001F

	LCDWriteRegister(0x2A); // Column Address Set
	LCDWriteData(0x00);     // Start column high byte
	LCDWriteData(0x00);     // Start column low byte
	LCDWriteData((LCD_XSIZE_TFT - 1) >> 8);  // End column high byte
	LCDWriteData((LCD_XSIZE_TFT - 1) & 0xFF); // End column low byte

	LCDWriteRegister(0x2B); // Row Address Set
	LCDWriteData(0x00);     // Start row high byte
	LCDWriteData(0x00);     // Start row low byte
	LCDWriteData((LCD_YSIZE_TFT - 1) >> 8);  // End row high byte
	LCDWriteData((LCD_YSIZE_TFT - 1) & 0xFF); // End row low byte

	// Write the color to the display RAM
	LCDWriteRegister(0x2C); // Memory Write command

	// Send color data for the entire screen
	for (uint32_t i = 0; i < (LCD_XSIZE_TFT * LCD_YSIZE_TFT); i++) {
		LCDWriteData(color >> 8); // Send high byte of color
		LCDWriteData(color & 0xFF); // Send low byte of color
	}
}


void LCD_Hor_Ver_Timing() {
	// Horizontal Timing
	LCDWriteRegister(0x14); LCDWriteData(0x27); // Horizontal Display Width
	LCDWriteRegister(0x16); LCDWriteData(0x09); // Horizontal Back Porch
	LCDWriteRegister(0x18); LCDWriteData(0x0C); // Horizontal Front Porch
	LCDWriteRegister(0x19); LCDWriteData(0x00); // HSYNC Pulse Width

	// Vertical Timing
	LCDWriteRegister(0x1A); LCDWriteData(0xFF); // Vertical Display Height (Low byte)
	LCDWriteRegister(0x1B); LCDWriteData(0x03); // Vertical Display Height (High bits)
	LCDWriteRegister(0x1C); LCDWriteData(0x05); // Vertical Back Porch (Low byte)
	LCDWriteRegister(0x1D); LCDWriteData(0x00); // Vertical Back Porch (High bits)
	LCDWriteRegister(0x1E); LCDWriteData(0x18); // Vertical Front Porch
	LCDWriteRegister(0x1F); LCDWriteData(0x01); // VSYNC Pulse Width
}


void BuyDisplay_Init() {

	//LCDWriteRegister(0x01); // DT software reset    added by IanJ
	//HAL_Delay(120);

	// Refer to ST7701S datasheet and your TFT LCD datasheet in order to make teh following settings

	//*************************************************************************************************************************************
	LCDWriteRegister(0xFF);			// CND2BKxSEL (FFh/FF00h): Command2 BK3 Selection - Command bank selection - Bank 3
	LCDWriteData(0x77);
	LCDWriteData(0x01);
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x13);	// 10011b
	
	LCDWriteRegister(0xEF);			// not known
	LCDWriteData(0x08);
	
	//*************************************************************************************************************************************
	LCDWriteRegister(0xFF);			// CND2BKxSEL (FFh/FF00h): Command2 BK0 Selection - Command bank selection - Bank 0 (system)
	LCDWriteData(0x77);
	LCDWriteData(0x01);
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x10);	// 10000b

	LCDWriteRegister(0xC0);			// LNESET (C0h/C000h): Display Line Setting
	LCDWriteData(0x77);				// 960 lines	77
	LCDWriteData(0x00);				// 960 lines	00

	LCDWriteRegister(0xC1);			// PORCTRL (C1h/C100h): Porch Control 
	LCDWriteData(0x10);				// VBP = 10															originally 0x09 = 9
	LCDWriteData(0x12);				// VFP = 12															originally 0x08 = 8

	LCDWriteRegister(0xC2);			// INVSET (C2h/C200h): Inversion selection & Frame Rate Control 
	LCDWriteData(0x37);				// C200h: Inversion Selection NLINV = 7								originally 37
	LCDWriteData(0x08);				// C201h: Frame Rate Control RTNI =	8								originally = 0x02, tried 0x00

	LCDWriteRegister(0xC3);			// RGBCTRL (C3h/C300h): RGB control
	LCDWriteData(0x81);				// C300h: DE/HV=HV, VSP=L, HSP=L, DP=Rising, EP=High	=> 10000001										0x81
	LCDWriteData(0x00);				// C301h: HBP_HVRGB[7:0] (Horizontal Back Porch in HSYNC in cycles)	originally 0x05 = 5					0x01	0x20 works		50	00
	LCDWriteData(0x22);				// C302h: VBP_HVRGB[7:0] (Vertical Back Porch HSYNC in cycles)		originally 0x0D = 13				0x0D					0A	22
	
	LCDWriteRegister(0xCC);			// not known
	LCDWriteData(0x10);
	
	LCDWriteRegister(0xB0);			// PVGAMCTRL (B0h/B000h): Positive Voltage Gamma Control 
	LCDWriteData(0x40);				// VC0P[3:0], AJ0P[1:0]
	LCDWriteData(0x14);				// VC4P[5:0], AJ1P[1:0]
	LCDWriteData(0x59);				// VC8P[5:0], AJ2P[1:0]
	LCDWriteData(0x10);				// VC16P[4:0]
	LCDWriteData(0x12);				// VC24P[4:0], AJ3P[1:0]
	LCDWriteData(0x08);				// VC52P[3:0]
	LCDWriteData(0x03);				// VC80P[5:0]
	LCDWriteData(0x09);				// VC108P[3:0]
	LCDWriteData(0x05);				// VC147P[3:0]
	LCDWriteData(0x1E);				// VC175P[5:0]
	LCDWriteData(0x05);				// VC203P[3:0]
	LCDWriteData(0x14);				// VC231P[4:0], AJ4P[1:0]
	LCDWriteData(0x10);				// VC239P[4:0]
	LCDWriteData(0x68);				// VC247P[5:0], AJ5P[1:0]
	LCDWriteData(0x33);				// VC251P[5:0], AJ6P[1:0]
	LCDWriteData(0x15);				// VC255P[4:0], AJ7P[1:0]

	LCDWriteRegister(0xB1);			// VCOMS (B1h/B100h):VCOM amplitude setting  
	LCDWriteData(0x40);				// VC0N[3:0], AJ0N[1:0]
	LCDWriteData(0x08);				// VC4N[5:0], AJ1N[1:0]
	LCDWriteData(0x53);				// VC8N[5:0], AJ2N[1:0]
	LCDWriteData(0x09);				// VC16N[4:0]
	LCDWriteData(0x11);				// VC24N[4:0], AJ3N[1:0]
	LCDWriteData(0x09);				// VC52N[3:0]
	LCDWriteData(0x02);				// VC80N[5:0]
	LCDWriteData(0x07);				// VC108N[3:0]
	LCDWriteData(0x09);				// VC147N[3:0]
	LCDWriteData(0x1A);				// VC175N[5:0]
	LCDWriteData(0x04);				// VC203N[3:0]
	LCDWriteData(0x12);				// VC231N[4:0], AJ4N[1:0]
	LCDWriteData(0x12);				// VC239N[4:0]
	LCDWriteData(0x64);				// VC247N[5:0], AJ5N[1:0]
	LCDWriteData(0x29);				// VC251N[5:0], AJ6N[1:0]
	LCDWriteData(0x29);				// VC255N[4:0], AJ7N[1:0]

	//*************************************************************************************************************************************
	LCDWriteRegister(0xFF);			// CND2BKxSEL (FFh/FF00h): Command2 BK1 Selection - Command bank selection - Bank 1
	LCDWriteData(0x77);
	LCDWriteData(0x01);
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x11);	//10001b

	LCDWriteRegister(0xB0);			// BK1: VRHS (B0h/B000h): Vop Amplitude setting 
	LCDWriteData(0x6D);				// VRHA[7:0] = 0x6D (109 in decimal) - 4.9Vdc

	LCDWriteRegister(0xB1);			// VCOMS (B1h/B100h): VCOM amplitude setting 
	LCDWriteData(0x1D);

	LCDWriteRegister(0xB2);			// VGHSS (B2h/B200h): VGH Voltage setting  
	LCDWriteData(0x87);

	LCDWriteRegister(0xB3);			// TESTCMD (B3h/B300h): Internal TEST Command Setting  
	LCDWriteData(0x00);				// 0x80 = enable test (if LCD has routine), 0x00 = disable										originally 0x80 = enabled

	LCDWriteRegister(0xB5);			// VGLS (B5h/B500h): VGL Voltage setting  
	LCDWriteData(0x49);

	LCDWriteRegister(0xB7);			// PWCTRL1 (B7h/B700h): Power Control 1  
	LCDWriteData(0x85);

	LCDWriteRegister(0xB8);			// PWCTRL2 (B8h/B800h): Power Control 2 
	LCDWriteData(0x20);

	LCDWriteRegister(0xC1);			// SPD1 (C1h/C100h): Source pre_drive timing set1  
	LCDWriteData(0x78);

	LCDWriteRegister(0xC2);			// SPD2 (C2h/C200h): Source EQ2 Setting  
	LCDWriteData(0x78);

	LCDWriteRegister(0xD0);			// MIPISET1 (D0h/D000h): MIPI Setting 1  - Ignore if LCD is using parallel or SPI comms
	LCDWriteData(0x88);

	LCDWriteRegister(0xE0);			// SECTRL (E0h/E000h): Sunlight Readable Enhancement  
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x02);

	// NRCTRL (E1h/E100h): Noise Reduce Control
	uint8_t enablenr = 0;  // 1 to enable, 0 to disable noise reduction
	uint8_t levelnr = 0x02; // Noise reduction level (0b00, 0b01, 0b10, 0b11)
	uint8_t byte1nr = (enablenr ? (1 << 4) : 0x00) | (levelnr & 0x03);
	LCDWriteRegister(0xE1);
	LCDWriteData(byte1nr); // Byte 1: Enable/Disable and Level Adjust
	LCDWriteData(0x8C);  // Byte 2: Reserved/unknown
	LCDWriteData(0x00);  // Byte 3: Reserved/unknown
	LCDWriteData(0x00);  // Byte 4: Reserved/unknown
	LCDWriteData(0x03);  // Byte 5: Reserved/unknown
	LCDWriteData(0x8C);  // Byte 6: Reserved/unknown
	LCDWriteData(0x00);  // Byte 7: Reserved/unknown
	LCDWriteData(0x00);  // Byte 8: Reserved/unknown
	LCDWriteData(0x00);  // Byte 9: Reserved/unknown
	LCDWriteData(0x33);  // Byte 10: Reserved/unknown
	LCDWriteData(0x33);  // Byte 11: Reserved/unknown

	// SECTRL (E2h/E200h): Sharpness Control  
	uint8_t enablesc = 0;  // 1 to enable sharpness, 0 to disable
	uint8_t levelsc = 0x02; // Sharpness level (0b00, 0b01, 0b10, 0b11)
	// Construct Byte 1: Enable/Disable and Level Adjust
	uint8_t byte1sc = (enablesc ? (1 << 4) : 0x00) | (levelsc & 0x03);
	LCDWriteRegister(0xE2);
	LCDWriteData(byte1sc); // Byte 1: Enable/Disable and Level Adjust
	LCDWriteData(0x33);  // Byte 2: Reserved/unknown
	LCDWriteData(0x33);  // Byte 3: Reserved/unknown
	LCDWriteData(0x33);  // Byte 4: Reserved/unknown
	LCDWriteData(0x33);  // Byte 5: Reserved/unknown
	LCDWriteData(0xC9);  // Byte 6: Reserved/unknown
	LCDWriteData(0x3C);  // Byte 7: Reserved/unknown
	LCDWriteData(0x00);  // Byte 8: Reserved/unknown
	LCDWriteData(0x00);  // Byte 9: Reserved/unknown
	LCDWriteData(0xCA);  // Byte 10: Reserved/unknown
	LCDWriteData(0x3C);  // Byte 11: Reserved/unknown
	LCDWriteData(0x00);  // Byte 12: Reserved/unknown
	LCDWriteData(0x00);  // Byte 13: Reserved/unknown
	LCDWriteData(0x00);  // Byte 14: Reserved/unknown

	LCDWriteRegister(0xE3);			// CCCTRL (E3h/E300h): Color Calibration Control 
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x33);
	LCDWriteData(0x33);

	LCDWriteRegister(0xE4);			// SKCTRL (E4h/E400h): Skin Tone Preservation Control 
	LCDWriteData(0x44);
	LCDWriteData(0x44);
	
	LCDWriteRegister(0xE5);			// not known
	LCDWriteData(0x05);
	LCDWriteData(0xCD);
	LCDWriteData(0x82);
	LCDWriteData(0x82);
	LCDWriteData(0x01);
	LCDWriteData(0xC9);
	LCDWriteData(0x82);
	LCDWriteData(0x82);
	LCDWriteData(0x07);
	LCDWriteData(0xCF);
	LCDWriteData(0x82);
	LCDWriteData(0x82);
	LCDWriteData(0x03);
	LCDWriteData(0xCB);
	LCDWriteData(0x82);
	LCDWriteData(0x82);
	
	LCDWriteRegister(0xE6);			// not known
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x33);
	LCDWriteData(0x33);

	LCDWriteRegister(0xE7);			// not known
	LCDWriteData(0x44);
	LCDWriteData(0x44);

	LCDWriteRegister(0xE8);			// not known
	LCDWriteData(0x06);
	LCDWriteData(0xCE);
	LCDWriteData(0x82);
	LCDWriteData(0x82);
	LCDWriteData(0x02);
	LCDWriteData(0xCA);
	LCDWriteData(0x82);
	LCDWriteData(0x82);
	LCDWriteData(0x08);
	LCDWriteData(0xD0);
	LCDWriteData(0x82);
	LCDWriteData(0x82);
	LCDWriteData(0x04);
	LCDWriteData(0xCC);
	LCDWriteData(0x82);
	LCDWriteData(0x82);

	LCDWriteRegister(0xEB);			// not known
	LCDWriteData(0x08);
	LCDWriteData(0x01);
	LCDWriteData(0xE4);
	LCDWriteData(0xE4);
	LCDWriteData(0x88);
	LCDWriteData(0x00);
	LCDWriteData(0x40);

	LCDWriteRegister(0xEC);			// not known
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x00);

	LCDWriteRegister(0xED);			// not known
	LCDWriteData(0xFF);
	LCDWriteData(0xF0);
	LCDWriteData(0x07);
	LCDWriteData(0x65);
	LCDWriteData(0x4F);
	LCDWriteData(0xFC);
	LCDWriteData(0xC2);
	LCDWriteData(0x2F);
	LCDWriteData(0xF2);
	LCDWriteData(0x2C);
	LCDWriteData(0xCF);
	LCDWriteData(0xF4);
	LCDWriteData(0x56);
	LCDWriteData(0x70);
	LCDWriteData(0x0F);
	LCDWriteData(0xFF);

	LCDWriteRegister(0xEF);			// not known
	LCDWriteData(0x10);
	LCDWriteData(0x0D);
	LCDWriteData(0x04);
	LCDWriteData(0x08);
	LCDWriteData(0x3F);
	LCDWriteData(0x1F);
	
	//*************************************************************************************************************************************
	LCDWriteRegister(0xFF);			// CND2BKxSEL (FFh/FF00h): Command2 BKx Selection - Command bank selection - Disable Banl function
	LCDWriteData(0x77);
	LCDWriteData(0x01);
	LCDWriteData(0x00);
	LCDWriteData(0x00);
	LCDWriteData(0x00);	//00000b

	LCDWriteRegister(0x11);			// SLPOUT (11h/1100h): Sleep Out  
	HAL_Delay(120);

	//LCDWriteRegister(0x34);			// TEON (35h/3500h): Tearing Effect Line OFF
	//LCDWriteData(0x00);

	LCDWriteRegister(0x35);			// TEON (35h/3500h): Tearing Effect Line ON  
	LCDWriteData(0x00);

	LCDWriteRegister(0x3A);			// COLMOD (3Ah/3A00h): Interface Pixel Format  
	LCDWriteData(0x55);				// 16bpp for compatibility with LT7680A-R
	//LCDWriteData(0x66);				// 18bpp
	
	LCDWriteRegister(0x29); 		// DISPON (29h/2900h): Display On 
}

