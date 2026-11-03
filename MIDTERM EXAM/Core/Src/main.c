/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>   // snprintf()：將數字轉換成文字
#include <string.h>  // strlen()：取得字串長度
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;

TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;


/* USER CODE BEGIN PV */

/* =========================================
   1. 時間設定：單位為秒
   ========================================= */

#define STUDY_TIME   20U
#define BREAK_TIME   10U
#define REWARD_TIME  15U

/* =========================================
   2. FSM 狀態定義
   ========================================= */

typedef enum
{
    STATE_WAIT,
    STATE_STUDY,
    STATE_BREAK,
    STATE_PAUSED,
    STATE_DONE
} FSM_State;

/* =========================================
   3. FSM 使用的變數
   ========================================= */


FSM_State state = STATE_WAIT;


FSM_State previousState = STATE_WAIT;

uint16_t totalRounds = 0;


uint16_t currentRound = 0;


uint8_t studyPaused = 0;


uint32_t phaseStart = 0;


uint32_t phaseDuration = 0;


uint32_t remainingTime = 0;

/* =========================================
   4. 中斷旗標
   ========================================= */

// PC13 按鈕中斷旗標
volatile uint8_t buttonFlag = 0;

// TIM6 週期中斷旗標
volatile uint8_t timerFlag = 0;

// 記錄上次有效按鈕時間，防止彈跳
volatile uint32_t lastButtonTime = 0;

// 是否曾經接受過有效按鈕事件
volatile uint8_t buttonInitialized = 0;


uint8_t rxChar = 0;

volatile uint8_t rxStatus = 0;

volatile uint16_t rxRounds = 0;

uint16_t rxNumber = 0;

uint8_t rxDigits = 0;

uint8_t rxInvalid = 0;

char txBuffer[128];

/* USER CODE END PV */


/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM6_Init(void);

/* USER CODE BEGIN PFP */

void UART_Send(const char *msg);

void PWM_Set(uint32_t channel, uint16_t brightness);

void StartStudy(void);

void StartBreak(void);

void HandleButton(void);

void FSM_Update(void);


void LED_Update(void);

/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();

  /* USER CODE BEGIN 2 */

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);

  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

  PWM_Set(TIM_CHANNEL_1, 0);
  PWM_Set(TIM_CHANNEL_2, 0);


  HAL_TIM_Base_Start_IT(&htim6);

  HAL_UART_Receive_IT(&hlpuart1, &rxChar, 1);

  UART_Send("\r\n========================\r\n");
  UART_Send(" STM32 Study Timer\r\n");
  UART_Send("========================\r\n");

  UART_Send("Study : 20 seconds\r\n");
  UART_Send("Break : 10 seconds\r\n");
  UART_Send("Reward: 15 seconds\r\n");

  UART_Send("\r\nEnter rounds (1-99):\r\n");

  /* USER CODE END 2 */


  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	     if (rxStatus != 0)
	     {
	         uint8_t result;
	         uint16_t inputRounds;


	         uint32_t primask = __get_PRIMASK();
	         __disable_irq();

	         result = rxStatus;
	         inputRounds = rxRounds;
	         rxStatus = 0;

	         __set_PRIMASK(primask);


	         if (result == 1)
	         {

	             if (state == STATE_WAIT ||
	                 state == STATE_DONE)
	             {
	                 totalRounds = inputRounds;
	                 currentRound = 0;
	                 studyPaused = 0;


	                 state = STATE_WAIT;


	                 PWM_Set(TIM_CHANNEL_1, 0);
	                 PWM_Set(TIM_CHANNEL_2, 0);


	                 snprintf(txBuffer, sizeof(txBuffer),
	                          "\r\nRounds set: %u\r\n",
	                          (unsigned int)totalRounds);

	                 UART_Send(txBuffer);
	                 UART_Send("Press USER to start.\r\n");
	             }
	             else
	             {

	                 UART_Send("Timer is active. Input ignored.\r\n");
	             }
	         }
	         else
	         {

	             UART_Send("Invalid input! Enter 1-99.\r\n");
	         }
	     }


	     if (buttonFlag == 1)
	     {
	         buttonFlag = 0;


	         HandleButton();
	     }



	     if (timerFlag == 1)
	     {

	         timerFlag = 0;


	         FSM_Update();


	         LED_Update();
	     }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 169;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 16999;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : USER_BUTTON_Pin */
  GPIO_InitStruct.Pin = USER_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(USER_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* =========================================
   函式 1：UART 傳送文字
   ========================================= */

void UART_Send(const char *msg)
{


    HAL_UART_Transmit(
        &hlpuart1,
        (uint8_t *)msg,
        (uint16_t)strlen(msg),
        1000
    );
}


void PWM_Set(uint32_t channel, uint16_t brightness)
{

    if (brightness > 1000U)
    {
        brightness = 1000U;
    }

    uint32_t period =
        __HAL_TIM_GET_AUTORELOAD(&htim3) + 1U;


    uint32_t compare =
        (period * brightness) / 1000U;


    __HAL_TIM_SET_COMPARE(&htim3, channel, compare);
}

/* =========================================
   函式 3：開始新的一輪讀書
   ========================================= */

void StartStudy(void)
{
    // FSM 切換至讀書狀態
    state = STATE_STUDY;

    // 讀書時間為 20 秒
    // HAL_GetTick() 使用毫秒
    // 所以需要乘上 1000
    phaseDuration = STUDY_TIME * 1000U;

    // 記錄現在的開始時間
    phaseStart = HAL_GetTick();

    // 新的一輪尚未暫停
    // 因此有機會得到獎勵
    studyPaused = 0;

    // 讀書期間藍燈恆亮
    PWM_Set(TIM_CHANNEL_1, 1000);

    // 黃燈關閉
    PWM_Set(TIM_CHANNEL_2, 0);

    // 顯示目前讀書輪數
    snprintf(txBuffer, sizeof(txBuffer),
             "\r\nStudy Round %u/%u\r\n",
             (unsigned int)(currentRound + 1),
             (unsigned int)totalRounds);

    UART_Send(txBuffer);
    UART_Send("Study started: 20 sec\r\n");
}

/* =========================================
   函式 4：開始休息並判斷獎勵
   ========================================= */

void StartBreak(void)
{
    // FSM 切換至休息狀態
    state = STATE_BREAK;

    // 記錄休息開始時間
    phaseStart = HAL_GetTick();

    // 判斷剛剛的讀書是否曾經暫停
    if (studyPaused == 0)
    {
        // 沒有暫停，可以獲得獎勵
        // 獎勵休息 15 秒
        phaseDuration = REWARD_TIME * 1000U;

        UART_Send("\r\nReward unlocked!\r\n");
        UART_Send("Break: 15 sec\r\n");
    }
    else
    {
        // 讀書期間曾經暫停
        // 只能正常休息 10 秒
        phaseDuration = BREAK_TIME * 1000U;

        UART_Send("\r\nNormal break\r\n");
        UART_Send("Break: 10 sec\r\n");
    }

    // 關閉黃色 LED
    PWM_Set(TIM_CHANNEL_2, 0);

    // 藍色呼吸燈由 LED_Update() 控制
}

/* =========================================
   函式 5：處理 USER 按鈕
   ========================================= */

void HandleButton(void)
{
    // 取得目前的毫秒時間
    uint32_t now = HAL_GetTick();

    /* ===== 情況 1：等待開始 ===== */
    if (state == STATE_WAIT)
    {
        // 確認使用者已設定輪數
        if (totalRounds == 0)
        {
            UART_Send("Please enter rounds first.\r\n");
            return;
        }

        // 開始第一輪讀書
        StartStudy();
    }

    /* ===== 情況 2：正在讀書或休息 ===== */
    else if (state == STATE_STUDY ||
             state == STATE_BREAK)
    {
        // 記住目前狀態
        // 方便恢復時回到原狀態
        previousState = state;

        // 計算本階段已經經過的時間
        uint32_t elapsed = now - phaseStart;

        // 計算暫停時的剩餘時間
        if (elapsed >= phaseDuration)
        {
            remainingTime = 0;
        }
        else
        {
            remainingTime = phaseDuration - elapsed;
        }

        // 如果在讀書狀態暫停
        if (state == STATE_STUDY)
        {
            // 記錄曾經暫停，失去本輪獎勵
            studyPaused = 1;
        }

        // 進入暫停狀態
        state = STATE_PAUSED;

        // 暫停時關閉兩顆 LED
        PWM_Set(TIM_CHANNEL_1, 0);
        PWM_Set(TIM_CHANNEL_2, 0);

        // 顯示剩餘秒數
        snprintf(txBuffer, sizeof(txBuffer),
                 "Paused. Remaining: %lu sec\r\n",
                 (unsigned long)
                 ((remainingTime + 999U) / 1000U));

        UART_Send(txBuffer);
    }

    /* ===== 情況 3：恢復計時 ===== */
    else if (state == STATE_PAUSED)
    {
        // 回到暫停前的狀態
        state = previousState;

        // 從現在開始計算剩餘的時間
        phaseStart = now;

        // 只需執行尚未完成的時間
        phaseDuration = remainingTime;

        UART_Send("Timer resumed!\r\n");

        // 立即恢復燈光
        LED_Update();
    }

    /* ===== 情況 4：所有輪數已完成 ===== */
    else if (state == STATE_DONE)
    {
        UART_Send("Enter new rounds to restart.\r\n");
    }
}

/* =========================================
   函式 6：FSM 狀態更新
   ========================================= */

void FSM_Update(void)
{
    // 取得現在時間，單位為毫秒
    uint32_t now = HAL_GetTick();

    /* ===== 讀書狀態 ===== */
    if (state == STATE_STUDY)
    {
        // 判斷讀書時間是否到達
        if ((now - phaseStart) >= phaseDuration)
        {
            // 一輪讀書完成
            currentRound++;

            UART_Send("Study completed!\r\n");

            // 判斷是否完成全部輪數
            if (currentRound >= totalRounds)
            {
                // 所有輪數完成
                state = STATE_DONE;

                // 關閉藍色 LED
                PWM_Set(TIM_CHANNEL_1, 0);

                // 顯示完成訊息
                UART_Send("\r\n====================\r\n");
                UART_Send("ALL ROUNDS COMPLETED!\r\n");
                UART_Send("Well done!\r\n");
                UART_Send("====================\r\n");

                UART_Send("Enter new rounds to restart.\r\n");
            }
            else
            {
                // 尚未完成全部輪數
                // 進入休息，並判斷是否獲得獎勵
                StartBreak();
            }
        }
    }

    /* ===== 休息狀態 ===== */
    else if (state == STATE_BREAK)
    {
        // 判斷休息時間是否結束
        if ((now - phaseStart) >= phaseDuration)
        {
            UART_Send("Break finished!\r\n");

            // 自動開始下一輪讀書
            StartStudy();
        }
    }

    /* ===== 暫停狀態 ===== */
    else if (state == STATE_PAUSED)
    {
        // 暫停時不更新倒數
        // 等待使用者按下 USER 恢復
    }

    /* ===== 等待或完成狀態 ===== */
    else
    {
        // 不進行讀書或休息計時
    }
}

/* =========================================
   函式 7：根據 FSM 狀態控制 LED
   ========================================= */

void LED_Update(void)
{
    uint32_t now = HAL_GetTick();

    /* ===== STUDY：藍燈恆亮 ===== */
    if (state == STATE_STUDY)
    {
        // 藍色 LED 100% 亮度
        PWM_Set(TIM_CHANNEL_1, 1000);

        // 黃色 LED 關閉
        PWM_Set(TIM_CHANNEL_2, 0);
    }

    /* ===== BREAK：藍色呼吸燈 ===== */
    else if (state == STATE_BREAK)
    {
        // 呼吸週期為 2000 ms
        // 前 1 秒變亮、後 1 秒變暗
        uint32_t t = (now - phaseStart) % 2000U;

        // 計算 PWM 亮度
        uint16_t brightness;

        if (t < 1000U)
        {
            // 第一階段：漸亮
            brightness =
                50U + (950U * t) / 1000U;
        }
        else
        {
            // 第二階段：漸暗
            brightness =
                50U + (950U * (2000U - t)) / 1000U;
        }

        // 使用 TIM3 CH1 改變藍燈亮度
        PWM_Set(TIM_CHANNEL_1, brightness);

        // 黃燈關閉
        PWM_Set(TIM_CHANNEL_2, 0);
    }

    /* ===== DONE：黃色 LED 閃爍 ===== */
    else if (state == STATE_DONE)
    {
        // 藍燈關閉
        PWM_Set(TIM_CHANNEL_1, 0);

        // 每 250 ms 切換一次黃燈亮滅
        if ((now / 250U) % 2U == 0U)
        {
            PWM_Set(TIM_CHANNEL_2, 1000);
        }
        else
        {
            PWM_Set(TIM_CHANNEL_2, 0);
        }
    }

    /* ===== WAIT / PAUSED ===== */
    else
    {
        // 等待或暫停時兩顆 LED 都關閉
        PWM_Set(TIM_CHANNEL_1, 0);
        PWM_Set(TIM_CHANNEL_2, 0);
    }
}

/* =========================================
   中斷 1：PC13 USER 按鈕
   ========================================= */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // 確認中斷來源是 PC13
    if (GPIO_Pin == GPIO_PIN_13)
    {
        // 取得按下按鈕時的時間
        uint32_t now = HAL_GetTick();

        // 第一次按下直接接受
        // 之後兩次有效按下需至少間隔 180 ms
        if (buttonInitialized == 0 ||
            (now - lastButtonTime) >= 180U)
        {
            // 記錄本次按鈕時間
            lastButtonTime = now;

            // 標記已經接收過按鈕事件
            buttonInitialized = 1;

            // 通知主程式處理按鈕
            buttonFlag = 1;
        }
    }
}

/* =========================================
   中斷 2：TIM6 計時器
   ========================================= */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    // 判斷是不是 TIM6 產生的中斷
    if (htim->Instance == TIM6)
    {
        // 通知主程式更新狀態機與 LED
        timerFlag = 1;
    }
}

/* =========================================
   中斷 3：LPUART1 接收字元
   功能：接收 1~99 的讀書輪數
   ========================================= */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // 確認是 LPUART1 收到字元
    if (huart->Instance == LPUART1)
    {
        /* ===== 收到 Enter ===== */
        if (rxChar == '\r' || rxChar == '\n')
        {
            // 有輸入內容才處理
            if (rxDigits > 0 || rxInvalid)
            {
                // 避免覆蓋主程式尚未處理的結果
                if (rxStatus == 0)
                {
                    // 檢查輸入是否為 1~99
                    if (rxInvalid == 0 &&
                        rxNumber >= 1 &&
                        rxNumber <= 99)
                    {
                        // 有效輸入
                        rxRounds = rxNumber;
                        rxStatus = 1;
                    }
                    else
                    {
                        // 無效輸入
                        rxStatus = 2;
                    }
                }
            }

            // 清除暫存資料
            rxNumber = 0;
            rxDigits = 0;
            rxInvalid = 0;
        }

        /* ===== 收到數字或其他字元 ===== */
        else
        {
            // 判斷字元是否為 0~9
            if (rxChar >= '0' && rxChar <= '9')
            {
                // 只允許輸入最多兩位數
                if (rxDigits < 2 && rxInvalid == 0)
                {
                    // 將字元轉換為數值
                    rxNumber =
                        rxNumber * 10U + (rxChar - '0');

                    // 數字位數加 1
                    rxDigits++;
                }
                else
                {
                    // 超過兩位數
                    rxInvalid = 1;
                }
            }
            else
            {
                // 收到非數字字元
                rxInvalid = 1;
            }
        }

        // 再次啟動 UART 接收中斷
        // 等待下一個字元
        HAL_UART_Receive_IT(&hlpuart1, &rxChar, 1);
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
