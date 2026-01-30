/**
  ******************************************************************************
  * @file    display.c
  * @brief   This file provides code for the
  *          display MAIN, AUX, ANNUNCIATORS & SPLASH.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "spi.h"
#include "main.h"
#include "timer.h"
#include "lcd.h"
#include "lt7680.h"
#include "display.h"
#include <string.h>  // For strchr, strncpy
#include <stdio.h>   // For debugging (optional)

//#define DURATION_MS 5000     // 5 seconds in milliseconds
#define TIMER_INTERVAL_MS 35 // The interval of your timed sub in milliseconds

// Display colours default
uint32_t MainColourFore = 0xFFFF00; // Yellow
uint32_t AnnunColourFore = 0x00FF00; // Green


//************************************************************************************************************************************************************

void DisplayMain(void)
{
	SetTextColors(0xFFFFFF, 0x000000); // Foreground: green, Background: Black
	ConfigureFontAndPosition(
		0b00,    // Internal CGROM
		0b10,    // Font size
		0b00,    // ISO 8859-1
		0,       // Full alignment enabled
		0,       // Chroma keying disabled
		1,       // Rotate 90 degrees counterclockwise
		0b11,    // Width multiplier
		0b10,    // Height multiplier
		1,       // Line spacing
		4,       // Character spacing
		Xpos_MAIN,     // Cursor X
		Ypos_MAIN      // Cursor Y
	);

	// Always draw exactly 13 characters (pad with spaces if shorter)
	char text1[14];               // 13 chars + terminator
	int i = 0;

	// Copy up to 13 chars from displayWithPunct
	while (i < 13 && displayWithPunct[i] != '\0') {
		text1[i] = displayWithPunct[i];
		i++;
	}

	// Pad remainder with spaces
	while (i < 13) {
		text1[i++] = ' ';
	}

	text1[13] = '\0';

	DrawText(text1);
}


void DisplayAnnunciators() {

	// ANNUNCIATORS - Print or clear text on the LCD
	const char* AnnuncNames[12] = {
		"SMPL", "REM", "SRQ", "ADRS", "AC+DC", "4Wohm", "AZOFF", "MRNG", "MATH", "REAR", "ERR", "SHIFT"
	};


	// Set Y-position of the annunciators
	int AnnuncYCoords[12] = {
		10,   // SMPL
		87,   // REM
		165,  // SRQ
		242,  // ADRS
		319,  // AC+DC
		397,  // 4Wohm
		474,  // AZOFF
		551,  // MRNG
		629,  // MATH
		706,  // REAR
		783,  // ERR
		860   // SHIFT
	};


	for (int i = 0; i < 12; i++) {
		if (Annunc[12 - i] == 1) {  // Turn the annunciator ON
			SetTextColors(AnnunColourFore, 0x000000); // Foreground: Green, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // 16-dot font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b01,    // Width X0
				0b01,    // Height X0
				5,       // Line spacing
				0,       // Character spacing
				Xpos_ANNUNC,  // Cursor X (fixed)
				AnnuncYCoords[i] // Cursor Y (from array)
			);
			DrawText(AnnuncNames[i]); // Print the corresponding name
		}
		else {  // Turn the annunciator OFF
			SetTextColors(0x000000, 0x000000); // Foreground: Black, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // 16-dot font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b01,    // Width X0
				0b01,    // Height X0
				5,       // Line spacing
				0,       // Character spacing
				Xpos_ANNUNC,  // Cursor X (fixed)
				AnnuncYCoords[i] // Cursor Y (from array)
			);
			DrawText(AnnuncNames[i]); // Clear the text by drawing in black
		}
	}

}
