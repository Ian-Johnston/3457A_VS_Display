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
#include <stdlib.h>
#include "display.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"
#include <stddef.h>


// Interrupt/Timer Priorities:
// Interrupt			Priority	Description
// EXTI15_10_IRQn		1			SYNC interrupt(critical timing)
// TIM3_IRQn			2			3457A O2 signal processing
// DMA1_Channel3_IRQn	3			SPI DMA for LCD
// TIM2_IRQn			3			Periodic timer for general tasks - REMOVED


TIM_HandleTypeDef htim3; // Definition of htim3
//volatile uint8_t syncState = 0; // Definition of syncState


//******************************************************************************
// Variables

//volatile uint8_t debugSyncState = 0; // Tracks the raw state of the SYNC pin
//volatile uint8_t syncState = 5;             // 1 = SYNC high, 0 = SYNC low
//volatile uint32_t syncHighCounter = 0;      // Count O2 edges when SYNC is high
//volatile uint32_t syncLowCounter = 0;       // Count O2 edges when SYNC is low
volatile uint8_t TEST1 = 0;
volatile uint8_t TEST2 = 0;
volatile uint8_t TEST3 = 0;

#define MAX_BUFFER_SIZE 256  // Define the size of the buffer

// Buffers to store captured data
//uint8_t isaBuffer[MAX_BUFFER_SIZE] = { 0 };
//uint8_t inaBuffer[MAX_BUFFER_SIZE] = { 0 };

// Pointers to track the current position in the buffers
//uint16_t isaIndex = 0;
//uint16_t inaIndex = 0;

// Debug variables
//uint16_t debugIsaCount = 0; // Track how many bits are captured in ISA
//uint16_t debugInaCount = 0; // Track how many bits are captured in INA
//uint8_t debugLastIsaData[5] = { 0 }; // Store the last few ISA values
//uint8_t debugLastInaData[5] = { 0 }; // Store the last few INA values

volatile uint32_t debugO2Callback = 0;

//uint32_t isaCombined = 0;
//uint32_t inaCombined = 0;

#define DISPLAY_LENGTH 12  // Number of characters in the main display

char displayString[DISPLAY_LENGTH + 1];  // +1 for null-terminator

uint8_t isaState = 0;
uint8_t inaState = 0;

extern volatile uint8_t logReady;

//******************************************************************************
// HP Symbols

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


// Punctuation Lookup Table
const char punctuation_map[] = { ' ', '.', ':', ',' };

// Global Variables for Registers
//uint8_t regA[6], regB[6], regC[6], ina[2];


//void DecodeAndFormatDisplay(void);
//void UpdateCombinedValues(void);





//******************************************************************************

// Flag indicating finish of SPI transmission to OLED
volatile uint8_t SPI1_TX_completed_flag = 1;

// Flag indicating finish of start-up initialization
volatile uint8_t Init_Completed_flag = 0;

// Private function prototypes
void SystemClock_Config(void);

//******************************************************************************

// LT7680A-R - SPI transmission finished interrupt callback
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi) {
	if (hspi->Instance == SPI1)
	{
		SPI1_TX_completed_flag = 1;
	}
}


//************************************************************************************************************************************************************
//************************************************************************************************************************************************************

// Main
int main(void) {

	//__disable_irq();  // Disable all interrupts
	//HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);			// disable 3457A inputs whilst we set up everything
	//__HAL_GPIO_EXTI_CLEAR_IT(DMM_SYNC_Pin);			// Clear any pending interrupt flag for SYNC (PB11)

	// Reset of all peripherals, Initializes the Flash interface and the Systick.
	HAL_Init();

	// Configure the system clock
	SystemClock_Config();

	// Initialize all configured peripherals
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_SPI1_Init();		// LT7680A-R

	MX_TIM3_Init();

	//MX_NVIC_Init();

	// Start TIM3 input-capture on CH4 (PB1 = TIM3_CH4)
	HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_4);


	// Pull CS high and SCLK low immediately after reset
	HAL_GPIO_WritePin(LCD_CS_Port, LCD_CS_Pin, GPIO_PIN_SET);			// Pull CS high
	HAL_GPIO_WritePin(LCD_SCK_Port, LCD_SCK_Pin, GPIO_PIN_RESET);		// CLK pin low

	// Pull LT7680 RESET pin high immediately after reset
	HAL_GPIO_WritePin(RESET_PORT, RESET_PIN, GPIO_PIN_SET);   // Release reset high

	//TIM2_Init();					// Initialize the timer

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
	//SetBacklightFull();
	ConfigurePWMAndSetBrightness(BACKLIGHTFULL);  // Configure Timer-1 and PWM-1 for backlighting. Settable 0-100%

	ClearScreen();					// Again.....


	// Right wipe to clear random pixels down the far right hand side
	DrawLine(0, 959, 239, 959, 0x00, 0x00, 0x00);	// far right hand vertical line, black, 1 pixel line. (this line hidden!)
	DrawLine(0, 958, 239, 958, 0x00, 0x00, 0x00);	// (this line hidden!)
	DrawLine(0, 957, 239, 957, 0x00, 0x00, 0x00);
	DrawLine(0, 956, 239, 956, 0x00, 0x00, 0x00);
	DrawLine(0, 955, 239, 955, 0x00, 0x00, 0x00);
	DrawLine(0, 954, 239, 954, 0x00, 0x00, 0x00);
	DrawLine(0, 953, 239, 953, 0x00, 0x00, 0x00);
	DrawLine(0, 952, 239, 952, 0x00, 0x00, 0x00);

	// Test only - 400pixel based test lines for viewing the centre line and the left, middle and far right positions.
	// The internal memory is set up as 400x960 but the leftmost 80 pixels are considered overscan and don't show up, thus 320
	// startX, startY, endX, endY, colorRED, colorGREEN, colorBLUE
	//DrawLine(0, 0, 239, 0, 0x00, 0xFF, 0xFF);		// far left hand vertical line, black, 1 pixel line. 938 not 960 seems to be far right edge!
	//DrawLine(0, 480, 239, 480, 0xFF, 0x00, 0xFF);	// mid-way
	//DrawLine(0, 959, 239, 959, 0xFF, 0xFF, 0x00);	// far right
	//DrawLine(119, 0, 119, 959, 0xFF, 0xFF, 0xFF);	// centred on R6243 horizontally


	__HAL_GPIO_EXTI_CLEAR_IT(DMM_SYNC_Pin);			// Clear any pending interrupt flag for SYNC (PB11)
	//HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);			// Ready to accept 3457A inputs

	//**************************************************************************************************
	// Main loop initialize

	//Init_Completed_flag = 1; // Now is a safe time to enable the EXTI interrupt handler

	//printf("READY");

	while (1) {			// While loop running continious, full speed

		//task_ready = 1; // Mark tasks as complete so the timer driven code is allowed to run again


		// TEST
		isaState = HAL_GPIO_ReadPin(DMM_ISA_GPIO_Port, DMM_ISA_Pin);
		inaState = HAL_GPIO_ReadPin(DMM_INA_GPIO_Port, DMM_INA_Pin);
		volatile uint8_t debugIsaState = isaState; // Monitor these in Live Watch
		volatile uint8_t debugInaState = inaState;

		/*
		printf("Logging ISA and INA Buffers:\n");
		for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
			printf("ISA[%d]: 0x%02X, INA[%d]: 0x%02X\n", i, isaBuffer[i], i, inaBuffer[i]);
		}

		HAL_Delay(1000); // Log every second for readability
		*/


		// Call validation functions
		//ValidateISAData();
		//ValidateISAFlags();
		//ValidateKnownPatterns();

		//HAL_Delay(1000); // Delay to avoid flooding the output


		//if (logReady) {
		//	logReady = 0; // Reset the logging flag
		//	LogBuffers();
		//}


		//printf("SYNC: %s, ISA: %d, INA: %d\n",
		//	(syncState == GPIO_PIN_SET) ? "HIGH" : "LOW",
		//	HAL_GPIO_ReadPin(DMM_ISA_GPIO_Port, DMM_ISA_Pin),
		//	HAL_GPIO_ReadPin(DMM_INA_GPIO_Port, DMM_INA_Pin));

		//HAL_Delay(100); // Log every 100ms for readability



		//*******************************************************************************************
		// Timed Action - Check if timer flag is set and tasks are ready and run the LCD sub
		// This loop runs at the SetTimerDuration setting continously AND as long as task_ready is set
		//if (timer_flag && task_ready) {

			//printf("READY TIMER");


			//timer_flag = 0;   // Clear the timer flag
			//task_ready = 0;   // Reset task-ready flag    

			//myCounter2++;

			HAL_GPIO_TogglePin(GPIOC, TEST_OUT_Pin); // Test LED toggle


			//HAL_Delay(6); // Allow the LT7680 sufficient processing time

			/*
			if (processBufferFlag = 1) {
				processBufferFlag = 0;
				ProcessBuffer();          // Process data or commands


				//snprintf(formattedString, sizeof(formattedString), "%s\n", debugDisplayString);

				// Inline extraction and rendering in your timed action loop
				//char mainReadout[14]; // 13 characters + null terminator
				// Assuming `displayString` contains the full decoded display data
				//for (int i = 0; i < 13; i++) {
				//	mainReadout[i] = displayString[i]; // Copy up to 13 characters
				//}
				//mainReadout[13] = '\0'; // Null-terminate the string

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
				DrawText(debugDisplayString); // Send the decoded string to the display
				//DrawText("+ 1.23456  VDC");
			}
			HAL_Delay(6); // Allow the LT7680 sufficient processing time
			*/

		//}


			DisplayMain();

			HAL_Delay(10);

			DisplayAnnunciators();

			HAL_Delay(10);
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
