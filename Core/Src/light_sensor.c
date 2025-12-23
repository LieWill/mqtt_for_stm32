/**
  ******************************************************************************
  * @file           : light_sensor.c
  * @brief          : 光敏传感器驱动源文件 (轮询模式)
  * @author         : Antigravity AI
  * @version        : V1.1.0
  * @date           : 2025-12-17
  ******************************************************************************
  * @attention
  *
  * 本驱动使用ADC3通道5 (PF7引脚) 采集光敏传感器模拟信号
  * 使用轮询模式读取，简单可靠
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "light_sensor.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* 光敏传感器句柄实例 */
LightSensor_Handle_t lightSensor = {0};

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化光敏传感器驱动
  */
LightSensor_Status_t LightSensor_Init(void)
{
    /* 绑定ADC句柄 */
    lightSensor.hadc = &hadc3;
    
    /* 初始化状态变量 */
    lightSensor.current_value = 0;
    lightSensor.filtered_value = 0;
    lightSensor.last_update_tick = 0;
    lightSensor.dma_running = 0;
    lightSensor.conversion_complete = 0;
    
    /* 标记为已初始化 */
    lightSensor.is_initialized = 1;
    
    LightSensor_DebugPrint("[LightSensor] Initialized\r\n");
    
    return LIGHT_SENSOR_OK;
}

/**
  * @brief  读取光敏传感器ADC值 (轮询模式)
  */
LightSensor_Status_t LightSensor_Read(void)
{
    HAL_StatusTypeDef status;
    
    if (!lightSensor.is_initialized) {
        return LIGHT_SENSOR_NOT_INITIALIZED;
    }
    
    /* 启动ADC转换 */
    status = HAL_ADC_Start(lightSensor.hadc);
    if (status != HAL_OK) {
        return LIGHT_SENSOR_ERROR;
    }
    
    /* 等待转换完成 */
    status = HAL_ADC_PollForConversion(lightSensor.hadc, 100);
    if (status != HAL_OK) {
        HAL_ADC_Stop(lightSensor.hadc);
        return LIGHT_SENSOR_TIMEOUT;
    }
    
    /* 读取转换结果 */
    lightSensor.current_value = (uint16_t)HAL_ADC_GetValue(lightSensor.hadc);
    lightSensor.filtered_value = lightSensor.current_value;
    lightSensor.last_update_tick = HAL_GetTick();
    
    /* 停止ADC */
    HAL_ADC_Stop(lightSensor.hadc);
    
    return LIGHT_SENSOR_OK;
}

/**
  * @brief  获取当前ADC原始值 (自动读取一次)
  */
uint16_t LightSensor_GetValue(void)
{
    /* 自动读取一次 */
    LightSensor_Read();
    return lightSensor.current_value;
}

/**
  * @brief  获取光照百分比 (0-100%)
  */
uint8_t LightSensor_GetPercent(void)
{
    uint16_t adc_value = lightSensor.current_value;
    return (uint8_t)((uint32_t)adc_value * 100 / LIGHT_SENSOR_ADC_MAX);
}

/**
  * @brief  获取电压值 (mV)
  */
uint32_t LightSensor_GetVoltage_mV(void)
{
    uint16_t adc_value = lightSensor.current_value;
    return (uint32_t)adc_value * LIGHT_SENSOR_VREF_MV / LIGHT_SENSOR_ADC_MAX;
}

/**
  * @brief  获取光照等级
  */
LightSensor_LightLevel_t LightSensor_GetLightLevel(void)
{
    uint16_t adc_value = lightSensor.current_value;
    
    if (adc_value < LIGHT_LEVEL_DARK_THRESHOLD) {
        return LIGHT_LEVEL_DARK;
    } else if (adc_value < LIGHT_LEVEL_DIM_THRESHOLD) {
        return LIGHT_LEVEL_DIM;
    } else if (adc_value < LIGHT_LEVEL_NORMAL_THRESHOLD) {
        return LIGHT_LEVEL_NORMAL;
    } else if (adc_value < LIGHT_LEVEL_BRIGHT_THRESHOLD) {
        return LIGHT_LEVEL_BRIGHT;
    } else {
        return LIGHT_LEVEL_VERY_BRIGHT;
    }
}

/**
  * @brief  获取光照等级字符串描述
  */
const char* LightSensor_GetLevelString(LightSensor_LightLevel_t level)
{
    switch (level) {
        case LIGHT_LEVEL_DARK:
            return "Dark";
        case LIGHT_LEVEL_DIM:
            return "Dim";
        case LIGHT_LEVEL_NORMAL:
            return "Normal";
        case LIGHT_LEVEL_BRIGHT:
            return "Bright";
        case LIGHT_LEVEL_VERY_BRIGHT:
            return "Very Bright";
        default:
            return "Unknown";
    }
}

/**
  * @brief  检查是否已初始化
  */
uint8_t LightSensor_IsInitialized(void)
{
    return lightSensor.is_initialized;
}

/**
  * @brief  调试打印函数 - 使用统一日志库
  */
void LightSensor_DebugPrint(const char *format, ...)
{
#if LIGHT_SENSOR_DEBUG_ENABLE
    char buffer[128];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    /* 使用LOG_Raw以兼容原有格式 */
    LOG_Raw("%s", buffer);
#else
    (void)format;
#endif
}

/* End of file ---------------------------------------------------------------*/
