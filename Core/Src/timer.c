/**
  ******************************************************************************
  * @file    timer.c
  * @brief   This file provides code for the configuration
  *          of the timer, and used in main.c
  ******************************************************************************
*/

#include "timer.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_tim.h"
#include "main.h"  // Include main.h to access DMM pin definitions

//***********************************************************************************
// Timer 2 - Timed Action loop
// Timer interrupt sets a flag at regular intervals.
// Use your main loop to monitor this flag and ensures other conditions (like task completion)
// are met before running the dependent subroutine.  

// Timer flag variables
volatile uint8_t timer_flag = 0;
volatile uint8_t task_ready = 0;

uint8_t data = 0;  // Global variable for Live Watch visibility

// Timer 2 - initialization function
void TIM2_Init(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable TIM2 clock

    TIM2->PSC = 7200 - 1;               // Prescaler: 7200 (72 MHz / 7200 = 10 kHz)
    TIM2->ARR = 5000 - 1;               // Default Auto-reload: 500 ms
    TIM2->DIER |= TIM_DIER_UIE;         // Enable update interrupt
    TIM2->CR1 |= TIM_CR1_CEN;           // Enable the timer

    NVIC_EnableIRQ(TIM2_IRQn);          // Enable TIM2 interrupt in NVIC

    HAL_NVIC_SetPriority(TIM2_IRQn, 3, 0);  // Set lower priority for TIM2
    HAL_NVIC_EnableIRQ(TIM2_IRQn);


}

// Timer 2 - interrupt handler
void TIM2_IRQHandler(void) {

    if (TIM2->SR & TIM_SR_UIF) { // Check for update interrupt
        TIM2->SR &= ~TIM_SR_UIF; // Clear the interrupt flag
        timer_flag = 1;          // Set the timer flag
    }
}

// Timer 2 - Dynamic timer duration setting function
void SetTimerDuration(uint16_t ms) {
    // Calculate ARR based on the desired duration in milliseconds
    TIM2->ARR = (10 * ms) - 1; // For a 10 kHz timer clock (1 tick = 0.1 ms)
    TIM2->EGR = TIM_EGR_UG;    // Force update to apply changes immediately
}



//***********************************************************************************
// Timer 3 - 3457A Input Capture Functionality

/* Buffers for Data Capture */
#define MAX_BUFFER_SIZE 256


volatile uint8_t isaBuffer[MAX_BUFFER_SIZE];
volatile uint8_t inaBuffer[MAX_BUFFER_SIZE];



static uint16_t isaIndex = 0;
static uint16_t inaIndex = 0;

volatile uint8_t debugIsaByte = 0;
volatile uint8_t debugInaByte = 0;

volatile uint32_t ISAo2InterruptCount = 0;
volatile uint32_t INAo2InterruptCount = 0;

#define LOG_TRIGGER_COUNT 256 // Log after 256 samples


volatile uint8_t loggedIsa[MAX_BUFFER_SIZE];
volatile uint8_t loggedIna[MAX_BUFFER_SIZE];


extern volatile uint8_t logReady;
volatile uint8_t logReady = 0;

volatile uint8_t loggedIsa[MAX_BUFFER_SIZE];
volatile uint8_t loggedIna[MAX_BUFFER_SIZE];

volatile uint16_t debugIsaIndex = 0;
volatile uint16_t debugInaIndex = 0;

volatile uint8_t isLogging = 0;




/* Private Function Prototypes */
static void ProcessCapturedData(void);

volatile uint32_t logReadySetCount = 0;

volatile uint32_t logExecutionCount = 0;

volatile uint8_t bufferFull = 0; // Indicates if the buffer is full


extern volatile uint8_t isaBuffer[MAX_BUFFER_SIZE];
extern volatile uint8_t inaBuffer[MAX_BUFFER_SIZE];




/*
void LogBuffers(void) {
    if (logReady) {
        logReady = 0; // Reset the flag

        printf("Logging Buffers:\n");
        for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
            printf("ISA[%d] = %d, INA[%d] = %d\n", i, isaBuffer[i], i, inaBuffer[i]);
        }
    }
}
*/





void DMM_HandleO2Clock(void) {

    if (HAL_GPIO_ReadPin(DMM_PWO_GPIO_Port, DMM_PWO_Pin) == GPIO_PIN_SET) {

        uint8_t isaState = HAL_GPIO_ReadPin(DMM_ISA_GPIO_Port, DMM_ISA_Pin);
        uint8_t inaState = HAL_GPIO_ReadPin(DMM_INA_GPIO_Port, DMM_INA_Pin);

        for (volatile int i = 0; i < 10; i++) __NOP(); // Fine-tune delay as needed

        if (syncState == GPIO_PIN_SET) {

            // Capture ISA data
            isaBuffer[isaIndex++] = HAL_GPIO_ReadPin(DMM_ISA_GPIO_Port, DMM_ISA_Pin);
            isaIndex %= MAX_BUFFER_SIZE;

            //printf("ISA Captured: %d (Index: %d)\n", isaState, isaIndex);
            // Print ISA data
            //printf("ISA Captured: %d (Index: %d)\n", isaBuffer[isaIndex - 1], isaIndex - 1);


        } else {
            // Capture INA data
            inaBuffer[inaIndex++] = HAL_GPIO_ReadPin(DMM_INA_GPIO_Port, DMM_INA_Pin);
            inaIndex %= MAX_BUFFER_SIZE;

            //printf("INA Captured: %d (Index: %d)\n", inaState, inaIndex);
            // Print INA data
            //printf("INA Captured: %d (Index: %d)\n", inaBuffer[inaIndex - 1], inaIndex - 1);


        }
    } else {
        // PWO is LOW, no data is captured
        //printf("PWO LOW: No data captured.\n");
    }

}





static void ProcessCapturedData(void) {
    // Combine captured ISA and INA data if needed
    // Example: Decode data, update display buffers, etc.
    // This is left as a placeholder for further processing.
}


void MX_TIM3_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
    TIM_MasterConfigTypeDef sMasterConfig = { 0 };

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 72 - 1;       // 1 MHz clock
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 18 - 1;          // ~55 kHz for O2 signal
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
}


void TIM3_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim3); // Handle TIM3 base events
    DMM_HandleO2Clock();        // Capture data from O2 clock

    //printf("TIM3 Interrupt Triggered (O2 Clock Rising Edge)\n");

}


void DMM_HandleSyncState(void) {
    // Update syncState based on SYNC pin level
    syncState = HAL_GPIO_ReadPin(DMM_SYNC_GPIO_Port, DMM_SYNC_Pin);

    // Optional: Toggle LED to indicate SYNC change
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == DMM_SYNC_Pin) {
        // Update syncState or perform your custom logic
        syncState = HAL_GPIO_ReadPin(DMM_SYNC_GPIO_Port, DMM_SYNC_Pin);

        // Optional: Toggle LED for debugging
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    }
}