/**
  ******************************************************************************
  * @file           : light_sensor_example.c
  * @brief          : 光敏传感器驱动使用示例
  * @author         : Antigravity AI
  * @version        : V1.0.0
  * @date           : 2025-12-17
  ******************************************************************************
  * @attention
  *
  * 本文件提供光敏传感器驱动的使用示例代码
  * 不参与编译，仅供参考
  *
  ******************************************************************************
  */

#if 0  /* 示例代码，默认不编译 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "light_sensor.h"
#include <stdio.h>
#include <string.h>

/* Private variables ---------------------------------------------------------*/
static char print_buffer[128];

/* Private function prototypes -----------------------------------------------*/
static void PrintLightSensorData(void);

/* Example functions ---------------------------------------------------------*/

/**
  * ============================================================================
  * @brief  示例1: 基本使用 - DMA自动采集模式 (推荐)
  * ============================================================================
  * 
  * 将以下代码添加到 main.c 中:
  * 
  * 1. 在 main.c 头部添加:
  *    #include "light_sensor.h"
  * 
  * 2. 在 main() 函数的初始化部分 (USER CODE BEGIN 2) 添加:
  *    LightSensor_Init();
  *    LightSensor_StartDMA();
  * 
  * 3. 在 while(1) 循环中 (USER CODE BEGIN 3) 添加:
  *    // 获取光敏传感器数据
  *    uint16_t adc_value = LightSensor_GetValue();
  *    float voltage = LightSensor_GetVoltage();
  *    uint8_t percent = LightSensor_GetPercent();
  *    LightSensor_LightLevel_t level = LightSensor_GetLightLevel();
  *    
  *    // 打印数据
  *    sprintf(print_buffer, "ADC: %d, Voltage: %.2fV, Light: %d%%, Level: %s\r\n",
  *            adc_value, voltage, percent, LightSensor_GetLevelString(level));
  *    HAL_UART_Transmit(&huart1, (uint8_t*)print_buffer, strlen(print_buffer), 100);
  *    
  *    HAL_Delay(1000);  // 每秒读取一次
  */
void LightSensor_Example_DMA(void)
{
    /* 初始化光敏传感器 */
    if (LightSensor_Init() != LIGHT_SENSOR_OK) {
        sprintf(print_buffer, "LightSensor init failed!\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)print_buffer, strlen(print_buffer), 100);
        return;
    }
    
    /* 启动DMA自动采集 */
    if (LightSensor_StartDMA() != LIGHT_SENSOR_OK) {
        sprintf(print_buffer, "LightSensor DMA start failed!\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)print_buffer, strlen(print_buffer), 100);
        return;
    }
    
    sprintf(print_buffer, "LightSensor DMA started successfully!\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)print_buffer, strlen(print_buffer), 100);
    
    /* 主循环中读取数据 */
    while (1) {
        PrintLightSensorData();
        HAL_Delay(1000);
    }
}

/**
  * ============================================================================
  * @brief  示例2: 轮询模式 (适用于低功耗场景)
  * ============================================================================
  */
void LightSensor_Example_Polling(void)
{
    /* 初始化光敏传感器 */
    LightSensor_Init();
    
    while (1) {
        /* 手动触发一次采集 */
        if (LightSensor_ReadPolling(100) == LIGHT_SENSOR_OK) {
            PrintLightSensorData();
        } else {
            sprintf(print_buffer, "Read failed!\r\n");
            HAL_UART_Transmit(&huart1, (uint8_t*)print_buffer, strlen(print_buffer), 100);
        }
        
        HAL_Delay(1000);
    }
}

/**
  * ============================================================================
  * @brief  示例3: 光照阈值控制LED
  * ============================================================================
  */
void LightSensor_Example_LED_Control(void)
{
    LightSensor_Init();
    LightSensor_StartDMA();
    
    while (1) {
        LightSensor_LightLevel_t level = LightSensor_GetLightLevel();
        
        switch (level) {
            case LIGHT_LEVEL_DARK:
            case LIGHT_LEVEL_DIM:
                /* 暗环境，点亮LED */
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                break;
            
            case LIGHT_LEVEL_NORMAL:
            case LIGHT_LEVEL_BRIGHT:
            case LIGHT_LEVEL_VERY_BRIGHT:
                /* 亮环境，关闭LED */
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
                break;
        }
        
        HAL_Delay(100);
    }
}

/**
  * ============================================================================
  * @brief  示例4: 带滤波的稳定读取
  * ============================================================================
  */
void LightSensor_Example_Filtered(void)
{
    LightSensor_Init();
    LightSensor_StartDMA();
    
    /* 等待DMA缓冲区填满 */
    HAL_Delay(100);
    
    while (1) {
        /* 获取滤波后的值 (多次采样平均) */
        uint16_t filtered = LightSensor_GetFilteredValue();
        uint32_t voltage_mv = LightSensor_GetVoltage_mV();
        
        sprintf(print_buffer, "Filtered ADC: %d, Voltage: %lu mV\r\n", filtered, voltage_mv);
        HAL_UART_Transmit(&huart1, (uint8_t*)print_buffer, strlen(print_buffer), 100);
        
        HAL_Delay(500);
    }
}

/**
  * ============================================================================
  * @brief  打印光敏传感器数据
  * ============================================================================
  */
static void PrintLightSensorData(void)
{
    uint16_t adc_value = LightSensor_GetValue();
    float voltage = LightSensor_GetVoltage();
    uint8_t percent = LightSensor_GetPercent();
    LightSensor_LightLevel_t level = LightSensor_GetLightLevel();
    
    sprintf(print_buffer, 
            "==============================\r\n"
            "Light Sensor Data:\r\n"
            "  ADC Value: %d\r\n"
            "  Voltage:   %.2f V\r\n"
            "  Light:     %d %%\r\n"
            "  Level:     %s\r\n"
            "==============================\r\n",
            adc_value, voltage, percent, LightSensor_GetLevelString(level));
    
    HAL_UART_Transmit(&huart1, (uint8_t*)print_buffer, strlen(print_buffer), 100);
}

/**
  * ============================================================================
  * @brief  在stm32f4xx_it.c中添加的回调 (可选，用于DMA完成通知)
  * ============================================================================
  * 
  * 如果需要在每次DMA传输完成时执行操作，可以在stm32f4xx_it.c中添加:
  * 
  * 1. 在文件头部添加:
  *    #include "light_sensor.h"
  * 
  * 2. 添加或修改 HAL_ADC_ConvCpltCallback 函数:
  * 
  *    void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
  *    {
  *        LightSensor_ConvCpltCallback(hadc);
  *    }
  */

#endif /* 示例代码结束 */

/* End of file ---------------------------------------------------------------*/
