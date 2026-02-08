/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "RGB.h"
#include "lcd.h"
#include "lcd_anim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern SPI_HandleTypeDef hspi3;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static uint16_t line_buffer[240];

lcd_io lcd_io_desc = {
    .spi = &hspi3,
    .rst = {LCD_RST_GPIO_Port, LCD_RST_Pin, 0},
    .bl  = {LCD_PWR_GPIO_Port, LCD_PWR_Pin, 0},
    .cs  = {LCD_CS_GPIO_Port, LCD_CS_Pin, 0},
    .dc  = {LCD_DC_GPIO_Port,  LCD_DC_Pin,  0},
    .te  = { /* TE */ }
};

lcd lcd_desc = {
    .io = &lcd_io_desc,
    .line_buffer = line_buffer,
};
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for RGBTask */
osThreadId_t RGBTaskHandle;
const osThreadAttr_t RGBTask_attributes = {
  .name = "RGBTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LCDTask */
osThreadId_t LCDTaskHandle;
const osThreadAttr_t LCDTask_attributes = {
  .name = "LCDTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void RGB_StartTask(void *argument);
void LCD_StartTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationTickHook(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void) {
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

__weak unsigned long getRunTimeCounterValue(void) { 
  return DWT->CYCCNT; 
}
/* USER CODE END 1 */

/* USER CODE BEGIN 3 */
void vApplicationTickHook( void )
{
   /* This function will be called by each tick interrupt if
   configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h. User code can be
   added here, but the tick hook is called from an interrupt context, so
   code must not attempt to block, and only the interrupt safe FreeRTOS API
   functions can be used (those that end in FromISR()). */
}
/* USER CODE END 3 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of RGBTask */
  RGBTaskHandle = osThreadNew(RGB_StartTask, NULL, &RGBTask_attributes);

  /* creation of LCDTask */
  LCDTaskHandle = osThreadNew(LCD_StartTask, NULL, &LCDTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_RGB_StartTask */
/**
* @brief Function implementing the RGBTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RGB_StartTask */
void RGB_StartTask(void *argument)
{
  /* USER CODE BEGIN RGB_StartTask */
  uint8_t color_index = 0;
  /* Infinite loop */
  for(;;)
  {
    RGB_SetColor(color_index);
    
    color_index++;
    if(color_index >= 7) {
        color_index = 0;
    }
    osDelay(1000); 
  }
  /* USER CODE END RGB_StartTask */
}

/* USER CODE BEGIN Header_LCD_StartTask */
/**
* @brief Function implementing the LCDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_LCD_StartTask */
void LCD_StartTask(void *argument)
{
    /* USER CODE BEGIN LCD_StartTask */
    lcd_init_dev(&lcd_desc, LCD_1_14_INCH, LCD_ROTATE_90);
    lcd_clear(&lcd_desc, BLACK);
    
    lcd_print(&lcd_desc, 5, 5, "STM32F411");
    lcd_print(&lcd_desc, 160, 5, "FPS: N/A");

    lcd_anim_cube_t my_cube;
    lcd_anim_cube_init(&my_cube, &lcd_desc, 25.0f, LIGHTBLUE, 120, 80);

    uint32_t frame_count = 0;
    uint32_t last_tick = HAL_GetTick();

    for(;;)
    {
        lcd_anim_cube_run(&my_cube);

        frame_count++;
        if (HAL_GetTick() - last_tick >= 1000) {
            lcd_print(&lcd_desc, 160, 5, "FPS:%d  ", frame_count);
            frame_count = 0;
            last_tick = HAL_GetTick();
        }

        osDelay(1);
    }
    /* USER CODE END LCD_StartTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

