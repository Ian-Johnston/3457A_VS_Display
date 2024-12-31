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
#include "lcd.h"
#include "lt7680.h"
#include "display.h"
#include <string.h>  // For strchr, strncpy
#include <stdio.h>   // For debugging (optional)

#define DURATION_MS 5000     // 5 seconds in milliseconds
#define TIMER_INTERVAL_MS 35 // The interval of your timed sub in milliseconds

// Display colours default
uint32_t MainColourFore = 0xFFFF00; // Yellow
uint32_t AnnunColourFore = 0x00FF00; // Green


//************************************************************************************************************************************************************

void DisplayAnnunciators() {

	// ANNUNCIATORS - Print or clear text on the LCD
	const char* AnnuncNames[19] = {
		"SMPL", "IDLE", "AUTO", "LOP", "NULL", "DFILT", "MATH", "AZERO",
		"ERR", "INFO", "FRONT", "REAR", "SLOT", "LO_G", "RMT", "TLK",
		"LTN", "SRQ"
	};


	// Set Y-position of the annunciators
	int AnnuncYCoords[19] = {
		10,   // SMPL
		62,   // IDLE
		114,  // AUTO
		166,  // LOP
		218,  // NULL
		270,  // DFILT
		322,  // MATH
		374,  // AZERO
		426,  // ERR
		478,  // INFO
		530,  // FRONT
		582,  // REAR
		634,  // SLOT
		686,  // LO_G
		738,  // RMT
		790,  // TLK
		842,  // LTN
		900   // SRQ
	};


	for (int i = 0; i < 18; i++) {
		if (Annunc[i + 1] == 1) {  // Turn the annunciator ON
			SetTextColors(AnnunColourFore, 0x000000); // Foreground: Green, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b00,    // 16-dot font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width X0
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
				0b00,    // Width X0
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

//******************************************************************************

void DisplaySplash() {

	// Splash text to display
	static uint32_t cycle_count = 0; // Persistent counter for cycles
	static uint8_t timer_active = 1; // Flag to track timer status
	if (timer_active) {
		cycle_count++;
		// Check if the 5-second period has elapsed
		if (cycle_count >= (DURATION_MS / TIMER_INTERVAL_MS)) {
			// Runs once
			timer_active = 0; // Stop counting after 5 seconds
			SetTextColors(0x00FF00, 0x000000); // Foreground: Yellow, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b01,    // Font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width multiplier
				0b00,    // Height multiplier
				1,       // Line spacing
				4,       // Character spacing
				Xpos_SPLASH,     // Cursor X
				270      // Cursor Y
			);
			char text[] = "                       ";
			DrawText(text);
		}
		else {
			// Perform operations within the 5-second window
			// Splash text
			SetTextColors(0x00FF00, 0x000000); // Foreground: Yellow, Background: Black
			ConfigureFontAndPosition(
				0b00,    // Internal CGROM
				0b01,    // Font size
				0b00,    // ISO 8859-1
				0,       // Full alignment enabled
				0,       // Chroma keying disabled
				1,       // Rotate 90 degrees counterclockwise
				0b00,    // Width multiplier
				0b00,    // Height multiplier
				1,       // Line spacing
				4,       // Character spacing
				Xpos_SPLASH,     // Cursor X	230
				270      // Cursor Y	360
			);
			char text[] = "TFT LCD by Ian Johnston";
			DrawText(text);
		}
	}

}