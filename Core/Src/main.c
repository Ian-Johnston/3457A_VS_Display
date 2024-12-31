/*
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  *
  * By Ian Johnston (IanSJohnston on YouTube),
  * for 3.71" 960x240 TFT LCD (ST7701S) and using LT7680 controller adaptor
  * Visual Studio 2022 with VisualGDB plugin:
  * To upload HEX from VS2022 = BUILD then PROGRAM AND START WITHOUT DEBUGGING
  * Use LIVE WATCH to view variables live debug
  * 
  * For use with LT7680A-R & 240x960 TFT LCD
  *
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "lt7680.h"
#include "timer.h"
#include <stdbool.h>    // bool support, otherwise use _Bool
//#include <stdlib.h> // For rand()
#include "display.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"


// Test only - IanJ
//volatile uint32_t myVariable0 = 0;
//volatile uint32_t myVariable1 = 0;
//volatile uint32_t myVariable2 = 0;
//volatile uint32_t myVariable3 = 0;
//volatile uint32_t myVariable4 = 0;
//volatile uint32_t myVariable5 = 0;
//volatile uint32_t myVariable6 = 0;
//volatile uint32_t myVariable7 = 0;
//volatile uint32_t myVariable8 = 0;
//volatile uint64_t myCounter1 = 0;
//volatile uint64_t myCounter2 = 0;
//volatile uint16_t lastCommand = 0;  // Stores the last received command
//volatile uint8_t lastMode = 0;      // Stores the current mode (0 = Data, 1 = Command)
//volatile uint8_t dataBuffer[MAX_BUFFER_SIZE]; // Buffer for data bytes
//volatile uint8_t dataBufferLength = 0;       // Length of data in the buffer
//volatile uint8_t bitValueDebug = 0;      // Last sampled bit value
//volatile uint16_t syncTransitions = 0;  // Counts transitions on SYNC
//volatile uint16_t dataCaptured = 0;     // Counts captured data bytes
//volatile uint8_t debugBufferIndex = 0; // Monitor which index is being written
//volatile uint16_t commandProcessed = 0;
//volatile uint8_t debugBitIndex = 0;
//volatile uint8_t decodedData[MAX_BUFFER_SIZE] = { 0 };
//volatile uint8_t decodedDataLength = 0;
//volatile const char* lastCommandDebug = ""; // For debug messages
// sample Live watch lastCommandDebug will show "Data Captured"
//volatile _Bool processBufferFlag = 0;
//volatile uint8_t byteCounter = 0;
//char displayString[MAX_BUFFER_SIZE];
//#define MAX_DISPLAY_SIZE 64
//char displayString[MAX_DISPLAY_SIZE];

#define MAX_BUFFER_SIZE 64
uint16_t unknownCommandDebug = 0;
volatile uint8_t debugDataBuffer6 = 0;  // Debugging dataBuffer[6]
volatile uint8_t debugDataBuffer7 = 0;  // Debugging dataBuffer[7]
volatile uint16_t debugLastCommandExtracted = 0;  // Debugging last extracted command
volatile uint8_t debugDecodedData[MAX_BUFFER_SIZE]; // Debug buffer to monitor decoded data

volatile uint16_t lastCommand = 0;            // Last extracted command
volatile const char* lastCommandDebug = "";   // Debug string for the last command
volatile uint8_t debugBitIndex = 0;           // Tracks bit position in current byte
volatile uint8_t byteCounter = 0;             // Tracks the byte index in the buffer
volatile uint8_t dataBuffer[MAX_BUFFER_SIZE]; // Holds incoming data
volatile uint8_t decodedData[MAX_BUFFER_SIZE];
volatile uint8_t decodedDataLength = 0;

volatile uint8_t incomingBits[MAX_BUFFER_SIZE];
volatile uint8_t bitIndex = 0;
volatile uint8_t currentMode = 0; // 0: Data, 1: Command

volatile char punctuationString[13]; // Stores punctuation for the 12 digits (null-terminated)

char displayString[13]; // 12 digits + null terminator
char debugDisplayString[13]; // For Live Watch

volatile uint8_t processBufferFlag = 0;

char DecodeChar(uint8_t charValue);

volatile uint8_t debugFlagBitIndexReset = 0; // Tracks the number of times debugBitIndex is reset




// Placeholder definitions for missing symbols
#define Da 0x01
#define Db 0x02
#define Dc 0x04
#define Dd 0x08
#define De 0x10
#define Df 0x20
#define Dg1 0x40
#define Dg2 0x80
#define Dm 0x100
#define Ds 0x200
#define Dk 0x400
#define Dt 0x800
#define Dn 0x1000
#define Dr 0x2000

// HP Charset lookup table
const uint16_t hp_charset[] = {
	Da | Db | Dd | De | Df | Dg2 | Dm,  // 0 : @
	Da | Db | Dc | De | Df | Dg1 | Dg2, // 1 : A
	Da | Db | Dc | Dd | Dg2 | Dm | Ds,  // 2 : B
	Da | Dd | De | Df,                  // 3 : C
	Da | Db | Dc | Dd | Dm | Ds,        // 4 : D
	Da | Dd | De | Df | Dg1 | Dg2,      // 5 : E
	Da | De | Df | Dg1 | Dg2,           // 6 : F
	Da | Dc | Dd | De | Df | Dg2,       // 7 : G
	Db | Dc | De | Df | Dg1 | Dg2,      // 8 : H
	Da | Dd | Dm | Ds,                  // 9 : I
	Db | Dc | Dd | De,                  // 10: J
	De | Df | Dg1 | Dn | Dt,            // 11: K
	Dd | De | Df,                       // 12: L
	Db | Dc | De | Df | Dk | Dn,        // 13: M
	Db | Dc | De | Df | Dk | Dt,        // 14: N
	Da | Db | Dc | Dd | De | Df,        // 15: O
	Da | Db | De | Df | Dg1 | Dg2,      // 16: P
	Da | Db | Dc | Dd | De | Df | Dt,   // 17: Q
	Da | Db | De | Df | Dg1 | Dg2 | Dt, // 18: R
	Da | Dc | Dd | Df | Dg1 | Dg2,      // 19: S
	Da | Dm | Ds,                       // 20: T
	Db | Dc | Dd | De | Df,             // 21: U
	De | Df | Dn | Dr,                  // 22: V
	Db | Dc | De | Df | Dr | Dt,        // 23: W
	Dk | Dn | Dr | Dt,                  // 24: X
	Dk | Dn | Ds,                       // 25: Y
	Da | Dd | Dn | Dr,                  // 26: Z
	Da | Dd | De | Df,                  // 27: [
	Dk | Dt,                            // 28: \
	Da | Db | Dc | Dd,                  // 29: ]
	Da | Db | Dn | Dr,                  // 30: Top-right pointing arrow
	Dd,                                 // 31: _
	0,                                  // 32: Space
	Dm | Ds,                            // 33: !
	Df | Dm,                            // 34: "
	Db | Dc | Dd | Dg1 | Dg2 | Dm | Ds, // 35: #
	Da | Dc | Dd | Df | Dg1 | Dg2 | Dm | Ds, // $
	Dc | Df | Dg1 | Dg2 | Dk | Dn | Dr | Dt, // %
	Da | Dc | Dd | Dk | Dn | Dr | Dt,   // 38: &
	Dm,                                 // 39: '
	Dn | Dt,                            // 40: (
	Dk | Dr,                            // 41: )
	Dg1 | Dg2 | Dk | Dm | Dn | Dr | Ds | Dt, // *
	Dg1 | Dg2 | Dm | Ds,                // 43: +
	Dg1 | Dg2 | Dn | Dt,                // 44: <-
	Dg1 | Dg2,                          // 45: -
	Dg1 | Dg2 | Dk | Dr,                // 46: ->
	Dr | Dn,                            // 47: /
	Da | Db | Dc | Dd | De | Df | Dn | Dr, // 48: 0
	Db | Dc,                            // 49: 1
	Da | Db | Dd | De | Dg1 | Dg2,      // 50: 2
	Da | Db | Dc | Dd | Dg1 | Dg2,      // 51: 3
	Db | Dc | Df | Dg1 | Dg2,           // 52: 4
	Da | Dc | Dd | Dk | Dg2,            // 53: 5
	Da | Dc | Dd | De | Df | Dg1 | Dg2, // 54: 6
	Da | Db | Dc,                       // 55: 7
	Da | Db | Dc | Dd | De | Df | Dg1 | Dg2, // 56: 8
	Da | Db | Dc | Dd | Df | Dg1 | Dg2  // 57: 9
};


//******************************************************************************

// Flag indicating finish of SPI transmission to OLED
volatile uint8_t SPI1_TX_completed_flag = 1;

// Flag indicating finish of start-up initialization
volatile uint8_t Init_Completed_flag = 0;

// Private function prototypes
void SystemClock_Config(void);

//******************************************************************************

//SPI transmission finished interrupt callback
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
	if (hspi->Instance == SPI1)
	{
		SPI1_TX_completed_flag = 1;
	}
}


// SYNC pin has gone HIGH, interrupt calls this sub
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == DMM_SYNC_Pin) {
		// Handle rising edge of SYNC
		HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);	// Disable interrupt
		//StartO2Timer();							// Call sub to process incoming data
		__HAL_GPIO_EXTI_CLEAR_IT(DMM_SYNC_Pin);	// Clear any pending interrupt flag for SYNC (PB11)
		HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);		// Re-enable interrupt
	}
}


// Captured data via interrupt
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim) {
	if (htim->Instance == TIM3 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
		// Determine mode (SYNC high = command mode, low = data mode)
		currentMode = HAL_GPIO_ReadPin(DMM_SYNC_GPIO_Port, DMM_SYNC_Pin) ? 1 : 0;

		// Read bit from ISA (command) or INA (data)
		uint8_t bitValue = currentMode
			? HAL_GPIO_ReadPin(DMM_ISA_GPIO_Port, DMM_ISA_Pin)
			: HAL_GPIO_ReadPin(DMM_INA_GPIO_Port, DMM_INA_Pin);

		// Append reversed bit order
		dataBuffer[byteCounter] = (dataBuffer[byteCounter] >> 1) | (bitValue << 7);
		debugBitIndex++;

		// Increment byteCounter every 8 bits
		if (debugBitIndex == 8) {
			debugFlagBitIndexReset++; // Increment flag when bitIndex resets
			debugBitIndex = 0;        // Reset bitIndex
			byteCounter++;            // Move to the next byte

			// Prevent buffer overflow
			if (byteCounter >= MAX_BUFFER_SIZE) {
				byteCounter = 0;
				memset(dataBuffer, 0, MAX_BUFFER_SIZE); // Reset buffer
			}
		}

		// Trigger processing if enough data is captured
		if ((currentMode == 1 && byteCounter >= 2) || (currentMode == 0 && byteCounter >= 8)) {
			processBufferFlag = 1;
		}
	}
}



// Process the incoming data or command
void ProcessBuffer() {

	if (currentMode == 0 && byteCounter >= 18) { // Ensure full data frame
		// Decode the registers to extract the display content
		DecodeRegisters(&dataBuffer[0], &dataBuffer[6], &dataBuffer[12], displayString);

		// Decode punctuation for the display
		DecodePunctuation(&dataBuffer[6], punctuationString);

		// Debugging: Combine digits and punctuation for Live Watch
		for (int i = 0; i < 12; i++) {
			debugDisplayString[i] = displayString[i];
			if (punctuationString[i] != ' ') {
				debugDisplayString[i] = punctuationString[i]; // Add punctuation if present
			}
		}
		debugDisplayString[12] = '\0'; // Null-terminate
	}

	// Reset buffer after processing
	byteCounter = 0;
	memset(dataBuffer, 0, MAX_BUFFER_SIZE);

}




void DecodeRegisters(const uint8_t* regA, const uint8_t* regB, const uint8_t* regC, char* displayString) {
	for (int digit = 0; digit < 12; digit++) {
		// Identify byte index in each register
		int byteIndex = digit / 2;

		// Extract character bits
		uint8_t charBits = 0;
		if (digit % 2 == 0) { // Even digits
			charBits |= (regA[byteIndex] & 0x0F) |         // Lower 4 bits
				((regB[byteIndex] & 0x03) << 4) | // Middle 2 bits
				((regC[byteIndex] & 0x01) << 6);  // Upper bit
		}
		else { // Odd digits
			charBits |= ((regA[byteIndex] >> 4) & 0x0F) |        // Lower 4 bits
				((regB[byteIndex] >> 4) & 0x03) << 4 |  // Middle 2 bits
				((regC[byteIndex] >> 4) & 0x01) << 6;   // Upper bit
		}

		// Map to character
		displayString[digit] = DecodeChar(charBits);
	}
	displayString[12] = '\0'; // Null-terminate the string
}


void DecodePunctuation(const uint8_t* regB, char* punctuation) {
	for (int digit = 0; digit < 12; digit++) {
		int byteIndex = digit / 2;

		// Extract punctuation bits
		uint8_t punctBits = (digit % 2 == 0)
			? (regB[byteIndex] >> 2) & 0x03
			: (regB[byteIndex] >> 6) & 0x03;

		// Map punctuation
		switch (punctBits) {
		case 0: punctuation[digit] = ' '; break; // None
		case 1: punctuation[digit] = '.'; break; // Point
		case 2: punctuation[digit] = ':'; break; // Colon
		case 3: punctuation[digit] = ','; break; // Comma
		}
	}
	punctuation[12] = '\0'; // Null-terminate
}



// Function to decode and format the display string
void DecodeDisplayString(char* displayString, uint8_t* dataBuffer, uint8_t dataLength) {
	// Clear the output string
	memset(displayString, 0, MAX_BUFFER_SIZE);

	// Decode each character from the data buffer
	for (uint8_t i = 0; i < dataLength && i < MAX_BUFFER_SIZE - 1; i++) {
		uint8_t charValue = dataBuffer[i] & 0x7F; // Mask to 7 bits as needed
		if (charValue < sizeof(hp_charset) / sizeof(hp_charset[0])) {
			displayString[i] = (char)hp_charset[charValue];
		}
		else {
			displayString[i] = '?'; // Unknown character
		}
	}

	// Ensure the string is null-terminated
	displayString[dataLength] = '\0';
}


// Decode a single character using the HP charset
char DecodeChar(uint8_t charValue) {
	// Ensure charValue is within bounds
	if (charValue < sizeof(hp_charset) / sizeof(hp_charset[0])) {
		return hp_charset[charValue];
	}
	return '?'; // Unknown character
}



//************************************************************************************************************************************************************
//************************************************************************************************************************************************************

// Main
int main(void) {

	//__disable_irq();  // Disable all interrupts
	HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);			// disable 3457A inputs whilst we set up everything
	__HAL_GPIO_EXTI_CLEAR_IT(DMM_SYNC_Pin);			// Clear any pending interrupt flag for SYNC (PB11)

	// Reset of all peripherals, Initializes the Flash interface and the Systick.
	HAL_Init();

	// Configure the system clock
	SystemClock_Config();

	// Initialize all configured peripherals
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_SPI1_Init();

	// Initialize TIM3 for 3457A Input Capture
	TIM3_Init_InputCapture();

	// Pull CS high and SCLK low immediately after reset
	HAL_GPIO_WritePin(LCD_CS_Port, LCD_CS_Pin, GPIO_PIN_SET);			// Pull CS high
	HAL_GPIO_WritePin(LCD_SCK_Port, LCD_SCK_Pin, GPIO_PIN_RESET);		// CLK pin low

	// Pull LT7680 RESET pin high immediately after reset
	HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);   // Release reset high

	TIM2_Init();					// Initialize the timer
	
	// Read pin B0 - Set colours for MAIN
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_SET) {
		// B0 high
		MainColourFore = 0xFFFFFF;		// WHite
	}
	else {
		// B0 low
		MainColourFore = 0xFFFF00;		// Yellow
	}

	HardwareReset();				// Reset LT7680 - Pull LCM_RESET low for 100ms and wait

	HAL_Delay(1000);
	
	BuyDisplay_Init();				// Initialize ST7701S BuyDisplay 3.71" driver IC

	HAL_Delay(100);

	SendAllToLT7680_LT();			// run subs to setup LT7680 based on Levetop info

	HAL_Delay(10);

	// Main loop timer
	SetTimerDuration(35);			// 35 ms timed action set

	HAL_Delay(5);
	ConfigurePWMAndSetBrightness(BACKLIGHTFULL);  // Configure Timer-1 and PWM-1 for backlighting. Settable 0-100%

	// Reconfigure SYNC pin as an interrupt enabled input now that setup is complete
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	GPIO_InitStruct.Pin = DMM_SYNC_Pin;            // Specify the pin explicitly
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;    // Enable interrupt on rising edge
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;          // Configure pull-down resistor
	HAL_GPIO_Init(DMM_SYNC_GPIO_Port, &GPIO_InitStruct);

	//__enable_irq();
	__HAL_GPIO_EXTI_CLEAR_IT(DMM_SYNC_Pin);			// Clear any pending interrupt flag for SYNC (PB11)
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);			// Ready to accept 3457A inputs

//**************************************************************************************************
// Main loop initialize

	Init_Completed_flag = 1; // Now is a safe time to enable the EXTI interrupt handler

	while (1) {			// While loop running continious, full speed

		//myCounter1++;


		//if (tim3_capture_flag) {
		//	ProcessBuffer();
		//	tim3_capture_flag = 0;                              // Clear flag after handling
		//}



		//Process_3457A_Data();		// Acquire data from 3457A

		//Packets_to_chars();         // Convert packets from R6581 to characters
		//Main_Aux_R6581();           // Get R6581 VFD drive data

		task_ready = 1; // Mark tasks as complete so the timer driven code is allowed to run again

		//*******************************************************************************************
		// Timed Action - Check if timer flag is set and tasks are ready and run the LCD sub
		// This loop runs at the SetTimerDuration setting continously AND as long as task_ready is set
		if (timer_flag && task_ready) {
			timer_flag = 0;   // Clear the timer flag
			task_ready = 0;   // Reset task-ready flag    

			//myCounter2++;
			
			//HAL_GPIO_TogglePin(GPIOC, TEST_OUT_Pin); // Test LED toggle

			DisplaySplash();

			HAL_Delay(6); // Allow the LT7680 sufficient processing time

			//DisplayMain();

			if (processBufferFlag = 1) {
				processBufferFlag = 0;
				ProcessBuffer();          // Process data or commands

				// Inline extraction and rendering in your timed action loop
				char mainReadout[14]; // 13 characters + null terminator
				// Assuming `displayString` contains the full decoded display data
				for (int i = 0; i < 13; i++) {
					mainReadout[i] = displayString[i]; // Copy up to 13 characters
				}
				mainReadout[13] = '\0'; // Null-terminate the string

				SetTextColors(MainColourFore, 0x000000); // Foreground, Background
				
				ConfigureFontAndPosition(
					0b00,    // Internal CGROM
					0b10,    // Font size
					0b00,    // ISO 8859-1
					0,       // Full alignment enabled
					0,       // Chroma keying disabled
					1,       // Rotate 90 degrees counterclockwise
					0b11,    // Width multiplier
					0b11,    // Height multiplier
					1,       // Line spacing
					4,       // Character spacing
					Xpos_MAIN, // Cursor X
					0          // Cursor Y
				);
				DrawText(mainReadout); // Send the decoded string to the display
				//DrawText("+ 1.23456  VDC");
			}


			//HAL_Delay(6); // Allow the LT7680 sufficient processing time

			//DisplayAux();

			HAL_Delay(6); // Allow the LT7680 sufficient processing time

			//DisplayAnnunciators();

			HAL_Delay(6); // Allow the LT7680 sufficient processing time

		}
	}

}


// System Clock Configuration
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	//Initializes the RCC Oscillators according to the specified parameters in the RCC_OscInitTypeDef structure.
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	// Initializes the CPU, AHB and APB buses clocks
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
		| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
}


// This function is executed in case of error occurrence.
void Error_Handler(void) {
	// User can add his own implementation to report the HAL error return state
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	   /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
